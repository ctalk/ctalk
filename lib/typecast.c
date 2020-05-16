/* $Id: typecast.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2019  Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 *  Type casts and typedefs.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "cvar.h"
#include "typeof.h"

extern CVAR *typedefs;           /* Declared in rt_cvar.c. */
extern CVAR *typedefs_ptr;
extern HASHTAB typedefhash;

extern I_PASS interpreter_pass;  /* Declared in rtinfo.c.  */

extern DEFAULTCLASSCACHE *rt_defclasses;

#define A_IDX_(a,b,ncols) (((a)*(ncols))+(b))

static inline int __l_check_state (int stack_idx, MESSAGE_STACK messages, 
				 int *states, 
				 int cols) {
  int i, j;
  int token;
  int next_idx = stack_idx;

  token = messages[stack_idx] -> tokentype;

  for (i = 0; ; i++) {

  nextrans:
     if (states[A_IDX_(i,0,cols)] == ERROR)
       return ERROR;

     if (states[A_IDX_(i,0,cols)] == token) {

       for (j = 1, next_idx = stack_idx - 1; ; j++, next_idx--) {
	
	 while (states[A_IDX_(i,j,cols)]) {
	   if (!messages[next_idx] || !IS_MESSAGE (messages[next_idx]))
	     return ERROR;
           while (M_ISSPACE(messages[next_idx])) {
	     next_idx--;
	     /* TO DO - 
		Find a way to notify the calling function whether 
		the message is the end of the stack, or determine
		the end of the stack by the calling function.
	     */
	     if (!messages[next_idx] || !IS_MESSAGE (messages[next_idx]))
	       return ERROR;
	   }
	   if (states[A_IDX_(i,j,cols)] != messages[next_idx]->tokentype) {
	     ++i;
	     goto nextrans;
	   }
	   else
	     ++j;
	 }
	 return i;
       }
     }
  }
}

int __ctalkRegisterCTypedef (char *type, 
			     char *qualifier,
			     char *qualifier2,
			     char *qualifier3, 
			     char *qualifier4,
			     char *storage_class,
			     char *name,
			     int n_derefs,
			     int attrs,
			     int is_unsigned,
			     int scope) {
  CVAR *c;

  _new_cvar (CVARREF(c));

  if (type) strcpy (c -> type, type);
  if (qualifier) strcpy (c -> qualifier, qualifier);
  if (qualifier2) strcpy (c -> qualifier2, qualifier2);
  if (qualifier3) strcpy (c -> qualifier3, qualifier3);
  if (qualifier4) strcpy (c -> qualifier4, qualifier4);
  if (storage_class) strcpy (c -> storage_class, storage_class);
  if (name) strcpy (c -> name, name);
  c -> n_derefs = n_derefs;
  c -> attrs = attrs;
  c -> is_unsigned = is_unsigned;
  c -> scope = scope;

  if (typedefhash == NULL)
    _new_hash (&typedefhash);

  _hash_put (typedefhash, c, c -> name);

  return SUCCESS;
}

static int typecast_states[] = {
  #include "../include/typecaststate.h"
  -1, 0, 0
};

#define TYPECAST_STATE_COLS 3

#define TYPECAST_STATE(x) (__l_check_state ((x), messages, \
              typecast_states, TYPECAST_STATE_COLS))
#define TYPECAST_STATE_EXPR(x) (__l_check_state ((x), p -> m_s, \
              typecast_states, TYPECAST_STATE_COLS))

int __rt_is_typecast (MESSAGE_STACK messages, int start, int end) {

  int i,
    state,
    n_parens,
    lookahead,
    cast_expr_paren_lvl;
  MESSAGE *m_tok;

  for (i = start, n_parens = 0, cast_expr_paren_lvl = -1; i >= end; i--) {

    m_tok = messages[i];

    if (M_ISSPACE (m_tok)) continue;

    if ((state = TYPECAST_STATE (i)) == ERROR) {
      /* a failsafe in case there's a token following the closing
	 paren that we don't know about */
      if (M_TOK(m_tok) == CLOSEPAREN) {
	int open_paren_idx;
	if ((open_paren_idx = __ctalkMatchParenRev (messages, i, start))
	    != ERROR) {
	  if (open_paren_idx == start)
	    return TRUE;
	}
      }
      return FALSE;
    }

    switch (m_tok -> tokentype)
      {
      case LABEL:
      case CTYPE:
	if (!is_c_data_type (m_tok -> name) &&
	    !__ctalkGetTypedef (m_tok -> name))
	  return FALSE;
	break;
      case OPENPAREN:
	++n_parens;
	if ((lookahead = __ctalkNextLangMsg (messages, i, 
					     _get_e_message_ptr ()))
	    != ERROR) {
	  if ((M_TOK(messages[lookahead]) == CTYPE) ||
	      (M_TOK(messages[lookahead]) == LABEL)) {
	    if (is_c_data_type (M_NAME(messages[lookahead])) ||
		__ctalkGetTypedef (M_NAME(messages[lookahead])))
	      cast_expr_paren_lvl = n_parens;
	  }
	  /* We want this because only the innermost parens are
	     part of the typecast proper. */
	  if (M_TOK(messages[lookahead]) == OPENPAREN) {
	    return FALSE;
	  }
	}
	break;
      case CLOSEPAREN:
	if (n_parens == cast_expr_paren_lvl) return TRUE;
	--n_parens;
	break;
      default:
	break;
      }
  }

  return FALSE;
}

/* Like above, but called from eval_expr. */
int __rt_is_typecast_expr (EXPR_PARSER *p, int start) {

  int i,
    state,
    n_parens,
    lookahead,
    cast_expr_paren_lvl;
  MESSAGE *m_tok;

  for (i = start, n_parens = 0, cast_expr_paren_lvl = -1;
       i >= p -> msg_frame_top; i--) {

    m_tok = p -> m_s[i];

    if (M_ISSPACE (m_tok)) continue;

    if ((state = TYPECAST_STATE_EXPR (i)) == ERROR) {
      /* a failsafe in case there's a token following the closing
	 paren that we don't know about */
      if (M_TOK(m_tok) == CLOSEPAREN) {
	int open_paren_idx;
	if ((open_paren_idx = __ctalkMatchParenRev (p -> m_s, i, start))
	    != ERROR) {
	  if (open_paren_idx == start)
	    return TRUE;
	}
      }
      return FALSE;
    }

    switch (m_tok -> tokentype)
      {
      case LABEL:
      case CTYPE:
	if (!is_c_data_type (m_tok -> name) &&
	    !__ctalkGetTypedef (m_tok -> name))
	  return FALSE;
	break;
      case OPENPAREN:
	++n_parens;
	if ((lookahead = __ctalkNextLangMsg (p -> m_s, i, 
					     _get_e_message_ptr ()))
	    != ERROR) {
	  if ((M_TOK(p -> m_s[lookahead]) == CTYPE) ||
	      (M_TOK(p -> m_s[lookahead]) == LABEL)) {
	    if (is_c_data_type (M_NAME(p -> m_s[lookahead])) ||
		__ctalkGetTypedef (M_NAME(p -> m_s[lookahead])))
	      cast_expr_paren_lvl = n_parens;
	  }
	  /* We want this because only the innermost parens are
	     part of the typecast proper. */
	  if (M_TOK(p -> m_s[lookahead]) == OPENPAREN) {
	    return FALSE;
	  }
	}
	break;
      case CLOSEPAREN:
	if (n_parens == cast_expr_paren_lvl) return TRUE;
	--n_parens;
	break;
      default:
	break;
      }
  }

  return FALSE;
}

int typecast_is_pointer (MESSAGE_STACK messages, int start, int end) {
  int i,
    n_derefs,
    end_paren;
  
  if ((end_paren = __ctalkMatchParen (messages, start, end)) == ERROR)
    return 0;

  for (i = start, n_derefs = 0; i >= end_paren; i--) 
    if (messages[i] -> tokentype == ASTERISK)
      ++n_derefs;
  return n_derefs;
}

/* The OBJECT * param declaration is a little weird, but it
   works with everything. */
static OBJECT *mk_tc_symbol (OBJECT *val) {
  OBJECT *o;
  o = create_object_init_internal
    ("result", rt_defclasses -> p_symbol_class, LOCAL_VAR, "");
  if (val)
    *(OBJECT **)o -> __o_value =
      *(OBJECT **)o -> instancevars -> __o_value = val;
  return o;
}

/*
 *  For now, it should be sufficient to cast all pointer expressions
 *  to a Symbol.  If the result of the expression is an Integer or
 *  LongInteger, use that as the symbol address.  If the result is a
 *  Character string or array, use the address of the first element.
 *  All other Classes that correspond to derived types should have a
 *  Symbol as the class of the value instance variable.
 */
int typecast_ptr_expr (MESSAGE_STACK messages, int cast_expr_start, 
		       int cast_expr_end, int stack_start, 
		       int stack_end) {
  char expr[MAXMSG],
    valbuf[MAXMSG];
  int i, i_2,
    expr_end,
    cast_start,
    cast_end = -1,  /* Avoid a warning. */
    lookahead,
    scope,
    all_objects_deleted;
  bool void_ptr_cast = false;
  OBJECT *e_result, 
    *e_result_val, 
    *recv_object,
    *recv_class_object,
    *ptr_result = NULL;       /* Avoid a warning. */

  /*
   *  Find the parentheses of the cast.
   */
  for (i = cast_expr_start; i >= cast_expr_end; i--) {
    if (M_TOK(messages[i]) == OPENPAREN) {
      if ((lookahead = __ctalkNextLangMsg (messages, i, 
					   __rt_get_stack_end (messages)))
	  == ERROR) {
	_warning ("End of stack in typecast_ptr_expr.");
      }
      if ((M_TOK(messages[lookahead]) == LABEL) ||
	  (M_TOK(messages[lookahead]) == CTYPE)) {
	if (is_c_data_type (M_NAME(messages[lookahead])) ||
	    __ctalkGetTypedef (M_NAME(messages[lookahead]))) {
	  cast_start = i;
	  if ((cast_end = 
	       __ctalkMatchParen (messages, cast_start,
				  __rt_get_stack_end (messages))) == ERROR) {
	    _warning ("typecast_ptr_expr: mismatched parentheses.\n");
	    return ERROR;
	  }
	  if (str_eq (M_NAME(messages[lookahead]), "void")) {
	    void_ptr_cast = true;
	  }
	}
      }
    }
  }

  /* Then it's a repeat, just return. */
  if ((messages[cast_start] -> attrs & RT_TOK_IS_TYPECAST_EXPR) &&
      (messages[cast_end] -> attrs & RT_TOK_IS_TYPECAST_EXPR))
    return SUCCESS;

  if (cast_end == -1) {
    _warning ("typecast_ptr_expr: Could not find cast end - probably.\n");
    _warning ("typecast_ptr_expr: undefined or unrecognized data type.\n");
    return ERROR;
  }

  recv_object = __ctalk_receiver_pop ();
  __ctalk_receiver_push (recv_object);

  if (recv_object) {
    recv_class_object = recv_object -> __o_class;
    scope = recv_object -> scope;
  } else {
    recv_class_object = __ctalk_get_object (INTEGER_CLASSNAME,
				       INTEGER_SUPERCLASSNAME);
    scope = RECEIVER_VAR;
  }
  if ((e_result = eval_expr (
			     /* expr */
	  collect_typecast_expr (messages, 
				 __ctalkNextLangMsg (messages, cast_end,
				     __rt_get_stack_end (messages)),
				 &expr_end), 
	  recv_class_object, NULL, NULL, scope, 
			     FALSE)) != NULL) {
    e_result_val = e_result -> instancevars ? e_result -> instancevars :
      e_result;
    if ((e_result_val -> attrs & OBJECT_VALUE_IS_BIN_INT) ||
	(e_result_val -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG)) {
      ptr_result = mk_tc_symbol (*(OBJECT **)e_result_val -> __o_value);
    } else if (lextype_is_LITERAL_CHAR_T (e_result_val -> __o_value)) {
      ptr_result = mk_tc_symbol (*(OBJECT **)e_result_val -> __o_value);
    } else {
      __ctalkExceptionInternal (NULL, ptr_conversion_x, "",0);
      ptr_result = mk_tc_symbol (0);
    }
  }

  __ctalkSetObjectScope (ptr_result, ptr_result->scope | TYPECAST_OBJECT);
  for (i = cast_expr_start; i >= cast_expr_end; i--) {
    if (messages[i] && IS_MESSAGE(messages[i])) {
      ++messages[i] -> evaled;
      if (messages[i]->obj) {
	if ((i == cast_expr_start) &&
	    messages[i] -> value_obj && 
	    (messages[i] -> value_obj -> scope & TYPECAST_OBJECT)) {
	  delete_all_value_objs_expr_frame (messages, i,
					    stack_start, stack_end,
					    &all_objects_deleted);
	}
	messages[i]->value_obj = ptr_result;
      } else {
	if ((i == cast_expr_start) &&
	    messages[i] -> obj &&
	    (messages[i] -> obj -> scope & TYPECAST_OBJECT))
	  __ctalkDeleteObject (messages[i]->obj);
	messages[i]->obj = ptr_result;
	messages[i] -> attrs |= RT_TOK_IS_TYPECAST_EXPR;
      }
    }
  }
  __ctalkDeleteObject(e_result);
  return SUCCESS;
}

int typecast_ptr_expr_b (EXPR_PARSER *p, int cast_expr_start, 
			 int cast_expr_end) {
  char expr[MAXMSG],
    valbuf[MAXMSG];
  int i, i_2,
    expr_end,
    cast_start,
    cast_end = -1,  /* Avoid a warning. */
    lookahead,
    scope,
    all_objects_deleted;
  bool void_ptr_cast = false;
  OBJECT *e_result, 
    *e_result_val, 
    *recv_object,
    *recv_class_object,
    *ptr_result = NULL;       /* Avoid a warning. */

  /*
   *  Find the parentheses of the cast.
   */
  for (i = cast_expr_start; i >= cast_expr_end; i--) {
    if (M_TOK(p -> m_s[i]) == OPENPAREN) {
      if ((lookahead = __ctalkNextLangMsg (p -> m_s, i,
					   p -> msg_frame_top))
	  == ERROR) {
	_warning ("End of stack in typecast_ptr_expr.");
      }
      if ((M_TOK(p -> m_s[lookahead]) == LABEL) ||
	  (M_TOK(p -> m_s[lookahead]) == CTYPE)) {
	if (is_c_data_type (M_NAME(p -> m_s[lookahead])) ||
	    __ctalkGetTypedef (M_NAME(p -> m_s[lookahead]))) {
	  cast_start = i;
	  if ((cast_end = 
	       __ctalkMatchParen (p -> m_s, cast_start,
				  p -> msg_frame_top)) == ERROR) {
	    _warning ("typecast_ptr_expr: mismatched parentheses.\n");
	    return ERROR;
	  }
	  if (str_eq (M_NAME(p -> m_s[lookahead]), "void")) {
	    void_ptr_cast = true;
	  }
	}
      }
    }
  }

  /* Then it's a repeat, just return. */
  if ((p -> m_s[cast_start] -> attrs & RT_TOK_IS_TYPECAST_EXPR) &&
      (p -> m_s[cast_end] -> attrs & RT_TOK_IS_TYPECAST_EXPR))
    return SUCCESS;

  if (cast_end == -1) {
    _warning ("typecast_ptr_expr: Could not find cast end - probably.\n");
    _warning ("typecast_ptr_expr: undefined or unrecognized data type.\n");
    return ERROR;
  }

  recv_object = __ctalk_receiver_pop ();
  __ctalk_receiver_push (recv_object);

  if (recv_object) {
    recv_class_object = recv_object -> __o_class;
    scope = recv_object -> scope;
  } else {
    recv_class_object = __ctalk_get_object (INTEGER_CLASSNAME,
				       INTEGER_SUPERCLASSNAME);
    scope = RECEIVER_VAR;
  }
  if ((e_result = eval_expr
       (collect_typecast_expr (p -> m_s, __ctalkNextLangMsg
			       (p -> m_s, cast_end,
				p -> msg_frame_top), &expr_end),
	recv_class_object, NULL, NULL, scope, FALSE))
      != NULL) {
    e_result_val = e_result -> instancevars ? e_result -> instancevars :
      e_result;
    if (e_result_val -> attrs & OBJECT_VALUE_IS_BIN_INT) {
      if (void_ptr_cast && (atoi (e_result_val -> __o_value) == 0)) {
	ptr_result = mk_tc_symbol (0);
      } else {
	ptr_result = mk_tc_symbol (*(OBJECT **)e_result_val -> __o_value);
      }
    } else if (lextype_is_LONGLONG_T (e_result_val -> __o_value) ||
	       lextype_is_LITERAL_CHAR_T (e_result_val -> __o_value)) {
      ptr_result = mk_tc_symbol (*(OBJECT **)e_result_val -> __o_value);
    } else {
      __ctalkExceptionInternal (NULL, ptr_conversion_x, "",0);
      ptr_result = mk_tc_symbol (0);
    }
  }

  __ctalkSetObjectScope (ptr_result, ptr_result->scope | TYPECAST_OBJECT);
  for (i = cast_expr_start; i >= cast_expr_end; i--) {
    if (p -> m_s[i] && IS_MESSAGE(p -> m_s[i])) {
      ++p -> m_s[i] -> evaled;
      if (p -> m_s[i]->obj) {
	if ((i == cast_expr_start) &&
	    p -> m_s[i] -> value_obj && 
	    (p -> m_s[i] -> value_obj -> scope & TYPECAST_OBJECT)) {
	  delete_all_value_objs_expr_frame (p -> m_s, i,
					    p -> msg_frame_start,
					    p -> msg_frame_top,
					    &all_objects_deleted);
	}
	p -> m_s[i]->value_obj = ptr_result;
      } else {
	if ((i == cast_expr_start) &&
	    p -> m_s[i] -> obj &&
	    (p -> m_s[i] -> obj -> scope & TYPECAST_OBJECT))
	  __ctalkDeleteObject (p -> m_s[i]->obj);
	p -> m_s[i]->obj = ptr_result;
	p -> m_s[i] -> attrs |= RT_TOK_IS_TYPECAST_EXPR;
      }
    }
  }
  __ctalkDeleteObject(e_result);
  return SUCCESS;
}

/*
 *  TODO - Most scalar typecasts can be elided, until we have a 
 *  case where the value alone doesn't translate directly to an
 *  object.
 */
int typecast_value_expr (MESSAGE_STACK messages, int cast_start, 
		       int cast_end) {
  int i;
  for (i = cast_start; i >= cast_end; i--) {
    messages[i] -> attrs |= RT_TOK_IS_TYPECAST_EXPR;
    ++(messages[i] -> evaled);
    ++(messages[i] -> output);
  }
  return FALSE;
}

char *collect_typecast_expr (MESSAGE_STACK messages, int expr_start,
			     int *expr_end) {
  static char buf[MAXMSG];
  int stack_end,
    lookahead,
    n_parens,
    n_blocks;

  stack_end = __rt_get_stack_end (messages);

  for (*expr_end = expr_start, *buf = 0, n_blocks = 0, n_parens = 0; 
       *expr_end > stack_end; --(*expr_end)) {

    strcatx2 (buf, messages[*expr_end] -> name, NULL);

    if ((lookahead = __ctalkNextLangMsg (messages, *expr_end, stack_end))
	== ERROR)
      return buf;

    switch (messages[lookahead] -> tokentype)
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (n_parens < 0) return buf;
	break;
      case ARRAYOPEN:
	++n_blocks;
	break;
      case ARRAYCLOSE:
	--n_blocks;
	if (n_blocks < 0) return buf;
	break;
      case ARGSEPARATOR:
      case SEMICOLON:
	return buf;
	break;
      default:
	break;
      }
  }
  return buf;
}

CVAR *__ctalkGetTypedef (char *name) {
  return (CVAR *)_hash_get (typedefhash, name);
}

/*
 *  Sets the expression value (up to the opening typecast paren)
 *  if a constant is the last token.
 */
void constant_typecast_expr (MESSAGE_STACK messages, 
			     int last_tok_idx,
			     int close_paren_idx,
			     int stack_start_idx) {
  int i;
  if (messages[close_paren_idx] -> attrs & RT_TOK_IS_TYPECAST_EXPR) {
    i = close_paren_idx;
    while (messages[i] ->  attrs & RT_TOK_IS_TYPECAST_EXPR) {
      messages[i] -> value_obj = messages[last_tok_idx] -> obj;
      if (++i > stack_start_idx)
	break;
    }
  }
}

static inline int prev_msg (MESSAGE_STACK messages, 
			    int this_msg, int stack_start) {
  int i = this_msg + 1;

  while ( i <= stack_start) {
    if (!M_ISSPACE(messages[i]))
      return i;
    ++i;
  }
  return ERROR;
}

int typecast_or_constant_expr (MESSAGE_STACK messages,
			       int i, 
			       int typecast_open_paren_idx,
			       int typecast_close_paren_idx,
			       EXPR_PARSER *p,
			       METHOD *method,
			       int scope,
			       OBJECT *arg_class_integer,
			       bool typecast_is_ptr) {

  int prev_idx;
  MESSAGE *m = messages[i];

  if (typecast_open_paren_idx != -1 &&
      typecast_close_paren_idx != -1) {
    prev_idx = prev_msg (messages, i, p -> msg_frame_start);
    if (messages[prev_idx] -> attrs & RT_TOK_IS_TYPECAST_EXPR) {
      if (typecast_is_ptr) { 
	/* e.g., ((void *)0), so use the value of the typecast -
	   - We already have a promoted typecast value,
	   created by typecast_ptr_expr, below. */
	messages[i] -> obj = messages[prev_idx] -> obj;
      } else {
	if (const_created_param (messages, i, method, scope,
				 m, arg_class_integer) < 0) {
	  return ERROR;
	}
	constant_typecast_expr (messages, i, prev_idx, p -> msg_frame_start);
      }
      if (i == p -> msg_frame_top && 
	  typecast_open_paren_idx == p -> msg_frame_start)
	return ERROR;
    } else if (const_created_param (messages, i, method, scope,
				    m, arg_class_integer) < 0) {
      return ERROR;
    }
  } else if (const_created_param (messages, i, method, scope,
				  m, arg_class_integer) < 0) {
    return ERROR;
  }
  return SUCCESS;
}

OBJECT *cast_object_to_c_type (MESSAGE_STACK messages, int cast_start,
			       int cast_end, OBJECT *object) {
  OBJECT *cast_object;
  int i, n_derefs = 0, type_label_idx;
  int int_val, tok_idx;
  long long int llint_val;
  double d_val;
  char valbuf[MAXMSG];

  for (i = cast_start; i > cast_end; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if (is_c_data_type(M_NAME(messages[i]))) {
	type_label_idx = i;
      }
    } else if (M_TOK(messages[i]) == MULT) {
      ++n_derefs;
    }
  }
  for (i = cast_end - 1; messages[i]; i--) {
    if (!M_ISSPACE(messages[i])) {
      tok_idx = i;
      break;
    }
  }
  if (str_eq (M_NAME(messages[type_label_idx]), "float") ||
      str_eq (M_NAME(messages[type_label_idx]), "double")) {
    if (str_eq (object -> __o_class -> __o_name, INTEGER_CLASSNAME)) {
      int_val = *(int *)(IS_OBJECT(object -> instancevars) ?
			 object -> instancevars -> __o_value :
			 object -> __o_value);
      sprintf (valbuf, "%lf", (double)int_val);
    } else if (str_eq (object -> __o_class -> __o_name, LONGINTEGER_CLASSNAME)){
      llint_val = strtoll (object -> __o_value, NULL, 0);
      sprintf (valbuf, "%lf", (double)llint_val);
    } else if (str_eq (object -> __o_class -> __o_name, FLOAT_CLASSNAME)) {
      return object;
    } else {
      _error ("Conversion from \"%s\" to \"%s\" is not (yet) implemented "
	      "for typecast\n\n\t%s%s\n\n", object -> __o_class -> __o_name,
	      FLOAT_CLASSNAME,
	      toks2str (messages, cast_start, cast_end, valbuf),
	      object -> __o_name);
    }
    object -> __o_class = rt_defclasses -> p_float_class;
    __xfree (MEMADDR(object -> __o_value));
    object -> __o_value = strdup (valbuf);
    if (IS_OBJECT(object -> instancevars)) {
      object -> instancevars -> __o_class =
	rt_defclasses -> p_float_class;
      __xfree (MEMADDR(object -> instancevars -> __o_value));
      object -> instancevars -> __o_value = strdup (valbuf);
    }
    if (object -> attrs & OBJECT_VALUE_IS_BIN_INT) {
      __ctalkSetObjectAttr (object, object -> attrs & ~OBJECT_VALUE_IS_BIN_INT);
    }
  } else if (str_eq (M_NAME(messages[type_label_idx]), "int")) {
    if (str_eq (object -> __o_class -> __o_name, INTEGER_CLASSNAME)) {
      return object;
    } else if (str_eq (object -> __o_class -> __o_name, LONGINTEGER_CLASSNAME)){
      llint_val = strtoll (object -> __o_value, NULL, 0);
      sprintf (valbuf, "%lf", (double)llint_val);
    } else if (str_eq (object -> __o_class -> __o_name, FLOAT_CLASSNAME)) {
      d_val = strtod (object -> __o_value, NULL);
      sprintf (valbuf, "%d", (int)d_val);
      return object;
    } else {
      _error ("Conversion from \"%s\" to \"%s\" is not (yet) implemented "
	      "for typecast\n\n\t%s%s\n\n", object -> __o_class -> __o_name,
	      INTEGER_CLASSNAME,
	      toks2str (messages, cast_start, cast_end, valbuf),
	      object -> __o_name);
    }
    object -> __o_class = rt_defclasses -> p_integer_class;
    __xfree (MEMADDR(object -> __o_value));
    object -> __o_value = strdup (valbuf);
    if (IS_OBJECT(object -> instancevars)) {
      object -> instancevars -> __o_class =
	rt_defclasses -> p_integer_class;
      __xfree (MEMADDR(object -> instancevars -> __o_value));
      object -> instancevars -> __o_value = strdup (valbuf);
    }
  } else {
    _error ("Conversion from \"%s\" to \"%s\" is not (yet) implemented "
	    "for typecast\n\n\t%s%s\n\n", object -> __o_class -> __o_name,
	    M_NAME(messages[type_label_idx]),
	    toks2str (messages, cast_start, cast_end, valbuf),
	    M_NAME(messages[tok_idx]));
  }
  return object;
}
