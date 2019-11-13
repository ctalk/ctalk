/* $Id: escstr.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#ifndef MAXMSG
#define MAXMSG 8192
#endif

/* From ctalk.h */
#define TRIM_LITERAL(s) (substrcpy (s, s, 1, strlen (s) - 2))
void _warning (char *, ...);
char *substrcpy (char *, char *, int, int);

/*
 *  Escape the opening and closing quotes of a string.
 *  The easiest way to do this is to chop the quotes
 *  and re-format with escaped quotes.
 *
 *  NOTE: changes the original buffer.  This fn also
 *  assumes that s_in is enclosed in quotes.
 */

char *escape_str (char *s_in, char *s_out) {
  char *buf;
  int len;
  TRIM_LITERAL (s_in);
  strcatx (s_out, "\\\"", s_in, "\\\"", NULL);
  return s_out;
}


#define  J_TERM (j + 1)
char *escape_str_quotes (char *s_in, char *s_out) {

  register int i, j;
  RADIX r;

  for (i = 0, j = 0; s_in[i]; i++, j++) {
    switch (s_in[i]) 
      {
      case '\\':
	/* Find out if the sequence is a '\0' or an octal:  \0[dd]. */
	if (s_in[i + 1] == '0') {
	  if ((r = radix_of (&s_in[i+1])) == octal) {
	    s_out[j] = s_in[i]; /* s_out[J_TERM] = 0; */
	  } else {
	    /* Look for a '\0' sequence exactly. */
	    if (s_in[i + 2] == '\'') {
	      s_out[j++] = '\\';
	      s_out[j] = s_in[i]; /* s_out[J_TERM] = 0; */
	    } else {
	      s_out[j] = s_in[i]; /* s_out[J_TERM] = 0; */
	    }
	  }
	} else {
	  if (!ESC_SEQUENCE_CHAR(s_in[i+1]))
	    s_out[j++] = '\\';
	  s_out[j] = s_in[i]; /* s_out[J_TERM] = 0; */
	}
	break;
      case '\"':
        s_out[j++] = '\\';
	s_out[j] = s_in[i]; /* s_out[J_TERM] = 0; */
	break;
      default:
	s_out[j] = s_in[i]; /* s_out[J_TERM] = 0; */
      }
  }
  s_out[j] = '\0';
  
  return s_out;
}

/* like escape_str_quotes, but it doesn't do anything
   differently for ASCII escape sequences. */
char *escape_pattern_quotes (char *s_in, char *s_out) {

  int i, j;
  RADIX r;

  for (i = 0, j = 0; s_in[i]; i++, j++) {
    switch (s_in[i]) 
      {
      case '\\':
	/* Find out if the sequence is a '\0' or an octal:  \0[dd]. */
	if (s_in[i + 1] == '0') {
	  if ((r = radix_of (&s_in[i+1])) == octal) {
	    s_out[j] = s_in[i]; s_out[J_TERM] = 0;
	  } else {
	    /* Look for a '\0' sequence exactly. */
	    if (s_in[i + 2] == '\'') {
	      s_out[j++] = '\\';
	      s_out[j] = s_in[i]; s_out[J_TERM] = 0;
	    } else {
	      s_out[j] = s_in[i]; s_out[J_TERM] = 0;
	    }
	  }
	} else {
	  s_out[j++] = '\\';
	  s_out[j] = s_in[i]; s_out[J_TERM] = 0;
	}
	break;
      case '\"':
        s_out[j++] = '\\';
	s_out[j] = s_in[i]; s_out[J_TERM] = 0;
	break;
      default:
	s_out[j] = s_in[i]; s_out[J_TERM] = 0;
      }
  }

  return s_out;
}

char *unescape_str_quotes (char *s_in, char *s_out) {

  int i, 
    j,
    esc_chr;

  /* This is a special case for "" , which
     the loop below doesn't handle. */
  if (s_in[0] == '"' && s_in[1] == '"' && s_in[2] == '\0') {
    s_out[0] = 0;
    return s_out;
  }

  for (i = 0, j = 0, esc_chr = -3; s_in[i]; i++) {
    switch (s_in[i])
      {
      case '\\':
	if (esc_chr == (i - 1)) {
	  s_out[j++] = s_in[i]; s_out[J_TERM] = 0;
	  esc_chr = -3;
	} else {
	  esc_chr = i;
	}
        break;
      case '\"':
	if (esc_chr == (i - 1))
	  s_out[j++] = s_in[i]; s_out[J_TERM] = 0;
	break;
      default:
        s_out[j++] = s_in[i]; s_out[J_TERM] = 0;
        break;
      }
  }

  return s_out;
}

/*
 *  This function is only called by generate_store_arg_call ().
 *  As long as it is only called by output.c functions, it
 *  should be all right to use the e_messages stack.
 */

char *esc_expr_quotes (char *expr_in, char *expr_out) {

  int stack_start,
    stack_end,
    i;
  MESSAGE_STACK messages;
  char esc_buf_out[MAXMSG];

  if (expr_in == NULL || *expr_in == '\0')
    return NULL;

  messages = _get_e_messages ();
  stack_start = _get_e_message_ptr ();
  stack_end = tokenize_no_error (e_message_push, expr_in);

  for (i = stack_start, *expr_out = 0; i >=stack_end; i--) {

    switch (messages[i] -> tokentype)
      {
      case LITERAL:
	strcatx2 (expr_out, escape_str_quotes (M_NAME(messages[i]), esc_buf_out),
		  NULL);
	break;
      default:
	strcatx2 (expr_out, M_NAME(messages[i]), NULL);
	break;
      }
  }

  for (i = stack_end; i <= stack_start; i++)
    /* delete_message (e_message_pop ()); */
    reuse_message (e_message_pop ());

  return expr_out;
}

char *esc_expr_and_pattern_quotes (char *expr_in, char *expr_out) {

  int stack_start,
    stack_end,
    i;
  MESSAGE_STACK messages;
  char esc_buf_out[MAXMSG];

  messages = _get_e_messages ();
  stack_start = _get_e_message_ptr ();
  stack_end = tokenize_no_error (e_message_push, expr_in);

  for (i = stack_start, *expr_out = 0; i >=stack_end; i--) {

    switch (messages[i] -> tokentype)
      {
      case LITERAL:
	strcatx2 (expr_out, escape_str_quotes (M_NAME(messages[i]), esc_buf_out), NULL);
	break;
      case PATTERN:
	strcatx2 (expr_out, escape_pattern_quotes
		  (M_NAME(messages[i]), esc_buf_out), NULL);
	break;
      default:
	strcatx2 (expr_out, M_NAME(messages[i]), NULL);
	break;
      }
  }

  for (i = stack_end; i <= stack_start; i++)
    reuse_message (e_message_pop ());

  return expr_out;
}

void de_newline_buf (char *s) {
  char *p;

  while ((p = index (s, '\n')) != NULL)
    *p = ' ';
}

/* 
   Recognizes all of the possible constant quoting and escape sequences -
   see the switch statement cases, below.
   Not much faster than handling this in a method, but a lot less messy. 
*/
int __ctalkIntFromCharConstant (char *s) {
  char *s_start = &s[0];
  bool have_esc = false;
  if (s[0] == '\'') {
    s_start = &s[1];
    if (*s_start == '\\') {
      have_esc = true;
      s_start = &s[2];
    } else if (*s_start == 0) {
      /* Just a single quote, nothing else. */
      return (int)'\'';
    }
  } else if (s[0] == '\'') {
    have_esc = true;
    s_start = &s[1];
  } else if (str_eq (s, NULLSTR)) {
    return 0;
  } else if (s[0] == '\'' && s[1] == 0) {
    /* In case we store a null byte at the end. */
    return 0;
  }
  if (have_esc) {
    switch (*s_start)
      {
      case 'a':	return 1; break;
      case 'b':	return 2; break;
      case 'f':	return 6; break;
      case 'n':	return 10; break;
      case 'r':	return 13; break;
      case 't':	return 9; break;
      case 'v':	return 11; break;
      case '0':	return 0; break;
      default: return (int) *s_start; break;
      }
  } else {
    return (int)*s_start;
  }
}

int docstring_to_line_count (MESSAGE *m) {
  char *buf;
  int i, j;
  int n_new_lines = 0;
  if ((buf = (char *)__xalloc (strlen (m -> name))) == NULL) 
    _error ("docstring_to_line_count: %s.\n", strerror (errno));

  j = 0;
  for (i = 0; m -> name[i]; ++i) {
    if (m -> name[i] == '\n') {
      buf[j++] = '\n';
      ++n_new_lines;
    }
  }
  buf[j] = '\0';
  strcpy (m -> name, buf);
  m -> tokentype = WHITESPACE;
  __xfree (MEMADDR(buf));
  return n_new_lines;
}
