/* $Id: errmsgs.c,v 1.2 2020/07/08 02:47:49 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014-2020 Robert Kiesling, rk3314042@gmail.com.
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


extern I_PASS interpreter_pass;

extern int nolinemarker_opt;  /* Declared in main.c */

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;

extern int error_line, error_column;  /* Declared in parser.c.             */

extern LIST *unresolved_labels;

extern MESSAGE *m_messages[N_MESSAGES+1]; /* Declared in method.c */
extern int m_message_ptr;

extern CTRLBLK *ctrlblks[MAXARGS + 1];  /* Declared in control.c. */
extern int ctrlblk_ptr;

extern bool argblk;                            /* Declared in argblk.c. */

extern HASHTAB defined_instancevars;    /* Declared in primitives.c. */

void method_args_wrong_number_of_arguments_1 (MESSAGE_STACK messages,
					      int err_idx,
					      METHOD *new_method,
					      METHOD *obj_method,
					      char *fn_name,
					      I_PASS interpreter_pass) {
    switch (interpreter_pass)
      {
      case method_pass:
	warning (messages[err_idx],
		 "Method %s: Wrong number of arguments.",
		 obj_method -> name);
	break;
      case parsing_pass:
	if (fn_name != NULL) {
	  warning (messages[err_idx],
		   "In function %s:", fn_name);
	  warning (messages[err_idx],
		   "Method %s: Wrong number of arguments.",
		   obj_method -> name);
	} else {
	  warning (messages[err_idx],
		   "Method %s: Wrong number of arguments.",
		   obj_method -> name);
	}
	break;
      case expr_check:
	/* Do nothing. */
	break;
      default:
	warning (messages[err_idx],
		 "Method %s: Wrong number of arguments.",
		 obj_method -> name);
	break;
    } 
}

void method_args_wrong_number_of_arguments_2 (MESSAGE_STACK messages,
					      int err_idx,
					      METHOD *new_method,
					      METHOD *obj_method,
					      char *fn_name,
					      int n_expr_args,
					      char *expr_buf,
					      I_PASS interpreter_pass) {
    switch (interpreter_pass)
      {
      case method_pass:
	warning (messages[err_idx],
		 "In method %s:",
		 new_method -> name);
	warning (messages[err_idx],
		 "Method %s: Wrong number of arguments.",
		 obj_method -> name);
	warning (messages[err_idx],
		 "Method %s requires %d parameters.",
		 obj_method -> name, obj_method -> n_params);
	warning (messages[err_idx],
		 "Expression provides %d arguments:",
		 n_expr_args);
	printf ("\n\t%s\n", expr_buf);
	break;
      case parsing_pass:
	if (fn_name != NULL) {
	  warning (messages[err_idx],
		   "In function %s:", fn_name);
	  warning (messages[err_idx],
		   "Method %s: Wrong number of arguments.",
		   obj_method -> name);
	  warning (messages[err_idx],
		   "Method %s requires %d parameters.",
		   obj_method -> name, obj_method -> n_params);
	  warning (messages[err_idx],
		   "Expression provides %d arguments:",
		   n_expr_args);
	  printf ("\n\t%s\n", expr_buf);
	} else {
	  warning (messages[err_idx],
		   "Method %s: Wrong number of arguments.",
		   obj_method -> name);
	  warning (messages[err_idx],
		   "Method %s requires %d parameters.",
		   obj_method -> name, obj_method -> n_params);
	  warning (messages[err_idx],
		   "Expression provides %d arguments:",
		   n_expr_args);
	  printf ("\n\t%s\n", expr_buf);
	}
	break;
      default:
	warning (messages[err_idx],
		 "Method %s: Wrong number of arguments.",
		 obj_method -> name);
	warning (messages[err_idx],
		 "Method %s requires %d parameters.",
		 obj_method -> name, obj_method -> n_params);
	warning (messages[err_idx],
		 "Expression provides %d arguments:",
		 n_expr_args);
	printf ("\n\t%s\n", expr_buf);
	break;
    } 
}

void method_args_ambiguous_argument_1 (MESSAGE_STACK messages,
				       int err_idx,
				       METHOD *new_method,
				       METHOD *obj_method,
				       int arglist_start_idx,
				       char *fn_name,
				       I_PASS interpreter_pass) {
    switch (interpreter_pass)
      {
      case method_pass:
	warning (messages[err_idx],
		 "In method %s:",
		 new_method -> name);
	warning (messages[err_idx],
 "Message, \"%s,\" follows method, \"%s,\" which returns \"Any\" class object.",
		 M_NAME(messages[arglist_start_idx]), obj_method -> name);
	break;
      case parsing_pass:
	if (fn_name != NULL) {
	  warning (messages[err_idx],
		   "In function %s:", fn_name);
	  warning (messages[err_idx],
   "Message, \"%s,\" follows method, \"%s,\" which returns \"Any\" class object.",
		   M_NAME(messages[arglist_start_idx]), obj_method -> name);
	} else {
	  warning (messages[err_idx],
   "Message, \"%s,\" follows method, \"%s,\" which returns \"Any\" class object.",
		   M_NAME(messages[arglist_start_idx]), obj_method -> name);
	}
	break;
      default:
	warning (messages[err_idx],
   "Message, \"%s,\" follows method, \"%s,\" which returns \"Any\" class object.",
		 M_NAME(messages[arglist_start_idx]), obj_method -> name);
	break;
    } 
}

/*
 *  Warn about obvious errors at the start of argument expressions.
 */
void undefined_arg_first_label_warning (MESSAGE_STACK m_messages,
					int arg_start_idx,
					int stack_ptr_idx,
					int main_stack_idx,
					OBJECT *tmp_object,
					CVAR *cvar) {

  char buf_out[MAXMSG], unresolved_msg[MAXMSG];
  LIST *l;
  MSINFO ms;

  if (tmp_object || cvar)
    return;

  if (M_TOK(m_messages[arg_start_idx]) != LABEL)
    return;

  if (m_messages[arg_start_idx] -> attrs & TOK_SELF ||
      m_messages[arg_start_idx] -> attrs & TOK_SUPER ||
      str_eq (M_NAME(m_messages[arg_start_idx]), "eval"))
    return;

  if (interpreter_pass == method_pass) {
    if (is_method_parameter (m_messages, arg_start_idx))
      return;
  }

  if (get_class_object (M_NAME(m_messages[arg_start_idx])) ||
      is_pending_class (M_NAME(m_messages[arg_start_idx]))) 
    return;

  if (is_c_type (M_NAME(m_messages[arg_start_idx])))
    return;

  if (is_instance_var (M_NAME(m_messages[arg_start_idx]))) {
    int expr_end;
    ms.messages = m_messages;
    ms.stack_start = stack_start (m_messages);
    ms.stack_ptr = get_stack_top (m_messages);
    ms.tok = arg_start_idx;
    char *tmp = collect_expression (&ms, &expr_end);
    warning (message_stack_at (main_stack_idx),
	     "Instance variable, \"%s,\" does not have a receiver."
	     "\n\n\t%s\n\n",
	     M_NAME(m_messages[arg_start_idx]), tmp);
    __xfree (MEMADDR(tmp));
  } else {
    warning_text (buf_out, message_stack_at (main_stack_idx),
	     "Could not resolve label, \"%s.\"", 
	     M_NAME(m_messages[arg_start_idx]));
    strcatx (unresolved_msg, M_NAME(m_messages[arg_start_idx]),
	     ":::", buf_out, NULL);
    l = new_list ();
    l -> data = strdup (unresolved_msg);
    if (unresolved_labels == NULL) {
      unresolved_labels = l;
    } else {
      list_add (unresolved_labels, l);
    }
  }
}

void self_within_constructor_warning (MESSAGE *m_orig) {
  warning (m_orig, "\"self\" used within a constructor.");
} 
				      
/* Only here so we don't have to un-static the pass_error_line ()
   in error.c. */
static int pass_error_line_b (int orig_line) {
  /*
   *  Try subtracting 1 from the method start line, because
   *  the parser already numbers the method's first line
   *  as 1.
   */
  return ((interpreter_pass == method_pass) ?
	  (orig_line + (method_start_line () - 1)) : orig_line);
}
void arglist_syntax_error_msg (MESSAGE_STACK messages, 
			       int arglist_start_idx) {
  char *s;
  int j, i, expr_start_idx, expr_end_idx;

  expr_start_idx = stack_start (messages);
  for (j = arglist_start_idx; j <= expr_start_idx; j++) {
    if ((M_TOK(messages[j]) == SEMICOLON) || 
	(M_TOK(messages[j]) == OPENBLOCK) ||
	(M_TOK(messages[j]) == CLOSEBLOCK)) {
      expr_start_idx = j;
    }
  }
  expr_end_idx = get_stack_top (messages) + 1;
  for (i = arglist_start_idx; i > expr_end_idx; i--) {
    if ((M_TOK(messages[i]) == SEMICOLON) || 
	(M_TOK(messages[i]) == OPENBLOCK) ||
	(M_TOK(messages[i]) == CLOSEBLOCK)) {
      expr_end_idx = i;
    }
  }
  s = collect_tokens (messages, expr_start_idx, expr_end_idx);
  printf ("%s: %d: Warning: Argument list syntax error:\n",
	  source_filename (),
	  pass_error_line_b (messages[arglist_start_idx]->error_line));
  printf ("\t%s\n", s);
  __xfree (MEMADDR(s));
}

void undefined_label_after_instance_variable_warning (MESSAGE_STACK messages,
						      int instancevar_idx) {
  int next_idx;

  if ((next_idx = nextlangmsg (messages, instancevar_idx)) != ERROR) {
    if (M_TOK(messages[next_idx]) == LABEL) {
      warning (messages[next_idx],
	       "Unresolved label, \"%s,\" follows instance variable, \"%s,\""
	       "(class %s)",
	       M_NAME(messages[next_idx]),
	       M_NAME(messages[instancevar_idx]),
	       ((messages[instancevar_idx] -> obj) ? 
		messages[instancevar_idx] -> obj -> CLASSNAME : 
		NULLSTR));
    }
  }
  
}

void rval_fn_extra_tok_warning (MESSAGE_STACK messages, int expr_start,
				int expr_end) {
  char *expr;

  expr = collect_tokens (messages, expr_start, expr_end);
  warning (messages[expr_start], "Mixing C functions and objects in "
	   "right-hand expressions is not (yet) supported." "\n\n\t%s\n",
	   expr);
  __xfree (MEMADDR(expr));
  exit (1);
}

void object_in_initializer_warning (MESSAGE_STACK messages, int obj_idx,
				    METHOD *new_method, 
				    OBJECT *rcvr_class_obj) {
  if (new_method && rcvr_class_obj) {
    warning (messages[obj_idx], "In method, \"%s,\" (class %s):",
	     new_method -> name, rcvr_class_obj -> __o_name);
    warning (messages[obj_idx], "Use of the object, \"%s,\" in a "
	     "C variable initializer is not (yet) supported.",
	     M_NAME(messages[obj_idx]));
  } else {
    warning (messages[obj_idx], "Use of the object, \"%s,\" in a "
	     "C variable initializer is not (yet) supported.",
	     M_NAME(messages[obj_idx]));
  }
}

void prev_constant_tok_warning (MESSAGE_STACK messages, int prev_tok_idx,
				int i, int expr_end) {
  char *expr;

  expr = collect_tokens (messages, prev_tok_idx, expr_end);
  warning (messages[i], "Object label \"%s\" follows a constant: "
	   "\n\n\t%s\n", M_NAME(messages[i]), expr);
  __xfree (MEMADDR(expr));
}

void still_prototyped_constructor_warning (MESSAGE *m_orig,
					   char *rcvr_class_name,
					   char *method_name) {
  warning (m_orig, "Use of a still-prototyped constructor:");
  printf ("\n\t%s : %s.\n\n", rcvr_class_name, method_name);
  printf ("Consider adding a, \"require %s,\" statement to the class, or creating\n",
	  rcvr_class_name);
  printf ("the object as an instance variable.\n");

}

/* Actually an instance of class variable declaration syntax error,
   since it's called from var_definition (). */
void invalid_class_variable_declaration_error (MESSAGE_STACK messages,
					       int decl_start_idx) {
  int decl_end_idx;
  char *s, buf[MAXMSG];

  if (decl_start_idx != ERROR) {
    if ((decl_end_idx = scanforward (messages, decl_start_idx,
				     get_stack_top (messages),
				     SEMICOLON)) != ERROR) {
      s = collect_tokens (messages, decl_start_idx, decl_end_idx);
      strcpy (buf, s);
      free (s);
      error (messages[decl_start_idx], 
	       "Instance or class variable declaration syntax error:\n\t"
	       "\n\t%s\n", buf);
    }
  }
}

void var_shadows_method_warning (MESSAGE_STACK messages,
				 int tok_idx) {
  char *s, expr_buf_out[MAXMSG];
  int expr_end_idx;
  /* We need this call to find the end of the expression. */
  (void)fmt_rt_expr (messages, tok_idx, &expr_end_idx, expr_buf_out);
  s = collect_tokens (messages, tok_idx, expr_end_idx);
  warning (messages[tok_idx], "Object \"%s\" shadows a method:\n\t%s\n",
	   M_NAME(messages[tok_idx]), s);
  free (s);
}

void unsupported_return_type_warning_a (MESSAGE_STACK messages,
					int expr_start_idx) {
  int keyword_idx;
  int expr_end_idx;
  char *buf = NULL;

  if ((keyword_idx = prevlangmsg (messages, expr_start_idx)) != ERROR) {
    if ((expr_end_idx = scanforward (messages, expr_start_idx,
				     get_stack_top (messages),
				     SEMICOLON)) != ERROR) {
      buf = collect_tokens (messages, keyword_idx, expr_end_idx);
    } else {
      expr_end_idx = nextlangmsg (messages, expr_start_idx);
      buf = collect_tokens (messages, keyword_idx, expr_end_idx);
    }
  } else {
    if ((expr_end_idx = scanforward (messages, expr_start_idx,
				     get_stack_top (messages),
				     SEMICOLON)) != ERROR) {
      buf = collect_tokens (messages, expr_start_idx, expr_end_idx);
    }
  }

  if (interpreter_pass == method_pass && nolinemarker_opt) {
    /* Make the line numbers method-local. */
    warning (messages[expr_start_idx],
	     "In method, \"%s,\" (class %s):\n",
	     new_methods[new_method_ptr + 1] -> method -> name,
	     rcvr_class_obj -> __o_name);
    if (buf) {
      warning (messages[expr_start_idx], 
	       "Unsupported return type, \"%s,\" in expression,\n\n\t%s\n",
	       M_NAME(messages[expr_start_idx]), buf);
      __xfree (MEMADDR(buf));
    } else {
      warning (messages[expr_start_idx], 
	       "Unsupported type, \"%s,\" in return expression.\n",
	       M_NAME(messages[expr_start_idx]));
    }
  } else {
    if (buf) {
      warning (messages[expr_start_idx], 
	       "Unsupported return type, \"%s,\" in expression,\n\n\t%s\n",
	       M_NAME(messages[expr_start_idx]), buf);
      __xfree (MEMADDR(buf));
    } else {
      warning (messages[expr_start_idx], 
	       "Unsupported type, \"%s,\" in return expression.\n",
	       M_NAME(messages[expr_start_idx]));
    }
  }

}

void object_does_not_understand_msg_warning_a (MESSAGE *m_orig,
					       char *prev_tok_name,
					       char *prev_tok_classname,
					       char *tok_name) {
  warning 
    (m_orig, 
     "Object, \"%s,\" (Class %s) does not understand message, \"%s.\"",
     prev_tok_name,
     prev_tok_classname,
     tok_name);
}

void undefined_method_follows_class_object (MESSAGE *m_sender) {
  char s[MAXLABEL];
  strcatx (s, "Undefined label, \"", m_sender -> name, 
	   ",\" follows a class object without a constructor.", NULL);
  warning (m_sender, s);
  __ctalkExceptionInternal (m_sender, undefined_method_x,
			    m_sender -> name,0);
}

void arg_shadows_a_c_variable_warning (MESSAGE *m_orig, char *argstr) {
  warning (m_orig,
	   "Argument, \"%s,\" shadows a C variable or function.",
	   argstr);
}

void resolve_undefined_param_class_error (MESSAGE *m_orig,
					  char *param_class,
					  char *param_name) {
  error (m_orig, 
   "resolve: Undefined class, \"%s,\" in definition of parameter, \"%s,\".",
			   param_class, param_name);
}

void undefined_self_fmt_arg_class (MESSAGE_STACK messages, 
				int start_idx, int end_idx) {
  char *buf;
  buf = collect_tokens (messages, start_idx, end_idx);
  warning (messages[start_idx],
	   "Can't find the class of format argument:\n\n\t%s" 
	   "\n\nUsing Integer as the default class.",
	   buf);
  __xfree (MEMADDR(buf));
}

void return_expr_undefined_label (MESSAGE_STACK messages,
				  int start_idx, int end_idx,
				  int undefined_label_idx) {
  char *buf;
  buf = collect_tokens (messages, start_idx, end_idx);
  warning (messages[undefined_label_idx], 
	   "Undefined label: \"%s\" in expression:"
	   "\n\n\t%s\n",
	   M_NAME(messages[undefined_label_idx]), buf);
  __xfree (MEMADDR(buf));
}

void primitive_arg_shadows_a_parameter_warning (MESSAGE_STACK messages,
						int arg_idx) {
  if (interpreter_pass == method_pass) {
    warning (messages[arg_idx], "Argument, \"%s,\" shadows a method "
	     "parameter.", M_NAME(messages[arg_idx]));
  }
}

void assignment_in_constant_expr_warning (MESSAGE_STACK messages,
					  int start_idx,
					  int end_idx) {
  char *buf;
  buf = collect_tokens (messages, start_idx, end_idx);
  if (messages != message_stack ()) {
    /* Stacks like fn_messages keep track of error_line
       independently of the main stack. */
    messages[start_idx] -> error_line = error_line;
  }
  warning (messages[start_idx],
	   "Assignment in constant expression:\n\n\t%s\n",
	   buf);
  __xfree (MEMADDR(buf));
}

/* for now, only works for ++ and -- at the end of an expression -
   doesn't worry about right-associativity with following labels. */
bool postfix_following_method_warning (MESSAGE_STACK messages,
			       int postfix_tok_idx) {
  int prev_tok_idx,
    lookback, stack_start_idx,
    lookahead;
  MESSAGE *m_postfix_tok, *m_prev;
  METHOD *m;

  m_postfix_tok = messages[postfix_tok_idx];

  if ((prev_tok_idx = prevlangmsg (messages, postfix_tok_idx)) != ERROR) {
    m_prev = messages[prev_tok_idx];
    if (M_TOK(m_prev) == LABEL || M_TOK(m_prev) == METHODMSGLABEL) {
      if (!IS_OBJECT(m_prev -> obj)) {
	stack_start_idx = stack_start (messages);
	for (lookback = prev_tok_idx + 1; lookback <= stack_start_idx;
	     ++lookback) {
	  if (IS_OBJECT(messages[lookback] -> obj))
	    break;
	}
	if ((lookback <= stack_start_idx) &&
	    IS_OBJECT(messages[lookback] -> obj)) {
	  if (lookback != prev_tok_idx) {
	    if (((m = get_instance_method
		  (m_postfix_tok,
		   messages[lookback] -> obj -> __o_class,
		   M_NAME(m_prev), ANY_ARGS, FALSE))
		 != NULL) ||
		((m = get_class_method
		  (m_postfix_tok,
		   messages[lookback] -> obj -> __o_class,
		   M_NAME(m_prev), ANY_ARGS, FALSE)) != NULL)) {
	      
	      if (m -> n_params == 0 && !m -> varargs && !m -> prefix) {
		/* don't worry about right-associativity for now -
		   assume that if the method has no args then the
		   operator was intended to increment the method
		   result. */
		if (messages == m_messages) {
		  if (postfix_tok_idx == (m_message_ptr + 1))
		    warning (m_postfix_tok, "Postfix %s following a method may "
			     "have no effect.", M_NAME(m_postfix_tok));
		  return TRUE;
		} else if (messages == message_stack ()) {
		  lookahead = nextlangmsg (messages, postfix_tok_idx);
		  if (METHOD_ARG_TERM_MSG_TYPE(messages[lookahead])) {
		    warning (m_postfix_tok, "Postfix %s following a method may "
			     "have no effect.", M_NAME(m_postfix_tok));
		    return TRUE;
		  }
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

/* called only by method_args.  prints warning and outputs a run-time
   expression which may or may not work. used in expressions that don't
   have lvalues, like:
   
   myInt asCharacter++;
*/
bool rcvr_cx_postfix_warning (MESSAGE_STACK messages, int postfix_tok_idx) {
  MESSAGE *m;
  int i, rcvr_idx = postfix_tok_idx;
  char *s, expr_out[MAXMSG], expr_out_2[MAXMSG];
  bool rval;
  OBJECT_CONTEXT expr_context;

  if ((rval = postfix_following_method_warning (messages, postfix_tok_idx))
      == true) {
    /* NoTE: this is always the main message stack, so we can use 
       P_MESSAGES directly. */
    /* NoTe 2: this is a only very simple receiver lookback so far. */
    for (i = postfix_tok_idx; i <= P_MESSAGES; ++i) {
      m = messages[i];
      if (IS_OBJECT(m -> obj)) {
	rcvr_idx = i;
	break;
      }
    }
    expr_context = object_context (messages, rcvr_idx);
    switch (expr_context)
      {
      case c_argument_context:
	if (is_fmt_arg (messages, rcvr_idx, P_MESSAGES,
			get_stack_top (messages))) {
	  s = collect_tokens (messages, rcvr_idx, postfix_tok_idx);
	  fmt_eval_expr_str (s, expr_out);
	  fmt_printf_fmt_arg (messages, rcvr_idx, P_MESSAGES,
			      expr_out,
			      expr_out_2);
	  fileout (expr_out_2, FALSE, rcvr_idx);
	} else {
	  s = collect_tokens (messages, rcvr_idx, postfix_tok_idx);
	  fileout (fmt_eval_expr_str (s, expr_out), FALSE, rcvr_idx);
	}
	break;
      default:
	s = collect_tokens (messages, rcvr_idx, postfix_tok_idx);
	fileout (fmt_eval_expr_str (s, expr_out), FALSE, rcvr_idx);
	break;
      }
    for (i = rcvr_idx; i >= postfix_tok_idx; i--) {
      m = messages[i];
      ++m -> evaled;
      ++m -> output;
    }

    __xfree (MEMADDR(s));
  }
  

  return rval;
}

/* basically anything that is a <++|--><rcvr_label> <method_label>
   expression needs a warning */
void prefix_inc_before_method_expr_warning (MESSAGE_STACK messages,
					    int prefix_tok_idx) {
  int lookahead, lookahead_2;

  if (interpreter_pass != expr_check) {
    if (M_TOK(messages[prefix_tok_idx]) == INCREMENT ||
	M_TOK(messages[prefix_tok_idx]) == DECREMENT) {
      if ((lookahead = nextlangmsg (messages, prefix_tok_idx)) != ERROR) {
	if ((lookahead_2 = nextlangmsg (messages, lookahead)) != ERROR) {
	  if ((M_TOK(messages[lookahead]) == LABEL) &&
	      ((M_TOK(messages[lookahead_2]) == LABEL) ||
	       (M_TOK(messages[lookahead_2]) == METHODMSGLABEL))) {
	    warning (messages[prefix_tok_idx],
		     "Prefix %s before a method expression may "
		     "have no effect.", M_NAME(messages[prefix_tok_idx]));
	  }
	}
      }
    }
  }
}

/* this is only for ctrlblk_predicates so far */
bool is_OBJECT_ptr_array_deref (MESSAGE_STACK messages, int basename_idx) {
  int open_array_idx, close_array_idx, next_idx, struct_end_idx,
    post_struct_idx;
  char *expr, expr_buf[MAXMSG];
  CTRLBLK *c;

  if ((c = ctrlblks[ctrlblk_ptr+1]) == NULL)
    return false;
  
  if ((open_array_idx = nextlangmsg (messages, basename_idx)) != ERROR) {
    if ((close_array_idx = is_subscript_expr
	 (messages, basename_idx, get_stack_top (messages))) > 0) {
      if ((next_idx = nextlangmsg (messages, close_array_idx)) != ERROR) {
	if (M_TOK(messages[next_idx]) == DEREF) {
	  if ((struct_end_idx = struct_end (messages, basename_idx,
					    get_stack_top (messages)))
	      > 0) {
	    if (argblk) {
	      /* not supported by cvartab fns yet.... */
	      expr = collect_tokens (messages,
				     c -> keyword_ptr,
				     c -> pred_end_ptr);
	      /* the error function does not return, so we strcpy first */
	      strcpy (expr_buf, expr);
	      error (messages[basename_idx],
		     "In the expression:"
		     "\n\n\t%s\n\nDereferencing subscripted "
		     "OBJECT * members are "
		     "not (yet) supported in this context.", expr_buf);
	    } else {
	      /* look for non-legal syntax in an otherwise plain
		 C expression (i.e., label following a label). */
	      if ((post_struct_idx = nextlangmsg (messages,
						  struct_end_idx))
		  != ERROR) {
		if ((M_TOK(messages[post_struct_idx]) == LABEL) ||
		    (M_TOK(messages[post_struct_idx]) == METHODMSGLABEL)) {
		  expr = collect_tokens (messages,
					 c -> keyword_ptr,
					 c -> pred_end_ptr);
		/* the error function does not return, so we strcpy first */
		  strcpy (expr_buf, expr);
		  error (messages[basename_idx],
			 "In the expression:"
			 "\n\n\t%s\n\nDereferencing subscripted "
			 "OBJECT * members are "
			 "not (yet) supported in this context.", expr_buf);
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return false;
}

/* only exits with an error if there is a ++ or -- used with the
   subscripted int type (arrays are not constructed, char * arrays
   can be continguous, so they can work when we refer to it elsewhere) */
bool subscripted_int_in_code_block_error (MESSAGE_STACK messages,
					  int var_tok, CVAR *c) {
  int subscript_end, next_ptr, prev_ptr;

  if (c -> type_attrs == CVAR_TYPE_CHAR)
    return false;

  if ((subscript_end = is_subscript_expr (messages, var_tok,
					  get_stack_top (messages)))
      == ERROR)
    return false;

  if ((next_ptr = nextlangmsg (messages, subscript_end))!= ERROR) {
    if ((prev_ptr = prevlangmsg (messages, var_tok)) != ERROR) {
      if (M_TOK(messages[next_ptr]) != INCREMENT &&
	  M_TOK(messages[next_ptr]) != DECREMENT &&
	  M_TOK(messages[prev_ptr]) != INCREMENT &&
	  M_TOK(messages[prev_ptr]) != DECREMENT) {
	return false;
      }
    } else if (M_TOK(messages[next_ptr]) != INCREMENT &&
	       M_TOK(messages[next_ptr]) != DECREMENT) {
      return false;
    }
  } else if ((prev_ptr = prevlangmsg (messages, var_tok)) != ERROR) {
    if (M_TOK(messages[prev_ptr]) != INCREMENT &&
	M_TOK(messages[prev_ptr]) != DECREMENT) {
      return false;
    }
  } else if (next_ptr == ERROR && prev_ptr == ERROR) {
    return false;
  }
  /* error does not return */
  error (messages[var_tok], "Subscripted %ss, with a unary ++ or --, are "
	 "not (yet) supported in this context.", c -> type);

  return false;
}

void object_follows_object_error (MESSAGE_STACK messages,
				  int rcvr_tok, int next_tok) {
  char expr_buf[MAXMSG], *expr;
  expr = collect_tokens (messages, rcvr_tok, next_tok);
  strcpy (expr_buf, expr);
  __xfree (MEMADDR(expr));
  error (messages[rcvr_tok],
	 "In the expression:"
	 "\n\n\t%s\n\nLabel, \"%s,\" is not expected after "
	 "label, \"%s.\"",
	 expr_buf, M_NAME(messages[next_tok]),
	 M_NAME(messages[rcvr_tok]));
}

/* this might be a better place to put this, if when starting
   to parse a method, we don't have line number info yet... */
void method_shadows_c_keyword_warning_a (MESSAGE_STACK messages,
					 int method_tok,
					 int prev_tok) {
  char expr_buf[MAXMSG], *expr;
  expr = collect_tokens (messages, prev_tok, method_tok);
  strcpy (expr_buf, expr);
  __xfree (MEMADDR(expr));
  warning (messages[prev_tok],
	   "In the expression:"
	   "\n\n\t%s\n\nLabel, \"%s,\" shadows a C keyword.",
	   expr_buf, M_NAME(messages[method_tok]));
}

void self_outside_method_error (MESSAGE_STACK messages,
				int keyword_idx) {
  char *expr, expr_buf[MAXMSG];
  int end_idx;
  MSINFO ms;
  ms.messages = messages;
  ms.stack_start = stack_start (messages);
  ms.stack_ptr = get_stack_top (messages);
  ms.tok = keyword_idx;
  expr = collect_expression (&ms, &end_idx); 
  strcpy (expr_buf, expr);
  __xfree (MEMADDR(expr));
  error (messages[keyword_idx],
	   "In the expression:"
	   "\n\n\t%s\n\nKeyword, \"self,\" is used outside of a method.",
	   expr_buf);
}

void object_follows_a_constant_warning (MESSAGE_STACK messages,
					int constant_ptr,
					int next_label_ptr) {
  switch (messages[constant_ptr] -> tokentype)
    {
    case LITERAL:
      warning (messages[constant_ptr],
	       "Identifier, \"%s\", follows a string constant.",
	       M_NAME(messages[next_label_ptr]));
      break;
    case LITERAL_CHAR:
      warning (messages[constant_ptr],
	       "Identifier, \"%s\", follows a character constant.",
	       M_NAME(messages[next_label_ptr]));
      break;
    case INTEGER:
      warning (messages[constant_ptr],
	       "Identifier, \"%s\", follows an int constant.",
	       M_NAME(messages[next_label_ptr]));
      break;
    case LONG:
      warning (messages[constant_ptr],
	       "Identifier, \"%s\", follows a long int constant.",
	       M_NAME(messages[next_label_ptr]));
      break;
    case LONGLONG:
      warning (messages[constant_ptr],
	       "Identifier, \"%s\", follows a long long int constant.",
	       M_NAME(messages[next_label_ptr]));
      break;
    case FLOAT:
      warning (messages[constant_ptr],
	       "Identifier, \"%s\", follows a floating point constant.",
	       M_NAME(messages[next_label_ptr]));
      break;
    }
}

void unknown_format_conversion_warning (MESSAGE_STACK messages,
					int expr_tok) {
  int fmt_str_lookback, fn_label_lookback, terminator_lookahead,
    stack_start_idx;
  char *errbuf;

  stack_start_idx = stack_start (messages);
  if ((fmt_str_lookback = scanback (messages, expr_tok,
				    stack_start_idx,
				    LITERAL)) != ERROR) {
    if ((fn_label_lookback = scanback (messages,
				       fmt_str_lookback,
				       stack_start_idx,
				       LABEL)) != ERROR) {
      if ((terminator_lookahead = scanforward (messages,
					       expr_tok,
					       get_stack_top (messages),
					       SEMICOLON))
	  != ERROR) {
	errbuf = collect_tokens (messages, fn_label_lookback,
				 terminator_lookahead);
	warning (messages[expr_tok],
		 "Expression has unknown format conversion "
		 "specifier:\n\n\t%s\n\n",
		 errbuf);
	__xfree (MEMADDR(errbuf));
      }
    }
  }
}

void unknown_format_conversion_warning_ms (MSINFO *ms) {
  return unknown_format_conversion_warning (ms -> messages, ms -> tok);
}

/* also checks for a method label as the final label before a '=' */
void self_instvar_expr_unknown_label (MESSAGE_STACK messages,
				      int self_idx, int unknown_tok_idx) {
  char *s, errbuf[MAXMSG];
  int next_idx;

  if (is_instance_method (messages, unknown_tok_idx) ||
      is_proto_selector (M_NAME(messages[unknown_tok_idx]))) {
    if ((next_idx = nextlangmsg (messages, unknown_tok_idx)) != ERROR) {
      if (IS_C_ASSIGNMENT_OP(M_TOK(messages[next_idx]))) {
	s = collect_tokens (messages, self_idx, next_idx);
	strcpy (errbuf, s);
	__xfree (MEMADDR(s));
	error (messages[unknown_tok_idx], "Lvalue required:\n\n\t%s\n\n",
	       errbuf);
      } else {
	return;
      }
    } else {
      return;
    }
  }

  s = collect_tokens (messages, self_idx, unknown_tok_idx);
  strcpy (errbuf, s);
  __xfree (MEMADDR(s));

  error (messages[unknown_tok_idx], "Undefined label, \"%s,\" in "
	   "expression.\n\n\t%s\n\n", M_NAME(messages[unknown_tok_idx]),
	   errbuf);
}

/***/
/* This is still preliminary. */
void instancevar_wo_rcvr_warning (MESSAGE_STACK messages, int tok_idx,
				  bool first_label, int main_stack_idx) {

  MESSAGE *m_tok;
  int prev_tok;

  m_tok = messages[tok_idx];

  if (M_TOK(m_tok) == LABEL &&
      interpreter_pass != expr_check) {
    if (!first_label) {
      if ((prev_tok = prevlangmsg (messages, tok_idx)) != -1) {
	if ((M_TOK(messages[prev_tok]) != LABEL) &&
	    (M_TOK(messages[prev_tok]) != CLOSEPAREN) &&
	    (M_TOK(messages[prev_tok]) != ARRAYCLOSE) &&
	    !IS_CONSTANT_TOK(M_TOK(messages[prev_tok]))) {
	  /* a closing paren as the previous token
	     can be the end of a receiver constant,
	     subscripted constant, or expression;
	     i.e., the label is a method, so don't 
	     check if the label is defined here */
	  if (!(m_tok -> attrs & TOK_SELF) &&
	      !(m_tok -> attrs & TOK_SUPER) &&
	      !IS_DEFINED_LABEL(M_NAME(m_tok))) {
	    if (_hash_get (defined_instancevars, M_NAME(m_tok)))
	      warning (message_stack_at (main_stack_idx),
		       "Instance variable, \"%s,\" used without a receiver.",
		       M_NAME(m_tok));
	    else
	      warning (message_stack_at (main_stack_idx),
		       "Undefined label, \"%s.\"",
		       M_NAME(m_tok));
	  }
	}
      }
    }
  }
}

char *collect_errmsg_expr (MESSAGE_STACK messages, int tok_idx) {
  int last_sep_semicolon, last_sep_closeblock;
  int next_sep;
  char *expr = NULL;
  last_sep_semicolon = scanback (message_stack (), tok_idx,
				  stack_start (message_stack ()), SEMICOLON);
  last_sep_closeblock = scanback (message_stack (), tok_idx,
				  stack_start (message_stack ()), CLOSEBLOCK);
  next_sep = scanforward (message_stack (), tok_idx,
			  get_stack_top (message_stack ()), SEMICOLON);
  
  if (last_sep_closeblock < last_sep_semicolon) {
    for (;last_sep_closeblock > next_sep; last_sep_closeblock--) {
      if ((M_TOK(messages[last_sep_closeblock]) != CLOSEBLOCK) &&
	  !M_ISSPACE(messages[last_sep_closeblock])) {
	expr = collect_tokens (messages, last_sep_closeblock,
			       next_sep);
	break;
      }
    }
  } else {
    for (;last_sep_semicolon > next_sep; last_sep_semicolon--) {
      if ((M_TOK(messages[last_sep_semicolon]) != SEMICOLON) &&
	  !M_ISSPACE(messages[last_sep_semicolon])) {
	expr = collect_tokens (messages, last_sep_semicolon,
			       next_sep);
	break;
      }
    }
    expr = collect_tokens (messages, last_sep_semicolon, next_sep);
  }
  return expr;
}

void undefined_blk_method_warning (MESSAGE *m_orig,
				   MESSAGE *m_rcvr,
				   MESSAGE *m_method) {
  if (IS_OBJECT (m_rcvr->obj)) {
    if (IS_OBJECT (m_rcvr -> obj -> instancevars)) {
      warning (m_orig, "Undefined method, \"%s.\" Receiver, \"%s.\""
	       " (class %s).\n",
	       M_NAME(m_method), M_NAME(m_rcvr),
	       m_rcvr -> obj -> instancevars -> __o_class -> __o_name);
    } else {
      warning (m_orig, "Undefined method, \"%s.\" Receiver, \"%s.\""
	       " (class %s).\n,",
	       M_NAME(m_method), M_NAME(m_rcvr),
	       m_rcvr -> obj -> __o_class -> __o_name);
    }
  } else {
    warning (m_orig, "Undefined method, \"%s.\" Receiver, \"%s.\"",
	     M_NAME(m_method), M_NAME(m_rcvr));
  }
}

OBJECT *resolve_rcvr_is_undefined (MESSAGE *m_rcvr, MESSAGE *m_method) {
  /* And we need to check for secure osx lib replacements,
     and we'll skip them in parser pass. */
#if defined __APPLE__ && defined _x86_64
  if (!strstr (m_method -> name, "__builtin_")) {
    return NULL;
  } else if (str_eq (M_NAME(m_method), "instanceVariable") ||
	     str_eq (M_NAME(m_method), "classVariable")) {
    if (m_method -> receiver_obj == NULL) {
      error (m_method, "Method \"%s:\" Undefined receiver, \"%s.\"",
	     M_NAME(m_method), M_NAME(m_rcvr));
    }
  } else {
    return NULL;
  }
#else	  
  if (str_eq (M_NAME(m_method), "instanceVariable") ||
      str_eq (M_NAME(m_method), "classVariable")) {
    /* we have to look for bad receiver names here ... */
    if (m_method -> receiver_obj == NULL) {
      error (m_method, "Method \"%s:\" Undefined receiver, \"%s.\"",
	     M_NAME(m_method), M_NAME(m_rcvr));
    }
  } else {
    return M_VALUE_OBJ (m_rcvr);
  }
#endif	
  return NULL;
}
