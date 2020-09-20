/* $Id: return.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"

extern I_PASS interpreter_pass;         /* Declared in lib/rtinfo.c.  */
extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c.*/
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;        /* Declared in lib/rtnwmthd.c and    */
                                      /* filled in by                      */
                                      /* new_instance|class_method ().     */

bool make_argblk_expr = false;
char g_argblk_expr[MAXMSG];

extern char *ascii[8193];             /* from intascii.h */

static char *basic_constant_return_class_fn (int tok) {
  METHOD *m;
  static char fn_name[MAXLABEL];
  if (interpreter_pass == method_pass) {
    m = new_methods[new_method_ptr + 1] -> method;
    *fn_name = '\0';
    if (*(m -> returnclass)) {
      if (!strcmp (m -> returnclass, LONGINTEGER_CLASSNAME)) {
	strcpy (fn_name, LLINT_CONSTANT_RETURN_FN);
      }
      if (!strcmp (m -> returnclass, BOOLEAN_CLASSNAME)) {
	strcpy (fn_name, BOOL_CONSTANT_RETURN_FN);
      }
      if (!strcmp (m -> returnclass, INTEGER_CLASSNAME)) {
	strcpy (fn_name, INT_CONSTANT_RETURN_FN);
      }
      if (!strcmp (m -> returnclass, CHARACTER_CLASSNAME)) {
	strcpy (fn_name, CHAR_CONSTANT_RETURN_FN);
      }
      if (!strcmp (m -> returnclass, STRING_CLASSNAME)) {
	strcpy (fn_name, CHAR_PTR_CONSTANT_RETURN_FN);
      }
      if (!strcmp (m -> returnclass, FLOAT_CLASSNAME)) {
	strcpy (fn_name, FLOAT_CONSTANT_RETURN_FN);
      }
    } else {
      if (!strcmp (rcvr_class_obj -> __o_name, LONGINTEGER_CLASSNAME)) {
	strcpy (fn_name, LLINT_CONSTANT_RETURN_FN);
      }
      if (!strcmp (rcvr_class_obj -> __o_name, BOOLEAN_CLASSNAME)) {
	strcpy (fn_name, BOOL_CONSTANT_RETURN_FN);
      }
      if (!strcmp (rcvr_class_obj -> __o_name, INTEGER_CLASSNAME)) {
	strcpy (fn_name, INT_CONSTANT_RETURN_FN);
      }
      if (!strcmp (rcvr_class_obj -> __o_name, CHARACTER_CLASSNAME)) {
	strcpy (fn_name, CHAR_CONSTANT_RETURN_FN);
      }
      if (!strcmp (rcvr_class_obj -> __o_name, STRING_CLASSNAME)) {
	strcpy (fn_name, CHAR_PTR_CONSTANT_RETURN_FN);
      }
      if (!strcmp (rcvr_class_obj -> __o_name, FLOAT_CLASSNAME)) {
	strcpy (fn_name, FLOAT_CONSTANT_RETURN_FN);
      }
    }
    if ((*fn_name == '\0') || 
	!strcmp (m -> returnclass, "Any")) {
      switch (tok)
	{
	case INTEGER:
	  strcpy (fn_name, INT_CONSTANT_RETURN_FN);
	  break;
	case LONG:
	case LONGLONG:
	  strcpy (fn_name, LLINT_CONSTANT_RETURN_FN);
	  break;
	case LITERAL:
	  strcpy (fn_name, CHAR_PTR_CONSTANT_RETURN_FN);
	  break;
	case LITERAL_CHAR:
	  strcpy (fn_name, CHAR_CONSTANT_RETURN_FN);
	  break;
	case DOUBLE:
	  strcpy (fn_name, FLOAT_CONSTANT_RETURN_FN);
	  break;
	default:
	  _warning ("Warning: Return class of method %s (class %s) not supported, defaulting to Integer.\n", m -> name, rcvr_class_obj -> __o_name);
	  strcpy (fn_name, INT_CONSTANT_RETURN_FN);
	  break;
	}
    }
    if (! *fn_name) {
      _warning ("Warning: Return class of method %s (class %s) not supported, defaulting to Integer.\n", m -> name, rcvr_class_obj -> __o_name);
      strcpy (fn_name, INT_CONSTANT_RETURN_FN);
    }
    return fn_name;
  } else if (make_argblk_expr) { /*  if (interpreter_pass == method_pass) */
    switch (tok)
      {
      case INTEGER:
	strcpy (fn_name, INT_CONSTANT_RETURN_FN);
	break;
      case LONG:
      case LONGLONG:
	strcpy (fn_name, LLINT_CONSTANT_RETURN_FN);
	break;
      case LITERAL:
	strcpy (fn_name, CHAR_PTR_CONSTANT_RETURN_FN);
	break;
      case LITERAL_CHAR:
	strcpy (fn_name, CHAR_CONSTANT_RETURN_FN);
	break;
      case DOUBLE:
	strcpy (fn_name, FLOAT_CONSTANT_RETURN_FN);
	break;
      default:
	_warning ("Warning: Return class not supported, defaulting to Integer.\n");
	strcpy (fn_name, INT_CONSTANT_RETURN_FN);
	break;
      }
    return fn_name;
  } /*   if (interpreter_pass == method_pass) */
  return fn_name;
}

static char *deref_terminal_return_fn (char *classname) {
  static char fn_name[MAXLABEL];
  *fn_name = '\0';
  if (!strcmp (classname, LONGINTEGER_CLASSNAME)) {
    strcpy (fn_name, LLINT_CONSTANT_RETURN_FN);
  } else if (!strcmp (classname, BOOLEAN_CLASSNAME)) {
    strcpy (fn_name, BOOL_CONSTANT_RETURN_FN);
  } else if (!strcmp (classname, INTEGER_CLASSNAME)) {
    strcpy (fn_name, INT_CONSTANT_RETURN_FN);
  } else if (!strcmp (classname, CHARACTER_CLASSNAME)) {
    strcpy (fn_name, CHAR_CONSTANT_RETURN_FN);
  } else if (!strcmp (classname, STRING_CLASSNAME)) {
    strcpy (fn_name, CHAR_PTR_CONSTANT_RETURN_FN);
  } else if (!strcmp (classname, FLOAT_CLASSNAME)) {
    strcpy (fn_name, FLOAT_CONSTANT_RETURN_FN);
  } else if (!strcmp (classname, OBJECT_CLASSNAME)) {
    strcpy (fn_name, OBJECT_MBR_RETURN_FN);
  }
  if (!*fn_name) {
    _warning ("Struct member return class %s not supported, defaulting to String\n", classname);
    strcpy (fn_name, CHAR_PTR_CONSTANT_RETURN_FN);
  }
  return fn_name;
}

static int ret_is_method_param (char *paramname) {

  METHOD *n_method;
  int n_th_param;

  if (new_method_ptr >= MAXARGS)
    return false;

  if ((n_method = new_methods[new_method_ptr + 1] -> method) != NULL) {
    for (n_th_param = 0; n_th_param < n_method -> n_params; n_th_param++) {
      if (str_eq (paramname, n_method -> params[n_th_param]->name))
	return n_th_param;
    }
  }

  return ERROR;
}

/*
 *  Not simple any more - handles most things where the first
 *  token is "self" and the second is a math operator.
 */
static bool ssr_operand_expr (MESSAGE_STACK messages,
			      int keyword_ptr, int self_idx,
			      int *op_idx_out, int *expr_end_idx_out,
			      char *operand_out) {
  int op_idx, i, prev_tok, n_th_param;
  OBJECT *operand_obj;
  METHOD *n_method;
  char operand_out_tmp[MAXMSG], operand_out_tmp_2[MAXMSG];
  bool have_cvar_reg;

  if (!(messages[self_idx] -> attrs & TOK_SELF))
    return false;

  *operand_out = 0;
  if ((op_idx = nextlangmsg (messages, self_idx)) != ERROR) {
    *op_idx_out = op_idx;
    if (IS_C_BINARY_MATH_OP(M_TOK(messages[op_idx]))) {
      prev_tok = op_idx;
      for (i = op_idx; messages[i]; i--) {
	if (M_ISSPACE(messages[i])) {
	  strcat (operand_out, M_NAME(messages[i]));
	  continue;
	}
	if (M_TOK(messages[i]) == LABEL) {
	  if ((operand_obj = get_object (M_NAME(messages[i]), NULL))
	      != NULL) {
	    ssroe_class_mismatch_warning
	      (messages[self_idx],rcvr_class_obj -> __o_name,
	       operand_obj -> __o_class -> __o_name);
	    /* A token or expression that begins with an object
	       identifier. */
	    if (i == *expr_end_idx_out) {
	      if (str_eq (operand_obj -> __o_class -> __o_name,
			  LONGINTEGER_CLASSNAME)) {
		strcatx2 (operand_out,
			  "*(long long int *)__ctalk_get_object (\"",
			  M_NAME(messages[i]),
			  "\", (void *)0) -> __o_value", NULL);
	      } else if (str_eq (operand_obj -> __o_class -> __o_name,
				 BOOLEAN_CLASSNAME)) {
		strcatx2 (operand_out,
			  "*(int *)__ctalk_get_object (\"",
			  M_NAME(messages[i]),
			  "\", (void *)0) -> __o_value", NULL);
	      } else if (str_eq (operand_obj -> __o_class -> __o_name,
				 FLOAT_CLASSNAME)) {
		strcatx2 (operand_out,
			  "strtod (__ctalk_get_object (\"",
			  M_NAME(messages[i]),
			  "\", (void *)0) -> __o_value, (void *)0)", NULL);
	      } else if (str_eq (operand_obj -> __o_class -> __o_name,
				 CHARACTER_CLASSNAME)) {
		strcatx2 (operand_out,
			  "(__ctalk_get_object (\"",
			  M_NAME(messages[i]),
			  "\", (void *)0) -> instancevars -> __o_value[0] == '\\'' ? ",
			  "__ctalk_get_object (\"",
			  M_NAME(messages[i]),
			  "\", (void *)0) -> instancevars -> __o_value[1] : ",
			  "__ctalk_get_object (\"",
			  M_NAME(messages[i]),
			  "\", (void *)0) -> instancevars -> __o_value[0]) ", NULL);
	      } else {
		/* class of object defaults to Integer */
		strcatx2 (operand_out, "*(int *)__ctalk_get_object (\"",
			  M_NAME(messages[i]), "\", (void *)0) -> __o_value",
			  NULL);
	      }
	    } else {
	      rt_expr_return (messages, keyword_ptr, i, expr_end_idx_out,
			      operand_out_tmp,
			 &have_cvar_reg);
	      fmt_rt_return (operand_out_tmp, rcvr_class_obj -> __o_name,
			     (have_cvar_reg ?
			      OBJTOC_OBJECT_KEEP|OBJTOC_DELETE_CVARS :
			      OBJTOC_OBJECT_KEEP),
			     operand_out_tmp_2);
	      strcatx2 (operand_out, operand_out_tmp_2, NULL);
	      i = *expr_end_idx_out;
	    }
	  } else if ((n_th_param = ret_is_method_param (M_NAME(messages[i])))
		     != ERROR) {
	    /* A token or expression that begins with the parameter label */
	    n_method = new_methods[new_method_ptr + 1] -> method;
	    ssroe_class_mismatch_warning
	      (messages[self_idx],rcvr_class_obj -> __o_name,
	       n_method -> params[n_method -> n_params -
				  n_th_param - 1] -> class);
	    if (i == *expr_end_idx_out) {
	      if (str_eq (n_method -> params[n_method -> n_params -
					     n_th_param - 1] -> class,
			  LONGINTEGER_CLASSNAME)) {
		strcatx2 (operand_out, "*(long long int *)",
			  METHOD_ARG_VALUE_ACCESSOR_FN, " (",
			  ascii[n_method -> n_params - n_th_param - 1],
			  ") -> __o_value", NULL);
	      } else if (str_eq (n_method -> params[n_method -> n_params -
						    n_th_param - 1]
				 -> class, BOOLEAN_CLASSNAME)) {
		strcatx2 (operand_out, "*( int *)",
			  METHOD_ARG_VALUE_ACCESSOR_FN, " (",
			  ascii[n_method -> n_params - n_th_param - 1],
			  ") -> __o_value", NULL);
	      } else if (str_eq (n_method -> params[n_method -> n_params -
						    n_th_param - 1]
				 -> class, FLOAT_CLASSNAME)) {
		strcatx2 (operand_out, "strtod (",
			  METHOD_ARG_VALUE_ACCESSOR_FN, " (",
			  ascii[n_method -> n_params - n_th_param - 1],
			  ") -> __o_value, (void *)0)", NULL);
	      } else if (str_eq (n_method -> params[n_method -> n_params -
						    n_th_param - 1]
				 -> class, CHARACTER_CLASSNAME)) {
		strcatx2 (operand_out,
			  "(", METHOD_ARG_VALUE_ACCESSOR_FN, " (",ascii[n_method -> n_params - n_th_param - 1],") -> __o_value[0] == '\\'' ? ",
			  METHOD_ARG_VALUE_ACCESSOR_FN, " (",ascii[n_method -> n_params - n_th_param - 1],") -> __o_value[1] : ",
			  METHOD_ARG_VALUE_ACCESSOR_FN, " (",ascii[n_method -> n_params - n_th_param - 1],") -> __o_value[0]) ",
			  NULL);
	      } else {
		/* The default class for a parameter is Integer */
		strcatx2 (operand_out, "*(int *)",
			  METHOD_ARG_VALUE_ACCESSOR_FN, " (",
			  ascii[n_method -> n_params - n_th_param - 1],
			  ") -> __o_value", NULL);
	      }
	    } else {
	      rt_expr_return (messages, keyword_ptr, i, expr_end_idx_out,
			      operand_out_tmp,
			      &have_cvar_reg);
	      fmt_rt_return (operand_out_tmp, rcvr_class_obj -> __o_name,
			     (have_cvar_reg ?
			      OBJTOC_OBJECT_KEEP|OBJTOC_DELETE_CVARS :
			      OBJTOC_OBJECT_KEEP),
			     operand_out_tmp_2);
	      strcatx2 (operand_out, operand_out_tmp_2, NULL);
	      i = *expr_end_idx_out;
	    }
	  } else {
	    if (!IS_DEFINED_LABEL(M_NAME(messages[i]))) {
	      warning (messages[i], "Could not resolve label, \"%s.\"",
		       M_NAME(messages[i]));
	    }
	    strcat (operand_out, M_NAME(messages[i]));
	  }
	} else if (M_TOK(messages[i]) == SEMICOLON) {
	  *expr_end_idx_out = prev_tok;
	  return true;
	} else {
	  strcat (operand_out, M_NAME(messages[i]));
	}
	prev_tok = i;
      }
    }
  }
  return false;
}

/*
 *  Prevents generating code within the old methodReturn* 
 *  macros.
 */
static int backward_compatibility_cookie (MESSAGE_STACK messages,
					   int expr_end_idx) {
  if ((M_TOK(messages[expr_end_idx-1]) == SEMICOLON) &&
      (M_TOK(messages[expr_end_idx-2]) == SEMICOLON) &&
      (M_TOK(messages[expr_end_idx-3]) == SEMICOLON))
    return TRUE;
  return FALSE;
}

static char *self_accessor_fn = "__ctalk_self_internal ()";
static char *self_accessor_fn_int =
  "*(int *)__ctalk_self_internal_value () -> __o_value ";
static char *self_accessor_fn_llint =
  "*(long long int *)__ctalk_self_internal_value () -> __o_value ";
static char *self_accessor_fn_bool =
  "*(int *)__ctalk_self_internal_value () -> __o_value ";
static char *self_accessor_fn_float =
  "strtod (__ctalk_self_internal () -> __o_value, (void *)0) ";
static char *self_accessor_fn_char =
  "(__ctalk_self_internal_value () -> __o_value[0] == '\\'' ? __ctalk_self_internal_value () -> __o_value[1] : __ctalk_self_internal_value () -> __o_value[0]) ";

/*
 *  Try to handle anything that begins with "return self ..."
 */
static int simple_self_return_expr (MESSAGE_STACK messages,
				    int keyword_ptr,
				    int expr_start_idx,
				    int expr_end_idx) {
  int expr_end, op_idx, i;
  char expr_buf_out[MAXMSG], operand[MAXMSG], *return_reg_fn;
  METHOD *n_method;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (expr_start_idx == expr_end_idx) {
    if (messages[expr_start_idx] -> attrs & TOK_SELF) {
      /* this is faster at run time than calling __ctalkEvalExpr, 
	 which is what rt_self_expr () would generate. */
      if (make_argblk_expr) {
	  strcpy (g_argblk_expr, self_accessor_fn);
      } else {
	fileout (self_accessor_fn, 0, expr_start_idx);
      }
      ++messages[expr_start_idx] -> evaled;
      ++messages[expr_start_idx] -> output;
      return TRUE;
    }
  } else {
    if (messages[expr_start_idx] -> attrs & TOK_SELF &&
	!strcmp (M_NAME(messages[expr_end_idx]), "value")) {
      if (interpreter_pass != expr_check) {
	(void)rt_self_expr (message_stack (), expr_start_idx, &expr_end,
			    expr_buf_out);
	++messages[expr_start_idx] -> evaled;
	++messages[expr_end_idx] -> evaled;
	return TRUE;
      }
    } else if (ssr_operand_expr (messages, keyword_ptr, expr_start_idx,
				 &op_idx, &expr_end_idx, operand)) {
      *expr_buf_out = 0;

      n_method = new_methods[new_method_ptr+1] -> method;
      if (str_eq (n_method -> returnclass, INTEGER_CLASSNAME))
	return_reg_fn = INT_CONSTANT_RETURN_FN;
      else if (str_eq (n_method -> returnclass, LONGINTEGER_CLASSNAME))
	return_reg_fn = LLINT_CONSTANT_RETURN_FN;
      else if (str_eq (n_method -> returnclass, BOOLEAN_CLASSNAME))
	return_reg_fn = BOOL_CONSTANT_RETURN_FN;
      else if (str_eq (n_method -> returnclass, FLOAT_CLASSNAME))
	return_reg_fn = FLOAT_CONSTANT_RETURN_FN;
      else if (str_eq (n_method -> returnclass, CHARACTER_CLASSNAME))
	return_reg_fn = CHAR_CONSTANT_RETURN_FN;
      else
	return FALSE;

      if (str_eq (rcvr_class_obj -> __o_name, INTEGER_CLASSNAME)) {
	  strcatx (expr_buf_out, return_reg_fn, " (",
		   self_accessor_fn_int, operand, ")", NULL);
      } else if (str_eq (rcvr_class_obj -> __o_name, LONGINTEGER_CLASSNAME)) {
	strcatx (expr_buf_out, return_reg_fn, " (",
		 self_accessor_fn_llint, operand, ")", NULL);
      } else if (str_eq (rcvr_class_obj -> __o_name, BOOLEAN_CLASSNAME)) {
	strcatx (expr_buf_out, return_reg_fn, " (",
		 self_accessor_fn_bool, operand, ")", NULL);
      } else if (str_eq (rcvr_class_obj -> __o_name, FLOAT_CLASSNAME)) {
	strcatx (expr_buf_out, return_reg_fn, " (",
		 self_accessor_fn_float, operand, ")", NULL);
      } else if (str_eq (rcvr_class_obj -> __o_name, CHARACTER_CLASSNAME)) {
	strcatx (expr_buf_out, return_reg_fn, " (",
		 self_accessor_fn_char, operand, ")", NULL);
      } else {
	return FALSE;
      }
      if (make_argblk_expr) {
	strcpy (g_argblk_expr, expr_buf_out);
      } else {
	fileout (expr_buf_out, 0, expr_start_idx);
      }
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
      return TRUE;
    }
  }
  return FALSE;
}

#define IS_SIGN(c) ((c == '-' || c == '+'))

int is_zero (char *buf) {
  int i, idx, bin_exp;

  while (*buf == '\'')
    TRIM_CHAR (buf);

  if (isdigit ((int)buf[0]) || IS_SIGN(buf[0])) {
    switch (radix_of (buf))
      {
      case hexadecimal:
	errno = 0;
	i = strtol (buf, NULL, 16);
	if (errno)
	  _warning ("is_zero: %s : %s\n", buf, strerror (errno));
	break;
      case octal:
	errno = 0;
	i = strtol (buf, NULL, 8);
	if (errno)
	  _warning ("is_zero: %s : %s\n", buf, strerror (errno));
	break;
      case binary:
	for (idx = strlen(buf) - 1, bin_exp = 1, i = 0; idx >= 0; idx--) {
	  if ((buf[idx] == 'b') || (buf[idx] == 'B'))
	    continue;
	  i += (buf[idx] == '1') ? bin_exp: 0;
	  bin_exp *= 2;
	}
	break;
      case decimal:
      default:
	errno = 0;
	i = strtol (buf, NULL, 0);
	if (errno)
	  _warning ("is_zero: %s : %s\n", buf, strerror (errno));
	break;
      }
  } else {
    if (!strcmp (buf, NULLSTR)) {
      i = 0;
    } else {
      if (*buf == '\0') {
	i = 0;
      } else {
	/* Character constant. */
	if (*buf == '\'') {
	  sscanf (buf, "'%c'", (char *)&i);
	} else {
	  _warning ("is_zero: Incompatible return type.\n");
	  i = 0;
	}
      }
    }
  } 
  return !i;
}

static int const_false_return_expr (MESSAGE_STACK messages,
				    int keyword_ptr,
				    int expr_start_idx,
				    int expr_end_idx) {
  char buf[MAXMSG];

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (expr_start_idx == expr_end_idx) {
    if ((M_TOK(messages[expr_start_idx]) == INTEGER) ||
	(M_TOK(messages[expr_start_idx]) == LONG) ||
	(M_TOK(messages[expr_start_idx]) == LONGLONG)) {
      if (is_zero (M_NAME(messages[expr_start_idx]))) {
	/*
	 *  Use the actual token as an Integer object in case what
	 *  the program wants is actually the integer.
	 */
	if (make_argblk_expr) {
	  strcatx (g_argblk_expr, BOOL_CONSTANT_RETURN_FN, " (0)", NULL);
	} else {
	  strcatx (buf, BOOL_CONSTANT_RETURN_FN, " (0)", NULL);
	  fileout (buf, 0, expr_start_idx);
	}
	++messages[expr_start_idx] -> evaled;
	++messages[expr_start_idx] -> output;
	return TRUE;
      }
    }
  }
    return FALSE;
}

/*
 *  Checks for non-zero constant argument as well as the 
 *  TRUE macro, which expands to "!(0)"
 */

static int const_true_return_expr (MESSAGE_STACK messages,
				    int keyword_ptr,
				    int expr_start_idx,
				    int expr_end_idx) {
  char buf[MAXMSG];
  int i;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (expr_start_idx == expr_end_idx) {
    if ((M_TOK(messages[expr_start_idx]) == INTEGER) ||
	(M_TOK(messages[expr_start_idx]) == LONG) ||
	(M_TOK(messages[expr_start_idx]) == LONGLONG)) {
      if (!is_zero (M_NAME(messages[expr_start_idx]))) {
	/*
	 *  Use the actual token as an Integer object in case what
	 *  the program wants is actually the integer.
	 */
	strcatx (buf, basic_constant_return_class_fn
		 (M_TOK(messages[expr_start_idx])), " (",
		 M_NAME(messages[expr_start_idx]), ")", NULL);
	fileout (buf, 0, expr_start_idx);
	++messages[expr_start_idx] -> evaled;
	++messages[expr_start_idx] -> output;
	return TRUE;
      }
    }
  } else {
    /* Handle our definition of TRUE: !(0) */
    if ((M_TOK(messages[expr_start_idx]) == EXCLAM) &&
	(M_TOK(messages[expr_start_idx-1]) == OPENPAREN) &&
	((M_TOK(messages[expr_start_idx-2]) == INTEGER) &&
	 is_zero(M_NAME(messages[expr_start_idx-2]))) &&
	(M_TOK(messages[expr_start_idx-3]) == CLOSEPAREN)) {
      if (make_argblk_expr) {
	strcatx (g_argblk_expr, 
		 basic_constant_return_class_fn
		 (M_TOK(messages[expr_start_idx - 2])), " (1)", NULL);
      } else {
	strcatx (buf, 
		 basic_constant_return_class_fn
		 (M_TOK(messages[expr_start_idx - 2])), " (1)", NULL);
	fileout (buf, 0, expr_start_idx);
      }
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
      return TRUE;
    }
  }
  return FALSE;
}

static int single_tok_return_expr (MESSAGE_STACK messages,
				   int keyword_ptr,
				   int expr_tok_idx) {
  char buf[MAXMSG];
  OBJECT *o;
  switch (M_TOK(messages[expr_tok_idx]))
    {
    case INTEGER:
      if (make_argblk_expr) {
	strcatx (g_argblk_expr,
		 INT_CONSTANT_RETURN_FN, 
		 " (", M_NAME(messages[expr_tok_idx]), ")", NULL);
      } else {
	strcatx (buf,
		 INT_CONSTANT_RETURN_FN,
		 " (", M_NAME(messages[expr_tok_idx]), ")", NULL);
	fileout (buf, 0, expr_tok_idx);
      }
      ++messages[expr_tok_idx] -> evaled;
      ++messages[expr_tok_idx] -> output;
      return TRUE;
      break;
    case LONG:
    case LONGLONG:
      if (make_argblk_expr) {
	strcatx (g_argblk_expr,
		 LLINT_CONSTANT_RETURN_FN, " (",
		 M_NAME(messages[expr_tok_idx]), ")", NULL);
      } else {
	strcatx (buf,
		 LLINT_CONSTANT_RETURN_FN, " (",
		 M_NAME(messages[expr_tok_idx]), ")", NULL);
	fileout (buf, 0, expr_tok_idx);
      }
      ++messages[expr_tok_idx] -> evaled;
      ++messages[expr_tok_idx] -> output;
      return TRUE;
      break;
    case FLOAT:
      if (make_argblk_expr) {
	strcatx (g_argblk_expr,
		 FLOAT_CONSTANT_RETURN_FN, " (",
		 M_NAME(messages[expr_tok_idx]), ")", NULL);
      } else {
	strcatx (buf,
		 FLOAT_CONSTANT_RETURN_FN, " (",
		 M_NAME(messages[expr_tok_idx]), ")", NULL);
	fileout (buf, 0, expr_tok_idx);
      }
      ++messages[expr_tok_idx] -> evaled;
      ++messages[expr_tok_idx] -> output;
      return TRUE;
      break;
    case LITERAL_CHAR:
      if (make_argblk_expr) {
	strcatx (g_argblk_expr,
		 CHAR_CONSTANT_RETURN_FN,
		 " (", M_NAME(messages[expr_tok_idx]), ")", NULL);
      } else {
	strcatx (buf,
		 CHAR_CONSTANT_RETURN_FN,
		 " (", M_NAME(messages[expr_tok_idx]), ")", NULL);
	fileout (buf, 0, expr_tok_idx);
      }
      ++messages[expr_tok_idx] -> evaled;
      ++messages[expr_tok_idx] -> output;
      return TRUE;
      break;
    case LITERAL:
      if (make_argblk_expr) {
	strcatx (g_argblk_expr,
		 CHAR_PTR_CONSTANT_RETURN_FN,
		 " (", M_NAME(messages[expr_tok_idx]), ")", NULL);
      } else {
	strcatx (buf,
		 CHAR_PTR_CONSTANT_RETURN_FN,
		 " (", M_NAME(messages[expr_tok_idx]), ")", NULL);
	fileout (buf, 0, expr_tok_idx);
      }
      ++messages[expr_tok_idx] -> evaled;
      ++messages[expr_tok_idx] -> output;
      return TRUE;
      break;
    case LABEL:
      if (messages[expr_tok_idx] -> attrs & TOK_SELF) {
	if (make_argblk_expr) {
	  strcatx (g_argblk_expr, SELF_ACCESSOR_FN, " ()", NULL);
	} else {
	  strcatx (buf, SELF_ACCESSOR_FN, " ()", NULL);
	  fileout (buf, 0, expr_tok_idx);
	}
	++messages[expr_tok_idx] -> evaled;
	++messages[expr_tok_idx] -> output;
	return TRUE;
      } else if (((o = get_object (M_NAME(messages[expr_tok_idx]),NULL))
		 != NULL) ||
		 ((o = get_method_param_obj (messages, expr_tok_idx))
		  != NULL)) {
	if (make_argblk_expr) {
	  strcatx (g_argblk_expr, OBJECT_TOK_RETURN_FN, " (\"",
		   M_NAME(messages[expr_tok_idx]), "\", ((void *)0))", 
		   NULL);
	} else {
	  strcatx (buf, OBJECT_TOK_RETURN_FN, " (\"",
		   M_NAME(messages[expr_tok_idx]), "\", ((void *)0))", 
		   NULL);
	  fileout (buf, 0, expr_tok_idx);
	}
	++messages[expr_tok_idx] -> evaled;
	++messages[expr_tok_idx] -> output;
	return TRUE;
      }
      break;
    }
  return FALSE;
}

/*
 *  Warn if a method tries to return an array without an 
 *  Array return class, unless the return class is String and 
 *  the array is declared as, char[<size>].
 */
static void array_class_return_warning (MESSAGE *orig, METHOD *m, CVAR *c) {

  /* if (!strcmp (c -> type, "char") &&  */
  if ((c -> type_attrs & CVAR_TYPE_CHAR) &&
      (c -> n_derefs == 1)) {
    if ((*(m -> returnclass))  && 
	(!strcmp (m -> returnclass, STRING_CLASSNAME)))
      return;
    if (rcvr_class_obj && 
	!strcmp (rcvr_class_obj -> __o_classname, 
		 STRING_CLASSNAME))
      return;
  }

  if (c -> n_derefs > 1) {
    warning (orig,
	     "Method, \"%s,\" returns array, \"%s,\" which has more than one subscript.",
	     m -> name, c -> name);
  }

  if (*(m -> returnclass)) {
    if (strcmp (m -> returnclass, ARRAY_CLASSNAME)) {
      warning (orig,
	       "Method, \"%s,\" returns array, \"%s,\" while its return class is defined as \"%s.\"",
	       m -> name, c -> name, m -> returnclass);
    } else {
      if (rcvr_class_obj) {
	if (strcmp (rcvr_class_obj -> __o_name,
		    ARRAY_CLASSNAME)) {
	  warning (orig,
   "Method, \"%s,\" returns array, \"%s,\" while its return class is defined as, \"%s.\"",
		   m -> name, c -> name, rcvr_class_obj -> __o_name);
	}
      }
    }
  }
}

static void cvar_token_single_token_output (MESSAGE_STACK messages,
					    int tok_idx, 
					    char *trans_fn_name) {
  char buf[MAXMSG];
  if (make_argblk_expr) {
    strcatx (g_argblk_expr, trans_fn_name, 
	     "(", M_NAME(messages[tok_idx]), ")", NULL);
  } else {
    strcatx (buf, trans_fn_name, "(", M_NAME(messages[tok_idx]), ")", NULL);
    fileout (buf, 0, tok_idx);
  }
  ++messages[tok_idx] -> evaled;
  ++messages[tok_idx] -> output;
}

static void cvar_token_typecast_output (MESSAGE_STACK messages,
					int start_idx, int end_idx, 
					char *trans_fn_name) {
  char *t;
  char buf[MAXMSG];
  int i;
  t = collect_tokens (messages, start_idx, end_idx);
  if (make_argblk_expr) {
    strcatx (g_argblk_expr, trans_fn_name, " (", t, ")", NULL);
  } else {
    strcatx (buf, trans_fn_name, " (", t, ")", NULL);
    fileout (buf, 0, start_idx);
  }
  __xfree (MEMADDR(t));
  for (i = start_idx; i >= end_idx; --i) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
}

static int cvar_token_return_expr (MESSAGE_STACK messages,
				    int keyword_ptr,
				    int expr_start_idx,
				    int expr_end_idx) {
  char buf[MAXMSG], c_buf[MAXMSG], *s_ptr;
  CVAR *c;
  char *t;
  int typecast_end_idx = -1,
    tok_start_idx;  /* If there's a typecast expression, then
		       this points to the label of the variable
		       we're returning.  If there's no typecast,
		       then this is the same as expr_start_idx. */
  bool leading_typecast;
  char intbuf[0xff];

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (backward_compatibility_cookie (messages, expr_end_idx))
    return FALSE;

  if (is_typecast_expr (messages, expr_start_idx, &typecast_end_idx)) {
    tok_start_idx = nextlangmsg (messages, typecast_end_idx);
    leading_typecast = true;
  } else {
    tok_start_idx = expr_start_idx;
    leading_typecast = false;
  }

  if (tok_start_idx == expr_end_idx) {
    if (M_TOK(messages[tok_start_idx]) == LABEL) {

      if (((c = get_local_var (M_NAME(messages[tok_start_idx]))) != NULL)||
	  ((c = get_global_var(M_NAME(messages[tok_start_idx]))) != NULL)){
	/*
	 *  Here we can re-use the dedicated functions for C constants 
	 *  for each basic class if there is one, which is a lot quicker
	 *  than just registering everything then translating at run time.
	 *  This should probably be in its own section later on.  TODO - 
	 *  Issue a warning if a basic type doesn't match the method's
	 *  return type.  Note that OBJECT * vars still need to be
	 *  registered as method args and saved with 
	 *  __ctalkSaveCVARResource - TODO-maybe - create a purpose-built
	 *  libctalk API function for this.
	 *  
	 */
	if (c -> type_attrs == CVAR_TYPE_BOOL && c -> n_derefs == 0) {
	  if (leading_typecast) {
	    cvar_token_typecast_output (messages, expr_start_idx,
					expr_end_idx,
					BOOL_CONSTANT_RETURN_FN);
	  } else {
	    cvar_token_single_token_output (messages, tok_start_idx,
					    BOOL_CONSTANT_RETURN_FN);
	  }
	  return TRUE;
	} else if (c -> type_attrs == CVAR_TYPE_INT && c -> n_derefs == 0) {
	  if (leading_typecast) {
	    cvar_token_typecast_output (messages, expr_start_idx,
					expr_end_idx,
					INT_CONSTANT_RETURN_FN);
	  } else {
	    cvar_token_single_token_output (messages, tok_start_idx,
					    INT_CONSTANT_RETURN_FN);
	  }
	  return TRUE;
	} else if (c -> type_attrs == CVAR_TYPE_CHAR && c -> n_derefs == 0) {
	  if (leading_typecast) {
	    cvar_token_typecast_output (messages, expr_start_idx,
					expr_end_idx,
					CHAR_CONSTANT_RETURN_FN);
	  } else {
	    cvar_token_single_token_output (messages, tok_start_idx,
					    CHAR_CONSTANT_RETURN_FN);
	  }
	  return TRUE;
	} else if ((c -> type_attrs == CVAR_TYPE_CHAR && 
		    c -> n_derefs == 1) && 
		   !(c ->  attrs & CVAR_ATTR_ARRAY_DECL))  {
	  if (leading_typecast) {
	    cvar_token_typecast_output (messages, expr_start_idx,
					expr_end_idx,
					CHAR_PTR_CONSTANT_RETURN_FN);
	  } else {
	    cvar_token_single_token_output (messages, tok_start_idx,
					    CHAR_PTR_CONSTANT_RETURN_FN);
	  }
	  return TRUE;
	} else if (c -> type_attrs == CVAR_TYPE_FLOAT && c -> n_derefs == 0) {
	  if (leading_typecast) {
	    cvar_token_typecast_output (messages, expr_start_idx,
					expr_end_idx,
					FLOAT_CONSTANT_RETURN_FN);
	  } else {
	    cvar_token_single_token_output (messages, tok_start_idx,
					    FLOAT_CONSTANT_RETURN_FN);
	  }
	  return TRUE;
	} else if (c -> type_attrs == CVAR_TYPE_LONGLONG &&
		   c -> n_derefs == 0) {
	  if (leading_typecast) {
	    cvar_token_typecast_output (messages, expr_start_idx,
					expr_end_idx,
					LLINT_CONSTANT_RETURN_FN);
	  } else {
	    cvar_token_single_token_output (messages, tok_start_idx,
					    LLINT_CONSTANT_RETURN_FN);
	  }
	  return TRUE;
	}

	fmt_register_c_method_arg_call 
	  (c, M_NAME(messages[expr_end_idx]), LOCAL_VAR, c_buf);
	/* 
	 * The fmt_register_c_method_arg_call adds a semicolon at the
	 * end of the function.  Remove it in this case.
	 */
	if ((s_ptr = strchr (c_buf, ';')) != NULL)
	  *s_ptr = '\0';
  	if (c -> attrs & CVAR_ATTR_ARRAY_DECL) {
	  array_class_return_warning 
	    (messages[expr_start_idx], 
	     new_methods[new_method_ptr + 1] -> method,
	     c); 
	  if (leading_typecast) {
	    t = collect_tokens (messages, expr_start_idx, expr_end_idx);
	    strcatx (buf, "(", c_buf, ", ", CVAR_ARRAY_TOK_RETURN_FN, " (\"",
		     M_NAME(messages[tok_start_idx]), "\", ",
		     ctitoa (c -> initializer_size, intbuf), ", &",
		     t, "))", NULL);
	    __xfree (MEMADDR(t));
	  } else {
	    strcatx (buf, "(", c_buf, ", ", CVAR_ARRAY_TOK_RETURN_FN, " (\"",
		     M_NAME(messages[expr_start_idx]), "\", ",
		     ctitoa (c -> initializer_size, intbuf), ", &",
		     M_NAME(messages[expr_start_idx]), "))", NULL);
	  }
  	} else {  /* if (c -> attrs & CVAR_ATTR_ARRAY_DECL) */
	  if (leading_typecast) {
	    t = collect_tokens (messages, expr_start_idx, expr_end_idx);
	    strcatx (buf, "(", c_buf, ", ", CVAR_ARRAY_TOK_RETURN_FN, " (\"",
		     M_NAME(messages[tok_start_idx]), "\", ",
		     ctitoa (c -> initializer_size, intbuf), ", &",
		     t, "))", NULL);
	    __xfree (MEMADDR(t));
	  } else {
	    strcatx (buf, "(", c_buf, ", " CVAR_TOK_RETURN_FN, "(\"",
		     M_NAME(messages[tok_start_idx]), "\"))", NULL);
	  }
  	}
	if (make_argblk_expr) {
	  strcpy (g_argblk_expr, buf);
	} else {
	  fileout (buf, 0, expr_start_idx);
	}
	++messages[expr_start_idx] -> evaled;
	++messages[expr_start_idx] -> output;
	return TRUE;
      }
    }
  }
  return FALSE;
}

static EXPR_FN_REC *expr_fns[MAXARGS];
static int expr_fns_ptr = 0;

/* remember to keep these local to one stack - we can't
   define a MESSAGE_STACK member for this in cvar.h without
   multiply including .h files. */
static EXPR_FN_REC *new_expr_fn_rec (int stack_idx,
				    CFUNC *fn) {
  static EXPR_FN_REC *e;
  if ((e = __xalloc (sizeof (struct _expr_fn_rec))) == NULL) {
    _error ("new_expr_fn_rec: %s.\n", strerror (errno));
  }
  e -> stack_idx = stack_idx;
  e -> cfunc = fn;
  return e;
}

/* Also checks for nested functions. */
static bool have_object_in_arg (MESSAGE_STACK messages,
				int arg_start_idx,
				int arg_end_idx) {
  int i;
  for (i = arg_start_idx; i >= arg_end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if (messages[i] -> attrs & TOK_SELF ||
	  messages[i] -> attrs & TOK_SUPER) 
	return true;
      if (get_object (M_NAME(messages[i]), NULL))
	return true;
      if (get_function (M_NAME(messages[i]))) {
	error (messages[i], "Nested function, \"%s\", is not "
	       "(yet) supported in this context.", M_NAME(messages[i]));
      }
      if (interpreter_pass == method_pass) {
	if (is_method_parameter (messages, i))
	  return true;
      }
    }
  }
  return false;
}

static int crfa_scan_args (MESSAGE_STACK messages,
			   int fn_label_idx,
			   int expr_end_idx,
			   int stack_top_idx,
			   CFUNC *fn_cfunc,
			   int *arg_indexes,
			   int *n_arg_indexes) {
  int arg_start_ptr, arg_end_ptr;
  char *err_buf;
  
  memset (arg_indexes, '\0', MAXARGS * sizeof (int));
  *n_arg_indexes = 0;
  
  if ((arg_start_ptr = scanforward (messages, fn_label_idx, stack_top_idx,
				    OPENPAREN))
      == ERROR) {
    err_buf = collect_tokens (messages, fn_label_idx, expr_end_idx);
    warning (messages[fn_label_idx],
	     "Opening parenthesis not found in expression:\n\t%s\n",
	     err_buf);
    free (err_buf);
    return -1;
  }
  if ((arg_end_ptr = match_paren (messages, arg_start_ptr, stack_top_idx))
      == ERROR) {
    err_buf = collect_tokens (messages, fn_label_idx, expr_end_idx);
    warning (messages[fn_label_idx],
	     "Closing parenthesis not found in expression:\n\t%s\n",
	     err_buf);
    free (err_buf);
    return -1;
  }
  split_args_idx (messages, arg_start_ptr, arg_end_ptr, 
		  arg_indexes, n_arg_indexes);
  return SUCCESS;
}

static char *check_return_fn_args (MESSAGE_STACK messages,
				  int expr_start_idx,
				  int expr_end_idx,
				  CFUNC *fn_cfunc) {
  int stack_top_idx;
  int arg_indexes[MAXARGS], n_arg_indexes, n_th_arg;
  int i;
  int end_idx;
  static char buf[MAXMSG];
  char arg_expr_buf1[MAXMSG], arg_expr_buf2[MAXMSG];
  CVAR *fn_param;
  CFUNC *fn_cfunc_next;
  
  memset (buf, '\0', MAXMSG);
  n_th_arg = 0;

  stack_top_idx = get_stack_top (messages);
  if (crfa_scan_args (messages, expr_start_idx, expr_end_idx, stack_top_idx,
		      fn_cfunc, arg_indexes, &n_arg_indexes) < 0)
    return NULL;

  n_th_arg = 0;
  fn_param = fn_cfunc -> params;

  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (i == arg_indexes[n_th_arg * 2]) {
      if (have_object_in_arg (messages, arg_indexes[n_th_arg * 2],
			      arg_indexes[n_th_arg * 2 + 1])) {
	fmt_rt_expr (messages, arg_indexes[n_th_arg * 2], &end_idx,
		     arg_expr_buf1);
	fmt_rt_return (arg_expr_buf1,
		       basic_class_from_cvar (messages[expr_start_idx],
						fn_param, 0),
		       TRUE, arg_expr_buf2);
	strcatx2 (buf, arg_expr_buf2, NULL);
	i = arg_indexes[n_th_arg * 2 + 1];
	++n_th_arg;
	fn_param = fn_param -> next;
	continue;
      } else {
	strcatx2 (buf, M_NAME(messages[i]), NULL);
      }
    } else if ((M_TOK(messages[i]) == LABEL) &&
	       (i < expr_start_idx) && 
	       ((fn_cfunc_next = get_function (M_NAME(messages[i])))
		!= NULL)) {
      if (crfa_scan_args (messages, i, expr_end_idx, stack_top_idx,
		      fn_cfunc_next, arg_indexes, &n_arg_indexes) < 0)
	return NULL;
      n_th_arg = 0;
      fn_param = fn_cfunc_next -> params;
      strcatx2 (buf, M_NAME(messages[i]), NULL);
    } else {
      strcatx2 (buf, M_NAME(messages[i]), NULL);
    }
    if (i == arg_indexes[n_th_arg * 2 + 1]) {
      ++n_th_arg;
      fn_param = fn_param -> next;
    }

  }
  return buf;
}

/* like above, but uses the expr_fns stack */
static char *check_return_fn_args_2 (MESSAGE_STACK messages,
				     int expr_start_idx,
				     int expr_end_idx) {
  int stack_top_idx;
  int arg_indexes[MAXARGS], n_arg_indexes, n_th_arg;
  int i;
  int end_idx;
  int l_fn_idx = 0;
  static char buf[MAXMSG];
  char arg_expr_buf1[MAXMSG], arg_expr_buf2[MAXMSG];
  CVAR *fn_param;
  
  memset (buf, '\0', MAXMSG);
  n_th_arg = 0;

  stack_top_idx = get_stack_top (messages);
  if (crfa_scan_args (messages, expr_start_idx, expr_end_idx,
		      stack_top_idx,
		      expr_fns[l_fn_idx] -> cfunc,
		      arg_indexes, &n_arg_indexes) < 0)
    return NULL;

  n_th_arg = 0;
  fn_param = expr_fns[l_fn_idx] -> cfunc -> params;
  
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (i == arg_indexes[n_th_arg * 2]) {
      if (have_object_in_arg (messages, arg_indexes[n_th_arg * 2],
			      arg_indexes[n_th_arg * 2 + 1])) {
	fmt_rt_expr (messages, arg_indexes[n_th_arg * 2], &end_idx,
		     arg_expr_buf1);
	fmt_rt_return (arg_expr_buf1,
		       basic_class_from_cvar (messages[expr_start_idx],
						fn_param, 0),
		       TRUE, arg_expr_buf2);
	strcatx2 (buf, arg_expr_buf2, NULL);
	i = arg_indexes[n_th_arg * 2 + 1];
	++n_th_arg;
	fn_param = fn_param -> next;
	continue;
      } else {
	strcatx2 (buf, M_NAME(messages[i]), NULL);
      }
    } else if ((M_TOK(messages[i]) == LABEL) &&
	       (i < expr_start_idx) && 
	       (expr_fns[l_fn_idx] ?
		(i == expr_fns[l_fn_idx] -> stack_idx) : false)) {
      if (crfa_scan_args (messages, i, expr_end_idx, stack_top_idx,
		      expr_fns[l_fn_idx] -> cfunc,
			  arg_indexes, &n_arg_indexes) < 0)
	return NULL;
      n_th_arg = 0;
      fn_param = expr_fns[l_fn_idx] -> cfunc -> params;
      strcatx2 (buf, M_NAME(messages[i]), NULL);
    } else {
      strcatx2 (buf, M_NAME(messages[i]), NULL);
    }
    if (i == arg_indexes[n_th_arg * 2 + 1]) {
      ++n_th_arg;
      fn_param = fn_param -> next;
    }
    if (i == arg_indexes[n_arg_indexes - 1])
      ++l_fn_idx;

  }
  return buf;
}

/*
 *  Expressions of the form: 
 *    <cvar|fn> -> mbr [ -> mbr]*
 *    <cvar|fn> . mbr [ . mbr ]*
 *    ... etc.
 *
 *  Expressions can start with type casts, but they are handled 
 *  below.
 */

static int deref_expr_return_expr (MESSAGE_STACK messages,
				   int keyword_ptr,
				   int expr_start_idx,
				   int expr_end_idx) {
  int last_op_token_idx;
  CVAR *label_cvar, *type_cvar = NULL, /* Avoid a warning */
    *member_cvar;
  CFUNC *label_cfunc;
  char *return_expr = NULL, *return_classname, buf[MAXMSG];
  int i;
  char obj_arg_buf[MAXMSG] = "";

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (is_struct_or_union_expr (messages, 
			       expr_start_idx,
			       stack_start (messages),
			       get_stack_top (messages))) {
    if (M_TOK(messages[expr_end_idx]) == LABEL) {
      if ((last_op_token_idx=prevlangmsg (messages, expr_end_idx)) != ERROR){
	if ((M_TOK(messages[last_op_token_idx]) == DEREF) ||
	    (M_TOK(messages[last_op_token_idx]) == PERIOD)) {
	  if ((label_cfunc = get_function (M_NAME(messages[expr_start_idx])))
	      != NULL) {
	    if (((label_cvar = get_local_var (label_cfunc -> return_type))
		 == NULL) &&
		((label_cvar = get_global_var (label_cfunc -> return_type))
		 == NULL)) {
	      if ((type_cvar = get_typedef (label_cfunc -> return_type)) 
		  == NULL) {
		warning (messages[keyword_ptr], 
			 "Unsupported return type %s.",
			 label_cfunc -> return_type);
		return FALSE;
	      }
	    }
	  } else {/*if ((label_cfunc = get_function ... */
	    if (((label_cvar = 
		  get_local_var (M_NAME(messages[expr_start_idx])))
		 == NULL) &&
		((label_cvar = 
		  get_global_var (M_NAME(messages[expr_start_idx])))
		 == NULL)) {
	      if ((type_cvar = get_typedef (M_NAME(messages[expr_start_idx])))
		  == NULL) {

		/* An object with a deref operator can get handled later. */
		if (!(messages[expr_start_idx] -> attrs & TOK_SELF) &&
		    !get_object (M_NAME(messages[expr_start_idx]), NULL))
		  unsupported_return_type_warning_a (messages, expr_start_idx);
		return FALSE;
	      }
	    } else {
	      type_cvar = get_typedef (label_cvar -> type);
	    }
	  }
	  if (label_cfunc != NULL) {
	    strcpy (obj_arg_buf,
		    check_return_fn_args (messages,
					 expr_start_idx,
					 expr_end_idx, label_cfunc));
	  } else {
	    return_expr = collect_tokens (messages, expr_start_idx,
					  expr_end_idx);
	  }
	  if ((member_cvar = 
	       struct_member_from_expr_b (messages, 
					expr_start_idx,
					expr_end_idx,
					type_cvar)) == NULL) {
	    if (return_expr) {
	      warning (messages[expr_start_idx], 
		       "Could not find member definition for expression $s.",
		       return_expr);
	      __xfree (MEMADDR(return_expr));
	    } else {
	      warning (messages[expr_start_idx], 
		       "Could not find member definition for expression $s.",
		       obj_arg_buf);
	    }
	    return FALSE;
	  }
	  return_classname = basic_class_from_cvar
	    (messages[expr_start_idx], member_cvar, 0);
	  if (return_expr != NULL) {
	    strcatx (buf,
		     deref_terminal_return_fn (return_classname),
		     " (", return_expr, ")", NULL);
	    __xfree (MEMADDR(return_expr));
	  } else {
	    strcatx (buf,
		     deref_terminal_return_fn (return_classname),
		     " (", obj_arg_buf, ")", NULL);
	  }
	  if (make_argblk_expr) {
	    strcpy (g_argblk_expr, buf);
	  } else {
	    fileout (buf, 0, expr_start_idx);
	  }
	  for (i = expr_start_idx; i >= expr_end_idx; i--) {
	    ++messages[i] -> evaled;
	    ++messages[i] -> output;
	  } 
	  return TRUE;
	} /* if ((M_TOK(messages[last_op_token_idx]) == DEREF) ... */
      }
    }
  }
  return FALSE;
}

/* Check if the function is also the name of some object.
   If so, then look for the start of the argument list after
   the label before quitting and returning false. */
static bool cfre_label_is_object (MESSAGE_STACK messages,
				  int idx) {
  int lookahead;
  if (get_method_param_obj (messages, idx)) {
    warning (messages[idx], "Label, \"%s\", shadows a C function.",
	     M_NAME(messages[idx]));
    if ((lookahead = nextlangmsg (messages, idx)) != ERROR) {
      if (M_TOK(messages[lookahead]) == OPENPAREN) {
	return false;
      } else {
	return true;
      }
    }
  } else if (get_global_object (M_NAME(messages[idx]), NULL) ||
	     get_local_object (M_NAME(messages[idx]), NULL)) {
    warning (messages[idx], "Label, \"%s\", shadows a C function.",
	     M_NAME(messages[idx]));
    if ((lookahead = nextlangmsg (messages, idx)) != ERROR) {
      if (M_TOK(messages[lookahead]) == OPENPAREN) {
	return false;
      } else {
	return true;
      }
    }
  }
  return false;
}

static int c_function_return_expr (MESSAGE_STACK messages,
				     int keyword_ptr,
				     int expr_start_idx,
				     int expr_end_idx) {
  CFUNC *c_fn;
  char buf[MAXMSG], buf2[MAXMSG], *err_buf;
  int i;
  int arg_start_idx, arg_end_idx, next_tok_idx; 
  int stack_top_idx;
  bool multiple_fns = false;
  char *fn_return_class, *return_class;
  METHOD *new_method;
  EXPR_FN_REC *e;
  
  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return  ERROR;

  if ((c_fn = get_function (M_NAME(messages[expr_start_idx])))
      == NULL) {
    return FALSE;
  } else {
    /* Do some sanity checks first */
    if (expr_start_idx == expr_end_idx)
      return FALSE;
    else if (cfre_label_is_object (messages, expr_start_idx))
      return FALSE;
  }

  if ((new_method = new_methods[new_method_ptr+1] -> method) == NULL) {
    return ERROR;
  }

  e = new_expr_fn_rec (expr_start_idx, c_fn);
  expr_fns[expr_fns_ptr++] = e;

  for (i = expr_start_idx - 1; i >= expr_end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if ((c_fn = get_function (M_NAME(messages[i]))) != NULL) {
	if (cfre_label_is_object (messages, i))
	  return FALSE;
	e = new_expr_fn_rec (i, c_fn);
	expr_fns[expr_fns_ptr++] = e;
	multiple_fns = true;
      }
    }
  }

  strcpy (buf,
	  check_return_fn_args_2
	  (messages, expr_start_idx, expr_end_idx));
  if (!str_eq (expr_fns[0] -> cfunc -> return_type, "OBJECT") || 
      (expr_fns[0] -> cfunc -> return_derefs != 1)) {
    fn_return_class = basic_class_from_cfunc
      (messages[expr_start_idx], expr_fns[0] -> cfunc, 0);
    /* 
     *  1. If we have multiple functions in the return expresssion, 
     *     use the declared return class.
     *  2. If the method declares a different return class from
     *     the receiver class, use it as the return class.
     *  3. If the method doesn't declare a  return class, use
     *     the function's return class.
     */

    if (multiple_fns) {
      return_class = new_method -> returnclass;
    } else {
      if (!str_eq (new_method -> returnclass,
		   rcvr_class_obj -> __o_class -> __o_name)) {
	return_class = new_method -> returnclass;
      } else {
	return_class = fn_return_class;
      }
    }

    if (str_eq (return_class, "Integer") ||
	str_eq (return_class, "Boolean") ||
	str_eq (return_class, "Character")) {
      strcatx (buf2, "__ctalkCIntToObj (", buf, ")", NULL);
    } else if (str_eq (return_class, "Float")) {
      strcatx (buf2, "__ctalkCDoubleToObj (", buf, ")", NULL);
    } else if (str_eq (return_class, "String")) {
      strcatx (buf2, "__ctalkCCharPtrToObj (", buf, ")", NULL);
    } else if (str_eq (return_class, "LongInteger")) {
      strcatx (buf2, "__ctalkCLongLongToObj (", buf, ")", NULL);
    } else {
      warning (messages[expr_start_idx],
	       "c_function_return_expr: Unimplemented return class %s.  "
	       "Class defaulting to Integer.",
	       return_class);
      strcatx (buf2, "__ctalkCIntToObj (", buf, ")", NULL);
    }
    strcpy (buf, buf2);
  }

  if (make_argblk_expr) {
    strcpy (g_argblk_expr, buf);
  } else {
    fileout (buf, 0, expr_start_idx);
  }
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  } 

  stack_top_idx = get_stack_top (messages);

  arg_start_idx = scanforward (messages, expr_start_idx, 
			       stack_top_idx,
			       OPENPAREN);
  arg_end_idx = match_paren (messages, arg_start_idx, stack_top_idx);
  if ((next_tok_idx = nextlangmsgstack (messages, arg_end_idx)) != ERROR) {
    if (M_TOK(messages[next_tok_idx]) == LABEL) {
      err_buf = collect_tokens (messages, expr_start_idx, expr_end_idx);
      error (messages[expr_start_idx],
       "Message, \"%s,\" in this context is not (yet) supported.\n\t%s\n",
	       M_NAME(messages[next_tok_idx]),
	       err_buf);
      free (err_buf);
    }
  }

  --expr_fns_ptr;
  for (; expr_fns_ptr >= 0; expr_fns_ptr--) {
    __xfree (MEMADDR(expr_fns[expr_fns_ptr]));
    expr_fns[expr_fns_ptr] = NULL;
  }
  expr_fns_ptr = 0;

  return TRUE;
}

static int subscript_expr_return_expr (MESSAGE_STACK messages,
				       int keyword_ptr,
				       int expr_start_idx,
				       int expr_end_idx) {
  int last_op_token_idx;
  CVAR *label_cvar;
  CFUNC *label_cfunc;
  char *return_expr, *return_classname, buf[MAXMSG];
  int i;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if ((last_op_token_idx = 
       is_subscript_expr (messages, 
			  expr_start_idx,
			  get_stack_top (messages))) > 0) {
    if ((label_cfunc = get_function (M_NAME(messages[expr_start_idx])))
	!= NULL) {
      if (((label_cvar = get_local_var (label_cfunc -> return_type))
	   == NULL) &&
	  ((label_cvar = get_global_var (label_cfunc -> return_type))
	   == NULL)) {
	warning (messages[keyword_ptr],
		 "Unsupported type %s for %s.",
		 label_cvar -> type,
		 M_NAME(messages[expr_start_idx]));
	return FALSE;
      }
    } else {/*if ((label_cfunc = get_function ... */
      if (((label_cvar = 
	    get_local_var (M_NAME(messages[expr_start_idx])))
	   == NULL) &&
	  ((label_cvar = 
	    get_global_var (M_NAME(messages[expr_start_idx])))
	   == NULL)) {
	warning (messages[keyword_ptr],
		 "Unsupported type %s for %s.",
		 label_cvar -> type,
		 M_NAME(messages[expr_start_idx]));
      }
    }
    return_expr = collect_tokens (messages, expr_start_idx,
				  expr_end_idx);
    return_classname = basic_class_from_cvar
      (messages[expr_start_idx], label_cvar, 1);
    if (make_argblk_expr) {
      strcatx (g_argblk_expr, deref_terminal_return_fn (return_classname), " (",
	       return_expr, ")", NULL);
    } else {
      strcatx (buf, deref_terminal_return_fn (return_classname), " (",
	       return_expr, ")", NULL);
      fileout (buf, 0, expr_start_idx);
    }
    __xfree (MEMADDR(return_expr));
    for (i = expr_start_idx; i >= expr_end_idx; i--) {
      ++messages[i] -> evaled;
      ++messages[i] -> output;
    } 
    return TRUE;
  }
  return FALSE;
}

/*
 *  C-only expressions that return int.  Expressions that 
 *  contain objects get evaluated separately.
 */
static int int_expr_return_expr (MESSAGE_STACK messages,
				 int keyword_ptr,
				 int expr_start_idx,
				 int expr_end_idx) {
  char *s, buf[MAXLABEL];
  CVAR *c;
  CFUNC *fn = NULL;
  int i, after_prefix_tok_idx, typecast_end_idx = -1;
  int fn_idx = keyword_ptr;
  OBJECT *fn_arg_object;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  /*
   *  Format expressions with leading type casts immediately.
   *
   */
  if (is_typecast_expr (messages, expr_start_idx, &typecast_end_idx)) {
    if (strcmp (basic_class_from_typecast (messages, expr_start_idx,
					   typecast_end_idx), 
		INTEGER_CLASSNAME)) {
      return FALSE;
    } else {
      s = collect_tokens (messages, expr_start_idx, expr_end_idx);
      if (make_argblk_expr) {
	strcatx (g_argblk_expr, basic_constant_return_class_fn (INTEGER), " (",
		 s, ")", NULL);
      } else {
	strcatx (buf, basic_constant_return_class_fn (INTEGER), " (",
		 s, ")", NULL);
	fileout (buf, 0, expr_start_idx);
      }
      __xfree (MEMADDR(s));
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      } 
      return TRUE;
    }
  }

  if (IS_C_UNARY_MATH_OP(M_TOK(messages[expr_start_idx]))) {
    after_prefix_tok_idx = nextlangmsg (messages, expr_start_idx);
  } else {
    after_prefix_tok_idx = expr_start_idx;
  }

  if (((c = get_local_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL) ||
      ((c = get_global_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL)) {
    if (!(c -> type_attrs & CVAR_TYPE_INT))
      return FALSE;
    if (*(c -> qualifier) || *(c -> qualifier2) || *(c -> qualifier3)
	|| (c -> qualifier4))
      return FALSE;
    if (c -> n_derefs) return FALSE;
  } else {
    if ((fn = get_function (M_NAME(messages[after_prefix_tok_idx]))) != NULL) {
      if (!(fn -> return_type_attrs & CVAR_TYPE_INT))
	return FALSE;
      if (fn -> return_type_attrs & CVAR_TYPE_LONG)
	return FALSE;
      if (fn -> return_derefs)
	return FALSE;
      fn_idx = after_prefix_tok_idx;
    }
  }
  if (!c && !fn)
    return FALSE;

  if (fn) {
    int arg_start_idx, arg_end_idx;
    char buf2[MAXMSG], *s_tail;
    fn_arg_object = fn_arg_expression 
      (rcvr_class_obj, new_methods[new_method_ptr+ 1] -> method,
       messages, fn_idx);
				       
    arg_start_idx = scanforward (messages, expr_start_idx,
				 expr_end_idx, OPENPAREN);
    arg_end_idx = match_paren (messages, arg_start_idx,
			       expr_end_idx);

    s = collect_tokens (messages, expr_start_idx, arg_start_idx);
    if (arg_end_idx == expr_end_idx)
      s_tail = messages[arg_end_idx] -> name;
    else
      s_tail = collect_tokens (messages, arg_end_idx, expr_end_idx);
    if (*fn_arg_object -> __o_name) {
      strcatx (buf, s, fn_arg_object -> __o_name, s_tail, NULL);
    } else {
      strcatx (buf, s, s_tail, NULL);
    }
    if (make_argblk_expr) {
      strcatx (g_argblk_expr, basic_constant_return_class_fn (INTEGER), 
	       " (", buf, ")", NULL);
    } else {
      strcatx (buf2, basic_constant_return_class_fn (INTEGER), " (", buf, ")",
	       NULL);
      fileout (buf2, 0, expr_start_idx);
    }
    __xfree (MEMADDR(s));
    if (arg_end_idx < expr_end_idx)
      __xfree (MEMADDR(s_tail));
  } else {
    s = collect_tokens (messages, expr_start_idx, expr_end_idx);
    if (make_argblk_expr) {
      strcatx (g_argblk_expr, basic_constant_return_class_fn (INTEGER), 
	       " (", s, ")", NULL);
    } else {
      strcatx (buf, basic_constant_return_class_fn (INTEGER), " (", s, ")",
	       NULL);
      fileout (buf, 0, expr_start_idx);
    }
    __xfree (MEMADDR(s));
  }
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  } 
  return TRUE;
}

/*
 *  C-only expressions that return bool.  Expressions that 
 *  contain objects get evaluated separately.
 */
static int bool_expr_return_expr (MESSAGE_STACK messages,
				 int keyword_ptr,
				 int expr_start_idx,
				 int expr_end_idx) {
  char *s, buf[MAXLABEL];
  CVAR *c;
  CFUNC *fn = NULL;
  int i, after_prefix_tok_idx, typecast_end_idx = -1;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  /*
   *  Return if the expression refers to any objects.
   */
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if (get_object (M_NAME(messages[i]), NULL) != NULL)
	return FALSE;
      if (is_method_parameter (messages, i))
	return FALSE;
      if (messages[i] -> attrs & TOK_SELF)
	return FALSE;
      if (messages[i] -> attrs & TOK_SUPER)
	return FALSE;
    }
  }

  /*
   *  Format expressions with leading type casts immediately.
   *
   */
  if (is_typecast_expr (messages, expr_start_idx, &typecast_end_idx)) {
    if (strcmp (basic_class_from_typecast (messages, expr_start_idx,
					   typecast_end_idx), 
		BOOLEAN_CLASSNAME)) {
      return FALSE;
    } else {
      s = collect_tokens (messages, expr_start_idx, expr_end_idx);
      strcatx (buf, basic_constant_return_class_fn (INTEGER), " (",
	       s, ")", NULL);
      __xfree (MEMADDR(s));
      fileout (buf, 0, expr_start_idx);
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      } 
      return TRUE;
    }
  }

  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    /* if it's a C expression with a relational op, output
       the __ctalkCBoolToObj expression and return. */
    if (IS_C_RELATIONAL_OP(M_TOK(messages[i]))) {
      s = collect_tokens (messages, expr_start_idx, expr_end_idx);
      strcatx (buf, "__ctalkCBoolToObj (",
	       s, ")", NULL);
      __xfree (MEMADDR(s));
      fileout (buf, 0, expr_start_idx);
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      } 
      return TRUE;
    }
  }

  
  if (IS_C_UNARY_MATH_OP(M_TOK(messages[expr_start_idx]))) {
    after_prefix_tok_idx = nextlangmsg (messages, expr_start_idx);
  } else {
    after_prefix_tok_idx = expr_start_idx;
  }

  if (((c = get_local_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL) ||
      ((c = get_global_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL)) {
    if (!(c -> type_attrs & CVAR_TYPE_INT) &&
	!(c -> type_attrs & CVAR_TYPE_LONG) &&
	!(c -> type_attrs & CVAR_TYPE_LONGLONG))
      return FALSE;
    if (c -> n_derefs) return FALSE;
  } else if ((fn = get_function (M_NAME(messages[after_prefix_tok_idx])))
	     != NULL) {
    if ((!(fn -> return_type_attrs & CVAR_TYPE_INT) &&
	 !(fn -> return_type_attrs & CVAR_TYPE_LONG) &&
	 !(fn -> return_type_attrs & CVAR_TYPE_LONGLONG)) &&
	strcmp (fn -> return_type, "_Bool")) 
      return FALSE;
    if (fn -> return_derefs)
      return FALSE;
  }
  if (!c && !fn)
    return FALSE;

  s = collect_tokens (messages, expr_start_idx, expr_end_idx);
  if (make_argblk_expr) {
    strcatx (g_argblk_expr, basic_constant_return_class_fn (LABEL), " (",
	     s, ")", NULL);
  } else {
    strcatx (buf, basic_constant_return_class_fn (LABEL), " (",
	     s, ")", NULL);
    fileout (buf, 0, expr_start_idx);
  }
  __xfree (MEMADDR(s));
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  } 
  return TRUE;
}

/*
 *  C-only expressions that return <long long> | <long> int.  Expressions that 
 *  contain objects get evaluated separately.
 */
static int long_int_expr_return_expr (MESSAGE_STACK messages,
				 int keyword_ptr,
				 int expr_start_idx,
				 int expr_end_idx) {
  char *s, buf[MAXLABEL];
  CVAR *c;
  CFUNC *fn = NULL;
  int i, after_prefix_tok_idx, typecast_end_idx = -1;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  /*
   *  Return if the expression refers to any objects.
   */
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if (get_object (M_NAME(messages[i]), NULL) != NULL)
	return FALSE;
      if (messages[i] -> attrs & TOK_SELF)
	return FALSE;
      if (messages[i] -> attrs & TOK_SUPER)
	return FALSE;
    }
  }

  /*
   *  Format expressions with leading type casts immediately.
   *
   */
  if (is_typecast_expr (messages, expr_start_idx, &typecast_end_idx)) {
    if (strcmp (basic_class_from_typecast (messages, expr_start_idx,
					   typecast_end_idx), 
		LONGINTEGER_CLASSNAME)) {
      return FALSE;
    } else {
      s = collect_tokens (messages, expr_start_idx, expr_end_idx);
      if (make_argblk_expr) {
	strcatx (g_argblk_expr, basic_constant_return_class_fn (LONGLONG), " (",
		 s, ")", NULL);
      } else {
	strcatx (buf, basic_constant_return_class_fn (LONGLONG), " (",
		 s, ")", NULL);
	fileout (buf, 0, expr_start_idx);
      }
      __xfree (MEMADDR(s));
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      } 
      return TRUE;
    }
  }

  if (IS_C_UNARY_MATH_OP(M_TOK(messages[expr_start_idx]))) {
    after_prefix_tok_idx = nextlangmsg (messages, expr_start_idx);
  } else {
    after_prefix_tok_idx = expr_start_idx;
  }

  if (((c = get_local_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL) ||
      ((c = get_global_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL)) {
    if (!(c -> type_attrs & CVAR_TYPE_INT))
      return FALSE;
    if (c -> n_derefs) return FALSE;
  } else {
    if ((fn = get_function (M_NAME(messages[after_prefix_tok_idx]))) != NULL) {
      if (!(fn -> return_type_attrs & CVAR_TYPE_INT))
	return FALSE;
      if (fn -> return_derefs)
	return FALSE;
    }
  }
  if (!c && !fn)
    return FALSE;

  s = collect_tokens (messages, expr_start_idx, expr_end_idx);
  if (make_argblk_expr) {
    strcatx (g_argblk_expr, basic_constant_return_class_fn (LONGLONG), 
	     " (", s, ")", NULL);
    /*  strcpy (g_argblk_expr, buf); */
  } else {
    strcatx (buf, basic_constant_return_class_fn (LONGLONG), " (", s, ")",
	     NULL);
    fileout (buf, 0, expr_start_idx);
  }
  __xfree (MEMADDR(s));
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  } 
  return TRUE;
}

/*
 *  C-only expressions that return char.  Expressions that 
 *  contain objects get evaluated separately.
 */
static int char_expr_return_expr (MESSAGE_STACK messages,
				 int keyword_ptr,
				 int expr_start_idx,
				 int expr_end_idx) {
  char *s = NULL, buf[MAXLABEL];
  CVAR *c, *c_derived, *c_mbr;
  CFUNC *fn = NULL;
  int i, after_prefix_tok_idx, typecast_end_idx = -1,
    terminal_struct_label;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  /*
   *  Return if the expression refers to any objects.
   */
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if (get_object (M_NAME(messages[i]), NULL) != NULL)
	return FALSE;
      if (messages[i] -> attrs & TOK_SELF)
	return FALSE;
      if (messages[i] -> attrs & TOK_SUPER)
	return FALSE;
    }
  }

  /*
   *  Format expressions with leading type casts immediately.
   *
   */
  if (is_typecast_expr (messages, expr_start_idx, &typecast_end_idx)) {
    if (strcmp (basic_class_from_typecast (messages, expr_start_idx,
					   typecast_end_idx), 
		CHARACTER_CLASSNAME)) {
      return FALSE;
    } else {
      s = collect_tokens (messages, expr_start_idx, expr_end_idx);
      if (make_argblk_expr) {
	strcatx (g_argblk_expr, 
		 basic_constant_return_class_fn (LITERAL_CHAR), " (",
		 s, ")", NULL);
      } else {
	strcatx (buf, basic_constant_return_class_fn (LITERAL_CHAR), " (",
		 s, ")", NULL);
	fileout (buf, 0, expr_start_idx);
      }
      __xfree (MEMADDR(s));
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      } 
      return TRUE;
    }
  }

  if (IS_C_UNARY_MATH_OP(M_TOK(messages[expr_start_idx]))) {
    after_prefix_tok_idx = nextlangmsg (messages, expr_start_idx);
  } else {
    after_prefix_tok_idx = expr_start_idx;
  }

  if (((c = get_local_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL) ||
      ((c = get_global_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL)) {
    if (!(c -> type_attrs & CVAR_TYPE_CHAR) ||
	*(c -> qualifier) ||
	(c -> n_derefs)) {
      /* If the operand isn't a plain old char, first check for
	 an object member (with subscript) .... */
      if ((c -> type_attrs & CVAR_TYPE_OBJECT) &&
	  (c -> n_derefs == 1)) {
	if ((c_derived = get_typedef (c -> type)) != NULL) {
	  if ((terminal_struct_label = struct_end
	       (messages, after_prefix_tok_idx,
		get_stack_top (messages))) > 0) {
	    if ((c_mbr = struct_member_from_expr_b
		 (messages, expr_start_idx, expr_end_idx, 
		  c_derived)) != NULL) {
	      if (c_mbr -> type_attrs & CVAR_TYPE_CHAR) {
		switch (c -> n_derefs) 
		  {
		  case 0:
		    if (terminal_struct_label == expr_end_idx)
		      c = c_mbr;
		    else
		      return FALSE;
		    break;
		  case 1:
		    if ((terminal_struct_label > expr_end_idx) &&
			(M_TOK(messages[expr_end_idx]) == ARRAYCLOSE)) {
		      c = c_mbr;
		    } else {
		      return FALSE;
		    }
		    break;
		  default:
		    return FALSE;
		    break;
		  }
	      } else {
		return FALSE;
	      }
	    } else {
	      __xfree (MEMADDR(s));
	      return FALSE;
	    }
	  } else {
	    return FALSE;
	  }
	} else {
	  return FALSE;
	}
      } else { /* if (str_eq (c -> type, "OBJECT") && ... */
	/* subscript_expr_return_expr, above, handles subscripted
	   char *'s. */
	return FALSE;
      }
    } /* if ((c -> type_attrs & CVAR_TYPE_CHAR) || ... */
  } else {
    if ((fn = get_function (M_NAME(messages[after_prefix_tok_idx]))) != NULL) {
      if (!(fn -> return_type_attrs & CVAR_TYPE_CHAR))
	return FALSE;
      if (*(fn -> qualifier_type))
	return FALSE;
      if (fn -> return_derefs)
	return FALSE;
    }
  }
  if (!c && !fn)
    return FALSE;

  s = collect_tokens (messages, expr_start_idx, expr_end_idx);
  if (make_argblk_expr) {
    strcatx (g_argblk_expr, basic_constant_return_class_fn (LITERAL_CHAR), " (",
	     s, ")", NULL);
  } else {
    strcatx (buf, basic_constant_return_class_fn (LITERAL_CHAR), " (",
	     s, ")", NULL);
    fileout (buf, 0, expr_start_idx);
  }
  __xfree (MEMADDR(s));
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  } 
  return TRUE;
}

/*
 *  C-only expressions that return char *.  Expressions that 
 *  contain objects get evaluated separately.
 */
static int char_ptr_expr_return_expr (MESSAGE_STACK messages,
				 int keyword_ptr,
				 int expr_start_idx,
				 int expr_end_idx) {
  char *s, buf[MAXLABEL];
  CVAR *c;
  CFUNC *fn = NULL;
  int i, after_prefix_tok_idx, typecast_end_idx = -1;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  /*
   *  Return if the expression refers to any objects.
   */
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if (get_object (M_NAME(messages[i]), NULL) != NULL)
	return FALSE;
      if (messages[i] -> attrs & TOK_SELF)
	return FALSE;
      if (messages[i] -> attrs & TOK_SUPER)
	return FALSE;
    }
  }

  /*
   *  Format expressions with leading type casts immediately.
   *
   */
  if (is_typecast_expr (messages, expr_start_idx, &typecast_end_idx)) {
    if (strcmp (basic_class_from_typecast (messages, expr_start_idx,
					   typecast_end_idx), 
		STRING_CLASSNAME)) {
      return FALSE;
    } else {
      s = collect_tokens (messages, expr_start_idx, expr_end_idx);
      if (make_argblk_expr) {
	strcatx (g_argblk_expr, basic_constant_return_class_fn (LITERAL), " (",
		 s, ")", NULL);
      } else {
	strcatx (buf, basic_constant_return_class_fn (LITERAL), " (",
		 s, ")", NULL);
	fileout (buf, 0, expr_start_idx);
      }
      __xfree (MEMADDR(s));
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      } 
      return TRUE;
    }
  }

  if (IS_C_UNARY_MATH_OP(M_TOK(messages[expr_start_idx]))) {
    after_prefix_tok_idx = nextlangmsg (messages, expr_start_idx);
  } else {
    after_prefix_tok_idx = expr_start_idx;
  }

  if (((c = get_local_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL) ||
      ((c = get_global_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL)) {
    /*if (strcmp (c -> type, "char")) */
    if (!(c -> type_attrs & CVAR_TYPE_CHAR))
      return FALSE;
    if (*(c -> qualifier))
      return FALSE;
    if (c -> n_derefs != 1) 
      return FALSE;
  } else {
    if ((fn = get_function (M_NAME(messages[after_prefix_tok_idx]))) != NULL) {
      if (!(fn -> return_type_attrs & CVAR_TYPE_CHAR))
	return FALSE;
      if (*(fn -> qualifier_type))
	return FALSE;
      if (fn -> return_derefs != 1)
	return FALSE;
    }
  }
  if (!c && !fn)
    return FALSE;

  s = collect_tokens (messages, expr_start_idx, expr_end_idx);
  if (make_argblk_expr) {
    strcatx (g_argblk_expr, basic_constant_return_class_fn (LITERAL), " (",
	     s, ")", NULL);
  } else {
    strcatx (buf, basic_constant_return_class_fn (LITERAL), " (",
	     s, ")", NULL);
    fileout (buf, 0, expr_start_idx);
  }
  __xfree (MEMADDR(s));
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  } 
  return TRUE;
}

/*
 *  C-only expressions that return float or double.  Expressions that 
 *  contain objects get evaluated separately.
 */
static int float_expr_return_expr (MESSAGE_STACK messages,
				 int keyword_ptr,
				 int expr_start_idx,
				 int expr_end_idx) {
  char *s, buf[MAXLABEL];
  CVAR *c;
  CFUNC *fn = NULL;
  int i, after_prefix_tok_idx, typecast_end_idx = -1;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  /*
   *  Return if the expression refers to any objects.
   */
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if (get_object (M_NAME(messages[i]), NULL) != NULL)
	return FALSE;
      if (messages[i] -> attrs & TOK_SELF)
	return FALSE;
      if (messages[i] -> attrs & TOK_SUPER)
	return FALSE;
    }
  }

  /*
   *  Format expressions with leading type casts immediately.
   *
   */
  if (is_typecast_expr (messages, expr_start_idx, &typecast_end_idx)) {
    if (strcmp (basic_class_from_typecast (messages, expr_start_idx,
					   typecast_end_idx), 
		FLOAT_CLASSNAME)) {
      return FALSE;
    } else {
      s = collect_tokens (messages, expr_start_idx, expr_end_idx);
      if (make_argblk_expr) {
	strcatx (g_argblk_expr, basic_constant_return_class_fn (DOUBLE), " (",
		 s, ")", NULL);
      } else {
	strcatx (buf, basic_constant_return_class_fn (DOUBLE), " (",
		 s, ")", NULL);
	fileout (buf, 0, expr_start_idx);
      }
      __xfree (MEMADDR(s));
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      } 
      return TRUE;
    }
  }

  if (IS_C_UNARY_MATH_OP(M_TOK(messages[expr_start_idx]))) {
    after_prefix_tok_idx = nextlangmsg (messages, expr_start_idx);
  } else {
    after_prefix_tok_idx = expr_start_idx;
  }

  if (((c = get_local_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL) ||
      ((c = get_global_var (M_NAME(messages[after_prefix_tok_idx]))) != NULL)) {
    if (!(c -> type_attrs & CVAR_TYPE_CHAR) &&
	!(c ->  type_attrs & CVAR_TYPE_DOUBLE))
      return FALSE;
    if (*(c -> qualifier))
      return FALSE;
    if (c -> n_derefs) 
      return FALSE;
  } else {
    if ((fn = get_function (M_NAME(messages[after_prefix_tok_idx]))) != NULL) {
      if (!(fn -> return_type_attrs & CVAR_TYPE_FLOAT) &&
	  !(fn -> return_type_attrs & CVAR_TYPE_DOUBLE)) 
	return FALSE;
      if (*(fn -> qualifier_type))
	return FALSE;
      if (fn -> return_derefs)
	return FALSE;
    }
  }
  if (!c && !fn)
    return FALSE;

  s = collect_tokens (messages, expr_start_idx, expr_end_idx);
  if (make_argblk_expr) {
    strcatx (g_argblk_expr, basic_constant_return_class_fn (DOUBLE), " (",
	     s, ")", NULL);
  } else {
    strcatx (buf, basic_constant_return_class_fn (DOUBLE), " (",
	     s, ")", NULL);
    fileout (buf, 0, expr_start_idx);
  }
  __xfree (MEMADDR(s));
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  } 
  return TRUE;
}

static int obj_fn_return_expr (MESSAGE_STACK messages,
				  int keyword_ptr,
				  int expr_start_idx,
				  int expr_end_idx) {
  CFUNC *label_cfunc;
  CVAR *arg_cvar;
  char buf[MAXMSG], param_buf[MAXMSG], *s, s_expr[MAXMSG];
  int args[MAXARGS], n_args, n_th_arg, i, open_paren_idx, close_paren_idx,
    have_object;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (M_TOK(messages[expr_start_idx]) == LABEL) {
    if ((label_cfunc = get_function (M_NAME(messages[expr_start_idx])))
	!= NULL) {
      if (!strcmp (label_cfunc -> return_type, "OBJECT") && 
	  (label_cfunc -> return_derefs == 1)) {
	sprintf (buf, "%s (", M_NAME(messages[expr_start_idx]));
	open_paren_idx = nextlangmsg (messages, expr_start_idx);
	close_paren_idx = match_paren (messages, open_paren_idx,
				       get_stack_top (messages));
	split_args_idx (messages, open_paren_idx, close_paren_idx,
			args, &n_args);
	arg_cvar = label_cfunc -> params;
	for (n_th_arg = 0; n_th_arg < n_args; n_th_arg += 2) {
	  have_object = FALSE;
	  for (i = args[n_th_arg]; i >= args[n_th_arg+1]; i--) {
	    if (get_object (M_NAME(messages[i]), NULL))
	      have_object = TRUE;
	    if (messages[i] -> attrs & TOK_SELF)
	      have_object = TRUE;
	  }
	  s = collect_tokens (messages, args[n_th_arg], args[n_th_arg+1]);
	  sprintf (s_expr, "%s (\"%s\")", EVAL_EXPR_FN, s);
	  de_newline_buf (s_expr);
	  if (have_object) {
	    if (arg_cvar) {
	      fmt_rt_return (s_expr,
			     basic_class_from_cvar (messages[expr_start_idx],
						      arg_cvar, 0),
			     TRUE, param_buf);
	    } else {
	      strcpy (param_buf, s);
	    }
	  } else {
	    strcpy (param_buf, s);
	  }
	  __xfree (MEMADDR(s));
	  if (n_th_arg == (n_args - 2)) {
	    strcatx2 (buf, param_buf, ")", NULL);
	  } else {
	    strcatx2 (buf, param_buf, ",", NULL);
	  }
	}
	if (make_argblk_expr) {
	  strcpy (g_argblk_expr, buf);
	} else {
	  fileout (buf, 0, expr_start_idx);
	}
	for (i = expr_start_idx; i >= expr_end_idx; i--) {
	  ++messages[i] -> evaled;
	  ++messages[i] -> output;
	} 
	return TRUE;
      }
    }
  }
  return FALSE;
}

static int int_fn_return_expr (MESSAGE_STACK messages,
				  int keyword_ptr,
				  int expr_start_idx,
				  int expr_end_idx) {
  CFUNC *label_cfunc;
  CVAR *arg_cvar;
  char buf[MAXMSG], param_buf[MAXMSG], *s, s_expr[MAXMSG];
  int args[MAXARGS], n_args, n_th_arg, i, open_paren_idx, close_paren_idx,
    have_object;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (M_TOK(messages[expr_start_idx]) == LABEL) {
    if ((label_cfunc = get_function (M_NAME(messages[expr_start_idx])))
	!= NULL) {
      if ((label_cfunc -> return_type_attrs & CVAR_TYPE_INT) &&
	  !(label_cfunc -> return_type_attrs & CVAR_TYPE_LONG) &&
	  !(label_cfunc -> return_type_attrs & CVAR_TYPE_LONGLONG) &&
	  !label_cfunc -> return_derefs) {
	  
	sprintf (buf, "%s ( %s (", INT_CONSTANT_RETURN_FN,
		 M_NAME(messages[expr_start_idx]));
	open_paren_idx = nextlangmsg (messages, expr_start_idx);
	close_paren_idx = match_paren (messages, open_paren_idx,
				       get_stack_top (messages));
	split_args_idx (messages, open_paren_idx, close_paren_idx,
			args, &n_args);
	arg_cvar = label_cfunc -> params;
	for (n_th_arg = 0; n_th_arg < n_args; n_th_arg += 2) {
	  have_object = FALSE;
	  for (i = args[n_th_arg]; i >= args[n_th_arg+1]; i--) {
	    if (get_object (M_NAME(messages[i]), NULL))
	      have_object = TRUE;
	    if (messages[i] -> attrs & TOK_SELF)
	      have_object = TRUE;
	  }
	  s = collect_tokens (messages, args[n_th_arg], args[n_th_arg+1]);
	  sprintf (s_expr, "%s (\"%s\")", EVAL_EXPR_FN, s);
	  de_newline_buf (s_expr);
	  if (have_object) {
	    if (arg_cvar) {
	      fmt_rt_return (s_expr,
			     basic_class_from_cvar (messages[expr_start_idx],
						      arg_cvar, 0),
			     TRUE, param_buf);
	    } else {
	      strcpy (param_buf, s);
	    }
	  } else {
	    strcpy (param_buf, s);
	  }
	  __xfree (MEMADDR(s));
	  if (n_th_arg == (n_args - 2)) {
	    strcatx2 (buf, param_buf, ")", NULL);
	  } else {
	    strcatx2 (buf, param_buf, ",", NULL);
	  }
	}
	strcatx2 (buf, ")", NULL);
	if (make_argblk_expr) {
	  strcpy (g_argblk_expr, buf);
	} else {
	  fileout (buf, 0, expr_start_idx);
	}
	for (i = expr_start_idx; i >= expr_end_idx; i--) {
	  ++messages[i] -> evaled;
	  ++messages[i] -> output;
	} 
	return TRUE;
      }
      if ((label_cfunc -> return_type_attrs & CVAR_TYPE_LONG) &&
	  !(label_cfunc -> return_type_attrs & CVAR_TYPE_LONGLONG)) {
	sprintf (buf, "%s ( %s(", INT_CONSTANT_RETURN_FN,
		 M_NAME(messages[expr_start_idx]));
	open_paren_idx = nextlangmsg (messages, expr_start_idx);
	close_paren_idx = match_paren (messages, open_paren_idx,
				       get_stack_top (messages));
	split_args_idx (messages, open_paren_idx, close_paren_idx,
			args, &n_args);
	arg_cvar = label_cfunc -> params;
	for (n_th_arg = 0; n_th_arg < n_args; n_th_arg += 2) {
	  have_object = FALSE;
	  for (i = args[n_th_arg]; i >= args[n_th_arg+1]; i--) {
	    if (M_TOK(messages[i]) == LABEL) {
	      if (get_object (M_NAME(messages[i]), NULL))
		have_object = TRUE;
	      if (messages[i] -> attrs & TOK_SELF)
		have_object = TRUE;
	    }
	  }
	  s = collect_tokens (messages, args[n_th_arg], args[n_th_arg+1]);
	  sprintf (s_expr, "%s (\"%s\")", EVAL_EXPR_FN, s);
	  de_newline_buf (s_expr);
	  if (have_object) {
	    if (arg_cvar) {
	      fmt_rt_return (s_expr,
			     basic_class_from_cvar (messages[expr_start_idx],
						      arg_cvar, 0),
			     TRUE, param_buf);
	    } else {
	      strcpy (param_buf, s);
	    }
	  } else {
	    strcpy (param_buf, s);
	  }
	  __xfree (MEMADDR(s));
	  if (n_th_arg == (n_args - 2)) {
	    strcatx2 (buf, param_buf, ")", NULL);
	  } else {
	    strcatx2 (buf, param_buf, ",", NULL);
	  }
	}
	strcatx2 (buf, ")", NULL);
	if (make_argblk_expr) {
	  strcpy (g_argblk_expr, buf);
	} else {
	  fileout (buf, 0, expr_start_idx);
	}
	for (i = expr_start_idx; i >= expr_end_idx; i--) {
	  ++messages[i] -> evaled;
	  ++messages[i] -> output;
	} 
	return TRUE;
      }
    }
  }
  return FALSE;
}

static int long_long_int_fn_return_expr (MESSAGE_STACK messages,
				  int keyword_ptr,
				  int expr_start_idx,
				  int expr_end_idx) {
  CFUNC *label_cfunc;
  CVAR *arg_cvar;
  char buf[MAXMSG], param_buf[MAXMSG], *s, s_expr[MAXMSG];
  int args[MAXARGS], n_args, n_th_arg, i, open_paren_idx, close_paren_idx,
    have_object;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (M_TOK(messages[expr_start_idx]) == LABEL) {
    if ((label_cfunc = get_function (M_NAME(messages[expr_start_idx])))
	!= NULL) {
      if ((label_cfunc -> return_type_attrs & CVAR_TYPE_LONGLONG) &&
	  !label_cfunc -> return_derefs) {
	sprintf (buf, "%s ( %s(", LLINT_CONSTANT_RETURN_FN,
		 M_NAME(messages[expr_start_idx]));
	open_paren_idx = nextlangmsg (messages, expr_start_idx);
	close_paren_idx = match_paren (messages, open_paren_idx,
				       get_stack_top (messages));
	split_args_idx (messages, open_paren_idx, close_paren_idx,
			args, &n_args);
	arg_cvar = label_cfunc -> params;
	for (n_th_arg = 0; n_th_arg < n_args; n_th_arg += 2) {
	  have_object = FALSE;
	  for (i = args[n_th_arg]; i >= args[n_th_arg+1]; i--) {
	    if (get_object (M_NAME(messages[i]), NULL))
	      have_object = TRUE;
	    if (messages[i] -> attrs & TOK_SELF)
	      have_object = TRUE;
	  }
	  s = collect_tokens (messages, args[n_th_arg], args[n_th_arg+1]);
	  sprintf (s_expr, "%s (\"%s\")", EVAL_EXPR_FN, s);
	  de_newline_buf (s_expr);
	  if (have_object) {
	    if (arg_cvar) {
	      fmt_rt_return (s_expr,
			     basic_class_from_cvar (messages[expr_start_idx],
						      arg_cvar, 0),
			     TRUE, param_buf);
	    } else {
	      strcpy (param_buf, s);
	    }
	  } else {
	    strcpy (param_buf, s);
	  }
	  __xfree (MEMADDR(s));
	  if (n_th_arg == (n_args - 2)) {
	    strcatx2 (buf, param_buf, ")", NULL);
	  } else {
	    strcatx2 (buf, param_buf, ",", NULL);
	  }
	}
	strcatx2 (buf, ")", NULL);
	if (make_argblk_expr) {
	  strcpy (g_argblk_expr, buf);
	} else {
	  fileout (buf, 0, expr_start_idx);
	}
	for (i = expr_start_idx; i >= expr_end_idx; i--) {
	  ++messages[i] -> evaled;
	  ++messages[i] -> output;
	} 
	return TRUE;
      }
    }
  }
  return FALSE;
}

static int char_fn_return_expr (MESSAGE_STACK messages,
				  int keyword_ptr,
				  int expr_start_idx,
				  int expr_end_idx) {
  CFUNC *label_cfunc;
  CVAR *arg_cvar;
  char buf[MAXMSG], param_buf[MAXMSG], *s, s_expr[MAXMSG];
  int args[MAXARGS], n_args, n_th_arg, i, open_paren_idx, close_paren_idx,
    have_object;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (M_TOK(messages[expr_start_idx]) == LABEL) {
    if ((label_cfunc = get_function (M_NAME(messages[expr_start_idx])))
	!= NULL) {
      if ((label_cfunc -> return_type_attrs & CVAR_TYPE_CHAR) &&
	  (label_cfunc -> return_derefs == 0)) {
	sprintf (buf, "%s ( %s(", CHAR_CONSTANT_RETURN_FN,
		 M_NAME(messages[expr_start_idx]));
	open_paren_idx = nextlangmsg (messages, expr_start_idx);
	close_paren_idx = match_paren (messages, open_paren_idx,
				       get_stack_top (messages));
	split_args_idx (messages, open_paren_idx, close_paren_idx,
			args, &n_args);
	arg_cvar = label_cfunc -> params;
	for (n_th_arg = 0; n_th_arg < n_args; n_th_arg += 2) {
	  have_object = FALSE;
	  for (i = args[n_th_arg]; i >= args[n_th_arg+1]; i--) {
	    if (get_object (M_NAME(messages[i]), NULL))
	      have_object = TRUE;
	    if (messages[i] -> attrs & TOK_SELF)
	      have_object = TRUE;
	  }
	  s = collect_tokens (messages, args[n_th_arg], args[n_th_arg+1]);
	  sprintf (s_expr, "%s (\"%s\")", EVAL_EXPR_FN, s);
	  de_newline_buf (s_expr);
	  if (have_object) {
	    if (arg_cvar) {
	      fmt_rt_return (s_expr,
			     basic_class_from_cvar (messages[expr_start_idx],
						      arg_cvar, 0),
			     TRUE, param_buf);
	    } else {
	      strcpy (param_buf, s);
	    }
	  } else {
	    strcpy (param_buf, s);
	  }
	  __xfree (MEMADDR(s));
	  if (n_th_arg == (n_args - 2)) {
	    strcatx2 (buf, param_buf, ")", NULL);
	  } else {
	    strcatx2 (buf, param_buf, ",", NULL);
	  }
	}
	strcatx2 (buf, ")", NULL);
	if (make_argblk_expr) {
	  strcpy (g_argblk_expr, buf);
	} else {
	  fileout (buf, 0, expr_start_idx);
	}
	for (i = expr_start_idx; i >= expr_end_idx; i--) {
	  ++messages[i] -> evaled;
	  ++messages[i] -> output;
	} 
	return TRUE;
      }
    }
  }
  return FALSE;
}

static int char_ptr_fn_return_expr (MESSAGE_STACK messages,
				    int keyword_ptr,
				    int expr_start_idx,
				    int expr_end_idx) {
  CFUNC *label_cfunc;
  CVAR *arg_cvar;
  char buf[MAXMSG], param_buf[MAXMSG], *s, s_expr[MAXMSG];
  int args[MAXARGS], n_args, n_th_arg, i, open_paren_idx, close_paren_idx,
    have_object;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (M_TOK(messages[expr_start_idx]) == LABEL) {
    if ((label_cfunc = get_function (M_NAME(messages[expr_start_idx])))
	!= NULL) {
      if ((label_cfunc -> return_type_attrs & CVAR_TYPE_CHAR) &&
	  (label_cfunc -> return_derefs == 1)) {
	sprintf (buf, "%s ( %s(", CHAR_PTR_CONSTANT_RETURN_FN,
		 M_NAME(messages[expr_start_idx]));
	open_paren_idx = nextlangmsg (messages, expr_start_idx);
	close_paren_idx = match_paren (messages, open_paren_idx,
				       get_stack_top (messages));
	split_args_idx (messages, open_paren_idx, close_paren_idx,
			args, &n_args);
	arg_cvar = label_cfunc -> params;
	for (n_th_arg = 0; n_th_arg < n_args; n_th_arg += 2) {
	  have_object = FALSE;
	  for (i = args[n_th_arg]; i >= args[n_th_arg+1]; i--) {
	    if (get_object (M_NAME(messages[i]), NULL))
	      have_object = TRUE;
	    if (messages[i] -> attrs & TOK_SELF)
	      have_object = TRUE;
	  }
	  s = collect_tokens (messages, args[n_th_arg], args[n_th_arg+1]);
	  sprintf (s_expr, "%s (\"%s\")", EVAL_EXPR_FN, s);
	  de_newline_buf (s_expr);
	  if (have_object) {
	    if (arg_cvar) {
	      fmt_rt_return (s_expr,
			     basic_class_from_cvar (messages[expr_start_idx],
						      arg_cvar, 0),
			     TRUE, param_buf);
	    } else {
	      strcpy (param_buf, s);
	    }
	  } else {
	    strcpy (param_buf, s);
	  }
	  __xfree (MEMADDR(s));
 	  if (n_th_arg == (n_args - 2)) {
	    strcatx2 (buf, param_buf, ")", NULL);
	  } else {
	    strcatx2 (buf, param_buf, ",", NULL);
	  }
	}
	strcatx2 (buf, ")", NULL);
	if (make_argblk_expr) {
	  strcpy (g_argblk_expr, buf);
	} else {
	  fileout (buf, 0, expr_start_idx);
	}
	for (i = expr_start_idx; i >= expr_end_idx; i--) {
	  ++messages[i] -> evaled;
	  ++messages[i] -> output;
	} 
	return TRUE;
      }
    }
  }
  return FALSE;
}

static int double_fn_return_expr (MESSAGE_STACK messages,
				    int keyword_ptr,
				    int expr_start_idx,
				    int expr_end_idx) {
  CFUNC *label_cfunc;
  CVAR *arg_cvar;
  char buf[MAXMSG], param_buf[MAXMSG], *s, s_expr[MAXMSG];
  int args[MAXARGS], n_args, n_th_arg, i, open_paren_idx, close_paren_idx,
    have_object;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (M_TOK(messages[expr_start_idx]) == LABEL) {
    if ((label_cfunc = get_function (M_NAME(messages[expr_start_idx])))
	!= NULL) {
      if ((label_cfunc -> return_type_attrs & CVAR_TYPE_DOUBLE) &&
	  !label_cfunc -> return_derefs) {
	sprintf (buf, "%s ( %s(", FLOAT_CONSTANT_RETURN_FN,
		 M_NAME(messages[expr_start_idx]));
	open_paren_idx = nextlangmsg (messages, expr_start_idx);
	close_paren_idx = match_paren (messages, open_paren_idx,
				       get_stack_top (messages));
	split_args_idx (messages, open_paren_idx, close_paren_idx,
			args, &n_args);
	arg_cvar = label_cfunc -> params;
	for (n_th_arg = 0; n_th_arg < n_args; n_th_arg += 2) {
	  have_object = FALSE;
	  for (i = args[n_th_arg]; i >= args[n_th_arg+1]; i--) {
	    if (get_object (M_NAME(messages[i]), NULL))
	      have_object = TRUE;
	    if (messages[i] -> attrs & TOK_SELF)
	      have_object = TRUE;
	  }
	  s = collect_tokens (messages, args[n_th_arg], args[n_th_arg+1]);
	  sprintf (s_expr, "%s (\"%s\")", EVAL_EXPR_FN, s);
	  de_newline_buf (s_expr);
	  if (have_object) {
	    if (arg_cvar) {
	      fmt_rt_return (s_expr,
			     basic_class_from_cvar (messages[expr_start_idx],
						      arg_cvar, 0),
			     TRUE, param_buf);
	    } else {
	      strcpy (param_buf, s);
	    }
	  } else {
	    strcpy (param_buf, s);
	  }
	  __xfree (MEMADDR(s));
	  if (n_th_arg == (n_args - 2)) {
	    strcatx2 (buf, param_buf, ")", NULL);
	  } else {
	    strcatx2 (buf, param_buf, ",", NULL);
	  }
	}
	strcatx2 (buf, ")", NULL);
	if (make_argblk_expr) {
	  strcpy (g_argblk_expr, buf);
	} else {
	  fileout (buf, 0, expr_start_idx);
	}
	for (i = expr_start_idx; i >= expr_end_idx; i--) {
	  ++messages[i] -> evaled;
	  ++messages[i] -> output;
	} 
	return TRUE;
      }
    }
  }
  return FALSE;
}

static char *NULL_expr = "((void *)0)";

/*
 *  A return NULL is also a no-op here.
 */
static int NULL_return_expr (MESSAGE_STACK messages,
				  int keyword_ptr,
				  int expr_start_idx,
				  int expr_end_idx) {
  int n_parens, i;
  bool have_void = false;
  n_parens = 0;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if ((M_TOK(messages[expr_start_idx]) != OPENPAREN) ||
      (M_TOK(messages[expr_end_idx]) != CLOSEPAREN))
    return ERROR;

  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    switch (M_TOK(messages[i]))
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	break;
      case LABEL:
	if (strcmp (M_NAME(messages[i]), "void") ||
	    (n_parens != 2)) {
	  return FALSE;
	} else {
	  have_void = true;
	}
	break;
      case ASTERISK:
	if (n_parens != 2)
	  return FALSE;
	break;
      case INTEGER:
	if (strcmp (M_NAME(messages[i]), "0") ||
	    (n_parens != 1))
	  return FALSE;
	break;
      }
  }

  if (have_void) {
    if (make_argblk_expr)
      strcpy (g_argblk_expr, NULL_expr);
    return TRUE;
  } else {
    return FALSE;
  }

}

static int eval_expr_return_expr (MESSAGE_STACK messages,
				  int keyword_ptr,
				  int expr_start_idx,
				  int expr_end_idx) {
  char buf[MAXMSG], *s;
  int i;
  bool have_object = false;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if (backward_compatibility_cookie (messages, expr_end_idx))
    return FALSE;
  /* checking for only the first object also prevents warnings
     for instance variable series */
  for (i = expr_start_idx; (i >= expr_end_idx) &&
	 (have_object == false); --i) {
    if (M_TOK(messages[i]) == LABEL) {
      if (messages[i] -> attrs & TOK_SELF) {
	have_object = true;
	break;
      }
      if (!get_local_object (M_NAME(messages[i]), NULL) &&
	  !get_global_object (M_NAME(messages[i]), NULL) &&
	  !is_method_parameter (messages, i) &&
	  !is_method_name (M_NAME(messages[i])) &&
	  !is_instance_variable_message (messages, i) &&
	  !(messages[i] -> attrs & TOK_SUPER) &&
	  !is_class_name (M_NAME(messages[i])) &&
	  !is_method_proto (rcvr_class_obj, M_NAME(messages[i]))) {
	if (!is_c_keyword (M_NAME(messages[i])) &&
	    !is_ctalk_keyword (M_NAME(messages[i])) &&
	    !is_gnu_extension_keyword (M_NAME(messages[i])) &&
	    !global_var_is_declared (M_NAME(messages[i])) &&
	    !get_local_var (M_NAME(messages[i])) &&
	    !get_function (M_NAME(messages[i])) &&
	    !typedef_is_declared (M_NAME(messages[i])) &&
	    !is_enum_member (M_NAME(messages[i])) &&
	    !is_struct_member (M_NAME(messages[i])) &&
	    !is_gnuc_builtin_type (M_NAME(messages[i])) &&
#ifdef __ppc__
	    !is_apple_ppc_math_builtin (M_NAME(messages[i])) &&
#endif	    
	    !is_fn_param (M_NAME(messages[i]))) {
	  return_expr_undefined_label (messages, keyword_ptr,
				       expr_end_idx, i);
	}
      } else {
	have_object = true;
      }
    }
  }
  if (make_argblk_expr) {
    fmt_rt_expr (messages, expr_start_idx, &expr_end_idx, g_argblk_expr);
  } else {
    if (have_object) {
      rt_expr (messages, expr_start_idx, &expr_end_idx, buf);
    } else {
      s = collect_tokens (messages, expr_start_idx, expr_end_idx);
      strcatx (buf, basic_constant_return_class_fn
	       (M_TOK(messages[expr_start_idx])),
	       "(", s, ")", NULL);
      fileout (buf, 0, expr_start_idx);
      __xfree (MEMADDR(s));
    }
  }
  for (i = expr_start_idx; i >= expr_end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  } 
  return TRUE;
}

static int eval_stmt_return_expr (MESSAGE_STACK messages,
				  int keyword_ptr,
				  int expr_start_idx,
				  int expr_end_idx) {
  char buf[MAXMSG];
  int i, next_tok_idx;

  if ((interpreter_pass != method_pass) && !make_argblk_expr)
    return ERROR;

  if(!strcmp (M_NAME(messages[expr_start_idx]), "eval")) {
    if ((next_tok_idx = nextlangmsg (messages, expr_start_idx)) != ERROR) {
      fmt_rt_expr (messages, expr_start_idx, &expr_end_idx, buf);
      fileout (buf, 0, expr_start_idx);
      for (i = expr_start_idx; i >= expr_end_idx; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      } 
      return TRUE;
    }
  }
  return FALSE;
}

int eval_return_expr (MESSAGE_STACK messages, int keyword_ptr) {
    int expr_start_idx,
      expr_end_idx,
      i, prev_tok,
      stack_end,
      typecast_end_idx;

    if ((interpreter_pass != method_pass) && !make_argblk_expr)
      return SUCCESS;

  stack_end = get_stack_top (messages);
  if ((expr_start_idx = nextlangmsgstack (messages, keyword_ptr)) 
      == ERROR)
    _error ("eval_return_expr: parser error.\n");

  if ((M_TOK(messages[expr_start_idx]) == OPENPAREN) &&
      !is_typecast_expr (messages, expr_start_idx, &typecast_end_idx)) {
    if ((expr_end_idx = match_paren (messages, expr_start_idx,
				     get_stack_top (messages))) 
	== ERROR) {
      warning (messages[expr_start_idx], "Mismatched parentheses.");
      return ERROR;
    }
    expr_start_idx = nextlangmsg (messages, expr_start_idx);
    expr_end_idx = prevlangmsg (messages, expr_end_idx);
  } else {
    prev_tok = expr_end_idx = expr_start_idx;
    for (i = expr_start_idx; i > stack_end; i--) {
      if (M_ISSPACE(messages[i]))
	continue;
      if (M_TOK(messages[i]) == SEMICOLON) {
	expr_end_idx = prev_tok;
	break;
      }
      prev_tok = i;
    }
  }

  /* If a method's return expression is just a void return, add a NULL. */
  /* Don't adjust the evaled or output counts of the keyword
     or semicolon, to avoid framing errors later on. */
  if ((M_TOK(messages[expr_start_idx]) == SEMICOLON) && 
      (interpreter_pass == method_pass)) {
    if (make_argblk_expr) {
      strcpy (g_argblk_expr, NULL_expr);
    } else {
      fileout (NULL_expr, 0, expr_start_idx);
    }
    return SUCCESS;
  } else if (M_TOK(messages[expr_start_idx]) == MINUS) {
    /* the lexer evaluates a leading minus as a binary op
       because the "return" keyword is only evaluated as label
       in the lexer. so we combine the two tokens here. */
    if ((i = nextlangmsg (messages, expr_start_idx)) != ERROR) {
      if ((i == expr_end_idx) &&
	  ((M_TOK(messages[i]) == INTEGER) ||
	   (M_TOK(messages[i]) == LONG) ||
	   (M_TOK(messages[i]) == LONGLONG))) {
	strcatx (messages[expr_start_idx] -> name, "-",
		 messages[i] -> name, NULL);
	messages[expr_start_idx] -> tokentype =
	  messages[i] -> tokentype;
	messages[i] -> name[0] = ' '; messages[i] -> name[1] = '\0';
	messages[i] -> tokentype = WHITESPACE;
	expr_end_idx = expr_start_idx;
      }
    }
  }

  if (expr_start_idx == expr_end_idx) {
    if (interpreter_pass == method_pass || make_argblk_expr) {
      if (single_tok_return_expr (messages, keyword_ptr,
				  expr_start_idx) == TRUE)
	return SUCCESS;
    }
  }

  if (simple_self_return_expr (messages, keyword_ptr, expr_start_idx, expr_end_idx)
      == TRUE)
    return SUCCESS;
  if (NULL_return_expr (messages, keyword_ptr,
			expr_start_idx,expr_end_idx) == TRUE)
			return SUCCESS;
  if (eval_stmt_return_expr (messages, keyword_ptr,
			     expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;

  if (const_false_return_expr (messages, keyword_ptr,
 			       expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (const_true_return_expr (messages, keyword_ptr,
 			       expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;

  if (cvar_token_return_expr (messages, keyword_ptr,
  			      expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (deref_expr_return_expr (messages, keyword_ptr,
 			      expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (c_function_return_expr (messages, keyword_ptr,
 			      expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (subscript_expr_return_expr (messages, keyword_ptr,
				  expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (int_expr_return_expr (messages, keyword_ptr,
			    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;

  if (bool_expr_return_expr (messages, keyword_ptr,
			    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;

  if (long_int_expr_return_expr (messages, keyword_ptr,
			    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (char_expr_return_expr (messages, keyword_ptr,
			    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (char_ptr_expr_return_expr (messages, keyword_ptr,
			    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (float_expr_return_expr (messages, keyword_ptr,
			    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (obj_fn_return_expr (messages, keyword_ptr,
			  expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (int_fn_return_expr (messages, keyword_ptr,
 			  expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (long_long_int_fn_return_expr (messages, keyword_ptr,
				    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (char_fn_return_expr (messages, keyword_ptr,
				    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (char_ptr_fn_return_expr (messages, keyword_ptr,
			       expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (double_fn_return_expr (messages, keyword_ptr,
				    expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  if (eval_expr_return_expr (messages, keyword_ptr,
 			     expr_start_idx, expr_end_idx) == TRUE)
    return SUCCESS;
  return SUCCESS;
}

char *fmt_argblk_return_expr (MESSAGE_STACK messages,
			      int keyword_ptr) {
  make_argblk_expr = true;
  eval_return_expr (messages, keyword_ptr);
  make_argblk_expr = false;
  return g_argblk_expr;
}
