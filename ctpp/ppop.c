/* $Id: ppop.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
 *  Functions that handle C language operators.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "ctpp.h"
#include "typeof.h"

extern EXCEPTION parse_exception;

/*
 *  Operators have a binary context if they are preceded 
 *  by non-whitespace tokens that are not operators, unary
 *  otherwise.
 */

OP_CONTEXT op_context (MESSAGE_STACK messages, int op_ptr) {

  MESSAGE *m_op;
  int lookahead,
    lookback;
  int op_res;
  OP_CONTEXT context;

  m_op = messages[op_ptr];

#if 0
  /***/
  if (!IS_C_OP (m_op -> tokentype))
    return op_not_an_op;
#endif  

  /* Just return unary here - checking for mismatched
     parentheses is done elsewhere. */
  if ((m_op -> tokentype == OPENPAREN) ||
      (m_op -> tokentype == CLOSEPAREN)) {
    context = op_unary_context;
    return context;
  }

  if ((op_res = operands (messages, op_ptr, &lookback, &lookahead))
      == ERROR)
    return op_not_an_op;

  context = op_binary_context;

  if (lookahead == ERROR) {
    warning (m_op, "Parse error.");
    parse_exception = parse_error_x;
    return op_null_context;
  }

  /* Typecast. */
  if (messages[lookahead] -> name[0] == ')')
    return op_cast_context;

  /* Evaluating how an operator binds is determined by whether the 
     context is unary or binary, and then evaluating in order of 
     operator precedence. */
  if (lookback == ERROR) {
    context = op_unary_context;
  } else {
    if (IS_C_OP_TOKEN_NOEVAL (messages[lookback] -> tokentype) || 
	is_c_keyword (messages[lookback] -> name) ||
	is_macro_keyword (messages[lookback] -> name))
      context = op_unary_context;
  }
  return context;
}


/*
 *  Return true if an operator's expression should be evaluated at
 *  this precedence level.
 *
 *  Here is the order of C language operator precedence.
 *
 *   0. Postfix (), [], ++ -- 
 *   1. unary +, -, !; ~ ++, (cast), & (address of), sizeof ()
 *   2. *, /, % (multiplicative)
 *   3. +, - (additive)
 *   4. <<, >>
 *   5. <, <=, >, >=
 *   6. ==, !=
 *   7. & (bitwise and)
 *   8. ^ (bitwise xor), | (bitwise or)
 *   9. && (logical and)
 *   10. || (logical or)
 *   11. ? : (conditional)
 *   12. Assignment ops.
 *   13. , (comma)
 */

int op_precedence (int precedence, MESSAGE_STACK messages, int op_ptr) {

  int retval = FALSE;
  MESSAGE *m_op;
  OP_CONTEXT context = op_not_an_op;

  m_op = messages[op_ptr];

#if 0
  /***/
  if (!IS_C_OP (m_op -> tokentype))
    return FALSE;
#endif  

  switch (precedence)
    {
    case 0:
      switch (m_op -> tokentype)
	{
	case OPENPAREN:
	case CLOSEPAREN:
	case ARRAYOPEN:
	case ARRAYCLOSE:
	case INCREMENT:
	case DECREMENT:
	case DEREF:
	  retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 1:
      switch (m_op -> tokentype) 
	{
	case SIZEOF:
	case EXCLAM:
	  retval = TRUE;
	  break;
	case PLUS:      
	case MINUS:     
	case ASTERISK:  
	case AMPERSAND:
	  if ((context = op_context (messages, op_ptr)) == op_unary_context)
	    retval = TRUE;
	default: 
	  break;
	}
      break;
    case 2:
      if ((m_op -> tokentype == DIVIDE) ||
	  (m_op -> tokentype == MODULUS))
	retval = TRUE;
      if ((m_op -> tokentype == ASTERISK) && 
	  ((context = op_context (messages, op_ptr)) == op_binary_context))
	retval = TRUE;
      break;
    case 3:
      if ((m_op -> tokentype == PLUS) ||
	  (m_op -> tokentype == MINUS))
	retval = TRUE;
      break;
    case 4:
      if ((m_op -> tokentype == ASL) ||
	  (m_op -> tokentype == ASR))
	retval = TRUE;
      break;
    case 5:
      if ((m_op -> tokentype == LT) ||
	  (m_op -> tokentype == LE) ||
	  (m_op -> tokentype == GT) ||
	  (m_op -> tokentype == GE))
	retval = TRUE;
      break;
    case 6:
      if ((m_op -> tokentype == BOOLEAN_EQ) ||
	  (m_op -> tokentype == INEQUALITY))
	retval = TRUE;
      break;
    case 7:
      if (m_op -> tokentype == BIT_AND)
	retval = TRUE;
      break;
    case 8:
      if ((m_op -> tokentype == BIT_OR) || 
	  (m_op -> tokentype == BIT_XOR))
	retval = TRUE;
      break;
    case 9:
      if (m_op -> tokentype == BOOLEAN_AND)
	retval = TRUE;
      break;
    case 10:
      if (m_op -> tokentype == BOOLEAN_OR)
	retval = TRUE;
      break;
    case 11:
      if ((m_op -> tokentype == CONDITIONAL) ||
	  (m_op -> tokentype == COLON))
	retval = TRUE;
    case 12:
      if ((m_op -> tokentype == EQ) ||
	  (m_op -> tokentype == ASR_ASSIGN) ||
	  (m_op -> tokentype == ASL_ASSIGN) ||
	  (m_op -> tokentype == PLUS_ASSIGN) ||
	  (m_op -> tokentype == MINUS_ASSIGN) ||
	  (m_op -> tokentype == MULT_ASSIGN) ||
	  (m_op -> tokentype == DIV_ASSIGN) ||
	  (m_op -> tokentype == BIT_AND_ASSIGN) ||
	  (m_op -> tokentype == BIT_OR_ASSIGN) ||
	  (m_op -> tokentype == BIT_XOR_ASSIGN))
	retval = TRUE;
      break;
    case 13:
      if (m_op -> tokentype == ARGSEPARATOR)
	retval = TRUE;
      break;
    default:
      break;
    }

  return retval;
}

/*
 *  Perform syntax checking here.  Returns error if the 
 *  message is not an operator.
 */

int operands (MESSAGE_STACK messages, int op_ptr, int *op1_ptr, int *op2_ptr) {

  int i,
    stack_top,
    stackstart;
  MESSAGE *m, *m_op;

  stack_top = get_stack_top (messages);
  stackstart = stack_start (messages);
  
  m_op = messages[op_ptr];

  if (!IS_C_OP (m_op -> tokentype))
    return ERROR;

  for (i = op_ptr + 1, *op1_ptr = ERROR; 
       (i <= stackstart) && (*op1_ptr == ERROR); i++) {

    m = messages[i];

    if (m -> tokentype == WHITESPACE)
      continue;

    if ((IS_C_OP_TOKEN_NOEVAL (m -> tokentype)) &&
	!IS_C_UNARY_OP (m_op -> tokentype)) {
      warning (m, "Invalid operand to %s.", m_op -> name);
      parse_exception = parse_error_x;
      return ERROR;
    }

    if (m -> tokentype == NEWLINE)
      goto op1_done;

    *op1_ptr = i;
  }

 op1_done:

  for (i = op_ptr - 1, *op2_ptr = ERROR; 
       (i >= stack_top) && (*op2_ptr == ERROR); i--) {

    m = messages[i];

    if (m -> tokentype == WHITESPACE)
      continue;

    /* It should not be necessary to check for a unary
       op in the operand because it will already have been
       evaluated because the unary op has a higher precedence. */

    if ((m -> name[0] == '\\') &&
	(messages[i+1] -> tokentype != NEWLINE)) 
      return FALSE;
    
    *op2_ptr = i;
  }

  return FALSE;
}
