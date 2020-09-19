/* $Id: objtoc.c,v 1.2 2020/09/19 01:08:28 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2020 Robert Kiesling, rk3314042@gmail.com.
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
 *  Generate calls that translate objects to their C equivalents.  In
 *  the case of object that do not correspond directly to C data types,
 *  return the value pointer, or at least try to do something 
 *  intelligent.
 *
 *  In this version, C int and char types are synonymous, Float class
 *  translates to double, as in C, and a long double class isn't 
 *  implemented.
 *
 *  Long int and int are synonymous, as in GNU C for most, if not all, 
 *  platforms.
 *
 *  NOTE - Future versions might allow the class library to define
 *  what instance variable to return in a C context.
 */

#include <stdio.h>
#include <string.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "objtoc.h"

extern bool ctrlblk_pred,            /* Declared in control.c.   */
  ctrlblk_blk,
  ctrlblk_else_blk;

extern bool argblk;                  /* Declared in argblk.c */

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern DEFAULTCLASSCACHE *ct_defclasses; /* Declared in cclasses.c.       */

extern bool missing_fmt_arg_specifier;  /* Declared in fmtargtype.c. */

/* Tells us whether a pointer format is "%p" or "%#x" */
extern bool ptr_fmt_is_alt_int_fmt;

char *fmt_printf_fmt_arg (MESSAGE_STACK messages, 
			  int expr_start_idx,
			  int stack_start_idx,
			  char *expr,
			  char *expr_out) {
  *expr_out = '\0';

  switch (fmt_arg_type (messages, expr_start_idx, stack_start_idx))
    {
    case fmt_arg_char_ptr:
      fmt_rt_return (expr, STRING_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_ptr:
      fmt_rt_return (expr, SYMBOL_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_char:
      fmt_rt_return (expr, CHARACTER_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_double:
      fmt_rt_return  (expr, FLOAT_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_int:
      fmt_rt_return (expr, INTEGER_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_long_int:
      fmt_rt_return (expr, INTEGER_CLASSNAME_L, TRUE, expr_out);
      break;
    case fmt_arg_long_long_int:
      fmt_rt_return (expr, LONGINTEGER_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_null:
      /* Check for a missing conversion specifier. */
      if (missing_fmt_arg_specifier)
	unknown_format_conversion_warning (messages, expr_start_idx);
      break;
    }
  if (ptr_fmt_is_alt_int_fmt)
    ptr_fmt_is_alt_int_fmt = false;
  
  /* any other format should be compatible with OBJECT *'s */
  if (*expr_out)
    return expr_out;
  else 
    return expr;
}

char *fmt_printf_fmt_arg_ms (MSINFO *ms, char *expr, char *expr_out) {
  *expr_out = '\0';

  switch (fmt_arg_type_ms (ms))
    {
    case fmt_arg_char_ptr:
      fmt_rt_return (expr, STRING_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_ptr:
      fmt_rt_return (expr, SYMBOL_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_char:
      fmt_rt_return (expr, CHARACTER_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_double:
      fmt_rt_return  (expr, FLOAT_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_int:
      fmt_rt_return (expr, INTEGER_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_long_int:
      fmt_rt_return (expr, INTEGER_CLASSNAME_L, TRUE, expr_out);
      break;
    case fmt_arg_long_long_int:
      fmt_rt_return (expr, LONGINTEGER_CLASSNAME, TRUE, expr_out);
      break;
    case fmt_arg_null:
      /* This can also happen if the conversion is for an OBJECT *,
	 so its checked. */
      if (missing_fmt_arg_specifier)
	unknown_format_conversion_warning_ms (ms);
      break;
    }

  if (*expr_out)
    return expr_out;
  else 
    return expr;
}

/*
 *  The function needs to decide whether to use the return class of
 *  the receiver object or of the method if they are different.  Use
 *  the return class of the method, unless the method is defined in a
 *  superclass and does not have a corresponding basic C data type.
 *  In that case, use the return class of the receiver object.
 */

char *obj_2_c_wrapper (MESSAGE *orig, OBJECT *object, 
		       METHOD *method, char *rt_expr, int keep) {

  char *return_class_name;
  char expr_out_buf[MAXMSG];

  if (!method) {
    /*
     *  Look for an instance variable message and use the
     *  instance variable's class as the return class,
     *  or the object's class if necessary.
     */
    OBJECT *instancevar;
    if ((instancevar = __ctalkGetInstanceVariable (object, M_NAME(orig),
						   TRUE)) == NULL) {
      return_class_name = object -> __o_classname;
    } else {
	  if (DEFAULTCLASS_CMP(instancevar, ct_defclasses->p_character_class,
			       CHARACTER_CLASSNAME) ||
	      DEFAULTCLASS_CMP(instancevar, ct_defclasses->p_float_class,
			       FLOAT_CLASSNAME) ||
	      DEFAULTCLASS_CMP(instancevar, ct_defclasses->p_integer_class,
			       INTEGER_CLASSNAME) ||
	      DEFAULTCLASS_CMP(instancevar, ct_defclasses->p_longinteger_class,
			       LONGINTEGER_CLASSNAME) ||
	      DEFAULTCLASS_CMP(instancevar, ct_defclasses->p_string_class,
			       STRING_CLASSNAME))
	    return_class_name = instancevar -> __o_classname;
	  else
	    return_class_name = object -> __o_classname;
    }
	 
  } else {
    /*
     *  Use the method return class as the expression
     *  return class.
     */
    if (!strcmp (method -> returnclass, "Character") ||
	!strcmp (method -> returnclass, "Float") ||
	!strcmp (method -> returnclass, "Integer") ||
	!strcmp (method -> returnclass, "LongInteger") ||
	!strcmp (method -> returnclass, "String")) {
      return_class_name = method -> returnclass;
    } else {
      if (orig -> attrs & OBJ_IS_CLASS_VAR) {
	return_class_name = ((object -> instancevars) ?
			     object -> instancevars -> __o_classname :
			     object -> __o_classname);
      } else {
	return_class_name = object -> __o_classname;
      }
    }
  }

  return fmt_rt_return (rt_expr, return_class_name, keep, expr_out_buf);
}

#define STRING_RETURN_CLASS_IDX 0
#define SYMBOL_RETURN_CLASS_IDX 1
#define CHARACTER_RETURN_CLASS_IDX 2
#define INTEGER_RETURN_CLASS_IDX 3
#define FLOAT_RETURN_CLASS_IDX 4
#define OBJECT_RETURN_CLASS_IDX 5
#define LONGINTEGER_RETURN_CLASS_IDX 6

static char *return_classes[] = {
  STRING_CLASSNAME,
  SYMBOL_CLASSNAME,
  CHARACTER_CLASSNAME,
  INTEGER_CLASSNAME,
  FLOAT_CLASSNAME,
  OBJECT_CLASSNAME,
  LONGINTEGER_CLASSNAME
};

#define __VALUE_INSTVAR_CLASSNAME(__o) ((__o)->instancevars ? \
                                        (__o)->instancevars->__o_classname : \
                                        (__o)->__o_classname)
#define __VALUE_INSTVAR_CLASS(__o) ((__o)->instancevars ? \
				    (__o)->instancevars->__o_class : \
				    (__o)->__o_class)
#define __VALUE_INSTVAR(__o) ((__o)->instancevars ? \
				    (__o)->instancevars : \
				    (__o))
char *obj_2_c_wrapper_trans (MESSAGE_STACK messages, int idx, 
			     MESSAGE *orig, OBJECT *object, 
			     METHOD *method, char *rt_expr, int keep) {

  char *obj_return_class_name;
  int fn_idx, arg_idx, prev_tok_idx;
  int need_cast_to_OBJECT_ptr = FALSE;
  char expr_out[MAXMSG];
#ifndef __GCC__
  char tmp_buffer[MAXMSG];
#endif

  /*
   *  First check if a function arg prototype requires only
   *  an object.  Note that we should be able to use 
   *  arg_return_class_name, above, for prototypes other
   *  than OBJECT *.
   */
  if ((arg_idx = obj_expr_is_arg (messages, idx,
				  stack_start(messages),
				  &fn_idx)) != ERROR) {
    CFUNC *fn;
    CVAR *param_prototype;
    int n_th_arg;
    if ((fn = get_function (M_NAME(messages[fn_idx]))) != NULL) {
      for (n_th_arg = 0, param_prototype = fn -> params; 
	   /*
	    *  Need check for valid param prototype in loop
	    *  in case of stdargs functions.
	    */
	   (n_th_arg < arg_idx) && 
	     IS_CVAR(param_prototype); 
	   n_th_arg++, param_prototype = param_prototype -> next)
	;
      if (IS_CVAR(param_prototype)) {
	if ((param_prototype -> type_attrs & CVAR_TYPE_OBJECT) &&
	    param_prototype -> n_derefs == 1) {
	  return rt_expr;
	}
      }
    }
  }

  switch (fmt_arg_type (messages, idx, stack_start (messages)))
    {
    case fmt_arg_char_ptr:
      obj_return_class_name = return_classes[STRING_RETURN_CLASS_IDX];
      return fmt_rt_return (rt_expr, obj_return_class_name, keep, expr_out);
      break;
    case fmt_arg_ptr:
      obj_return_class_name = return_classes[SYMBOL_RETURN_CLASS_IDX];
      return fmt_rt_return (rt_expr, obj_return_class_name, keep, expr_out);
      break;
    case fmt_arg_char:
      obj_return_class_name = return_classes[CHARACTER_RETURN_CLASS_IDX];
      return fmt_rt_return (rt_expr, obj_return_class_name, keep, expr_out);
      break;
    case fmt_arg_double:
      obj_return_class_name = return_classes[FLOAT_RETURN_CLASS_IDX];
      return fmt_rt_return (rt_expr, obj_return_class_name, keep, 
			      expr_out);
      break;
    case fmt_arg_int:
      obj_return_class_name = return_classes[INTEGER_RETURN_CLASS_IDX];
      return fmt_rt_return (rt_expr, obj_return_class_name, keep, expr_out);
      break;
    case fmt_arg_long_int:
      return fmt_rt_return (rt_expr, INTEGER_CLASSNAME_L, keep, expr_out);
      break;
    case fmt_arg_long_long_int:
      return fmt_rt_return (rt_expr, LONGINTEGER_CLASSNAME, keep, expr_out);
      break;
    case fmt_arg_null:
      break;
    }


  if (!method) {
    /*
     *  Look for an instance variable message and use the
     *  instance variable's class as the return class,
     *  or the object's class if necessary.
     */
    OBJECT *instancevar;
    if ((instancevar = __ctalkGetInstanceVariable (object, M_NAME(orig),
						   TRUE)) == NULL) {
      obj_return_class_name = object -> __o_classname;
    } else {
	if (DEFAULTCLASS_CMP(__VALUE_INSTVAR(instancevar),
			     ct_defclasses->p_boolean_class,
			     BOOLEAN_CLASSNAME) ||
	    DEFAULTCLASS_CMP(__VALUE_INSTVAR(instancevar),
			     ct_defclasses->p_string_class,
			     STRING_CLASSNAME) ||
	    DEFAULTCLASS_CMP(__VALUE_INSTVAR(instancevar),
			     ct_defclasses->p_integer_class,
			     INTEGER_CLASSNAME) ||
	    DEFAULTCLASS_CMP(__VALUE_INSTVAR(instancevar),
			     ct_defclasses->p_character_class,
			     CHARACTER_CLASSNAME) ||
	    DEFAULTCLASS_CMP(__VALUE_INSTVAR(instancevar),
			     ct_defclasses->p_float_class,
			     FLOAT_CLASSNAME) ||
	    DEFAULTCLASS_CMP(__VALUE_INSTVAR(instancevar),
			     ct_defclasses->p_longinteger_class,
			     LONGINTEGER_CLASSNAME))
	obj_return_class_name = __VALUE_INSTVAR_CLASSNAME(instancevar);
      else
	obj_return_class_name = object -> __o_classname;
    }
  } else {
    /*
     *  Use the method return class as the expression
     *  return class.
     */
    if (!strcmp (method -> returnclass, "Character") ||
	!strcmp (method -> returnclass, "Float") ||
	!strcmp (method -> returnclass, "Integer") ||
	!strcmp (method -> returnclass, "LongInteger") ||
	!strcmp (method -> returnclass, "String")) {
      obj_return_class_name = method ->  returnclass;
    } else {
      if (orig -> attrs & OBJ_IS_CLASS_VAR) {
	obj_return_class_name = (object  -> instancevars ?
				 object -> instancevars -> __o_classname :
				 object  -> __o_classname);
      } else {
	if (!strncmp (rt_expr, EVAL_EXPR_FN, EVAL_EXPR_FN_LENGTH)) {
	  obj_return_class_name  = return_classes[OBJECT_RETURN_CLASS_IDX];
	} else {
	  obj_return_class_name = object -> __o_classname;
	}
      }
    }
  }

  /*
   *  Generic translation in a control block.
   */
  if (ctrlblk_pred && !strcmp (obj_return_class_name, OBJECT_CLASSNAME)) {
    CTRLBLK *__c;
    int __i;
    __c = ctrlblk_pop ();
    ctrlblk_push (__c);
    for (__i = __c -> pred_start_ptr; __i > __c -> pred_end_ptr; __i--) {
      if (M_TOK(messages[__i]) == LABEL)
	break;
    }
    if (__i == idx)
      obj_return_class_name = return_classes[INTEGER_RETURN_CLASS_IDX];
  }

  /*
   *  Variable translation on right side of 
   *    <cvar> = <arg-expr>
   *  assignment.   Instead of adjusting the return 
   *  class of an expression, which are generally correct
   *  for the *value* of the return, here we'll simply add a 
   *  typecast if necessary.
   */
  if ((prev_tok_idx = prevlangmsg (messages, idx)) != ERROR) {
    if (M_TOK(messages[prev_tok_idx]) == EQ) {
      int __lvalue_tok_idx;
      if ((__lvalue_tok_idx = prevlangmsg (messages, prev_tok_idx)) != ERROR) {
	CVAR *lvalue_cvar;
	if (((lvalue_cvar = get_local_var (M_NAME(messages[__lvalue_tok_idx])))
	    != NULL) ||
	    ((lvalue_cvar = get_global_var (M_NAME(messages[__lvalue_tok_idx])))
	     != NULL)) {
	  if (!strcmp (basic_class_from_cvar
		       (messages[idx],
			lvalue_cvar, 0),
		       OBJECT_CLASSNAME))
	    need_cast_to_OBJECT_ptr = TRUE;
	}
      }
    }
  }

  if (need_cast_to_OBJECT_ptr) {
#ifdef __GCC__
    /* GCC can handle the overlapping buffers - not sure about other
       compilers. */
    strcatx (rt_expr, "(OBJECT *)", 
	     fmt_rt_return (rt_expr, obj_return_class_name, keep, expr_out),
	     NULL);
    return rt_expr;
#else
    strcatx (tmp_buffer, "(OBJECT *)", 
	     fmt_rt_return (rt_expr, obj_return_class_name, keep, expr_out),
	     NULL);
    strcpy (rt_expr, tmp_buffer);
    return rt_expr;
#endif
  } else {
    return fmt_rt_return (rt_expr, obj_return_class_name, keep, expr_out);
  }
}

/*
 *  This function is, for now, only called for method
 *  expressions that have arguments.
 *  If the return class of the method is, "Any," then
 *  we should be able to use the class of the method
 *  argument(s).
 */
char *array_obj_2_c_wrapper (MESSAGE_STACK messages, int idx, OBJECT *object, 
		       METHOD *method, char *rt_expr) {

  char return_class_name[MAXLABEL];
  OBJECT *instancevar;
  static char buf[MAXMSG];
  COLLECTION_CONTEXT context;
  int stack_begin;
  int rcvr_idx, arglist_start_idx, arglist_end_idx, expr_start_idx, stack_top_idx;
  int expr_paren_level, expr_open_paren_idx, expr_close_paren_idx;

  if (!strcmp (method -> returnclass, "Character") ||
      !strcmp (method -> returnclass, "Float") ||
      !strcmp (method -> returnclass, "Integer") ||
      !strcmp (method -> returnclass, "LongInteger") ||
      !strcmp (method -> returnclass, "String")) {
    return fmt_rt_return (rt_expr, method -> returnclass, TRUE, buf);
  } else {
    if (!strcmp (method -> returnclass, "Any")) {
      if (method -> args[0] && 
	  ((instancevar = method -> args[0] -> obj) != NULL)) {
	strcpy (return_class_name, instancevar -> __o_classname);
      } else {
	strcpy (return_class_name, object -> __o_classname);
      }
    }
  }
  
  stack_top_idx = stack_start (messages);
  for (rcvr_idx = idx + 1; rcvr_idx <= stack_top_idx; rcvr_idx++) {
    if (!strcmp (M_NAME(messages[rcvr_idx]), object->__o_name))
      break;
  }
  arglist_start_idx = nextlangmsg (messages, idx);
  arglist_end_idx = 
    method_arglist_limit (messages, arglist_start_idx, method -> n_args,
			  method -> varargs);
  /*
   *  Check for opening parentheses on the expression.
   */
  expr_paren_level = expr_outer_parens (messages, rcvr_idx, arglist_end_idx, 
                     stack_start (messages), get_stack_top (messages),
		     &expr_open_paren_idx, &expr_close_paren_idx);

  expr_start_idx = rcvr_idx;
  if (expr_paren_level) {
    int __t;
    __t = expr_start_idx;
    while ((__t = prevlangmsg (messages, __t)) != ERROR) {
      if (M_TOK(messages[__t]) == OPENPAREN) {
	expr_start_idx = __t;
	continue;
      }
      break;
    }
  }
  context = collection_context (messages, expr_start_idx);

  switch (context) 
    {
    case int_collection_context:
      strcatx (buf, ARRAY_INT_TRANS_FN, " (", rt_expr, ")", NULL);
      break;
    case char_ptr_collection_context:
      strcatx (buf, STRING_TRANS_FN, " (", rt_expr, ", 1)", NULL);
      break;
    default:
      if (messages[idx] -> attrs & TOK_IS_PRINTF_ARG) {

	stack_begin = stack_start (messages);

	switch (fmt_arg_type (messages, expr_start_idx,
				  stack_begin))
	  {
	  case fmt_arg_char_ptr:
	    strcatx (buf, ARRAY_TRANS_CHAR_PTR_CTYPE_FN, " (",
		     rt_expr, ")", NULL);
	    break;
	  case fmt_arg_ptr:
	    strcatx (buf, ARRAY_TRANS_PTR_CTYPE_FN, " (", rt_expr,
		     ")", NULL);
	    break;
	  case fmt_arg_char:
	    strcatx (buf, ARRAY_TRANS_CHAR_CTYPE_FN, " (", rt_expr, ")",
		     NULL);
	    break;
	  case fmt_arg_double:
	    strcatx (buf, ARRAY_TRANS_DOUBLE_CTYPE_FN, " (", rt_expr, ")",
		     NULL);
	    break;
	  case fmt_arg_int:
	    strcatx (buf, ARRAY_TRANS_INT_CTYPE_FN, " (", rt_expr, ")",
		     NULL);
	    break;
	    /* should probaly use the scalar classes' trans functions
	       in every case if possible */
	    case fmt_arg_long_int:
	      strcatx (buf, LONGINT_TRANS_FN, " (", rt_expr, ", 1)",
		       NULL);
	    break;
	  case fmt_arg_long_long_int:
	    strcatx (buf, ARRAY_TRANS_LONG_LONG_INT_CTYPE_FN, " (", rt_expr,
		     ")", NULL);
	    break;
	  default:
	    warning (messages[expr_start_idx],
		     "Expression, \"%s,\" has an unrecognized printf () format.",
		     rt_expr);
	    strcatx (buf, ARRAY_TRANS_FN, " (", rt_expr, ")", NULL);
	    break;
	  }

      } else {
	strcatx (buf, ARRAY_TRANS_FN, " (", rt_expr, ")", NULL);
	/* __ctalkToCArrayElement () returns a C-compatible
	   result.  The cast to (int) here prevents a lot
	   of compiler warnings.  Check that it's the first
	   expression in the predicate, though ... the previous
	   label from the receiver message _should_ be, "if,"
           but we'll check back to the start of the frame
           anyway.
	*/
	if (ctrlblk_pred) {
	  int i_1, have_if;
	  for (i_1 = rcvr_idx + 1, have_if = FALSE; 
	       (i_1 <= FRAME_START_IDX) && !have_if; i_1++) {
	    if (M_TOK(messages[i_1]) == LABEL) {
	      if (str_eq (M_NAME(messages[i_1]), "if")) {
		have_if = TRUE;
		char buf_2[MAXMSG];
		strcatx (buf_2, "(int)", buf, NULL);
		strcpy (buf, buf_2);
	      }
	    }
	  }
	}
      }
      break;
    }
  return buf;
}

/*
 *  This function handles some special cases.
 */
char *fmt_rt_return_chk_fn_arg (char *exprbuf, char *returnclassname, 
				int keep, MESSAGE_STACK messages,
				int expr_start_idx) {
  int fn_idx, 
    n_th_arg,
    n;
  CFUNC *fn;
  CVAR *param_cvar;
  char expr_out[MAXMSG];
  /*
   *  Check if the expression is a call to __ctalkEvalExpr
   *  as an argument to a function that needs an OBJECT *
   *  there.
   */
  if (!strncmp (exprbuf, EVAL_EXPR_FN, EVAL_EXPR_FN_LENGTH)) {
    if ((n_th_arg = obj_expr_is_arg (messages, expr_start_idx,
				     stack_start (messages),
				     &fn_idx)) != -1) {
      
      if ((fn = get_function (M_NAME(messages[fn_idx]))) == NULL) {
	if (!is_ctrl_keyword (M_NAME(messages[fn_idx])))
	  warning (messages[fn_idx],
		   "Prototype of C function, \"%s,\" not found.",
		   M_NAME(messages[fn_idx]));
	int i_1, stack_start_idx;
	/* Here we should check for whatever the expression is
	   to the right of. The first of any number of expressions
	   is already taken care of in method_call (). */
	stack_start_idx = stack_start (messages);
	/* Skip past spaces and opening parens. */
	for (i_1 = expr_start_idx + 1; i_1 <= stack_start_idx; ++i_1) {
	  if (M_ISSPACE(messages[i_1]))
	    continue;
	  if (M_TOK(messages[i_1]) != OPENPAREN)
	    break;
	}
	if ((M_TOK(messages[i_1]) == BOOLEAN_AND) || 
	    (M_TOK(messages[i_1]) == BOOLEAN_OR)) {
	  /* 
	   *  Case where we have a complex expression with
	   *  boolean ops between expressions in a ctrlblk predicate:
	   *
	   *   if (<expr> || <expr> ... 
	   *
	   *  It may only be necessary to look for the && or ||
	   *  tokens to decide we need an int return.
	   */
	  strcpy (returnclassname, INTEGER_CLASSNAME);
	}
	return fmt_rt_return (exprbuf, returnclassname, keep, expr_out);
      }
      for (n = 0, param_cvar = fn -> params; 
	   (n < n_th_arg) && 
	     IS_CVAR(param_cvar); ++n)
	param_cvar = param_cvar -> next;
      if (param_cvar) {
	return fmt_rt_return (exprbuf,
			      basic_class_from_cvar
			      (messages[fn_idx],
			       param_cvar, 0), keep, expr_out);
      }
    }
  }
  return fmt_rt_return (exprbuf, returnclassname, keep, expr_out);
}


#ifdef __x86_64
/* So we can use __ctalkToCLong for pointers instead of
   just __ctalk_to_c_long_long.  We also could just
   make a new variation of fmt_rt_return, but
   this is more compact. */
extern bool longinteger_fold_is_ptr;
#endif

static char *rtr_keepstr (int i) {
  switch (i)
    {
    case 0: return "0";
    case 1: return "1";
    case 2: return "2";
    case 3: return "3";
    }
  return "1";
}

char *fmt_rt_return (char *exprbuf, char *returnclassname, int keep,
		     char *expr_out) {
  if (!strcmp (returnclassname, CHARACTER_CLASSNAME) ||
      is_subclass_of (returnclassname, CHARACTER_CLASSNAME)) {

    if (!strcmp (returnclassname, STRING_CLASSNAME) ||
	is_subclass_of (returnclassname, STRING_CLASSNAME)) {
      strcatx (expr_out, STRING_TRANS_FN, " (", exprbuf, ", ", 
	       rtr_keepstr(keep), ")", NULL);
      return expr_out;
    } else {
      strcatx (expr_out, CHAR_TRANS_FN, " (", exprbuf, ")", NULL);
      return expr_out;
    }
  } else {
    /* This is here in case String class isn't loaded yet,
       and is_subclass_of (), above, would need to return ERROR. */
    if (!strcmp (returnclassname, STRING_CLASSNAME)) {
      strcatx (expr_out, STRING_TRANS_FN, "(", exprbuf, ", ",
	       rtr_keepstr(keep), ")", NULL);
      return expr_out;
    }
  }

  if (!strcmp (returnclassname, FLOAT_CLASSNAME) ||
      is_subclass_of (returnclassname, FLOAT_CLASSNAME)) {
    strcatx (expr_out, FLOAT_TRANS_FN, " (", exprbuf, ")", NULL);
    return expr_out;
  }

  if (!strcmp (returnclassname, INTEGER_CLASSNAME) ||
      is_subclass_of (returnclassname, INTEGER_CLASSNAME)) {
    strcatx (expr_out, INT_TRANS_FN, " (", exprbuf, ", ",
	     rtr_keepstr(keep), ")", NULL);
    return expr_out;
  }

  if (!strcmp (returnclassname, BOOLEAN_CLASSNAME) ||
      is_subclass_of (returnclassname, BOOLEAN_CLASSNAME)) {
    strcatx (expr_out, INT_TRANS_FN, " (", exprbuf, ", ",
	     rtr_keepstr(keep), ")", NULL);
    return expr_out;
  }

  if (!strcmp (returnclassname, INTEGER_CLASSNAME_L) ||
      is_subclass_of (returnclassname, INTEGER_CLASSNAME_L)) {
    strcatx (expr_out, LONGINT_TRANS_FN, " (", exprbuf, ", ",
	     rtr_keepstr(keep), ")", NULL);
    return expr_out;
  }

  if (!strcmp (returnclassname, LONGINTEGER_CLASSNAME) ||
      is_subclass_of (returnclassname, LONGINTEGER_CLASSNAME)) {
#ifdef __x86_64
    if (longinteger_fold_is_ptr) {
      strcatx (expr_out, LONGINT_TRANS_FN, " (", exprbuf, ", ",
	       rtr_keepstr(keep), ")", NULL);
      longinteger_fold_is_ptr = false;
    } else {
      strcatx (expr_out, LONGLONGINT_TRANS_FN, " (", exprbuf, ", ",
	       rtr_keepstr(keep), ")", NULL);
    }
#else    
    strcatx (expr_out, LONGLONGINT_TRANS_FN, " (", exprbuf, ", ",
	     rtr_keepstr(keep), ")", NULL);
#endif
    return expr_out;
  }

  if (!strcmp (returnclassname, KEY_CLASSNAME) ||
      is_subclass_of (returnclassname, KEY_CLASSNAME)) {
    strcatx (expr_out, PTR_TRANS_FN_U, " (", exprbuf, ")", NULL);
    return expr_out;
  }

  if (!strcmp (returnclassname, OBJECT_CLASSNAME)) {
    strcpy (expr_out, exprbuf);
    return expr_out;
  }

  /*
   *  Otherwise, just return the value pointer.
   */

#if defined (__GNUC__) && defined (i386)
  if (ptr_fmt_is_alt_int_fmt)
    strcatx (expr_out, ALT_PTR_FMT_CAST, PTR_TRANS_FN, 
	     " (", exprbuf, ")", NULL);
  else
    strcatx (expr_out, PTR_TRANS_FN, " (", exprbuf, ")", NULL);
#else
  /* Untested anywhere else. */
    strcatx (expr_out, PTR_TRANS_FN, " (", exprbuf, ")", NULL);
#endif

  return expr_out;
}

static struct __C_type_conv {
  int type_attr;
  char c_type_name[MAXLABEL];
  int n_derefs;
  char class_name[MAXLABEL];
} __C_type_conv [] = {

  {CVAR_TYPE_LONGLONG, "long long int", 0, LONGINTEGER_CLASSNAME},
  {CVAR_TYPE_LONGLONG, "long long int", 1, SYMBOL_CLASSNAME},
  {CVAR_TYPE_LONGLONG, "long long int", 2, ARRAY_CLASSNAME},
#ifdef __x86_64
  {CVAR_TYPE_LONG,     "long int", 0, LONGINTEGER_CLASSNAME},
#else
  {CVAR_TYPE_LONG,     "long int", 0, INTEGER_CLASSNAME},
#endif  
  {CVAR_TYPE_LONG,     "long int", 1, SYMBOL_CLASSNAME},
  {CVAR_TYPE_LONG,     "long int", 2, ARRAY_CLASSNAME},
  {CVAR_TYPE_INT,      "int", 0, INTEGER_CLASSNAME},
  {CVAR_TYPE_OBJECT,   "OBJECT", 1, OBJECT_CLASSNAME},
  {CVAR_TYPE_CHAR,     "char", 0, CHARACTER_CLASSNAME},
  {CVAR_TYPE_CHAR,     "char", 1, STRING_CLASSNAME},
  {CVAR_TYPE_CHAR,     "char", 2, ARRAY_CLASSNAME},
  {CVAR_TYPE_INT,      "int", 1, SYMBOL_CLASSNAME},
  {CVAR_TYPE_INT,      "int", 2, ARRAY_CLASSNAME},
  {0,                  "struct", 1, SYMBOL_CLASSNAME},
  {CVAR_TYPE_DOUBLE,   "double", 0, FLOAT_CLASSNAME},
  {CVAR_TYPE_DOUBLE,   "double", 1, SYMBOL_CLASSNAME},
  {CVAR_TYPE_DOUBLE,   "double", 2, ARRAY_CLASSNAME},
  {CVAR_TYPE_FLOAT,    "float", 0, FLOAT_CLASSNAME},
  {CVAR_TYPE_FLOAT,    "float", 1, SYMBOL_CLASSNAME},
  {CVAR_TYPE_FLOAT,    "float", 2, ARRAY_CLASSNAME},
  {CVAR_TYPE_OBJECT,   "OBJECT", 0, OBJECT_CLASSNAME},
  {CVAR_TYPE_OBJECT,   "OBJECT", 2, OBJECT_CLASSNAME},
  /* Note that CVAR_TYPE_STRUCT is not specific enough, so we check
     the type label. */
  {0,                  "_object", 0, OBJECT_CLASSNAME},
  {0,                  "_object", 1, OBJECT_CLASSNAME},
  {0,                  "_object", 2, OBJECT_CLASSNAME},
  {CVAR_TYPE_VOID,     "void", 0, OBJECT_CLASSNAME},
  {CVAR_TYPE_VOID,     "void", 1, SYMBOL_CLASSNAME},
  {CVAR_TYPE_VOID,     "void", 2, SYMBOL_CLASSNAME},
  {0,                  "EXCEPTION", 0, INTEGER_CLASSNAME},
  {CVAR_TYPE_BOOL,     "_Bool", 0, BOOLEAN_CLASSNAME},
  {CVAR_TYPE_BOOL,     "_Bool", 1, SYMBOL_CLASSNAME},
  {CVAR_TYPE_BOOL,     "_Bool", 2, SYMBOL_CLASSNAME},
  /* this might need to be combined with the type of array member */
  {CVAR_ATTR_STRUCT_TAG, "void", 1, SYMBOL_CLASSNAME},
  {CVAR_ATTR_STRUCT_TAG, "void", 2, SYMBOL_CLASSNAME},
  /*
   *  Types defined by C99.
   */
#if defined (__linux__)
  {CVAR_TYPE_FILE,     "_IO_FILE", 0, "FileStream"},
  {CVAR_TYPE_FILE,     "_IO_FILE", 1, "FileStream"},
  {CVAR_TYPE_FILE,     "_IO_FILE", 2, "FileStream"},
#else
# if defined (__sparc__)
  {CVAR_TYPE_FILE,     "__FILE", 0, "FileStream"},
  {CVAR_TYPE_FILE,     "__FILE", 1, "FileStream"},
  {CVAR_TYPE_FILE,     "__FILE", 2, "FileStream"},
# endif /* #if defined (__sparc__) */
#endif /* #if defined (__linux__) */
  {CVAR_TYPE_FILE,     "FILE", 0, "FileStream"},
  {CVAR_TYPE_FILE,     "FILE", 1, "FileStream"},
  {CVAR_TYPE_FILE,     "FILE", 2, "FileStream"},
  {0,                  "GC", 0, SYMBOL_CLASSNAME},
  {0,                  "GC", 1, SYMBOL_CLASSNAME},
  {0,                  "GC", 2, SYMBOL_CLASSNAME},
  {0,                  "size_t", 0, INTEGER_CLASSNAME},
  {0,                  "size_t", 1, SYMBOL_CLASSNAME},
  {0,                  "size_t", 2, SYMBOL_CLASSNAME},
  {0,                  "GLubyte", 0, CHARACTER_CLASSNAME},
  {0,                  "GLubyte", 1, STRING_CLASSNAME},
  {0,                  "GLbyte", 0, CHARACTER_CLASSNAME},
  {0,                  "GLbyte", 1, STRING_CLASSNAME},
  {0,                  "GLuint", 0, INTEGER_CLASSNAME},
  {0,                  "GLuint", 1, SYMBOL_CLASSNAME},
  {0,                  "GLint", 0, INTEGER_CLASSNAME},
  {0,                  "GLint", 1, SYMBOL_CLASSNAME},
  {0, "", -1, ""}
};

static char int_decl[] = "int";
static char long_long_int_decl[] = "long long int";
/* index in __C_type_conv array of the frequently used
   classes. */
#define INTEGER0 0
#define OBJECT1 1
#define SYMBOL1 24

char *get_type_conv_name (char *s) {
  int i;
  for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
    if (!strcmp (s, __C_type_conv[i].c_type_name))
      return (__C_type_conv[i].c_type_name);
  }
  return NULL;
}

static bool __have_unknown_c_type = false;
bool have_unknown_c_type (void) {
  return __have_unknown_c_type;
}

/* when changing this, also update basic_class_from_paramcvar, below. */

char *basic_class_from_cvar (MESSAGE *m_orig, CVAR *c, int n_expr_derefs){
  char *p_type;
  CVAR *type_alias;
  int i;

  __have_unknown_c_type = false;

  if (!IS_CVAR(c)) {
    _warning ("basic_class_from_cvar: "
	      "Invalid CVAR, class defaulting to Integer.\n");
    __have_unknown_c_type = true;
    return __C_type_conv[INTEGER0].class_name;
  }
    

  if (c -> type_attrs != 0) {
    for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
      if ((c -> type_attrs & __C_type_conv[i].type_attr) &&
	  ((c -> n_derefs - n_expr_derefs) ==
	   __C_type_conv[i].n_derefs)) {
	return __C_type_conv[i].class_name;
      }
    }
  }

  /* If we didn't match a type attribute, try
     to match the type by name. */
  if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
    p_type = long_long_int_decl;
  } else if (c -> type_attrs & CVAR_TYPE_LONG) {
      p_type = int_decl;
  } else {
#ifdef __GNUC__
    if (is_gnu_extension_keyword (c -> type)) {
      p_type = c -> qualifier;
    } else {
      p_type = c -> type;
    }
#else
    p_type = c -> type;
#endif
  }

  for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
    if (!strcmp (__C_type_conv[i].c_type_name, p_type) &&
	((c -> n_derefs - n_expr_derefs) ==
	 __C_type_conv[i].n_derefs)) {
      return __C_type_conv[i].class_name;
    }
  }
      
  if ((type_alias = get_typedef (c -> type)) != NULL) {
    if (type_alias == c) {
      __have_unknown_c_type = true;
      warn_unknown_c_type (m_orig, c -> name);
      return __C_type_conv[INTEGER0].class_name;
    } else {
      bool n_deref_adj = false;
      __have_unknown_c_type = true;
      int orig_n_derefs = 0;
      char *derived_class;
      /* TODO work through this as we have examples. */
      if (c -> n_derefs > 0 && type_alias -> n_derefs == 0) {
	orig_n_derefs = type_alias -> n_derefs;
	type_alias -> n_derefs = c -> n_derefs;
	n_deref_adj = true;
      }
      derived_class =
	basic_class_from_cvar (m_orig, type_alias, n_expr_derefs);
      if (n_deref_adj) {
	type_alias -> n_derefs = orig_n_derefs;
      }
      /* return something that isn't going away. */
      for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
	if (!strcmp (__C_type_conv[i].class_name, derived_class))
	  return __C_type_conv[i].class_name;
      }
    }
  } else {
    __have_unknown_c_type = true;
    warn_unknown_c_type (m_orig, c -> type);
    return __C_type_conv[INTEGER0].class_name;
  }

  /*
   *  TODO - This is needed if get_typedef looks only 
   *  in the declared_typedefs hash.
   */
  if ((c -> type_attrs & CVAR_TYPE_OBJECT) &&
      (c -> n_derefs == 1)) {
    return __C_type_conv[OBJECT1].class_name;
  }
  return NULL;
}

/* make sure this works similarly to basic_class_from_cvar. */
/* it's useful mainly to avoid conversions from CVAR to PARAMCVAR
   and back. */
char *basic_class_from_paramcvar (MESSAGE *m_orig, PARAMCVAR *c, 
				  int n_expr_derefs) {
  char *p_type;
  CVAR *type_alias;
  int i, derefs;

  __have_unknown_c_type = false;

  if (!IS_CVAR(c)) {
    _warning ("basic_class_from_paramcvar: "
	      "Invalid CVAR, class defaulting to Integer.\n");
    __have_unknown_c_type = true;
    return __C_type_conv[INTEGER0].class_name;
  }
    

  if ((c -> type && c -> qualifier && c -> qualifier2) &&
      !strcmp (c -> type, "int") &&
      !strcmp (c -> qualifier, "long") &&
      !strcmp (c -> qualifier2, "long")) {
    p_type = long_long_int_decl;
  } else {
    if (!strcmp (c -> type, "long") &&
	(c -> qualifier == NULL) && 
	(c -> qualifier2 == NULL)) {
      p_type = int_decl;
    } else {
      if (!strcmp (c -> type, "long") &&
	  !strcmp (c -> qualifier, "long") && 
	  (c -> qualifier2 == NULL)) {
	p_type = long_long_int_decl;
      } else {
#ifdef __GNUC__
	if (is_gnu_extension_keyword (c -> type)) {
	  p_type = c -> qualifier;
	} else {
	  p_type = c -> type;
	}
#else
	p_type = c -> type;
#endif
      }
    }
  }

  derefs = c -> n_derefs - n_expr_derefs;
  for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
    if (!strcmp (__C_type_conv[i].c_type_name, p_type) &&
	(derefs == __C_type_conv[i].n_derefs)) 
      return __C_type_conv[i].class_name;
  }
      
  if ((type_alias = get_typedef (c -> type)) != NULL) {
    if ((c -> name && *type_alias -> name) &&
	(c  -> type && *type_alias  -> type)) {
      if (str_eq (c -> name, type_alias -> name) &&
	  str_eq (c -> type, type_alias -> type)) {
	__have_unknown_c_type = true;
	warn_unknown_c_type (m_orig, c -> name);
	return __C_type_conv[INTEGER0].class_name;
      } else {
	__have_unknown_c_type = true;
	return basic_class_from_cvar (m_orig, type_alias,
				      n_expr_derefs);
      }
    } else if (c -> name && *type_alias -> name) {
      if (str_eq (c -> name, type_alias -> name)) {
	__have_unknown_c_type = true;
	warn_unknown_c_type (m_orig, c -> name);
	return __C_type_conv[INTEGER0].class_name;
      } else {
	__have_unknown_c_type = true;
	return basic_class_from_cvar
	  (m_orig, type_alias, n_expr_derefs);
      }
    } else {
      __have_unknown_c_type = true;
      return basic_class_from_cvar
	(m_orig, type_alias, n_expr_derefs);
    }
  } else {
    __have_unknown_c_type = true;
    warn_unknown_c_type (m_orig, c -> type);
    return __C_type_conv[INTEGER0].class_name;
  }
  
  /*
   *  TODO - This is needed if get_typedef looks only 
   *  at declared_typedefs.
   */
  if (c -> name) {
    if ((c -> type_attrs & CVAR_TYPE_OBJECT) &&
	(c -> n_derefs == 1)) {
      return __C_type_conv[OBJECT1].class_name;
    }
  }
  return NULL;
}

char *basic_class_from_cfunc (MESSAGE *m_orig, CFUNC *c, int n_expr_derefs) {
  CVAR *type_alias, *type_alias_next = NULL;
  char *type_p = NULL;
  int i, derefs;

  if ((derefs = c -> return_derefs - n_expr_derefs) < 0) {
    warning (m_orig, "Function %s has too many dereferences in expression.",
	     c -> decl);
    derefs = 0;
  }

  /* First, try to match a type attribute if possible. */
  if (c -> return_type_attrs != 0) {
    for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
      if ((c -> return_type_attrs & __C_type_conv[i].type_attr) &&
	  (derefs == __C_type_conv[i].n_derefs)) 
	return __C_type_conv[i].class_name;
    }
  }
  
  type_p = c -> return_type;

  if (c -> return_type_attrs & CVAR_TYPE_LONGLONG) {
    type_p = long_long_int_decl;
  } else if ((c -> return_type_attrs & CVAR_TYPE_LONG) ||
	     (c -> return_type_attrs & CVAR_TYPE_INT)) {
#ifdef __GNUC__
    if (is_gnu_extension_keyword (c -> return_type)) {
      type_p = c -> qualifier_type;
    } else {
      type_p = c -> return_type;
    }
#else
    type_p = c -> return_type;
#endif
  }

  for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
    if (!strcmp (__C_type_conv[i].c_type_name, type_p) &&
	(derefs == __C_type_conv[i].n_derefs)) 
      return __C_type_conv[i].class_name;
  }

  if ((type_alias = get_typedef (c -> return_type)) != NULL) {
    for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
      if (str_eq (__C_type_conv[i].c_type_name, type_alias -> type)) {
	return __C_type_conv[i].class_name;
      }
    }
  }

  if (type_alias != NULL) {
    while (type_alias_next != type_alias) {
      if (type_alias_next != NULL)
	type_alias = type_alias_next;
      if ((type_alias_next = get_typedef (type_alias -> type)) != NULL) {
	for (i = 0; __C_type_conv[i].n_derefs != -1; i++) {
	  if (str_eq (__C_type_conv[i].c_type_name,
		      type_alias_next -> type)) {
	    return __C_type_conv[i].class_name;
	  }
	}
      }
    }
  }

  warning (m_orig,
	   "Function, \"%s,\" return type, \"%s,\" "
	   "not found. Return class defaulting to Integer.",
	   c -> decl, c -> return_type);

  return __C_type_conv[INTEGER0].class_name;

}

/*
 * Returns the name of a class for any message for which 
 * CONSTANT_TOK () is true.
 */
char *basic_class_from_constant_tok (MESSAGE *m_tok) {
  if (CONSTANT_TOK(m_tok)) {
    switch (M_TOK(m_tok))
      {
      case LITERAL:
	return return_classes[STRING_RETURN_CLASS_IDX];
	break;
      case LITERAL_CHAR:
	return return_classes[CHARACTER_RETURN_CLASS_IDX];
	break;
      case LONG:
      case INTEGER:
	return return_classes[INTEGER_RETURN_CLASS_IDX];
	break;
      case FLOAT:
	return return_classes[FLOAT_RETURN_CLASS_IDX];
	break;
      case LONGLONG:
	return return_classes[LONGINTEGER_RETURN_CLASS_IDX];
	break;
      }
  } else {
    return NULL;
  }
  return NULL;
}

int is_translatable_basic_class (char *classname) {
  if (!strcmp (classname, CHARACTER_CLASSNAME) ||
      is_subclass_of (classname, CHARACTER_CLASSNAME)) {
    return TRUE;
  }

  if (!strcmp (classname, FLOAT_CLASSNAME) ||
      is_subclass_of (classname, FLOAT_CLASSNAME)) {
    return TRUE;
  }

  if (!strcmp (classname, INTEGER_CLASSNAME) ||
      is_subclass_of (classname, INTEGER_CLASSNAME)) {
    return TRUE;
  }

  if (!strcmp (classname, LONGINTEGER_CLASSNAME) ||
      is_subclass_of (classname, LONGINTEGER_CLASSNAME)) {
    return TRUE;
  }

  return FALSE;
}

/*
 *  In most cases, if the input contains a CVAR for a function
 *  argument, we can just use it and let the compiler warn of
 *  any type mismatches.  This is here just in case we want to
 *  provide some type checking ourselves.  Called by
 *  fn_param_return_trans ().
 */
int match_c_type (CVAR *c1, CVAR *c2) {
  return TRUE;
}

char *basic_class_from_fmt_arg (MESSAGE_STACK messages, int arg_tok_idx) {

  switch (fmt_arg_type (messages, arg_tok_idx, stack_start (messages)))
    {
    case fmt_arg_char_ptr:
      return STRING_CLASSNAME;
      break;
    case fmt_arg_ptr:
      return SYMBOL_CLASSNAME;
      break;
    case fmt_arg_char:
      return CHARACTER_CLASSNAME;
      break;
    case fmt_arg_double:
      return FLOAT_CLASSNAME;
      break;
    case fmt_arg_int:
      return INTEGER_CLASSNAME;
      break;
    case fmt_arg_long_int:
      return INTEGER_CLASSNAME;
      break;
    case fmt_arg_long_long_int:
      return LONGINTEGER_CLASSNAME;
      break;
    case fmt_arg_null:
      break;
    }

  return NULL;
}

/* As a special case, if we have a void * or void ** lval
   and a Symbol object as the start of the rval, return
   "Symbol", so we can use __ctalk_to_c_ptr to assign the
   object's value to the CVAR, instead of the object itself. 
   This function also checks for a prefix operator, which
   is what the normal semantics would suggest when assigning
   a value (then dereferenced by a '*', to a void *. */
static bool symbol_rval_to_void_ptr_lval (MESSAGE_STACK messages,
					  int rval_start_idx,
					  CVAR *c) {
  if (str_eq (c -> type, "void") && (c -> n_derefs > 0)) {
    if (IS_OBJECT(messages[rval_start_idx] -> obj)) {
      if (str_eq (messages[rval_start_idx] -> obj -> __o_classname,
		  SYMBOL_CLASSNAME) ||
	  is_subclass_of (messages[rval_start_idx]->obj->__o_classname,
			  SYMBOL_CLASSNAME)) {
	return true;
      }
    } else {
      if ((messages[rval_start_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) &&
	  (M_TOK(messages[rval_start_idx]) == ASTERISK)) {
	int _next_idx;
	if ((_next_idx = nextlangmsg (messages, rval_start_idx)) != ERROR) {
	  if (IS_OBJECT(messages[_next_idx] -> obj)) {
	    if (str_eq (messages[_next_idx] -> obj -> __o_classname,
			SYMBOL_CLASSNAME) ||
		is_subclass_of (messages[_next_idx]->obj->__o_classname,
				SYMBOL_CLASSNAME)) {
	      return true;
	    }
	  }
	}
      }
    }
  }
  return false;
}

/*
 *  Returns the name of the lval cvar's class for cases like 
 *  <cvar> <assignment_op> <object_expr>
 */
char *c_lval_class (MESSAGE_STACK messages, int rval_start_idx) {
  int assignment_op_idx, lval_idx;
  int i, stack_start_idx,  n_subscripts = 0;
  CVAR *c;

  if ((assignment_op_idx = prevlangmsg (messages, rval_start_idx)) 
      == ERROR)
    return NULL;
  if (!IS_C_ASSIGNMENT_OP(M_TOK(messages[assignment_op_idx]))) {
    if (M_TOK(messages[assignment_op_idx]) == OPENPAREN) {
      /* backtrack through opening parens (only opening parens,
	 not close parens for now) if necessary. */
      stack_start_idx = stack_start (messages);
      for  (i = assignment_op_idx; i <= stack_start_idx; ++i) {
	if (M_ISSPACE(messages[i]))
	  continue;
	if (M_TOK(messages[i]) == OPENPAREN)
	  continue;
	if (IS_C_ASSIGNMENT_OP(M_TOK(messages[i]))) {
	  assignment_op_idx = i;
	  break;
	} else {
	  return NULL;
	}
      }
    } else {
      return NULL;
    }
  }
  if ((lval_idx = prevlangmsg (messages, assignment_op_idx)) == ERROR)
    return NULL;
  
  if (M_TOK(messages[lval_idx]) == ARRAYCLOSE) {
    /* backtrack through subscripts if any. */
    stack_start_idx = stack_start (messages);
    /* here lval_idx points to the first subscript brace. */
    lval_idx = match_array_brace_rev (messages, lval_idx,
				      stack_start_idx,
				      &n_subscripts);
    if ((lval_idx = prevlangmsg (messages, lval_idx)) == ERROR) 
      return NULL;
  }

  if (argblk) {
    if ((c = get_var_from_cvartab_name (M_NAME(messages[lval_idx])))
	!= NULL) {
      if (symbol_rval_to_void_ptr_lval (messages, rval_start_idx, c)) {
	return __C_type_conv[SYMBOL1].class_name;
      } else {
	return basic_class_from_cvar (messages[lval_idx], c, 
				      n_subscripts);
      }
    }
  } else {
    if (((c = get_local_var (M_NAME(messages[lval_idx]))) != NULL) ||
	((c = get_global_var (M_NAME(messages[lval_idx]))) != NULL)) {
      if (symbol_rval_to_void_ptr_lval (messages, rval_start_idx, c)) {
	return __C_type_conv[SYMBOL1].class_name;
      } else {
 	return basic_class_from_cvar (messages[lval_idx], c, 
					n_subscripts);
      }
    }
  }
  return NULL;
}

/*
 *  This is a fixup if we have an argument block expression like
 *  
 *  <cvar> <assignment_op> eval <expr>
 *
 *  And <cvar> is declared as a char *.
 */
void argblk_ptrptr_trans_1 (MESSAGE_STACK messages, int rval_start_idx) {
  int assignment_op_idx, lval_idx;
  char tmp[MAXLABEL];

  if ((assignment_op_idx = prevlangmsg (messages, rval_start_idx)) 
      == ERROR)
    return;
  if (!IS_C_ASSIGNMENT_OP(M_TOK(messages[assignment_op_idx]))) 
    return;
  if ((lval_idx = prevlangmsg (messages, assignment_op_idx)) == ERROR)
    return;
  
  if (messages[lval_idx] -> name[0] == '*') {
    strcpy (tmp, messages[lval_idx] -> name);
    strcpy (messages[lval_idx] -> name, &tmp[1]);
  }
}
