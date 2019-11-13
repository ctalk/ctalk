/* $Id: psubexpr.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011 Robert Kiesling, rk3314042@gmail.com.
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
 *   Functions to parse subexpressions and scan a message stack for
 *   lexical types.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctpp.h"
#include "typeof.h"

extern EXCEPTION parse_exception;   /* Declared in pexcept.c.     */

/*
 *  Search for the matching close parenthesis in preprocessor
 *  statements, or return an error if it isn't found before a 
 *  newline.
 */

int p_match_paren (MESSAGE **messages, int this_message, 
		 int end_ptr) {

  int i, parens = 0;
  bool arg_start = False,
    arg_end = False;
  MESSAGE *m;

  for (i = this_message; i >= end_ptr; i--) {
    m = messages[i];
    if (m && IS_MESSAGE (m)) {
      switch (m -> tokentype)
	{
	case OPENPAREN:
	  if (!parens)
	    arg_start = True;
	  ++parens;
	  break;
	case CLOSEPAREN:
	  --parens;
	  if (!parens)
	    arg_end = True;
	  break;
	case NEWLINE:
	  if (!LINE_SPLICE (messages, i) && (parens != 0)) {
	    new_exception (mismatched_paren_x);
	    return -1;
	  }
	  break;
	default:
	  break;
	}

      /* If we started before the opening paren, don't return yet. */
      if (!parens && arg_start && arg_end)
	return i;
    }
  }
  new_exception (mismatched_paren_x);
  return -1;
}

/*
 *  As above, but return an error if a matching close parenthesis
 *  isn't found before a semicolon or closing block.
 */

int match_paren (MESSAGE **messages, int this_message, 
		 int end_ptr) {

  int i, parens = 0;
  MESSAGE *m;

  for (i = this_message; i >= end_ptr; i--) {
    m = messages[i];
    if (m && IS_MESSAGE (m)) {
      switch (m -> tokentype)
	{
	case OPENPAREN:
	  ++parens;
	  break;
	case CLOSEPAREN:
	  --parens;
	  break;
	default:
	  break;
	}

      if (!parens && (i != this_message))
	return i;
    }
  }
  new_exception (mismatched_paren_x);
  return -1;
}

/*
 *  Return the stack index of the next non-whitespace and 
 *  non-newline message.
 *
 *  Warning - the function should be preprocessor safe now,
 *  but it is inadequately tested.  Use with caution when
 *  called by preprocessor functions.  Also not tested 
 *  with cparse.c functions.
 */
int nextlangmsg (MESSAGE **messages, int this_msg) {

  int i;
  int last_ptr;

  last_ptr = get_stack_top (messages);

  for (i = this_msg - 1; i > last_ptr; i--) {
    if (!messages[i] || !IS_MESSAGE (messages[i]))
      return ERROR;
    if (messages[i] -> tokentype != WHITESPACE)
	return i;
  }
  return ERROR;
}

/*
 *   Return the stack index of the previous non-whitespace
 *   message.  FIXME! Also check for newlines.
 */

int prevlangmsg (MESSAGE **messages, int this_msg) {

  int i;
  int top_ptr;

  top_ptr = stack_start (messages);

  for (i = this_msg + 1; i <= top_ptr; i++) {
    if (messages[i] -> tokentype != WHITESPACE)
      return i;
  }
  return ERROR;
}

/*
 *  Functions to scan the message stack for the next or previous
 *  token type, or the next or previous message that does not
 *  have the specified token type.
 *
 *  Warning - not preprocessor safe.  Call only from 
 *  functions called by parser_pass ().
 */
int scanforward_other (MESSAGE **messages, int start_ptr, 
		 int end_ptr, int tokentype) {
  return _scanforward (messages, start_ptr, end_ptr, tokentype, TRUE);
}

int scanforward (MESSAGE **messages, int start_ptr, 
		 int end_ptr, int tokentype) {
  return _scanforward (messages, start_ptr, end_ptr, tokentype, FALSE);
}

int _scanforward (MESSAGE **messages, int start_ptr, 
		 int end_ptr, int tokentype, int n) {

  MESSAGE *m;
  int i;

  for (i = start_ptr; i >= end_ptr; i--) {
    m = messages[i];
    if (!m || !IS_MESSAGE (m))
      break;
    if (n) {
      if (m -> tokentype != tokentype)
	return i;
    } else {
      if (m -> tokentype == tokentype)
	return i;
    }
  }
  return ERROR;
}

int scanback (MESSAGE **messages, int start_ptr, 
	      int end_ptr, int tokentype) {
  return _scanback (messages, start_ptr, end_ptr,
		    tokentype, FALSE);
}

int scanback_other (MESSAGE **messages, int start_ptr, 
		    int end_ptr, int tokentype) {
  return _scanback (messages, start_ptr, end_ptr,
		    tokentype, TRUE);
}

int _scanback (MESSAGE **messages, int start_ptr, 
	      int end_ptr, int tokentype, int n) {

  MESSAGE *m;
  int i;

  for (i = start_ptr; i <= end_ptr; i++) {
    m = messages[i];
    if (!m || !IS_MESSAGE (m))
      return ERROR;
    if (n) {
      if (m -> tokentype != tokentype)
	return i;
    } else {
      if (m -> tokentype == tokentype)
	return i;
    }
  }
  return ERROR;
}

/*
 *  Recursively evaluate subexpressions within parentheses by
 *  calling the function with stack indexes to the subexpression,
 *  excluding the parentheses.
 */
int macro_subexpr (MESSAGE **messages, int start_ptr, int *end_ptr, 
		   VAL *result) {

  int i, 
    inner_start, 
    inner_end, 
    macro_result_type,
    subexpr_end;

#ifdef MACRO_STACK_TRACE
  debug ("---------------------------------");
  debug ("macro subexpr start %d, end %d.", start_ptr, end_ptr);

  for (i = start_ptr; i >= end_ptr; i--) 
    debug ("%d. %s.", i, messages[i] -> name);
#endif

  /* Look for inner sets of parentheses and evaluate
     those expressions first. */
  inner_start = start_ptr;
  while (1) {
    if ((inner_start = 
	 scanforward (messages, inner_start - 1, *end_ptr, OPENPAREN)) != -1) {
      if ((inner_end = 
	   match_paren (messages, inner_start, *end_ptr + 1)) != -1) {
	
	if (inner_start == ERROR || inner_end == ERROR)
	  goto outer;

	(void)macro_subexpr (messages, inner_start, &inner_end, 
				      result);

      }
    }
    if (inner_start == ERROR || inner_end == ERROR)
      goto outer;

    inner_start = inner_end;
  }

 outer:

  subexpr_end = *end_ptr + 1;
  macro_result_type = macro_sub_parse (messages, start_ptr - 1, &subexpr_end, 
				       result);
  *end_ptr = subexpr_end - 1;

  for (i = start_ptr; i >= *end_ptr; i--) {
    ++(messages[i] -> evaled);
    messages[i] -> tokentype = PREPROCESS_EVALED;
    m_print_val (messages[i], result);
  }

  return macro_result_type;
}

/* 
 * Set the token value of a subexpression from any token in the
 * subexpression.  A subexpression begins and ends when its
 * token type is PREPROCESS_EVALED, and its value is the original
 * value.  
 * TO DO - Generalize to use RESULT token if called from the main
 * parser, and try to verify whether checking for the same value 
 * of a series subexpression tokens is too restrictive.
 */

int set_subexpr_val (MESSAGE_STACK messages, int msg_ptr, VAL *val) {

  int i,
    stack_end,
    stackstart;
  char startval[MAXMSG];

  if (!messages[msg_ptr] || !IS_MESSAGE (messages[msg_ptr]))
    error (messages[msg_ptr], "Invalid message in set_subexpr_val.");

  stack_end = get_stack_top (messages);
  stackstart = stack_start (messages);

  strcpy (startval, messages[msg_ptr] -> value);

  for (i = msg_ptr; i <= stackstart; i++) {
    if (!messages[i] || !IS_MESSAGE (messages[i]))
      break;
    if (((messages[i] -> tokentype != PREPROCESS_EVALED) &&
	 (messages[i] -> tokentype != RESULT)) ||
	strcmp (messages[i] -> value, startval))
      break;
    m_print_val (messages[i], val);
  }

  for (i = msg_ptr - 1; i >= stack_end; i--) {
    if (!messages[i] || !IS_MESSAGE (messages[i]))
      break;
    if (((messages[i] -> tokentype != PREPROCESS_EVALED) &&
	 (messages[i] -> tokentype != RESULT)) ||
	strcmp (messages[i] -> value, startval))
      break;
    m_print_val (messages[i], val);
  }

  return SUCCESS;
}

int constant_subexpr (MESSAGE_STACK messages, int start, int *end, 
		      VAL *result) {

  int i,
    subexpr_tok,
    subexpr_end;

  if ((messages[start] -> tokentype != OPENPAREN) &&
      (messages[start] -> tokentype != CLOSEPAREN)) {
    parse_exception = mismatched_paren_x;
    result -> __type = INTEGER_T;
    result -> __value.__i = FALSE;
    return FALSE;
  }

  subexpr_end = *end + 1;

  eval_constant_expr (messages, start - 1, &subexpr_end, result);

  *end = subexpr_end - 1;

  subexpr_tok = PREPROCESS_EVALED;
    
  for (i = start; i >= *end; i--) {
    ++(messages[i] -> evaled);
    messages[i] -> tokentype = subexpr_tok;
    m_print_val (messages[i], result);
  }

  return is_val_true (result);
}
