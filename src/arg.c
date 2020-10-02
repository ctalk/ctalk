/* $Id: arg.c,v 1.4 2020/10/02 17:16:13 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2020 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

extern I_PASS interpreter_pass;    /* Declared in main.c.               */

extern int arg_has_leading_unary;  /* Declared in method.c.             */

extern bool ctrlblk_pred;           /* Declared in control.c.            */

extern char *ascii[8193];             /* from intascii.h */

extern bool argblk;                 /* Declared in argblk.c.             */

extern DEFAULTCLASSCACHE *ct_defclasses; /* Declared in cclasses.c.       */

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c.   */
extern int new_method_ptr;

extern HASHTAB declared_global_variables;
extern HASHTAB declared_typedefs;
extern HASHTAB declared_functions;

extern OBJECT *rcvr_class_obj;     /* Declared in primitives.c.        */

/*
 *  Find the end of an argument list, recursing if necessary
 *  for functions and methods as arguments.  
 *
 *  If the argument list begins with a parenthesis, simply
 *  find the matching closing parenthesis.
 *
 *  The parameter, n_args, should come from the method, or -1 
 *  if the number of args is unknown.  
 * 
 *  Empty argument lists get handled immediately.  They generate
 *  warning messages in method_args () if method -> n_args > 0.
 */

int paren_expr (MESSAGE_STACK messages, int open_paren_idx) {

  int _m_paren;

  if ((_m_paren = match_paren (messages, open_paren_idx, 
			       get_stack_top(messages)))
      == ERROR) {
    __ctalkExceptionInternal (messages[open_paren_idx], 
			      mismatched_paren_x, NULL,0);
    return ERROR;
  }

  return _m_paren;
}

int method_arg_limit (MESSAGE_STACK messages, int arg_start) {

  int n_parens, n_subscripts, i, i_2, stack_end_idx, lookahead;
  int arg_arglist_limit;
  int typecast_open_idx, typecast_close_idx;
  int stack_top;

  n_parens = n_subscripts = i = stack_end_idx = 0;

  switch (M_TOK(messages[arg_start]))
    {
    case OPENPAREN:
      if ((i = paren_expr (messages, arg_start)) == ERROR)
	return ERROR;
      if (interpreter_pass == expr_check) {
	stack_top = get_stack_top (messages);
	if (i == (stack_top + 1))
	  return i;
      }
      if ((lookahead = nextlangmsg (messages, i)) == ERROR)
 	return ERROR;
      /*
       *  If we don't find an argument terminator immediately
       *  after the expression's closing parenthesis, scan
       *  forward until we do.
       */
      switch (M_TOK(messages[lookahead]))
	{
	case CLOSEPAREN:
	case SEMICOLON:
	case ARGSEPARATOR:
	  return i;
	  /* break; */   /* Not reached. */
	case CONDITIONAL:
	  stack_top = get_stack_top (messages);
	  for (i_2 = lookahead; i_2 > stack_top; i_2--) {
	    if (M_TOK(messages[i_2]) == SEMICOLON) {
	      return prevlangmsg (messages, i_2);
	    } else if (M_TOK(messages[i_2]) == ARGSEPARATOR) {
	      return prevlangmsg (messages, i_2);
	    }
	  }
	  break;
	default:
	  /*
	   *  The function also needs to handle cases like 
	   *    a = (1 + 1) + (2 + 2); 
	   *    a = (1 + 1) + (2 + 2) + (3 + 3);
	   *  etc., here.
	   */
	  if (IS_C_BINARY_MATH_OP(M_TOK(messages[lookahead]))) {
	    if (!n_parens) {
 	      if ((lookahead = nextlangmsg (messages, lookahead)) == ERROR)
 		return ERROR;
	      return method_arg_limit (messages, lookahead);
	    }
	  } else {
	    for (; i > stack_end_idx; i--) {
 	      if ((lookahead = nextlangmsg (messages, i)) == ERROR)
 		return ERROR;
	      switch (M_TOK(messages[lookahead]))
		{
		case CLOSEPAREN:
		  /* check for the closing paren of a typecast 
		     before just returning */
		  if ((typecast_open_idx =
		       match_paren_rev (messages, lookahead,
					stack_start (messages))) != ERROR) {
		    if (!is_typecast_expr (messages, typecast_open_idx,
					   &typecast_close_idx)) {
		      return i;
		    }
		  }
		  break;
		case SEMICOLON:
		case ARGSEPARATOR:
		  return i;
		  /* break; */  /* Not reached. */
		}
	    }
	  }
	}
      break;
    default:
      /*
       *  Arguments that don't begin with a parenthesis.
       */
      stack_end_idx = get_stack_top (messages);
      for (i = arg_start; i > stack_end_idx; i--) {
	if (M_ISSPACE(messages[i]))
	  continue;
	if ((lookahead = nextlangmsg (messages, i)) == ERROR) {
	  if (EXPR_CHECK) {
	    return i;
	  } else {
	    return ERROR;
	  }
	}
	switch (M_TOK(messages[i]))
	  {
	  case LABEL:
	    if ((arg_arglist_limit = resolve_arg2 (messages, i)) != 0) {
	      /*
	       *  Reset i and lookahead to end of an internal method
	       *  arglist.
	       */
	      i = arg_arglist_limit;
 	      if ((lookahead = nextlangmsg (messages, i)) == ERROR) {
		if (EXPR_CHECK) {
		  return i;
		} else {
		  return ERROR;
		}
	      }
	    }
	    break;
	  case OPENPAREN:
	    ++n_parens;
	    break;
	  case ARRAYOPEN:
	    ++n_subscripts;
	    break;
	  case ARRAYCLOSE:
	    if (--n_subscripts < 0) return i;
	    break;
	  case CLOSEPAREN:
	    if (--n_parens < 0) return i;
	    break;
	  }
	switch (M_TOK(messages[lookahead])) 
	  {
	  case ARGSEPARATOR:
	    if (!n_parens) return i;
	    break;
	  case CLOSEPAREN:
	    if (!n_parens) return i;
	    break;
	  case ARRAYCLOSE:
	    if (!n_subscripts) return i;
	    break;
	  case SEMICOLON:
	    return i;
	    /* 	    break; */  /* Not reached. */
	  }
      }
      break;
    }
  return ERROR;
}

/*
 * Like above, but it doesn't do any object checking.  Only for 
 * method_arglist_n_args (), below.
 */
static int method_arg_limit_2 (MESSAGE_STACK messages, int arg_start) {

  int n_parens, n_subscripts, i, stack_end_idx, lookahead;
  int arg_arglist_limit;

  n_parens = n_subscripts = i = stack_end_idx = 0;

  switch (M_TOK(messages[arg_start]))
    {
    case OPENPAREN:
      if ((i = paren_expr (messages, arg_start)) == ERROR)
	return ERROR;
      if (interpreter_pass == expr_check) {
	int __stack_top;
	__stack_top = get_stack_top (messages);
	if (i == (__stack_top + 1))
	  return i;
      }
      if ((lookahead = nextlangmsg (messages, i)) == ERROR)
 	return ERROR;
      /*
       *  If we don't find an argument terminator immediately
       *  after the expression's closing parenthesis, scan
       *  forward until we do.
       */
      switch (M_TOK(messages[lookahead]))
	{
	case CLOSEPAREN:
	case SEMICOLON:
	case ARGSEPARATOR:
	  return i;
	  /* break; */   /* Not reached. */
	default:
	  /*
	   *  The function also needs to handle cases like 
	   *    a = (1 + 1) + (2 + 2); 
	   *    a = (1 + 1) + (2 + 2) + (3 + 3);
	   *  etc., here.
	   */
	  if (IS_C_BINARY_MATH_OP(M_TOK(messages[lookahead]))) {
	    if (!n_parens) {
 	      if ((lookahead = nextlangmsg (messages, lookahead)) == ERROR)
 		return ERROR;
	      return method_arg_limit (messages, lookahead);
	    }
	  } else {
	    for (; i > stack_end_idx; i--) {
 	      if ((lookahead = nextlangmsg (messages, i)) == ERROR)
 		return ERROR;
	      switch (M_TOK(messages[lookahead]))
		{
		case OPENPAREN:
		  ++n_parens;
		  break;
		case CLOSEPAREN:
		  if (n_parens == 0)
		    return i;
		  else
		    --n_parens;
		  break;
		case SEMICOLON:
		case ARGSEPARATOR:
		  return i;
		  /* break; */  /* Not reached. */
		}
	    }
	  }
	}
      break;
    default:
      /*
       *  Arguments that don't begin with a parenthesis.
       */
      stack_end_idx = get_stack_top (messages);
      for (i = arg_start; i > stack_end_idx; i--) {
	if (M_ISSPACE(messages[i]))
	  continue;
	if ((lookahead = nextlangmsg (messages, i)) == ERROR) {
	  if (EXPR_CHECK) {
	    return i;
	  } else {
	    return ERROR;
	  }
	}
	switch (M_TOK(messages[i]))
	  {
	  case LABEL:
	  case METHODMSGLABEL:
	    if ((arg_arglist_limit = resolve_arg2 (messages, i)) != 0) {
	      /*
	       *  Reset i and lookahead to end of an internal method
	       *  arglist.
	       */
	      i = arg_arglist_limit;
	      if ((lookahead = nextlangmsg (messages, i)) == ERROR) {
	     	if (EXPR_CHECK) {
	     	  return i;
	     	} else {
	     	  return ERROR;
	     	}
	      }
	    }
	    break;
	  case OPENPAREN:
	    ++n_parens;
	    break;
	  case ARRAYOPEN:
	    ++n_subscripts;
	    break;
	  case ARRAYCLOSE:
	    if (--n_subscripts < 0) return i;
	    break;
	  case CLOSEPAREN:
	    if (--n_parens < 0) return i;
	    break;
	  }
	switch (M_TOK(messages[lookahead])) 
	  {
	  case ARGSEPARATOR:
	    if (!n_parens) return i;
	    break;
	  case CLOSEPAREN:
	    if (!n_parens) return i;
	    break;
	  case ARRAYCLOSE:
	    if (!n_subscripts) return i;
	    break;
	  case SEMICOLON:
	    return i;
	    /* 	    break; */  /* Not reached. */
	  }
      }
      break;
    }
  return ERROR;
}

int method_arglist_limit (MESSAGE_STACK messages, int arglist_start,
			  int n_params, int varargs) {

  int arg_start, arg_end, lookahead, n_args;

  arg_start = arglist_start;
  n_args = 0;

  if (METHOD_ARG_TERM_MSG_TYPE(messages[arglist_start]))
    return arglist_start;

  /*
   *  Instance and class variables.
   */
  if ((arg_end = complex_arglist_limit (messages, arglist_start)) != ERROR) {
    return arg_end;
  }

  while (TRUE) {
    if ((arg_end = method_arg_limit (messages, arg_start)) == ERROR)
      return ERROR;
    if ((lookahead = nextlangmsg (messages, arg_end)) == ERROR) {
      if (EXPR_CHECK) {
	return arg_end;
      } else {
	__ctalkExceptionInternal (messages[arglist_start], 
				  parse_error_x, NULL,0);
	return ERROR;
      }
    }

    switch (M_TOK(messages[lookahead]))
      {
      case CLOSEPAREN:
      case SEMICOLON:
      case ARRAYCLOSE:
	return arg_end;
	/* 	break; */  /* Not reached.*/
      case ARGSEPARATOR:
	if ((n_args < (n_params - 1)) || varargs) {
	  ++n_args;
 	  if ((arg_start = nextlangmsg (messages, lookahead)) == ERROR) {
	    __ctalkExceptionInternal (messages[arglist_start], 
				      parse_error_x, NULL,0);
	    return ERROR;
	  }
	} else {
	  return arg_end;
	}
	break;
      }
  }

  return ERROR;
}

/*
 *  Same as above, but also calls compound_method_limit, for now
 *  called only by method_args ().
 */
int method_arglist_limit_2 (MESSAGE_STACK messages, 
			    int method_msg_ptr, int arglist_start,
			    int n_params, int varargs) {

  int arg_start, arg_end, lookahead, n_args;

  arg_start = arglist_start;
  n_args = 0;

  if (METHOD_ARG_TERM_MSG_TYPE(messages[arglist_start]))
    return arglist_start;

  /* For an assignment op, the number of arguments is
     always 1, so just scan forward to the semicolon. */
  if (IS_C_ASSIGNMENT_OP_NAME(M_NAME(messages[method_msg_ptr]))) {
    if (!ctrlblk_pred) { /* this doesn't work in "for" clauses. */
      if ((arg_end = scanforward (messages, method_msg_ptr,
				  get_stack_top (messages),
				  SEMICOLON)) != ERROR) {
	return prevlangmsg (messages, arg_end);
      }
    }
  }

  /*
   *  Instance and class variables.
   */
  if ((arg_end = complex_arglist_limit (messages, arglist_start)) != ERROR) {
    return arg_end;
  }

  if ((arg_end = compound_method_limit (messages, method_msg_ptr,
					arglist_start,
					n_params, varargs)) != ERROR) {
    return arg_end;
  }

  if ((arg_end = cvar_rcvr_arg_expr_limit (messages, arglist_start))
      != ERROR) {
    return arg_end;
  }

  while (TRUE) {
    if ((arg_end = method_arg_limit (messages, arg_start)) == ERROR)
      return ERROR;
    if ((lookahead = nextlangmsg (messages, arg_end)) == ERROR) {
      if (EXPR_CHECK) {
	return arg_end;
      } else {
	__ctalkExceptionInternal (messages[arglist_start], 
				  parse_error_x, NULL,0);
	return ERROR;
      }
    }

    switch (M_TOK(messages[lookahead]))
      {
      case CLOSEPAREN:
      case SEMICOLON:
      case ARRAYCLOSE:
	return arg_end;
	/* 	break; */  /* Not reached.*/
      case ARGSEPARATOR:
	if ((n_args < (n_params - 1)) || varargs) {
	  ++n_args;
 	  if ((arg_start = nextlangmsg (messages, lookahead)) == ERROR) {
	    __ctalkExceptionInternal (messages[arglist_start], 
				      parse_error_x, NULL,0);
	    return ERROR;
	  }
	} else {
	  return arg_end;
	}
	break;
      }
  }

  return ERROR;
}

/*
 * Check for a comma preceding the receiver.
 * Doesn't look count parens within the arglist.
 */
static int is_comma_before_receiver (MESSAGE_STACK messages, 
				   int method_msg_idx) {
  int i, i_prev;

  if (messages[method_msg_idx] -> receiver_msg) {
    for (i = method_msg_idx; ; i++) 
      if (messages[i] == messages[method_msg_idx] -> receiver_msg)
	break;
    if ((i_prev = prevlangmsg (messages, i)) != ERROR) {
      if (M_TOK(messages[i_prev]) == ARGSEPARATOR) {
	return TRUE;
      } else {
	return FALSE;
      }
    } else {
      return FALSE;
    }
  } else {
    return FALSE;
  }
}

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

/*
 * Returns 
 *   n The number of arguments in the argument list, or:
 *
 *  -1 on parser error (like a stack error).
 *  -2 if the arglist is for defineInstanceMethod () and defineClassMethod (),
 *    which have the form of a function declaration.
 *  -4 if the arglist is in a control statement predicate.
 *
 */
int method_arglist_n_args (MESSAGE_STACK messages, int method_msg_idx) {
			  
  int arglist_start, arg_start, arg_end, lookahead, n_args,
    close_paren_idx; 
  int comma_delimits_arglist;
  METHOD *method = NULL;

  if (ctrlblk_pred)
    return -4;

  /* By now we've already checked the actual method, so this
     should be enough to determine an assignment statement. 
     (Works for '==', too.)  */
  if (messages[method_msg_idx] -> name[0] == '=' ||
      messages[method_msg_idx] -> name[1] == '=' ||
      messages[method_msg_idx] -> name[2] == '=')
    return 1;

  /* defineInstanceMethod () and defineClassMethod () */
  if (is_ctalk_keyword (M_NAME(messages[method_msg_idx]))) {
    return -2;
  }

  /*
   * Do a pre-check here if we've already found a method.
   */
  if (M_TOK(messages[method_msg_idx]) == METHODMSGLABEL) {
    if (IS_OBJECT(messages[method_msg_idx]->receiver_obj)) {
      if (((method = get_instance_method 
	   (messages[method_msg_idx], 
	    messages[method_msg_idx] -> receiver_obj,
	    M_NAME(messages[method_msg_idx]), 
	    ANY_ARGS, FALSE)) != NULL) ||
	  ((method = get_class_method 
	    (messages[method_msg_idx], 
	     messages[method_msg_idx] -> receiver_obj,
	     M_NAME(messages[method_msg_idx]),
	     ANY_ARGS, FALSE)) != NULL)) {
	/* nothing at the moment. */
      }
    }
  }

  /*
   *  If there's a comma before the receiver, then 
   *  the next comma (at this level of parentheses)
   *  is end of the arglist.
   */
  comma_delimits_arglist = is_comma_before_receiver
    (messages, method_msg_idx);

  if ((arglist_start = nextlangmsg (messages, method_msg_idx)) == ERROR)
    return ERROR;

  /* Adjust for parentheses. */
  if (M_TOK(messages[arglist_start]) == OPENPAREN) {
    if ((close_paren_idx = match_paren (messages, arglist_start,
					get_stack_top (messages)))
	== ERROR)
      return ERROR;

    if ((lookahead = nextlangmsg (messages, close_paren_idx)) == ERROR)
      return ERROR;

    /* Make sure that the close paren is the end of the complete argument
       list before adjusting. */
    if ((M_TOK(messages[lookahead]) == CLOSEPAREN) ||
	(M_TOK(messages[lookahead]) == SEMICOLON) ||
	(M_TOK(messages[lookahead]) == ARRAYCLOSE) ||
	/*
	 *  If we have an expression like:
	 *
	 *  rcvr method (args...) == 0
	 *
	 *  the relational operator is not part of the
	 *  argument list, because the complete expression
	 *  returns a Boolean object, instead of whatever
	 *  the method returns.
	 *  This should also work for arglists without parens,
	 *  but we might need to check the return class
	 *  sometime....
	 *
	 *  See the comment in method_arg_limit_2 also.
	 */
	IS_C_RELATIONAL_OP(M_TOK(messages[lookahead]))) {

      if ((arglist_start = nextlangmsg (messages, arglist_start)) ==
	  ERROR)
	return ERROR;
    }

  }

  arg_start = arglist_start;

  if (METHOD_ARG_TERM_MSG_TYPE(messages[arglist_start]))
    return 0;

  /* if the next token is a method for the return class of
     the first method, that's a compound method expression,
     not an argument list. */
  if (IS_METHOD(method)) {
    if (method -> n_params == 0) {
      if (is_method_name (M_NAME(messages[arglist_start])) ||
	    is_instance_var (M_NAME(messages[arglist_start])))  {
	  /* no examples of class vars yet. */
	return 0;
      }
    }
  }

  n_args = 1;

  while (TRUE) {
    if ((arg_end = method_arg_limit_2 (messages, arg_start)) == ERROR)
      return ERROR;
    if ((lookahead = nextlangmsg (messages, arg_end)) == ERROR) {
      if (EXPR_CHECK) {
	return ERROR;
      } else {
	__ctalkExceptionInternal (messages[arg_start], 
				  parse_error_x, NULL,0);
	return ERROR;
      }
    }

    switch (M_TOK(messages[lookahead]))
      {
      case CLOSEPAREN:
	/* We rely on method_arg_limit_2 () to match the parens
	   that belong to the last argument, so this paren 
	   is the one that encloses the entire arglist. */
      case SEMICOLON:
      case ARRAYCLOSE:
	return n_args;
	/* 	break; */  /* Not reached.*/
      case ARGSEPARATOR:
	/* Using a greedy match if there are no parens. */
	if (comma_delimits_arglist) {
	  if (method) {
	    if (n_args == method -> n_params) {
	      return n_args;
	    }
	  }
	}
	++n_args;
	if ((arg_start = nextlangmsg (messages, lookahead)) == ERROR)
	  return ERROR;
	break;
      }
  }

  return ERROR;
}

static void add_comma (char *expr_buf, int n_th_arg, int n_args) {
  if (n_th_arg < (n_args - 1))
    strcatx2 (expr_buf, ", ", NULL);
}
static void add_arg (char *expr_buf, char *arg, int n_th_arg, int n_args) {
  if (n_th_arg < (n_args - 1)) {
    if (n_th_arg == 0) {
      strcatx (expr_buf, arg, ", ", NULL);
    } else {
      strcatx2 (expr_buf, arg, ", ", NULL);
    }
  } else {
    strcat (expr_buf, arg);
  }
}

static void add_rt_return_expr (char *expr_buf, PARAM *p, char *argbuf) {
  char expr_buf_out[MAXMSG];
  if (p != NULL) {
    strcatx2 (expr_buf, fmt_rt_return (argbuf, p -> class, TRUE, expr_buf_out), NULL);
  } else {
    strcatx2 (expr_buf, argbuf, NULL);
  }
}

static char *arg_var_basename (MESSAGE_STACK messages,
			      ARGSTR args[], int n_th_arg) {
  int i;
  for (i = args[n_th_arg].start_idx; i >= args[n_th_arg].end_idx;
       --i) {
    if (M_TOK(messages[i]) == LABEL) {
      if (!is_c_data_type(M_NAME(messages[i])) &&
	  !is_c_derived_type (M_NAME(messages[i]))) { /* skip a typecast */
	return M_NAME(messages[i]);
      }
    }
  }
  return NULL;
}

static char *fn_arg_expr_param_expr_class (MESSAGE_STACK messages,
					   int expr_start,
					   int expr_end,
					   char *param_rcvr_classname) {
  OBJECT *rcvr_class_obj;
  rcvr_class_obj = get_class_object (param_rcvr_classname);
  get_new_method_param_instance_variable_series (rcvr_class_obj,
						 messages,
						 expr_start,
						 get_stack_top (messages));
  if (IS_OBJECT(M_OBJ(messages[expr_end]))) {
    if (IS_OBJECT(M_OBJ(messages[expr_end]) -> instancevars)) {
      return M_OBJ(messages[expr_end]) -> instancevars -> __o_classname;
    } else {
      return M_OBJ(messages[expr_end]) -> __o_classname;
    }
  } else {
    /* This is what the instance variable series that is parsed
       above should default to, and it should have also printed a
       warning if it can't resolve the instance variable series. */
    return OBJECT_CLASSNAME; 
  }
}

int fn_arg_expression_call = 0;

#define FN_ARG_EVAL_EXPR_MAX (MAXMSG + EVAL_EXPR_FN_LENGTH + 5)

/* This should always be called with messages == m_messages in
   order to stay consistent with eval_arg's environment --
   see eval_params (c_rval.c) for an example of switching to
   the m_messages stack.  */
OBJECT *fn_arg_expression (OBJECT *rcvr_class, METHOD *method, 
			 MESSAGE_STACK messages, int fn_idx) {
  int n_args, arg_start, arg_end, n_th_arg;
  int n_params = 0;  /* Avoid a warning. */
  int stack_start_idx, expr_end;
  ARGSTR argstrs[MAXARGS];
  char expr_buf[MAXMSG], expr_buf_tmp[MAXMSG], expr_buf_tmp_2[MAXMSG],
    expr_buf_tmp_3[MAXMSG];
  char constant_arg[MAXMSG], *basename = NULL;
  char __argbuf[FN_ARG_EVAL_EXPR_MAX];
  OBJECT *arg_object;
  CVAR *arg_cvar = NULL;
  CFUNC *fn_cfunc = NULL;
  CVAR *fn_param_cvar = NULL;
  char arg_expr[MAXMSG];
  bool have_function = false;
  bool fn_varargs = false;

  stack_start_idx = get_stack_top (messages);

  if ((arg_start = scanforward (messages, fn_idx, 
				stack_start_idx,
				OPENPAREN)) == ERROR) {
    warning (messages[fn_idx], "Function syntax error.");
    return NULL;
  }

  if ((arg_end = match_paren (messages, arg_start, stack_start_idx))
      == ERROR) {
    warning (messages[fn_idx], "Function syntax error.");
    return NULL;
  }

  split_args_argstr (messages, arg_start, arg_end, argstrs, &n_args);

  if (n_args == 0) {
    /* empty arglist */
    arg_object = create_object (CFUNCTION_CLASSNAME, "");
    return arg_object;
  }

  if ((fn_cfunc = get_function (M_NAME(messages[fn_idx]))) != NULL) {
    for (n_params = 0, fn_param_cvar = fn_cfunc -> params; 
	 fn_param_cvar; fn_param_cvar = fn_param_cvar -> next, ++n_params) {
      if (str_eq (fn_param_cvar -> name, "...")) {
      /* if (fn_param_cvar -> attrs & CVAR_ATTR_ELLIPSIS) { */
	fn_varargs = true;
      }
    }
  }

  /* We don't issue a warning here because, for now, at least,
     if a macroized prototype (like MATHCALL in math.h) does anything
     tricky with the prototype, we don't add a parameter list to
     the CFUNC. */
  if (!fn_varargs) {
    if (n_params == n_args) {
      have_function = true;
    }
  }

  *expr_buf = 0;
  if (have_function)
    fn_param_cvar = fn_cfunc -> params;

  for (n_th_arg = 0; n_th_arg < n_args; n_th_arg++) {

    /* check for a class cast, set the object's class and skip
       the leading cast tokens - set the attributes ourselves,
       in case resolve started with a lot of tokens before these
       tokens */
    if (M_TOK(messages[argstrs[n_th_arg].start_idx]) == OPENPAREN) {
      int l, l_2, l_3, l_4;
      OBJECT *ccobj;
      l = nextlangmsg (messages, argstrs[n_th_arg].start_idx);
      if (M_TOK(messages[l]) == LABEL) {
	if ((ccobj = get_class_object (M_NAME(messages[l]))) != NULL) {
	  l_2 = nextlangmsg (messages, l);
	  if (M_TOK(messages[l_2]) == MULT) {
	    l_3 = nextlangmsg (messages, l_2);
	    if (M_TOK(messages[l_3]) == CLOSEPAREN) {
	      l_4 = nextlangmsg (messages, l_3);
	      if (M_TOK(messages[l_4]) == LABEL) {
		if (is_object_or_param (M_NAME(messages[l_4]), NULL)) {
		  messages[l_4] -> obj = ccobj;
		  messages[l_4] -> attrs |= TOK_IS_CLASS_TYPECAST;
		  argstrs[n_th_arg].start_idx = l_4;
		}
	      }
	    }
	  }
	}
      }
    }

    if ((basename = arg_var_basename (messages, argstrs, n_th_arg))
	== NULL) {
      basename = argstrs[n_th_arg].arg;
    }

    if (((arg_cvar = get_local_var (basename)) != NULL) ||
	((arg_cvar = get_global_var (basename)) != NULL)) {
      add_arg (expr_buf, argstrs[n_th_arg].arg, n_th_arg, n_args);
      if (fn_param_cvar)
	fn_param_cvar = fn_param_cvar -> next;
      continue;
    } else if (get_function (basename)) {
      add_arg (expr_buf, argstrs[n_th_arg].arg, n_th_arg, n_args);
      if (fn_param_cvar)
	fn_param_cvar = fn_param_cvar -> next;
      continue;
    } else if (get_local_object (basename, NULL) ||
	       get_global_object (basename, NULL)) {
      if (fn_varargs) {
	fmt_eval_expr_str (basename, expr_buf_tmp);
	add_arg (expr_buf,
		 fmt_printf_fmt_arg (messages, argstrs[n_th_arg].start_idx,
				     stack_start (messages),
				     expr_buf_tmp, expr_buf_tmp_2),
		 n_th_arg, n_args);
		 
	continue;
      } else {
	fmt_eval_expr_str (basename, expr_buf_tmp);
	fmt_rt_return (expr_buf_tmp, 
		       basic_class_from_cvar
		       (messages[argstrs[n_th_arg].start_idx],
			fn_param_cvar, 0),
		       TRUE, expr_buf_tmp_2);
	add_arg (expr_buf, expr_buf_tmp_2, n_th_arg, n_args);
	if (fn_param_cvar)
	  fn_param_cvar = fn_param_cvar -> next;
	continue;
      }
    } else if (str_eq (basename, "self")) {
      strcatx (expr_buf_tmp_2, fmt_rt_return
	       (fmt_rt_expr (messages, argstrs[n_th_arg].start_idx,
			     &expr_end, expr_buf_tmp),
		basic_class_from_cvar
		(messages[argstrs[n_th_arg].start_idx], fn_param_cvar, 0),
		true, expr_buf_tmp_3), NULL);
      add_arg (expr_buf, expr_buf_tmp_2, n_th_arg, n_args);
      if (fn_param_cvar)
	fn_param_cvar = fn_param_cvar -> next;
      continue;
	       
    } else if ((M_TOK(messages[argstrs[n_th_arg].start_idx]) ==
		INTEGER) ||
	       (M_TOK(messages[argstrs[n_th_arg].start_idx]) ==
		FLOAT) ||
	       (M_TOK(messages[argstrs[n_th_arg].start_idx]) ==
		LONG) ||
	       (M_TOK(messages[argstrs[n_th_arg].start_idx]) ==
		LONGLONG) ||
	       (M_TOK(messages[argstrs[n_th_arg].start_idx]) ==
		LITERAL) ||
	       (M_TOK(messages[argstrs[n_th_arg].start_idx]) ==
		LITERAL_CHAR)) {
      /* constants */
      if (argstrs[n_th_arg].start_idx == argstrs[n_th_arg].end_idx) {
	add_arg (expr_buf, M_NAME(messages[argstrs[n_th_arg].start_idx]),
		 n_th_arg, n_args);
      } else {
	int i_2;
	toks2str (messages,
		  argstrs[n_th_arg].start_idx,
		  argstrs[n_th_arg].end_idx,
		  constant_arg);

	for (i_2 = argstrs[n_th_arg].start_idx;
	     i_2 <= argstrs[n_th_arg].end_idx;
	     --i_2) {
	  if (M_TOK(messages[i_2]) == LABEL) {
	    warning (messages[argstrs[n_th_arg].start_idx],
		     "Identifiers are not (yet) supported in this "
		     "context\n\n\t%s\n\n.",
		     constant_arg);
	  }
	}
	add_arg (expr_buf, constant_arg, n_th_arg, n_args);
      }
      if (fn_param_cvar)
	fn_param_cvar = fn_param_cvar -> next;
      continue;
    }
    /* Look for method parameters. */
    if (interpreter_pass == method_pass) {
      METHOD *__n_method;
      int __n_th_param;
      PARAM *p = NULL;
      OBJECT *local_obj;
      __n_method = new_methods[new_method_ptr+1] -> method;
      for (__n_th_param = 0; __n_th_param < __n_method -> n_params; 
	   __n_th_param++) {

	if (argstrs[n_th_arg].start_idx == argstrs[n_th_arg].end_idx) {
	  if (!strcmp (__n_method -> params[__n_th_param] -> name, 
		       argstrs[n_th_arg].arg)) {

	    p = __n_method -> params[__n_th_param];

	    if (have_function) {
	      add_arg (expr_buf,
		       fmt_rt_return 
			(format_method_arg_accessor 
			 (__n_method -> n_params - __n_th_param - 1,
			  __n_method -> params[__n_th_param] -> name,
			  __n_method -> varargs, expr_buf_tmp_2),
			 basic_class_from_cvar (messages[n_th_arg * 2],
						fn_param_cvar, 0), 
			 TRUE, expr_buf_tmp),
		       n_th_arg, n_args);
	    } else {
	      add_arg (expr_buf,
		       fmt_rt_return (__argbuf, p -> class, TRUE,
				      expr_buf_tmp),
		       n_th_arg, n_args);
	    }
	    goto next_arg;
	  }
	} else if (str_eq (__n_method -> params[__n_th_param] -> name,
			   basename)) {
	  /* if (arg_idx_s[n_th_arg * 2] == ... */
	  /* e.g., multiple tokens */
	  char *__arg_t = collect_tokens (messages, argstrs[n_th_arg].start_idx,
					  argstrs[n_th_arg].end_idx);
	  char *__class_t = fn_arg_expr_param_expr_class
	    (messages, argstrs[n_th_arg].start_idx, argstrs[n_th_arg].end_idx,
	     __n_method -> params[__n_th_param] -> class);
	  char param_eval_expr[MAXMSG];
	  fmt_eval_expr_str (__arg_t, param_eval_expr);
	  __xfree (MEMADDR (__arg_t));
	  add_arg (expr_buf,
		   fmt_rt_return (param_eval_expr, __class_t, TRUE,
				  expr_buf_tmp),
		   n_th_arg, n_args);
	  goto next_arg;
	}
      }
      /*
       *  Check for a single-token object, then an expression.
       */
      if (argstrs[n_th_arg].start_idx == argstrs[n_th_arg].end_idx) {
	if ((local_obj = get_local_object (argstrs[n_th_arg].arg, 
					   NULL)) != NULL) {
	  snprintf (__argbuf, FN_ARG_EVAL_EXPR_MAX,
		    "%s (\"%s\")", EVAL_EXPR_FN,
		    local_obj -> __o_name);
	  add_rt_return_expr (expr_buf, p, __argbuf);
	} else if (is_enum_member (argstrs[n_th_arg].arg)) {
	  add_rt_return_expr (expr_buf, p, argstrs[n_th_arg].arg);
	  add_comma (expr_buf, n_th_arg, n_args);
	} else {
	  /* unknown token */
	  warning (messages[argstrs[n_th_arg].start_idx],
		   "Undefined function argument, \"%s.\"",
		   argstrs[n_th_arg].arg);
	  add_arg (expr_buf, argstrs[n_th_arg].arg, n_th_arg, n_args);
	  if (fn_param_cvar)
	    fn_param_cvar = fn_param_cvar -> next;
	  continue;
	}

      }  else { /* if (arg_idx_s[n_th_arg * 2] == ... */
	if ((local_obj = get_local_object 
	     (M_NAME(messages[argstrs[n_th_arg].start_idx]), 
	      NULL)) != NULL) {
	  toks2str (messages,
		    argstrs[n_th_arg].start_idx,
		    argstrs[n_th_arg].end_idx,
		    arg_expr);
	  snprintf (__argbuf, FN_ARG_EVAL_EXPR_MAX,
		    "%s (\"%s\")", EVAL_EXPR_FN,
		    arg_expr);
	  add_rt_return_expr (expr_buf, p, __argbuf);
	  add_comma (expr_buf, n_th_arg, n_args);
	} else {
	  if (is_typecast_expr (messages, argstrs[n_th_arg].start_idx,
				&arg_end)) {
	    toks2str (messages,
		      argstrs[n_th_arg].start_idx,
		      argstrs[n_th_arg].end_idx,
		      arg_expr);
	    add_rt_return_expr (expr_buf, p, arg_expr);
	    add_comma (expr_buf, n_th_arg, n_args);
	    /* __xfree (MEMADDR(arg_expr)); */
	    /* Shouldn't need to call eval_arg () circularly,
	       below. */
	    goto fn_arg_expr_done;
	  }
	}
      }  /* if (arg_idx_s[n_th_arg * 2] == ... */
      continue;
    }

    if (argstrs[n_th_arg].arg[0] != '\0') {
      if (interpreter_pass != expr_check) {
	++fn_arg_expression_call;
	arg_object = eval_arg (method, rcvr_class, &argstrs[n_th_arg], fn_idx);
	--fn_arg_expression_call;
      } else {
	arg_object = NULL;
      }
    } else {
      arg_object = NULL;
    }

    if (arg_object) {
      OBJECT *__tmp_arg_object;
      if (DEFAULTCLASS_CMP(arg_object, ct_defclasses->p_expr_class,
			   EXPR_CLASSNAME)) {
	/*
	 *  Within a function, an arg can be used unchanged, 
	 *  unless it uses, "self" as the receiver, in which
	 *  case the arg needs a __ctalkEvalExpr call.
	 * 
	 *  TO DO - Signal to the eval_arg call above that it 
	 *  is not necessary to register the variable as a 
	 *  method arg.
	 */
	if (strstr (arg_object -> __o_name, "self")) {
	  add_arg (expr_buf,
		   fmt_eval_expr_str (arg_object -> __o_name,
				      expr_buf_tmp), n_th_arg, n_args);
	  delete_object (arg_object);
	  continue;
	} else {
	  add_arg (expr_buf, argstrs[n_th_arg].arg, n_th_arg, n_args);
	  delete_object (arg_object);
	  continue;
	}
      } else {
	/* 
	 *  A CFunction object. 
	 */
	if (DEFAULTCLASS_CMP(arg_object, ct_defclasses->p_cfunction_class,
			     CFUNCTION_CLASSNAME)) {
	  add_arg (expr_buf, arg_object -> __o_name, n_th_arg, n_args);
	  continue;
	} else {
	  /*
	   *  A genuine, declared object.
	   */
	  if (((__tmp_arg_object = get_object (arg_object -> __o_name,
					       arg_object -> __o_classname))
	       != NULL) && 
	      (__tmp_arg_object == arg_object)) {
	    add_arg (expr_buf,
		     obj_arg_to_c_expr 
		     (messages, fn_idx, arg_object, n_args),
		     n_th_arg, n_args);
	    continue;
	  } else if (strstr (arg_object -> __o_name, "self")) {

	    /*
	     *  Handle "self" as a receiver in an argument.
	     */
	    add_arg (expr_buf,
		     obj_arg_to_c_expr (messages, fn_idx, arg_object, 
					n_args),
		     n_th_arg, n_args);
	    delete_object (arg_object);
	    continue;
	  } else {
	    if (get_local_var (arg_object -> __o_name) || 
		global_var_is_declared (arg_object -> __o_name)) {
	      /*
	       *  Eval_arg gives C variables an "Expr" class, so
	       *  they are handled above.
	       */
	      delete_object (arg_object);
	    }  else {
	      /*
	       *  Anything else - make sure that the argument is
	       *  added to expr_buf.
	       */
	      add_arg (expr_buf, arg_object -> __o_name,
		       n_th_arg, n_args);
	      delete_object (arg_object);
	      arg_object = NULL;
	      continue;
	    }
	  }
	}
      }
    }
  next_arg:
    if (have_function)
      fn_param_cvar = fn_param_cvar -> next;
  }

 fn_arg_expr_done:
  for (n_th_arg = 0; n_th_arg < n_args; n_th_arg++)
    __xfree (MEMADDR(argstrs[n_th_arg].arg));

#if 0
  /* keep this here in case there's a case the function
     doesn't handle */
  if (*expr_buf == '\0') {
    /* 
     *  Look for a constant token.
     */
    int __arg_tok_start, __arg_tok_end;
    if ((((__arg_tok_start = nextlangmsg (messages, arg_start)) != ERROR) &&
	 ((__arg_tok_end = prevlangmsg (messages, arg_end)) != ERROR)) &&
	(__arg_tok_start == __arg_tok_end)) {
      if (!lextype_is_PTR_T (basename)) {
	strcpy (expr_buf, basename);
      }
    }
  }
#endif
  
  arg_object = create_object (CFUNCTION_CLASSNAME, expr_buf);
  return arg_object;
}

static bool empty_parens (MESSAGE_STACK messages, int start_paren,
			  int end_paren) {
  int i;
  for (i = start_paren - 1; i >= end_paren; i--) {
    if (M_ISSPACE(messages[i]))
      continue;
    if (M_TOK(messages[i]) == CLOSEPAREN)
      return true;
    else
      return false;
  }
  return false;
}

int prev_msg_arg (MESSAGE **messages, int this_msg, int stack_start_idx) {

  int i;

  int top_ptr, top_ptr_frame;

  top_ptr_frame = message_frame_top_n (parser_frame_ptr ());

  top_ptr = (top_ptr_frame < stack_start_idx) ? top_ptr_frame :
    stack_start_idx;

  for (i = this_msg + 1; i <= top_ptr; i++) {
    if ((messages[i] -> tokentype != NEWLINE) &&
	(messages[i] -> tokentype != WHITESPACE))
      return i;
  }
  return ERROR;
}

static inline int next_msg_arg (MESSAGE **messages, int this_msg, 
				int stack_end_idx) {

  int i;
  for (i = this_msg - 1; i > stack_end_idx; i--) {
    if ((M_TOK(messages[i]) != NEWLINE) && 
	(M_TOK(messages[i]) != WHITESPACE))
      return i;
  }
  return ERROR;
}

int split_args_argstr (MESSAGE_STACK messages, int start_paren, int end_paren,
		       ARGSTR args[], int *n_args) {

  int i, arg_start_idx, arg_end_idx, n_parens;

  if (empty_parens (messages, start_paren, end_paren)) {
    *n_args = 0;
    return SUCCESS;
  }

  for (i = start_paren, *n_args = 0, n_parens = 0, arg_start_idx = -1; 
       i >= end_paren; i--) {
    
    if (M_ISSPACE (messages[i])) continue;

    switch (M_TOK(messages[i]))
      {
      case OPENPAREN:
	++n_parens;
	if (n_parens == 1) {
 	  if ((arg_start_idx = next_msg_arg (messages, i, end_paren)) 
	      == ERROR) {
	    warning (messages[i], "Parser error.");
	    return ERROR;
	  } else {
	    i = arg_start_idx + 1;
	  }
	}
	break;
      case CLOSEPAREN:
	--n_parens;
	if (n_parens == 0) {
	  if ((arg_end_idx = prev_msg_arg (messages, i, start_paren)) 
	      == ERROR) {
	    warning (messages[i], "Parser error.");
	    return ERROR;
	  } else {
	    if (arg_end_idx < start_paren) {
	      /* Check for empty argument list. */
	      args[*n_args].arg = 
		collect_tokens (messages, arg_start_idx, arg_end_idx);
	      args[*n_args].start_idx = arg_start_idx;
	      args[*n_args].end_idx = arg_end_idx;
	      args[*n_args].m_s = messages;
	      ++(*n_args);
	    }
	  }
	}
	break;
      case ARGSEPARATOR:
	if (n_parens == 1) {
	  if ((arg_end_idx = prev_msg_arg (messages, i, start_paren)) 
	      == ERROR) {
	    warning (messages[i], "Parser error.");
	    return ERROR;
	  } else {
	    args[*n_args].arg = 
	      collect_tokens (messages, arg_start_idx, arg_end_idx);
	    args[*n_args].start_idx = arg_start_idx;
	    args[*n_args].end_idx = arg_end_idx;
	    args[*n_args].m_s = messages;
	    ++(*n_args);
	  }
 	  if ((arg_start_idx = next_msg_arg (messages, i, end_paren)) 
	      == ERROR) {
	    warning (messages[i], "Parser error.");
	    return ERROR;
	  } else {
	    i = arg_start_idx + 1;
	  }
	}
	break;
      }
  }
  return SUCCESS;
}

/*
 *  Returns array with arg_start_index, arg_end_index,... 
 *
 */
int split_args_idx (MESSAGE_STACK messages, int start_paren, int end_paren,
		    int *arg_indexes, int *n_arg_indexes) {

  int i, arg_start_idx, arg_end_idx, n_parens;

  if (empty_parens (messages, start_paren, end_paren)) {
    *n_arg_indexes = 0;
    return SUCCESS;
  }

  for (i = start_paren, *n_arg_indexes = 0, n_parens = 0, arg_start_idx = -1; 
       i >= end_paren; i--) {
    
    if (M_ISSPACE (messages[i])) continue;

    switch (M_TOK(messages[i]))
      {
      case OPENPAREN:
	++n_parens;
	if (n_parens == 1) {

 	  if ((arg_start_idx = next_msg_arg (messages, i, end_paren)) 
	      == ERROR) {
	    warning (messages[i], "Parser error.");
	    return ERROR;
	  } else {
	    arg_indexes[(*n_arg_indexes)++] = arg_start_idx;
 	    i = arg_start_idx + 1;
	  }
	}
	break;
      case CLOSEPAREN:
	--n_parens;
	if (n_parens == 0) {
	  if ((arg_end_idx = prev_msg_arg (messages, i, start_paren)) 
	      == ERROR) {
	    warning (messages[i], "Parser error.");
	    return ERROR;
	  } else {
	    arg_indexes[(*n_arg_indexes)++] = arg_end_idx;
	  }
	}
	break;
      case ARGSEPARATOR:
	if (n_parens == 1) {
	  if ((arg_end_idx = prev_msg_arg (messages, i, start_paren)) 
	      == ERROR) {
	    warning (messages[i], "Parser error.");
	    return ERROR;
	  } else {
	    arg_indexes[(*n_arg_indexes)++] = arg_end_idx;
	  }
 	  if ((arg_start_idx = next_msg_arg (messages, i, end_paren)) 
	      == ERROR) {
	    warning (messages[i], "Parser error.");
	    return ERROR;
	  } else {
	    arg_indexes[(*n_arg_indexes)++] = arg_start_idx;
 	    i = arg_start_idx + 1;
	  }
	}
	break;
      }
  }
  return SUCCESS;
}

char *obj_arg_to_c_expr (MESSAGE_STACK messages, int fn_idx, 
			 OBJECT *arg_object, int n_args) {
  CFUNC *fn;
  CVAR *c, *param_c = NULL;  /* Avoid a warning. */
  int n_th_param;
  char t_buf[MAXMSG];
  char expr_buf_out[MAXMSG];

  SNPRINTF (t_buf, MAXMSG, "%s(\"%s\")", 
	    EVAL_EXPR_FN, arg_object -> __o_name);
  fn = get_function (M_NAME(messages[fn_idx]));
  c = get_global_var (M_NAME(messages[fn_idx]));
  if (fn) {
    for (n_th_param = 0, param_c = fn -> params; 
	 (n_th_param < (n_args - 1)); n_th_param++) {
      param_c = param_c -> next;
    }
  } else {
    if (c) {
      for (n_th_param = 0, param_c = c -> params; 
	   (n_th_param < (n_args - 1)) && param_c; n_th_param++) {
	param_c = param_c -> next;
      }
    } else {
      warning (messages[fn_idx], "Could not find function definition %s.",
		     M_NAME(messages[fn_idx]));
    }
  }

  if (param_c) {
    return fmt_rt_return (t_buf, 
		   basic_class_from_cvar (messages[fn_idx], 
					  param_c, 0),
			  TRUE, expr_buf_out);
  } else {
    if (is_translatable_basic_class (arg_object -> __o_classname)) {
      /*
       *  TO DO - Provide an optional warning if we can't find the
       *  type of the argument.
       */
      return fmt_rt_return 
	(t_buf, arg_object->__o_classname, TRUE, expr_buf_out);
    } else {
      /*
       *  TO DO! - If evaluating a control block predicate, see if we can
       *  avoid creating a function expression altogether, by avoiding
       *  the call to fn_arg_obj () from eval_arg () in method.c.  It should
       *  be okay to avoid evaluating the args completely if we are simply
       *  going to write a template expression.
       */
      if (!ctrlblk_pred && !EXPR_CHECK) {
	warning (messages[fn_idx], 
		 "Could not find %s parameter definition. Type defaulting to int.", 
		 arg_object -> __o_name);
      }
      return fmt_rt_return (t_buf, INTEGER_CLASSNAME, TRUE, expr_buf_out);
    }
  }

  return NULL;
}

int arg_is_constant (MESSAGE_STACK messages, int start_idx, int end_idx) {

  int i;
  MESSAGE *m_tok;

  for (i = start_idx; i > end_idx; i--) {

    m_tok = messages[i];

    if (M_ISSPACE(m_tok)) continue;

    switch (M_TOK(m_tok))
      {
      case LABEL:
	if (!is_c_keyword(M_NAME(m_tok)))
	  return FALSE;
	break;
      default:
	break;
      }
  }

  return TRUE;
}

char *fmt_c_fn_obj_args_expr (MESSAGE_STACK messages, int fn_idx,
			      char *expr_buf) {
  char expr_buf_tmp[MAXMSG], expr_buf_tmp_2[MAXMSG];
  int arglist_start_ptr,
    arglist_end_ptr;
  int n_args, n_th_arg;
  CVAR *fn_param;
  CFUNC *fn_cfunc;
  ARGSTR argstrs[MAXARGS];

  if ((arglist_start_ptr = nextlangmsg (messages, fn_idx)) == ERROR)
    error (messages[fn_idx], "fmt_c_fn_obj_args_expr: Parser error.");
  if ((arglist_end_ptr = match_paren (messages, arglist_start_ptr, 
				      get_stack_top (messages))) == ERROR)
    error (messages[fn_idx], "fmt_c_fn_obj_args_expr: Parser error.");
  
  split_args_argstr (messages, arglist_start_ptr,
		     arglist_end_ptr, argstrs, &n_args);

  if ((fn_cfunc = get_function (M_NAME(messages[fn_idx]))) == NULL) {
    warning (messages[fn_idx], "Undefined function %s.\n", 
	       M_NAME(messages[fn_idx]));
  }
  SNPRINTF (expr_buf, MAXMSG, "%s (", M_NAME(messages[fn_idx]));
  for (n_th_arg = 0, fn_param = fn_cfunc -> params; 
       (n_th_arg < n_args);
       n_th_arg++, fn_param = fn_param -> next) {
    if (!fn_param) {
      _warning ("fmt_c_fn_obj_args_expr: wrong number of arguments.\n");
      break;
    }
    if (strstr (argstrs[n_th_arg].arg, "self")) {
      if (fn_param -> attrs & CVAR_ATTR_ELLIPSIS) {
	char *__c;
	if ((__c = basic_class_from_fmt_arg (messages, 
					     argstrs[n_th_arg].start_idx))
	    != NULL) {
	  add_arg (expr_buf,
		   fmt_rt_return 
		  (fmt_eval_expr_str (argstrs[n_th_arg].arg,
				      expr_buf_tmp), 
		   __c, TRUE, expr_buf_tmp_2),
		   n_th_arg, n_args);
	} else {
	  add_arg (expr_buf,
		   fmt_rt_return 
		   (fmt_eval_expr_str (argstrs[n_th_arg].arg,
				       expr_buf_tmp),
		    basic_class_from_cvar (messages[fn_idx], 
					   fn_param, 0),
		    TRUE, expr_buf_tmp_2),
		   n_th_arg, n_args);
	}
      } else {
	add_arg (expr_buf,
		 fmt_rt_return 
		(fmt_eval_expr_str (argstrs[n_th_arg].arg,
				    expr_buf_tmp),
		 basic_class_from_cvar (messages[fn_idx], fn_param, 0),
		 TRUE, expr_buf_tmp_2),
		 n_th_arg, n_args);
      }
      continue;
    }

    if (IS_CONSTANT_TOK(M_TOK(messages[argstrs[n_th_arg].start_idx]))) {
      add_arg (expr_buf, argstrs[n_th_arg].arg, n_th_arg, n_args);
      continue;
    } else {
      if (arg_needs_rt_eval (messages, argstrs[n_th_arg].start_idx)) {
	int end_idx;
	add_arg (expr_buf,
 		fn_param_return_trans 
		(messages[fn_idx], 
		 fn_cfunc, 
		 fmt_rt_expr (messages, argstrs[0].start_idx, &end_idx,
			      expr_buf_tmp), n_th_arg),
		 n_th_arg, n_args);
      } else {
	add_arg (expr_buf,
		 fn_param_return_trans 
		(messages[fn_idx], 
		 fn_cfunc, argstrs[n_th_arg].arg, n_th_arg),
		 n_th_arg, n_args);
      }
    }
    if (!IS_CVAR(fn_param))
       break; 
  }
  strcat (expr_buf, ")");

  for (n_th_arg = 0; n_th_arg < n_args; n_th_arg++) {
    __xfree (MEMADDR(argstrs[n_th_arg].arg));
    __xfree (MEMADDR(argstrs[n_th_arg]));
  }
  return expr_buf;
}

extern MESSAGE *var_messages[N_VAR_MESSAGES + 1];
extern int var_messageptr;

int resolve_arg2 (MESSAGE_STACK messages, int idx) {

  OBJECT *result_object;
  OBJECT *shadow_instance_var_object;
  int prev_msg_idx;
  int m_next_idx;
  int arg_method_arglist_limit;
  int n_param_sets, nth_set;
  METHOD *arg_method;

  /*
   *  Look for a receiver.
   */
  if ((result_object = class_object_search (M_NAME(messages[idx]), 
					    FALSE)) != NULL) {
    if (!M_OBJ(messages[idx])) {
      messages[idx] -> obj = result_object;
    } else {
      messages[idx] -> value_obj = result_object;
    }
    return SUCCESS;
  }
  
  if (messages[idx] -> attrs & TOK_SELF) {
    if ((interpreter_pass != expr_check) && 
	(interpreter_pass != method_pass) && !argblk) {
      self_outside_method_error (messages, idx);
    }
    if (!M_OBJ(messages[idx])) {
      messages[idx] -> obj = instantiate_self_object ();
    } else {
      messages[idx] -> value_obj = instantiate_self_object ();
    }
    return SUCCESS;
  }

  if ((result_object = get_local_object (M_NAME(messages[idx]), NULL)) 
      != NULL) {
    if (!M_OBJ(messages[idx])) {
      messages[idx] -> obj = result_object;
    } else {
      messages[idx] -> value_obj = result_object;
    }
    return SUCCESS;
  }

  /*
   *  Expression check pass.
   */
  if ((prev_msg_idx = prevlangmsg (messages, idx)) == ERROR)
    return SUCCESS;

  /*
   *  TO DO - This does not check for constant receivers.
   */
  if ((M_TOK(messages[prev_msg_idx]) != LABEL) &&
      (M_TOK(messages[prev_msg_idx]) != METHODMSGLABEL))
    return SUCCESS;

  if (M_VALUE_OBJ(messages[prev_msg_idx])) {
    OBJECT *_rcvr_obj, *_rcvr_value_obj;

    _rcvr_obj = M_VALUE_OBJ(messages[prev_msg_idx]);
    _rcvr_value_obj = (IS_OBJECT (_rcvr_obj -> instancevars) ?
		       _rcvr_obj -> instancevars : 
		       _rcvr_obj);
    if ((arg_method = get_instance_method (messages[idx], 
					   _rcvr_value_obj,
					   M_NAME(messages[idx]),
					   ERROR,
					   FALSE)) != NULL) {
      if (strcmp (arg_method -> name, "value") && 
	  (shadow_instance_var_object = 
	   get_instance_variable 
	   (M_NAME(messages[idx]),
	    (IS_OBJECT(M_VALUE_OBJ(messages[prev_msg_idx])) ? 
	     M_VALUE_OBJ(messages[prev_msg_idx])->__o_class->__o_name :
	     NULL),
	    FALSE)) != NULL) {
	method_shadows_instance_variable_warning_1
	  (messages[idx], 
	   ((interpreter_pass == method_pass) ? 
	    new_methods[new_method_ptr+1] -> method -> name : NULL),
	   rcvr_class_obj -> __o_name,
	   arg_method -> name, 
	   M_VALUE_OBJ(messages[prev_msg_idx])->__o_classname);
	return ERROR;
      } else {
	m_next_idx = nextlangmsg (messages, idx);
	if (METHOD_ARG_TERM_MSG_TYPE(messages[m_next_idx]))
	  return SUCCESS;
	arg_method_arglist_limit = 
	  method_arglist_limit (messages, m_next_idx, 
				arg_method -> n_params, 
				arg_method -> varargs);
	return arg_method_arglist_limit;
      }
    } else {
      if (M_VALUE_OBJ(messages[prev_msg_idx])) {
	OBJECT *__rcvr_obj = M_VALUE_OBJ(messages[prev_msg_idx]);

	if (is_method_proto (__rcvr_obj, M_NAME(messages[idx]))) {

	  m_next_idx = nextlangmsg (messages, idx);
	  (void)is_method_proto_max_params 
	    ((IS_CLASS_OBJECT (__rcvr_obj) ?
	      __rcvr_obj -> __o_name : 
	      __rcvr_obj -> __o_classname),
	     M_NAME(messages[idx]));
	  n_param_sets = get_proto_params_ptr () - 1;
	  if ((n_param_sets == 0) && (get_nth_proto_param (0) == 0)) {
	    /* This could also be in method_arglist_limit ().... */
	    return idx;
	  } else {
	    for (nth_set = n_param_sets; nth_set >= 0; --nth_set) {
	      if ((arg_method_arglist_limit = 
		   method_arglist_limit (messages, m_next_idx, 
					 get_nth_proto_param (nth_set),
					 get_nth_proto_varargs (nth_set))) 
		  != ERROR)
		return arg_method_arglist_limit;
	    }
	  }
	} else { /* if (is_method_proto ((IS_CLASS_OBJECT (__rcvr_obj) ? */

	  OBJECT *var_object;
	  
	  if (M_VALUE_OBJ(messages[prev_msg_idx])) {
	    if (((var_object = get_instance_variable 
		  (M_NAME(messages[idx]),
		   M_VALUE_OBJ(messages[prev_msg_idx]) -> __o_classname, 
		   FALSE)) != NULL) ||
		((var_object = get_class_variable 
		  (M_NAME(messages[idx]),
		   M_VALUE_OBJ(messages[prev_msg_idx]) -> __o_classname, 
		   FALSE)) != NULL)) {
	      
	      if (!M_OBJ(messages[idx])) {
		messages[idx] -> obj = var_object;
	      } else {
		messages[idx] -> value_obj = var_object;
	      }
	      messages[idx] -> receiver_msg = messages[prev_msg_idx];
	      
	    }
	    
	    return SUCCESS;
	  }

#if 0
	  /* For a later clause, probably. */
	  int stack_end_ptr;
	  stack_end_ptr = get_stack_top (messages);
	  __param_end_idx = __param_start_idx = -1;
	  start_idx = idx;
	  if (((end_idx = 
		scanforward (messages, idx, stack_end_ptr, 
			     SEMICOLON)) != -1) ||
	      ((end_idx = 
		scanforward (messages, idx, stack_end_ptr, 
			     CLOSEBLOCK)) != -1) ||
	      ((end_idx = 
		scanforward (messages, idx, stack_end_ptr, 
			     ARRAYCLOSE)) != -1)) {
	      
	    for (__idx = start_idx; 
		 (__idx >= end_idx) & (__param_end_idx == -1); 
	       __idx--) {
	      if (M_TOK(messages[__idx]) == OPENPAREN) {
		__param_start_idx = __idx;
		__param_end_idx = match_paren (messages, 
					       __param_start_idx,
					       end_idx);
	      }
	    }
	    fn_params (messages, __param_start_idx, 
		       __param_end_idx);

	    m_next_idx = nextlangmsg (messages, idx);
	    arg_method_arglist_limit = 
	      method_arglist_limit (messages, m_next_idx, 
				    get_param_ptr () + 1, 
				    get_vararg_status ());
	    return arg_method_arglist_limit;
	  }
#endif
	} /* if (is_method_proto ((IS_CLASS_OBJECT (__rcvr_obj) ? ... */
      }
    }
  }

  return 0;
}

/* 
 * For an expression that is on the right side of an '=' operator,
 * the end of the expression is either a semicolon, a subscript
 * close, or the first unmatched closing paren.
 */

static int c_rexpr_arg_limit (MESSAGE_STACK messages, int idx, int stack_end) {
  int i, separator_idx = idx;  /* avoid a warning */
  int n_parens = 0;
  int n_subscripts = 0;
  bool have_separator = false;
  for (i = idx; (i > stack_end) && !have_separator; --i) {

    if (M_ISSPACE(messages[i]))
      continue;

    switch (M_TOK(messages[i]))
      {
      case ARRAYOPEN:
	++n_subscripts;
	break;
      case ARRAYCLOSE:
	--n_subscripts;
	if (n_subscripts < 0) {
	  have_separator = true;
	  separator_idx = i;
	}
	break;
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (n_parens < 0) {
	  separator_idx = i;
	  have_separator = true;
	}
	break;
      case SEMICOLON:
	separator_idx = i;
	have_separator = true;
	break;
      default:
	break;
      }
  }
  if (have_separator)
    return prevlangmsg (messages, separator_idx);
  else
    return ERROR;
}
			      

static void cpea_cvar_cleanup (MSINFO *ms, int tok_idx) {
  int term_idx = scanforward (ms -> messages, tok_idx,
			      ms -> stack_ptr, SEMICOLON);
  fileout ("\ndelete_method_arg_cvars ();\n", 0, term_idx - 1);
}

/*
 *  Param evaluation needs to be lazy in the front end, in case we
 *  don't know what its class is.  If we evaluate the expression as a
 *  printf argument or a normal function argument, translate it to the
 *  type of the function parameter's prototype.  If the expression isn't a 
 *  function argument, translate into a basic C type.  
 *
 *  This fn can also be called for the right-hand side of an
 *  assignment operator so check for a '=' or other assignment op
 *  as the previous token.
 *
 */
int c_param_expr_arg (MSINFO *ms) {
  int n, n_th_arg, n_th_param, fn_idx, arg_limit, struct_expr_limit,
    op_idx, leading_typecast_start_idx, leading_typecast_lookback_idx,
    prev_idx = -1;
  CFUNC *fn;
  CVAR *c, *param_cvar;
  char *buf, tokbuf[MAXMSG], expr_buf[MAXMSG], esc_buf_out[MAXMSG];
  char *param_instance_var_expr_class;
  METHOD *m;
  int expr_start_idx = ms -> tok;  /* avoid a warning */
  OBJECT_CONTEXT cx;
  OBJECT *o, *param_class;
  bool cvar_reg = false;

  /* We have to make two calls to prevlangmsg. */
  if ((op_idx = prevlangmsg (ms -> messages, ms -> tok)) != ERROR) {
    if (ms -> messages[op_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
      expr_start_idx = op_idx;
    } else if ((M_TOK(ms -> messages[op_idx]) == CLOSEPAREN) &&
	       (ms -> messages[op_idx] -> attrs & TOK_IS_TYPECAST_EXPR)) {
      /* scan back through a leading typecast */
      if ((leading_typecast_start_idx =
	   match_paren_rev (ms -> messages, op_idx, ms -> stack_start)) != ERROR) {
	while (leading_typecast_start_idx <= ms -> stack_start) {
	  if (M_TOK(ms -> messages[leading_typecast_start_idx]) == OPENPAREN) {
	    if ((leading_typecast_lookback_idx =
		 prevlangmsg (ms -> messages, leading_typecast_start_idx))
		!= ERROR) {
	      if (M_TOK(ms -> messages[leading_typecast_lookback_idx])
		  != OPENPAREN) {
		expr_start_idx = leading_typecast_start_idx;
		goto typecast_lookback_done;
	      } else {
		leading_typecast_start_idx =
		  leading_typecast_lookback_idx;
	      }
	    }
	  }
	}
      }
    } else {
      expr_start_idx = ms -> tok;
    }
  }
 typecast_lookback_done:
  
  if ((op_idx = prevlangmsg (ms -> messages, expr_start_idx)) != ERROR) {
    if (IS_C_ASSIGNMENT_OP (M_TOK(ms -> messages[op_idx]))) {
      arg_limit = c_rexpr_arg_limit (ms -> messages,  expr_start_idx, ms -> stack_ptr);
    } else {
      arg_limit = method_arg_limit (ms -> messages, expr_start_idx);
    }
  } else {
    arg_limit = method_arg_limit (ms -> messages, expr_start_idx);
  }

  toks2str (ms -> messages, expr_start_idx, arg_limit, tokbuf);

  snprintf (expr_buf, MAXMSG, "%s(\"%s\")", EVAL_EXPR_FN, 
	    escape_str_quotes (tokbuf, esc_buf_out));

  for (n = expr_start_idx; n >= arg_limit; n--) {
    if (M_TOK(ms -> messages[n]) != LABEL)
      continue;
    if ((o = get_local_object (M_NAME(ms -> messages[n]), NULL)) != NULL) {
      ms -> messages[n] -> obj = o;
      prev_idx = n;
      continue;
    }
    if ((param_class = new_method_parameter_class_object (ms -> messages, n))
	!= NULL) {
      ms -> messages[n] -> obj = param_class;
      prev_idx = n;
      continue;
    }
    if ((prev_idx != -1) && (IS_OBJECT(ms -> messages[prev_idx] -> obj))) {
      if (get_instance_variable_series (ms -> messages[prev_idx] -> obj,
					ms -> messages[n], n,
					ms -> stack_ptr)) {
	continue;
      }
    }
    if (((c = get_local_var (M_NAME(ms -> messages[n]))) != NULL) ||
	((c = get_global_var (M_NAME(ms -> messages[n]))) != NULL)) {
      if (is_struct_or_union_expr (ms -> messages, n, 
				   ms -> stack_start,
				   ms -> stack_ptr )) {
	generate_register_c_method_arg_call 
	  (c, 
	   struct_or_union_expr (ms -> messages, n, 
				 ms -> stack_ptr,
				 &struct_expr_limit),
	   LOCAL_VAR,
	   expr_start_idx);
	cvar_reg = true;
	n = struct_expr_limit;
      } else {
	/*
	 *  Adds the --warnduplicatenames option here.
	 */
	if (global_var_is_declared_not_duplicated 
	    (ms -> messages, expr_start_idx, M_NAME(ms -> messages[expr_start_idx]))) {
	  generate_register_c_method_arg_call (c, M_NAME(ms -> messages[n]),
					       LOCAL_VAR,
					       expr_start_idx);
	  cvar_reg = true;
	} else {
	  generate_register_c_method_arg_call (c, M_NAME(ms -> messages[n]),
					       LOCAL_VAR,
					       FRAME_START_IDX);
	  cvar_reg = true;
	}
      }
    }
  }

  /* this should stay for now, so we don't alter ms -> tok */
  if (is_fmt_arg (ms -> messages, expr_start_idx, 
		  ms -> stack_start, ms -> stack_ptr)) {
    fileout (fmt_printf_fmt_arg (ms -> messages,
				 expr_start_idx,
				 ms -> stack_start,
				 expr_buf, esc_buf_out),
	     0, expr_start_idx);
    if (cvar_reg)
      cpea_cvar_cleanup (ms, expr_start_idx);
  } else {
    if ((n_th_arg = obj_expr_is_arg (ms -> messages, expr_start_idx,
				     ms -> stack_start,
				     &fn_idx)) != ERROR) {
      if ((fn = get_function (M_NAME(ms -> messages[fn_idx]))) == NULL) {
	warning (ms -> messages[expr_start_idx],
		 "Prototype of C function, \"%s,\" not found.",
		 M_NAME(ms -> messages[fn_idx]));
	warning (ms -> messages[expr_start_idx],
		 "Argument type of, \"%s,\" defaulting to, \"int.\"",
		 M_NAME(ms -> messages[expr_start_idx]));
	fileout (fmt_rt_return (expr_buf,
				INTEGER_CLASSNAME,
				TRUE, esc_buf_out),
		 0, expr_start_idx);
	if (cvar_reg)
	  cpea_cvar_cleanup (ms, expr_start_idx);
	goto c_param_expr_arg_cleanup;
      }
      for (n = 0, param_cvar = fn -> params; 
	   (n < n_th_arg) && 
	     (IS_CVAR(param_cvar)); ++n)
	param_cvar = param_cvar -> next;
      if (param_cvar) {
	fileout (fmt_rt_return (expr_buf,
				basic_class_from_cvar (ms -> messages[fn_idx],
							 param_cvar, 0),
				TRUE, esc_buf_out),
		 0, expr_start_idx);
	if (cvar_reg)
	  cpea_cvar_cleanup (ms, expr_start_idx);
	goto c_param_expr_arg_cleanup;
      } else {
	/*
	 *  Use the class of the parameter definition.
	 */
	m = new_methods[new_method_ptr + 1] -> method;
	for (n_th_param = 0; n_th_param < m -> n_params; n_th_param++) {
	  if (!strcmp (m->params[n_th_param]->name,
		       M_NAME(ms -> messages[ms -> tok])))
	    break;
	}
	fileout (fmt_rt_return (expr_buf,
				m->params[n_th_param]->class, TRUE,
				esc_buf_out),
		 0, expr_start_idx);
	if (cvar_reg)
	  cpea_cvar_cleanup (ms, expr_start_idx);
	goto c_param_expr_arg_cleanup;
      }
    } else { /* ... obj_expr_is_arg ... */
      /*
       *  Simple expression only for now.
       */
      m = new_methods[new_method_ptr+1] -> method;
      for (n_th_param = 0; n_th_param < m -> n_params; n_th_param++) {
	if (!strcmp (m->params[n_th_param]->name,
		     M_NAME(ms -> messages[ms -> tok])))
	  break;
      }
      if ((param_instance_var_expr_class = 
	   get_param_instance_variable_series_class 
	   (m -> params[n_th_param],
	    ms -> messages,
	    ms -> tok,
	    ms -> stack_ptr)) != NULL) {
	fileout (fmt_rt_return (expr_buf,
				param_instance_var_expr_class, TRUE,
				esc_buf_out),
		 0, expr_start_idx);
	if (cvar_reg)
	  cpea_cvar_cleanup (ms, expr_start_idx);
	goto c_param_expr_arg_cleanup;
      } else {
	cx = object_context (ms -> messages, expr_start_idx);
	switch (cx)
	  {
	  case c_argument_context:
	    /* E.g., the case where the param is on the right
	       side of an assignment: <cvar> = <param>....
	       The c_lval_class () fn is still somewhat limited.
	       TODO - Try to derive the class from the rval and
	       compare it to the lval's class.
	    */
	    if ((buf = c_lval_class (ms -> messages, expr_start_idx)) != NULL) {
	      fileout (fmt_rt_return (expr_buf, buf, TRUE, esc_buf_out),
		       0, expr_start_idx);
	      if (cvar_reg)
		cpea_cvar_cleanup (ms, expr_start_idx);
	    } else {
	      fileout (fmt_rt_return (expr_buf,
				      m->params[n_th_param]->class,
				      TRUE, esc_buf_out),
		       0, expr_start_idx);
	      if (cvar_reg)
		cpea_cvar_cleanup (ms, expr_start_idx);
	      goto c_param_expr_arg_cleanup;
	    }
	    break;
	  default:
	    fileout (fmt_rt_return (expr_buf,
				    m->params[n_th_param]->class, TRUE,
				    esc_buf_out),
		     0, expr_start_idx);
	    if (cvar_reg)
	      cpea_cvar_cleanup (ms, expr_start_idx);
	    break;
	  }
      }
    }
  } /* is_fmt_arg ... */

 c_param_expr_arg_cleanup:
  for (n = expr_start_idx; n >= arg_limit; n--) {
    ++ms -> messages[n] -> evaled;
    ++ms -> messages[n] -> output;
  }
  return SUCCESS;
}

extern CTRLBLK *ctrlblks[MAXARGS + 1];
extern int ctrlblk_ptr;

/*
 *  obj_expr_is_arg can sometimes return an arg for
 *  an "if" statement, so check here.
 */
int obj_expr_is_fn_arg (MESSAGE_STACK messages, int expr_start_idx,
			int stack_start_idx, int *__fn_idx) {
  int n_th_arg;
  if ((n_th_arg = obj_expr_is_arg (messages, expr_start_idx,
				   stack_start_idx, __fn_idx)) != ERROR) {
    if (ctrlblk_pred && (*__fn_idx == C_CTRL_BLK -> keyword_ptr))
      return ERROR;
    else
      return n_th_arg;
			 
  }
  return ERROR;
}

int obj_expr_is_fn_arg_ms (MSINFO *ms, int *__fn_idx) {
  int n_th_arg;
  if ((n_th_arg = obj_expr_is_arg_ms (ms, __fn_idx)) != ERROR) {

    if (ctrlblk_pred && (*__fn_idx == C_CTRL_BLK -> keyword_ptr))
      return ERROR;
    else
      return n_th_arg;
			 
  }
  return ERROR;
}

/*
 *  Only class or instance variable expressions so far - doesn't handle
 *  "self," either.
 */
int complex_arglist_limit (MESSAGE_STACK messages, int arglist_start) {
  OBJECT *rcvr_obj;
  int next_tok_idx, expr_end_idx;
  char expr_buf[MAXMSG];
  if ((rcvr_obj = get_object (M_NAME(messages[arglist_start]), NULL)) != NULL) {

    if (!IS_OBJECT(rcvr_obj -> __o_class)) {
      if ((rcvr_obj -> __o_class = 
	   get_class_object (rcvr_obj -> __o_classname)) == NULL) {
	warning (messages[arglist_start], 
		 "complex_arglist_limit: could not find class, \"%s.\"",
		 rcvr_obj -> __o_classname);
      }
    }

    messages[arglist_start]->obj = rcvr_obj;
    if ((next_tok_idx = nextlangmsg (messages, arglist_start)) != ERROR) {
      if (get_class_variable (M_NAME(messages[next_tok_idx]),
			      rcvr_obj -> __o_class -> __o_name,
			      FALSE) ||
	  get_instance_variable (M_NAME(messages[next_tok_idx]),
				 rcvr_obj -> __o_class -> __o_name,
				 FALSE)) {
	messages[next_tok_idx]->receiver_obj = rcvr_obj;
	if (complex_var_or_method_message (rcvr_obj, messages, next_tok_idx)) {
	  fmt_rt_expr (messages, arglist_start, &expr_end_idx,
		       expr_buf);
	  return expr_end_idx;
	}
      }
    }
  }
  return ERROR;
}

char *writable_arg_rt_arg_expr (MESSAGE_STACK messages, int start_idx, 
				int end_idx, char *eval_expr_buf) {

  int i, j, lookahead;
  int stack_start_idx;
  int agg_var_end_idx;
  char expr_buf[MAXMSG], esc_buf_out[MAXMSG];
  MESSAGE *m = NULL;   /* Avoid a warning. */
  RT_EXPR_CLASS rt_expr_class = rt_expr_null;

  *expr_buf = 0;

  stack_start_idx = get_stack_top (messages);

  for (i = start_idx; i >= end_idx; i--) {
    m = message_stack_at (i);

    if (libc_fn_needs_writable_args (M_NAME(m))) {
      if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	if ((messages[i] -> tokentype == LABEL) && 
	    (messages[lookahead] -> tokentype == OPENPAREN)) {
	  char *e;
	  if ((e = clib_fn_expr (messages, i, i, m -> obj)) != NULL) {
	    strcatx2 (expr_buf, e, NULL);
	    rt_expr_class = rt_expr_fn;
	    i =  match_paren (messages, lookahead, stack_start_idx);
	    for (j = lookahead; j >= i; j--) {
	      ++messages[j] -> evaled;
	      ++messages[j] -> output;
	    }
	  }
	}
      }
    } else {
      if (IS_C_OP(M_TOK(m))) {
	operand_type_check (message_stack (), i);
	strcatx2 (expr_buf, m -> name, NULL);
      } else {
	if (get_function (M_NAME(m))) {
	  if (rt_expr_class == rt_expr_null)
	    rt_expr_class = rt_c_fn;
	  strcatx2 (expr_buf, m -> name, NULL);
	}  else {
	  strcatx2 (expr_buf, m -> name, NULL);
	}
      }
    }

    /*
     *  If the expression also contains C variables or functions,
     *  they get registered now in register_c_var () called by
     *  method_args (), etc., so check here if the token has
     *  been evaluated and register the C variable here if 
     *  necessary.
     */
    if ((m -> tokentype == LABEL) && !m -> evaled) {
      CVAR *c;
      if (((c = get_local_var (m -> name)) != NULL) ||
	  ((c = get_global_var (m -> name)) != NULL)) {
	register_c_var (m, message_stack (), i, &agg_var_end_idx);
      } else {
	if (m -> attrs & TOK_SELF) {
	  if (self_class_or_instance_variable_lookahead 
	      (messages, i)) {
	    rt_expr_class = rt_self_instvar_simple_expr;
	  }
	}
      }
    }
    ++m -> evaled;
    ++m -> output;
  }
      
  snprintf (eval_expr_buf, MAXMSG, "%s(%s (\"%s\"), %d)", INT_TRANS_FN, 
	    EVAL_EXPR_FN, escape_str_quotes(expr_buf, esc_buf_out), FALSE);
  return eval_expr_buf;
}

ARG *create_arg (void) {
  ARG *a;
  a = __xalloc ((int)sizeof (struct _arg));
  a -> sig = ARG_SIG;
  return a;
}

ARG *create_arg_init (OBJECT *o) {
  ARG *a;
  a = create_arg ();
  a -> obj = o;
  return a;
}

void delete_arg (ARG *a) {
  __xfree (MEMADDR(a));
}

char *stdarg_fmt_arg_expr (MESSAGE_STACK messages, int method_idx, 
			   METHOD *method,
			   char *result_buf) {
  int arglist_start, arglist_end, rcvr_end, i;
  char tok_buf[MAXMSG], esc_buf_out[MAXMSG];
  if ((rcvr_end = prevlangmsg (messages, method_idx)) == ERROR)
    error (messages[method_idx], "Parser error.");
  if (method -> n_params) {
    if ((arglist_start = nextlangmsg (messages, method_idx)) == ERROR)
      error (messages[method_idx], "Parser error.");
    if ((arglist_end = method_arglist_limit (messages, arglist_start,
					     method -> n_params,
					     method -> varargs)) == ERROR)
      error (messages[method_idx], "Parser error.");
    
    toks2str (messages, rcvr_end, arglist_end, tok_buf);
    de_newline_buf (tok_buf);
    strcatx (result_buf, EVAL_EXPR_FN, " (\"", 
	     escape_str_quotes (tok_buf, esc_buf_out), "\")", NULL);
    for (i = rcvr_end; i >= arglist_end; i--) {
      ++messages[i] -> evaled;
      ++messages[i] -> output;
    }
    return result_buf;
  } else {
    if (!messages[rcvr_end] -> obj && !messages[rcvr_end] -> value_obj) {
      CVAR *c;
      char *cvar_class;
      OBJECT *class_obj;
      int agg_var_end_idx;
      FRAME *f;
      if (((c = get_local_var (M_NAME(messages[rcvr_end]))) != NULL) ||
	  ((c = get_global_var (M_NAME(messages[rcvr_end]))))) {
	if ((cvar_class = basic_class_from_cvar (messages[rcvr_end],
						 c, 0)) != NULL) {
	  if ((class_obj = get_class_object (cvar_class)) != NULL) {
	    messages[rcvr_end] -> obj =
	      create_object (class_obj -> __o_name, M_NAME(messages[rcvr_end]));
	    register_c_var (messages[rcvr_end], message_stack (), rcvr_end,
			    &agg_var_end_idx);
	    rt_library_method_call 
	      (messages[rcvr_end] -> obj, method, messages, method_idx, result_buf);
	    delete_object (messages[rcvr_end] -> obj);
	    messages[rcvr_end] -> obj = NULL;
 	    f = frame_at (CURRENT_PARSER -> frame - 1);
	    output_delete_cvars_call (messages, f -> message_frame_top + 1,
				      get_stack_top (message_stack ()));
 	    return result_buf;
	  }
	}
      } else if (M_TOK(messages[rcvr_end]) == CLOSEPAREN) {
	int open_paren_idx, end_idx;
	if ((open_paren_idx = match_paren_rev (messages, rcvr_end,
					       stack_start (messages)))
	    != ERROR) {
	  fmt_rt_expr (messages, open_paren_idx, &end_idx, result_buf);
	  return result_buf;
	}
      }
    } else {
      return rt_library_method_call 
	(M_VALUE_OBJ(messages[rcvr_end]),
	 method, messages, method_idx, result_buf);
    }
  }
  return NULL;
}

int method_expr_is_c_fmt_arg (MESSAGE_STACK messages, int method_msg_ptr,
			      int stack_start, int stack_top) {
  int rcvr_tok_idx, r;
  int array_open_idx;
  OBJECT_CONTEXT context;
  if ((rcvr_tok_idx = prevlangmsg (messages, method_msg_ptr)) != ERROR) {
    if (M_TOK(messages[rcvr_tok_idx]) == CLOSEPAREN) {
      if ((rcvr_tok_idx = 
	   match_paren_rev (messages, rcvr_tok_idx, stack_start)) == ERROR) {
	warning (messages[method_msg_ptr], 
		 "method_expr_is_c_fmt_arg: mismatched receiver parentheses.");
	return FALSE;
      }
    } else {
      if (M_TOK(messages[rcvr_tok_idx]) == ARRAYCLOSE) {
	int __i, n_blocks;
	for (__i = rcvr_tok_idx, array_open_idx = -1, n_blocks = 0; 
	     (__i <= stack_start) && (array_open_idx == -1);
	     __i++) {
	  switch (M_TOK(messages[__i]))
	    {
	    case ARRAYCLOSE:
	      --n_blocks;
	      break;
	    case ARRAYOPEN:
	      ++n_blocks;
	      break;
	    }
	  if (!n_blocks)
	    array_open_idx = __i;
	}
	rcvr_tok_idx = prevlangmsg (messages, array_open_idx);
      }
    }

    r = is_fmt_arg (messages, rcvr_tok_idx,
		    stack_start, stack_top);
    context = object_context (messages, rcvr_tok_idx);
    return (r && (context == c_argument_context));
  }
  return FALSE;
}

/*
 *  Rather simple function just fills a semantic hole - only checks
 *  whether a label token at the arglist start is also a method (but
 *  not also an instance or class variable message), which means that
 *  the method can *only* have zero parameters. No reason yet to issue
 *  a warning for a method which returns, "Any," either.  Tokens at
 *  the start of an argument list are taken as complex expressions
 *  anyway.  */
int compound_method_limit (MESSAGE_STACK messages, 
			   int method_msg_ptr, int arglist_start,
			   int n_params, int varargs) {
  METHOD *method, *arg_method;
  int prev_tok_idx, arglist_start_2_idx;
  MESSAGE *m_prev_tok, *m, *m_arg_start;
  OBJECT *m_class_object;
  if (n_params || varargs)
    return ERROR;
  if (M_TOK(messages[arglist_start]) != LABEL) 
    return ERROR;

 if ((prev_tok_idx = prevlangmsg (messages, method_msg_ptr)) != ERROR) {
    m_prev_tok = messages[prev_tok_idx];
    m = messages[method_msg_ptr];
    if (((method = get_instance_method 
	  (m_prev_tok, m_prev_tok -> obj, M_NAME(m), 
	   ERROR, FALSE)) != NULL) ||
	((method = get_class_method 
	  (m_prev_tok, m_prev_tok -> obj, M_NAME(m), 
	   ERROR, FALSE)) != NULL)) {
      
      if (*method -> returnclass &&
	  ((m_class_object = get_class_object (method ->returnclass)) 
	   != NULL)) {
	m_arg_start = messages[arglist_start];
	if (((arg_method = get_instance_method 
	      (m_arg_start, m_class_object, M_NAME(m_arg_start), 
	       ERROR, FALSE)) 
	     != NULL) ||
	    ((arg_method = get_class_method 
	      (m_arg_start, m_class_object, M_NAME(m_arg_start), 
	       ERROR, FALSE)) 
	     != NULL)) {
  	  if (!get_instance_variable (M_NAME(m_arg_start),
  				      m_class_object -> __o_name,
  				      FALSE) && 
  	      !get_class_variable (M_NAME(m_arg_start),
  				   m_class_object -> __o_name,
  				   FALSE)) {
	    messages[arglist_start] -> tokentype = METHODMSGLABEL;
	    if ((arglist_start_2_idx = 
		 nextlangmsg (messages, arglist_start)) != ERROR) {
	      if (METHOD_ARG_TERM_MSG_TYPE(messages[arglist_start_2_idx])){
		return arglist_start;
	      } else {
		return method_arglist_limit_2 (messages, 
					       arglist_start,
					       arglist_start_2_idx,
					       arg_method -> n_params,
					       arg_method -> varargs);
	      }
	    } else {
	      return arglist_start;
	    }
  	  }
	}
      }
    }
  }

  return ERROR;
}

char *format_method_arg_accessor (int param_idx, char *tok, bool varargs,
				  char *expr_out) {
  if (argblk || varargs) {
    return fmt_eval_expr_str (tok, expr_out);
  } else {
    strcatx (expr_out, METHOD_ARG_ACCESSOR_FN, "(", 
	     ascii[param_idx], ")", NULL);
    return expr_out;
  }
  return NULL;
}
