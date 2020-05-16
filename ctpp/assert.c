/* $Id: assert.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2018 Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 *  Preprocessor assertions are a GNU cpp(1) extension.  You can
 *  define them with the #assert directive or the -A command line
 *  option.  For more information about using assertions in the
 *  preprocessor, consult the GNU cpp(1) Texinfo manual.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ctpp.h"
#include "typeof.h"

extern EXCEPTION parse_exception;   /* Declared in pexcept.c.            */

DEFINITION *macro_assertions = NULL;  /* List of assertion definitions. */
DEFINITION *last_assertion = NULL;

static char *collect_answer (MESSAGE_STACK messages, int start, int end) {
  static char *s, buf[MAXMSG];
  if ((s = collect_tokens (messages, start, end)) != NULL) {
    strcpy (buf, s);
    free (s);
    return buf;
  }
  return NULL;
}

int check_assertion (MESSAGE_STACK messages, int msg_ptr, 
		     int *expr_end_ptr, VAL *result) {
  int i;
  int stack_ptr;
  int pred_ptr;
  int n_parens;
  int ans_start_ptr;
  int ans_end_ptr;
  MESSAGE *m;
  DEFINITION *d;
  char ansbuf[MAXMSG];
  
  stack_ptr = get_stack_top (messages);

  for (i = msg_ptr, pred_ptr = ERROR, n_parens = 0,
	 ans_start_ptr = -1, ans_end_ptr = -1, *ansbuf = 0; 
       i > stack_ptr; i--) {

    m = messages[i];

    if (m -> tokentype == WHITESPACE)
      continue;

    switch (m -> tokentype)
      {
      case LABEL:
	if (pred_ptr == ERROR) {
#ifdef DEBUG_CODE
	  if (!prefix (messages, i, (int)'#')) {
	    debug (_("Invalid assertion predicate %s."), m -> name);
	    location_trace (m);
	  } 
#endif
	  *expr_end_ptr = pred_ptr = i;
	}
	break;
      case OPENPAREN:
	if (!n_parens)
	  if ((ans_start_ptr = nextlangmsg (messages, i)) == ERROR)
	    _error ("Out of stack in check_assertion ().\n");
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (!n_parens) {
	  if ((ans_end_ptr = prevlangmsg (messages, i)) == ERROR)
	    _error ("Stack underflow in check_assertion ().\n");
	  *expr_end_ptr = i;
	}
	goto val_evaled;
	break;
      case NEWLINE:
	goto val_evaled;
	break;
      default:
	break;
      }
  }

 val_evaled:

  if (ans_start_ptr != -1 && ans_end_ptr != -1)
    strcpy (ansbuf, collect_answer (messages, ans_start_ptr, ans_end_ptr));


  if (macro_assertions) {
    for (d = macro_assertions; ; d = d -> next) {
      if (!strcmp (d -> name, messages[pred_ptr] -> name)) {
	if (*ansbuf) {
	  if (eval_assertion (d -> value, ansbuf)) {
	    result -> __type = INTEGER_T;
	    result -> __value.__i = 1;
	    return TRUE;
	  }
	} else {
	  /*
	   *  If no value is given in the conditional, then the 
	   *  conditional matches any assertion with this
	   *  predicate.
	   */
	  result -> __type = INTEGER_T;
	  result -> __value.__i = 1;
	  return TRUE;
	}
      }
      if (!d || !d -> next)
	break;
    }
  }
    
  return FALSE;
}

int is_assertion (char *name) {
  DEFINITION *d;
  
  for (d = macro_assertions; d; d = d -> next)
    if (!strcmp (name, d -> name))
      return TRUE;

  return FALSE;
}

int define_assertion (MESSAGE_STACK messages, int keyword_ptr) {

  int i;
  int pred_ptr;
  int ans_start_ptr,
    ans_end_ptr;
  int stack_ptr;
  int n_parens;
  MESSAGE *m;
  DEFINITION *d;
  char *ansbuf;
  
  stack_ptr = get_stack_top (messages);

  for (i = keyword_ptr - 1, pred_ptr = ERROR, n_parens = 0, 
	 ans_start_ptr = -1, ans_end_ptr = -1; 
       i >= stack_ptr; i--) {
    
    m = messages[i];

    if (m -> tokentype == WHITESPACE)
      continue;
    if (m -> tokentype == NEWLINE)
      break;

    switch (m -> tokentype)
      {
      case LABEL:
	if (pred_ptr == ERROR) pred_ptr = i;
	break;
      case OPENPAREN:
	if (!n_parens)
	  if ((ans_start_ptr = nextlangmsg (messages, i)) == ERROR)
	    _error ("End of stack in define_assertion ().\n");
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (!n_parens) {
	  if ((ans_end_ptr = prevlangmsg (messages, i)) == ERROR)
	    _error ("Stack overflow in define_assertion ().\n");
	}
	break;
      default:
	break;
      }
  }

  if (n_parens) {
    parse_exception = mismatched_paren_x;
    return ERROR;
  }

  if ((d = 
       (DEFINITION *)calloc (1, sizeof (struct _macro_symbol)))
      == NULL)
    _error (_("add_symbol: %s."), strerror (errno));

  d -> sig = MACRODEF_SIG;
  strcpy (d -> name, messages[pred_ptr] -> name);
  if (ans_start_ptr != -1 && ans_end_ptr != -1) {
    ansbuf = collect_tokens (messages, ans_start_ptr, ans_end_ptr);
    strcpy (d -> value, ansbuf);
    free (ansbuf);
  } else {
    /*
     *  Default answer.
     */
    strcpy (d -> value, "1");
  }


  if (!macro_assertions) {
    last_assertion = macro_assertions = d;
  } else {
    last_assertion -> next = d;
    d -> prev = last_assertion;
    last_assertion = d;
  }

  return SUCCESS;
}

/*
 *  An assertion is True if:
 *  1.  There is no answer given when it is tested, so the 
 *      predicate matches; 
 *  2.  If the answer and the value are identical labels; 
 *  3.  If the answer evaluates the same as the value.
 *
 *  TO DO - Allow token pasting and literalization in 
 *  assertion expressions.  That's what a_messages, et al.,
 *  are here for.
 */

extern MESSAGE *a_messages[];
extern int a_message_ptr;

int eval_assertion (char *value, char *answer) {

  int i,
    value_is_expr,
    stack_begin,
    stack_end;
  VAL value_val, answer_val;
  MESSAGE *m;

  if (! *answer) 
    return TRUE;

  value_is_expr = str_is_expr (value);

  if (!value_is_expr && !strcmp (value, answer))
      return TRUE;

    stack_begin = P_MESSAGES;
    stack_end = tokenize_reuse (a_message_push, value);
    (void)eval_constant_expr (a_messages, stack_begin, &stack_end, &value_val);

    for (i = stack_begin; i >= stack_end; i--) {
      m = a_message_pop ();
      if (m && IS_MESSAGE (m))
	reuse_message (m);
    }
    
    stack_begin = P_MESSAGES;
    stack_end = tokenize_reuse (a_message_push, answer);
    (void)eval_constant_expr (a_messages, stack_begin, &stack_end, &answer_val);

    for (i = stack_begin; i >= stack_end; i--) {
      m = a_message_pop ();
      if (m && IS_MESSAGE (m))
	reuse_message (m);
    }

  if (val_eq (&value_val, &answer_val))
    return TRUE;

  return FALSE;
}

/*
 *  TO DO - This should be good enough for most uses, but
 *  if there are cases of illegal characters in labels,
 *  it might be necessary to perform a macro match.
 */

int str_is_expr (char *s) {

  int i,
    start_ptr,
    end_ptr,
    return_val;
  MESSAGE *m;

  start_ptr = P_MESSAGES;

  end_ptr = tokenize_reuse (t_message_push, s);

  if(start_ptr == end_ptr)
    return_val = FALSE;
  else
    return_val = TRUE;

  for (i = start_ptr; i >= end_ptr; i--) {
    m = t_message_pop ();
    if (m && IS_MESSAGE (m))
      reuse_message (m);
  }

  return return_val;
}


