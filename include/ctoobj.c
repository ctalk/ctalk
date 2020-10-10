/* $Id: ctoobj.c,v 1.3 2020/10/10 01:22:01 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016-2019 Robert Kiesling, rk3314042@gmail.com.
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
 *   Translate C function calls to objects.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

/*
 *  At the moment, we can use the c_message stack to parse 
 *  a C function call.
 */

MESSAGE *c_messages[N_MESSAGES + 1];  /* Declared in cparse.c.  */
int c_message_ptr;                   

MESSAGE_STACK c_message_stack (void) {
  return c_messages;
}

static int c_message_push (MESSAGE *m) {
  if (c_message_ptr == 0) {
    warning (m, "c_message_push: stack overflow.");
    return ERROR;
  }
  c_messages[c_message_ptr--] = m;
  return c_message_ptr;
}

extern CFUNC *functions;                     /* Declared in rt_cvar.c. */

extern char *ascii[8193];             /* from intascii.h */

extern HASHTAB declared_typedefs;

static int basic_type_attr_from_typedef (char *typedef_param) {
  CVAR *c;
  char *type_label = typedef_param;
  while ((c = get_typedef (type_label)) != NULL) {
    if ((c -> attrs & CVAR_ATTR_TYPEDEF) && (c -> type_attrs & CVAR_TYPE_TYPEDEF)) {
      type_label = c -> type;
    } else {
      return c -> type_attrs;
    }
  }
  return 0;
}

/*
 *  Call with arg_object of class, "Expr," with the name and 
 *  the value the complete function call.  When changing this
 *  function, also change fmt_c_to_obj_call (), below.
 */

/* Needed when we have a function label on its own, without parens -
   then we just translate the address into a Symbol object. */
int generate_c_to_obj_call (MESSAGE_STACK messages, int fn_ptr,
			    OBJECT *rcvr_obj, METHOD *method,
			    OBJECT *arg_object) {

  char fn_name[MAXLABEL], *s, *t;
  CFUNC *fn_cfunc;

  s = arg_object -> __o_name;
  t = fn_name;
  while (isalnum ((int)*s) || *s == '_')
    *t++ = *s++;
  *t = 0;

  if ((fn_cfunc = get_function (fn_name)) != NULL) {
    if (fn_cfunc -> return_type_attrs == 0) {
      if ((fn_cfunc -> return_type_attrs =
	   basic_type_attr_from_typedef (fn_cfunc -> return_type)) == 0) {
	warning (messages[fn_ptr],
		 "Ctalk: Function, \"%s,\" return type, \"%s,\" "
		 "unknown.  Return type defaulting to, \"int.\"\n",
		 fn_cfunc -> decl, fn_cfunc -> return_type);
	fn_cfunc -> return_type_attrs = CVAR_TYPE_INT;
      }
    }
    if ((fn_cfunc -> return_type_attrs & CVAR_TYPE_INT) ||
	(fn_cfunc -> return_type_attrs & CVAR_TYPE_LONG) ||
	(fn_cfunc -> return_type_attrs & CVAR_TYPE_CHAR &&
	 fn_cfunc -> return_derefs == 0)) {
      c_int_to_obj_call (rcvr_obj, method, arg_object);
    } else if ((fn_cfunc -> return_type_attrs & CVAR_TYPE_OBJECT) &&
	       (fn_cfunc -> return_derefs == 1)) {
      /* Returns an OBJECT * - we don't need to do anything (yet). */
      return SUCCESS;
    } else if (fn_cfunc -> return_type_attrs & CVAR_TYPE_FLOAT ||
	       fn_cfunc -> return_type_attrs & CVAR_TYPE_DOUBLE) {
      c_double_to_obj_call (rcvr_obj, method, arg_object);
    } else if (fn_cfunc -> return_type_attrs & CVAR_TYPE_CHAR &&
	       fn_cfunc -> return_derefs > 0) {
      c_char_ptr_to_obj_call (rcvr_obj, method, arg_object);
    } else if (fn_cfunc -> return_type_attrs & CVAR_TYPE_LONGLONG) {
      c_longlong_to_obj_call (rcvr_obj, method, arg_object);
    } else {
      warning (messages[fn_ptr],
	       "ctalk: Function, \"%s,\" has unimplemented return class.",
	       fn_name);
	return ERROR;
    }
  } else if (c99_name (fn_name, FALSE)) {
    warning (messages[fn_ptr], "Libc function %s has no prototype, "
	     "return class defaulting to, \"int.\"\n", fn_name);
    c_int_to_obj_call (rcvr_obj, method, arg_object);
  } else if (ctalk_lib_fn_name (fn_name, FALSE)) {
    warning (messages[fn_ptr], "Libctalk function %s has no prototype, "
	     "return class defaulting to, \"int.\"\n", fn_name);
    c_int_to_obj_call (rcvr_obj, method, arg_object);
  } else {
    warning (messages[fn_ptr], "generate_c_to_obj_call: Undefined function %s.",
       fn_name);
      return ERROR;
  }

  return SUCCESS;
}

char *fmt_c_to_symbol_obj_call (OBJECT *rcvr_obj, METHOD *method,
				OBJECT *arg_object, char *buf_out) {
  strcatx (buf_out, "__ctalkCSymbolToObj ((void *)", 
	   arg_object -> __o_name, ")", NULL);
  return buf_out;
}

char *fmt_c_to_obj_call (MESSAGE_STACK messages, int fn_ptr,
			 OBJECT *rcvr_obj, METHOD *method,
			 OBJECT *arg_object, char *buf_out,
			 ARGSTR *argstr) {

  int start_idx,
    end_idx;
  char *cast_class;

  char fn_name[MAXLABEL], *s, *t;
  CFUNC *fn_cfunc;

  if (argstr -> leading_typecast) {
    start_idx = get_stack_top (c_messages);
    end_idx = tokenize_no_error (c_message_push, argstr -> typecast_expr);
    cast_class = basic_class_from_typecast (c_messages, start_idx, end_idx);
    if (!strcmp (cast_class, "Integer") ||
	!strcmp (cast_class, "Character")) {
      strcatx (buf_out, "__ctalkCIntToObj (",
	       arg_object -> __o_name, ")", NULL);
    } else if (!strcmp (cast_class, "Float")) {
      strcatx (buf_out, "__ctalkCDoubleToObj (",
	       arg_object -> __o_name, ")", NULL);
    } else if (!strcmp (cast_class, "String")) {
      strcatx (buf_out, "__ctalkCCharPtrToObj (",
	       arg_object -> __o_name, ")", NULL);
    } else if (!strcmp (cast_class, "LongInteger")) {
      strcatx (buf_out, "__ctalkCLongLongToObj (", 
	       arg_object -> __o_name, ")", NULL);
    } else if (str_eq (cast_class, "Object")) {
      strcpy (buf_out, arg_object -> __o_name);
    } else {
      warning (c_messages[start_idx],
	       "generate_c_to_obj_call: Unimplemented class %s.",
	       cast_class);
      REUSE_MESSAGES(c_messages,c_message_ptr,N_MESSAGES)
	return NULL;
    }
    REUSE_MESSAGES(c_messages,c_message_ptr,N_MESSAGES)
    return buf_out;
  }

  s = arg_object -> __o_name;
  t = fn_name;
  while (isalnum ((int)*s) || *s == '_')
    *t++ = *s++;
  *t = 0;

  if ((fn_cfunc = get_function (fn_name)) != NULL) {
    if (fn_cfunc -> return_type_attrs == 0) {
      if ((fn_cfunc -> return_type_attrs =
	   basic_type_attr_from_typedef (fn_cfunc -> return_type)) == 0) {
	warning (messages[fn_ptr],
		 "Ctalk: Function, \"%s,\" return type, \"%s,\" "
		 "unknown.  Return type defaulting to, \"int.\"\n",
		 fn_cfunc -> decl, fn_cfunc -> return_type);
	fn_cfunc -> return_type_attrs = CVAR_TYPE_INT;
      }
    }
    if ((fn_cfunc -> return_type_attrs & CVAR_TYPE_INT) ||
	(fn_cfunc -> return_type_attrs & CVAR_TYPE_LONG) ||
	(fn_cfunc -> return_type_attrs & CVAR_TYPE_CHAR)) {
      switch (fn_cfunc -> return_derefs)
	{
	case 0:
	  strcatx (buf_out, "__ctalkCIntToObj (",
		   arg_object -> __o_name, ")", NULL);
	  break;
	case 1: /***/
	  /* This should have a clause of its own above if
	     it gets any more complicated. */
	  if (fn_cfunc -> return_type_attrs & CVAR_TYPE_CHAR) {
	    strcatx (buf_out, "__ctalkCCharPtrToObj (",
		     arg_object -> __o_name, ")", NULL);
	  }
	  break;
	default:
	  /* nothing should be needed for pointers */
	  strcpy (buf_out, arg_object -> __o_name);
	  break;
	}
    } else if (fn_cfunc -> return_type_attrs & CVAR_TYPE_FLOAT ||
	       fn_cfunc -> return_type_attrs & CVAR_TYPE_DOUBLE) {
      strcatx (buf_out, "__ctalkCDoubleToObj (",
	       arg_object -> __o_name, ")", NULL);
    } else if (fn_cfunc -> return_type_attrs & CVAR_TYPE_CHAR &&
	       fn_cfunc -> return_derefs > 0) {
      strcatx (buf_out, "__ctalkCCharPtrToObj (",
	       arg_object -> __o_name, ")", NULL);
    } else if (fn_cfunc -> return_type_attrs & CVAR_TYPE_LONGLONG) {
      strcatx (buf_out, "__ctalkCLongLongToObj (", 
	       arg_object -> __o_name, ")", NULL);
    } else if (fn_cfunc -> return_type_attrs & CVAR_TYPE_OBJECT) {
      strcpy (buf_out, arg_object -> __o_name);
    } else if (fn_cfunc -> return_type_attrs & CVAR_TYPE_VOID) {
      strcatx (buf_out, "__ctalkCSymbolToObj (", arg_object -> __o_name,
	       ")", NULL);
    } else {
      warning (messages[fn_ptr],
	       "Function, \"%s,\" has unimplemented return class.",
	       fn_name);
      return NULL;
    }
  } else {
    warning (messages[fn_ptr],
	     "Undefined function, \"%s.\"", fn_name);
      return NULL;
  }

  return buf_out;
}

extern ARG_TERM arg_c_fn_terms[MAXARGS];
extern int arg_c_fn_term_ptr;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

static int mixed_c_to_obj_call_id = 0;

static char obj_arg_int_decl[] = "int";
static char obj_arg_long_int_decl[] = "long int";
static char obj_arg_unsigned_long_int_decl[] = "unsigned long int";
static char obj_arg_long_long_int_decl[] = "long long int";
static char obj_arg_float_decl[] = "float";
static char obj_arg_double_decl[] = "double";
static char obj_arg_char_ptr_decl[] = "char *";
static char obj_arg_char_decl[] = "char ";
static char obj_arg_OBJECT_ptr_decl[] = "OBJECT *";

#define C_TO_OBJ_ARG_TMPL "{ \n\
  %s %s = %s; \n\
  %s \
}\n"

/*
 *  This registers a temporary C variable with the output of
 *  a function that is embedded in an argument expression, so
 *  the function's output in the variable can be used when the
 *  run-time library evaluates the expression; e.g.,
 *
 *    ex raiseException undefined_label_x,
 *      "In myMethod: " + strerror (errno) + ".";
 *
 *  This fn is specifically for formatting __ctalk_arg calls.
 */
void output_mixed_c_to_obj_arg_block (MESSAGE_STACK messages,
				       int method_msg_idx,
				       ARGSTR *argbuf,
				       METHOD *method) {
  CFUNC *fn;
  CVAR tmp_cvar;
  int i, j, start_arglist, end_arglist,
    start_arglist_2, end_arglist_2;
  char tmp_label[MAXLABEL];
  char tmp_var_block[0x4000];  /* MAXMSG * 2 */
  char cvar_reg_buf[MAXLABEL];
  char arg_buf[MAXMSG];
  char arg_expr_esc[MAXMSG];
  char fn_expr_buf[MAXMSG], *tmp_var_decl = obj_arg_int_decl, *arg_expr;
  char *fn_pre_seg, param_buf[MAXMSG];

  if (arg_c_fn_term_ptr == 0)
    return;

  /* Make sure we have all of the function calls in the argument. */
  for (i = arg_c_fn_terms[0].end - 1; i >= argbuf -> end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if ((fn = get_function (M_NAME(messages[i]))) != NULL) {
	arg_c_fn_terms[arg_c_fn_term_ptr].messages = messages;
	arg_c_fn_terms[arg_c_fn_term_ptr].start = i;
	if ((start_arglist = nextlangmsg (messages, i)) != ERROR) {
	  if ((end_arglist = match_paren (messages, start_arglist,
					  get_stack_top (messages)))
	      != ERROR) {
	    arg_c_fn_terms[arg_c_fn_term_ptr].end = end_arglist;
	  }
	}
	++arg_c_fn_term_ptr;
      }
    }
  }

  for (i = 0; i < arg_c_fn_term_ptr; ++i) {
    if ((fn = get_function (M_NAME(messages[arg_c_fn_terms[i].start])))
	== NULL) {
      error (messages[arg_c_fn_terms[i].start],
	     "Can't find definition of function, \"%s.\"",
	     M_NAME(messages[i]));
    }
    start_arglist_2 = nextlangmsg (messages,
				   arg_c_fn_terms[i].start);
    if (M_TOK(messages[start_arglist_2]) != OPENPAREN) {
      error (messages[arg_c_fn_terms[i].start],
	     "Bad function call syntax, \"%s.\"",
	     M_NAME(messages[arg_c_fn_terms[i].start]));
    }
    if ((end_arglist_2 = match_paren (messages, start_arglist_2,
				      get_stack_top (messages)))
	== ERROR) {
      error (messages[arg_c_fn_terms[i].start],
	     "Mismatched parentheses in function \"%s\" call.",
	     M_NAME(messages[arg_c_fn_terms[i].start]));
    }
    memset (&tmp_cvar, 0, sizeof (struct _cvar));
    tmp_cvar.sig = CVAR_SIG;
    if (fn -> return_type_attrs & CVAR_TYPE_CHAR) {
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_char_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "char");
	  tmp_cvar.type_attrs = CVAR_TYPE_CHAR;
	  tmp_cvar.n_derefs = 0;
	  break;
	case 1:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_char_ptr_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "char");
	  tmp_cvar.type_attrs = CVAR_TYPE_CHAR;
	  tmp_cvar.n_derefs = 1;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	} /* switch */
    } else if (fn -> return_type_attrs & CVAR_TYPE_INT) {
      if (fn -> return_type_attrs & CVAR_TYPE_LONGLONG) {
	switch (fn -> return_derefs)
	  {
	  case 0:
	    strcatx (tmp_label, 
		     "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		     NULL);
	    tmp_var_decl = obj_arg_long_long_int_decl;
	    strcpy (tmp_cvar.name, tmp_label);
	    strcpy (tmp_cvar.type, "int");
	    strcpy (tmp_cvar.qualifier, "long");
	    strcpy (tmp_cvar.qualifier2, "long");
	    tmp_cvar.type_attrs = CVAR_TYPE_INT | CVAR_TYPE_LONGLONG;
	    tmp_cvar.n_derefs = 0;
	    break;
	  default:
	    warning (messages[arg_c_fn_terms[i].start],
		     "Unsupported return derefs for function, \"%s\".",
		     M_NAME(messages[arg_c_fn_terms[i].start]));
	    break;
	  }
      } else if (fn -> return_type_attrs & CVAR_TYPE_LONG) {
	switch (fn -> return_derefs)
	  {
	  case 0:
	    strcatx (tmp_label, 
		     "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		     NULL);
	    tmp_var_decl = obj_arg_long_int_decl;
	    strcpy (tmp_cvar.name, tmp_label);
	    strcpy (tmp_cvar.type, "int");
	    strcpy (tmp_cvar.qualifier, "long");
	    tmp_cvar.type_attrs = CVAR_TYPE_INT | CVAR_TYPE_LONG;
	    tmp_cvar.n_derefs = 0;
	    break;
	  default:
	    warning (messages[arg_c_fn_terms[i].start],
		     "Unsupported return derefs for function, \"%s\".",
		     M_NAME(messages[arg_c_fn_terms[i].start]));
	    break;
	  }
      } else {
	switch (fn -> return_derefs)
	  {
	  case 0:
	    strcatx (tmp_label, 
		     "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		     NULL);
	    tmp_var_decl = obj_arg_int_decl;
	    strcpy (tmp_cvar.name, tmp_label);
	    strcpy (tmp_cvar.type, "int");
	    tmp_cvar.type_attrs = CVAR_TYPE_INT;
	    tmp_cvar.n_derefs = 0;
	    break;
	  default:
	    warning (messages[arg_c_fn_terms[i].start],
		     "Unsupported return derefs for function, \"%s\".",
		     M_NAME(messages[arg_c_fn_terms[i].start]));
	    break;
	  }
      } /* if (fn -> return_type_attrs & CVAR_TYPE_LONGLONG) */
    } else if (fn -> return_type_attrs & CVAR_TYPE_LONGLONG) {
      /* Sometime we find only the CVAR_TYPE_LONGLONG attribute alone. */
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_long_long_int_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "int");
	  strcpy (tmp_cvar.qualifier, "long");
	  strcpy (tmp_cvar.qualifier2, "long");
	  tmp_cvar.type_attrs = CVAR_TYPE_INT | CVAR_TYPE_LONGLONG;
	  tmp_cvar.n_derefs = 0;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	}
    } else if (str_eq (fn -> return_type, "size_t")) { /***/
      /* handle like an unsigned long int */
      /* TODO - should have return type attributes derived 
	 from size_t */
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_unsigned_long_int_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "int");
	  strcpy (tmp_cvar.qualifier, "long");
	  tmp_cvar.type_attrs = CVAR_TYPE_INT | CVAR_TYPE_LONG | CVAR_TYPE_UNSIGNED;
	  tmp_cvar.n_derefs = 0;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	}
    } else if (fn -> return_type_attrs & CVAR_TYPE_FLOAT) {
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_float_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "float");
	  tmp_cvar.type_attrs = CVAR_TYPE_FLOAT;
	  tmp_cvar.n_derefs = 0;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	}
    } else if (fn -> return_type_attrs & CVAR_TYPE_OBJECT) {
      if (fn -> return_derefs == 1) {
	strcatx (tmp_label, 
		 "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		 NULL);
	tmp_var_decl = obj_arg_OBJECT_ptr_decl;
	strcpy (tmp_cvar.name, tmp_label);
	strcpy (tmp_cvar.type, "OBJECT");
	tmp_cvar.type_attrs = CVAR_TYPE_OBJECT;
	tmp_cvar.n_derefs = 1;
      } else {
	warning (messages[arg_c_fn_terms[i].start],
		 "Unsupported return derefs for function, \"%s\".",
		 M_NAME(messages[arg_c_fn_terms[i].start]));
      }
    } else if (fn -> return_type_attrs & CVAR_TYPE_DOUBLE) {
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_double_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "double");
	  tmp_cvar.type_attrs = CVAR_TYPE_DOUBLE;
	  tmp_cvar.n_derefs = 0;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	}
    } else {
      warning (messages[method_msg_idx], "Unimplemented return type "
	       "for function, \"%s\". (Return type, \"%s\".)",
	       fn -> decl,
	       fn -> return_type);
    }
    eval_params_inline (messages, arg_c_fn_terms[i].start,
			start_arglist_2, end_arglist_2, fn,
			param_buf);
    fn_pre_seg = collect_tokens (messages, arg_c_fn_terms[i].start,
				 start_arglist_2);
    strcatx (fn_expr_buf, fn_pre_seg, param_buf, ")", NULL);
    __xfree (MEMADDR(fn_pre_seg));

    fmt_register_c_method_arg_call
        (&tmp_cvar, tmp_label, LOCAL_VAR, cvar_reg_buf);
    sprintf (tmp_var_block, C_TO_OBJ_ARG_TMPL,
	     tmp_var_decl, tmp_label,fn_expr_buf, cvar_reg_buf);

    fileout (tmp_var_block, 0,
	     frame_at (CURRENT_PARSER -> frame) -> message_frame_top);

    for (j = arg_c_fn_terms[i].start; j >= arg_c_fn_terms[i].end; j--) {
        messages[j] -> name[0] = ' '; messages[j] -> name[1] = '\0';
        messages[j] -> tokentype = WHITESPACE;
    }
    strcpy (messages[arg_c_fn_terms[i].start] -> name, tmp_label);
    messages[arg_c_fn_terms[i].start] -> tokentype = LABEL; /***/
    messages[arg_c_fn_terms[i].start] -> attrs |= TOK_IS_TMP_FN_RESULT;
  } /* for (i = 0; i < arg_c_fn_term_ptr; ++i) */
  if (!(messages[method_msg_idx] -> attrs & TOK_IS_PRINTF_ARG)) { /***/
    /* printf args should be handled by stdarg_fmt_arg_expr, arg.c */
    arg_expr = collect_tokens (messages, argbuf -> start_idx,
      argbuf -> end_idx);
    escape_str_quotes (arg_expr, arg_expr_esc);
    de_newline_buf (arg_expr_esc);
    strcatx (arg_buf, "__ctalk_arg (\"",
      messages[method_msg_idx] -> receiver_obj -> __o_name,
      "\", \"",
      method -> name, "\", ", ascii[method -> n_params],
      ", (void *)", "\"", arg_expr_esc, "\"", ");\n", NULL);
    fileout (arg_buf, 0,
      frame_at (CURRENT_PARSER -> frame) -> message_frame_top);
  
    __xfree (MEMADDR(arg_expr));
  }
  arg_c_fn_term_ptr = 0;
}

/* Similar to above, but for printf format args. */
void output_mixed_c_to_obj_fmt_arg (MESSAGE_STACK messages,
				    int rcvr_idx,
				    int method_msg_idx) {
  CFUNC *fn;
  CVAR tmp_cvar;
  int i, j, start_arglist, end_arglist;
  int expr_end_idx;
  char tmp_label[MAXLABEL];
  char tmp_var_block[MAXMSG];
  char cvar_reg_buf[MAXLABEL];
  char arg_expr[MAXMSG], arg_expr_2[MAXMSG];
  char *fn_expr, *tmp_var_decl = obj_arg_int_decl;

  if (arg_c_fn_term_ptr == 0)
    return;

  /* Make sure we have all of the function calls in the argument. */
  for (i = arg_c_fn_terms[0].end - 1;
       !METHOD_ARG_TERM_MSG_TYPE(messages[i]); i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if ((fn = get_function (M_NAME(messages[i]))) != NULL) {
	arg_c_fn_terms[arg_c_fn_term_ptr].messages = messages;
	arg_c_fn_terms[arg_c_fn_term_ptr].start = i;
	if ((start_arglist = nextlangmsg (messages, i)) != ERROR) {
	  if ((end_arglist = match_paren (messages, start_arglist,
					  get_stack_top (messages)))
	      != ERROR) {
	    arg_c_fn_terms[arg_c_fn_term_ptr].end = end_arglist;
	  }
	}
	++arg_c_fn_term_ptr;
      }
    }
  }

  for (i = 0; i < arg_c_fn_term_ptr; ++i) {
    if ((fn = get_function (M_NAME(messages[arg_c_fn_terms[i].start])))
	== NULL) {
      error (messages[arg_c_fn_terms[i].start],
	     "Can't find definition of function, \"%s.\"",
	     M_NAME(messages[i]));
    }
    memset (&tmp_cvar, 0, sizeof (struct _cvar));
    tmp_cvar.sig = CVAR_SIG;
    if (fn -> return_type_attrs & CVAR_TYPE_CHAR) {
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_char_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "char");
	  tmp_cvar.type_attrs = CVAR_TYPE_CHAR;
	  tmp_cvar.n_derefs = 0;
	  break;
	case 1:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_char_ptr_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "char");
	  tmp_cvar.type_attrs = CVAR_TYPE_CHAR;
	  tmp_cvar.n_derefs = 1;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	} /* switch */
    } else if (fn -> return_type_attrs & CVAR_TYPE_INT) {
      if (fn -> return_type_attrs & CVAR_TYPE_LONGLONG) {
	switch (fn -> return_derefs)
	  {
	  case 0:
	    strcatx (tmp_label, 
		     "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		     NULL);
	    tmp_var_decl = obj_arg_long_long_int_decl;
	    strcpy (tmp_cvar.name, tmp_label);
	    strcpy (tmp_cvar.type, "int");
	    strcpy (tmp_cvar.qualifier, "long");
	    strcpy (tmp_cvar.qualifier2, "long");
	    tmp_cvar.type_attrs = CVAR_TYPE_INT | CVAR_TYPE_LONGLONG;
	    tmp_cvar.n_derefs = 0;
	    break;
	  default:
	    warning (messages[arg_c_fn_terms[i].start],
		     "Unsupported return derefs for function, \"%s\".",
		     M_NAME(messages[arg_c_fn_terms[i].start]));
	    break;
	  }
      } else if (fn -> return_type_attrs & CVAR_TYPE_LONG) {
	switch (fn -> return_derefs)
	  {
	  case 0:
	    strcatx (tmp_label, 
		     "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		     NULL);
	    tmp_var_decl = obj_arg_long_int_decl;
	    strcpy (tmp_cvar.name, tmp_label);
	    strcpy (tmp_cvar.type, "int");
	    strcpy (tmp_cvar.qualifier, "long");
	    tmp_cvar.type_attrs = CVAR_TYPE_INT | CVAR_TYPE_LONG;
	    tmp_cvar.n_derefs = 0;
	    break;
	  default:
	    warning (messages[arg_c_fn_terms[i].start],
		     "Unsupported return derefs for function, \"%s\".",
		     M_NAME(messages[arg_c_fn_terms[i].start]));
	    break;
	  }
      } else {
	switch (fn -> return_derefs)
	  {
	  case 0:
	    strcatx (tmp_label, 
		     "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		     NULL);
	    tmp_var_decl = obj_arg_int_decl;
	    strcpy (tmp_cvar.name, tmp_label);
	    strcpy (tmp_cvar.type, "int");
	    tmp_cvar.type_attrs = CVAR_TYPE_INT;
	    tmp_cvar.n_derefs = 0;
	    break;
	  default:
	    warning (messages[arg_c_fn_terms[i].start],
		     "Unsupported return derefs for function, \"%s\".",
		     M_NAME(messages[arg_c_fn_terms[i].start]));
	    break;
	  }
      } /* if (fn -> return_type_attrs & CVAR_TYPE_LONGLONG) */
    } else if (fn -> return_type_attrs & CVAR_TYPE_LONGLONG) {
      /* Sometime we find only the CVAR_TYPE_LONGLONG attribute alone. */
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_long_long_int_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "int");
	  strcpy (tmp_cvar.qualifier, "long");
	  strcpy (tmp_cvar.qualifier2, "long");
	  tmp_cvar.type_attrs = CVAR_TYPE_INT | CVAR_TYPE_LONGLONG;
	  tmp_cvar.n_derefs = 0;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	}
    } else if (fn -> return_type_attrs & CVAR_TYPE_FLOAT) {
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_float_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "float");
	  tmp_cvar.type_attrs = CVAR_TYPE_FLOAT;
	  tmp_cvar.n_derefs = 0;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	}
    } else if (fn -> return_type_attrs & CVAR_TYPE_DOUBLE) {
      switch (fn -> return_derefs)
	{
	case 0:
	  strcatx (tmp_label, 
		   "arg_c_fn_", ascii[mixed_c_to_obj_call_id++],
		   NULL);
	  tmp_var_decl = obj_arg_double_decl;
	  strcpy (tmp_cvar.name, tmp_label);
	  strcpy (tmp_cvar.type, "double");
	  tmp_cvar.type_attrs = CVAR_TYPE_DOUBLE;
	  tmp_cvar.n_derefs = 0;
	  break;
	default:
	  warning (messages[arg_c_fn_terms[i].start],
		   "Unsupported return derefs for function, \"%s\".",
		   M_NAME(messages[arg_c_fn_terms[i].start]));
	  break;
	}
    } else {
      warning (messages[method_msg_idx], "Unimplemented return type "
	       "for function, \"%s\". (Return type, \"%s\".)",
	       fn -> decl,
	       fn -> return_type);
    }
    fn_expr = collect_tokens (messages, arg_c_fn_terms[i].start,
       arg_c_fn_terms[i].end);
    fmt_register_c_method_arg_call
        (&tmp_cvar, tmp_label, LOCAL_VAR, cvar_reg_buf);
    sprintf (tmp_var_block, C_TO_OBJ_ARG_TMPL,
	       tmp_var_decl, tmp_label, fn_expr,
	       cvar_reg_buf);
    __xfree (MEMADDR(fn_expr));

    fileout (tmp_var_block, 0,
	     frame_at (CURRENT_PARSER -> frame) -> message_frame_top);

    for (j = arg_c_fn_terms[i].start; j >= arg_c_fn_terms[i].end; j--) {
        messages[j] -> name[0] = ' '; messages[j] -> name[1] = '\0';
        messages[j] -> tokentype = WHITESPACE;
    }
    strcpy (messages[arg_c_fn_terms[i].start] -> name, tmp_label);
    messages[arg_c_fn_terms[i].start] -> tokentype = LABEL;
  } /* for (i = 0; i < arg_c_fn_term_ptr; ++i) */
  memset (arg_expr, 0, MAXMSG);
  fmt_rt_expr (messages, rcvr_idx, &expr_end_idx, arg_expr);
  de_newline_buf (arg_expr);
  fmt_rt_return (arg_expr, M_OBJ(messages[rcvr_idx]) -> __o_name,
		 TRUE, arg_expr_2);
  fileout (arg_expr_2, 0, rcvr_idx);
  arg_c_fn_term_ptr = 0;
  for (i = rcvr_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
}

int c_int_to_obj_call (OBJECT *rcvr_object, METHOD *method, 
		       OBJECT *arg_object) {
  char buf[MAXMSG];

  sprintf (buf, 
	   "__ctalk_arg (\"%s\", \"%s\", %d, __ctalkCIntToObj (%s));\n",
	   rcvr_object->__o_name, method->name, 
	   method -> n_params, arg_object -> __o_name);

   if (is_global_frame ())
     fn_init (buf, FALSE);
   else
     fileout (buf, 0, 0);

  return SUCCESS;
}

int c_longlong_to_obj_call (OBJECT *rcvr_object, METHOD *method, 
			    OBJECT *arg_object) {
  char buf[MAXMSG];

  sprintf (buf, "__ctalk_arg (\"%s\", \"%s\", %d, __ctalkCLongLongToObj (%s));\n",
	   rcvr_object->__o_name, method->name, 
	   method -> n_params, arg_object -> __o_name);

   if (is_global_frame ())
     fn_init (buf, FALSE);
   else
     fileout (buf, 0, 0);

  return SUCCESS;
}

int c_double_to_obj_call (OBJECT *rcvr_object, METHOD *method, 
		       OBJECT *arg_object) {
  char buf[MAXMSG];

  sprintf (buf, "__ctalk_arg (\"%s\", \"%s\", %d, __ctalkCDoubleToObj (%s));\n",
	   rcvr_object->__o_name, method->name, 
	   method -> n_params, arg_object -> __o_name);

   if (is_global_frame ())
     fn_init (buf, FALSE);
   else
     fileout (buf, 0, 0);

  return SUCCESS;
}

int c_char_ptr_to_obj_call (OBJECT *rcvr_object, METHOD *method, 
		       OBJECT *arg_object) {
  char buf[MAXMSG];

  sprintf (buf, "__ctalk_arg (\"%s\", \"%s\", %d, __ctalkCCharPtrToObj (%s));\n",
	   rcvr_object->__o_name, method->name, 
	   method -> n_params, arg_object -> __o_name);

   if (is_global_frame ())
     fn_init (buf, FALSE);
   else
     fileout (buf, 0, 0);

  return SUCCESS;
}

char *fn_return_class (char *fn_name) {
  CFUNC *fn_cfunc;
  char *fn_libname;
  if ((fn_cfunc = get_function (fn_name)) != NULL) {
    return cfunc_return_class (fn_cfunc);
  }

  if ((fn_libname = c99_name (fn_name, FALSE)) == NULL) {
    _warning ("fn_return_class (): Libc function %s has no prototype, return class defaulting to %s.\n", fn_libname, DEFAULT_LIBC_RETURNCLASS);
    return DEFAULT_LIBC_RETURNCLASS;
  }
  if ((fn_libname = ctalk_lib_fn_name (fn_name, FALSE)) == NULL) {
    _warning ("fn_return_class (): Libctalk function %s has no prototype, return class defaulting to %s.\n", fn_libname, DEFAULT_LIBC_RETURNCLASS);
    return DEFAULT_LIBC_RETURNCLASS;
  }

  return NULL;
}

/* 
   Uncomment this #define if the compiler needs some of the return 
   value buffered in static memory. 
*/
/* #define CFUNC_RETURN_CLASS_BUFFER_RETURN */

char *cfunc_return_class (CFUNC *fn_cfunc) {

#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
  static char buf[MAXLABEL];
#endif

  if (fn_cfunc -> return_type_attrs == CVAR_TYPE_CHAR) {
  switch (fn_cfunc -> return_derefs)
      {
      case 2:
#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
	strcpy (buf, "Array");
#else
	return "Array";
#endif
	break;
      case 1:
#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
	strcpy (buf, "String");
#else
	return "String";
#endif
	break;
      case 0:
#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
	strcpy (buf, "Character");
#else
	return "Character";
#endif
	break;
      default:
	break;
      }
  } else {
    if (!strcmp (fn_cfunc -> return_type, "int") ||
 	!strcmp (fn_cfunc -> return_type, "short") ||
 	!strcmp (fn_cfunc -> return_type, "long")) {
      if ((!strcmp (fn_cfunc -> return_type, "long") && 
 	   !strcmp (fn_cfunc -> qualifier_type, "long")) ||
 	  (!strcmp (fn_cfunc -> qualifier_type, "long") &&
 	   !strcmp (fn_cfunc -> qualifier2_type, "long")))
#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
 	strcpy (buf, "LongInteger");
#else
	return "LongInteger";
#endif
      else
#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
	strcpy (buf, "Integer");
#else
	return "Integer";
#endif
    } else {
      if (!strcmp (fn_cfunc -> return_type, "float") ||
 	  !strcmp (fn_cfunc -> return_type, "double")) {
#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
 	strcpy (buf, "Float");
#else
	return "Float";
#endif
      } else {
	if (str_eq (fn_cfunc -> return_type, "OBJECT")) {
#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
	  strcpy (buf, "Object");
#else
	  return "Object";
#endif
	} else {
	  CVAR *__typedef_cvar;
	  if ((__typedef_cvar = (CVAR *)_hash_get
	       (declared_typedefs, (void *)fn_cfunc -> return_type)) != NULL) {
	    return basic_class_from_cvar (NULL, __typedef_cvar, 0);
	  }
	}
      }
    }
  }
#ifdef CFUNC_RETURN_CLASS_BUFFER_RETURN
  return buf;
#else
  return NULL;
#endif
}

