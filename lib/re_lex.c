/* $Id: re_lex.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2015 - 2018  Robert Kiesling, rk3314042@gmail.com.
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

/*
  Regular expression tokenizer, adapted from lexical () and tokenize
  () in lex.c.  So far, only the cases that apply to metacharacters
  and labels have been adapted.  Everything else is staying the same,
  until we need a particular punctation token as a metacharacter().
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "lex.h"
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "typeof.h"
#include "rtinfo.h"
#include "regex.h"

extern int ascii_chr_tok[];

#define METACHARACTER(c) (c == '^' || c == '$' || c == '*' || c == '.' || \
			  c == '\\' || c == '+' || c == '|')
#define BR_SEP(c) (c == '(' || c == ')')
#define ESC_CHR(c) (c == '\\')
#define IS_ASCII_ESC_CHR(c) (c == 'a' ||  c == 'b' || c == 'f' || \
				 c == 'n' || c == 'r' || c == 't' ||\
			     c == 'v' || c == '0')

extern int error_line;      /* Declared in errorloc.c. */
extern int error_column;
extern RT_INFO rtinfo;      /* Declared in rtinfo.c.   */

extern int line_info_line;  /* Declared in lineinfo.c. */

extern I_PASS interpreter_pass;  /* Declared in rtinfo.c. */

/*
 *  Used for parsing literals.
 */
#define DEFL_ESC_IDX -2
#define ADD_CHR_RESIZE if (end_str == -1) { m -> name[k++] = buf[j]; } else { continue ; } \
                       if (k >= resize_x) {resize_x *= 2; resize_message (m, resize_x); }

#define _MPUTCHAR(__m,__c) (__m)->name[0] = (__c); (__m)->name[1] = '\0';
#define _MPUT2CHAR(__m,__s) (__m)->name[0] = (__s[0]); \
				 (__m)->name[1] = (__s[1]); \
				 (__m)->name[2] = '\0';
#define _MPUT3CHAR(__m,__s) (__m)->name[0] = (__s[0]); \
				 (__m)->name[1] = (__s[1]); \
				 (__m)->name[2] = (__s[2]); \
				 (__m)->name[3] = '\0';

static int gnuc_attribute;  /* Set by __attribute__ label, cleared after )). */

static int preprocess_line = FALSE;  /* Used to determine statement endings -
                                        Set when we encounter a PREPROCESS
					token and cleared at the NEWLINE 
                                        token.  If set, statements end
                                        at the NEWLINE.  Otherwise, statements
                                        end at the SEMICOLON.
				     */
static bool comment;                 /* True if within a comment.*/ 

static int regex_esc_chr;


static void ascii_escape_chr (MESSAGE *m, char lit_char) {
  switch (lit_char)
    {
    case 'a':	m -> name[0] = '\a'; break;
    case 'b':	m -> name[0] = '\b'; break;
    case 'f':	m -> name[0] = '\f'; break;
    case 'n':	m -> name[0] = '\n'; break;
    case 'r':	m -> name[0] = '\r'; break;
    case 't':	m -> name[0] = '\t'; break;
    case 'v':	m -> name[0] = '\v'; break;
    case '0':	m -> name[0] = '\0'; break;
    }
}

#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
static void about_signed_hex_octal_constant_warning (void) {
  _warning ("(To get rid of this warning, use the --without-signed-hex-octal-constant-warnings option with configure.)\n");
}
#endif

/* an unescaped open or close paren signals the start or end
   of a backreference. */
static bool is_br_sep (const char *s, int i) {
  if (i > 0) {
    if (BR_SEP(s[i]) && (s[i-1] != '\\')) {
      return true;
    }
  } else if (BR_SEP(s[i])) {
    return true;
  }
  return false;
}

static int match_br_sep (const char *buf, int idx) {
  int i, n_parens = 0;
  for (i = idx; buf[i]; i++) {
    if (buf[i] == '(') {
      ++n_parens;
    } else if (buf[i] == ')') {
      --n_parens;
    }
    if (n_parens == 0) {
      return i;
    }
  }
  return ERROR;
}

static bool is_unescaped_meta (const char *s, int idx) {
  if (idx == 0) {
    return METACHARACTER_LESS_DOT(s[idx]);
  } else if (s[idx - 1] != '\\') {
    return METACHARACTER_LESS_DOT(s[idx]);
  }
  return false;
}

static bool unescaped_escape (const char *s, int idx) {
  if (idx == 0) {
    return ESC_CHR(s[idx]);
  } else if (s[idx - 1] != '\\') {
    return ESC_CHR(s[idx]);
  }
  return false;
}
#if 0
static bool unescaped_escape (const char *s, int idx) {
  if (idx == 0 && ESC_CHR(s[idx])) {
    return true;  /* semantically it's an escape chr. */
  } else if (ESC_CHR(s[idx-1]) && ESC_CHR(s[idx])) {
    return true;
  }
  return false;
}
#endif

static char *bar_l_subexpr_scanback (char *bar_ptr,
				     char *buf_start) {
  char *c = bar_ptr - 1;
  char *c_lookback, *c_ptr;
  int n_parens;

  if (c == buf_start)
    return c;

  c_lookback = bar_ptr - 2;

  if (IS_CHAR_CLASS_ABBREV(*c) && *c_lookback == '\\') {
    return c_lookback;
  } else if (RIGHT_ASSOC_META(*c)) {
    --c; --c_lookback;
    if (IS_CHAR_CLASS_ABBREV(*c) && *c_lookback == '\\') {
      return c_lookback;
    } else {
      /* a plain character with a + or * following. '.', too. */
      return c;
    }
  } else if (*c == ')') {
    n_parens = 1;
    c_ptr = c - 1;
    while (c_ptr >= buf_start) {
      if (*c_ptr == ')') {
	++n_parens;
      } else if (*c_ptr == '(') {
	--n_parens;
      }
      if (n_parens == 0) {
	c = c_ptr;
	break;
      }
      --c_ptr;
    }
    return c;
  } else {
    return c;
  }
}

static char *bar_r_subexpr_scanforward (char *bar_ptr) {
  char *c = bar_ptr + 1;
  char *c_lookahead = bar_ptr + 2;
  char *c_lookahead_2 = bar_ptr + 3;
  char *c_ptr;
  int n_parens;
  if (*c == '\\' && IS_CHAR_CLASS_ABBREV (*c_lookahead)) {
    if (RIGHT_ASSOC_META(*c_lookahead_2)) {
      return c_lookahead_2;
    } else {
      return c_lookahead;
    }
  } else if (RIGHT_ASSOC_META(*c_lookahead)) {
    return c_lookahead;
  } else if (*c == '(') {
    c_ptr = c + 1;
    n_parens = 1;
    while (*c_ptr) {
      if (*c_ptr == '(') {
	++n_parens;
      } else if (*c_ptr == ')') {
	--n_parens;
      }
      if (n_parens == 0) {
	c = c_ptr;
	break;
      }
      ++c_ptr;
    }
    return c;
  } else {
    return c;
  }
}

  static int label_length, x_resize;

static int collect_label (const char *buf, long long int *buf_idx,
			  MESSAGE *m,
			  int *label_idx) {
  if (unescaped_escape (buf, *buf_idx)) {
    if (RIGHT_ASSOC_META(buf[(*buf_idx) + 2])) {
      /* The character gets prepended to the metacharacter 
	 in the next token, below. */
      m -> name[*label_idx] = 0;
      m -> tokentype = LABEL;
      return -1;
    } else if (buf[(*buf_idx)-1] == '\\' &&
	       IS_CHAR_CLASS_ABBREV(buf[*buf_idx])) {
      m -> attrs |= META_CHAR_CLASS;
    } else if (buf[*buf_idx]  == '\\' &&
	       IS_ASCII_ESC_CHR(buf[(*buf_idx)+1])) {
      ascii_escape_chr (m, buf[(*buf_idx)+1]);
      *label_idx += 1;
      *buf_idx += 2;
      m -> tokentype = WHITESPACE;
      return WHITESPACE;
    } else {
      (*buf_idx)++;
      m -> name[*label_idx] = buf[(*buf_idx)++]; (*label_idx)++;
      return 0;
    }
  }
  if (RIGHT_ASSOC_META(buf[(*buf_idx) + 1])) {
    /* The character gets prepended to the metacharacter 
       in the next token, below. */
    m -> name[*label_idx] = 0;
    (*buf_idx)++;
    m -> tokentype = LABEL;
    return -1;
  } else if (buf[(*buf_idx)-1] == '\\' &&
	     IS_CHAR_CLASS_ABBREV(buf[*buf_idx])) {
    m -> attrs |= META_CHAR_CLASS;
    if ((buf[(*buf_idx) + 1] == '\\') ||
	(buf[(*buf_idx) +1] == ')')) {
      m -> name[(*label_idx)++] = buf[(*buf_idx)++];
      m -> tokentype = LABEL;
      return LABEL;
    } else if (isalnum((int)buf[(*buf_idx)+1]) ||
	       buf[(*buf_idx)+1] == '_') {
      m -> name[(*label_idx)++] = buf[(*buf_idx)++];
      m -> tokentype = LABEL;
      return LABEL;
    }
  } else if ((buf[*buf_idx] == '\\') &&
	     IS_ASCII_ESC_CHR(buf[(*buf_idx)+1])) {
    ascii_escape_chr (m, buf[(*buf_idx)+1]);
    *label_idx += 1;
    *buf_idx += 2;
    m -> tokentype = WHITESPACE;
    return WHITESPACE;
  } else if (is_br_sep ((char *)buf, *buf_idx)) {
    m -> name[*label_idx] = 0;
    m -> tokentype = LABEL;
    return LABEL;
  }
  m -> name[*label_idx] = buf[(*buf_idx)++]; (*label_idx)++;
  if (++label_length == x_resize) {
    x_resize *= 2;
    resize_message (m, x_resize);
  }
  if (buf[*buf_idx] == '$')
    return -1;
  if (buf[*buf_idx] == '\0')
    return -1;

  return 0;
}

/* See the  comments in lex.c. */
static int char_is_delimiter (const char *buf, int open_delim_idx) {
  int i;
  char delim = buf[open_delim_idx];
  if (isalnum ((int)delim))
    return ERROR;
  for (i = open_delim_idx + 1; buf[i]; ++i) {
    if (IS_SEPARATOR(buf[i]) && (buf[i-1] != '\\') && (buf[i] != delim))
      return ERROR;
    if ((buf[i] == delim) && (buf[i-1] != '\\')) {
      if (buf[i+1] == 0) {
	return i;
      } else if (IS_SEPARATOR(buf[i+1]) && (buf[i+1] != delim)) {
	return i;
      }
    }
  }
  return ERROR;
}

/*
 *    Lexical scanning for regexps.  A lot of the code
 *    is left over from lexical (), but the clauses are
 *    still there so we can use them to handle new 
 *    metacharacters when we add them.
 */

int re_lexical (const char *buf, long long *idx, MESSAGE *m) {

  int c, i, j;
  int numeric_type;
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
  int need_signed_hexadecimal_constant_warning = FALSE,
    need_signed_octal_constant_warning = FALSE;
#endif
  char tmpbuf[MAXMSG];
  int label_idx, closing_br_idx, opening_br_idx;

  c = buf[(*idx)++];

  /* A m<pattern> */
  if (c == 'm') {
    int j, end_idx;
    if ((end_idx = char_is_delimiter (buf, *idx)) != ERROR) {
      j = 0;
      (*idx)--; /* the 'm' is still useful to the regexp parser. */
      if ((end_idx - *idx) > MAXLABEL) {
	resize_message (m, (end_idx - *idx) + 1);
      }
      while (1) {
	m -> name[j++] = buf[(*idx)++];
	if (*idx > end_idx) {
	  m -> name[j] = 0;
	  m -> tokentype = PATTERN;
	  return PATTERN;
	}
      }
    }
  }

  if (c == '/') {
    int end_idx, j;
    if ((end_idx = char_is_delimiter (buf, (*idx) - 1)) != ERROR) {
      --(*idx);
      j = 0;
      if ((end_idx - *idx) > MAXLABEL) {
	resize_message (m, (end_idx - *idx) + 1);
      }
      while (1) {
	m -> name[j++] = buf[(*idx)++];
	if (*idx > end_idx) {
	  m -> name[j] = 0;
	  m -> tokentype = PATTERN;
	  return PATTERN;
	}
      }
    }
  }


  /* LABEL */
  if (!is_unescaped_meta (buf, (*idx)-1)  && !is_br_sep (buf, (*idx)-1) &&
      !ESC_CHR(c) && !isspace((int)c)) {
    /* Strings with '.' get matched by re_dot_str (), so they
       can be part of the text here. */
    (*idx)--;

    label_idx = 0;
    label_length = 0;
    x_resize = MAXLABEL;
    while (isalnum ((int)buf[*idx]) || !is_unescaped_meta(buf, *idx) ||
	   buf[*idx] == '_') {

      if (collect_label (buf, idx, m, &label_idx))
	break;
      if (m -> attrs & META_CHAR_CLASS)
	break;
    }
    m -> name [label_idx] = 0;
    m -> tokentype = LABEL;
    return LABEL;
  }
    
  if (isspace((int)c)) {
    --(*idx);
    label_idx = 0;
    while (isspace ((int)buf[*idx])) {
      if (RIGHT_ASSOC_META(buf[(*idx) + 1])) {
	m -> name[label_idx] = 0;
	(*idx)++;
	m -> tokentype = WHITESPACE;
	return WHITESPACE;
      } else {
	m -> name[label_idx++] = buf[(*idx)++];
	m -> name[label_idx] = 0;
      }
    }
    m -> tokentype = WHITESPACE;
    return WHITESPACE;
  }

  if (c == '*') {
    if (*idx >= 3) {
      if (buf[(*idx) - 3] == '\\' && IS_CHAR_CLASS_ABBREV(buf[(*idx) - 2])) {
	m -> attrs |= META_CHAR_CLASS;
      }
    }
    if (buf[*idx] == '|') {
      m -> name[0] = '\0';
      m -> tokentype = ASTERISK;
      return ASTERISK;
    } else if (buf[(*idx) - 2] == ')') {
      const char *subexpr_start =
	bar_l_subexpr_scanback ((char *)&buf[(*idx) - 1], (char *)buf);
      memset (m -> name, 0, MAXLABEL);
      strncpy (m -> name, subexpr_start, &buf[*idx] - subexpr_start);
      m -> tokentype = ASTERISK;
      return ASTERISK;
    } else {
      m -> name[0] = buf[(*idx) - 2];
      m -> name[1] = c;
      m -> name[2] = '\0';
      m -> tokentype = ASTERISK;
      return ASTERISK;
    }
  }

  /* the tokentype CONDITIONAL == '?' */
  if (c == '?') {
    if (*idx >= 3) {
      if (buf[(*idx) - 3] == '\\' && IS_CHAR_CLASS_ABBREV(buf[(*idx) - 2])) {
	m -> attrs |= META_CHAR_CLASS;
      }
    }
    if (buf[*idx] == '|') {
      m -> name[0] = '\0';
      m -> tokentype = CONDITIONAL;
      return CONDITIONAL;
    } else if (buf[(*idx) - 2] == ')') {
      const char *subexpr_start =
	bar_l_subexpr_scanback ((char *)&buf[(*idx) - 1], (char *)buf);
      memset (m -> name, 0, MAXLABEL);
      strncpy (m -> name, subexpr_start, &buf[*idx] - subexpr_start);
      m -> tokentype = CONDITIONAL;
      return CONDITIONAL;
    } else {
      m -> name[0] = buf[(*idx) - 2];
      m -> name[1] = c;
      m -> name[2] = '\0';
      m -> tokentype = CONDITIONAL;
      return CONDITIONAL;
    }
  }

  if (c == '^') {
    _MPUTCHAR(m, c)
    m -> tokentype = BIT_XOR;
    return BIT_XOR;
  }

  if (c == '(') {
    if ((closing_br_idx = match_br_sep (buf, *idx - 1)) > 0) {
      if (RIGHT_ASSOC_META(buf[closing_br_idx + 1])) {
	*idx = closing_br_idx + 1;
	m -> name[0] = 0;
	m -> tokentype = OPENPAREN;
	return OPENPAREN;
      }
    } 
    if (is_br_sep (buf, (*idx) - 1)) {
      _MPUTCHAR(m, c)
	m -> tokentype = OPENPAREN;
      m -> attrs |= META_BROPEN;
      return OPENPAREN;
    }
  }

  if (c == ')') {
    if (is_br_sep (buf, (*idx) - 1)) {
      _MPUTCHAR(m, c)
	m -> tokentype = CLOSEPAREN;
      m -> attrs |= META_BRCLOSE;
      return CLOSEPAREN;
    }
  }

  if (c == '$') {
    _MPUTCHAR(m, c)
    m -> tokentype = DOLLAR;
    return DOLLAR;
  }

  /* vertical bar - so far only singe character on each side */
  if (c == '|') {
    --*idx;
    char prev_expr[MAXMSG];
    char next_expr[MAXMSG];
    char expr[MAXMSG];
    char *l_expr_start, *r_expr_end;
    memset (prev_expr, 0, MAXMSG);
    memset (next_expr, 0, MAXMSG);

    l_expr_start = bar_l_subexpr_scanback ((char *)&buf[*idx],
					   (char *)buf);
    r_expr_end = bar_r_subexpr_scanforward ((char *)&buf[*idx]);

    /* strncpy (prev_expr, &buf[(*idx)-1], 1); */
    strncpy (prev_expr, l_expr_start, &buf[*idx] - l_expr_start);
    strncpy (next_expr, &buf[(*idx)+1], r_expr_end - &buf[*idx]);

    strcatx (expr, prev_expr, "|", next_expr, NULL);
    (*idx) += strlen (next_expr) + 1;
    m -> name = strdup (expr);
    m -> tokentype = BAR;
    return BAR;
  }

  /* DIVIDE, DIV_ASSIGN */
  if (c == '/') {
    switch (buf[*idx]) 
      {
      case '=':
	_MPUT2CHAR(m, "/=")
	++(*idx);
	m -> tokentype = DIV_ASSIGN;
	return DIV_ASSIGN;
	break;
      default:
	_MPUTCHAR(m,c)
	m -> tokentype = DIVIDE;
	return DIVIDE;
	break;
      }
  }

  /* LT, ASL, ASL_ASSIGN */
  if (c == '<') {
    switch (buf[*idx])
      {
      case '<':
	(*idx)++;
	switch (buf[*idx])
	  {
	  case '=':
	    (*idx)++;
	    _MPUT3CHAR(m, "<<=")
	    m -> tokentype = ASL_ASSIGN;
	    return ASL_ASSIGN;
	    break;
	  default:
	    _MPUT2CHAR(m, "<<")
	    m -> tokentype = ASL;
	    return ASL;
	    break;
	  }
	break;
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, "<=")
	m -> tokentype = LE;
	return LE;
	break;
      default:
	_MPUTCHAR(m,c)
	m -> tokentype = LT;
	return LT;
	break;
      }
  }

  /* GT, ASR, ASR_ASSIGN, GE */
  if (c == '>') {
    switch (buf[*idx])
      {
      case '>':
	(*idx)++;
	switch (buf[*idx])
	  {
	  case '=':
	    (*idx)++;
	    _MPUT3CHAR(m, ">>=")
	    m -> tokentype = ASR_ASSIGN;
	    return ASR_ASSIGN;
	    break;
	  default:
	    _MPUT2CHAR(m, ">>")
	    m -> tokentype = ASR;
	    return ASR;
	    break;
	  }
	break;
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, ">=")
	m -> tokentype = GE;
	return GE;
	break;
      default:
	_MPUTCHAR(m,c)
	m -> tokentype = GT;
	return GT;
	break;
      }
  }

  /* PERIOD, ELLIPSIS */
  if (c == '.') {
    if (buf[*idx] == '.' && buf[*idx+1] == '.') {
      (*idx) += 2;
      _MPUT3CHAR(m, "...")
      m -> tokentype = ELLIPSIS;
      return ELLIPSIS;
    } else {
      _MPUTCHAR(m,c)
      m -> tokentype = PERIOD;
      return PERIOD;
    }
  }

  /* BIT_OR, BOOLEAN_OR, BIT_OR_ASSIGN */
  if (c == '|') {
    switch (buf[*idx])
      {
      case '|':
	(*idx)++;
	_MPUT2CHAR(m, "||")
	m -> tokentype = BOOLEAN_OR;
	return BOOLEAN_OR;
	break;
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, "|=")
	m -> tokentype = BIT_OR_ASSIGN;
	return BIT_OR_ASSIGN;
	break;
      default:
	_MPUTCHAR(m,c)
	m -> tokentype = BAR;
	return BAR;
	break;
      }
  }

  /* AMPERSAND, BIT_AND_ASSIGN, BOOLEAN_AND */
  if (c == '&') {
    int c_idx;
    switch (buf[*idx])
      {
      case '&':
	(*idx)++;
	_MPUT2CHAR(m, "&&")
	m -> tokentype = BOOLEAN_AND;
	return BOOLEAN_AND;
	break;
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, "&=")
	m -> tokentype = BIT_AND_ASSIGN;
	return BIT_AND_ASSIGN;
	break;
      default:
	_MPUTCHAR(m,c)
	/*
	 *  If the preceding non-whitespace character is an operator,
	 *  then the ampersand is an address-of operator, otherwise
	 *  it is a bitwise and operator.
	 * 
	 *  If the ampersand is the first character of the buffer,
	 *  then it is an address-of operator.
	 */
	if (((*idx) - 1) == 0) {
	  m -> tokentype = AMPERSAND;
	  return AMPERSAND;
	} else {
	  for (c_idx = (*idx) - 2; c_idx >= 0; c_idx--) {
	    if (isspace ((int)buf[c_idx])) 
	      continue;
	    if (IS_C_OP_CHAR (buf[c_idx])) {
	      if ((buf[c_idx] == ')') ||
		  (buf[c_idx] == ']')) {
		m -> tokentype = BIT_AND;
		return BIT_AND;
	      } else {
		m -> tokentype = AMPERSAND;
		return AMPERSAND;
	      }
	    } else {
	      m -> tokentype = BIT_AND;
	      return BIT_AND;
	    }
	  }
	}
	break;
      }
  }

  /* EQ, BOOLEAN_EQ */
  if (c == '=') {
    switch (buf[*idx])
      {
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, "==")
	m -> tokentype = BOOLEAN_EQ;
	return BOOLEAN_EQ;
	break;
      default:
	_MPUTCHAR(m,c)
	m -> tokentype = EQ;
	return EQ;
	break;
      }
  }

  /* BIT_COMP */
  /*
   *  ~= is not given as an operator in C99, so probably
   *  shouldn't support it here.
   */
  if (c == '~') {
    _MPUTCHAR(m,c)
      m -> tokentype = BIT_COMP;
    return BIT_COMP;
  }

  /* EXCLAM, INEQUALITY */
  if (c == '!') {
    switch (buf[*idx]) 
      {
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, "!=")
	m -> tokentype = INEQUALITY;
	return INEQUALITY;
	break;
      default:
	_MPUTCHAR(m,c)
	m -> tokentype = EXCLAM;
	return EXCLAM;
	break;
      }
  }

  /* Note that this occurs after the numeric constant tokens. */
  if (c == '+') {
    if (*idx >= 3) {
      if (buf[(*idx) - 3] == '\\' && IS_CHAR_CLASS_ABBREV(buf[(*idx) - 2])) {
	m -> attrs |= META_CHAR_CLASS;
      }
    }
    if (buf[*idx] == '|') {
      m -> name[0] = '\0';
      m -> tokentype = PLUS;
      return PLUS;
    } else if (buf[(*idx) - 2] == ')') {
      const char *subexpr_start =
	bar_l_subexpr_scanback ((char *)&buf[(*idx) - 1], (char *)buf);
      memset (m -> name, 0, MAXLABEL);
      strncpy (m -> name, subexpr_start, &buf[*idx] - subexpr_start);
      m -> tokentype = PLUS;
      return PLUS;
    } else {
      m -> name[0] = buf[(*idx - 2)];
      m -> name[1] = c;
      m -> name[2] = '\0';
      m -> tokentype = PLUS;
      return PLUS;
    }
  }

  /* MINUS, MINUS_ASSIGN, DECREMENT, DEREF */
  /* Note that this occurs after the checking for numeric constants. */
  if (c == '-') {
    switch (buf[*idx])
      {
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, "-=")
	m -> tokentype = MINUS_ASSIGN;
	return MINUS_ASSIGN;
	break;
      case '-':
	(*idx)++;
	_MPUT2CHAR(m, "--")
	m -> tokentype = DECREMENT;
	return DECREMENT;
	break;
      case '>':
	(*idx)++;
	_MPUT2CHAR(m, "->")
	m -> tokentype = DEREF;
	return DEREF;
	break;
      default:
	_MPUTCHAR(m,c)
	m -> tokentype = MINUS;
	return MINUS;
	break;
      }
  }

  /* PREPROCESS, LITERALIZE, MACRO_CONCAT */
  if (c == '#') {
    /*
     *  We need to ensure that the token is the first *non-whitespace*
     *  token after a newline, or is the first non-whitespace 
     *  token in the text.
     *
     */
    int lookback = ((*idx - 2) > 0) ? *idx - 2 : 0;

    while (((buf[lookback] == ' ') || 
	   (buf[lookback] == '\t')) &&
	   (lookback > 0)) {
      lookback--;
    }

    if ((buf[lookback] == '\n' || lookback == 0)) {

      /* 
       *  If this is a line marker, parse it and set the 
       *  error location.
       */

      if (ASCII_DIGIT ((int)buf[*idx + 1]) && !line_info_line) {
	char *n, linebuf[MAXMSG];
	if ((n = index (&buf[*idx + 1], '\n')) != NULL) {
	  substrcpy (linebuf, (char *)&buf[*idx-1], 0, n - &buf[*idx - 1]);
	} else {
	  strcpy (linebuf, &((char *)buf)[*idx - 1]);
	}
	line_info_tok (linebuf);
      }
      _MPUTCHAR(m,c)
      m -> tokentype = PREPROCESS;
      preprocess_line = TRUE;
      return PREPROCESS;
    } else {
      if (buf[*idx] == '#') {
	(*idx)++;
	_MPUT2CHAR(m, "##")
	m -> tokentype = MACRO_CONCAT;
	return MACRO_CONCAT;
      } else {
	_MPUTCHAR(m,c)
	m -> tokentype = LITERALIZE;
	return LITERALIZE;
      }
    }
  }

  /* LITERAL */
  /* Check for quote characters ('"') and escaped quotes within
     the literal. */
  if (c == '"') {
    int j, k, in_str, in_quoted_chr, j_1;
    int n_quotes;
    int resize_x;
    int end_str;

    /*
     *  regex_esc_chr needs a default value that can't be mistaken
     *  for a <string index> - 1, so use -2.
     */

    for (j = *idx - 1, in_str = FALSE, in_quoted_chr = FALSE,
	   k = 0, resize_x = MAXLABEL,
	   regex_esc_chr = DEFL_ESC_IDX, n_quotes = 0, end_str = -1; 
	 buf[j];
	 j++) {
      switch (buf[j])
	{
	case '"':
	  if (regex_esc_chr == (j - 1)) {
	    regex_esc_chr = DEFL_ESC_IDX;
	    ADD_CHR_RESIZE
	  } else {
	    ++n_quotes;
	    if (in_str == TRUE) {
	      in_str = FALSE;
	      ADD_CHR_RESIZE
	      end_str = j;
	    } else {
	      in_str = TRUE;
	      ADD_CHR_RESIZE
	    }
	  }
	  break;
	case '\\':
	  regex_esc_chr = (regex_esc_chr == DEFL_ESC_IDX) ? j : DEFL_ESC_IDX;
	  ADD_CHR_RESIZE
	  break;
	case '\'':
	  /*
	   *  Check for a '"' but not a "'c'"; i.e., only 
	   *  register the escape on the leading single quote.
	   */
	  if (!in_quoted_chr) {
	    /* TODO- Tidy this whole clause up a bit. */
	    if (regex_esc_chr != DEFL_ESC_IDX) 
	      regex_esc_chr = DEFL_ESC_IDX;
	    in_quoted_chr = TRUE;
	  } else {
	    regex_esc_chr = DEFL_ESC_IDX;
	    in_quoted_chr = FALSE;
	  }
          ADD_CHR_RESIZE
	  break;

	  /* Scan past comments. */
	case '/':
	  if (!in_str) {
	    if (buf[j+1]) {
	      switch (buf[j+1])
		{
		default:
		  if (regex_esc_chr != DEFL_ESC_IDX) regex_esc_chr = DEFL_ESC_IDX;
		  ADD_CHR_RESIZE
		    break;
		}
	    } else {
	      if (regex_esc_chr != DEFL_ESC_IDX) regex_esc_chr = DEFL_ESC_IDX;
	      ADD_CHR_RESIZE
            }
	  } else {
	    if (regex_esc_chr != DEFL_ESC_IDX) regex_esc_chr = DEFL_ESC_IDX;
	    ADD_CHR_RESIZE
	  }
	  break;

	default:
	  if (regex_esc_chr != DEFL_ESC_IDX) regex_esc_chr = DEFL_ESC_IDX;
	  ADD_CHR_RESIZE
	  break;
	}
    }

    /*
     *  If the number of quotes to the end of the input is odd,
     *  then we have an unterminated string somehow.
     */

    if (n_quotes % 2) {
      switch (interpreter_pass)
	{
	case run_time_pass:
	  _warning ("%s:%d:warning: Unterminated string constant.\n",
		    ((__getClassLibRead () == True) ? __classLibFileName () : 
		     __argvFileName ()),
		    error_line );
	  break;
	case preprocessing_pass:
	case var_pass:
	case parsing_pass:
	case library_pass:
	case method_pass:
	case expr_check:
	case c_fn_pass:
	  _warning ("%s:%d:warning: Unterminated string constant.\n",
		    __source_filename (), error_line);
	default:
	  break;
	}
    }

    m -> name[k] = 0;
    *idx += strlen (m -> name) - 1;
    m -> tokentype = LITERAL;
    return LITERAL;
  }

  /* LITERAL_CHAR in single quotes.*/
  /*
   *   There should not be much need to re-interpret character
   *   constants, except in __ctalk_to_c_char ().  It is much easier
   *   to keep the characters delimited by single quotes, so we can
   *   distinguish them from labels and escape sequences every time we
   *   retokenize them.
   *
   *   Resize message here if necessary in case there's a buffer
   *   overrun.
   */
  if (c == '\'') {
    int i = 0;
    int resize_x = MAXLABEL;
    m -> name[i++] = c;
    if (!comment) {
      while (1) {
	if ((buf[*idx] == ';') && (buf[(*idx)-1] != '\''))
	  _error ("Unterminated character constant line %d, column %d.\n",
		  error_line, error_column);
	m -> name[i++] = buf[(*idx)++];
	if (i >= resize_x) {
	  resize_x *= 2;
	  resize_message (m, resize_x);
	}
	/*
	 *  If quoting an escaped single quote, break
	 *  on the final single quote only.
	 */
	if ((buf[(*idx) - 1] == '\'') && (buf[*idx] != '\''))
	  break;
	/*
	 *  If we have double escaped NULL - '\\0', then
	 *  save it as '\0'.
	 */
	if (buf[*idx] == '0' && buf[*idx + 1] == '\'') {
	  m->name[0] = '\''; m->name[1] = '\\';m->name[2] = '0'; 
	  m -> name[3] = '\''; m -> name[4] = '\0';
	}
      }
    }
    m -> tokentype = LITERAL_CHAR;
    return LITERAL_CHAR;
  }

  /* CR - LF  to NEWLINE.
     A newline token can contain a run of newlines, *but it 
     should contain no other characters* between the newlines, 
     otherwise, line numbers will not be calculated correctly 
     in tokenize (), below. 

     There are two different clauses here - one for '\n' newlines
     and one for '\r' newlines.  The '\r' clause translates the
     CR to a complier '\n'.

*/ 
  if (c == '\n') {
      int buflength = 0;
      int resize_x = MAXLABEL;
      if ((*idx >= 2) && buf[*idx - 2] == '\\') {
	_MPUTCHAR(m, ' ')
	/* Increment the error line and reset error column also. */
	++error_line; error_column = 0;
	m -> tokentype = WHITESPACE;
	return WHITESPACE;
      }

      _MPUTCHAR (m, '\n');

   while (buf[*idx] == '\n') {
     if (++buflength >= resize_x) {
       resize_x += resize_x;
       resize_message (m, resize_x);
     }
     m -> name[buflength] = '\n';
     /* Don't increment the error line here - 
	set_error_location does that for all tokens that 
	contain newlines. */
     (*idx)++;
   }
   m -> name[buflength+1] = '\0';
   error_column = 0;
    if (preprocess_line)
      preprocess_line = FALSE;
    m -> tokentype = NEWLINE;
    return NEWLINE;
  }

  if (c == '\r') {
      int buflength = 0;
      int resize_x = MAXLABEL;
      if ((*idx >= 2) && buf[*idx - 2] == '\\') {
	_MPUTCHAR(m, ' ')
	/* Increment the error line and reset error column also. */
	++error_line; error_column = 0;
	m -> tokentype = WHITESPACE;
	return WHITESPACE;
      }

      _MPUTCHAR (m, '\n');

      while (buf[*idx] == '\r') {
	if (++buflength >= resize_x) {
	  resize_x += resize_x;
	  resize_message (m, resize_x);
	}
	m -> name[buflength] = '\n';
	/* Don't increment the error line here - 
	   set_error_location does that for all tokens that 
	   contain newlines. */
	(*idx)++;
      }
      m -> name[buflength+1] = '\0';
      error_column = 0;
      if (preprocess_line)
	preprocess_line = FALSE;
      m -> tokentype = NEWLINE;
      return NEWLINE;
  }

  /* MODULUS */
  if (c == '%') {
    switch (buf[*idx])
      {
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, "%=")
	m -> tokentype = MODULUS_ASSIGN;
	return MODULUS_ASSIGN;
	break;
      default:
	_MPUTCHAR(m,c)
	m -> tokentype = MODULUS;
	return MODULUS;
	break;
      }
  }

  if (c == 0) {
    m -> tokentype = EOS;
    _MPUTCHAR(m,'\n')
    return 0;
  }

  if (c == '\\') {
    if (buf[*idx]) {
      if (buf[*idx] == '+' || buf[*idx] == '*' || buf[*idx] == '^' ||
	  buf[*idx] == '$' || buf[*idx] == '\\' || buf[*idx] == '?' ||
	  buf[*idx] == '.') {
	m -> name[0] = buf[*idx]; m -> name[1] = '\0';
	m -> attrs = META_CHAR_LITERAL_ESC;
	m -> tokentype = LABEL;
	++(*idx);
	return LABEL;
      } else {
	int label_idx = 0;
	while (1) {
	  if (collect_label (buf, idx, m, &label_idx))
	    break;
	}
	m -> name[label_idx] = 0;
	m -> tokentype = LABEL;
	return LABEL;
      }
    } else {
      m -> tokentype = BACKSLASH;
      _MPUTCHAR(m,'\\');
      return BACKSLASH;
    }
  }
  return 0;
}

/* Tokenize a character string and place the messages on the 
   message stack used by the (push() () function.
   Return the last stack pointer.
*/

int re_tokenize (int (*push)(MESSAGE *), char *buf) {

  long long i = 0L;
  int tok;
  MESSAGE *m;
  int stack_ptr = ERROR;             /* Avoid warnings. */
  int comment_open_line = FALSE;

  comment = gnuc_attribute = FALSE;
  while (buf[i]) {

    if ((m = new_message()) == NULL)
      _error ("Tokenize: Invalid new_message.");

  reuse_message:
    tok = re_lexical (buf, &i, m);  /* get token */

    if (tok == OPENCCOMMENT){
      if (comment) {
	_error ("Unterminated comment at line %d", comment_open_line);
      } else {
	/* Eats the whole comment now. */
	strcpy (m -> name, " ");
	m -> tokentype = WHITESPACE;
	stack_ptr = (push) (m);
      }
      continue;
    }

    if (comment && m -> tokentype != NEWLINE) {
      m -> name[0] = '\0';
      m -> tokentype = 0;
      goto reuse_message;
    }

    if (m -> name[0] == 0)
      goto reuse_message;

    stack_ptr = (push) (m);
  }

  return stack_ptr;
}

