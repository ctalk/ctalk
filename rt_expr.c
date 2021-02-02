/* $Id: rt_expr.c,v 1.3 2021/01/01 17:50:57 rkiesling Exp $ */

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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "objtoc.h"

extern I_PASS interpreter_pass;    /* Declared in main.c.               */

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;
extern OBJECT *rcvr_class_obj;        /* Declared in lib/rtnwmthd.c. */

extern bool ctrlblk_pred;          /* Declared in control.c. */
extern bool argblk;                /* Declared in argblk.c. */
extern bool for_init,              /* States of for loop predicates.         */
  for_term,
  for_inc;
extern bool ptr_fmt_is_alt_int_fmt; /* Declared in pattypes.c. */
extern char *ascii[8193];             /* from intascii.h */
extern bool resolve_ctrlblk_cvar_reg;

extern CTRLBLK *ctrlblks[MAXARGS + 1];  /* Declared in control.c. */
extern int ctrlblk_ptr;
extern bool cpre_have_cvar_reg;        /* Declared in ifexpr.c.  */

#define _MPUTCHAR(__m,__c) (__m)->name[0] = (__c); (__m)->name[1] = '\0';

extern DEFAULTCLASSCACHE *ct_defclasses; 

extern HASHTAB defined_instancevars;
extern HASHTAB declared_method_names;


/* If the previous label was considered
   as an instance variable messages, but it's actually a method
   message of its previous  label, return true. */
static bool prev_instvar_is_actually_method (MESSAGE_STACK messages,
					     int idx) {
  int prev_idx;
  MESSAGE *prev_message;
  METHOD *method;

  if ((prev_idx = prevlangmsg (messages, idx)) != ERROR) {
    prev_message = messages[prev_idx];
    if (M_TOK(prev_message) != LABEL)
      return false;

    if (IS_OBJECT(prev_message -> receiver_obj)) {
      if (((method = get_instance_method (prev_message,
					  prev_message -> receiver_obj ->
					  instancevars,
					  M_NAME(prev_message),
					  ANY_ARGS,
					  FALSE)) != NULL) ||
	  ((method = get_class_method (prev_message,
				       prev_message -> receiver_obj ->
				       instancevars,
				       M_NAME(prev_message),
				       ANY_ARGS,
				       FALSE)) != NULL)) {
	return true;
      }
    }
  }
  return false;
}

char *fmt_eval_expr_str (char *expr, char *expr_out) {
  /* strange, but it avoids buffer overflow warnings */
  char trimstr_out[MAXMSG-20], esc_out[MAXMSG-20];
  trim_leading_whitespace (expr, trimstr_out);
  escape_str_quotes (trimstr_out, esc_out);
  strcatx (expr_out, EVAL_EXPR_FN, "(\"", esc_out, "\")", NULL);
  de_newline_buf (expr_out);
  return expr_out;
}

char *fmt_eval_u_expr_str (char *expr, char *expr_out) {
  /* see the comment above */
  char trimstr_out[MAXMSG-20], esc_out[MAXMSG-20];
  trim_leading_whitespace (expr, trimstr_out);
  escape_str_quotes (trimstr_out, esc_out);
  snprintf (expr_out, MAXMSG, "%s(\"%s\")", EVAL_EXPR_FN_U, esc_out);
  de_newline_buf (expr_out);
  return expr_out;
}

char *esc_expr_quotes (char *, char *);

char *fmt_pattern_eval_expr_str (char *expr, char *expr_out) {
  /* see the comment above */
  char trimstr_out[MAXMSG-20], esc_out[MAXMSG-20];
  trim_leading_whitespace (expr, trimstr_out);
  /* TODO - Maybe we can just use the tokens that we have,
     instead of letting this fn re-tokenize the expression
     again. */
  esc_expr_and_pattern_quotes (trimstr_out, esc_out);
  snprintf (expr_out, MAXMSG, "%s(\"%s\")", EVAL_EXPR_FN, esc_out);
  de_newline_buf (expr_out);
  return expr_out;
}

/*
 *  Simple or complex expression that should be evaled at run time.
 */
char *fmt_rt_expr (MESSAGE_STACK messages, int start_idx, int *end_idx,
		   char *expr_out) {
  char exprbuf[MAXMSG];
  int idx;
  MSINFO ms;

  ms.messages = messages;
  ms.stack_start = stack_start (messages);
  ms.stack_ptr = get_stack_top (messages);
  ms.tok = start_idx;
  collect_expression_buf (&ms, end_idx, exprbuf); 
  for (idx = start_idx; idx >= *end_idx; idx--) {
    /* NOTE: This should be only temporary fix - if
       we have an empty message at the end of a subexpr.
       See ctxlogo.ca for this to happen. */
    if (messages[idx]) {
      messages[idx]->attrs |= TOK_IS_RT_EXPR;
    } else {
      break;
    }
  }
  fmt_eval_expr_str (exprbuf, expr_out);
  return expr_out;
}

char *fmt_rt_expr_ms (MSINFO *ms, int *end_idx, char *expr_out) {
  char exprbuf[MAXMSG];
  int idx;

  collect_expression_buf (ms, end_idx, exprbuf);
  for (idx = ms -> tok; idx >= *end_idx; idx--) {
    /* See the note above. */
    if (ms -> messages[idx]) {
      ms -> messages[idx]->attrs |= TOK_IS_RT_EXPR;
    } else {
      break;
    }
  }
  fmt_eval_expr_str (exprbuf, expr_out);
  return expr_out;
}

/*
 *  Like fmt_rt_expr, but handles CVAR argblk translation.
 */
char *fmt_rt_argblk_expr (MESSAGE_STACK messages, int start_idx, int *end_idx,
		   char *expr_out) {
  char *exprbuf;
  int idx, next_idx;
  CVAR *cvar;
  MSINFO ms;

  ms.messages = messages;
  ms.stack_start = stack_start (messages);
  ms.stack_ptr = get_stack_top (messages);
  ms.tok = start_idx;

  /* called twice - the first time to find the end of the
     expression. */
  
  exprbuf = collect_expression (&ms, end_idx); 

  for (idx = start_idx; idx >= *end_idx; --idx) {
    if (M_TOK(messages[idx]) != LABEL)
      continue;
    if ((cvar = get_local_var (M_NAME(messages[idx]))) != NULL) {
      next_idx = nextlangmsg (messages, idx);
      handle_cvar_argblk_translation (messages, idx, next_idx,
				      cvar);
      messages[idx] -> attrs |= TOK_OBJ_IS_CREATED_CVAR_ALIAS;
    }
  }

  __xfree (MEMADDR(exprbuf));
  exprbuf = collect_expression (&ms, end_idx); 

  for (idx = start_idx; idx >= *end_idx; idx--)
    messages[idx]->attrs |= TOK_IS_RT_EXPR;

  fmt_eval_expr_str (exprbuf, expr_out);
  __xfree (MEMADDR(exprbuf));
  return expr_out;
}

int rte_expr_contains_c_fn_arg_call (MESSAGE_STACK messages,
					    int start, int end) {
  int i, i_next;
  for (i = start; i >= end; --i) {
    if (M_TOK(messages[i]) == LABEL) {
      if (get_function (M_NAME(messages[i]))) {
	i_next = nextlangmsgstack (messages, i);
	if (M_TOK(messages[i_next]) == OPENPAREN) {
	  return i;
	}
      }
    }
  }
  return ERROR;
}
					

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;

int is_method_param_name (char *s) {
  METHOD *m;
  int n_th_param;
  if (interpreter_pass != method_pass)
    return FALSE;

  if ((m = new_methods[new_method_ptr+1] -> method) != NULL) {
    for (n_th_param = 0; n_th_param < m -> n_params; n_th_param++) {
      if (!strcmp (s, m -> params[n_th_param]->name))
	return TRUE;
    }
  }
  return FALSE;
}

/* TODO - See if we can adapt the clauses that output 
   obj_expr_is_arg in other modules to use this. */
static bool rte_output_fn_arg (MESSAGE_STACK messages, int start_idx,
			       int stack_start_idx, char *expr) {
  int arg_idx, fn_idx;
  CFUNC *cfn;

  if ((arg_idx = obj_expr_is_arg (messages, start_idx,
				  stack_start_idx, &fn_idx))
      != ERROR) {
    if ((cfn = get_function (M_NAME(messages[fn_idx]))) != NULL) {
      fileout  (fn_param_return_trans
		(messages[fn_idx], cfn, expr, arg_idx),
		0, start_idx);
      return true;
    }
  }
  return false;
}

bool fn_arg_conditional = false;
int fn_cond_arg_fn_label_idx = -1;
/* Set in is_question_conditional_predicate (object.c), cleared below. */
#if 0
extern bool qcond_fn_arg_cvar_registration;
extern int qcond_fn_arg_cvar_registration_idx;
#endif

char *rt_expr (MESSAGE_STACK messages, int start_idx, int *end_idx,
	       char *expr_out) {
  int i, struct_expr_limit, subscript_expr_limit;
  int message_of_rcvr, message_of_arg;
  int i_lbl, next, next_2, matching_paren;
  int fn_name_idx, fn_expr_end_idx;
  int stack_start_idx, stack_top_idx;
  CVAR *__c, *__c_argblk;
  char tmp_fn_alias[MAXLABEL];
  OBJECT *rcvr_obj = NULL;
  METHOD *method = NULL;;
  int n_commas = 0, n_parens = 0;
  bool have_cvar_registration = false;

  stack_start_idx = stack_start (messages);
  stack_top_idx = get_stack_top (messages);

  if (argblk)
    fmt_rt_argblk_expr (messages, start_idx, end_idx, expr_out);
  else
    fmt_rt_expr (messages, start_idx, end_idx, expr_out);
  message_of_rcvr = message_of_arg = FALSE;
  for (i = start_idx; i >= *end_idx; i--) {
    /* Here we can be slightly off on the last message
       when checking constant expressions... see ctxlogo.ca
       for an example of what needs to be checked. */
    if (!messages[i])
      break;
    messages[i] -> attrs |= TOK_IS_RT_EXPR;
    if (M_TOK(messages[i]) == LABEL) {
      /* 
       *  See the comments in rt_self_expr, below.
       *  Should only need to check for receivers that
       *  are method parameters.
       */
      if (is_method_param_name (M_NAME(messages[i])))
	message_of_rcvr = TRUE;
      if (message_of_rcvr) {
	if ((messages[i] -> attrs & TOK_OBJ_IS_CREATED_CVAR_ALIAS) ||
	    /* Convenient attribute set by fmt_rt_argblk_expr, above,
	       although it's sort of a misnomer until we create
	       the object below. */
	    get_global_var_not_shadowed (M_NAME(messages[i]))
	    || get_local_var (M_NAME(messages[i])))  {
	  message_of_rcvr = FALSE;
	} else {
	  /* It's probably an instance variable message if the
	     previous token is a method parameter label */
	  if (!is_method_parameter (messages, i)) {
	    if (!_hash_get (defined_instancevars, M_NAME(messages[i])) &&
		!_hash_get (declared_method_names, M_NAME(messages[i])) &&
		!get_local_object (M_NAME(messages[i]), NULL) &&
		!get_global_object (M_NAME(messages[i]), NULL) &&
		!is_method_proto_name (M_NAME(messages[i])) &&
		!get_class_object (M_NAME(messages[i])) &&
		!(messages[i] -> attrs & TOK_SELF)) {
	      char *err_expr = collect_tokens (messages, start_idx, *end_idx);;
	      warning (messages[i],
		       "Could not resolve label, \"%s\":\n\n\t%s\n\n",
		       M_NAME(messages[i]), err_expr);
	      __xfree (MEMADDR(err_expr));
	    }
	  }
	  continue;
	}
      }
      /*
       *   Not receiver, instance or class variable message.
       */
      if ((messages[i] -> obj == NULL) && 
	  (messages[i] -> receiver_obj == NULL)) {
	i_lbl = 0;
	__c_argblk = NULL;
	while (messages[i] -> name[i_lbl] == '*')
	  ++i_lbl;
	if (((__c = get_local_var (M_NAME(messages[i]))) != NULL) ||
	    ((__c = get_global_var_not_shadowed (M_NAME(messages[i])))
	     != NULL) ||
	     ((__c_argblk = get_var_from_cvartab_name
	       (&messages[i] -> name[i_lbl])) != NULL)) {
	  if (is_struct_or_union_expr (messages, i, 
				       stack_start_idx,
				       stack_top_idx)) {
	    if (fn_arg_conditional) {
	      generate_register_c_method_arg_call 
		(__c, 
		 struct_or_union_expr (messages, i, 
				       stack_top_idx,
				       &struct_expr_limit),
		 LOCAL_VAR,
		 fn_cond_arg_fn_label_idx);
	    } else {
	      generate_register_c_method_arg_call 
		(__c, 
		 struct_or_union_expr (messages, i, 
				       stack_top_idx,
				       &struct_expr_limit),
		 LOCAL_VAR,
		 start_idx);
	    }
	    i = struct_expr_limit;
	    have_cvar_registration = true;
	  } else if (need_cvar_argblk_translation (__c_argblk)) {
	    register_argblk_cvar_from_basic_cvar
	      (messages, i, start_idx, __c_argblk);
	    have_cvar_registration = true;
	  } else {
	    if (is_subscript_expr (messages, i, 
				   stack_top_idx) > 0) {
	      generate_register_c_method_arg_call 
		(__c, 
		 subscript_cvar_registration_expr (messages, i, 
						   stack_top_idx,
						   &subscript_expr_limit),
		 LOCAL_VAR,
		 start_idx);
	      i = subscript_expr_limit;
	      have_cvar_registration = true;
	    } else {
	      if (fn_arg_conditional) {
		generate_register_c_method_arg_call
		  (__c, M_NAME(messages[i]),
		   LOCAL_VAR,
		   fn_cond_arg_fn_label_idx);
	      } else {
		generate_register_c_method_arg_call (__c, M_NAME(messages[i]),
						     LOCAL_VAR,
						     start_idx);
	      }
	      have_cvar_registration = true;
	    }
	  }
	} else if ((fn_name_idx = rte_expr_contains_c_fn_arg_call
		    (messages, start_idx, *end_idx)) != ERROR)  {
	  int i_2, major_fn_idx;
	  if (fn_arg_conditional && (obj_expr_is_arg
				     (messages, start_idx, stack_start_idx,
				      &major_fn_idx) >= 0)) {
	    /* This is probably going to need to handle other expression
	       variants, so it has its own clause for now. */
	    format_fn_call_method_expr_block_cond (messages, fn_name_idx,
						   &fn_expr_end_idx,
						   tmp_fn_alias);
	    for (i_2 = fn_name_idx; i_2 >= fn_expr_end_idx; --i_2) {
	      messages[i_2] -> name[0] = ' ', messages[i_2] -> name[1] = '\0';
	      messages[i_2] -> tokentype = WHITESPACE;
	    }
	    memset (messages[fn_name_idx] -> name, 0, MAXLABEL);
	    strncpy (messages[fn_name_idx] -> name, tmp_fn_alias, MAXLABEL);
	    fmt_rt_expr (messages, start_idx, end_idx, expr_out);
	  } else {
	    format_fn_call_method_expr_block (messages, fn_name_idx,
					      &fn_expr_end_idx,
					      tmp_fn_alias);
	    for (i_2 = fn_name_idx; i_2 >= fn_expr_end_idx; --i_2) {
	      messages[i_2] -> name[0] = ' ', messages[i_2] -> name[1] = '\0';
	      messages[i_2] -> tokentype = WHITESPACE;
	    }
	    memset (messages[fn_name_idx] -> name, 0, MAXLABEL);
	    strncpy (messages[fn_name_idx] -> name, tmp_fn_alias, MAXLABEL);
	  }
	  if (argblk) {
	    fmt_rt_argblk_expr (messages, start_idx, end_idx, expr_out);
	  } else if (fn_arg_conditional) {
	    for (i = start_idx; i >= *end_idx; i--) {
	      ++(messages[i] -> evaled);
	      ++(messages[i] -> output);
	    }
	  } else {
	    fmt_rt_expr (messages, start_idx, end_idx, expr_out);
	  }
	  /* Do a quick check so we can warn if the number of arguments
	     is incorrect. */
	  n_commas = n_parens = 0;
	  for (i_2 = start_idx; i_2 >= *end_idx; --i_2) {
	    if (M_ISSPACE(messages[i_2]))
	      continue;
	    if (IS_OBJECT(M_OBJ(messages[i_2])) &&
		(rcvr_obj == NULL)) {
	      rcvr_obj = messages[i_2] -> obj;
	    } else if (rcvr_obj && !method) {
	      if (((method = get_instance_method
		    (messages[i_2],
		     rcvr_obj,
		     M_NAME(messages[i_2]),
		     ERROR, FALSE)) != NULL) ||
		  ((method = get_class_method
		    (messages[i_2], rcvr_obj, M_NAME(messages[i_2]),
		     ERROR, FALSE)) != NULL)) {
		continue;
	      }
	      /* Also check the receiver class in case the method
		 that we're in is a constructor. */
	      if (interpreter_pass == method_pass) 
		if (((method = get_instance_method
		      (messages[i_2],
		       rcvr_class_obj,
		       M_NAME(messages[i_2]),
		       ERROR, FALSE)) != NULL) ||
		    ((method = get_class_method
		      (messages[i_2], rcvr_class_obj, M_NAME(messages[i_2]),
		       ERROR, FALSE)) != NULL)) {
		  continue;
		}
	    } else if (M_TOK(messages[i_2]) == OPENPAREN) {
	      ++n_parens;
	    } else if (M_TOK(messages[i_2]) == CLOSEPAREN) {
	      --n_parens;
	    } else if (M_TOK(messages[i_2]) == ARGSEPARATOR) {
	      if (n_parens == 0) {
		++n_commas;
	      }
	    }
	  }
	  if (method) {
	    if (((n_commas + 1) != method -> n_params) && !method -> varargs) {
	      warning (messages[i], "Incorrect number of arguments to method "
		       "\"%s.\"", method -> name);
	    }
	  }
	} else {
	  /* Set by register_argblk_c_vars_1 () when we construct a 
	     cvartab name for an argument block. */
	  if (!(messages[i] -> attrs & TOK_IS_RT_EXPR)) {
	    undefined_label_check (messages, i);
	  } else if (prev_instvar_is_actually_method (messages, i)) {
	    if (!IS_DEFINED_LABEL(M_NAME(messages[i])) &&
		!get_local_object (M_NAME(messages[i]), NULL) &&
		!get_global_object (M_NAME(messages[i]), NULL)) {
	      __ctalkExceptionInternal (messages[i], 
					undefined_label_x, 
					M_NAME(messages[i]), 0);
	    }
	  }
	}
      }
    } else {
      if (!M_ISSPACE(messages[i])) {
	message_of_rcvr = FALSE;
	/*
	 *  This is necessary almost as a last attempt to warn the
	 *  user in the front end of a syntax error, when the front
	 *  end needs to evaluate an expression lazily, for example an
	 *  object mutation.  So this can certainly be expanded.
	 */
	switch (M_TOK(messages[i]))
	  {
	  case ARGSEPARATOR:
	    message_of_arg = TRUE;
	    break;
	  case EQ:
	    if (message_of_arg)
	      warning (messages[i], 
		       "Possible assignment in method argument.");
	    break;
	  case OPENPAREN:
	    /* We don't need class casts here at the moment, so
	       just elide it. */
	    if ((next = nextlangmsg (messages, i)) != ERROR) {
	      if (get_class_object (M_NAME(messages[next]))) {
		if ((next_2 = nextlangmsg (messages, next)) != ERROR) {
		  if (M_TOK(messages[next_2]) == MULT) {
		    if ((matching_paren = match_paren (messages, i, *end_idx))
			!= ERROR) {
		      int i_2;
		      for (i_2 = i; i_2 >= matching_paren; --i_2) {
			messages[i_2] -> name[0] = ' '; messages[i_2] -> name[1] = 0;
			messages[i_2] -> tokentype = WHITESPACE;
		      }
		      if (argblk)
			fmt_rt_argblk_expr (messages, start_idx, end_idx, expr_out);
		      else
			fmt_rt_expr (messages, start_idx, end_idx, expr_out);
		    }
		  }
		}
	      }
	    }
	    break;
	  }
      }
    }
  }

  if (!rte_output_fn_arg (messages, start_idx, stack_start_idx, expr_out)) {
    fileout (expr_out, 0, start_idx);
  }
  if (have_cvar_registration) {
    output_delete_cvars_call (messages, *end_idx, stack_top_idx);
  }
  if (fn_arg_conditional) {
    fn_arg_conditional = false;
    fn_cond_arg_fn_label_idx = -1;
  }
  for (i = start_idx; i >= *end_idx; i--) {
    if (!messages[i]) break;
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
  return expr_out;
}

/*
 *  Like rt_expr, except it doesn't file out the expression,
 *  places the cvar registration calls before the return statement,
 *  and it sets have_cvar_registration so the calling function
 *  can output a delete method arg cvars call itself.
 */
char *rt_expr_return (MESSAGE_STACK messages, int keyword_ptr,
		      int start_idx, int *end_idx,
		      char *expr_out, bool *have_cvar_registration) {
  int i, struct_expr_limit, subscript_expr_limit;
  int message_of_rcvr, message_of_arg;
  int i_lbl, next, next_2, matching_paren;
  int fn_name_idx, fn_expr_end_idx;
  int stack_start_idx, stack_top_idx;
  CVAR *__c, *__c_argblk;
  char tmp_fn_alias[MAXLABEL];
  OBJECT *rcvr_obj = NULL;
  METHOD *method = NULL;;
  int n_commas = 0, n_parens = 0;


  *have_cvar_registration = false;
  stack_start_idx = stack_start (messages);
  stack_top_idx = get_stack_top (messages);

  if (argblk)
    fmt_rt_argblk_expr (messages, start_idx, end_idx, expr_out);
  else
    fmt_rt_expr (messages, start_idx, end_idx, expr_out);
  message_of_rcvr = message_of_arg = FALSE;
  for (i = start_idx; i >= *end_idx; i--) {
    /* Here we can be slightly off on the last message
       when checking constant expressions... see ctxlogo.ca
       for an example of what needs to be checked. */
    if (!messages[i])
      break;
    messages[i] -> attrs |= TOK_IS_RT_EXPR;
    if (M_TOK(messages[i]) == LABEL) {
      /* 
       *  See the comments in rt_self_expr, below.
       *  Should only need to check for receivers that
       *  are method parameters.
       */
      if (is_method_param_name (M_NAME(messages[i])))
	message_of_rcvr = TRUE;
      if (message_of_rcvr) {
	if ((messages[i] -> attrs & TOK_OBJ_IS_CREATED_CVAR_ALIAS) ||
	    /* Convenient attribute set by fmt_rt_argblk_expr, above,
	       although it's sort of a misnomer until we create
	       the object below. */
	    get_global_var_not_shadowed (M_NAME(messages[i]))
	    || get_local_var (M_NAME(messages[i]))) 
	  message_of_rcvr = FALSE;
	else
	  continue;
      }
      /*
       *   Not receiver, instance or class variable message.
       */
      if ((messages[i] -> obj == NULL) && 
	  (messages[i] -> receiver_obj == NULL)) {
	i_lbl = 0;
	__c_argblk = NULL;
	while (messages[i] -> name[i_lbl] == '*')
	  ++i_lbl;
	if (((__c = get_local_var (M_NAME(messages[i]))) != NULL) ||
	    ((__c = get_global_var_not_shadowed (M_NAME(messages[i])))
	     != NULL) ||
	     ((__c_argblk = get_var_from_cvartab_name
	       (&messages[i] -> name[i_lbl])) != NULL)) {
	  if (is_struct_or_union_expr (messages, i, 
				       stack_start_idx,
				       stack_top_idx)) {
	    generate_register_c_method_arg_call 
	      (__c, 
	       struct_or_union_expr (messages, i, 
				     stack_top_idx,
				     &struct_expr_limit),
	       LOCAL_VAR,
	       keyword_ptr + 1);
	    i = struct_expr_limit;
	    *have_cvar_registration = true;
	  } else if (need_cvar_argblk_translation (__c_argblk)) {
	    register_argblk_cvar_from_basic_cvar
	      (messages, i, start_idx, __c_argblk);
	    *have_cvar_registration = true;
	  } else {
	    if (is_subscript_expr (messages, i, 
				   stack_top_idx) > 0) {
	      generate_register_c_method_arg_call 
		(__c, 
		 subscript_cvar_registration_expr (messages, i, 
						   stack_top_idx,
						   &subscript_expr_limit),
		 LOCAL_VAR,
		 keyword_ptr + 1);
	      i = subscript_expr_limit;
	      *have_cvar_registration = true;
	    } else {
	      generate_register_c_method_arg_call (__c, M_NAME(messages[i]),
						   LOCAL_VAR,
						   keyword_ptr + 1);
	      *have_cvar_registration = true;
	    }
	  }
	} else if ((fn_name_idx = rte_expr_contains_c_fn_arg_call
		    (messages, start_idx, *end_idx)) != ERROR)  {
	  int i_2;
	  format_fn_call_method_expr_block (messages, fn_name_idx,
					      &fn_expr_end_idx,
					      tmp_fn_alias);
	  for (i_2 = fn_name_idx; i_2 >= fn_expr_end_idx; --i_2) {
	    messages[i_2] -> name[0] = ' ', messages[i_2] -> name[1] = '\0';
	    messages[i_2] -> tokentype = WHITESPACE;
	  }
	  memset (messages[fn_name_idx] -> name, 0, MAXLABEL);
	  strncpy (messages[fn_name_idx] -> name, tmp_fn_alias, MAXLABEL);
	  if (argblk)
	    fmt_rt_argblk_expr (messages, start_idx, end_idx, expr_out);
	  else
	    fmt_rt_expr (messages, start_idx, end_idx, expr_out);
	  /* Do a quick check so we can warn if the number of arguments
	     is incorrect. */
	  n_commas = n_parens = 0;
	  for (i_2 = start_idx; i_2 >= *end_idx; --i_2) {
	    if (M_ISSPACE(messages[i_2]))
	      continue;
	    if (IS_OBJECT(M_OBJ(messages[i_2])) &&
		(rcvr_obj == NULL)) {
	      rcvr_obj = messages[i_2] -> obj;
	    } else if (rcvr_obj && !method) {
	      if (((method = get_instance_method
		    (messages[i_2],
		     rcvr_obj,
		     M_NAME(messages[i_2]),
		     ERROR, FALSE)) != NULL) ||
		  ((method = get_class_method
		    (messages[i_2], rcvr_obj, M_NAME(messages[i_2]),
		     ERROR, FALSE)) != NULL)) {
		continue;
	      }
	      /* Also check the receiver class in case the method
		 that we're in is a constructor. */
	      if (interpreter_pass == method_pass) 
		if (((method = get_instance_method
		      (messages[i_2],
		       rcvr_class_obj,
		       M_NAME(messages[i_2]),
		       ERROR, FALSE)) != NULL) ||
		    ((method = get_class_method
		      (messages[i_2], rcvr_class_obj, M_NAME(messages[i_2]),
		       ERROR, FALSE)) != NULL)) {
		  continue;
		}
	    } else if (M_TOK(messages[i_2]) == OPENPAREN) {
	      ++n_parens;
	    } else if (M_TOK(messages[i_2]) == CLOSEPAREN) {
	      --n_parens;
	    } else if (M_TOK(messages[i_2]) == ARGSEPARATOR) {
	      if (n_parens == 0) {
		++n_commas;
	      }
	    }
	  }
	  if (method) {
	    if (((n_commas + 1) != method -> n_params) && !method -> varargs) {
	      warning (messages[i], "Incorrect number of arguments to method "
		       "\"%s.\"", method -> name);
	    }
	  }
	} else {
	  /* Set by register_argblk_c_vars_1 () when we construct a 
	     cvartab name for an argument block. */
	  if (!(messages[i] -> attrs & TOK_IS_RT_EXPR))
	    undefined_label_check (messages, i);
	}
      }
    } else {
      if (!M_ISSPACE(messages[i])) {
	message_of_rcvr = FALSE;
	/*
	 *  This is necessary almost as a last attempt to warn the
	 *  user in the front end of a syntax error, when the front
	 *  end needs to evaluate an expression lazily, for example an
	 *  object mutation.  So this can certainly be expanded.
	 */
	switch (M_TOK(messages[i]))
	  {
	  case ARGSEPARATOR:
	    message_of_arg = TRUE;
	    break;
	  case EQ:
	    if (message_of_arg)
	      warning (messages[i], 
		       "Possible assignment in method argument.");
	    break;
	  case OPENPAREN:
	    /* We don't need class casts here at the moment, so
	       just elide it. */
	    if ((next = nextlangmsg (messages, i)) != ERROR) {
	      if (get_class_object (M_NAME(messages[next]))) {
		if ((next_2 = nextlangmsg (messages, next)) != ERROR) {
		  if (M_TOK(messages[next_2]) == MULT) {
		    if ((matching_paren = match_paren (messages, i, *end_idx))
			!= ERROR) {
		      int i_2;
		      for (i_2 = i; i_2 >= matching_paren; --i_2) {
			messages[i_2] -> name[0] = ' '; messages[i_2] -> name[1] = 0;
			messages[i_2] -> tokentype = WHITESPACE;
		      }
		      if (argblk)
			fmt_rt_argblk_expr (messages, start_idx, end_idx, expr_out);
		      else
			fmt_rt_expr (messages, start_idx, end_idx, expr_out);
		    }
		  }
		}
	      }
	    }
	    break;
	  }
      }
    }
  }
  for (i = start_idx; i >= *end_idx; i--) {
    if (!messages[i]) break;
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
  return expr_out;
}

/* 
 *  Like rt_expr (), above, but it adds all the variable registration to
 *  a single buffer with comma operators, instead of outputting immediately,
 *  and adds a __ctalkToCInteger () wrapper to the expression, before
 *  outputting.
 */
char *for_term_rt_expr (MESSAGE_STACK messages, int start_idx, int *end_idx,
			char *blk_expr_out) {

  char exprbuf[MAXMSG];
  char reg_buf[MAXMSG];
  char cvar_buf[MAXMSG];
  char *r;
  int i, struct_expr_limit, subscript_expr_limit;
  int message_of_rcvr, message_of_arg;
  bool have_cvar_reg = false;

  *blk_expr_out = 0;

  fmt_rt_expr (messages, start_idx, end_idx, exprbuf);
  message_of_rcvr = message_of_arg = FALSE;
  for (i = start_idx; i >= *end_idx; i--) {
    messages[i] -> attrs |= TOK_IS_RT_EXPR;
    if (M_TOK(messages[i]) == LABEL) {
      CVAR *__c;
      /* 
       *  See the comments in rt_self_expr, below.
       *  Should only need to check for receivers that
       *  are method parameters.
       */
      if (is_method_param_name (M_NAME(messages[i])))
	message_of_rcvr = TRUE;
      if (message_of_rcvr) {
	if (get_global_var_not_shadowed (M_NAME(messages[i]))
	    || get_local_var (M_NAME(messages[i]))) 
	  message_of_rcvr = FALSE;
	else
	  continue;
      }
      /*
       *   Not receiver, instance or class variable message.
       */
      if ((messages[i] -> obj == NULL) && 
	  (messages[i] -> receiver_obj == NULL)) {
	if (((__c = get_local_var (M_NAME(messages[i]))) != NULL) ||
	    ((__c = get_global_var_not_shadowed (M_NAME(messages[i]))) != NULL)) {
	  if (is_struct_or_union_expr (messages, i, 
				       stack_start(messages),
				       get_stack_top (messages))) {
	    strcatx (reg_buf, "(void)",
		     fmt_register_c_method_arg_call
		     (__c, 
		      struct_or_union_expr 
		      (messages, i, 
		       get_stack_top(messages),
		       &struct_expr_limit),
		      LOCAL_VAR, cvar_buf), NULL);
		     
	    if ((r = strchr (reg_buf, ';')) != NULL)
	      *r = ',';
	    strcatx2 (blk_expr_out, reg_buf, NULL);
	    have_cvar_reg = true;
	    i = struct_expr_limit;
	  } else {
	    if (is_subscript_expr (messages, i, 
				   get_stack_top (messages)) > 0) {
	      strcatx (reg_buf, "(void)", 
		       fmt_register_c_method_arg_call
		       (__c, 
			subscript_cvar_registration_expr 
			(messages, i, 
			 get_stack_top(messages),
			 &subscript_expr_limit),
			LOCAL_VAR, cvar_buf), NULL);
		       
	      if ((r = strchr (reg_buf, ';')) != NULL)
		*r = ',';
	      strcatx2 (blk_expr_out, reg_buf, NULL);
	      i = subscript_expr_limit;
	      have_cvar_reg = true;
	    } else {
	      strcatx (reg_buf, "(void)",
		       fmt_register_c_method_arg_call
		       (__c, M_NAME(messages[i]),
			LOCAL_VAR, cvar_buf), NULL);
		       
	      if ((r = strchr (reg_buf, ';')) != NULL)
		*r = ',';
	      strcatx2 (blk_expr_out, reg_buf, NULL);
	      have_cvar_reg = true;
	    }
	  }
	} else {
	  /* Set by register_argblk_c_vars_1 () when we construct a 
	     cvartab name for an argument block. */
	  if (!(messages[i] -> attrs & TOK_IS_RT_EXPR))
	    undefined_label_check (messages, i);
	}
      }
    } else {
      if (!M_ISSPACE(messages[i])) {
	message_of_rcvr = FALSE;
	/*
	 *  This is necessary almost as a last attempt to warn the
	 *  user in the front end of a syntax error, when the front
	 *  end needs to evaluate an expression lazily, for example an
	 *  object mutation.  So this can certainly be expanded.
	 */
	switch (M_TOK(messages[i]))
	  {
	  case ARGSEPARATOR:
	    message_of_arg = TRUE;
	    break;
	  case EQ:
	    if (message_of_arg)
	      warning (messages[i], 
		       "Possible assignment in method argument.");
	    break;
	  }
      }
    }
  }
  if (have_cvar_reg) {
    /* OBJTOC_DELETE_CVARS | OBJTOC_OBJECT_DELETE */
    strcatx2 (blk_expr_out, "__ctalkToCInteger (", exprbuf, ", 2)", NULL);
  } else {
    /* OBJTOC_OBJECT_DELETE */
    strcatx2 (blk_expr_out, "__ctalkToCInteger (", exprbuf, ", 0)", NULL);
  } 
  
  fileout (blk_expr_out, 0, start_idx);
  for (i = start_idx; i >= *end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
  return blk_expr_out;
}

static char *handle_ellipsis_param (char *arg_expr) {
  OBJECT *obj;
  char expr_out[MAXMSG];
  if ((obj = get_object (arg_expr, NULL)) != NULL) {
    return fmt_rt_return (arg_expr, obj-> __o_classname, TRUE, expr_out);
  } else {
    return arg_expr;
  }
  return NULL;
}

#ifdef __x86_64
bool longinteger_fold_is_ptr;
#endif

char *fn_param_return_trans (MESSAGE *m_orig, CFUNC *cfn, char *expr, int arg_idx) {
  int __n_th_arg;
  CVAR *__param;
  CVAR *__arg;
  static char expr_out[MAXMSG];

  if (IS_CVAR(cfn -> params)) {
    for (__n_th_arg = 0, __param = cfn -> params; 
	 (__n_th_arg < arg_idx) && __param; __n_th_arg++) 
      __param = __param -> next;
    if (__param) {
      if (__param ->  attrs & CVAR_ATTR_ELLIPSIS) {
	return handle_ellipsis_param (expr);
      } else {
	if (((__arg = get_local_var (expr)) != NULL) ||
	    ((__arg = get_global_var (expr)) != NULL)) {
	  /*
	   * If the arg is a CVAR with the same type as the parameter,
	   * no translation is necessary - just return the arg.
	   *
	   * In this case, match_c_type () should *normally* return TRUE,
	   * and then we let the compiler complain of any type mismatches.
	   * match_c_type () can still be used if we want to warn about
	   * type mismatches ourselves.
	   */
	  if (match_c_type (__param, __arg)) {
	    return expr;
	  } else {
#ifdef __x86_64
	    /* See the comment in objtoc.c */
	  if ((__param -> type_attrs & CVAR_TYPE_LONG) &&
	      (__param -> type_attrs & CVAR_TYPE_UNSIGNED) &&
	      !(__param -> type_attrs & CVAR_TYPE_LONGLONG)) {
	    longinteger_fold_is_ptr = true;
	  } else {
	    longinteger_fold_is_ptr = false;
	  }
#endif	  
	    return fmt_rt_return (expr,
				  basic_class_from_cvar
				  (m_orig, __param, 0),
				  TRUE, expr_out);
	  }
	} else {
#ifdef __x86_64
	    /* Comment in objtoc.c for this, too. */
	  if ((__param -> type_attrs & CVAR_TYPE_LONG) &&
	      (__param -> type_attrs & CVAR_TYPE_UNSIGNED) &&
	      !(__param -> type_attrs & CVAR_TYPE_LONGLONG)) {
	    longinteger_fold_is_ptr = true;
	  } else {
	    longinteger_fold_is_ptr = false;
	  }
#endif	  
	  return fmt_rt_return (expr,
				basic_class_from_cvar
				(m_orig, __param, 0),
				TRUE, expr_out);
	}
      }
    } else {
      /* Should be more args than provided by a stdarg prototype. */
      return handle_ellipsis_param (expr);
    }
  } else {
    warning 
      (m_orig, 
       "Function, \"%s,\" prototype has no parameter definitions.",
       cfn -> decl);
    warning 
      (m_orig,
       "Parameter class defaulting to, \"Integer.\"\n");
    return fmt_rt_return (expr, INTEGER_CLASSNAME, TRUE, expr_out);
  }
  return NULL;
}

/*
 *  This should normally be in c_argument_context, called after
 *  a call to obj_expr_is_fn_arg_ms, which should normally provide
 *  the stack index for fn_label_idx and the arg_idx argument.
 *  TODO - Also call from rt_self_expr, below.
 */
void rt_obj_arg (MESSAGE_STACK messages, 
		 int arg_start_idx, int *arg_end_idx, int fn_label_idx,
		 int arg_idx) {
  int i, stack_end, lookahead, n_parens, frame_start_idx,
    struct_expr_limit;
  MESSAGE *m;
  METHOD *method;
  int n_th_param, max_param;
  static char expr_buf[MAXMSG];
  char expr_buf_tmp[MAXMSG], *param_class = NULL;
  bool param_is_c_param = false;
  bool match_param = false;
  CFUNC *cfn;

  stack_end = get_stack_top (messages);
  for (i = arg_start_idx, n_parens = 0; i > stack_end; i--) {
    m = messages[i];
    lookahead = nextlangmsg (messages, i);
    switch (M_TOK(m))
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
      default:
	if ((M_TOK(messages[lookahead]) == CLOSEPAREN) ||
	    (M_TOK(messages[lookahead]) == ARGSEPARATOR)) {
	  if (!n_parens) {
	    *arg_end_idx = i;
	    goto got_arg_limit;
	  }
	}
	break;
      }
  }
 got_arg_limit:
  toks2str (messages, arg_start_idx, *arg_end_idx, expr_buf_tmp);
  de_newline_buf (expr_buf_tmp);

  // 
  // 
  // Optimize special cases as we can get to them.
  //
  if (interpreter_pass == method_pass) {

    if (arg_start_idx == *arg_end_idx) {

      // If the argument is a single token that is a method parameter,
      // use __ctalk_arg_internal (n), UNLESS the parameter has a
      // PARAM_C_PARAM attribute, when we just copy it to the
      // output.

      if ((method = new_methods[new_method_ptr+1] -> method) != NULL) {

	max_param = method -> n_params - 1;
	
	for (n_th_param = 0, match_param = FALSE; 
	     (n_th_param < method -> n_params) && !match_param; ++n_th_param) {
	  if (str_eq (expr_buf_tmp, method->params[n_th_param] -> name)) {
	    if (method->params[n_th_param] -> attrs & PARAM_C_PARAM) {
	      strcpy (expr_buf, M_NAME(messages[arg_start_idx]));
	      param_is_c_param = true;
	    } else {
	      format_method_arg_accessor (max_param  - n_th_param,
					  M_NAME(messages[arg_start_idx]),
					  method -> varargs, expr_buf);
	      param_class = method -> params[n_th_param] -> class;
	    }
	    match_param = TRUE;
	  }
	}
	if (!match_param) {
	  if (messages[arg_start_idx] -> attrs & TOK_SELF) {

	    // "self" alone is nearly the same.  Use 
	    // __ctalk_self_internal ().
	    strcatx (expr_buf, SELF_ACCESSOR_FN, " ()", NULL);

	  } else {
	    fmt_eval_expr_str (expr_buf_tmp, expr_buf);
	  }
	}
      } else {
	fmt_eval_expr_str (expr_buf_tmp, expr_buf);
      }
    } else {
      fmt_eval_expr_str (expr_buf_tmp, expr_buf);
    }
  } else {
    fmt_eval_expr_str (expr_buf_tmp, expr_buf);
  }

  /*
   *  NOTE - In a very complex expression, this might need to be the 
   *  frame start of the function or control block, if different 
   *  than the frame of the argument.
   */
  frame_start_idx = 
    frame_at (CURRENT_PARSER -> frame) -> message_frame_top;

  for (i = arg_start_idx; i >= *arg_end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      CVAR *__c;
      if (((__c = get_local_var (M_NAME(messages[i]))) != NULL) ||
 	  ((__c = get_global_var_not_shadowed (M_NAME(messages[i]))) != NULL)) {
	if (is_struct_or_union_expr (messages, i, 
				     stack_start(messages),
				     get_stack_top (messages))) {
	  generate_register_c_method_arg_call 
	    (__c, 
	     struct_or_union_expr (messages, i, 
				   get_stack_top(messages),
				   &struct_expr_limit),
	     LOCAL_VAR,
	     frame_start_idx);
	  i = struct_expr_limit;
	} else {
	  generate_register_c_method_arg_call (__c, M_NAME(messages[i]),
					       LOCAL_VAR,
					       frame_start_idx);
	}
      } else if (interpreter_pass == method_pass) {
	if (match_param & !param_is_c_param) {
	  if ((cfn = get_function (M_NAME(messages[fn_label_idx])))
	      != NULL) {
	    fileout (fn_param_return_trans
		     (messages[fn_label_idx], cfn, expr_buf, arg_idx),
		     FALSE, arg_start_idx);
	  } else {
	    warning  (messages[fn_label_idx], "Prototype of function, "
		      "\"%s\" not found.  Using the class of \"%s\" "
		      " (%s) to determine "
		      "the type of the argument.",
		      M_NAME(messages[fn_label_idx]),
		      M_NAME(messages[arg_start_idx]),
		      param_class);
	    fileout (fmt_rt_return
		     (expr_buf, param_class, TRUE,
		      expr_buf_tmp), FALSE, arg_start_idx);
	  }
	  for (i = arg_start_idx; i >= *arg_end_idx; i--) {
	    ++messages[i] -> evaled;
	    ++messages[i] -> output;
	  }
	  return;
	}
      }
    }
  }

  if (param_is_c_param) {
    fileout (expr_buf, 0, arg_start_idx);
  } else if ((cfn = get_function (M_NAME(messages[fn_label_idx]))) != NULL) {
    fileout (fn_param_return_trans
	     (messages[fn_label_idx], cfn, expr_buf, arg_idx),
	     0, arg_start_idx);
  } else {
    warning  (messages[fn_label_idx], "Prototype of function, "
	      "\"%s\" not found.  Using \"%s\" to determine the type "
	      "of the argument, \"%s\".",
	      M_NAME(messages[fn_label_idx]),
	      INTEGER_CLASSNAME,
	      M_NAME(messages[arg_start_idx]));
    fileout (fmt_rt_return
	     (expr_buf, INTEGER_CLASSNAME, TRUE,
	      expr_buf_tmp), FALSE, arg_start_idx);
  }

  for (i = arg_start_idx; i >= *arg_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
}

/*
 *  This should get more abbreviated as we narrow it to the
 *  case where a single method parameter token is the argument
 *  of a C function.  Called by resolve.
 */
void param_to_fn_arg (MESSAGE_STACK messages, int arg_start_idx) {
  METHOD *method;
  int n_th_param, match_param, max_param;
  char expr_buf[MAXMSG];
  bool param_is_c_param = false;


  // Single token arguments that are method parameters use 
  // __ctalk_arg_internal (n). Check also for a PARAM_C_PARAM
  // attribute, where the param can be used by itself, and
  // "self", where we can substitute __ctalk_self_internal.

  if ((method = new_methods[new_method_ptr+1] -> method) != NULL) {

    max_param = method -> n_params - 1;
	
    for (n_th_param = 0, match_param = FALSE; 
	 (n_th_param < method -> n_params) && !match_param; ++n_th_param) {
      if (str_eq (M_NAME(messages[arg_start_idx]), 
		  method->params[n_th_param] -> name)) {
	if (method->params[n_th_param] -> attrs & PARAM_C_PARAM) {
	  strcpy (expr_buf, M_NAME(messages[arg_start_idx]));
	  param_is_c_param = true;
	} else {
	  format_method_arg_accessor (max_param  - n_th_param,
				      M_NAME(messages[arg_start_idx]),
				      method -> varargs, expr_buf);
	}
	match_param = TRUE;
      }
    }
    if (!match_param) {
      if (messages[arg_start_idx] -> attrs & TOK_SELF) {
	
	// "self" alone is nearly the same.  Use 
	// __ctalk_self_internal ().
	strcatx (expr_buf, SELF_ACCESSOR_FN, " ()", NULL);

      } else {
	fmt_eval_expr_str (M_NAME(messages[arg_start_idx]), expr_buf);
      }
    }
  } else {
    fmt_eval_expr_str (M_NAME(messages[arg_start_idx]), expr_buf);
  }

  if (param_is_c_param) {
    fileout (expr_buf, 0, arg_start_idx);
  } else if (!rte_output_fn_arg (messages, arg_start_idx,
				 stack_start (messages), expr_buf)) {
    _warning ("Can't find self argument context in rt_self_arg.\n");
    fileout (expr_buf, 0, arg_start_idx);
  }

  ++messages[arg_start_idx] -> evaled;
  ++messages[arg_start_idx] -> output;

}

static inline int is_instancevar_of_param (MESSAGE_STACK messages,
					   int idx,
					   int stack_start_idx,
					   int stack_end_idx) {
  int i;
  OBJECT *param_class_object;
  for (i = idx + 1; i <= stack_start_idx; i++) {
    if (M_ISSPACE(messages[i]))
      continue;
    else
      break;
  }
  if (M_TOK(messages[i]) == LABEL) {
    if (is_method_parameter (messages, i)) {
      if ((param_class_object = new_method_parameter_class_object
	   (messages, i)) != NULL) {
	if ((get_instance_variable (M_NAME(messages[idx]),
				    param_class_object -> __o_name,
				    FALSE)) != NULL) {
	  get_new_method_param_instance_variable_series (param_class_object,
							 messages, i, 
							 stack_end_idx);
	  return TRUE;
	}
      }
    }
  }
  return FALSE;
}

static inline int is_aggregate_member (MESSAGE_STACK messages,
					int idx,
					int stack_start_idx) {
  int i;
  for (i = idx + 1; i <= stack_start_idx; i++) {
    if (M_ISSPACE(messages[i]))
      continue;
    else
      break;
  }
  if ((M_TOK(messages[i]) == PERIOD) || M_TOK(messages[i]) == DEREF)
    return TRUE;
  else
    return FALSE;
}

/* 
   Try to catch self expressions that don't start
   the expression; .e.g, self <var> <var> = self <var> <var>...
*/
static inline void self_instvar_series_internal (MESSAGE_STACK messages, 
						 int self_idx, 
						 int stack_top_idx) {
  int next_idx;
  if ((next_idx = nextlangmsg (messages, self_idx)) != ERROR) {
    if (M_TOK(messages[next_idx]) == LABEL) {
      get_self_instance_variable_series
	(messages[next_idx], self_idx, next_idx, stack_top_idx);
    }
  }
}

static inline int is_expr_method (MESSAGE_STACK messages, int idx,
				  char *exprbuf) {
  char exprbuf_save[MAXMSG];
  int r = FALSE, prev_idx, prev_idx_2;
  OBJECT *class_obj;

  /* Save the original expression in case we have to 
     compile a method. */
  strcpy (exprbuf_save, exprbuf);

  if ((prev_idx = prevlangmsg (messages, idx)) != ERROR) {
    if (IS_OBJECT(messages[prev_idx] -> obj)) {
      /* Find the object's value class from the value instance var. */
      if (TOK_IS_MEMBER_VAR(messages[prev_idx])) {
	if (IS_OBJECT(messages[prev_idx] -> obj -> instancevars)) {
	  if (IS_OBJECT(messages[prev_idx] -> obj -> instancevars ->
			__o_class)) {
	    class_obj = messages[prev_idx] -> obj -> instancevars ->
	      __o_class;
	  } else {
	    class_obj = get_class_object
	      (messages[prev_idx] -> obj -> instancevars ->
	       __o_classname);
	  }
	} else { /* value instance vars get handled here, but since
		    the compiler doesn't use the __o_p_obj member,
		    we don't check for it expressly (yet). */
	  if (IS_OBJECT(messages[prev_idx] -> obj -> __o_class)) {
	    class_obj = messages[prev_idx] -> obj -> __o_class;
	  } else {
	    class_obj = get_class_object
	      (messages[prev_idx] -> obj -> __o_classname);
	  }
	}
      } else {
	if (IS_OBJECT(messages[prev_idx] -> obj -> __o_class)) {
	  class_obj = messages[prev_idx] -> obj -> __o_class;
	} else {
	  class_obj = get_class_object
	    (messages[prev_idx] -> obj -> __o_classname);
	}
      }
      if (get_class_method (messages[idx], class_obj,
			    M_NAME(messages[idx]), ANY_ARGS, FALSE) ||
	  get_instance_method (messages[idx], class_obj,
			       M_NAME(messages[idx]), ANY_ARGS, FALSE) ||
	  is_method_proto (class_obj, M_NAME(messages[idx]))) {
	return TRUE;
      } else {
	if ((prev_idx_2 = prevlangmsg (messages, prev_idx)) != ERROR) {
	  if (messages[prev_idx_2] -> attrs & TOK_SELF) {
	    if (messages[prev_idx_2] -> receiver_msg &&
		(messages[prev_idx_2] -> receiver_msg -> attrs &
		 TOK_IS_CLASS_TYPECAST)) {
	      class_obj = messages[prev_idx_2] -> receiver_msg -> obj;
	      if (get_class_method (messages[idx], class_obj,
				    M_NAME(messages[idx]), ANY_ARGS,
				    FALSE) ||
		  get_instance_method (messages[idx], class_obj,
				       M_NAME(messages[idx]),
				       ANY_ARGS, FALSE) ||
		  is_method_proto (class_obj, M_NAME(messages[idx]))) {
		return TRUE;
	      }
	    }
	  }
	}
	return FALSE;
      }
    }
  }

  if (get_class_method (messages[idx], rcvr_class_obj,
			M_NAME(messages[idx]), ANY_ARGS, FALSE) ||
      get_instance_method (messages[idx], rcvr_class_obj,
			   M_NAME(messages[idx]), ANY_ARGS, FALSE) ||
      is_method_name (M_NAME(messages[idx])) ||
      is_method_proto (rcvr_class_obj, M_NAME(messages[idx]))) {
    r = TRUE;
  }
  
  strcpy (exprbuf, exprbuf_save);
  return r;
}

static int check_expr_c_vars (MESSAGE_STACK messages, int tok_idx,
			      int stack_start_idx,
			      int stack_top_idx,
			       int *expr_end_idx) {
  CVAR *__c;
  int struct_expr_limit,
    subscript_expr_limit;
  int prev_tok_idx;

  if (M_TOK(messages[tok_idx]) != LABEL) {
    *expr_end_idx = tok_idx;
    return FALSE;
  }

  if (messages[tok_idx] -> attrs & OBJ_IS_INSTANCE_VAR ||
      messages[tok_idx] -> attrs & VALUE_OBJ_IS_INSTANCE_VAR ||
      messages[tok_idx] -> attrs & OBJ_IS_CLASS_VAR ||
      messages[tok_idx] -> attrs & VALUE_OBJ_IS_CLASS_VAR) {
    *expr_end_idx = tok_idx;
    return FALSE;
  } else {
    if ((prev_tok_idx = prevlangmsg (messages, tok_idx)) != ERROR) {
      if (M_TOK(messages[prev_tok_idx]) == LABEL) {
	if (messages[prev_tok_idx] -> attrs & TOK_SELF ||
	    messages[prev_tok_idx] -> attrs & TOK_SUPER) {
	  /* Within an argument block, this may be all we can figure
	     out at the moment, but at least it helps prevent warning
	     messages. */
	  messages[tok_idx] -> attrs |= OBJ_IS_INSTANCE_VAR;
	  *expr_end_idx = tok_idx;
	  return FALSE;
	}
      }
    }
  }

  if ((__c = ifexpr_is_cvar_not_shadowed (messages, tok_idx)) != NULL) {
    if (is_struct_or_union_expr (messages, tok_idx, 
				 stack_start_idx,
				 stack_top_idx)) {
      generate_register_c_method_arg_call 
	(__c, 
	 struct_or_union_expr (messages, tok_idx, 
			       stack_top_idx,
			       &struct_expr_limit),
	 LOCAL_VAR,
	 FRAME_START_IDX);
      *expr_end_idx = struct_expr_limit;
      return TRUE;
    } else {
      if (is_subscript_expr (messages, tok_idx, stack_top_idx) > 0) {

	generate_register_c_method_arg_call 
	  (__c, 
	   subscript_cvar_registration_expr (messages, tok_idx, 
					     stack_top_idx,
					     &subscript_expr_limit),
	   LOCAL_VAR,
	   FRAME_START_IDX);
	*expr_end_idx = subscript_expr_limit;
	return TRUE;
      } else {
	if (argblk) {
	  CVAR *c_local;
	  /* TODO - See if we can avoid repeating the get_local_var ()
	     call above. */
	  if ((c_local = get_local_var (M_NAME(messages[tok_idx]))) != NULL) {
	    static char buf[MAXMSG];
	    /* This should be okay as long as we don't try to mung the
	       original token in the expression..... This fn registers
	       the cvartab name with the variable's original name. */
	    /* fmt_register_argblk_c_vars_3 (messages[tok_idx], c_local,
	       buf); */
	    fmt_register_argblk_c_vars_2 (messages[tok_idx], c_local,
					  buf);
	    /* TODO - This should actually be filed out at the start of
	       the expression ... but see if we can avoid rewriting this
	       fn. */
	    fileout (buf, 0, tok_idx);
	  } else {
	    generate_register_c_method_arg_call (__c, M_NAME(messages[tok_idx]),
						 LOCAL_VAR,
						 FRAME_START_IDX);
	  }
	} else {
	  generate_register_c_method_arg_call (__c, M_NAME(messages[tok_idx]),
					       LOCAL_VAR,
					       FRAME_START_IDX);
	}
	*expr_end_idx = tok_idx;
	return TRUE;
      }
    }
  }
  *expr_end_idx = tok_idx;
  return FALSE;
}

static bool c_operand (MESSAGE_STACK messages, int assign_op_idx,
		       int stack_top_idx, int *expr_end_out) {
  int i, prev = assign_op_idx;
  if ((i = nextlangmsg (messages, assign_op_idx)) == ERROR)
    return false;
  for (; i > stack_top_idx; i--) {
    if (M_ISSPACE(messages[i]))
      continue;
    if ((messages[i] -> attrs & TOK_SELF) ||
	(messages[i] -> attrs & TOK_SUPER) ||
	get_object (M_NAME(messages[i]), NULL)) {
      return false;
    } else if (M_TOK(messages[i]) == SEMICOLON) {
      *expr_end_out = prev;
      return true;
    }
    prev = i;
  }
  return false;
}

/* This could handle more checks as we need them... */
static bool rtse_instvar_msg (MESSAGE_STACK messages, int tok_idx,
			      int stack_start_idx) {
  int lookback;
  OBJECT *prev_tok_obj, *t;
  if ((lookback = prevlangmsg (messages, tok_idx)) != ERROR) {
    if (M_TOK(messages[lookback]) == LABEL) {
      if (interpreter_pass == method_pass) {
	if ((prev_tok_obj = get_local_object (M_NAME(messages[lookback]), NULL))
	    != NULL) {
	  messages[lookback] -> obj = prev_tok_obj;
	  for (t = prev_tok_obj -> instancevars; t; t = t -> next) {
	    if (str_eq (t -> __o_name, M_NAME(messages[tok_idx]))) {
	      messages[tok_idx] -> attrs |= OBJ_IS_INSTANCE_VAR;
	      messages[tok_idx] -> obj = t;
	      return true;
	    }
	  }
	} else if (messages[lookback] -> attrs & OBJ_IS_INSTANCE_VAR &&
		   IS_OBJECT(messages[lookback] -> obj)) {
	  for (t = messages[lookback] -> obj -> instancevars; t; t = t -> next) {
	    if (str_eq (t -> __o_name, M_NAME(messages[tok_idx]))) {
	      messages[tok_idx] -> attrs |= OBJ_IS_INSTANCE_VAR;
	      messages[tok_idx] -> obj = t;
	      return true;
	    }
	  }
	}
      }
    }
  }
  return false;
}

/* static char object_class_str[] = "Object";*//***/

/*
 *  Use this macro when rt_self_expr () needs to use the first token
 *  of the expression to determine its context.  The, "start_idx," 
 *  parameter normally points to a, "self," keyword, regardless of
 *  prefix operators, leading parentheses, etc.
 */
/*
 *  The rt_self_expr function needs to use fileout, not fileout,
 *  because a "self" token can appear many times within a frame, and
 *  each occurence needs its own output buffer (ie. via fileout). 
 */

#define _RTSE_1ST_TOK ((leading_tok_idx==-1) ? start_idx : leading_tok_idx)

char *rt_self_expr (MESSAGE_STACK messages, int start_idx, int *end_idx,
		    char *arg_expr_buf) {

  static char exprbuf[MAXMSG];
  char *p_cl, msgbuf[MAXMSG];
  char *errbuf;
  char *class_ptr;
  char *ret_ptr;
  int i;
  int fn_idx, arg_idx, next_idx;
  int lookback;
  int leading_tok_idx, pri_method_idx;
  int expr_limit;
  int messages_self_is_receiver_of;
  OBJECT *expr_rcvr_class = rcvr_class_obj;
  OBJECT_CONTEXT context;
  CVAR *c, *struct_decl;
  CVAR *struct_defn = NULL;   /* Avoid a warning. */
  CFUNC *fn;
  int stack_start_idx, stack_top_idx;
  METHOD *assign_method;
  bool have_cvar_registration = false;
  bool have_rcvr_expr_open_paren_adj = false;
  int rcvr_expr_open_paren_idx = start_idx, actual_expr_start = start_idx;
  char *c99name, *cfn_buf;
  OBJECT *arg_object;

  stack_start_idx = stack_start (messages);
  stack_top_idx = get_stack_top (messages);

  ret_ptr = arg_expr_buf;

  if ((leading_tok_idx = find_leading_tok_idx (messages, start_idx,
					       stack_start_idx,
					       stack_top_idx)) 
      != start_idx) {
    fmt_rt_expr (messages, leading_tok_idx, end_idx, exprbuf);
    /*
     *  Do not free ().
     */
  } else {

    /*
     *  If the expression is, "self," alone, use __ctalk_self_internal ().
     */
    if (((next_idx = nextlangmsg (messages, start_idx)) != ERROR) &&
     	METHOD_ARG_TERM_MSG_TYPE(messages[next_idx])) {

      strcatx (exprbuf, SELF_ACCESSOR_FN, " ()", NULL);
      *end_idx = start_idx;
      leading_tok_idx = -1;
      goto self_alone_trans;
    } else if (IS_C_ASSIGNMENT_OP(M_TOK(messages[next_idx])) &&
	       str_eq (rcvr_class_obj -> __o_name, INTEGER_CLASSNAME) &&
	       c_operand (messages, next_idx, stack_top_idx,
			  end_idx)) {
      /* If the receiver is an Integer, and the argument is entirely a 
	 C expression, then we can use a simple method call. We set the
	 param index to 513 to tell __ctalk_arg that the value is to be
	 taken as a binary whole number (it's declared as a void *,
	 so the width can determined by the receiver in __ctalk_arg). */
      assign_method = get_instance_method (messages[start_idx],
					rcvr_class_obj,
					M_NAME(messages[next_idx]),
					1, true);
      arg_idx = nextlangmsg (messages, next_idx);
      p_cl = collect_tokens (messages, arg_idx, *end_idx);
      /* the (unsigned long) cast prevents compiler warnings when
	 we translate a binary int into a (void *) */
      strcatx (arg_expr_buf, "__ctalk_arg (\"",
	       M_NAME(messages[start_idx]), "\", \"",
	       M_NAME(messages[next_idx]), "\", ",
	       ascii[513], ", (void *)((unsigned long)", p_cl, "));\n", NULL);
      __xfree (MEMADDR(p_cl));
      fileout (arg_expr_buf, false, start_idx);
      strcatx (exprbuf, "__ctalk_method (\"self\", ", 
	       assign_method -> selector, 
	       ", \"", assign_method -> name, "\");\n", NULL);
      fileout (exprbuf, false, start_idx);
      fileout ("__ctalk_arg_cleanup((void *)0);\n", false, start_idx);
      for (i = start_idx; i >= *end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
      if ((next_idx = nextlangmsg (messages, *end_idx)) != ERROR) {
	/* there's a semicolon at the end of expression in the
	   input. eat it. */
	++messages[next_idx] -> evaled;
	++messages[next_idx] -> output;
      }
      return arg_expr_buf;
    } else {
      int lookahead, lookahead2, l_lookback;
      fmt_rt_expr (messages, start_idx, end_idx, exprbuf);
      if ((lookahead = nextlangmsg (messages, *end_idx)) != ERROR) {
	if (M_TOK(messages[lookahead]) == CLOSEPAREN) {
	  /* 
	   *  Check for an expression like this: 
	   *
	   *    (<self_expr>) <method_expr>
	   *               ^^^
	   *
	   *  and backtrack.  TODO - multiple enclosing parens. 
	   */
	  if ((lookahead2 = nextlangmsg (messages, lookahead)) != ERROR) {
	    if (M_TOK(messages[lookahead2]) == LABEL) {
	      if (leading_tok_idx != ERROR) {
		if ((l_lookback = prevlangmsg (messages, leading_tok_idx))
		    != ERROR) {
		  if (M_TOK(messages[l_lookback]) == OPENPAREN) {
		    fmt_rt_expr (messages, l_lookback, end_idx, exprbuf);
		    have_rcvr_expr_open_paren_adj = true;
		    rcvr_expr_open_paren_idx = l_lookback;
		  }
		}
	      }
	    }
	  }
	}
      }
      leading_tok_idx = -1;
    }
  }

  if (!IS_OBJECT (rcvr_class_obj)) /* Can't do a syntax check without it. */
    return exprbuf;

  next_idx = nextlangmsg (messages, start_idx);
  /*
   *  Syntax check kludge for sequence of labels beginning with
   *  "self," which are not, based only on their own syntax, 
   *  distinguishable from a variable declaration, which means
   *  that the parser could register a message as a C variable
   *  during the var_pass, or in parser_pass a few messages later.  
   *  So check here again for the "self" expression.
   *  TO DO - To get this more accurately, it will be necessary to 
   *  evaluate the expression here, and determine the class of self.  
   *  It probably isn't  practical to evaluate the expression while 
   *  registering C variables.  Even though there's another check 
   *  for messages that are declared C variables, this is faster 
   *  anyway.
   */
  messages_self_is_receiver_of = FALSE;
  if (have_rcvr_expr_open_paren_adj) {
    actual_expr_start = rcvr_expr_open_paren_idx;
  } else {
    actual_expr_start =
      ((leading_tok_idx != -1) ? leading_tok_idx : start_idx);
  }
  for (i = actual_expr_start; i >= *end_idx; i--) {
    messages[i] -> attrs |= TOK_IS_RT_EXPR;
    if (M_ISSPACE(messages[i]))
      continue;
    if (messages[i] -> attrs & TOK_SELF) {
      messages_self_is_receiver_of = TRUE;
      if (messages[i] -> receiver_msg &&
	  messages[i] -> receiver_msg -> attrs & TOK_IS_CLASS_TYPECAST) {
	expr_rcvr_class = messages[i] -> receiver_msg -> obj;
      }
      if (i < start_idx)
	self_instvar_series_internal (messages, i, stack_top_idx);
	
      continue;
    }
    if (M_TOK(messages[i]) == LABEL) {
      if (messages_self_is_receiver_of) {
	if (global_var_is_declared (M_NAME(messages[i]))
	    /* This is to retrieve a local C var. */
	    || get_local_var (M_NAME(messages[i]))) {
	  if (!(messages[i] -> attrs & OBJ_IS_INSTANCE_VAR &&
		messages[i] -> attrs & VALUE_OBJ_IS_INSTANCE_VAR &&
		messages[i] -> attrs & OBJ_IS_CLASS_VAR &&
		messages[i] -> attrs & VALUE_OBJ_IS_CLASS_VAR)) {
	    messages_self_is_receiver_of = FALSE;
	  }
	} else {
	  if (!TOK_IS_MEMBER_VAR(messages[i]) &&
	      is_expr_method (messages, i, exprbuf)) {
	    messages_self_is_receiver_of = FALSE;
	  } else {

	    if (!get_instance_variable (M_NAME(messages[i]), 
					(argblk ? NULL : 
					 expr_rcvr_class -> __o_name),
					FALSE) &&

		!TOK_IS_MEMBER_VAR(messages[i]) &&
		!get_class_variable (M_NAME(messages[i]), 
				     (argblk ? NULL : 
				      expr_rcvr_class -> __o_name),
				     FALSE) &&
		!get_local_object (M_NAME(messages[i]), NULL) &&
		!rtse_instvar_msg (messages, i, stack_start_idx) &&
		/* Believe it or not... */
		!is_method_name (M_NAME(messages[i])) &&
		!is_method_parameter (messages, i) &&
		!(messages[i] -> attrs & TOK_SUPER) &&
		strcmp (M_NAME(messages[i]), "eval")) {
	      toks2str (messages, start_idx, *end_idx, msgbuf);
	      unresolved_eval_delay_warning_1 
		(messages[i],
		 messages, i,
		 (interpreter_pass == method_pass ? 
		  new_methods[new_method_ptr+1] -> method -> name : NULL),
		 expr_rcvr_class -> __o_name,
		 M_NAME(messages[i]),
		 msgbuf);
	      messages_self_is_receiver_of = FALSE;
	      goto check_vars;
	    }
	  }
	  continue;
	}
      }
    check_vars:
      
      if (check_expr_c_vars (messages, i, stack_start_idx, stack_top_idx,
			     &expr_limit)) {
	i = expr_limit;
	have_cvar_registration = true;
      } else if ((fn = get_function (M_NAME(messages[i]))) != NULL) {
	if (format_self_lval_fn_expr (messages, start_idx) != ERROR) {
	  goto rt_expr_evaled_2;
	} else {
	  if ((c99name = template_name (M_NAME(messages[i]))) == NULL) {
	    /* If there's a template for the function, paste that
	       in later. */
	    int fn_expr_end_idx;
	    switch (context = object_context (messages, i))
	      {
	      case argument_context:
		method_arg_is_fn_call (messages, start_idx, i, stack_top_idx,
				       pri_method_idx, &fn_expr_end_idx);
		have_cvar_registration = true;
		fmt_rt_expr (messages, start_idx, end_idx, exprbuf);
		i = fn_expr_end_idx;
		continue;
		break;
	      default:
		break;
	      }
	  } else { /* if ((c99name ... */
	    int fn_arg_start_idx, fn_arg_end_idx;
	    char expr_buf_1[MAXMSG], expr_buf_tmp[MAXMSG],
	      /***/
	      tmpl_cvar_register_buf[MAXMSG] /*, *fn_return_class*/;
	    cfn_buf = clib_fn_rt_expr (messages, i);
	    fn_arg_start_idx = nextlangmsg (messages, i);
	    fn_arg_end_idx = match_paren (messages, fn_arg_start_idx,
					  stack_top_idx);
	    toks2str (messages, fn_arg_start_idx, fn_arg_end_idx,
		      expr_buf_tmp);
	    memset (tmpl_cvar_register_buf, 0, MAXMSG);
	    register_template_arg_CVARs (messages, fn_arg_start_idx,
					 fn_arg_end_idx,
					 tmpl_cvar_register_buf);
	    strcatx (expr_buf_1, cfn_buf, " ", expr_buf_tmp, NULL);
	    /***/
	    /* fn_return_class = object_class_str; */
	    arg_object = create_object (CFUNCTION_CLASSNAME,
					fmt_eval_expr_str
					(expr_buf_1, expr_buf_tmp));
	    delete_object (arg_object);
	    /* TO DO - check for trailing characters */
	  }
	}
      } else {
	/*
	 *  Report undefined labels where "self" *isn't*
	 *  the receiver.
	 */
	if (!is_expr_method (messages, i, exprbuf) &&
	    !is_method_parameter (messages,i) && 
	    !is_aggregate_member (messages,i, stack_start_idx) && 
	    !get_object (M_NAME(messages[i]), NULL) &&
	    !(messages[i] -> attrs & OBJ_IS_INSTANCE_VAR) &&
	    !is_instancevar_of_param (messages, i, stack_start_idx,
				      stack_top_idx) &&
	    !is_method_name (M_NAME(messages[i])) &&
	    !class_object_search (M_NAME(messages[i]), FALSE) &&
	    !is_c_data_type (M_NAME(messages[i])) &&
	    !(messages[i] -> attrs & TOK_SUPER) &&
	    !is_instance_var (M_NAME(messages[i]))) {
	  warning (messages[i], "Undefined label, \"%s.\"",
		   M_NAME(messages[i]));
	}
      }
    } else if (M_TOK(messages[i]) == METHODMSGLABEL) { /***/
      pri_method_idx = i;
      if (messages[actual_expr_start] -> attrs & TOK_SELF) {
	messages[i] -> receiver_msg = messages[actual_expr_start];
      } else if (messages[start_idx] -> attrs & TOK_SELF) {
	messages[i] -> receiver_msg = messages[start_idx];
      }
    } else {
      if (!M_ISSPACE(messages[i])) {
	messages_self_is_receiver_of = FALSE;
      }
    }
  }
 self_alone_trans:
  /* TODO - Figure out if we can get rte_output_fn_arg to work
     with this clause. */
  if ((arg_idx = 
       obj_expr_is_arg (messages, _RTSE_1ST_TOK, 
			stack_start_idx,
			&fn_idx)) != ERROR) {
    CFUNC *cfn;
    if ((cfn = get_function (M_NAME(messages[fn_idx]))) != NULL) {
      strcpy (arg_expr_buf, 
	      fn_param_return_trans (messages[fn_idx], 
				     cfn, exprbuf, arg_idx));
    } else {
      if (messages[*end_idx] -> attrs & TOK_SELF) {
	/* "self" is the only token in the argument.  Use the
	   class of rcvr_class_obj's value variable, or the
	   class of rcvr_class_obj if there isn't a value
	   var. */
	if (IS_OBJECT(rcvr_class_obj -> instancevars)) {
	  fmt_rt_return (exprbuf,
			 messages[*end_idx] -> obj ->
			 instancevars -> __o_classname,
			 TRUE, arg_expr_buf);
	} else {
	  fmt_rt_return (exprbuf,
			 messages[*end_idx] -> obj -> __o_classname,
			 TRUE, arg_expr_buf);
	}
      } else if (IS_OBJECT (messages[*end_idx] -> obj)) {
	/* An instance variable expression. Use the class of the 
	 instance var, or the instance var's value var if it exists. */
	if (IS_OBJECT(messages[*end_idx] -> obj -> instancevars)) {
	  fmt_rt_return (exprbuf,
			 messages[*end_idx] -> obj -> instancevars ->
			 __o_classname,
			 TRUE, arg_expr_buf);
	} else {
	  fmt_rt_return (exprbuf,
			 messages[*end_idx] -> obj -> __o_classname,
			 TRUE, arg_expr_buf);
	}
      } else {
	/* Expression that contain methods, for now, get only a
	   warning. */
	if (leading_tok_idx == -1) {
	  errbuf = collect_tokens (messages, start_idx, *end_idx);
	} else {
	  errbuf = collect_tokens (messages, leading_tok_idx, *end_idx);
	}
	warning  (messages[fn_idx], "Prototype of function, "
		  "\"%s\" not found.  Using, \"int\" as "
		  "the type of the argument, \"%s\".",
		  M_NAME(messages[fn_idx]), errbuf);
	fmt_rt_return (exprbuf, INTEGER_CLASSNAME, TRUE, arg_expr_buf);
	__xfree (MEMADDR(errbuf));
      }
    }
  } else {

    context = object_context (messages, _RTSE_1ST_TOK);
    switch (context)
      {
      case c_context:
      case c_argument_context:

	/* Limited right now - we should be able to do
	   expressions with methods the same way. */

	actual_expr_start = ((leading_tok_idx != -1) ?
			     leading_tok_idx : start_idx);
	if (have_rcvr_expr_open_paren_adj) {
	  actual_expr_start = rcvr_expr_open_paren_idx;
	}
	if (messages[actual_expr_start] -> attrs & TOK_IS_PRINTF_ARG) {

	  /* Simplify this, or fix is_fmt_arg () to handle more cases. */
	  if (is_fmt_arg (messages, _RTSE_1ST_TOK, stack_start_idx,
	   		  stack_top_idx)) {
	    fmt_printf_fmt_arg (messages, _RTSE_1ST_TOK,
				stack_start_idx, exprbuf, arg_expr_buf);
	  } else {
	    if ((class_ptr = complex_expr_class (messages, start_idx))
		== NULL) {
	      undefined_self_fmt_arg_class (messages, _RTSE_1ST_TOK,
					 start_idx);
	      fmt_rt_return (exprbuf, 
			     DEFAULT_LIBC_RETURNCLASS, 
			     TRUE, arg_expr_buf);
	    } else {
	      fmt_rt_return (exprbuf, complex_expr_class (messages, start_idx),
			     TRUE, arg_expr_buf);
	    }
	  }
	  goto rt_expr_evaled;
	}

	if ((lookback = prevlangmsg (messages, actual_expr_start))
	    != ERROR) {
	  if (M_TOK(messages[lookback]) == ARRAYOPEN) {
	    fmt_rt_return (exprbuf, INTEGER_CLASSNAME,
			   TRUE, arg_expr_buf);
	    if (M_TOK(messages[*end_idx]) == LABEL) {
	      if (IS_OBJECT(messages[*end_idx] -> obj)) {
		OBJECT *terminal_value_obj;
		char expr_toks[MAXMSG];
		if ((terminal_value_obj =
		     messages[*end_idx] ->  obj -> instancevars)
		    != NULL) {
		  if (!str_eq (terminal_value_obj -> __o_class -> __o_name,
			       "Integer") &&
		      !str_eq (terminal_value_obj -> __o_class -> __o_name,
			       "LongInteger")) {
		    toks2str (messages, start_idx, *end_idx, expr_toks);
		    warning (messages[start_idx],
			     "Subscript object's class is not an Integer or "
			     "LongInteger.\n\n\t%s\n\n",
			     expr_toks);
		  }
		}
	      }
	    }
	    goto rt_expr_evaled;
	  }
	}

	if ((lookback = lval_idx_from_arg_start (messages, start_idx))
	    == ERROR) {
	  fileout (exprbuf, 0, start_idx);
	  ret_ptr = exprbuf;
	  goto rt_expr_evaled_2;
	}

	if (((c = get_local_var (M_NAME(messages[lookback]))) != NULL) ||
	    ((c = get_global_var (M_NAME(messages[lookback]))) != NULL)) {
	  fmt_rt_return (exprbuf, 
			 basic_class_from_cvar
			 (messages[start_idx], c, 0),
			 TRUE, arg_expr_buf);
	} else {
	  if (argblk && ((c = get_var_from_cvartab_name
			  (M_NAME(messages[lookback]))) != NULL)) {
	    fmt_rt_return (exprbuf, 
			   basic_class_from_cvar
			   (messages[start_idx], c, 0),
			   TRUE, arg_expr_buf);
	  } else {
	    if (is_struct_member_tok (messages, lookback)) {
	      int lookback2 = lookback;  /* Avoid a warning. */
	      int last_expr_tok;
	      last_expr_tok = lookback;
	      while ((lookback = prevlangmsg (messages, lookback)) != ERROR) {
		if (((lookback2 = prevlangmsg (messages, lookback)) != ERROR) 
		    && ((M_TOK(messages[lookback2]) != PERIOD) ||
			(M_TOK(messages[lookback2]) != DEREF)))
		  break;
	      }
	      if (((struct_decl = 
		    get_local_var (M_NAME(messages[lookback2]))) == NULL) &&
		  ((struct_decl =
		    get_global_var (M_NAME(messages[lookback2]))) == NULL)) {
		if (M_TOK(messages[lookback2]) == LABEL) {
		  warning (messages[start_idx], 
			   "Definition of struct or union, \"%s,\" not found.",
			   M_NAME(messages[lookback2]));
		}
		strcpy (arg_expr_buf, exprbuf);
	      } else {
		if (struct_decl -> attrs == CVAR_ATTR_STRUCT) {
		  if (((struct_defn = get_global_var (struct_decl->type))
		       == NULL) &&
		      ((struct_defn = get_local_var (struct_decl->type))
		       == NULL)) {
		    warning (messages[start_idx], 
			     "Definition of struct or union, \"%s,\" not found.",
			     struct_decl -> type);
		    struct_defn = struct_decl;
		  }
		} else if (struct_decl -> type_attrs == CVAR_TYPE_TYPEDEF) {
		  /***/
		  if ((struct_defn = get_typedef (struct_decl -> type))
		      == NULL) {
		    warning (messages[start_idx], 
			     "Definition of typedef, \"%s,\" not found.",
			     struct_decl -> type);
		    struct_defn = struct_decl;
		  }
		}
		c = struct_member_from_expr_b (messages,
					       lookback2, 
					       last_expr_tok, 
					       struct_defn);
		fmt_rt_return (exprbuf, 
			       basic_class_from_cvar
			       (messages[start_idx], c, 0),
			       TRUE, arg_expr_buf);
	      }
	    } else {
	      /*
	       * Check again for cases not covered above.
	       */
	      if (is_fmt_arg (messages, 
			      _RTSE_1ST_TOK,
			      stack_start_idx, 
			      stack_top_idx)) {
		fmt_printf_fmt_arg (messages, _RTSE_1ST_TOK, 
				    stack_start_idx, exprbuf, arg_expr_buf);
	      } else {
		fileout (exprbuf, 0, start_idx);
		ret_ptr = exprbuf;
		goto rt_expr_evaled_2;
	      }
	    }
	  }
	}
	break;
      case argument_context:
	if ((p_cl = use_new_c_rval_semantics_b (messages, start_idx)) != NULL) {
	  fmt_rt_return (exprbuf, p_cl, TRUE, arg_expr_buf);
	} else {
	  fileout (exprbuf, 0, start_idx);
	  if (have_cvar_registration) {
	    output_delete_cvars_call (messages, start_idx, stack_top_idx);
	  }
	  ret_ptr = exprbuf;
	  goto rt_expr_evaled_2;
	}
	break;
      default:
	fileout (exprbuf, 0, start_idx);
	ret_ptr = exprbuf;
	if (have_cvar_registration) {
	  output_delete_cvars_call (messages, start_idx, stack_top_idx);
	}
	goto rt_expr_evaled_2;
	break;
      }      
  }
 rt_expr_evaled:
  if (have_rcvr_expr_open_paren_adj) {
    actual_expr_start = rcvr_expr_open_paren_idx;
  } else {
    actual_expr_start = start_idx;
  }
  fileout (arg_expr_buf, 0, actual_expr_start);
 rt_expr_evaled_2:
  if (have_rcvr_expr_open_paren_adj) {
    for (i = actual_expr_start; i >= *end_idx; i--) {
      ++messages[i] -> evaled;
      ++messages[i] -> output;
    }
  } else if (leading_tok_idx != -1) {
    for (i = leading_tok_idx; i >= *end_idx; i--) {
      ++messages[i] -> evaled;
      ++messages[i] -> output;
    }
  } else {
    for (i = start_idx; i >= *end_idx; i--) {
      ++messages[i] -> evaled;
      ++messages[i] -> output;
    }
  }
  return ret_ptr;
}

static void comma_op_buf (char *s) {
  char *p;
  while ((p = index (s, ';')) != NULL) *p = ',';
  while ((p = index (s, '\n')) != NULL) *p = ' ';
}

static bool last_cvar_term (MESSAGE_STACK messages, int last_term_idx) {
  int i;
  for (i = last_term_idx - 1; i >= C_CTRL_BLK -> pred_end_ptr; --i) {
    if (M_TOK(messages[i]) == LABEL) {
      if (get_local_var (M_NAME(messages[i])) ||
	  get_global_var_not_shadowed (M_NAME(messages[i]))) {
	return false;
      }
    }
  }
  return true;
}

/* The macro is for expressions that don't need CVAR registration calls. */
#define DCE_KEEPSTR(i) ((i) ? "1" : "0")
static char *dce_keepstr (MESSAGE_STACK messages, int keep,
			  int expr_end_idx) {
  int lookahead;
  if ((lookahead = nextlangmsg (messages, expr_end_idx)) != ERROR) {
    if (lookahead == C_CTRL_BLK -> pred_end_ptr) {
      if (keep) {
	/* OBJTOC_OBJECT_KEEP | OBJTOC_DELETE_CVARS */
	return "3";
      } else {
	/* OBJTOC_OBJECT_DELETE | OBJTOC_DELETE_CVARS */
	return "2";
      }
    } else if (last_cvar_term (messages, expr_end_idx)) {
      if (keep) {
	/* OBJTOC_OBJECT_KEEP | OBJTOC_DELETE_CVARS */
	return "3";
      } else {
	/* OBJTOC_OBJECT_DELETE | OBJTOC_DELETE_CVARS */
	return "2";
      }
    } else {
      if (keep) {
	/* OBJTOC_OBJECT_KEEP */
	return "1";
      } else {
	/* OBJTOC_OBJECT_DELETE */
	return "0";
      }
    }
  } else {
    if (keep) {
      /* OBJTOC_OBJECT_KEEP */
      return "1";
    } else {
      /* OBJTOC_OBJECT_DELETE */
      return "0";
    }
  }
  /* default is OBJTOC_OBJECT_KEEP, and let the run time clean up
     a transient object if necessary */
  return "1";
}

/*
 *  Format a run-time expression, for example, a for loop termination
 *  predicate, with CVAR registration function calls if necessary.
 */
char *fmt_default_ctrlblk_expr (MESSAGE_STACK messages, int start_idx, 
				int end_idx, int keep,
				char *expr_buf_out) {
  char exprbuf[MAXMSG], *c99name,
    fn_call_buf[MAXMSG],
    *cfn_buf, expr_buf_tmp[MAXMSG],
    cvar_buf[MAXMSG];
  int i;
  int expr_end_out;
  MESSAGE *m;
  CVAR *cvar = NULL;
  CFUNC *fn;
  bool have_pattern = false;

  for (i = start_idx, *expr_buf_out = 0; i >= end_idx; i--) {

    m = messages[i];

    if (m && M_ISSPACE(m)) continue;

    if (M_TOK(m) == LABEL) {
      if (!(m -> attrs & TOK_CVAR_REGISTRY_IS_OUTPUT)) {
	if (((cvar = get_local_var (M_NAME(m))) != NULL) ||
	    ((cvar = get_global_var_not_shadowed (M_NAME(m))) != NULL)) {
	  if ((cvar -> attrs & CVAR_ATTR_FN_DECL) ||
	      (cvar -> attrs & CVAR_ATTR_FN_PTR_DECL) ||
	      (cvar -> attrs & CVAR_ATTR_FN_PROTOTYPE)) {
	    /*
	     *  First check if we have a template for the function.
	     *  If so, insert the template in the output, and if 
	     *  necessary, register the template in the initialization
	     *  (though now the function registration by clib_fn_expr ()
	     *  is handled earlier in ctrlblk_pred_rt_expr ()).
	     *  If there's no template, issue a warning.
	     *
	     *  The function should be able to simply do a token 
	     *  replacement here, but later, it might be necessary
	     *  to set the value object, if we need to rescan the
	     *  message.
	     */
	    if ((c99name = c99_name (M_NAME(m), FALSE)) != NULL) {
	      if ((cfn_buf = clib_fn_rt_expr (messages, i)) != NULL) {
		__xfree (MEMADDR(m -> name));
		m -> name = strdup (cfn_buf);
	      }
	    }
	  } else {
	    strcatx (fn_call_buf,
		     register_c_var_buf
		     (m, messages, i, &expr_end_out,
		      cvar_buf), " ", NULL);
	    comma_op_buf (fn_call_buf);
	    strcatx2 (expr_buf_out, fn_call_buf, NULL);
	    i = expr_end_out;
	    continue;
	  }
	} else {
	  if ((fn = get_function (M_NAME(m))) != NULL) {
	    if ((c99name = c99_name (M_NAME(m), FALSE)) != NULL) {
	      if ((cfn_buf = clib_fn_rt_expr (messages, i)) != NULL) {
		__xfree (MEMADDR(m -> name));
		m -> name = strdup (cfn_buf);
	      }
	    }
	  }
	}
      } /* if (!(m -> attrs & TOK_CVAR_REGISTRY_IS_OUTPUT)) */
    } else if (M_TOK(messages[i]) == PATTERN) {
      have_pattern = true;
    }
  }

  toks2str (messages, start_idx, end_idx, exprbuf);
  de_newline_buf (exprbuf);
  if (have_pattern) {
    if (cvar) {
      strcatx2 (expr_buf_out, INT_TRANS_FN, " (",
		fmt_pattern_eval_expr_str (exprbuf, expr_buf_tmp), 
		", ", dce_keepstr(messages, keep, end_idx), ")",
		NULL);
    } else {
      strcatx2 (expr_buf_out, INT_TRANS_FN, " (",
		fmt_pattern_eval_expr_str (exprbuf, expr_buf_tmp), 
		", ", DCE_KEEPSTR(keep), ")",
		NULL);
    }
  } else {
    if (cvar) {
      strcatx2 (expr_buf_out, INT_TRANS_FN, " (",
		fmt_eval_expr_str (exprbuf, expr_buf_tmp), 
		", ", dce_keepstr(messages, keep, end_idx), ")",
		NULL);
    } else if (resolve_ctrlblk_cvar_reg) {
      strcatx2 (expr_buf_out, INT_TRANS_FN, " (",
		fmt_eval_expr_str (exprbuf, expr_buf_tmp), 
		", ", dce_keepstr(messages, keep, end_idx), ")",
		NULL);
      resolve_ctrlblk_cvar_reg = false;
    } else if (cpre_have_cvar_reg) {
      strcatx2 (expr_buf_out, INT_TRANS_FN, " (",
		fmt_eval_expr_str (exprbuf, expr_buf_tmp), 
		", ", dce_keepstr(messages, keep, end_idx), ")",
		NULL);
      cpre_have_cvar_reg = false;
    } else {
      strcatx2 (expr_buf_out, INT_TRANS_FN, " (",
		fmt_eval_expr_str (exprbuf, expr_buf_tmp), 
		", ", DCE_KEEPSTR(keep), ")",
		NULL);
    }
  }

  return expr_buf_out;
}

/*
 *  This is much simpler than fmt_default_ctrlblk_expr, because user functions
 *  get a template for each occurrence (in fn_tmpl.c), so we don't 
 *  have to worry about registering arguments.  
 *
 *  NOTE - removes the newlines from the expression also.
 */
char *fmt_user_fn_rt_expr (MESSAGE_STACK messages, 
			   int start_idx, int end_idx, char *expr,
			   char *fn_call_buf) {
  char expr_buf_tmp[MAXMSG];
  de_newline_buf (expr);
  strcatx (fn_call_buf, "__ctalkToCInteger (",
	   fmt_eval_expr_str (expr, expr_buf_tmp), ", 0)", NULL);
  return fn_call_buf;
}

/* 
 *  As above, except we set the expression depending on whether we need 
 *  to unregister some CVARs from the expression; eventually, this should
 *  probably replace the function above.
 *  
 */
char *fmt_user_fn_rt_expr_b (MESSAGE_STACK messages, 
			     int start_idx, int end_idx, char *expr,
			     char *fn_call_buf,
			     bool cvar_unreg) {
  char expr_buf_tmp[MAXMSG];
  de_newline_buf (expr);
  if (cvar_unreg)
    /* OBJTOC_OBJECT_DELETE|OBJTOC_DELETE_CVARS */
    strcatx (fn_call_buf, "__ctalkToCInteger (",
	     fmt_eval_expr_str (expr, expr_buf_tmp), ", 2)", NULL);
  else
    /* OBJTOC_OBJECT_DELETE */
    strcatx (fn_call_buf, "__ctalkToCInteger (",
	     fmt_eval_expr_str (expr, expr_buf_tmp), ", 0)", NULL);
    
  return fn_call_buf;
}

/* This should work better when output as a unit. */
static void output_eval_bufs_a (char *register_buf, char *buf, 
			 int keyword_ptr) {
  if (*register_buf) {
    strcatx2 (register_buf, buf, NULL);
    fileout (register_buf, 0, keyword_ptr);
  } else {
    fileout (buf, 0, keyword_ptr);
  }
}

static void eval_c_argument_handler (MESSAGE_STACK messages, int keyword_ptr,
				      char *buf) {

  int prev_tok_idx;
  
  if ((prev_tok_idx = prevlangmsg (messages, keyword_ptr)) != ERROR) {
    if (IS_C_BINARY_MATH_OP(M_TOK(messages[prev_tok_idx]))) {
      int prev_tok_idx2;
      CVAR *l_cvar;
      char rbuf[MAXMSG];
      if ((prev_tok_idx2 = prevlangmsg (messages, prev_tok_idx))
	  != ERROR) {
	if (argblk) {
	  if (((l_cvar = get_local_var (M_NAME(messages[prev_tok_idx2]))) 
	       != NULL) ||
	      ((l_cvar = get_global_var (M_NAME(messages[prev_tok_idx2])))
	       != NULL) ||
	      ((l_cvar = get_var_from_cvartab_name 
		(M_NAME(messages[prev_tok_idx2]))) != NULL)) {
	    fmt_rt_return (buf,
			   basic_class_from_cvar
			   (messages[prev_tok_idx2], l_cvar, 0),
			   FALSE, rbuf);
	    fileout (rbuf, 0, keyword_ptr);
	  } else {
	    fileout (buf, 0, keyword_ptr);
	  }
	} else {
	  if (((l_cvar = get_local_var (M_NAME(messages[prev_tok_idx2]))) 
	       != NULL) ||
	      ((l_cvar = get_global_var (M_NAME(messages[prev_tok_idx2])))
	       != NULL)) {
	    fmt_rt_return (buf,
			   basic_class_from_cvar
			   (messages[prev_tok_idx2], l_cvar, 0),
			   FALSE, rbuf);
	    fileout (rbuf, 0, keyword_ptr);
	  } else {
	    fileout (buf, 0, keyword_ptr);
	  }
	}
      } else {
	fileout (buf, 0, keyword_ptr);
      }
    } else {
      fileout (buf, 0, keyword_ptr);
    }
  } else {
    fileout (buf, 0, keyword_ptr);
  }
}

static char *do_lval_ptrptr_argblk_trans (MESSAGE_STACK messages, 
					  int expr_start, char *buf,
					  char *buf2) {
  char *lval_class;
  static char expr_out[MAXMSG];
  if ((lval_class = c_lval_class (messages, expr_start)) != NULL) {
    if (argblk) {
      /* Then the lval is actually a char **, and we need to make
	 sure that it aligns correctly. */
      if (str_eq (lval_class, "String")) {
	argblk_ptrptr_trans_1 (messages, expr_start);
	strcatx (buf2, "(char **)",
		 fmt_rt_return (buf, lval_class, TRUE, expr_out), NULL);
	return buf2;
      } else {
	return fmt_rt_return (buf, lval_class, TRUE, buf2);
      }
    } else {
      return buf;
    }
  } else {
    return buf;
  }
}

static void mark_eval_expr (MESSAGE_STACK messages, int keyword_ptr,
			    int end_idx) {
  int i;
  for (i = keyword_ptr; i >= end_idx; i--) {
    ++messages[i]->evaled;
    ++messages[i]->output;
  }

}

/*
 * register_buf is only used (so far) when the statement occurs
 * within an argument block, which buffers statements differently.
 * In other places it should be safe to register C variables normally.
 */
int eval_keyword_expr (MESSAGE_STACK messages, int keyword_ptr) {
  int prev_tok_idx, next_tok_idx, end_idx, i;
  char buf[MAXMSG], expr_out[MAXMSG];
  char *c, register_buf[MAXMSG];
  int stack_start_idx, stack_end_idx;
  int agg_end_idx;
  OBJECT_CONTEXT context;
  bool have_cvar_registration = false;

  stack_start_idx = stack_start (messages);
  stack_end_idx = get_stack_top (messages);
  memset (register_buf, 0, MAXMSG);

  if ((next_tok_idx = nextlangmsg (messages, keyword_ptr)) != ERROR) {

    if (argblk) {
      /* The first fmt_rt_expr () call is to find the end of the
	 expression. The second call formats the expression with
	 the argblk variables provided by register_argblk_c_vars_1 (). */
      fmt_rt_expr (messages, next_tok_idx, &end_idx, buf);
      mark_eval_expr (messages, keyword_ptr, end_idx);
      if ((c = fmt_register_argblk_c_vars_1 (messages, next_tok_idx, end_idx))
	  != NULL) {
	strcpy (register_buf, c);
      }
      fmt_rt_expr (messages, next_tok_idx, &end_idx, buf);
    } else { /* if (argblk) */
      fmt_rt_expr (messages, next_tok_idx, &end_idx, buf);
      mark_eval_expr (messages, keyword_ptr, end_idx);
      for (i = keyword_ptr; i >= end_idx; i--) {
	if (check_expr_c_vars (messages, i, stack_start_idx,
			       stack_end_idx, 
			       &agg_end_idx)) {
	  have_cvar_registration = true;
	}
	i = agg_end_idx;
      }
    } /* if (argblk) */

    context = object_context (messages, next_tok_idx);
    switch (context)
      {
      case c_argument_context:
	/* 
	   No examples that show where to place the register calls -	   
	   this is the normal way of sequencing register calls
	   in method_args ()/eval_arg ().
	*/
	if (*register_buf) 
	  fileout (register_buf, 0, FRAME_START_IDX);
	prev_tok_idx = keyword_ptr;
	while ((prev_tok_idx = prevlangmsg (messages, prev_tok_idx)) 
	       != ERROR) {
	  if ((M_TOK(messages[prev_tok_idx]) == SEMICOLON) ||
	      (M_TOK(messages[prev_tok_idx]) == OPENBLOCK)) {
	    fileout (buf, 0, keyword_ptr);
	    return SUCCESS;
	  }
	  if (is_printf_fmt (M_NAME(messages[prev_tok_idx]),
			     M_NAME(messages[prev_tok_idx]))) {
	    fileout (fmt_printf_fmt_arg (messages, next_tok_idx,
					 stack_start (messages),
					 buf, expr_out),
		     0, keyword_ptr);
	    return SUCCESS;
	  } else {
	    if (use_new_c_rval_semantics_b (messages, keyword_ptr)) {
	      eval_c_argument_handler (messages, keyword_ptr, buf);
	      return SUCCESS;
	    }
	  }
	} 
	break;
      case c_context:
	/* 
	   Here also - just try normal output sequencing for now.
	*/
	if (*register_buf) 
	  fileout (register_buf, 0, FRAME_START_IDX);
	if (ctrlblk_pred) {
	  if ((prev_tok_idx = prevlangmsg (messages, keyword_ptr)) 
	      != ERROR) {
	    if (IS_C_UNARY_MATH_OP (M_TOK(messages[prev_tok_idx]))) {
	      fileout (fmt_rt_return (buf, INTEGER_CLASSNAME, FALSE, expr_out),
		       0, keyword_ptr);
	    } else {
	      /* TODO - This could probably use a check for
		 keyword_ptr == pred_start_ptr. */
	      if (M_TOK(messages[prev_tok_idx]) == OPENPAREN) {
		fileout (fmt_rt_return (buf, INTEGER_CLASSNAME, FALSE, 
					expr_out),
			 0, keyword_ptr);
	      } else {
		fileout (buf, 0, keyword_ptr);
	      }
	    }
	  } else {
	    fileout (buf, 0, keyword_ptr);
	  }
	} else {
	  char buf2[MAXMSG];
	  do_lval_ptrptr_argblk_trans (messages, keyword_ptr, buf, buf2);
	  fileout (buf2, 0, keyword_ptr);
	}
	break;
      case argument_context:
	/* 
	   See the comments for use_new_c_rval_semantics () in rexpr.c.
	   This only tries to find the type from the CVAR that is the
	   lvalue, so it could probably use a lot more work.
	   Translates anything it can find a lvalue CVAR for, however.
	*/
	if (*register_buf)
	  fileout (register_buf, 0, FRAME_START_IDX);
	eval_c_argument_handler (messages, keyword_ptr, buf);
	break;
      default:
	output_eval_bufs_a (register_buf, buf, keyword_ptr);
	if (have_cvar_registration) {
	  output_delete_cvars_call (messages, end_idx, stack_end_idx);
	}
	break;
      }
  }
  return SUCCESS;
}

/*
 *  This is similar to eval_keyword_expr (), above, and can probably
 *  be trimmed.
 */
int rval_expr_1 (MSINFO *ms) {
  int prev_tok_idx, end_idx, i;
  static char buf[MAXMSG];
  static char buf2[MAXMSG];
  static char *cvar_reg = NULL;
  char *lval_class;
  int agg_end_idx;
  OBJECT_CONTEXT context;

  if (argblk) {
    /* The first fmt_rt_expr () call is to find the end of the
       expression. The second call formats the expression with
       the argblk variables provided by register_argblk_c_vars_1 (). */
    fmt_rt_expr_ms (ms, &end_idx, buf);
    cvar_reg = fmt_register_argblk_c_vars_1
      (ms -> messages, ms -> tok, end_idx);
    fmt_rt_expr_ms (ms, &end_idx, buf);
  } else { /* if (argblk) */
    fmt_rt_expr_ms (ms, &end_idx, buf);
    for (i = ms -> tok; i >= end_idx; i--) {
      (void)check_expr_c_vars (ms -> messages, i, ms -> stack_start,
			       ms -> stack_ptr, 
			       &agg_end_idx);
      i = agg_end_idx;
    }
  } /* if (argblk) */

  for (i = ms -> tok; i >= end_idx; i--) {
    ++ms -> messages[i]->evaled;
    ++ms -> messages[i]->output;
  }

  context = object_context_ms (ms);
  switch (context)
    {
    case c_argument_context:
      /* 
	 No examples that show where to place the register calls -	   
	 this is the normal way of sequencing register calls
	 in method_args ()/eval_arg ().
      */
      if (cvar_reg) 
	fileout (cvar_reg, 0, FRAME_START_IDX);

      /* This call might be redundant if we give the parameter class
	 as an argument, but then we might need to write another 
	 specialized fn later. */
      if ((lval_class = c_lval_class (ms -> messages, ms -> tok)) 
	  != NULL) {
	if (argblk) {
	  do_lval_ptrptr_argblk_trans (ms -> messages, ms -> tok,
				       buf, buf2);
	} else {
	  fmt_rt_return (buf, lval_class, TRUE, buf2);
	}
	fileout (buf2, 0, ms -> tok);
	return SUCCESS;
      }
      prev_tok_idx = ms -> tok;
      while ((prev_tok_idx = prevlangmsg (ms -> messages, prev_tok_idx)) 
	     != ERROR) {
	if ((M_TOK(ms -> messages[prev_tok_idx]) == SEMICOLON) ||
	    (M_TOK(ms -> messages[prev_tok_idx]) == OPENBLOCK)) {
	  fileout (buf, 0, ms -> tok);
	  return SUCCESS;
	}
	if (is_printf_fmt (M_NAME(ms -> messages[prev_tok_idx]),
			   M_NAME(ms -> messages[prev_tok_idx]))) {
	  fmt_printf_fmt_arg_ms (ms, buf, buf2);
	  fileout (buf2, 0, ms -> tok);
	  return SUCCESS;
	} else {
	  if (use_new_c_rval_semantics_b (ms -> messages, ms -> tok)) {
	    eval_c_argument_handler (ms -> messages, ms -> tok, buf);
	    return SUCCESS;
	  }
	}
      } 
      break;
    case c_context:
      /* 
	 Here also - just try normal output sequencing for now.
      */
      if (cvar_reg) 
	fileout (cvar_reg, 0, FRAME_START_IDX);
      if (ctrlblk_pred) {
	if ((prev_tok_idx = prevlangmsg (ms -> messages, ms -> tok)) 
	    != ERROR) {
	  if (IS_C_UNARY_MATH_OP (M_TOK(ms -> messages[prev_tok_idx]))) {
	    fileout (fmt_rt_return (buf, INTEGER_CLASSNAME, FALSE, buf2),
		     0, ms -> tok);
	  } else {
	    /* TODO - This could probably use a check for
	       ms -> tok == pred_start_ptr. */
	    if (M_TOK(ms -> messages[prev_tok_idx]) == OPENPAREN) {
	      fileout (fmt_rt_return (buf, INTEGER_CLASSNAME, FALSE,
					  buf2),
			   0, ms -> tok);
	    } else {
	      fileout (buf, 0, ms -> tok);
	    }
	  }
	} else {
	  fileout (buf, 0, ms -> tok);
	}
      } else {
	if (argblk) {
	  do_lval_ptrptr_argblk_trans (ms -> messages, ms -> tok,
				       buf, buf2);
	  fileout (buf2, 0, ms -> tok);
	} else {
	  fileout (buf, 0, ms -> tok);
	}
      }
      break;
    case argument_context:
      /* 
	 See the comments for use_new_c_rval_semantics () in rexpr.c.
	 This only tries to find the type from the CVAR that is the
	 lvalue, so it could probably use a lot more work.
	 Translates anything it can find a lvalue CVAR for, however.
      */
      if (cvar_reg)
	fileout (cvar_reg, 0, FRAME_START_IDX);
      eval_c_argument_handler (ms -> messages, ms -> tok, buf);
      break;
    default:
      output_eval_bufs_a ((cvar_reg ? cvar_reg : ""),
			  buf, ms -> tok);
      break;
    }
  return SUCCESS;
}

static inline int const_expr_op_precedence (int precedence, MESSAGE_STACK messages, int op_ptr) {

  int retval = FALSE;
  MESSAGE *m_op;
  OP_CONTEXT context;

  m_op = messages[op_ptr];

  if (!IS_C_OP (m_op -> tokentype) ||
      ((context = op_context (messages, op_ptr)) == op_not_an_op))
    return FALSE;

  switch (precedence)
    {
    case 0:
      switch (m_op -> tokentype)
	{
	case OPENPAREN:
	case CLOSEPAREN:
	case ARRAYOPEN:
	case ARRAYCLOSE:
	case INCREMENT:
	case DECREMENT:
	case DEREF:
	case PERIOD:
	  retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 1:
      switch (m_op -> tokentype) 
	{
	case SIZEOF:
	case EXCLAM:
	  retval = TRUE;
	  break;
	case PLUS:      
	case MINUS:     
	case ASTERISK:  
	case AMPERSAND:
	  if (context == op_unary_context)
	    retval = TRUE;
	default: 
	  break;
	}
      break;
    case 2:
      if ((m_op -> tokentype == DIVIDE) ||
	  (m_op -> tokentype == MODULUS))
	retval = TRUE;
      if ((m_op -> tokentype == ASTERISK) && (context == op_binary_context))
	retval = TRUE;
      break;
    case 3:
      if ((m_op -> tokentype == PLUS) ||
	  (m_op -> tokentype == MINUS))
	retval = TRUE;
      break;
    case 4:
      if ((m_op -> tokentype == ASL) ||
	  (m_op -> tokentype == ASR))
	retval = TRUE;
      break;
    case 5:
      if ((m_op -> tokentype == LT) ||
	  (m_op -> tokentype == LE) ||
	  (m_op -> tokentype == GT) ||
	  (m_op -> tokentype == GE))
	retval = TRUE;
      break;
    case 6:
      if ((m_op -> tokentype == BOOLEAN_EQ) ||
	  (m_op -> tokentype == INEQUALITY))
	retval = TRUE;
      break;
    case 7:
      if (m_op -> tokentype == BIT_AND)
	retval = TRUE;
      break;
    case 8:
      if ((m_op -> tokentype == BIT_OR) || 
	  (m_op -> tokentype == BIT_XOR))
	retval = TRUE;
      break;
    case 9:
      if (m_op -> tokentype == BOOLEAN_AND)
	retval = TRUE;
      break;
    case 10:
      if (m_op -> tokentype == BOOLEAN_OR)
	retval = TRUE;
      break;
    case 11:
      if ((m_op -> tokentype == CONDITIONAL) ||
	  (m_op -> tokentype == COLON))
	retval = TRUE;
    case 12:
      if ((m_op -> tokentype == EQ) ||
	  (m_op -> tokentype == ASR_ASSIGN) ||
	  (m_op -> tokentype == ASL_ASSIGN) ||
	  (m_op -> tokentype == PLUS_ASSIGN) ||
	  (m_op -> tokentype == MINUS_ASSIGN) ||
	  (m_op -> tokentype == MULT_ASSIGN) ||
	  (m_op -> tokentype == DIV_ASSIGN) ||
	  (m_op -> tokentype == BIT_AND_ASSIGN) ||
	  (m_op -> tokentype == BIT_OR_ASSIGN) ||
	  (m_op -> tokentype == BIT_XOR_ASSIGN))
	retval = TRUE;
      break;
    case 13:
      if (m_op -> tokentype == ARGSEPARATOR)
	retval = TRUE;
      break;
    default:
      break;
    }

  return retval;
}

OBJECT *const_expr_class (MESSAGE_STACK messages, int start_idx, 
			  int end_idx) {
  int i, j,
    subexpr_start_idx,
    subexpr_end_idx,
    matching_paren_idx,
    precedence = 0,
    binary_op_rcvr_idx,
    binary_op_arg_idx,
    starting_eval_level,
    n_th_param;
  OBJECT *subexpr_class,
    *result_class,
    *token_object;
  CVAR *token_cvar;
  char *token_cvar_basic_class;
  MESSAGE *m, *m_op,
    *m_binary_op_rcvr;
  METHOD *op_method,
    *self_method;

  starting_eval_level = messages[start_idx] -> evaled;

 re_eval:  for (i = start_idx; i >= end_idx; i--) {
    m = messages[i];
    if (M_ISSPACE(m))
      continue;
    switch (M_TOK(m))
      {
      case LABEL:
	if (!precedence && !m -> obj) {
	  if ((token_object = get_object (M_NAME(m), NULL)) != NULL) {
	    m -> obj = token_object -> __o_class;
	  } else {
	    if (((token_cvar = get_local_var (M_NAME(m))) != NULL) ||
		((token_cvar = get_global_var (M_NAME(m))) != NULL)) {
	      if (((token_cvar -> type_attrs & CVAR_ATTR_STRUCT_TAG) ||
		  (token_cvar -> type_attrs & CVAR_ATTR_STRUCT_PTR_TAG)) &&
		  (token_cvar -> members == NULL)) {
		CVAR *c_defn, *c_mbr;
		if (((c_defn = get_local_var (token_cvar -> type)) != NULL) ||
		    ((c_defn = get_global_var (token_cvar -> type)) != NULL)) {
		  int struct_end_idx;
		  if ((struct_end_idx = 
		       struct_end (messages, i,
					 get_stack_top (messages))) > 0) {
		    if ((c_mbr = struct_member_from_expr_b (messages, 
							    i,
							    struct_end_idx,
							    c_defn)) 
			!= NULL) {
		      token_cvar_basic_class =
			basic_class_from_cvar (m, token_cvar, 0);
		    } else {/* if ((c_mbr = struct_member_from_expr ... */
		      token_cvar_basic_class =
			basic_class_from_cvar (m, token_cvar, 0);
		    }/* if ((c_mbr = struct_member_from_expr ... */
		  } else { /* if ((struct_end_idx =  ... */
		    token_cvar_basic_class =
		      basic_class_from_cvar (m, token_cvar, 0);
		  }  /* if ((struct_end_idx =  ... */ 
		} else { /* if (((c_defn =  ... */
		  token_cvar_basic_class =
		    basic_class_from_cvar (m, token_cvar, 0);
		} /* if (((c_defn =  ... */ 
	      } else {/* if(((token_cvar -> type_attrs & CVAR_ATTR_STRUCT_TAG)*/
		if ((token_cvar -> attrs & CVAR_ATTR_STRUCT_DECL) &&
		    token_cvar -> members) {
		  int struct_end_idx;
		  CVAR *c_mbr;
		  if ((struct_end_idx = 
		       struct_end (messages, i,
					 get_stack_top (messages))) > 0) {
		    if ((c_mbr = struct_member_from_expr_b (messages, 
							  i,
							  struct_end_idx,
							  token_cvar)) 
			!= NULL) {
		      token_cvar_basic_class =
			basic_class_from_cvar (messages[i], c_mbr, 0);
		    } else {/* if ((c_mbr = struct_member_from_expr ... */
		      token_cvar_basic_class =
			basic_class_from_cvar (messages[i], token_cvar, 0);
		    }/* if ((c_mbr = struct_member_from_expr ... */
		  } else {
		    token_cvar_basic_class =
		      basic_class_from_cvar (messages[i], token_cvar, 0);
		  }
		} else {/* if ((token_cvar -> attrs & CVAR_ATTR_STRUCT_DECL)*/
		  token_cvar_basic_class =
		    basic_class_from_cvar (messages[i], token_cvar, 0);
		}/* if ((token_cvar -> attrs & CVAR_ATTR_STRUCT_DECL)*/
	      }/* if (((token_cvar -> type_attrs & CVAR_ATTR_STRUCT_TAG) */
	      m -> obj = get_class_object (token_cvar_basic_class);
	    } else {
	      if (m -> attrs & TOK_SELF) {
		m -> obj = rcvr_class_obj;
	      } else {
		if (interpreter_pass == method_pass) {
		  if ((self_method = new_methods[new_method_ptr + 1]->method)
		      != NULL) {
		    for (n_th_param = 0; n_th_param < self_method -> n_params;
			 n_th_param++) {
		      if (!strcmp 
			  (M_NAME(m), 
			   self_method -> params[n_th_param] -> name)) {
			m -> obj = 
			  get_class_object 
			  (self_method -> params[n_th_param] -> class);
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
	break;
      case LITERAL_CHAR:
	if (!precedence && !m -> obj) {
	  if ((m -> obj = get_class_object (CHARACTER_CLASSNAME)) == NULL) {
	    class_object_search (CHARACTER_CLASSNAME, FALSE);
	    m -> obj = get_class_object (CHARACTER_CLASSNAME);
	  }
	}
	break;
      case LITERAL:
	if (!precedence && !m -> obj) {
	  if ((m -> obj = get_class_object (STRING_CLASSNAME)) == NULL)  {
	    class_object_search (STRING_CLASSNAME, FALSE);
	    m -> obj = get_class_object (STRING_CLASSNAME);
	  }
	}
	break;
      case INTEGER:
      case LONG:
	if (!precedence && !m -> obj) {
	  if ((m -> obj = get_class_object (INTEGER_CLASSNAME)) == NULL) {
	    class_object_search (INTEGER_CLASSNAME, FALSE);
	    m -> obj = get_class_object (INTEGER_CLASSNAME);
	  }
	}
	break;
      case FLOAT:
	if (!precedence && !m -> obj) {
	  if ((m -> obj = get_class_object (FLOAT_CLASSNAME)) == NULL) {
	    class_object_search (FLOAT_CLASSNAME, FALSE);
	    m -> obj = get_class_object (FLOAT_CLASSNAME);
	  }
	}
	break;
      case LONGLONG:
	if (!precedence && !m -> obj) {
	  if ((m -> obj = get_class_object (LONGINTEGER_CLASSNAME)) == NULL) {
	    class_object_search (LONGINTEGER_CLASSNAME, FALSE);
	    m -> obj = get_class_object (LONGINTEGER_CLASSNAME);
	  }
	}
	break;
      default:
	if (const_expr_op_precedence (precedence, messages, i)) {
	  m_op = messages[i];
	  switch (M_TOK(m_op))
	    {
	    case OPENPAREN:
	      if ((matching_paren_idx = match_paren (messages, i, end_idx))
		  == ERROR) {
		return NULL;
	      } else {
		int i_2;
		subexpr_start_idx = nextlangmsg (messages, i);
		subexpr_end_idx = prevlangmsg (messages, matching_paren_idx);
		subexpr_class = const_expr_class (messages, subexpr_start_idx,
						  subexpr_end_idx);

		for (i_2 = i; i_2 >= matching_paren_idx; i_2--) {
		  if (messages[i_2] -> obj == NULL) {
		    messages[i_2] -> obj = subexpr_class;
		  } else {
		    messages[i_2] -> value_obj = subexpr_class;
		  }
		  ++(messages[i_2] -> evaled);
		}
		i = subexpr_end_idx;
	      }
	      break;
	    case PLUS:
	    case MINUS:
	    case MULT:
	    case DIVIDE:
	    case ASL:
	    case ASR:
	    case BIT_AND:
	    case BIT_OR:
	    case BIT_XOR:
	    case BOOLEAN_EQ:
	    case GT:
	    case LT:
	    case GE:
	    case LE:
	    case INEQUALITY:
	    case BOOLEAN_AND:
	    case BOOLEAN_OR:
	    case MODULUS:
	      if (((binary_op_rcvr_idx = 
		   prevlangmsg (messages, i)) != ERROR) &&
		  ((binary_op_arg_idx = 
		    nextlangmsg (messages, i)) != ERROR)) {
		m_binary_op_rcvr = messages[binary_op_rcvr_idx];
		if ((op_method = get_instance_method 
		     (m_binary_op_rcvr, m_binary_op_rcvr->obj,
		      M_NAME(m_op), ERROR, FALSE)) != NULL) {
		  OBJECT *op_result_class;
		  int i_2;
		  op_result_class = 
		    get_class_object (op_method -> returnclass);
		  for (i_2 = binary_op_rcvr_idx; 
		       i_2 >= binary_op_arg_idx;
		       i_2--) {
		    if (messages[i_2] -> obj == NULL) {
		      messages[i_2] -> obj = op_result_class;
		    } else {
		      messages[i_2] -> value_obj = op_result_class;
		    }
		    ++(messages[i_2] -> evaled);
		  }
		}
	      }
	      break;
	    }  /* switch (M_TOK(m_op)) */
	} /* 	if (const_expr_op_precedence (precedence, messages, i)) { */
      } /* switch (M_TOK(m)) */
  }
  for (; precedence < 13; ++precedence) {
    j = 0;
    while (j >= end_idx) {
      if (const_expr_op_precedence (precedence, messages, j) &&
	  (messages[j] -> evaled <= starting_eval_level))
	goto re_eval;
      --j;
    }
  }
  result_class = M_VALUE_OBJ (messages[start_idx]);
  for (i = start_idx; i >= end_idx; i--) {
    m = messages[i];
    if (M_ISSPACE(m)) continue;
    if (IS_OBJECT(m -> obj) && IS_CLASS_OBJECT (m -> obj))
      m -> obj = NULL;
    if (IS_OBJECT(m -> value_obj) && IS_CLASS_OBJECT (m -> value_obj))
      m -> value_obj = NULL;
  }
  return result_class;
}

static bool register_prefix_expr_CVARs (MESSAGE_STACK messages,
					int expr_start_idx, 
					int expr_end_idx) {
  MESSAGE *m;
  int i;
  int agg_var_end_idx;
  bool retval = false;
  
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (!IS_MESSAGE(messages[i]))
      break;
    if (M_ISSPACE(messages[i]))
      continue;
    if (M_TOK(messages[i]) == LABEL) {
      m = messages[i];
      if ((get_local_var (m -> name) != NULL) ||
	  (get_global_var_not_shadowed (m -> name) != NULL)) {
	if (argblk) {
	  /* single-token only at the moment... */
	  register_argblk_c_vars_1 (messages, i, i);
	} else {
	  (void)register_c_var (m, messages, i, &agg_var_end_idx);
	}
	retval = true;
      }
    }
  }
  return retval;
}

static void check_r_assignment_expr (MESSAGE_STACK messages,
				     int expr_start_idx,
				     int expr_end_idx) {
  int i, arg_start_idx;
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (!IS_MESSAGE(messages[i]))
      break;
    if (M_ISSPACE(messages[i]))
      continue;
    if (IS_C_ASSIGNMENT_OP(M_TOK(messages[i])))  {
      arg_start_idx = nextlangmsg (messages, i);
      check_constant_expr (messages, arg_start_idx, expr_end_idx);
    }
  }
}

static bool is_symbol_deref_expr (MESSAGE_STACK messages, int prefix_idx) {
  int i;
  for (i = prefix_idx; messages[i]; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if (IS_OBJECT(messages[i] -> obj)) {
	if (IS_OBJECT(messages[i] -> obj -> __o_class) &&
	    (messages[i] -> obj -> __o_class ==
	     ct_defclasses -> p_symbol_class)) {
	  return true;
	}
      }
    }
  }
  return false;
}

static void pmea_cvar_cleanup_call (MESSAGE_STACK messages,
				    int tok_idx, int stack_ptr) {
  int term_idx;
  term_idx = scanforward (messages, tok_idx, stack_ptr, SEMICOLON);
  fileout ("\ndelete_method_arg_cvars ();\n",
	   0, term_idx - 1);
}

extern int m_message_stack;  /* True if called from eval_arg (). */
extern OBJECT *eval_arg_obj;
/*
 *  Simple prefix expression : <op><expr>.
 *  or, <op><(+><expr> if called by prefix_method_expr_b ().
 */
int prefix_method_expr_a (MESSAGE_STACK messages, int prefix_idx,
			  int obj_idx) {
  OBJECT_CONTEXT context;
  METHOD *prefix_method;
  char *expr_buf, result_buf[MAXMSG];
  static char esc_buf_out[MAXMSG];
  int i;
  int arg_idx, fn_idx, prefix_idx_2;
  int prev_idx, leading_paren_idx = -1,
    typecast_open_idx = -1;
  int stack_start_idx, stack_top_idx, end_idx;
  CFUNC *cfn;
  char *c = NULL;
  char *typecast_classname = NULL;
  bool is_rvalue = false;
  bool have_cvar_reg = false;
  CTRLBLK *ctrlblk;
  MSINFO ms;

  ms.messages = messages;
  ms.stack_start = stack_start_idx = stack_start (messages);
  ms.stack_ptr = stack_top_idx = get_stack_top (messages);

  if (obj_is_fn_expr_lvalue (messages, obj_idx, stack_top_idx)) {
    format_obj_lval_fn_expr (messages, obj_idx);
    return SUCCESS;
  }

  if (messages[obj_idx] -> obj) {
    if ((prefix_method = 
	 get_prefix_instance_method 
	 (messages[prefix_idx],
	  messages[obj_idx] -> obj -> __o_class,
	  M_NAME(messages[prefix_idx]),
	  FALSE)) != NULL) {

      if (m_message_stack) { /* Temporary stack used by eval_arg (). */ 
	context = argument_context;
      } else {
	if (leading_paren_idx != -1) {
	  context = object_context (messages, leading_paren_idx);
	} else {
	  context = object_context (messages, prefix_idx);
	}
      }

      switch (context)
	{
	case c_argument_context:
	case c_context:

	  prefix_idx_2 = find_leading_tok_idx (messages, prefix_idx,
					       stack_start_idx,
					       stack_top_idx);
	  if (!ctrlblk_pred) {
	    /* Check for a rvalue expression (this fn only handles
	       objects in the rvalue - i.e., the lvalue is C variable).
	    */
	    if ((prev_idx = prevlangmsg (messages, prefix_idx)) != ERROR) {
	      if  (M_TOK(messages[prev_idx]) == OPENPAREN) {
		while (M_TOK(messages[prev_idx]) == OPENPAREN) {
		  leading_paren_idx = prev_idx;
		  if ((prev_idx = prevlangmsg (messages, prev_idx)) == ERROR)
		    break;
		}
	      } else if ((M_TOK(messages[prev_idx]) == CLOSEPAREN) &&
			 (TOK_IS_TYPECAST_EXPR)) {
		typecast_open_idx = match_paren_rev (messages, prev_idx,
						     stack_start_idx);
	      }
	    }
	    if (leading_paren_idx != -1) {
	      if ((prev_idx = prevlangmsg (messages, leading_paren_idx)) 
		  != ERROR) {
		if (IS_C_ASSIGNMENT_OP (M_TOK(messages[prev_idx]))) {
		  is_rvalue = true;
		}
		/* Check here also for a function label, though we
		   do the actual check and translation below. 
		   We just don't count a leading paren that is the
		   start of a fn arg list as part of the object
		   expression. */
		if (M_TOK(messages[prev_idx]) == LABEL) {
		  if (get_function (M_NAME(messages[prev_idx]))) {
		    leading_paren_idx = -1;
		  }
		}
	      }
	    } else if (typecast_open_idx != -1) {
	      typecast_classname =
		basic_class_from_typecast (messages, typecast_open_idx,
					   prev_idx);
	    } else {
	      if ((prev_idx = prevlangmsg (messages, prefix_idx)) 
		  != ERROR) {
		if (IS_C_ASSIGNMENT_OP (M_TOK(messages[prev_idx]))) {
		  is_rvalue = true;
		}
	      }
	    }
	  }

	  if (ctrlblk_pred) {
	    /* Except for "for" loop clauses. */
	    if (!for_init && !for_term && !for_inc) {
	      ctrlblk = ctrlblk_pop ();
	      ctrlblk_push (ctrlblk);
	      ctrlblk_pred_rt_expr (messages, prefix_idx);
	      for (i = ctrlblk -> pred_start_ptr - 1;
		   i > ctrlblk -> pred_end_ptr; i--) {
		++messages[i] -> evaled;
		++messages[i] -> output;
	      }
	      return SUCCESS;
	    }
	  }

	  if (is_rvalue) {
	    if (leading_paren_idx != -1) {
	      ms.tok = leading_paren_idx;
	      expr_buf = collect_expression (&ms, &end_idx); 
	    } else {
	      ms.tok = prefix_idx_2;
	      expr_buf = collect_expression (&ms, &end_idx); 
	    }
	  } else {
	    ms.tok = prefix_idx_2;
	    expr_buf = collect_expression (&ms, &end_idx); 
	    de_newline_buf (expr_buf);
	    if (leading_paren_idx != -1)
	      leading_paren_idx = -1;
	  }

	  if (register_prefix_expr_CVARs (messages, prefix_idx_2, end_idx)) {
	    have_cvar_reg = true;
	    if (argblk) {
	      /* We have to re-write the tokens with the cvartab names. */
	      expr_buf = collect_expression (&ms, &end_idx); 
	      de_newline_buf (expr_buf);
	    }
	  }
	  check_r_assignment_expr (messages, prefix_idx_2, end_idx);
	  if (typecast_open_idx != -1) {
	    char result_buf2[MAXMSG];
	    strcatx (result_buf2, EVAL_EXPR_FN, " (\"",
		     escape_str_quotes (expr_buf, esc_buf_out), "\")", NULL);
	    fmt_rt_return (result_buf2, typecast_classname, TRUE,
			   result_buf);
	  } else {
	    strcatx (result_buf, EVAL_EXPR_FN, " (\"",
		     escape_str_quotes (expr_buf, esc_buf_out), "\")", NULL);
	  }
	  __xfree (MEMADDR(expr_buf));

	  if ((arg_idx =
	       obj_expr_is_arg (messages, prefix_idx_2,
				stack_start_idx,
				&fn_idx)) != ERROR) {
	    /*
	     *  The statement *must* be filed out at obj_idx, due to
	     *  the way the frames are set.  Otherwise, we get
	     *  memory corruption in the output buffering and ugly
	     *  heap corruption messages from glibc.
	     *
	     *  Not sure if that's correctable, if doing a lookahead 
	     *  (and probably a symbol evaluation) on a C unary op while
	     *  framing the input turns out to be too difficult.
	     */
	    if ((cfn = get_function (M_NAME(messages[fn_idx]))) != NULL) {
	      fileout (fn_param_return_trans
			   (messages[fn_idx], 
			    cfn, result_buf, arg_idx),
			   0, obj_idx);
	    } else {
	      /* Works as a catch-all here, too in case there's
		 an expression type not covered elsewhere. */
	      if (is_while_pred_start_2 (messages, prefix_idx_2)) {
		fileout 
		  (fmt_rt_return (result_buf, INTEGER_CLASSNAME, TRUE,
				  esc_buf_out),
		   0, obj_idx);
	      } else {
		fileout
		  (obj_2_c_wrapper_trans 
		   (messages, prefix_idx_2, messages[prefix_idx_2],
		    messages[obj_idx]->obj, prefix_method, 
		    result_buf, TRUE),
		   0, obj_idx);
	      }
	    }
	  } else {
	    if (is_rvalue) {
	      if (leading_paren_idx != -1) {
		if ((c = c_lval_class (messages, leading_paren_idx)) != NULL) {
		  fileout (fmt_rt_return (result_buf, c, TRUE, esc_buf_out),
			   0, obj_idx);
		} else {
		  fileout
		    (obj_2_c_wrapper_trans 
		     (messages, leading_paren_idx, messages[prefix_idx_2],
		      messages[obj_idx]->obj, prefix_method, 
		      result_buf, TRUE),
		     0, obj_idx);
		}
	      } else {
		if ((c = c_lval_class (messages, prefix_idx)) != NULL) {
		  fileout (fmt_rt_return (result_buf, c, TRUE, esc_buf_out),
			   0, obj_idx);
		} else {
		  fileout
		    (obj_2_c_wrapper_trans 
		     (messages, prefix_idx_2, messages[prefix_idx_2],
		      messages[obj_idx]->obj, prefix_method, 
		      result_buf, TRUE),
		     0, obj_idx);
		}
	      }
	    } else {
	      if ((c = c_lval_class (messages, prefix_idx)) != NULL) {
		fileout (fmt_rt_return (result_buf, c, TRUE, esc_buf_out),
			 0, obj_idx);
	      } else if (fmt_arg_type (messages, prefix_idx,
				       stack_start_idx) ==
			 fmt_arg_ptr) {
		/* Just check if we need a cast for a "%#x" printf
		   format -- no object-to-c translation is necessary
		   when the result is an object and we just want to
		   print the object's address - that is generally
		   when we have a deref expression... if we're here
		   that should be only object expressions with ->
		   representing the method, so this should be
		   sufficient. */
		if (strstr (result_buf, "->") ||
		    is_symbol_deref_expr (messages, prefix_idx)) {
		  if (ptr_fmt_is_alt_int_fmt) {
		    strcatx (esc_buf_out, ALT_PTR_FMT_CAST, result_buf, NULL);
		    fileout (esc_buf_out, 0, obj_idx);
		  } else {
		    fileout (result_buf, 0, obj_idx);
		  }
		} else {
		  fileout
		    (obj_2_c_wrapper_trans 
		     (messages, prefix_idx_2, messages[prefix_idx_2],
		      messages[obj_idx]->obj, prefix_method, 
		      result_buf, TRUE),
		     0, obj_idx);
		}
	      } else if (typecast_open_idx != -1) {
		/* typcast C translation handled above */
		fileout (result_buf, 0, obj_idx);
	      } else {
		/* A prefix op can cause the interpreter to use a 
		   c_context at least a few cases where there's
		   no separate lvalue or rvalue. */
		if ((prev_idx = prevlangmsgstack (messages, prefix_idx))
		    != ERROR) {
		  if ((M_TOK(messages[prev_idx]) == SEMICOLON) ||
		      (M_TOK(messages[prev_idx]) == CLOSEBLOCK)) {
		    fileout (result_buf, 0, obj_idx);
		    if (have_cvar_reg)
		      pmea_cvar_cleanup_call (messages, end_idx,
					      ms.stack_ptr);
		  } else {
		    fileout
		      (obj_2_c_wrapper_trans 
		       (messages, prefix_idx_2, messages[prefix_idx_2],
			messages[obj_idx]->obj, prefix_method, 
			result_buf, TRUE),
		       0, obj_idx);
		    if (have_cvar_reg)
		      pmea_cvar_cleanup_call (messages, end_idx,
					      ms.stack_ptr);
		  }
		} else {
		  fileout
		    (obj_2_c_wrapper_trans 
		     (messages, prefix_idx_2, messages[prefix_idx_2],
		      messages[obj_idx]->obj, prefix_method, 
		      result_buf, TRUE),
		     0, obj_idx);
		  if (have_cvar_reg)
		    pmea_cvar_cleanup_call (messages, end_idx, ms.stack_ptr);
		}
	      }
	    }
	  }
	  if (leading_paren_idx != -1) {
	    for (i = leading_paren_idx; i >= end_idx; i--) {
	      if (!IS_MESSAGE(messages[i]))
		break;
	      ++messages[i] -> evaled;
	      ++messages[i] -> output;
	    }
	  } else {
	    for (i = prefix_idx_2; i >= end_idx; i--) {
	      if (!IS_MESSAGE(messages[i]))
		break;
	      ++messages[i] -> evaled;
	      ++messages[i] -> output;
	    }
	  }
	  return SUCCESS;
	  break;
	case argument_context:
	  ms.tok = prefix_idx;
	  expr_buf = collect_expression (&ms, &end_idx); 
	  fmt_eval_expr_str (expr_buf, result_buf);
	  eval_arg_obj = __ctalkCreateObject (expr_buf,
					      EXPR_CLASSNAME,
					      EXPR_SUPERCLASSNAME,
					      LOCAL_VAR);
	  __xfree (MEMADDR(expr_buf));
	  return SUCCESS;
	  break;
	case receiver_context:
	  /*
	   *  This is like c_context and c_argument_context above,
	   *  except it's only needed (so far) for an expression like
	   * 
	   *    if (<expr>)
	   *      ++<object>;
	   *
	   *  Check also for leading open parens, in which case
	   *  we normally take the first opening  paren to be
	   *  the start of the expression: 
	   *
	   *    if (<expr>)
	   *      (*|++|--|&<object> ... ;
	   *
	   *  These expressions basically check the context to
	   *  the closing paren of the if predicate, so receiver
	   *  context has its own case in order to avoid other, 
	   *  possibly (probably) incorrect evaluations.
	   */
	  prefix_idx = find_leading_tok_idx (messages, prefix_idx,
					     stack_start_idx,
					     stack_top_idx);
	  if ((prefix_idx_2 = prevlangmsg (messages, prefix_idx))
	      != ERROR) {
	    int end_idx;
	    while ((prefix_idx_2 = prevlangmsgstack 
		    (messages, prefix_idx_2)) != ERROR) {
	      if (M_TOK(messages[prefix_idx_2]) != OPENPAREN)
		break;
	    }
	    prefix_idx_2 = nextlangmsg (messages, prefix_idx_2);
	    (void)rt_expr(messages, prefix_idx_2, &end_idx, esc_buf_out);
	    return SUCCESS;
	  }
	  ms.tok = prefix_idx;
	  expr_buf = collect_expression (&ms, &end_idx);
	  register_prefix_expr_CVARs (messages, prefix_idx, end_idx);
	  check_r_assignment_expr (messages, prefix_idx, end_idx);
	  fmt_eval_expr_str (expr_buf, result_buf);
	  __xfree (MEMADDR(expr_buf));

	  if (!rte_output_fn_arg (messages, prefix_idx,
				  stack_start_idx, result_buf)) {
	    /* Works as a catch-all here, too in case there's
	       an expression type not covered elsewhere. */
	    fileout
	      (obj_2_c_wrapper_trans 
	       (messages, prefix_idx, messages[prefix_idx],
		messages[obj_idx]->obj, prefix_method, 
		result_buf, TRUE),
	       0, obj_idx);
	  } else {
	    fileout
	      (obj_2_c_wrapper_trans 
	       (messages, prefix_idx, messages[prefix_idx],
		messages[obj_idx]->obj, prefix_method, 
		result_buf, TRUE),
	       0, obj_idx);
	  }    
	  for (i = prefix_idx; i >= end_idx; i--) {
	    ++messages[i] -> evaled;
	    ++messages[i] -> output;
	  }
	  return SUCCESS;
	  break;
	default:
	  break;
	}
    }
  }
  return ERROR;
}

/*
 *  Prefix expression : <op><(+><expr>.
 *  Nearly the same as prefix_method_expr_a.
 */
int prefix_method_expr_b (MESSAGE_STACK messages, int obj_idx) {
  int prefix_op_idx;
  int stack_start_idx;

  stack_start_idx = stack_start (messages);

  if ((prefix_op_idx = is_leading_prefix_op (messages, obj_idx,
					     stack_start_idx)) != ERROR) {
    if (messages[prefix_op_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
      prefix_method_expr_a (messages, prefix_op_idx, obj_idx);
      return TRUE;
    }
  }

  return FALSE;
}

/* Output a run time eval for a statement of the form 
   <rcvr>  <unknown-label>
   when <unknown-label> isn't defined yet.
*/
void deferred_method_eval_a (MESSAGE_STACK messages, 
			     int start_idx,
			     int *end_idx) {
  static char output_buf[MAXMSG], output_buf2[MAXMSG];
  int i;

  fmt_rt_expr (messages, start_idx, end_idx, output_buf);

  if (ctrlblk_pred) {
    fmt_rt_return (output_buf, INTEGER_CLASSNAME, TRUE, output_buf2);
    fileout (output_buf2, 0, start_idx);
  } else {
    fileout (output_buf, 0, start_idx);
  }

  for (i = start_idx; i >= *end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }

}

void output_arg_rt_expr (MESSAGE_STACK messages, int rcvr_ptr, 
			 int method_ptr, METHOD *method) {
  MESSAGE *m;
  char expr_buf_tmp[MAXMSG];
    
  m = messages[method_ptr];

  fileout
    (obj_2_c_wrapper_trans 
     (messages, rcvr_ptr,
      m, m -> receiver_obj, method, 
      fmt_eval_expr_str (M_VALUE_OBJ(messages[rcvr_ptr])->__o_name,
			 expr_buf_tmp), 
      FALSE), 0, method_ptr);
}

/*
 *  Like method_arg_rt_expr, but checks for a "self" and 
 *  "self value" expression first, in which case it outputs
 *  __ctalk_self_internal () or __ctalk_self_internal_value ().
 */

int method_self_arg_rt_expr (MESSAGE_STACK messages, int idx) {

  int i, i_2, expr_end, stack_end;
  int n_subs, n_parens;
  int idx_next, idx_next_2;
  int idx_prev, idx_prev_2;
  char tokenbuf[MAXMSG], exprbuf[MAXMSG];
  char cvar_buf_out[MAXMSG];
  char errbuf[MAXLABEL];
  CVAR *c;

  if ((idx_next = nextlangmsg (messages, idx)) != ERROR) {

    if (METHOD_ARG_TERM_MSG_TYPE (messages[idx_next])) {
      strcatx (messages[idx] -> name, SELF_ACCESSOR_FN, " ()", NULL);
      messages[idx] -> tokentype = RESULT;
      return SUCCESS;
    } else {

      if ((idx_next_2 = nextlangmsg (messages, idx_next)) != ERROR) {

	if (str_eq (M_NAME(messages[idx_next]), "value") &&
	    METHOD_ARG_TERM_MSG_TYPE (messages[idx_next_2])) {
	  strcatx (messages[idx] -> name, SELF_VALUE_ACCESSOR_FN, " ()",
		   NULL);
	  _MPUTCHAR(messages[idx_next], ' ');
	  messages[idx_next] -> tokentype = RESULT;

	  return SUCCESS;
	} else if (IS_MESSAGE (messages[idx] -> receiver_msg) &&
		   messages[idx] -> receiver_msg -> attrs &
		   TOK_IS_CLASS_TYPECAST) {
	  OBJECT *cast_class_obj, *t;
	  cast_class_obj =
	    get_class_object (M_NAME(messages[idx] -> receiver_msg));
	  for (t = cast_class_obj -> instancevars; t; t = t -> next) {
	    if (str_eq (t -> __o_name, M_NAME(messages[idx_next]))) {
	      messages[idx_next] -> attrs |= OBJ_IS_INSTANCE_VAR;
	    }
	  }
	  for (t = cast_class_obj -> classvars; t; t = t -> next) {
	    if (str_eq (t -> __o_name, M_NAME(messages[idx_next]))) {
	      messages[idx_next] -> attrs |= OBJ_IS_CLASS_VAR;
	    }
	  }
	} else if (IS_OBJECT (rcvr_class_obj) &&
		   interpreter_pass == method_pass) {
	  OBJECT *t;
	  if (argblk) {
	    toks2str (messages, idx, idx_next, errbuf);
	    warning (messages[idx_next],
		     "Message, \"%s,\" receiver class is undefined "
		     "in argument block.  Waiting until run time to "
		     "evaluate the expression."
		     "\n\n\t%s\n\n", M_NAME(messages[idx_next]),
		     errbuf);
	    messages[idx_next] -> attrs |= TOK_IS_UNRESOLVED;
	  }
	  for (t = rcvr_class_obj -> instancevars; t; t = t -> next) {
	    if (str_eq (t -> __o_name, M_NAME(messages[idx_next]))) {
	      messages[idx_next] -> attrs |= OBJ_IS_INSTANCE_VAR;
	    }
	  }
	  for (t = rcvr_class_obj -> classvars; t; t = t -> next) {
	    if (str_eq (t -> __o_name, M_NAME(messages[idx_next]))) {
	      messages[idx_next] -> attrs |= OBJ_IS_CLASS_VAR;
	    }
	  }
	} else if (interpreter_pass == parsing_pass && argblk) {
	  toks2str (messages, idx, idx_next, errbuf);
	  warning (messages[idx_next],
		   "Message, \"%s,\" receiver class is undefined "
		   "in argument block.  Waiting until run time to "
		   "evaluate the expression."
		   "\n\n\t%s\n\n", M_NAME(messages[idx_next]),
		   errbuf);
	  messages[idx_next] -> attrs |= TOK_IS_UNRESOLVED;
	}
      }

    }

  }

  stack_end = get_stack_top (messages);


  for (i = idx, n_subs = 0, n_parens = 0, expr_end = idx; 
       i >= stack_end; i--) {

    if (METHOD_ARG_TERM_MSG_TYPE (messages[i]) && !n_subs && !n_parens) {
      expr_end = i + 1;
      goto e_e;
    }
    switch (messages[i] -> tokentype)
      {
      case ARRAYOPEN:
	++n_subs;
	break;
      case ARRAYCLOSE:
	--n_subs;
	break;
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	break;
      }

  }

 e_e:

  toks2str (messages, idx, expr_end, tokenbuf);
  fmt_eval_expr_str (tokenbuf, exprbuf);

  messages[idx] -> tokentype = RESULT;

  *cvar_buf_out = '\0';

  if ((idx_prev = prevlangmsg (messages, idx)) == ERROR)
    return ERROR;

  for (i_2 = idx; i_2 >= expr_end; --i_2) {
    CVAR *c_1;
    if (M_TOK(messages[i_2]) == LABEL) {
      if (messages[i_2] -> attrs & OBJ_IS_INSTANCE_VAR ||
	  messages[i_2] -> attrs & OBJ_IS_CLASS_VAR ||
	  messages[i_2] -> attrs & TOK_IS_UNRESOLVED)
	continue;
      if (((c_1 = get_local_var (M_NAME(messages[i_2])))
	   != NULL) ||
	  ((c_1 = get_global_var (M_NAME(messages[i_2])))
	   != NULL)) {
	if (M_TOK(messages[idx_prev]) == CLOSEPAREN) {
	  idx_prev = match_paren_rev (messages, idx_prev,
				      stack_start (messages));
	  if ((idx_prev = prevlangmsg (messages, idx_prev)) != ERROR) {
	    if (IS_C_ASSIGNMENT_OP(M_TOK(messages[idx_prev]))) {
	      if ((idx_prev_2 = prevlangmsg (messages, idx_prev)) != ERROR) {
		fileout
		  (fmt_register_c_method_arg_call
		   (c_1, M_NAME(messages[i_2]),
		    frame_at (parser_frame_ptr ()) ->scope,
		    cvar_buf_out), 0, idx_prev_2);
	      }
	    }
	  }
	} else { /* if (M_TOK(messages[idx_prev]) == CLOSEPAREN) */
	  if (IS_C_ASSIGNMENT_OP(M_TOK(messages[idx_prev]))) {
	    if ((idx_prev_2 = prevlangmsg (messages, idx_prev)) != ERROR) {
	      fileout
		(fmt_register_c_method_arg_call
		 (c_1, M_NAME(messages[i_2]),
		  frame_at (parser_frame_ptr ()) ->scope,
		  cvar_buf_out), 0, idx_prev_2);
	    }
	  }
	} /* if (M_TOK(messages[idx_prev]) == CLOSEPAREN) */
      }
    }
  }

  for (i = idx - 1; i >= expr_end; i--) {
    messages[i] -> name[0] = ' ', messages[i] -> name[1] = '\0';
    messages[i] -> tokentype = RESULT;
  }

  /* Check for a C lvalue and assignment. For now, we should only be
     here in the case of a class cast. It's a little superfluous
     right now, because at present we get the return class of the
     expression from the CVAR that is the lvalue. */
  if (messages[idx] -> receiver_msg &&
      (messages[idx] -> receiver_msg -> attrs & TOK_IS_CLASS_TYPECAST)) {
    if ((idx_prev = prevlangmsg (messages, idx)) != ERROR) {
      if (M_TOK(messages[idx_prev]) == CLOSEPAREN) {
	idx_prev = match_paren_rev (messages, idx_prev,
				    stack_start (messages));
	if ((idx_prev = prevlangmsg (messages, idx_prev)) != ERROR) {
	  if (IS_C_ASSIGNMENT_OP(M_TOK(messages[idx_prev]))) {
	    if ((idx_prev_2 = prevlangmsg (messages, idx_prev)) != ERROR) {
	      if (messages[idx_prev_2] -> attrs & TOK_IS_DECLARED_C_VAR) {
		if (((c = get_local_var (M_NAME(messages[idx_prev_2]))) 
		    != NULL) ||
		    ((c = get_global_var (M_NAME(messages[idx_prev_2])))
		     != NULL) ||
		    ((c = get_var_from_cvartab_name
		      (M_NAME(messages[idx_prev_2]))) != NULL)) {
		  fmt_rt_return (exprbuf,
				 basic_class_from_cvar
				 (messages[idx_prev_2], c, 0),
				 TRUE, messages[idx] -> name);
#ifdef SYMBOL_SIZE_CHECK
		  check_symbol_size (messages[idx] -> name);
#endif		  
		  if (*cvar_buf_out) {
		    fileout (cvar_buf_out, 0, idx_prev_2);
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  } else if (IS_C_ASSIGNMENT_OP (M_TOK(messages[idx_prev]))) {
    if ((idx_prev_2 = prevlangmsg (messages, idx_prev)) != ERROR) {
      if (messages[idx_prev_2] -> attrs & TOK_IS_DECLARED_C_VAR) {
	if (((c = get_local_var (M_NAME(messages[idx_prev_2]))) 
	     != NULL) ||
	    ((c = get_global_var (M_NAME(messages[idx_prev_2])))
	     != NULL) ||
	    ((c = get_var_from_cvartab_name
	      (M_NAME(messages[idx_prev_2]))) != NULL)) {
	  fmt_rt_return (exprbuf,
			 basic_class_from_cvar
			 (messages[idx_prev_2], c, 0),
			 TRUE, messages[idx] -> name);
	  for (i = idx; i >= expr_end; i--) {
	    ++messages[i] -> evaled;
	    ++messages[i] -> output;
	  }
	}
      }
    }
  }

  return SUCCESS;
}

/* This works okay if it's on the right-hand side of an assignment
   to a C OBJECT *. */
int method_param_arg_rt_expr (MESSAGE_STACK messages, int idx,
			      METHOD *method) {

  int i, expr_end, stack_end;
  int n_subs, n_parens;
  int idx_next, idx_next_2;
  int idx_prev, idx_prev_2;
  CVAR *lval_cvar;
  int max_param;
  char tokenbuf[MAXMSG], exprbuf[MAXMSG];

  if ((idx_prev = prevlangmsg (messages, idx)) == ERROR)
    return ERROR;

  if (messages[idx_prev] -> attrs & TOK_IS_PREFIX_OPERATOR) {
    if (ctrlblk_pred) {
      ctrlblk_pred_rt_expr (messages, idx_prev);
      return SUCCESS;
    }
  }

  if (!IS_C_ASSIGNMENT_OP (M_TOK(messages[idx_prev])))
    return ERROR;

  if ((idx_prev_2 = prevlangmsg (messages, idx_prev)) == ERROR)
    return ERROR;

  if (((lval_cvar = get_local_var(M_NAME(messages[idx_prev_2]))) 
	     == NULL) ||
	    ((lval_cvar = get_global_var (M_NAME(messages[idx_prev_2])))
	     == NULL)) {
    return ERROR;
  }

  max_param = method -> n_params - 1;

  for (i = 0; i < method -> n_params; i++) {

    if (str_eq (method -> params[i] -> name, M_NAME(messages[idx]))) {

      if ((idx_next = nextlangmsg (messages, idx)) != ERROR) {

	if (METHOD_ARG_TERM_MSG_TYPE (messages[idx_next])) {
	  format_method_arg_accessor (max_param - i,
				      M_NAME(messages[idx]),
				      method -> varargs, exprbuf);
	  /* Can't use overlapping  buffers. */
	  strcpy (messages[idx] -> name, exprbuf);
	  messages[idx] -> tokentype = RESULT;
	  return SUCCESS;
	} else {

	  if ((idx_next_2 = nextlangmsg (messages, idx_next)) != ERROR) {

	    if (str_eq (M_NAME(messages[idx_next]), "value") &&
		METHOD_ARG_TERM_MSG_TYPE (messages[idx_next_2])) {
	      strcatx (messages[idx] -> name, 
		       METHOD_ARG_VALUE_ACCESSOR_FN, " (",
		       ascii[max_param - i], ")", NULL);
	      _MPUTCHAR (messages[idx_next], ' ');
	      messages[idx_next] -> tokentype = RESULT;

	      return SUCCESS;
	    }
	  
	  }
	
	}
      
      }
    }
  }

  stack_end = get_stack_top (messages);

  for (i = idx, n_subs = 0, n_parens = 0, expr_end = idx; 
       i >= stack_end; i--) {

    if (METHOD_ARG_TERM_MSG_TYPE (messages[i]) && !n_subs && !n_parens) {
      expr_end = i + 1;
      goto e_e;
    }
    switch (messages[i] -> tokentype)
      {
      case ARRAYOPEN:
	++n_subs;
	break;
      case ARRAYCLOSE:
	--n_subs;
	break;
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	break;
      }

  }

 e_e:

  toks2str (messages, idx, expr_end, tokenbuf);
  fmt_eval_expr_str (tokenbuf, messages[idx] -> name);
#ifdef SYMBOL_SIZE_CHECK
  check_symbol_size (messages[idx] -> name);
#endif		  
  messages[idx] -> tokentype = RESULT;

  for (i = idx - 1; i >= expr_end; i--) {
    messages[i] -> name[0] = ' ', messages[i] -> name[1] = '\0';
    messages[i] -> tokentype = RESULT;
  }

  return SUCCESS;
}

int expr_contains_method_msg (MESSAGE_STACK messages, int start_idx,
			      int end_idx) {
  int i;
  for (i = start_idx; i >= end_idx; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    if ((M_TOK(messages[i]) != LABEL) &&
	(!METHOD_ARG_TERM_MSG_TYPE(messages[i])))
      return TRUE;

    /* Also check for name shadowing. */
    if (is_method_name (M_NAME(messages[i])) &&
	!(messages[i] -> attrs & OBJ_IS_INSTANCE_VAR) &&
	!(messages[i] -> attrs & OBJ_IS_CLASS_VAR))
      return TRUE;
  }
  return FALSE;
}

/* 
 *  If we don't already have a return class, try to adjust the
 *  return to a printf format if any.  Otherwise, format as normal.
 */
char *fmt_rt_return_2 (MESSAGE_STACK messages, int rcvr_idx,
		       int expr_end, char *expr, char *expr_class) {
  static char expr_out[MAXMSG];
  if ((messages[rcvr_idx] -> attrs & TOK_IS_PRINTF_ARG) &&
      str_eq (expr_class, "Any")) {
    return fmt_printf_fmt_arg (messages, rcvr_idx, stack_start (messages),
			       expr, expr_out);
  } else {
    return fmt_rt_return (expr, expr_class,  TRUE, expr_out);
  }
}

static char *collect_fmt_instvar_expr (MESSAGE_STACK messages,
				       int start_idx,
				       int end_idx,
				       char *buf) {
  char exprbuf[MAXMSG];
  int i;

  toks2str (messages, start_idx, end_idx, exprbuf);
  fmt_eval_expr_str (exprbuf, buf);
  for (i = start_idx; i >= end_idx; i--) {
    messages[i]->attrs |= TOK_IS_RT_EXPR;
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }

  return buf;

}

/*
 *  This is where we finally format an expression that has an
 *  instancevar lazily marked as a METHODMSGLABEL
 */
char *fmt_instancevar_expr_a (MESSAGE_STACK messages, int rcvr_ptr,
			      int first_instancevar_ptr, int *expr_end) {
  static char expr_buf[MAXMSG];
  int stack_end_idx;
  int i;
  int n_proto_params;
  int method_idx = rcvr_ptr;
  int sender_idx;
  int arglist_start, arglist_end;
  METHOD *m = NULL;
  OBJECT *instancevar_obj, *instancevar_class;
  MESSAGE *m_tok;
  
  stack_end_idx = get_stack_top (messages);

  if (!IS_OBJECT (messages[first_instancevar_ptr] -> obj)) {
    warning (messages[first_instancevar_ptr],
	     "Unknown instance variable \"%s,\" class defaulting to "
	     "Integer.");
    instancevar_class = get_class_object (INTEGER_CLASSNAME);
    sender_idx = first_instancevar_ptr;
  } else {
    instancevar_obj = 
      (IS_OBJECT(messages[rcvr_ptr] -> obj -> __o_p_obj) ?
       messages[rcvr_ptr] -> obj -> __o_p_obj :
       messages[rcvr_ptr] -> obj);
    sender_idx = get_instance_variable_series_term_idx 
      (instancevar_obj, 
       messages[first_instancevar_ptr],
       first_instancevar_ptr, stack_end_idx);
    instancevar_class = 
      (IS_OBJECT(messages[first_instancevar_ptr] -> obj -> instancevars) ?
       messages[first_instancevar_ptr] -> obj -> instancevars -> __o_class :
       messages[first_instancevar_ptr] -> obj -> __o_class);
  }

  for (i = sender_idx - 1; i > stack_end_idx; i--) {
    if (M_ISSPACE(messages[i]))
      continue;

    m_tok = messages[i];

    if ((m = get_instance_method (m_tok, instancevar_class, 
				  M_NAME(m_tok), ANY_ARGS, FALSE))
	!= NULL) {
      if (is_method_proto_max_params (instancevar_class -> __o_name,
				      m -> selector)) {
	if ((n_proto_params = get_proto_params_ptr ()) > 0) {
	  m = get_instance_method (m_tok, instancevar_class, 
				   M_NAME(m_tok), 
				   get_nth_proto_param (0), 
				   TRUE);
	}
      }
      method_idx = i;
    }
    goto have_instvar_method;
  }

 have_instvar_method:

  if (m == NULL) {
    collect_fmt_instvar_expr (messages, rcvr_ptr, sender_idx, expr_buf);
    *expr_end = sender_idx;
    return expr_buf;
  }

  if ((arglist_start = nextlangmsg (messages, method_idx)) == ERROR) {
    collect_fmt_instvar_expr (messages, rcvr_ptr, method_idx, expr_buf);
    *expr_end = method_idx;
    return expr_buf;
  } else if (m -> n_params == 0) {
    collect_fmt_instvar_expr (messages, rcvr_ptr, method_idx, expr_buf);
    *expr_end = method_idx;
    return expr_buf;
  }

  if ((arglist_end = method_arglist_limit_2 (messages, method_idx,
					     arglist_start,
					     m -> n_params,
					     m -> varargs)) ==
      ERROR) {
    collect_fmt_instvar_expr (messages, rcvr_ptr, arglist_start, expr_buf);
    *expr_end = arglist_start;
    return expr_buf;
  } else {
    collect_fmt_instvar_expr (messages, rcvr_ptr, arglist_end, expr_buf);
    *expr_end = arglist_end;
    return expr_buf;
  }

  return NULL;
}

void handle_self_conditional_fmt_arg (MSINFO *ms) {
  char expr_ptr[MAXMSG], expr_buf_out[MAXMSG],
    trans_expr_buf_out[MAXMSG];
  int arg_end_idx, i;

  arg_end_idx = find_expression_limit (ms);

  toks2str (ms -> messages, ms -> tok, arg_end_idx, expr_ptr);
  fmt_eval_expr_str (expr_ptr, expr_buf_out);

  fileout (fmt_printf_fmt_arg_ms
	   (ms, expr_buf_out, trans_expr_buf_out), 0, ms -> tok);
  for (i = ms -> tok; i >= arg_end_idx; --i) {
    ++ms -> messages[i] -> evaled;
    ++ms -> messages[i] -> output;
  }
}

int rt_fn_arg_cond_expr (MSINFO *ms) {
  int pred_start_idx, pred_end_idx, expr_end_idx, fn_idx = -1, i;
  int n_parens;
  char expr_out[MAXMSG];
  if (obj_expr_is_arg_ms (ms, &fn_idx) < 0)
    return FALSE;
  /* obj_expr_is_arg can return true for a if or while expresion, so
     check */
  if ((fn_idx < 0) || !get_function (M_NAME(ms -> messages[fn_idx])))
    return FALSE;
  if ((pred_end_idx = prevlangmsg (ms -> messages, ms -> tok)) != ERROR) {
    if (M_TOK(ms -> messages[pred_end_idx]) == CLOSEPAREN) {
      pred_start_idx = match_paren_rev (ms -> messages, pred_end_idx,
					ms -> stack_start);
    } else {
      n_parens = 0;
      for (i = pred_end_idx; i <= fn_idx; i++) {
	if (M_TOK(ms -> messages[i]) == ARGSEPARATOR) {
	  break;
	} else if (M_TOK(ms -> messages[i]) == CLOSEPAREN) {
	  ++n_parens;
	} else if (M_TOK(ms -> messages[i]) == OPENPAREN) {
	  --n_parens;
	  if (n_parens < 0) {
	    break;
	  }
	}
	pred_start_idx = i;
      }
    }
  }

  for (i = pred_start_idx; i >= pred_end_idx; i--) {
    if (M_TOK(ms -> messages[i]) == LABEL) {
      if (get_local_object (M_NAME(ms -> messages[i]), NULL) ||
	  get_global_object (M_NAME(ms -> messages[i]), NULL) ||
	  is_method_parameter (ms -> messages, i)) {
	goto cond_has_objects;
      }
    }
  }

  for (i = ms -> tok; i > ms -> stack_ptr; --i) {
    if ((M_TOK(ms -> messages[i]) == ARGSEPARATOR) ||
	(M_TOK(ms -> messages[i]) == SEMICOLON)) {
      break;
    }
    if (M_TOK(ms -> messages[i]) == LABEL) {
      if (get_local_object (M_NAME(ms -> messages[i]), NULL) ||
	  get_global_object (M_NAME(ms -> messages[i]), NULL) ||
	  is_method_parameter (ms -> messages, i)) {
	goto cond_has_objects;
      }
    }
  }
  return FALSE;

 cond_has_objects:
  fn_arg_conditional = true;
  fn_cond_arg_fn_label_idx = fn_idx;
  rt_expr (ms -> messages, pred_start_idx, &expr_end_idx, expr_out);
  fn_arg_conditional = false;
  fn_cond_arg_fn_label_idx = -1;

  return TRUE;
}
