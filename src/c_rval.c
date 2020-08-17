/*
  This file is part of Ctalk.
  Copyright © 2015-2019 Robert Kiesling, rk3314042@gmail.com.
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

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;

extern bool argblk;                /* Declared in argblk.c. */

extern bool ctrlblk_pred,          /* Declared in control.c. */
  ctrlblk_blk,
  ctrlblk_else_blk;

extern SUBSCRIPT subscripts[MAXARGS]; /* declared in subscr.c. */
extern int subscript_ptr;

extern char *ascii[8193];             /* from intascii.h */

static int tmp_fn_block_id = 0;

extern HASHTAB defined_instancevars; /* declared in primitives.c. */

/* This function does not return on an error. */
static void check_extra_fn_expr_tokens (MESSAGE_STACK messages, 
					int fn_label_idx) {
  int arglist_start, arglist_end;
  int extra_tok_end;
  MESSAGE *m;
  int i;
  
  if ((arglist_start = nextlangmsg (messages, fn_label_idx)) != ERROR) {
    if (M_TOK(messages[arglist_start]) == OPENPAREN) {
      if ((arglist_end = match_paren (messages, arglist_start, 
				      get_stack_top (messages))) != ERROR) {

	extra_tok_end = -1;
	if ((extra_tok_end = scanforward (messages, 
					  arglist_end,
					  get_stack_top (messages),
					  SEMICOLON)) == ERROR) {
	  if ((extra_tok_end = scanforward (messages, arglist_end,
					    get_stack_top (messages),
					    CLOSEBLOCK)) == ERROR) {
	    if ((extra_tok_end = scanforward (messages, arglist_end,
					      get_stack_top (messages),
					      ARRAYCLOSE)) == ERROR) {
	      return;
	    }
	  }
	}

	for (i = arglist_end - 1; i > extra_tok_end; i--) {

	  m = messages[i];
	  if (!IS_MESSAGE(m))
	    break;

	  switch (M_TOK(m)) 
	    {
	    case LABEL:
	      if (m -> attrs & TOK_SELF || m -> attrs & TOK_SUPER ||
		  get_object (M_NAME(m), NULL))
		rval_fn_extra_tok_warning (messages,
					   fn_label_idx,
					   extra_tok_end);
	      break;
	    }
	} 
      }
    }
  }
}

static char *make_tmp_fn_block_name (char *buf_out) {
  if (interpreter_pass == method_pass) {
    strcatx (buf_out, TMP_FN_BLK_PFX, new_methods[new_method_ptr+1] -> method -> selector,
	     "_",
	     ascii[tmp_fn_block_id++],
	     NULL);
  } else {
    strcatx (buf_out, TMP_FN_BLK_PFX, get_fn_name (), 
	     "_",
	     ascii[tmp_fn_block_id++], NULL);
  }
  return buf_out;
}

/*
 *  Returns true if we have an expression like: 
 *  self = <c_function>();
 */
int is_self_as_fn_lvalue (MESSAGE *m_sender,
			       int self_idx,
			       int message_idx,
			       int stack_end) {
  int next_idx;
  CFUNC *c_fn;

  if (M_TOK(message_stack_at(message_idx)) == EQ) {
    if ((next_idx = nextlangmsg (message_stack (), message_idx)) 
	!= ERROR) {
      if (M_TOK(message_stack_at (next_idx)) == LABEL) {
	if ((c_fn = 
	     get_function (M_NAME(message_stack_at (next_idx))))
	    != NULL) {
	  check_extra_fn_expr_tokens (message_stack (), next_idx);
	  return TRUE;
	} else {
	  int next_idx_2;
	  if ((next_idx_2 = nextlangmsg (message_stack (),
					 next_idx)) != ERROR) {
	    if (M_TOK(message_stack_at (next_idx_2)) == OPENPAREN) {
	      warning (message_stack_at(next_idx), 
		       "Undefined C function, \"%s.\"",
		       M_NAME(message_stack_at(next_idx)));
	    }
	  }
	  return FALSE;
	}
      }
    } else {
      return FALSE;
    }
  } else {
    return FALSE;
  }

  return FALSE;
}

/*
 *  Returns true if we have an expression like: 
 *  self <instance_var>+ = <c_function>();
 */
int is_self_expr_as_fn_lvalue (MESSAGE *m_sender,
			       int self_idx,
			       int message_idx,
			       int stack_end) {
  OBJECT *first_instance_var, *next_instance_var;
  OBJECT *prev_object_l;
  MESSAGE *m_self;
  MESSAGE *next_message;
  MESSAGE *sender_rcvr_msg;
  int next_idx = message_idx;  /* avoid a warning */
  int i;
  PARSER *p;
  FRAME *f;
  CFUNC *c_fn;
  bool have_object;
  MESSAGE_STACK messages = message_stack ();
  if ((first_instance_var = 
       get_instance_variable (M_NAME(m_sender), 
			      rcvr_class_obj -> __o_name, FALSE)) != NULL) {

    m_self = messages[self_idx];

    sender_rcvr_msg = m_self;

    m_sender -> obj = first_instance_var;
    m_sender -> attrs |= OBJ_IS_INSTANCE_VAR;
    m_sender -> receiver_msg = m_self;
    m_sender -> receiver_obj = instantiate_self_object ();

    next_idx = message_idx;
    prev_object_l = first_instance_var;

    while ((next_idx = nextlangmsg (messages, next_idx)) != ERROR) {

      next_message = messages[next_idx];

      if ((next_instance_var = 
	   get_instance_variable 
	   (M_NAME(next_message),
	    (prev_object_l -> instancevars ? 
	     prev_object_l -> instancevars -> __o_classname : 
	     prev_object_l -> __o_classname),
	    FALSE)) != NULL) {

	next_message -> obj = next_instance_var;
	next_message -> attrs |= OBJ_IS_INSTANCE_VAR;
	next_message -> receiver_obj = prev_object_l;
	next_message -> receiver_msg = sender_rcvr_msg;

	prev_object_l = next_instance_var;
      } else if (_hash_get (defined_instancevars,
			    M_NAME(next_message))) {
	next_message -> attrs |= OBJ_IS_INSTANCE_VAR;
	next_message -> receiver_obj = prev_object_l;
	next_message -> receiver_msg = sender_rcvr_msg;

      } else {

	if ((next_instance_var = 
	     get_class_variable 
	     (M_NAME(next_message),
	      (prev_object_l -> instancevars ? 
	       prev_object_l -> instancevars -> __o_classname : 
	       prev_object_l -> __o_classname),
	      FALSE)) != NULL) {

	  next_message -> obj = next_instance_var;
	  next_message -> attrs |= OBJ_IS_CLASS_VAR;
	  next_message -> receiver_obj = prev_object_l;
	  next_message -> receiver_msg = sender_rcvr_msg;
	  
	  prev_object_l = next_instance_var;
	} else if (str_eq (M_NAME(messages[next_idx]), "super")) { /***/
	  return FALSE;
	} else {
	  
	  if (M_TOK(messages[next_idx]) == EQ) {
	    if ((next_idx = nextlangmsg (messages, next_idx)) 
		!= ERROR) {
	      if (M_TOK(messages[next_idx]) == LABEL) { 
		if ((c_fn = 
		     get_function (M_NAME(messages[next_idx])))
		    != NULL) {
		  p = pop_parser ();
		  push_parser (p);
		  f = frame_at (p -> frame);
		  have_object = false;
		  for (i = next_idx; i >= f -> message_frame_top; i--) {
		    if (messages[i] -> attrs & TOK_SELF ||
			messages[i] -> attrs & TOK_SUPER ||
			get_object (messages[i] -> name, NULL) ||
			is_method_parameter (messages, i)) {
		      have_object = true;
		      break;
		    }
		  }
		  if (have_object) {
		    check_extra_fn_expr_tokens (messages, next_idx);
		    return TRUE;
		  } else {
		    return FALSE;
		  }
		} else {
		  int next_idx_2;
		  if ((next_idx_2 = nextlangmsg (messages,
						 next_idx)) != ERROR) {
		    if (M_TOK(messages[next_idx_2]) == OPENPAREN) {
		      warning (messages[next_idx], 
			       "Undefined C function, \"%s.\"",
			       M_NAME(messages[next_idx]));
		    }
		  }
		  return FALSE;
		}
	      } else {
		return FALSE;
	      }
	    } else {
	      return FALSE;
	    }
	  } else {
	    if (M_TOK(messages[next_idx]) == LABEL) {
	      self_instvar_expr_unknown_label (message_stack (),
					       self_idx, next_idx);
	      /* doesn't return if error */
	    }
	    return FALSE;
	  }

	}
      }
    }
  }

  if (M_TOK(messages[next_idx]) == EQ) {
    if ((next_idx = nextlangmsg (messages, next_idx)) 
	!= ERROR) {
      if (M_TOK(messages[next_idx]) == LABEL) {
	if ((c_fn = 
	     get_function (M_NAME(messages[next_idx])))
	    != NULL) {
	  check_extra_fn_expr_tokens (messages, next_idx);
	  return TRUE;
	} else {
	  int next_idx_2;
	  if ((next_idx_2 = nextlangmsg (messages,
					 next_idx)) != ERROR) {
	    if (M_TOK(messages[next_idx_2]) == OPENPAREN) {
	      warning (messages[next_idx], 
		       "Undefined C function, \"%s.\"",
		       M_NAME(messages[next_idx]));
	    }
	  }
	  return FALSE;
	}
      }
    } else {
      return FALSE;
    }
  } else {
    return FALSE;
  }

  return FALSE;
}

/*
 *  Returns true if we have an expression like: 
 *  <obj> <instance_var>+ = <c_expr>;
 *
 *  A <c_expr> is anything that doesn't contain
 *  "self," "super," or an object.  
 *
 *  We only need to evaluate the expression as plain C if it
 *  contains a C function, which the expression parser can't
 *  exec directly at run time.
 */

static bool check_rval_C_expr (MESSAGE_STACK messages,
			  int start_idx, int end_idx) {
  int expr_idx;
  MESSAGE *expr_msg;
  bool retval = false;
  bool have_c_fn = false;
  bool have_object = false;
  
  for (expr_idx = start_idx; expr_idx >= end_idx; --expr_idx) {
    expr_msg = message_stack_at (expr_idx);
    if (!IS_MESSAGE(expr_msg))
      break;
    switch (M_TOK(expr_msg)) 
      {
      case LABEL:
	if (expr_msg -> attrs & TOK_SELF)
	  have_object = true;
	if (expr_msg -> attrs & TOK_SUPER)
	  have_object = true;
	if (get_object (M_NAME(expr_msg), NULL))
	  have_object = true;
	if (get_function (M_NAME(expr_msg)))
	  have_c_fn = true;
	break;
      case SEMICOLON:
      case CLOSEBLOCK:
      case ARRAYCLOSE:
	retval = true;
	goto check_rval_C_expr_done;
	break;
      }
  } 

 check_rval_C_expr_done:

  if (retval == true && have_object == false) {
    return have_c_fn;
  }  else {
    return !have_object;
  }
}

int is_self_expr_as_C_expr_lvalue (MESSAGE *m_sender,
			       int self_idx,
			       int message_idx,
			       int stack_end) {
  OBJECT *first_instance_var, *next_instance_var;
  OBJECT *prev_object_l;
  MESSAGE *m_self;
  MESSAGE *next_message;
  MESSAGE *sender_rcvr_msg;
  int next_idx = message_idx;  /* avoid a warning */
  if ((first_instance_var = 
       get_instance_variable (M_NAME(m_sender), 
			      rcvr_class_obj -> __o_name, FALSE)) != NULL) {

    m_self = message_stack_at (self_idx);

    sender_rcvr_msg = m_self;

    m_sender -> obj = first_instance_var;
    m_sender -> attrs |= OBJ_IS_INSTANCE_VAR;
    m_sender -> receiver_msg = m_self;
    m_sender -> receiver_obj = instantiate_self_object ();

    next_idx = message_idx;
    prev_object_l = first_instance_var;

    while ((next_idx = nextlangmsg (message_stack (), next_idx)) != ERROR) {

      next_message = message_stack_at (next_idx);

      if ((next_instance_var = 
	   get_instance_variable 
	   (M_NAME(next_message),
	    (prev_object_l -> instancevars ? 
	     prev_object_l -> instancevars -> __o_classname : 
	     prev_object_l -> __o_classname),
	    FALSE)) != NULL) {

	next_message -> obj = next_instance_var;
	next_message -> attrs |= OBJ_IS_INSTANCE_VAR;
	next_message -> receiver_obj = prev_object_l;
	next_message -> receiver_msg = sender_rcvr_msg;

	prev_object_l = next_instance_var;

      } else {

	if ((next_instance_var = 
	     get_class_variable 
	     (M_NAME(next_message),
	      (prev_object_l -> instancevars ? 
	       prev_object_l -> instancevars -> __o_classname : 
	       prev_object_l -> __o_classname),
	      FALSE)) != NULL) {

	  next_message -> obj = next_instance_var;
	  next_message -> attrs |= OBJ_IS_CLASS_VAR;
	  next_message -> receiver_obj = prev_object_l;
	  next_message -> receiver_msg = sender_rcvr_msg;
	  
	  prev_object_l = next_instance_var;
	} else {
	  
	  if (IS_C_ASSIGNMENT_OP(M_TOK(message_stack_at(next_idx)))) {
	    if ((next_idx = nextlangmsg (message_stack (), next_idx)) 
		!= ERROR) {
	      if (check_rval_C_expr (message_stack (), next_idx,
				     stack_end)) {
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
      }
    }
  }

  if (M_TOK(message_stack_at(next_idx)) == EQ) {
    if ((next_idx = nextlangmsg (message_stack (), next_idx)) 
	!= ERROR) {
      if (check_rval_C_expr (message_stack (), next_idx,
			     stack_end)) {
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

  return FALSE;
}

static char *handle_template_CVAR_label (MESSAGE_STACK messages,
					 int idx, char *buf_out) {
  CVAR *c;
  if (((c = get_local_var (M_NAME(messages[idx]))) != NULL) ||
      ((c = get_global_var_not_shadowed (M_NAME(messages[idx]))) != NULL)) {
    fmt_register_c_method_arg_call (c, M_NAME(messages[idx]),
				    LOCAL_VAR, buf_out);
    return buf_out;
  }
  return NULL;
}

static char *handle_template_subscript_expr (MESSAGE_STACK messages,
					     int basename_idx,
					     int end_idx,
					     char *buf_out,
					     int *expr_end_idx_out) {
  CVAR *c;
  SUBSCRIPT t_subs[MAXARGS];
  int n_t_subs;
  char label[MAXMSG];
  /* This works okay as long as there are no objects in the
     subscript, which should be handled elsewhere. */
  if (((c = get_local_var (M_NAME(messages[basename_idx]))) != NULL) ||
      ((c = get_global_var_not_shadowed (M_NAME(messages[basename_idx]))) 
       != NULL)) {
    parse_subscript_expr (messages, basename_idx, end_idx, t_subs, &n_t_subs);
    toks2str (messages, basename_idx, t_subs[n_t_subs-1].block_end_idx,
	      label);
    fmt_register_c_method_arg_call (c, label, LOCAL_VAR, buf_out);
    *expr_end_idx_out = t_subs[n_t_subs-1].block_end_idx;
    return buf_out;
  }
  return NULL;
}

static void register_template_arg_CVARs (MESSAGE_STACK messages,
					 int start_idx, int end_idx,
					 char *buf) {
  int i, lookahead, agg_end_idx;
  char expr_buf[MAXMSG], *e;

  buf[0] = 0;

  for (i = start_idx; i >= end_idx; i--) {
    if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
      if (M_TOK(messages[lookahead]) == ARRAYOPEN) {
	/* This works okay as long as there are no objects in the
	   subscript. */
	if ((e = handle_template_subscript_expr (messages, i, end_idx,
						 expr_buf, &agg_end_idx))
	    != NULL) {
	  strcatx2 (buf, e, NULL);
	  i = agg_end_idx;
	}
      } else { /* if (M_TOK(messages[lookahead]) == ARRAYOPEN) ...  */
	if (M_TOK(messages[i]) == LABEL) {
	  if ((e = handle_template_CVAR_label (messages, i, expr_buf)) != NULL) {
	    strcatx2 (buf, e, NULL);
	  }
	}
      } /* if (M_TOK(messages[lookahead]) == ARRAYOPEN) ...  */
    } else { /* if ((lookahead = nextlangmsg (messages, i)) != ERROR) ... */
      if (M_TOK(messages[i]) == LABEL) {
	if ((e = handle_template_CVAR_label (messages, i, expr_buf)) != NULL) {
	  strcatx2 (buf, e, NULL);
	}
      }
    } /* if ((lookahead = nextlangmsg (messages, i)) != ERROR) ... */
  }
}

/*
 *  Formats an expression like 
 *
 *   self <instance_var>+ = <c_func>(<args>) <expr>*;
 *
 * into
 *
 *  {
 *    <cfunc_return_type> <tmp_lval_name> = <c_func>(<args>) <expr>*;
 *    __ctalk_register_c_method_arg (... <tmp_lval_name>);
 *    __ctalkEvalExpr ("self <instance_var>+ = <tmp_lval_name>");
 *  }
 *
 *  NOTE: For the present, if there are *any* characters besides
 *  a semicolon after the function args, then we will use a plain
 *  C function - Ctalk won't try to fold the entire expression in
 *  if we use a template.
 *
 *  Also - there can't be any enclosing parentheses (i.e., before
 *  the start of the function name), for Ctalk to recognize the
 *  expression as a template.
 */

#define TMP_LVAL_INT 0
#define TMP_LVAL_INT_PTR 1
#define TMP_LVAL_INT_PTR_PTR 2
#define TMP_LVAL_OBJECT_PTR 3
#define TMP_LVAL_CHAR_PTR 4
#define TMP_LVAL_CHAR 5
#define TMP_LVAL_VOID_PTR 6
#define TMP_LVAL_FLOAT 7
#define TMP_LVAL_DOUBLE 8
#define TMP_LVAL_ARRAY 9
#define TMP_LVAL_LONG_LONG 10

static struct {
  char decl_str[0x20];
} tmp_lval_decl_tab[] = {
  {"int"},
  {"int *"},
  {"int **"},
  {"OBJECT *"},
  {"char *"},
  {"char"},
  {"void *"},
  {"float"},
  {"double"},
  {"char **"},
  {"long long int"}
};

CVAR tmp_lval_cvar_tab[] = {
  {CVAR_SIG, "", "int", "", "", "", "", "", "", 0, 0, 0, CVAR_TYPE_INT,
   false, 0,},
  {CVAR_SIG, "", "int", "", "", "", "", "", "", 1, 0, 0, CVAR_TYPE_INT,
   false, 0,},
  {CVAR_SIG, "", "int", "", "", "", "", "", "", 2, 0, 0, CVAR_TYPE_INT,
   false, 0,},
  {CVAR_SIG, "", "OBJECT", "", "", "", "", "", "", 1, 0, 0, CVAR_TYPE_STRUCT,
   false, 0,},
  {CVAR_SIG, "", "char", "", "", "", "", "", "", 1, 0, 0, CVAR_TYPE_CHAR, 
   false, 0,},
  {CVAR_SIG, "", "char", "", "", "", "", "", "", 0, 0, 0, CVAR_TYPE_CHAR, 
   false, 0,},
  {CVAR_SIG, "", "void", "", "", "", "", "", "", 1, 0, 0, CVAR_TYPE_VOID, 
   false, 0,},
  {CVAR_SIG, "", "float", "", "", "", "", "", "", 0, 0, 0, CVAR_TYPE_FLOAT, 
   false, 0,},
  {CVAR_SIG, "", "double", "", "", "", "", "", "", 0, 0, 0, CVAR_TYPE_DOUBLE, 
   false, 0,},
  {CVAR_SIG, "", "char", "", "", "", "", "", "", 2, 0, 0, CVAR_TYPE_CHAR, 
   false, 0,},
  {CVAR_SIG, "", "int", "long", "long", "", "", "", "", 0, 0, 0, CVAR_TYPE_LONGLONG, 
   false, 0,}
};

static struct {
  int return_type_attrs, 
    return_derefs,
    tmp_fn_block_idx;
} fn_return_tab[] = {
  {CVAR_TYPE_INT, 0, TMP_LVAL_INT},
  {CVAR_TYPE_INT, 1, TMP_LVAL_INT_PTR},
  {CVAR_TYPE_INT, 2, TMP_LVAL_INT_PTR_PTR},
  {CVAR_TYPE_OBJECT, 1, TMP_LVAL_OBJECT_PTR},
  {CVAR_TYPE_CHAR, 0, TMP_LVAL_CHAR},
  {CVAR_TYPE_FLOAT, 0, TMP_LVAL_FLOAT},
  {CVAR_TYPE_DOUBLE, 0, TMP_LVAL_DOUBLE},
  {CVAR_TYPE_CHAR, 1, TMP_LVAL_CHAR_PTR},
  {CVAR_TYPE_CHAR, 2, TMP_LVAL_ARRAY},
  {CVAR_TYPE_LONGLONG, 0, TMP_LVAL_LONG_LONG},
  {CVAR_TYPE_VOID, 1, TMP_LVAL_VOID_PTR},
  {CVAR_TYPE_LONG, 0, TMP_LVAL_LONG_LONG},
  {-1, -1, -1}
};

#define RVALTEMPLATEMAX 0xffff

static char object_class_str[] = "Object";

#define SELF_LVAL_FN_EXPR_TEMPLATE "\n\
       {\n\
          %s %s = %s;\n\
          %s\n\
          %s;\n\
       }\n"

int format_self_lval_fn_expr (MESSAGE_STACK messages, int self_tok_ptr) {
  char tmp_lval_name[MAXLABEL];
  char *fn_return_class;
  int fn_expr_start_idx, fn_arg_start_idx, fn_arg_end_idx, fn_term_idx,
    last_instancevar_idx, fn_term_idx_2;
  char fn_buf[MAXMSG], fn_expr_buf[MAXMSG] = "",
    fn_expr_buf_2[MAXMSG * 3] = "";
  char rt_expr_buf[MAXMSG];
  char expr_buf_tmp[MAXMSG];
  char outbuf[RVALTEMPLATEMAX];
  char tmpl_cvar_register_buf[MAXMSG],
    trailing_expr[MAXMSG];
  char *c99name, *cfn_buf;
  CFUNC *c_fn;
  int tmp_lval_tab_idx;
  int i;
  METHOD *arg_method;
  OBJECT *arg_object;
  bool have_complex_expr = false;

  if ((interpreter_pass != method_pass) && argblk) {
    warning (messages[self_tok_ptr], "\"self,\" used outside of a method.");
    return ERROR;
  }

  if ((fn_expr_start_idx = scanforward (messages, 
					self_tok_ptr,
					get_stack_top (messages),
					EQ)) == ERROR)
    return ERROR;
					
  last_instancevar_idx = prevlangmsg (messages, fn_expr_start_idx);

  if ((fn_expr_start_idx = nextlangmsg (messages,
					fn_expr_start_idx))
      != ERROR) {
    
    if ((c_fn = get_function (M_NAME(messages[fn_expr_start_idx]))) 
	== NULL) {
      /* Check for a user template before simply returning an error. */
      if (!user_template_name (M_NAME(messages[fn_expr_start_idx]))) {
	return ERROR;
      }
    }

    /* Also check this... */
    /* If we have *any* expression after a template, use a plain
       C expression instead. */
    fn_arg_start_idx = nextlangmsg (messages, fn_expr_start_idx);
    fn_arg_end_idx = match_paren (messages, fn_arg_start_idx,
				  get_stack_top (messages));
    fn_term_idx = nextlangmsg (messages, fn_arg_end_idx);
    if (M_TOK(messages[fn_term_idx]) == SEMICOLON)
      have_complex_expr = false;
    else
      have_complex_expr = true;

    if (((c99name = template_name (M_NAME(messages[fn_expr_start_idx])))
	!= NULL) && 
	!have_complex_expr) {
      char exprbuf_1[MAXMSG];
      cfn_buf = clib_fn_rt_expr (messages, fn_expr_start_idx);
      fn_arg_start_idx = nextlangmsg (messages, fn_expr_start_idx);
      fn_arg_end_idx = match_paren (messages, fn_arg_start_idx,
				    get_stack_top (messages));
      toks2str (messages, fn_arg_start_idx, fn_arg_end_idx, expr_buf_tmp);
      memset (tmpl_cvar_register_buf, 0, MAXMSG);
      register_template_arg_CVARs (messages, fn_arg_start_idx,
				   fn_arg_end_idx,
				   tmpl_cvar_register_buf);
      
      strcatx (exprbuf_1, cfn_buf, " ", expr_buf_tmp, NULL);
      fn_return_class = object_class_str;
      arg_object = create_object (CFUNCTION_CLASSNAME, 
				  fmt_eval_expr_str (exprbuf_1,
						     expr_buf_tmp));

      fn_term_idx_2 = -1;
      if ((fn_term_idx = nextlangmsg (messages, fn_arg_end_idx)) != ERROR) {
	if (M_TOK(messages[fn_term_idx]) != SEMICOLON) {
	  /* TODO - We haven't seen a case like this yet -
	     self = <template_fn_expr> <c_expr> ; */
	  fn_term_idx_2 = scanforward (messages, fn_term_idx,
				       get_stack_top (messages),
				       SEMICOLON);
	}
      }
    } else { /* if ((c99name ... */

      fn_return_class = 
	basic_class_from_cfunc (messages[self_tok_ptr], c_fn, 0);

      if ((fn_arg_start_idx = nextlangmsg (messages, fn_expr_start_idx))
	  == ERROR)
	return ERROR;
      if (M_TOK(messages[fn_arg_start_idx]) != OPENPAREN)
	return ERROR;
      if ((fn_arg_end_idx = match_paren (messages, fn_arg_start_idx,
					 get_stack_top (messages)))
	  == ERROR) 
	return ERROR;

      fn_term_idx_2 = -1;
      if ((fn_term_idx = nextlangmsg (messages, fn_arg_end_idx)) != ERROR) {
	if (M_TOK(messages[fn_term_idx]) == SEMICOLON) {
	  toks2str (messages, fn_expr_start_idx, fn_term_idx, fn_buf);
	} else {
	  fn_term_idx_2 = scanforward (messages, fn_term_idx,
				       get_stack_top (messages),
				       SEMICOLON);
	  toks2str (messages, fn_expr_start_idx, fn_term_idx_2, fn_buf);
	}
      } else {
	toks2str (messages, fn_expr_start_idx, fn_arg_end_idx, fn_buf);
      }

      /* the last instance variable in the receiver has an object
	 assigned to it before it gets to this fn. Note that '='
	 should be sufficient here, regardless of whether the
	 actual operator is +=, -=, |=, etc. */
      arg_method = get_instance_method 
	(messages[last_instancevar_idx],
	 (messages[last_instancevar_idx] -> obj -> instancevars ? 
	  messages[last_instancevar_idx] -> obj -> instancevars :
	  messages[last_instancevar_idx] -> obj),
	 "=", ANY_ARGS, TRUE);

      arg_object = fn_arg_expression (rcvr_class_obj, arg_method,
				      messages, fn_expr_start_idx);

    } /* if ((c99name ... */

    make_tmp_fn_block_name (tmp_lval_name);

    if (str_eq (fn_return_class, INTEGER_CLASSNAME)) {
      tmp_lval_tab_idx = TMP_LVAL_INT;
    } else if (str_eq (fn_return_class, OBJECT_CLASSNAME)) {
      tmp_lval_tab_idx = TMP_LVAL_OBJECT_PTR;
    } else if (str_eq (fn_return_class, STRING_CLASSNAME)) {
      tmp_lval_tab_idx = TMP_LVAL_CHAR_PTR;
    } else if (str_eq (fn_return_class, CHARACTER_CLASSNAME)) {
      tmp_lval_tab_idx = TMP_LVAL_CHAR;
    } else if (str_eq (fn_return_class, SYMBOL_CLASSNAME)) {
      tmp_lval_tab_idx = TMP_LVAL_VOID_PTR;
    } else if (str_eq (fn_return_class, FLOAT_CLASSNAME)) {
      tmp_lval_tab_idx = TMP_LVAL_FLOAT;
    } else if (str_eq (fn_return_class, ARRAY_CLASSNAME)) {
      tmp_lval_tab_idx = TMP_LVAL_ARRAY;
    } else {
      warning (messages[self_tok_ptr], 
	       "Function %s has unimplemented return class. "
	       "Return class defaulting to Integer.",
	       M_NAME(messages[fn_expr_start_idx]));
      tmp_lval_tab_idx = TMP_LVAL_INT;
    }

    if (*arg_object -> __o_name) { 
      if (c99name != NULL && !have_complex_expr) {
	strcpy (fn_expr_buf, arg_object -> __o_name);
      } else {
	strcatx (fn_expr_buf, M_NAME(messages[fn_expr_start_idx]),
		 " (", arg_object -> __o_name, ")", NULL);
      }
      if (fn_term_idx_2 != -1) {
	toks2str (messages, fn_term_idx, fn_term_idx_2, trailing_expr);
	/* strcatx doesn't handle overlapping buffers yet. */
	sprintf (fn_expr_buf_2, "%s %s", fn_expr_buf, trailing_expr);
      }
    } else {
      strcpy (fn_expr_buf, fn_buf);
    }
    toks2str (messages, self_tok_ptr, last_instancevar_idx, fn_buf);
    strcatx (rt_expr_buf, EVAL_EXPR_FN, "(\"", 
	     fn_buf, " = ", tmp_lval_name, "\");", NULL);
    /* this should still be sprintf for a while. */
    strcpy (tmp_lval_cvar_tab[tmp_lval_tab_idx].name, tmp_lval_name);
    strcat (rt_expr_buf, "\ndelete_method_arg_cvars ();\n");
    sprintf (outbuf, SELF_LVAL_FN_EXPR_TEMPLATE, 
	     tmp_lval_decl_tab[tmp_lval_tab_idx].decl_str, 
	     tmp_lval_name, /* fn_expr_buf, */
	     (*fn_expr_buf_2 ? fn_expr_buf_2 : fn_expr_buf),
	     fmt_register_c_method_arg_call 
	     (&tmp_lval_cvar_tab[tmp_lval_tab_idx],
	      tmp_lval_name, LOCAL_VAR, tmpl_cvar_register_buf),
	     rt_expr_buf);
    if (argblk)
      buffer_argblk_stmt (outbuf);
    else
      fileout (outbuf, TRUE, 0);

    if ((fn_term_idx != ERROR) && (M_TOK(messages[fn_term_idx]) == SEMICOLON)) {
      for (i = self_tok_ptr; i >= fn_term_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
    } else {
      for (i = self_tok_ptr; i >= fn_term_idx_2; i--) {    
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
    }
  }


  return SUCCESS;
}

static int rval_scanforward_assign_op (MESSAGE **messages, int start_ptr, 
				       int end_ptr) {

  MESSAGE *m;
  int i;

  for (i = start_ptr; i >= end_ptr; i--) {
    m = messages[i];
    if (!m || !IS_MESSAGE (m))
      break;
    if (IS_C_ASSIGNMENT_OP(m -> tokentype))
      return i;
  }
  return ERROR;
}

static int fn_return_tab_entry (MESSAGE *m_orig, CFUNC *c_fn) {
  int i, i_2;
  CVAR *derived_return_type;
  for (i = 0; ; ++i) {
    if ((c_fn -> return_type_attrs &
	 fn_return_tab[i].return_type_attrs) &&
	(c_fn -> return_derefs == fn_return_tab[i].return_derefs)) {
      return fn_return_tab[i].tmp_fn_block_idx;
    } else if (c_fn -> return_type_attrs == 0) {
      if ((derived_return_type = get_typedef (c_fn -> return_type))
	  != NULL) {
	for (i_2 = 0; ; ++i_2) {
	  if ((derived_return_type -> type_attrs &
	       fn_return_tab[i_2].return_type_attrs) &&
	      (c_fn -> return_derefs == fn_return_tab[i_2].return_derefs)) {
	    return fn_return_tab[i_2].tmp_fn_block_idx;
	  } else if ((derived_return_type -> attrs &
		      fn_return_tab[i_2].return_type_attrs) &&
		     (c_fn -> return_derefs ==
		      fn_return_tab[i_2].return_derefs)) {
	    /* Check the attrs as well, because the type_attrs could
	       simply be used to indicated that the var is a typedef. */
	    return fn_return_tab[i_2].tmp_fn_block_idx;
	  } else if (fn_return_tab[i_2].return_type_attrs == -1) {
	    char type_buf[MAXLABEL];
	    if (*c_fn -> qualifier2_type) {
	      strcatx (type_buf, c_fn -> qualifier2_type, " ",
		       c_fn -> qualifier_type, " ",
		       c_fn -> return_type, NULL);
	    } else if (*c_fn -> qualifier_type) {
	      strcatx (type_buf, c_fn -> qualifier_type, " ",
		       c_fn -> return_type, NULL);
	    } else if (str_eq (c_fn -> return_type, "OBJECT") &&
		       (c_fn -> return_derefs == 1)) { /***/
	      strcatx (type_buf, c_fn -> return_type, NULL);
	      return TMP_LVAL_OBJECT_PTR;
	    } else {
	      strcatx (type_buf, c_fn -> return_type, NULL);
	    }
	    warning (m_orig,
		     "Function %s (return type %s) has unimplemented "
		     "return class. "
		     "Return class defaulting to Integer.",
		     M_NAME(m_orig), type_buf);
	    return TMP_LVAL_INT;
	  }
	}
      }
    } else if (fn_return_tab[i].return_type_attrs == -1) {
      char type_buf[MAXLABEL];
      if (*c_fn -> qualifier2_type) {
	strcatx (type_buf, c_fn -> qualifier2_type, " ",
		 c_fn -> qualifier_type, " ",
		 c_fn -> return_type, NULL);
      } else if (*c_fn -> qualifier_type) {
	strcatx (type_buf, c_fn -> qualifier_type, " ",
		 c_fn -> return_type, NULL);
      } else {
	strcatx (type_buf, c_fn -> return_type, NULL);
      }
      warning (m_orig,
	       "Function %s (return type %s) has unimplemented "
	       "return class. "
	       "Return class defaulting to Integer.",
	       M_NAME(m_orig), type_buf);
      return TMP_LVAL_INT;
    }
  }
}


#define SELF_LVAL_C_EXPR_TEMPLATE "\n\
       {\n\
          %s %s = %s;\n\
          %s\n\
          %s;\n\
       }\n"
int format_self_lval_C_expr (MESSAGE_STACK messages, int self_tok_ptr) {
  char tmp_lval_name[MAXLABEL];
  char *instancevar_value_class;
  int c_op_idx, c_expr_start_idx, c_expr_end_idx, last_instancevar_idx;
  char lval_self_expr_ptr[MAXMSG], expr_buf[MAXMSG];
  char rt_expr_buf[MAXMSG], cvar_buf[MAXMSG];
  char outbuf[RVALTEMPLATEMAX];
  int i;
  int tmp_lval_tab_idx = -1;

  if ((interpreter_pass != method_pass) && !argblk) {
    warning (messages[self_tok_ptr], "\"self,\" used outside of a method.");
    return ERROR;
  }

  if ((c_op_idx = rval_scanforward_assign_op 
       (messages, 
	self_tok_ptr,
	get_stack_top (messages))) 
      == ERROR)
    return ERROR;
					
  last_instancevar_idx = prevlangmsg (messages, c_op_idx);

  if ((c_expr_start_idx = nextlangmsg (messages, c_op_idx))
      != ERROR) {
    
    c_expr_end_idx = ERROR;
    if ((c_expr_end_idx = scanforward (messages,
				       c_op_idx,
				       get_stack_top (messages),
				       SEMICOLON)) == ERROR) {
      if ((c_expr_end_idx = scanforward (messages,
					 c_op_idx,
					 get_stack_top (messages),
					 CLOSEBLOCK)) == ERROR) {
	if ((c_expr_end_idx = scanforward (messages,
					   c_op_idx,
					   get_stack_top (messages),
					   ARRAYCLOSE)) == ERROR) {
	  printf ("Warning: format_self_lval_C_expr: Could not find "
		  "expression end.\n");
	  return ERROR;
	}
      }
    }

    toks2str (message_stack (), c_expr_start_idx, c_expr_end_idx,
	      expr_buf);
    make_tmp_fn_block_name (tmp_lval_name);

    instancevar_value_class = messages[last_instancevar_idx] -> obj
      -> instancevars -> CLASSNAME;
    if (str_eq (instancevar_value_class, INTEGER_CLASSNAME)) {
      tmp_lval_tab_idx = TMP_LVAL_INT;
    } else {
      if (str_eq (instancevar_value_class, OBJECT_CLASSNAME)) {
	tmp_lval_tab_idx = TMP_LVAL_OBJECT_PTR;
      } else {
	if (str_eq (instancevar_value_class, STRING_CLASSNAME)) {
	  tmp_lval_tab_idx = TMP_LVAL_CHAR_PTR;
	} else {
	  if (str_eq (instancevar_value_class, CHARACTER_CLASSNAME)) {
	    tmp_lval_tab_idx = TMP_LVAL_CHAR;
	  } else {
	    if (str_eq (instancevar_value_class, SYMBOL_CLASSNAME)) {
	      tmp_lval_tab_idx = TMP_LVAL_VOID_PTR;
	    } else {
	      if (str_eq (instancevar_value_class, FLOAT_CLASSNAME)) {
		tmp_lval_tab_idx = TMP_LVAL_FLOAT;
	      } else {
		if (str_eq (instancevar_value_class, ARRAY_CLASSNAME)) {
		  tmp_lval_tab_idx = TMP_LVAL_ARRAY;
		} else {
		  warning (messages[self_tok_ptr], 
			   "Instance variable %s has unimplemented "
			   "C return class, \"%s.\" "
			   "Return class defaulting to Integer.",
			   M_NAME(messages[last_instancevar_idx]),
			   instancevar_value_class);
		  tmp_lval_tab_idx = TMP_LVAL_INT;
		}
	      }
	    }
	  }
	}
      }
    }

    toks2str (messages, self_tok_ptr, last_instancevar_idx, lval_self_expr_ptr);
    strcatx (rt_expr_buf, EVAL_EXPR_FN, "(\"", 
	     lval_self_expr_ptr, " ", M_NAME(messages[c_op_idx]), " ",
	     tmp_lval_name, "\");\ndelete_method_arg_cvars ();\n", NULL);
    strcpy (tmp_lval_cvar_tab[tmp_lval_tab_idx].name, tmp_lval_name);
    
    /* should still be sprintf for a while. */
    sprintf (outbuf, SELF_LVAL_C_EXPR_TEMPLATE, 
	     tmp_lval_decl_tab[tmp_lval_tab_idx].decl_str,
	     tmp_lval_name, expr_buf,
	     fmt_register_c_method_arg_call
	     (&tmp_lval_cvar_tab[tmp_lval_tab_idx],
	      tmp_lval_name,
	      LOCAL_VAR, cvar_buf),
	     rt_expr_buf);
    if (argblk)
      buffer_argblk_stmt (outbuf);
    else
      fileout (outbuf, TRUE, 0);

    for (i = self_tok_ptr; i >= c_expr_end_idx; i--) {    
      ++messages[i] -> evaled;
      ++messages[i] -> output;
    }

    return SUCCESS;

  } /* if ((c_expr_start_idx = nextlangmsg (messages, c_op_idx)) */

  return ERROR;
}

/*
 *  Returns true if we have an expression like: 
 *    <obj> = <c_function>(<args*>);
 *
 *  or
 *
 *   <obj> = (<typecast>)<c_function>(<args*>);
 */
int obj_is_fn_expr_lvalue (MESSAGE_STACK messages,
			       int obj_idx, int stack_end) {
  int next_idx;
  int typecast_end_idx;
  CFUNC *c_fn;

  /* Expressions in control structures get handled separately
     for now at least. */
  if (ctrlblk_pred) 
    return FALSE;
  if (interpreter_pass == expr_check)
    return FALSE;

  if ((next_idx = nextlangmsg (messages, obj_idx)) == ERROR)
    return FALSE;

  if (IS_C_ASSIGNMENT_OP(M_TOK(messages[next_idx]))) {
  if ((next_idx = nextlangmsg (message_stack (), next_idx)) 
	!= ERROR) {
      if (M_TOK(message_stack_at (next_idx)) == LABEL) {
	if ((c_fn = 
	     get_function (M_NAME(message_stack_at (next_idx))))
	    != NULL) {
	  check_extra_fn_expr_tokens (message_stack (), next_idx);
	  return TRUE;
	} else {
	  int next_idx_2;
	  if ((next_idx_2 = nextlangmsg (message_stack (),
					 next_idx)) != ERROR) {
	    if (M_TOK(message_stack_at (next_idx_2)) == OPENPAREN) {
	      warning (message_stack_at(next_idx), 
		       "Undefined C function, \"%s.\"",
		       M_NAME(message_stack_at(next_idx)));
	    }
	  }
	  return FALSE;
	}
      } else if (is_typecast_expr (messages, next_idx, &typecast_end_idx)) {
	if ((next_idx = nextlangmsg (message_stack (), typecast_end_idx)) 
	    != ERROR) {
	  if (M_TOK(message_stack_at (next_idx)) == LABEL) {
	    if ((c_fn = 
		 get_function (M_NAME(message_stack_at (next_idx))))
		!= NULL) {
	      check_extra_fn_expr_tokens (message_stack (), next_idx);
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
      } else {
	return FALSE;
      }
    } else {
      return FALSE;
    }
  } else {
    return FALSE;
  }

  return FALSE;
}

static bool arg_contains_objects (MESSAGE_STACK messages,
				  int start, int end) {
  int i;
  for (i = start; i >= end; --i) {
    if (M_TOK(messages[i]) == LABEL) {
      if (str_eq (M_NAME(messages[i]), "self") ||
	  str_eq (M_NAME(messages[i]), "super"))
	return true;
      if (get_local_object (M_NAME(messages[i]), NULL) ||
	  get_global_object (M_NAME(messages[i]), NULL) ||
	  get_method_param_obj (messages, i)) {
	return true;
      }
    }
  }
  return false;
}

extern MESSAGE *m_messages[N_MESSAGES+1]; /* Declared in method.c */
extern int m_message_ptr;

static void eval_params (MESSAGE_STACK messages, 
			 int fn_expr_start_idx,
			 int fn_arg_start_idx,
			 int fn_arg_end_idx, CFUNC *cfn,
			 METHOD *arg_method,
			 OBJECT *rcvr_class_obj,
			 char *param_buf_out) {
  OBJECT *o;
  int arg_idx_s[MAXARGS];
  int i, n_arg_idx_s, expr_end;
  CVAR *param;
  char expr_buf_out[MAXMSG], expr_buf_out_2[MAXMSG];

  split_args_idx (messages, fn_arg_start_idx, fn_arg_end_idx, arg_idx_s,
		  &n_arg_idx_s);

  *param_buf_out = 0;

  if ((param = cfn -> params)  != NULL) {
    for (i = 0; (i < n_arg_idx_s) && param; i += 2) {

      if (!arg_needs_rt_eval (messages, arg_idx_s[i])) {
	/* TODO! This needs work for multiple arguments, if they're
	   mixed C and Ctalk. */
	if (*param_buf_out == 0) {
	  /* This slurps the entire argument list. */
	  o = fn_arg_expression (rcvr_class_obj, arg_method,
				 messages, fn_expr_start_idx);
	  strcpy (param_buf_out, o -> __o_name);
	  delete_object (o);
	  /* goto eval_params_cleanup; */
	  return;
	}
      }

      if (i < (n_arg_idx_s - 2)) {
	strcatx2 (param_buf_out, fmt_rt_return 
		  (fmt_rt_expr (messages, arg_idx_s[i],
				&expr_end, expr_buf_out),
		   basic_class_from_cvar
		   (messages[fn_expr_start_idx], param, 0),
		   TRUE, expr_buf_out_2), ",", NULL);
      } else {
	strcatx2 (param_buf_out, fmt_rt_return 
		  (fmt_rt_expr (messages, arg_idx_s[i],
				&expr_end, expr_buf_out),
		   basic_class_from_cvar
		   (messages[fn_expr_start_idx], param, 0),
		   TRUE, expr_buf_out_2), NULL);
      }
	
      param = param -> next;

    }
  }
  
}

/* 
   eval_params_inline checks args of function calls 
   that are method arguments 
*/

void eval_params_inline (MESSAGE_STACK messages, 
			 int fn_expr_start_idx,
			 int fn_arg_start_idx,
			 int fn_arg_end_idx, CFUNC *cfn,
			 char *param_buf_out) {
  int arg_idx_s[MAXARGS];
  int i, n_arg_idx_s, expr_end;
  CVAR *param;
  char expr_buf_out[MAXMSG], expr_buf_out_2[MAXMSG], *literal_arg;

  split_args_idx (messages, fn_arg_start_idx, fn_arg_end_idx, arg_idx_s,
		  &n_arg_idx_s);

  *param_buf_out = 0;

  if ((param = cfn -> params)  != NULL) {
    for (i = 0; (i < n_arg_idx_s) && param; i += 2) {

      if (i < (n_arg_idx_s - 2)) {
	if (!arg_contains_objects (messages, arg_idx_s[i],
				   arg_idx_s[i+1])) {
	  literal_arg = collect_tokens (messages, arg_idx_s[i],
					arg_idx_s[i+1]);
	  strcatx2 (param_buf_out,literal_arg, ",", NULL);
	  __xfree (MEMADDR(literal_arg));
	} else {
	  strcatx2 (param_buf_out, fmt_rt_return 
		    (fmt_rt_expr (messages, arg_idx_s[i],
				  &expr_end, expr_buf_out),
		     basic_class_from_cvar
		     (messages[fn_expr_start_idx], param, 0),
		     TRUE, expr_buf_out_2), ",", NULL);
	}
      } else {
	if (!arg_contains_objects (messages, arg_idx_s[i],
				   arg_idx_s[i+1])) {
	  literal_arg = collect_tokens (messages, arg_idx_s[i],
					arg_idx_s[i+1]);
	  strcatx2 (param_buf_out,literal_arg, NULL);
	  __xfree (MEMADDR(literal_arg));
	} else {
	  strcatx2 (param_buf_out, fmt_rt_return 
		    (fmt_rt_expr (messages, arg_idx_s[i],
				  &expr_end, expr_buf_out),
		     basic_class_from_cvar
		     (messages[fn_expr_start_idx], param, 0),
		     TRUE, expr_buf_out_2), NULL);
	}
      }
	
      param = param -> next;

    }
  }
  
}

/* Basically, look for a subscript and no objects... more to come later. */
static bool is_single_arg_c_subscr_expr (CFUNC *cfn, MESSAGE_STACK messages,
				  int arg_start_idx,
				  int arg_end_idx) {
  int i;
  bool have_subscr = false;
  bool have_object = false;
  if (IS_CVAR(cfn -> params)  && cfn -> params -> next == NULL) {
    for (i = arg_start_idx; i >= arg_end_idx; --i) {
      switch (M_TOK(messages[i]))
	{
	case ARRAYCLOSE:
	  have_subscr = true;
	  break;
	case LABEL:
	  if (is_object_or_param (M_NAME(messages[i]), NULL)) {
	    have_object = true;
	  }
	  break;
	}
    }
  }
  return have_subscr && !have_object;
}

#define OBJ_LVAL_EXPR_TEMPLATE "\n\
       {\n\
          %s\n\
          %s %s = %s;\n\
          %s\n\
          %s;\n\
       }\n"
/*
 *  Also handles template expressions if the function is listed in
 *  c99_names ().
 */
int format_obj_lval_fn_expr (MESSAGE_STACK messages, int rcvr_tok_idx) {
  int fn_expr_start_idx = -1, fn_arg_start_idx, fn_arg_end_idx, fn_term_idx,
    last_instancevar_idx, fn_term_idx_2, op_idx = rcvr_tok_idx;
  char fn_buf[MAXMSG], fn_expr_buf[MAXMSG], *fn_return_class,
    fn_expr_buf_2[MAXMSG * 3] = "",
    fn_expr_buf_3[MAXMSG];
  char rt_expr_buf[MAXMSG];
  char outbuf[RVALTEMPLATEMAX];
  char param_buf[MAXMSG];
  char tokenbuf[MAXMSG], exprbuf_1[MAXMSG];
  char tmpl_cvar_register_buf[MAXMSG],
    fmt_cvar_call_buf[MAXMSG];
  CFUNC *c_fn;
  int i, next_frame;
  int prefix_idx = -1;
  int typecast_start_idx = -1, typecast_end_idx = -1;
  METHOD *arg_method;
  char *c99name = NULL, *cfn_buf = NULL;
  bool have_c99_name = false;
  int tmp_lval_tab_idx = -1;

  if ((interpreter_pass != method_pass) && argblk) {
    warning (messages[rcvr_tok_idx], "\"self,\" used outside of a method.");
    return ERROR;
  }
  
  next_frame = NEXT_FRAME_START;
  for (i = rcvr_tok_idx; i > next_frame; --i) {
    if (IS_C_ASSIGNMENT_OP (M_TOK(messages[i]))) {
      op_idx = fn_expr_start_idx = i;
      break;
    }
  }
  if (fn_expr_start_idx == ERROR)
    return ERROR;

  last_instancevar_idx = prevlangmsg (messages, fn_expr_start_idx);

  if ((fn_expr_start_idx = nextlangmsg (messages,
					fn_expr_start_idx))
      != ERROR) {
    
    if ((c_fn = get_function (M_NAME(messages[fn_expr_start_idx]))) 
	== NULL) {
      if (is_typecast_expr (messages, fn_expr_start_idx, &typecast_end_idx)) {
	typecast_start_idx = fn_expr_start_idx;
	if ((fn_expr_start_idx = nextlangmsg (messages, typecast_end_idx))
	    != ERROR) {
	  if ((c_fn = get_function (M_NAME(messages[fn_expr_start_idx])))
	      == NULL) {
	    return ERROR;
	  }
	} else {
	  return ERROR;
	}
      } else {
	return ERROR;
      }
    }

    fn_return_class =
      basic_class_from_cfunc (messages[rcvr_tok_idx], c_fn, 0);

    if ((fn_arg_start_idx = nextlangmsg (messages, fn_expr_start_idx))
	== ERROR)
      return ERROR;
    if (M_TOK(messages[fn_arg_start_idx]) != OPENPAREN)
      return ERROR;
    if ((fn_arg_end_idx = match_paren (messages, fn_arg_start_idx,
				       get_stack_top (messages)))
	== ERROR) 
      return ERROR;


    /* Simplifying this, in addition to, uh, being simpler.... ,
       sidesteps some output queue fouling if the parser
       needs to recurse before we're finished (like i.e., a template,
       which isn't necessary if the fn expression is all in C. */
    if (is_single_arg_c_subscr_expr (c_fn, messages, fn_arg_start_idx,
				     fn_arg_end_idx)) {
      if ((fn_term_idx = nextlangmsg (messages, fn_arg_end_idx)) != ERROR) {
	fn_term_idx_2 = scanforward (messages, fn_term_idx,
				     get_stack_top (messages),
				     SEMICOLON);
	goto completely_c_expr;
      }
    }

    fn_term_idx_2 = -1;
    if ((fn_term_idx = nextlangmsg (messages, fn_arg_end_idx)) != ERROR) {
      if (M_TOK(messages[fn_term_idx]) == SEMICOLON) {
	/* Use a template if we have one. */
	if ((c99name = template_name (M_NAME(messages[fn_expr_start_idx]))) 
	    != NULL) {
	  cfn_buf = clib_fn_rt_expr (messages, fn_expr_start_idx);
	  toks2str (messages, fn_arg_start_idx, fn_arg_end_idx, tokenbuf);
	  memset (tmpl_cvar_register_buf, 0, MAXMSG);
	  register_template_arg_CVARs (messages, fn_arg_start_idx,
				       fn_arg_end_idx,
				       tmpl_cvar_register_buf);
					   
	  strcatx (exprbuf_1, cfn_buf, " ", tokenbuf, NULL);
	  fmt_eval_expr_str (exprbuf_1, fn_buf);
	  have_c99_name = true;
	} else {
	  toks2str (messages, fn_expr_start_idx, fn_term_idx, fn_buf);
	}
      } else {
	fn_term_idx_2 = scanforward (messages, fn_term_idx,
				     get_stack_top (messages),
				     SEMICOLON);
	if (fn_term_idx_2 == ERROR) {
	  warning (messages[fn_term_idx], "Missing semicolon.");
	  return ERROR;
	}
	toks2str (messages, fn_expr_start_idx, fn_term_idx_2, fn_buf);
	if ((c99name = template_name (M_NAME(messages[fn_expr_start_idx]))) 
	    != NULL) {
	  warning (messages[fn_expr_start_idx], 
		   "Use of a template within an expression here is not "
		   "(yet) supported.\n\n\t%s\n", fn_buf);
	}
      }
    } else {
    completely_c_expr:
      toks2str (messages, fn_expr_start_idx, fn_arg_end_idx, fn_buf);
    }

    /* the last instance variable in the receiver has an object
       assigned to it before it gets to this fn. */
    arg_method = get_instance_method 
      (messages[last_instancevar_idx],
       (messages[last_instancevar_idx] -> obj -> instancevars ? 
	messages[last_instancevar_idx] -> obj -> instancevars :
	messages[last_instancevar_idx] -> obj),
       "=", ANY_ARGS, TRUE);

    eval_params (messages, fn_expr_start_idx,
		 fn_arg_start_idx, fn_arg_end_idx,
		 c_fn, arg_method,
		 messages[rcvr_tok_idx] -> obj -> __o_class,
		 param_buf);

    tmp_lval_tab_idx = fn_return_tab_entry
      (messages[fn_expr_start_idx], c_fn);
    make_tmp_fn_block_name (tmp_lval_cvar_tab[tmp_lval_tab_idx].name);
			
    if (typecast_end_idx != ERROR)
      toks2str (messages, typecast_start_idx, typecast_end_idx,
		exprbuf_1);
    if (c99name) {
	strcpy (fn_expr_buf, fn_buf);
      } else {
	if (*param_buf == 0) {
	  if (typecast_end_idx == ERROR) {
	    strcatx (fn_expr_buf, M_NAME(messages[fn_expr_start_idx]), 
		     " ()", NULL);
	  } else {
	    strcatx (fn_expr_buf, exprbuf_1,
		     M_NAME(messages[fn_expr_start_idx]), 
		     " ()", NULL);
	  }
	} else {
	  if (typecast_end_idx == ERROR) {
	    if (c_fn -> return_type_attrs & CVAR_TYPE_CONST) {
	      /* avoid a compiler warning if we have encountered one
		 by adding a cast... */
	      if (tmp_lval_tab_idx == TMP_LVAL_CHAR_PTR) {
		strcatx (fn_expr_buf, "(char *)",
			 M_NAME(messages[fn_expr_start_idx]), 
			 " (", param_buf, ")", NULL);
	      } else {
		strcatx (fn_expr_buf, M_NAME(messages[fn_expr_start_idx]), 
			 " (", param_buf, ")", NULL);
	      }
	    } else {
	      strcatx (fn_expr_buf, M_NAME(messages[fn_expr_start_idx]), 
		       " (", param_buf, ")", NULL);
	    }
	  } else {
	    strcatx (fn_expr_buf, exprbuf_1,
		     M_NAME(messages[fn_expr_start_idx]), 
		     " (", param_buf, ")", NULL);
	  }
	}
	if (fn_term_idx_2 != -1) {
	  /* Add any trailing expression tokens after the function args. */
	  toks2str (messages, fn_term_idx, fn_term_idx_2, exprbuf_1);
	  /* strcatx doesn't handle overlapping buffers (yet) */
	  sprintf (fn_expr_buf_2, "%s %s", fn_expr_buf, exprbuf_1);
	}
      }

    if (c99name) {
      if (!str_eq (fn_return_class, OBJECT_CLASSNAME)) {
	/* Use a strcpy here - it's safer with overlapping buffers. */
	fmt_rt_return (fn_expr_buf, fn_return_class, false, fn_expr_buf_3);
	strcpy (fn_expr_buf, fn_expr_buf_3); 
      }
    }

    if ((prefix_idx = prevlangmsg (messages, rcvr_tok_idx)) != ERROR) {
      if (messages[prefix_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	toks2str (messages, prefix_idx, last_instancevar_idx, exprbuf_1);
      } else {
	toks2str (messages, rcvr_tok_idx, last_instancevar_idx, exprbuf_1);
      }
    } else {
      toks2str (messages, rcvr_tok_idx, last_instancevar_idx, exprbuf_1);
    }
    strcatx (rt_expr_buf, EVAL_EXPR_FN, "(\"",
	     exprbuf_1, " ", M_NAME(messages[op_idx]), " ",
	     tmp_lval_cvar_tab[tmp_lval_tab_idx].name,
	     "\");\ndelete_method_arg_cvars ();\n", NULL);

    memset (fmt_cvar_call_buf, 0, MAXMSG);
    memset (outbuf, 0, RVALTEMPLATEMAX);
    /* should still be sprintf for a while. */
    sprintf (outbuf, OBJ_LVAL_EXPR_TEMPLATE, 
	     (have_c99_name ? tmpl_cvar_register_buf : ""),
	     tmp_lval_decl_tab[tmp_lval_tab_idx].decl_str,
	     tmp_lval_cvar_tab[tmp_lval_tab_idx].name,
	     (*fn_expr_buf_2 ? fn_expr_buf_2 : fn_expr_buf),
	     fmt_register_c_method_arg_call 
	     (&tmp_lval_cvar_tab[tmp_lval_tab_idx],
	      tmp_lval_cvar_tab[tmp_lval_tab_idx].name,
	      LOCAL_VAR,
	      fmt_cvar_call_buf),
	     rt_expr_buf);
    if (*fmt_cvar_call_buf) {
      strcat (outbuf, "\ndelete_method_arg_cvars ();\n");
    }
    if (argblk)
      buffer_argblk_stmt (outbuf);
    else
      fileout (outbuf, TRUE, 0);

    if (prefix_idx == -1)
      prefix_idx = rcvr_tok_idx;
    if ((fn_term_idx != ERROR) && (M_TOK(messages[fn_term_idx]) == SEMICOLON)) {
      for (i = prefix_idx; i >= fn_term_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
    } else {
      for (i = prefix_idx; i >= fn_term_idx_2; i--) {    
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
    }
  }


  return SUCCESS;
}

/* this is called by rt_expr. */
char *format_fn_call_method_expr_block (MESSAGE_STACK messages,
					int fn_idx, int *fn_end_idx_out,
					char *tmp_var_out) {
  int open_paren_idx, tmp_lval_tab_idx;
  CFUNC *fn;
  char outbuf[0x4000], /* MAXMSG * 2 */
    *fn_pre_seg, fn_expr_buf[MAXMSG],
    tmp_cvar_register_buf[MAXMSG],
    param_buf[MAXMSG]; 

  open_paren_idx = nextlangmsgstack (messages, fn_idx);
  if (M_TOK(messages[open_paren_idx]) != OPENPAREN) {
    error (messages[fn_idx], "Back function call syntax for function, \"%s\".",
	   M_NAME(messages[fn_idx])) ;
  }
  *fn_end_idx_out = match_paren (messages, open_paren_idx,
				 get_stack_top (messages));
  if (*fn_end_idx_out == ERROR) {
    error (messages[fn_idx], "Back function call syntax for function, \"%s\".",
	   M_NAME(messages[fn_idx])) ;
  }
  if ((fn = get_function (M_NAME(messages[fn_idx]))) == NULL) {
    error (messages[fn_idx], "Undefined function, \"%s\".",
	   M_NAME(messages[fn_idx]));
  }

  tmp_lval_tab_idx = fn_return_tab_entry (messages[fn_idx], fn);
  make_tmp_fn_block_name (tmp_lval_cvar_tab[tmp_lval_tab_idx].name);

  /* these need to be two different buffers */
  make_tmp_fn_block_name (tmp_var_out);
  strcpy (tmp_lval_cvar_tab[tmp_lval_tab_idx].name, tmp_var_out);

  eval_params_inline (messages, fn_idx, open_paren_idx,
		      *fn_end_idx_out, fn, param_buf);
  /* the post-arglist part of the function expression is just a closing
     paren */
  fn_pre_seg = collect_tokens (messages, fn_idx, open_paren_idx);
  strcatx (fn_expr_buf, fn_pre_seg, param_buf, ")", NULL);
  __xfree (MEMADDR(fn_pre_seg));

  sprintf (outbuf, SELF_LVAL_FN_EXPR_TEMPLATE, 
	   tmp_lval_decl_tab[tmp_lval_tab_idx].decl_str, 
	   tmp_var_out,
	   fn_expr_buf,
	   fmt_register_c_method_arg_call 
	   (&tmp_lval_cvar_tab[tmp_lval_tab_idx],
	   tmp_var_out, LOCAL_VAR, tmp_cvar_register_buf), "");

  if (argblk)
    buffer_argblk_stmt (outbuf);
  else
    fileout (outbuf, TRUE, 0);

  return tmp_var_out;
}

/* this is called by rt_expr. */
char *format_fn_call_method_expr_block_cond (MESSAGE_STACK messages,
					     int fn_idx, int *fn_end_idx_out,
					     char *tmp_var_out) {
  int open_paren_idx, tmp_lval_tab_idx;
  CFUNC *fn;
  char outbuf[0x4000], /* MAXMSG * 2 */
    *fn_pre_seg, fn_expr_buf[MAXMSG],
    tmp_cvar_register_buf[MAXMSG],
    param_buf[MAXMSG]; 

  open_paren_idx = nextlangmsgstack (messages, fn_idx);
  if (M_TOK(messages[open_paren_idx]) != OPENPAREN) {
    error (messages[fn_idx], "Back function call syntax for function, \"%s\".",
	   M_NAME(messages[fn_idx])) ;
  }
  *fn_end_idx_out = match_paren (messages, open_paren_idx,
				 get_stack_top (messages));
  if (*fn_end_idx_out == ERROR) {
    error (messages[fn_idx], "Back function call syntax for function, \"%s\".",
	   M_NAME(messages[fn_idx])) ;
  }
  if ((fn = get_function (M_NAME(messages[fn_idx]))) == NULL) {
    error (messages[fn_idx], "Undefined function, \"%s\".",
	   M_NAME(messages[fn_idx]));
  }

  tmp_lval_tab_idx = fn_return_tab_entry (messages[fn_idx], fn);
  make_tmp_fn_block_name (tmp_lval_cvar_tab[tmp_lval_tab_idx].name);

  /* these need to be two different buffers */
  make_tmp_fn_block_name (tmp_var_out);
  strcpy (tmp_lval_cvar_tab[tmp_lval_tab_idx].name, tmp_var_out);

  eval_params_inline (messages, fn_idx, open_paren_idx,
		      *fn_end_idx_out, fn, param_buf);
  /* the post-arglist part of the function expression is just a closing
     paren */
  fn_pre_seg = collect_tokens (messages, fn_idx, open_paren_idx);
  strcatx (fn_expr_buf, fn_pre_seg, param_buf, ")", NULL);
  __xfree (MEMADDR(fn_pre_seg));

  sprintf (outbuf, SELF_LVAL_FN_EXPR_TEMPLATE, 
	   tmp_lval_decl_tab[tmp_lval_tab_idx].decl_str, 
	   tmp_var_out,
	   fn_expr_buf,
	   fmt_register_c_method_arg_call 
	   (&tmp_lval_cvar_tab[tmp_lval_tab_idx],
	   tmp_var_out, LOCAL_VAR, tmp_cvar_register_buf), "");

  if (argblk)
    buffer_argblk_stmt (outbuf);
  else
    fileout (outbuf, TRUE, 0);

  return tmp_var_out;
}

/*
 *  Returns true if we have an expression like: 
 *  <obj> <instancevar>+ = <c_function>();
 */
int obj_expr_is_fn_expr_lvalue (MESSAGE_STACK messages,
			       int obj_idx, int stack_end) {
  int next_idx;
  CFUNC *c_fn;
  MESSAGE *m_self, *sender_rcvr_msg, *m_sender, *next_message;
  OBJECT *first_instance_var, *next_instance_var, *prev_object_l;

  /* Expressions in control structures get handled separately
     for now at least. */
  if (ctrlblk_pred) 
    return FALSE;
  if (interpreter_pass == expr_check)
    return FALSE;

  if ((next_idx = nextlangmsg (messages, obj_idx)) == ERROR)
    return FALSE;

  m_sender = messages[next_idx];

  if ((first_instance_var = 
       get_instance_variable 
       (M_NAME(messages[next_idx]), 
	messages[obj_idx] -> obj -> __o_classname, FALSE)) 
      != NULL) {

    m_self = message_stack_at (obj_idx);

    sender_rcvr_msg = m_self;

    m_sender -> obj = first_instance_var;
    m_sender -> attrs |= OBJ_IS_INSTANCE_VAR;
    m_sender -> receiver_msg = m_self;
    m_sender -> receiver_obj = m_self -> obj;

    prev_object_l = first_instance_var;

    while ((next_idx = nextlangmsg (message_stack (), next_idx)) != ERROR) {

      next_message = message_stack_at (next_idx);

      if ((next_instance_var = 
	   get_instance_variable 
	   (M_NAME(next_message),
	    (prev_object_l -> instancevars ? 
	     prev_object_l -> instancevars -> __o_classname : 
	     prev_object_l -> __o_classname),
	    FALSE)) != NULL) {

	next_message -> obj = next_instance_var;
	next_message -> attrs |= OBJ_IS_INSTANCE_VAR;
	next_message -> receiver_obj = prev_object_l;
	next_message -> receiver_msg = sender_rcvr_msg;

	prev_object_l = next_instance_var;

      } else {

	if ((next_instance_var = 
	     get_class_variable 
	     (M_NAME(next_message),
	      (prev_object_l -> instancevars ? 
	       prev_object_l -> instancevars -> __o_classname : 
	       prev_object_l -> __o_classname),
	      FALSE)) != NULL) {

	  next_message -> obj = next_instance_var;
	  next_message -> attrs |= OBJ_IS_CLASS_VAR;
	  next_message -> receiver_obj = prev_object_l;
	  next_message -> receiver_msg = sender_rcvr_msg;
	  
	  prev_object_l = next_instance_var;
	} else {
	  
	  if (IS_C_ASSIGNMENT_OP(M_TOK(message_stack_at (next_idx)))) {
	    if ((next_idx = nextlangmsg (message_stack (), next_idx)) 
		    != ERROR) {
	      if (M_TOK(message_stack_at (next_idx)) == LABEL) {
		      if ((c_fn = 
		        get_function (M_NAME(message_stack_at (next_idx))))
		          != NULL) {
		        check_extra_fn_expr_tokens (message_stack (), next_idx);
		        return TRUE;
		      } else {
		        int next_idx_2;
		        if ((next_idx_2 = nextlangmsg (message_stack (),
						 next_idx)) != ERROR) {
		          if (M_TOK(message_stack_at (next_idx_2)) == OPENPAREN) {
		            warning (message_stack_at(next_idx), 
			            "Undefined C function, \"%s.\"",
			            M_NAME(message_stack_at (next_idx)));
		          }
		        }
		        return FALSE;
		      }
	      }
	    } else {
	      return FALSE;
	    }
	  } else {
	    return FALSE;
	  }

	}
      }
    }
  }

  if (next_idx != ERROR) {
    if (M_TOK(message_stack_at(next_idx)) == EQ) {
      if ((next_idx = nextlangmsg (message_stack (), next_idx)) 
	  != ERROR) {
	if (M_TOK(message_stack_at (next_idx)) == LABEL) {
	  if ((c_fn = 
	       get_function (M_NAME(message_stack_at (next_idx))))
	      != NULL) {
	    check_extra_fn_expr_tokens (message_stack (), next_idx);
	    return TRUE;
	  } else {
	    int next_idx_2;
	    if ((next_idx_2 = nextlangmsg (message_stack (),
					   next_idx)) != ERROR) {
	      if (M_TOK(message_stack_at (next_idx_2)) == OPENPAREN) {
		warning (message_stack_at(next_idx), 
			 "Undefined C function, \"%s.\"",
			 M_NAME(message_stack_at(next_idx)));
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
 * Find the end of an argument expression that has a CVAR
 * receiver.
 *
 * Called by method_arglist_limit_2 ().   
 */
int cvar_rcvr_arg_expr_limit (MESSAGE_STACK messages, int arg_start_idx) {
  int next_tok_idx;

  if ((next_tok_idx = nextlangmsg (messages, arg_start_idx)) == ERROR)
    return ERROR;

  switch (M_TOK(messages[next_tok_idx]))
    {
    case ARRAYOPEN:
      return cvar_array_is_arg_expr_rcvr (messages, arg_start_idx);
      break;
    case DEREF:
      return cvar_struct_ptr_is_arg_expr_rcvr 
	(messages, arg_start_idx);
      break;
    case PERIOD:
      return cvar_struct_is_arg_expr_rcvr (messages, arg_start_idx);
      break;
    default:
      return cvar_plain_is_arg_expr_rcvr (messages, arg_start_idx);
      break;
    }
  return ERROR;
}

/*
 * Returns the stack index of the argument method's argument
 * list.
 *
 * Probably should only work for an argument that has the
 * form:
 *
 * ... = <array_cvar> <method> <args>*
 *
 * Called by cvar_is_arg_expr_rcvr (), above.
 */
int cvar_array_is_arg_expr_rcvr (MESSAGE_STACK messages, int arg_start_idx) {
  CVAR *arg_rcvr_cvar;
  int i, i_1, lookahead, stack_top;
  int n_subscripts, max_subscripts;
  int arg_method_arg_start;
  int arg_method_arg_end;
  int arg_method_idx;
  int prev_tok;
  OBJECT *arg_rcvr_class_obj;
  METHOD *arg_method;

  if ((prev_tok = prevlangmsg (messages, arg_start_idx)) == ERROR)
    return ERROR;
  if (!is_c_assignment_op_label (messages[prev_tok]))
    return ERROR;
  if (struct_end (messages, arg_start_idx, get_stack_top (messages)))
    return ERROR;


  if (((arg_rcvr_cvar = get_local_var (M_NAME(messages[arg_start_idx])))
       == NULL) &&
      ((arg_rcvr_cvar = get_global_var (M_NAME(messages[arg_start_idx])))
       == NULL)) {
    return ERROR;
  } else {
    /* This gets re-done below if we have a subscript or deref. */
    if ((arg_rcvr_class_obj = 
	 get_class_object (basic_class_from_cvar (messages[arg_start_idx],
						  arg_rcvr_cvar, 0)))
	== NULL) 
      return ERROR;
  }

  stack_top = get_stack_top (messages);

  for (i = arg_start_idx; i > stack_top; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    if ((lookahead = nextlangmsg (messages,i)) == ERROR)
      return ERROR;

    switch (M_TOK(messages[lookahead])) 
      {
      case ARRAYOPEN:
	n_subscripts = max_subscripts = 0;
	for (i_1 = lookahead; (i_1 > stack_top); i_1--) {
	  if (M_TOK(messages[i_1]) == ARRAYOPEN)
	    ++n_subscripts;
	  if (n_subscripts > max_subscripts)
	    max_subscripts = n_subscripts;
	  if (M_TOK(messages[i_1]) == ARRAYCLOSE)
	    --n_subscripts;
	  if (n_subscripts == 0) {
	    i = i_1;
	    if ((arg_rcvr_class_obj = get_class_object 
		 (basic_class_from_cvar (messages[arg_start_idx],
					 arg_rcvr_cvar,
					 max_subscripts)))
		== NULL) 
	      return ERROR;
	    break;
	  }
	}
	continue;
	break;
      }

    if (((arg_method = get_instance_method (messages[i], 
					   arg_rcvr_class_obj,
					    M_NAME(messages[i]),
					   ANY_ARGS, FALSE)) != NULL) ||
	((arg_method = get_class_method  (messages[i],
					  arg_rcvr_class_obj,
					  M_NAME(messages[i]),
					  ANY_ARGS, FALSE)) != NULL)) {
      arg_method_idx = i;

      if ((arg_method_arg_start = nextlangmsg (messages, arg_method_idx))
	  == ERROR) {
	return ERROR;
      }
      
      if (arg_method -> n_params) {
	if ((arg_method_arg_end = 
	     method_arglist_limit (messages, arg_method_arg_start,
				   arg_method -> n_params,
				   arg_method -> varargs)) == ERROR) {
	  return ERROR;
	} else {
	  return arg_method_arg_end;
	}
      } else {
	return arg_method_idx;
      }
    }
  }

  return ERROR;
}

/*
 * Returns the stack index of the argument method's argument
 * list.
 *
 * For arguments that have the form:
 *
 * ... = <simple_cvar> <method> <args>*
 *
 * Called by cvar_array_is_arg_expr_rcvr (), above.
 */
int cvar_plain_is_arg_expr_rcvr (MESSAGE_STACK messages, int arg_start_idx) {
  CVAR *arg_rcvr_cvar;
  int arg_method_arg_start;
  int arg_method_arg_end;
  int arg_method_idx;
  int prev_tok;
  OBJECT *arg_rcvr_class_obj;
  METHOD *arg_method;

  if ((prev_tok = prevlangmsg (messages, arg_start_idx)) == ERROR)
    return ERROR;
  if (!is_c_assignment_op_label (messages[prev_tok]))
    return ERROR;

  if (((arg_rcvr_cvar = get_local_var (M_NAME(messages[arg_start_idx])))
       == NULL) &&
      ((arg_rcvr_cvar = get_global_var (M_NAME(messages[arg_start_idx])))
       == NULL)) {
    return ERROR;
  } else {
    /* This should normally be the case that an object that shadows a C variable
       always takes precedence. The compiler prints a warning elsewhere. */
    if (get_object (M_NAME(messages[arg_start_idx]), NULL) != NULL) {
      return ERROR;
    } else {
      if ((arg_rcvr_class_obj = get_class_object 
	   (basic_class_from_cvar (messages[arg_start_idx],
				   arg_rcvr_cvar, 0)))
	  == NULL) 
	return ERROR;
    }
  }

  arg_method_idx = nextlangmsg (messages, arg_start_idx);

  if (((arg_method = get_instance_method (messages[arg_method_idx], 
					  arg_rcvr_class_obj,
					  M_NAME(messages[arg_method_idx]),
					  ANY_ARGS, FALSE)) != NULL) ||
      ((arg_method = get_class_method  (messages[arg_method_idx],
					arg_rcvr_class_obj,
					M_NAME(messages[arg_method_idx]),
					ANY_ARGS, FALSE)) != NULL)) {
    if ((arg_method_arg_start = nextlangmsg (messages, arg_method_idx))
	== ERROR) {
      return ERROR;
    }
      
    if (arg_method -> n_params) {
      if ((arg_method_arg_end = 
	   method_arglist_limit (messages, arg_method_arg_start,
				 arg_method -> n_params,
				 arg_method -> varargs)) == ERROR) {
	return ERROR;
      } else {
	return arg_method_arg_end;
      }
    } else {
      return arg_method_idx;
    }
  }

  return ERROR;
}

/*
 * Returns the stack index of the argument method's argument
 * list.
 *
 * Probably should only work for an argument that has the
 * form:
 *
 * ... = <struct_cvar> <method> <args>*
 *
 * Called by cvar_is_arg_expr_rcvr (), above.
 */
int cvar_struct_is_arg_expr_rcvr (MESSAGE_STACK messages, int arg_start_idx) {
  CVAR *arg_rcvr_cvar;
  CVAR *terminal_member_cvar;
  CVAR *struct_defn_cvar;
  int stack_top;
  int terminal_mbr_idx;
  int arg_method_arg_start;
  int arg_method_arg_end;
  int arg_method_idx;
  int prev_tok;
  OBJECT *arg_rcvr_class_obj;
  METHOD *arg_method;

  if ((prev_tok = prevlangmsg (messages, arg_start_idx)) == ERROR)
    return ERROR;
  if (!is_c_assignment_op_label (messages[prev_tok]))
    return ERROR;
  stack_top = get_stack_top (messages);
  if ((terminal_mbr_idx = struct_end (messages, arg_start_idx, 
					    stack_top)) == ERROR)
    return ERROR;


  if (((arg_rcvr_cvar = get_local_var (M_NAME(messages[arg_start_idx])))
       == NULL) &&
      ((arg_rcvr_cvar = get_global_var (M_NAME(messages[arg_start_idx])))
       == NULL)) {
    return ERROR;
  } else {
    /* Just structs for now. */
    if (arg_rcvr_cvar -> type_attrs & CVAR_TYPE_UNION)
      return ERROR;

    if ((struct_defn_cvar = struct_defn_from_struct_decl 
	 (arg_rcvr_cvar -> type)) != NULL)
      arg_rcvr_cvar = struct_defn_cvar;

    if ((terminal_member_cvar = 
	 struct_member_from_expr_b (messages, arg_start_idx,
				    terminal_mbr_idx, arg_rcvr_cvar))
	== NULL) 
      return ERROR;
    if ((arg_rcvr_class_obj = get_class_object 
	 (basic_class_from_cvar (messages[arg_start_idx],
				 terminal_member_cvar, 0)))
	== NULL) 
      return ERROR;
  }

  if (METHOD_ARG_TERM_MSG_TYPE(messages[terminal_mbr_idx]))
    return ERROR;

  arg_method_idx = nextlangmsgstack (messages, terminal_mbr_idx);

  if (((arg_method = get_instance_method (messages[arg_method_idx], 
					  arg_rcvr_class_obj,
					  M_NAME(messages[arg_method_idx]),
					  ANY_ARGS, FALSE)) != NULL) ||
      ((arg_method = get_class_method  (messages[arg_method_idx],
					arg_rcvr_class_obj,
					M_NAME(messages[arg_method_idx]),
					ANY_ARGS, FALSE)) != NULL)) {
    if ((arg_method_arg_start = nextlangmsg (messages, arg_method_idx))
	== ERROR) {
      return ERROR;
    }
      
    if (arg_method -> n_params) {
      if ((arg_method_arg_end = 
	   method_arglist_limit (messages, arg_method_arg_start,
				 arg_method -> n_params,
				 arg_method -> varargs)) == ERROR) {
	return ERROR;
      } else {
	return arg_method_arg_end;
      }
    } else {
      return arg_method_idx;
    }
  }

  return ERROR;
}


/*
 * Returns the stack index of the argument method's argument
 * list.
 *
 * Probably should only work for an argument that has the
 * form:
 *
 * ... = <struct_ptr_cvar> <method> <args>*
 *
 * Called by cvar_is_arg_expr_rcvr (), above.
 */
int cvar_struct_ptr_is_arg_expr_rcvr (MESSAGE_STACK messages, 
				      int arg_start_idx) {
  CVAR *arg_rcvr_cvar;
  CVAR *struct_defn_cvar;
  CVAR *terminal_member_cvar;
  int stack_top;
  int terminal_mbr_idx;
  int arg_method_arg_start;
  int arg_method_arg_end;
  int arg_method_idx;
  int prev_tok;
  OBJECT *arg_rcvr_class_obj;
  METHOD *arg_method;

  if ((prev_tok = prevlangmsg (messages, arg_start_idx)) == ERROR)
    return ERROR;
  if (!is_c_assignment_op_label (messages[prev_tok]))
    return ERROR;
  stack_top = get_stack_top (messages);
  if ((terminal_mbr_idx = struct_end (messages, arg_start_idx, 
					    stack_top)) == ERROR)
    return ERROR;


  if (((arg_rcvr_cvar = get_local_var (M_NAME(messages[arg_start_idx])))
       == NULL) &&
      ((arg_rcvr_cvar = get_global_var (M_NAME(messages[arg_start_idx])))
       == NULL)) {
    return ERROR;
  } else {
    if ((struct_defn_cvar = struct_defn_from_struct_decl
	 (arg_rcvr_cvar -> type)) == NULL)
      return ERROR;

    if ((terminal_member_cvar = 
	 struct_member_from_expr_b (messages, arg_start_idx,
				    terminal_mbr_idx, struct_defn_cvar))
	== NULL) 
      return ERROR;
    if ((arg_rcvr_class_obj = get_class_object 
	 (basic_class_from_cvar (messages[arg_start_idx],
				 terminal_member_cvar, 0)))
	== NULL) 
      return ERROR;
  }

  arg_method_idx = nextlangmsg (messages, terminal_mbr_idx);

  if (((arg_method = get_instance_method (messages[arg_method_idx], 
					  arg_rcvr_class_obj,
					  M_NAME(messages[arg_method_idx]),
					  ANY_ARGS, FALSE)) != NULL) ||
      ((arg_method = get_class_method  (messages[arg_method_idx],
					arg_rcvr_class_obj,
					M_NAME(messages[arg_method_idx]),
					ANY_ARGS, FALSE)) != NULL)) {
    if ((arg_method_arg_start = nextlangmsg (messages, arg_method_idx))
	== ERROR) {
      return ERROR;
    }
      
    if (arg_method -> n_params) {
      if ((arg_method_arg_end = 
	   method_arglist_limit (messages, arg_method_arg_start,
				 arg_method -> n_params,
				 arg_method -> varargs)) == ERROR) {
	return ERROR;
      } else {
	return arg_method_arg_end;
      }
    } else {
      return arg_method_idx;
    }
  }

  return ERROR;
}

