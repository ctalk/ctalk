/* $Id: rt_op.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016, 2019  Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

/*
 *  Return true if an operator's expression should be evaluated at
 *  this precedence level.
 *
 *  Here is the order of C language operator precedence.
 *
 *   0. Postfix (), [], ++ -- , . ->
 *   1. unary +, -, !; ~ ++, *, (cast), & (address of), sizeof ()
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

/*
 *  Perform syntax checking here.  Returns error if the 
 *  message is not an operator or an operand is invalid
 *  for the operator.
 */

extern DEFAULTCLASSCACHE *rt_defclasses;

int _rt_operands (EXPR_PARSER *p, int op_ptr, int *op1_ptr, 
		  int *op2_ptr) {

  int i;
  MESSAGE *m, *m_op;

  if (!IS_C_OP (M_TOK(p -> m_s[op_ptr])))
    return ERROR;

  m_op = p -> m_s[op_ptr];

  for (i = op_ptr + 1, *op1_ptr = ERROR; 
       (i <= p -> msg_frame_start) && (*op1_ptr == ERROR); i++) {

    if (M_ISSPACE(p -> m_s[i]))
      continue;

    m = p -> m_s[i];

    if ((IS_C_OP_TOKEN_NOEVAL (m -> tokentype)) &&
	!IS_C_UNARY_MATH_OP (m_op -> tokentype)) {
      return ERROR;
    }

    *op1_ptr = i;
  }

  for (i = op_ptr - 1, *op2_ptr = ERROR; 
       (i >= p -> msg_frame_top) && (*op2_ptr == ERROR); i--) {

    if (M_ISSPACE(p -> m_s[i]))
      continue;

    *op2_ptr = i;
  }

  return FALSE;
}

int _rt_unary_operand (EXPR_PARSER *p, int op_ptr, int *op1_ptr) {
#if 0
int _rt_unary_operand (MESSAGE_STACK messages, int op_ptr, int *op1_ptr,
		       int stack_top, int stack_start) {
#endif  
  int i;
  MESSAGE *m;

  for (i = op_ptr - 1, *op1_ptr = ERROR; 
       (i <= p -> msg_frame_start) && (*op1_ptr == ERROR); i--) {

    if (M_ISSPACE(p -> m_s[i]))
      continue;
    *op1_ptr = i;
  }

  if (*op1_ptr == ERROR)
    return ERROR;

  return SUCCESS;
}

/*
 *  Operations for logical negation (!) and bit comp (~) operators.
 *  If we encounter a pointer, treat it as a long int.
 */

OBJECT *_rt_unary_math (MESSAGE_STACK messages, int op_ptr, 
			OBJECT *operand_value) {
  VAL value;
  OBJECT *result_object = NULL;    /* Avoid a warning. */
  int int_r, ptr_r;
  long long int longlong_r;
  double double_r;
#ifndef __APPLE__
  long double longdouble_r;
#endif
  char buf[MAXMSG];

  numeric_value (operand_value -> __o_value, &value);

  switch (messages[op_ptr] -> tokentype) 
    {
    case LOG_NEG:
      switch (value.__type)
	{
	case INTEGER_T:
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_integer_class,
	     ARG_VAR, "");
	  SETINTVARS(result_object, (!value.__value.__i));
	  break;
	case LONG_T:
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_integer_class,
	     ARG_VAR, "");
	  SETINTVARS(result_object, (!value.__value.__l));
	  break;
	case LONGLONG_T:
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_longinteger_class,
	     ARG_VAR, "");
	  *(long long *)result_object -> __o_value =
	    *(long long *)result_object -> instancevars -> __o_value =
	    !value.__value.__ll;
	  break;
	case DOUBLE_T:
	case FLOAT_T:
	  double_r = ! value.__value.__d;
# if defined (__sparc__) && defined (__svr4__)
	  sprintf (buf, "%f", double_r);
#else
	  sprintf (buf, "%lf", double_r);
#endif
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_float_class,
	     ARG_VAR, buf);
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  longdouble_r = ! value.__value.__ld;
	  sprintf (buf, "%Lf", longdouble_r);
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_float_class,
	     ARG_VAR, buf);
	  break;
#endif
	case PTR_T:
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_integer_class,
	     ARG_VAR, buf);
	  SETINTVARS(result_object, ((unsigned long)!value.__value.__ptr));
	  break;
	default:
	  _warning ("Invalid operand in _rt_unary_math ().\n");
	  break;
	}
      break;
    case BIT_COMP:
      switch (value.__type)
	{
	case INTEGER_T:
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_integer_class,
	     ARG_VAR, "");
	  SETINTVARS(result_object, (~value.__value.__i));
	  break;
	case LONG_T:
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_integer_class,
	     ARG_VAR, "");
	  SETINTVARS(result_object, (~value.__value.__l));
	  break;
	case LONGLONG_T:
	  result_object =
	    create_object_init_internal
	    ("result", rt_defclasses -> p_longinteger_class,
	     ARG_VAR, "");
	  SETINTVARS(result_object, (~value.__value.__ll));
	  break;
	default:
	  _warning ("Invalid operand in _rt_unary_math ().\n");
	  break;
	}
      break;
    default:
      _warning ("Undefined operator in _rt_unary_math ().\n");
      result_object = NULL;
      break;
    } /* switch (messages[op_ptr] -> tokentype) */

  return result_object;
}

