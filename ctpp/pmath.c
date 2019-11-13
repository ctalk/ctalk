/* $Id: pmath.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2014, 2018 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "ctpp.h"
#include "typeof.h"

extern EXCEPTION parse_exception;   /* Declared in pexcept.c.     */

/*
 *  Evaluate operators that return true or false: ==, <, >, <=, 
 *  and >=.
 */

#define OPS_DEFINED (op1_val.__type && op2_val.__type)

bool eval_bool (MESSAGE_STACK messages, int op_ptr, VAL *result) {

  MESSAGE *m_op = NULL, 
    *m_op1 = NULL, 
    *m_op2 = NULL;
  VAL op1_val, op2_val;
  bool ret_value = False;
  int op_res, op1_ptr, op2_ptr;
  int i;
  OP_CONTEXT context;

  m_op  = messages[op_ptr];

  if ((op_res = operands (messages, op_ptr, &op1_ptr, &op2_ptr))
      == ERROR) {
    warning (m_op, "Invalid operand to %s.", m_op -> name);
    parse_exception = parse_error_x;
    result -> __type = INTEGER_T;
    result -> __value.__i = FALSE;
    return FALSE;
  }
  
  context = op_context (messages, op_ptr);

  switch (context)
    {
    case op_not_an_op:
    case op_cast_context:
    case op_null_context:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      location_trace (m_op);
      parse_exception = parse_error_x;
      result -> __type = INTEGER_T;
      result -> __value.__i = FALSE;
      return FALSE;
      break;
    case op_unary_context:
      m_op2 = messages[op2_ptr];
      numeric_value (m_op2 -> value, &op2_val);
      break;
    case op_binary_context:
      m_op1 = messages[op1_ptr];
      m_op2 = messages[op2_ptr];
      numeric_value (m_op1 -> value, &op1_val);
      numeric_value (m_op2 -> value, &op2_val);
      break;
    }

  if (context == op_binary_context) {
    if (pmatch_type (&op1_val, &op2_val) == ERROR)
      warning (m_op1, "Type mismatch: %s, %s.", m_op1 -> name, m_op2 -> name);

    if (!IS_C_TYPE (op1_val.__type))
      warning (m_op1, "Undefined C type %d in bool_arithmetic.", op1_val.__type);
    if (!IS_C_TYPE (op2_val.__type))
      warning (m_op2, "Undefined C type %d in bool_arithmetic.", op2_val.__type);
  } else {
    if (context == op_unary_context) {
      if (!IS_C_TYPE (op2_val.__type))
	warning (m_op2, "Undefined C type %d in bool_arithmetic.", 
		 op2_val.__type);
    } else {
      warning (m_op, "Bad context for operator %s.", m_op -> name);
    }
  }
     
   switch (m_op -> tokentype) 
     {
     case EQ:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   ret_value = (OPS_DEFINED && 
			(op1_val.__value.__ptr == op2_val.__value.__ptr) ? 
			True : False);
	   break;
	 case INTEGER_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__i == op2_val.__value.__i) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__u == op2_val.__value.__u) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__d == op2_val.__value.__d) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = (OPS_DEFINED && 
			(op1_val.__value.__ld == op2_val.__value.__ld) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__l == op2_val.__value.__l) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ul == op2_val.__value.__ul) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ll == op2_val.__value.__ll) ? True : False);
	   break;
	 }
       break;
     case GE:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ptr >= op2_val.__value.__ptr) ? 
			True : False);
	   break;
	 case INTEGER_T:
	   ret_value = (OPS_DEFINED && 
			(op1_val.__value.__i >= op2_val.__value.__i) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = (OPS_DEFINED && 
			(op1_val.__value.__u >= op2_val.__value.__u) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = (OPS_DEFINED && 
			(op1_val.__value.__d >= op2_val.__value.__d) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = (OPS_DEFINED && 
			(op1_val.__value.__ld >= op2_val.__value.__ld) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = (OPS_DEFINED &&
	     (op1_val.__value.__l >= op2_val.__value.__l) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = (OPS_DEFINED &&
	     (op1_val.__value.__ul >= op2_val.__value.__ul) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ll >= op2_val.__value.__ll) ? True : False);
	   break;
	 }
       break;
     case LE:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ptr <= op2_val.__value.__ptr) ? 
			True : False);
	   break;
	 case INTEGER_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__i <= op2_val.__value.__i) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__u <= op2_val.__value.__u) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__d <= op2_val.__value.__d) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ld <= op2_val.__value.__ld) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__l <= op2_val.__value.__l) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ul <= op2_val.__value.__ul) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ll <= op2_val.__value.__ll) ? True : False);
	   break;
	 }
       break;
     case GT:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ptr > op2_val.__value.__ptr) ? 
			True : False);
	   break;
	 case INTEGER_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__i > op2_val.__value.__i) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__u > op2_val.__value.__u) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__d > op2_val.__value.__d) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ld > op2_val.__value.__ld) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__l > op2_val.__value.__l) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ul > op2_val.__value.__ul) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ll > op2_val.__value.__ll) ? True : False);
	   break;
	 }
       break;
     case LT:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ptr < op2_val.__value.__ptr) ? 
			True : False);
	   break;
	 case INTEGER_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__i < op2_val.__value.__i) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__u < op2_val.__value.__u) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__d < op2_val.__value.__d) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ld < op2_val.__value.__ld) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__l < op2_val.__value.__l) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ul < op2_val.__value.__ul) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = (OPS_DEFINED &&
			(op1_val.__value.__ll < op2_val.__value.__ll) ? True : False);
	   break;
	 }
       break;
     case BOOLEAN_EQ:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   if (op1_val.__value.__ptr && op2_val.__value.__ptr) {
	     ret_value = (!strcmp ((char *)op1_val.__value.__ptr, 
				   (char *)op2_val.__value.__ptr)) ? True : False;
	   } else {
	     ret_value = False;
	   }
	   break;
	 case INTEGER_T:
	   ret_value = ((OPS_DEFINED &&
			 (op1_val.__value.__i == op2_val.__value.__i)) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = ((OPS_DEFINED &&
			 (op1_val.__value.__u == op2_val.__value.__u)) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__d == op2_val.__value.__d)) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ld == op2_val.__value.__ld)) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__l == op2_val.__value.__l)) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ul == op2_val.__value.__ul)) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ll == op2_val.__value.__ll)) ? True : False);
	   break;
	 }
       break;
     case INEQUALITY:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ptr != op2_val.__value.__ptr)) ? 
			True : False);
	   break;
	 case INTEGER_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__i != op2_val.__value.__i)) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__u != op2_val.__value.__u)) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__d != op2_val.__value.__d)) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ld != op2_val.__value.__ld)) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__l != op2_val.__value.__l)) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ul != op2_val.__value.__ul)) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ll != op2_val.__value.__ll)) ? True : False);
	   break;
	 }
       break;
     case BOOLEAN_AND:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   ret_value = ((OPS_DEFINED && 
			(op1_val.__value.__ptr && op2_val.__value.__ptr)) ? 
			True : False);
	   break;
	 case INTEGER_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__i && op2_val.__value.__i)) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__u && op2_val.__value.__u)) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__d && op2_val.__value.__d)) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ld && op2_val.__value.__ld)) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__l && op2_val.__value.__l)) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ul && op2_val.__value.__ul)) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = ((OPS_DEFINED &&
			(op1_val.__value.__ll && op2_val.__value.__ll)) ? True : False);
	   break;
	 }
       break;
     case BOOLEAN_OR:
       switch (op1_val.__type)
	 {
	 case PTR_T:
	   ret_value = ((OPS_DEFINED &&
	     (op1_val.__value.__ptr || op2_val.__value.__ptr)) ? True : False);
	   break;
	 case INTEGER_T:
	   ret_value = ((OPS_DEFINED &&
	     (op1_val.__value.__i || op2_val.__value.__i)) ? True : False);
	   break;
	 case UINTEGER_T:
	   ret_value = ((OPS_DEFINED &&
	     (op1_val.__value.__u || op2_val.__value.__u)) ? True : False);
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = ((OPS_DEFINED &&
	     (op1_val.__value.__d || op2_val.__value.__d)) ? True : False);
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = ((OPS_DEFINED &&
	     (op1_val.__value.__ld || op2_val.__value.__ld)) ? True : False);
	   break;
#endif
	 case LONG_T:
	   ret_value = ((OPS_DEFINED &&
	     (op1_val.__value.__l || op2_val.__value.__l)) ? True : False);
	   break;
	 case ULONG_T:
	   ret_value = ((OPS_DEFINED &&
	     (op1_val.__value.__ul || op2_val.__value.__ul)) ? True : False);
	   break;
	 case LONGLONG_T:
	   ret_value = ((OPS_DEFINED &&
	     (op1_val.__value.__ll || op2_val.__value.__ll)) ? True : False);
	   break;
	 }
       break;
     case LOG_NEG:
       switch (op2_val.__type)
	 {
	 case PTR_T:
	   ret_value = (op2_val.__value.__ptr) ? False : True;
	   break;
	 case INTEGER_T:
	   ret_value = (op2_val.__value.__i) ? False : True;
	   break;
	 case UINTEGER_T:
	   ret_value = (op2_val.__value.__u) ? False : True;
	   break;
	 case FLOAT_T:
	 case DOUBLE_T:
	   ret_value = (op2_val.__value.__d) ? False : True;
	   break;
#ifndef __APPLE__
	 case LONGDOUBLE_T:
	   ret_value = (op2_val.__value.__ld) ? False : True;
	   break;
#endif
	 case LONG_T:
	   ret_value = (op2_val.__value.__l) ? False : True;
	   break;
	 case ULONG_T:
	   ret_value = (op2_val.__value.__ul) ? False : True;
	   break;
	 case LONGLONG_T:
	   ret_value = (op2_val.__value.__ll) ? False : True;
	   break;
	 }
       break;
     default:
       break;
     }
   
   result -> __type = INTEGER_T;
   result -> __value.__i = (ret_value == True) ? TRUE : FALSE;
   
   if (context == op_unary_context) {
     if ((m_op2 -> tokentype == RESULT) ||
	 (m_op2 -> tokentype == PREPROCESS_EVALED))
       set_subexpr_val (messages, op2_ptr, result);
     for (i = op_ptr; i >= op2_ptr; i--) {
       ++messages[i] -> evaled;
       m_print_val (messages[i], result);
       messages[i] -> tokentype = RESULT;
     }
   } else {
     if ((m_op1 -> tokentype == RESULT) ||
	 (m_op1 -> tokentype == PREPROCESS_EVALED))
       set_subexpr_val (messages, op1_ptr, result);
     if ((m_op2 -> tokentype == RESULT) ||
	 (m_op2 -> tokentype == PREPROCESS_EVALED))
       set_subexpr_val (messages, op2_ptr, result);
     for (i = op1_ptr; i >= op2_ptr; i--) {
       ++messages[i] -> evaled;
       m_print_val (messages[i], result);
       messages[i] -> tokentype = RESULT;
     }
   }

   return ret_value;
}

/*
 *  Evaluate operators that calculate numeric values: +, -, *, /,
 *  >>, <<...
 */

int eval_math (MESSAGE_STACK messages, int op_ptr, VAL *result) {

  MESSAGE *m_op = NULL, 
    *m_op1 = NULL, 
    *m_op2 = NULL;
  int op1_ptr, op2_ptr;
  int op_res;
  int i;
  VAL op1_val, op2_val;
  OP_CONTEXT context;

  m_op = messages[op_ptr];

  if ((op_res = operands (messages, op_ptr, &op1_ptr, &op2_ptr))
      == ERROR) {
    warning (m_op, "Invalid operand to %s.", m_op -> name);
    parse_exception = parse_error_x;
    result -> __type = INTEGER_T;
    result -> __value.__i = FALSE;
    return FALSE;
  }
  
  context = op_context (messages, op_ptr);

  /* 
   * In each case, evaluate the operands in case they were not 
   * evaluated previously.  Subexpressions (in parentheses) 
   * should already have been evaluated.
   */

  switch (context)
    {
    case op_not_an_op:
    case op_null_context:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      parse_exception = parse_error_x;
      result -> __type = INTEGER_T;
      result -> __value.__i = FALSE;
      return FALSE;
      break;
    case op_cast_context:
      return FALSE;
    case op_unary_context:
      m_op2 = messages[op2_ptr];
      numeric_value (m_op2 -> value, &op2_val);
      break;
    case op_binary_context:
      m_op1 = messages[op1_ptr];
      if ((m_op1 -> tokentype != RESULT) &&
	  (m_op1 -> tokentype != PREPROCESS_EVALED)) {
	resolve_symbol (m_op1 -> name, &op1_val);
	if ((op1_val.__type == PTR_T) &&
	    (op1_val.__value.__ptr != NULL))
	  m_print_val (m_op1, &op1_val);
      } else {
	numeric_value (m_op1 -> value, &op1_val);
      }
      m_op2 = messages[op2_ptr];
      if ((m_op2 -> tokentype != RESULT) &&
	  (m_op2 -> tokentype != PREPROCESS_EVALED)) {
	resolve_symbol (m_op2 -> name, &op2_val);
	if ((op2_val.__type == PTR_T) &&
	    (op2_val.__value.__ptr != NULL))
	  m_print_val (m_op2, &op2_val);
      } else {
	numeric_value (m_op2 -> value, &op2_val);
      }
      break;
    }

  switch (m_op -> tokentype)
    {
      /* Promote to double if either operand is a double,
	 otherwise, promote to largest type. */
    case PLUS:
      perform_add (m_op, &op1_val, &op2_val, result);
      break;
    case MINUS:
      perform_subtract (m_op, &op1_val, &op2_val, result);
      break;
    case MULT:
      perform_multiply (m_op, &op1_val, &op2_val, result);
      break;
    case DIVIDE:
      perform_divide (m_op, &op1_val, &op2_val, result);
      break;
      /* Error if op2 is not an integer type. */
    case ASL:
      perform_asl (m_op, &op1_val, &op2_val, result);
      break;
    case ASR:
      perform_asr (m_op, &op1_val, &op2_val, result);
      break;
    case BIT_AND:
      perform_bit_and (m_op, &op1_val, &op2_val, result);
      break;
    case BIT_OR:
      perform_bit_or (m_op, &op1_val, &op2_val, result);
      break;
    case BIT_XOR:
      perform_bit_xor (m_op, &op1_val, &op2_val, result);
      break;
      /* Unary op. */
    case BIT_COMP:
      perform_bit_comp (m_op, &op2_val, result);
      break;
    default:
      break;
    }

  if (parse_exception != success_x)
    return ERROR;

   if (context == op_unary_context) {
     if ((m_op2 -> tokentype == RESULT) ||
	 (m_op2 -> tokentype == PREPROCESS_EVALED))
       set_subexpr_val (messages, op2_ptr, result);
     for (i = op_ptr; i >= op2_ptr; i--) {
       ++messages[i] -> evaled;
       m_print_val (messages[i], result);
       messages[i] -> tokentype = RESULT;
     }
   } else {
     if ((m_op1 -> tokentype == RESULT) ||
	 (m_op1 -> tokentype == PREPROCESS_EVALED))
       set_subexpr_val (messages, op1_ptr, result);
     if ((m_op2 -> tokentype == RESULT) ||
	 (m_op2 -> tokentype == PREPROCESS_EVALED))
       set_subexpr_val (messages, op2_ptr, result);
     for (i = op1_ptr; i >= op2_ptr; i--) {
       ++messages[i] -> evaled;
       m_print_val (messages[i], result);
       messages[i] -> tokentype = RESULT;
     }
   }

   return result -> __type;
}

int perform_add (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) {

  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i + op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i + op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i + op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__i + op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i + op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__i + op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__i + op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
#ifdef __x86_64
	  /***/
	  result -> __value.__ptr = (void *)
            (long long int)op1 -> __value.__i +
            ((op2 -> __value.__ptr == NULL) ? 0 :
             atoll((char *)op2 -> __value.__ptr));
#else	  
	  result -> __value.__ptr = (void *)
            op1 -> __value.__i +
            ((op2 -> __value.__ptr == NULL) ? 0 :
             atoi((char *)op2 -> __value.__ptr));
#endif	  
          result -> __type = INTEGER_T;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_add.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__u = op1 -> __value.__u + op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__u = op1 -> __value.__u + op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u + op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u + op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u + op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__u + op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__i + op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
#ifdef __x86_64
	  result -> __value.__ptr = (void *)
            (long long int)op1 -> __value.__i +
            ((op2 -> __value.__ptr == NULL) ? 0 :
             atoll((char *)op2 -> __value.__ptr));
#else	  
	  result -> __value.__ptr = (void *)
            op1 -> __value.__i +
            ((op2 -> __value.__ptr == NULL) ? 0 :
             atoi((char *)op2 -> __value.__ptr));
#endif	  
          result -> __type = INTEGER_T;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_add.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l + op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__l = op1 -> __value.__l + op2 -> __value.__u;
	  result -> __type = LONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l + op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l + op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l + op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__l + op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__l + op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  result -> __value.__ptr = op1 -> __value.__l + op2 -> __value.__ptr;
	  result -> __type = PTR_T;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_add.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l + op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l + op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l + op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l + op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l + op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__l + op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__l + op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  result -> __value.__ptr = op1 -> __value.__l + op2 -> __value.__ptr;
	  result -> __type = PTR_T;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_add.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll + op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll + op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll + op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll + op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll + op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__ll + op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__ll + op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  result -> __value.__ptr = op1 -> __value.__ll + op2 -> __value.__ptr;
	  result -> __type = PTR_T;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_add.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__d = op1 -> __value.__d + op2 -> __value.__i;
	  result -> __type = DOUBLE_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__d = op1 -> __value.__d + op2 -> __value.__u;
	  result -> __type = DOUBLE_T;
	  break;
	case LONG_T:
	  result -> __value.__d = op1 -> __value.__d + op2 -> __value.__l;
	  result -> __type = DOUBLE_T;
	  break;
	case ULONG_T:
	  result -> __value.__d = op1 -> __value.__d + op2 -> __value.__ul;
	  result -> __type = DOUBLE_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__d = op1 -> __value.__d + op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__d + op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__d + op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_add.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
#ifndef __APPLE__
    case LONGDOUBLE_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ld = op1 -> __value.__ld + op2 -> __value.__i;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ld = op1 -> __value.__ld + op2 -> __value.__u;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONG_T:
	  result -> __value.__ld = op1 -> __value.__ld + op2 -> __value.__l;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case ULONG_T:
	  result -> __value.__ld = op1 -> __value.__ld + op2 -> __value.__ul;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ld = op1 -> __value.__ld + op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__ld + op2 -> __value.__d;
	  result -> __type = LONGDOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__ld + op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_add.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
#endif
    case PTR_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ptr = op1 -> __value.__ptr + op2 -> __value.__i;
	  break;
	case UINTEGER_T:
	  result -> __value.__ptr = op1 -> __value.__ptr + op2 -> __value.__u;
	  break;
	case LONG_T:
	  result -> __value.__ptr = op1 -> __value.__ptr + op2 -> __value.__l;
	  break;
	case ULONG_T:
	  result -> __value.__ptr = op1 -> __value.__ptr + op2 -> __value.__ul;
	  break;
	case LONGLONG_T:
	  result -> __value.__ptr = op1 -> __value.__ptr + op2 -> __value.__ll;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_add.");
	  location_trace (m_op);
#endif
	  break;
	}
      result -> __type = PTR_T;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_add.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

int perform_subtract (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) { 
  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i - op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i - op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i - op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__i - op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i - op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__i - op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__i - op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
/* 	  warning (m_op, "Invalid operand to %s.", m_op -> name); */
	  parse_exception = invalid_operand_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__u - op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__u - op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u - op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u - op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u - op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__u - op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__u - op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
/* 	  warning (m_op, "Invalid operand to %s.", m_op -> name); */
	  parse_exception = invalid_operand_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l - op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__l = op1 -> __value.__l - op2 -> __value.__u;
	  result -> __type = LONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l - op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__ul - op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l - op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__l - op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__l - op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
/* 	  warning (m_op, "Invalid operand to %s.", m_op -> name); */
/* 	  location_trace (m_op); */
	  parse_exception = invalid_operand_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l - op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l - op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l - op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__ul - op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l - op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__l - op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__l - op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  parse_exception = invalid_operand_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll - op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll - op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll - op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll - op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll - op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__ll - op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__ll - op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
/* 	  warning (m_op, "Invalid operand to %s.", m_op -> name); */
/* 	  location_trace (m_op); */
	  parse_exception = invalid_operand_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__d = op1 -> __value.__d - op2 -> __value.__i;
	  result -> __type = DOUBLE_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__d = op1 -> __value.__d - op2 -> __value.__u;
	  result -> __type = DOUBLE_T;
	  break;
	case LONG_T:
	  result -> __value.__d = op1 -> __value.__d - op2 -> __value.__l;
	  result -> __type = DOUBLE_T;
	  break;
	case ULONG_T:
	  result -> __value.__d = op1 -> __value.__d - op2 -> __value.__ul;
	  result -> __type = DOUBLE_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__d - op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__d - op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__d - op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
/* 	  warning (m_op, "Invalid operand to %s.", m_op -> name); */
/* 	  location_trace (m_op); */
	  parse_exception = invalid_operand_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
#ifndef __APPLE__
    case LONGDOUBLE_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ld = op1 -> __value.__ld - op2 -> __value.__i;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ld = op1 -> __value.__ld - op2 -> __value.__u;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONG_T:
	  result -> __value.__ld = op1 -> __value.__ld - op2 -> __value.__l;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case ULONG_T:
	  result -> __value.__ld = op1 -> __value.__ld - op2 -> __value.__ul;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ld = op1 -> __value.__ld - op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__ld - op2 -> __value.__d;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__ld - op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case PTR_T:
/* 	  warning (m_op, "Invalid operand to %s.", m_op -> name); */
 	  parse_exception = invalid_operand_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
#endif
    case PTR_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ptr = op1 -> __value.__ptr - op2 -> __value.__i;
	  break;
	case UINTEGER_T:
	  result -> __value.__ptr = op1 -> __value.__ptr - op2 -> __value.__u;
	  break;
	case LONG_T:
	  result -> __value.__ptr = op1 -> __value.__ptr - op2 -> __value.__l;
	  break;
	case ULONG_T:
	  result -> __value.__ptr = op1 -> __value.__ptr - op2 -> __value.__ul;
	  break;
	case LONGLONG_T:
	  result -> __value.__ptr = op1 -> __value.__ptr - op2 -> __value.__ll;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
/* 	  warning (m_op, "Invalid operand to %s.", m_op -> name); */
	  parse_exception = invalid_operand_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      result -> __type = PTR_T;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_subtract.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

int perform_multiply (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) { 
  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i * op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i * op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i * op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__i * op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i * op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__i * op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__i * op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_multiply.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__u * op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__u * op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u * op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u * op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u * op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__u * op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__u * op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_multiply.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l * op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__l = op1 -> __value.__l * op2 -> __value.__u;
	  result -> __type = LONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l * op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l * op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l * op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__l * op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__l * op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_multiply.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l * op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l * op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l * op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l * op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l * op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__l * op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__l * op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_multiply.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll * op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll * op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll * op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll * op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll * op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__ll * op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__ll * op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_multiply.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__d = op1 -> __value.__d * op2 -> __value.__i;
	  result -> __type = DOUBLE_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__d = op1 -> __value.__d * op2 -> __value.__u;
	  result -> __type = DOUBLE_T;
	  break;
	case LONG_T:
	  result -> __value.__d = op1 -> __value.__d * op2 -> __value.__l;
	  result -> __type = DOUBLE_T;
	  break;
	case ULONG_T:
	  result -> __value.__d = op1 -> __value.__d * op2 -> __value.__ul;
	  result -> __type = DOUBLE_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__d * op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__d * op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__d * op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_multiply.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
#ifndef __APPLE__
    case LONGDOUBLE_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ld = op1 -> __value.__ld * op2 -> __value.__i;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ld = op1 -> __value.__ld * op2 -> __value.__u;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONG_T:
	  result -> __value.__ld = op1 -> __value.__ld * op2 -> __value.__l;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case ULONG_T:
	  result -> __value.__ld = op1 -> __value.__ld * op2 -> __value.__ul;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ld = op1 -> __value.__ld * op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__ld * op2 -> __value.__d;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__ld * op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_multiply.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
#endif
    case PTR_T:
      warning (m_op, "Invalid operand to %s in perform_multiply.", 
	       m_op -> name);
      location_trace (m_op);
      parse_exception = parse_error_x;
      return ERROR;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_multiply.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

int perform_divide (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) { 

  /* Check first for division by zero. */
  switch (op2 -> __type)
    {
    case INTEGER_T:
      if (op2 -> __value.__i == 0) {
	warning (m_op, "Division by zero.");
	location_trace (m_op);
	parse_exception = parse_error_x;
	return FALSE;
      }
      break;
    case LONG_T:
      if (op2 -> __value.__l == 0) {
	warning (m_op, "Division by zero.");
	location_trace (m_op);
	parse_exception = parse_error_x;
	return FALSE;
      }
      break;
    case LONGLONG_T:
      if (op2 -> __value.__ll == 0) {
	warning (m_op, "Division by zero.");
	location_trace (m_op);
	parse_exception = parse_error_x;
	return FALSE;
      }
      break;
    case DOUBLE_T:
      if (op2 -> __value.__d == 0) {
	warning (m_op, "Division by zero.");
	location_trace (m_op);
	parse_exception = parse_error_x;
	return FALSE;
      }
      break;
#ifndef __APPLE__
    case LONGDOUBLE_T:
      if (op2 -> __value.__ld == 0) {
	warning (m_op, "Division by zero.");
	location_trace (m_op);
	parse_exception = parse_error_x;
	return FALSE;
      }
      break;
#endif
    case PTR_T:
      if (op2 -> __value.__ptr == 0) {
	warning (m_op, "Division by zero.");
	location_trace (m_op);
	parse_exception = parse_error_x;
	return FALSE;
      }
      break;
    }

  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i / op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i / op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i / op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__l = op1 -> __value.__i / op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i / op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__i / op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__i / op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_divide.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__u / op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__u / op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u / op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u / op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u / op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__u / op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__u / op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_divide.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l / op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__l = op1 -> __value.__l / op2 -> __value.__u;
	  result -> __type = LONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l / op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l / op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l / op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__l / op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__l / op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_divide.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l / op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l / op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l / op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l / op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l / op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__l / op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__l / op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_divide.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll / op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll / op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll / op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll / op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll / op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__ll / op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__ll / op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_divide.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__d = op1 -> __value.__d / op2 -> __value.__i;
	  result -> __type = DOUBLE_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__d = op1 -> __value.__d / op2 -> __value.__u;
	  result -> __type = DOUBLE_T;
	  break;
	case LONG_T:
	  result -> __value.__d = op1 -> __value.__d / op2 -> __value.__l;
	  result -> __type = DOUBLE_T;
	  break;
	case ULONG_T:
	  result -> __value.__d = op1 -> __value.__d / op2 -> __value.__ul;
	  result -> __type = DOUBLE_T;
	  break;
#ifdef __APPLE__
	case LONGLONG_T:
	  result -> __value.__d = op1 -> __value.__d / op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
#else
	case LONGLONG_T:
	  result -> __value.__ld = op1 -> __value.__d / op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__d / op2 -> __value.__d;
	  result -> __type = DOUBLE_T;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__d / op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
#endif
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_divide.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
#ifndef __APPLE__
    case LONGDOUBLE_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ld = op1 -> __value.__ld / op2 -> __value.__i;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ld = op1 -> __value.__ld / op2 -> __value.__u;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONG_T:
	  result -> __value.__ld = op1 -> __value.__ld / op2 -> __value.__l;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case ULONG_T:
	  result -> __value.__ld = op1 -> __value.__ld / op2 -> __value.__ul;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ld = op1 -> __value.__ld / op2 -> __value.__ll;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case DOUBLE_T:
	  result -> __value.__d = op1 -> __value.__ld / op2 -> __value.__d;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case LONGDOUBLE_T:
	  result -> __value.__ld = op1 -> __value.__ld / op2 -> __value.__ld;
	  result -> __type = LONGDOUBLE_T;
	  break;
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_divide.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
#endif
    case PTR_T:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      location_trace (m_op);
      parse_exception = parse_error_x;
      return ERROR;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_divide.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

int perform_asl (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) { 
  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i << op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i << op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i << op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__l = op1 -> __value.__i << op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i << op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG
	  debug ("Unimplemented operand 2 type in perform_asl.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__u << op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__u << op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u << op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u << op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u << op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG
	  debug ("Unimplemented operand 2 type in perform_asl.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l << op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__l = op1 -> __value.__l << op2 -> __value.__u;
	  result -> __type = LONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l << op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l << op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l << op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_asl.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l << op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l << op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l << op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l << op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l << op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_asl.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll << op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll << op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll << op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll << op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll << op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  location_trace (m_op);
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_asl.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
    case LONGDOUBLE_T:
    case PTR_T:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      location_trace (m_op);
      parse_exception = parse_error_x;
      return ERROR;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_add.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

int perform_asr (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) { 
  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i >> op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i >> op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i >> op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__i >> op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i >> op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__u >> op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__u >> op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u >> op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u >> op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u >> op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_subtract.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l >> op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__l = op1 -> __value.__l >> op2 -> __value.__u;
	  result -> __type = LONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l >> op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l >> op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l >> op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_asr.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l >> op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l >> op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l >> op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l >> op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l >> op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_asr.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll >> op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll >> op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll >> op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll >> op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll >> op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_asr.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
    case LONGDOUBLE_T:
    case PTR_T:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      parse_exception = parse_error_x;
      return ERROR;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_asr.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}


int perform_bit_and (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) { 
  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i & op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i & op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i & op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__i & op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i & op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_and.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__u & op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__u & op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u & op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u & op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u & op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_and.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l & op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__l = op1 -> __value.__l & op2 -> __value.__u;
	  result -> __type = LONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l & op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l & op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l & op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_and.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;

    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l & op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l & op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l & op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l & op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l & op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_and.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;


    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll & op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll & op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll & op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll & op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll & op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_and.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
    case LONGDOUBLE_T:
    case PTR_T:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      location_trace (m_op);
      parse_exception = parse_error_x;
      return ERROR;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_bit_and.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

int perform_bit_or (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) { 
  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i | op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i | op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i | op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__i | op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i | op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_or.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__u | op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__u | op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u | op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u | op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u | op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_or.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l | op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__l = op1 -> __value.__l | op2 -> __value.__u;
	  result -> __type = LONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l | op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l | op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l | op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_or.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;

    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l | op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l | op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l | op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l | op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l | op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_or.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;


    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll | op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll | op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll | op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll | op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll | op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_or.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
    case LONGDOUBLE_T:
    case PTR_T:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      location_trace (m_op);
      parse_exception = parse_error_x;
      return ERROR;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_bit_or.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

int perform_bit_xor (MESSAGE *m_op, VAL *op1, VAL *op2, VAL *result) {
  switch (op1 -> __type) 
    {
    case INTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__i ^ op2 -> __value.__i;
	  result -> __type = INTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__i = op1 -> __value.__i ^ op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__i ^ op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__i ^ op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__i ^ op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_xor.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case UINTEGER_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__i = op1 -> __value.__u ^ op2 -> __value.__i;
	  result -> __type = UINTEGER_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__u = op1 -> __value.__u ^ op2 -> __value.__u;
	  result -> __type = UINTEGER_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__u ^ op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__u ^ op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__u ^ op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_xor.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case LONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__l = op1 -> __value.__l ^ op2 -> __value.__i;
	  result -> __type = LONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l ^ op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__l = op1 -> __value.__l ^ op2 -> __value.__l;
	  result -> __type = LONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l ^ op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l ^ op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_xor.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;

    case ULONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l ^ op2 -> __value.__i;
	  result -> __type = ULONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ul = op1 -> __value.__l ^ op2 -> __value.__u;
	  result -> __type = ULONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ul = op1 -> __value.__l ^ op2 -> __value.__l;
	  result -> __type = ULONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ul = op1 -> __value.__l ^ op2 -> __value.__ul;
	  result -> __type = ULONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__l ^ op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  location_trace (m_op);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_xor.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;


    case LONGLONG_T:
      switch (op2 -> __type)
	{
	case INTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll ^ op2 -> __value.__i;
	  result -> __type = LONGLONG_T;
	  break;
	case UINTEGER_T:
	  result -> __value.__ll = op1 -> __value.__ll ^ op2 -> __value.__u;
	  result -> __type = LONGLONG_T;
	  break;
	case LONG_T:
	  result -> __value.__ll = op1 -> __value.__ll ^ op2 -> __value.__l;
	  result -> __type = LONGLONG_T;
	  break;
	case ULONG_T:
	  result -> __value.__ll = op1 -> __value.__ll ^ op2 -> __value.__ul;
	  result -> __type = LONGLONG_T;
	  break;
	case LONGLONG_T:
	  result -> __value.__ll = op1 -> __value.__ll ^ op2 -> __value.__ll;
	  result -> __type = LONGLONG_T;
	  break;
	case DOUBLE_T:
	case LONGDOUBLE_T:
	case PTR_T:
	  warning (m_op, "Invalid operand to %s.", m_op -> name);
	  parse_exception = parse_error_x;
	  return ERROR;
	  break;
	default:
#ifdef DEBUG_CODE
	  debug ("Unimplemented operand 2 type in perform_bit_xor.");
	  location_trace (m_op);
#endif
	  break;
	}
      break;
    case DOUBLE_T:
    case LONGDOUBLE_T:
    case PTR_T:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      parse_exception = parse_error_x;
      return ERROR;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_bit_xor.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

int perform_bit_comp (MESSAGE *m_op, VAL *op2, VAL *result) { 

  switch (op2 -> __type) 
    {
    case INTEGER_T:
      result -> __value.__i = ~op2 -> __value.__i;
      result -> __type = INTEGER_T;
      break;
    case UINTEGER_T:
      result -> __value.__u = ~op2 -> __value.__u;
      result -> __type = UINTEGER_T;
      break;
    case LONG_T:
      result -> __value.__l = ~op2 -> __value.__l;
      result -> __type = LONG_T;
      break;
    case ULONG_T:
      result -> __value.__ul = ~op2 -> __value.__ul;
      result -> __type = ULONG_T;
      break;
    case LONGLONG_T:
      result -> __value.__ll = ~op2 -> __value.__ll;
      result -> __type = LONGLONG_T;
      break;
    case DOUBLE_T:
    case LONGDOUBLE_T:
    case PTR_T:
      warning (m_op, "Invalid operand to %s.", m_op -> name);
      location_trace (m_op);
      parse_exception = parse_error_x;
      return ERROR;
      break;
    default:
#ifdef DEBUG_CODE
      debug ("Unimplemented operand 1 type in perform_bit_comp.");
      location_trace (m_op);
#endif
      break;
  }

  return is_val_true (result);
}

extern MESSAGE *v_messages[P_MESSAGES +1];  /* Temporary stack for varargs.   */
extern int v_message_ptr;
extern int v_message_push (MESSAGE *);

static inline void clear_v_messages (int stack_start, int stack_end) {
  int j;

  for (j = stack_end; j <= stack_start; j++) 
    /* __xfree (v_message_pop ()); */
    reuse_message (v_message_pop ());
}

/*
 *  Just retokenize the sub-expressions, because we have probably
 *  evaluated the entire statement at least once already.
 *  We put the tokens on the v_message stack.
 */
int retokenize_conditional (MESSAGE_STACK messages, 
			     int start_ptr, int *end_ptr) {
  char *expr_str;
  int v_stack_end;

  if ((expr_str = collect_tokens (messages, start_ptr, *end_ptr)) == NULL)
    return -1;

  v_stack_end = tokenize_reuse (v_message_push, expr_str);

  __xfree (expr_str);

  return v_stack_end;
}

bool question_conditional_eval (MESSAGE_STACK messages, 
				   int op_ptr, int *end_ptr, VAL *result) {

  int i;
  int stacktop, stack_end;
  int n_parens;
  int v_stacktop, v_stack_end;
  int pred_start_ptr, pred_end_ptr,
    true_start_ptr, true_end_ptr,
    false_start_ptr, false_end_ptr;
  int else_op_ptr;
  int eval_token;
  VAL pred_val;
  MESSAGE *m;

  memset ((void *)&pred_val, 0, sizeof (VAL));
  pred_val.__type = 1;
  else_op_ptr = ERROR;

  stacktop = stack_start (messages);
  stack_end = get_stack_top (messages);

  /* Find the beginning and end of the predicate. */

  for (i = op_ptr + 1, n_parens = 0, 
	 pred_start_ptr = ERROR, pred_end_ptr = ERROR; 
       (i < stacktop) && (pred_start_ptr == ERROR || pred_end_ptr == ERROR); 
       i++) {

    m = messages[i];
    
    if (m -> tokentype == WHITESPACE)
      continue;
    switch (m -> tokentype) 
      {
      case LABEL:
	if (!n_parens)
	  pred_start_ptr = pred_end_ptr = i;
	break;
      case PREPROCESS_EVALED:
      case RESULT:
	if (pred_end_ptr == ERROR)
	  pred_end_ptr = i;
	/* Lookback should be sufficient. */
	if (messages[i+1] -> tokentype != m -> tokentype)
	  pred_start_ptr = i;
	break;
	/* Scanning right to left, so increment the parentheses on
	   close, decerement on open. */
      case OPENPAREN:
	--n_parens;
	if (!n_parens)
	  pred_start_ptr = i;
	break;
      case CLOSEPAREN:
	if (!n_parens)
	  pred_end_ptr = i;
	++n_parens;
	break;
      }
  }

  if (pred_start_ptr == ERROR || pred_end_ptr == ERROR) {
    parse_exception = parse_error_x;
    warning (messages[op_ptr], "Parse error.");
    result -> __type = INTEGER_T;
    result -> __value.__i = FALSE;
  }

  /* Find the beginning and end of the true subexpression. */

  for (i = op_ptr - 1, true_start_ptr = ERROR, true_end_ptr = ERROR; 
       (i > stack_end) && (true_start_ptr == ERROR || true_end_ptr == ERROR);
       i--) {

    m = messages[i];
    
    if (m -> tokentype == WHITESPACE)
      continue;

    if (true_start_ptr == ERROR)
      true_start_ptr = i;

    if ((m -> tokentype == COLON) ||
	!strncmp (m -> name, ":", 1)) {
      true_end_ptr = i+1;
      else_op_ptr = i;
    }
  }

  /* 
   *   Find the beginning and end of the false subexpression,
   *   which may also end at a parenthesis which is not part
   *   of the clause. 
   */

  for (i = else_op_ptr - 1, false_start_ptr = ERROR, false_end_ptr = ERROR,
	 n_parens = 0; 
       (i > stack_end) && (false_start_ptr == ERROR || false_end_ptr == ERROR);
       i--) {
    m = messages[i];
    
    if (m -> tokentype == WHITESPACE)
      continue;

    switch (m -> tokentype)
      {
      case NEWLINE:
	*end_ptr = false_end_ptr = i + 1;
	break;
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (n_parens < 0) {
	  false_end_ptr = i + 1;
	  *end_ptr = i;
	}
	break;
      default:
	if (false_start_ptr == ERROR)
	  false_start_ptr = i;
      }
  }

  v_stacktop = v_message_ptr;
  if ((v_stack_end = 
       retokenize_conditional (messages, pred_start_ptr, &pred_end_ptr)) 
      == ERROR) {
    parse_exception = parse_error_x;
    return FALSE;
  }
  eval_constant_expr (v_messages, v_stacktop, &v_stack_end, &pred_val);
  clear_v_messages (v_stacktop, v_stack_end);


  if (is_val_true (&pred_val)) {
    if ((v_stack_end = 
	 retokenize_conditional (messages, true_start_ptr, &true_end_ptr))
	== ERROR) {
      parse_exception = parse_error_x;
      return FALSE;
    }
    eval_constant_expr (v_messages, v_stacktop, &v_stack_end, result);
  } else {
    if ((v_stack_end = 
	 retokenize_conditional (messages, false_start_ptr, &false_end_ptr))
	== ERROR) {
      parse_exception = parse_error_x;
      return FALSE;
    }
    eval_constant_expr (v_messages, v_stacktop, &v_stack_end, result);
  }

  clear_v_messages (v_stacktop, v_stack_end);


  eval_token = PREPROCESS_EVALED;

  for (i = pred_start_ptr; i >= false_end_ptr; i--) {

    ++(messages[i] -> evaled);
    messages[i] -> tokentype = eval_token;
    m_print_val (messages[i], result);
  }

  return is_val_true (result);
}

/*
 *   We have to retokenize the argument because sizeof,
 *   being also a C keyword, may have had its arguemnt 
 *   evaluated elsewhere.
 */

int handle_sizeof_op (MESSAGE_STACK messages, int op_ptr, 
		      int *end, VAL *val) {

  int i,
    stack_end,
    first_token,
    arg_end,
    n_parens;
  MESSAGE *m;

  if (strcmp (messages[op_ptr] -> name, "sizeof"))
    _error ("handle_sizeof_op: wrong message.");

  stack_end = first_token = get_stack_top (messages);

  /* Find the start and end of the argument. */
  for (i = op_ptr - 1, n_parens = 0, arg_end = ERROR; i > stack_end; i--) {

    m = messages[i];

    switch (m -> tokentype)
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (!n_parens) {
	  arg_end = i;
	  goto retokenize;
	}
      case RESULT:
      case PREPROCESS_EVALED:
	if (m -> name[0] == '(') {
	  ++n_parens;
	}
	if (m -> name[0] == ')') {
	  --n_parens;
	  if (!n_parens) {
	    *end = arg_end = i;
	    goto retokenize;
	  }
	}
 	break;
      }
  }

 retokenize:

  /* FIXME! -
     Until we can calculate the size of derived types, simply set the 
     size to the size of a void *. */

  for (i = op_ptr; i >= arg_end; i--) {
    messages[i] -> tokentype = PREPROCESS_EVALED;
    
#if defined(__DJGPP__) || defined(__APPLE__)
    sprintf (messages[i] -> value, "%lu", sizeof (void *));
#else
# if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
    sprintf (messages[i] -> value, "%lu", sizeof (void *));
# else /* if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__) */
    sprintf (messages[i] -> value, "%u", sizeof (void *));
# endif /* if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__) */
#endif /* #if defined(__DJGPP__) || defined(__APPLE__) */
  }

  val -> __type = INTEGER_T;
  val -> __value.__i = sizeof (void *);

  *end = arg_end;

  return SUCCESS;
}

