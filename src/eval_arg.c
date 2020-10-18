/* $Id: eval_arg.c,v 1.12 2020/10/17 22:29:37 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2020
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
#include "bufmthd.h"

extern I_PASS interpreter_pass;    /* Declared in main.c.               */

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c.   */
extern int new_method_ptr;

extern int ambiguous_arg_start_idx,
  ambiguous_arg_end_idx;             /* Declared in method.c. */
extern OBJECT *eval_arg_obj;           /* Declared in method.c. */
extern int m_message_stack;

 extern PARSER *parsers[MAXARGS+1];
 extern int current_parser_ptr;

extern bool argblk;          /* Declared in argblk.c.                    */

extern OBJECT *rcvr_class_obj;        /* Declared in lib/rtnwmthd.c and    */
                                      /* filled in by                      */
                                      /* new_instance|class_method ().     */

extern MESSAGE *m_messages[N_MESSAGES+1]; /* Declared in method.c */
extern int m_message_ptr;

extern int error_line,      /* Declared in errorloc.c.                  */
  error_column;

extern bool ctrlblk_pred,            /* Global states.                    */
  ctrlblk_blk,
  ctrlblk_else_blk;
extern int for_init, for_term, for_inc;

extern int 
n_th_arg_CVAR_has_unary_inc_prefix;   /* Declared in prefixop.c.           */
extern int n_th_arg_CVAR_has_unary_dec_prefix;
extern int n_th_arg_CVAR_has_unary_inc_postfix;
extern int n_th_arg_CVAR_has_unary_dec_postfix;

extern ARG_CLASS arg_class;    /* Declared in method.c. */
extern int fn_arg_expression_call; /* Declared in arg.c. */

extern int warn_duplicate_name_opt;

extern CTRLBLK *ctrlblks[MAXARGS + 1];  /* Declared in control.c. */
extern int ctrlblk_ptr;

extern bool fn_tmpl_eval_arg;

ARG_TERM arg_c_fn_terms[MAXARGS] = {{NULL, 0, 0},};
int arg_c_fn_term_ptr = 0;

/* For single-token args. Looks only for a method in the
   return class at the moment. */
static bool member_of_method_return_class (METHOD *method,
					   MESSAGE *m_arg) {
  OBJECT *returnclass_object;
  METHOD *m;
  if ((returnclass_object = get_class_object (method -> returnclass))
      != NULL) {
    for (m = returnclass_object -> instance_methods; m; m = m -> next) {
      if (str_eq (m -> name, M_NAME(m_arg))) {
	return true;
      }
    }
  }
  return false;
}

/*
 *  Mark a prefix method syntactically _unless_
 *  it is a ! operator and it is the first token
 *  of a contrl block predicate, in which case we 
 *  let the compiler handle the negation in context....
 *
 *  Note: This is similar to prefix_method_attr in resolve.c,
 *  except it only works with the single stack frame that eval
 *  arg uses; i.e., the prevalngmsg call just gets replaced with
 *  N_MESSAGES.
 *
 *  Can probably be pared down further in the future also.
 */
#define PREFIX_PREV_OP_TOKEN_NOEVAL(tok) (IS_C_OP_TOKEN_NOEVAL(tok) || \
			    (tok == OPENBLOCK) || \
			    (tok == CLOSEBLOCK) || \
			       (tok==OPENPAREN) || \
			     (tok==CLOSEPAREN))
static int prefix_method_attr_arg (MESSAGE_STACK messages, int message_ptr) {
  int prev_tok_idx, next_tok_idx;
  int r;
  MESSAGE *m_prev_tok;
  MESSAGE *m = messages[message_ptr];
  MESSAGE *m_rcvr, *m_op;

  r = FALSE;

  if ((next_tok_idx = nextlangmsgstack_pfx (messages, message_ptr)) != ERROR) {
    if ((M_TOK(messages[next_tok_idx]) == SEMICOLON) ||
	(M_TOK(messages[next_tok_idx]) == ARGSEPARATOR))
      return r;
  }

  /* If there's an op at the beginning of a stack, then it's
     automatically a prefix op. */
  if (message_ptr == N_MESSAGES) {
#if 0 /***/
  if ((prev_tok_idx = prevlangmsg (messages, 
				   message_ptr)) == ERROR) {
#endif    
    if (IS_C_UNARY_MATH_OP(M_TOK(messages[message_ptr])) ||
	(M_TOK(messages[message_ptr]) == MINUS)) {
      messages[message_ptr] -> attrs |= TOK_IS_PREFIX_OPERATOR;
      return TRUE;
    } else {
      return FALSE;
    }
  } else {
    prev_tok_idx = prevlangmsg (messages, message_ptr); /***/
  }

  m_prev_tok = messages[prev_tok_idx];
  if (M_TOK(m) == EXCLAM) {
    if (ctrlblk_pred) {
      /*
       *  If a contrl block predicate begins with a sequence 
       *  like these, then use the C operators.  We can send
       *  them to the output verbatim.
       *
       *     <negation_prefix_tok> : !
       *     <negation_prefix_tok_sequence> :  
       *       <negation_prefix_tok> |
       *       ++ | -- | ~ | & | * <negation_prefix_tok> |
       *       <negation_prefix_tok_sequence> <negation_prefix_tok> |
       *
       *    <if|while> (<prefix_tok_sequence>!<expr>) { ...
       *  
       *  When compiling, though, only sequences of ! and ~ do not generate
       *  errors or warnings.
       *
       *  Sequences of multiple ++|-- operators are not legal C, so we
       *  don't support them in C expressions both here and elsewhere.
       *
       *  NOTE - If we see a leading ! operator, and it is overloaded
       *  by a method, the message gets the TOK_IS_PREFIX_OPERATOR
       *  attribute also, and we fix up the start of the predicate
       *  expression in ctrlblk_pred_rt_expr (), to include the
       *  operator in the run-time expression.
       *
       *  NOTE2 - In actual use this should always be the case,
       *  since we have an Object : ! method.  
       *
       *  TODO - we should simplify this so that ! works the same
       *  everywhere, no matter where it appears in an expression.
       *  See test/expect/condexprs3.c for an example.
       */
      if (!is_leading_ctrlblk_prefix_op_tok
	  (messages, message_ptr, P_MESSAGES)) {
	if (IS_C_UNARY_MATH_OP(M_TOK(m))) {
	  if (PREFIX_PREV_OP_TOKEN_NOEVAL(M_TOK(m_prev_tok))) {
	    m -> attrs |= TOK_IS_PREFIX_OPERATOR;
	    r = TRUE;
	  }
	}
      } else {
	int next_label_idx;
	OBJECT *_pfx_rcvr_obj;
	if ((next_label_idx = nextlangmsg (messages, message_ptr))
	    != ERROR) {
	  m_op = message_stack_at (message_ptr);
	  m_rcvr = message_stack_at (next_label_idx);
	  if (((_pfx_rcvr_obj = get_global_object (M_NAME(m_rcvr), NULL)) 
	       != NULL) || 
	      ((_pfx_rcvr_obj = get_local_object (M_NAME(m_rcvr), NULL)) 
	       != NULL)) {
	    if (get_prefix_instance_method (m, _pfx_rcvr_obj, M_NAME(m_op), 
				     FALSE)) {
	      m -> attrs |= TOK_IS_PREFIX_OPERATOR;
	      r = TRUE;
	    }
	  } else {
	    if (m -> attrs & TOK_SELF) {
	      if (get_prefix_instance_method (m, rcvr_class_obj, 
					      M_NAME(m_op), 
					      FALSE)) {
		m -> attrs |= TOK_IS_PREFIX_OPERATOR;
		r = TRUE;
	      }
	    } else if (m_rcvr -> attrs & TOK_SUPER) {
		if (argblk) {
		  if (get_prefix_instance_method (m, rcvr_class_obj, 
						  M_NAME(m_op), 
						  FALSE)) {
		    m -> attrs |= TOK_IS_PREFIX_OPERATOR;
		    r = TRUE;
		  }
		} else {
		  if (get_prefix_instance_method 
		      (m, 
		       rcvr_class_obj -> __o_superclass, 
		       M_NAME(m_op), 
		       FALSE)) {
		    m -> attrs |= TOK_IS_PREFIX_OPERATOR;
		    r = TRUE;
		  }
		}
	    }
	  }
	}
      }
    } else if (get_function (M_NAME(m_prev_tok))) {
      int i_2, n_fn_parens;
      /*
       *  Distinguish an expression like this:
       *
       *  fn_label (*<arg_expr>)...
       *
       *   from this:
       *
       *   fn_label () * <arg_expr>
       *
       *  ... because prevlangmsgstack_pfx returns the stack index of
       *  the fn_label token.
       */
      n_fn_parens = 0;
      for (i_2 = message_ptr + 1; i_2 <= prev_tok_idx; i_2++) {
	switch (M_TOK(messages[i_2]))
	  {
	  case WHITESPACE:
	  case NEWLINE:
	    continue;
	    break;
	  case OPENPAREN:
	    --n_fn_parens;
	    break;
	  case CLOSEPAREN:
	    ++n_fn_parens;
	    break;
	  }
      }
      if (n_fn_parens < 0) {
	/* then the closing parens of fn_label's argument list
	   occur after our expression's tokens. */
	m -> attrs |= TOK_IS_PREFIX_OPERATOR;
	r = TRUE;
      }
    } else { /* if (ctrlblk_pred) */
      if (IS_C_UNARY_MATH_OP(M_TOK(m))) {
	if (PREFIX_PREV_OP_TOKEN_NOEVAL(M_TOK(m_prev_tok))) {
	  m -> attrs |= TOK_IS_PREFIX_OPERATOR;
	  r = TRUE;
	}
      }
    }
  } else if (IS_C_UNARY_MATH_OP(M_TOK(m)) || /* if (M_TOK(m) == EXCLAM) */
	     is_unary_minus (messages, message_ptr)) {
    if (PREFIX_PREV_OP_TOKEN_NOEVAL(M_TOK(m_prev_tok))) {
      m -> attrs |= TOK_IS_PREFIX_OPERATOR;
      r = TRUE;
    }
  }
  return r;
}

static bool c_first_operand (MESSAGE_STACK messages, int
			     idx, int *end_idx_ret,
			     int *operand_idx_ret) {
  MESSAGE *m;
  int lookahead;
  int obj_operand_idx;
  int subscr_end;
  int tok_start_idx;

  m = messages[idx];

  if (messages == message_stack ()) {
    tok_start_idx = C_CTRL_BLK -> pred_start_ptr;
  } else if (messages == method_message_stack ()) {
      tok_start_idx = N_MESSAGES;
  } else {
    char buf[MAXMSG];
    sprintf (buf, "%s", "Error: Unimplemented message stack in "
	     "ctrlblk_is_rt_first_operand_expr.\n");
    _error (buf);
  }

  if (get_local_var (M_NAME(m)) ||
      get_global_var (M_NAME(m))) {
    if ((lookahead = nextlangmsg (messages, idx)) != ERROR) {
      if (IS_C_BINARY_MATH_OP (M_TOK(messages[lookahead]))) {
	/* This helps to prevent trying to mangle complex
	   expressions for now ... */
	if (tok_paren_level 
	    (messages, tok_start_idx, idx) 
	    <= 1) {
	  if ((obj_operand_idx = nextlangmsg 
	       (messages, lookahead)) != ERROR) {
	    if (is_object_or_param 
		(M_NAME(messages[obj_operand_idx]), NULL)) {
	      /*
	       *  This is an expression where the first op
	       *  is a C variable, like this.  Simple terms
	       *  only for now (at least the first term).
	       *
	       *  while (r < myObj myInstVar) {
	       *     ...
	       *  }
	       *
	       */
	      *end_idx_ret = lookahead;
	      *operand_idx_ret = obj_operand_idx;
	      return true;
	    }
	  }
	}  /* if (tok_paren_level (... */
      } else { /* if (IS_C_BINARY_MATH_OP ( ... */
	if (M_TOK(messages[lookahead]) == ARRAYOPEN) {
	  /* TODO - make sure we can handle object expressions
	     in the subscripts. */
	  if ((subscr_end = last_subscript_tok 
	       (messages, idx, get_stack_top (messages)))
	      != ERROR) {
	    if ((lookahead = nextlangmsg (messages, subscr_end)) != ERROR) {
	      if (IS_C_BINARY_MATH_OP (M_TOK(messages[lookahead]))) {
		/* As above, this helps to prevent trying to 
		   mangle complex expressions for now ... */
		if (tok_paren_level 
		    (messages, tok_start_idx, idx) 
		    <= 1) {
		  if ((obj_operand_idx = nextlangmsg 
		       (messages, lookahead)) != ERROR) {
		    if (is_object_or_param 
			(M_NAME(messages[obj_operand_idx]), NULL)) {
		      *end_idx_ret = lookahead;
		      *operand_idx_ret = obj_operand_idx;
		      return true;
		    }
		  }
		}
	      }
	    }
	  }
	} /* if (M_TOK(m) == ARRAYOPEN) { */
      } /* if (IS_C_BINARY_MATH_OP ( ... */
    } /* if ((lookahead = ... */
  }
  return false;
}

/* this is probably only useful by eval_arg.  doesn't
   try to count the arguments if there are parentheses
   in the arglist (so far) */
static int args_in_arglist (MESSAGE_STACK messages,
			    int method_tok, int end_idx,
			    METHOD *arg_method,
			    int arg_start_main_stack_idx,
			    char *expr_str) {
  int i, prev_idx, next_idx;
  int n_commas = 0;
  METHOD *next_method;
  OBJECT *arg_method_returnclass;

  if (method_tok == (end_idx + 1)) {
    return 0;
  }

  for (i = method_tok - 1; i > end_idx; --i) {

    switch (M_TOK(messages[i]))
      {
      case ARGSEPARATOR:
	++n_commas;
	break;
      case OPENPAREN:
      case CLOSEPAREN:
	return -1;
	break;
      case LABEL:
	/* if another method follows immediately, return 0 */
	prev_idx = prevlangmsg (messages, i);
	if (prev_idx == method_tok) {
	  /* need to defer the test until run time */
	  if (str_eq (arg_method -> returnclass, "Any"))
	    return -1;
	  if ((arg_method_returnclass =
	       get_class_object (arg_method -> returnclass)) != NULL) {
	    if (((next_method = get_instance_method
		  (messages[i], arg_method_returnclass,
		   M_NAME(messages[i]), ANY_ARGS, FALSE)) != NULL) ||
		((next_method = get_class_method
		  (messages[i], arg_method_returnclass,
		   M_NAME(messages[i]), ANY_ARGS, FALSE)) != NULL)) {
	      /* check for an object that shadows a method before
		 returning 0 */
	      if (is_object_or_param (M_NAME(messages[i]), NULL)) {
		warning (message_stack_at (arg_start_main_stack_idx),
			 "Object, \"%s,\" shadows a method:\n\n\t%s\n",
			 M_NAME(messages[i]),
			 expr_str);
		if (i == (end_idx + 1))
		  return n_commas + 1;
	      } else {
		return 0;
	      }
	    }
	  }
	}
      break;
      case INCREMENT:
      case DECREMENT:
	if (i == (end_idx + 1))
	  return n_commas;
	if ((next_idx = nextlangmsg (messages, i)) != ERROR) {
	  if (M_TOK(messages[next_idx]) == LABEL) {
	    /* if the next label is an object, then it's part of
	       an argument. If the next label is a method, it's
	       part of a compound statement. */
	    if (get_object (M_NAME(messages[next_idx]), NULL) ||
		get_local_var (M_NAME(messages[next_idx])) ||
		get_global_var (M_NAME(messages[next_idx]))) {
	      continue;
	    } else {
	      prev_idx = prevlangmsg (messages, i);
	      if (prev_idx == method_tok) {
		/* need to defer the test until run time */
		if (str_eq (arg_method -> returnclass, "Any"))
		  return -1;
		if ((arg_method_returnclass =
		     get_class_object (arg_method -> returnclass)) != NULL) {
		  if (((next_method = get_instance_method
			(messages[i], arg_method_returnclass,
			 M_NAME(messages[i]), ANY_ARGS, FALSE)) != NULL) ||
		      ((next_method = get_class_method
			(messages[i], arg_method_returnclass,
			 M_NAME(messages[i]), ANY_ARGS, FALSE)) != NULL)) {
		    return 0;
		  }
		}
	      }
	    }
	  }
	}
	break;
      }
  }
  return n_commas + 1;
}

static void constructor_arg_shadows_c_variable_warning (MESSAGE_STACK messages,
							int idx, 
							MESSAGE_STACK arg_messages,
							int local_idx,
							METHOD *method) {
  CVAR *cvar = NULL;
  MESSAGE *m_arg;
  if (!IS_CONSTRUCTOR(method))
    return;
  if (!warn_duplicate_name_opt)
    return;
  m_arg = arg_messages[local_idx];
  if (((cvar = get_global_var (M_NAME(m_arg))) != NULL) ||
      ((cvar = get_local_local_cvar (M_NAME(m_arg))) != NULL)) {
    if (IS_CVAR(cvar)) {
      warning (messages[idx], "Argument \"%s\" duplicates a C variable name.",
	       M_NAME(m_arg));
    }
  }
}

static inline void elide_inc_or_dec_prefix (ARGSTR *argbuf) {
  int i;
  MESSAGE *m;
  MESSAGE_STACK msgs;
  if (fn_arg_expression_call > 0) {
    msgs = method_message_stack ();
  } else {
    msgs = message_stack ();
  }
  if (n_th_arg_CVAR_has_unary_inc_prefix != -1) {
    for (i = argbuf -> start_idx; i >= argbuf -> end_idx; --i) {
      m = msgs[i];
      if (M_TOK(m) == INCREMENT) {
	m -> name[0] = ' '; m -> name[1] = 0;
	m -> tokentype = WHITESPACE;
      }
    }
  } else if (n_th_arg_CVAR_has_unary_dec_prefix != -1) {
    for (i = argbuf -> start_idx; i >= argbuf -> end_idx; --i) {
      m = msgs[i];
      if (M_TOK(m) == DECREMENT) {
	m -> name[0] = ' '; m -> name[1] = 0;
	m -> tokentype = WHITESPACE;
      }
    }
  }
}

static inline void elide_inc_or_dec_postfix (ARGSTR *argbuf) {
  int i;
  MESSAGE *m;
  MESSAGE_STACK msgs;
  if (fn_arg_expression_call > 0) {
    msgs = method_message_stack ();
  } else {
    msgs = message_stack ();
  }
  if (n_th_arg_CVAR_has_unary_inc_prefix != -1) {
    for (i = argbuf -> start_idx; i >= argbuf -> end_idx; --i) {
      m = msgs[i];
      if (M_TOK(m) == INCREMENT) {
	m -> name[0] = ' '; m -> name[1] = 0;
	m -> tokentype = WHITESPACE;
      }
    }
  } else if (n_th_arg_CVAR_has_unary_dec_prefix != -1) {
    for (i = argbuf -> start_idx; i >= argbuf -> end_idx; --i) {
      m = msgs[i];
      if (M_TOK(m) == DECREMENT) {
	m -> name[0] = ' '; m -> name[1] = 0;
	m -> tokentype = WHITESPACE;
      }
    }
  }

}

static bool is_method_of_rcvr_class (OBJECT *rcvr_class_obj,
				     char *method_name) {
  METHOD *m;
  if (IS_OBJECT(rcvr_class_obj)) {
    for (m = rcvr_class_obj -> instance_methods; m; m = m -> next) {
      if (str_eq (method_name, m -> name)) {
	return true;
      }
    }
  }
  return false;
}

/* this is sort of a fixup, because eval_arg doesn't change
   a method label's token to METHODMSGLABEL. */
static OBJECT *obj_or_param_after_method_label (MESSAGE_STACK messages,
						int label_idx,
						int next_idx) {
  OBJECT *next_object;
  OBJECT *arg_method_rcvr;
  METHOD *new_method;
  METHOD *arg_method;
  int i, prev_idx;

  if (IS_OBJECT((messages[label_idx] -> receiver_obj))) {
    arg_method_rcvr = messages[label_idx] -> receiver_obj;
  } else {
    if ((prev_idx = prevlangmsg (messages, label_idx)) != ERROR) {
      if (IS_OBJECT(messages[prev_idx] -> obj)) {
	arg_method_rcvr = messages[prev_idx] -> obj;
      } else {
	return NULL;
      }
    } else {
      return NULL;
    }
  }

  if (((arg_method = get_instance_method (messages[label_idx],
					 arg_method_rcvr -> __o_class,
					  M_NAME(messages[label_idx]),
					  ANY_ARGS, FALSE)) == NULL) &&
      ((arg_method = get_class_method (messages[label_idx],
				       arg_method_rcvr -> __o_class,
				       M_NAME(messages[label_idx]),
				       ANY_ARGS, FALSE)) == NULL))
    return NULL;

  if ((next_object = get_local_object
       (M_NAME(messages[next_idx]), NULL)) != NULL)
    return next_object;
  if ((next_object = get_global_object
       (M_NAME(messages[next_idx]), NULL)) != NULL)
    return next_object;

  if (interpreter_pass == method_pass) {
    new_method = new_methods[new_method_ptr+1] -> method;
    for (i = 0; i < new_method -> n_params; ++i) {
      if (str_eq (new_method -> params[i] -> name,
		  M_NAME(messages[next_idx]))) {
	next_object = create_object
	  (new_method -> params[i] -> class, M_NAME(messages[next_idx]));
	save_method_object (next_object);
	return next_object;
      }
    }
  }
  
  
  return NULL;
}

static void set_arg_object (MESSAGE *m_arg, OBJECT *o) {
  if (IS_OBJECT (m_arg -> obj)) {
    m_arg -> value_obj = o;
  } else {
    m_arg -> obj = o;
  }
}				       

/* these indices are on the main stack. */
static void leading_typecast_indexes (int main_stack_start_idx,
				      int *cast_start_paren_idx,
				      int *cast_end_paren_idx) {
  int _p_s, _p_s_l;
  _p_s = nextlangmsg (message_stack (), main_stack_start_idx);
  while ((_p_s_l = nextlangmsg (message_stack (), _p_s))
	 != ERROR) {
    if (M_TOK(message_stack_at (_p_s_l)) != OPENPAREN)
      break;
    _p_s = _p_s_l;
  }
  *cast_start_paren_idx = _p_s;
  *cast_end_paren_idx = match_paren (message_stack (), 
				     *cast_start_paren_idx,
				     get_stack_top (message_stack ()));
}

static OBJECT *eval_constant_arg (int arg_idx) {
  MESSAGE *m_arg;
  OBJECT *arg_obj;
  m_arg = message_stack_at (arg_idx);
  switch (M_TOK(m_arg))
    {
    case LITERAL:
    case LITERAL_CHAR:
    case INTEGER:
    case LONG:
    case FLOAT:
    case LONGLONG:
    case PATTERN:
      arg_obj = constant_token_object (m_arg);
      save_method_object (arg_obj);
      arg_class = arg_const_tok;
      return arg_obj;
      break;
    }
  return NULL;
}

/* This can save some calls to tokenize, once we can analyze the
   argument with the preceding method. */
static OBJECT *resolve_unary_minus_arg_0 (MESSAGE_STACK messages,
					  int arg_start_idx,
					  int arg_end_idx) {
  int lookahead;
  MESSAGE *m_sign, *m_n; /* ? */
  char tokbuf[MAXLABEL];  /* needed because buffers would overlap. */
  OBJECT *arg_obj;
  if (messages[arg_start_idx] -> tokentype != MINUS)
    return NULL;
  if ((lookahead = nextlangmsg (message_stack (), arg_end_idx))
      != ERROR) {
    switch (M_TOK(messages[lookahead]))
      {
      case SEMICOLON:
      case ARGSEPARATOR:
	m_sign = messages[arg_start_idx];
	m_n = messages[arg_end_idx];
	strcatx (tokbuf, M_NAME(m_sign), M_NAME(m_n), NULL);
	resize_message (m_sign, strlen (tokbuf));
	strcpy (m_sign -> name, tokbuf);
	m_sign -> tokentype = m_n -> tokentype;
	m_n -> name[0] = ' '; m_n -> name[1] = '\0';
	m_n -> tokentype = WHITESPACE;
	arg_obj = constant_token_object (m_sign);
	save_method_object (arg_obj);
	arg_class = arg_const_tok;
	return arg_obj;
	break;
      }
  }
  return NULL;
}

static OBJECT *resolve_single_token_arg (METHOD *rcvr_method,
					 MESSAGE_STACK messages, 
					 int message_ptr,
					 OBJECT *rcvr_class_obj_arg,
					 int main_stack_idx,
					 ARGSTR *argbuf) {

  OBJECT *result_object = NULL;
  OBJECT *class_object; 
  OBJECT *instancevar_object;
  MESSAGE *m;      
  int msg_frame_top;
  int prev_label_ptr = ERROR;  /* Stack pointers to messages with      */
  int prev_method_ptr = ERROR; /*  resolved objects.                   */
  int prev_tok_ptr = ERROR;
  MESSAGE *m_prev_label = NULL;    /* Messages that refer to previously  */
  MESSAGE *m_prev_method = NULL;   /* resolved objects. Initialized here */
				   /* to avoid warnings.                 */
  int next_label_ptr;          /* Forward message references to check  */
  int arglist_end;
  METHOD *method;
  char errbuf[MAXMSG];
  MSINFO ms;
  ms.messages = messages;
  ms.stack_start = stack_start (messages);
  ms.stack_ptr = get_stack_top (messages);
  ms.tok = message_ptr;
  
  m = messages[message_ptr];

  arg_class = arg_obj_tok;

  if (!m || !IS_MESSAGE(m))
    _error ("Invalid message in resolve_single_token_arg.\n");

  if (m -> evaled)
    return NULL;

  /* Look for a class object. */

  msg_frame_top = message_frame_top_n (parser_frame_ptr ());

  if ((result_object = class_object_search (m -> name, FALSE)) != NULL) {
    /***/
    if (str_eq (source_filename (), "assocdel1.c")) {
      if (str_eq (get_fn_name (), "main")) {
	printf ("TRUE: m : %s, result : %s\n", M_NAME(m), result_object -> __o_name);
      }
    }
    /***/
    if (str_eq (source_filename (), "assocdel2.c")) {
      if (str_eq (get_fn_name (), "main")) {
	printf ("TRUE: m : %s, result : %s\n", M_NAME(m), result_object -> __o_name);
      }
    }
    /***/
    m -> obj = result_object;
    /*
     *  Add a value method only if necessary.
     */
    if (!ctrlblk_pred)
      default_method (&ms);
    return result_object;
  }

  /*
   *   Check for a method parameter reference and create an
   *   object if necessary.
   */
  if (interpreter_pass == method_pass) {
    int i;
    METHOD *method;

    method = new_methods[new_method_ptr + 1] -> method;

    if (rcvr_class_obj_arg && IS_CONSTRUCTOR(method)) {
      if (method -> params[0]) {
	if (strcmp (M_NAME(messages[message_ptr]),
		    method->params[0]->name) &&
	    strstr (rcvr_class_obj_arg->__o_name, method->selector))
	  warning (messages[message_ptr], 
		   "Argument, \"%s,\" does not match constructor parameter.",
		   M_NAME(messages[message_ptr]));
      } else {
	warning (messages[message_ptr], 
		 "Argument, \"%s,\" does not match constructor parameter.",
		 M_NAME(messages[message_ptr]));
      }
    }

    if (method && IS_METHOD (method)) {
      for (i = 0; i < method -> n_params; i++) {
	if (!strcmp (m -> name, method -> params[i] -> name) &&
	    !(method->params[i]->attrs & PARAM_C_PARAM)) {
	  if (fn_tmpl_eval_arg || rcvr_method -> varargs) {
	    /* 
	     *  Function templates have their args evaluated
	     *  by __ctalkEvalExpr at run time.
	     *
	     *  The check for methods with a variable number of
	     *  arguments is temporary... mostly printOn for now.  We
	     *  might need to update this with an actual check if the
	     *  parameter is a format argument (at least for now, no
	     *  methods with a variable number of arguments). ...
	     */
	    if (!IS_OBJECT (m -> obj)) {
	      if ((class_object =
		   class_object_search (method -> params[i] -> class,
				      FALSE)) 
		  == NULL) 
		/* An Object works equally well to define a parameter
		   class at this point. */
		strcpy (method -> params[i] -> class, OBJECT_CLASSNAME);
	      result_object =
		create_object_init
		(method -> params[i] -> class,
		 "Object",
		 method -> params[i] -> name,
		 NULLSTR);
	      m -> obj = result_object;
	      save_method_object (result_object);
	      return result_object;
	    }
	  } else {
	    /*
	     *  ... in most other cases, we can snatch
	     *  the argument directly from the argument
	     *  stack.
	     */
	    method_arg_accessor_fn (messages, message_ptr, 
				    method -> n_params - i - 1,
				    null_context, method -> varargs);
	    message_stack_at (argbuf -> start_idx) -> attrs |=
	      OBJ_IS_SINGLE_TOK_ARG_ACCESSOR;
	    message_stack_at (argbuf -> start_idx) -> attr_data =
	      method -> n_params - i - 1;
	    return messages[message_ptr] -> obj;
	  }
	}
      }
    }
  }

  /* 
   *  Look for a global or local object.  
   */

  if ((result_object = get_object (m -> name, NULL)) != NULL) {
    if (IS_CONSTRUCTOR(rcvr_method) &&
	(result_object -> scope == GLOBAL_VAR) &&
	(frame_at (CURRENT_PARSER -> frame) -> scope == LOCAL_VAR)) {
      warning (m, "Object, \"%s,\" shadows a global object.", M_NAME(m));
      return NULL;
    } else {
      m -> obj = result_object;
      /*
       *  The default method should be handled by resolve (),
       *  if there is no method message, so we shouldn't need
       *  to check for it here.
       */
      return result_object;
    }
  }

  /* 
   *  Check for, "self," "super," "return," and 
   *  "returnObjectClass." If they don't translate 
   *  into run time code, they get elided here also.
   */

  if (!strcmp (m -> name, "self") && 
      need_rt_eval (messages, message_ptr)) {
    if ((new_method_ptr == MAXARGS) && !argblk) {
      warning (m, "Receiver, \"self,\" used within a C function.");
    }
    /* I.e., called from eval_arg, so only a subexpression is on 
       the stack. */
    if (messages != m_messages)
      ctrlblk_pred_rt_expr (messages, message_ptr);
  } else {
    if ((result_object = 
	 self_object (messages, message_ptr)) != NULL) {
      m -> obj = result_object;
      default_method (&ms);
      return result_object;
    }
  }

  if (fn_return (messages, message_ptr) == 0)
    return NULL;

  /* Attempt to resolve methods and arguments. */

  /*
   *  Here we have to account for, "for," loop predicates.  Because
   *  they all occur within the same frame, we use the CTRLBLK structure
   *  indexes instead of the frame top.
   */
#define STACK_MSG_FRAME(__idx_exp) ((messages == m_messages) ? \
      N_MESSAGES : \
     ((messages == tmpl_message_stack ()) ? \
      N_MESSAGES : \
     ((messages == fn_message_stack ()) ? \
      P_MESSAGES : \
     ((messages == method_buf_message_stack ()) ? \
      P_MESSAGES : \
     ((messages == c_message_stack ()) ? \
      N_MESSAGES : \
     ((messages == var_message_stack ()) ? \
      N_MESSAGES : \
     (__idx_exp)))))))

  if (ctrlblk_pred) {
    if (C_CTRL_BLK -> stmt_type == stmt_for) {
      if (for_init)
	msg_frame_top = STACK_MSG_FRAME(C_CTRL_BLK -> for_init_start);
      if (for_term)
	msg_frame_top = STACK_MSG_FRAME(C_CTRL_BLK -> for_term_start);
      if (for_inc)
	msg_frame_top = STACK_MSG_FRAME(C_CTRL_BLK -> for_inc_start);
    } else {
      /*
       *  NOTE - It may be necessary to add conditions for other
       *  special loop cases here.
       */
      msg_frame_top = 
	STACK_MSG_FRAME(message_frame_top_n (parser_frame_ptr ()));
    }
  } else {
    if (ctrlblk_blk) {
      /*
       *  If we have a code block without braces, then set 
       *  msg_frame_top to blk_start_ptr.  If the control block
       *  is delimited by braces, we can simply use the frame 
       *  top.
       */
      if (C_CTRL_BLK -> braces == 0) {
	msg_frame_top = STACK_MSG_FRAME(C_CTRL_BLK -> blk_start_ptr);
      } else {
	msg_frame_top = 
	  STACK_MSG_FRAME(message_frame_top_n (parser_frame_ptr ()));
      }
    } else {
      msg_frame_top = 
	STACK_MSG_FRAME(message_frame_top_n (parser_frame_ptr ()));
    }
  }

  /*
   *  The prev_*_ptr's are initialized at the start
   *  of the function.  Unless the function is using
   *  message_stack () (which it isn't currently), 
   *  these aren't needed.
   */
  if (message_ptr < N_MESSAGES) {
    prev_label_ptr = scanback (messages, message_ptr + 1,
			       msg_frame_top, LABEL);
    prev_method_ptr = scanback (messages, message_ptr + 1,
				msg_frame_top, METHODMSGLABEL);
    prev_tok_ptr = prevlangmsg (messages, message_ptr);
  }

  /*  If the label isn't resolved as an object above, it could be
   *  interpreted as an argument or method of an object in the
   *  predicate.
   */

  if (ctrlblk_blk) {
    if ((message_ptr <= C_CTRL_BLK -> blk_start_ptr) &&
	(message_ptr >= C_CTRL_BLK -> blk_end_ptr) &&
	(prev_label_ptr <= C_CTRL_BLK -> pred_start_ptr) &&
	(prev_label_ptr >= C_CTRL_BLK -> pred_end_ptr))
      return NULL;
  }

  if (prev_label_ptr != ERROR)
    m_prev_label = messages[prev_label_ptr];
  if (prev_method_ptr != ERROR)
    m_prev_method = messages[prev_method_ptr];

  if (prev_label_ptr == ERROR) {
    if ((message_ptr > N_MESSAGES) && 
	((prev_label_ptr = scanback (messages, message_ptr + 1,
 				    msg_frame_top, C_KEYWORD)) != -1)) {
      return NULL;
    } else {
      OBJECT_CONTEXT c;
      /* 
       *  Issue a warning only if we can determine that the 
       *  token appears in an object context.
       */
      if (is_c_identifier (m -> name) || is_c_keyword (m -> name))
	return NULL;
      c = object_context (messages, message_ptr);
      if ((c == argument_context) || (c == receiver_context))
	warning (m, "\"%s\" has unknown receiver.\n", m -> name);
      return NULL;
    }
  }

  /* The most recent reference is an object. */

  if (prev_label_ptr != ERROR && 
      ((prev_method_ptr == ERROR) || 
       (prev_label_ptr < prev_method_ptr))) {
    if (IS_OBJECT (m_prev_label -> obj)) {

      /* The previous reference is a class object. */
      if (IS_CLASS_OBJECT(m_prev_label->obj)) {
	if (((method = get_instance_method 
	      (m_prev_label, m_prev_label -> obj, m -> name, 
	       ERROR, FALSE)) != NULL) ||
	    ((method = get_class_method
	      (m_prev_label, m_prev_label -> obj, m -> name, 
	       ERROR, FALSE)) != NULL)) {
	  m -> receiver_msg = m_prev_label;
	  m -> receiver_obj = m_prev_label -> obj;
	  m -> tokentype = METHODMSGLABEL;
	} else {
	  sprintf (errbuf, METHOD_ERR_FMT, M_NAME(m), 
		   m_prev_label -> obj -> __o_classname);
	  __ctalkExceptionInternal (m, undefined_method_x, errbuf,0);
	  return NULL;
	}
      } else {
	
	/* 
	 *   The previous label refers to an object instance. 
	 *   Determine if this label refers to a method or 
	 *   an instance variable.  Give precedence to instance
	 *   variables.
	 */

	if ((class_object = m_prev_label -> obj -> __o_class) == NULL) {
	  if ((class_object = 
	       get_class_object (m_prev_label -> obj -> __o_classname))
	      == NULL)
	    error (m_prev_label, "Unknown class \"%s.\"", 
		   m_prev_label ->obj -> __o_classname);
	}
	/*
	 *   We still have to make sure that the method (at message_ptr)
	 *   immediately follows the receiver (at prev_label_ptr).
	 */
	if (prev_tok_ptr == prev_label_ptr) {
	  if (((instancevar_object = 
		__ctalkGetInstanceVariable (m_prev_label -> obj, m -> name,
					     FALSE)) != NULL) ||
	      ((method = get_instance_method (m, 
					      class_object, 
					      m -> name, 
					      ERROR, TRUE)) != NULL)) {
	    m -> receiver_msg = m_prev_label;
	    m -> receiver_obj = m_prev_label -> obj;
	    m -> tokentype = METHODMSGLABEL;
	  } else {
	    sprintf (errbuf, METHOD_ERR_FMT, M_NAME(m),
		     m_prev_label -> obj -> __o_classname);
	    __ctalkExceptionInternal (m, undefined_method_x, errbuf,0);
	    return NULL;
	  }
	}
      }
    }

    /* If there are no arguments, the method call code must be output
	immediately, because it also marks the ctalk statement not to
	be output.  Otherwise, the parser waits for the next pass, and
	the ctalk statement is output, which is non-parsable by GCC.
    */
    if ((next_label_ptr = nextlangmsg (messages, message_ptr))
	!= ERROR) {
      if (m -> tokentype == METHODMSGLABEL)
	method_call (message_ptr);
      return M_VALUE_OBJ (m_prev_label);
    }

    return m_prev_label -> obj;
  }
  
  /* The most recent reference is a method.  If not evaled, set the
     method args and call the method.
  */

  m -> obj = result_object;
  m -> receiver_obj = m_prev_label -> obj;

  if (prev_method_ptr < prev_label_ptr) {
    if ((method = get_instance_method 
	 (m_prev_label, m_prev_label -> obj, 
	  m_prev_method -> name, ERROR, TRUE)) != NULL) {

      if (!m -> evaled && !m_prev_label -> evaled) 
	method_args (method, message_ptr, &arglist_end,
		     m -> attrs & TOK_IS_PRINTF_ARG);
    }    
  }  

  /* ERROR here means that the statement has already been evaluated. */

  if (!m_prev_method -> evaled) {
    if (method_call (prev_method_ptr) == ERROR)
      return NULL;
  }

  return result_object;

}

bool eval_arg_cvar_reg = false;

OBJECT *eval_arg (METHOD *method, OBJECT *rcvr_class, ARGSTR *argbuf,
		  int main_stack_idx) {
  int i, i_2, j,
    start_stack,
    stack_end,
    n_brackets,
    parent_error_line = 0,   /* Avoid warnings.                      */
    parent_error_column = 0,
    prev_tok_idx,
    next_tok_idx,
    close_paren_idx;
  int arg_has_leading_unary = FALSE,  /* Avoid warnings.             */
    leading_unary_idx;
  int lookahead, lookback;
  int agg_var_end_idx;
  int _p;
  int first_label_idx = -1;
  OBJECT *arg_obj,           /* Object from argument.                */
    *fn_arg_obj,             /* Object of C function arg expression. */
    *class_obj;
  MESSAGE *m_arg;
  int have_struct_member = FALSE;
  int have_sizeof_expr = FALSE;
  int sizeof_expr_needs_rt_eval = FALSE;
  int typecast_end = -1;
  CVAR *cvar = NULL;
  CVAR *struct_defn = NULL;  /* Avoid a warning.  Maybe struct member
				lookups should be refactored further. */
  int have_constant_arg = FALSE;
  METHOD *arg_method, *arg_method_test;
  OBJECT *tmp_object = NULL;
  OBJECT *next_object = NULL;
  bool have_struct_op = false;
  MSINFO msi;
  char buf[MAXMSG];
  bool math_subexpr_rewrite = false;
  char *ptr_math_subexpr_rewrite_expr, math_subexpr_rewrite_expr[MAXMSG];

  if (argbuf -> start_idx == argbuf -> end_idx) {
    if ((arg_obj = eval_constant_arg (argbuf -> start_idx)) != NULL) {
      return arg_obj;
    }
  } else if ((argbuf -> start_idx == argbuf -> end_idx + 1) &&
	     (method -> n_params > 0)) {
    /* The argument start is a unary minus, not a subtraction. */
    if ((arg_obj = resolve_unary_minus_arg_0 (message_stack (),
					      argbuf -> start_idx,
					      argbuf -> end_idx))
	!= NULL) {
      return arg_obj;
    }
  }

  arg_class = arg_null;
  arg_obj = fn_arg_obj = class_obj = NULL;
  msi.stack_start = start_stack = m_message_ptr;

  
  /* Don't use tokenize_no_error here - we need to have the
     error line in order to trap exceptions.  */
  if (isspace (*(argbuf -> arg))) {
    trim_leading_whitespace (argbuf -> arg, buf);
    stack_end = tokenize (method_message_push, buf);
  } else {
    stack_end = tokenize (method_message_push, argbuf -> arg);
  }

  eval_arg_cvar_reg = false;
  msi.stack_ptr = stack_end;
  msi.messages = m_messages;

  error_line = parent_error_line;
  error_column = parent_error_column;

  /*
   *  Single token arguments.
   */
  if (start_stack == (stack_end + 1)) {

    m_arg = m_messages[start_stack];

    switch (m_arg -> tokentype)
      {
      case LABEL:
	if ((arg_obj = resolve_single_token_arg
	     (method, m_messages, start_stack, rcvr_class_obj,
	      main_stack_idx, argbuf))
	    == NULL) {
	  if (message_stack_at (main_stack_idx) -> tokentype ==
	      METHODMSGLABEL) { /***/
	    METHOD *m_1;
	    MESSAGE *m_main_stack;
	    m_main_stack = message_stack_at (main_stack_idx);
	    if (((m_1 = get_instance_method (m_main_stack, rcvr_class,
					   m_main_stack -> name, 0, FALSE))
		 != NULL) ||
		((m_1 = get_class_method (m_main_stack, rcvr_class,
					m_main_stack -> name, 0, FALSE))
		 != NULL)) {
	      if (m_1 -> n_params == 0) {
		arg_class = arg_compound_method;
		goto arg_evaled;
	      }
	    }
	  }
	  /*
	   *  Check for a constructor method - if shadowing a variable,
	   *  the object will not be registered yet.
	   */
	  constructor_arg_shadows_c_variable_warning (message_stack (),
						      main_stack_idx,
						      m_messages,
						      start_stack,
						      method);
						      
	  if ((((cvar = get_local_local_cvar (m_arg -> name)) != NULL) ||
	       ((cvar = get_global_var (m_arg -> name)) != NULL)) &&
	      IS_CVAR(cvar) &&
	      !IS_CONSTRUCTOR (method)) {
	    if (argblk && (cvar -> scope & LOCAL_VAR)) {
	      /* until we have an example of a CVAR expression,
		 check for duplicate output for single token args only. */
	      if (argbuf -> start_idx == argbuf -> end_idx) {
		MESSAGE *main_stack_msg;
		main_stack_msg = message_stack_at (argbuf -> start_idx);
		if (!(main_stack_msg -> attrs & TOK_CVAR_REGISTRY_IS_OUTPUT)) {
		  translate_argblk_cvar_arg_1 (m_arg, cvar);
		  main_stack_msg -> attrs |= TOK_CVAR_REGISTRY_IS_OUTPUT;
		  eval_arg_cvar_reg = true;
		}
	      } else {
		translate_argblk_cvar_arg_1 (m_arg, cvar);
		eval_arg_cvar_reg = true;
	      }
	    } else {
	      register_c_var (message_stack_at (main_stack_idx), 
			      m_messages, start_stack,
			      &agg_var_end_idx);
	      eval_arg_cvar_reg = true;
	    }
	    arg_class = arg_c_var_tok;
	    if ((arg_obj = create_arg_EXPR_object (argbuf)) != NULL) {
	      arg_obj -> scope |= CVAR_VAR_ALIAS_COPY;
	      save_method_object (arg_obj);
	    }
	  } else {
	    CFUNC *cfunc;

	    /* Check for a single-token C function as an argument. 
	       In that case, we can output the label verbatim, and it's
	       valid C. */
	    if ((cfunc = get_function (M_NAME(m_arg))) != NULL) {
	      arg_obj = create_arg_CFUNCTION_object (argbuf -> arg);
	      arg_obj -> attrs |= OBJECT_IS_FN_ARG_OBJECT;
	      goto arg_evaled;
	    }
	    if (!IS_DEFINED_LABEL(M_NAME(m_arg))) {
	      if ((prev_tok_idx = prevlangmsg (message_stack (),
					       main_stack_idx))
		  != ERROR) {
		MESSAGE *m_main_prev = message_stack_at (prev_tok_idx);
		MESSAGE *m_main = message_stack_at (main_stack_idx);
		if (!IS_CONSTRUCTOR_LABEL(M_NAME(m_main_prev)) &&
		    !IS_CONSTRUCTOR_LABEL(M_NAME(m_main)) &&
		    /* AssociativeArray keys can be constructed objects. */
		    !str_eq (rcvr_class -> __o_name,
			     "AssociativeArray") &&
		    !member_of_method_return_class (method, m_arg) &&
		    !is_method_parameter (m_messages, start_stack) &&
		    !get_instance_method (m_main, rcvr_class,
					  M_NAME(m_arg), ANY_ARGS, FALSE)) {
		  warning (m_main,
			   "Identifier, \"%s\" not resolved.", M_NAME(m_arg));
		  
		}
	      }
	    }
	    /*
	     * Undefined object and not a C variable.  If the method
	     * still requires an argument, create if necessary
	     * as the receiver's class.
	     * (We might still need to use the method's parameter class,
	     *  but it hasn't been necessary so far.)
	     */
	    if (method -> n_args < method -> n_params) {
	      if (!method -> params[method -> n_args]) {
		arg_obj = 
		  create_object_init (rcvr_class -> __o_name, 
				      ((rcvr_class -> __o_superclassname) ?
				       rcvr_class -> __o_superclassname : NULL),
				      m_arg -> name, m_arg -> name);
		arg_obj -> __o_class = rcvr_class;
		arg_obj -> __o_superclass = rcvr_class -> __o_superclass;
		arg_obj -> instancevars -> __o_class = rcvr_class;
		arg_obj -> instancevars -> __o_superclass = rcvr_class -> __o_superclass;
 	      } 
	      arg_class = arg_obj_tok;
	    } else {
	      /*
	       *  If the method doesn't require an argument, check 
	       *  for another method with a receiver of the same class, 
	       *  then backtrack and create an expression.  Unfortunately,
	       *  we still need the index of the argument on the main 
	       *  stack.
	       *
	       *  TO DO - We should easily be able to check if the next
	       *  message is a valid method, both here and below, for the 
	       *  multi-token argument lists.
	       */
	      if ((method -> n_args == 0) && (method -> n_params == 0)) {
		/* TODO - the return value check might not be needed
		     here - both clauses are the same right now. */
		if (strcmp (method -> returnclass, "Any")) {

		  int __idx, __arg_main_stack_idx;

		  __arg_main_stack_idx = 
		    nextlangmsg (message_stack (), main_stack_idx);
		  for (__idx = main_stack_idx; 
		       __idx <= P_MESSAGES;
		       __idx++) {
		    if (message_stack_at (__idx) == 
			message_stack_at (main_stack_idx) -> receiver_msg) 
		      break;
		  }
		  arg_obj = arg_expr_object_2 (message_stack (),
					       __idx,
					       __arg_main_stack_idx);
		  /*
		   *  Used when we return to method_call.
		   */
		  message_stack_at (__idx) -> value_obj = arg_obj;
		  arg_class = arg_rt_expr;

		} else { /* if (strcmp (method -> returnclass, "Any")) ... */

		  /* TODO - This might need a warning message. */

		  int __idx, __arg_main_stack_idx;

		  __arg_main_stack_idx = 
		    nextlangmsg (message_stack (), main_stack_idx);
		  for (__idx = main_stack_idx; 
		       __idx <= P_MESSAGES;
		       __idx++) {
		    if (message_stack_at (__idx) == 
			message_stack_at (main_stack_idx) -> receiver_msg) 
		      break;
		  }

		  arg_obj = arg_expr_object_2 (message_stack (),
					       __idx,
					       __arg_main_stack_idx);
		  /*
		   *  Used when we return to method_call.
		   */
		  message_stack_at (__idx) -> value_obj = arg_obj;
		  arg_class = arg_rt_expr;


		} /* if (strcmp (method -> returnclass, "Any")) ... */
	      }
	    }
	  }
	}
	break;
      default:
	warning (message_stack_at (main_stack_idx),
		 "Unknown argument token: \"%s.\"",
		 M_NAME(m_messages[start_stack]));
	break;
      } /* switch */

    if (!arg_obj) { 
      if (!get_local_var (M_NAME(m_arg))  && !get_global_var (M_NAME(m_arg))
	  && !str_eq (argbuf -> arg, "self") &&
	  !find_library_include (argbuf -> arg, FALSE) &&
	  !is_collection_initializer (main_stack_idx) &&
	  !str_eq (M_NAME(message_stack_at (main_stack_idx)), "new") &&
	  !is_OBJECT_member (argbuf -> arg) &&
	  (M_TOK(m_arg) != PATTERN) &&
	  !method_has_param_pfo_def (method) &&
	  /*. i.e., parameter of the enclosing method that we're parsing. */
	  !is_new_method_parameter (argbuf -> arg) &&
	  !is_method_of_rcvr_class (rcvr_class, argbuf -> arg))
	warning (message_stack_at (main_stack_idx),
		 "Unknown identifier, \"%s.\"", argbuf -> arg);
      arg_obj = create_arg_EXPR_object (argbuf);
      arg_class = arg_obj_expr;
    }

    goto arg_evaled;
  } /* if (start_stack == (stack_end + 1)) */

  /*
   *  Arguments with multiple tokens.
   */

  if (arg_is_constant (m_messages, start_stack, stack_end))
    have_constant_arg = TRUE;
  else
    have_constant_arg = FALSE;

  prev_tok_idx = -1;
  n_brackets = 0;
  have_struct_op = strstr (argbuf -> arg, "->") ||
			   strstr (argbuf -> arg, ".");
  arg_c_fn_term_ptr = 0;
  
  for (i = start_stack; i > stack_end; i--) {

    m_arg = m_messages[i];

    if ((m_arg -> tokentype == NEWLINE) ||
	(m_arg -> tokentype == WHITESPACE))
      continue;

      /*
       *  Check for a typecast expression in a sizeof () operator.
       */
    if ((M_TOK(m_messages[i]) == SIZEOF) && have_constant_arg) {
      arg_obj = create_arg_EXPR_object (argbuf);
      arg_class = arg_const_expr;
      if ((next_tok_idx = nextlangmsg (m_messages, i)) != ERROR) {
	if (M_TOK(m_messages[next_tok_idx]) == OPENPAREN) {
	  if ((next_tok_idx = nextlangmsg (m_messages, next_tok_idx)) != ERROR){
	    int __t;
	    if (is_typecast_expr (m_messages, next_tok_idx, &__t)) {
	      have_sizeof_expr = TRUE;
	      /* strcpy is necessary here in order to avoid
		 overlapping buffers */
	      strcpy (arg_obj -> __o_name,
		      c_sizeof_expr_wrapper (rcvr_class, method, arg_obj));
	      save_method_object (arg_obj);
	      store_arg_object (method, arg_obj);
	      arg_class = arg_c_sizeof_expr;
	    }
	  }
	}
      }
      goto arg_evaled;
    }
    if (M_TOK(m_messages[i]) == OPENPAREN) {
      if (is_typecast_expr (m_messages, i, &typecast_end)) {
	if (i == start_stack) {
	  if (fn_arg_expression_call == 0) {
	    int _a_s;
	    leading_typecast_indexes (main_stack_idx,
				      &argbuf -> typecast_start_idx,
				      &argbuf -> typecast_end_idx);
	    argbuf -> leading_typecast = true;
	    /* the tokens are the same regardless of which stack
	       we parse them on.... */
	    argbuf -> typecast_expr = collect_tokens
	      (m_messages, i, typecast_end);
	    /* 
	       Note that we still maintain the original expression,
	       which includes the typecast for now, because there aren't
	       many output functions that need to regard the typecast...
	       fmt_c_expr_store_arg_call is the only fn that
	       uses the leading typecast's class, if any, so far. 
	    */
	    _a_s = nextlangmsg (message_stack (),
				argbuf -> typecast_end_idx);
	    if (M_TOK(message_stack_at (_a_s)) == LABEL) {
	      /* 
		 This is not anywhere near comprehensive yet...
		 we really only want the argument if the
		 entire expression is not part of the cast,
		 like ((void *)0).
	      */
	      argbuf -> start_idx = _a_s;
	    }
	  } else {
	    /* There shouldn't be many cases where this is
	       used.  parser_pass catches nearly all of
	       the typecasts that precede function argumnts
	       and outputs them without any processing. */
	    argbuf -> leading_typecast = true;
	    argbuf -> typecast_start_idx = i;
	    argbuf -> typecast_end_idx = typecast_end;
	    argbuf -> typecast_expr = collect_tokens
	      (m_messages, i, typecast_end);
	  }
	}
	i = typecast_end;
	continue;
      } else if (is_class_typecast_2 (&msi, i)) {
	if ((close_paren_idx = match_paren (msi.messages, i,
					    msi.stack_ptr)) != ERROR) {
	  int rcvr_lookahead, class_tok_idx;
	  if ((rcvr_lookahead = nextlangmsg (m_messages, close_paren_idx))
	      != ERROR) {
	    if ((class_tok_idx = nextlangmsg (m_messages, i))
		!= ERROR) {
	      m_messages[class_tok_idx] -> obj =
		get_class_object (M_NAME(m_messages[class_tok_idx]));
	      m_messages[rcvr_lookahead] -> receiver_msg =
		m_messages[class_tok_idx];
	      m_messages[class_tok_idx] -> attrs |=
		TOK_IS_TYPECAST_EXPR;
	      argbuf -> class_typecast = true;
	      argbuf -> class_typecast_start_idx =
		argbuf -> start_idx - (msi.stack_start - i);
	      argbuf -> class_typecast_end_idx = argbuf -> start_idx -
		(msi.stack_start - close_paren_idx);
	      i = close_paren_idx;
	      continue;
	    }
	  }
	}
      }
    }
    
    if (M_TOK(m_arg) == LABEL) {
      if (first_label_idx == -1) {
	first_label_idx = i;
      }
      if (str_eq (M_NAME(m_arg), "self")) {
	if ((interpreter_pass != method_pass) &&
	    (interpreter_pass != expr_check) && !argblk) {
	  self_outside_method_error (m_messages, i);
	}
	m_arg -> attrs |= TOK_SELF;
      } else if (str_eq (M_NAME(m_arg), "super")) {
	m_arg -> attrs |= TOK_SUPER;
      } else if (is_class_typecast (&msi, i)) {
	int i_2, cast_lookahead;
	int cast_start_l, cast_end_l = stack_end, i_3;
	leading_typecast_indexes (main_stack_idx,
				  &argbuf -> typecast_start_idx,
				  &argbuf -> typecast_end_idx);
	argbuf -> leading_typecast = true;
	argbuf -> typecast_expr =
	  collect_tokens (message_stack (), argbuf -> typecast_start_idx,
			  argbuf -> typecast_end_idx);
	for (i_2 = i - 1; i_2 > stack_end; i_2--) {
	  if (M_TOK(m_messages[i_2]) == CLOSEPAREN) {
	    cast_end_l = i_2;
	    cast_lookahead = nextlangmsg (m_messages, i_2);
	    m_messages[cast_lookahead] -> receiver_msg = m_arg;
	    m_messages[cast_lookahead] -> receiver_msg -> obj =
	      get_class_object (M_NAME(m_messages[i]));
	    break;
	  }
	}
	if (m_messages[start_stack] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	  /* 
	     Handle a case where we have a class typecast preceded
	     by a prefix operator; i.e.,

	       *(Integer *)myInt myInstanceVar;

	     We do this my setting the cast expression tokens
	     to spaces (on the main stack!), but keeping the 
	     cast attributes and objects.

	   */
	  cast_start_l = prevlangmsg (m_messages, i);
	  for (i_2 = argbuf -> typecast_start_idx,
		 i_3 = cast_start_l;
	       (i_2 >= argbuf -> typecast_end_idx) &&
		 (i_3 >= cast_end_l); i_2 --, i_3--) {
	    message_stack_at (i_2) -> name[0] = ' ';
	    message_stack_at (i_2) -> name[1] = '\0';
	    message_stack_at (i_2) -> tokentype = WHITESPACE;
	    m_messages[i_3] -> name[0] = ' ';
	    m_messages[i_3] -> name[1] = '\0';
	    m_messages[i_3] -> tokentype = WHITESPACE;
	  }
	  argbuf -> start_idx = prevlangmsg (message_stack (),
					     argbuf -> typecast_start_idx);
	  argbuf -> leading_typecast = false;
	}
	continue;
      }
    } else if (IS_C_UNARY_MATH_OP (M_TOK(m_arg)) || M_TOK(m_arg) == MINUS) {
      if (prefix_method_attr_arg (m_messages, i)) {
	if (!unary_op_attributes (m_messages, i, &arg_has_leading_unary, 
				  &leading_unary_idx)) {
	  continue;
	}
      } else if (M_TOK(m_arg) == INCREMENT || M_TOK(m_arg) == DECREMENT) {
	postfix_following_method_warning (m_messages, i);
      }
    }

    /*
     *  C function.  
     *
     *  Search for functions in the following order.
     *
     *    1. A function in the source text.
     *    2. A cvar declaration that is a prototype, 
     *       typedef, or other declaration, generally
     *       from a header file.
     *    3. A C library function for which a CFunction
     *       class method exists.
     *
     *  If the function is part of a 
     *  complex expression, issue a warning (for now).
     */
    if (M_TOK(m_arg) == LABEL) {
      if (IS_DEFINED_FN (M_NAME (m_arg))) {
	char t_fn_buf[MAXMSG], t_fn_buf_2[MAXMSG];
	int fn_arg_start, fn_arg_end;

	fn_check_expr (m_messages, i, start_stack, main_stack_idx);

	if (interpreter_pass != expr_check)
	  fn_arg_obj = fn_arg_expression (rcvr_class, method, m_messages, i);
	
	if ((fn_arg_start = scanforward (m_messages, i, stack_end, 
					 OPENPAREN)) == ERROR)
	  error (m_arg, "Parser error.");
	if ((fn_arg_end = match_paren (m_messages, fn_arg_start,
				       stack_end)) == ERROR)
	  error (m_arg, "Parser error.");

	if (fn_arg_expression_call == 0) {
	  /* Translate the local stack indexes to the main stack tokens'
	     indexes. */
	  arg_c_fn_terms[arg_c_fn_term_ptr].messages = message_stack ();
	  arg_c_fn_terms[arg_c_fn_term_ptr].start = argbuf -> start_idx -
	    (start_stack - i);
	  arg_c_fn_terms[arg_c_fn_term_ptr].end = argbuf -> start_idx -
	    (start_stack - fn_arg_end);
	  ++arg_c_fn_term_ptr;
	}
      
	if ((i < start_stack) && (m_messages[start_stack] -> evaled == 0)) {
	  /* if we have a leading constant term or something, include it here.
	     (for most cases, we might get away with just using start_stack, but
	     we should really handle this with something like
	     format_obj_lval_fn_expr in c_rval.c). */
	  toks2str (m_messages, start_stack, fn_arg_start, t_fn_buf);
	} else {
	  toks2str (m_messages, i, fn_arg_start, t_fn_buf);
	}
	if (fn_arg_obj) {
	  /* TODO - This can be streamlined a lot, and expanded
	     to look for objects in the fn's argument list. */
	  if (fn_arg_obj -> __o_name[0] == '\0') {
	    strcpy (t_fn_buf_2, t_fn_buf);
	    toks2str (m_messages, fn_arg_start-1, fn_arg_end, t_fn_buf);
	    strcat (t_fn_buf_2, t_fn_buf);
	  } else {
	    strcatx (t_fn_buf_2, t_fn_buf, fn_arg_obj -> __o_name,
		     ")", NULL);
	  }
	  delete_object (fn_arg_obj);
	} else {
	  strcatx (t_fn_buf_2, t_fn_buf, ")", NULL);
	}
	fn_arg_obj = create_arg_CFUNCTION_object (t_fn_buf_2);

	if (fn_arg_expression_call > 0) {
	  /* i.e., it's a recursive call from 
	     fn_arg_expression for a single argument. */
	  if ((i == start_stack) &&
	      libc_fn_needs_writable_args (M_NAME(m_arg))) {
	    arg_class = arg_c_writable_fn_expr;
	  } else {
	    arg_class = arg_c_fn_expr;
	  }
	  save_method_object (fn_arg_obj);
	  goto arg_evaled;
	} else {
	  if ((i == start_stack) &&
	      libc_fn_needs_writable_args (M_NAME(m_arg))) {
	    save_method_object (fn_arg_obj);
	    arg_class = arg_c_writable_fn_expr;
	    goto arg_evaled;
	  } else {
	    /* for now, just check if we need to register C variables
	       for trailing tokens */
	    for (i_2 = i - 1; i_2 > stack_end; i_2 --) {
	      if (M_TOK(m_messages[i_2]) == LABEL) {
		if (((cvar = get_global_var (m_messages[i_2] -> name)) != NULL) ||
		    ((cvar = get_local_var (m_messages[i_2] -> name)) != NULL)) {
		  register_c_var (message_stack_at (main_stack_idx),
				  m_messages, i_2,
				  &agg_var_end_idx);
		}
	      }
	    }
	    arg_class = arg_c_fn_expr;
	    save_method_object (fn_arg_obj);
	    goto arg_evaled;
	  }
	}
      } /* if (IS_DEFINED_FN (M_NAME (m_arg))) */

      /*
       *  An aggregate variable.
       */
      if (((cvar = get_global_var (m_arg -> name)) != NULL) ||
	  ((cvar = get_local_var (m_arg -> name)) != NULL)) {
	/*
	 *  Check for leading unary operators and find out
	 *  if they're overloaded.  Also below when looking 
	 *  for objects.
	 */
	if ((lookback = prevlangmsg (m_messages, i)) != ERROR) {
	  if (m_messages[lookback] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	    if (prefix_op_is_sizeof (m_messages, i, start_stack)) {
	      have_sizeof_expr = TRUE;
	      if (sizeof_arg_needs_rt_eval (m_messages, i)) {
		sizeof_expr_needs_rt_eval = TRUE;
	      }
	    }
	  }
	}
	/*
	 *  If the variable label token is the final token 
	 *  of the argument, then simply register it and break, 
	 *  because the reference cannot be an aggregate type.
	 */
	if ((lookahead = nextlangmsg (m_messages, i)) != ERROR) {
	  switch (m_messages[lookahead] -> tokentype)
	    {
	    case PERIOD:
	      if (cvar -> type_attrs & CVAR_TYPE_TYPEDEF) {
		if ((struct_defn = get_typedef (cvar -> type)) == NULL) {
		  char errtxt[MAXLABEL];
		  strcatx (errtxt, "C variable, \"", cvar -> name,
			   ",\" is not a struct or union", NULL);
		  __ctalkExceptionInternal (m_messages[lookahead], 
					    invalid_operand_x, errtxt,0);
		}
	      }
	      
	      /*
	       *  If the struct is a declaration but not a definition,
	       *  then find the definition.
	       */
	      if (cvar -> members == NULL) {
		if ((struct_defn == NULL) &&
		    ((struct_defn = 
		      struct_defn_from_struct_decl (cvar -> type)) 
		     == NULL)) {
		  warning (m_arg, "Undefined struct or union %s.", 
			   cvar -> name);
		} else {
		  register_c_var (message_stack_at (main_stack_idx),
				  m_messages, i,
				  &agg_var_end_idx);
		  arg_obj = create_arg_EXPR_object (argbuf);
		  if (arg_class != arg_null) arg_class = arg_c_var_expr;
		  goto arg_evaled;
		}
	      } else {
		register_c_var (message_stack_at (main_stack_idx),
				m_messages, i,
				&agg_var_end_idx);
		arg_obj = create_arg_EXPR_object (argbuf);
		if (arg_class != arg_null)  arg_class = arg_c_var_expr;
		/* As a possible trailing token, this needs more work-up,
		   when we have code examples. */
		if (!strchr (argbuf -> arg, '?'))
		  goto arg_evaled;
	      }
	      break;
	    case DEREF:
	      if (cvar -> type_attrs & CVAR_TYPE_TYPEDEF) {
		if (((cvar = 
		      get_typedef (cvar -> type)) != NULL) ||
		    ((cvar = 
		      get_typedef (cvar -> qualifier)) != NULL)) {
		  /*
		   *  Check here in case there's another level
		   *  of type derivation.
		   */
		  if (str_eq (cvar -> qualifier, "struct")) {
		    if (((struct_defn = 
			  get_global_struct_defn (cvar -> type)) == NULL) &&
			((struct_defn = 
			  get_local_struct_defn (cvar -> type)) == NULL)) {
		      warning (m_arg, "Undefined struct or union %s.", 
			       cvar -> name);
		    }
		  } else {
		    struct_defn = cvar;
		  }
		}
	      } else {
		if (!cvar -> members) {
		  if ((struct_defn =
		       struct_defn_from_struct_decl (cvar -> type))
		      == NULL) {
		    warning (m_arg, "Undefined struct or union %s.", 
			     cvar -> name);
		  }
		} else {
		  struct_defn = cvar;
		}
	      }

	      if (struct_defn) {
		register_c_var (message_stack_at (main_stack_idx),
				m_messages, i, &agg_var_end_idx);
		arg_obj = create_arg_EXPR_object (argbuf);
		eval_arg_cvar_reg = true;
		if (arg_class != arg_null)  arg_class = arg_c_var_expr;
		goto arg_evaled;
	      } else {
		warning (m_arg, "Syntax error.");
		if ((lookahead = nextlangmsg (m_messages, lookahead)) == ERROR)
		  error (m_arg, "Parser error.");
	      }
	      break;
	    case ARRAYOPEN:
	      ++n_brackets;
	      if (ctrlblk_pred) {
		int end_idx_ret, operand_idx_ret;
		if (!c_first_operand (m_messages,
				      i,
				      &end_idx_ret,
				      &operand_idx_ret)) {
		  register_c_var (message_stack_at (main_stack_idx),
				  m_messages, i, &agg_var_end_idx);
		} else {
		  /* if the cvar is to be included literally, then skip over
		     any tokens that an aggregate variable would have also. */
		  i = operand_idx_ret;
		  continue;
		}
	      } else {
		register_c_var (message_stack_at (main_stack_idx),
				m_messages, i, &agg_var_end_idx);
		eval_arg_cvar_reg = true;
	      }
	      break;
	    case ARRAYCLOSE:
	      --n_brackets;
	      break;
	    default:
	      if (!n_brackets) {
		if (!have_sizeof_expr ||
		    (have_sizeof_expr && sizeof_expr_needs_rt_eval)) {
		  if (ctrlblk_pred) {
		    int end_idx_ret, operand_idx_ret;
		    if (!c_first_operand (m_messages,
					  i,
					  &end_idx_ret,
					  &operand_idx_ret)) {
		      register_c_var (message_stack_at (main_stack_idx),
				      m_messages, i,
				      &agg_var_end_idx);
		      eval_arg_cvar_reg = true;
		    } else {
		      /* in case the var is an aggregate type - see the
			 comment above. */
		      i = operand_idx_ret;
		      continue;
		    }
		  } else {
		    if (argblk) {
		      if (cvar && cvar -> scope & GLOBAL_VAR) {
			/* See the comment below, in the next
			   clause */
			argblk = false;
			register_c_var (message_stack_at (main_stack_idx),
					m_messages, i, &agg_var_end_idx);
			argblk = true;
		      }
		    } else {
		      register_c_var (message_stack_at (main_stack_idx),
				      m_messages, i, &agg_var_end_idx);
		    }
		    eval_arg_cvar_reg = true;
		  }
		}
	      }
	      break;
	    }
	} else {
	  /*
	   *  No further tokens - register the var, but first make sure that
	   *  it isn't an aggregate expression that we've already registered.
	   *  Some systems can register the member names independently of 
	   *  the struct definition.
	   */
	  if ((_p = prevlangmsg(m_messages, i)) != ERROR) {
	    if ((M_TOK(m_messages[_p]) != PERIOD) && (M_TOK(m_messages[_p]) != DEREF)) {
	      if (cvar && interpreter_pass != expr_check &&  !ctrlblk_pred) {
		/* CVARs in control structures get registered at
		   various places from control.c and ifexpr.c, 
		   and fmt_register_argblk_c_vars_* ...  */
		/***/
		if (!method_expr_is_c_fmt_arg (message_stack (),
					       main_stack_idx,
					       P_MESSAGES,
					       get_stack_top
					       (message_stack ()))) {
		  /* ... this CVAR gets registered in stdarg_fmt_arg_expr. */
		  register_c_var (message_stack_at (main_stack_idx),
				  m_messages, i, &agg_var_end_idx);
		  /***/
		  eval_arg_cvar_reg = true;
		  if (arg_class != arg_null) arg_class = arg_c_var_expr;
		} else {
		  if (interpreter_pass != expr_check) {
		    register_c_var (message_stack_at (main_stack_idx),
				    m_messages, i, &agg_var_end_idx);
		    /* but not necessarily this one, so we check for this
		       attribute later - remember, anything that follows
		       will use the main stack index. */
		    message_stack_at (argbuf -> start_idx -
				      (N_MESSAGES - i)) -> attrs
		      |= TOK_CVAR_REGISTRY_IS_OUTPUT;
		    if (arg_class != arg_null) arg_class = arg_c_var_expr;
		    eval_arg_cvar_reg = true;
		  }
		}
	      }
	      if (cvar && cvar -> scope & GLOBAL_VAR) {
		if (argblk) {
		  /* ... unless it's a global var, so here we still
		     need to write the CVAR because it's included in the
		     complete expression, but we don't need to work it 
		     into the block's CVARTAB, so we just work around the 
		     CVARTAB entry stuff. */
		  argblk = false;
		  register_c_var (message_stack_at (main_stack_idx),
				  m_messages, i, &agg_var_end_idx);
		  argblk = true;
		  /***/
		  eval_arg_cvar_reg = true;
		  if (arg_class != arg_null) arg_class = arg_c_var_expr;
		}
	      }
 	    }
	  }
	}
      } else { /*  if (((cvar = get_global_var (m_arg -> name)) != NULL) ... */
	if (TOK_HAS_CLASS_TYPECAST(m_arg)) {
	  int _a_s, _i_2;
	  if (!IS_OBJECT(m_arg -> obj)) {
	    /* this function calls save_method_object. */
	    m_arg -> obj = instantiate_self_object_from_class
	      (m_arg -> receiver_msg -> obj);
	  }
	  if (!(m_messages[start_stack] -> attrs & TOK_IS_PREFIX_OPERATOR)) {
	    /* handled above - see comments there */
	    leading_typecast_indexes (main_stack_idx,
				      &(argbuf -> typecast_start_idx),
				      &(argbuf -> typecast_end_idx));
	    if ((_a_s = nextlangmsg (message_stack (),
				     argbuf -> typecast_end_idx)) != ERROR)
	      argbuf -> start_idx = _a_s;
	    for (_i_2 = argbuf -> typecast_start_idx; 
		 _i_2 >= argbuf -> typecast_end_idx; --_i_2) {
	      ++message_stack_at (_i_2) -> evaled;
	      ++message_stack_at (_i_2) -> output;
	    }
	  }
	}

	if (((tmp_object = get_object (M_NAME(m_arg), NULL)) != NULL) ||
	    ((tmp_object = get_class_object (M_NAME(m_arg))) != NULL) ||
	    ((tmp_object = get_method_param_obj (m_messages, i)) != NULL)) {
	  if (IS_CLASS_OBJECT(tmp_object)) {
	    if (is_class_typecast (&msi, i)) {
	      set_arg_object (m_arg, tmp_object);
	      continue;
	    }
	  }
	  set_arg_object (m_arg, tmp_object);

	  /*
	   *  Check here also for leading unary operators and find out
	   *  if they're overloaded.
	   */
	  if (prev_tok_idx != -1) {
	    if (m_messages[prev_tok_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	      if (prefix_op_is_sizeof (m_messages, i, start_stack)) {
		have_sizeof_expr = TRUE;
		if (sizeof_arg_needs_rt_eval (m_messages,i)) {
		  sizeof_expr_needs_rt_eval = TRUE;
		}
	      }
	    }

	    if (IS_CONSTANT_TOK(M_TOK(m_messages[prev_tok_idx]))) {
	      prev_constant_tok_warning (m_messages, prev_tok_idx,
					 i, stack_end + 1);
	    }
	    
	  }
	  if (((next_tok_idx = nextlangmsg (m_messages, i)) != ERROR) &&
	      !METHOD_ARG_TERM_MSG_TYPE(m_messages[next_tok_idx])) {
	    /* Look for case of <object> <object> */
	    if (((next_object = 
		  get_instance_variable (M_NAME(m_messages[next_tok_idx]),
					 tmp_object -> CLASSNAME,
					 FALSE)) != NULL) ||
		/*
		 *  If we're looking for a class variable in a
		 *  <label> <label> expression, then the
		 *  previous label needs to be the name of a
		 *  class object, so we use, "tmp_object -> __o_name"
		 *  as the class name when checking for the 
		 *  class variable.
		 */
		  ((next_object = 
		    get_class_variable (M_NAME(m_messages[next_tok_idx]),
					tmp_object -> __o_name,
					FALSE)) != NULL) ||
		  ((next_object =
		    obj_or_param_after_method_label
		    (m_messages, i, next_tok_idx))  != NULL)) {
		m_messages[next_tok_idx] -> receiver_obj = tmp_object;
		if (complex_var_or_method_message (tmp_object,
						   m_messages,
						   next_tok_idx))
		  i = stack_end;
	      } else {
		MESSAGE *_m_next;
	      
		/* Look for case of <object> <method> */
		/* Try to do a fixup right here, before we need
		   to parse this in the cache output. */
	      
		/* Make sure we have a receiver object. */
		if (IS_OBJECT(tmp_object)) {

		  _m_next = m_messages[next_tok_idx];
		  arg_method = NULL;
		  
		  if (((arg_method_test = get_instance_method 
			(_m_next, tmp_object, M_NAME(_m_next), 
			 ANY_ARGS, FALSE)) 
		       != NULL) ||
		      ((arg_method_test = get_class_method 
			(_m_next, tmp_object, M_NAME(_m_next),
			 ANY_ARGS, FALSE)) 
		       != NULL)) {
		    char _output_buf[MAXMSG];
		    /* for now, only check argument count match for 
		       the right-hand of = methods */
		    if (method -> name[0] == '=' && method -> name[1] == '\0') {
		      /* also for now, only count commas if the argument
			 list doesn't contain parens */
		      int n_args;
		      if ((n_args = args_in_arglist (m_messages, next_tok_idx,
						     stack_end,
						     arg_method_test,
						     main_stack_idx,
						     argbuf -> arg)) >= 0) {
			if (((arg_method = get_instance_method 
			      (_m_next, tmp_object, M_NAME(_m_next), 
			       n_args, FALSE)) 
			     == NULL) &&
			    ((arg_method = get_class_method 
			      (_m_next, tmp_object, M_NAME(_m_next),
			       n_args, FALSE)) 
			     == NULL)) {
			  warning (message_stack_at (main_stack_idx),
				   "Argument mismatch:\n\n\t%s\n\n"
				   "Expression contains %d arguments, but "
				   "method %s (class %s) requires %d "
				   "arguments.",
				   argbuf -> arg, n_args,
				   arg_method_test -> name,
				   arg_method_test -> rcvr_class_obj
				   -> __o_name,
				   arg_method_test -> n_params);
			}
		      }
		    }
		    fmt_fixup_info (_output_buf,
				    _m_next -> error_line,
				    arg_method_test -> selector,
				    TRUE);

		    fileout (_output_buf, 0, FRAME_START_IDX);

		  } else { /* if (((arg_method = get_instance_method  ... */

		    if (arg_has_leading_unary) {
		      m_message_stack = TRUE;
		      prefix_method_expr_b (m_messages, first_label_idx);
		      arg_obj = eval_arg_obj;
		      save_method_object (arg_obj);
		      m_message_stack = FALSE;
		    } else {
		      
		      /* Look for a method prototype. */
		
		      /*
		       *  Don't warn (yet) if the expression is in an 
		       *  argument block -- many of the expressions still 
		       *  need to be deferred until run time.
		       */
		      if ((M_TOK(m_messages[next_tok_idx]) == LABEL) && !argblk) {
			if (!arg_method && !next_object) {
			  if (IS_OBJECT(m_messages[i] -> obj)) {
			    if (!is_method_proto 
				(m_messages[i] -> obj, 
				 M_NAME(m_messages[next_tok_idx])) &&
				!is_method_name
				(M_NAME(m_messages[next_tok_idx])) &&
				!is_instance_variable_message
				(m_messages, next_tok_idx) &&
				!have_user_prototype
				(m_messages[i] -> obj -> CLASSNAME,
				 M_NAME(m_messages[next_tok_idx]))) {
			      warning (message_stack_at (main_stack_idx),
				       "Undefined label, \"%s.\" (Previous"
				       " object, \"%s\", class, \"%s.\")", 
				       M_NAME(_m_next),
				       M_NAME(m_messages[i]),
				       m_messages[i] -> obj -> __o_classname);
			    }
			  } else {
			    warning (message_stack_at (main_stack_idx),
				     "Undefined label, \"%s.\"", M_NAME(_m_next));
			  }
			}
		      }
		    } /* if (arg_has_leading_unary) ... */

		  } /* if (((arg_method = get_instance_method  ... */

		} /* if (IS_OBJECT(tmp_object) ... */
	      }

	    } /* if (((next_tok_idx = nextlangmsg (m_messages, i)) != ERROR) &&...*/

	  } else { /* if ((tmp_object = get_object (M_NAME(m_arg), NULL))... */
	    /*
	     *  TO DO - At some point, we might need to start
	     *  backtracking here if there are a lot of unresolved
	     *  args from struct elements...
	     *  strstr () is here, but this should be refactored out
	     *  along with the struct member lookup code in register_c_var.
	     */
	    if (have_struct_op) {
	      if (!is_struct_member_tok (m_messages, i) &&
		  !have_struct_member) {
		if (!is_method_name (M_NAME(m_messages[i]))) {
		  if (!IS_OBJECT (m_messages[i]->receiver_obj) ||
		      (!get_instance_variable (M_NAME(m_messages[i]),
					       m_messages[i] -> receiver_obj -> 
					       __o_class -> __o_name, FALSE) &&
		       !get_class_variable (M_NAME(m_messages[i]),
					    m_messages[i] -> receiver_obj ->
					    __o_class -> __o_name, FALSE))) {
					    
					    
		    /*
		     *  For now, if a label isn't defined anywhere, generate an 
		     *  exception.  
		     */
		    
		    if (!label_is_defined (m_messages, i)) {
		      __ctalkExceptionInternal (m_messages[i], 
						undefined_label_x, 
						M_NAME(m_messages[i]),
						0);
		    }
		  }
		}
	      } else {
		have_struct_member = TRUE;
	      }
	    } else {
	      if ((method -> n_args == 0) && (method -> n_params == 0)) {
		if (strcmp (method -> returnclass, "Any")) {
		  int __idx, __main_stack_ptr,
		    __arg_main_stack_idx_begin,
		    __arg_main_stack_idx_end;
		  __main_stack_ptr = get_stack_top (message_stack ());
		  __arg_main_stack_idx_begin = 
		    nextlangmsg (message_stack (), main_stack_idx);

		  /* 
		   *  method_arglist_limit* doesn't work well with 
		   *  a request for 0 parameters.
		   */

		  if (((__arg_main_stack_idx_end = 
			scanforward (message_stack (), 
				     __arg_main_stack_idx_begin,
				     __main_stack_ptr, SEMICOLON))
		       == ERROR) &&
		      ((__arg_main_stack_idx_end = 
			scanforward (message_stack (), 
				     __arg_main_stack_idx_begin,
				     __main_stack_ptr, ARRAYCLOSE))
		       == ERROR) &&
		      ((__arg_main_stack_idx_end = 
			scanforward (message_stack (), 
				     __arg_main_stack_idx_begin,
				     __main_stack_ptr, CLOSEBLOCK))
		       == ERROR))
		    __arg_main_stack_idx_end = 
		      __arg_main_stack_idx_begin;
				  
		  for (__idx = main_stack_idx; 
		       __idx <= P_MESSAGES;
		       __idx++) {
		    if (message_stack_at (__idx) == 
			message_stack_at (main_stack_idx) -> receiver_msg) 
		      break;
		  }
		  arg_obj = arg_expr_object_2 (message_stack (), __idx,
					       __arg_main_stack_idx_end + 1);
		  /*
		   *  Used when we return to method_call.
		   */
		  message_stack_at (__idx) -> value_obj = arg_obj;
		  arg_class = arg_rt_expr;
		  goto arg_evaled;
		} else {
		  if (ambiguous_operand_throw (m_messages, i, method)) {
		    ambiguous_arg_start_idx =
		      nextlangmsg (message_stack (), main_stack_idx);
		    ambiguous_arg_end_idx = ambiguous_arg_start_idx - 
		      (start_stack - (m_message_ptr+1));
		    for (j = start_stack; j > stack_end; j--)
		      delete_message (method_message_pop ());
		    return NULL;
		  } else {
		    /*
		     *  TODO - also check secondary, etc. method and argument
		     *  labels.
		     */
		    if (i == start_stack && 
			!label_is_defined (m_messages, i)) {
		      warning (message_stack_at (main_stack_idx),
			       "Undefined label, \"%s\".", 
			       M_NAME(m_messages[i]));
		    } else {
		      if (m_messages[i] -> attrs & TOK_SELF ||
			  m_messages[i] -> attrs & TOK_SUPER) {
			if ((new_method_ptr == MAXARGS) && !argblk) {
			  warning (m_messages[i], 
				   "Label, \"%s,\" used within a C function.",
				   M_NAME(m_messages[i]));
			}
		      }
		    }
		  }
		}
	      } else { /* if ((method -> n_args == 0) ... */
		/*
		 *  I don't like this being hardcoded, but
		 *  we have to peek into objects sometimes.
		 */
		if (str_eq (method -> name, "->")) {
		  if (is_OBJECT_member (M_NAME(m_messages[i])))
		    continue;
		} else {
		  /* check for instance and class variable typos,
		     like the var on its own */
		  instancevar_wo_rcvr_warning
		    (m_messages, i, (first_label_idx == -1),
		     main_stack_idx);
		}
	      }  /* if ((method -> n_args == 0) ... */
	    }
	}  /* if ((tmp_object = get_object (M_NAME(m_arg), NULL))... */
      }  /*  if (((cvar = get_global_var (m_arg -> name)) != NULL) ... */
    } else if (M_TOK(m_arg) == LITERAL) {
      /* Do a token concatenation here in case the preprocessor didn't
	 because it looked like a doc string.  Do it on both this
	 stack and the main stack. */
      char *concat_buf;
      int tok_prev, concat_length, i_2;
      next_tok_idx = i;
    concat_next_tok:
      tok_prev = next_tok_idx;
      if ((next_tok_idx = nextlangmsg (m_messages, next_tok_idx)) != ERROR) {
	if (M_TOK(m_messages[next_tok_idx]) == LITERAL) {
	  goto concat_next_tok;
	}
      }
      if (tok_prev < i) {
	/* this gives us a buffer large enough for all the tokens */
	concat_buf = collect_tokens (m_messages, i, tok_prev);
	/* leave room for new quotes */
	concat_length = strlen (concat_buf) + 5;
	if (concat_length >= MAXLABEL) {
	  resize_message (m_messages[i], concat_length + 1);
	}

	*concat_buf = '\0';
	for (i_2 = i; i_2 >= tok_prev; i_2--) {
	  if (M_TOK(m_messages[i_2]) == LITERAL) {
	    TRIM_LITERAL(m_messages[i_2] -> name);
	    strcat (concat_buf, m_messages[i_2] -> name);
	    m_messages[i_2] -> name[0] = ' ';
	    m_messages[i_2] -> name[1] = '\0';
	    m_messages[i_2] -> tokentype = WHITESPACE;
	  }
	}
	strcatx (m_messages[i] -> name, "\"", concat_buf, "\"", NULL);

	for (i_2 = argbuf -> start_idx; i_2 >= argbuf -> end_idx;
	     --i_2) {
	  if (M_TOK(message_stack_at (i_2)) == LITERAL) {
	    next_tok_idx = i_2;
	  concat_next_tok_main_stack:
	    tok_prev = next_tok_idx;
	    if ((next_tok_idx = nextlangmsg (message_stack (), next_tok_idx))
		!= ERROR) {
	      if (M_TOK(message_stack_at (next_tok_idx)) == LITERAL) {
		goto concat_next_tok_main_stack;
	      } else if ((M_TOK(message_stack_at (next_tok_idx)) ==
			 WHITESPACE) ||
			 (M_TOK(message_stack_at (next_tok_idx)) ==
			  NEWLINE)) {
		goto concat_next_tok_main_stack;
	      } else {
		if (i_2 > next_tok_idx) {
		  int i_3;
		  if (concat_length >= MAXLABEL) {
		    resize_message (message_stack_at (i_2), concat_length + 1);
		  }
		  memset (concat_buf, 0, concat_length);
		  for (i_3 = i_2; i_3 >= tok_prev; i_3--) {
		    if (M_TOK(message_stack_at (i_3)) == LITERAL) {
		      TRIM_LITERAL(message_stack_at (i_3) -> name);
		      strcat (concat_buf, message_stack_at (i_3) -> name);
		      message_stack_at (i_3) -> name[0] = ' ';
		      message_stack_at (i_3) -> name[1] = '\0';
		      message_stack_at (i_3) -> tokentype = WHITESPACE;
		    }
		  }
		  strcatx (message_stack_at (i_2) -> name, "\"",
			   concat_buf, "\"", NULL);
		}
		i_2 = next_tok_idx; 
	      }
	    }
	  }
	}

	__xfree (MEMADDR(concat_buf));
      }
    } else if (M_TOK(m_arg) == OPENPAREN) {
      /* check for a class cast */

      int close_paren_idx, rcvr_idx, deref_prefix_op_idx;
      if ((next_tok_idx = nextlangmsg (m_messages, i)) != ERROR) {
	if (is_class_typecast (&msi, next_tok_idx)) {
	  if ((close_paren_idx = match_paren (m_messages,
					      i, stack_end)) != ERROR) {
	    class_cast_receiver_scan (m_messages, stack_end, i,
				      &rcvr_idx, &deref_prefix_op_idx);
	    if ((m_messages[rcvr_idx] -> obj =
		 get_class_object (M_NAME(m_messages[next_tok_idx])))
		!= NULL) {
	      m_arg -> receiver_msg = m_messages[next_tok_idx];
	      m_messages[next_tok_idx] -> obj =
		m_messages[rcvr_idx] -> obj;
	      m_messages[next_tok_idx] -> attrs |= TOK_IS_CLASS_TYPECAST;
	    } else if (is_cached_class (M_NAME(m_messages[next_tok_idx]))) {
		/* what? */
	    }
	    prev_tok_idx = i;
	    i = rcvr_idx;
	    continue;
	  }
	}
      }

    } else if (IS_C_BINARY_MATH_OP (M_TOK(m_arg))) {/* if (M_TOK(m_arg) == OPENPAREN) */
      /* If we have an expression like this:
       *
       *   (<operand 1 expr>) / (<operand 2 expr>)
       *
       * enclosed in parens and consisting only of label tokens, 
       * rewrite to this.
       *
       *   <operand 1 expr> / <operand 2 expr>
       *
       *
       * This is because subexprs enclosed in parentheses get only
       * one result value in eval_arg, and in this case there would be
       * two subexpr results, so we remove the parens, and evaluate
       * the expression by operator precedence only. Aargh.
       */
      if (((prev_tok_idx = prevlangmsg (m_messages, i)) != ERROR) &&
	  ((next_tok_idx = nextlangmsg (m_messages, i)) != ERROR)) {
	if (M_TOK(m_messages[prev_tok_idx]) == CLOSEPAREN &&
	    M_TOK(m_messages[next_tok_idx]) == OPENPAREN) {
	  int op1_start_idx, op2_end_idx, i_2;
	  op1_start_idx = match_paren_rev (m_messages, prev_tok_idx,
					   msi.stack_start);
	  op2_end_idx = match_paren (m_messages, next_tok_idx,
				     stack_end);
	  if (M_TOK(m_messages[op1_start_idx]) == OPENPAREN &&
	      M_TOK(m_messages[op2_end_idx]) == CLOSEPAREN) {
	    if (op1_start_idx == msi.stack_start &&
		op2_end_idx == (msi.stack_ptr + 1)) {
	      for (i_2 = op1_start_idx - 1; i_2 > prev_tok_idx; i_2--) {
		if (!M_ISSPACE(m_messages[i_2]) &&
		    M_TOK(m_messages[i_2]) != LABEL &&
		    M_TOK(m_messages[i_2]) != METHODMSGLABEL) {
		  goto subexpr_rewrite_done;
		}
	      }
	      for (i_2 = next_tok_idx - 1; i_2 > op2_end_idx; i_2--) {
		if (!M_ISSPACE(m_messages[i_2]) &&
		    M_TOK(m_messages[i_2]) != LABEL &&
		    M_TOK(m_messages[i_2]) != METHODMSGLABEL) {
		  goto subexpr_rewrite_done;
		}
	      }
	      math_subexpr_rewrite = true;
	      ptr_math_subexpr_rewrite_expr =
		collect_tokens (m_messages, msi.stack_start,
				msi.stack_ptr);
	      for (i_2 = 0; ptr_math_subexpr_rewrite_expr[i_2]; i_2++) {
		if (ptr_math_subexpr_rewrite_expr[i_2] == '(' ||
		    ptr_math_subexpr_rewrite_expr[i_2] == ')') {
		  ptr_math_subexpr_rewrite_expr[i_2] = ' ';
		}
	      }
	      trim_leading_whitespace (ptr_math_subexpr_rewrite_expr,
				       math_subexpr_rewrite_expr);
	      __xfree (MEMADDR(ptr_math_subexpr_rewrite_expr));
	    }
	  }
	}
      }
    subexpr_rewrite_done:
      prev_tok_idx = i;
    } /* if (M_TOK(m_arg) == OPENPAREN) */
    prev_tok_idx = i;

    if (i == start_stack) {
      if (M_TOK(m_messages[i]) == LABEL) {
	if (!is_method_parameter (m_messages, i) &&
	    !get_object (M_NAME(m_messages[i]), NULL) &&
	    (!IS_CVAR(cvar) &&
	     !get_local_var (M_NAME(m_messages[i])) &&
	     !get_global_var (M_NAME(m_messages[i]))) &&
	    !is_OBJECT_member (M_NAME(m_messages[i]))) {
	  /* Lots of fns don't return an object during an expr_check.
	     Something like a method parameter shouldn't trigger a
	     warning here so we need another check. Aaargh. */
	  undefined_arg_first_label_warning (m_messages, start_stack,
					     stack_end,
					     main_stack_idx, 
					     tmp_object, cvar);
	}
      }
    }
  } /*   for (i = start_stack, n_brackets = 0; i > stack_end; i--) */
      
  /*
   *  A ctalk expression.  Create an, "Expr," object.
   */
  if (arg_has_leading_unary) {
    /*
     *  Trimming a leading unary from the argument expression should
     *  not be necessary in this context, but we'll leave it in should 
     *  a case occur later.
     */
    if (have_sizeof_expr) {
      if (sizeof_expr_needs_rt_eval) {
	elide_inc_or_dec_prefix (argbuf);
	elide_inc_or_dec_postfix (argbuf);
	arg_obj = create_arg_EXPR_object (argbuf);
	save_method_object (arg_obj);
      } else {
	elide_inc_or_dec_prefix (argbuf);
	elide_inc_or_dec_postfix (argbuf);
	arg_obj = create_arg_EXPR_object (argbuf);
	/* strcpy is necessary here because we need to avoid
	   overlapping buffers */
	strcpy (arg_obj -> __o_name,
		c_sizeof_expr_wrapper (rcvr_class, method, arg_obj));
	store_arg_object (method, arg_obj);
	save_method_object (arg_obj);
	arg_class = arg_c_sizeof_expr;
	goto arg_evaled;
      }
    } else {
      elide_inc_or_dec_prefix (argbuf);
      elide_inc_or_dec_postfix (argbuf);
      arg_obj = create_arg_EXPR_object (argbuf);
      save_method_object (arg_obj);
    }
  } else {
    elide_inc_or_dec_prefix (argbuf);
    elide_inc_or_dec_postfix (argbuf);
    if (math_subexpr_rewrite) {
      char *argbuf_save = argbuf -> arg;
      argbuf -> arg = math_subexpr_rewrite_expr;
      arg_obj  = create_arg_EXPR_object_2 (argbuf);
      argbuf -> arg = argbuf_save;
    } else {
      arg_obj  = create_arg_EXPR_object (argbuf);
    }
    save_method_object (arg_obj);
  }

  if (arg_class != arg_null)
    arg_class = arg_obj_expr;

 arg_evaled:
  for (j = start_stack; j > stack_end; j--) {
    delete_message(method_message_pop ());
  }

  if (fn_arg_obj)
    return fn_arg_obj;
  else
    return arg_obj;
}
