/* $Id: complexmethd.c,v 1.2 2020/04/17 04:21:00 rkiesling Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "list.h"

/* Declared in argblk.c. */
extern bool have_complex_arg_block;     /* also reset in method_call ().    */
extern int complex_arg_block_start;
extern int complex_arg_block_rcvr_ptr; /* Set by method_call ()            */

extern I_PASS interpreter_pass;    /* Declared in rtinfo.c.                    */

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;

/*
 *  Can handle statements like the following:
 *
 *  1. <object> <method|instancevar> <method|instancevar>* <args>...
 *  2. <self> <method|instancevar|classvar> <method|instancevar>* <args>...
 *  3. <class> <classvar|method>* <args>...
 *
 *  complex_method_statement and complex_self_method_statement are
 *  called by routines in method.c and rexpr.c.
 */

int complex_method_statement (OBJECT *receiver_object, MESSAGE_STACK messages, 
			      int method_idx) {

  if (complex_var_or_method_message (receiver_object, messages, method_idx))
    return TRUE;
  else
    return FALSE;
}

/*
 *  "Self" normally stays unevaled until run time.  Set the receiver object
 *  temporarily if necessary.
 */
int complex_self_method_statement (OBJECT *receiver_object, MESSAGE_STACK messages, 
			      int method_idx) {
  int r;

  if (messages[method_idx]->receiver_obj == NULL) {
    messages[method_idx]->receiver_obj = receiver_object;
    r = complex_var_or_method_message (receiver_object, messages, method_idx);
    messages[method_idx]->receiver_obj = NULL;
    return ((r) ? TRUE : FALSE);
  } else {
    r = complex_var_or_method_message (receiver_object, messages, method_idx);
    return ((r) ? TRUE : FALSE);
  }

  return FALSE;
}

int complex_var_or_method_message (OBJECT *receiver_object, MESSAGE_STACK messages,
				   int idx) {

  int next_message_idx;
  MESSAGE_CLASS next_message_class;

  have_complex_arg_block = FALSE;
  complex_arg_block_start = -1;

  if (messages[idx] -> receiver_obj == NULL) {
    /*
     *  For now - Ths should not be the case on the first call
     *  to this function.  However, we should add the ability
     *  to backtrack if the backtracking in message_class 
     *  is not sufficient.
     */
    return FALSE;
  } else {
    if ((next_message_idx = nextlangmsg (messages, idx)) == ERROR)
      return FALSE;
    if ((METHOD_ARG_TERM_MSG_TYPE(messages[next_message_idx])) &&
	!is_overloaded_aggregate_op (messages, next_message_idx))
      return FALSE;
    next_message_class = message_class (messages, next_message_idx);
    if (next_message_class == null_message) return FALSE;
    return TRUE;
  }

  return FALSE;
}

MESSAGE_CLASS message_class (MESSAGE_STACK messages, int idx) {

  int prev_idx;
  OBJECT *var_message_object = NULL,
    *rcvr_value_obj = NULL;
  OBJECT *__instvar, *__classvar;
  MESSAGE_CLASS prev_message_class;

  if (messages[idx] -> receiver_obj) {
    if ((var_message_object = 
	 get_class_variable (M_NAME(messages[idx]),
			     (IS_CLASS_OBJECT(messages[idx]->receiver_obj) ?
			      messages[idx]->receiver_obj->__o_name : 
			      messages[idx]->receiver_obj-> __o_classname),
			     FALSE)) != NULL) {
      return class_var_message;
    } else {
      if (messages[idx] -> attrs & OBJ_IS_INSTANCE_VAR)
	return instance_var_message;
      if ((var_message_object = 
 	   get_instance_variable (M_NAME(messages[idx]),
		  (IS_CLASS_OBJECT (messages[idx]->receiver_obj) ?
		   (messages[idx]->receiver_obj->instancevars ? 
		    /*
		     *  This shouldn't be fixed just yet....
		     *  But the value object of a class variable
		     *  still has the name, "value," and the
		     *  classname, of the class, not, "Class."
		     */
		    messages[idx]->receiver_obj->instancevars->__o_classname :
		    messages[idx]->receiver_obj->__o_name) :
		   (messages[idx]->receiver_obj->instancevars ? 
		    messages[idx]->receiver_obj->instancevars->__o_classname :
		    messages[idx]->receiver_obj-> __o_classname)),
		   FALSE)) != NULL) {
 	return instance_var_message;
      }
      if (messages[idx]->receiver_obj->instancevars) {
	rcvr_value_obj = messages[idx]->receiver_obj->instancevars;
      } else {
	rcvr_value_obj = messages[idx]->receiver_obj;
      }

      /* Same as below. Looking for a prototype helps prevent
	 a recursive class library search. */
      if (method_proto (rcvr_value_obj -> __o_classname,
			M_NAME(messages[idx]))) {
	int next_idx;
	if ((next_idx = nextlangmsg (messages, idx)) != ERROR) {
	  if (M_TOK(messages[next_idx]) == OPENBLOCK) {
	    have_complex_arg_block = TRUE;
	    complex_arg_block_start = next_idx;
	  } else {
	    have_complex_arg_block = FALSE;
	  }
	} else {
	  have_complex_arg_block = FALSE;
	}
	return instance_method_message;
      }

      if (is_method_name (M_NAME(messages[idx]))) {
	int next_idx;
	if ((next_idx = nextlangmsg (messages, idx)) != ERROR) {
	  if (M_TOK(messages[next_idx]) == OPENBLOCK) {
	    have_complex_arg_block = TRUE;
	    complex_arg_block_start = next_idx;
	  } else {
	    have_complex_arg_block = FALSE;
	  }
	} else {
	  have_complex_arg_block = FALSE;
	}
	return instance_method_message;
      }
      if (get_class_method (messages[idx],
			    rcvr_value_obj, M_NAME(messages[idx]), 
			    ERROR, FALSE))
	return class_method_message;
    }
  } else {
    /*
     *  Backtrack until we find a receiver.
     */
    if ((prev_idx = prevlangmsg (messages, idx)) == ERROR)
      return null_message;

    prev_message_class = message_class (messages, prev_idx);

    switch (prev_message_class)
      {
      case instance_method_message:
	if (M_TOK(messages[idx]) == DEREF) {
	  return instance_method_message;
	} else if (IS_C_BINARY_MATH_OP(M_TOK(messages[idx]))) {
	  METHOD *m_tmp;
	  /* Check the case where we have a numeric method return class
	     with 0 params and a math op message following. 
	     e.g.,
	           str length - 10;
	     TODO - Check method prototypes also when we have an
	     example.
	  */

	  if (((m_tmp = get_instance_method 
		(messages[prev_idx], 
		 messages[prev_idx] -> receiver_obj -> instancevars,
		 M_NAME(messages[prev_idx]), ERROR, FALSE)) != NULL) ||
	      ((m_tmp = get_class_method
		(messages[prev_idx], 
		 messages[prev_idx] -> receiver_obj -> instancevars,
		 M_NAME(messages[prev_idx]), ERROR, FALSE)) != NULL)) {
	    if (m_tmp -> n_params == 0) {
	      if (is_subclass_of (m_tmp -> returnclass, 
				  MAGNITUDE_CLASSNAME)) {
		if (is_method_name (M_NAME(messages[idx]))) {
		  return instance_method_message;
		}
	      }
	    }
	  }
	}
	break;
      case class_method_message:
	break;
      case instance_var_message:
	for (__instvar = messages[prev_idx]->receiver_obj->instancevars;
	     __instvar; __instvar = __instvar -> next) {
	  if (!strcmp (M_NAME(messages[prev_idx]), __instvar->__o_name)) {
	    messages[idx]->receiver_obj = __instvar;
	    return message_class (messages, idx);
	  }
	}
	break;
      case class_var_message:
	if ((__classvar = 
	     get_class_variable 
	     (M_NAME(messages[prev_idx]),
	      (IS_CLASS_OBJECT(messages[prev_idx]->receiver_obj) ?
	       messages[prev_idx]->receiver_obj->__o_name : 
	       messages[prev_idx]->receiver_obj-> __o_classname),
	      FALSE)) != NULL) {
	  messages[idx]->receiver_obj = ((__classvar->instancevars) ?
					 __classvar->instancevars : 
					 __classvar);
	  return message_class (messages, idx);
	} else {
	  warning (messages[prev_idx], "Class variable %s not found.", 
		   M_NAME(messages[prev_idx]));
	  return null_message;
	}
	break;
      case null_message:
	return null_message;
	break;
      }
  }

  return null_message;
}

/*
 *  Must be called after one of the functions, above.
 *  Returns the class of the final receiver in the 
 *  statement.
 */
char *complex_expr_class (MESSAGE_STACK messages, int rcvr_msg_idx) {

  int i, stack_end_idx, prev_tok_idx;
  char  *class_str = NULL;
  OBJECT *instancevar_object;
  METHOD *m;

  stack_end_idx = get_stack_top (messages);

  prev_tok_idx = -1;
  for (i = rcvr_msg_idx-1; i >= stack_end_idx; i--) {

    if (M_ISSPACE(messages[i])) continue;

    if ((M_TOK(messages[i]) == OPENBLOCK) ||
	(M_TOK(messages[i]) == CLOSEBLOCK) ||
	(M_TOK(messages[i]) == SEMICOLON) ||
	(M_TOK(messages[i]) == ARRAYCLOSE))
      break;

    if (messages[i] -> receiver_obj) {
      if (IS_CLASS_OBJECT(messages[i]->receiver_obj)) {
	class_str =  messages[i] -> receiver_obj -> __o_name;
      } else {
	if (messages[i]->receiver_obj->instancevars) {
	  if ((prev_tok_idx != -1) && 
	      IS_OBJECT(messages[prev_tok_idx]->obj) &&
	      ((instancevar_object = 
	       __ctalkGetInstanceVariable (messages[prev_tok_idx] -> obj, 
					   messages[i] -> name,
					   FALSE)) != NULL)) {
	    class_str = ((instancevar_object -> instancevars) ?
			 instancevar_object -> instancevars -> __o_classname :
			 instancevar_object -> __o_classname);
	  } else {
	    if (((m = get_instance_method 
		  (messages[i], 
		   messages[i] -> receiver_obj -> instancevars,
		   M_NAME(messages[i]), ERROR, FALSE)) != NULL) ||
		((m = get_class_method
		  (messages[i], 
		   messages[i] -> receiver_obj -> instancevars,
		   M_NAME(messages[i]), ERROR, FALSE)) != NULL)) {
	      class_str =  m ->  returnclass;
	    } else {

	      /*
	       *  TODO - This clause is more specific to an
	       *  updated instance variable series.  Probably
	       *  could be combined with the instance variable
	       *  clause above if we can do away with the prev_tok_idx
	       *  pointer....
	       *
	       *  Or maybe not, if we want to tailor the clause
	       *  to the OBJ_IS_INSTANCE_VAR message attribute.
	       */
	      if (messages[i] -> attrs & OBJ_IS_INSTANCE_VAR) {
		if ((instancevar_object = 
		     __ctalkGetInstanceVariable 
		     (messages[i] -> receiver_obj, M_NAME(messages[i]),
		      FALSE)) != NULL) {
		  class_str = instancevar_object -> instancevars ? 
		    instancevar_object -> instancevars -> __o_classname :
		    instancevar_object -> __o_classname;
		} else {
		  /* TODO - Maybe this could generate a warning. */
		  class_str = messages[i] -> receiver_obj -> instancevars
		    -> __o_classname;
		}
	      } else {
		class_str = messages[i] -> receiver_obj  -> instancevars -> 
		  __o_classname;
	      }
	    }
	  }
	} else {
	  class_str = messages[i] -> receiver_obj -> __o_classname;
	}
      }
    } else {
      if (messages[i] -> receiver_obj == NULL)
	return class_str;
    }
    prev_tok_idx = i;
  }

  return class_str;
}

int is_overloaded_aggregate_op (MESSAGE_STACK messages, int idx) {
  int prev_tok_idx;
  if ((prev_tok_idx = prevlangmsg (messages, idx)) != ERROR) {
    if (messages[prev_tok_idx] -> obj)
      return TRUE;
  }
  return FALSE;
}

static bool objs_in_cvar_term (MESSAGE_STACK messages, int tok_idx) {
  int next_tok, prev_tok;
  if ((next_tok = nextlangmsg (messages, tok_idx)) != ERROR) {
    if (M_TOK(messages[next_tok]) == LABEL) {
      return true;
    }
  }
  if ((prev_tok = prevlangmsg (messages, tok_idx)) != ERROR) {
    if (M_TOK(messages[prev_tok]) == LABEL) {
      return true;
    }
  }
  return false;
}

/* 
 *   Called by super_argblk_rcvr_expr () when simply writing expressions
 *   that start with, "super."
 *
 *   Also called by ctrlblk_pred_rt_expr () for any if, while, or switch
 *   expression within an argblk.
 */

void register_argblk_c_vars_1 (MESSAGE_STACK messages, 
				    int expr_start,
				    int expr_end) {
  int i, next_idx;
  CVAR *cvar;
  char buf[MAXMSG];

  for (i = expr_start; i >= expr_end; i--) {

    if (M_ISSPACE(messages[i]) || (M_TOK(messages[i]) != LABEL))
      continue;
    
    if (((cvar = get_local_var (M_NAME(messages[i]))) != NULL) ||
	((cvar = get_global_var (M_NAME(messages[i]))) != NULL)) {

      next_idx = nextlangmsg (messages, i);
      handle_cvar_argblk_translation (messages, i, next_idx, cvar);

      /* So we don't cause an undefined label exception later on. */
      messages[i] -> attrs |= TOK_IS_RT_EXPR;

      if (objs_in_cvar_term (messages, i)) {
	fmt_register_argblk_cvar_from_basic_cvar (messages, i,
						  cvar, buf);
	buffer_argblk_stmt (buf);
      }
    }
    
  }
}



/*
 *  Called by eval_keyword_expr (), etc., which can sequence expressions
 *  differently - so output the expression and the register calls
 *  as a unit.
 */
char *fmt_register_argblk_c_vars_1 (MESSAGE_STACK messages, 
				    int expr_start,
				    int expr_end) {
  int i;
  int prev_idx;
  CVAR *cvar, *cvar_basic;
  static char collector_buf[MAXMSG];
  int scope;

  memset (collector_buf, 0, MAXMSG);

  for (i = expr_start; i >= expr_end; i--) {

    if (M_ISSPACE(messages[i]) || (M_TOK(messages[i]) != LABEL))
      continue;
    
    if (((cvar = get_local_var (M_NAME(messages[i]))) != NULL) ||
	((cvar = get_global_var (M_NAME(messages[i]))) != NULL)) {

      /* If the variables have duplicate names, an object,
	 instance, or class variable takes precedence. */
      if (is_object_or_param (M_NAME(messages[i]), NULL))
	continue;
      if ((messages[i] -> attrs & OBJ_IS_INSTANCE_VAR) ||
	  (messages[i] -> attrs & OBJ_IS_CLASS_VAR))
	continue;
      if ((prev_idx = prevlangmsg (messages, i)) != ERROR) {
	if (M_TOK(messages[prev_idx]) == LABEL) {
	  if ((messages[prev_idx] -> attrs & TOK_SELF) ||
	      (messages[prev_idx] -> attrs & TOK_SUPER)) {
	    continue;
	  }
	}
      }

      if (interpreter_pass == method_pass) {
	method_cvar_alias_basename
	  (new_methods[new_method_ptr+1] -> method, cvar,
	   messages[i] -> name);
      } else {
	function_cvar_alias_basename (cvar, messages[i] -> name);
      }

      /* So we don't cause an undefined label exception later on. */
      messages[i] -> attrs |= TOK_IS_RT_EXPR;

      cvar_basic = basic_type_of (cvar);
      scope = this_frame () -> scope;
      sprintf (collector_buf, 
	       "\n\t%s (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d, %d, %d, (void *)%s%s);\n",
	       REGISTER_C_METHOD_ARG,
	       ((*cvar_basic->decl) ? cvar_basic -> decl : NULLSTR),
	       ((*cvar_basic->type) ? cvar_basic -> type : NULLSTR),
	       ((*cvar_basic->qualifier) ? cvar_basic -> qualifier : NULLSTR),
	       ((*cvar_basic->qualifier2) ? cvar_basic -> qualifier2 : NULLSTR),
	       ((*cvar_basic->storage_class) ? cvar_basic -> storage_class : NULLSTR),
	       messages[i] -> name,
	       cvar -> type_attrs,
	       0,
	       0,
	       scope,
	       CVAR_ATTR_CVARTAB_ENTRY,
	       "",  /* we don't need to an a prefix ampersand in
		       this call. */
	       messages[i] -> name);

    }
    
  }
  return *collector_buf ? collector_buf : NULL;
}

/*
 *  Called by eval_arg () ... ?
 */
char *fmt_register_argblk_c_vars_2 (MESSAGE *m_tok, CVAR *cvar,
				    char *buf) {
  argblk_CVAR_name_to_msg (m_tok, cvar);

  m_tok -> attrs |= TOK_IS_RT_EXPR;

  fmt_register_argblk_cvar_from_basic_cvar_2 (m_tok,
					    cvar, buf);
  return buf;
}
    
/*
 * super in an argument block.  Simple expressions only for now.
 */
void super_argblk_rcvr_expr (MESSAGE_STACK messages, int super_idx, 
			OBJECT *rcvr_class_object) {
  int method_idx, arglist_start_idx, arglist_end_idx,
    stack_end_idx, i;
  MESSAGE *method_message;
  static char expr_out[MAXMSG];
  METHOD *m;
  OBJECT *var_object = NULL;

  if (!IS_OBJECT(rcvr_class_object)) /***/
    return;

  if ((method_idx = nextlangmsg (messages, super_idx)) != ERROR) {
    method_message = messages[method_idx];
    if ((m = get_instance_method (method_message, rcvr_class_object,
				  M_NAME(method_message), -1, FALSE)) != NULL) {
      if ((arglist_start_idx = nextlangmsg (messages, method_idx)) 
	  != ERROR) {
	if ((arglist_end_idx = 
	    method_arglist_limit_2 (messages, method_idx,
				    arglist_start_idx,
				    m -> n_params, m -> varargs)) != ERROR) {
	  rt_expr (messages, super_idx, &arglist_end_idx, expr_out);
	  for (i = super_idx; i >= arglist_end_idx; i--) {
	    ++messages[i] -> evaled;
	    ++messages[i] -> output;
	  }
	}
      }
    } else {
      if ((var_object = get_instance_variable 
	   (M_NAME(method_message), 
	    rcvr_class_object -> __o_name,
	    FALSE)) != NULL) {
	if ((arglist_start_idx = nextlangmsg (messages, method_idx)) 
	    != ERROR) {
	  stack_end_idx = get_stack_top (messages);
	  if ((arglist_end_idx = scanforward (messages, method_idx,
					      stack_end_idx, SEMICOLON))
	      != ERROR) {
	    rt_expr (messages, super_idx, &arglist_end_idx, expr_out);
	    for (i = super_idx; i >= arglist_end_idx; i--) {
	      ++messages[i] -> evaled;
	      ++messages[i] -> output;
	    }
	  }
	}
      }
    }

    if (!m && !var_object && !(interpreter_pass == expr_check)) {
      warning (method_message, "Undefined label, \"%s\" (Class %s).", 
	       M_NAME(method_message), rcvr_class_object -> __o_classname);
    }

    if (arglist_end_idx == -1) {
      char *_ebuf;
      int next_newline_idx;
      
      stack_end_idx = get_stack_top (messages);
      next_newline_idx = scanforward (messages, super_idx, stack_end_idx,
				      NEWLINE);

      _ebuf  = collect_tokens (messages, super_idx, next_newline_idx);
      warning (method_message, "Could not parse expression %s.",
	       _ebuf);
      __xfree (MEMADDR(_ebuf));

    }
  }
}

