/* $Id: collection.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "objtoc.h"

extern DEFAULTCLASSCACHE *ct_defclasses; /* Declared in cclasses.c.       */

extern bool eval_arg_cvar_reg;           /* Declared in eval_arg.c        */

int collection_rt_expr (METHOD *method, MESSAGE_STACK messages,
			int receiver_ptr, int msg_ptr,
			OBJECT_CONTEXT o_context) {

  char expr_buf[MAXMSG];
  char expr_buf_tmp[MAXMSG], expr_buf_tmp_2[MAXMSG];
  char *cvar_lval_class;
  int n_th_arg;

  toks2str (messages, receiver_ptr, msg_ptr - 1, expr_buf);
  for (n_th_arg = 0; n_th_arg < method -> n_args; n_th_arg++) {
    if (n_th_arg < (method -> n_args - 1)) {
      strcatx2 (expr_buf, method -> args[n_th_arg] -> obj -> __o_name,
		",", NULL);
    } else {
      strcatx2 (expr_buf,
		method -> args[n_th_arg] -> obj -> __o_name, NULL);
    }
  }

  switch (o_context)
    {
    case c_context:
    case c_argument_context:
      if ((cvar_lval_class = c_lval_class (messages, receiver_ptr)) != NULL) {
	fileout (fmt_rt_return (fmt_eval_expr_str (expr_buf, expr_buf_tmp), 
				cvar_lval_class, TRUE, expr_buf_tmp_2),
		 FALSE, msg_ptr);
      } else {
	fileout
	  (array_obj_2_c_wrapper (messages, msg_ptr, 
				  messages[msg_ptr]->receiver_obj,
				  method, fmt_eval_expr_str (expr_buf,
							     expr_buf_tmp)), 
	   FALSE, msg_ptr);
      }
      break;
    case receiver_context:
      /* this also works within argblks */
      if (eval_arg_cvar_reg) {
	fmt_eval_expr_str (expr_buf, expr_buf_tmp);
	strcat (expr_buf_tmp, ";\ndelete_method_arg_cvars ();\n");
	fileout (expr_buf_tmp, FALSE, msg_ptr);
      } else {
	fileout (fmt_eval_expr_str (expr_buf, expr_buf_tmp), FALSE, msg_ptr);
      }
      break;
    default:
      /* this also works within argblks, too */
      if (eval_arg_cvar_reg) {
	fmt_eval_expr_str (expr_buf, expr_buf_tmp);
	strcat (expr_buf_tmp, ";\ndelete_method_arg_cvars ();\n");
	fileout (expr_buf_tmp, FALSE, msg_ptr);
      } else {
	fileout (fmt_eval_expr_str (expr_buf, expr_buf_tmp), FALSE, msg_ptr);
      }
      break;
    }

  return SUCCESS;
}

int collection_needs_rt_eval (OBJECT *class_obj, METHOD *m) {

  int r;
  if (!IS_OBJECT(class_obj)) return FALSE;

  r = (IS_COLLECTION_SUBCLASS_OBJ(class_obj) &&
       !IS_STREAM_SUBCLASS_OBJ(class_obj) && 
       !IS_PRIMITIVE_METHOD(m) && strcmp (m -> name, "new")
       && (m -> n_params != 0));

  return r;
}


COLLECTION_CONTEXT collection_context (MESSAGE_STACK messages, int rcvr_idx) {

  int fn_idx, n_th_param, n_args;
  int stack_begin, stack_top;
  CFUNC *fn;
  CVAR *c, *param_c = 0;  /* Avoid a warning. */
  char *arg_class;

  stack_begin = stack_start (messages);
  stack_top = get_stack_top (messages);

  if ((n_th_param = obj_expr_is_fn_arg (messages, rcvr_idx,
					stack_begin,
					&fn_idx)) != ERROR) {

    fn = get_function (M_NAME(messages[fn_idx]));
    c = get_global_var (M_NAME(messages[fn_idx]));
    if (fn) {
      for (n_args = 0, param_c = fn -> params; 
	   (n_args < n_th_param) && param_c; n_args++) {
	param_c = param_c -> next;
      }
    } else {
      if (c) {
	for (n_args = 0, param_c = c -> params; 
	     (n_args < n_th_param) && param_c; n_args++) {
	  param_c = param_c -> next;
	}
      } else {
	warning (messages[fn_idx], "Could not find function definition %s.",
		 M_NAME(messages[fn_idx]));
      }
    }
    arg_class = basic_class_from_cvar (messages[rcvr_idx], param_c, 0);
    if (str_eq (arg_class, STRING_CLASSNAME) ||
	(is_subclass_of (arg_class, STRING_CLASSNAME) == TRUE))
      return char_ptr_collection_context;
    if (str_eq (arg_class, CHARACTER_CLASSNAME) ||
	(is_subclass_of (arg_class, CHARACTER_CLASSNAME) == TRUE))
      return int_collection_context;
    if (str_eq (arg_class, INTEGER_CLASSNAME) ||
	(is_subclass_of (arg_class, INTEGER_CLASSNAME) == TRUE))
      return int_collection_context;
    return ptr_collection_context;
  }

  if (IS_SIMPLE_SUBSCRIPT (messages, rcvr_idx,
			   stack_begin,
			   stack_top))
    return int_collection_context;

  switch (fmt_arg_type (messages, rcvr_idx, stack_begin))
    {
    case fmt_arg_char_ptr:
      return char_ptr_collection_context;
      break;
    case fmt_arg_int:
      return int_collection_context;
      break;
    case fmt_arg_char:
    case fmt_arg_double:
    case fmt_arg_long_int:
    case fmt_arg_long_long_int:
    case fmt_arg_ptr:
    case fmt_arg_null:
      return ptr_collection_context;
      break;
    }
      
  return ptr_collection_context;  /* avoid a warning */
}

/* Called by eval_arg. */
bool is_collection_initializer (int main_stack_method_idx) {
  int rcvr_idx;
  OBJECT *rcvr_obj;
  METHOD *method;
  if ((rcvr_idx = prevlangmsg (message_stack (), main_stack_method_idx))
      != ERROR) {
    if (IS_OBJECT(message_stack_at (rcvr_idx) -> obj)) {
      rcvr_obj = message_stack_at (rcvr_idx) -> obj;
      if ((method = get_instance_method
	   (message_stack_at (main_stack_method_idx),
	    rcvr_obj,
	    M_NAME(message_stack_at (main_stack_method_idx)),
	    ANY_ARGS, FALSE)) != NULL) {
	if (method -> varargs) {
	  return true;
	}
      }
    }
  }
  return false;
}
