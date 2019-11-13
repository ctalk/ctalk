/* $Id: constrcvr.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2014, 2016, 2018, 2019 
    Robert Kiesling, rk3314042@gmail.com.
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
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "classlib.h"
#include "symbol.h"

/*
 *  Handle constant receiver expressions, in no particular order.
 */

extern I_PASS interpreter_pass;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern bool ctrlblk_pred,                     /* Global states.        */
  ctrlblk_blk,
  ctrlblk_else_blk;
extern bool argblk;

extern ARG_TERM arg_c_fn_terms[MAXARGS]; /* Declared in ctoobj.c. */
extern int arg_c_fn_term_ptr;

static bool const_rcvr_is_rval_start (MESSAGE_STACK messages,
				      int idx_rcvr,
				      int idx_method) {
  int expr_end_idx;
  char *lval_class;
  char buf2[MAXMSG];
  char expr_buf_out[MAXMSG], expr_buf_out_2[MAXMSG];
  char *c;
  int i;

  if ((lval_class = c_lval_class (messages, idx_rcvr)) == NULL)
    return FALSE;

  
  /* 
     Again, the first fmt_rt_expr () call is to find the end of the
     expression. The second call formats the expression with
     the argblk variables provided by register_argblk_c_vars_1 (). 
  */
  (void)fmt_rt_expr (messages, idx_rcvr, &expr_end_idx, expr_buf_out);
  if (argblk) {
    /* Note - this might duplicate register_cvar_arg_expr_a (), called
       resolve (). */
    if ((c = fmt_register_argblk_c_vars_1 (messages, idx_rcvr, expr_end_idx))
	!= NULL) {
      fileout (c, 0, FRAME_START_IDX);
    }
  }
  if (str_eq (lval_class, "String")) {
    argblk_ptrptr_trans_1 (messages, idx_rcvr);
    strcatx (buf2, "(char **)", 
	     fmt_rt_return (fmt_rt_expr (messages, idx_rcvr,
					 &expr_end_idx, expr_buf_out), 
			    lval_class, TRUE, expr_buf_out_2), NULL);
    fileout (buf2, 0, idx_rcvr);
  } else {
    fileout (fmt_rt_return 
	     (fmt_rt_expr (messages, idx_rcvr, &expr_end_idx, expr_buf_out),
	      lval_class, TRUE, expr_buf_out_2), 
	     0, idx_rcvr);
  }

  for (i = idx_rcvr; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }

  return TRUE;
}

static bool const_rcvr_and_c_expr_fmt_arg (MESSAGE_STACK messages,
				 int idx_rcvr, int idx_method) {
  int i, i_2, expr_end;
  int fn_arg_start, fn_arg_end;
  int stack_ptr;
  CVAR *cvar;
  CFUNC *cfunc;
  char expr_out[MAXMSG], expr_out_2[MAXMSG];
  
  if (!(messages[idx_rcvr] -> attrs & TOK_IS_PRINTF_ARG) ||
      !(messages[idx_rcvr] -> attrs & RCVR_OBJ_IS_CONSTANT))
    /* RCVR_OBJ_IS_CONSTANT is set in have_constant_rcvr_class,
       and the object should be the class object. */
    return false;
  for (i = idx_method; ; i--) {
    if (METHOD_ARG_TERM_MSG_TYPE(messages[i])) {
      return false;
    } else if (M_TOK(messages[i]) == LABEL) {
      if (((cvar = get_local_var (M_NAME(messages[i]))) != NULL) ||
	  ((cvar = get_global_var (M_NAME(messages[i]))) != NULL)) {
	generate_register_c_method_arg_call 
	  (cvar, M_NAME(messages[i]), FRAME_SCOPE, FRAME_START_IDX);
	fileout
	  (fmt_rt_return
	   (fmt_rt_expr (messages, idx_rcvr, &expr_end, expr_out),
	    /* the receiver object is actually the
	       receiver's class object here. */
	    M_OBJ(messages[idx_rcvr]) -> __o_name,
	    TRUE, expr_out_2),
	   FALSE, idx_rcvr);
	for (i_2 = idx_rcvr; i_2 >= expr_end; --i_2) {
	  ++messages[i_2] -> evaled;
	  ++messages[i_2] -> output;
	}
	return true;
      } else if ((cfunc = get_function (M_NAME(messages[i]))) != NULL) {
	stack_ptr = get_stack_top (messages);
	if ((fn_arg_start = scanforward (messages, i, stack_ptr,
					 OPENPAREN)) == ERROR)
	  error (messages[i], "Parser error.");
	if ((fn_arg_end = match_paren (messages, fn_arg_start, stack_ptr))
	    == ERROR)
	  error (messages[i], "Parser error.");
	arg_c_fn_terms[arg_c_fn_term_ptr].messages = messages;
	arg_c_fn_terms[arg_c_fn_term_ptr].start = i;
	arg_c_fn_terms[arg_c_fn_term_ptr].end = fn_arg_end;
	++arg_c_fn_term_ptr;
	output_mixed_c_to_obj_fmt_arg (messages, idx_rcvr, idx_method);
	return true;
      }
    }
  }
  return false;
}

/* returns stack index of receiver if found, 0 if not a
   constant receiver */
static int have_constant_rcvr_class (MESSAGE_STACK messages, 
				     int method_msg_ptr) {
  int prev_tok_idx;
  OBJECT *result_class;
  METHOD *method;
  MESSAGE *m_msg, *m_rcvr;
  m_msg = messages[method_msg_ptr];
  if ((prev_tok_idx = prevlangmsg (messages, method_msg_ptr)) != ERROR) {
    m_rcvr = messages[prev_tok_idx];
    /*
     *  A numeric constant and a C operator alone should not need any
     *  further evaluation.
     */
    if (IS_NUMERIC_CONSTANT_TOK(M_TOK(m_rcvr))) {
      if (IS_C_OP(M_TOK(m_msg)))
	return 0;
    }
    if (IS_CONSTANT_TOK(M_TOK(m_rcvr))) {
      if ((result_class = 
	   get_class_object (basic_class_from_constant_tok (m_rcvr)))
	  != NULL) {
	if (((method = get_instance_method (m_msg,
					    result_class,
					    M_NAME(m_msg),
					    ERROR, FALSE)) != NULL) ||
	    ((method = get_class_method (m_msg,
					 result_class,
					 M_NAME(m_msg),
					 ERROR, FALSE)) != NULL)) {
	  m_msg -> attrs = RCVR_OBJ_IS_CONSTANT;
	  m_msg -> receiver_obj = result_class;
	  m_msg -> receiver_msg = m_rcvr;
	  m_rcvr -> obj = result_class;
	  m_rcvr -> attrs |= RCVR_OBJ_IS_CONSTANT;
	  if (M_TOK(m_msg) == LABEL) m_msg -> tokentype = METHODMSGLABEL;
	  return prev_tok_idx;
	}
      }
    }
  }
  return 0;
}

/*
 *  <constant_tok> <op> call from anywhere *except* within
 *  a receiver expression (closing paren followed by a method 
 *  label).  This needs an update if it's ever called with
 *  anything besides the main message stack.
 */
int method_call_constant_tok_expr_a (MESSAGE_STACK messages,
					    int idx_method) {
  int idx_rcvr;
  if ((idx_rcvr = have_constant_rcvr_class (message_stack (),
						idx_method))
      != 0) {
    if (!is_in_rcvr_subexpr (message_stack (), 
			     idx_method,
			     P_MESSAGES,
			     get_stack_top (messages))) {
      if (const_rcvr_is_rval_start (messages, idx_rcvr, idx_method)) {
	return TRUE;
      } else if (const_rcvr_and_c_expr_fmt_arg (messages, idx_rcvr,
						idx_method)) {
	return TRUE;
      } else {
	method_call (idx_method); 
	return TRUE;
      }
    }
  }
  return FALSE;
}

/*
 *  Register a CVAR within a receiver expression.
 *
 *  Also check for a CVAR as the argument.
 */
int register_cvar_rcvr_expr_a (MESSAGE_STACK messages, 
			       int method_idx,
			       CVAR *rcvr_cvar,
			       char *rcvr_name) {
  int next_tok_idx, aggregate_end_idx;
  int agg_var_end_idx, subexpr_end_a;
  CVAR *arg_cvar;
  if ((subexpr_end_a = is_in_rcvr_subexpr (message_stack (),
					   method_idx,
					   /* this is okay if we only use
					      the main message stack */
					   P_MESSAGES,
					   get_stack_top (messages)))
      != 0) {
    if ((next_tok_idx = nextlangmsg (messages, subexpr_end_a)) != ERROR) {
      if (!is_method_name (M_NAME(messages[next_tok_idx]))) {
	return FALSE;
      }
    }
    if ((rcvr_cvar -> type_attrs & CVAR_ATTR_STRUCT_TAG) ||
	(rcvr_cvar -> type_attrs & CVAR_ATTR_STRUCT_PTR_TAG) ||
	(rcvr_cvar -> attrs & CVAR_ATTR_STRUCT_DECL)) {
      MESSAGE *m_method;
      m_method = message_stack_at (method_idx);
      /*
       *  This should be sufficient to prevent interpreting an actual
       *  -> or . operator as an overloaded method.  Of course, if it
       *  isn't sufficient, we can just find the CVAR for the struct
       *  member and output a possibly unecessary CVAR registration
       *  call anyway.  See cvar_is_method_receiver () in resolve.c for
       *  examples of how to do this.
       */
      if ((M_TOK(m_method) == PERIOD) ||
	  (M_TOK(m_method) == DEREF)) 
	return FALSE;
    } else {
      generate_register_c_method_arg_call (rcvr_cvar, rcvr_name, FRAME_SCOPE,
					   FRAME_START_IDX);
    }

    if ((next_tok_idx = nextlangmsg (messages, method_idx)) != ERROR) {
      if (M_TOK(messages[next_tok_idx]) == LABEL) {
	if (((arg_cvar = get_local_var (M_NAME(messages[next_tok_idx]))) 
	     != NULL) ||
	    ((arg_cvar = get_global_var (M_NAME(messages[next_tok_idx])))
	     != NULL)) {
	  if ((aggregate_end_idx = 
	       is_aggregate_term_b (messages, next_tok_idx)) != 0) {
	    register_c_var (messages[method_idx], 
			    messages, next_tok_idx,
			    &agg_var_end_idx);
	  } else {
	    generate_register_c_method_arg_call 
	      (arg_cvar, M_NAME(messages[next_tok_idx]), FRAME_SCOPE,
	       FRAME_START_IDX);
	  }
	}
      }
    }

    return TRUE;
  } 
  return FALSE;
}

/*
 *  Register an aggregate expression  CVAR within a receiver expression.
 *
 *  Also check for a CVAR as the argument.
 */
int register_cvar_rcvr_expr_b (MESSAGE_STACK messages, 
			       int method_idx) {
  int prev_tok_idx, next_tok_idx, aggregate_start_idx, aggregate_end_idx;
  int agg_var_end_idx;
  CVAR *rcvr_cvar, *arg_cvar;
  if (is_in_rcvr_subexpr (message_stack (), method_idx,
			  /* here, too, this is okay as long
			     as the function is called with
			     the main message stack. */
			  P_MESSAGES,
			  get_stack_top (messages))) {
    if ((prev_tok_idx = prevlangmsg (messages, method_idx)) != ERROR) {

      if (M_TOK(messages[prev_tok_idx]) == ARRAYCLOSE) { 
	if ((aggregate_start_idx = is_aggregate_term_a (messages, prev_tok_idx))
	    != 0) {
	  if (!(messages[aggregate_start_idx] -> attrs &
		TOK_CVAR_REGISTRY_IS_OUTPUT)) {
	    if (((rcvr_cvar = get_local_var 
		  (M_NAME(messages[aggregate_start_idx])))
		 != NULL) ||
		((rcvr_cvar = get_global_var 
		  (M_NAME(messages[aggregate_start_idx])))
		 != NULL)) {
	      /* Add attr here... at aggregate_start_idx */
	      register_c_var (messages[method_idx],
			      messages, aggregate_start_idx,
			      &agg_var_end_idx);
	      messages[aggregate_start_idx] -> attrs |=
		TOK_CVAR_REGISTRY_IS_OUTPUT;
	    } /* if (((rcvr_cvar = get_local_var ... */
	  }

	  if ((next_tok_idx = nextlangmsg (messages, method_idx)) != ERROR) {
	    if (M_TOK(messages[next_tok_idx]) == LABEL) {
	      if (((arg_cvar = get_local_var (M_NAME(messages[next_tok_idx])))
		   != NULL) ||
		  ((arg_cvar = get_global_var (M_NAME(messages[next_tok_idx])))
		   != NULL)) {
		if ((aggregate_end_idx = 
		     is_aggregate_term_b (messages, next_tok_idx)) != 0) {
		  /* And here... at next_tok_idx */
		  if (!(messages[next_tok_idx] -> attrs & 
			TOK_CVAR_REGISTRY_IS_OUTPUT)) {
		    register_c_var (messages[method_idx],
				    messages, next_tok_idx,
				    &agg_var_end_idx);
		    messages[next_tok_idx] -> attrs |=
		      TOK_CVAR_REGISTRY_IS_OUTPUT;
		  }
		} else {
		  generate_register_c_method_arg_call
		    (arg_cvar, M_NAME(messages[next_tok_idx]), FRAME_SCOPE,
		     FRAME_START_IDX);
		}
	      }
	    }
	  } /* if ((next_tok_idx = nextlangmsg (messages, method_idx)) != ERROR) */
	  return TRUE;
	} /* if ((aggregate_start_idx = is_aggregate_term_a ... */
      }  /* if (M_TOK(messages[prev_tok_idx]) == ARRAYCLOSE) */
    } /* if ((prev_tok_idx = prevlangmsg (messages, method_idx)) != ERROR) */
  } /* if (is_in_rcvr_subexpr (message_stack (), method_idx, */
  return FALSE;
}

/*
 *  Set postfix op attributes in a <cvar><++|-->... <method> expression.
 *
 *  Called by resolve ().  TOK_IS_POSTFIX_OPERATOR and the actual
 *  reciever_msg are used by cvar_is_method_receiver ().
 *
 *  Definitely note however that multiple ++|-- are not valid C statements, 
 *  so the functions in prefixop.c don't support them.
 */
int set_cvar_rcvr_postfix_attrs_a (MESSAGE_STACK messages, 
			       int method_idx,
			       int rcvr_idx,
			       CVAR *rcvr_cvar,
			       char *rcvr_name) {
  int next_tok_idx, i, j;
  if ((M_TOK(messages[method_idx]) == INCREMENT) || 
      (M_TOK(messages[method_idx]) == DECREMENT)) {
    if (!strcmp (M_NAME(messages[rcvr_idx]), rcvr_name)) {
      if ((next_tok_idx = nextlangmsg (messages, method_idx)) != ERROR) {
	/*
	 *  Single ++|--
	 *
	 *  Actually check for a method here, not just a label.
	 */
	if (M_TOK(messages[next_tok_idx]) == LABEL) {
	  messages[method_idx] -> attrs |= TOK_IS_POSTFIX_OPERATOR;
	  messages[method_idx] -> receiver_msg = messages[rcvr_idx];
	  return TRUE;
	}
      } else {
	for (i = method_idx - 1; messages[i]; i--) {
	  if (M_ISSPACE(messages[i])) 
	    continue;
	  if ((M_TOK(messages[i]) == INCREMENT) || 
	      (M_TOK(messages[i]) == DECREMENT))
	    continue;
	  /*
	   *  Multiple ++|--
	   *
	   *  Actually check for a method here, not just a label.
	   *
	   *  Actually not supported by standard C, should generate
	   *  a warning in prefixop.c.
	   */
	  if (M_TOK(messages[i]) == LABEL) {
	    generate_register_c_method_arg_call (rcvr_cvar, 
						 rcvr_name, FRAME_SCOPE,
						 FRAME_START_IDX);
	    for (j = method_idx; j >= i; j--) {
	      if ((M_TOK(messages[j]) == INCREMENT) ||
		  (M_TOK(messages[j]) == DECREMENT)) {
		messages[j] -> attrs |= TOK_IS_POSTFIX_OPERATOR;
	      }
	    }
	    return TRUE;
	  }
	}
      }
    } /* if (!strcmp (M_NAME(messages[rcvr_idx]), rcvr_name)) */
  }  /* if ((M_TOK(messages[method_idx]) == INCREMENT) ||  */
  return FALSE;
}

/*
 *  If the argument of an expression 
 *    <constant_rcvr> <math_op_method> <argument> 
 *
 *  is a C variable, then register the C variable.
 */
int register_cvar_arg_expr_a (MESSAGE_STACK messages,
			      int method_idx) {
  int idx_next_tok;
  int agg_var_end_idx;
  CVAR *c_next_tok;
  if ((idx_next_tok = nextlangmsg (messages, method_idx)) != ERROR) {
    if (((c_next_tok = 
	  get_local_var (M_NAME(messages[idx_next_tok]))) 
	 != NULL) ||
	((c_next_tok = 
	  get_global_var (M_NAME(messages[idx_next_tok]))) 
	 != NULL)) {
      register_c_var (messages[method_idx], messages, idx_next_tok,
		      &agg_var_end_idx);
    }
  }
  return SUCCESS;
}

/*
 *  Unsuported cases so far - either they don't belong in a 
 *  constant expression, or the support still needs to be 
 *  written for that case.
 */
static int const_rcvr_tok_is_unsupported(int tok) {
  if ((tok == OPENBLOCK) ||
      (tok == CLOSEBLOCK) ||
      /*
       *  Checking for ARGSEPARATOR's 
       *  should keep the constant receiver 
       *  parsers from confusing function calls 
       *  in the middle of expressions.
       */
      (tok == ARGSEPARATOR) ||
      (tok == ASTERISK) ||
      (tok == SEMICOLON))
    return TRUE;
  return FALSE;
}

#define CONSTRCVR_GLOBAL_DECL_SCOPE (HAVE_FRAMES && \
			   ((FRAME_SCOPE == GLOBAL_VAR) || \
			    (FRAME_SCOPE == PROTOTYPE_VAR)))

/*
 *  E.g., <close paren> <math_op> expression
 *
 *  Also check for LITERAL tokens, objects, and 
 *  variables that represent aggregate declarations.  Check
 *  here for a function preceding the operator - the
 *  function is_in_rcvr_subexpr () checks for control 
 *  structure keywords.  These as well as everything
 *  else can be handled as C expressions.
 *
 *  It's okay to use P_MESSAGES as the stack start
 *  index here.
 */
int method_call_subexpr_postfix_a (MESSAGE_STACK messages,
				    int method_idx) {
  int preceding_tok_idx, open_paren_idx, close_paren_idx, i;
  MESSAGE *m;
  CVAR *c;
  OBJECT *o;
  int stack_top_idx;
  MSINFO ms;

  if (CONSTRCVR_GLOBAL_DECL_SCOPE)
    return ERROR;
  if (get_struct_decl () == True)
    return ERROR;

  stack_top_idx = get_stack_top (messages);
  ms.messages = messages;
  ms.stack_start = stack_start (messages);
  ms.stack_ptr = stack_top_idx;

  if (is_possible_receiver_subexpr_postfix (&ms, method_idx)) {
    if ((close_paren_idx = prevlangmsg (messages, method_idx)) !=
	ERROR) {
      if ((open_paren_idx = match_paren_rev (messages, close_paren_idx,
					     P_MESSAGES))
	  != ERROR) {
	/*
	 *  Check for a function name preceding the opening
	 *  parenthesis.
	 */
	if ((preceding_tok_idx = prevlangmsg (messages, open_paren_idx))
	    != ERROR) {
	  if (M_TOK(messages[preceding_tok_idx]) == LABEL) {
	    if (get_function (M_NAME(messages[preceding_tok_idx])))
	      return FALSE;
	  }
	}

	/*
	 *  First we do a quick sanity check for things 
	 *  that definitely aren't expressions.
	 */
	for (i = open_paren_idx; i >= close_paren_idx; i--) {
	  m = messages[i];
	  if (M_ISSPACE(m)) continue;
	  if (const_rcvr_tok_is_unsupported (M_TOK(m)))
	    return FALSE;
	}
	for (i = open_paren_idx; i >= close_paren_idx; i--) {
	  m = messages[i];
	  if (M_TOK(m) == LITERAL) {
	    /*
	     *  This check handles a case where one expression is
	     *  within another.  If we handle an expression like
	     *  this:
	     *
	     *   (("Hello, " + "world ") + "again!") length
	     *
	     *  this case handles the second '+' operator, so check again 
	     *  for a further expression, which then will notice the, 
	     *  "length," message and wait until it gets resolved.
	     */

	    /* Check the correctness and speed of "message_stack ()" here and 
	       below. */
	    if (!is_in_rcvr_subexpr (message_stack (), 
				     method_idx,
				     P_MESSAGES,
				     stack_top_idx)) {
	      handle_subexpr_rcvr (method_idx);
	      return TRUE;
	    }
	  }
	  if (M_TOK(m) == LABEL) {
	    if (((c = get_local_var (M_NAME(m))) != NULL) ||
		((c = get_global_var (M_NAME(m))) != NULL)) {
	      /*
	       *  Only handle if the variable is declared as 
	       *  an aggregate.
	       */
	      if ((c -> attrs & CVAR_ATTR_STRUCT_DECL) ||
		  (c -> attrs & CVAR_ATTR_ARRAY_DECL)) {

		if (!is_in_rcvr_subexpr (message_stack (), 
					 method_idx,
					 P_MESSAGES,
					 stack_top_idx)) {
		  /*
		   *  NOTE NOTE NOTE: Workaround for escaped char constants - 
		   *  especially '\0'.  If inside a control block predicate,
		   *  Handle left-hand operands as C if they don't contain
		   *  objects - in case the compiler interprets a zero 
		   *  character literally and not as a constant when the 
		   *  entire expression is being passed to 
		   *  __ctalkEvalExpr ().
		   */
		  if (ctrlblk_pred) {
		    int i_1;
		    MESSAGE *m_1;
		    for (i_1 = open_paren_idx; i_1 >= close_paren_idx; i_1--) {
		      m_1 = messages[i];
		      if (M_TOK(m_1) == LABEL) {
			if (m_1 -> attrs & TOK_IS_DECLARED_C_VAR) {
			  continue;
			} else if (m_1 -> attrs & TOK_SELF ||
				   m_1 -> attrs & TOK_SUPER ||
				   get_object (M_NAME(m_1), NULL)) {
			  handle_subexpr_rcvr (method_idx);
			}
		      }
		    }
		  } else {
		    handle_subexpr_rcvr (method_idx);
		    return TRUE;
		  }
		}
	      }
	    } else {
	      if (((o = get_local_object (M_NAME(m), NULL)) != NULL) ||
		  ((o = get_global_object (M_NAME(m), NULL)) != NULL)) {
		if (!is_in_rcvr_subexpr (message_stack (), 
					 method_idx,
					 P_MESSAGES,
					 stack_top_idx)) {
		  handle_subexpr_rcvr (method_idx);
		  return TRUE;
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return FALSE;
}

/*
 *  A (<aggregate_cvar> <op> <aggregate_cvar|object> ) <postfix_method>
 *  expression *from the <op> method.  
 *
 *  postfix_method := <math_op|method> 
 *
 *  Needs support for nested parentheses, and sequences like this:
 *
 *    aggregate_rcvr_expr := 
 *      (<aggregate_cvar> <op> <aggregate_cvar|object> ) 
 *      | (aggregate_cvar_expr <op> <aggregate_cvar|object>)
 *
 *  Returns -2 if it encounters a case that isn't supported yet.
 */
int method_call_subexpr_postfix_b (MESSAGE_STACK messages,
				    int method_idx) {
  int preceding_tok_idx, open_paren_idx, close_paren_idx, i,
    op_method_idx;
  int stack_end;
  int need_subexpr_rcvr_call;
  MESSAGE *m;
  CVAR *c;
  OBJECT *o;
  OBJECT_CONTEXT expr_context;

  if (CONSTRCVR_GLOBAL_DECL_SCOPE)
    return ERROR;
  if (get_struct_decl () == True)
    return ERROR;

  stack_end = get_stack_top (messages);

  open_paren_idx = close_paren_idx = -1;
  need_subexpr_rcvr_call = FALSE;
  for (i = method_idx; (i <= P_MESSAGES) && (open_paren_idx == -1); 
       i++) {
    switch (M_TOK(messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case OPENPAREN:
	open_paren_idx = i;
	break;
      case ARGSEPARATOR:
      case SEMICOLON:
      case OPENBLOCK:
      case CLOSEBLOCK:
	goto mcsp_done_1;
	break;
      }
  }
 mcsp_done_1:

  for (i = method_idx; (i > stack_end) && (close_paren_idx == -1); 
       i--) {
    m = messages[i];
    if (M_ISSPACE(m))
      continue;
    if (M_TOK(m) == CLOSEPAREN)
      close_paren_idx = i;
    if ((M_TOK(m) == ARGSEPARATOR) || 
	(M_TOK(m) == SEMICOLON) ||
	(M_TOK(m) == OPENBLOCK) ||
	(M_TOK(m) == CLOSEBLOCK))
      break;
  }

  if (open_paren_idx == -1 || close_paren_idx == -1)
    return ERROR;
  if ((op_method_idx = nextlangmsg (messages, close_paren_idx)) == ERROR)
    return ERROR;
  if (!IS_C_OP_TOKEN_NOEVAL(M_TOK(messages[op_method_idx]))) {
    if (!is_in_rcvr_subexpr_obj_check (messages, method_idx,
				       stack_end)) {
      return ERROR;
    }
  }
  /* 
   * Once again, rule out a C control strucures and 
   * argument expressions.  
   * TODO - Check if these operators are actually overloaded - 
   * unlikely, but possible.
   */
  if ((M_TOK(messages[op_method_idx]) == OPENBLOCK) ||
      (M_TOK(messages[op_method_idx]) == COLON) ||
      (M_TOK(messages[op_method_idx]) == SEMICOLON) ||
      (M_TOK(messages[op_method_idx]) == ARGSEPARATOR))
    return ERROR;

  if ((preceding_tok_idx = prevlangmsg (messages, open_paren_idx))
      != ERROR) {
    if (M_TOK(messages[preceding_tok_idx]) == LABEL) {
      if (get_function (M_NAME(messages[preceding_tok_idx])))
	return FALSE;
    }
  }

  /*
   *  First we do a quick sanity check for things 
   *  that definitely aren't expressions.
   */
  for (i = open_paren_idx; i >= close_paren_idx; i--) {
    m = messages[i];
    if (M_ISSPACE(m)) continue;
    if (const_rcvr_tok_is_unsupported (M_TOK(m)))
      /* If we return -2, then resolve () can generate a
	 somewhat useless warning, so just return. */
      /*      return -2; */
      return -1;
  }
  expr_context = object_context (messages, op_method_idx);
  for (i = open_paren_idx; i >= close_paren_idx; i--) {
    m = messages[i];
    if (M_TOK(m) == LITERAL) {
      ++need_subexpr_rcvr_call;
    }
    if (M_TOK(m) == LABEL) {
      if (((c = get_local_var (M_NAME(m))) != NULL) ||
	  ((c = get_global_var (M_NAME(m))) != NULL)) {
	/*
	 *  Only handle if the variable is declared as 
	 *  an aggregate.
	 */
	if ((c -> attrs & CVAR_ATTR_STRUCT_DECL) ||
	    (c -> type_attrs & CVAR_ATTR_STRUCT_TAG) ||
	    (c -> type_attrs & CVAR_ATTR_STRUCT_PTR_TAG)) {
	  /*
	   *  This should be sufficient to prevent interpreting an
	   *  actual -> or . operator as an overloaded method.  Of
	   *  course, if it isn't sufficient, we can just find the
	   *  CVAR for the struct member and output a possibly
	   *  unecessary CVAR registration call anyway.  See
	   *  cvar_is_method_receiver () in resolve.c for examples of
	   *  how to do this.
	   */
	  if ((M_TOK(messages[method_idx]) == PERIOD) ||
	      (M_TOK(messages[method_idx]) == DEREF)) {
	    return FALSE;
	  } else {
	    if ((expr_context == c_argument_context) ||
		(expr_context == c_context)) {
	      register_cvar_rcvr_expr_a 
		(messages, method_idx, c, M_NAME(m));
	    }
	    ++need_subexpr_rcvr_call;
	  }
	} else {
	  if (c -> attrs & CVAR_ATTR_ARRAY_DECL) {
	    if ((expr_context == c_argument_context) ||
		(expr_context == c_context)) {
	      register_cvar_rcvr_expr_a 
		(messages, method_idx, c, M_NAME(m));
	    }
	    ++need_subexpr_rcvr_call;
	  }
	}
      } else {
	if (((o = get_local_object (M_NAME(m), NULL)) != NULL) ||
	    ((o = get_global_object (M_NAME(m), NULL)) != NULL)) {
	  ++need_subexpr_rcvr_call;
	}
      }
    }
  }
  if (need_subexpr_rcvr_call) {
    handle_subexpr_rcvr (op_method_idx);
    return TRUE;
  }
  return FALSE;
}

void handle_subexpr_rcvr (int actual_method_idx) {
  OBJECT_CONTEXT subexpr_rcvr_context;
  subexpr_rcvr_context = 
    object_context (message_stack (), actual_method_idx);
  switch (subexpr_rcvr_context)
    {
    default:
      if (have_subexpr_rcvr_class_postfix (message_stack (),
					   actual_method_idx)) {
	/*
	 *  Same as above: this check handles a case where one expression 
	 *  is within another.  If we handle an expression like this:
	 *
	 *   (("Hello, " + "world ") + "again!") length
	 *
	 *  this case handles the second '+' operator, so check again 
	 *  for a further expression, which then will notice the, 
	 *  "length," message and wait until it gets resolved.
	 */
	if (!is_in_rcvr_subexpr (message_stack (), 
				 actual_method_idx,
				 P_MESSAGES,
				 get_stack_top (message_stack ()))) {
	  method_call (actual_method_idx); 
	}
      }
      break;
    }
}

/*
 *  Returns the class object of a receiver expression, which is all
 *  that should be needed in most cases to find the method and
 *  generate a __ctalkEvalExpr call later on.  Only for parenthesized
 *  receiver expressions - any other complex receiver expression
 *  should already be covered. If successful, the method message's
 *  receiver_msg points to the *start* (i.e., opening parenthesis) of
 *  the receiver expression.
 *
 *  Mainly for receiver expressions that contain only constants - 
 *  resolve () doesn't handle constant objects on its own, so it 
 *  waits until it sees a ") <label>" construct.
 */
int have_subexpr_rcvr_class_postfix (MESSAGE_STACK messages, 
				     int method_msg_ptr) {
  int prev_tok_idx, open_paren_idx;
  OBJECT *result_class;
  METHOD *method;
  MESSAGE *m_msg;
  m_msg = messages[method_msg_ptr];
  if ((prev_tok_idx = prevlangmsg (messages, method_msg_ptr)) != ERROR) {
    if ((open_paren_idx = match_paren_rev (messages, prev_tok_idx,
					   P_MESSAGES))
	!= ERROR) {
      result_class = const_expr_class (messages, open_paren_idx,
					 prev_tok_idx);
      if (((method = get_instance_method (m_msg,
					  result_class,
					  M_NAME(m_msg),
					  ERROR,
					  FALSE)) != NULL) ||
	  ((method = get_class_method (m_msg,
				       result_class,
				       M_NAME(m_msg),
				       ERROR, FALSE)) != NULL)) {
	m_msg -> attrs = RCVR_OBJ_IS_SUBEXPR;
	m_msg -> receiver_obj = result_class;
	m_msg -> receiver_msg = messages[open_paren_idx];
	m_msg -> tokentype = METHODMSGLABEL;
	return TRUE;
      }
    }
  }
  return FALSE;
}

void constrcvr_unhandled_case_warning (MESSAGE_STACK messages, 
				       int method_idx) {
  warning (messages[method_idx],
	   "Unsupported operands to %s in expression.",
	   M_NAME(messages[method_idx]));
}

/*
 *  CVARs normally get skipped over in resolve, but in the case of 
 *  and expression like the following:
 *
 *   ((<rcvr> <method> <arg>) <method> <arg>) <method>
 *
 *  the second <arg>, if a CVAR, does not get evaluated independently,
 *  so check for it here.
 */
int handle_cvar_arg_before_terminal_method_a (MESSAGE_STACK messages, 
					      int cvar_tok_idx) {
  int prev_tok_idx, next_tok_idx, next_tok_idx_2, i,
    typecast_start, tc_end_t;
  int stack_top_idx, stack_start_idx;
  int agg_var_end_idx;
  CVAR *c;

  stack_top_idx = get_stack_top (messages);
  stack_start_idx = stack_start (messages);

  if ((next_tok_idx = nextlangmsg (messages, cvar_tok_idx)) != ERROR) {
    /*
     *  Handle an aggregate term here.  Handle single-token terms 
     *  below.  Anything else gets handled elsewhere.
     */
    if ((M_TOK(messages[next_tok_idx]) == ARRAYOPEN) ||
	(M_TOK(messages[next_tok_idx]) == PERIOD) ||
	(M_TOK(messages[next_tok_idx]) == DEREF)) {
      if ((prev_tok_idx = prevlangmsg (messages, cvar_tok_idx)) != ERROR) {
	if (M_TOK(messages[prev_tok_idx]) == OPENPAREN) {
	  /* skip intervening parens and look for a preceding typecast */
	  for (i = prev_tok_idx; i <= stack_start_idx; ++i) {
	    if (M_TOK(messages[i]) != OPENPAREN)
	      break;
	  }
	  if (M_TOK(messages[i]) == CLOSEPAREN) {
	    if ((typecast_start = match_paren_rev (messages, i,
						   stack_start_idx))
		!= ERROR) {
	      if (is_typecast_expr (messages, typecast_start,  &tc_end_t)) {
		return FALSE;
	      }
	    }
	  }
	}

	if (is_in_rcvr_subexpr (messages, prev_tok_idx, 
				P_MESSAGES,
				stack_top_idx)) {
	  if ((next_tok_idx_2 = nextlangmsg (messages, next_tok_idx)) != ERROR) {
	    if (!(messages[cvar_tok_idx] -> attrs &
		  TOK_CVAR_REGISTRY_IS_OUTPUT)) {
	      if (((c = get_local_var (M_NAME(messages[cvar_tok_idx]))) 
		   != NULL) ||
		  ((c = get_global_var (M_NAME(messages[cvar_tok_idx])))
		   != NULL)) {
		/* And here - at cvar_tok_idx. */
		register_c_var (messages[cvar_tok_idx], 
				messages, cvar_tok_idx,
				&agg_var_end_idx);
		messages[cvar_tok_idx] -> attrs |=
		  TOK_CVAR_REGISTRY_IS_OUTPUT;
	      }
	    }
	  }
	}
      }
    } else {
      if (M_TOK(messages[next_tok_idx]) != CLOSEPAREN)
	return ERROR;
    }
  }

  if ((prev_tok_idx = prevlangmsg (messages, cvar_tok_idx)) != ERROR) {
    if (is_in_rcvr_subexpr (messages, prev_tok_idx, 
			    P_MESSAGES,
			    stack_top_idx)) {
      if ((next_tok_idx_2 = nextlangmsg (messages, next_tok_idx)) != ERROR) {
	if (!is_in_rcvr_subexpr (messages, next_tok_idx_2,
				 P_MESSAGES,
				 stack_top_idx)) {
	  if (((c = get_local_var (M_NAME(messages[cvar_tok_idx]))) 
	     != NULL) ||
	    ((c = get_global_var (M_NAME(messages[cvar_tok_idx])))
	     != NULL)) {
	    if (!(messages[cvar_tok_idx] -> attrs &
		  TOK_CVAR_REGISTRY_IS_OUTPUT)) {
	      generate_register_c_method_arg_call 
		(c, M_NAME(messages[cvar_tok_idx]),
		 FRAME_SCOPE, FRAME_START_IDX);
	      messages[cvar_tok_idx] -> attrs
		|= TOK_CVAR_REGISTRY_IS_OUTPUT;
	    }
	    return SUCCESS;
	  }
	}
      }
    }
  }
  return ERROR;
}

/*
 *  Returns the first token index of an aggregate expression, or 
 *  FALSE; 
 */
int is_aggregate_term_a (MESSAGE_STACK messages, int last_tok_idx) {
  int i, n_brackets, prev_label_idx;
  if (M_TOK(messages[last_tok_idx]) == ARRAYCLOSE) {
    n_brackets = 0;
    for (i = last_tok_idx; i <= P_MESSAGES; i++) {
      if (M_ISSPACE(messages[i]))
	continue;
      if (M_TOK(messages[i]) == ARRAYCLOSE)
	--n_brackets;
      if (M_TOK(messages[i]) == ARRAYOPEN) 
	++n_brackets;
      if (!n_brackets) {
	if ((prev_label_idx = prevlangmsg (messages, i)) != ERROR)
	  return prev_label_idx;
      }
    }
  }
  return FALSE;
}

/*
 *  Return the last token index of an aggregate expression, or 
 *  FALSE;
 */
int is_aggregate_term_b (MESSAGE_STACK messages, int first_tok_idx) {
  int i, n_brackets, next_tok_idx; 
  if ((next_tok_idx = nextlangmsg (messages, first_tok_idx)) != ERROR) {
    if (M_TOK(messages[next_tok_idx]) == ARRAYOPEN) {
      n_brackets = 0;
      for (i = next_tok_idx; i > P_MESSAGES; i--) {
	if (M_ISSPACE(messages[i]))
	  continue;
	if (M_TOK(messages[i]) == ARRAYOPEN) 
	  ++n_brackets;
	if (M_TOK(messages[i]) == ARRAYCLOSE)
	  --n_brackets;
	if (!n_brackets) {
	  return i;
	}
      }
    }
  }
  return FALSE;
}
