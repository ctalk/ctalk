/* $Id: wregex.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015, 2017-2018
    Robert Kiesling,  rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "regex.h"

typedef struct {
  int start, length;
  char *text;
} MATCHREC;

MATCHREC matches[MAXARGS] = {{0, 0, NULL},};
int match_ptr = 0;

MATCHREC brs[MAXARGS] = {{0, 0, NULL},};
int br_ptr = 0;

/* 
   All of the regex metacharacters that we use.  Parentheses are not
   metacharacters unless they surround a matching regex, in which case
   they are considered to create a backreference.
*/

typedef struct {
  char *str;
  int attr;
  int matchstart, matchend;
} TOKEN;

typedef enum {
  zero_length_null = 0,
  zero_length_true,
  zero_length_false
} ZERO_LENGTH_STATE;

/* When recursing, if we call match_text_local, we can get
   non-contiguous matches as we scan over the remainder of then input,
   as with re_bar.  If the handler, like re_star, checks the rest of
   the input itself, then the matches after the recursion are
   contiguous */
typedef enum {
  recurse_state_null = 0,
  recurse_state_contiguous,
  recurse_state_non_contiguous
} RECURSE_STATE;

static TOKEN toks[MAXARGS];
static int n_toks = 0;

static int match_text_internal (char *, char *,	long long int *);
static int match_text_anchored (char *, char *,	long long int *);

static int lastmatchlength = 0;

static MESSAGE *re_messages[P_MESSAGES+1]; /* Regex message stack.        */
static int re_message_ptr = P_MESSAGES;    /* Regex stack pointer.        */

static MESSAGE *pat_messages[P_MESSAGES+1]; /* Regex message stack.        */
static int pat_message_ptr = P_MESSAGES;    /* Regex stack pointer.        */

void init_matchrecs () {
  int i;
  if (match_ptr != 0) {
    for (i = 0; i <= match_ptr; ++i) {
      matches[i].start = matches[i].length = 0;
      if (matches[i].text) {
	__xfree (MEMADDR(matches[i].text));
	matches[i].text = NULL;
      }
    }
  }
  match_ptr = 0;
}

void init_brs () {
  int i;
  if (br_ptr != 0) {
    for (i = 0; i <= br_ptr; ++i) {
      brs[i].start = brs[i].length = 0;
      if (brs[i].text) {
	__xfree (MEMADDR(brs[i].text));
	brs[i].text = NULL;
      }
    }
    br_ptr = 0;
  }
}

static char record_separator = '\n';

void __ctalkSetRS (char c) {record_separator = c;}
char __ctalkGetRS (void) {return record_separator;}

static bool print_toks_and_matches = false;

void __ctalkMatchPrintToks (bool b) {
  print_toks_and_matches = b;
}

static struct _ma {
  int val;
  char label[MAXLABEL];
} meta_attrs[9] =  {{0, ""},
		    {1, "backreference start"},
		    {2, "backreference end"}, {0, ""},
		    {4, "character class"},
		    {0, ""}, {0, ""}, {0, ""},
		    {8, "literal character"}};

static void print_toks (char *text, int last_n_toks, bool subexpr) {
  int i;
  char matchbuf[MAXLABEL * 4];
  for (i = n_toks - last_n_toks; i < last_n_toks; ++i) {
    memset (matchbuf, 0, MAXLABEL * 4);
    strncpy (matchbuf, &text[toks[i].matchstart],
	     toks[i].matchend - toks[i].matchstart);
    if (subexpr)
      printf ("\t");
    printf ("TOK: %s\t\t(%s)\t\tMATCH: \"%s\"\n",
	    toks[i].str, meta_attrs[toks[i].attr].label, matchbuf);
  }
}

static int re_message_push (MESSAGE *m) {
  if (re_message_ptr == 0) {
    _warning (_("re_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("re_message_push %d. %s."), re_message_ptr, m -> name);
#endif
  re_messages[re_message_ptr--] = m;
  return re_message_ptr + 1;
}

static MESSAGE *re_message_pop (void) {
  MESSAGE *m;
  if (re_message_ptr == P_MESSAGES) {
    _warning (_("re_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("re_message_pop %d. %s."), re_message_ptr, 
	 re_messages[re_message_ptr+1] -> name);
#endif
  if (re_messages[re_message_ptr + 1] && 
      IS_MESSAGE(re_messages[re_message_ptr + 1])) {
    m = re_messages[re_message_ptr + 1];
    re_messages[++re_message_ptr] = NULL;
    return m;
  } else {
    re_messages[++re_message_ptr] = NULL;
    return NULL;
  }
}


static int pat_message_push (MESSAGE *m) {
  if (pat_message_ptr == 0) {
    _warning (_("pat_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("pat_message_push %d. %s."), pat_message_ptr, m -> name);
#endif
  pat_messages[pat_message_ptr--] = m;
  return pat_message_ptr + 1;
}

static MESSAGE *pat_message_pop (void) {
  MESSAGE *m;
  if (pat_message_ptr == P_MESSAGES) {
    _warning (_("pat_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("pat_message_pop %d. %s."), pat_message_ptr, 
	 pat_messages[pat_message_ptr+1] -> name);
#endif
  if (pat_messages[pat_message_ptr + 1] && 
      IS_MESSAGE(pat_messages[pat_message_ptr + 1])) {
    m = pat_messages[pat_message_ptr + 1];
    pat_messages[++pat_message_ptr] = NULL;
    return m;
  } else {
    pat_messages[++pat_message_ptr] = NULL;
    return NULL;
  }
}

int _get_re_message_ptr (void) {
  return re_message_ptr;
}

static void cleanup_toks (int l_n_toks) {
  int i;
  for (i = 0; i < l_n_toks; i++) {
    __xfree (MEMADDR(toks[n_toks - 1].str));
    toks[n_toks - 1].str = NULL;
    toks[n_toks - 1].attr = toks[n_toks - 1].matchstart =
      toks[n_toks - 1].matchend = 0;
    n_toks--;
  }
}

static int eos_idx (char *s) {
  char *c;
  c = strchr (s, '\0');
  return (c - s) * sizeof (char);
}

static void split_pattern (char *pat_in, char *pat_out) {
  int i, j;
  char delim;
  i = 0, j = 0;
  if (pat_in[0] == 'm') {
    i = 2;
    delim = pat_in[1];
  } else {
    delim = pat_in[0];
    i = 1;
  }
  while (pat_in[i]) {
    if (pat_in[i] == delim || pat_in[i] == 0) {
      pat_out[j] = 0;
      return;
    }
    pat_out[j++] = pat_in[i++];
  }
  pat_out[j] = 0;
}

static int tokenize_pattern (char *pattern) {

  int re_stack_start,
    re_stack_top,
    pat_stack_start,
    pat_stack_top,
    i, j, l_n_toks;
  MESSAGE *n;
  char pat_out[MAXMSG];

  re_stack_start = re_message_ptr;

  re_stack_top = re_tokenize (re_message_push, pattern);
  
  l_n_toks = 0;
  for (i = re_stack_start; i >= re_stack_top; i--) {
    if (M_TOK(re_messages[i]) == PATTERN) {
      split_pattern (re_messages[i] -> name, pat_out);
      pat_stack_start = pat_message_ptr;
      pat_stack_top = re_tokenize (pat_message_push, pat_out);
      for (j = pat_stack_start; j >= pat_stack_top; j--) {
	toks[n_toks].attr = pat_messages[j] -> attrs;
	toks[n_toks++].str = strdup (pat_messages[j] -> name);
	l_n_toks++;
      }
      for (j = pat_stack_top; j <= pat_stack_start; j++) {
	n = pat_message_pop ();
	delete_message(n);
      }
    } else {
      toks[n_toks].attr = re_messages[i] -> attrs;
      toks[n_toks++].str = strdup (re_messages[i] -> name);
      l_n_toks++;
    }
  }

  for (i = re_stack_top; i <= re_stack_start; i++) {
    n = re_message_pop ();
    delete_message(n);
  }

  return l_n_toks;
}

static void make_match_rec (char *text, int text_start, int match_end) {
  char buf[MAXMSG];
  matches[match_ptr].start = text_start;
  matches[match_ptr].length = match_end - text_start;
  memset (buf, 0, MAXMSG);
  strncpy (buf, &text[text_start], (match_end - text_start));
  matches[match_ptr].text = strdup(buf);
  ++match_ptr;
}

/* 
   It's tricky to figure out how a random previous metacharacter
   calculated its match end, so we don't record the length,
   but we can derive it from the start index and the
   collected backreference string later if needed.
*/
static void make_br_rec (char *text, int text_start) {
  brs[br_ptr].start = text_start;
  brs[br_ptr].text = strdup(text);
  ++br_ptr;
}

static void make_br_rec_fmt (char *text, int start, int length) {
  char buf[MAXMSG];
  memset (buf, 0, MAXMSG);
  strncpy (buf, &text[start], length);
  make_br_rec (buf, start);
}

static bool pattern_has_char_class (char *pat) {
  char *c;
  if ((c = strchr (pat, '\\')) != NULL) {
    ++c;
    if (IS_CHAR_CLASS_ABBREV(*c)) {
      return true;
    }
  }
  return false;
}

static bool re_exact_str (char *buf, char *pat) {
  while (*pat++  == *buf++) {
    if (*pat == '\0')
      return true;
    if (*buf == '\0')
      return false;
  }
  return false;
}

static bool re_dot_str (char *buf, char *pat) {
  while ((*pat == *buf++) || (*pat == '.')) {
    pat++;
    if (*pat == '\0')
      return true;
    if (*buf == '\0')
      return false;
  }
  return false;
}

static bool start_of_rec (char *text, int *matchend) {
  if (record_separator == '\0') {
    if (*matchend == 0)
      return true;
  } else {
    if (*matchend == 0) {
      return true;
    }
    if (text[(*matchend) - 1] == record_separator) {
      return true;
    }
  }
  return false;
}

static bool end_of_rec (char *text, int *matchend, int eos) {
  if (record_separator == 0) {
    if (*matchend == eos)
      return true;
  } else {
    if (*matchend == eos) {
      return true;
    }
    if (text[*matchend] == record_separator) {
      return true;
    }
  }
  return false;
}

static bool char_class_match (char class_id, char c) {
  switch (class_id)
    {
    case 'W': return isalpha ((int)c); break;
    case 'p': return ispunct ((int)c); break;
    case 'w': return isspace ((int)c); break;
    case 'd': return isdigit ((int)c); break;
    case 'x': return isxdigit ((int)c) || (c == 'x') || (c == 'X');
      break;
    case 'l': return isalnum ((int)c) || (c == '_'); break;
    }
  return false;
}

/* If a recursive parser saved a backreference, its position
   and size will show up show up with offsets measured
   from the start of the recurse, so then adjust them to the
   actual start of the text.
*/
static void re_bar_update_br (char *pat, char *text,
			      int start, int length) {
  char matchbuf[MAXMSG];
  if (strchr (pat, '(') && strchr (pat, ')')) {
    __xfree (MEMADDR(brs[br_ptr - 1].text));
    memset (matchbuf, 0, MAXMSG);
    strncpy (matchbuf, &text[start], length);
    brs[br_ptr - 1].text = strdup (matchbuf);
    brs[br_ptr - 1].start = start;
    brs[br_ptr - 1].length = length;
  }
}

/* because expressions like <char>* can match zero length,
   we return the longer match of the two alternate 
   expressions. */
static bool re_bar (char *text, int *matchend, TOKEN *tok) {
  char l_expr[MAXMSG];
  char r_expr[MAXMSG];
  int matchlength_r, matchlength_l, matchpos_r, matchpos_l;
  bool l_match = false, r_match = false;
  long long int bar_offsets[MAXARGS];
  char *bar_ptr;
  memset (l_expr, 0, MAXMSG);
  memset (r_expr, 0, MAXMSG);
  bar_ptr = strchr (tok -> str, '|');
  strncpy (l_expr, tok -> str, bar_ptr - tok -> str);
  strcpy (r_expr, bar_ptr + 1);
  match_text_anchored (l_expr, &text[*matchend], bar_offsets);
  matchlength_l = lastmatchlength;
  if (bar_offsets[0] != ERROR) {
    l_match = true;
    matchpos_l = bar_offsets[0];
  }

  match_text_anchored (r_expr, &text[*matchend], bar_offsets);
  matchlength_r = lastmatchlength;
  if (bar_offsets[0] != ERROR) {
    r_match = true;
    matchpos_r = bar_offsets[0];
  }

  /* The end of the match is the position + length of the match
     if only one term matches, or the position + length of
     the longer match if both terms match. */
  if (l_match) {
    if (r_match && (matchlength_r > matchlength_l)) {
      lastmatchlength = matchlength_r;
      (*matchend) += matchpos_r + lastmatchlength;
      re_bar_update_br (r_expr, text, *matchend - matchlength_r,
			matchlength_r);
    } else {
      lastmatchlength = matchlength_l;
      (*matchend) += matchpos_l + matchlength_l;
      re_bar_update_br (l_expr, text, *matchend - matchlength_l,
			matchlength_l);
    }
  } else if (r_match) {
    /* this could be redundant but we'll keep it here for now */
    if (l_match && (matchlength_l > matchlength_r)) {
      lastmatchlength = matchlength_l;
      (*matchend) += matchpos_l + lastmatchlength;
      re_bar_update_br (l_expr, text, *matchend - matchlength_l,
			matchlength_l);
    } else {
      lastmatchlength = matchlength_r;
      (*matchend) += matchpos_r + matchlength_r;
      re_bar_update_br (r_expr, text, *matchend - matchlength_r,
			matchlength_r);
    }
  } else {
    (*matchend) += lastmatchlength;
  }

  return (l_match | r_match);
}

static char *matching_br_lookback (char *br_ptr, char *buf_start) {
  char *c;
  int n_parens = 1;
  for (c = br_ptr - 1; c >= buf_start; c--) {
    if (*c == ')') {
      ++n_parens;
    } else if (*c == '(') {
      --n_parens;
      if (n_parens == 0) {
	return c;
      }
    }
  }
  return c;
}

/* 
 *  A '*' matches zero or more of the preceding characters,
 *  so this always returns true. (__ctalkMatchText () checks 
 *  for a '*' as the first character of a pattern).  The
 *  next_tok_n parameter checks if char c_1 matches the
 *  next pattern token, so we can check for a zero-length
 *  match.
 */
static bool re_star (char c, char *text, int *matchend,
		     int tok_n, int next_tok_n, int prev_tok_n,
		     RECURSE_STATE *recurse_state) {
  int i = *matchend;
  int prev_length;
  char c_1, c_prev, char_class_id, *pat_star, *matching_br_open,
    a_expr[MAXMSG], *text_eos;
  long long int star_offsets[MAXARGS];
  ZERO_LENGTH_STATE zero_length;
  
  if (c == '.') {

    /* check for a zero length match. */
    c_1 = text[*matchend];
    if (((*matchend - 1) >= 0) && (prev_tok_n >= 0) &&
	toks[next_tok_n].str) {
      /* Check if there is both a preceding and following token. */
      c_prev = text[(*matchend)-1];
      prev_length = strlen (toks[prev_tok_n].str);
      if ((c_1 == *toks[next_tok_n].str) &&
	  (c_prev == toks[prev_tok_n].str[prev_length - 1])) {
	return true;
      }
    } else if (((*matchend - 1) > 0) && (prev_tok_n >= 0) &&
	       !toks[next_tok_n].str) {
      /* Then check if there is only a preceding token. */
      c_prev = text[(*matchend)-1];
      prev_length = strlen (toks[prev_tok_n].str);
      if (c_prev == toks[prev_tok_n].str[prev_length-1]) {
	return true;
      }
    } else if (toks[next_tok_n].str && (prev_tok_n < 0)) {
      /* Then only a following token. */
      if (c_1 == *toks[next_tok_n].str)
	return true;
    }

    for (; text[i] && (text[i] == c_1); ++i)
      ;
  } else if (toks[tok_n].str[strlen (toks[tok_n].str) - 2] == ')') {
    zero_length = zero_length_null;
    *recurse_state = recurse_state_contiguous;
    pat_star = strchr (toks[tok_n].str, '*');
    text_eos = strchr (text, 0);
    matching_br_open = matching_br_lookback (pat_star - 1, toks[tok_n].str);
    memset (a_expr, 0, MAXMSG);
    strncpy (a_expr, matching_br_open + 1,
	     (pat_star - 1) - (matching_br_open + 1));
    while (&text[*matchend] < text_eos) {
      match_text_anchored (a_expr, &text[*matchend], star_offsets);
      if (star_offsets[0] == -1) {
	if (zero_length != zero_length_false) {
	  toks[tok_n].matchend = *matchend;
	  lastmatchlength = 0;
	} else {
	  lastmatchlength = toks[tok_n].matchend - toks[tok_n].matchstart;
	}
	if (zero_length == zero_length_false) {
	  make_br_rec_fmt (text, toks[tok_n].matchstart, lastmatchlength);
	  return true;
	} else {
	  /* we should just break off if we're in a sub-parser */
	  return false;
	}
      } else {
	*matchend += lastmatchlength;
	toks[tok_n].matchend = *matchend;
	zero_length = zero_length_false;
	lastmatchlength = toks[tok_n].matchend - toks[tok_n].matchstart;
      }
    }
    make_br_rec_fmt (text, toks[tok_n].matchstart, lastmatchlength);
    return true;
  } else if (toks[tok_n].attr & META_CHAR_CLASS) {
      char_class_id = *toks[tok_n].str;
      *matchend = i;
      for (; text[i] && (char_class_match (char_class_id, text[i])); ++i)
	++(*matchend);
      return true;
  } else {
    for (; text[i] && (text[i] == c); ++i) {
      if (text[i+1] == 0) {
	*matchend = i;
	return true;
      }
    }
  }
  
  *matchend = i;
  return true;
}

static bool re_plus (char c, char *text, int *matchend,
		     int tok_n, RECURSE_STATE *recurse_state) {
  int i;
  int prev_length;
  char c_1, c_prev, char_class_id, *pat_plus, *text_eos,
    *matching_br_open, a_expr[MAXMSG];
  ZERO_LENGTH_STATE zero_length;
  long long int plus_offsets[MAXARGS];
  
  if (toks[tok_n].attr & META_CHAR_CLASS) {
    char_class_id = *toks[tok_n].str;
    for (i = *matchend; text[i] &&
	   (char_class_match (char_class_id, text[i])); ++i)
      ++(*matchend);
    return true;
  } else if (toks[tok_n].str[strlen (toks[tok_n].str) - 2] == ')') {
    zero_length = zero_length_null;
    *recurse_state = recurse_state_contiguous;
    pat_plus = strchr (toks[tok_n].str, '+');
    text_eos = strchr (text, 0);
    matching_br_open = matching_br_lookback (pat_plus - 1, toks[tok_n].str);
    memset (a_expr, 0, MAXMSG);
    strncpy (a_expr, matching_br_open + 1,
	     (pat_plus - 1) - (matching_br_open + 1));
    while (&text[*matchend] < text_eos) {
      match_text_anchored (a_expr, &text[*matchend], plus_offsets);
      if (plus_offsets[0] == -1) {
	if (zero_length != zero_length_false) {
	  toks[tok_n].matchend = *matchend;
	  lastmatchlength = 0;
	} else {
	  lastmatchlength = toks[tok_n].matchend - toks[tok_n].matchstart;
	}
	if (zero_length == zero_length_false) {
	  make_br_rec_fmt (text, toks[tok_n].matchstart, lastmatchlength);
	  return true;
	} else {
	  /* again, we should just break off if we're in a sub-parser */
	  return false;
	}
      } else {
	*matchend += lastmatchlength;
	toks[tok_n].matchend = *matchend;
	zero_length = zero_length_false;
	lastmatchlength = toks[tok_n].matchend - toks[tok_n].matchstart;
      }
    }
    make_br_rec_fmt (text, toks[tok_n].matchstart, lastmatchlength);
    return true;
  } else {
    for (i = *matchend; text[i] && (text[i] == c); ++i)
      ++(*matchend);
    return true;
  }
  
  return true;
}

static bool re_question (char c, char *text, int *matchend,
			 int tok_n, RECURSE_STATE *recurse_state) {
  char *pat_question, *matching_br_open,
    a_expr[MAXMSG];
  long long int question_offsets[MAXARGS];
  ZERO_LENGTH_STATE zero_length;
  if (toks[tok_n].attr & META_CHAR_CLASS) {
    if (char_class_match (*toks[tok_n].str, *text)) {
      ++(*matchend);
    }
  } else if (toks[tok_n].str[strlen (toks[tok_n].str) - 2] == ')') {
    zero_length = zero_length_null;
    pat_question = strchr (toks[tok_n].str, '?');
    zero_length = zero_length_null;
    matching_br_open = matching_br_lookback (pat_question - 1, toks[tok_n].str);
    memset (a_expr, 0, MAXMSG);
    strncpy (a_expr, matching_br_open + 1,
	     (pat_question - 1) - (matching_br_open + 1));
    match_text_anchored (a_expr, &text[*matchend], question_offsets);
    if (question_offsets[0] == -1) {
      if (zero_length == zero_length_true) {
	toks[tok_n].matchend = *matchend;
	lastmatchlength = 0;
      } else {
	lastmatchlength = toks[tok_n].matchend - toks[tok_n].matchstart;
      }
      if (zero_length == zero_length_false) {
	make_br_rec_fmt (text, toks[tok_n].matchstart, lastmatchlength);
	return true;
      } else {
	/* again, we should just break off if we're in a sub-parser */
	return false;
      }
    } else {
      *matchend += lastmatchlength;
      toks[tok_n].matchend = *matchend;
      zero_length = zero_length_false;
      lastmatchlength = toks[tok_n].matchend - toks[tok_n].matchstart;
      if (zero_length == zero_length_false) {
	make_br_rec_fmt (text, toks[tok_n].matchstart, lastmatchlength);
	return true;
      }
    }
  } else if (*text == c) {
    ++(*matchend);
  }
  return true;
}

static void match_toks (char *text, int text_start, long long int *offsets,
			int *matchend, int eos, int *n_matches,
			int l_n_toks, RECURSE_STATE *recurse_state) {

  int i, i_2;
  char *t_seg;
  bool in_br = false,
    br_done = false;
  int br_start;
  char br_buf[MAXMSG];
  int matchstart;
  int frame_start = n_toks - l_n_toks;

  t_seg = &text[text_start];
  *matchend = text_start;
  *recurse_state = recurse_state_null;

  for (i = frame_start; (i < n_toks) && (*matchend < eos); ++i) {

    toks[i].matchstart = matchstart = *matchend;
    
    /* TODO - Save the results of the previous metacharacter scan. */
    /* check for the '|' metacharacter first */
    if (strchr (toks[i].str, '|')) {
      if (!(toks[i].attr & META_CHAR_LITERAL_ESC)) {
	re_bar (text, matchend, &toks[i]);
	toks[i].matchstart = *matchend - lastmatchlength;
	toks[i].matchend = *matchend;
	t_seg = &text[*matchend];
	if (in_br) {
	  strncat (br_buf, &text[matchstart], (*matchend - matchstart));
	}
	*recurse_state = recurse_state_contiguous;
      } else {
	if (*t_seg == *toks[i].str) {
	  lastmatchlength = 1;
	  ++(*matchend);
	  toks[i].matchend = *matchend;
	  if (in_br) {
	    strncat (br_buf, t_seg, 1);
	  }
	  t_seg = &text[*matchend];
	} else {
	  toks[i].matchend = *matchend = text_start;
	  return;
	}
      }
    } else if (strchr (toks[i].str, '*')) {
      if (!(toks[i].attr & META_CHAR_LITERAL_ESC)) {
	/* A '*' matches zero or more characters, so re_star () is
	   always successful unless it is the first character. */
	/* Passing "text" is correct here. */
	if (re_star (*toks[i].str, text, matchend, i, i+1, i-1,
		      recurse_state)) {
	  t_seg = &text[*matchend];
	  if (*recurse_state == recurse_state_null) {
	    lastmatchlength = (*matchend - matchstart);
	  } else {
	    lastmatchlength += (*matchend - matchstart);
	  }
	  toks[i].matchend = *matchend;
	  if (in_br) {
	    strncat (br_buf, &text[matchstart], (*matchend - matchstart));
	  }
	} else {
	  /* a false return means we made a recursive call that
	     resulted in a zero length match */
	  return;
	}
      } else {
	if (*t_seg == *toks[i].str) {
	  lastmatchlength = 1;
	  ++(*matchend);
	  toks[i].matchend = *matchend;
	  if (in_br) {
	    strncat (br_buf, t_seg, 1);
	  }
	  t_seg = &text[*matchend];
	} else {
	  toks[i].matchend = *matchend = text_start;
	  return;
	}
      }
    } else if (strchr (toks[i].str, '?')) {
      if (!(toks[i].attr & META_CHAR_LITERAL_ESC)) {
	re_question (*toks[i].str, t_seg, matchend, i, recurse_state);
	t_seg = &text[*matchend];
	lastmatchlength += (*matchend - matchstart);
	toks[i].matchend = *matchend;
	if (in_br) {
	  strncat (br_buf, &text[matchstart], (*matchend - matchstart));
	}
	if (lastmatchlength == 0)
	  continue;
      } else {
	if (*t_seg == *toks[i].str) {
	  lastmatchlength = 1;
	  ++(*matchend);
	  toks[i].matchend = *matchend;
	  if (in_br) {
	    strncat (br_buf, t_seg, 1);
	  }
	  t_seg = &text[*matchend];
	} else {
	  toks[i].matchend = *matchend = text_start;
	  return;
	}
      }
    } else if (strchr (toks[i].str, '+')) {
      if (!(toks[i].attr & META_CHAR_LITERAL_ESC)) {
	re_plus (*toks[i].str, text, matchend, i, recurse_state);
	t_seg = &text[*matchend];
	lastmatchlength = (*matchend - matchstart);
	toks[i].matchend = *matchend;
	if (in_br) {
	  strncat (br_buf, &text[matchstart], (*matchend - matchstart));
	}
	if (lastmatchlength == 0)
	  return;
      } else {
	if (*t_seg == *toks[i].str) {
	  lastmatchlength = 1;
	  ++(*matchend);
	  toks[i].matchend = *matchend;
	  if (in_br) {
	    strncat (br_buf, t_seg, 1);
	  }
	  t_seg = &text[*matchend];
	} else {
	  toks[i].matchend = *matchend = text_start;
	  return;
	}
      }
    } else if (toks[i].attr & META_CHAR_CLASS) {  /* ie. without the '*' */
      if (char_class_match (*toks[i].str, text[*matchend])) {
	if (in_br) {
	  strncat (br_buf, &text[*matchend], 1);
	}
	++lastmatchlength;
	*matchend += 1;
	toks[i].matchend = *matchend;
      } else {
	*matchend = text_start;
	toks[i].matchend = *matchend;
	return;
      }
    } else if (toks[i].str[0] == '^') { 
      if (!(toks[i].attr & META_CHAR_LITERAL_ESC)) {
	if (!start_of_rec (text, matchend)) {
	  toks[i].matchend = *matchend = text_start;
	  return;
	} else {
	  toks[i].matchend = toks[i].matchstart;
	}
      } else {
	if (*t_seg == *toks[i].str) {
	  lastmatchlength = 1;
	  ++(*matchend);
	  toks[i].matchend = *matchend;
	  if (in_br) {
	    strncat (br_buf, t_seg, 1);
	  }
	  t_seg = &text[*matchend];
	} else {
	  *matchend = text_start;
	  toks[i].matchend = *matchend;
	  return;
	}
      }
    } else if (toks[i].str[0] == '$') {
      if (!(toks[i].attr & META_CHAR_LITERAL_ESC)) {
	if (!end_of_rec (text, matchend, eos)) {
	  toks[i].matchend = *matchend = text_start;
	  return;
	}
      } else {
	if (*t_seg == *toks[i].str) {
	  lastmatchlength = 1;
	  ++(*matchend);
	  toks[i].matchend = *matchend;
	  if (in_br) {
	    strncat (br_buf, t_seg, 1);
	  }
	  t_seg = &text[*matchend];
	} else {
	  *matchend = text_start;
	  toks[i].matchend = *matchend;
	  return;
	}
      }
    } else if (toks[i].attr & META_BROPEN) {
      in_br = true;
      br_start = *matchend;
      memset (br_buf, 0, MAXMSG);
      toks[i].matchend = toks[i].matchstart;
    } else if (toks[i].attr & META_BRCLOSE) {
      in_br = false;
      toks[i].matchend = toks[i].matchstart;
      if (i < (l_n_toks - 2) && (lastmatchlength > 0)) {
	make_br_rec (br_buf, br_start);
      } else {
	br_done = true;
      }
    } else if (strchr (toks[i].str, '.')) {
      if (!(toks[i].attr & META_CHAR_LITERAL_ESC)) {
	if (re_dot_str (t_seg, toks[i].str)) {
	  lastmatchlength = lastmatchlength + strlen (toks[i].str);
	  toks[i].matchend = toks[i].matchstart + strlen (toks[i].str);
	  if (in_br) {
	    strncat (br_buf, t_seg, strlen (toks[i].str));
	  }
	  /* 
	   * i.e., if the pattern can match repeatedly over
	   * the length of the  entire string, don't move t_seg
	   * forward here - the starting position gets incremented
	   * on next call to match_toks.
	   */
	  if (i < (l_n_toks - 1)) {
	    *matchend += strlen (toks[i].str);
	    t_seg = &text[*matchend];
	  }
	} else {
	  toks[i].matchend = text_start;
	  *matchend = text_start;
	  return;
	}
      } else {
	if (*t_seg == *toks[i].str) {
	  lastmatchlength = 1;
	  ++(*matchend);
	  toks[i].matchend = *matchend;
	  if (in_br) {
	    strncat (br_buf, t_seg, 1);
	  }
	  t_seg = &text[*matchend];
	} else {
	  *matchend = text_start;
	  toks[i].matchend = *matchend;
	  return;
	}
      }
    } else {
      if (re_exact_str (&text[*matchend], toks[i].str)) {
	if (in_br) {
	  strncat (br_buf, &text[*matchend], strlen(toks[i].str));
	}
	lastmatchlength = lastmatchlength + strlen (toks[i].str);
	*matchend = *matchend + strlen (toks[i].str);
	toks[i].matchend = *matchend;
	t_seg = &text[*matchend];
      } else {
	*matchend = text_start;
	toks[i].matchend = *matchend;
	return;
      }
    }
  }

  /* Make sure we matched the complete pattern, and haven't
     just run out of text. ... */
  if (i >= l_n_toks) {

    for (i_2 = frame_start; i_2 < (n_toks - 1); i_2++) {
      if (toks[i_2].matchend != toks[i_2+1].matchstart) {
	*matchend = text_start;
	return;
      }
    }

    if (frame_start == 0) {
      make_match_rec (text, text_start, *matchend);
      if (br_done)
	make_br_rec (br_buf, br_start);
      offsets[(*n_matches)++] = text_start;
      offsets[(*n_matches)] = -1;
    } else {
      /* in recursed parsers, we still need to record the matches in
	 locally declared offsets arrays; i.e., not the array that is
	 a parameter to __ctalkMatchText. But we don't need to make
	 match records that the outer program uses. */
      /* It's also possible we can recurse with a parenthesized
	 expression that will become a backreference, as in re_bar */
      if (br_done)
	make_br_rec (br_buf, br_start);
      offsets[(*n_matches)++] = text_start;
      offsets[(*n_matches)] = -1;
    }
    /* 
     *  ... we matched to the end of the text and 
     *  there's still (probably zero-length) tokens... 
     */
  } else if ((i == (l_n_toks - 1)) && (toks[l_n_toks - 1].str[0] == '$')) {
    make_match_rec (text, text_start, *matchend);
    if (br_done)
      make_br_rec (br_buf, br_start);
    offsets[(*n_matches)++] = text_start;
    offsets[(*n_matches)] = -1;
  } else if ((i == (l_n_toks - 1)) && (toks[l_n_toks - 1].str[0] == ')')) {
    /* ... unless the last pattern token is a '$'. */
    make_match_rec (text, text_start, *matchend);
    /* br_done need not be set here if the last  tok is ')'. */
    make_br_rec (br_buf, br_start);
    offsets[(*n_matches)++] = text_start;
    offsets[(*n_matches)] = -1;
  }
}

/* this simplifies match_text_internal when recursion with
   expressions that end with *, +, etc. */
static int match_text_anchored (char *pattern, char *text,
				long long int *offsets) {

  int matchend, end_idx, br_stack_start;
  char pattern_split[MAXMSG];
  int n_matches, l_n_toks;
  RECURSE_STATE recurse_state;

  n_matches = 0;
  offsets[n_matches] = -1;
  lastmatchlength = 0;

  if (strpbrk (pattern, METACHARS) || strpbrk (pattern, "()") ||
      pattern_has_char_class (pattern)) {

    /* A pattern that starts with a '*' needs to look past
       the beginning of the buffer and isn't valid anyway,
       so just return. */
    if (*pattern == '*')
      return n_matches;

    end_idx = eos_idx (text);
    if (*pattern == '/' || *pattern == 'm') {
      split_pattern (pattern, pattern_split);
      l_n_toks = tokenize_pattern (pattern_split);
    } else {
      l_n_toks = tokenize_pattern (pattern);
    }
    matchend = 0;

    br_stack_start = br_ptr;
    match_toks (text, 0, offsets, &matchend, end_idx, &n_matches,
		l_n_toks, &recurse_state);
    if (matchend == 0) {
      /* if we don't match everything, reset the backreference
	 ptr to where it was. */
      br_ptr = br_stack_start;
    }

    if (print_toks_and_matches) {
      printf ("\tSUBEXPRESSION PATTERN: %s\t\tTEXT: \"%s\"\n", pattern, text);
      print_toks (text, l_n_toks, true);
    }
    cleanup_toks (l_n_toks);

  } else { /* if (strpbrk (pattern, METACHARS)) */
    if (*pattern == '/' || *pattern == 'm') {
      split_pattern (pattern, pattern_split);
      n_matches = __ctalkSearchBuffer (pattern_split, text, offsets);
    } else {
      n_matches = __ctalkSearchBuffer (pattern, text, offsets);
    }
    if (n_matches > 0)
      lastmatchlength = strlen (pattern);
    else
      lastmatchlength = 0;
  } /* if (strpbrk (pattern, METACHARS)) */

  return n_matches;
}

int g_n_matches = 0;

/*
 *  Return value is the number of matches.
 */
static int match_text_internal (char *pattern, char *text,
				  long long int *offsets) {

  int i, matchend, end_idx, br_stack_start;
  char pattern_split[MAXMSG];
  int n_matches, l_n_toks;
  RECURSE_STATE recurse_state;

  n_matches = 0;
  offsets[n_matches] = -1;
  lastmatchlength = 0;

  /* init_matchrecs ();
     init_brs (); */

  if (strpbrk (pattern, METACHARS) || strpbrk (pattern, "()") ||
      pattern_has_char_class (pattern)) {

    /* A pattern that starts with a '*' needs to look past
       the beginning of the buffer and isn't valid anyway,
       so just return. */
    if (*pattern == '*')
      return n_matches;

    end_idx = eos_idx (text);
    if (*pattern == '/' || *pattern == 'm') {
      split_pattern (pattern, pattern_split);
      l_n_toks = tokenize_pattern (pattern_split);
    } else {
      l_n_toks = tokenize_pattern (pattern);
    }
    matchend = 0;

    for (i = 0; text[i] && (i < end_idx); ++i) {
      br_stack_start = br_ptr;
      match_toks (text, i, offsets, &matchend, end_idx, &n_matches,
		  l_n_toks, &recurse_state);
      if (matchend == i) {
	/* if we don't match everything, reset the backreference
	   ptr to where it was. */
	br_ptr = br_stack_start;
      } else {
	/* a recursive match should be able to occur anywhere on 
	   the text. ... */
	if ((n_matches > 0) && (recurse_state != recurse_state_non_contiguous))
	  i = matchend - 1;
      }
    }

    if (print_toks_and_matches) {
      if (re_message_ptr < P_MESSAGES) {
	printf ("\tSUBEXPRESSION PATTERN: %s\t\tTEXT: \"%s\"\n", pattern, text);
	print_toks (text, l_n_toks, true);
      } else {
	printf ("PATTERN: %s\t\tTEXT: \"%s\"\n", pattern, text);
	print_toks (text, l_n_toks, false);
      }
    }
    cleanup_toks (l_n_toks);

  } else { /* if (strpbrk (pattern, METACHARS)) */
    if (*pattern == '/' || *pattern == 'm') {
      split_pattern (pattern, pattern_split);
      n_matches = __ctalkSearchBuffer (pattern_split, text, offsets);
    } else {
      n_matches = __ctalkSearchBuffer (pattern, text, offsets);
    }
    if (n_matches > 0)
      lastmatchlength = strlen (pattern);
    else
      lastmatchlength = 0;
  } /* if (strpbrk (pattern, METACHARS)) */

  return n_matches;
}

int __ctalkMatchText (char *pattern, char *text, long long int *offsets) {
  int n_matches = 0;
  init_matchrecs ();
  init_brs ();
  n_matches += match_text_internal (pattern, text, offsets);
  g_n_matches = n_matches;
  return n_matches;
}

int __ctalkLastMatchLength (void) {
  return lastmatchlength;
}

char *__ctalkMatchAt (int n) {
  return brs[n].text;
}

int __ctalkMatchIndexAt (int n) {
  return brs[n].start;
}

int __ctalkNMatches (void) {
  return g_n_matches;
}
