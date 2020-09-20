/* $Id: ufntmpl.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017 Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include <unistd.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "list.h"
#include "fntmpl.h"
#include "ufntmpl.h"

extern bool ctrlblk_pred;            /* Declared in control.c. */
extern char *ascii[8193];             /* from intascii.h */

static struct _template_info {
  char fn_name[MAXLABEL];
  int n_params;
  int n_args;
  PARAM *params[MAXARGS];
  OBJECT *args[MAXARGS];
  char *argstrs[MAXARGS];
} template_info;

extern RT_EXPR_CLASS rt_expr_class;        /* Declared in control.c.         */

static int cfn_ext = 0;

/*
 *  Not needed in this version.
 */
#if 0
char *user_fn_expr_args_from_params (MESSAGE_STACK messages, int fn_idx,
				     char *buf) {
  int n_th_param_arg;
  sprintf (buf, "%s (", M_NAME(messages[fn_idx]));
  for (n_th_param_arg = 0; n_th_param_arg < template_info.n_args; 
       n_th_param_arg++) {
    strcat (buf, template_info.params[n_th_param_arg] -> name);
    if (n_th_param_arg < (template_info.n_args - 1))
      strcat (buf, ", ");
  }
  strcat (buf, ")");
  return buf;
}
#endif

char *user_fn_expr_args (MESSAGE_STACK messages, int fn_idx,
				     char buf[]) {
  int n_th_arg;
  strcpy (buf, " (");
  for (n_th_arg = 0; n_th_arg < template_info.n_args; n_th_arg++) {
    strcatx2 (buf, template_info.argstrs[n_th_arg], NULL);
    if (n_th_arg < (template_info.n_args - 1))
      strcatx2 (buf, ", ", NULL);
  }
  strcatx2 (buf, ")", NULL);
  return buf;
}

int register_user_args (char **args, int n_args, CFUNC *fn, 
			int *cvar_registration) {

  int stack_start_idx, stack_end_idx;
  int n_th_arg, i, typecast_end_idx, var_idx, n_expr_derefs;
  int agg_var_end_idx;
  char var_expr_buf[MAXMSG], *var_classname = NULL, cast_expr[MAXMSG], 
    var_cast_expr[MAXMSG], cast_var_expr[MAXMSG];
  char expr_out[MAXMSG];
  CVAR *c, *param_cvar;
  MESSAGE *m_deref_expr;

  template_info.n_args = 0;
  *cvar_registration = FALSE;
  for (n_th_arg = 0, param_cvar = fn -> params; 
       n_th_arg < n_args; 
       n_th_arg++, param_cvar = param_cvar -> next) {

    if (param_cvar == NULL) {
      _error ("ctalk: %s: argument mismatch. "
	      "(If the function uses variable arguments, they are not "
	      "(yet) supported in user templates.)\n",
	      fn -> decl);
    }

    if (get_local_object (args[n_th_arg], NULL) ||
	get_global_object (args[n_th_arg], NULL)) {
      char _expr_buf[MAXMSG];
      strcatx (_expr_buf, METHOD_ARG_ACCESSOR_FN, " (",
	       ascii[n_args - n_th_arg - 1], ")", NULL);
      template_info.argstrs[template_info.n_args++] 
	= strdup (fmt_rt_return 
		  (_expr_buf,
		   basic_class_from_cvar (NULL, param_cvar, 0), 
		   TRUE, expr_out));
      continue;
    }

    stack_start_idx = stack_start (tmpl_message_stack ());
    stack_end_idx = tokenize_reuse (tmpl_message_push, args[n_th_arg]);

    for (i = stack_start_idx, n_expr_derefs = 0; i > stack_end_idx; i--) {
      m_deref_expr = tmpl_message_stack_at (i);
      if ((M_TOK(m_deref_expr) == ARRAYOPEN) ||
	  (M_TOK(m_deref_expr) == PERIOD) ||
	  (M_TOK(m_deref_expr) == DEREF))
	++n_expr_derefs;
    }

    if (is_typecast_expr (tmpl_message_stack (), stack_start_idx, &typecast_end_idx)) {
      toks2str (tmpl_message_stack (), stack_start_idx, typecast_end_idx,
		cast_expr);
      var_idx = nextlangmsg (tmpl_message_stack (), typecast_end_idx);
      if ((c = get_local_var (tmpl_message_stack_at (var_idx) -> name))
	  != NULL) {
	register_c_var (tmpl_message_stack_at (var_idx),
			tmpl_message_stack (), var_idx,
			&agg_var_end_idx);
	*cvar_registration = TRUE;
	strcatx (var_expr_buf, TMPL_CVAR_ACCESSOR_FN, " (\"",
		 c -> name, "\")", NULL);
	strcatx (var_cast_expr, 
		 cast_expr,
		 fmt_rt_return (var_expr_buf, 
				basic_class_from_cvar (NULL,
							 c, n_expr_derefs),
				TRUE, expr_out),
		 NULL);
	template_info.argstrs[template_info.n_args++] 
	  = strdup (var_cast_expr);
      } else {
	toks2str (tmpl_message_stack (), var_idx, stack_end_idx + 1,
		  cast_var_expr);
	strcatx (var_cast_expr, cast_expr, cast_var_expr, NULL);
	template_info.argstrs[template_info.n_args++] 
	  = strdup (var_cast_expr);
      }
    } else {
      if ((c = get_local_var (tmpl_message_stack_at (stack_start_idx) -> name))
	  != NULL) {
	register_c_var (tmpl_message_stack_at (stack_start_idx),
			tmpl_message_stack (), stack_start_idx,
			&agg_var_end_idx);
	*cvar_registration = TRUE;
	if (c -> attrs & CVAR_ATTR_STRUCT_DECL) {
	  CVAR *_mbr;
	  var_classname = NULL;
	  for (_mbr = c -> members; _mbr; _mbr = _mbr -> next) {
	    if (strstr (args[n_th_arg], _mbr -> name)) {
	      var_classname =
		basic_class_from_cvar (NULL, _mbr, 0);
	    }
	  }
	  if (!var_classname) {
	    warning (tmpl_message_stack_at(stack_start_idx), 
     "Undefined variable %s, class defaulting to Integer.", args[n_th_arg]);
	    var_classname = INTEGER_CLASSNAME;
	  }
	} else {/* if (c -> attrs & CVAR_ATTR_STRUCT_DECL) { */
	  if ((c -> attrs & CVAR_ATTR_STRUCT_PTR) ||
	      (c -> attrs & CVAR_ATTR_STRUCT)) {
	    CVAR *__c_struct;
	    register_c_var (tmpl_message_stack_at (stack_start_idx), 
			    tmpl_message_stack (), stack_start_idx,
			    &agg_var_end_idx);
	    if ((__c_struct = have_struct (c -> type)) != NULL) {
	      CVAR *_mbr;
	      for (_mbr = __c_struct -> members; _mbr; _mbr = _mbr -> next) {
		if (strstr (args[n_th_arg], _mbr -> name)) {
		  var_classname =
		    basic_class_from_cvar (NULL, _mbr, 0);
		}
	      }
	      if (!*var_classname) {
		warning (tmpl_message_stack_at(stack_start_idx), 
			 "Undefined variable %s, class defaulting to Integer.", args[n_th_arg]);
		strcpy (var_classname, INTEGER_CLASSNAME);
	      }
	    } else {
	      warning (tmpl_message_stack_at (stack_start_idx), 
		       "Definition of struct %s not found.", c -> type);
	    }
	  } else { /* if (c -> attrs & CVAR_ATTR_STRUCT_PTR) { */
	    var_classname =
	      basic_class_from_cvar (NULL, c, n_expr_derefs);
	  }
	}
	/*
	 *  register_c_var basically registers the entire argument, 
	 *  so for aggregate types it should be okay to use the 
	 *  arg string in the variable expression.
	 */
	strcatx (var_expr_buf, TMPL_CVAR_ACCESSOR_FN, " (\"",
		 args[n_th_arg], "\")", NULL);
	template_info.argstrs[template_info.n_args++] = 
	  strdup (fmt_rt_return (var_expr_buf, var_classname, TRUE,
				 expr_out));
      } else {
	template_info.argstrs[template_info.n_args++] = 
	  strdup (args[n_th_arg]);
      }
    }

    for (i = stack_end_idx; i <= stack_start_idx; i++)
      reuse_message(tmpl_message_pop ());

  }
  return SUCCESS;
}

FN_TMPL *create_user_fn_template (MESSAGE_STACK messages, int fn_idx) {

  int arg_paren_start_idx, arg_paren_end_idx;
  char fn_expr_buf[MAXMSG];
  char arg_buf[MAXMSG];
  char *template_name;
  CVAR *fn_cvar;
  CFUNC *fn_cfunc = NULL; /* Avoid warnings. */
  FN_TMPL *f;
  int n_args, n_th_arg = 0;
  int n_th_param = 0;
  int cvar_registration;
  char *args[MAXARGS];

  if (((fn_cvar = get_global_var (M_NAME(messages[fn_idx]))) == NULL) &&
      ((fn_cfunc = get_function (M_NAME(messages[fn_idx]))) == NULL)) {
    warning (messages[fn_idx], 
	     "create_user_fn_template: Undefined function %s.",
	     M_NAME(messages[fn_idx]));
    return NULL;
  }

  find_fn_arg_parens (messages, fn_idx, &arg_paren_start_idx,
		      &arg_paren_end_idx);
  f = new_fn_template ();

  template_name = user_fn_template_name (M_NAME(messages[fn_idx]));
  strcpy (f -> tmpl_fn_name, template_name);

  if (fn_cfunc) {
    char param_buf[MAXMSG];
    user_fn_arg_declarations (messages, arg_paren_start_idx,
			      arg_paren_end_idx, args, &n_args);
    template_info.n_args = template_info.n_params = n_args;
    register_user_args (args, n_args, fn_cfunc, &cvar_registration);
    user_template_params (messages, fn_idx);
    fn_param_prototypes_from_cfunc (fn_cfunc, param_buf);

    user_fn_expr_args (messages, fn_idx, arg_buf);
    strcatx (fn_expr_buf, "(", M_NAME(messages[fn_idx]), " ", 
	     arg_buf, ")", NULL);
    f -> def = strdup (template_from_user_cfunc (fn_cfunc,
						 template_name,
						 fn_expr_buf, param_buf,
						 cvar_registration));
    init_c_fn_class ();
    generate_user_fn_template_init (f -> tmpl_fn_name, "Object");
    buffer_fn_template (f -> def);
    
    for (n_th_arg = 0; n_th_arg < template_info.n_args; n_th_arg++) {
      __xfree (MEMADDR(args[n_th_arg]));
      __xfree (MEMADDR(template_info.args[n_th_arg]));
    }

    for (n_th_param = 0; n_th_param < template_info.n_params; n_th_param++) {
      __xfree (MEMADDR(template_info.params[n_th_param]));
      template_info.params[n_th_param] = NULL;
    }

    return f;
  } else {
    warning (messages[fn_idx], "Use of CVAR for user function template is not implemented yet.");
    return NULL;
  }

  return NULL;
}

char *user_fn_template_name (char *fn_name) {
  static char name[MAXLABEL];
  char s[MAXLABEL], *_p_alpha;

  strcpy (s, fn_name);
  _p_alpha = strpbrk (s, "abcdefghijklmnopqrstuvwxyz");
  if (_p_alpha) *_p_alpha &= 223;  /* 11011111b */

  strcatx (name, UFN_PFX, s, "_", ascii[cfn_ext++], NULL);

  return name;
}

char *fmt_user_template_rt_expr (MESSAGE_STACK messages, int fn_idx,
				 FN_TMPL *f) {

  static char exprbuf[MAXMSG];
  char argbuf[MAXMSG];

  int arg_paren_start_idx, arg_paren_end_idx;
  find_fn_arg_parens (messages, fn_idx, &arg_paren_start_idx, 
		      &arg_paren_end_idx);
  toks2str (messages, arg_paren_start_idx, arg_paren_end_idx, argbuf);
  strcatx (exprbuf, "(" CFUNCTION_CLASSNAME, " ",
	   f -> tmpl_fn_name, " ", argbuf, ")", NULL);
  return exprbuf;
}

char *template_from_user_cfunc (CFUNC *fn, char *template_name, 
				char *fn_expr, char *param_expr,
				int cvar_registration) {
  static char template_buf[MAXMSG];

  if (!strcmp (fn -> return_type, "void")) {
    sprintf (template_buf, VOID_FN_RETURN_TMPL, template_name,
	     param_expr, fn_expr);
    return template_buf;
  }

  if (!strcmp (fn -> return_type, "int")) {
    if (cvar_registration) {
      sprintf (template_buf, INT_FN_RETURN_TMPL_FROM_CVAR, 
	       template_name, 
	       param_expr, fn_expr);
    } else {
      sprintf (template_buf, INT_FN_RETURN_TMPL, template_name, 
	       param_expr, fn_expr);
    }
    return template_buf;
  }

  if (!strcmp (fn -> return_type, "double") ||
      !strcmp (fn -> return_type, "float")) {
      if (cvar_registration) {
	sprintf (template_buf, DOUBLE_FN_RETURN_TMPL_FROM_CVAR, 
		 template_name, 
		 param_expr, fn_expr);
      } else {
	sprintf (template_buf, DOUBLE_FN_RETURN_TMPL, template_name, 
		 param_expr, fn_expr);
      }
    return template_buf;
  }

  if (!strcmp (fn -> return_type, "char")) {
    if (fn -> return_derefs == 1) {
      if (cvar_registration) {
	sprintf (template_buf, CHAR_PTR_FN_RETURN_TMPL_FROM_CVAR, 
		 template_name, 
		 param_expr, fn_expr);
      } else {
	sprintf (template_buf, CHAR_PTR_FN_RETURN_TMPL, template_name, 
		 param_expr, fn_expr);
      }
      return template_buf;
    } else {
      if (cvar_registration) {
	sprintf (template_buf, CHAR_FN_RETURN_TMPL_FROM_CVAR, 
		 template_name, 
		 param_expr, fn_expr);
      } else {
	sprintf (template_buf, CHAR_FN_RETURN_TMPL, template_name, 
		 param_expr, fn_expr);
      }
      return template_buf;
    }
  }

  _warning ("Function %s return type (%s) is not yet implemented as a template.  You should consider writing a method.  Return class defaulting to Integer anyway.\n",
	    fn->decl, fn->return_type);

  sprintf (template_buf, INT_FN_RETURN_TMPL, template_name, 
	   param_expr, fn_expr);

  return template_buf;
}


char *user_fn_template (MESSAGE_STACK messages, int fn_idx) {
  static char *exprbuf;
  FN_TMPL *template;
  int n_th_param = 0;

  template = create_user_fn_template (messages, fn_idx);
  exprbuf = fmt_user_template_rt_expr (messages, fn_idx, template);
  if (template && template -> def) __xfree (MEMADDR(template -> def));
  if (template) __xfree (MEMADDR(template));
  rt_expr_class = rt_expr_user_fn;
  for (n_th_param = 0; n_th_param < template_info.n_params; n_th_param++) {
    __xfree (MEMADDR(template_info.params[n_th_param]));
  }
  template_info.n_params = template_info.n_args = 0;
  return exprbuf;
}

extern int function_param_states[];  /* Declared in fn_param.c */

#define USER_FN_ARG_STATE_COLS 2

#define USER_FN_ARG_STATE(x) (check_state ((x), messages, \
              function_param_states, USER_FN_ARG_STATE_COLS))

int user_fn_arg_declarations (MESSAGE_STACK messages, int start_ptr, 
			      int end_ptr, char **args, int *n_args) {
  int j, k;
  int state;
  int last_state = ERROR;               /* Avoid warnings. */
  int n_parens;
  int param_start_ptr = start_ptr;
  MESSAGE *m_tok;
  char param_buf[MAXMSG];

  for (j = start_ptr, *n_args = 0, n_parens = 0; j >= end_ptr; j--) {

    if ((state = USER_FN_ARG_STATE (j)) == ERROR) {
      /* Checking here for white space simplifies the state 
	 diagram, and check_state skips the whitespace tokens,
	 but the whitespace still ends up in the parameter in 
	 the args array.
	 
	 GNU C attributes are not in the state transition diagram,
	 either.
      */
#ifdef __GNUC__
      if ((j != end_ptr) || 
	  ((j == end_ptr) && (!fn_has_gnu_attribute (messages, j)))) {
	if (!M_ISSPACE (messages[j]))
	  error (messages[j], "Parse error.");
      }
#else
      if (!M_ISSPACE (messages[j]))
	error (messages[j], "Parse error.");
#endif
    }

    m_tok = messages[j];

    switch (m_tok -> tokentype)
      {
      case LABEL:
	if (last_state == 0 || last_state == 6) {
	  if (n_parens == 1) {
	    param_start_ptr = j;
	  }
	}
	break;
      case CLOSEPAREN:
	--n_parens;
	if ((last_state != 19) && (last_state != 20)) {
	  /*
	   *  Make sure that the paren is the end 
	   *  of the parameter list.
	   */
	  if ((param_start_ptr != ERROR) && (j == end_ptr)) {
	    for (k = param_start_ptr, *param_buf = '\0'; k > j; k--)
	      strcatx2 (param_buf, messages[k] -> name, NULL);
	    args[(*n_args)++] = strdup (param_buf);
	  }
	}
	break;
      case ARGSEPARATOR:
	if ((last_state != 19) && (last_state != 20)) {
	  if (param_start_ptr != ERROR) {
	    for (k = param_start_ptr, *param_buf = '\0'; k > j; k--)
	      strcatx2 (param_buf, messages[k] -> name, NULL);
	    args[(*n_args)++] = strdup (param_buf);
	  }
	}
	break;
      case ARRAYOPEN:
	j = find_arg_end (messages, j);
	++j;
	break;
      case ELLIPSIS:
	param_start_ptr = j;
	break;
      case OPENPAREN:
	++n_parens;
	if (((state == 0) && (last_state == 96)) ||/* Typecast: , <(>label..*/
	    ((state == 0) && (last_state == 97))){ /* Typecast: (<(>label...*/
	  param_start_ptr = j;
	}
	break;
      case INTEGER:
	if ((last_state == 82) || (last_state == 48)) {
	  param_start_ptr = j;
	}
	break;
      case DOUBLE:  /* Float or double. */
	if ((last_state == 83) || (last_state == 84) || 
	    (last_state == 49) || (last_state == 50)) {
	  param_start_ptr = j;
	}
	break;
      case LITERAL:
	if ((last_state == 79) || (last_state == 16)) {
	  param_start_ptr = j;
	}
	break;
      case LITERAL_CHAR:
	if ((last_state == 80) || (last_state == 100)) {
	  param_start_ptr = j;
	}
	break;
      case LONG:
	if ((last_state == 85) || (last_state == 51)) {
	  param_start_ptr = j;
	}
	break;
      case LONGLONG:
	if ((last_state == 86) || (last_state == 52)) {
	  param_start_ptr = j;
	}
	break;
      case AMPERSAND:
	if ((last_state == 87) || (last_state == 53)) {
	  param_start_ptr = j;
	}
	break;
      case EXCLAM:
	if ((last_state == 88) || (last_state == 54)) {
	  param_start_ptr = j;
	}
	break;
      case INCREMENT:
	if ((last_state == 89) || (last_state == 55)) {
	  param_start_ptr = j;
	}
	break;
      case DECREMENT:
	if ((last_state == 90) || (last_state == 56)) {
	  param_start_ptr = j;
	}
	break;
      case BIT_COMP:
	if ((last_state == 91) || (last_state == 57)) {
	  param_start_ptr = j;
	}
	break;
      case ASTERISK:
	if ((last_state == 92) || (last_state == 58)) {
	  param_start_ptr = j;
	}
	break;
      default:
	break;
      }

    /* See the comment above. */
    if ((m_tok -> tokentype != WHITESPACE) &&
	(m_tok -> tokentype != NEWLINE))
    last_state = state;
  }

  return SUCCESS;
}

void generate_user_fn_template_init (char *template_name, char *return_class) {
  static char buf[MAXMSG];

  strcatx (buf, "__ctalkDefineTemplateMethod (\"CFunction\", \"",
	   template_name, "\", (OBJECT *(*)())", template_name, ", ",
	   ascii[template_info.n_params], ", ",
	   ascii[template_info.n_args], ");\n", NULL);
  global_init_statement (buf, FALSE);
  strcatx (buf, "__ctalkClassMethodInitReturnClass (\"CFunction\", \"",
	   template_name, "\", \"", 
	   ((return_class) ? return_class : "Any"), "\",  -1);\n", NULL);
  global_init_statement (buf, FALSE);
}

int user_template_params (MESSAGE_STACK messages, int fn_idx) {

  CFUNC *fn;
  CVAR *cvar_fn, *_param;
  char _param_buf[MAXMSG];

  if ((fn = get_function (M_NAME(messages[fn_idx]))) != NULL) {
    if (fn -> params == NULL) {
      return SUCCESS;
    } else {
      template_info.n_params = 0;
      for (_param = fn -> params; _param; _param = _param -> next) {
	*_param_buf = 0;
	declaration_expr_from_cvar (_param, _param_buf);
	template_info.params[template_info.n_params++] = 
	  method_param_str (_param_buf);
      }
    }
  } else {
    if ((cvar_fn = get_global_var (M_NAME(messages[fn_idx]))) != NULL) {
      _warning (
	"Template from CVAR for function %s is not yet implemented.\n",
	M_NAME(messages[fn_idx]));
      return ERROR;
    } else {
      _warning ("Could not find function %s definition.\n",
		M_NAME(messages[fn_idx]));
      return ERROR;
    }
  }

  return SUCCESS;
}

