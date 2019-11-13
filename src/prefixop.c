/* $Id: prefixop.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2019 Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "bufmthd.h"

/* Declared in control.c                    */
extern bool ctrlblk_pred, ctrlblk_blk, ctrlblk_else_blk;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern char input_source_file[FILENAME_MAX]; /* Declared in main.c.   */
extern int nolinemarker_opt;

#define _MPUTCHAR(__m,__c) (__m)->name[0] = (__c); (__m)->name[1] = '\0';

static int set_unary_op_attributes (MESSAGE_STACK messages, int idx,
				    int stack_top_idx) {
  int i;
  /*
   *  As called from eval_arg (), for each token of an argument, this 
   *  loop should be sufficient, since the stack is the m_message 
   *  stack.  If necessary to call the function from elsewhere, the loop 
   *  should limit the scan to the parser frame.
   */
  for (i = idx; 
       IS_C_UNARY_MATH_OP (M_TOK(messages[i])) && (i > stack_top_idx); 
       i--) {
    if (M_TOK(messages[i]) == SIZEOF) {
      int __sizeof_arg_start, __sizeof_arg_end;
      if ((__sizeof_arg_start = nextlangmsg (messages, i)) != ERROR) {
	if (M_TOK(messages[__sizeof_arg_start]) == OPENPAREN) {
	  if ((__sizeof_arg_end = match_paren (messages, __sizeof_arg_start,
					       stack_top_idx)) != ERROR) {
	    if (M_TOK(messages[__sizeof_arg_end]) == CLOSEPAREN) {
	      messages[i] -> attrs |= TOK_IS_PREFIX_OPERATOR;
	      messages[__sizeof_arg_start] -> attrs |= TOK_IS_PREFIX_OPERATOR;
	      messages[__sizeof_arg_end] -> attrs |= TOK_IS_PREFIX_OPERATOR;
	    } else { /* if ((__sizeof_arg_end =  */
	      error 
		(messages[idx], 
		 "set_unary_op_attributes: sizeof () : parser error.");
	    } /* if ((__sizeof_arg_end =  */
	  } else { /* if ((__sizeof_arg_end =  */
	    error 
	      (messages[idx], "set_unary_op_attributes: sizeof () : parser error.");
	  }  /* if ((__sizeof_arg_end =  */
	} else { /* if (M_TOK(messages[__sizeof_arg_start]) == OPENPAREN) */
	  error 
	    (messages[idx], "set_unary_op_attributes: sizeof () : parser error.");
	} /* if (M_TOK(messages[__sizeof_arg_start]) == OPENPAREN) */
      } else { /*if ((__sizeof_arg_start = nextlangmsg (messages, idx)) != ERROR)*/
	error (messages[idx], "set_unary_op_attributes: sizeof () : parser error.");
      } /*if ((__sizeof_arg_start = nextlangmsg (messages, idx)) != ERROR)*/
    } else {
      messages[idx] -> attrs |= TOK_IS_PREFIX_OPERATOR;
    }
  }
  return SUCCESS;
}

int unary_op_attributes (MESSAGE_STACK messages, int op_idx, 
		     int *op_tokentype, int *prefix_start_idx) {
  int stack_start_idx, stack_top_idx, prev_tok_idx;
  stack_start_idx = stack_start (messages);
  stack_top_idx = get_stack_top (messages);
  
  if (messages[op_idx] -> attrs & TOK_IS_PREFIX_OPERATOR)
    return SUCCESS;

  if (op_idx == stack_start_idx) {
    set_unary_op_attributes (messages, op_idx, stack_top_idx);
    *op_tokentype = M_TOK(messages[op_idx]);
    *prefix_start_idx = op_idx;
    return SUCCESS;
  }

  if ((prev_tok_idx = prevlangmsg (messages, op_idx)) != ERROR) {
    if (messages[prev_tok_idx] -> attrs & TOK_IS_PREFIX_OPERATOR)
      return SUCCESS;
  }

  if (prev_tok_idx != ERROR) {
    if (IS_C_OP_TOKEN_NOEVAL (M_TOK(messages[prev_tok_idx]))) {
      set_unary_op_attributes (messages, op_idx, stack_top_idx);
      *op_tokentype = M_TOK(messages[op_idx]);
      *prefix_start_idx = op_idx;
      return SUCCESS;
    }
  }

  return ERROR;
}

/* We have to use a separate buffer in order to avoid overlapping
   buffers. */
char *c_sizeof_expr_wrapper (OBJECT *rcvr_obj, METHOD *m, OBJECT *arg_obj) {
  static char buf[MAXMSG];
  if (m -> n_args > m -> n_params) {
    _warning ("c_sizeof_expr_wrapper: method, \"%s,\" wrong number of arguments.",
	      m -> name);
    return arg_obj -> __o_name;
  } else {
    sprintf (buf, "__ctalk_arg (\"%s\", \"%s\", %d, __ctalkCIntToObj (%s));\n",
	     rcvr_obj->__o_name, m->name, 
	     m -> n_params, arg_obj -> __o_name);
    return buf;
  }
}

int sizeof_arg_needs_rt_eval (MESSAGE_STACK messages, int arg_start_idx) {
  int prev_tok_idx;
  int typecast_end_idx;
  if ((prev_tok_idx = prevlangmsg (messages, arg_start_idx)) != ERROR) {
    if (is_typecast_expr (messages, arg_start_idx, &typecast_end_idx))
      return FALSE;
    if (need_rt_eval (messages, arg_start_idx))
      return TRUE;
    if ((M_TOK(messages[prev_tok_idx]) == OPENPAREN) &&
	get_object (M_NAME(messages[arg_start_idx]), NULL))
      return TRUE;
  }
  return FALSE;
}


int n_th_arg_CVAR_has_unary_inc_prefix = -1;
int n_th_arg_CVAR_has_unary_dec_prefix = -1;
int n_th_arg_CVAR_has_unary_inc_postfix = -1;
int n_th_arg_CVAR_has_unary_dec_postfix = -1;

/*
 *  Only used for ++<CVAR> and --<CVAR> when they occur in method 
 *  arguments.  Only called by register_c_var () in method.c.
 *  Setting n_th_arg_CVAR_has_unary_inc|dec_prefix is a necessary
 *  evil when we need to construct the actual argument call a bit
 *  later somewhere in method.c.
 */
int prefix_arg_cvar_expr_registration (MESSAGE_STACK messages, 
				       int cvar_label_idx, CVAR *v,
				       int prefix_tok, 
				       int prefix_tok_idx) {
  char output_buf[MAXMSG], linemarker_buf[MAXMSG], *n,
    expr_buf[MAXMSG], cvar_buf[MAXMSG];
  if (ctrlblk_pred) {
    ctrlblk_register_c_var 
      (fmt_register_c_method_arg_call 
       (v, M_NAME(messages[cvar_label_idx]), FRAME_SCOPE, output_buf),
       FRAME_START_IDX);
  } else {
    strcatx (expr_buf, M_NAME(messages[prefix_tok_idx]),
	     M_NAME(messages[cvar_label_idx]), ";\n", NULL);
    strcatx (output_buf, "\n{\n", expr_buf,
	     fmt_register_c_method_arg_call 
	     (v, M_NAME(messages[cvar_label_idx]), FRAME_SCOPE, cvar_buf),
	     "\n}\n", NULL);
    if (!nolinemarker_opt) {
      fmt_line_info (linemarker_buf, messages[cvar_label_idx] -> error_line, 
		     input_source_file, 1, FALSE);
      /*
       *  If outputting at the start of a frame, the line marker should
       *  not need an extra trailing newline of its own, so the line number 
       *  doesn't register with the compiler as the source line + 1.
       *  So trim the linemarker string.  
       *  Later functions, in method.c and output.c, check 
       *  n_th_arg_CVAR_has_unary_inc|dec_prefix to determine
       *  if we've inserted a statement without a newline.
       */
      if ((n = rindex (linemarker_buf, '\n')) != NULL)
	*n = '\0';
      strcatx2 (output_buf, linemarker_buf, NULL);
    }
    fileout (output_buf, 0, FRAME_START_IDX);
    if (M_TOK(messages[prefix_tok_idx]) == INCREMENT)
      n_th_arg_CVAR_has_unary_inc_prefix = TRUE;
    if (M_TOK(messages[prefix_tok_idx]) == DECREMENT)
      n_th_arg_CVAR_has_unary_dec_prefix = TRUE;
  }
  return SUCCESS;
}

int postfix_arg_cvar_expr_registration (MESSAGE_STACK messages, 
					int cvar_label_idx, CVAR *v,
					int postfix_tok, 
					int postfix_tok_idx) {
  char output_buf[MAXMSG], linemarker_buf[MAXMSG], *n,
    expr_buf[MAXMSG], cvar_buf[MAXMSG];
  int prev_tok;
  prev_tok = prevlangmsg (messages, cvar_label_idx);
  if (ctrlblk_pred) {
    ctrlblk_register_c_var 
      (fmt_register_c_method_arg_call 
       (v, M_NAME(messages[cvar_label_idx]), FRAME_SCOPE, output_buf),
       FRAME_START_IDX);
  } else {
    if ((prev_tok != ERROR) && 
	(messages[prev_tok] -> attrs & TOK_IS_PREFIX_OPERATOR)) {
      /* of course, if it's an int *, that's completely different... 
	 so... */
      strcatx (output_buf, fmt_register_c_method_arg_call
	       (v, M_NAME(messages[cvar_label_idx]), FRAME_SCOPE, cvar_buf),
	       NULL);
    } else {
      /* ... We want to limit this as much as possible as a special
	 case and let eval_expr do a writeback at run time */
      strcatx (expr_buf, M_NAME(messages[cvar_label_idx]),
	     M_NAME(messages[postfix_tok_idx]), ";\n", NULL);
      strcatx (output_buf, "\n{\n",
	       fmt_register_c_method_arg_call
	       (v, M_NAME(messages[cvar_label_idx]), FRAME_SCOPE, cvar_buf),
	       expr_buf, "\n}\n", NULL);
    }
    if (!nolinemarker_opt) {
      fmt_line_info (linemarker_buf, messages[cvar_label_idx] -> error_line, 
		     input_source_file, 1, FALSE);
      /*
       *  If outputting at the start of a frame, the line marker should
       *  not need an extra trailing newline of its own, so the line number 
       *  doesn't register with the compiler as the source line + 1.
       *  So trim the linemarker string.  
       *  Later functions, in method.c and output.c, check 
       *  n_th_arg_CVAR_has_unary_inc|dec_prefix to determine
       *  if we've inserted a statement without a newline.
       */
      if ((n = rindex (linemarker_buf, '\n')) != NULL)
	*n = '\0';
      strcatx2 (output_buf, linemarker_buf, NULL);
    }
    fileout (output_buf, 0, FRAME_START_IDX);
    if (M_TOK(messages[postfix_tok_idx]) == INCREMENT)
      n_th_arg_CVAR_has_unary_inc_postfix = TRUE;
    if (M_TOK(messages[postfix_tok_idx]) == DECREMENT)
      n_th_arg_CVAR_has_unary_dec_postfix = TRUE;
  }
  return SUCCESS;
}

/*
 *  Called by method_call ();
 */
int postfix_rcvr_cvar_expr_registration (MESSAGE_STACK messages, 
					int rcvr_idx) {
  char output_buf[MAXMSG], linemarker_buf[MAXMSG], *n,
    expr_buf[MAXMSG], cvar_buf[MAXMSG];
  char aggregate_buf[MAXMSG];
  int postfix_tok_idx, subscript_start_idx, subscript_end_idx, stack_end_idx,
    i, struct_end_idx;
  int open_paren_idx;
  int close_paren_idx;
  CVAR *v, *v_defn, *v_mbr;

  if ((postfix_tok_idx = nextlangmsg (messages, rcvr_idx)) == ERROR)
    return ERROR;
  if ((M_TOK(messages[postfix_tok_idx]) != INCREMENT) &&
      (M_TOK(messages[postfix_tok_idx]) != DECREMENT) && 
      (M_TOK(messages[postfix_tok_idx]) != ARRAYOPEN) &&
      (M_TOK(messages[postfix_tok_idx]) != PERIOD) &&
      (M_TOK(messages[postfix_tok_idx]) != DEREF))
    return ERROR;

  /*
   *  Handle a case where a receiver is enclosed in parentheses.
   */
  open_paren_idx = close_paren_idx = -1;
  if ((i = prevlangmsg (messages, rcvr_idx)) != ERROR) {
    while (M_TOK(messages[i]) == OPENPAREN) {
      open_paren_idx = i;
      if ((i = prevlangmsg (messages, open_paren_idx)) == ERROR)
	return ERROR;
    }
  }
  if (open_paren_idx != -1) {
    close_paren_idx = match_paren (messages, open_paren_idx,
				   get_stack_top (messages));
  }


  if (M_TOK(messages[postfix_tok_idx]) == ARRAYOPEN) {
    stack_end_idx = get_stack_top (messages);
    if ((subscript_end_idx = 
	 is_subscript_expr (messages, rcvr_idx, stack_end_idx)) <= 0)
      return ERROR;
    subscript_start_idx = postfix_tok_idx;
    postfix_tok_idx = nextlangmsg (messages, subscript_end_idx);
    if (((v = get_local_var (M_NAME(messages[rcvr_idx]))) == NULL) &&
	((v = get_global_var (M_NAME(messages[rcvr_idx]))) == NULL))
      return ERROR;
    if (ctrlblk_pred) {
      ctrlblk_register_c_var 
	(fmt_register_c_method_arg_call 
	 (v, M_NAME(messages[rcvr_idx]), FRAME_SCOPE, cvar_buf),
	 FRAME_START_IDX);
    } else { /* if (ctrlblk_pred) */
      toks2str (messages, rcvr_idx, subscript_end_idx, aggregate_buf);
      strcatx (expr_buf, aggregate_buf, M_NAME(messages[postfix_tok_idx]),
	       ";\n", NULL);
      strcatx (output_buf, "\n{\n",
	       fmt_register_c_method_arg_call
	       (v, aggregate_buf, FRAME_SCOPE, cvar_buf),
	       expr_buf, "\n}\n", NULL);
      if (!nolinemarker_opt) {
	fmt_line_info (linemarker_buf, messages[rcvr_idx] -> error_line, 
		       input_source_file, 1, FALSE);
	/*
	 *  See the comment below.
	 */
	if ((n = rindex (linemarker_buf, '\n')) != NULL)
	  *n = '\0';
	strcatx2 (output_buf, linemarker_buf, NULL);
      }
      fileout (output_buf, 0, FRAME_START_IDX);
      for (i = subscript_start_idx; i >= postfix_tok_idx; i--) {
	_MPUTCHAR(messages[i], ' ')
	messages[i] -> tokentype = WHITESPACE;
	messages[i] -> attrs &= !TOK_IS_POSTFIX_OPERATOR;
      }
    } /* if (ctrlblk_pred) */
    return SUCCESS;
  }  /* if (M_TOK(messages[postfix_tok_idx]) == ARRAYOPEN) */

  if ((M_TOK(messages[postfix_tok_idx]) == PERIOD) ||
      (M_TOK(messages[postfix_tok_idx]) == DEREF)) {
    stack_end_idx = get_stack_top (messages);
    if ((struct_end_idx = 
	 struct_end (messages, rcvr_idx, stack_end_idx))
	> 0) {
      if (((v = get_local_var (M_NAME(messages[rcvr_idx]))) == NULL) &&
	  ((v = get_global_var (M_NAME(messages[rcvr_idx]))) == NULL))
	return ERROR;
      char __c[MAXMSG], __c_blk[MAXMSG];
      if ((open_paren_idx != -1) && (close_paren_idx != -1)) {
	toks2str (messages, rcvr_idx, struct_end_idx, __c);
	toks2str (messages, open_paren_idx, close_paren_idx, __c_blk);
	postfix_tok_idx = nextlangmsg (messages, close_paren_idx);
      } else {
	postfix_tok_idx = nextlangmsg (messages, struct_end_idx);
	toks2str (messages, rcvr_idx, struct_end_idx, __c);
	toks2str (messages, rcvr_idx, struct_end_idx, __c_blk);
      }
      if ((v -> attrs & CVAR_ATTR_STRUCT_DECL) &&
	  (v -> members)) {
	if ((v_mbr = struct_member_from_expr_b (messages, rcvr_idx, 
					      struct_end_idx, v))
	    != NULL) {
	  strcatx (expr_buf, __c_blk, M_NAME(messages[postfix_tok_idx]),
		   ";\n", NULL);
	  strcatx (output_buf, "\n{\n",
		   fmt_register_c_method_arg_call
		   (v_mbr, __c, FRAME_SCOPE, cvar_buf),
		   expr_buf, "\n}\n", NULL);
	} else {/* if ((v_mbr = struct_member_from_expr_b  ... */
	  strcatx (expr_buf, __c_blk, M_NAME(messages[postfix_tok_idx]),
		   ";\n", NULL);
	  strcatx (output_buf, "\n{\n",
		   fmt_register_c_method_arg_call
		   (v, __c, FRAME_SCOPE, cvar_buf), expr_buf,
		   "\n}\n", NULL);
	}/* if ((v_mbr = struct_member_from_expr_b ... */
	if (!nolinemarker_opt) {
	  fmt_line_info (linemarker_buf, messages[rcvr_idx] -> error_line, 
			 input_source_file, 1, FALSE);
	  /*
	   *  See the comment above.
	   */
	  if ((n = rindex (linemarker_buf, '\n')) != NULL)
	    *n = '\0';
	  strcatx2 (output_buf, linemarker_buf, NULL);
	}
	fileout (output_buf, 0, FRAME_START_IDX);
	_MPUTCHAR(messages[postfix_tok_idx], ' ')
	messages[postfix_tok_idx] -> tokentype = WHITESPACE;
	messages[postfix_tok_idx] -> attrs &= !TOK_IS_PREFIX_OPERATOR;
	return SUCCESS;
      } else {/* if ((v -> attrs & CVAR_ATTR_STRUCT_DECL)... */
	if ((v -> type_attrs & CVAR_ATTR_STRUCT_TAG) ||
	    (v -> type_attrs & CVAR_ATTR_STRUCT_PTR_TAG) ||
	    (v -> attrs & CVAR_ATTR_STRUCT_TAG) ||
	    (v -> attrs & CVAR_ATTR_STRUCT_PTR_TAG) ||
	    ((v -> attrs & CVAR_ATTR_STRUCT_PTR) &&
	     !v -> members)) {
	  if (((v_defn = get_local_struct_defn (v -> type)) != NULL) ||
	      ((v_defn = get_global_struct_defn (v -> type)) != NULL)) {
	    if ((v_mbr = struct_member_from_expr_b (messages,
						    rcvr_idx,
						    struct_end_idx, 
						    v_defn))
		!= NULL) {
	      strcatx (expr_buf, __c_blk, M_NAME(messages[postfix_tok_idx]),
		       ";\n", NULL);
	      strcatx (output_buf, "\n{\n",
		       fmt_register_c_method_arg_call
		       (v_mbr, __c, FRAME_SCOPE, cvar_buf),
		       expr_buf, "\n}\n", NULL);
	    } else {/*if ((v_mbr = struct_member_from_expr_b ... */
	      strcatx (expr_buf, __c_blk, M_NAME(messages[postfix_tok_idx]),
		       ";\n", NULL);
	      strcatx (output_buf, "\n{\n",
		       fmt_register_c_method_arg_call
		       (v, __c, FRAME_SCOPE, cvar_buf),
		       expr_buf, "\n}\n", NULL);
	    }/* if ((v_mbr = struct_member_from_expr_b... */
	  } else {/* if (((v_defn = ... */
	    strcatx (expr_buf, __c_blk, M_NAME(messages[postfix_tok_idx]),
		     ";\n", NULL);
	    strcatx (output_buf, "\n{\n", 
		     fmt_register_c_method_arg_call
		     (v, __c, FRAME_SCOPE, cvar_buf),
		     expr_buf, "\n}\n", NULL);
	  } /* if (((v_defn = ... */
	  if (!nolinemarker_opt) {
	    fmt_line_info (linemarker_buf, messages[rcvr_idx] -> error_line, 
			   input_source_file, 1, FALSE);
	    /*
	     *  See the comment above.
	     */
	    if ((n = rindex (linemarker_buf, '\n')) != NULL)
	      *n = '\0';
	    strcatx2 (output_buf, linemarker_buf, NULL);
	  }
	  fileout (output_buf, 0, FRAME_START_IDX);
	  _MPUTCHAR(messages[postfix_tok_idx], ' ')
	  messages[postfix_tok_idx] -> tokentype = WHITESPACE;
	  messages[postfix_tok_idx] -> attrs &= !TOK_IS_PREFIX_OPERATOR;
	  return SUCCESS;
	}/* if ((v -> type_attrs & CVAR_ATTR_STRUCT_TAG) ... */
      }/* if ((v -> attrs & CVAR_ATTR_STRUCT_DECL)... */
    } /* if ((struct_end_idx = ... */
  } /* if ((M_TOK(messages[postfix_tok_idx]) == PERIOD) ... */

  if (((v = get_local_var (M_NAME(messages[rcvr_idx]))) == NULL) &&
      ((v = get_global_var (M_NAME(messages[rcvr_idx]))) == NULL))
    return ERROR;

  if (ctrlblk_pred) {
    ctrlblk_register_c_var 
      (fmt_register_c_method_arg_call 
       (v, M_NAME(messages[rcvr_idx]), FRAME_SCOPE, cvar_buf),
       FRAME_START_IDX);
  } else {
    strcatx (expr_buf, M_NAME(messages[postfix_tok_idx]),
	     M_NAME(messages[rcvr_idx]), ";\n", NULL);
    strcatx (output_buf, "\n{\n",
	     fmt_register_c_method_arg_call
	     (v, M_NAME(messages[rcvr_idx]), FRAME_SCOPE, cvar_buf),
	     expr_buf, "\n}\n", NULL);
    if (!nolinemarker_opt) {
      fmt_line_info (linemarker_buf, messages[rcvr_idx] -> error_line, 
		     input_source_file, 1, FALSE);
      /*
       *  If outputting at the start of a frame, the line marker should
       *  not need an extra trailing newline of its own, so the line number 
       *  doesn't register with the compiler as the source line + 1.
       *  So trim the linemarker string.  
       *  Later functions, in method.c and output.c, check 
       *  n_th_arg_CVAR_has_unary_inc|dec_prefix to determine
       *  if we've inserted a statement without a newline.
       */
      if ((n = rindex (linemarker_buf, '\n')) != NULL)
	*n = '\0';
      strcatx2 (output_buf, linemarker_buf, NULL);
    }
    fileout (output_buf, 0, FRAME_START_IDX);
    _MPUTCHAR(messages[postfix_tok_idx], ' ')
    messages[postfix_tok_idx] -> tokentype = WHITESPACE;
    messages[postfix_tok_idx] -> attrs &= !TOK_IS_POSTFIX_OPERATOR;
  }
  return SUCCESS;
}

/*
 *  Called by method_call ();
 */
int prefix_rcvr_cvar_expr_registration (MESSAGE_STACK messages, 
					int rcvr_idx) {
  char output_buf[MAXMSG], linemarker_buf[MAXMSG], *n,
    expr_buf[MAXMSG], cvar_buf[MAXMSG];
  char __c[MAXMSG], __c_blk[MAXMSG];
  int prefix_tok_idx, open_paren_idx, close_paren_idx;
  int next_tok_idx, subscript_end_idx, struct_end_idx;
  CVAR *v;

  if ((prefix_tok_idx = prevlangmsg (messages, rcvr_idx)) == ERROR)
    return ERROR;

  /*
   *  Handle a case where a receiver is enclosed in parentheses.
   */
  open_paren_idx = close_paren_idx = -1;
  while (M_TOK(messages[prefix_tok_idx]) == OPENPAREN) {
    open_paren_idx = prefix_tok_idx;
    if ((prefix_tok_idx = prevlangmsg (messages, prefix_tok_idx)) == ERROR)
      return ERROR;
  }
  if (open_paren_idx != -1) {
    close_paren_idx = match_paren (messages, open_paren_idx,
				   get_stack_top (messages));
  }
  if ((M_TOK(messages[prefix_tok_idx]) != INCREMENT) &&
      (M_TOK(messages[prefix_tok_idx]) != DECREMENT))
    return ERROR;
  if (((v = get_local_var (M_NAME(messages[rcvr_idx]))) == NULL) &&
      ((v = get_global_var (M_NAME(messages[rcvr_idx]))) == NULL))
    return ERROR;

  if (ctrlblk_pred) {
    ctrlblk_register_c_var 
      (fmt_register_c_method_arg_call 
       (v, M_NAME(messages[rcvr_idx]), FRAME_SCOPE, cvar_buf),
       FRAME_START_IDX);
  } else {
    if ((next_tok_idx = nextlangmsg (messages, rcvr_idx)) != ERROR) {
      if (M_TOK(messages[next_tok_idx]) == ARRAYOPEN) {
	if ((subscript_end_idx = 
	     is_subscript_expr (messages, rcvr_idx, get_stack_top (messages)))
	    > 0) {
	  toks2str (messages, rcvr_idx, subscript_end_idx, __c);
	  strcatx (expr_buf, M_NAME(messages[prefix_tok_idx]),
		   __c, ";\n", NULL);
	  strcatx (output_buf, "\n{\n",
		   expr_buf, 
		   fmt_register_c_method_arg_call
		   (v, __c, FRAME_SCOPE, cvar_buf), "\n}\n", NULL);
	}
      } else { /* if (M_TOK(messages[next_tok_idx]) == ARRAYOPEN) */
	if (messages[rcvr_idx] -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR) {
	  if ((struct_end_idx = 
	       struct_end (messages, rcvr_idx, 
				 get_stack_top (messages))) > 0) {
	    if ((open_paren_idx != -1) && (close_paren_idx != -1)) {
	      toks2str (messages, open_paren_idx, close_paren_idx, __c_blk);
	      toks2str (messages, rcvr_idx, struct_end_idx, __c);
	    } else {
	      toks2str (messages, rcvr_idx, struct_end_idx, __c);
	      toks2str (messages, rcvr_idx, struct_end_idx, __c_blk);
	    }
	    if (((v -> attrs & CVAR_ATTR_STRUCT_TAG) ||
		(v -> attrs & CVAR_ATTR_STRUCT_PTR_TAG)) &&
		(v -> members == NULL)) {
	      CVAR *v_defn, *v_mbr;
	      if (((v_defn = get_local_struct_defn (v -> type)) != NULL) ||
		  ((v_defn = get_global_struct_defn (v -> type)) != NULL)) {
		if ((v_mbr = struct_member_from_expr_b (messages,
							rcvr_idx,
							struct_end_idx, 
							v_defn))
		    != NULL) {
		  strcatx (expr_buf, M_NAME(messages[prefix_tok_idx]),
			   __c_blk, ";\n", NULL);
		  strcatx (output_buf, "\n{\n",
			   expr_buf,
			   fmt_register_c_method_arg_call
			   (v_mbr, __c, FRAME_SCOPE, cvar_buf),
			   "\n}\n", NULL);
		} else {/*if ((v_mbr = struct_member_from_expr_b ... */
		  strcatx (expr_buf, M_NAME(messages[prefix_tok_idx]),
			   __c_blk, ";\n", NULL);
		  strcatx (output_buf, "\n{\n",
			   expr_buf,
			   fmt_register_c_method_arg_call
			   (v, __c, FRAME_SCOPE, cvar_buf), "\n}\n", NULL);
		}/* if ((v_mbr = struct_member_from_expr_b ... */
	      } else {/* if (((v_defn = ... */
		strcatx (expr_buf, M_NAME(messages[prefix_tok_idx]),
			 __c_blk, ";\n", NULL);
		strcatx (output_buf, "\n{\n",
			 expr_buf,
			 fmt_register_c_method_arg_call
			 (v, __c, FRAME_SCOPE, cvar_buf),
			 "\n}\n", NULL);
	      } /* if (((v_defn = ... */
	    } else {/* if (((v -> attrs & CVAR_ATTR_STRUCT_TAG)... */
	      strcatx (expr_buf, M_NAME(messages[prefix_tok_idx]),
		       __c_blk, ";\n", NULL);
	      strcatx (output_buf, "\n{\n",
		       expr_buf,
		       fmt_register_c_method_arg_call
		       (v, __c, FRAME_SCOPE, cvar_buf),
		       "\n}\n", NULL);
	    }/* if (((v -> attrs & CVAR_ATTR_STRUCT_TAG)... */
	  }
	} else {/*if (messages[rcvr_idx] -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR)*/
	  strcatx (expr_buf, M_NAME(messages[prefix_tok_idx]),
		   M_NAME(messages[rcvr_idx]), ";\n", NULL);
	  strcatx (output_buf, "\n{\n",
		   expr_buf,
		   fmt_register_c_method_arg_call
		   (v, M_NAME(messages[rcvr_idx]), FRAME_SCOPE, cvar_buf),
		   "\n}\n", NULL);
	}/*if (messages[rcvr_idx] -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR)*/
      } /* if (M_TOK(messages[next_tok_idx]) == ARRAYOPEN) */
    } else {  /* if ((next_tok_idx = nextlangmsg (messages, rcvr_idx)) != ERROR) */
      strcatx (expr_buf, M_NAME(messages[prefix_tok_idx]),
	       M_NAME(messages[rcvr_idx]), ";\n", NULL);
      strcatx (output_buf, "\n{\n",
	       expr_buf,
	       fmt_register_c_method_arg_call
	       (v, M_NAME(messages[rcvr_idx]), FRAME_SCOPE, cvar_buf),
	       "\n}\n", NULL);
    } /* if ((next_tok_idx = nextlangmsg (messages, rcvr_idx)) != ERROR) */
    if (!nolinemarker_opt) {
      fmt_line_info (linemarker_buf, messages[rcvr_idx] -> error_line, 
		     input_source_file, 1, FALSE);
      /*
       *  If outputting at the start of a frame, the line marker should
       *  not need an extra trailing newline of its own, so the line number 
       *  doesn't register with the compiler as the source line + 1.
       *  So trim the linemarker string.  
       *  Later functions, in method.c and output.c, check 
       *  n_th_arg_CVAR_has_unary_inc|dec_prefix to determine
       *  if we've inserted a statement without a newline.
       */
      if ((n = rindex (linemarker_buf, '\n')) != NULL)
	*n = '\0';
      strcatx2 (output_buf, linemarker_buf, NULL);
    }
    fileout (output_buf, 0, FRAME_START_IDX);
    _MPUTCHAR(messages[prefix_tok_idx], ' ')
    messages[prefix_tok_idx] -> tokentype = WHITESPACE;
    messages[prefix_tok_idx] -> attrs &= !TOK_IS_PREFIX_OPERATOR;
    /*
     *  It should be safe enough to elide any enclosing parentheses
     *  in the receiver once we have the C expression factored out - 
     *  Receivers that get to here are simple lvalues, not expressions.
     */
    if ((open_paren_idx != -1) && (close_paren_idx != -1)) {
      int stack_top_idx = get_stack_top (messages);
      while (M_TOK(messages[open_paren_idx]) == OPENPAREN) {
	_MPUTCHAR(messages[open_paren_idx], ' ')
	messages[open_paren_idx] -> tokentype = WHITESPACE;
	_MPUTCHAR (messages[close_paren_idx], ' ')
	messages[close_paren_idx] -> tokentype = WHITESPACE;
	open_paren_idx = nextlangmsg (messages, open_paren_idx);
	if (M_TOK(messages[open_paren_idx]) == OPENPAREN) {
	  close_paren_idx = match_paren (messages, open_paren_idx,
					 stack_top_idx);
	}
      }
    }
  }
  return SUCCESS;
}

