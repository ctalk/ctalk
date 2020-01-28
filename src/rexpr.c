/* $Id: rexpr.c,v 1.4 2020/01/28 05:22:08 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2019 Robert Kiesling, rk3314042@gmail.com.
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
 *  Evaluate the right hand side of an equation, and translate
 *  if:
 *  1. The right-hand expression is a method argument;
 *  2. The right-hand expression contains objects.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "ctrlblk.h"

extern I_PASS interpreter_pass;

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;

extern CTRLBLK *ctrlblks[MAXARGS + 1];
extern int ctrlblk_ptr;
extern bool ctrlblk_pred,              /* Global states. */
   ctrlblk_blk;
extern int for_init, 
  for_term,
  for_inc;

extern bool argblk;

extern ARG_TERM arg_c_fn_terms[MAXARGS];  /* Declared in ctoobj.c. */
extern int arg_c_fn_term_ptr;

extern ARG_CLASS arg_class;  /* Declared in method.c. */

/*
 *  Returns true when tok_idx points to the closing paren of
 *  a control block predicate.  Useful for single line blocks
 *  which don't use braces; e.g.,
 *
 *   if (somesomething == 0)
 *     (Point *)myPoint x = 10;
 *
 */

static inline bool braceless_ctrlblk_pred_end (MESSAGE_STACK messages,
					       int tok_idx) {
  if (M_TOK(messages[tok_idx]) == CLOSEPAREN &&
      ctrlblk_blk &&
      (ctrlblks[ctrlblk_ptr+1] -> pred_end_ptr == tok_idx))
    return true;
  else
    return false;
}

/*
 *  Return the context that an object appears in: as an lvalue,
 *  a receiver, an argument, or any C context.  
 */

OBJECT_CONTEXT object_context (MESSAGE_STACK messages, int obj_ptr) {

  int i, j,
    stack_start_ptr,
    stack_end_ptr;
  int prev_token_ptr,
    lookback,
    lookback2,
    lookahead,
    open_paren_ptr;
  MESSAGE *m;
  OBJECT_CONTEXT context;

  if (is_c_prefix_op (messages, obj_ptr) ||
      (M_TOK(messages[obj_ptr]) == MINUS))
     return c_context;

  stack_start_ptr = stack_start (messages);
  stack_end_ptr = get_stack_top (messages);

  /*
   *  Scan back to determine if we can find the context.
   */
  for (i = obj_ptr + 1, context = null_context; i <= stack_start_ptr; i++) {

    m = messages[i];

    switch (m -> tokentype)
      {
	/*
	 *  If the previous identifier is specifically a C type, 
	 *  or a prefix operarator, we're done.
	 */
      case CHAR:
      case AMPERSAND:
      case EXCLAM:
      case BIT_COMP:
      case CTYPE:
      case INCREMENT:
      case DECREMENT:
      case SIZEOF:
	return c_context;
	break;
	/*
	 *  If the previous identifier is a constant, we might check what 
	 *  it and our object are arguments to.  A context like this 
	 *  *should* normally be taken care of in the ARGSEPARATOR case, 
	 *  below.
	 */
      case LITERAL:
      case LITERAL_CHAR:
      case INTEGER:
      case FLOAT:
      case LONG:
      case LONGLONG:
	return c_context;
	break;
	/* 
	 *  If the previous identifier is a label that is a C 
	 *  function, variable, keyword, return c_context. If it's 
	 *  a method, return argument_context.
	 *
	 *  An exception is the, "return," keyword.  We will have to
	 *  match the type with the return value.  So far, we can only
	 *  make an exception for methods, where we can return
	 *  argument_context.  That should not need to change soon,
	 *  because all methods return, "OBJECT *."
	 *
	 *  Returning argument_context generates a, "__ctalk_method
	 *  ("self", <value_method>)," method call, and is probably
	 *  not worth making an exception for, although returning,
	 *  "receiver_context," would generate the more direct
	 *  "__ctalk_self_internal ()," call.
	 */
      case LABEL:
	if (str_eq (m -> name, "return")) {
	  return argument_context;
	} else {
	  if (str_eq (m -> name, "else")) {
	    /* TO DO -
	     * Still haven't set ctrlblk_blk for else clauses.
	     * For now, just check that there's a control block.
	     */
	    if (ctrlblks[ctrlblk_ptr+1]) {
	      if (i == ctrlblks[ctrlblk_ptr+1]->keyword2_ptr) 
		return receiver_context;
	    }
	  } else {
	    if (get_function (m -> name) ||
		get_local_var (m -> name) ||
		global_var_is_declared (m -> name) ||
		get_typedef (m -> name) ||
		is_c_keyword (m -> name)) {
	      return c_context;
	    } else {
	      if (is_class_name (m -> name))
		return argument_context;
	    }
	  }
	}
	break;
	/*
	 *   If the previous non-whitespace token is a semicolon, 
	 *   colon, or left curly brace, the object begins the statement, 
	 *   and it is the receiver, unless it is a leading operator.
	 *   If the brace is the closing brace of a struct or union, then
	 *   the label is a struct or union tag, return C context.
	 */
      case SEMICOLON:
      case OPENBLOCK:
      case CLOSEBLOCK:
      case COLON:
	if ((M_TOK(messages[obj_ptr]) == ASTERISK) ||
	    IS_C_UNARY_MATH_OP(M_TOK(messages[obj_ptr])))
	  return c_context;
	/* NOTE: If dealing with a CVAR, check for a
	   CVAR_ATTR_STRUCT_TAG attr member, which also
	   looks like a receiver context.  (Scanning back
	   for the struct declaration here is slow.) */
	return receiver_context;
	break;
	/*
	 *  Determine if the preceding label token is a method
	 *  by looking up the method from the receiver object,
	 *  if any.  If a receiver or method is not found, the
	 *  label is either a C keyword or a C library function.
	 *  
	 *  Note that this criterion also applies to overloaded 
	 *  C operators.
	 */
      case EQ:
      case BOOLEAN_EQ:
      case GT:
      case GE:
      case ASR:
      case ASR_ASSIGN:
      case LT:
      case LE:
      case ASL:
      case ASL_ASSIGN:
      case PLUS:
      case PLUS_ASSIGN:
      case MINUS:
      case MINUS_ASSIGN:
      case DEREF:
      case ASTERISK:
      case MULT_ASSIGN:
      case DIVIDE:
      case DIV_ASSIGN:
      case BOOLEAN_AND:
      case BIT_AND_ASSIGN:
      case INEQUALITY:
      case BIT_OR:
      case BOOLEAN_OR:
      case BIT_OR_ASSIGN:
      case BIT_XOR:
      case BIT_XOR_ASSIGN:
      case PERIOD:
      case ELLIPSIS:
      case LITERALIZE:
      case MACRO_CONCAT:
      case CONDITIONAL:
      case MODULUS:
	/* If the object itself is not a label, then it 
	   can only be an overloaded operator, in which case the
	   previous token must be a label if it is used in an object
	   context.
	*/
	if (messages[obj_ptr] -> tokentype != LABEL) {
	  return c_context;
	} else {
	  /*
	   *  If the label before the operator does not have an object
	   *  associated with it, and the label is not the name of a 
	   *  variable of type OBJECT *, use C argument context.  If it 
	   *  has an object, use argument context.
	   */

	  if (((lookback = prevlangmsg (messages, i)) != ERROR) &&
	      (messages[lookback] -> tokentype == LABEL) &&
	      (messages[lookback] -> obj == NULL)) {
	    CVAR *c;
	    if (((c = get_local_var (messages[lookback] -> name)) != NULL) ||
		((c = get_global_var (messages[lookback] -> name)) != NULL)) {
	      if ((c -> type_attrs & CVAR_TYPE_OBJECT) &&
		  (c -> n_derefs)) {
		/*
		 * This case is treated the same as a normal method argument,
		 * because methods use OBJECT *.
		 */
		return argument_context;
	      } else {
		return c_argument_context;
	      }
	    } else {
	      if (is_struct_member_tok (messages, lookback)) {
		return c_argument_context;
	      } else {
		if (argblk) {
		  if ((c = 
		       get_var_from_cvartab_name (M_NAME(messages[lookback])))
		      != NULL) {
		    if ((c -> type_attrs & CVAR_TYPE_OBJECT) &&
			(c -> n_derefs)) {
		      return argument_context;
		    } else {
		      return c_argument_context;
		    }
		  }
		} else {
		  /*
		   *  This case should cover anything that the parser 
		   *  doesn't know about.
		   */
		  return argument_context;
		}
	      }
	    }
	  } else {
	    /*
	     *  If the lookback token is a C constant expression,
	     *  then the operator is a C operator and not a method.
	     */
	    if ((lookback != ERROR) && 
		((messages[lookback] -> tokentype == LITERAL) ||
		 (messages[lookback] -> tokentype == LITERAL_CHAR) ||
		 (messages[lookback] -> tokentype == INTEGER) ||
		 (messages[lookback] -> tokentype == FLOAT) ||
		 (messages[lookback] -> tokentype == DOUBLE) ||
		 (messages[lookback] -> tokentype == LONG) ||
		 (messages[lookback] -> tokentype == LONGLONG))) {
	      return c_argument_context;
	    } else {
	      /*
	       *  If the lookback token is a closing parenthesis, then
	       *  check whether the previous operand is a function 
	       *  call.
	       */
	      if ((lookback != ERROR) &&
		  (messages[lookback] -> tokentype == CLOSEPAREN)) {
		lookback2 = match_paren_rev (messages, lookback,
					     stack_start_ptr);
		if ((lookback2 = prevlangmsg (messages, lookback2)) != ERROR){
		  if (M_TOK(messages[lookback2]) == LABEL) {
		    if (get_function(M_NAME(messages[lookback2]))) {
		      return c_argument_context;
		    } else {
		      if (M_TOK(messages[lookback]) == ARGSEPARATOR) {
			if (is_fmt_arg (messages, lookback,
					stack_start_ptr,
					stack_end_ptr)) {
			  return c_argument_context;
			} else {
			  return argument_context;
			}
		      } else {
			return argument_context;
		      }
		    }
		  }
		}
	      } else {
		if ((lookback != ERROR) &&
		    (messages[lookback] -> tokentype == ARGSEPARATOR)) {
		  if (is_fmt_arg (messages, i,
				  stack_start_ptr,
				  stack_end_ptr)) {
		    return c_argument_context;
		  } else {
		    int __arg_idx, __fn_idx;
		    if ((__arg_idx = 
			 obj_expr_is_arg (messages, i,
					  stack_start_ptr,
					  &__fn_idx)) != ERROR) {
		      return c_context;
		    } else {
		      return argument_context;
		    }
		  }
		} else {
		  if ((lookback != ERROR)  &&
		      (M_TOK(messages[lookback]) == ARRAYCLOSE)) {
		    return c_argument_context;
		  } else {
		    return argument_context;
		  }
		}
	      }
	    }
	  }
	}
	break;
      case CLOSEPAREN:
	if (TOK_HAS_CLASS_TYPECAST(messages[obj_ptr])) {
	  if ((open_paren_ptr =
	       match_paren_rev (messages, i, stack_start_ptr)) != ERROR) {
	    if ((lookback = prevlangmsgstack (messages, open_paren_ptr))
		!= ERROR) {
	      /* this set might yet be incomplete ... */
	      if (M_TOK(messages[lookback]) == SEMICOLON ||
		  M_TOK(messages[lookback]) == COLON ||
		  M_TOK(messages[lookback]) == OPENBLOCK ||
		  M_TOK(messages[lookback]) == CLOSEBLOCK ||
		  /* This is a single-line controlblock block
		     without braces. */
		  (M_TOK(messages[lookback]) == CLOSEPAREN &&
		   ctrlblk_blk &&
		   (ctrlblks[ctrlblk_ptr+1] -> pred_end_ptr ==
		    lookback))) {
		return receiver_context;
	      } else if (M_TOK(messages[lookback]) == ARGSEPARATOR &&
			 is_fmt_arg (messages, lookback, stack_start_ptr,
				     stack_end_ptr)) {
		return c_argument_context;
	      } else {
		return argument_context;
	      }
	    } else {
	      return receiver_context;
	    }
	  }
	}
	/*
	 *  Look for the case where the token is the first after
	 *  a control block predicate.
	 */
	if (ctrlblk_blk) {
	  if (i == ctrlblks[ctrlblk_ptr+1]->pred_end_ptr)
	    return receiver_context;
	}
	/* Check for a typecast expr, and skip past it. */
	if ((lookback2 = match_paren_rev (messages, i, stack_start_ptr))
	    != ERROR) {
	  int _end_ptr, lookback3;
	  /* Make sure we start on the first non-whitespace token, just
	     in case is_typecast_expr () changes. */
	  lookback3 = nextlangmsg (messages, lookback2);
	  if (is_typecast_expr (messages, lookback3, &_end_ptr))
	    i = lookback2;
	}
	break;
      case ARGSEPARATOR:
	/*
	 *  Check for a printf format argument.
	 */
	if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	  if (is_fmt_arg(messages,lookahead, stack_start_ptr,
			 stack_end_ptr))
	    return c_argument_context;
	}
	/*
	 *  If we're in an argument list, scan back toward
	 *  the beginning of the list, and try to determine
	 *  what we're an argument to.
	 */
	for (j = i + 1, context = c_context; j <= stack_start_ptr; j++) {
	  /*
	   *  If we reach the end of the previous statement or the
	   *  beginning of a block, we're in a statement.
	   */
	  if ((messages[j] -> tokentype == SEMICOLON) ||
	      (messages[j] -> tokentype == OPENBLOCK))
	    break;

	  if ((M_TOK(messages[j]) == ARRAYCLOSE) ||
	      (M_TOK(messages[j]) == DEREF) ||
	      (M_TOK(messages[j]) == PERIOD)) {
	    return c_argument_context;
	  }

	  switch (messages[j] -> tokentype)
	    {
	    case LABEL:
	      if ((prev_token_ptr = prevlangmsg (messages, j)) != ERROR) {
		/*
		 *  If we have two successive labels, try to determine
		 *  if they're a receiver and a method.
		 */
		if (messages[prev_token_ptr] -> tokentype == LABEL) {
		  OBJECT *rcvr;
		  METHOD *method;
		  if ((rcvr = 
		       get_object (messages[prev_token_ptr] -> name, NULL))
		      != NULL) {
		    if ((method = get_instance_method 
			 (messages[j],
			  rcvr, messages[j] -> name, ERROR, FALSE))
			!= NULL)
		      context = argument_context;
		  }
		}
	      }
	      return context;
	      break;
	    case METHODMSGLABEL:
	      context = argument_context;
	      break;
	    case CLOSEPAREN:
	      j = match_paren_rev (messages, j, stack_start_ptr);
	      break;
	    }
	}
      default:
	break;
      }
  }
  return context;
}

OBJECT_CONTEXT object_context_ms (MSINFO *ms) {

  int i, j;
  int prev_token_ptr,
    lookback,
    lookback2,
    lookahead,
    open_paren_ptr;
  MESSAGE *m;
  OBJECT_CONTEXT context;
  char *path;

  if (is_c_prefix_op (ms -> messages, ms -> tok) ||
      (M_TOK(ms -> messages[ms -> tok]) == MINUS))
     return c_context;

  /*
   *  Scan back to determine if we can find the context.
   */
  for (i = ms -> tok + 1, context = null_context; i <= ms -> stack_start; i++) {

    m = ms -> messages[i];

    switch (m -> tokentype)
      {
	/*
	 *  If the previous identifier is specifically a C type, 
	 *  or a prefix operarator, we're done.
	 */
      case CHAR:
      case AMPERSAND:
      case EXCLAM:
      case BIT_COMP:
      case CTYPE:
      case INCREMENT:
      case DECREMENT:
      case SIZEOF:
	return c_context;
	break;
	/*
	 *  If the previous identifier is a constant, we might check what 
	 *  it and our object are arguments to.  A context like this 
	 *  *should* normally be taken care of in the ARGSEPARATOR case, 
	 *  below.
	 */
      case LITERAL:
      case LITERAL_CHAR:
      case INTEGER:
      case FLOAT:
      case LONG:
      case LONGLONG:
	return c_context;
	break;
	/* 
	 *  If the previous identifier is a label that is a C 
	 *  function, variable, keyword, return c_context. If it's 
	 *  a method, return argument_context.
	 *
	 *  An exception is the, "return," keyword.  We will have to
	 *  match the type with the return value.  So far, we can only
	 *  make an exception for methods, where we can return
	 *  argument_context.  That should not need to change soon,
	 *  because all methods return, "OBJECT *."
	 *
	 *  Returning argument_context generates a, "__ctalk_method
	 *  ("self", <value_method>)," method call, and is probably
	 *  not worth making an exception for, although returning,
	 *  "receiver_context," would generate the more direct
	 *  "__ctalk_self_internal ()," call.
	 */
      case LABEL:
	if (str_eq (m -> name, "return")) {
	  return argument_context;
	} else {
	  if (str_eq (m -> name, "else")) {
	    /* TO DO -
	     * Still haven't set ctrlblk_blk for else clauses.
	     * For now, just check that there's a control block.
	     */
	    if (ctrlblks[ctrlblk_ptr+1]) {
	      if (i == ctrlblks[ctrlblk_ptr+1]->keyword2_ptr) 
		return receiver_context;
	    }
	  } else {
	    if (get_function (m -> name) ||
		get_local_var (m -> name) ||
		global_var_is_declared (m -> name) ||
		get_typedef (m -> name) ||
		is_c_keyword (m -> name)) {
	      return c_context;
	    } else {
	      if ((path = find_library_include (m -> name, FALSE)) != NULL)
		return argument_context;
	    }
	  }
	}
	break;
	/*
	 *   If the previous non-whitespace token is a semicolon, 
	 *   colon, or left curly brace, the object begins the statement, 
	 *   and it is the receiver, unless it is a leading operator.
	 *   If the brace is the closing brace of a struct or union, then
	 *   the label is a struct or union tag, return C context.
	 */
      case SEMICOLON:
      case OPENBLOCK:
      case CLOSEBLOCK:
      case COLON:
	if ((M_TOK(ms -> messages[ms -> tok]) == ASTERISK) ||
	    IS_C_UNARY_MATH_OP(M_TOK(ms -> messages[ms -> tok])))
	  return c_context;
	/* NOTE: If dealing with a CVAR, check for a
	   CVAR_ATTR_STRUCT_TAG attr member, which also
	   looks like a receiver context.  (Scanning back
	   for the struct declaration here is slow.) */
	return receiver_context;
	break;
	/*
	 *  Determine if the preceding label token is a method
	 *  by looking up the method from the receiver object,
	 *  if any.  If a receiver or method is not found, the
	 *  label is either a C keyword or a C library function.
	 *  
	 *  Note that this criterion also applies to overloaded 
	 *  C operators.
	 */
      case EQ:
      case BOOLEAN_EQ:
      case GT:
      case GE:
      case ASR:
      case ASR_ASSIGN:
      case LT:
      case LE:
      case ASL:
      case ASL_ASSIGN:
      case PLUS:
      case PLUS_ASSIGN:
      case MINUS:
      case MINUS_ASSIGN:
      case DEREF:
      case ASTERISK:
      case MULT_ASSIGN:
      case DIVIDE:
      case DIV_ASSIGN:
      case BOOLEAN_AND:
      case BIT_AND_ASSIGN:
      case INEQUALITY:
      case BIT_OR:
      case BOOLEAN_OR:
      case BIT_OR_ASSIGN:
      case BIT_XOR:
      case BIT_XOR_ASSIGN:
      case PERIOD:
      case ELLIPSIS:
      case LITERALIZE:
      case MACRO_CONCAT:
      case CONDITIONAL:
      case MODULUS:
	/* If the object itself is not a label, then it 
	   can only be an overloaded operator, in which case the
	   previous token must be a label if it is used in an object
	   context.
	*/
	if (ms -> messages[ms -> tok] -> tokentype != LABEL) {
	  return c_context;
	} else {
	  /*
	   *  If the label before the operator does not have an object
	   *  associated with it, and the label is not the name of a 
	   *  variable of type OBJECT *, use C argument context.  If it 
	   *  has an object, use argument context.
	   */

	  if (((lookback = prevlangmsg (ms -> messages, i)) != ERROR) &&
	      (ms -> messages[lookback] -> tokentype == LABEL) &&
	      (ms -> messages[lookback] -> obj == NULL)) {
	    CVAR *c;
	    if (((c = get_local_var (ms -> messages[lookback] -> name))
		 != NULL) ||
		((c = get_global_var (ms -> messages[lookback] -> name))
		 != NULL)) {
	      if ((c -> type_attrs & CVAR_TYPE_OBJECT) &&
		  (c -> n_derefs)) {
		/*
		 * This case is treated the same as a normal method argument,
		 * because methods use OBJECT *.
		 */
		return argument_context;
	      } else {
		return c_argument_context;
	      }
	    } else {
	      if (is_struct_member_tok (ms -> messages, lookback)) {
		return c_argument_context;
	      } else {
		if (argblk) {
		  if ((c = 
		       get_var_from_cvartab_name (M_NAME(ms -> messages[lookback])))
		      != NULL) {
		    if ((c -> type_attrs & CVAR_TYPE_OBJECT) &&
			(c -> n_derefs)) {
		      return argument_context;
		    } else {
		      return c_argument_context;
		    }
		  }
		} else {
		  /*
		   *  This case should cover anything that the parser 
		   *  doesn't know about.
		   */
		  return argument_context;
		}
	      }
	    }
	  } else {
	    /*
	     *  If the lookback token is a C constant expression,
	     *  then the operator is a C operator and not a method.
	     */
	    if ((lookback != ERROR) && 
		((ms -> messages[lookback] -> tokentype == LITERAL) ||
		 (ms -> messages[lookback] -> tokentype == LITERAL_CHAR) ||
		 (ms -> messages[lookback] -> tokentype == INTEGER) ||
		 (ms -> messages[lookback] -> tokentype == FLOAT) ||
		 (ms -> messages[lookback] -> tokentype == DOUBLE) ||
		 (ms -> messages[lookback] -> tokentype == LONG) ||
		 (ms -> messages[lookback] -> tokentype == LONGLONG))) {
	      return c_argument_context;
	    } else {
	      /*
	       *  If the lookback token is a closing parenthesis, then
	       *  check whether the previous operand is a function 
	       *  call.
	       */
	      if ((lookback != ERROR) &&
		  (ms -> messages[lookback] -> tokentype == CLOSEPAREN)) {
		lookback2 = match_paren_rev (ms -> messages, lookback,
					     ms -> stack_start);
		if ((lookback2 = prevlangmsg (ms -> messages, lookback2))
		    != ERROR){
		  if (M_TOK(ms -> messages[lookback2]) == LABEL) {
		    if (get_function(M_NAME(ms -> messages[lookback2]))) {
		      return c_argument_context;
		    } else {
		      if (M_TOK(ms -> messages[lookback]) == ARGSEPARATOR) {
			ms -> tok = lookback;
			if (is_fmt_arg_2 (ms)) {
			  return c_argument_context;
			} else {
			  return argument_context;
			}
		      } else {
			return argument_context;
		      }
		    }
		  }
		}
	      } else {
		if ((lookback != ERROR) &&
		    (ms -> messages[lookback] -> tokentype == ARGSEPARATOR)) {
		  ms -> tok = i;
		  if (is_fmt_arg_2 (ms)) {
		    return c_argument_context;
		  } else {
		    int __arg_idx, __fn_idx;
		    if ((__arg_idx = 
			 obj_expr_is_arg_ms (ms, &__fn_idx)) != ERROR) {
		      return c_context;
		    } else {
		      return argument_context;
		    }
		  }
		} else {
		  if ((lookback != ERROR)  &&
		      (M_TOK(ms -> messages[lookback]) == ARRAYCLOSE)) {
		    return c_argument_context;
		  } else {
		    return argument_context;
		  }
		}
	      }
	    }
	  }
	}
	break;
      case CLOSEPAREN:
	if (TOK_HAS_CLASS_TYPECAST(ms -> messages[ms -> tok])) {
	  if ((open_paren_ptr =
	       match_paren_rev (ms -> messages, i, ms -> stack_start)) != ERROR) {
	    if ((lookback = prevlangmsgstack (ms -> messages, open_paren_ptr))
		!= ERROR) {
	      /* this set might yet be incomplete ... */
	      if (M_TOK(ms -> messages[lookback]) == SEMICOLON ||
		  M_TOK(ms -> messages[lookback]) == COLON ||
		  M_TOK(ms -> messages[lookback]) == OPENBLOCK ||
		  M_TOK(ms -> messages[lookback]) == CLOSEBLOCK ||
		  /* This is a single-line controlblock block
		     without braces. */
		  (M_TOK(ms -> messages[lookback]) == CLOSEPAREN &&
		   ctrlblk_blk &&
		   (ctrlblks[ctrlblk_ptr+1] -> pred_end_ptr ==
		    lookback))) {
		return receiver_context;
	      } else if (M_TOK(ms -> messages[lookback]) == ARGSEPARATOR &&
			 /* this should stay like this */
			 is_fmt_arg (ms -> messages, lookback,
				     ms -> stack_start,
				     ms -> stack_ptr)) {
		return c_argument_context;
	      } else {
		return argument_context;
	      }
	    } else {
	      return receiver_context;
	    }
	  }
	}
	/*
	 *  Look for the case where the token is the first after
	 *  a control block predicate.
	 */
	if (ctrlblk_blk) {
	  if (i == ctrlblks[ctrlblk_ptr+1]->pred_end_ptr)
	    return receiver_context;
	}
	/* Check for a typecast expr, and skip past it. */
	if ((lookback2 = match_paren_rev (ms -> messages, i, ms -> stack_start))
	    != ERROR) {
	  int _end_ptr, lookback3;
	  /* Make sure we start on the first non-whitespace token, just
	     in case is_typecast_expr () changes. */
	  lookback3 = nextlangmsg (ms -> messages, lookback2);
	  if (is_typecast_expr (ms -> messages, lookback3, &_end_ptr))
	    i = lookback2;
	}
	break;
      case ARGSEPARATOR:
	/*
	 *  Check for a printf format argument.
	 */
	if ((lookahead = nextlangmsg (ms -> messages, i)) != ERROR) {
	  /* This should also stay like this. */
	  if (is_fmt_arg(ms -> messages,lookahead, ms -> stack_start,
			 ms -> stack_ptr))
	    return c_argument_context;
	}
	/*
	 *  If we're in an argument list, scan back toward
	 *  the beginning of the list, and try to determine
	 *  what we're an argument to.
	 */
	for (j = i + 1, context = c_context; j <= ms -> stack_start; j++) {
	  /*
	   *  If we reach the end of the previous statement or the
	   *  beginning of a block, we're in a statement.
	   */
	  if ((ms -> messages[j] -> tokentype == SEMICOLON) ||
	      (ms -> messages[j] -> tokentype == OPENBLOCK))
	    break;

	  if ((M_TOK(ms -> messages[j]) == ARRAYCLOSE) ||
	      (M_TOK(ms -> messages[j]) == DEREF) ||
	      (M_TOK(ms -> messages[j]) == PERIOD)) {
	    return c_argument_context;
	  }

	  switch (ms -> messages[j] -> tokentype)
	    {
	    case LABEL:
	      if ((prev_token_ptr = prevlangmsg (ms -> messages, j)) != ERROR) {
		/*
		 *  If we have two successive labels, try to determine
		 *  if they're a receiver and a method.
		 */
		if (ms -> messages[prev_token_ptr] -> tokentype == LABEL) {
		  OBJECT *rcvr;
		  METHOD *method;
		  if ((rcvr = 
		       get_object (ms -> messages[prev_token_ptr] -> name, NULL))
		      != NULL) {
		    if ((method = get_instance_method 
			 (ms -> messages[j],
			  rcvr, ms -> messages[j] -> name, ERROR, FALSE))
			!= NULL)
		      context = argument_context;
		  }
		}
	      }
	      return context;
	      break;
	    case METHODMSGLABEL:
	      context = argument_context;
	      break;
	    case CLOSEPAREN:
	      j = match_paren_rev (ms -> messages, j, ms -> stack_start);
	      break;
	    }
	}
      default:
	break;
      }
  }
  return context;
}

/*
 *  Handle the case of:
 *
 *    <cvar> == <obj>
 *
 *  And <cvar> is an OBJECT *
 *
 */
static bool dm_object_identity (MESSAGE_STACK messages, int obj_ptr) {
  int m_prev_ptr, m_prev_ptr_2;
  CVAR *l_term;
  char buf[MAXMSG];
  if ((m_prev_ptr = prevlangmsg (messages, obj_ptr)) != ERROR) {
    if (IS_C_RELATIONAL_OP(M_TOK(messages[m_prev_ptr]))) {
      if ((m_prev_ptr_2 = prevlangmsg (messages, m_prev_ptr)) != ERROR) {
	if (messages[m_prev_ptr_2] -> attrs & TOK_IS_DECLARED_C_VAR) {
	  l_term = NULL;
	  if (argblk) {
	    l_term = get_var_from_cvartab_name (M_NAME(messages[m_prev_ptr_2]));
	  } else {
	    if ((l_term = get_local_var (M_NAME(messages[m_prev_ptr_2])))
		== NULL) {
	      l_term = get_global_var (M_NAME(messages[m_prev_ptr_2]));
	    }
	  }
	  if (IS_CVAR(l_term)) {
	    if (((l_term -> n_derefs == 1) &&
		 (l_term -> type_attrs & CVAR_TYPE_OBJECT)) ||
		/* function parameter don't (yet) have the type_attrs
		   filled in */
		((l_term -> n_derefs == 1) &&
		 str_eq (l_term -> type, "OBJECT"))) {
	      strcatx (buf, OBJECT_TOK_RETURN_FN, " (\"",
		       M_NAME(messages[obj_ptr]),
		       "\", ((void *)0))", 
		       NULL);
	      output_buffer (buf, obj_ptr);
	      ++messages[obj_ptr] -> evaled;
	      ++messages[obj_ptr] -> output;
	      return true;
	    }
	  }
	}
      }
    }
  }
  return false;
}

int default_method (MSINFO *ms) {

  MESSAGE *m_obj,
    *m_next;
  int m_next_ptr;
  int tmp_end_ptr;
  int method_attrs;
  int m_prev_ptr;
  METHOD *m;
  OBJECT *class;
  OBJECT_CONTEXT context;
  char buf[MAXMSG], expr_out_buf[MAXMSG], expr_out_buf_2[MAXMSG];
  char buf2[MAXMSG], mcbuf[MAXMSG];
  int arg_idx, fn_idx, n_th_arg, i, fn_arg_start_idx, fn_arg_end_idx;
  CFUNC *cfn;
  CVAR *param;

  if (interpreter_pass == expr_check)
    return SUCCESS;

  m_obj = ms -> messages[ms -> tok];
  
  if ((m_next_ptr = nextlangmsg (ms -> messages, ms -> tok)) == ERROR) {
    /* Called by eval_arg or something similar that has its own
     * stack.
     */
    return ERROR;
  }

  m_next = ms -> messages[m_next_ptr];

  /*
   *  If a non-label token is a method, we don't need to 
   *  process it here.
   */

  if (IS_CLASS_OBJECT (m_obj -> obj)) {
    class = m_obj -> obj;
  } else {
    if (!strcmp (m_obj -> obj -> __o_classname, EXPR_CLASSNAME)) {
      return SUCCESS;
    } else {
      if ((class = get_class_object (m_obj -> obj -> __o_classname)) == NULL)
	error (m_obj, "Undefined class in default_method ().");
    }
  }

  /*
   *  Cases like <any-receiver> <classvariable>
   */
  if (m_next -> attrs & OBJ_IS_CLASS_VAR)
    return ERROR;

  if (m_obj -> receiver_msg &&
      m_obj -> receiver_msg -> attrs & TOK_IS_CLASS_TYPECAST)
    return ERROR;
  if (m_obj -> attrs & TOK_IS_CLASS_TYPECAST)
    return ERROR;

  /*
   *  Anything that is inside of an expression used as 
   *  a receiver gets evaluated later.
   */
  if (is_in_rcvr_subexpr_obj_check (ms -> messages, ms -> tok,
				    ms -> stack_ptr)) 
    return ERROR;

  /*
   *  This case should not be needed if there is no 
   *  m_obj->obj->instancevars.
   */
  if ((m_obj->attrs & OBJ_IS_CLASS_VAR) && m_obj->obj->instancevars) {
    if (M_TOK(m_next) == LABEL && !str_eq (M_NAME(m_next), "value"))
      return ERROR;
  }

  /*
    TODO - A <label> <label> that returns an error here probably
    means that the second label's parsing is still in progress from
    a previous parser level, and could be deferred until run time,
    with a warning.
  */
  if ((!METHOD_ARG_TERM_MSG_TYPE (m_next)) &&
      (((m = get_instance_method (m_obj, m_obj->obj, m_next->name, 
				  ERROR, FALSE))!=NULL) ||
       ((m = get_class_method (m_obj, m_obj->obj, m_next->name, 
			       ERROR, FALSE))!=NULL) || 
       __ctalkGetInstanceVariable (m_obj -> obj, m_next -> name, FALSE) ||
       __ctalkGetClassVariable (m_obj -> obj, m_next -> name, FALSE) ||
       (compound_method (ms -> messages, m_next_ptr, &method_attrs) != m_next_ptr) ||
       is_method_proto (m_obj->obj->__o_class, M_NAME(m_next)) ||
       template_name (M_NAME(m_next)) ||
       have_user_prototype (m_obj -> obj -> __o_class -> __o_name, 
			    M_NAME(m_next))))
    return ERROR;

  if ((M_TOK(m_next) == PERIOD) || (M_TOK(m_next) == DEREF))
    return ERROR;


  /* 
   * Look up the "value" method.  It should at least be defined in 
   * Object class, so there won't be a need for another library
   * search, and possible redefinition of the receiver's class.
   */

  if ((m = get_instance_method (m_obj, m_obj -> obj, "value", 
				ERROR, TRUE)) == NULL) {
    sprintf (buf, "%s (Class %s)", "value", m_obj -> obj -> __o_classname);
    __ctalkExceptionInternal (m_obj, undefined_method_x, buf, 0);
    return ERROR;
  }

  if ((m_prev_ptr = prevlangmsg (ms -> messages, ms -> tok)) != ERROR) {
    if (ms -> messages[m_prev_ptr] -> attrs & TOK_IS_PREFIX_OPERATOR)
      return ERROR;
  }

  context = object_context_ms (ms);

  switch (context)
    {
    case c_context:
    case c_argument_context:
      /*
       *  Handle expression cases where an object is preceded
       *  by a C variable and a C operator.  In the case of 
       *  control structure predicates, we need to output the
       *  expression here, because the operator probably won't
       *  be recognized as a method.  Normally, loop expressions
       *  get analyzed and output in loop_block_start () in 
       *  loop.c.
       */
      if (ctrlblk_pred &&
	  ((C_CTRL_BLK -> stmt_type == stmt_while) ||
	   (C_CTRL_BLK -> stmt_type == stmt_do) ||
 	   (C_CTRL_BLK -> stmt_type == stmt_if) ||
	   (for_term && (C_CTRL_BLK -> stmt_type == stmt_for)))) {

	tmp_end_ptr = ms -> tok;

	if ((arg_idx =
	     obj_expr_is_arg (ms -> messages, ms -> tok,
			      ms -> stack_start,
			      &fn_idx)) != ERROR) {
	  if ((cfn = get_function (M_NAME(ms -> messages[fn_idx]))) != NULL) {
	    if (arg_idx == 0) {
	      param = cfn -> params;
 	    } else {
	      for (n_th_arg = 0, param = cfn -> params; 
		   n_th_arg < arg_idx; ++n_th_arg, param = param -> next)
		;
	    }
	    /*
	     * Another special case - if the return class is Object
	     * (i.e., if the param type is OBJECT *), use an
	     * OBJECT * return, otherwise, make it a Symbol, because
	     * we're in C context here.
	     */
	    strcatx (buf, EVAL_EXPR_FN, " (\"", M_NAME(m_obj), "\")",
		     NULL);
	    fmt_rt_return (buf,
			   ((param -> type_attrs & CVAR_TYPE_OBJECT) ?
			    OBJECT_CLASSNAME : SYMBOL_CLASSNAME), TRUE, buf2);
	    output_buffer (buf2, ms -> tok);
	    ++m_obj -> evaled;
	    ++m_obj -> output;
	    return SUCCESS;
	  }
	} else if (for_term && (C_CTRL_BLK -> stmt_type == stmt_for)) {
	  fileout 
	    (fmt_rt_return
	     (fmt_rt_expr 
	      (ms -> messages, ms -> tok, &tmp_end_ptr, expr_out_buf), 
	      ((M_OBJ(ms -> messages[ms -> tok])->instancevars) ?
	       M_OBJ(ms -> messages[ms -> tok])->instancevars->__o_classname :
	       M_OBJ(ms -> messages[ms -> tok])->__o_classname),
	      TRUE, expr_out_buf_2),
	     FALSE, ms -> tok);
	  C_CTRL_BLK -> pred_expr_evaled = TRUE;
	  ++m_obj -> evaled;
	  ++m_obj -> output;
	  return SUCCESS;
	} else if (dm_object_identity (ms -> messages, ms -> tok)) {
	  return SUCCESS;
	} else if (C_CTRL_BLK -> pred_start_ptr -
		   C_CTRL_BLK -> pred_end_ptr > 3) {
	  /* if we have more than a single-token expression, just
	     call ctrlblk_pred_rt_expr on the first lone object
	     that we encounter. Otherwise fall through. */
	  return ctrlblk_pred_rt_expr (ms -> messages, ms -> tok);
	}

	/*
	 *  Otherwise, for now, simply translate the object's class directly
	 *  to its C type, without worrying what a receiver or
	 *  lvalue might need.
	 */
	fileout 
	  (fmt_rt_return
	   (fmt_rt_expr 
	    (ms -> messages, ms -> tok, &tmp_end_ptr, expr_out_buf), 
	    ((M_OBJ(ms -> messages[ms -> tok])->instancevars) ?
	     M_OBJ(ms -> messages[ms -> tok])->instancevars->__o_classname :
	     M_OBJ(ms -> messages[ms -> tok])->__o_classname),
	    TRUE, expr_out_buf_2),
	    FALSE, ms -> tok);
	C_CTRL_BLK -> pred_expr_evaled = TRUE;
      } else if (is_fmt_arg_2 (ms)) {
	fmt_printf_fmt_arg_ms
	  (ms, rt_library_method_call
	   (m_obj -> obj, m, ms -> messages,
	    ms -> tok, mcbuf), buf);
	output_buffer (buf, ms -> tok);
      } else {
	/* this is specifically for C function calls appearing in
	   receiver context that need to use a template so they
	   can handle writeable args */
	if ((arg_idx =
	     obj_expr_is_arg (ms -> messages, ms -> tok,
			      ms -> stack_start, &fn_idx)) != ERROR) {
	  if (libc_fn_needs_writable_args (M_NAME(ms -> messages[fn_idx]))) {
	    template_call_from_CFunction_receiver 
	      (ms -> messages, fn_idx);
	    fn_arg_start_idx = nextlangmsg (ms -> messages, fn_idx);
	    fn_arg_end_idx = match_paren (ms -> messages, fn_arg_start_idx,
					  ms -> stack_ptr);
	    for (i = fn_idx; i >= fn_arg_end_idx; i--) {
	      ++ms -> messages[i] -> evaled;
	      ++ms -> messages[i] -> output;
	    }
	    return SUCCESS;
	  }
	}
	strcpy (buf, 
		obj_2_c_wrapper_trans 
		(ms -> messages, ms -> tok, m_obj, m_obj -> obj, m, 
		 rt_library_method_call (m_obj -> obj, m,
					 ms -> messages, ms -> tok, mcbuf),
		 TRUE));
	output_buffer (buf, ms -> tok);
      } /* if (for_term) { */
      break;
    case argument_context:
      /* This can happen if there are many objects in a function's
	 argument list.... */
      if ((arg_idx =
	   obj_expr_is_arg (ms -> messages, ms -> tok,
			    ms -> stack_start,
			    &fn_idx)) != ERROR) {
	if ((cfn = get_function (M_NAME(ms -> messages[fn_idx]))) != NULL) {
	  if (arg_idx == 0) {
	    param = cfn -> params;
	  } else {
	    for (n_th_arg = 0, param = cfn -> params; 
		 n_th_arg < arg_idx; ++n_th_arg, param = param -> next)
	      ;
	  }
	  strcatx (buf, EVAL_EXPR_FN, " (\"", M_NAME(m_obj), "\")",
		   NULL);
	  fmt_rt_return (buf,
			 basic_class_from_cvar
			 (ms -> messages[ms -> tok], param, 0),
			 TRUE, buf2);
	  output_buffer (buf2, ms -> tok);
	  ++m_obj -> evaled;
	  ++m_obj -> output;
	  return SUCCESS;
	}
      } else {
	if (!dm_object_identity (ms -> messages, ms -> tok))
	  goto dmdc;
      }
      break;
    default:
    dmdc:
      /*
       *  If we're parsing the arguments of a C function, then
       *  the context could be argument_context if the first 
       *  token of the sub-parser is an opening parenthesis.
       */
      if (interpreter_pass == c_fn_pass) {
	strcpy (buf, 
		obj_2_c_wrapper 
		(m_obj, m_obj -> obj, m, 
		 rt_library_method_call (m_obj -> obj, m,
					 ms -> messages, ms -> tok, mcbuf),
		 TRUE));
	output_buffer (buf, ms -> tok);
      } else {
	if (M_TOK(m_next) == LABEL) {
	  /* this should probably stay as a separate call to
	     is_superclass_method_proto. */
	  if (is_superclass_method_proto 
	      (m_obj -> obj -> __o_classname, M_NAME(m_next))) {
	    warning (m_obj, "Undefined Label.\n"
		     "\tLabel, \"%s,\" is not (yet) defined. "
		     "Deferring evaluation until run time.",
		     M_NAME (m_next));
	    deferred_method_eval_a (ms -> messages, ms -> tok, &m_next_ptr);
	  } else {
	    if (get_local_object (M_NAME(m_next), NULL) ||
		get_global_object (M_NAME(m_next), NULL)) {
	      object_follows_object_error (ms -> messages, ms -> tok, m_next_ptr);
	    } else {
	      generate_method_call (m_obj -> obj, m -> selector, 
				    m -> name, ms -> tok);
	    }
	  }
	} else {
	  generate_method_call (m_obj -> obj, m -> selector, 
				m -> name, ms -> tok);
	}
      }
      break;
    }

  ++m_obj -> evaled;
  ++m_obj -> output;

  return SUCCESS;

}

bool obj_rcvr_after_opening_parens (MESSAGE_STACK messages, 
				    int prefix_op_idx) {
  int i = prefix_op_idx;
  while (M_TOK(messages[--i]) == OPENPAREN)
    ;
  if (get_object (M_NAME(messages[i]), NULL))
    return true;
  return false;
}

/* Like need_rt_eval (), below, but this fn can also break off
   at commas and closing parentheses. */
int arg_needs_rt_eval (MESSAGE_STACK messages, int rcvr_ptr) {

  int i, j,
    stack_end,
    stack_start_idx,
    have_object,
    have_c_var,
    have_c_fn,
    have_complex_expression,
    n_parens = 0,
    is_single_token_param = FALSE;
  METHOD *method = NULL;
  MESSAGE *m;
  OBJECT *obj = NULL;
  CVAR *c;
  CFUNC *fn;

  /*
   *  Run time expressions in for and while loops get handled in
   *  loop_blk_start.
   */
  if (ctrlblk_pred)
    if (C_CTRL_BLK -> stmt_type == stmt_for)
      return FALSE;

  stack_end = get_stack_top (messages);
  stack_start_idx = stack_start (messages);

  if (interpreter_pass == method_pass)
    method = new_methods[new_method_ptr + 1] -> method;

  for (i = rcvr_ptr, have_object = FALSE, 
	 have_c_var = FALSE, have_complex_expression = FALSE; 
       i > stack_end; i--) {

    m = messages[i];
    if (M_ISSPACE(m))
      continue;

    /*
     *   If parsing a method, determine if the label is the name of 
     *   a parameter.  Otherwise check global and local objects and
     *   c variables.
     */
    if ((interpreter_pass == method_pass) && (M_TOK(m) == LABEL)) {
      for (j = 0; j < method -> n_params; j++) {
	if (!strcmp (method -> params[j] -> name, m -> name)) {
	  ++have_object;
	  if (i == (stack_end + 1)) {
	    is_single_token_param = TRUE;
	  }
	}
      }
    }

    /*
     *  Check for global and local objects.
     */
    if (!have_object)
      if ((obj = get_object (m -> name, NULL)) != NULL) {
	++have_object;
	continue;
      }

    /*
     *  Check for, "self," and, "super," keywords.
     */

    if (m -> attrs & TOK_SELF) {
      int _next_idx, _next_idx_2, _prev_idx, 
	have_method_proto = 0;
      OBJECT_CONTEXT self_cx;
      ++have_object;

      if (argblk) {

	/*
	 *  This probably needs to be an immediate return after
	 *  the first token - there's not enough info about
	 *  other cases yet.
	 */
	if ((self_cx = object_context (messages, rcvr_ptr)) == 
	    c_argument_context) {
	  if ((_next_idx = nextlangmsg (messages, rcvr_ptr)) != ERROR) {
	    if (!METHOD_ARG_TERM_MSG_TYPE (messages[_next_idx]))
	      return TRUE;
	  }
	}

      }

      /* NOT rcvr_ptr */
      if ((_next_idx = nextlangmsg (messages, i)) != ERROR) {
	if (IS_OBJECT(rcvr_class_obj)) {
	  if (complex_self_method_statement (rcvr_class_obj, 
					     messages, _next_idx) ||
	      self_class_or_instance_variable_lookahead (messages, 
							 i) ||
	      ((have_method_proto = 
		is_method_proto (rcvr_class_obj,
				 M_NAME(messages[_next_idx]))) == TRUE)) {
	    /* 
	     *  Case of self <math_op> <c_fn>
	     *  gets evaluated as a simple argument 
	     *  using __ctalk_arg, etc., 
	     *  instead of __ctalkEvalExpr.
	     */

	    if (have_method_proto &&
		(M_TOK(messages[_next_idx]) != LABEL)) {
	      if ((_next_idx_2 = nextlangmsg (messages, _next_idx))
		  != ERROR) {
		if ((fn = get_function (M_NAME(messages[_next_idx_2])))
		    == NULL) {
		  have_complex_expression = TRUE;
		}
	      } else {
		have_complex_expression = TRUE;
	      }
	    } else {
	      have_complex_expression = TRUE;
	    } /* if (have_method_proto && ... */
	  }
	}
      }
      if ((_prev_idx = prevlangmsg (messages, rcvr_ptr)) != ERROR) {
	if (messages[_prev_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	  if (IS_C_UNARY_MATH_OP(M_TOK(messages[_prev_idx]))) {
	    have_complex_expression = TRUE;
	  }
	} else {
	  if (M_TOK(messages[_prev_idx]) == OPENPAREN) {
	    do {
	      _prev_idx = prevlangmsg (messages, _prev_idx);
	    } while ((_prev_idx != ERROR) && 
		     (M_TOK(messages[_prev_idx]) == OPENPAREN));

	    if (_prev_idx != ERROR) {
	      if (messages[_prev_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
		if (IS_C_UNARY_MATH_OP(M_TOK(messages[_prev_idx]))) {
		  have_complex_expression = TRUE;
		}
	      }
	    }

	  }
	}
      }
      
      if (!have_complex_expression) {
	if (self_used_in_simple_object_deref (messages, i,
					      stack_start_idx,
					      stack_end)) {
	  --have_object;
	}
      }

    }
    if (m -> attrs & TOK_SUPER) {
      have_complex_expression = TRUE;
    }

    if (((c = get_local_var (m -> name)) != NULL) ||
	global_var_is_declared (m -> name)) {
      ++have_c_var;
    }

    if ((fn = get_function (m -> name)) != NULL) {
      ++have_c_fn;
    }

    if ((m -> tokentype == SEMICOLON) ||
	(m -> tokentype == CLOSEBLOCK) ||
	(m -> tokentype == ARRAYCLOSE))
      break;

    if (((m -> tokentype == DEREF) ||
	 (m -> tokentype == PERIOD)) &&
	have_object) {
      have_complex_expression = TRUE;
    }

    /* We can't always rely on the message having been evaled and
       having a TOK_IS_PREFIX_OP attribute, so check here. */
    if (IS_C_UNARY_MATH_OP(m -> tokentype)) {
      int next_idx, prev_idx;
      MESSAGE *m_prev, *m_next;
      if ((next_idx = nextlangmsg (messages, i)) != ERROR) {
	if ((prev_idx = prevlangmsg (messages, i)) != ERROR) {
	  m_prev = messages[prev_idx];
	  m_next = messages[next_idx];
	  if (IS_C_OP_TOKEN_NOEVAL(M_TOK(m_prev)) ||
	      (M_TOK(m_prev) == OPENPAREN) ||
	      (M_TOK(m_prev) == COLON) ||
	      (M_TOK(m_prev) == SEMICOLON) ||
	      (M_TOK(m_prev) == OPENBLOCK)) {
	    if ((obj = get_object (M_NAME(m_next), NULL)) != NULL) {
	      have_complex_expression = TRUE;
	    } else if (obj_rcvr_after_opening_parens (messages, i)) {
		have_complex_expression = TRUE;
	    }
	  }
	} else {
	  m_next = messages[next_idx];
	  if ((obj = get_object (M_NAME(m_next), NULL)) != NULL) {
	    have_complex_expression = TRUE;
	  } else if (obj_rcvr_after_opening_parens (messages, i)) {
	    have_complex_expression = TRUE;
	  }
	}
      }
    }

    if (m -> tokentype == ARRAYCLOSE || m -> tokentype == SEMICOLON ||
	m -> tokentype == CLOSEBLOCK)
      break;

    if (m -> tokentype == OPENPAREN) 
      ++n_parens;

    if (m -> tokentype == CLOSEPAREN) {
      --n_parens;
      if (n_parens < 0)
	break;
    }

    if (m -> tokentype == ARGSEPARATOR)
      if (!n_parens)
	break;

  }

  /*
   *  Expressions with objects within if, while, and do... while
   *  predicates require run time evaluation.
   */

  if (is_single_token_param)
    return FALSE;

  if ((have_object && ctrlblk_pred) ||
      have_complex_expression)
    return TRUE;

  return FALSE;
}   


int need_rt_eval (MESSAGE_STACK messages, int rcvr_ptr) {

  int i, j,
    stack_end,
    have_object,
    have_c_var,
    have_c_fn,
    have_complex_expression,
    have_c_fn_arg = 0,
    n_parens = 0,
    is_single_token_param = FALSE,
    prev_tok,
    _next_idx;
  METHOD *method = NULL;
  MESSAGE *m, *m_tok;
  OBJECT *obj = NULL;
  CVAR *c;
  CFUNC *fn;

  /*
   *  Run time expressions in for and while loops get handled in
   *  loop_blk_start.
   */
  if (ctrlblk_pred)
    if (C_CTRL_BLK -> stmt_type == stmt_for)
      return FALSE;

  stack_end = get_stack_top (messages);

  if (interpreter_pass == method_pass)
    method = new_methods[new_method_ptr + 1] -> method;

  if ((_next_idx = nextlangmsg (messages, rcvr_ptr)) == ERROR)
    return FALSE;

  for (i = rcvr_ptr, have_object = FALSE, 
	 have_c_var = FALSE, have_complex_expression = FALSE;
       i > stack_end; i--) {

    m = messages[i];
    if (M_ISSPACE(m))
      continue;

    if (M_TOK(m) == OPENPAREN) {
      ++n_parens;
      continue;
    }

    if (M_TOK(m) == CLOSEPAREN) {
      --n_parens;
      if (n_parens < 0)
	break;
    }

    /*
     *   If parsing a method, determine if the label is the name of 
     *   a parameter.  Otherwise check global and local objects and
     *   c variables.
     */
    if ((interpreter_pass == method_pass) && (M_TOK(m) == LABEL)) {
      for (j = 0; j < method -> n_params; j++) {
	if (!strcmp (method -> params[j] -> name, m -> name)) {
	  ++have_object;
	  if (i == (stack_end + 1)) {
	    return FALSE;  /* i.e., is_single_token_param = TRUE */
	  } else {
	    continue;
	  }
	}
      }
    }

    /*
     *  Check for global and local objects.
     */
    if (!have_object)
      if ((obj = get_object (m -> name, NULL)) != NULL) {
	++have_object;
	continue;
      }

    /*
     *  Check for, "self," and, "super," keywords.
     */

    if (m -> attrs & TOK_SELF) {
      int _next_idx_2, _prev_idx, 
	have_method_proto = 0;
      OBJECT_CONTEXT self_cx;
      ++have_object;

	if (argblk) {
	  if ((self_cx = object_context (messages, rcvr_ptr)) == 
	      c_argument_context) {
	    if ((_next_idx = nextlangmsg (messages, rcvr_ptr)) != ERROR) {
	      if (!METHOD_ARG_TERM_MSG_TYPE (messages[_next_idx]))
		return TRUE;
	    }
	  }
	}

	if (ctrlblk_pred) {
	  if (!METHOD_ARG_TERM_MSG_TYPE(messages[_next_idx])) {
	    return TRUE;
	  }
	}

	if (IS_OBJECT(rcvr_class_obj)) {
	  if (complex_self_method_statement (rcvr_class_obj, 
					     messages, _next_idx) ||
	      self_class_or_instance_variable_lookahead (messages, 
							 rcvr_ptr)) {
	    have_complex_expression = TRUE;
	  } else if ((have_method_proto = 
		      is_method_proto (rcvr_class_obj,
				       M_NAME(messages[_next_idx])))
		     == TRUE) {
	    /* 
	     *  Case of self <math_op> <c_fn>
	     *  gets evaluated as a simple argument 
	     *  using __ctalk_arg, etc., 
	     *  instead of __ctalkEvalExpr.
	     */

	    if (have_method_proto &&
		(M_TOK(messages[_next_idx]) != LABEL)) {
	      if ((_next_idx_2 = nextlangmsg (messages, _next_idx))
		  != ERROR) {
		if ((fn = get_function (M_NAME(messages[_next_idx_2])))
		    == NULL) {
		  have_complex_expression = TRUE;
		}
	      } else {
		have_complex_expression = TRUE;
	      }
	    } else {
	      /* Check for a c_fn here, too, a method argument. */
	      for (j = rcvr_ptr; have_c_fn_arg == FALSE; --j) {
		if (messages[j] == NULL)
		  break;
		if (M_ISSPACE(messages[j]))
		  continue;
		if (M_TOK(messages[j]) == LABEL &&
		    get_function (M_NAME(messages[j]))) {
		  if ((prev_tok = prevlangmsg_np (messages, j)) != ERROR) {
		    if (M_TOK(messages[prev_tok]) == ARGSEPARATOR) {
		      have_c_fn_arg = TRUE;
		    } else if (M_TOK(messages[prev_tok]) == LABEL) {
		      m_tok = messages[prev_tok];
		      if (get_instance_method (m_tok, rcvr_class_obj,
					       M_NAME(m_tok),
					       ANY_ARGS, FALSE) ||
			  get_class_method (m_tok, rcvr_class_obj,
					    M_NAME(m_tok),
					    ANY_ARGS, FALSE) ||
			  is_method_proto (rcvr_class_obj,
					   M_NAME(m_tok))) {
			have_c_fn_arg = TRUE;
		      }
		    }
		  }
		}
		if ((M_TOK(messages[j]) == SEMICOLON) ||
		    (M_TOK(messages[j]) == CLOSEBLOCK) ||
		    (M_TOK(messages[j]) == ARRAYCLOSE)) 
		  break;
	      }
	      if (!have_c_fn_arg) {
		have_complex_expression = TRUE;
	      }
		/* have_complex_expression = TRUE; */
	    } /* if (have_method_proto && ... */
	  }
	}
      if ((_prev_idx = prevlangmsg (messages, rcvr_ptr)) != ERROR) {
	if (messages[_prev_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	  if (IS_C_UNARY_MATH_OP(M_TOK(messages[_prev_idx]))) {
	    have_complex_expression = TRUE;
	  }
	} else {
	  if (M_TOK(messages[_prev_idx]) == OPENPAREN) {
	    do {
	      _prev_idx = prevlangmsg (messages, _prev_idx);
	    } while ((_prev_idx != ERROR) && 
		     (M_TOK(messages[_prev_idx]) == OPENPAREN));

	    if (_prev_idx != ERROR) {
	      if (messages[_prev_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
		if (IS_C_UNARY_MATH_OP(M_TOK(messages[_prev_idx]))) {
		  have_complex_expression = TRUE;
		}
	      }
	    }

	  }
	}
      }
      
      if (!have_complex_expression) {
	if (self_used_in_simple_object_deref (messages, i,
					      stack_start (messages),
					      stack_end)) {
	  --have_object;
	}
      }

    } else { /* if (m -> attrs & TOK_SELF) */
      if ((prev_tok = prevlangmsg_np (messages, rcvr_ptr)) != ERROR) {
	if (messages[prev_tok] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	  have_complex_expression = TRUE;
	}
      }
    }   /* if (m -> attrs & TOK_SELF) */
    
    if (m -> attrs & TOK_SUPER) {
      have_complex_expression = TRUE;
    }

    if (((c = get_local_var (m -> name)) != NULL) ||
	global_var_is_declared (m -> name)) {
      ++have_c_var;
      continue;
    }

    if ((fn = get_function (m -> name)) != NULL) {
      ++have_c_fn;
      continue;
    }

    if (((m -> tokentype == DEREF) ||
	 (m -> tokentype == PERIOD)) &&
	have_object) {
      have_complex_expression = TRUE;
    } else if (m -> tokentype == ARRAYCLOSE ||
	       m -> tokentype == SEMICOLON ||
	       m -> tokentype == CLOSEBLOCK) {
      break;
    }

  }

  /*
   *  Expressions with objects within if, while, and do... while
   *  predicates require run time evaluation.
   */

  if (is_single_token_param)
    return FALSE;

  if ((have_object && ctrlblk_pred) ||
      have_complex_expression)
    return TRUE;

  return FALSE;
}   

int is_single_token_method_param (MESSAGE_STACK messages, int tok_idx, 
				  METHOD *method) {
  int next_idx, n_th_param;

  if ((next_idx = nextlangmsg (messages, tok_idx)) != ERROR) {
    if (METHOD_ARG_TERM_MSG_TYPE (messages[next_idx])) {

      for (n_th_param = 0; n_th_param < method -> n_params; n_th_param++) {

	if (str_eq (M_NAME(messages[tok_idx]), 
		    method -> params[n_th_param] -> name))
	  return TRUE;

      }

    }
  }

  return FALSE;
}

/*
 *  Use the parser to check a constant expression without writing
 *  any output. 
 */
int check_constant_expr (MESSAGE_STACK messages, int start, int end) {

  char buf[MAXMSG];
  int i,
    p_ctrlblk_pred;
  I_PASS p_pass;

  for (i = start, *buf = 0; i >= end; i--)
    strcatx2 (buf, messages[i] -> name, NULL);

  p_pass = interpreter_pass;
  p_ctrlblk_pred = ctrlblk_pred;
  ctrlblk_pred = 0;
  interpreter_pass = expr_check;
  parse (buf, strlen (buf));
  /*
   *  "Self" needs to be determined for an interpreter pass,
   *  and probably a few other things before the interpreter
   *  can use the library call.
   */
  interpreter_pass = p_pass;
  ctrlblk_pred = p_ctrlblk_pred;
  return SUCCESS;
}

/*
 *  Determine if the C function is a library function
 *  that handles complete objects.  See the IS_OBJ_FN
 *  macro in object.h.  This should only be called in 
 *  a C context.
 *
 *  TO DO - This only works if there are no intervening
 *  opening parenthesis between the object token and 
 *  the start of the argument list.  Rewrite so 
 *  that the function can determine the start of a 
 *  complex argument list, including within a control
 *  block predicate.
 */


/*
 *  Called by method_args (), so the expression always
 *  has an object to cope with.
 *
 *  If we can use a __ctalk_arg () call, then call the
 *  function from within the __ctalk_arg () call, with
 *  an object wrapper.  If the function requires
 *  a run-time evaluation, then register the function
 *  result.  We also have to do this for multiple
 *  argument methods, below.
 *
 *  This function should be called for functions that are 
 *  arguments.  That means method_msg_ptr points to the
 *  method, and arg_start and arg_end point to the 
 *  first token of the function and the closing paren,
 *  respectively.  However, we check here for a valid function
 *  name in several places.
 *
 *  If within a control block predicate, wait until we 
 *  format the expression.
 */

int fn_output_context (MESSAGE_STACK messages, int method_msg_ptr,
		       OBJECT *arg_object, METHOD *method, 
		       int arg_start, int arg_end) {

  int rcvr_ptr, fn_ptr, method_context_ptr;
  MESSAGE *m_method;

  if (ctrlblk_pred) return SUCCESS;

  m_method = messages[method_msg_ptr];

  if ((rcvr_ptr = prevlangmsg (message_stack (), method_msg_ptr))
      == ERROR) {
    warning (message_stack_at (method_msg_ptr),
	     "Unknown receiver in method_args ().");
    return ERROR;
  }

  /*
   *  The function name should be at the start of the argument,
   *  but if it isn't, search for it within the expression.
   */
  fn_ptr = arg_start;

  if ((method_context_ptr = prevlangmsg (messages, arg_start)) == ERROR) {
    warning (message_stack_at (method_msg_ptr),
	     "Unknown function in method_args ().");
    return ERROR;
  }

  if (!strcmp (M_NAME(messages[method_context_ptr]), method -> name)) {
    return FN_IS_ARG_RETURN_VAL;
  } else if (M_TOK(messages[method_context_ptr]) == ARGSEPARATOR) {
    /* The function is in an arglist... handle later */
    return FN_IS_METHOD_ARG;
  } else {
    OBJECT_CONTEXT __c;
    __c = object_context (messages, fn_ptr);
    switch (__c)
      {
      case c_argument_context:
	if (arg_class == arg_c_fn_expr) {
	  if ((arg_start == arg_c_fn_terms[arg_c_fn_term_ptr - 1].start) &&
	      (arg_end == arg_c_fn_terms[arg_c_fn_term_ptr - 1].end)) {
	    return FN_IS_ARG_RETURN_VAL;
	  } else {
	    return FN_IS_METHOD_ARG;
	  }
	} else {
	  return FN_IS_ARG_RETURN_VAL;
	}
	break;
      case argument_context:
	return FN_IS_METHOD_ARG;
	break;
      case c_context:
	if (!(messages[fn_ptr] -> attrs & OBJ_IS_SINGLE_TOK_ARG_ACCESSOR)) {
	  /* The attribute is set in resolve_single_token_arg in eval_arg.c,
	     and output already. */
	  generate_c_to_obj_call
	    (messages, fn_ptr,
	     m_method -> receiver_obj, method, arg_object);
	}
	break;
      default:
	generate_c_to_obj_call
	  (messages, fn_ptr,
	   m_method -> receiver_obj, method, arg_object);
	break;
      }
  }

  return SUCCESS;
}

int expr_has_objects (MESSAGE_STACK messages, int start_idx, int end_idx) {

  int i;
  MESSAGE *m;

  for (i = start_idx; i >= end_idx; i--) {
    m = messages[i];
    if (m && m -> obj && !strcmp (M_NAME(m), m -> obj -> __o_name))
      return TRUE;
  }
  return FALSE;
}

/*
 * Expression like <classname> <classvar>.
 */
#define CLSEXPR_START_IDX ((prefix_op_idx != -1) ? prefix_op_idx : \
			    prev_label_ptr)
int class_variable_expression (MESSAGE_STACK messages, int idx) {
  int prev_label_ptr, next_label_ptr, i, prefix_op_idx = -1;
  int fn_idx, arg_idx;
  int end_idx;
  char exprbuf[MAXMSG];
  static char expr_buf_tmp[MAXMSG];  /* for use by rt_expr's fileout call*/
  char expr_buf_tmp_2[MAXMSG];
  OBJECT_CONTEXT context;
  OBJECT *classvariable_object, *classvariable_value_object;
  METHOD *method;
  MSINFO ms;

  prev_label_ptr = prevlangmsg (messages, idx);

  if ((prefix_op_idx = is_leading_prefix_op (messages, prev_label_ptr,
					     stack_start (messages)))
      != ERROR) {
    if (!(messages[prefix_op_idx] -> attrs & TOK_IS_PREFIX_OPERATOR)) {
      prefix_op_idx = -1;
    }
  }
  ms.messages = messages;
  ms.stack_start = P_MESSAGES;
  ms.stack_ptr = get_stack_top (messages);
  ms.tok = CLSEXPR_START_IDX;
  end_idx = find_expression_limit (&ms);

  if (prev_label_ptr == ERROR)
    return ERROR;

  messages[idx]->receiver_obj = messages[prev_label_ptr]->obj;
  context = object_context (message_stack (), prev_label_ptr);

  if (complex_method_statement (messages[prev_label_ptr]->obj, 
				messages, idx)) {
    switch (context)
      {
      case c_context:
      case c_argument_context:
	if ((next_label_ptr = nextlangmsg (messages, idx)) != ERROR) {
	  if ((classvariable_object = 
	       get_class_variable (M_NAME(messages[idx]), 
				   messages[idx]->receiver_obj->__o_name,
				   FALSE)) != NULL) {
	    classvariable_value_object = 
	      ((classvariable_object -> instancevars != NULL) ?
	      classvariable_object -> instancevars :
	       classvariable_object);
	    if (((method = 
		  get_instance_method (messages[idx], 
				       classvariable_value_object,
				       M_NAME(messages[next_label_ptr]),
				       ERROR, FALSE)) != NULL) ||
		((method = 
		  get_class_method (messages[idx], 
				       classvariable_value_object,
				       M_NAME(messages[next_label_ptr]),
				    ERROR, FALSE)) != NULL)) {

	      if (messages[idx] -> attrs & TOK_IS_PRINTF_ARG) {
		OBJECT *class_object;
		if ((class_object = 
		     get_class_object 
		     (classvariable_value_object -> __o_classname))
		    != NULL) {
		  if (IS_COLLECTION_SUBCLASS_OBJ (class_object)) {
		    fileout 
		      (array_obj_2_c_wrapper
		       (messages, idx, 
			messages[idx]->receiver_obj, 
			method,
			fmt_rt_expr (messages, 
				     CLSEXPR_START_IDX,
				     &end_idx, expr_buf_tmp)),
		       FALSE, prev_label_ptr);
		  } else {
		    fileout 
		      (obj_2_c_wrapper
		       (messages[idx], 
			messages[idx]->receiver_obj, 
			method,
			fmt_rt_expr (messages, 
				     CLSEXPR_START_IDX,	&end_idx,
				     expr_buf_tmp),
			TRUE), FALSE, prev_label_ptr);
		  }
		} else {
		  fileout 
		    (obj_2_c_wrapper
		     (messages[idx], 
		      messages[idx]->receiver_obj, 
		      method,
		      fmt_rt_expr (messages, CLSEXPR_START_IDX, &end_idx,
				   expr_buf_tmp), 
		      TRUE),
		     FALSE, prev_label_ptr);
		}
	      } else {
		fileout 
		  (obj_2_c_wrapper
		   (messages[idx], 
		    messages[idx]->receiver_obj, 
		    method,
		    fmt_rt_expr (messages, CLSEXPR_START_IDX, &end_idx,
				 expr_buf_tmp), 
		    TRUE),
		   FALSE, prev_label_ptr);
	      }
		for (i = CLSEXPR_START_IDX; i >= end_idx; i--) {
		  ++messages[i]->evaled;
		  ++messages[i]->output;
		}
	    } else {
	      /*
	       *  Method not found - don't translate with 
	       *  array_obj_2_c_wrapper.
	       */
	      rt_expr (messages, CLSEXPR_START_IDX, &end_idx, expr_buf_tmp);
	    }
	  } else {
	    /*
	     *  Class variable object not found.
	     */
	    rt_expr (messages, CLSEXPR_START_IDX, &end_idx, expr_buf_tmp);
	  }
	} else {
	  /*
	   *  Terminal class variable message - 
	   *  should not be reached.
	   */
	  rt_expr (messages, ((prefix_op_idx != -1) ? prefix_op_idx :
			      prev_label_ptr), &end_idx, expr_buf_tmp);
	}
	break;
      default:
	rt_expr (messages, CLSEXPR_START_IDX, &end_idx, expr_buf_tmp);
	break;
      }
  } else {
    /*
     *  Warn if there's a nonterminal next message.
     */
    int _next_idx;
    OBJECT *_class_variable_object;
    char _variable_value_class_name[MAXLABEL];
    if (((_next_idx = nextlangmsg (messages, idx)) != ERROR) &&
	!METHOD_ARG_TERM_MSG_TYPE(messages[_next_idx])) {
      toks2str (message_stack (), CLSEXPR_START_IDX, end_idx, exprbuf);
      warning (messages[idx], "Unknown statement, \"%s.\"",exprbuf);
    }
    toks2str (message_stack (), CLSEXPR_START_IDX, end_idx, exprbuf);
    switch (context)
      {
      case c_context:
      case c_argument_context:
	if (ctrlblk_pred) {
	  ctrlblk_pred_rt_expr (message_stack (), idx);
	} else { /* if (ctrlblk_pred) { */
	  if ((_class_variable_object = 
	       get_class_variable (M_NAME(messages[idx]), 
				   messages[idx]->receiver_obj->__o_name,
				   FALSE)) == NULL) {
	    warning(
		    messages[idx], 
		    "Class variable expression: Unknown receiver class %s, defaulting to Integer.", messages[idx]->receiver_obj->__o_name);
	    strcpy (_variable_value_class_name, INTEGER_CLASSNAME);
	  } else {
	    if (_class_variable_object -> instancevars) {
	      strcpy (_variable_value_class_name, 
		      _class_variable_object->instancevars->__o_classname);
	    } else {
	      warning( messages[idx], 
		       "Class variable expression: Unknown variable %s value, class defaulting to Integer.", _class_variable_object -> __o_name);
	    }
	  }
	  if ((arg_idx = 
	       obj_expr_is_arg (message_stack (), prev_label_ptr,
				P_MESSAGES,
				&fn_idx)) != ERROR) {
	  
	    CFUNC *cfn;
	    if ((cfn = get_function (M_NAME(messages[fn_idx]))) != NULL) {
	      int __n_th_arg;
	      CVAR *__arg;
	      for (__n_th_arg = 0, __arg = cfn -> params; 
		   __n_th_arg < arg_idx; __n_th_arg++) 
		__arg = __arg -> next;
	      fileout 
		(fmt_rt_return 
		 (fmt_eval_expr_str (exprbuf,
				     expr_buf_tmp), 
		  basic_class_from_cvar
		  (messages[idx], __arg, 0),
		  TRUE, expr_buf_tmp_2), FALSE, idx);
	    } else {
	      _warning ("class variable expression: could not find function %s.\n", M_NAME(messages[fn_idx]));
	    }
	  } else {
	    fileout 
	      (fmt_rt_return 
	       (fmt_eval_expr_str (exprbuf,
				   expr_buf_tmp), 
		_variable_value_class_name, TRUE, expr_buf_tmp_2),
	       FALSE, idx);
	  }
	} /* if (ctrlblk_pred) { */
	break;
      default:
	fileout ( fmt_eval_expr_str (exprbuf, expr_buf_tmp), FALSE, idx);
	break;
      }
    for (i = CLSEXPR_START_IDX; i >= end_idx; i--) {
      ++messages[i] -> evaled;
      ++messages[i] -> output;
    }
  }
  return SUCCESS;
}

/*
 *  Expressions like self <classvariable>,
 *  or self <instancevariable>, but not
 *  "self value" which is handled by
 *  default_method ().
 */
int self_class_or_instance_variable_lookahead (MESSAGE_STACK messages, 
						int idx) {
  int next_label_ptr, next_tok_ptr;
  OBJECT *var_object, *superclass_obj;


  if ((next_label_ptr = nextlangmsg (messages, idx)) == ERROR) {
    return FALSE;
  } else {
    if (METHOD_ARG_TERM_MSG_TYPE(messages[next_label_ptr])) {
      return FALSE;
    } else {
      if (M_TOK(messages[next_label_ptr]) != LABEL) { 
	return FALSE;
      } else {
 	if (!strcmp (M_NAME(messages[next_label_ptr]), "value")) {

	  if ((next_tok_ptr = nextlangmsg (messages, next_label_ptr)) 
	      == ERROR)
	    return FALSE;

	  if (METHOD_ARG_TERM_MSG_TYPE(messages[next_tok_ptr]))
	    return FALSE;
	  else
	    return TRUE;

	}
      }
    } 
  }

  if (IS_OBJECT(rcvr_class_obj)) {

    for (superclass_obj = rcvr_class_obj; superclass_obj;
	 superclass_obj = superclass_obj -> __o_superclass) {

      for (var_object = superclass_obj -> classvars; 
	   var_object; var_object = var_object -> next) {
	if (!strcmp (M_NAME(messages[next_label_ptr]), 
		     var_object -> __o_name))
	  return TRUE;
      }

      for (var_object = superclass_obj -> instancevars; 
	   var_object; var_object = var_object -> next) {
	if (!strcmp (M_NAME(messages[next_label_ptr]), 
		     var_object -> __o_name))
	  return TRUE;
      }
    }
  }

  return FALSE;

}

/* like above, but looks for a class cast before self... used
   in ifexpr.c so far for label checking.
*/
int self_class_or_instance_variable_lookahead_2 (MESSAGE_STACK messages, 
						 int idx) {
  int next_label_ptr, next_tok_ptr,
    prev_tok_ptr, i;
  OBJECT *var_object, *cast_object, *superclass_obj;


  if ((next_label_ptr = nextlangmsg (messages, idx)) == ERROR) {
    return FALSE;
  } else {
    if (METHOD_ARG_TERM_MSG_TYPE(messages[next_label_ptr])) {
      return FALSE;
    } else {
      if (M_TOK(messages[next_label_ptr]) != LABEL) { 
	return FALSE;
      } else {
 	if (!strcmp (M_NAME(messages[next_label_ptr]), "value")) {

	  if ((next_tok_ptr = nextlangmsg (messages, next_label_ptr)) 
	      == ERROR)
	    return FALSE;

	  if (METHOD_ARG_TERM_MSG_TYPE(messages[next_tok_ptr]))
	    return FALSE;
	  else
	    return TRUE;

	}
      }
    } 
  }

  /* look for a class cast before the, "self," keyword. */
  if ((prev_tok_ptr = prevlangmsg (messages, idx)) != ERROR) {
    if (M_TOK(messages[prev_tok_ptr]) == CLOSEPAREN) {
      for (i = prev_tok_ptr; ; ++i) {
	if (M_TOK(messages[i]) == OPENPAREN)
	  break;
	if (messages[i] -> attrs & TOK_IS_CLASS_TYPECAST) {
	  if ((cast_object = get_class_object (M_NAME(messages[i])))
	      != NULL) {
	    for (superclass_obj = cast_object; superclass_obj;
		 superclass_obj = superclass_obj -> __o_superclass) {

	      for (var_object = superclass_obj -> classvars; 
		   var_object; var_object = var_object -> next) {
		if (!strcmp (M_NAME(messages[next_label_ptr]), 
			     var_object -> __o_name))
		  return TRUE;
	      }

	      for (var_object = superclass_obj -> instancevars; 
		   var_object; var_object = var_object -> next) {
		if (!strcmp (M_NAME(messages[next_label_ptr]), 
			     var_object -> __o_name))
		  return TRUE;
	      }
	    }
	    if (!get_instance_method (messages[next_label_ptr],
				      cast_object,
				      M_NAME(messages[next_label_ptr]),
				      ANY_ARGS, FALSE) &&
		!get_class_method (messages[next_label_ptr],
				   cast_object,
				   M_NAME(messages[next_label_ptr]),
				   ANY_ARGS, FALSE) &&
		!is_method_proto (cast_object,
				  M_NAME(messages[next_label_ptr]))) {
	      warning (messages[next_label_ptr],
		       "Undefined label, \"%s,\" in the expression,\n\n\t"
		       "(%s *)self %s\n",
		       M_NAME(messages[next_label_ptr]),
		       cast_object -> __o_name,
		       M_NAME(messages[next_label_ptr]));
	    }
	  }
	  return FALSE;
	}
      }
    }
  }

  /* If there's no class cast expression before the, "self,"
     keyword, then the expression is unresolvable until
     run time. */
  if (argblk) 
    return FALSE;

  if (IS_OBJECT(rcvr_class_obj)) {

    for (superclass_obj = rcvr_class_obj; superclass_obj;
	 superclass_obj = superclass_obj -> __o_superclass) {

      for (var_object = superclass_obj -> classvars; 
	   var_object; var_object = var_object -> next) {
	if (!strcmp (M_NAME(messages[next_label_ptr]), 
		     var_object -> __o_name))
	  return TRUE;
      }

      for (var_object = superclass_obj -> instancevars; 
	   var_object; var_object = var_object -> next) {
	if (!strcmp (M_NAME(messages[next_label_ptr]), 
		     var_object -> __o_name))
	  return TRUE;
      }
    }
  }

  return FALSE;

}

/*
 *  This is fairly limited right now.  It simply
 *  looks for a terminal object on the right side 
 *  of a nonterminal C token (i.e., a NOEVAL token),
 *  that is preceded by a terminal C token (i.e., a CVAR).
 *
 *  So what we get mostly is a better selection of 
 *  statements that look like this, for example:
 *
 *   <cvar_tok> = <object_tok>
 *
 *  Then we can try to cast the object (or its value) to the type 
 *  of the CVAR.
 *
 * 
 *  Actually, we're only going to start with 
 *
 *   <OBJECT *something> = Object;
 *
 *  Then work into other cases so we don't screw things up.
 *  
 *  Update: Try 
 *
 *   <OBJECT *something> <math_op> Object
 *
 *  Another Update:
 *
 *  Try use_new_c_rval_semantics_b () for statements in argument
 *  blocks.
 */
int use_new_c_rval_semantics (MESSAGE_STACK messages, 
			      int rval_obj_ptr) {
  int prev_tok_idx,
    prev_tok_idx_2,
    next_tok_idx;
  CVAR *l_cvar;

  if ((prev_tok_idx = prevlangmsg (messages, rval_obj_ptr)) != ERROR) {

    if ((prev_tok_idx_2 = prevlangmsg (messages, prev_tok_idx)) != ERROR) {

      if (IS_C_BINARY_MATH_OP(M_TOK(messages[prev_tok_idx]))) {

	if (((l_cvar = get_local_var (M_NAME(messages[prev_tok_idx_2]))) 
	     != NULL) ||
	    ((l_cvar = get_global_var (M_NAME(messages[prev_tok_idx_2]))) 
	     != NULL)) {

	  if ((l_cvar -> type_attrs & CVAR_TYPE_OBJECT) &&
	      (l_cvar -> n_derefs == 1)) {

	    if ((next_tok_idx = nextlangmsg (messages, rval_obj_ptr)) != 
		ERROR) {

	      if (METHOD_ARG_TERM_MSG_TYPE (messages[next_tok_idx]))  {

		return TRUE;

	      }
	    }
	  }
	}
      }
    }
  }

  /*
   *  We should already have set the TOK_IS_PRINTF_ARG attribute
   *  on the object's message.  
   *
   *  printf formats that work with pointers are "%p" and,
   *  on most systems, "%#x".
   */

  if (messages[rval_obj_ptr] -> attrs & TOK_IS_PRINTF_ARG) {

    if ((next_tok_idx = nextlangmsg (messages, rval_obj_ptr)) != 
	ERROR) {

      if (METHOD_ARG_TERM_MSG_TYPE (messages[next_tok_idx]))  {

	if (fmt_arg_type (messages, rval_obj_ptr, stack_start (messages))
	    == fmt_arg_ptr) {
	  return TRUE;
	}
      }
    }
  }

  return FALSE;
}

/* Like above, but checks for argument block cvar aliases and 
   more lval types that simply OBJECT *'s.  Also does not choke on 
   complex right-hand expressions.  Called by rt_self_expr ()
   argblk_super_expr () for expressions in argument context.
*/
char *use_new_c_rval_semantics_b (MESSAGE_STACK messages, 
				int rval_obj_ptr) {
  int prev_tok_idx,
    prev_tok_idx_2,
    next_tok_idx;
  CVAR *l_cvar;
  static char *lval_class = NULL;

  if ((prev_tok_idx = prevlangmsg (messages, rval_obj_ptr)) != ERROR) {

    if ((prev_tok_idx_2 = prevlangmsg (messages, prev_tok_idx)) != ERROR) {

      if (IS_C_BINARY_MATH_OP(M_TOK(messages[prev_tok_idx]))) {
	if (argblk) {
	  if ((l_cvar = 
	       get_var_from_cvartab_name (M_NAME(messages[prev_tok_idx_2])))
	      != NULL) {
	    lval_class = basic_class_from_cvar
	      (messages[prev_tok_idx_2], l_cvar, 0);
	  } else {
	    if (((l_cvar = get_local_var (M_NAME(messages[prev_tok_idx_2]))) 
		 != NULL) ||
		((l_cvar = get_global_var (M_NAME(messages[prev_tok_idx_2]))) 
		 != NULL)) {
	      lval_class = basic_class_from_cvar
		(messages[prev_tok_idx_2], l_cvar, 0);
	    }
	  }
	}
      }
    }
  }

  /*
   *  We should already have set the TOK_IS_PRINTF_ARG attribute
   *  on the object's message.  
   *
   *  printf formats that work with pointers are "%p" and,
   *  on most systems, "%#x".
   */

  if (messages[rval_obj_ptr] -> attrs & TOK_IS_PRINTF_ARG) {

    if ((next_tok_idx = nextlangmsg (messages, rval_obj_ptr)) != 
	ERROR) {

      if (METHOD_ARG_TERM_MSG_TYPE (messages[next_tok_idx]))  {

	if (fmt_arg_type (messages, rval_obj_ptr,
			  stack_start (messages))
	    == fmt_arg_ptr) {
	  strcpy (lval_class, OBJECT_CLASSNAME);
	  return lval_class;

	}
      }
    }
  }

  return lval_class;
}

/*
 *  use_new_c_rval_semantics () already did the syntax checking, so 
 *  all we should do here is try to cast an object to the cvar 
 *  on the left-hand side of an operator.
 */
int terminal_rexpr (MESSAGE_STACK messages, int terminal_rval_object) {
  int prev_tok_idx,
    prev_tok_idx_2;
  int end_ptr;
  CVAR *l_cvar;
  char expr_buf[MAXMSG];

  if ((prev_tok_idx = prevlangmsg (messages, terminal_rval_object)) !=
      ERROR) {

    if ((prev_tok_idx_2 = prevlangmsg (messages, prev_tok_idx)) !=
	ERROR) {

      if (((l_cvar = get_local_var (M_NAME(messages[prev_tok_idx_2]))) 
	   != NULL) ||
	  ((l_cvar = get_global_var (M_NAME(messages[prev_tok_idx_2]))))) {

	if ((l_cvar -> type_attrs & CVAR_TYPE_OBJECT) &&
	    (l_cvar -> n_derefs == 1)) {
	  /* Just __ctalkEvalExpr() (returns an OBJECT *) on the right. */
	  /*
	   *  However, if the  CVAR is an OBJECT *, then we create a
	   *  reference to the right-hand object's tag with _makeRef, 
	   *  so we always get the current object associated with the
	   *  tag when we by retrieving the tag first whereever the 
	   *  CVAR occurs later.
	   *
	   *  Except we exclude Symbol, Key, and similar objects which
	   *  already point to something else, which (for now at
	   *  least), seems a little superfluous....
	   */
	  if ((!str_eq (messages[terminal_rval_object] -> obj -> __o_classname,
		       SYMBOL_CLASSNAME) ||
	      is_subclass_of (messages[terminal_rval_object] -> 
			      obj -> __o_classname, 
			      SYMBOL_CLASSNAME)) &&
	      (M_TOK(messages[prev_tok_idx]) == EQ)) {
	    /* Sort of superfluous for symbols references so far... */
	    new_object_reference (M_NAME(messages[terminal_rval_object]),
				  l_cvar -> name, l_cvar -> scope, expr_buf);
	  } else {
	    fmt_rt_expr (messages, terminal_rval_object, &end_ptr, expr_buf);
	  }

	  fileout (expr_buf, 0, terminal_rval_object);
	  ++messages[terminal_rval_object] -> evaled;
	  ++messages[terminal_rval_object] -> output;
	} else {

	  return ERROR;

	}

      } else {

	return ERROR;

      }

    }

  }

  return SUCCESS;
}

/*
 *  use_new_c_rval_semantics () has already checked for a 
 *  "%p" or "%#x" printf format, so this should be all
 *  we need.
 */

/* Tells us whether the format is "%p" or "%#x" */
/* Declared in pattypes.c. */
extern bool ptr_fmt_is_alt_int_fmt;

int terminal_printf_arg (MESSAGE_STACK messages, int terminal_rval_object) {

  char expr_buf[MAXMSG];
  int end_ptr;

  fmt_rt_expr (messages, terminal_rval_object, &end_ptr, expr_buf);
  if (ptr_fmt_is_alt_int_fmt) {

    /* Check if we need a type cast to avoid a warning. */
#if defined (__GNUC__) && defined (i386)
    char expr_buf_2[MAXMSG];

    strcatx (expr_buf_2, ALT_PTR_FMT_CAST, expr_buf, NULL);
    fileout (expr_buf_2, 0, terminal_rval_object);

  } else {

    fileout (expr_buf, 0, terminal_rval_object);

  }

#else
#if defined (__GNUC__) && defined (__APPLE__)
    char expr_buf_2[MAXMSG];

    strcatx (expr_buf_2, ALT_PTR_FMT_CAST, expr_buf, NULL);
    fileout (expr_buf_2, 0, terminal_rval_object);

  } else {

    fileout (expr_buf, 0, terminal_rval_object);

  }
#else

  } else {

  /* Untested anywhere else. */
    fileout (expr_buf, 0, terminal_rval_object);

  }

#endif /* #if defined (__GNUC__) && defined (__APPLE__) */
#endif /* #if defined (__GNUC__) && defined (i386) */
  
  ++messages[terminal_rval_object] -> evaled;
  ++messages[terminal_rval_object] -> output;

  return SUCCESS;
}

/*
 *  This is the same procedure that object_context
 *  uses to look for a C argument context. 
 *  Called by rt_self_expr ().
 */
int lval_idx_from_arg_start (MESSAGE_STACK messages,
					 int arg_first_label_idx) {
  int lookback;

  lookback = arg_first_label_idx;

  while ((lookback = prevlangmsg (messages, lookback)) != ERROR) {
    if ((messages[lookback] -> tokentype == LABEL) &&
	(messages[lookback] -> obj == NULL))
      break;
  }
  return lookback;
}

/*
 *  Translate an assignment expression like 
 *    <Symbol_obj> = &<arg>
 *
 *  into
 *
 *    *<Symbol_obj> = <arg>
 *
 *  Called from resolve () _only_.
 */
int rval_ptr_context_translate (MSINFO *ms, int prev_label_idx) {  
  char *pfx_op_idx, arg_buf[MAXMSG], expr_buf[MAXMSG],
    rt_expr_buf[MAXMSG], rt_expr_buf_esc[MAXMSG];
  int arg_start_idx, arg_end_idx, idx;
  
  if (!IS_C_ASSIGNMENT_OP(M_TOK(ms -> messages[ms -> tok])))
    return FALSE;

  if (!ms -> messages[prev_label_idx] -> obj ||
      !str_eq (ms -> messages[prev_label_idx] -> obj -> __o_classname,
	       SYMBOL_CLASSNAME))
    return FALSE;

  if ((arg_start_idx = nextlangmsg (ms -> messages, ms -> tok))
      == ERROR)
    return FALSE;

  if (M_TOK(ms -> messages[arg_start_idx]) != AMPERSAND)
    return FALSE;

  arg_end_idx = find_expression_limit (ms);

  toks2str (ms -> messages, arg_start_idx, arg_end_idx, arg_buf);

  if ((pfx_op_idx = strchr (arg_buf, '&')) == NULL)
    return FALSE;
  *pfx_op_idx = ' ';
  strcatx (expr_buf, "*", M_NAME(ms -> messages[prev_label_idx]), " ",
	   M_NAME(ms -> messages[ms -> tok]), " ",
	   arg_buf, NULL);
 
  de_newline_buf (expr_buf);
  for (idx = prev_label_idx; idx >= arg_end_idx; idx--) {
    ms -> messages[idx]->attrs |= TOK_IS_RT_EXPR;
    ++ms -> messages[idx] -> evaled;
    ++ms -> messages[idx] -> output;
  }
  
  escape_str_quotes (expr_buf, rt_expr_buf_esc);
  strcatx (rt_expr_buf, EVAL_EXPR_FN, " (\"", rt_expr_buf_esc, "\")",
	   NULL);

  fileout (rt_expr_buf, FALSE, prev_label_idx);
  return TRUE;
}

/*
 *  Returns the expression without parentheses.
 */
static void postfix_trans_a (MESSAGE_STACK messages, 
			     int start_idx,
			     int end_idx, char *expr_buf) {
  int i;
  char *l;
  char *m;
  l = expr_buf;
  for (i = start_idx; i >= end_idx; i--) {
    if ((M_TOK(messages[i]) != OPENPAREN) &&
	(M_TOK(messages[i]) != CLOSEPAREN)) {
      m = messages[i] -> name;
      while (*m) {
	*l++ = *m++;
	*l = 0;
      }
    }
  }
}

/*
 *  Removes the parentheses on the left-hand side of an operator.
 */
static void postfix_trans_b (MESSAGE_STACK messages, 
			     int start_idx,
			     int op_idx,
			     int end_idx, char *expr_buf) {
  int i;
  char *l;
  char *m;
  l = expr_buf;
  for (i = start_idx; i >= end_idx; i--) {
    if (i >= op_idx) {
      if ((M_TOK(messages[i]) != OPENPAREN) &&
	  (M_TOK(messages[i]) != CLOSEPAREN)) {
	m = messages[i] -> name;
	while (*m) {
	  *l++ = *m++;
	  *l = 0;
	}
      }
    } else {
      m = messages[i] -> name;
      while (*m) {
	*l++ = *m++;
	*l = 0;
      }
    }
  }
}

/*
 *  Handles cases like (t)++, ((t))++, (t)++ != NULL, etc.  
 *  rcvr_idx should point to the final token in the receiver
 *  expression.  
 *
 *  This *only* fits something that has closing parens between the 
 *  receiver and the operator.
 *
 *  If possible, recasts the expression into something that
 *  we can use __ctalk_method to call the postfix operator for -- 
 *  or, at least, eval_expr () can call the method on the receiver
 *  directly -- it simplifies the i handling in the run time greatly.
 *  
 *  Does not (yet) handle expressions like ((t)(<another_sub_expr>))++
 */
int postfix_method_expr_a (MESSAGE_STACK messages, int rcvr_idx) {

  int lookahead, lookahead_2;
  int first_paren_idx, last_paren_idx = rcvr_idx;
  int pre_first_paren_idx;
  int stack_end_idx, stack_start_idx;
  int i;
  int n_close_parens = 0;
  OBJECT_CONTEXT cx;
  char expr[MAXMSG], expr_buf[MAXMSG], expr_buf_tmp[MAXMSG];
  bool paren_delim = False;

  if (interpreter_pass == expr_check)
    return FALSE;

  stack_start_idx = stack_start (messages);
  stack_end_idx = get_stack_top (messages);

  for (lookahead = rcvr_idx-1; lookahead > stack_end_idx; lookahead--) {

    if (M_ISSPACE(messages[lookahead]))
      continue;

    if (M_TOK(messages[lookahead]) != CLOSEPAREN)
      break;

    ++n_close_parens;
    last_paren_idx = lookahead;
  } 

  /* Probably during expr_check pass. */
  if (lookahead == stack_end_idx)
    return FALSE;

  if (!n_close_parens)
    return FALSE;

  if ((M_TOK(messages[lookahead]) != INCREMENT) &&
      (M_TOK(messages[lookahead]) != DECREMENT))
    return FALSE;

  if ((first_paren_idx = match_paren_rev 
       (messages, last_paren_idx, stack_start_idx)) == ERROR)
    return FALSE;

  if ((pre_first_paren_idx = prevlangmsg (messages, first_paren_idx))
      != ERROR) {
    if (M_TOK(messages[pre_first_paren_idx]) == OPENPAREN) {
      paren_delim = True;
    }
  }

  cx = object_context (messages, rcvr_idx);

  switch (cx)
    {
    case receiver_context:
      if ((lookahead_2 = nextlangmsg (messages, lookahead)) != ERROR) {
	switch (M_TOK(messages[lookahead_2]))
	  {
	  case SEMICOLON:
	  case CLOSEPAREN:
	    postfix_trans_a (messages, first_paren_idx, lookahead,
			     expr);
	    strcatx (expr_buf, EVAL_EXPR_FN, " (\"", expr, "\")", NULL);
	    fileout (expr_buf, FALSE, rcvr_idx);
	    for (i = first_paren_idx; i >= lookahead; i--) {
	      ++messages[i] -> evaled;
	      ++messages[i] -> output;
	    }
	    return TRUE;
	    break;
	  }
      }
      break;
    case c_context:
      if (ctrlblk_pred) {
	if ((lookahead_2 = nextlangmsg (messages, lookahead)) != ERROR) {
	  switch (M_TOK(messages[lookahead_2]))
	    {
	    case SEMICOLON:
	    case CLOSEPAREN:
	      postfix_trans_a (messages, first_paren_idx, lookahead, expr);
	      /*
	       *  TODO - If the previous token is a C operator, we need
	       *  to try to match the output.  So if there's a method
	       *  that overloads the operator, try to find out what
	       *  its return class is, and use that.
	       *  
	       *  For now, though, we'll just assume that the
	       *  program realizes that the output of a postfix
	       *  expression is a pointer, and use the output without
	       *  worrying about a cast to a C type.
	       */
	      if (IS_C_BINARY_MATH_OP (M_TOK(messages[pre_first_paren_idx]))){
		strcatx (expr_buf, PTR_TRANS_FN_U, " (",
			 fmt_eval_expr_str (expr,
					    expr_buf_tmp), ")", NULL);
	      } else {
		strcatx (expr_buf, INT_TRANS_FN, " (",
			 fmt_eval_expr_str (expr,
					    expr_buf_tmp), ", 1)", NULL);
	      }
	      fileout (expr_buf, FALSE, rcvr_idx);
	      /*
	       *  TODO - This needs to be watched... this can occur
	       *  on the right-hand side of an operator, and there
	       *  aren't many cases where the left side is a C expression,
	       *  and the right side is a Ctalk expression.  
	       *
	       *  The following line prevents the entire predicate from
	       *  being re-evaluated (and re-output, which can cause a
	       *  memory error if it overlaps)  in loop_pred_end (),
	       *  loop_block_start (), etc.  In short, it skips nearly
	       *   all of the control statement predicate processing,
	       *  so we might need to duplicate some of it here.
	       *
	       *  So if there's also a Ctalk expression on the left
	       *  side of the operator, it needs to be evaluated
	       *  separately.
	       */
	      ctrlblks[ctrlblk_ptr+1] -> pred_expr_evaled = true;

	      for (i = first_paren_idx; i >= lookahead; i--) {
		++messages[i] -> evaled;
		++messages[i] -> output;
	      }
	      return TRUE;
	      break;
	    case PLUS:
	    case MINUS:
	    case ASTERISK:
	    case DIVIDE:
	    case MODULUS:
	    case ASL:
	    case  ASR:
	    case LT:
	    case LE:
	    case GT:
	    case GE:
	    case BOOLEAN_EQ:
	    case INEQUALITY:
	    case BIT_AND:
	    case BIT_OR:
	    case BIT_XOR:
	    case BIT_COMP:
	    case BOOLEAN_AND:
	    case BOOLEAN_OR:
	    case EQ:
	    case ASR_ASSIGN:
	    case ASL_ASSIGN:
	    case PLUS_ASSIGN:
	    case MINUS_ASSIGN:
	    case MULT_ASSIGN:
	    case DIV_ASSIGN:
	    case BIT_AND_ASSIGN:
	    case BIT_OR_ASSIGN:
	    case BIT_XOR_ASSIGN:
	      if (paren_delim) {
		lookahead_2 = match_paren (messages, pre_first_paren_idx,
					   stack_end_idx);
		postfix_trans_b (messages, first_paren_idx, lookahead, 
				 lookahead_2 + 1, expr);
		strcatx (expr_buf, INT_TRANS_FN, " (",
			 fmt_eval_expr_str (expr, expr_buf_tmp), 
			 ", 1)", NULL);
		fileout (expr_buf, FALSE, rcvr_idx);

		ctrlblks[ctrlblk_ptr+1] -> pred_expr_evaled = true;

		for (i = first_paren_idx; i >= lookahead_2 + 1; i--) {
		  ++messages[i] -> evaled;
		  ++messages[i] -> output;
		}
		return TRUE;
	      }
	      break;
	    }
	}
      }
      break;
    case c_argument_context:
      if (messages[rcvr_idx] -> attrs & TOK_IS_PRINTF_ARG) {
	switch (M_TOK(messages[pre_first_paren_idx]))
	  {
	  case OPENPAREN:
	  case ARGSEPARATOR:
	    lookahead_2 = nextlangmsg (messages, lookahead);
	    switch (M_TOK(messages[lookahead_2])) 
	      {
	      case CLOSEPAREN:
	      case ARGSEPARATOR:
		postfix_trans_a (messages, first_paren_idx, lookahead, expr);
		fmt_eval_expr_str (expr, expr_buf);
		fileout 
		  (fmt_printf_fmt_arg 
		   (messages, first_paren_idx,
		    stack_start_idx, expr_buf,
		    expr_buf_tmp),
		   0, rcvr_idx);
		for (i = first_paren_idx; i >= lookahead; i--) {
		  ++messages[i] -> evaled;
		  ++messages[i] -> output;
		}
		return TRUE;
		break;
	      }
	    break;
	  }
      }
      break;
    default:
      break;
    }

  return FALSE;
}
