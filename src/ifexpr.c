/* $Id: ifexpr.c,v 1.1.1.1 2020/07/26 05:50:11 rkiesling Exp $ */

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
 *  Functions to parse if predicates.  While loop predicates
 *  work similarly, so these functions work for while loops also.
 *
 *  Does *not* work with for loop clauses, which need more than
 *  constant expression evaluation, or predicate expressions that
 *  contain C functions, because the compiler uses method_call and
 *  the functions it calls - method_args, eval_arg, etc.,... to
 *  determine if there is a template that should be substituted for
 *  the function call.
 *
 *  Expression Classes
 *
 *  rt_mixed_c_ctalk - Mixed C/Ctalk terms separated by math
 *  operators, but without C function calls (the compiler checks
 *  for templates elsewhere when evaluating these expressions).  The
 *  compiler translates the Ctalk terms, and passes the C terms
 *  directly to the output.  That also means that these expressions
 *  must evaluate to a valid C type; e.g., the term cannot evaluate
 *  to a string literal or another type that can't appear on either
 *  side of a math operator.  This is especially important when assigning
 *  to a C variable within a control block expression, and it also saves
 *  a lot of processing whenever C variables appear in a control structure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "ctrlblk.h"

extern CTRLBLK *ctrlblks[MAXARGS + 1];  /* Declared in control.c. */
extern int ctrlblk_ptr;

extern bool ctrlblk_pred,            /* Global states.                    */
  ctrlblk_blk,
  ctrlblk_else_blk;

extern RT_EXPR_CLASS rt_expr_class;

extern int warn_unresolved_self_opt;  /* Declared in main.c. */

extern bool argblk;          /* Declared in argblk.c.                   */

extern OBJECT *rcvr_class_obj; /* Declared in primitives.c */

extern DEFAULTCLASSCACHE *ct_defclasses; /* Declared in cclasses.c.       */

bool have_major_booleans = false;

extern I_PASS interpreter_pass;    /* Declared in rtinfo.c.               */

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;

bool cpre_have_cvar_reg = false;
  
static bool ctblk_is_major_op_tok (MESSAGE *m) {
  /* Check the token directly in case the tokentype got
     changed to METHODMSGLABEL. */
  if (((m -> name[0] == '|') && (m -> name[1] == '|')) ||
      ((m -> name[0] == '&') && (m -> name[1] == '&')))
    return true;
  else
    return false;
}

void ctblk_handle_rt_bool_multiple_subexprs_parens (MESSAGE_STACK messages,
					     int pred_start_idx) {
  char expr_out[MAXMSG];
  fileout (handle_ctrlblk_subexprs (messages, expr_out),
	   FALSE, pred_start_idx);
}

void ctblk_handle_rt_bool_multiple_subexprs_no_parens 
(MESSAGE_STACK messages, int pred_start_idx, int pred_end_idx) {
  int i;
  fileout (ctblk_eval_major_bool_subexprs (messages, pred_start_idx,
					   pred_end_idx),
	   FALSE, pred_start_idx);
  for (i = pred_start_idx; i >= pred_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
}



/* this is for a subexpression. */
char *ctblk_handle_mixed_c_ctalk_subexpr (MESSAGE_STACK messages,
					      int pred_start_idx,
					      int obj_operand_idx,
					       int pred_end_idx,
					       char *expr_buf_out) {
  int i;
  char expr_buf_tmp[MAXMSG];

  toks2str (messages, pred_start_idx, obj_operand_idx + 1, expr_buf_out);
  strcatx2 (expr_buf_out, fmt_default_ctrlblk_expr 
	    (messages, obj_operand_idx, pred_end_idx, TRUE, expr_buf_tmp), NULL);
  for (i = pred_start_idx; i >= pred_end_idx; --i) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
  return expr_buf_out;
}

/* 
   Only called by resolve during expr_check from check_constant_expr. 
   If we find a constant expression with || or && without parens,
   then we parse the expression after retokenizing and inserting parens
   for each subexpr.
   NOTE: This only works if the expression has no internal parens. 
*/
void check_major_boolean_parens (MESSAGE_STACK messages, int idx) {
  PARSER *p;
  int i;
  int n_parens;
  p = pop_parser ();
  push_parser (p);
  /* during expr_check, this should be the last frame on the frame
     stack. */
  i = frame_at (p -> top_frame) -> message_frame_top;
  n_parens = 0;
  while (IS_MESSAGE(messages[i])) {
    switch (M_TOK(messages[i]))
      {
      case OPENPAREN:
	++n_parens;
	break;
      case BOOLEAN_AND:
      case BOOLEAN_OR:
	if (n_parens == 0) {
	  have_major_booleans = true;
	  return;
	}
      }
    --i;
  }
  have_major_booleans = false;
}

static void check_leading_class_cast (MESSAGE_STACK messages,
				      int *pred_start_idx, 
				      int pred_end_idx) {
  int i;
  int cast_open_paren_idx, cast_close_paren_idx;
  for (i = *pred_start_idx; i >= pred_end_idx; --i) {
    /* The attribute should be set on the class name token.
       Then the previous token is the opening paren of the
       cast expression. */
    if (messages[i] -> attrs & TOK_IS_CLASS_TYPECAST) {
      if ((cast_open_paren_idx = prevlangmsg (messages, i)) != ERROR) {
	if (M_TOK(messages[cast_open_paren_idx]) == OPENPAREN) {
	  if ((cast_close_paren_idx = match_paren 
	       (messages, cast_open_paren_idx, pred_end_idx)) != ERROR) {
	    if (M_TOK(messages[cast_close_paren_idx]) == CLOSEPAREN) {
	      *pred_start_idx = nextlangmsg (messages, cast_close_paren_idx);
	    }
	  }
	}
      }
    }
  }
}

static void c_fn_self_if_expr_a (MESSAGE_STACK messages,
				 int fn_idx, int self_idx,
				 int pred_start_idx,
				 int pred_end_idx,
				 char *expr_buf) {
  /* 
     this is a case like 
     getmonth () == self month value
  */

  int rt_fn_start_paren, rt_fn_end_paren;
  int op_ptr, op_next_ptr, i_2;
  char _t[MAXMSG];

  if ((rt_fn_start_paren = nextlangmsg (messages, fn_idx))
		!= ERROR) {
    if (M_TOK(messages[rt_fn_start_paren]) != OPENPAREN) {
      warning (messages[self_idx],
	       "Argument syntax not supported:\n\n\t%s\n\n",
	       expr_buf);
    } else {
      if ((rt_fn_end_paren =
	   match_paren (messages,
			rt_fn_start_paren,
			get_stack_top (messages)))
	  != ERROR) {
	if (M_TOK(messages[rt_fn_end_paren]) != CLOSEPAREN) {
	  warning (messages[self_idx],
		   "Argument syntax not supported:\n\n\t%s\n\n",
		   expr_buf);
	}
	op_ptr = nextlangmsg (messages, rt_fn_end_paren);
	if (IS_C_OP(M_TOK(messages[op_ptr]))) {
	  if ((op_next_ptr = nextlangmsg
	       (messages, op_ptr)) == self_idx) {
	    for (i_2 = pred_start_idx; i_2 > self_idx; --i_2) {
	      fileout (M_NAME(messages[i_2]), 0, i_2);
	      ++messages[i_2] -> evaled;
	      ++messages[i_2] -> output;
	    }
	    fileout 
	      (fmt_default_ctrlblk_expr 
	       (messages, self_idx,
		pred_end_idx, 
		(((messages[self_idx]-> attrs & TOK_IS_METHOD_ARG)|| 
		  (messages[self_idx] -> receiver_obj && 
		   get_local_object 
		   (messages[self_idx] -> receiver_obj -> __o_name,
		    messages[self_idx] -> receiver_obj -> __o_classname)) ||
		  messages[self_idx] -> attrs & TOK_SELF) ?
		 TRUE:FALSE), _t), 
	       FALSE,
	       self_idx);
	  }
	}
      }
    }
  }
}

/* The parameter, "op_prev_idx" points to the token just before
   the major logical operator.  Returns true if there are
   only labels and whitespace between tokens at self_idx and
   op_prev_idx. */
static bool op_prev_self_label_series (MESSAGE_STACK messages,
					  int self_idx,
					  int op_prev_idx) {
  int i;
  for (i = self_idx; i <= op_prev_idx; --i) {
    if (M_ISSPACE(messages[i]))
      continue;
    if (M_TOK(messages[i]) != LABEL)
      return false;
  }
  return true;
}

static void c_fn_self_if_expr_b (MESSAGE_STACK messages,
				 int fn_idx, int self_idx,
				 int pred_start_idx,
				 int pred_end_idx,
				 char *expr_buf) {
  /* 
     this is a case like 
     if (self month value == getmonth ()) ...
  */

  int op_ptr, op_prev_ptr, i;
  char _t[MAXMSG];

  if ((op_ptr = prevlangmsg (messages, fn_idx)) != ERROR) {
    if (IS_C_OP(M_TOK(messages[op_ptr]))) {
      if ((op_prev_ptr = prevlangmsg (messages, op_ptr)) != ERROR) {
	if (op_prev_self_label_series (messages, self_idx, op_prev_ptr)) {
	  fileout 
	    (fmt_default_ctrlblk_expr 
	     (messages, self_idx, op_prev_ptr,
	      (((messages[self_idx]-> attrs & TOK_IS_METHOD_ARG)|| 
		(messages[self_idx] -> receiver_obj && 
		 get_local_object 
		 (messages[self_idx] -> receiver_obj -> __o_name,
		  messages[self_idx] -> receiver_obj -> __o_classname))||
		messages[self_idx] -> attrs & TOK_SELF) ?
	       TRUE:FALSE), _t), 
	     FALSE,
	     pred_start_idx);
	} else {
	  fileout 
	    (fmt_default_ctrlblk_expr 
	     (messages, pred_start_idx,
	      self_idx, 
	      (((messages[self_idx]-> attrs & TOK_IS_METHOD_ARG)|| 
		(messages[self_idx] -> receiver_obj && 
		 get_local_object 
		 (messages[self_idx] -> receiver_obj -> __o_name,
		  messages[self_idx] -> receiver_obj -> __o_classname))||
		messages[self_idx] -> attrs & TOK_SELF) ?
	       TRUE:FALSE), _t), 
	     FALSE,
	     pred_start_idx);
	}
      }
      for (i = op_prev_ptr - 1; i >= pred_end_idx; i--) {
	fileout (M_NAME(messages[i]), 0, i);
      }
      for (i = pred_start_idx; i >= pred_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
    }
  }
}

static int n_CVARS (MESSAGE_STACK messages, int start_idx, int end_idx) {
  int n_cvars, i;
  n_cvars = 0;
  for (i = start_idx; i >= end_idx; --i) {
    if (M_TOK(messages[i]) == LABEL) {
      if (get_local_var (M_NAME(messages[i])) ||
	  get_global_var (M_NAME(messages[i]))) {
	++n_cvars;
      }
    }
  }
  return n_cvars;
}

/* This is similar to ctrlblk_pred_rt_expr (), except that we call
   this from resolve () while pointing at the "self," receiver token. 
   That means that the method argument(s) is/are not yet evaluated
   (e.g., template expansions), and we have to do them ourselves. */
int ctrlblk_pred_rt_expr_self (MESSAGE_STACK messages, int self_ptr) {

  int i, fn_idx = -1, op_idx = -1;
  int rt_fn_start_paren, rt_fn_end_paren;
  char expr_buf[MAXMSG], expr_buf_tmp[MAXMSG];
  char _t[MAXMSG], *e;
  MESSAGE *m = NULL;   /* Avoid a warning. */
  int n_parens;
  int pred_start_idx, pred_end_idx;
  int rt_prefix_rexpr_idx = self_ptr;  /* Avoid a warning. */
  int agg_var_end_idx, n_cvars;
  int idx_end_ret, obj_operand_idx = -1;
  OBJECT_CONTEXT fn_cx;
  METHOD *op_method;
  CVAR *c;

  if (expr_paren_check (messages, self_ptr)) {
    (void) __ctalkExceptionInternal (messages[self_ptr], mismatched_paren_x, NULL,0);
    return ERROR;
  }

  switch (C_CTRL_BLK -> stmt_type)
    {
    case stmt_switch:
    case stmt_if:
    case stmt_do:
    case stmt_while:
      *expr_buf = 0;
      rt_expr_class = rt_expr_null;

      /* 
	 This is a fixup from the if_stmt settings, see
	 the leading_unary_op () call in if_stmt (), above.
	 In resolve(), we checked whether the operator is overloaded
	 by a method, in which case it has the TOK_IS_PREFIX_OPERATOR
	 attribute set.
      */
      m = message_stack_at (C_CTRL_BLK -> pred_start_ptr);
      if (m -> attrs & TOK_IS_PREFIX_OPERATOR) {
	pred_start_idx = C_CTRL_BLK -> pred_start_ptr;
      } else {
	pred_start_idx = C_CTRL_BLK -> pred_start_ptr - 1;
      }
      pred_end_idx = C_CTRL_BLK -> pred_end_ptr + 1;

      check_leading_class_cast (message_stack (), &pred_start_idx,
				pred_end_idx);

      (void)check_constant_expr (message_stack (), pred_start_idx,
			   pred_end_idx);

      /*
       *  This is transitional - we'd like to use ctrlblk_pred_rt_expr
       *  for everything. 
       */
      if ((n_cvars = n_CVARS (messages, pred_start_idx, pred_end_idx)) > 1)
	return ctrlblk_pred_rt_expr (messages, self_ptr);

      /* 
	 This could be arranged better - If we're *not* in an argument block,
	 then we use register_c_var () as normal, about 700 lines down
	 from here.
      */
      if (argblk)
	register_argblk_c_vars_1 (message_stack (), 
				  pred_start_idx, pred_end_idx);

      n_parens = 0;
      for (i = pred_start_idx; i >= pred_end_idx; i--) {

	m = messages[i];

	if (M_ISSPACE(m)) {
	  strcatx2 (expr_buf, M_NAME(m), NULL);
	  ++m -> evaled; ++m -> output;
	  continue;
	}

	if (IS_C_OP(M_TOK(m))) {
	  (void)operand_type_check (message_stack (), i);
	  strcatx2 (expr_buf, m -> name, NULL);

	  /* This is for an operator that self as an lvalue is the
	     receiver of...  normally the first op (second token) in
	     an expression.  Should only be needed if we have a fn
	     template. */
	  if (IS_C_ASSIGNMENT_OP(M_TOK(m)) && op_idx == -1) {
	    op_idx = i; 
	  }

	} else if (get_function (M_NAME(m))) {
	  if (rt_expr_class == rt_expr_null) {
	    fn_idx = i;
	    switch (fn_cx = object_context (messages, i))
	      {
	      case argument_context:
		if ((op_method = get_instance_method 
		     (messages[i], rcvr_class_obj, M_NAME(messages[op_idx]),
		      ANY_ARGS, TRUE)) != NULL) {
		  /* method_args () needs this. */
		  messages[op_idx] -> receiver_obj = 
		    instantiate_self_object ();
		  method_args (op_method, op_idx);
		  if (IS_ARG(op_method -> args[0]) &&
		      IS_OBJECT(op_method -> args[0] -> obj)) {
		    if (DEFAULTCLASS_CMP(op_method->args[0]->obj,
					 ct_defclasses->p_cfunction_class,
					 CFUNCTION_CLASSNAME)) {
		      if ((e = clib_fn_expr (messages, 
					     i, i, m -> obj)) != NULL) {
			strcatx2 (expr_buf, e, NULL);
			rt_expr_class = rt_expr_fn;
		      } else {
			warning (messages[fn_idx],
				 "Template for function, \"%s,\" "
				 "not found.", 
				 M_NAME(messages[fn_idx]));
			strcatx2 (expr_buf, M_NAME(messages[fn_idx]), NULL);
			rt_expr_class = rt_expr_fn;
		      }
		    }
		    delete_arg_object (op_method -> args[0] -> obj);
		  }
		  delete_object (messages[op_idx] -> receiver_obj);
		  messages[op_idx] -> receiver_obj = NULL;
		}
		rt_expr_class = rt_expr_fn;
		break;
	      case c_context:   /* c_context is what we get if the fn
				   is the first token in the expression. 
				   cover everything else here, too for 
				   now.*/
	      default:
		rt_expr_class = rt_c_fn;
		strcatx2 (expr_buf, m -> name, NULL);
		continue;
		break;
	      }
	  } else {
	    fn_idx = i;
	    strcatx2 (expr_buf, m -> name, NULL);
	    continue;
	  }
	}  else {
	  strcatx2 (expr_buf, m -> name, NULL);
	}

	/*
	 *  If the expression also contains C variables or functions,
	 *  they get registered now in register_c_var () called by
	 *  method_args (), etc., so check here if the token has
	 *  been evaluated and register the C variable here if 
	 *  necessary.
	 */
 	if (m -> tokentype == LABEL) {
	  if (is_mcct_expr (messages, i,
						&idx_end_ret,
						&obj_operand_idx)) {
	    rt_expr_class = rt_mixed_c_ctalk;
	    i = idx_end_ret;
	    continue;
	  }

	  if (!m -> evaled) {
	    /* If we *are* in an argument block, then we've already
	       called register_argblk_c_vars_1 (), near the top of
	       the fn. */
	    if (!argblk && 
		(((c = get_local_var (m -> name)) != NULL) ||
		 ((c = get_global_var_not_shadowed (m -> name)) != NULL))) {
	      (void)register_c_var (m, message_stack (), i,
				    &agg_var_end_idx);
	    } else if (m -> attrs & TOK_SELF) {
	      int self_lookahead;
	      if ((self_lookahead =
		   nextlangmsg (messages, i)) != ERROR) {
		if (M_TOK(messages[self_lookahead]) == LABEL) {
		  if (self_class_or_instance_variable_lookahead_2 
		      (messages, i)) {
		    rt_expr_class = rt_self_instvar_simple_expr;
		  } else if (!get_instance_method
			     (messages[self_lookahead],
			      rcvr_class_obj,
			      M_NAME(messages[self_lookahead]),
			      ANY_ARGS, FALSE) &&
			     !get_class_method
			     (messages[self_lookahead],
			      rcvr_class_obj,
			      M_NAME(messages[self_lookahead]),
			      ANY_ARGS, FALSE) &&
			     !is_method_parameter
			     (messages, self_lookahead) &&
			     !is_method_proto
			     (rcvr_class_obj,
			      M_NAME(messages[self_lookahead])) &&
			     !have_user_prototype
			     (rcvr_class_obj -> __o_name,
			      M_NAME(messages[self_lookahead]))) {
		    if (argblk) {
		      if (warn_unresolved_self_opt) {
			warning (messages[self_lookahead],
				 "Unresolved label, \"%s.\" "
				 "Waiting until run time to evaluate the "
				 "expression.",
				 M_NAME(messages[self_lookahead]));
		      }
		    } else {
		      warning (messages[self_lookahead],
			       "Undefined label, \"%s.\"",
			       M_NAME(messages[self_lookahead]));
		    }
		  }
		} /* if (M_TOK(messages[self_lookahead]) == LABEL) */
	      } /* if ((self_lookahead = ... */
	    }
	  }
	}


	switch (M_TOK(m))
	  {
	  case OPENPAREN:
	    ++n_parens;
	    break;
	  case CLOSEPAREN:
	    --n_parens;
	    break;
	  case BOOLEAN_AND:
	  case BOOLEAN_OR:
	    if (n_parens == 0) {
	      rt_expr_class = rt_bool_multiple_subexprs;
	    }
	  }

	if (m -> attrs & TOK_IS_PREFIX_OPERATOR) {
	  int p_prev, p_lookback, p_next;
	  bool have_lval_object = false;
	  if ((p_prev = prevlangmsg (messages, i)) != ERROR) {
	    if (IS_C_BINARY_MATH_OP(M_TOK(messages[p_prev]))) {
	      for (p_lookback = p_prev; 
		   (p_lookback <= pred_start_idx) && !have_lval_object;
		   p_lookback++) {
		if (messages[p_lookback] -> obj)
		  have_lval_object = true;
	      }
	      if ((!have_lval_object) && 
		  (M_TOK(messages[i]) != M_TOK(messages[p_prev]))) {
		/*  See the next clause, which handles the first of
		    several repeated ops. */
		rt_expr_class = rt_prefix_rexpr;
		rt_prefix_rexpr_idx = i;
	      }
	    } else {
	      /* For cases like if (**<expr>) ... */
	      if ((p_next = nextlangmsg (messages, i))  != ERROR) {
		if (M_TOK(messages[p_next]) == M_TOK(messages[i])) {
		  rt_prefix_rexpr_idx = i;
		}
	      }
	    }
	  }
	}
	

	++m -> evaled;
	++m -> output;
      }
      
      switch (rt_expr_class) 
	{
	case rt_mixed_c_ctalk:
	  if (obj_operand_idx != -1) {
	    handle_mcct_expr (messages, pred_start_idx, pred_end_idx);
	  }
	  break;
	case rt_expr_fn:
	case rt_expr_user_fn:
	  fileout (fmt_user_fn_rt_expr (messages, 
					pred_start_idx, pred_end_idx,
					expr_buf, _t), 
		   FALSE, 
		   pred_end_idx);
	  break;
	case rt_c_fn:
	  /*
	   *  If there are any tokens in the predicate after the 
	   *  function, output them here.
	   *
	   *  Also check for further functions in the expression.
	   *
	   *  This should not be needed because it isn't valid in
	   *  an expression that uses "self".
	   */
	  fmt_c_fn_obj_args_expr (messages, fn_idx, expr_buf);
	  rt_fn_start_paren = nextlangmsg (messages, fn_idx);
	  rt_fn_end_paren = match_paren (messages, rt_fn_start_paren,
					 get_stack_top (messages));
	  /*
	   *  If the function is not the first token in a control
	   *  predicate, prepend the preceding tokens.
	   */
	  if (ctrlblk_pred) {
	    int __pred_expr_start_idx;
	    __pred_expr_start_idx = 
	      nextlangmsg (messages, pred_start_idx);
	    if (__pred_expr_start_idx > fn_idx) {
	      
	      /* The tokens before the function name. */
	      toks2str (messages, __pred_expr_start_idx, fn_idx + 1,
			expr_buf_tmp);
	      /*  the partial bufs come out of order - clean this up? */
	      strcatx (_t, expr_buf_tmp, expr_buf, NULL);
	      strcpy (expr_buf, _t);
	    }
	  }

	  for (i = rt_fn_end_paren - 1; 
	       i > pred_end_idx; i--) {
	    if (get_function (M_NAME(m))) {
	      strcatx2 (expr_buf, fmt_c_fn_obj_args_expr (messages, i,
							  expr_buf_tmp), NULL);
	      i = match_paren (messages, nextlangmsg (messages, i),
					 get_stack_top (messages));
	    } else {
	      strcatx2 (expr_buf, M_NAME(messages[i]), NULL);
	    }
	  }
	  fileout (expr_buf, FALSE, pred_end_idx);
	  break;
	case rt_bool_multiple_subexprs:
	  handle_ctrlblk_subexprs (messages, expr_buf);
	  fileout (expr_buf, FALSE, pred_start_idx);
	  break;
	case rt_prefix_rexpr:
	  handle_rt_prefix_rexpr 
	    (messages, rt_prefix_rexpr_idx,
	     pred_start_idx, pred_end_idx, expr_buf);
	  fileout (expr_buf, FALSE, rt_prefix_rexpr_idx);
	  break;
	case rt_self_instvar_simple_expr:
	default:
	  if (major_logical_op_check (messages, 
				      C_CTRL_BLK->pred_start_ptr,
				      C_CTRL_BLK->pred_end_ptr)) {
	    /* Then we also need to include a leading unary op
	       if there is one, instead of leaving it to the
	       C compiler. */
	    if (IS_C_UNARY_MATH_OP(M_TOK(messages[C_CTRL_BLK->pred_start_ptr]))){
	      pred_start_idx = C_CTRL_BLK -> pred_start_ptr;
	      ++messages[pred_start_idx] -> evaled;
	      ++messages[pred_start_idx] -> output;
	    }
	  }
	  if (fn_idx == -1) {
	    fileout 
	      (fmt_default_ctrlblk_expr 
	       (messages, pred_start_idx, 
		pred_end_idx, 
		(((messages[self_ptr]-> attrs & TOK_IS_METHOD_ARG)|| 
		  (messages[self_ptr] -> receiver_obj && 
		   get_local_object 
		   (messages[self_ptr] -> receiver_obj -> __o_name,
		    messages[self_ptr] -> receiver_obj -> __o_classname))||
		  messages[self_ptr] -> attrs & TOK_SELF) ?
		 TRUE:FALSE), _t), 
	       FALSE,
	       pred_start_idx);
	  } else if (fn_idx > self_ptr) {
	    c_fn_self_if_expr_a (messages, fn_idx, self_ptr,
				 pred_start_idx, pred_end_idx,
				 expr_buf);
	  } else if (fn_idx < self_ptr) {
	    c_fn_self_if_expr_b (messages, fn_idx, self_ptr,
				 pred_start_idx, pred_end_idx,
				 expr_buf);
	  }
	  break;
	}
      /*
       *  loop_block_start () checks this.
       */
      C_CTRL_BLK -> pred_expr_evaled = TRUE;
      break; /* case stmt_switch, stmt_if, stmt_while */
    case stmt_case:
      /*
       *  Case arguments are still constants as in C99.
       */
      break;
    case stmt_for:
      /*
       *   The function for_stmt () and others handle 
       *   the for loop predicates.
       */
      break;
    case stmt_default:
    default:
      break;
    }
  return SUCCESS;
}

static int bool_op_indexes[MAXARGS];
static int bool_op_index_ptr;
static struct {
  int start, end;
} cond_subexpr_indexes[MAXARGS];
static int subexpr_index_ptr;

static int ifexpr_match_paren_silently (MESSAGE **messages, int this_message, 
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
  return -1;
}

void parse_ctrlblk_subexprs (MESSAGE_STACK messages) {
  int i, i_2, i_3, i_4;
  int pred_start_idx, pred_end_idx;

  /* A simple expression puts some leading unary ops - mainly ! - 
     outside of the Ctalk expression.  Here we need to include it. */
  if (IS_C_UNARY_MATH_OP(M_TOK(messages[C_CTRL_BLK->pred_start_ptr]))) {
    pred_start_idx = C_CTRL_BLK->pred_start_ptr;
  } else {
    if ((pred_start_idx = nextlangmsg (messages, C_CTRL_BLK -> pred_start_ptr))
	== ERROR) {
      warning (messages[C_CTRL_BLK -> pred_start_ptr],
	       "parse_ctrlblk_subexprs: Invalid predicate start.");
    }
  }

  if ((pred_end_idx = prevlangmsg (messages, C_CTRL_BLK -> pred_end_ptr))
      == ERROR) {
    warning (messages[C_CTRL_BLK -> pred_start_ptr],
	     "parse_ctrlblk_subexprs: Invalid predicate end.");
  }

  bool_op_index_ptr = subexpr_index_ptr = 0;

  for (i = pred_start_idx; i >= pred_end_idx; i--) {
    if ((M_TOK(messages[i]) == BOOLEAN_AND) ||
	(M_TOK(messages[i]) == BOOLEAN_OR) ||
	ctblk_is_major_op_tok (messages[i])) {
      bool_op_indexes[bool_op_index_ptr++] = i;
    }
  }

  for (i = 0; i <= bool_op_index_ptr; i++) {
    if (i == 0) {
      cond_subexpr_indexes[subexpr_index_ptr].start = 
	pred_start_idx;
      cond_subexpr_indexes[subexpr_index_ptr].end = 
	bool_op_indexes[i] + 1;
      ++subexpr_index_ptr;
    } else {
      if (i == bool_op_index_ptr) {
	cond_subexpr_indexes[subexpr_index_ptr].start = 
	  bool_op_indexes[i - 1] - 1;
	cond_subexpr_indexes[subexpr_index_ptr].end = 
	  pred_end_idx;
	++subexpr_index_ptr;
      } else {
	cond_subexpr_indexes[subexpr_index_ptr].start = 
	  bool_op_indexes[i-1] - 1;
	cond_subexpr_indexes[subexpr_index_ptr].end = 
	  bool_op_indexes[i] + 1;
	++subexpr_index_ptr;
      }
    }
  }

  /* 
     Look for an expression like this:
     
       if ( (x == a  &&  y == b)  &&  (i > 10))
            | sub1|      |sub2 |      |sub3|

       Then combine the first two subexpressions into one
       so the parentheses match; e.g.,

       if (  (x == a && y == b)  &&  (i > 10)  )
             |-- subexpr 1----|      |sub2  |

       We also compress the stack after combining two of the
       subexpr entries.
   */
  i = 0;
  i_2 = 0;
 ifexpr_next_bool_sub: while (1) {
    if (i >= bool_op_index_ptr)
      break;
    if ((ifexpr_match_paren_silently (messages,
				      cond_subexpr_indexes[i].start,
				      cond_subexpr_indexes[i].end) < 0) &&
	(ifexpr_match_paren_silently (messages,
				      cond_subexpr_indexes[i + 1].start,
				      cond_subexpr_indexes[i].end) < 0)) {
      i_3 = cond_subexpr_indexes[i].start;
      i_4 = cond_subexpr_indexes[i+1].end;
      while (M_ISSPACE(messages[i_3]))
	--i_3;
      while (M_ISSPACE(messages[i_4]))
	++i_4;
      if ((M_TOK(messages[i_3]) == OPENPAREN) &&
	  (M_TOK(messages[i_4]) == CLOSEPAREN)) {
	cond_subexpr_indexes[i].end = cond_subexpr_indexes[i+1].end;

	/* Then we have to shuffle the following expressions
	   down. */
	for (i_2 = i + 1; i_2 <= bool_op_index_ptr; i_2++) {
	  cond_subexpr_indexes[i_2].start =
	    cond_subexpr_indexes[i_2 + 1].start;
	  cond_subexpr_indexes[i_2].end =
	    cond_subexpr_indexes[i_2 + 1].end;
	}
	--bool_op_index_ptr;
	--subexpr_index_ptr;
	++i;
	goto ifexpr_next_bool_sub;
      }
    } else {
      ++i;
    }
  }

}

/*
 *  If a subexpr call returns NULL, it means that the handler output
 *  the expression directly, and we should simply output the || or
 *  && token that follows it, if it's not the last subexpr in the
 *  series.
 */
char *handle_ctrlblk_subexprs (MESSAGE_STACK messages, char *expr_out) {
  int i, j;
  char subexpr[MAXMSG];
  char reg_buf[MAXMSG], expr_call_buf[MAXMSG], reg_call_buf[MAXMSG];

  parse_ctrlblk_subexprs (messages);

  j = 0;
  memset (expr_out, 0, MAXMSG);
  memset (reg_buf, 0, MAXMSG);
  memset (reg_call_buf, 0, MAXMSG);
  memset (subexpr, 0, MAXMSG);
  memset (expr_call_buf, 0, MAXMSG);
  for (i = 0; i < subexpr_index_ptr; i++) {
    ctrlblk_pred_rt_subexpr
      (messages, 
       cond_subexpr_indexes[i].start,
       cond_subexpr_indexes[i].end,
       reg_call_buf, expr_call_buf);
    if (*reg_call_buf)
      strcatx2 (reg_buf, reg_call_buf, NULL);
    strcatx2 (subexpr, expr_call_buf, NULL);
    if (i < (subexpr_index_ptr - 1))
      strcat (subexpr, messages[bool_op_indexes[j++]] -> name);
  }

  if (*reg_buf)
    strcatx (expr_out, reg_buf, subexpr, NULL);
  else
    strcatx (expr_out, subexpr, NULL);
  return expr_out;
}

/* this fixup is (so far) more compact than creating
   a separate version of fmt_default_ctrlblk_expr for
   subexpressions that would otherwise intersperse
   the CVAR registration with the expressions - this
   separates them so they can be collected as groups by the
   caller. */
static void split_cvar_registration (char *expr_buf_in,
				     char *reg_out,
				     char *expr_out) {
  char *c;
  int n_parens;
  if (strstr (expr_buf_in, "__ctalk_register_c_method_arg")) {
    /* find the comma before the opening paren of the
       __ctalkEvalExpr call */
    c = strrchr (expr_buf_in, ')');
    n_parens = 0;
    while (c >= expr_buf_in) {
      if (*c == ')') ++n_parens;
      else if (*c == '(') --n_parens;
      if (n_parens == 0)
	break;
      --c;
    }
    while (*c != ',')
      --c;
    ++c;  /* keep the last comma with the reg function */
    memset (reg_out, 0, MAXMSG);
    memset (expr_out, 0, MAXMSG);
    strncpy (reg_out, expr_buf_in, c - expr_buf_in);
    strcpy (expr_out, c);
  } else {
    strcpy (expr_out, expr_buf_in);
    *reg_out = 0;
  }
}

/*
 *  If a handler outputs the subexpr directly, return NULL immediately
 *  after handling the subexpr.
 */
char *ctrlblk_pred_rt_subexpr (MESSAGE_STACK messages, 
			       int subexpr_start_ptr,
			       int subexpr_end_ptr,
			       char *reg_buf_out,
			       char *expr_buf_out) {

  int i, lookahead, fn_idx = -1;
  int rt_fn_start_paren, rt_fn_end_paren;
  int end_idx_ret, operand_idx_ret = -1;
  int agg_var_end_idx;
  static char expr_buf[MAXMSG], expr_buf_tmp[MAXMSG];
  char _t[MAXMSG];
  MESSAGE *m = NULL;   /* Avoid a warning. */
  int msg_ptr;

  *expr_buf = 0;
  rt_expr_class = rt_expr_null;

  /* 
     This could be arranged better - If we're *not* in an argument block,
     then we use register_c_var () as normal, about 700 lines down
     from here.
  */

  for (i = subexpr_start_ptr; i >= subexpr_end_ptr; i--) {
    m = messages[i];
    if (IS_OBJECT (m -> obj) &&
	DEFAULTCLASS_CMP(m->obj, ct_defclasses -> p_cfunction_class,
			 CFUNCTION_CLASSNAME)) {
      if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	if ((messages[i] -> tokentype == LABEL) && 
	    (messages[lookahead] -> tokentype == OPENPAREN)) {
	  char *e;
	  if ((e = clib_fn_expr (messages, i, i, m -> obj)) != NULL) {
	    strcatx2 (expr_buf, e, NULL);
	    rt_expr_class = rt_expr_fn;
	    i = 
	      match_paren (messages, lookahead, get_stack_top (messages));
	  }
	}
      }
    } else {
      if (IS_C_OP(M_TOK(m))) {
	(void) operand_type_check (message_stack (), i);
	strcatx2 (expr_buf, m -> name, NULL);
      } else {
	if (get_function (M_NAME(m))) {
	  if (rt_expr_class == rt_expr_null)
	    rt_expr_class = rt_c_fn;
	  strcatx2 (expr_buf, m -> name, NULL);
	  fn_idx = i;
	}  else {
	  if (is_mcct_expr (messages, i,
						&end_idx_ret,
						&operand_idx_ret)) {
	    rt_expr_class = rt_mixed_c_ctalk;
	    i = end_idx_ret;
	    continue;
	  } else {
	    strcatx2 (expr_buf, m -> name, NULL);
	  }
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
      /* If we *are* in an argument block, then we've already
	 called register_argblk_c_vars_1 (), near the top of
	 the fn. */
      if (!argblk && 
	  (((c = get_local_var (m -> name)) != NULL) ||
	   ((c = get_global_var_not_shadowed (m -> name)) != NULL))) {
	(void)register_c_var (m, message_stack (), i,
			      &agg_var_end_idx);
      } else {
	if (m -> attrs & TOK_SELF) {
	  if (self_class_or_instance_variable_lookahead 
	      (messages, i)) {
	    rt_expr_class = rt_self_instvar_simple_expr;
	  }
	} else {
	  /* Also in the clause above, might not be needed here. */
	  if (is_mcct_expr (messages, i,
						&end_idx_ret,
						&operand_idx_ret)) {
	    rt_expr_class = rt_mixed_c_ctalk;
	    i = end_idx_ret;
	    continue;
	  }
	}
      }
    }
  }
      
  switch (rt_expr_class) 
    {
    case rt_expr_fn:
    case rt_expr_user_fn:
      fmt_user_fn_rt_expr (messages, subexpr_start_ptr,
			   subexpr_end_ptr, expr_buf, expr_buf);
      break;
    case rt_c_fn:
      /*
       *  If there are any tokens in the predicate after the 
       *  function, output them here.
       *
       *  Also check for further functions in the expression.
       */
      fmt_c_fn_obj_args_expr (messages, fn_idx, expr_buf);
      rt_fn_start_paren = nextlangmsg (messages, fn_idx);
      rt_fn_end_paren = match_paren (messages, rt_fn_start_paren,
				     get_stack_top (messages));
      /*
       *  If the function is not the first token in a control
       *  predicate, prepend the preceding tokens.
       */
      if (ctrlblk_pred) {
	int __pred_expr_start_idx;
	__pred_expr_start_idx = 
	  nextlangmsg (messages, subexpr_start_ptr);
	if (__pred_expr_start_idx > fn_idx) {
	  toks2str (messages, __pred_expr_start_idx, fn_idx + 1, expr_buf_tmp);
	  strcatx (_t, expr_buf_tmp, expr_buf, NULL);
	  /* clean this up here, too? */
	  strcpy (expr_buf, _t);
	}
      }

      for (i = rt_fn_end_paren - 1; 
	   i > subexpr_end_ptr; i--) {
	if (get_function (M_NAME(m))) {
	  strcatx2 (expr_buf, fmt_c_fn_obj_args_expr (messages, i,
						      expr_buf_tmp), NULL);
	  i = match_paren (messages, nextlangmsg (messages, i),
			   get_stack_top (messages));
	} else {
	  strcatx2 (expr_buf, M_NAME(messages[i]), NULL);
	}
      }
      break;

    case rt_mixed_c_ctalk:
      if (operand_idx_ret != -1) {
	ctblk_handle_mixed_c_ctalk_subexpr 
	  (messages, subexpr_start_ptr,
	   operand_idx_ret,
	   subexpr_end_ptr,
	   expr_buf_tmp);
	split_cvar_registration (expr_buf_tmp, reg_buf_out, expr_buf_out);
	return expr_buf_out;
      }
      break;

    case rt_self_instvar_simple_expr:
    default:
      msg_ptr = nextlangmsg (messages, subexpr_start_ptr);
      fmt_default_ctrlblk_expr 
	(messages, subexpr_start_ptr, subexpr_end_ptr, 
	 (((messages[msg_ptr]-> attrs & TOK_IS_METHOD_ARG)|| 
	   (messages[msg_ptr] -> receiver_obj && 
	    get_local_object 
	    (messages[msg_ptr] -> receiver_obj -> __o_name,
	     messages[msg_ptr] -> receiver_obj -> __o_classname))||
	   messages[msg_ptr] -> attrs & TOK_SELF) ?
	  TRUE:FALSE), expr_buf);
      break;
    }
  split_cvar_registration (expr_buf, reg_buf_out, expr_buf_out);
  return expr_buf_out;
}

static bool cpre_paramlist_next (MESSAGE_STACK messages, int i) {
  int next_tok;
  if ((next_tok = nextlangmsg (messages, i)) != ERROR) {
    if (M_TOK(messages[next_tok]) == OPENPAREN) {
      return true;
    }
  }
  return false;
}

/* Set by resolve if ctrlblk_pred_rt_expr is called and we've
   found a method. That means the token should also be a
   METHODMSGLABEL. */
extern METHOD *resolve_method_tok_method;

int ctrlblk_pred_rt_expr (MESSAGE_STACK messages, int msg_ptr) {

  int i, j, lookahead, fn_idx = -1;
  int rt_fn_start_paren, rt_fn_end_paren;
  char expr_buf[MAXMSG], expr_buf_tmp[MAXMSG];
  char _t[MAXMSG], *e;
  MESSAGE *m = NULL;   /* Avoid a warning. */
  int n_parens;
  int pred_start_idx, pred_end_idx;
  int idx_end_ret;
  int rt_prefix_rexpr_idx = msg_ptr;  /* Avoid a warning. */
  int obj_operand_idx = -1;
  int agg_var_end_idx;
  CVAR *c, *c_1;

  if (expr_paren_check (messages, msg_ptr)) {
    (void) __ctalkExceptionInternal (messages[msg_ptr], mismatched_paren_x, NULL,0);
    return ERROR;
  }

  switch (C_CTRL_BLK -> stmt_type)
    {
    case stmt_switch:
    case stmt_if:
    case stmt_do:
    case stmt_while:
      *expr_buf = 0;
      rt_expr_class = rt_expr_null;

      /* 
	 This is a fixup from the if_stmt settings, see
	 the leading_unary_op () call in if_stmt (), above.
	 In resolve(), we checked whether the operator is overloaded
	 by a method, in which case it has the TOK_IS_PREFIX_OPERATOR
	 attribute set.
      */
      m = message_stack_at (C_CTRL_BLK -> pred_start_ptr);
      if (m -> attrs & TOK_IS_PREFIX_OPERATOR) {
	pred_start_idx = C_CTRL_BLK -> pred_start_ptr;
      } else {
	pred_start_idx =
	  nextlangmsg (messages, C_CTRL_BLK -> pred_start_ptr);
      }

      pred_end_idx = prevlangmsg (messages, C_CTRL_BLK -> pred_end_ptr);

      check_leading_class_cast (message_stack (), &pred_start_idx,
				pred_end_idx);

      have_major_booleans = false;
      (void)check_constant_expr (message_stack (), pred_start_idx,
			   pred_end_idx);

      /* 
	 We can get away with doing this here once.  Then we don't
	 need to worry about registering CVARs either, because
	 the C and Ctalk terms get interpreted and included in
	 the output separately.
       */
      if (is_mcct_expr (messages, pred_start_idx,
					    &idx_end_ret,
					    &obj_operand_idx)) {
	rt_expr_class = rt_mixed_c_ctalk;

      } else if (have_major_booleans) { /* this needs to be "else" */
	ctblk_handle_rt_bool_multiple_subexprs_no_parens (message_stack (),
							  pred_start_idx,
							  pred_end_idx);
	return SUCCESS;
      }
      /* 
	 This could be arranged better - If we're *not* in an argument block,
	 then we use register_c_var () as normal, about 700 lines down
	 from here.

	 Expressions of the mixed_c_ctalk class use the C
	 variables directly in the code... they don't need to be
	 registered for libctalk.  But we check that all of them
	 are translated to their cvartab entries.
      */
      if (argblk) {
	if ((rt_expr_class != rt_mixed_c_ctalk) &&
	    (rt_expr_class != rt_expr_null)) {
	  /* this should be phased out. */
	  register_argblk_c_vars_1 (message_stack (), 
	     pred_start_idx, pred_end_idx);
	} else if (rt_expr_class == rt_expr_null) {
	  /* this should work okay for now */
	  for (i = pred_start_idx; i >= pred_end_idx; i--) {
	    if (M_TOK(messages[i]) == LABEL) {
	      if ((c_1 = ifexpr_is_cvar_not_shadowed (messages, i))
		  != NULL) {
		argblk_CVAR_name_to_msg (messages[i], c_1);
		/***/
		fileout (fmt_register_argblk_cvar_from_basic_cvar
			 (messages, i, c_1, expr_buf), false,
			 C_CTRL_BLK->keyword_ptr+1);
		cpre_have_cvar_reg = true;
		buffer_argblk_stmt (expr_buf);
	      }
	    }
	  }
	} else {
	  for (i = pred_start_idx; i >= pred_end_idx; i--) {
	    if (M_TOK(messages[i]) == LABEL) {
	      if ((c_1 = get_local_var (M_NAME(messages[i]))) != NULL) {
		argblk_CVAR_name_to_msg (messages[i], c_1);
	      }
	    }
	  }
	}
      }

      n_parens = 0;
      for (i = pred_start_idx; i >= pred_end_idx; i--) {

	m = messages[i];

	if (M_ISSPACE(m)) {
	  strcatx2 (expr_buf, M_NAME(m), NULL);
	  ++m -> evaled;
	  ++m -> output;
	  continue;
	}

	if (IS_OBJECT (m -> obj) &&
	    DEFAULTCLASS_CMP(m->obj, ct_defclasses -> p_cfunction_class,
			     CFUNCTION_CLASSNAME)) {
	  if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	    if ((messages[i] -> tokentype == LABEL) && 
		(messages[lookahead] -> tokentype == OPENPAREN)) {
  	      if ((e = clib_fn_expr (messages, i, i, m -> obj)) != NULL) {
		/* this previous call is so involved when it
		   gets to a CVAR registration, that we'll
		   just recheck it here directly */
		for (j = i; messages[j]; --j) {
		  if (M_TOK(messages[j]) == OPENBLOCK) {
		    break;
		  } else if (messages[j] -> attrs &
			     TOK_CVAR_REGISTRY_IS_OUTPUT) {
		    cpre_have_cvar_reg = true;
		    break;
		  } else if (get_local_var (M_NAME(messages[j])) ||
			     get_global_var (M_NAME(messages[j]))) {
		    /* In case the CVAR registration occurred on 
		       another stack. */
		    cpre_have_cvar_reg = true;
		    break;
		  }
		}
		strcatx2 (expr_buf, e, NULL);
		rt_expr_class = rt_expr_fn;
		i = 
		  match_paren (messages, lookahead, get_stack_top (messages));
		for (j = lookahead; j >= i; j--) {
		  ++messages[j] -> evaled;
		  ++messages[j] -> output;
		}
	      }
	    }
	  }
	} else {
	  if (IS_C_OP(M_TOK(m))) {
	    (void)operand_type_check (message_stack (), i);
	    strcatx2 (expr_buf, m -> name, NULL);
	  } else {
	    /* if (get_function (M_NAME(m))) { */
	    if (get_function (M_NAME(m)) &&
		cpre_paramlist_next (messages, i)) {
	      if (rt_expr_class == rt_expr_null)
		rt_expr_class = rt_c_fn;
	      strcatx2 (expr_buf, m -> name, NULL);
	      fn_idx = i;
	    }  else {
	      if (is_mcct_expr (messages, i,
				&idx_end_ret,
				&obj_operand_idx)) {
		rt_expr_class = rt_mixed_c_ctalk;
		i = idx_end_ret;
		continue;
	      } else {
		strcatx2 (expr_buf, m -> name, NULL);
	      }
	    }
	  }
	}

	/*
	 *  If the expression also contains C variables or functions,
	 *  they get registered now in register_c_var () called by
	 *  method_args (), etc., so check here if the token has
	 *  been evaluated and register the C variable here if 
	 *  necessary.
	 *
	 *  Also do some argument checking if we've found a method
	 *  for a METHODMSGLABEL token.
	 */
	switch (M_TOK(m))
	  {
	  case LABEL:
	    if (!m -> evaled) {
	      /* If we *are* in an argument block, then we've already
		 called register_argblk_c_vars_1 (), near the top of
		 the fn. */
	      if (!argblk && 
		  (((c = get_local_var (m -> name)) != NULL) ||
		   ((c = get_global_var_not_shadowed (m -> name)) != NULL))) {
		if ((rt_expr_class != rt_mixed_c_ctalk) &&
		    (rt_expr_class != rt_expr_null)) {
		  /* this should go away as we rework the different
		     expresstion classes */
		  (void)register_c_var (m, message_stack (), i,
					&agg_var_end_idx);
		}
	      } else {
		if (m -> attrs & TOK_SELF) {
		  if (self_class_or_instance_variable_lookahead 
		      (messages, i)) {
		    rt_expr_class = rt_self_instvar_simple_expr;
		  }
		}
	      }
	    }
	    break;
	  case OPENPAREN:
	    ++n_parens;
	    break;
	  case CLOSEPAREN:
	    --n_parens;
	    break;
	  case BOOLEAN_AND:
	  case BOOLEAN_OR:
	    rt_expr_class = rt_bool_multiple_subexprs;
	    break;
	  case METHODMSGLABEL:
	    if (IS_METHOD(resolve_method_tok_method)) {
	      if (resolve_method_tok_method -> n_params == 0) {
		if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
		  if (!METHOD_ARG_TERM_MSG_TYPE (messages[lookahead]) &&
		      !IS_C_OP_TOKEN_NOEVAL(M_TOK(messages[lookahead]))) {
		    method_args_wrong_number_of_arguments_1
		      (messages, i, resolve_method_tok_method,
		       resolve_method_tok_method,
		       ((interpreter_pass == parsing_pass) ? get_fn_name () :
			new_methods[new_method_ptr + 1] -> method -> name),
		       interpreter_pass);
		  }
		}
	      }
	      resolve_method_tok_method = NULL;
	    }
	    break;
	  } /* switch (M_TOK(m)) */

	if (m -> attrs & TOK_IS_PREFIX_OPERATOR) {
	  int p_prev, p_lookback, p_next;
	  bool have_lval_object = false;
	  if ((p_prev = prevlangmsg (messages, i)) != ERROR) {
	    if (IS_C_BINARY_MATH_OP(M_TOK(messages[p_prev]))) {
	      for (p_lookback = p_prev; 
		   (p_lookback <= pred_start_idx) && !have_lval_object;
		   p_lookback++) {
		if (messages[p_lookback] -> obj)
		  have_lval_object = true;
	      }
	      if ((!have_lval_object) && 
		  (M_TOK(messages[i]) != M_TOK(messages[p_prev]))) {
		/*  See the next clause, which handles the first of
		    several repeated ops. */
		rt_expr_class = rt_prefix_rexpr;
		rt_prefix_rexpr_idx = i;
	      }
	    } else {
	      /* For cases like if (**<expr>) ... */
	      if ((p_next = nextlangmsg (messages, i))  != ERROR) {
		if (M_TOK(messages[p_next]) == M_TOK(messages[i])) {
		  rt_prefix_rexpr_idx = i;
		}
	      }
	    }
	  }
	}
	

	++m -> evaled;
	++m -> output;
      }
      
      switch (rt_expr_class) 
	{
	case rt_expr_fn:
	case rt_expr_user_fn:
	  fileout (fmt_user_fn_rt_expr_b (messages, 
					  pred_start_idx, pred_end_idx,
					  expr_buf, _t, cpre_have_cvar_reg), 
		   FALSE, 
		   pred_end_idx);
	  break;
	case rt_c_fn:
	  /*
	   *  If there are any tokens in the predicate after the 
	   *  function, output them here.
	   *
	   *  Also check for further functions in the expression.
	   */
	  fmt_c_fn_obj_args_expr (messages, fn_idx, expr_buf);
	  rt_fn_start_paren = nextlangmsg (messages, fn_idx);
	  rt_fn_end_paren = match_paren (messages, rt_fn_start_paren,
					 get_stack_top (messages));
	  /*
	   *  If the function is not the first token in a control
	   *  predicate, prepend the preceding tokens.
	   */
	  if (ctrlblk_pred) {
	    int __pred_expr_start_idx;
	    __pred_expr_start_idx = 
	      nextlangmsg (messages, pred_start_idx);
	    if (__pred_expr_start_idx > fn_idx) {
	      
	      /* prepend tokens before the fn label. */
	      toks2str (messages, __pred_expr_start_idx, fn_idx + 1, expr_buf_tmp);
	      strcatx (_t, expr_buf_tmp, expr_buf, NULL);
	      strcpy (expr_buf, _t);
	    }
	  }

	  for (i = rt_fn_end_paren - 1; 
	       i > pred_end_idx; i--) {
	    if (get_function (M_NAME(m))) {
	      strcatx2 (expr_buf, fmt_c_fn_obj_args_expr (messages, i,
							  expr_buf_tmp), NULL);
	      i = match_paren (messages, nextlangmsg (messages, i),
					 get_stack_top (messages));
	    } else {
	      strcatx2 (expr_buf, M_NAME(messages[i]), NULL);
	    }
	  }
	  fileout (expr_buf, FALSE, pred_end_idx);
	  break;
	case rt_bool_multiple_subexprs:
	  ctblk_handle_rt_bool_multiple_subexprs_parens 
	    (messages, pred_start_idx);
	  break;
	case rt_prefix_rexpr:
	  handle_rt_prefix_rexpr 
	    (messages, rt_prefix_rexpr_idx,
	     pred_start_idx, pred_end_idx, expr_buf);
	  fileout (expr_buf, FALSE, rt_prefix_rexpr_idx);
	  break;
	case rt_mixed_c_ctalk:
	  if (obj_operand_idx != -1) {
	    handle_mcct_expr (messages,
			      C_CTRL_BLK -> pred_start_ptr,
			      C_CTRL_BLK -> pred_end_ptr);
	  }
	  break;
	case rt_self_instvar_simple_expr:
	default:
	  if (major_logical_op_check (messages, 
				      C_CTRL_BLK->pred_start_ptr,
				      C_CTRL_BLK->pred_end_ptr)) {
	    /* Then we also need to include a leading unary op
	       if there is one, instead of leaving it to the
	       C compiler. */
	    if (IS_C_UNARY_MATH_OP(M_TOK(messages[C_CTRL_BLK->pred_start_ptr]))){
	      pred_start_idx = C_CTRL_BLK -> pred_start_ptr;
	      ++messages[pred_start_idx] -> evaled;
	      ++messages[pred_start_idx] -> output;
	    }
	  }
	  /***/
	  if (cpre_have_cvar_reg) {
	    fileout 
	      (fmt_default_ctrlblk_expr 
	       (messages, pred_start_idx, 
		pred_end_idx, 
		(((messages[msg_ptr]-> attrs & TOK_IS_METHOD_ARG)|| 
		  (messages[msg_ptr] -> receiver_obj && 
		   get_local_object 
		   (messages[msg_ptr] -> receiver_obj -> __o_name,
		    messages[msg_ptr] -> receiver_obj -> __o_classname))||
		  messages[msg_ptr] -> attrs & TOK_SELF) ?
		 TRUE:FALSE), _t), 
	       FALSE, 
	       pred_start_idx);
	    if (C_CTRL_BLK -> else_end_ptr > 0) {
	      fileout (DELETE_CVARS_CALL, 0,
		       C_CTRL_BLK -> else_end_ptr - 1);
	    } else if (C_CTRL_BLK -> blk_end_ptr > 0) {
	      fileout (DELETE_CVARS_CALL, 0,
		       C_CTRL_BLK -> blk_end_ptr - 1);
	    }
	  } else {
	    fileout 
	      (fmt_default_ctrlblk_expr 
	       (messages, pred_start_idx, 
		pred_end_idx, 
		(((messages[msg_ptr]-> attrs & TOK_IS_METHOD_ARG)|| 
		  (messages[msg_ptr] -> receiver_obj && 
		   get_local_object 
		   (messages[msg_ptr] -> receiver_obj -> __o_name,
		    messages[msg_ptr] -> receiver_obj -> __o_classname))||
		  messages[msg_ptr] -> attrs & TOK_SELF) ?
		 TRUE:FALSE), _t), 
	       FALSE, 
	       pred_start_idx);
	  }
	  break;
	}
      /*
       *  loop_block_start () checks this.
       */
      C_CTRL_BLK -> pred_expr_evaled = TRUE;
      break; /* case stmt_switch, stmt_if, stmt_while */
    case stmt_case:
      /*
       *  Case arguments are still constants as in C99.
       */
      break;
    case stmt_for:
      /*
       *   The function for_stmt () and others handle 
       *   the for loop predicates.
       */
      break;
    case stmt_default:
    default:
      break;
    }
  return SUCCESS;
}

/*
 *  For expressions that have more than one subexpr separated by
 *  && or ||, without parenthesization.  This is a lot like
 *  handle_ctrlblk_subexprs, but we can alter it if necessary
 *  for expressions without parens around the subexprs..
 */
char *ctblk_eval_major_bool_subexprs (MESSAGE_STACK messages,
				  int main_stack_pred_start_idx,
				  int main_stack_pred_end_idx) {
   int i, j;
  static char exprbuf[MAXMSG];
  char subexprbuf[MAXMSG];
  char reg_buf[MAXMSG], expr_call_buf[MAXMSG], reg_call_buf[MAXMSG];

  parse_ctrlblk_subexprs (messages);

  j = 0;
  memset (exprbuf, 0, MAXMSG);
  memset (reg_buf, 0, MAXMSG);
  memset (subexprbuf, 0, MAXMSG);

  for (i = 0; i < subexpr_index_ptr; i++) {
    ctrlblk_pred_rt_subexpr
      (messages, 
       cond_subexpr_indexes[i].start,
       cond_subexpr_indexes[i].end,
       reg_call_buf, expr_call_buf);
    if (*reg_call_buf)
      strcatx2 (reg_buf, reg_call_buf, NULL);
    strcatx2 (subexprbuf, expr_call_buf, NULL);
    if (i < (subexpr_index_ptr - 1))
      strcat (subexprbuf, messages[bool_op_indexes[j++]] -> name);
  }

  if (*reg_buf)
    strcatx (exprbuf, reg_buf, subexprbuf, NULL);
  else
    strcatx (exprbuf, subexprbuf, NULL);
  return exprbuf;
}
