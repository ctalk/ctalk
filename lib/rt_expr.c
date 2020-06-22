/* $Id: rt_expr.c,v 1.5 2020/06/22 04:22:08 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2020  Robert Kiesling, rk3314042@gmail.com.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "rtinfo.h"

extern DEFAULTCLASSCACHE *rt_defclasses; /* Defined in rtclslib.c. */

#define CAN_CACHE_METHOD(mthd, msg_prev) \
  ((!(msg_prev -> attrs & RT_OBJ_IS_INSTANCE_VAR) &&	\
    !str_eq (msg_prev -> name, "super") &&		\
    !(msg_prev -> attrs & RT_OBJ_IS_CLASS_VAR)) &&	\
   (mthd -> varargs == 0))

/*
 *  The expression parser stack.
 */

static OBJECT *null_result_obj_2 (METHOD *, OBJECT *, int);

static int expr_parser_lvl = 0;

EXPR_PARSER *expr_parsers[MAXARGS+1];
int expr_parser_ptr = MAXARGS + 1;

int eval_status = 0;

static int __cvar_postfix_method_retry = 0;

extern OBJECT *__ctalk_argstack[MAXARGS+1];
extern int __ctalk_arg_ptr;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

extern RT_INFO rtinfo;
extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

MESSAGE *e_messages[P_MESSAGES+1]; /* Eval message stack.        */
int e_message_ptr = P_MESSAGES;    /* Eval stack pointer.        */

int __get_expr_parser_ptr (void) {
  return expr_parser_ptr;
}

int __get_expr_parser_level (void) {
  return expr_parser_lvl;
}

int expr_n_occurrences (METHOD *m) {
  int i, n;
  n = 0;
  if (C_EXPR_PARSER)
    for (i = 0; i < C_EXPR_PARSER -> e_method_ptr; ++i) {
      if (C_EXPR_PARSER -> e_methods[i] == m)
	++n;
    }
  return n;
}

static OBJECT *reffed_arg_obj (OBJECT *arg_object) {
  OBJECT *r;
  if (!IS_OBJECT(arg_object))
    return NULL;
  if (arg_object -> attrs == OBJECT_VALUE_IS_BIN_SYMBOL) {
    if (IS_OBJECT(arg_object -> instancevars)) {
      if (arg_object -> instancevars -> __o_value) {
	r = *(OBJECT **)arg_object -> instancevars -> __o_value;
	if (IS_OBJECT(r)) {
	  return r;
	}
      }
    } else {
      if (arg_object -> __o_value) {
	r = *(OBJECT **)arg_object -> __o_value;
	if (IS_OBJECT(r)) {
	  return r;
	}
      }
    }
  }
  return NULL;
}

static inline void clear_expr_obj (MESSAGE_STACK messages,
				   int start_idx, int end_idx,
				   OBJECT *obj) {
  int i;
  for (i = start_idx; i >= end_idx; i--) {
    if (messages[i] -> obj == obj)
      messages[i] -> obj = NULL;
  }
}

static inline bool object_is_cvartab_entry (CVAR *c) {
  return (((c -> type_attrs & CVAR_TYPE_OBJECT) &&
	   (c -> attrs & CVAR_ATTR_CVARTAB_ENTRY) &&
	   (c -> n_derefs == 2)) ||
	  (str_eq (c -> type, "OBJECT") &&
	   (c -> attrs & CVAR_ATTR_CVARTAB_ENTRY) &&
	   (c -> n_derefs == 2)));
}

static bool expr_is_assign_arg (void) {
  int i, j;
  EXPR_PARSER *p;

  for (i = expr_parser_ptr; i <= MAXARGS; ++i) {
    if (expr_parsers[i] -> entry_eval_status | EVAL_STATUS_ASSIGN_ARG) {
      p = expr_parsers[i];
      for (j = p -> msg_frame_start; j >= p -> msg_frame_top; --j) {
	if (M_TOK(e_messages[j]) == METHODMSGLABEL) {
	  if (e_messages[j] -> name[0] == '+' ||
	      e_messages[j] -> name[0] == '-' ||
	      e_messages[j] -> name[0] == '/' ||
	      e_messages[j] -> name[0] == '*') {
	    /* I.e., the expression calculates a value. */
	    return false;
	  }
	}
      }
      return true;
    }
  }
  return false;
}

static inline bool has_active_i (OBJECT *obj) {
  VARENTRY *v;
  if (HAS_VARTAGS(obj)) {
    if (!IS_EMPTY_VARTAG(obj -> __o_vartags)) {
      v = obj -> __o_vartags -> tag;
      if (v -> i != I_UNDEF ||
	  v -> i_post != I_UNDEF ||
	  v -> i_temp != I_UNDEF) {
	return true;
      }
    }
  }
  return false;
}

/*
 *  If we have an unparenthesized expression like
 *
 *   *<cvar>++
 *
 *  then return true, so we can evaluate the *<cvar>
 *  first.
 */
static int has_rassoc_pfx (int idx) {
  int i;
  bool have_label = false;
  for (i = idx + 1; i <= expr_parsers[expr_parser_ptr] -> msg_frame_start;
       ++i) {
    if (M_ISSPACE(e_messages[i]))
      continue;
    if (have_label) {
      if ((M_TOK(e_messages[i]) == MULT) &&
	  (e_messages[i] -> attrs & RT_TOK_IS_PREFIX_OPERATOR)) {
	return i;
      } else {
	return ERROR;
      }
    }
    if (M_TOK(e_messages[i]) == OPENPAREN ||
	M_TOK(e_messages[i]) == CLOSEPAREN)
      return ERROR;
    if (M_TOK(e_messages[i]) == LABEL) {
      have_label = true;
    } else {
      return ERROR;
    }
  }
  return ERROR;
}

static inline int max_params_set (METHOD **mset, int n_mset) {
  int i, max = 0;
  for (i = 0; i < n_mset; ++i) {
    if (mset[i] -> n_params > max)
      max = mset[i] -> n_params;
  }
  return max;
}

static int expr_typecast_is_pointer (EXPR_PARSER *p, int start) {
  int i,
    n_derefs,
    end_paren;
  
  if ((end_paren = __ctalkMatchParen (p -> m_s, start,
				      p -> msg_frame_top)) == ERROR)
    return 0;

  for (i = start, n_derefs = 0; i >= end_paren; i--) 
    if (p -> m_s[i] -> tokentype == ASTERISK)
      ++n_derefs;
  return n_derefs;
}

static bool terminal_member_var_ref (void) {
  int i, j;
  for (i = expr_parser_ptr; i <= MAXARGS; ++i) {
    if (expr_parsers[i]) {
      if (expr_parsers[i] -> expr_str[0]) {
	if (strstr (expr_parsers[i] -> expr_str, "instancevars") ||
	    strstr (expr_parsers[i] -> expr_str, "classvars") ||
	    strstr (expr_parsers[i] -> expr_str, "value")) {
	  eval_status |= (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);

	  return true;
	}
      }
    }
  }
  return false;
}

static inline int next_msg (MESSAGE_STACK messages, int this_msg) { 

  int i = this_msg - 1;

  while ( i >= C_EXPR_PARSER -> msg_frame_top ) {
    if (!M_ISSPACE(messages[i]))
       return i;
    --i;
  }
  return ERROR;
}

static inline bool method_label_is_arg (int i) {
  int next_tok_idx;
  for (i = i - 1; e_messages[i]; --i) {
    if (M_ISSPACE(e_messages[i])) {
      continue;
    } else if (M_TOK(e_messages[i]) == ARGSEPARATOR) {
      return true;
    }
  }
  return false;
}

static inline bool mr_arglist_check (MESSAGE_STACK messages,
				     int prev_method_tok_idx,
				     int tok_idx,
				     METHOD *prev_method) {
  /* check if the method expects any args, and if our token
     immediately follows the method. */
  if ((prev_method -> n_params > 0) &&
      (next_msg (messages, prev_method_tok_idx) == tok_idx)) {
    return false;
  } else {
    return true;
  }
}

/* This returns true if we have an as yet unresolved label in
   a sequence of labels where a preceding method exists; e.g.,

   myRcvr at index contains searchString;
          ^              ^
          | receiver of  |

   where the receiver of "contains" can be the result of "at"

   Only valid for an expression composed entirely of LABELS
   and METHODMSGLABELS so far. 

   This is a little sketchy right now because there aren't
   many examples of expressions where this is useful.
*/

static bool method_result_can_be_receiver (MESSAGE_STACK messages,
					   int tok_idx,
					   int *prev_mthd_idx) {
  int i;
  MESSAGE *m;
  METHOD *prev_method, *tok_method;
  OBJECT *prev_method_rcvr, *prev_method_returnclass;

  /* an argblk method is not a successive call - it's actually
     the argument to the preceding map-type method */
  if (strstr (M_NAME(messages[tok_idx]), ARGBLK_LABEL))
    return false;
  
  for (i = tok_idx + 1; i <= C_EXPR_PARSER -> msg_frame_start; ++i) {
    m = messages[i];
    if (M_ISSPACE(m) || M_TOK(m) == LABEL) { 
      continue;
    } else if (M_TOK(m) == METHODMSGLABEL) {
      if (IS_OBJECT(m -> receiver_obj)) {
	prev_method_rcvr = m -> receiver_obj;
	if (((prev_method = __ctalkFindInstanceMethodByName
	   (&prev_method_rcvr, M_NAME(m), FALSE, ANY_ARGS)) != NULL) ||
	    ((prev_method = __ctalkFindClassMethodByName
	      (&prev_method_rcvr, M_NAME(m), FALSE, ANY_ARGS)) != NULL)) {
	  if (str_eq (prev_method -> returnclass, "Any")) {
	    *prev_mthd_idx = i;
	    return mr_arglist_check (messages, i, tok_idx, prev_method);
	  } else {
	    prev_method_returnclass =
	      __ctalkGetClass (prev_method -> returnclass);
	    if (((tok_method = __ctalkFindInstanceMethodByName
		  (&prev_method_returnclass, M_NAME(messages[tok_idx]),
		   FALSE, ANY_ARGS)) != NULL) ||
		((tok_method = __ctalkFindClassMethodByName
		  (&prev_method_returnclass, M_NAME(messages[tok_idx]),
		   FALSE, ANY_ARGS)) != NULL)) {
	      *prev_mthd_idx = i;
	      return mr_arglist_check (messages, i, tok_idx, prev_method);
	    } else {
	      return false;
	    }
	  }
	}
      }
    } else {
      break;
    }
  }
  return false;
}

static void find_prefixed_CVAR_for_write (int op_idx, METHOD *method) {
  int i;
  if (((M_TOK(e_messages[op_idx]) == INCREMENT) ||
       (M_TOK(e_messages[op_idx]) == DECREMENT)) &&
      (e_messages[op_idx] -> attrs & RT_TOK_IS_PREFIX_OPERATOR)) {
    for (i = op_idx - 1;
	 i >= expr_parsers[expr_parser_ptr] -> msg_frame_top; --i) {
      if (e_messages[i] -> attrs &
	  RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) {
	write_val_CVAR (M_VALUE_OBJ(e_messages[i]), method);
      }
    }
  }
}

bool have_unevaled (MESSAGE_STACK messages, int this_method_token) {
  int i;
#if 0
#ifdef __GNUC__
  asm(""); /* prevents the function from being optimized out. */
#endif
#endif
  for (i = C_EXPR_PARSER -> msg_frame_start; i > this_method_token; --i) {
    /* Often the space before a method will still be unevaled. */
    if (M_ISSPACE(messages[i]))
      continue;
    if (messages[i] -> evaled == 0) {
      return true;
    }
  }
  return false;
}

static inline void clear_expr_value_obj (MESSAGE_STACK messages,
					 int start_idx, int end_idx,
					 OBJECT *obj) {
  int i;
  
  for (i = start_idx; i >= end_idx; i--) {
    if (messages[i] -> value_obj == obj)
      messages[i] -> value_obj = NULL;
  }
}

static int expr_match_paren (EXPR_PARSER *p, int this_message) { 

  int i, parens = 0;
  MESSAGE *m;

  if (M_TOK(p -> m_s[this_message]) != OPENPAREN)
    return ERROR;

  for (i = this_message; i >= p -> msg_frame_top; i--) {
    m = p -> m_s[i];
    if (M_ISSPACE(m))
      continue;
    if (m && IS_MESSAGE (m)) {
      switch (m -> tokentype)
	{
	case OPENPAREN:
	  ++parens;
	  break;
	case CLOSEPAREN:
	  --parens;
	  break;
	default:
	  break;
	}

      if (!parens)
	return i;
    }
  }
  _warning ("Mismatched parentheses.\n");
  return -1;
}

static inline void cleanup_created_param_arg (MESSAGE_STACK messages,
					      int idx,
					      OBJECT *arg_object) {
  if (IS_OBJECT(messages[idx] -> obj)) {
    if (arg_object -> scope & CREATED_PARAM) {
      if (messages[idx] -> attrs & RT_TOK_OBJ_IS_CREATED_PARAM) {
	if (!(arg_object -> scope & VAR_REF_OBJECT)) {
	  __ctalkDeleteObject (arg_object);
	}
      }
    }
  }
}

static inline int prev_msg (MESSAGE_STACK messages, int this_msg) {
  int i = this_msg + 1;

  while ( i <= C_EXPR_PARSER -> msg_frame_start) {
    if (!M_ISSPACE(messages[i]))
      return i;
    ++i;
  }
  return ERROR;
}

static bool method_is_instancevar_deref (EXPR_PARSER *p, int idx) {
  int next_tok_idx;
  if (str_eq (M_NAME(p -> m_s[idx]), "->")) {
    /* use a string comparison because the token has been changed
       to METHODMSGLABEL */
    next_tok_idx = next_msg (p -> m_s, idx);
    if (str_eq (M_NAME(p-> m_s[next_tok_idx]), "instancevars")) {
      return true;
    }
  }
  return false;
}

static bool __prev_tok_is_instancevar_ref (EXPR_PARSER *p, int idx) {
  int prev_tok_idx;
  if ((prev_tok_idx = prev_msg (p -> m_s, idx)) != ERROR) {
    if (p -> m_s[prev_tok_idx] -> attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL)
      return true;
  }
  return false;
}

static OBJECT *expr_result_object (void) {
  OBJECT *o;
  EXPR_PARSER *p = C_EXPR_PARSER;
  MESSAGE *m_term = e_messages[p -> msg_frame_top];
  /*
   *  Terminal instance variable message that is a 
   *  complete object - eg not "value," and value object 
   *  is an instance variable of the receiver - make an 
   *  ARG_VAR copy.
   */
  if (((m_term ->attrs & RT_OBJ_IS_INSTANCE_VAR) ||
       (m_term -> attrs & RT_OBJ_IS_CLASS_VAR)) &&
      (m_term -> value_obj ? 
       !(m_term -> value_obj -> attrs & OBJECT_IS_VALUE_VAR) :
       (m_term -> obj ?
	!(m_term -> obj -> attrs & OBJECT_IS_VALUE_VAR) : 0))) {
    /*
     *  NOTE: If the terminal object is a fixed-up var,
     *  then eval_expr should also set the expression value, in
     *  addition to handling the case here.
     */
    if ((o = __ctalkGetInstanceVariableComplex 
	 (M_VALUE_OBJ(e_messages[p -> msg_frame_start]), 
	  M_NAME(e_messages[p -> msg_frame_top]),
	  FALSE))
 	!= NULL) {
      if (o == M_VALUE_OBJ(e_messages[p -> msg_frame_start])) {
	eval_status |= (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
	return M_VALUE_OBJ(e_messages[p -> msg_frame_top]);
      } else {
	if ((e_messages[p -> msg_frame_top] -> obj || 
	     e_messages[p -> msg_frame_top] -> value_obj) &&
	    __ctalkIsInstanceVariableOf 
	    (M_VALUE_OBJ(e_messages[p -> msg_frame_start]) -> __o_class,
	     M_NAME(e_messages[p -> msg_frame_top]))) {
	  eval_status |= (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
	  return M_VALUE_OBJ(e_messages[p -> msg_frame_top]);
	} else {
	  int m_pm_idx;
	  if ((m_pm_idx = prev_msg (e_messages, p -> msg_frame_top)) != ERROR) {
	    if ((o = __ctalkGetInstanceVariable 
		 (M_VALUE_OBJ(e_messages[m_pm_idx]), 
		  M_NAME(e_messages[p -> msg_frame_top]),
		  FALSE)) != NULL) {
	      eval_status |= 
		(EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
	      return M_VALUE_OBJ(e_messages[p -> msg_frame_top]);
	    }
	  }
	}
      }
    } else if ((eval_status & EVAL_STATUS_TERMINAL_TOK) &&
	       (eval_status & EVAL_STATUS_CLASS_VAR)) {
      return M_VALUE_OBJ(e_messages[p -> msg_frame_top]);
    }
  } else {
    /*
     *  Typecast alone not part of an expression.
     */
    if (e_messages[p -> msg_frame_start] -> attrs & RT_TOK_IS_TYPECAST_EXPR) {
      int i_1;
      for (i_1 = p -> msg_frame_start; i_1 >= p -> msg_frame_top; i_1--) {
	if (!(e_messages[i_1]->attrs & RT_TOK_IS_TYPECAST_EXPR))
	  return M_VALUE_OBJ(e_messages[i_1]);
      }

    } else {
      if (e_messages[p -> msg_frame_top] -> attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL) {
	eval_status |= EVAL_STATUS_TERMINAL_TOK;
	return M_VALUE_OBJ (e_messages[p -> msg_frame_top]);
      } else {
	/*
	 *  Set at 
	 *  if ((e_result = 
	 *    __ctalkGetInstanceVariable (m_prev_value_obj, 
	 *                                M_NAME(m),
	 *                                FALSE)) != NULL)
	 *  or
	 *  if ((e_result = 
	 *   __ctalkGetClassVariable (m_prev_value_obj, 
	 *                            M_NAME(m),
	 *                            FALSE)) != NULL)
	 *
	 *  in the second pass of eval_expr, case METHODMSGLABEL,
	 *  above.
	 */
	if ((e_messages[p -> msg_frame_top]->attrs & RT_OBJ_IS_INSTANCE_VAR) ||
	    (e_messages[p -> msg_frame_top]->attrs & RT_OBJ_IS_CLASS_VAR)) {
	  if (e_messages[p -> msg_frame_top] -> receiver_obj == 
	      e_messages[p -> msg_frame_start] -> value_obj) {
	    eval_status |= 
	      (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
	  } else {
	    /*
	     *  A terminal instance variable message following an 
	     *  aggregate member label, after re-evaluation of the e_messages 
	     *  following the deref operator.
	     */
	    int __idx;
	    for (__idx = p -> msg_frame_start; __idx >= p -> msg_frame_top; __idx--) {
	      if (e_messages[__idx]->attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL) {
		if ((e_messages[p -> msg_frame_top]->attrs & RT_OBJ_IS_INSTANCE_VAR) ||
		    (e_messages[p -> msg_frame_top]->attrs & RT_OBJ_IS_CLASS_VAR)) {
		  eval_status |= 
		    (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return e_messages[p -> msg_frame_start]->value_obj;
}

/* The args to -> normally translate to Objects,
   Strings, or Integers. */
static OBJECT *class_for_deref_arg (MESSAGE_STACK messages,
				    int deref_method_idx,
				    int deref_arg_idx) {

  OBJECT *deref_rcvr_val, *mbr_class_t;
  int rcvr_tok = prev_msg (e_messages, deref_method_idx);
  MESSAGE *m_rcvr_tok = messages[rcvr_tok];
  if (M_TOK(m_rcvr_tok) == METHODMSGLABEL) {
    deref_rcvr_val = m_rcvr_tok -> receiver_obj;
  } else {
    deref_rcvr_val = m_rcvr_tok -> obj;
  }
  if (IS_OBJECT(deref_rcvr_val)) {
    deref_rcvr_val =
      (IS_OBJECT(deref_rcvr_val -> instancevars) ?
       deref_rcvr_val -> instancevars : deref_rcvr_val);
    mbr_class_t = OBJECT_mbr_class (deref_rcvr_val -> __o_class,
				    M_NAME(messages[deref_arg_idx]));
  } else {
    mbr_class_t = rt_defclasses -> p_object_class;
  }
  return mbr_class_t;
}

static int rcvr_scan_back_b (int idx, EXPR_PARSER *p) {
  int rcvr_ptr, i_1;
  rcvr_ptr = prev_msg (e_messages, idx);
  if (e_messages[rcvr_ptr] -> attrs & RT_TOK_IS_SELF_KEYWORD)
    return rcvr_ptr;
  if (e_messages[rcvr_ptr] -> attrs & RT_OBJ_IS_INSTANCE_VAR)
    return rcvr_ptr;
  if ((M_TOK(e_messages[rcvr_ptr]) == LABEL) &&
      (e_messages[rcvr_ptr] -> attrs == 0) &&
      (IS_OBJECT(M_VALUE_OBJ(e_messages[rcvr_ptr]))))
    return rcvr_ptr;

  while (rcvr_ptr < p -> msg_frame_start) {
    /*
     *  Otherwise, scan back until we find the leftmost
     *  token with the receiver object.
     */
    if (M_TOK(e_messages[rcvr_ptr+1]) == ARGSEPARATOR) {
      while (M_ISSPACE(e_messages[rcvr_ptr]))
	rcvr_ptr--;
      return rcvr_ptr;
    }
    if (e_messages[rcvr_ptr] -> attrs & 
	RT_TOK_IS_POSTFIX_MODIFIED) {
      while (e_messages[rcvr_ptr] -> attrs & 
	     RT_TOK_IS_POSTFIX_MODIFIED)
	rcvr_ptr++;
      return rcvr_ptr;
    }
    if (e_messages[rcvr_ptr] -> attrs & 
	RT_TOK_HAS_LVAL_PTR_CX) {
      if (p -> msg_frame_start ==
	  prev_msg (e_messages, rcvr_ptr))
	return rcvr_ptr;
    }
    /*  The same as above, but check for a class object token
	preceding a class variable, which should then have the
	RT_TOK_HAS_LVAL_PTR_CX attribute. Also set the class
	variable token's attribute to RT_TOK_HAS_LVAL_PTR_CX.
    */
    if (e_messages[rcvr_ptr] -> attrs & RT_OBJ_IS_CLASS_VAR) {
      if (e_messages[rcvr_ptr] -> receiver_msg) {
	if (e_messages[rcvr_ptr] -> receiver_msg -> attrs &
	    RT_TOK_HAS_LVAL_PTR_CX) {
	  for (i_1 = idx; i_1 <= p -> msg_frame_start; ++i_1) {
	    if (e_messages[i_1] == 
		e_messages[rcvr_ptr] -> receiver_msg)
	      break;
	  }
	  if (p -> msg_frame_start == prev_msg (e_messages, i_1))
	    return rcvr_ptr;
	}
      }
    }
    /* this handles finding the receiver of following
       methods in a compound method expression. */
    if (e_messages[rcvr_ptr] -> attrs &
	RT_TOK_VALUE_OBJ_IS_RECEIVER) {
      return rcvr_ptr;
    }
    if (M_VALUE_OBJ (e_messages[rcvr_ptr + 1])
	== M_VALUE_OBJ (e_messages[rcvr_ptr]))
      rcvr_ptr++;
    else
      break;
  }
  return rcvr_ptr;
}

/* 
 * This is only for successive method calls right now.
 * We need to determine whether to move the local_var
 * frame.  TODO - make sure the local vars get placed on the
 * next *frame), not just a simple increment as in 
 * register_successive method_call ().  
 *
 * There aren't very many examples of this construction, 
 * so it's not possible to test anything more involved.

 * This object can actually be NULL - it's not used for
 * further evals. But we can't just delete it because
 * it's also the messages's value object.
 *
 * It's only set after one of the __ctalkCallMethodFn ()
 * calls right now.
 */

static OBJECT *__last_eval_result = NULL;

OBJECT *last_eval_result (void) {
  return __last_eval_result;
}

void reset_last_eval_result (void) {
  __last_eval_result = NULL;
}

static inline int method_is_postfix_of_cvar (MESSAGE_STACK messages, int method_idx) {
  int __m_p_idx;

  if (__cvar_postfix_method_retry > 0)
    return 0;

  if ((__m_p_idx = prev_msg (messages, method_idx)) != ERROR) {
    if (messages[__m_p_idx] -> attrs & RT_CVAR_AGGREGATE_TOK) {
      return __m_p_idx;
    }
  }
  return 0;
}

static inline int eval_op_precedence (int precedence, 
				      MESSAGE_STACK messages, int op_ptr) {
  int retval = FALSE;

  switch (precedence)
    {
    case 0:
      switch (M_TOK(messages[op_ptr]))
	{
	case OPENPAREN:
	case CLOSEPAREN:
	case ARRAYOPEN:
	case ARRAYCLOSE:
	case DEREF:
	case PERIOD:
	  retval = TRUE;
	  break;
	case INCREMENT:
	case DECREMENT:
	  if (!(messages[op_ptr] -> attrs & RT_TOK_IS_PREFIX_OPERATOR))
	    retval = TRUE;
	default:
	  break;
	}
      break;
    case 1:
      switch (M_TOK(messages[op_ptr]))
	{
	case SIZEOF:
	case EXCLAM:
	case BIT_COMP:
	  retval = TRUE;
	  break;
	case PLUS:      
	case MINUS:     
	case ASTERISK:  
	case AMPERSAND:
	case INCREMENT:
	case DECREMENT:
	  if (messages[op_ptr] -> attrs & RT_TOK_IS_PREFIX_OPERATOR)
	    retval = TRUE;
	default: 
	  break;
	}
      break;
    case 2:
      switch (messages[op_ptr] -> tokentype)
	{
	case DIVIDE:
	case MODULUS:
	  retval = TRUE;
	  break;
	case ASTERISK:
	  if (!(messages[op_ptr] -> attrs & RT_TOK_IS_PREFIX_OPERATOR))
	    retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 3:
      switch (messages[op_ptr] -> tokentype)
	{
	case PLUS:
	case MINUS:
	  if (!(messages[op_ptr] -> attrs & RT_TOK_IS_PREFIX_OPERATOR))
	    retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 4:
      switch (messages[op_ptr] -> tokentype)
	{
	case ASL:
	case ASR:
	  retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 5:
      switch (messages[op_ptr] -> tokentype)
	{
	case LT:
	case LE:
	case GT:
	case GE:
	  retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 6:
      switch (messages[op_ptr] -> tokentype)
	{
	case BOOLEAN_EQ:
	case INEQUALITY:
	case MATCH:
	case NOMATCH:
	  retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 7:
      if (messages[op_ptr] -> tokentype == BIT_AND)
	retval = TRUE;
      break;
    case 8:
      switch (messages[op_ptr] -> tokentype)
	{
	case BIT_OR:
	case BIT_XOR:
	  retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 9:
      if (messages[op_ptr] -> tokentype == BOOLEAN_AND)
	retval = TRUE;
      break;
    case 10:
      if (messages[op_ptr] -> tokentype == BOOLEAN_OR)
	retval = TRUE;
      break;
    case 11:
      switch (messages[op_ptr] -> tokentype)
	{
	case CONDITIONAL:
	case COLON:
	  retval = TRUE;
	  break;
	default:
	  break;
	}
    case 12:
      switch (messages[op_ptr] -> tokentype)
	{
	case EQ:
	case ASR_ASSIGN:
	case ASL_ASSIGN:
	case PLUS_ASSIGN:
	case MINUS_ASSIGN:
	case MULT_ASSIGN:
	case DIV_ASSIGN:
	case BIT_AND_ASSIGN:
	case BIT_OR_ASSIGN:
	case BIT_XOR_ASSIGN:
	  retval = TRUE;
	  break;
	default:
	  break;
	}
      break;
    case 13:
      if (messages[op_ptr] -> tokentype == ARGSEPARATOR)
	retval = TRUE;
      break;
    default:
      break;
    }

  return retval;
}

EXPR_PARSER *__pop_expr_parser (void) {
  EXPR_PARSER *p;
  if (expr_parser_ptr > MAXARGS)
    _error ("__pop_expr_parser: stack underflow.");
  p = expr_parsers[expr_parser_ptr] ;
  expr_parsers[expr_parser_ptr++] = NULL;
  return p;
}

EXPR_PARSER *__current_expr_parser (void) {
  return expr_parsers[expr_parser_ptr];
}

EXPR_PARSER *__expr_parser_at (int i) {
  return expr_parsers[i];
}

EXPR_PARSER *__new_expr_parser (void) {

  EXPR_PARSER *p;

  if ((p = (EXPR_PARSER *)__xalloc (sizeof (EXPR_PARSER))) == NULL)
    _error ("__new_expr_parser: %s.", strerror (errno));

  p -> lvl = expr_parser_lvl;
  p -> call_stack_level = __current_call_stack_idx ();

  return p;
}

void __delete_expr_parser (EXPR_PARSER *p) {
  __xfree (MEMADDR(p));
}

/*
 *  Expression message stack.
 */

int _get_e_message_ptr (void) {
  return e_message_ptr;
}

MESSAGE_STACK _get_e_messages (void) {
  return e_messages;
}

bool is_constant_rcvr = False;
bool is_constant_arg = False;

static bool is_complete_var_expr (MESSAGE_STACK messages,
				  int idx,
				  int end_paren_idx,
				  OBJECT *subexpr_result) {
  /* Called after eval_subexpr.  If this is the complete
     expression, then we're done:
     
     (<subexpr>) <instancevar>
     (<subexpr>) <classvar>
     

     idx points to the opening paren of the expression.
  */
  int prev_idx, i;
  EXPR_PARSER *p = C_EXPR_PARSER;

  if (idx == p -> msg_frame_top) {
    if (messages[idx] -> attrs & RT_OBJ_IS_INSTANCE_VAR ||
	messages[idx] -> attrs & RT_OBJ_IS_CLASS_VAR) {
      if ((prev_idx = prev_msg (messages, p -> msg_frame_top)) 
	  == end_paren_idx) {
	for (i = p -> msg_frame_start; i > p -> msg_frame_top; --i) {
	  if (IS_OBJECT(messages[i] -> obj)) {
	    messages[i] -> value_obj = 
	      messages[p -> msg_frame_top] -> value_obj;
	  } else {
	    messages[i] -> obj = messages[p -> msg_frame_top] -> value_obj;
	  }
	  /* also set the scope and refcnt of a new object if
	     necessary */
	  if (subexpr_result -> nrefs == 0) {
	    __ctalkSetObjectScope (subexpr_result, 
				   subexpr_result -> scope | 
				   SUBEXPR_CREATED_RESULT);
	    (void)__objRefCntInc(OBJREF(subexpr_result));
	  }
	  return true;
	}
      }
    }
  }
  return false;
}

/*
 *  This can probably go away if we ever make certain that
 *  all class variables have value instance variables.
 */
static inline int __is_class_var_of_prev_msg (MESSAGE_STACK messages, int idx,
				       int prev_tok_idx) {
  return (messages[idx] -> obj &&
	  messages[prev_tok_idx]->obj &&
	  __ctalkIsClassVariableOf (messages[prev_tok_idx]->obj,
				    M_NAME(messages[idx])));
}

/*
 *  This also checks the precedence in case a prefix operator
 *  has a lower precedence than the rest of the expression.
 *  Note that this fn is normally used to find a _leading_
 *  prefix operator for an entire expression, so it may
 *  only be needed in limited cases.
 */
static int __is_prefix_method_rcvr (MESSAGE_STACK messages, int idx,
				    int current_precedence) {
  int prev_tok_idx;
  int p;
  
  if ((prev_tok_idx = prev_msg (messages, idx)) == ERROR)
    return ERROR;

  if (messages[prev_tok_idx] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
    for (p = 0; p <= 13; ++p) { 
      if (eval_op_precedence (p, messages, prev_tok_idx)) {
	if (p <= current_precedence) {
	  return prev_tok_idx;
	}
      }
    }
  }

  return ERROR;
}

/*
 *  rt version of is_leading_prefix_op () in pattypes.c.
 */
static int __ctalkIsLeadingPrefixOp_a (MESSAGE_STACK messages, int prefix_idx) {
  int j;
  for (j = prefix_idx - 1; j >= C_EXPR_PARSER -> msg_frame_top; j--) {
    if (M_ISSPACE(messages[j]))
	continue;
    if (M_TOK(messages[j]) == OPENPAREN) {
      continue;
    } else {
      return j;
    }
  }
  return -1;
}

static int __expr_rcvr_backtrack (MESSAGE_STACK messages, int idx,
				  int prev_msg_idx, int current_precedence) {
  int expr_start_idx, expr_start_idx_1, __n_parens, __n_subs;
  int var_prev_idx;
  if (messages[idx] -> receiver_msg) {
    expr_start_idx = idx;
  backtrack_start: while (expr_start_idx < C_EXPR_PARSER -> msg_frame_start) {
      /* 
       * Handle the different overloads of "super." 
       */
      if (M_TOK(messages[expr_start_idx]) == LABEL) {
	if (!strcmp (M_NAME(messages[expr_start_idx]), "super")) {
	  int e_s_1;
	  e_s_1 = prev_msg (messages, expr_start_idx);
	  /* eval_expr () does the checking  for nonterminal nodes
	     before assigning the PREFIX attribute, so we don't
	     have to here.  */
	  if (messages[e_s_1] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
	    return expr_start_idx;
	  } else if (IS_C_ASSIGNMENT_OP(M_TOK(messages[e_s_1]))) {
	    return expr_start_idx;
	  } else {
	    expr_start_idx = e_s_1;
	    continue;
	  }
	}
      }
      if ((expr_start_idx_1 = 
	   __is_prefix_method_rcvr (e_messages,
				    expr_start_idx,
				    current_precedence)) != ERROR) {
	expr_start_idx = expr_start_idx_1;
	continue;
      }

      var_prev_idx = prev_msg (messages, expr_start_idx);
      if (__is_class_var_of_prev_msg (messages, expr_start_idx,
				      var_prev_idx)){
	expr_start_idx = var_prev_idx;
	continue;
      }
      if (messages[expr_start_idx] -> attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL){
	/*
	 *  Skip to the deref token's receiver.
	 */
	++expr_start_idx;
	for (; expr_start_idx <= C_EXPR_PARSER -> msg_frame_start; expr_start_idx++) {
	  if (M_TOK(messages[expr_start_idx]) == LABEL)
	    break;
	}

	continue;
      }
      if (M_TOK(messages[expr_start_idx]) == CLOSEPAREN) {
	__n_parens = 0;
	for (expr_start_idx_1 = expr_start_idx; expr_start_idx <= C_EXPR_PARSER -> msg_frame_start;
	     expr_start_idx_1++) {
	  if (M_TOK(e_messages[expr_start_idx_1]) == CLOSEPAREN)
	    ++__n_parens;
	  if (M_TOK(e_messages[expr_start_idx_1]) == OPENPAREN)
	    --__n_parens;
	  if (__n_parens == 0)
	    break;
	}
	expr_start_idx = expr_start_idx_1;
	continue;
      }

      if ((M_TOK(messages[expr_start_idx]) == ARRAYCLOSE) &&
	  (messages[expr_start_idx] -> attrs & RT_CVAR_AGGREGATE_TOK)) {
	__n_subs = 0;
	for (expr_start_idx_1 = expr_start_idx; 
	     (expr_start_idx <= C_EXPR_PARSER -> msg_frame_start) && 
	       (messages[expr_start_idx_1] -> attrs & RT_CVAR_AGGREGATE_TOK);
	     expr_start_idx_1++) {
	  if (M_TOK(e_messages[expr_start_idx_1]) == ARRAYCLOSE)
	    ++__n_subs;
	  if (M_TOK(e_messages[expr_start_idx_1]) == ARRAYOPEN)
	    --__n_subs;
	  if (__n_subs == 0)
	    break;
	}
	expr_start_idx = expr_start_idx_1;
	continue;
      }

      if (messages[expr_start_idx] -> receiver_msg == NULL)
	break;
      for (expr_start_idx_1 = expr_start_idx; 
	   expr_start_idx_1 <= C_EXPR_PARSER -> msg_frame_start;
	   expr_start_idx_1++)
	if (messages[expr_start_idx_1] == 
	    messages[expr_start_idx] -> receiver_msg)
	  break;
      expr_start_idx = expr_start_idx_1;
      if (M_TOK(messages[expr_start_idx]) == METHODMSGLABEL) {
	if ((expr_start_idx_1 = prev_msg (messages, expr_start_idx)) != ERROR) {
	  if (M_TOK(messages[expr_start_idx_1]) == CLOSEPAREN) {
	    expr_start_idx = __ctalkMatchParenRev (messages,
						   expr_start_idx_1,
						   C_EXPR_PARSER -> msg_frame_start);
	  }
	}
      }
    }
  } else {
    expr_start_idx = prev_msg_idx;
  }
  if (M_TOK(messages[expr_start_idx]) == CLOSEPAREN) {
    __n_parens = 0;
    for (expr_start_idx_1 = expr_start_idx; expr_start_idx <= C_EXPR_PARSER -> msg_frame_start;
	 expr_start_idx_1++) {
      if (M_TOK(e_messages[expr_start_idx_1]) == CLOSEPAREN)
	++__n_parens;
      if (M_TOK(e_messages[expr_start_idx_1]) == OPENPAREN)
	--__n_parens;
      if (__n_parens == 0)
	break;
    }
    expr_start_idx = expr_start_idx_1;
  }
  if ((M_TOK(messages[expr_start_idx]) == ARRAYOPEN) &&
      (messages[expr_start_idx] -> attrs & RT_CVAR_AGGREGATE_TOK)) {
    while ((expr_start_idx < C_EXPR_PARSER -> msg_frame_start) && 
	   (messages[expr_start_idx] -> attrs & RT_CVAR_AGGREGATE_TOK))
      ++expr_start_idx;
    /* Loop ends at the previous token. */
    --expr_start_idx;
  }
  if ((expr_start_idx < C_EXPR_PARSER -> msg_frame_start) &&
      ((expr_start_idx_1 = prev_msg (messages, expr_start_idx)) != ERROR)) {
    /*
     *  At this point any overloaded method or operator should
     *  have a methodmsglabel token.
     */
    if (M_TOK(messages[expr_start_idx_1]) == METHODMSGLABEL) {
      expr_start_idx = expr_start_idx_1;
      goto backtrack_start;
    } 
    if ((messages[expr_start_idx] -> attrs & RT_VALUE_OBJ_IS_INSTANCE_VAR) &&
	(M_TOK(messages[expr_start_idx_1]) == CLOSEPAREN)) {
      /*
       *  An expression like:
       *    (<rcvr_expr>) <instancevar> <method>...
       *                  ^--- expr_start_idx
       *                ^----- expr_start_idx_1
       */
      int __e_1;
      if ((__e_1 = __ctalkMatchParenRev (messages, expr_start_idx_1,
					 C_EXPR_PARSER -> msg_frame_start)) != ERROR) {
	expr_start_idx = __e_1;
	goto backtrack_start;
      }
    }
  }
  /*
   *  Again, if we end up in an aggregate expression, check again for the start 
   *  of the expression.
   */
  if (messages[expr_start_idx] -> attrs & RT_CVAR_AGGREGATE_TOK) {
    while ((expr_start_idx < C_EXPR_PARSER -> msg_frame_start) && 
	   (messages[expr_start_idx] -> attrs & RT_CVAR_AGGREGATE_TOK))
      ++expr_start_idx;
  }

  return expr_start_idx;
}

static int fixup_forward_instance_var_of_method_return
                                (EXPR_PARSER *p, int idx) {
  int next_tok_idx;
  MESSAGE *m, *m_next_tok;
  OBJECT *var;

  if ((next_tok_idx = next_msg (p -> m_s, idx)) != ERROR) {
    m = p -> m_s[idx];
    m_next_tok = p -> m_s[next_tok_idx];
    if (M_VALUE_OBJ(m)) {
      if ((IS_OBJECT(m -> obj) && IS_OBJECT(m -> value_obj)) &&
	  (m -> attrs & RT_OBJ_IS_INSTANCE_VAR)) {
	/* If the following message is an instance var,
	   do a little fixup in case eval_expr has already set the 
	   entire expression's value to the original receiver. */
	if (str_eq (m -> obj -> __o_name, M_NAME(m)) &&
	    (m -> obj -> __o_p_obj == m -> value_obj)) {
	  m -> value_obj = NULL;
	}
      }
      if ((var = __ctalkGetInstanceVariable (M_VALUE_OBJ (m),
					     M_NAME (m_next_tok),
					     FALSE)) != NULL) {
	if (!m_next_tok -> obj) {
	  m_next_tok -> obj = var;
	  m_next_tok -> receiver_obj = M_VALUE_OBJ(m);
	  m_next_tok -> receiver_msg = m;
	  m_next_tok -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
	} else {
	  m_next_tok -> value_obj = var;
	  m_next_tok -> receiver_obj = M_VALUE_OBJ(m);
	  m_next_tok -> receiver_msg = m;
	  m_next_tok -> attrs |= RT_VALUE_OBJ_IS_INSTANCE_VAR;
	}
      } else if ((var = __ctalkGetClassVariable (M_VALUE_OBJ (m),
						 M_NAME (m_next_tok),
						 FALSE)) != NULL) {
	if (!m_next_tok -> obj) {
	  m_next_tok -> obj = var;
	  m_next_tok -> receiver_obj = M_VALUE_OBJ(m);
	  m_next_tok -> receiver_msg = m;
	  m_next_tok -> attrs |= RT_OBJ_IS_CLASS_VAR;
	} else {
	  m_next_tok -> value_obj = var;
	  m_next_tok -> receiver_obj = M_VALUE_OBJ(m);
	  m_next_tok -> receiver_msg = m;
	  m_next_tok -> attrs |= RT_VALUE_OBJ_IS_CLASS_VAR;
	}
      }
    }
  }
  return SUCCESS;
}

static void fixup_forward_receiver_obj (EXPR_PARSER *p, int idx,
				OBJECT *result_obj) {
  int next_tok_idx,
    all_objects_deleted;
  OBJECT *var;
  if ((next_tok_idx = next_msg (p -> m_s, idx)) != ERROR) {
    if (p -> m_s[idx] -> obj && 
	(p -> m_s[idx] -> obj -> scope == CREATED_PARAM) &&
	(p -> m_s[next_tok_idx] -> receiver_obj == p -> m_s[idx] -> obj)) {
      p -> m_s[next_tok_idx] -> receiver_obj = result_obj;
    }
    if ((M_TOK(p -> m_s[idx]) == METHODMSGLABEL) &&
	(p -> m_s[idx] -> obj == NULL)) {
      if (IS_OBJECT(result_obj)) {
	p -> m_s[next_tok_idx] -> receiver_obj = result_obj;
      }
      p -> m_s[next_tok_idx] -> receiver_msg = p -> m_s[idx];
    }
    if ((p -> m_s[next_tok_idx] -> obj == NULL) &&
	(p -> m_s[next_tok_idx] -> value_obj == NULL) &&
	(p -> m_s[next_tok_idx] -> receiver_obj == result_obj)) {
      OBJECT *var = NULL;
      if ((var = 
	   __ctalkGetInstanceVariable 
	   (p -> m_s[next_tok_idx] -> receiver_obj, 
	    M_NAME(p -> m_s[next_tok_idx]), FALSE)) != NULL) {
	p -> m_s[next_tok_idx] -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
	p -> m_s[next_tok_idx] -> obj = var;
      } else {
	if ((var = 
	     __ctalkGetClassVariable 
	     (p -> m_s[next_tok_idx] -> receiver_obj, 
	      M_NAME(p -> m_s[next_tok_idx]), FALSE)) != NULL) {
	  p -> m_s[next_tok_idx] -> attrs |= RT_OBJ_IS_CLASS_VAR;
	  p -> m_s[next_tok_idx] -> obj = var;
	}
      }
      /*
       *  In case there is a method with the same name as
       *  the instance or class variable.... fix up.
       */
      if (var) {
	if (M_TOK(p -> m_s[next_tok_idx]) == METHODMSGLABEL) {
	  M_TOK(p -> m_s[next_tok_idx]) = LABEL;
	  if (p -> m_s[next_tok_idx] -> attrs & RT_DATA_IS_NR_ARGS_DECLARED)
	    p -> m_s[next_tok_idx] -> attrs &= ~RT_DATA_IS_NR_ARGS_DECLARED;
	}
      }
    }
    if ((p -> m_s[next_tok_idx] -> obj  && 
	 (p -> m_s[next_tok_idx] -> obj -> scope & CREATED_PARAM) &&
	 (p -> m_s[next_tok_idx] -> obj -> nrefs == 0)) &&
	(p -> m_s[next_tok_idx] -> receiver_obj == result_obj)) {
      delete_all_objs_expr_frame (p, next_tok_idx, &all_objects_deleted);
      p -> m_s[next_tok_idx] -> obj = NULL;
      var = NULL;
      if ((var = 
	   __ctalkGetInstanceVariable 
	   (p -> m_s[next_tok_idx] -> receiver_obj, 
	    M_NAME(p -> m_s[next_tok_idx]), FALSE)) != NULL) {
	p -> m_s[next_tok_idx] -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
	p -> m_s[next_tok_idx] -> obj = var;
	/*
	 *  Here, too.
	 */
	if (var) {
	  if (M_TOK(p -> m_s[next_tok_idx]) == METHODMSGLABEL) {
	    M_TOK(p -> m_s[next_tok_idx]) = LABEL;
	    if (p -> m_s[next_tok_idx] -> attrs & RT_DATA_IS_NR_ARGS_DECLARED)
	      p -> m_s[next_tok_idx] -> attrs &= ~RT_DATA_IS_NR_ARGS_DECLARED;
	  }
	}
      } else if (__ctalk_isMethod_2 (M_NAME(p -> m_s[next_tok_idx]),
				     p -> m_s, next_tok_idx,
				     p -> msg_frame_start)) {
	OBJECT *rcvr_p;
	METHOD *method_fixup;
	int i_2;
	/* probably should fix up the args too, if this method wasn't
	   resolved in the first pass, then the labels following it
	   weren't either... this isn't needed much yet, so for now,
	   just remove the created params... and/or add a clause in
	   __rt_method_args */
	M_TOK(p -> m_s[next_tok_idx]) = METHODMSGLABEL;
	/* Make sure we have a receiver object before proceeding. */
	if ((rcvr_p = p -> m_s[next_tok_idx] -> receiver_obj) != NULL) {
	  if ((method_fixup = __ctalkFindMethodByName
	       (&rcvr_p, M_NAME(p -> m_s[next_tok_idx]), FALSE, ANY_ARGS))
	      != NULL) {
	    if (method_fixup -> n_params > 0) {
	      for (i_2 = next_tok_idx - 1; i_2 >= p -> msg_frame_top;
		   --i_2) {
		if (IS_OBJECT(p -> m_s[i_2] -> obj)) {
		  if ((p -> m_s[i_2] -> obj -> scope == CREATED_PARAM) &&
		      (p -> m_s[i_2] -> obj -> nrefs == 0)) {
		    __ctalkDeleteObject (p -> m_s[i_2] -> obj);
		    p -> m_s[i_2] -> obj = NULL;
		  }
		}
	      }
	    }
	  }
	}
      }
    } else {
      if (p -> m_s[next_tok_idx] -> obj &&
	  (p -> m_s[next_tok_idx] -> obj -> scope & CREATED_PARAM) &&
	  /* An object can be created as an argument and then reffed
	     elsewhere... see the break3.c test program for an example. */
	  !(p -> m_s[next_tok_idx] -> obj -> scope & VAR_REF_OBJECT) &&
	  (p -> m_s[next_tok_idx] -> attrs & RT_OBJ_IS_INSTANCE_VAR) &&
	  (M_TOK(p -> m_s[idx]) == METHODMSGLABEL)) {
	__objRefCntZero (OBJREF(p -> m_s[next_tok_idx] -> obj));
	delete_all_objs_expr_frame (p, next_tok_idx, &all_objects_deleted);
	p -> m_s[next_tok_idx] -> obj = NULL;
	var = NULL;
	if ((var = 
	     __ctalkGetInstanceVariable 
	     (p -> m_s[next_tok_idx] -> receiver_obj, 
	      M_NAME(p -> m_s[next_tok_idx]), FALSE)) != NULL) {
	  p -> m_s[next_tok_idx] -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
	  p -> m_s[next_tok_idx] -> obj = var;
	} else {
	  if ((var = 
	       __ctalkGetClassVariable 
	       (p -> m_s[next_tok_idx] -> receiver_obj, 
		M_NAME(p -> m_s[next_tok_idx]), FALSE)) != NULL) {
	    p -> m_s[next_tok_idx] -> attrs |= RT_OBJ_IS_CLASS_VAR;
	    p -> m_s[next_tok_idx] -> obj = var;
	  }
	}
	/*
	 *  Fixup here, too.
	 */
	if (var) {
	  if (M_TOK(p -> m_s[next_tok_idx]) == METHODMSGLABEL) {
	    M_TOK(p -> m_s[next_tok_idx]) = LABEL;
	    if (p -> m_s[next_tok_idx] -> attrs & RT_DATA_IS_NR_ARGS_DECLARED)
	      p -> m_s[next_tok_idx] -> attrs &= ~RT_DATA_IS_NR_ARGS_DECLARED;
	  }
	}
      }
    }
  }
}
				
static int set_method_expr_value (MESSAGE_STACK messages, int idx, 
				  int prev_rcvr_msg_ptr,
				  int last_arg_ptr, 
				  int current_precedence, 
				  OBJECT *expr_result) {
  int j, i_2, j_1,
    expr_start_idx,
    t_last_arg_ptr;
  OBJECT *__s;


  expr_start_idx =
    __expr_rcvr_backtrack (e_messages, idx, prev_rcvr_msg_ptr, current_precedence);

   /* Scan for instance variable(s) following the last arg ptr.
      This probably needs more testing for a multiple instance 
      variable series. */
  t_last_arg_ptr = last_arg_ptr;
  for (j = last_arg_ptr - 1;
       j >= expr_parsers[expr_parser_ptr] -> msg_frame_top; --j) {
    if (M_ISSPACE(e_messages[j]))
      continue;

    if (IS_OBJECT(e_messages[j] -> obj) &&
	M_OBJ_IS_VAR(e_messages[j]) &&
	(M_TOK(e_messages[last_arg_ptr]) == METHODMSGLABEL))
      /* i.e., it's an instance var of a method result, which we've
	 just set in fixup_forward_receiver_obj */
      break;

    if (M_TOK(e_messages[j]) == LABEL) {
      if (e_messages[j] -> receiver_msg == e_messages[t_last_arg_ptr]) {
	t_last_arg_ptr = j;
      }
    }
  }

  for (j = expr_start_idx; 
       (j >= t_last_arg_ptr) && e_messages[j]; j--) {
    if (IS_OBJECT(e_messages[j] -> value_obj)) {
      __s = e_messages[j]->value_obj;
      /*
       *  Don't decrement an object if one of its
       *  instance variables is a result.
       */
      if (IS_OBJECT(expr_result)) {
	if ((expr_result -> __o_p_obj == NULL) ||
	    (expr_result -> __o_p_obj != __s)) {
	  if (!_cleanup_temporary_objects
	      (((e_messages[j] -> value_obj &&
		 (e_messages[j] -> value_obj ->__o_p_obj 
		  != e_messages[j] -> obj)) ?
		e_messages[j] -> value_obj : NULL),
	       expr_result, NULL, rt_cleanup_null)) {
	    __s = e_messages[j] -> value_obj;
	    for (j_1 = j; j_1 >= last_arg_ptr; j_1--) {
	      if (e_messages[j_1] -> value_obj == __s)
		e_messages[j_1] -> value_obj = NULL;
	      if (e_messages[j_1] -> receiver_obj == __s)
		e_messages[j_1] -> receiver_obj = NULL;
	    }
	  }
	}
      }
      if (IS_OBJECT(__s)) {
	if (__s -> attrs & OBJECT_IS_I_RESULT) {
	  if (__s != expr_result) {
	    __ctalkDeleteObject (__s);
	  }
	} else if (__s -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) {
	  if (__s != expr_result) {
	    __ctalkDeleteObject (__s);
	  }
	}
      }
      /*
       *  Might be redundant.
       */
      if (last_arg_ptr != -1) {
	for (j_1 = j; j_1 >= last_arg_ptr; j_1--) {
	  if (e_messages[j_1] -> value_obj == __s) 
	    e_messages[j_1] -> value_obj = NULL;
	  if (e_messages[j_1] -> obj == __s) 
	    e_messages[j_1] -> obj = NULL;
	}
      }
    }
    /* If we have a prefix operator with parentheses between the
       operator and the receiver. */
    if (e_messages[j] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
      i_2 = next_msg (e_messages, j);
      if (M_TOK(e_messages[i_2]) == OPENPAREN) {
	int i_3, i_4;
	i_3 = expr_match_paren (C_EXPR_PARSER, i_2);
	for (i_4 = j; i_4 >= i_3; i_4--) {
	  if (is_temporary_i_param (e_messages[i_4] -> value_obj)) {
	    OBJECT *__o_tmp = e_messages[i_4] -> value_obj;
	    clear_expr_value_obj (e_messages, j, i_3, __o_tmp);
	    __ctalkDeleteObject (__o_tmp);
	  }
	  e_messages[i_4] -> value_obj = expr_result;
	  ++e_messages[i_4] -> evaled;
	}
      }
    }
    e_messages[j]->value_obj = expr_result;
    ++e_messages[j]->evaled;
  }
  return SUCCESS;
}

static char *undefined_method_text (char *rcvr_name, char *rcvr_class,
				    char *method_name) {
  static char s[MAXMSG];
  sprintf 
    (s, 
     "eval_expr: (): Object %s (Class %s) does not understand message %s.\n",
     rcvr_name, rcvr_class, method_name);
  return s;
}

static void undefined_receiver_class_warning_a (char *msg_name, char 
					  *object_name) {
  __warning_trace ();
  _warning ("eval_expr: Class for message %s (object %s) not found.\n",
	    msg_name, object_name);
}

static void undefined_receiver_class_error_a (char *object_name) {
  __warning_trace ();
  _error ("__ctalkEvalExpr: Undefined class for receiver %s.\n",
	  object_name);
}

static void delete_created_object_ref (MESSAGE_STACK messages,
				       int idx, 
				       bool obj_ref_is_created,
				       OBJECT *reffed_obj) {
  if ((messages[idx] -> attrs & RT_TOK_OBJ_IS_CREATED_PARAM) &&
      obj_ref_is_created) {
    __ctalkDeleteObject(reffed_obj);
  }
}
				       
static inline void set_receiver_msg_and_obj 
  (MESSAGE *m, MESSAGE *m_prev_msg) {
  if (m && m_prev_msg) {
    if ((m_prev_msg -> attrs & RT_OBJ_IS_INSTANCE_VAR) ||
	(m_prev_msg -> attrs & RT_OBJ_IS_CLASS_VAR)) {
      m -> receiver_obj = m_prev_msg -> receiver_obj;
    } else {
      m -> receiver_obj = M_VALUE_OBJ(m_prev_msg);
    }
    m -> receiver_msg = 
      ((m_prev_msg -> receiver_msg) ?
       m_prev_msg -> receiver_msg : m_prev_msg);
  }
}

/*
 *  For cases where we need to deal with a Symbol-class instance var
 *  and use the semantics in Symbol : =.  If an expression looks like this:
 *
 *    *myObj mySymInstVar = someObj;
 *
 *  we change it to the following, and let Symbol : = handle
 *  the semantics.
 *
 *    myObj mySymInstVar = someObj
 */
static inline void set_receiver_msg_and_obj_fixup (int tok_idx,
						   MESSAGE *m,
						   MESSAGE *m_prev_msg) {
  int i, lookahead;
  EXPR_PARSER *p;
  if (m && m_prev_msg) {
    if ((m_prev_msg -> attrs & RT_OBJ_IS_INSTANCE_VAR) ||
	(m_prev_msg -> attrs & RT_OBJ_IS_CLASS_VAR)) {
      m -> receiver_obj = m_prev_msg -> receiver_obj;
    } else {
      m -> receiver_obj = M_VALUE_OBJ(m_prev_msg);
    }
    m -> receiver_msg = 
      ((m_prev_msg -> receiver_msg) ?
       m_prev_msg -> receiver_msg : m_prev_msg);
    if ((lookahead = next_msg (e_messages, tok_idx)) != ERROR) {
      if (IS_C_ASSIGNMENT_OP(M_TOK(e_messages[lookahead]))) {
	p = expr_parsers[expr_parser_ptr];
	if (IS_OBJECT(m -> obj -> instancevars) &&
	    (m -> obj -> instancevars -> __o_class ==
	     rt_defclasses -> p_symbol_class)) {
	  for (i = tok_idx; i <= p -> msg_frame_start; ++i) {
	    if (M_ISSPACE(e_messages[i]))
	      continue;
	    if (M_TOK(e_messages[i]) == LABEL)
	      continue;
	    if (e_messages[i] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
	      e_messages[i] -> name[0] = ' '; e_messages[i] -> name[1] = '\0';
	      e_messages[i] -> tokentype = WHITESPACE;
	      e_messages[i] -> attrs = 0;
	    }
	  }
	}
      }
    }
  }
}

static inline int set_c_rcvr_idx 
  (MESSAGE *m_prev_msg, int prev_msg_ptr_tmp,
		    int prev_c_rcvr_idx) {
  if (!(m_prev_msg -> attrs & RT_OBJ_IS_INSTANCE_VAR) &&
      !(m_prev_msg -> attrs & RT_OBJ_IS_CLASS_VAR))
    return prev_msg_ptr_tmp;
  else
    return prev_c_rcvr_idx;
}

static inline bool internal_exception_trap (void) {
  I_EXCEPTION *ex;
  if ((ex = __ctalkTrapExceptionInternal (NULL)) != NULL) {
    if (ex -> is_internal != 0) {
      return true;
    }
  }
  return false;
}

static inline bool internal_exception_trap_b (MESSAGE_STACK messages, int idx) {
  I_EXCEPTION *ex;
  if ((ex = __ctalkTrapExceptionInternal (messages[idx])) != NULL) {
    __ctalkExceptionNotifyInternal (ex);
    __ctalkDeleteExceptionInternal (ex);
    return true;
  }
  return false;
}

/* Check for an object with an i on a prefix method's receiver label, and
   give that priority, regardless of a previous value object. */
static OBJECT *prefix_receiver (MESSAGE *m) {
  void *c;
  OBJECT *value_obj, *obj;

  value_obj = M_VALUE_OBJ (m);
  obj = M_OBJ(m);

  if (IS_OBJECT(value_obj)) {
    if ((c = active_i (value_obj)) != I_UNDEF) {
      return value_obj;
    }
    if ((c = active_i (obj)) != I_UNDEF) {
      return obj;
    }

    return value_obj;
  }

  if (IS_OBJECT (obj))
    return obj;
  else
    return NULL;
}

#define AGGREGATE_NAME_CHECK_RETURN {if (get_method_arg_cvars(namebuf)) \
                              return i; else return -1;}

static int is_aggregate_type (MESSAGE_STACK messages, int token_start, char *namebuf) {

  int i,
    n_brackets,
    n_parens,
    lookahead;
  bool have_brackets,
    have_deref,
    have_parens;

  if (messages[token_start] -> tokentype != LABEL)
    return -1;
  if (token_start == C_EXPR_PARSER -> msg_frame_top)
    return -1;

  if (messages[token_start] -> attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL)
    return -1;

  /* If not needed, return quickly. */
  if ((lookahead = next_msg (messages, token_start)) != ERROR) {
    if ((M_TOK(messages[lookahead]) != DEREF) &&
	(M_TOK(messages[lookahead]) != PERIOD) &&
	(M_TOK(messages[lookahead]) != ARRAYOPEN)) {
      return -1;
    }
  }
  *namebuf = 0;
  n_brackets = n_parens = 0;
  have_brackets = have_deref = have_parens = False;

  for (i = token_start; i >= C_EXPR_PARSER -> msg_frame_top; i--) {
    
    if (M_ISSPACE(messages[i]))
      continue;
    switch (messages[i] -> tokentype) 
      {
      case LABEL:
	strcatx2 (namebuf, messages[i] -> name, NULL);
	lookahead = next_msg (messages, i);
	if (lookahead == ERROR) {
	  if (!n_brackets && !n_parens) {
	    if (have_deref == True) {
	      if (i == C_EXPR_PARSER -> msg_frame_top) {
		AGGREGATE_NAME_CHECK_RETURN;
	      } else {
		return -1;
	      }
	    } else {
	      return -1;
	    }
	  }
	}
	if ((messages[lookahead] -> tokentype != PERIOD) &&
	    (messages[lookahead] -> tokentype != DEREF) &&
	    (messages[lookahead] -> tokentype != ARRAYOPEN) &&
	    (messages[lookahead] -> tokentype != OPENPAREN)) {
	  
	  if (!n_brackets && !n_parens) {
	    if (have_deref == True) {
	      AGGREGATE_NAME_CHECK_RETURN;
	    } else {
	      return -1;
	    }
	  }
	}
	/*
	 *  Don't keep going for a method or function call.
	 */
	if ((i == token_start) &&
	    (messages[lookahead] -> tokentype == OPENPAREN))
	  return -1;
	break;
      case PERIOD:
      case DEREF:
	have_deref = True;
	strcatx2 (namebuf, messages[i] -> name, NULL);
	break;
      case ARRAYOPEN:
	have_brackets = True;
	++n_brackets;
	strcatx2 (namebuf, messages[i] -> name, NULL);
	break;
      case ARRAYCLOSE:
	--n_brackets;
	strcatx2 (namebuf, messages[i] -> name, NULL);

	/*
	 *  Check for array[n].label or array[n] -> label.
	 */
	if ((have_brackets == True) && !n_brackets && !n_parens) {
	  lookahead = next_msg (messages, i);
	  if (lookahead == ERROR) {
	    AGGREGATE_NAME_CHECK_RETURN;
	  }
	  if ((messages[lookahead] -> tokentype != PERIOD) &&
	      (messages[lookahead] -> tokentype != DEREF) &&
	      (messages[lookahead] -> tokentype != ARRAYOPEN)) {
	    AGGREGATE_NAME_CHECK_RETURN;
	  }
	}
	break;
      case OPENPAREN:
	++n_parens;
	have_parens = True;
	strcatx2 (namebuf, messages[i] -> name, NULL);
	break;
      case CLOSEPAREN:
	--n_parens;
	strcatx2 (namebuf, messages[i] -> name, NULL);
	if ((have_parens == True) && !n_brackets && !n_parens)
	  AGGREGATE_NAME_CHECK_RETURN;
	break;
      default:
	strcatx2 (namebuf, messages[i] -> name, NULL);
	break;
      }

  }

  return -1;
}

static void terminal_tok_obj (MESSAGE *m_prev_msg, MESSAGE *m,
			      OBJECT *inst_var,
			      OBJECT *cls_var) {
  if (inst_var)
    __refObj (OBJREF(m_prev_msg -> value_obj),
	      OBJREF(inst_var));
  else if (cls_var)
    __refObj (OBJREF(m_prev_msg -> value_obj),
	      OBJREF(cls_var));

  if (m -> obj == NULL) {
    if (inst_var) {
      __refObj (OBJREF(m -> obj), OBJREF(inst_var));
      m -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
      eval_status |=
	(EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
    } else if (cls_var) {
      __refObj (OBJREF(m -> obj), OBJREF(cls_var));
      m -> attrs |= RT_OBJ_IS_CLASS_VAR;
      eval_status |=
	(EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_CLASS_VAR);
    }
  } else {
    if (inst_var) {
      __refObj (OBJREF(m -> value_obj), OBJREF(inst_var));
      m -> attrs |= RT_VALUE_OBJ_IS_INSTANCE_VAR;
      eval_status |=
	(EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
    } else if (cls_var) {
      __refObj (OBJREF(m -> value_obj), OBJREF(cls_var));
      m -> attrs |= RT_VALUE_OBJ_IS_CLASS_VAR;
      eval_status |=
	(EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_CLASS_VAR);
    }
  }
}

/*
 *  Scopes automatically applied to existing objects, mainly receivers. 
 *  They should not be used for newly created objects.
 *
 *  The n_params wanted argument is the number of parameters the
 *  primary method in an expression needs, or -1 if the compiler
 *  doesn't know or doesn't care.
 */
#define AUTOMATIC_SCOPES (METHOD_USER_OBJECT|CREATED_PARAM| \
 			  CVAR_VAR_ALIAS_COPY|VAR_REF_OBJECT)
OBJECT *__ctalkEvalExpr (char *s) {
  OBJECT *__recv_obj, *__recv_class, *arg_object = NULL;
  int scope;

  /*
   *   Find the receiver depending on whether it is a class
   *   object, a dictionary object, or a method parameter.
   *
   *   We have to push the receiver again to make the stack
   *   the same for _eval_expr () as when called from rt_arg ().
   *
   *   If there's no receiver, then the receiver class defaults
   *   to, "Integer."
   */

  __recv_obj = __ctalk_receiver_pop ();
  __ctalk_receiver_push (__recv_obj);
  __ctalk_receiver_push_ref (__recv_obj);

  if (__recv_obj) {
    if ((__recv_class = __recv_obj->__o_class) == NULL) {
      undefined_receiver_class_error_a (__recv_obj -> __o_name);
    }
    scope = __recv_obj -> scope;
    scope &= ~AUTOMATIC_SCOPES;
  } else {
    __recv_class = get_class_by_name (INTEGER_CLASSNAME);
    scope = RECEIVER_VAR;
  }

  eval_status = 0;

  arg_object = eval_expr (s, __recv_class, NULL, NULL, scope, FALSE);

  /*
   *  Critical errors from an expression should be handled by
   *  the caller of __ctalkEvalExpr ().
   */

  if (internal_exception_trap ()) {
    __ctalkHandleRunTimeExceptionInternal ();
    if (!IS_OBJECT (arg_object)) {
      arg_object = create_object_init_internal
	("result", rt_defclasses -> p_integer_class, scope, "0");
    }    
  }
  __ctalk_receiver_pop_deref ();

  if (IS_OBJECT(arg_object) && IS_VARTAG(arg_object -> __o_vartags)) {
    if (arg_object && (arg_object -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) &&
	(arg_object -> __o_vartags -> tag == NULL) && !is_arg (arg_object)) {
      __ctalkDeleteObject (arg_object);
      arg_object = NULL;
    }
  } else if (IS_OBJECT(arg_object) && 
	     (arg_object -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) &&
	     !is_arg (arg_object) &&
	     (arg_object -> __o_p_obj == NULL)) {
    __ctalkDeleteObject (arg_object);
    arg_object = NULL;
  } else if (IS_OBJECT(arg_object) &&
	     (arg_object -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) &&
	     !is_arg (arg_object) &&
	     arg_object -> __o_value[0] == 0) {
    __ctalkDeleteObject (arg_object);
    arg_object = NULL;
  }

  reset_last_eval_result ();

  if (arg_object) {
    if ((arg_object -> attrs & OBJECT_IS_I_RESULT) &&
	((arg_object -> __o_vartags == NULL) ||
	 (arg_object -> __o_vartags -> tag == NULL))) {
      __ctalkRegisterUserObject (arg_object);
    } else if (arg_object -> scope & TYPECAST_OBJECT) {
      __ctalkRegisterUserObject (arg_object);
    }
  }

  return arg_object;
}

/* 
   "Unbuffered" version - returns a C NULL instead of
   a null result object.
*/
OBJECT *__ctalkEvalExprU (char *s) {
  OBJECT *__recv_obj, *__recv_class, *arg_object = NULL;
  int scope;

  /*
   *   Find the receiver depending on whether it is a class
   *   object, a dictionary object, or a method parameter.
   *
   *   We have to push the receiver again to make the stack
   *   the same for _eval_expr () as when called from rt_arg ().
   *
   *   If there's no receiver, then the receiver class defaults
   *   to, "Integer."
   */

  __recv_obj = __ctalk_receiver_pop ();
  __ctalk_receiver_push (__recv_obj);
  __ctalk_receiver_push_ref (__recv_obj);

  if (__recv_obj) {
    if ((__recv_class = __recv_obj->__o_class) == NULL) {
      undefined_receiver_class_error_a (__recv_obj -> __o_name);
    }
    scope = __recv_obj -> scope;
    /* TODO - this is not very effective. */
    scope &= ~AUTOMATIC_SCOPES;
  } else {
    __recv_class = get_class_by_name (INTEGER_CLASSNAME);
    scope = RECEIVER_VAR;
  }

  eval_status = 0;

  arg_object = eval_expr (s, __recv_class, NULL, NULL, scope, FALSE);

  /*
   *  Critical errors from an expression should be handled by
   *  the caller of __ctalkEvalExpr ().
   */

  if (internal_exception_trap ()) {
    __ctalkHandleRunTimeExceptionInternal ();
    if (!IS_OBJECT (arg_object)) {
      arg_object = create_object_init_internal
	("result", rt_defclasses -> p_integer_class, scope, "0");
    }    
  }
  __ctalk_receiver_pop_deref ();

  if (is_zero_q (arg_object -> instancevars ?
		 arg_object -> instancevars -> __o_value :
		 arg_object -> __o_value)) {
    if ((arg_object -> attrs & OBJECT_IS_DEREF_RESULT) ||
	(arg_object -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) ||
	(arg_object -> scope & CVAR_VAR_ALIAS_COPY)) {
      if (!(arg_object -> scope & METHOD_USER_OBJECT)) {
	__ctalkDeleteObject (arg_object);
      }
      arg_object = NULL;
    }
  }

  reset_last_eval_result ();

  if (arg_object) {
    if ((arg_object -> attrs & OBJECT_IS_I_RESULT) &&
	((arg_object -> __o_vartags == NULL) ||
	 (arg_object -> __o_vartags -> tag == NULL))) {
      __ctalkRegisterUserObject (arg_object);
    }
  }

  return arg_object;
}

int e_message_push (MESSAGE *m) {
#ifdef MINIMUM_MESSAGE_HANDLING
  e_messages[e_message_ptr--] = m;
  return e_message_ptr + 1;
#else  
  if (e_message_ptr == 0) {
    _warning (_("e_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("e_message_push %d. %s."), e_message_ptr, m -> name);
#endif
  e_messages[e_message_ptr--] = m;
  return e_message_ptr + 1;
#endif /* #if MINIMUM_MESSAGE_HANDLING */
}

MESSAGE *e_message_pop (void) {
#ifdef MINIMUM_MESSAGE_HANDLING
  MESSAGE *m;
    m = e_messages[e_message_ptr + 1];
    e_messages[++e_message_ptr] = NULL;
    return m;
#else
  MESSAGE *m;
  if (e_message_ptr == P_MESSAGES) {
    _warning (_("e_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("e_message_pop %d. %s."), e_message_ptr, 
	 e_messages[e_message_ptr+1] -> name);
#endif
  if (e_messages[e_message_ptr + 1] && 
      IS_MESSAGE(e_messages[e_message_ptr + 1])) {
    m = e_messages[e_message_ptr + 1];
    e_messages[++e_message_ptr] = NULL;
    return m;
  } else {
    e_messages[++e_message_ptr] = NULL;
    return NULL;
  }
#endif /* #if MINIMUM_MESSAGE_HANDLING */
}

/*
 *  Return the method's params table entry for the next argument, 
 *  or NULL.
 */

PARAM *has_param_def (METHOD *m) {

  if (!m) return NULL;

  if (m -> n_args >= m -> n_params) {
#ifdef DEBUG_DYNAMIC_C_ARGS
    _warning ("has_param_def (): method %s has no further definitions.\n",
	      m -> name);
#endif
    return NULL;
  }
  return m -> params[m -> n_args];
}

static OBJECT *check_label_i (OBJECT *obj) {
  void *c_i;
  if ((c_i = active_i (obj)) == I_UNDEF) {
    return obj;
  } else {
    return NULL;
  }
}

static OBJECT *eval_label_token (char *name, bool is_arg_expr) {
  OBJECT *e_result;
  char *p = name;

  if (!isalpha (*p) && *p != '_')
    return NULL;
  else
    ++p;

  while (isalnum (*p) || *p == '_')
    ++p;
  if (*p != 0)
    return NULL;

  if (str_eq (name, "self")) {
    /* this is basically a duplication of how self would
       be evaluated below in eval_expr */
    if (__call_stack[__call_stack_ptr+1] -> inline_call) {
      return check_label_i (rtinfo.rcvr_obj);
    } else {

      if ((is_constant_rcvr == True) || (is_arg_expr == True)) {
	return check_label_i (self_expr (NULL, is_arg_expr));
      } else {
	return check_label_i (__ctalk_receivers[__ctalk_receiver_ptr + 2]);
      } 
	    
    }
  } else if ((e_result = __ctalk_get_eval_expr_tok (name)) != NULL) {
    return check_label_i (e_result);
  } else {
    return NULL;
  }
}

/* resolve a single object, but only if the the message doesn't
   already have an object resolved for it */
static inline bool resolve_single_object (MESSAGE *m, char *name) {
  if (!IS_OBJECT(m -> obj)) {
    if ((m -> obj = __ctalk_get_object (name, NULL)) != NULL) {
      return true;
    } else {
      return false;
    }
  } else {
    return true;
  }
  return false;
}

/* be sure that have_instance_var and have_class_var are initialized
   before calling this fn.  Doesn't (yet) handle class names. */
static bool non_method_label (MESSAGE *prev_msg,
			      char *msg_name,
			      char *pname,
			      OBJECT **instance_var_tmp,
			      OBJECT **class_var_tmp) {
  if ((*instance_var_tmp = __ctalkGetInstanceVariable
       (prev_msg -> obj, msg_name, FALSE)) != NULL) {
  } else if (IS_OBJECT(prev_msg -> obj) &&
	     str_eq (pname, prev_msg -> obj -> __o_name) &&
	     IS_OBJECT(prev_msg -> obj -> __o_p_obj) &&
	     ((*instance_var_tmp =
	       __ctalkGetInstanceVariable (prev_msg -> obj -> __o_p_obj,
					   pname, FALSE)) != NULL)) {
    /* this situation can occur if we have an instance or class
       var expression as an argument - the entire expression
       has the value of the terminal var token. */
    return true;
  } else {
    *class_var_tmp = __ctalkFindClassVariable (pname, FALSE);
  }
  
  if ((*instance_var_tmp == NULL && *class_var_tmp == NULL) &&
      __call_stack[__call_stack_ptr + 1] -> inline_call &&
      (prev_msg -> attrs & RT_TOK_IS_SELF_KEYWORD)) {
    /* If we haven't found an instance var or a method, 
       and the preceding token is "self" and we're in an
       argument block, then check for the message in the
       receiver's superclass, and warn the user if that's
       the case. */
    OBJECT *encl_rcvr_class = __call_stack[__call_stack_ptr + 2]
      -> rcvr_class_obj;
    self_enclosing_class_message_warning
      (__call_stack[__call_stack_ptr+2] -> rcvr_class_obj,
       expr_parsers[expr_parser_ptr] -> expr_str,
       msg_name);
  }

  return !(*instance_var_tmp || *class_var_tmp);
}

/* return false and skip the entire instance/class var and method
   lookup if we find the object for a receiver message here. */
static bool dont_have_prev_message_object (MESSAGE *m_label,
					   int aggregate_type_end,
					   MESSAGE *prev_message) {
  if (aggregate_type_end > 0)
    /* i.e., the current token is part of an aggregate term */
    return true;
  if (prev_message == NULL) {
    m_label -> obj = __ctalk_get_arg_tok (M_NAME(m_label));
    if (m_label -> obj) {
      return false;
    }
  } else if (!IS_OBJECT(prev_message -> obj)) {
    /* a receiver object can appear later in an expression, too. */
    m_label -> obj = __ctalk_get_arg_tok (M_NAME(m_label));
    if (m_label -> obj) {
      return false;
    }
  }
  return true;
}

/* The cache for constant token object classes. */
OBJECT *arg_class_string = NULL,
  *arg_class_character = NULL,
  *arg_class_integer = NULL,
  *arg_class_longinteger = NULL,
  *arg_class_float = NULL;

/*
 *   Evaluate an expression. 
 *
 *   NOTE - unlike __ctalk_arg (), __ctalkEvalExpr () 
 *   and __ctalk_arg_expr () call this function 
 *   with METHOD == NULL.
 */

OBJECT *eval_expr (char *s, OBJECT *recv_class, METHOD *method, 
		   OBJECT *subexpr_rcvr_arg, int scope, 
		   int is_arg_expr) {

  int i, i_1, i_2, j, j_1,
    aggregate_type_end,
    aggregate_type_start,
    prev_rcvr_msg_ptr,
    next_msg_ptr,
    expr_end_ptr,
    prev_msg_ptr,
    expr_lookahead_ptr,
    fn_tok_ptr,
    precedence = 0,
    matching_paren_ptr,
    __typecast_close_paren_idx = -1,
    __typecast_open_paren_idx = -1,
    c_rcvr_idx,           /* Start token of receiver of compound statements.*/
    cvar_obj_is_created,
    prev_tok_idx,
    next_tok_idx,
    m_p_idx,
    last_arg_ptr,
    old_arg_frame_top,
    op1_ptr, op2_ptr,
    prev_mthd_idx,
    prefix_expr_end,
    prefix_expr_start,
    max_params;
  OBJECT *class_var_tmp, *instance_var_tmp,
    *e_class, *m_prev_value_obj, *m_prev_super_obj, *n_result, *e_result_dup,
    *obj;
  MESSAGE *m = NULL,      /* These must be initialized.        */
    *m_prev_msg = NULL,
    *n;
  OBJECT *postfixed_object = NULL;
  int all_objects_deleted;
  int m_next_idx;
  OBJECT *result_obj, *value_obj, *arg_class, *val_class, *subexpr_result,
    *subexpr_rcvr, *param_class, *rcvr_tmp, *m_prev_value_value_obj;
  OBJECT *op1_subexpr_result, *op2_subexpr_result;
  OBJECT *e_result;
  OBJECT *prefix_rcvr;
  OBJECT *var_i_var, *var;
  CVAR *c;
  char *pname, namebuf[MAXLABEL];
  EXPR_PARSER *p;
  char tmpbuf[MAXMSG], esc_buf_out[MAXMSG],
    s_out[MAXMSG], struct_buf[MAXMSG];
  METHOD *e_method;
  int args_in_expr;
  void *c_i;
  bool typecast_is_ptr, is_arg_value;
  METHOD *m_set[MAXARGS];
  int n_m_set;

  if (*s == '\0')
    return null_result_obj (method, scope);

  /* Deleting the leading whitespace saves unnecessary backtracking. */
  if (isspace ((int)*s)) {
    trim_leading_whitespace (s, s_out);
    pname = s_out;
  } else {
    pname = s;
  }

  if ((result_obj = eval_label_token (pname, (bool)is_arg_expr)) != NULL) {
    return result_obj;
  } else {
    elide_cvartab_struct_alias (pname, struct_buf);
    if (*struct_buf) {
      pname = struct_buf;
    }
  }
  
  value_obj = arg_class = val_class = subexpr_result =
    subexpr_rcvr = param_class = NULL;
  op1_subexpr_result = op2_subexpr_result = e_result = NULL;

  aggregate_type_start = aggregate_type_end = c_rcvr_idx =
    prev_msg_ptr = -1;

  ++expr_parser_lvl;
  p = __new_expr_parser ();
  p -> rcvr_frame_top = __ctalk_receiver_ptr;
  p -> msg_frame_start = e_message_ptr;
  p -> entry_eval_status = eval_status;
  p -> is_arg_expr = (bool)is_arg_expr;
  p -> expr_str = s;
  p -> m_s = e_messages;
  expr_parsers[--expr_parser_ptr] = p;

  p -> msg_frame_top = tokenize_no_error (e_message_push, pname);

  for (i = p -> msg_frame_start; i >= p -> msg_frame_top; i--) {

    m = e_messages[i];

    switch (m -> tokentype)
      {
      case LABEL:

	if (prev_msg_ptr != ERROR)
	  m_prev_msg = e_messages[prev_msg_ptr];
	/*
	 *  Handle an an array or struct member.  If we find 
	 *  the declaration end here, we don't need to 
	 *  count opening and closing blocks below.
	 *  However, we do need to check for a method 
	 *  message after a closing block, so we should do
	 *  it here.
	 *
	 *  If, "aggregate_type_start," and, "aggregate_type_end,"
	 *  are not -1, they are used below to set the message
	 *  object for the entire aggregate expression, and 
	 *  they are reset after the expression tokens' object
	 *  is set.
	 *
	 *  TO DO - Evaluate array element indexes that are
	 *  expressions.
	 */
	/* 
	 *  is_aggregate_type returns -1 on error, -2 if the first token
	 *  is a method.  We can use the return value below to avoid
	 *  another check for a method label.  The method message
	 *  label check is valid only for the first token of a possible
	 *  aggregate reference - this function does not skip 
	 *  to the end of a non-aggregate expression here.
	 *
	 *  It also fills in namebuf, - we don't just strcpy () it
	 *  for aggregates.
	 *
	 *  Doesn't need to be called for the final two tokens,
	 *  btw.
	 */
	if ((i > (p -> msg_frame_top + 1)) &&
	    ((aggregate_type_end =
	      is_aggregate_type (e_messages, i, namebuf)) > 0)) {
	  aggregate_type_start = i;
	  i = aggregate_type_end;
	  prev_msg_ptr = prev_msg (e_messages, i);
	  pname = namebuf;
	} else {
	  pname = m -> name;
	}
	instance_var_tmp = class_var_tmp = NULL;

	if (str_eq (pname, "self")) {

	  /*
	   *  Find out if the label refers to an object or a class.  If
	   *  not, create the object.  If the label refers to a C variable,
	   *  use that as the object's value.
	   *  
	   *  An argument object that represents a C variable has the
	   *  class, "Expr," and the "value," instance variable has 
	   *  the class of the receiver and the value of the C variable.
	   *
	   *  In the case where the receiver is a constant expression,
	   *  then we have to use the most immediate receiver.  For the
	   *  moment, we'll simply use the _is_constant_rcvr state, 
	   *  which is set and cleared in __ctalk_arg ().
	   *
	   *  This is also the case where the receiver is a normal object.
	   *  To cope with it, __ctalk_arg () sets is_arg_expr to True.
	   *  This use is experimental until __ctalkEvalExpr () can 
	   *  figure out on its own whether self appears as an lvalue
	   *  or receiver for an entire statement, or as the value in an 
	   *  argument.  __ctalkEvalExpr (), and the functions that call it, 
	   *  should also be able to DTRT if we have an expression like,
	   *  "self subString 1, self length - 1," or,
	   *  "myString subString 1, self length - 1."
	   *
	   *  Generally, self should refer to the same object
	   *  wherever the identifier appears within one set of braces.
	   */
	  m -> obj = resolve_self (is_constant_rcvr, is_arg_expr);
	  ++m -> evaled;
	  m -> attrs |= RT_TOK_IS_SELF_KEYWORD;
	  prev_msg_ptr = i;
	  continue;
	} else if (str_eq (pname, "eval")) {
	  /* 
	   *  Be draconian here and say, if "eval" appears *anywhere*
	   *  in an expression that we've already decided needs to be 
	   *  evaluated, then elide it.
	   */
	  m -> name[0] = ' '; m -> name[1] = '\0';
	  m -> tokentype = WHITESPACE;
	  ++m -> evaled;
	  prev_msg_ptr = i;
	  continue;
	} else {

	  if (dont_have_prev_message_object (m, aggregate_type_end,
					     m_prev_msg)) {
	    METHOD *test_mthd;
	    if ((aggregate_type_end == -2) ||
		((m_prev_msg && 
		  non_method_label (m_prev_msg, m -> name,
				    pname, &instance_var_tmp,
				    &class_var_tmp) &&
		  ((test_mthd =
		    get_method_greedy (m_prev_msg -> obj,
				       pname, m_set, &n_m_set))
		   != NULL)))) {
	      if (m -> tokentype == LABEL) {
		m -> tokentype = METHODMSGLABEL;
		if (m_prev_msg && m_prev_msg -> obj) {
		  m -> receiver_msg = m_prev_msg;
		  m -> receiver_obj = m_prev_msg -> obj;
		  /* 
		   *  The fn only checks the number of arguments
		   *  for the first method of an expression for now -
		   *  any  methods within argument lists get handled
		   *  by _rt_method_args (), called below.
		   */
		  if (!(m -> attrs & RT_DATA_IS_NR_ARGS_DECLARED)) {
		    if (test_mthd) {
		      if (CAN_CACHE_METHOD(test_mthd,m_prev_msg)) {
			/* 
			   The method that is cached here only
			   gets used in the second pass if:

			   1. It takes a fixed number of arguments.
			   2. The receiver object and message
			   are the same as saved just above.
			   (There is no separate value_obj
			   set in the receiver_msg, or a super
			   keyword.)
			   3. The previous message is an
			   instance or class variable and
			   c_rcvr_idx points to a token to
			   the left of it.  For now, the
			   alignment of the two passes
			   should stay the way it is.
			*/
			if ((max_params = max_params_set (m_set, n_m_set))
			    == 0) {
			  m -> attr_data = test_mthd -> n_params;
			  m -> attrs |= RT_DATA_IS_CACHED_METHOD;
			  m -> attr_method = (long int)test_mthd;
			} else {
			  for (; max_params >= 0; --max_params) {
			    for (i_2 = 0; i_2 < n_m_set; ++i_2) {
			      if (m_set[i_2] -> n_params == max_params) {
				m -> attr_data = 
				  __rt_method_arglist_n_args (p, i, m_set[i_2]);
				if (m_set[i_2] -> n_params == m -> attr_data) {
				  m -> attr_data = test_mthd -> n_params;
				  m -> attrs |= RT_DATA_IS_CACHED_METHOD;
				  m -> attr_method = (long int)m_set[i_2];
				  goto have_method_fit;
				}
			      }
			    }
			  }
			have_method_fit:
			  if (!(m -> attrs & RT_DATA_IS_CACHED_METHOD)) {
			    __warning_trace ();
			    _error ("method mismatch: Method \"%s\":\n\n\t%s\n\n",
				    M_NAME(m),
				    expr_parsers[expr_parser_ptr] -> expr_str);
			  }
			}
		      } else {
			m -> attr_data = 
			  __rt_method_arglist_n_args (p, i, test_mthd);
		      }
		    }
		    m -> attrs |= RT_DATA_IS_NR_ARGS_DECLARED;
		  }
		  ++m -> evaled;
		  prev_msg_ptr = i;
		  continue;
		} else {
		  if (M_TOK(m_prev_msg) == METHODMSGLABEL) {
		    m -> receiver_msg = m_prev_msg;
		    m -> receiver_obj = m_prev_msg -> obj;

		    if (!method_label_is_arg (i)) {
		      /*
		       * Do a little fixup here.  If this token 
		       * follows another METHODMSGLABEL, then 
		       * set n_args_declared to ANY_ARGS, because it was
		       * probably mis-set for the previous token.
		       * __rt_method_arglist_n_args () only checks
		       * syntactically - it doesn't evaluate any
		       * of the tokens on its own, so it didn't
		       * check (again) whether two methods appeared
		       * in succession.
		       *
		       * We can't just say use 0 args here, because
		       * a method can take another method as an argument -
		       * like a map* method.  
		       */
		      m -> attr_data = ANY_ARGS;
		      m -> attrs |= RT_DATA_IS_NR_ARGS_DECLARED; 
		      m_prev_msg -> attrs |= RT_DATA_IS_NR_ARGS_DECLARED;
		      /* TODO - This *should* be okay when set to zero. 
			 There probably needs to be another fixup below.
			 See the break3.c test program for an example
			 that can break if we set the previous message
			 required args to zero.
		      */
		      m_prev_msg -> attr_data = ANY_ARGS;
		    }

		    ++m -> evaled;
		    prev_msg_ptr = i;
		    continue;
		  }
		}
	      }
 	    } else { /* if (dont_have_prev_message_object ... */
	      if (IS_OBJECT(class_var_tmp)) {
		m -> obj = class_var_tmp;
		m -> attrs = RT_OBJ_IS_CLASS_VAR;
		if (m_prev_msg) {
		  m -> receiver_msg = m_prev_msg;
		  m -> receiver_obj = m_prev_msg -> obj;
		}
		++m -> evaled;
		prev_msg_ptr = i;
		continue;
	      } else if (m_prev_msg && IS_OBJECT(M_VALUE_OBJ(m_prev_msg)) &&
			 ((m -> obj = instance_var_tmp) != NULL)) {
		m -> attrs = RT_OBJ_IS_INSTANCE_VAR;
		set_receiver_msg_and_obj_fixup (i, m, m_prev_msg);
		if ((c_rcvr_idx = 
		     set_c_rcvr_idx (m_prev_msg, prev_msg_ptr,
				     c_rcvr_idx)) != ERROR) {

		  if (IS_OBJECT (e_messages[c_rcvr_idx] -> value_obj)) {
		    if (!(e_messages[c_rcvr_idx]->value_obj->attrs &
			  OBJECT_IS_I_RESULT)) {
		      e_messages[c_rcvr_idx] -> value_obj = m -> obj;
		    } 
		  } else {
		    e_messages[c_rcvr_idx] -> value_obj = m -> obj;
		  }

		}
		/* If we have an instance var and no i values in
		   the receiver object, then we're done. */
		if (c_rcvr_idx == p -> msg_frame_start &&
		    i == p -> msg_frame_top &&
		    !has_active_i (m_prev_msg -> obj)) {
		  result_obj = m -> obj;
		  eval_status = 
		    (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
		  for (j = p -> msg_frame_start;
		       j >= p -> msg_frame_top; j--) {
		    reuse_message(e_message_pop ());
		  }
		  --expr_parser_lvl;
		  __delete_expr_parser (__pop_expr_parser ());
		  return result_obj;
		} else {
		  ++m_prev_msg -> evaled;
		  ++m -> evaled;
		}
	      } else {
		if (m_prev_msg && 
		    (M_TOK(m_prev_msg) == METHODMSGLABEL) &&
		    __is_instvar_of_method_return (p, i)) {
		  METHOD *__method;
		  OBJECT *m_prev_tok_obj;
		  m_prev_tok_obj = m_prev_msg -> receiver_obj;
		  /*
		   *  Another arglist fixup, if we find that
		   *  the next token after a method is actually
		   *  an instance or class variable.  
		   *  It should be safe to set n_args_declared
		   *  to zero here.
		   */
		  m_prev_msg -> attr_data = 0;
		  m_prev_msg -> attrs |= RT_DATA_IS_NR_ARGS_DECLARED;
		  if ((__method = 
		       __ctalkFindMethodByName (&m_prev_tok_obj,
						M_NAME(m_prev_msg),
						FALSE,
						M_ARGS_DECLARED(m))) 
		      != NULL) {
		    if (*(__method -> returnclass)) {
		      m -> obj =
			create_instancevar_param
			(M_NAME(m),
			 __ctalkGetClass (__method->returnclass));
		      m -> attrs |= 
			(RT_TOK_OBJ_IS_CREATED_PARAM|RT_OBJ_IS_INSTANCE_VAR);
		      __ctalkSetObjectScope 
			(m -> obj, m -> obj -> scope | CREATED_PARAM);
		    }
		  }
		  /* Try to resolve an object that isn't already resolved
		     if it appears later in a compound expression. */
		} else if (!resolve_single_object (m, pname)) {
		  if ((c = get_method_arg_cvars_not_evaled (pname,
							    expr_parser_ptr))
		      != NULL) {
		    if ((aggregate_type_start != -1) && 
			(aggregate_type_end != -1)) {
		      if (!IS_OBJECT (m -> obj)) {
			m -> obj =
			  cvar_object_mark_evaled (c, &cvar_obj_is_created,
						   expr_parser_ptr);
			/*
			 *  TODO - This scope might also be
			 *  used for objects created from
			 *  scratch in cvar_object ().  Also
			 *  used below.
			 */
			if (cvar_obj_is_created)
			  m -> attrs |= RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS;
		      }
		      if (eval_status & EVAL_STATUS_CVARTAB_OBJECT_PTR) {
			/* see the comments in rtinfo.h */
			int aggregate_prev_tok;
			MESSAGE *m_aggregate_prev;
			if ((aggregate_prev_tok = prev_msg
			     (e_messages, aggregate_type_start)) != ERROR) {
			  if (M_TOK(e_messages[aggregate_prev_tok]) ==
			      MULT) {
			    m_aggregate_prev = e_messages[aggregate_prev_tok];
			    m_aggregate_prev -> tokentype = WHITESPACE;
			    m_aggregate_prev -> name[0] = ' ';
			    m_aggregate_prev -> name[1] = '\0';
			    m_aggregate_prev -> attrs = 0;
			    /* if the prev tok is the start of the
			       parser frame, switch the label with the
			       space so the parser doesn't have to deal
			       with leading white space later on. */
			    if (aggregate_prev_tok == p -> msg_frame_start) {
			      MESSAGE *agg_tmp;
			      agg_tmp = e_messages[aggregate_prev_tok];
			      e_messages[aggregate_prev_tok] =
				e_messages[aggregate_type_start];
			      e_messages[aggregate_type_start] =
				agg_tmp;
			      aggregate_type_start = aggregate_prev_tok;
			    }
			  }
			}
			eval_status &= ~EVAL_STATUS_CVARTAB_OBJECT_PTR;
		      }
		      __set_aggregate_obj (e_messages, aggregate_type_start,
					   aggregate_type_end, m -> obj);
		      __resolve_aggregate_method (e_messages,
						  aggregate_type_end);
		      aggregate_type_start = aggregate_type_end = -1;
		    } else {
		      if (!IS_OBJECT (m -> obj))
			m -> obj =
			  cvar_object_mark_evaled (c, &cvar_obj_is_created,
						   expr_parser_ptr);
		      if (c -> attrs & CVAR_ATTR_CVARTAB_ENTRY) {
			unref_vartab_var (&i, c, m -> obj);
			m -> attrs |= RT_TOK_IS_VARTAB_ID;
		      }
		      if (cvar_obj_is_created) {
			m -> attrs |= RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS;
			/* the reference count on a new object should be 0 */
			__objRefCntInc (OBJREF(m -> obj));

			/* We don't need the & before an int type
			   because we've embedded the value in
			   an OBJECT *. */
			if (prev_msg_ptr != ERROR) {
			  if ((M_TOK(m_prev_msg) == AMPERSAND) &&
			      (m_prev_msg -> attrs & RT_TOK_IS_PREFIX_OPERATOR)) {
			    if ((c -> type_attrs & CVAR_TYPE_INT) ||
				(((c -> type_attrs & CVAR_TYPE_CHAR) &&
				  (c -> n_derefs == 0))) ||
				(c -> type_attrs & CVAR_TYPE_LONGLONG) ||
				((c -> type_attrs & CVAR_TYPE_TYPEDEF) &&
				 str_eq (c -> type, "int"))) {
			      m_prev_msg -> name[0] = ' ';
			      m_prev_msg -> tokentype = WHITESPACE;
			      m_prev_msg -> attrs = 0;
			    }
			  }
			}
		      } else if (object_is_cvartab_entry (c) &&
			    (m_prev_msg -> attrs &
			     RT_TOK_IS_PREFIX_OPERATOR)) {
			if ((i == p -> msg_frame_top) &&
			    (prev_msg_ptr == p -> msg_frame_start)) {
			  /* if the entire expression is a 
			   *<cvartab_entry>, that's all we need to do. */
			  result_obj = m -> obj;
			  eval_status = EVAL_STATUS_TERMINAL_TOK;
			  for (j = p -> msg_frame_start;
			       j >= p -> msg_frame_top; j--) {
			    reuse_message(e_message_pop ());
			  }
			  --expr_parser_lvl;
			  __delete_expr_parser (__pop_expr_parser ());
			  return result_obj;
			} else {
			  /* it it's part of an expression,
			     just fill in the object and its leading '*' */
			  m_prev_msg -> value_obj =
			    m -> value_obj = m -> obj;
			  ++m_prev_msg -> evaled;
			  ++m -> evaled;
			}
		      } /* if (cvar_obj_is_created) */
		    }
		  } else {/*if ((c = get_method_arg_cvars (pname))... */
		    /* If we have the syntax of a method, (although
		       the token could still be an instance or
		       class variable) change the token to a method
		       and wait until after evaluating the receiver
		       expression below to find an actual method or
		       whatever. */
		    if (IS_MESSAGE(m_prev_msg)) {
		      if (M_TOK(m_prev_msg) == CLOSEPAREN) {
			/* Label that follows a closing paren. */
			m -> tokentype = METHODMSGLABEL;
			prev_msg_ptr = i;
			continue;
		      }
		    }
		    
		    if (method_result_can_be_receiver (e_messages, i,
						       &prev_mthd_idx)) {
		      m -> tokentype = METHODMSGLABEL;
		      m -> receiver_msg = e_messages[prev_mthd_idx];
		      m -> receiver_obj = e_messages[prev_mthd_idx]
			-> receiver_obj;
		      prev_msg_ptr = i;
		      continue;
		    }

		    if ((prev_msg_ptr != -1) &&
			M_TOK(m_prev_msg) == DEREF) {
		      METHOD *deref_method =
			__ctalkGetInstanceMethodByName
			(rt_defclasses -> p_object_class,
			 "->", FALSE, 1);
		      if ((m -> obj = create_param
			   (pname, deref_method,
			    class_for_deref_arg (e_messages, prev_msg_ptr, i),
			    e_messages, i))
			  == NULL) {
			m -> obj = exception_null_obj (e_messages, i);
			result_obj = m -> obj;
			goto done;
		      }
		    } else {
		      if ((m -> obj = create_param (pname, 
						    method,
						    recv_class,
						    e_messages, i))
			  == NULL) {
			m -> obj = exception_null_obj (e_messages, i);
			result_obj = m -> obj;
			goto done;
		      }
		    }
		    __ctalkSetObjectScope (m -> obj, CREATED_PARAM);
		    /* Here also, if we have an as-yet unresolved
		       message that follows a method message, fix up
		       the previous message's n_args request. 
		       For now, we should just say ANY_ARGS, 
		       in case this label also happens to evaluate
		       to a method.
		    */
		  } /* if ((c = get_method_arg_cvars (pname)) != NULL) */
		}/* if (__is_instvar_of_method_return (e_messages, i,...*/
	      } /*if (m_prev_msg -> obj && IS_OBJECT (m_prev_msg -> obj)&&*/
	    } /* else */
	  }  else { /* if (dont_have_prev_message_object ... */

	    if ((c_i = active_i (m -> obj)) != I_UNDEF) {
	      if ((m -> value_obj = create_param_i (m -> obj, c_i)) == NULL) {
		/* This is easy, as long as it doesn't cause a memory leak. */
		e_messages[p -> msg_frame_start]->obj = NULL;
		e_messages[p -> msg_frame_start]->value_obj = NULL;
		goto done;
	      }
	    } else { /* if ((c_i = active_i (m -> obj)) != I_UNDEF) */
	      /* Check that a local or global object or class name
	       * doesn't shadow an instance or class variable, (or is 
	       * shadowed by one...)
	       */
	      if (m_prev_msg && m_prev_msg -> obj) {
		OBJECT *t;
		if ((t = __ctalkGetInstanceVariable 
		     (m_prev_msg -> obj, m->name, FALSE))
		    != NULL) {
		  m -> obj = t;
		  m -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
		  set_receiver_msg_and_obj (m, m_prev_msg);
		  c_rcvr_idx = 
		    set_c_rcvr_idx (m_prev_msg, 
				    prev_msg_ptr,
				    c_rcvr_idx);
		  e_messages[c_rcvr_idx] -> value_obj = m -> obj;
		  ++m_prev_msg -> evaled;
		  ++m -> evaled;
		  /* Another shortcut. */
		  if (c_rcvr_idx == p -> msg_frame_start &&
		      i == p -> msg_frame_top)
		    goto done;
		} else if ((t = __ctalkGetClassVariable 
			    (m_prev_msg -> obj, m->name, FALSE))
			   != NULL) {
		  m -> obj = t;
		  m -> attrs |= RT_OBJ_IS_CLASS_VAR;
		  set_receiver_msg_and_obj (m, m_prev_msg);
		  c_rcvr_idx = 
		    set_c_rcvr_idx (m_prev_msg, 
				    prev_msg_ptr,
				    c_rcvr_idx);
		  e_messages[c_rcvr_idx] -> value_obj = m -> obj;
		  ++m_prev_msg -> evaled;
		  ++m -> evaled;
		  if (c_rcvr_idx == p -> msg_frame_start &&
		      i == p -> msg_frame_top)
		    goto done;
		}
	      } else if (m_prev_msg &&
			 m_prev_msg -> attrs & RT_TOK_IS_TYPECAST_EXPR) {
		/* if (m_prev_msg && m_prev_msg -> obj) */
		cast_object_to_c_type (e_messages, __typecast_open_paren_idx,
				       __typecast_close_paren_idx,
				       m -> obj);
	      } /* if (m_prev_msg && m_prev_msg -> obj) */
	    } /* if ((c_i = active_i (m -> obj)) != I_UNDEF) */
	  } /* if (((m -> obj = __ctalk_get_object (pname... */
	}
	/*
	 * If the label is a class variable in an expression like,
	 *  <class> <classvariable>...  Then set the attributes
	 *  and receiver here.
	 */
	if (m_prev_msg && IS_MESSAGE(m_prev_msg) &&
	    M_OBJ(m_prev_msg) &&
	    IS_CLASS_OBJECT(m_prev_msg->obj) &&
	    M_OBJ(m) && 
	    (m->obj->__o_class == m_prev_msg->obj)) {
	  if (__ctalkIsClassVariableOf(m_prev_msg->obj,
				       m->obj->__o_name)) {
	    m->receiver_obj = m_prev_msg->obj;
	    m->attrs |= RT_OBJ_IS_CLASS_VAR;
	    m_prev_msg -> value_obj = m -> obj;
	  }
	}
	
	if (m -> obj) {
	  /* Not all prefix ops will appear in expressions - only
	     the ops that the compiler includes; e.g., 
	     the compiler doesn't include a unary ! at the start of 
	     a conditional predicate in the expression.
	  */
	  if (m_prev_msg && (m_prev_msg -> attrs & RT_TOK_IS_PREFIX_OPERATOR)) {
	    if (tok_precedes_assignment_op (p, i)) {
	      __ctalkSetObjectAttr (m -> obj, 
				    m -> obj -> attrs | OBJECT_HAS_PTR_CX);
	      m -> attrs |= RT_TOK_HAS_LVAL_PTR_CX;
	    }
	  }
	}

	/*
	 *  The check for a data type should be sufficient to 
	 *  prevent warnings in casts.
	 */
	if (!m -> obj && (m -> tokentype != METHODMSGLABEL) &&
	    !is_c_data_type (m -> name)) {
	  __ctalkExceptionInternal (m, undefined_label_x, pname,0);
	  goto cleanup;
	}	  
	++m -> evaled;
	prev_msg_ptr = i;
	break;
      case PATTERN:
	/* basically the same as LITERAL, below except we keep
	   the backslashes. */
	if (arg_class_string == NULL)  {
	  if ((arg_class_string = get_class_by_name ("String")) == NULL) {
	  _warning ("%s: Class \"String\" undefined.\n",__argvFileName ());
	    return NULL;
	  } else {
	    if ((m -> obj = 
		 create_param 
		 (m -> name,
		  method, arg_class_string,
		  e_messages, i))
		== NULL) {
	      m -> obj = exception_null_obj (e_messages, i);
	      m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	    } else {
	      __ctalkSetObjectScope (m -> obj, m -> obj -> scope | scope);
	      m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	    }
	  }
	} else {
	    if ((m -> obj = 
		 create_param 
		 (m -> name,
		  method, arg_class_string, e_messages, i))
		== NULL) {
	      m -> obj = exception_null_obj (e_messages, i);
	      m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	      
	    } else {
	      __ctalkSetObjectScope (m -> obj, m -> obj -> scope | scope);
	      m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	    }
	}
	m -> obj -> attrs |= OBJECT_IS_STRING_LITERAL;
	++m -> evaled;
	prev_msg_ptr = i;
	break;
      case LITERAL:
	if (arg_class_string == NULL)  {
	  if ((arg_class_string = get_class_by_name ("String")) == NULL) {
	  _warning ("%s: Class \"String\" undefined.\n",__argvFileName ());
	    return NULL;
	  } else {
	    if ((m -> obj = 
		 create_param 
		 (unescape_str_quotes (m -> name, esc_buf_out), 
		  method, arg_class_string,
		  e_messages, i))
		== NULL) {
	      m -> obj = exception_null_obj (e_messages, i);
	    } else {
	      if (!(scope & VAR_REF_OBJECT))
		__ctalkSetObjectScope (m -> obj, m -> obj -> scope | scope);
	    }
	    m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	  }
	} else {
	    if ((m -> obj = 
		 create_param 
		 (unescape_str_quotes (m -> name, esc_buf_out), 
		  method, arg_class_string, e_messages, i))
		== NULL) {
	      m -> obj = exception_null_obj (e_messages, i);
	    } else {
	      if (!(scope & VAR_REF_OBJECT))
		__ctalkSetObjectScope (m -> obj, m -> obj -> scope | scope);
	    }
	    m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	}
	m -> obj -> attrs |= OBJECT_IS_STRING_LITERAL;
	++m -> evaled;
	prev_msg_ptr = i;
	break;
      case LITERAL_CHAR:
	if (arg_class_character == NULL) {
	  if ((arg_class_character = get_class_by_name ("Character")) == NULL){
	  _warning ("%s: Class \"Character\" undefined.\n",__argvFileName ());
	    return NULL;
	  } else {
	    if (const_created_param (e_messages, i, method, scope,
				     m, arg_class_character) < 0)
	      goto done;
	  }
	} else {
	  if (const_created_param (e_messages, i, method, scope,
				   m, arg_class_character) < 0)
	    goto done;
	}
	m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	m -> obj ->  attrs |= OBJECT_IS_LITERAL_CHAR;
	++m -> evaled;
	prev_msg_ptr = i;
	break;
      case INTEGER:
      case LONG:
	if (arg_class_integer == NULL) {
	  if ((arg_class_integer = get_class_by_name ("Integer")) == NULL) {
	  _warning ("%s: Class \"Integer\" undefined.\n",__argvFileName ());
	    return NULL;
	  } else {
	    if (typecast_or_constant_expr (e_messages, i, 
					   __typecast_open_paren_idx,
					   __typecast_close_paren_idx,
					   p,  method, scope,
					   arg_class_integer,
					   typecast_is_ptr) == ERROR)
	    goto done;
	  }
	} else {
	  if (typecast_or_constant_expr (e_messages, i, 
					 __typecast_open_paren_idx,
					 __typecast_close_paren_idx,
					 p, method, scope, arg_class_integer,
					 typecast_is_ptr) == ERROR)
	    goto done;
	}
	m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	m -> obj ->  attrs |= OBJECT_IS_CONSTANT_INT_OR_LONG;
	++m -> evaled;
	prev_msg_ptr = i;
	break;
      case LONGLONG:
	if (arg_class_longinteger == NULL) {
	  if ((arg_class_longinteger = get_class_by_name ("LongInteger"))
	      == NULL) {
	  _warning ("%s: Class \"LongInteger\" undefined.\n",
		      __argvFileName ());
	    return NULL;
	  } else {
	    if (typecast_or_constant_expr (e_messages, i, 
					   __typecast_open_paren_idx,
					   __typecast_close_paren_idx,
					   p, method, scope, 
					   arg_class_longinteger,
					   typecast_is_ptr) == ERROR)
	      goto done;
	  }
	} else {
	  if (typecast_or_constant_expr (e_messages, i, 
					 __typecast_open_paren_idx,
					 __typecast_close_paren_idx,
					 p, method, scope, 
					 arg_class_longinteger,
					 typecast_is_ptr) == ERROR)
	    goto done;
	}
	m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	m -> obj ->  attrs |= OBJECT_IS_CONSTANT_LONGLONG;
	++m -> evaled;
	prev_msg_ptr = i;
	break;
      case FLOAT:
	if (arg_class_float == NULL) {
	  if ((arg_class_float = get_class_by_name ("Float")) == NULL) {
	    _warning ("%s: Class \"Float\" undefined.\n",__argvFileName ());
	    return NULL;
	  } else {
	    if (typecast_or_constant_expr (e_messages, i, 
					   __typecast_open_paren_idx,
					   __typecast_close_paren_idx,
					   p, method, scope, 
					   arg_class_float,
					   typecast_is_ptr) == ERROR)
	    goto done;
	  }
      } else {
	  if (typecast_or_constant_expr (e_messages, i, 
					 __typecast_open_paren_idx,
					 __typecast_close_paren_idx,
					 p, method, scope, 
					 arg_class_float,
					 typecast_is_ptr) == ERROR)
	    goto done;
	}
	m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	m -> obj -> attrs |= OBJECT_IS_CONSTANT_FLOAT;
	++m -> evaled;
	prev_msg_ptr = i;
	break;
      case NEWLINE:
      case WHITESPACE:
       	++m -> evaled;
       	break;
      case SEMICOLON:
      case CHAR:
      case PREPROCESS_EVALED:
      case RESULT:
	/*
	 *  Characters that are not operators can be a character type.
	 *  We can fake our way through here if Character class isn't
	 *  loaded.
	 */
	m -> obj = create_object_init_internal
	  (m -> name, rt_defclasses -> p_character_class,
	   CREATED_PARAM, m -> name);
	++m -> evaled;
	prev_msg_ptr = i;
	break;
      case OPENPAREN:
	if (__rt_is_typecast_expr (p, i)) {
	  if ((__typecast_close_paren_idx = 
	       expr_match_paren (p, i)) == ERROR) {
	    __ctalkExceptionInternal (NULL, mismatched_paren_x,
				      M_NAME 
				      (e_messages
				       [next_msg (e_messages, i)]),0);
	    prev_msg_ptr = i;
	    continue;
	  }
	  __typecast_open_paren_idx = i;
	  if (expr_typecast_is_pointer (p, i)) {
	    typecast_ptr_expr_b (p, i, __typecast_close_paren_idx);
	    i = __typecast_close_paren_idx;
	    typecast_is_ptr = true;
	  } else {
	    typecast_value_expr (e_messages, i, __typecast_close_paren_idx);
	    i = __typecast_close_paren_idx;
	    typecast_is_ptr = false;
	  }
	}
	prev_msg_ptr = i;
	break;
      case ASTERISK:
      case EXCLAM:
      case INCREMENT:
      case DECREMENT:
      case BIT_COMP:
      case AMPERSAND:
      case MINUS:
	if (i == p -> msg_frame_start) {
	  e_messages[i] -> attrs |= RT_TOK_IS_PREFIX_OPERATOR;
	} else if (op_is_prefix_op (p, i)) {
	  e_messages[i] -> attrs |= RT_TOK_IS_PREFIX_OPERATOR;
	}
	prev_msg_ptr = i;
	break;
      case SIZEOF:
	e_messages[i] -> attrs |= RT_TOK_IS_PREFIX_OPERATOR;
	prev_msg_ptr = i;
	break;
      default:
	prev_msg_ptr = i;
	break;
      }
  }

  /* 
     This is still needed during initialization ... the objects that
     eval_label_token would return may not be present yet.
  */

  if (p -> msg_frame_start == p -> msg_frame_top) {
    m = e_message_pop ();
    result_obj = M_VALUE_OBJ(m);
    if (is_arg_expr) {
      if (result_obj -> attrs & OBJECT_IS_STRING_LITERAL ||
	  result_obj -> attrs & OBJECT_IS_LITERAL_CHAR ||
	  result_obj -> attrs & OBJECT_IS_CONSTANT_INT_OR_LONG ||
	  result_obj -> attrs & OBJECT_IS_CONSTANT_LONGLONG ||
	  result_obj -> attrs & OBJECT_IS_CONSTANT_FLOAT) {
	is_constant_arg = true;
      }
    }
    reuse_message (m);
    --expr_parser_lvl;
    p = __pop_expr_parser ();
    __delete_expr_parser (p);
    return result_obj;
  }
  
 re_eval:  for (i = p -> msg_frame_start; i >= p -> msg_frame_top; i--) {

    m = e_messages[i];
    switch (m -> tokentype)
      {
      case SEMICOLON:
	++m -> evaled;
	goto done;
	break;
      case CHAR:
      case LITERAL_CHAR:
      case LITERAL:
      case PATTERN:
      case INTEGER:
      case LONG:
      case LONGLONG:
      case DOUBLE:
      case PREPROCESS_EVALED:
      case RESULT:
	break;
      case WHITESPACE:
      case NEWLINE:
	break;
      case METHODMSGLABEL:
	if (m -> evaled <= 1) {
	  prev_rcvr_msg_ptr = rcvr_scan_back_b (i, p);
	  if (m -> attrs & RT_DATA_IS_CACHED_METHOD) {
	    m_prev_msg = m -> receiver_msg;
	    if (m -> receiver_obj == M_VALUE_OBJ(m_prev_msg)) {
	      /*
		m -> receiver_obj was set in the first pass.
		We only use the cached method if there is no
		value_obj, i.e.; a recalculated receiver object.
		(That is, the actual receiver is the result
		of an expression whose tokens appear to the
		left of the method token and which has just been 
		evaluated in this pass.)

		Also the receiver scan in the first pass is very
		simple, so we don't use a cached method	if there
		are tokens between the receiver and the method, like
		a parenthesized expression (yet).
	      */
	      e_method = (METHOD *)m -> attr_method;
	      m_prev_value_obj = m -> receiver_obj;
	      goto have_cached_method;
	    }
	  }
	  m_prev_msg = e_messages[prev_rcvr_msg_ptr];
	  if (m_prev_msg->obj == NULL)
	    m_prev_msg->obj = m_prev_msg->value_obj;
	  if (m_prev_msg && IS_OBJECT (m_prev_msg -> obj)) {
	    e_class = e_result = e_result_dup = n_result = NULL;
	    last_arg_ptr = -1; 
	    old_arg_frame_top = -1;
	    is_arg_value = False;

	    if (m -> attrs & RT_TOK_IS_POSTFIX_MODIFIED) {
	      m_prev_value_obj = m_prev_msg -> obj;
	    } else if (m_prev_msg -> attrs & RT_TOK_HAS_LVAL_PTR_CX) {
	      m_prev_value_obj = m_prev_msg -> obj;
	    } else {
	      m_prev_value_obj = M_VALUE_OBJ(m_prev_msg);
	    }

	    if (M_OBJ_IS_VAR(m_prev_msg)) {
	      if (m_prev_msg -> attrs & RT_TOK_HAS_LVAL_PTR_CX) {
		m_prev_value_value_obj = m_prev_msg -> obj;
	      } else {
		if (IS_VALUE_INSTANCE_VAR(M_VALUE_OBJ(m_prev_msg))) {
		  /* should be easier to follow? */
		  m_prev_value_value_obj = 
		    ((IS_VALUE_INSTANCE_VAR(M_VALUE_OBJ(m_prev_msg))) ?
		     M_VALUE_OBJ(m_prev_msg) -> __o_class :
		     ((M_VALUE_OBJ(m_prev_msg) -> instancevars) ? 
		      ((M_VALUE_OBJ(m_prev_msg) -> instancevars 
			-> attrs & OBJECT_IS_VALUE_VAR) ?
		       M_VALUE_OBJ(m_prev_msg) -> instancevars -> __o_class :
		       M_VALUE_OBJ(m_prev_msg) -> __o_class) :
		      M_VALUE_OBJ(m_prev_msg) -> __o_class));
		} else if (m_prev_msg -> attrs & RT_OBJ_IS_INSTANCE_VAR) {
		  /* to do... the OBJECT_IS_VALUE_VAR attribute doesn't
		     always get set in instance vars' value objects,
		     where the clause above would match it */
		  if (IS_OBJECT(m_prev_msg -> obj -> __o_p_obj) &&
		      (m_prev_msg -> obj -> __o_p_obj -> instancevars ==
		       m_prev_value_obj)) {
		    m_prev_value_value_obj = m_prev_msg -> obj;
		  } else {
		    m_prev_value_value_obj = m_prev_msg -> obj -> instancevars;
		  }
		} else if (m_prev_msg -> attrs & RT_VALUE_OBJ_IS_INSTANCE_VAR) {
		  m_prev_value_value_obj = m_prev_msg -> value_obj
		    -> instancevars;
		} else if (m_prev_msg -> attrs & RT_OBJ_IS_CLASS_VAR) {
		  m_prev_value_value_obj = m_prev_msg -> obj -> instancevars;
		} else if (m_prev_msg -> attrs & RT_VALUE_OBJ_IS_CLASS_VAR) {
		  m_prev_value_value_obj = m_prev_msg -> value_obj
		    -> instancevars;
		} else {
		  m_prev_value_value_obj = M_VALUE_OBJ(m_prev_msg)
		    -> instancevars;
		}
	      }
	    } else {
	      if (m_prev_value_obj -> attrs & OBJECT_HAS_PTR_CX) {
		m_prev_value_value_obj = m_prev_value_obj;
	      } else {
		m_prev_value_value_obj = M_VALUE_OBJ(m_prev_msg);
	      }
	    }

	    m_prev_msg ->  attrs |= RT_TOK_VALUE_OBJ_IS_RECEIVER;

	    /* 
	       We set the token to be a possible method, above.
	       Now check here for the instance or class variable
	       of an evaluated receiver expression. (First make
	       sure the token is a C label.)
	    */
	    if (IS_C_LABEL(M_NAME(m))) {
	      
	      if ((e_result = 
		   __ctalkGetInstanceVariable (m_prev_value_obj, 
					       M_NAME(m),
					       FALSE)) != NULL) {
		m -> tokentype = LABEL;
		if (!m -> receiver_obj)
		  m -> receiver_obj = e_result;
		if (!m -> receiver_msg)
		  m -> receiver_msg = e_messages[prev_rcvr_msg_ptr];
		m -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
		/*
		 *  If the next message is a method, then 
		 *  create a complete object - hopefully 
		 *  only ARG_VAR scope is needed, with
		 *  no references from here.
		 */
		if ((next_tok_idx = 
		     next_msg (e_messages, i)) != ERROR) {
		  if (__ctalk_isMethod_2 (M_NAME(e_messages[next_tok_idx]),
					  e_messages, next_tok_idx,
					  p -> msg_frame_start)) {
		    e_result = 
		      __ctalkCreateObjectInit 
		      (e_result -> __o_name,
		       e_result -> CLASSNAME,
		       _SUPERCLASSNAME(e_result),
		       ARG_VAR, e_result -> __o_value);
		  }
		}
		last_arg_ptr = i;
		goto set_method_expr_value_label;
	      } else if ((e_result = 
			  __ctalkGetClassVariable (m_prev_value_obj, 
						   M_NAME(m),
						   FALSE)) != NULL) {
		m -> tokentype = LABEL;
		last_arg_ptr = i;
		m -> attrs |= RT_OBJ_IS_CLASS_VAR;
		goto set_method_expr_value_label;
	      }

	    } /* if (IS_C_LABEL (M_NAME(m))) ...  */

	    if (M_VALUE_OBJ_IS_SUPERCLASS(m_prev_msg)) {
	      OBJECT *__r;
	      if ((__r = __ctalkRtGetMethod() -> rcvr_class_obj) != NULL)
		m_prev_super_obj = __r -> __o_superclass;
	    }

	    if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
	      if ((e_method = 
		   __ctalkFindPrefixMethodByName 
		   ((M_VALUE_OBJ_IS_SUPERCLASS(m_prev_msg) ? 
		     &m_prev_super_obj : &m_prev_value_value_obj),
		    m -> name, FALSE)) == NULL) {
		if (m_prev_msg->obj && 
		    HAS_INSTANCEVAR_CLASS(m_prev_msg->obj) &&
		    m_prev_msg->obj->instancevars) {
		  m_prev_value_value_obj = m_prev_msg->obj->instancevars;
		  if ((e_method = 
		       __ctalkFindPrefixMethodByName 
		       (&m_prev_value_value_obj,
			m->name, FALSE)) == NULL) {
		    __ctalkExceptionInternal 
		      (m, undefined_method_x, 
		       undefined_method_text 
		       (m_prev_msg->obj->__o_name,
			m_prev_msg->obj->CLASSNAME,
			m -> name),0);
		    goto done;
		  }
		}
	      }
	    } else { /* if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) */

	      __cvar_postfix_method_retry = 0;

	    find_method_again:
	      if ((e_method = 
		   __ctalkFindMethodByName 
		   ((M_VALUE_OBJ_IS_SUPERCLASS(m_prev_msg) ? 
		     &m_prev_super_obj : &m_prev_value_value_obj),
		    m -> name, FALSE, 
		    M_ARGS_DECLARED(m))) == NULL) {

		if (m_prev_msg->obj && 
		    HAS_INSTANCEVAR_CLASS(m_prev_msg->obj) &&
		    m_prev_msg->obj->instancevars) {
		  m_prev_value_value_obj = m_prev_msg->obj->instancevars;
		  if ((e_method = 
		       __ctalkFindMethodByName (&m_prev_value_value_obj,
						m->name, FALSE,
						M_ARGS_DECLARED(m))) 
		      == NULL) {
		    __ctalkExceptionInternal 
		      (m, undefined_method_x, 
		       undefined_method_text 
		       (m_prev_msg->obj->__o_name,
			m_prev_msg->obj->CLASSNAME,
			m -> name),0);
		    /* Don't try again. */
		    if (M_ARGS_DECLARED(m) == ANY_ARGS) {
		      _warning ("Warning: Method, \"%s,\" receiver, \"%s:\"\n",
				m -> name, m_prev_msg -> name);
		      _warning ("Warning:\tNot found.\n");
		      goto done;
		    }
		  }
		} else {
		  /* Check that the token is not actually a C operator. */
		  /* TODO - This should be in a separate fn - it's
		     probably incomplete due to lack of test cases,
		     *or* maybe this should just be another fixup
		     for a postfix operator, whenever the op is 
		     included in the expression. */
		  if (!e_method)  {
		    if (m -> attrs & RT_TOK_IS_POSTFIX_MODIFIED) {
		      
		      if ((m_p_idx = method_is_postfix_of_cvar
			   (e_messages, i)) != 0) {
			
			m_prev_msg = e_messages[m_p_idx];
			/* The obj member is the CVAR translation
			   to an object - use that for the value
			   object also in this case - it's what 
			   gets pushed onto the receiver stack 
			   before the method call - if we're
			   not careful about using the CVAR translation,
			   as the receiver, then things get really ugly 
			   in the next few method calls. */
			m_prev_value_value_obj = e_messages[m_p_idx]->obj;
			m_prev_value_obj = e_messages[m_p_idx]->obj;
			m -> attr_data = ANY_ARGS;

			/* Causes method_is_postfix_cvar () to
			   return an error on the next iteration
			   if we still haven't found a method
			   by then. */
			++__cvar_postfix_method_retry;

			goto find_method_again;
		      }
		    }

		    /* Otherwise, don't try again here, either. */
		    if (!IS_METHOD(e_method)) {

		      /* Don't positively say that a method can't
			 be found until we've evaluated all of
			 the tokens in the expression. */
		      if (have_unevaled (e_messages, i))
			continue;

		      prev_tok_idx = prev_msg (e_messages, i);
		      if (M_VALUE_OBJ(e_messages[prev_tok_idx])) {
			if (e_messages[prev_tok_idx] -> attrs &
			    RT_OBJ_IS_INSTANCE_VAR ||
			    e_messages[prev_tok_idx] -> attrs &
			    RT_OBJ_IS_CLASS_VAR) {
			  _warning ("Warning: Method, \"%s,\" not found. "
				    "(Previous object, \"%s,\" (class "
				    "%s).)\n", M_NAME(m),
				    M_VALUE_OBJ(e_messages[prev_tok_idx]) 
				    -> __o_name,
				    (M_VALUE_OBJ(e_messages[prev_tok_idx]) ->
				     instancevars ?
				     M_VALUE_OBJ(e_messages[prev_tok_idx]) ->
				     instancevars -> CLASSNAME :
				     M_VALUE_OBJ(e_messages[prev_tok_idx]) ->
				     CLASSNAME));
			} else {
			  _warning ("Warning: Method, \"%s,\" not found. "
				    "(Previous object, \"%s,\" (class "
				    "%s).)\n", M_NAME(m),
				    M_VALUE_OBJ(e_messages[prev_tok_idx]) 
				    -> __o_name,
				    M_VALUE_OBJ(e_messages[prev_tok_idx]) 
				    -> CLASSNAME);
			}
			goto done;
		      } else {
			_warning ("Warning: Method, \"%s,\" not found.\n",
				  M_NAME(m));
		      }
		    } else {
		      method_wrong_number_of_arguments_prefix_error
			(m, m_prev_msg,
			 expr_parsers[expr_parser_ptr] -> expr_str);
		      goto done;
		    }
		  }
		}
	      }
	    } /* if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) */
	      /*
	       *  Again, we only use the declared arguments for
	       *  the first method in the expression.
	       */
	    if (!e_method) {
	      method_wrong_number_of_arguments_warning 
		(m, m_prev_msg,
		 expr_parsers[expr_parser_ptr] -> expr_str);
	      m -> attrs |= RT_DATA_IS_NR_ARGS_DECLARED;
	      m -> attr_data = ANY_ARGS;
	      goto find_method_again;
	    }

	    if (M_ARGS_DECLARED(m) != ANY_ARGS) {
	      m -> attrs |= RT_DATA_IS_NR_ARGS_DECLARED;
	      m -> attr_data = ANY_ARGS;
	    }

	  have_cached_method:
	    if (!e_class) 
	      e_class = m_prev_msg->obj->__o_class;
	    if (e_method) {
	      m -> receiver_obj = M_VALUE_OBJ (m_prev_msg);
	      if ((e_method -> n_params == 0) && !e_method -> varargs) {
		last_arg_ptr = i;
	      } else {
		last_arg_ptr = __rt_method_args
		  (e_method, e_messages,
		   p -> msg_frame_start, e_message_ptr, i,
		   is_arg_expr);
	      }
	      if (last_arg_ptr == ERROR)
		goto done;
	      old_arg_frame_top = e_method -> arg_frame_top;
	      e_method -> arg_frame_top = __ctalk_arg_ptr;
	      if (!IS_CLASS_OBJECT(m_prev_msg->obj)) {
		if ((e_class = m_prev_msg -> obj -> __o_class) == NULL) 
		  undefined_receiver_class_warning_a 
		    (M_NAME(m_prev_msg), m_prev_msg -> obj -> __o_name);
	      }
	      p -> e_methods[p -> e_method_ptr++] = e_method;
	      __save_rt_info (m_prev_value_obj, e_class, NULL, 
			      e_method, e_method -> cfunc, False);
	      
	      if (M_VALUE_OBJ_IS_SUPERCLASS(m_prev_msg)) {
		__ctalk_receiver_push_ref (m_prev_msg->receiver_obj);
	      } else {
		__ctalk_receiver_push_ref (m_prev_value_obj);
	      }
	      e_result = __ctalkCallMethodFn (e_method);
	      /*
	       *  NULL result gets filled in below.
	       *  TODO - Make sure that IS_OBJECT 
	       *  does not segfault no matter what
	       *  segment the pointer is in.  Issue
	       *  with older systems/compilers.
	       */
	      if (!IS_OBJECT(e_result)) 
		e_result = NULL;

	      if (!expr_is_assign_arg ()) {
		if ((c_i = active_i (e_result)) != I_UNDEF) {
		  if (need_postfix_fetch_update ())
		    postfixed_object = e_result;
		  e_result = 
		    create_param_i (e_result, c_i);
		  clear_postfix_fetch_update ();
		} else if (m -> attrs & RT_TOK_IS_POSTFIX_MODIFIED) {
		  /* writeback for receivers that are aliases of
		     C variables. */
		  if (M_OBJ(m_prev_msg) -> attrs &
		      OBJECT_REF_IS_CVAR_PTR_TGT) {
		    write_val_CVAR (M_OBJ(m_prev_msg), e_method);
		  } else {
		    /* otherwise check thru a complex receiver
		       expression */
		    for (i_2 = i + 1; i_2 <= prev_rcvr_msg_ptr; ++i_2) {
		      if (e_messages[i_2] -> attrs &
			  RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) {
			write_val_CVAR (M_OBJ(e_messages[i_2]), e_method);
		      }
		    }
		  }
		}
	      }

	      __last_eval_result = e_result;

	      if ((i == p -> msg_frame_top) && 
		  IS_OBJECT(e_result) &&
		  (e_result -> scope & VAR_REF_OBJECT))
		eval_status |= 
		  (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_VAR_REF);

	      expr_parsers[expr_parser_ptr] -> e_result = e_result;

	      rcvr_tmp = __ctalk_receiver_pop_deref ();
	      __restore_rt_info ();
	      e_method -> arg_frame_top = old_arg_frame_top;

	      args_in_expr = ((e_method -> varargs) ? 
			      e_method -> n_args : 
			      e_method -> n_params);
	      
	      for (j = 0; j < args_in_expr; j++) {
		
		OBJECT *__arg_object, *__r;
		int i_1;

		__arg_object = __ctalk_arg_pop ();
		__ctalk_arg_push (__arg_object);
		__r = reffed_arg_obj (__arg_object);
		if (!__ctalk_arg_cleanup (e_result)) {
		  for (i_1 = p -> msg_frame_start;
		       i_1 >= p -> msg_frame_top; i_1--) {
		    if (e_messages[i_1] -> obj == __arg_object) {
		      if (e_messages[i_1] -> attrs & 
			  RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) {
			__ctalkDeleteObject (__arg_object);
		      } else {
			if (e_messages[i_1] -> attrs &
			    RT_TOK_OBJ_IS_CREATED_PARAM) {
			  /* If object was already deleted
			     in __ctalk_arg_cleanup (), then remove
			     the message attribute. */
			  if (!IS_OBJECT(e_messages[i_1] -> obj)) {
			    e_messages[i_1] -> obj = NULL;
			    e_messages[i_1] -> attrs &= 
			      ~RT_TOK_OBJ_IS_CREATED_PARAM;
			  }
			} else {
			  if (__r)
			    delete_created_object_ref 
			      (e_messages, i_1,
			       __arg_object -> scope & CVAR_VAR_ALIAS_COPY,
			       __r);
			}
		      }
		      e_messages[i_1] -> obj = NULL;
		    }
		    if (e_messages[i_1] -> value_obj == __arg_object) {
		      if (e_messages[i_1] -> attrs &
			  RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) {
			__ctalkDeleteObject (__arg_object);
		      } else {
			if (__r)
			  delete_created_object_ref 
			    (e_messages, i_1,
			     __arg_object -> scope & CVAR_VAR_ALIAS_COPY,
			     __r);
		      }
		      e_messages[i_1] -> value_obj = NULL;
		    }
		  }
		} else {
		  for (i_1 = p -> msg_frame_start; i_1 >= p -> msg_frame_top;
		       i_1--) {
		    if (e_messages[i_1] -> value_obj == __arg_object) {
		      if (cleanup_subexpr_created_arg (p, __arg_object,
						       e_result))
			continue;
		    }
		    if (e_messages[i_1] -> obj == __arg_object) {
		      cleanup_created_param_arg (e_messages, i_1,
						 __arg_object);
		      if (!(e_messages[i_1] -> attrs &
			    RT_TOK_OBJ_IS_ARG_W_FOLLOWING_EXPR) &&
			  /* Instance and class vars are still
			     checked by expr_result_object,
			     especially if it's the terminal
			     token of the expression. */
			  !(e_messages[i_1] -> attrs &
			    RT_OBJ_IS_INSTANCE_VAR) &&
			  !(e_messages[i_1] -> attrs & 
			    RT_OBJ_IS_CLASS_VAR)) {
			e_messages[i_1] -> obj = NULL;
		      }
		    }
		    if (e_messages[i_1] -> value_obj == __arg_object) {
		      /* See the comments in pattypes.c */
		      int _expr_end;
		      if ((_expr_end =
			   prefix_value_obj_is_term_result (p, i_1)) != 0) {
			if (IS_OBJECT(e_messages[i_1] -> obj)) {
			  e_messages[i_1] -> value_obj = e_result;
			} else {
			  e_messages[i_1] -> obj = e_result;
			}
			e_messages[i_1] -> value_obj = NULL;
			e_messages[_expr_end] -> value_obj = NULL;
			i_1 = _expr_end;
		      } else {
			e_messages[i_1] -> value_obj = NULL;
		      }
		    }
		  }
		}

		if (e_method->args[e_method -> n_args - 1] && 
		    IS_ARG(e_method->args[e_method -> n_args - 1])) {

		  OBJECT *__arg_obj_tmp = 
		    e_method -> args[e_method -> n_args - 1] -> obj;

		  /* TO DO
		   * 
		   * This is one place we need to straighten
		   * out the object references.
		   * A ref count decrease here should be 
		   * correct for any sort of argument.
		   * 
		   * All args that get processed by
		   * __rt_method_args () use 
		   * __add_arg_object_entry_frame ()
		   * to add the arg and increase the object's
		   * reference count.  
		   *
		   * All __ctalk_arg_push_ref () calls in 
		   * rt_args.c are paired with an 
		   * __add_arg_entry_frame () call.  Should check
		   * elsewhere, too.
		   *
		   * This issue only shows up in the formatObject*
		   * tests so far - missing object after it's used
		   * as an argument, and a segfault while trying
		   * to clean up trashed method user objects when
		   * exiting.
		   *
		   * If we don't decrease the ref count for CVAR
		   * copies here, we get a memory leak in the 
		   * margexpr* tests.
		   *
		   * Also find out if maybe there are just too
		   * many ref counts for CVAR copies.
		   *
		   * Doesn't seem to be - try checking whether there
		   * are two nrefs for each remaining arg stack
		   * entry.  Arrrgh.
		   */
		  if (IS_OBJECT (__arg_obj_tmp) &&
		      (__arg_obj_tmp -> scope & CVAR_VAR_ALIAS_COPY) &&
		      (cvar_alias_arg_min_refcount_check (__arg_obj_tmp)
		       > 0)) 
		    (void)__objRefCntDec (OBJREF (__arg_obj_tmp));

		  if (IS_ARG (e_method->args[e_method -> n_args - 1]))
		    __xfree 
		      (MEMADDR(e_method -> args[e_method -> n_args - 1]));
		  e_method -> args[e_method -> n_args - 1] = NULL;
		}
		--e_method -> n_args;
	      }
	      /*
	       *  If the method returns NULL, create an
	       *  object that evaluates to False.
	       */

	      if (!e_result)
		e_result = null_result_obj (e_method, scope);

	      /*
	       *  If a method returns an instance variable, 
	       *  then copy it to a new instance variable 
	       *  object.  Then create a new object, and
	       *  ad the instance variable.
	       *
	       *  We should not simply add a reference to the
	       *  existing instance variable.  We would need 
	       *  to make certain that the object is de-reffed 
	       *  in all cases of return from methods and 
	       *  expressions.
	       *
	       *  This might not be correct in all cases, but 
	       *  the result here should be consistent with
	       *  other result objects.
	       */
	      /*
	       *  Try to find an actual object for the method
	       *  return value.
	       *
	       *  TO DO - This should be done in __ctalk_method
	       *  also.
	       */
	      if (!e_result -> instancevars && (is_arg_value == False)) {
		if (e_result == m_prev_value_obj -> instancevars) {
		  if (!method_is_instancevar_deref (p, i)) {
		    /* unless the deref argument is "instancevars", it's
		       probably "self" */
		    e_result = m_prev_value_obj;
		  }
		} else {
		  if (e_result -> __o_p_obj) {
		    if (!__prev_tok_is_instancevar_ref (p, i) && 
			!terminal_member_var_ref ()) {
		      /* Then we returned an instance variable.  If the
			 expression doesn't call for an instance variable,
			 return the complete object. */
		      e_result = e_result -> __o_p_obj;
		    }
		  } else {
		    if (IS_OBJECT (e_result)) {
		      if (__ctalkIsInstanceVariableOf 
			  (e_result -> __o_class, 
			   e_result -> __o_name)) {
			__ctalkCopyObject (OBJREF(e_result), 
					   OBJREF(e_result_dup));
			n_result = 
			  __ctalkCreateObjectInit ("result", 
						   e_result -> CLASSNAME,
						   _SUPERCLASSNAME(e_result),
						   scope,
						   e_result -> __o_value);
			__ctalkAddInstanceVariable (n_result, 
						    e_result_dup -> __o_name,
						    e_result_dup);
			/*
			 *  __ctalkAddInstanceVariable copies a value
			 *  variable if one does not exist, and simply 
			 *  adds other variables. So we do not
			 *  need e_result_dup in that case.
			 *  That case should be simplified.
			 *
			 *  Also the expression should *not*
			 *  use IS_VALUE_INSTANCE_VAR (), in case 
			 *  the result is a value instance variable
			 *  copy.
			 */
			if (!strcmp (e_result_dup -> __o_name, "value"))
			  __ctalkDeleteObject (e_result_dup);
		      
			(void)__objRefCntSet (OBJREF(n_result), e_result -> nrefs);
			__ctalkDeleteObjectInternal (e_result);
			e_result = n_result;
		      }
		    } else {
		      /*
		       *  TODO - this should be an exception.
		       */
		      e_result = null_result_obj (e_method, scope);
		    }
		  }
		}
	      }
	      
	      /*
	       *  c_rcvr_idx is set above for an argument that
	       *  contains an instance or class variable message
	       *  expression. Check here for constant operands
	       *  also.
	       */
	    set_method_expr_value_label:
	      fixup_forward_receiver_obj (p, i, e_result);
	      if (last_arg_ptr != ERROR) {
		set_method_expr_value (e_messages, i, prev_rcvr_msg_ptr,
				       last_arg_ptr, precedence,
				       e_result);
		if (IS_OBJECT(e_messages[prev_rcvr_msg_ptr] -> value_obj)
		    && (e_messages[prev_rcvr_msg_ptr] -> value_obj -> attrs 
			& OBJECT_IS_NULL_RESULT_OBJECT)) {
		  /* This is checked in clean_up_message_objects (), 
		     below. */
		  e_messages[prev_rcvr_msg_ptr] -> attrs |= 
		    RT_TOK_VALUE_OBJ_IS_NULL_RESULT;
		}
	      }
	      /*
	       *  Might be redundant.
	       */
	      if ((next_msg_ptr = 
		   next_msg (e_messages, last_arg_ptr)) != ERROR) {
		/*
		 *   Add the value object at the following paren
		 *   iff 
		 *   1. The token following the paren is an operator.
		 *   This is so the argument expression works as 
		 *   an operand when != is evaled.
		 *   
		 *     e.g.  (CFunction cScanf ("%c", c) != EOF)
		 *                                     ^
		 *   2. The paren is the end of the stack.
		 *
		 *   Should only be necessary in cases like these, at 
		 *   least so far.
		 */
		if (M_TOK(e_messages[next_msg_ptr]) == CLOSEPAREN) {
		  if ((expr_lookahead_ptr = 
		       next_msg (e_messages, next_msg_ptr)) != ERROR) {
		    if (IS_C_OP_TOKEN_NOEVAL
			(M_TOK(e_messages[expr_lookahead_ptr])))
		      e_messages[next_msg_ptr] -> value_obj = e_result;
		  } else {
		    if (next_msg_ptr == p -> msg_frame_top) {
		      e_messages[next_msg_ptr] -> value_obj = e_result;
		    }
		  }
		}
	      } /* if ((next_msg_ptr = ... */
	    }
	  }
	} else { /* 	if (m -> evaled <= 1) { */
	  /*
	   *  Check for a fixup here.  A high eval count means
	   *  that: 
	   *  a) the next message(s) were evaled as arguments 
	   *  during a greedy arglist match, but the value
	   *  objects were not reset with the result of 
	   *  the expression.  Should only occur for messages 
	   *  that follow methods that return "Any" class.
	   */
 	  if (m -> value_obj && 
 	      (m -> value_obj -> scope & CREATED_PARAM)) {
	    if ((prev_tok_idx = prev_msg (e_messages, i)) != ERROR) {
	      if (M_TOK(e_messages[prev_tok_idx]) == METHODMSGLABEL) {
		if ((var = 
		     __ctalkGetInstanceVariable
		     (M_VALUE_OBJ(e_messages[prev_tok_idx]), 
		      M_NAME(e_messages[i]), FALSE)) != NULL) {
		  for (i_1 = prev_tok_idx; 
		       e_messages[i_1] != 
			 e_messages[prev_tok_idx] -> receiver_msg; 
		       i_1++) {
		    e_messages[i_1] -> value_obj = var;
		  }
		  for (i_2 = i - 1; i_2 >= p -> msg_frame_top; i_2--) {
		    if (e_messages[i_2] -> obj == 
			e_messages[i] -> obj)
		      e_messages[i_2] -> obj = NULL;
		    if (e_messages[i_2] -> value_obj == 
			e_messages[i] -> obj)
		      e_messages[i_2] -> value_obj = NULL;
		    if (e_messages[i_2] -> obj == 
			e_messages[i] -> value_obj)
		      e_messages[i_2] -> obj = NULL;
		    if (e_messages[i_2] -> value_obj == 
			e_messages[i] -> value_obj)
		      e_messages[i_2] -> value_obj = NULL;
		    if (e_messages[i_2] -> receiver_obj == 
			e_messages[i] -> obj)
		      e_messages[i_2] -> receiver_obj = var;
		    if (e_messages[i_2] -> receiver_obj == 
			e_messages[i] -> value_obj)
		      e_messages[i_2] -> receiver_obj = var;
		  }
		  if (IS_OBJECT(e_messages[i] -> obj))
		    __ctalkDeleteObjectInternal(e_messages[i]->obj);
		  if (IS_OBJECT(e_messages[i] -> value_obj) && 
		      (e_messages[i] -> obj != e_messages[i] -> value_obj))
		    __ctalkDeleteObjectInternal(e_messages[i]->value_obj);
		  e_messages[i] -> value_obj = 
		    e_messages[i] -> obj = var;
		  e_messages[i] -> receiver_msg = e_messages[prev_tok_idx];
		  e_messages[i] -> receiver_obj = 
		    e_messages[i_1] -> value_obj;
		}
	      }
	    }
	  }
	} 
	++m -> evaled;
	break;
      case LABEL:
	if (m -> attrs & RT_CVAR_AGGREGATE_TOK) {
	  if ((aggregate_type_end =
	       is_aggregate_type (e_messages, i, namebuf)) > 0) {
	    for (j = i; j >= aggregate_type_end; j--)
	      ++e_messages[j] -> evaled;
	    i = aggregate_type_end;
	    continue;
	  }
	} else if (m -> attrs & RT_TOK_IS_SELF_KEYWORD) {
	  if (IS_OBJECT(e_messages[i] -> obj)) {
	    ++e_messages[i] -> evaled;
	    continue;
	  }
	}

	/*
	 *  Check whether we've evaluated a receiver from the previous 
	 *  subexpressions.  If so, evaluate this token again.
	 *  is_aggregate_type, above, should not have found a method
	 *  label, but we'll look again anyway.
	 *
	 */
	if ((i < p -> msg_frame_start) &&
	    (IS_OBJECT(m -> obj) && (m -> obj -> attrs
				     & OBJECT_IS_VALUE_VAR))){
	  /* this is only used for the value label */
	  if ((prev_tok_idx = prev_msg (e_messages, i)) != ERROR) {
	    if ((m -> attrs & RT_OBJ_IS_INSTANCE_VAR) ||
		__ctalkGetInstanceVariable
		(M_VALUE_OBJ(e_messages[prev_tok_idx]), 
		 M_NAME(e_messages[i]), FALSE)) {
	      var = m -> obj;
	      if (!_cleanup_temporary_objects 
		  (M_VALUE_OBJ(m),
		   e_result,
		   subexpr_result,
		   rt_cleanup_null)) {
		if ((next_tok_idx = 
		     next_msg (e_messages, i)) != ERROR) {
		  if (m -> obj == 
		      e_messages[next_tok_idx] -> receiver_obj) {
		    e_messages[next_tok_idx] -> receiver_obj =
		      var;
		  }
		}
		m -> obj = var;
		m -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
	      }
	      /*
	       *  If the previous message is also an
	       *  instance variable fixup, then re-eval.
	       *  As of now, "value" is the only message that is 
	       *  both a method and an instance variable, so that 
	       *  is the case this clause handles.
	       */
	      /*
	       *  Somewhat replaced by fixup_forward_receiver_obj ().
	       */
	      if ((e_messages[prev_tok_idx]->value_obj==m->receiver_obj) && 
		  (m -> obj && (m->obj->scope==64))) {
		m -> tokentype = METHODMSGLABEL;
		if (!m -> receiver_msg)
		  m -> receiver_msg = e_messages[prev_tok_idx];
		m -> evaled = 0;
		++i;
		continue;
	      } else {
		if ((M_VALUE_OBJ(e_messages[prev_tok_idx]) ==
 		     m -> receiver_obj) &&
		    (i == p -> msg_frame_top)) {
		  if ((var_i_var = var_i (var, scope)) != var) {
		    if (!(var_i_var -> attrs & OBJECT_IS_NULL_RESULT_OBJECT)) {
		      set_method_expr_value (e_messages, i,
					     prev_tok_idx, 
					     i, precedence,
					     var_i_var);
		    } else {
		      set_method_expr_value (e_messages, i,
					     prev_tok_idx, 
					     i, precedence,
					     var);
		      if (var_i_var != var) {
			__ctalkDeleteObject (var_i_var);
		      }
		    }
		    eval_status |= 
		      (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
		  } else { 
		    if (prev_tok_idx == p -> msg_frame_start) {
		      /* two-token expression; e.g., 'myObject value'. */
		      eval_status |= 
			(EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
		      goto done;
		    } else {
		      if (!(var_i_var -> attrs & OBJECT_IS_NULL_RESULT_OBJECT)) {
			set_method_expr_value (e_messages, i,
					       prev_tok_idx, 
					       i, precedence,
					       var_i_var);
		      } else {
			set_method_expr_value (e_messages, i,
					       prev_tok_idx, 
					       i, precedence,
					       var);
			if (var_i_var != var) {
			  __ctalkDeleteObject (var_i_var);
			}
		      }
		      eval_status |= 
			(EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
		    }
		  }
  		} else {
		  if ((i == p -> msg_frame_top) &&
		      (e_messages[prev_tok_idx] -> attrs 
		       & RT_OBJ_IS_CLASS_VAR) &&
		      (m -> receiver_obj == 
		       e_messages[prev_tok_idx] -> receiver_obj)) {
		    
		    var = var_i (var, scope);

		    set_method_expr_value (e_messages, i,
					   prev_tok_idx, 
					   i, precedence,
					   var);
		    eval_status |= 
		      (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
		  } else if ((i == p -> msg_frame_top) &&
			     (prev_tok_idx == p -> msg_frame_start) &&
			     (e_messages[prev_tok_idx] -> value_obj ==
			      m -> obj) &&
			     !(e_messages[prev_tok_idx] -> obj -> attrs &
			       OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) &&
			     ((m -> attrs & RT_OBJ_IS_INSTANCE_VAR) ||
			      (m -> attrs & RT_OBJ_IS_CLASS_VAR))) {
		    /* two-token expression with an instance or class
		       variable as the second token. 
		       - BUT NOT A COLLECTION MEMBER - maybe should
		       adjust this later, when we have an
		       example case. */
		    eval_status |= 
		      (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
		    goto done;
		  }
		}
 	      }
	    } else {
	      m -> tokentype = METHODMSGLABEL;
	      if (m -> receiver_msg == NULL)
		m -> receiver_msg = e_messages[prev_tok_idx];
	      ++i;
	      /*
	       *  Fixup for deref method results.
	       */
	      if ((e_messages[prev_tok_idx] -> attrs & 
		   RT_TOK_IS_AGGREGATE_MEMBER_LABEL) &&
		  e_messages[prev_tok_idx] -> value_obj) {
		m -> receiver_obj = 
		  e_messages[prev_tok_idx] -> value_obj;
		if (!e_messages[prev_tok_idx] -> obj)
		  e_messages[prev_tok_idx] -> obj =
		    e_messages[prev_tok_idx] -> value_obj;
		for (i_2 = i; i_2 >= p -> msg_frame_top; i_2--) {
		  e_messages[i_2] -> evaled = 0;
		}
	      }
	      continue;
	    }
	  } else {
	    m -> tokentype = METHODMSGLABEL;
	    ++i;
	    continue;
	  }
	} else {
	  prev_msg_ptr = prev_msg (e_messages, i);
	  m_prev_msg = e_messages[prev_msg_ptr];
	  if (m_prev_msg&&IS_MESSAGE(m_prev_msg)&&IS_OBJECT(m_prev_msg->obj)){
	    if (!(m -> attrs & RT_OBJ_IS_INSTANCE_VAR) &&
		!(m -> attrs & RT_VALUE_OBJ_IS_INSTANCE_VAR)) {
	      OBJECT *t_insvar = NULL, *t_clsvar = NULL;
		if (((t_insvar = 
		    __ctalkGetInstanceVariable (M_VALUE_OBJ(m_prev_msg), 
						M_NAME(m),
						FALSE)) != NULL) ||
		  ((t_clsvar = 
		    __ctalkGetClassVariable (M_VALUE_OBJ(m_prev_msg),
					     M_NAME(m),
					     FALSE)) != NULL)) {
		  terminal_tok_obj (m_prev_msg, m, t_insvar, t_clsvar);
		  for (i_2 = i; i_2 >= p -> msg_frame_top; i_2--) {
		    if (!(e_messages[i_2] -> attrs & RT_TOK_IS_TYPECAST_EXPR))
		      e_messages[i_2] -> evaled = 0;
		    if (e_messages[i_2] -> receiver_obj ==
			m -> obj) {
		      e_messages[i_2] -> receiver_obj
			= m -> value_obj;
		    }
		  }
		  /*
		   *  This is the easiest way to ensure that 
		   *  a parent object doesn't get fewer references,
		   *  and then deleted sooner, than an instance variable, 
		   *  without trying to adjust within the generic 
		   *  rtobjref.c functions.
		   */
		  if (IS_OBJECT(t_insvar)) {
		    (void)__objRefCntSet (OBJREF(t_insvar -> __o_p_obj),
					  t_insvar -> nrefs);
		  } else if (IS_OBJECT(t_clsvar)) {
		    (void)__objRefCntSet (OBJREF(t_clsvar -> __o_p_obj),
					  t_clsvar -> nrefs);
		  }
		  if (c_rcvr_idx == -1)
		    c_rcvr_idx = prev_msg (e_messages, i);
		}
	      // This is the super that modifies a method.
	      // The super that works in arg block scope 
	      // is in __ctalk_get_object ().
	      if (!strcmp (M_NAME(m), "super")) {
		__refObj (OBJREF(m->value_obj),
			  OBJREF(m_prev_msg->obj->__o_superclass));
		m -> attrs |= RT_TOK_VALUE_OBJ_IS_SUPERCLASS;
		m -> receiver_obj = M_VALUE_OBJ(m_prev_msg);
		if (c_rcvr_idx == -1)
		  c_rcvr_idx = prev_msg (e_messages, i);
	      }
	    } 
	  } else {
	    if (m_prev_msg && IS_MESSAGE(m_prev_msg) && 
		IS_OBJECT(m_prev_msg -> value_obj)) {
	      /*
	       *  Having a forward object fixup now should simplify
	       *  this case.
	       */
	      if (__is_instvar_of_method_return (p, i)) { 
		if ((e_messages[i] -> attrs & RT_TOK_OBJ_IS_CREATED_PARAM) ||
		    (IS_OBJECT(e_messages[i]->obj) &&
		     (e_messages[i] -> obj -> scope & CREATED_PARAM))) {
		  e_messages[i] -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
		  if (((var = 
			__ctalkGetInstanceVariable (m_prev_msg -> value_obj,
						    M_NAME(m),
						    FALSE)) != NULL) ||
		      ((var = 
			__ctalkGetClassVariable (m_prev_msg -> value_obj,
						 M_NAME(m),
						 FALSE)) != NULL)) {
		    
		    /* Fixup here and next message if necessary. */
		    if ((m_next_idx =
			 next_msg (e_messages, i)) != ERROR) {
		      if (e_messages[m_next_idx] -> receiver_obj ==
			  e_messages[i] -> obj)
			e_messages[m_next_idx] -> receiver_obj = NULL;
		      if (e_messages[i] -> obj && 
			  (e_messages[i] -> obj != var))
			delete_all_objs_expr_frame (p, i, 
						    &all_objects_deleted);
		      e_messages[i]->obj = var;
		      e_messages[i]->value_obj = var;
		      if (!e_messages[m_next_idx] -> receiver_obj)
			e_messages[m_next_idx] -> receiver_obj = var;
		      if (!e_messages[m_next_idx] -> receiver_obj)
			e_messages[m_next_idx] -> receiver_obj = var;
		      if (((var = 
			    __ctalkGetInstanceVariable 
			    (e_messages[i] -> obj,
			     M_NAME(e_messages[m_next_idx]),
			     FALSE)) != NULL) ||
			  ((var = 
			    __ctalkGetClassVariable 
			    (e_messages[i] -> obj, 
			     M_NAME(e_messages[m_next_idx]),
			     FALSE)) != NULL)) {
			if ((e_messages[i] -> value_obj ==
			     e_messages[m_next_idx] -> obj) && 
			    (e_messages[m_next_idx] -> attrs & 
			     (RT_OBJ_IS_INSTANCE_VAR|RT_OBJ_IS_CLASS_VAR))) {
			  if (e_messages[m_next_idx] -> obj &&
			      (e_messages[m_next_idx] -> obj != var))
			    delete_all_objs_expr_frame (p, m_next_idx,
							&all_objects_deleted);
			  e_messages[m_next_idx] -> obj = 
			    e_messages[i] -> value_obj = var;
			}
		      }
 		    }
		  }
		} else {
		  fixup_forward_instance_var_of_method_return (p, i);
		}
	      } else { /* if (__is_instvar_of_method_return ... */

		/*
		 * This fixes up cases like 
		 *   *<rcvr> <method> [<args>...]
		 */
		if (IS_OBJECT(e_messages[i] -> obj) && 
		    e_messages[i] -> obj -> attrs & OBJECT_HAS_PTR_CX) {
		  if ((m_next_idx =
		       next_msg (e_messages, i)) != ERROR) {
		    if ((M_TOK(e_messages[m_next_idx]) == LABEL) &&
			__ctalk_isMethod_2 (M_NAME(e_messages[m_next_idx]), 
					    e_messages, 
					    m_next_idx, p -> msg_frame_start)) {
		      e_messages[m_next_idx] -> tokentype = METHODMSGLABEL;
		      e_messages[m_next_idx] -> evaled = 0;
		      continue;
		    }
		  }
		}
	      }  /* if (__is_instvar_of_method_return ... */
	    } /*if ((e_messages[i] -> attrs & RT_TOK_OBJ_IS_CREATED_PARAM) ||*/
	  } /* if (__is_instvar_of_method_return (p, i) */
	  if (i == p -> msg_frame_top && 
	      ((m -> attrs & RT_OBJ_IS_INSTANCE_VAR) ||
	       (m -> attrs & RT_VALUE_OBJ_IS_INSTANCE_VAR))) {
	    eval_status |= (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR);
	  } else if (i == p -> msg_frame_top && 
		     (m -> attrs & RT_OBJ_IS_CLASS_VAR)) {
	    eval_status |= (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_CLASS_VAR);
	  } else if (i == p -> msg_frame_top &&
		     IS_OBJECT(m -> obj) &&
		     (m -> obj -> scope & CREATED_PARAM) &&
		     !((m -> obj -> scope & LOCAL_VAR) ||
		       (m -> obj -> scope & GLOBAL_VAR)) &&
		     !str_eq (M_NAME(m), "super") &&
		     !is_cvar (M_NAME(m)) &&
		     !(m -> attrs & RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) &&
		     !is_method_param (M_NAME(m)) &&
		     !__ctalk_isMethod_2 (M_NAME(m), e_messages, i,
					  p -> msg_frame_start) &&
		     !rcvr_has_ptr_cx (p, i, c_rcvr_idx) &&
		     !prev_tok_is_symbol (e_messages, i)) {
	    /* If we've resolved a self token here, it might be helpful. 
	       Any more complex and this should go in rt_error.c */
	    int j_2, j_self = -1;
	    if (__ctalkGetExceptionTrace ())
	      __warning_trace ();
	    for (j_2 = i; j_2 <= expr_parsers[expr_parser_ptr] -> msg_frame_start;
		 ++j_2) {
	      if (e_messages[j_2] -> attrs & RT_TOK_IS_SELF_KEYWORD) {
		j_self = j_2;
	      }
	    }
	    if (j_self > 0) {
	      _warning ("Warning: In the expression:\n\n\t%s\n\n"
			"Label, \"%s,\" is not resolved as an object "
			"or a method.  (Receiver \"self,\" class, \"%s.\")\n",
			p -> expr_str, M_NAME(m),
			(IS_OBJECT(e_messages[j_self] -> obj -> instancevars) ?
			 e_messages[j_self] -> obj -> instancevars -> 
			 __o_class -> __o_name :
			 e_messages[j_self] -> obj -> __o_class -> __o_name));
	    } else {
	      _warning ("Warning: In the expression:\n\n\t%s\n\n"
			"Label, \"%s,\" is not resolved as an object "
			"or a method.\n", p -> expr_str, M_NAME(m));
	    }
	  }
	  ++m -> evaled;
	}
	break;
      default:
	if (eval_op_precedence (precedence, e_messages, i)) { 
	  switch (m -> tokentype)
	    {
	    case OPENPAREN:
	      /*
	       *  Check for a matching close parenthesis, then:
	       *  1. Typecast.
	       *  2. C function, evaluated by syntax.
	       *  3. Subexpression.
	       */
	      if ((matching_paren_ptr = 
		   expr_match_paren (p, i)) == ERROR) {
		sprintf (namebuf, "\n\n\t%s\n\n",
			 expr_parsers[expr_parser_ptr] -> expr_str);
		__ctalkExceptionInternal (m, mismatched_paren_x, namebuf,0);
		__ctalkHandleRunTimeExceptionInternal ();
		printf ("%s", namebuf);
		exit (1);
	      }
	      fn_tok_ptr = prev_msg (e_messages, i);
	      if (__rt_is_typecast_expr (p, i)) {
		if (expr_typecast_is_pointer (p, i)) {
		  typecast_ptr_expr_b (p, i, matching_paren_ptr);
		} else {
		  /* TODO - this duplicates the first pass - check
		     if it's for every case, and maybe we can remove it */
		  typecast_value_expr (e_messages, i, matching_paren_ptr);
		}
	      } else {
 		if ((fn_tok_ptr != ERROR) && 
		    is_c_fn_syntax (e_messages, fn_tok_ptr)) {
		  for (j = fn_tok_ptr; j >= matching_paren_ptr; j--)
		    ++e_messages[j] -> evaled;
 		} else {
		  int close_param_list_idx;
		  int i_1;
		  /* This is touchy if the (*expr)() is enclosed in parentheses,
		     but we don't need to waste a message attribute on
		     it, either. */
		  if (!sym_ptr_expr (p, i, matching_paren_ptr,
				     &close_param_list_idx)) {
		    for (i_1 = i; i_1 >= close_param_list_idx; --i_1) {
		      e_messages[i_1] -> evaled += 2;
		    }
		    i = close_param_list_idx;
		  } else {
		    int __expr_close_paren;
		    /* See the comment in pattypes.c. */
		    if (is_single_token_cast (p, i, &__expr_close_paren)) {
		      i = __expr_close_paren;
		      continue;
		    }

		    if ((subexpr_result = eval_subexpr (e_messages, i,
							is_arg_expr)) == NULL){
		      /*
		       *  Empty set of parentheses.
		       */
		      int prev_val_ptr;
		      if ((prev_val_ptr = prev_msg (e_messages, i)) != ERROR) {
			if (e_messages[prev_val_ptr] && 
			    IS_MESSAGE(e_messages[prev_val_ptr])) {
			  _set_expr_value (e_messages, prev_val_ptr, 
					   matching_paren_ptr, i, 
					   M_VALUE_OBJ(e_messages[prev_val_ptr]));
			  for (; i >= matching_paren_ptr; i--)
			    ++e_messages[i] -> evaled;
			}
		      }
		    } else if (is_complete_var_expr
			       (e_messages, i,
				matching_paren_ptr,
				subexpr_result)) {
		      e_result = e_messages[p -> msg_frame_top] -> value_obj;
		      goto done;
		    } else {
		      /*
		       *  subexpr_result needs at least 1 
		       *  reference count.
		       */
		      if (!is_current_receiver(subexpr_result)) {
			if (subexpr_result -> nrefs == 0)
			  __ctalkSetObjectScope (subexpr_result, 
						 subexpr_result -> scope | 
						 SUBEXPR_CREATED_RESULT);
			(void)__objRefCntInc(OBJREF(subexpr_result));
		      }
		      i = matching_paren_ptr;
		    }
		  }
 		}
	      }
	      break;
	    case CLOSEPAREN:
	      ++m -> evaled;
	      break;
	      /* 
	       * Binary numeric and boolean operators, and overloaded
	       * unary ops with the same token. 
	       */
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
	    case MATCH:
	    case NOMATCH:
	    case MODULUS:
	      if (!m -> evaled) {
		if (!(m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) &&
		    /* we should check this attribute here too - it's
		       a little quicker */
		    __ctalk_op_isMethod_2 (M_NAME(m),
					   e_messages, i,
					   p -> msg_frame_start)) {
		  /*
		   *  If we decide to control the iteration order of
		   *  multiple operators better, then we can actually
		   *  set n_args_declared to 1, which is what 
		   *  _rt_operands () looks for.
		   */
		  m -> attrs |= RT_DATA_IS_NR_ARGS_DECLARED;
		  m -> attr_data = ANY_ARGS;
		  _rt_operands (p, i, &op1_ptr, &op2_ptr); 
		  if (IS_OBJECT(e_messages[op1_ptr] -> value_obj) &&
		      e_messages[op1_ptr] 
  		      -> value_obj -> scope & SUBEXPR_CREATED_RESULT) {
		    if (IS_OBJECT (op1_subexpr_result)) {
		      (void)__objRefCntDec(OBJREF(op1_subexpr_result));
		      __ctalkDeleteObjectInternal (op1_subexpr_result);
		    }
		    op1_subexpr_result = e_messages[op1_ptr]->value_obj;
		  }
		  if (op2_ptr != ERROR) {
		    if (IS_OBJECT(e_messages[op2_ptr] -> value_obj) &&
			e_messages[op2_ptr] 
			-> value_obj -> scope & SUBEXPR_CREATED_RESULT) {
		      if (IS_OBJECT (op2_subexpr_result)) {
			(void)__objRefCntDec(OBJREF(op2_subexpr_result));
			__ctalkDeleteObjectInternal (op2_subexpr_result);
		      }
		      op2_subexpr_result = e_messages[op2_ptr]->value_obj;
		    }
		  } else {
		    if (__ctalkGetExceptionTrace ())
		      __warning_trace ();
		    _warning ("Warning: Invalid expression:\n\n\t%s\n\n",
			      p -> expr_str);
		    exit (EXIT_FAILURE);
		  }

		  if ((c_rcvr_idx != -1) && (c_rcvr_idx < i))
		    c_rcvr_idx = -1;
		  m -> receiver_msg = e_messages[op1_ptr];
		        
		  m -> tokentype = METHODMSGLABEL;
		  ++i;
		  continue;
		} else {
		  if (e_messages[i] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
		    if ((next_msg_ptr = 
			 next_msg (e_messages, i)) != ERROR) {

		      /*
		       * If the operator has a lower precedence than
		       * the rest of the expression, then check for
		       * a (null) value object that is the result of
		       * calculations from previous passes, and delete 
		       * it if necessary.
		       */
		      if (IS_OBJECT(e_messages[next_msg_ptr] -> value_obj) &&
			  e_messages[next_msg_ptr] -> value_obj -> attrs &
			  OBJECT_IS_NULL_RESULT_OBJECT) {
			OBJECT *__null_obj;
			__null_obj = e_messages[next_msg_ptr] -> value_obj;
			__objRefCntZero (OBJREF(__null_obj));
			clear_expr_value_obj (e_messages, p -> msg_frame_start,
					      p -> msg_frame_top, __null_obj);
		      }

		      if ((prefix_rcvr = M_VALUE_OBJ(e_messages[next_msg_ptr])) 
			  != NULL) {
			if ((e_method = __ctalkFindPrefixMethodByName 
			     (&prefix_rcvr,
			      M_NAME(e_messages[i]), TRUE)) != NULL) {
			  __ctalk_receiver_push_ref (prefix_rcvr);
			  __save_rt_info (prefix_rcvr, prefix_rcvr->__o_class, 
					  NULL, 
					  e_method, e_method -> cfunc, 
					  False);
			  e_result = __ctalkCallMethodFn (e_method);
			  __ctalk_receiver_pop_deref ();
			  __restore_rt_info ();
			  find_self_unary_expr_limits_2 (e_messages, 
							 next_msg_ptr,
							 &expr_end_ptr,
							 p -> msg_frame_top);
			  set_method_expr_value (e_messages, 
						 i, i, expr_end_ptr,
						 precedence,
						 e_result);
			  if (i == p -> msg_frame_top)
			    goto done;
			} else {
			  __ctalkExceptionInternal 
			    (e_messages[i], undefined_method_x, 
			     M_NAME(e_messages[i]), 0);
			  ++m -> evaled;
			}
		      }
		    }
		  } else {/*if (e_mssages[i] -> attrs & RT_TOK_IS_PREFIX_OPERATOR)*/
		    if ((value_obj = _rt_math_op (e_messages, i, p -> msg_frame_start, 
						  p -> msg_frame_top)) != NULL) {
		      _set_expr_value (e_messages, p -> msg_frame_start, p -> msg_frame_top, i, 
				       value_obj);
		    } else {
		      goto done;
		    }
		    ++m -> evaled;
		  }
		}
	      }
	      break;
	      /* Unary math and boolean operators, unless an overloaded 
	       * binary op above.
	       */
	    case BIT_COMP:
	    case LOG_NEG:
	      if (!m -> evaled) {
		if (e_messages[i] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
		  if ((next_msg_ptr = 
		       next_msg (e_messages, i)) != ERROR) {
		    if ((prefix_rcvr = M_VALUE_OBJ(e_messages[next_msg_ptr])) 
			!= NULL) {
		      if ((e_method = __ctalkFindPrefixMethodByName 
			   (&prefix_rcvr,
			    M_NAME(e_messages[i]), TRUE)) != NULL) {
			__ctalk_receiver_push_ref (prefix_rcvr);
			__save_rt_info (prefix_rcvr, prefix_rcvr->__o_class, 
					NULL, 
					e_method, e_method -> cfunc, 
					False);
			e_result = __ctalkCallMethodFn (e_method);
			__ctalk_receiver_pop_deref ();
			__restore_rt_info ();
			/* Check if there are further math ops
			   before setting the expr_end - these ops
			   have precedence of 1, so we could so
			   far stop at any other math op. -- any
			   labels have already been evaluated. This
			   is a little sketched out right now, should
			   become finer grained in the future. */
			expr_end_ptr = next_msg_ptr;
			for (i_2 = next_msg_ptr; i_2 >= p -> msg_frame_top;
			     --i_2) {
			  if (M_ISSPACE(e_messages[i_2]))
			    continue;
			  if (IS_C_OP_CHAR(e_messages[i_2] -> name[0])) {
			    break;
			  } else {
			    expr_end_ptr = i_2;
			  }
			}
			if (expr_end_ptr == p -> msg_frame_top) {
			  for (i_2 = i; i_2 >= p -> msg_frame_top; --i_2) {
			    e_messages[i_2] -> value_obj = e_result;
			    ++e_messages[i_2] -> evaled;
			  }
			} else {
			  set_method_expr_value (e_messages, 
						 i, i, next_msg_ptr,
						 precedence,
						 e_result);
			}
		      } else {
			value_obj = _rt_unary_math_op (p, i);
			if (internal_exception_trap ())
			  goto done;
			_set_unary_expr_value (p, i, value_obj,
					       &subexpr_result);
			++m -> evaled;
		      }
		    }
		  }
		} else { /* if (e_messages[i] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) */
		  value_obj = _rt_unary_math_op (p, i);
		  if (internal_exception_trap ())
		    goto done;
		  _set_unary_expr_value (p, i, value_obj, &subexpr_result);
		  ++m -> evaled;
		}
	      }
	      break;
	    case CONDITIONAL:     /* Gets re-arranged in the parser, so
				     this is a no-op here. */
	      break;
	    case SIZEOF:
	      if ((next_msg_ptr = 
		   next_msg (e_messages, i)) != ERROR) {
		if (M_TOK(e_messages[next_msg_ptr]) == OPENPAREN) {
		  prefix_rcvr = 
		    eval_subexpr (e_messages, next_msg_ptr, is_arg_expr);
		  if ((e_method = __ctalkFindPrefixMethodByName 
		       (&prefix_rcvr, M_NAME(m), TRUE)) != NULL) {
		    __ctalk_receiver_push_ref (prefix_rcvr);
		    __save_rt_info (prefix_rcvr, prefix_rcvr->__o_class, 
				    NULL, 
				    e_method, e_method -> cfunc, 
				    False);
		    e_result = __ctalkCallMethodFn (e_method);
		    __ctalk_receiver_pop_deref ();
		    __restore_rt_info ();
		    set_method_expr_value (e_messages, 
					   i, i, next_msg_ptr,
					   precedence,
					   e_result);
		  }
		} else { /* if (M_TOK(e_messages[next_msg_ptr] == OPENPAREN)) */
		  __ctalkExceptionInternal (NULL, parse_error_x,
					    M_NAME(m), 0);
		} /* if (M_TOK(e_messages[next_msg_ptr] == OPENPAREN)) */
	      }
	      ++m -> evaled;
	      break;
	    case INCREMENT:
	    case DECREMENT:
	      if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
		/*
		 *  Skip over parentheses around the variable.
		 */
		if ((next_msg_ptr = 
		     __ctalkIsLeadingPrefixOp_a (e_messages, i)) != ERROR) {
		  if ((prefix_rcvr = prefix_receiver 
		       (e_messages[next_msg_ptr])) != NULL) {
		    if ((e_method = __ctalkFindPrefixMethodByName 
			 (&prefix_rcvr,
			  M_NAME(e_messages[i]), TRUE)) != NULL) {
		      __ctalk_receiver_push_ref (prefix_rcvr);
		      __save_rt_info (prefix_rcvr, prefix_rcvr->__o_class, 
				      NULL, 
				      e_method, e_method -> cfunc, 
				      False);
		      e_result = __ctalkCallMethodFn (e_method);
		      __ctalk_receiver_pop_deref ();
		      __restore_rt_info ();
		      
		      /* This is almost the same as after the call to
			 __ctalkCallMethodFn (), above */
		      if ((c_i = active_i (e_result)) != I_UNDEF) {
			e_result = create_param_i (e_result, c_i);
		      }
		      
		      set_method_expr_value (e_messages, 
					     i, i, next_msg_ptr,
					     precedence,
					     e_result);
		      /*
		       *  Handle a case like ++(var) without 
		       *  re-evaluating the following tokens as a 
		       *  subexpression.  This still does not work
		       *  correctly when used as a method argument
		       *  (the evaluator has trouble determining a 
		       *  receiver), but at least it presents a 
		       *  decent exception message.
		       */
		      if ((next_msg_ptr =
			   next_msg (e_messages, i)) != ERROR) {
			if (M_TOK(e_messages[next_msg_ptr]) == OPENPAREN) {
			  int __m;
			  if ((__m = expr_match_paren (p, next_msg_ptr))
			      != ERROR) {
			    find_prefixed_CVAR_for_write (i, e_method);
			    if (__m == p -> msg_frame_top) {
			      goto done;
			    }
			  }
			}
		      }
		      
		    } else {
		      value_obj = _rt_unary_math_op (p, i);
		      if (internal_exception_trap ())
			goto done;
		      _set_unary_expr_value (p, i, value_obj,
					     &subexpr_result);
		      ++m -> evaled;
		    }
		  } else if (e_messages[next_msg_ptr] -> attrs &
			     RT_TOK_IS_PREFIX_OPERATOR) { /*if ((prefix_rcvr=*/
		    /* handle an expression like ++*<var> */
		    if ((subexpr_result =
			 eval_rassoc_rcvr_as_subexpr (e_messages, i,
						      is_arg_expr,
						      &prefix_expr_end))
			!= NULL) {
		      prefix_rcvr = subexpr_result;
		      if ((e_method = __ctalkFindPrefixMethodByName 
			   (&prefix_rcvr,
			    M_NAME(e_messages[i]), TRUE)) != NULL) {
			__ctalk_receiver_push_ref (prefix_rcvr);
			__save_rt_info (prefix_rcvr, prefix_rcvr->__o_class, 
					NULL, 
					e_method, e_method -> cfunc, 
					False);
			e_result = __ctalkCallMethodFn (e_method);
			__ctalk_receiver_pop_deref ();
			__restore_rt_info ();
			if ((c_i = active_i (e_result)) != I_UNDEF) {
			  e_result = create_param_i (e_result, c_i);
			}
		      
			set_method_expr_value (e_messages, 
					       i, i, prefix_expr_end,
					       precedence, e_result);
			find_prefixed_CVAR_for_write (i, e_method);
		      }
		    }
		  } /* else if (e_messages[next_msg_ptr] -> attrs & ... */
		} /* if ((next_msg_ptr = ... */
	      } else { /* if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) */
		int m_prev_tok;
		if (__ctalk_isMethod_2 (M_NAME(m), e_messages, i,
					p -> msg_frame_start)) {
		  int ag_label_idx;
		  if ((ag_label_idx = next_msg (e_messages, i)) != ERROR) {
		    if ((M_TOK(e_messages[ag_label_idx]) == LABEL) &&
			((M_TOK(m) == DEREF) || (M_TOK(m) == PERIOD))) {
		      e_messages[ag_label_idx] -> attrs |= RT_TOK_IS_AGGREGATE_MEMBER_LABEL;
		      if ((next_tok_idx = 
			   next_msg (e_messages,ag_label_idx)) != ERROR) {
			e_messages[next_tok_idx]->receiver_msg = 
			  e_messages[ag_label_idx];
		      }
		    }
		  }
		  m -> attrs |= RT_TOK_IS_POSTFIX_MODIFIED;
		  m -> tokentype = METHODMSGLABEL;
		  if ((m_prev_tok = prev_msg (e_messages, i)) != ERROR) {
		    if ((M_TOK(e_messages[m_prev_tok]) == LABEL) &&
			(m -> receiver_msg == NULL))
		      m -> receiver_msg = e_messages[m_prev_tok];
		    if (e_messages[m_prev_tok] -> attrs & 
			RT_TOK_IS_POSTFIX_MODIFIED)
		      m -> receiver_msg = 
			e_messages[m_prev_tok] -> receiver_msg;
		  }
		  ++i;
		  continue;
		} else if (!m -> evaled) {
		  if ((prefix_expr_start = has_rassoc_pfx (i)) != ERROR) {
		    /* handle expression like 
		     *
		     *  *<cvar>++
		     *
		     * similar to
		     *
		     *  (*<cvar>)++
		     *
		     * Not sure if this is equivalent o correct C, 
		     * but it seems to work here.
		     */
		    prefix_rcvr = eval_rassoc_rcvr_as_subexpr_b
		      (e_messages, prev_msg (e_messages, i),
		       prefix_expr_start, is_arg_expr);
		    if ((e_method = __ctalkFindInstanceMethodByName 
			 (&prefix_rcvr,
			  M_NAME(e_messages[i]), TRUE, ANY_ARGS)) != NULL) {
		      __ctalk_receiver_push_ref (prefix_rcvr);
		      __save_rt_info (prefix_rcvr, prefix_rcvr->__o_class, 
				      NULL, 
				      e_method, e_method -> cfunc, 
				      False);
		      e_result = __ctalkCallMethodFn (e_method);
		      __ctalk_receiver_pop_deref ();
		      __restore_rt_info ();
		      if ((c_i = active_i (e_result)) != I_UNDEF) {
			e_result = create_param_i (e_result, c_i);
		      }
		      
		      set_method_expr_value (e_messages, 
					     i, prefix_expr_start, i,
					     precedence, e_result);
		      write_val_CVAR (prefix_rcvr, e_method);
		    }
		  } else {
		    if ((value_obj = _rt_math_op (e_messages, i,
						  p -> msg_frame_start, 
						  p -> msg_frame_top))
			!= NULL) {
		      _set_expr_value (e_messages, p -> msg_frame_start,
				       p -> msg_frame_top, i, 
				       value_obj);
		    } else {
		      goto done;
		    }
		  }
		  ++m -> evaled;
		}
	      } /* if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) */
	      break;
	    case EQ:
	    case ASR_ASSIGN:
	    case ASL_ASSIGN:
	    case PLUS_ASSIGN:
	    case MINUS_ASSIGN:
	    case MULT_ASSIGN:
	    case DIV_ASSIGN:
	    case BIT_AND_ASSIGN:
	    case BIT_OR_ASSIGN:
	    case BIT_XOR_ASSIGN:
	    case PERIOD:
	    case DEREF:
	      if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
		if ((next_msg_ptr = 
		     next_msg (e_messages, i)) != ERROR) {
		  if ((prefix_rcvr = M_VALUE_OBJ(e_messages[next_msg_ptr])) 
		      != NULL) {
		    if ((e_method = __ctalkFindPrefixMethodByName 
			 (&prefix_rcvr,
			  M_NAME(e_messages[i]), TRUE)) != NULL) {
		      __ctalk_receiver_push_ref (prefix_rcvr);
		      __save_rt_info (prefix_rcvr, prefix_rcvr->__o_class, 
				      NULL, 
				      e_method, e_method -> cfunc, 
				      False);
		      e_result = __ctalkCallMethodFn (e_method);
		      __ctalk_receiver_pop_deref ();
		      __restore_rt_info ();
		      set_method_expr_value (e_messages, 
					     i, i, next_msg_ptr,
					     precedence,
					     e_result);
		    } else {
		      value_obj = _rt_unary_math_op (p, i);
		      if (internal_exception_trap ())
			goto done;
		      _set_unary_expr_value (p, i, value_obj, &subexpr_result);
		      ++m -> evaled;
		    }
		  } /* if ((prefix_rcvr = M_VALUE_OBJ(e_messages[next_msg_ptr])) */
		} /* 		if ((next_msg_ptr =  */
	      } else { /* if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) */
		if (__ctalk_isMethod_2 (M_NAME(m), e_messages, i,
					p -> msg_frame_start)) {
		  int ag_label_idx;
		  if ((ag_label_idx = next_msg (e_messages, i)) != ERROR) {
		    if ((M_TOK(e_messages[ag_label_idx]) == LABEL) &&
			((M_TOK(m) == DEREF) || (M_TOK(m) == PERIOD))) {
		      e_messages[ag_label_idx] -> attrs |= RT_TOK_IS_AGGREGATE_MEMBER_LABEL;
		      if ((next_tok_idx = 
			   next_msg (e_messages,ag_label_idx)) != ERROR) {
			e_messages[next_tok_idx]->receiver_msg = 
			  e_messages[ag_label_idx];
		      }
		    }
		  }
		  m -> tokentype = METHODMSGLABEL;
		  ++i;
		  continue;
		} else {
		  if (!m -> evaled) {
		    if ((value_obj = _rt_math_op (e_messages, i, p -> msg_frame_start,
						  p -> msg_frame_top)) != NULL) {
		      _set_expr_value (e_messages, p -> msg_frame_start, p -> msg_frame_top, i, 
				       value_obj);
		    } else {
		      goto done;
		    }
		    ++m -> evaled;
		  }
		}
	      } /* if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) */
	      break;
	    default:
	      ++m -> evaled;
	      break;
	    }
	}
	break;
      }
  }

  for (; precedence < 13; ++precedence) {
    for (i = p -> msg_frame_start; i >= p -> msg_frame_top; i--) {
      if (e_messages[i] -> evaled == 0) {
	if (eval_op_precedence (precedence, e_messages, i)) {
	  goto re_eval;
	}
      }
    }
  }

  done:

  /*
   *  If there is an exception, the result object should
   *  be a null object.  Otherwise, the value object or
   *  object of the expression is the result.
   *
   *  Check the critical flag, so the exception is handled 
   *  at the level of the __ctalkEvalExpr () call if necessary.
   */
  if (internal_exception_trap ()) {
    OBJECT *__rcvr_object_tmp;
    /*
     *  TODO - Make this function more useful.
     */
    result_obj = create_object_init_internal
      (NULLSTR, rt_defclasses -> p_integer_class, scope, "0");
    __ctalkHandleRunTimeException ();
    if ((__rcvr_object_tmp = rtinfo.rcvr_obj) != NULL) {
      (void)__objRefCntSet (OBJREF(result_obj), __rcvr_object_tmp -> nrefs);
      __ctalkSetObjectScope (result_obj, __rcvr_object_tmp -> scope);
    }
  } else {
    if (IS_OBJECT (e_messages[p -> msg_frame_start] -> value_obj)) {
      result_obj = expr_result_object ();
    } else {
      if ((eval_status & EVAL_STATUS_TERMINAL_TOK) &&
	  ((eval_status & EVAL_STATUS_INSTANCE_VAR) ||
	   (eval_status & EVAL_STATUS_CLASS_VAR))) {
	if (M_VALUE_OBJ(e_messages[p -> msg_frame_top]) &&
	    IS_OBJECT (M_VALUE_OBJ(e_messages[p -> msg_frame_top]))) {
	  result_obj = M_VALUE_OBJ(e_messages[p -> msg_frame_top]);
	}
      } else {
	if (IS_OBJECT (e_messages[p -> msg_frame_start] -> obj)) {
	  result_obj = e_messages[p -> msg_frame_start]->obj;
	} else {
	  if (e_messages[p -> msg_frame_start]->attrs & RT_TOK_IS_TYPECAST_EXPR) {
	    result_obj = expr_result_object ();
	  } else {
	    result_obj = NULL;
	  }
	}
      }
    }
  }

  /*
   *  NOTE - This will have to be factored out eventually.
   *
   *  Note that the routine must do a sweep for the deleted
   *  object (in the loops with j) in the messages preceding 
   *  the message of the deleted object, if _cleanup_temporary_objects () 
   *  decided it was deleteable.  Otherwise, we run into a 
   *  problem where a freed object's memory still looks like an 
   *  object, and the routine tries to delete it again.
   *
   *  This routine must take care that i matches e_message_ptr.
   */

 cleanup:

  if (IS_OBJECT(op1_subexpr_result) &&
      (op1_subexpr_result -> scope & SUBEXPR_CREATED_RESULT)) {
    op1_subexpr_result -> scope &= ~SUBEXPR_CREATED_RESULT;
    (void)__objRefCntDec (OBJREF(op1_subexpr_result));
    if (op1_subexpr_result -> nrefs <= 0) {
      __ctalkDeleteObjectInternal (op1_subexpr_result);
      if (subexpr_result == op1_subexpr_result)
	subexpr_result = NULL;
      op1_subexpr_result = NULL;
    } else {
      __ctalkDeleteObjectInternal (op1_subexpr_result);
    }
  }
  __delete_operand_result (OBJREF(op2_subexpr_result), 
			   OBJREF(subexpr_result));

  for (i = p -> msg_frame_start; i >= p -> msg_frame_top; i--) {
    n = e_messages[i];
    if (n -> attrs & RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) {
      cvar_alias_rcvr_created_here (e_messages, i, p -> msg_frame_top,
			       n -> obj, result_obj);
      n -> obj = NULL;
    }
    if (IS_OBJECT (n -> obj)) {
      if (n -> attrs & RT_TOK_OBJ_IS_ARG_W_FOLLOWING_EXPR) {
	for (j = i - 1; j >= p -> msg_frame_top; j--) {
	  if (e_messages[j] -> obj == n -> obj)
	    e_messages[j] -> obj = NULL;
	}
      }
    }
  }

  for (i = p -> msg_frame_top; i <= p -> msg_frame_start; i++) {
    n = e_message_pop ();
    if (!n || !IS_MESSAGE (n))
      continue;
    if (n -> attrs & RT_TOK_HAS_LVAL_PTR_CX) {
      if (IS_OBJECT (n -> obj)) 
	__ctalkSetObjectAttr (n -> obj,
			      n -> obj -> attrs & ~OBJECT_HAS_PTR_CX);
    }
    if (IS_OBJECT (n -> obj)) {
      clean_up_message_objects (e_messages, n, n -> obj,
				result_obj, subexpr_result, i,
				p -> msg_frame_start, FALSE, 
				rt_cleanup_exit_delete);
    }
    if (IS_OBJECT (n -> value_obj)) {
      clean_up_message_objects (e_messages, n, n -> value_obj,
				result_obj, subexpr_result, i,
				p -> msg_frame_start, TRUE, 
				rt_cleanup_exit_delete);
			      
    }
    reuse_message (n);
  }

  --expr_parser_lvl;
  p = __pop_expr_parser ();
  /*
   *  If not called by __ctalk_arg; e.g., we don't need local vars
   *  to persist for another method and can delete them immediately.
   *  Otherwise, wait until __ctalk_arg_cleanup ().
   */
  if (!is_arg_expr)
    delete_extra_local_objects (p);
  else 
    save_e_methods (p);

  __delete_expr_parser (p);

  if (IS_OBJECT(subexpr_result) && (subexpr_result -> nrefs == 0)) {
    if (subexpr_result != result_obj) {
      __ctalkDeleteObjectInternal (subexpr_result);
    } else {
      if (subexpr_result -> __o_p_obj && 
	  (subexpr_result -> __o_p_obj != result_obj)) {
	__ctalkDeleteObjectInternal (subexpr_result);
      } else {
	if (result_obj -> __o_p_obj && 
	    (result_obj -> __o_p_obj != subexpr_result)) {
	  __ctalkDeleteObjectInternal (subexpr_result);
	}
      }
    }
  } else {
    if (IS_OBJECT(subexpr_result)) {
      /*
       *  One default reference left over.
       */
      if ((subexpr_result -> scope & 
	   (CREATED_PARAM|SUBEXPR_CREATED_RESULT)) &&
	  (subexpr_result -> nrefs <= 1) && 
	  (subexpr_result != result_obj)) {
	__ctalkDeleteObject (subexpr_result);
      }
    }
  }

  if (IS_OBJECT(result_obj)) {
    /* This is how we handle even an initial postfix operation
       for *String Class* (where i is still I_UNDEF). */
    if (need_postfix_fetch_update () && 
	result_obj -> __o_class == rt_defclasses -> p_string_class) {
      if (!(result_obj -> attrs & OBJECT_IS_I_RESULT) &&
	  (IS_VARTAG(result_obj -> __o_vartags) &&
	   IS_VARENTRY (result_obj -> __o_vartags -> tag))) {
	/* i.e, on the first postfix, the delared object's value is
	   correct. */
	postfixed_object = result_obj;
	result_obj = create_param (result_obj -> __o_name,
				   NULL, result_obj -> __o_class,
				   e_messages, e_message_ptr);
	__objRefCntSet (OBJREF(result_obj), 1);
	__ctalkSetObjectValueVar (result_obj, postfixed_object -> __o_value);
	__ctalkRegisterUserObject (result_obj);
	make_postfix_current (postfixed_object);
      } else {
	make_postfix_current (postfixed_object);
      }
      clear_postfix_fetch_update ();
    }
  }

  return result_obj;
}

/*
 *   These operators are all binary, so the function does not need to
 *   check their context.
 */

OBJECT *_rt_math_op (MESSAGE_STACK messages, int op_ptr, int stack_start, 
		int stack_end) {

  int op1_ptr, op2_ptr;
  OBJECT *(*cfunc)(), *result_object, *rcvr_class_object;
  METHOD *__caller_op_method, *op_method;
  char errmsg[MAXMSG];

  _rt_operands (C_EXPR_PARSER, op_ptr, &op1_ptr, &op2_ptr);
  if ((op1_ptr == ERROR) || (op2_ptr == ERROR)) {
    if (expr_parsers[expr_parser_ptr+1]) {
      if (expr_parsers[expr_parser_ptr+1] -> expr_str) {
	_warning ("Warning: rt_math_op (): Invalid operand.\n");
	_warning ("Warning: In expression:\n");
	_warning ("\t%s\n", expr_parsers[expr_parser_ptr+1] -> expr_str);
	__warning_trace ();
      } else {
	_warning ("rt_math_op (): Invalid operand.\n");
      }
    } else {
      _warning ("Warning: rt_math_op (): Invalid operand.\n");
      _warning ("Warning: Operator:\n");
      _warning ("\t%s\n", M_NAME(messages[op_ptr]));
      __warning_trace ();
    }
    return NULL;
  }

  if (!IS_OBJECT(messages[op1_ptr] -> obj)) {
    if (!IS_OBJECT(messages[op1_ptr] -> value_obj)) {
      if (M_TOK(messages[op1_ptr]) == CLOSEPAREN) {
	int prev;
	/* If an operand result is NULL, this may not be filled in,
	   so we'll do a fixup. */
	prev = prev_msg (messages, op1_ptr);
	while (M_TOK(messages[prev]) == CLOSEPAREN)
	  --prev;
	if (IS_OBJECT(messages[prev] -> obj)) {
	  messages[op1_ptr] -> obj = messages[prev] -> obj;
	} else {
	  _warning ("rt_math_op (): Invalid operand object.\n");
	  return NULL;
	}
      } else {
	_warning ("rt_math_op (): Invalid operand object.\n");
	return NULL;
      }
    }
  }
  /*
   *  If the class of the message's object is, "Expr,", try to get the
   *  class from the, "value," instance variable.
   */

  if (M_VALUE_OBJ(messages[op1_ptr]) -> __o_class == 
      rt_defclasses -> p_expr_class) {
    OBJECT *value_var;
    if ((value_var = M_VALUE_OBJ(messages[op1_ptr]) -> instancevars)
	== NULL) {
      _warning ("rt_math_op: %s undefined value.\n", 
		M_VALUE_OBJ (messages[op1_ptr]) -> __o_name);
      return NULL;
    }
    
    if ((rcvr_class_object = value_var ->  __o_class) == NULL) {
      _warning ("rt_math_op: Undefined class %s.\n", value_var -> CLASSNAME);
      return NULL;
    }

  } else {
    if ((rcvr_class_object = M_VALUE_OBJ (messages[op1_ptr]) -> __o_class)
	== NULL) {
      _warning ("rt_math_op: Undefined class %s.\n", 
		M_VALUE_OBJ (messages[op1_ptr]) -> CLASSNAME);
      return NULL;
    }
  }

  /* Should be 1 arg needed, if we need to put in stricter arg
     checking for math expressions. */
  if ((op_method = 
       __ctalkGetInstanceMethodByName (M_VALUE_OBJ (messages[op1_ptr]), 
				       messages[op_ptr] -> name, TRUE,
				       ANY_ARGS)) 
      == NULL) {


    if ((op_method = 
	 __ctalkGetInstanceMethodByName (M_VALUE_OBJ (messages[op2_ptr]), 
					 messages[op_ptr] -> name, TRUE,
					 ANY_ARGS)) 
	== NULL) {

      strcpy (errmsg, 
	      math_op_does_not_understand_error (messages,
						 op1_ptr,
						 op_ptr,
						 stack_start,
						 stack_end));
      __ctalkExceptionInternal (messages[op_ptr], undefined_method_x, errmsg,0);
      return NULL;
    }
  }
  
  __ctalk_receiver_push (M_VALUE_OBJ (messages[op1_ptr]));
  __ctalk_arg_push (M_VALUE_OBJ (messages[op2_ptr]));
  op_method -> args[0] = 
    __ctalkCreateArgEntryInit (M_VALUE_OBJ(messages[op2_ptr]));
  (void)__objRefCntInc (OBJREF (op_method -> args[0] -> obj));
  ++op_method -> n_args;

  __caller_op_method = op_method;
      
  __save_rt_info (M_VALUE_OBJ(messages[op1_ptr]), rcvr_class_object, NULL, 
		  op_method, op_method->cfunc, False);

  cfunc = op_method -> cfunc;
  result_object = (*cfunc) ();

  /*
   *  If a method has a return statement like, "return self,"
   *  then we are likely to get back the, "value," instance variable
   *  of the receiver.  Check if the result is an instance variable
   *  of the receiver, and if so, make the receiver the result
   *  object.
   */

  if (IS_VALUE_INSTANCE_VAR(result_object))
    result_object = result_object -> __o_p_obj;

  op_method = __caller_op_method;

  __ctalk_arg_pop ();
  (void)__objRefCntDec (OBJREF (op_method -> args[0] -> obj));
  __ctalkDeleteArgEntry (op_method -> args[0]);
  op_method -> args[0] = NULL;
  op_method -> n_args = 0;
  __ctalk_receiver_pop ();

  __restore_rt_info ();

  op_method = NULL;

  ++e_messages[op_ptr] -> evaled;

  return result_object;
}

OBJECT *_rt_unary_math_op (EXPR_PARSER *p, int op_ptr) { 

  int op1_ptr;
  OBJECT *result_object;
  OBJECT *value_var;

  _rt_unary_operand (p, op_ptr, &op1_ptr);
#if 0
  _rt_unary_operand (p -> m_s, op_ptr, &op1_ptr, p -> msg_frame_top,
		     p -> msg_frame_start);
#endif  

  if (op1_ptr == ERROR) {
    _warning ("_rt_unary_math_op (): Invalid operand.\n");
    return NULL;
  }

  if ((value_var = M_VALUE_OBJ(p -> m_s[op1_ptr]) ->  instancevars)
      == NULL) {
    _warning ("_rt_unary_math_op: %s undefined value.\n", 
	      M_VALUE_OBJ (p -> m_s[op1_ptr]) -> __o_name);
    return NULL;
  }
    
  result_object = _rt_unary_math (p -> m_s, op_ptr, value_var);

  ++(p -> m_s[op_ptr] -> evaled);

  return result_object;
}

/*
 *  Set the value of an expression.  Check the values of the 
 *  operand token(s) objects and value objects to determine
 *  the limits of the operands.  If the operands have a value
 *  object already, then clean it up if it is temporary.
 */

int _set_expr_value (MESSAGE_STACK messages, int stack_start, int stack_end, 
		     int op_ptr, OBJECT *value_object) {
  int i, op1_ptr, op2_ptr;
  int op1_start = stack_start,   /* Avoid warnings. */
    op2_end = stack_end;

  _rt_operands (C_EXPR_PARSER, op_ptr, &op1_ptr, &op2_ptr);
  
  if ((op1_ptr == ERROR) || (op2_ptr == ERROR)) {
    if (expr_parsers[expr_parser_ptr+1]) {
      if (expr_parsers[expr_parser_ptr+1] -> expr_str) {
	_warning ("Warning: __set_expr_value (): Invalid operand.\n");
	_warning ("Warning: In expression:\n");
	_warning ("\t%s\n", expr_parsers[expr_parser_ptr+1] -> expr_str);
	__warning_trace ();
      } else {
	_warning ("__set_expr_value (): Invalid operand.\n");
      }
    } else {
      _warning ("__set_expr_value (): Invalid operand.\n");
    }
    return ERROR;
  }

  if (IS_OBJECT (messages[op1_ptr] -> value_obj)) {
    for (i = op1_ptr; i <= stack_start; i++) {
      if ((messages[i] -> value_obj == messages[op1_ptr] -> value_obj) ||
	  (messages[i] == messages[op1_start]->receiver_msg))
	op1_start = i;
    }
    _cleanup_temporary_objects (messages[op1_ptr] -> value_obj, value_object, NULL,
				rt_cleanup_null);
  } else {
    if (M_TOK(messages[op1_ptr]) == CLOSEPAREN) {
      if ((op1_start = __ctalkMatchParenRev (messages, op1_ptr, stack_start))
	== ERROR) {
	op1_start = op1_ptr;
      }
    } else {
      op1_start = op1_ptr;
    }
  }

  if (IS_OBJECT (messages[op2_ptr] -> value_obj)) {
    for (i = op2_ptr; i >= stack_end; i--) {
      if (messages[i] -> value_obj == messages[op2_ptr] -> value_obj)
	op2_end = i;
    }
    _cleanup_temporary_objects (messages[op2_ptr] -> value_obj, value_object, NULL,
				rt_cleanup_null);
  } else {
    op2_end = op2_ptr;
  }

  for (i = op1_start; i >= op2_end; i--)
    messages[i] -> value_obj = value_object;

  return SUCCESS;
}

void _set_unary_expr_value (EXPR_PARSER *p, int op_ptr,
			   OBJECT *value_object,
			   OBJECT **subexpr_result) {
  int i, operand_ptr;
  int operand_end = p -> msg_frame_top;   /* Avoid a warning. */
  int all_objects_deleted, operand_value_is_subexpr_result;

  _rt_unary_operand (p, op_ptr, &operand_ptr);
  if (IS_OBJECT (p -> m_s[operand_ptr] -> value_obj)) {
    for (i = operand_ptr; i >= p -> msg_frame_top; i--) {
      if (p -> m_s[i] -> value_obj == p -> m_s[operand_ptr] -> value_obj)
	operand_end = i;
    }
    if (p -> m_s[operand_ptr]->value_obj == *subexpr_result) {
      operand_value_is_subexpr_result = TRUE;
    } else {
      operand_value_is_subexpr_result = FALSE;
    }
    _cleanup_temporary_objects_all_instances
      (p -> m_s[operand_ptr]->value_obj, rt_cleanup_null,
       C_EXPR_PARSER, operand_ptr, &all_objects_deleted);
  } else {
    operand_end = operand_ptr;
    operand_value_is_subexpr_result = FALSE;
  }

  if (all_objects_deleted && operand_value_is_subexpr_result)
    *subexpr_result = NULL;

  for (i = op_ptr; i >= operand_end; i--)
    p -> m_s[i] -> value_obj = value_object;
}

int __set_left_operand_value (MESSAGE_STACK messages, int ptr, 
			      OBJECT *val_obj) {
  int i, stack_start;
  OBJECT *prev_val_obj;

  stack_start = __rt_get_stack_top (messages);

  prev_val_obj = messages[ptr] -> value_obj;

  if (IS_OBJECT (prev_val_obj)) {
    val_obj -> scope = prev_val_obj -> scope;
    val_obj -> nrefs = prev_val_obj -> nrefs;
  }

  _cleanup_temporary_objects (prev_val_obj, val_obj, NULL, rt_cleanup_null);

  for (i = ptr; i <= stack_start; i++) {
    if (messages[i] -> value_obj != prev_val_obj) {
      prev_val_obj = messages[i] -> value_obj;
      _cleanup_temporary_objects (messages[i] -> value_obj, val_obj, NULL,
				  rt_cleanup_null);
    }
    messages[i] -> value_obj = val_obj;
  }

  return SUCCESS;
}

/*
 *  This should occur only for single-token CVARs, at least 
 *  for now.
 */
int __set_unary_prefix_expression (MESSAGE_STACK messages, int start,
				   int end, int stack_start, OBJECT *obj) {
  int i, prev_tok_idx;
  if ((prev_tok_idx = prev_msg (messages, start)) != ERROR) {
    for (i = prev_tok_idx; i >= end; i--) {
      if (messages[i] -> obj != obj)
	messages[i] -> obj = obj;
    }
  }
  return SUCCESS;
}

int __set_aggregate_obj (MESSAGE_STACK messages, int start, int end,
			       OBJECT *obj) {
  int i;
  SUBSCRIPT subscripts[MAXARGS];
  int n_subscripts, nth_subscript;
  char **array_ptr, *subscript_ptr, *element_ptr, *expr_name;
  OBJECT *subscript_eval_result, *expr_obj_out;

  /* If we have a char **, dereference it if the expression
     that eval_expr received contains any subscripts. */
  if (obj -> attrs & OBJECT_VALUE_IS_C_CHAR_PTR_PTR) {
    parse_subscript_expr (messages, start, end, subscripts, &n_subscripts);

    if (n_subscripts == 1) {
      if (obj -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
	array_ptr = (char **)*(uintptr_t *)obj -> __o_value;
      } else {
	array_ptr = generic_ptr_str (obj -> __o_value);
      }
      /* we don't need to increment the array pointer to the nth
	 element here - the subscript increment has already been calculated
	 in the register_c_method_arg call.  */
      element_ptr = strdup (*array_ptr);
      expr_name = collect_tokens (messages, start, end);

      expr_obj_out = create_object_init_internal
	(expr_name, rt_defclasses -> p_string_class, obj -> scope,
	 element_ptr);
      __ctalkDeleteObject (obj);
      
      __xfree (MEMADDR(element_ptr));
      __xfree (MEMADDR(expr_name));
      for (i = start; i >= end; i--) {
	messages[i] -> obj = expr_obj_out;
	messages[i] -> attrs |= RT_CVAR_AGGREGATE_TOK;
      }
    } else {
      for (i = start; i >= end; i--) {
	if (messages[i] -> obj != obj)
	  messages[i] -> obj = obj;
	messages[i] -> attrs |= RT_CVAR_AGGREGATE_TOK;
      }
    }
  } else {
    for (i = start; i >= end; i--) {
      if (messages[i] -> obj != obj)
	messages[i] -> obj = obj;
      messages[i] -> attrs |= RT_CVAR_AGGREGATE_TOK;
    }
  }

  return SUCCESS;
}

int __ctalkMatchParen (MESSAGE_STACK messages, int this_message, 
			 int end_ptr) {

  int i, parens = 0;
  MESSAGE *m;

  for (i = this_message; i >= end_ptr; i--) {
    m = messages[i];
    if (m && IS_MESSAGE (m)) {
      switch (m -> tokentype)
	{
	case OPENPAREN:
	  ++parens;
	  break;
	case CLOSEPAREN:
	  --parens;
	  break;
	default:
	  break;
	}

      if (!parens)
	return i;
    }
  }
  _warning ("Mismatched parentheses.\n");
  return -1;
}

int __ctalkMatchParenRev (MESSAGE_STACK messages, int this_message, 
			 int start_ptr) {

  int i, parens = 0;
  MESSAGE *m;

  for (i = this_message; i <= start_ptr; i++) {
    m = messages[i];
    if (m && IS_MESSAGE (m)) {
      switch (m -> tokentype)
	{
	case OPENPAREN:
	  --parens;
	  break;
	case CLOSEPAREN:
	  ++parens;
	  break;
	default:
	  break;
	}

      if (parens == 0)
	return i;
    }
  }
  _warning ("Mismatched parentheses.\n");
  return -1;
}

/*
 *  Check the exception and the message token type and create
 *  a NULL object of the appropriate class.  Default to an
 *  Integer object.
 */

OBJECT *exception_null_obj (MESSAGE_STACK messages, int x_msg_ptr) {

  OBJECT *o;
  I_EXCEPTION *x_list;
  int prev_msg_ptr;

  if ((x_list = __get_x_list ()) == NULL)
    return NULL;

  o = NULL;
  

  switch (x_list -> _exception)
    {
    case no_x:
    case file_is_directory_x:
    case mismatched_paren_x:
    case undefined_param_class_x:
    case parse_error_x:
    case undefined_class_x:
    case self_without_receiver_x:
    case undefined_label_x:
    case undefined_type_x:
    case undefined_receiver_x:
      break;
    case undefined_method_x:
      /*
       *  Should occur on a method message, so scan back to 
       *  the previous message.  Create an object of the 
       *  receiver class that evaluates to False.  If there's 
       *  no receiver, create an Integer object that evaluates 
       *  to False.
       */
      if ((prev_msg_ptr = 
	   prev_msg (e_messages, x_msg_ptr)) == ERROR) {
	o = create_object_init_internal
	  ("result", rt_defclasses -> p_integer_class, LOCAL_VAR, "0");
      } else {
	if (IS_OBJECT (messages[prev_msg_ptr] -> obj)) {
	  o = __ctalkCreateObjectInit ("result", 
				       messages[prev_msg_ptr] -> 
				       obj -> CLASSNAME,
				       _SUPERCLASSNAME(messages[prev_msg_ptr]
						      ->obj),
				       LOCAL_VAR, "0");
	} else {
	  o = create_object_init_internal
	    ("result", rt_defclasses -> p_integer_class, LOCAL_VAR, "0");
	}
      }
      break;
    default:
      break;
    }
  return o;
}

/*
 *  If the method returns NULL, create an
 *  object that evaluates to False.
 */
OBJECT *null_result_obj (METHOD *m, int scope) {

  OBJECT *o, *return_class;

  if (!m || !strcmp (m -> returnclass, "Any")) {
    o = create_object_init_internal
      (NULLSTR, rt_defclasses -> p_integer_class, scope, "0x0");
  } else {
    if ((return_class = __ctalkGetClass (m -> returnclass)) == NULL) {
      _warning ("Undefined return class %s.\n",
		m -> returnclass);
      o = create_object_init_internal
	(NULLSTR, rt_defclasses -> p_integer_class, scope, "0x0");
    } else {
      o = __ctalkCreateObjectInit (NULLSTR, return_class -> __o_name,
				   _SUPERCLASSNAME(return_class),
				   scope, "0x0");
    }
  }

  o -> attrs |= OBJECT_IS_NULL_RESULT_OBJECT;
  o -> instancevars -> attrs |= OBJECT_IS_NULL_RESULT_OBJECT;

  return o;
}

/* As above, but it takes the object's class from the receiver
   object. */
static OBJECT *null_result_obj_2 (METHOD *m, OBJECT *rcvr, int scope) {

  OBJECT *o, *return_class;

  o = create_object_init_internal (NULLSTR, rcvr -> __o_class,
				   scope, "0x0");
#if 0
  if (!m || !strcmp (m -> returnclass, "Any")) {
    o = create_object_init_internal
      (NULLSTR, rt_defclasses -> p_integer_class, scope, "0x0");
  } else {
    if ((return_class = __ctalkGetClass (m -> returnclass)) == NULL) {
      _warning ("Undefined return class %s.\n",
		m -> returnclass);
      o = create_object_init_internal
	(NULLSTR, rt_defclasses -> p_integer_class, scope, "0x0");
    } else {
      o = __ctalkCreateObjectInit (NULLSTR, return_class -> __o_name,
				   _SUPERCLASSNAME(return_class),
				   scope, "0x0");
    }
  }
#endif  

  o -> attrs |= OBJECT_IS_NULL_RESULT_OBJECT;
  o -> instancevars -> attrs |= OBJECT_IS_NULL_RESULT_OBJECT;

  return o;
}


/*
 *  When called by eval_expr (), we can be reasonably certain
 *  of the context that, "self," appears in.  If it appears
 *  in a method, then we can return the rtinfo () receiver
 *  object.  If, "self," appears in a function, then it
 *  should be as an argument to a method, and refers to 
 *  the receiver.  So a case like,
 *
 *    subDir = path substring 1, self length - 1;
 *
 *  should be evaluated as:
 *
 *    subDir =        path substring 1, self length - 1;
 *    ^      ^        ^
 *    rcvr   method   arg
 *
 *                    path substring 1, self length - 1;
 *                    ^    ^
 *                    rcvr args
 *    rcvr_ptr
 *    + 1
 *
 *  A case like,
 *
 *    subDir =    self substring 1, self length - 1;
 *    ^           ^
 *    rcvr_ptr    rcvr_ptr  
 *    + 1         + 2 (the method's receiver,
 *                     and also rtinfo.rcvr_obj.)
 */

OBJECT *self_expr (METHOD *method, int is_arg_expr) {

  OBJECT *rt_rcvr;

  if (!method) {

    if (is_constant_rcvr == True)
      return __ctalk_receivers[__ctalk_receiver_ptr + 1];
    if (is_arg_expr == True) {
      if ((rt_rcvr = rtinfo.rcvr_obj) != NULL)
	return rt_rcvr;
      else
	return __ctalk_receivers[__ctalk_receiver_ptr + 1];
    }
  }
  return NULL;
}

/*
 *  The subexpression receiver class is the class of the 
 *  first constant or label (i.e., not an operator or
 *  whitespace) in the subexpression.
 */

static OBJECT *get_subexpr_rcvr (MESSAGE_STACK messages,
				  int start, int end,
				  int *subexpr_scope) {
  int i;
  OBJECT *subexpr_rcvr = NULL;
  
  for (i = start, subexpr_rcvr = NULL; 
       (i >= end) && ! subexpr_rcvr; i--) {
    if (!IS_C_OP (messages[i] -> tokentype) && 
	(messages[i] -> tokentype != NEWLINE) &&
	(messages[i] -> tokentype != WHITESPACE) &&
	messages[i] -> obj) {

      /* 
       *  If the receiver's class is, "Expr," then
       *  try to get the class name from the, "value,"
       *  instance variable.
       */

      if (messages[i] -> obj -> __o_class == rt_defclasses -> p_expr_class) {
	OBJECT *value_var;
	if ((value_var = messages[i] -> obj -> instancevars) == NULL)
	  _warning ("eval_expr: %s: undefined value.\n",messages[i] -> name);
	else
	  subexpr_rcvr = value_var -> __o_class;
	*subexpr_scope = value_var -> scope;
      } else {
	subexpr_rcvr = M_VALUE_OBJ(messages[i]);
	*subexpr_scope = messages[i] -> obj -> scope;
      }
    }
  }

  return subexpr_rcvr;
}

/*
 *  Set the value_obj of the subexpression tokens,
 *  and the obj if there is no object for the token.
 */
static void set_subexpr_obj (MESSAGE_STACK messages, int start, int end,
			     OBJECT *subexpr_result) {
  int i;
  for (i = start; i >= end; --i) {
    if (!IS_OBJECT (messages[i] -> obj)) {
      messages[i] -> obj = subexpr_result;
    } else {
      if (messages[i] -> obj -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) {
	OBJECT *__v_tmp = messages[i] -> obj;
	clear_expr_obj (messages, start, end,
			messages[i] -> obj);
	__ctalkDeleteObject (__v_tmp);
	messages[i] -> obj = subexpr_result;
      }
    }
    if (IS_OBJECT (messages[i] -> value_obj)) {
      /*
       *  Check for temporary value objects before replacing them.
       */
	if (is_temporary_i_param (messages[i] -> value_obj)) {
	  OBJECT *__v_tmp = messages[i] -> value_obj;
	  clear_expr_value_obj (messages, start, end,
				messages[i] -> value_obj);
	  __ctalkDeleteObject (__v_tmp);
      }
    }
    messages[i] -> value_obj = subexpr_result;
    messages[i] -> evaled += 2;
  }
}

/*
 *   Classes for basic C types should be parsed by
 *   the time the expression is evaluated.  If not,
 *   issue a warning and continue anyway.
 *
 *   TO DO - Decide if we need a mechanism to load
 *   basic C type classes on the fly.  For now, 
 *   only issue warnings if debugging is enabled -
 *   DEBUG_DYNAMIC_C_ARGS, like all other debugging
 *   #defines, is in include/ctalk.h.
 *
 *   The function assumes that m -> obj contains the 
 *   basic object with the value printed in m -> value.  
 *   This function does not handle PTR_T value types.
 */

/*
 *  Call with idx pointing to the opening parenthesis of the
 *  expression.
 */

OBJECT *eval_subexpr (MESSAGE_STACK messages, int idx, int is_arg_expr) {

  int i = idx,                   /* Avoid warnings. */
    subexpr_scope = LOCAL_VAR,
    stack_top,
    subexpr_start,
    subexpr_end,
    matching_paren_ptr,
    n_subexpr_toks;
  char token_buf[MAXMSG];
  OBJECT *subexpr_rcvr,
    *subexpr_result, *var;
  METHOD *method = NULL;        /* Avoid a warning. */
  int post_expr_ptr;
  MESSAGE *m_next;

  stack_top = e_message_ptr;

  if (messages[idx] -> evaled && M_VALUE_OBJ (messages[idx]))
    return M_VALUE_OBJ (messages[idx]);

  if ((matching_paren_ptr = expr_match_paren (C_EXPR_PARSER, idx)) == ERROR) {
    __ctalkExceptionInternal (messages[i], mismatched_paren_x,
			      messages[i] -> name,0);
    return exception_null_obj (e_messages, idx);
  }

  /* empty parens */
  if ((idx - matching_paren_ptr) == 1)
    return NULL;

  /* one token within the parens - check if it's already evaled -
     if so, just return it */
  n_subexpr_toks = 0;
  for (i = idx - 1; i > matching_paren_ptr; i--) {
    if (M_ISSPACE(e_messages[i]))
      continue;
    ++n_subexpr_toks;
    if (n_subexpr_toks == 1) {
      if (IS_OBJECT(e_messages[i] -> obj) &&
	  (e_messages[i] -> evaled > 0)) {
	subexpr_result = e_messages[i] -> obj;
      }
    } else if (n_subexpr_toks > 1) {
      break;
    }
  }
  if (n_subexpr_toks == 1) {
    for (i = idx; i >= matching_paren_ptr; i--) {
      e_messages[i] -> obj = subexpr_result;
      e_messages[i] -> evaled += 2;
    }
    return subexpr_result;
  }

  subexpr_start = next_msg (e_messages, idx);
  subexpr_end = prev_msg (e_messages, matching_paren_ptr);
  toks2str (messages, subexpr_start, subexpr_end, token_buf);

  subexpr_rcvr = get_subexpr_rcvr (messages, idx, matching_paren_ptr,
				   &subexpr_scope);

  if (subexpr_rcvr) {
    subexpr_result = eval_expr (token_buf, subexpr_rcvr -> __o_class, 
				method, subexpr_rcvr, subexpr_scope,
				is_arg_expr);
  } else {
    __ctalkExceptionInternal (messages[i], undefined_receiver_x,
			      messages[i] -> name,0);
    return NULL;
  }

  set_subexpr_obj (messages, idx, matching_paren_ptr,
		   subexpr_result);

  /* In case we have a LABEL after the closing paren,
     check if it's a method for the subexpr_result object. */
  if ((post_expr_ptr = next_msg (messages, matching_paren_ptr)) != ERROR) {
    m_next = messages[post_expr_ptr];
    if (M_TOK(m_next) == LABEL) {
      if (__ctalk_isMethod_2 (M_NAME(m_next),
			    messages, matching_paren_ptr,
			    idx)) {
	m_next -> tokentype = METHODMSGLABEL;
	m_next -> receiver_msg = messages[matching_paren_ptr];
	m_next -> receiver_obj =
	  M_VALUE_OBJ(messages[matching_paren_ptr]);
      } else if ((var = __ctalkGetInstanceVariable 
		  (M_VALUE_OBJ(messages[matching_paren_ptr]),
		   M_NAME(m_next), FALSE)) != NULL) {
	if (IS_OBJECT(m_next->obj)) {
	  m_next -> value_obj = var;
	} else {
	  m_next -> obj = var;
	}
	m_next -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
	m_next -> receiver_msg = messages[matching_paren_ptr];
	m_next -> receiver_obj = 
	  M_VALUE_OBJ(messages[matching_paren_ptr]);
      } else if ((var = __ctalkGetClassVariable
		  (M_VALUE_OBJ(messages[matching_paren_ptr]),
		   M_NAME(m_next), FALSE)) != NULL) {
	if (IS_OBJECT(m_next->obj)) {
	  m_next -> value_obj = var;
	} else {
	  m_next -> obj = var;
	}
	m_next -> attrs |= RT_OBJ_IS_CLASS_VAR;
	m_next -> receiver_msg = messages[matching_paren_ptr];
	m_next -> receiver_obj = 
	  M_VALUE_OBJ(messages[matching_paren_ptr]);
      }
    } else if (M_TOK(m_next) == METHODMSGLABEL) {
      /* Here we have to check if we need a fixup, because we simply
	 set a label to METHODMSGLABEL after a closing paren in
	 the first loop of eval_expr, before we knew what the
	 actual receiver was. Now we can check exactly because
	 we've evaluated the receiver (or parent object) 
	 expression. */
      if ((var = __ctalkGetInstanceVariable 
	   (M_VALUE_OBJ(messages[matching_paren_ptr]),
	    M_NAME(m_next), FALSE)) != NULL) {
	if (IS_OBJECT(m_next->obj)) {
	  m_next -> value_obj = var;
	} else {
	  m_next -> obj = var;
	}
	m_next -> tokentype = LABEL;
	m_next -> attrs |= RT_OBJ_IS_INSTANCE_VAR;
	m_next -> receiver_msg = messages[matching_paren_ptr];
	m_next -> receiver_obj = 
	  M_VALUE_OBJ(messages[matching_paren_ptr]);
      } else if ((var = __ctalkGetClassVariable
		  (M_VALUE_OBJ(messages[matching_paren_ptr]),
		   M_NAME(m_next), FALSE)) != NULL) {
	if (IS_OBJECT(m_next->obj)) {
	  m_next -> value_obj = var;
	} else {
	  m_next -> obj = var;
	}
	m_next -> tokentype = LABEL;
	m_next -> attrs |= RT_OBJ_IS_CLASS_VAR;
	m_next -> receiver_msg = messages[matching_paren_ptr];
	m_next -> receiver_obj = 
	  M_VALUE_OBJ(messages[matching_paren_ptr]);
      } else {
	m_next -> receiver_msg = messages[matching_paren_ptr];
	m_next -> receiver_obj =
	  M_VALUE_OBJ(messages[matching_paren_ptr]);
      }
    } else if (M_TOK(m_next) == EQ) {
      /*
       *  In this case we do a fixup if we have an expression like:
       *
       *    (*self instVar) = argVar;
       *
       *  And instVar is a Symbol.  That's because the semantics
       *  of Symbol : = require that we evaluate the expression 
       *  as if it were this:
       *
       *    self instVar = argVar;
       *
       *  That is, Symbol : = automatically does the assignment
       *  if the operand is not another Symbol.
       */
      if (M_TOK(messages[subexpr_start]) == MULT) {
	if (messages[subexpr_end] -> attrs & RT_OBJ_IS_INSTANCE_VAR) {
	  if (IS_OBJECT(messages[subexpr_end] -> obj)) {
	    if (IS_OBJECT(messages[subexpr_end] -> obj -> instancevars)) {
	      if (messages[subexpr_end] -> obj -> instancevars -> __o_class
		  == rt_defclasses -> p_symbol_class) {
		if (subexpr_rcvr) {
		  subexpr_start = next_msg (e_messages, subexpr_start);
		  toks2str (messages, subexpr_start, subexpr_end, token_buf);
		  subexpr_result = eval_expr (token_buf, subexpr_rcvr -> __o_class, 
					      method, subexpr_rcvr, subexpr_scope,
					      is_arg_expr);
		}
	      }
	    }
	  }
	}
      }
    }
  }

  return subexpr_result;
}

/*
 *  For expressions like ++*<varname>.
 *
 *  Treats the expression the same as ++(*<varname>), i.e.,
 *  as if we called eval_subexpr, but without the parens.
 */
OBJECT *eval_rassoc_rcvr_as_subexpr (MESSAGE_STACK messages, int idx,
				     int is_arg_expr,
				     int *pfx_expr_end_out) {

  int i = idx,                   /* Avoid warnings. */
    subexpr_scope = LOCAL_VAR,
    stack_top,
    subexpr_start,
    subexpr_end,
    n_subexpr_toks;
  char token_buf[MAXMSG];
  OBJECT *subexpr_rcvr,
    *subexpr_result, *var;
  METHOD *method = NULL;        /* Avoid a warning. */
  int post_expr_ptr;

  stack_top = e_message_ptr;

  if (messages[idx] -> evaled && M_VALUE_OBJ (messages[idx]))
    return M_VALUE_OBJ (messages[idx]);

  if ((subexpr_start = next_msg (messages, idx)) == ERROR) {
    return exception_null_obj (e_messages, idx);
  }

  subexpr_end = -1;
  for (i = subexpr_start; messages[i]; --i) {
    if (M_TOK(messages[i]) == LABEL) {
      subexpr_end = i;
      *pfx_expr_end_out = i;
      break;
    }
  }

  if (subexpr_end == -1) {
    return exception_null_obj (e_messages, idx);
  }

  toks2str (messages, subexpr_start, subexpr_end, token_buf);

  subexpr_rcvr = get_subexpr_rcvr (messages, subexpr_start, subexpr_end,
				   &subexpr_scope);

  if (subexpr_rcvr) {
    subexpr_result = eval_expr (token_buf, subexpr_rcvr -> __o_class, 
				method, subexpr_rcvr, subexpr_scope,
				is_arg_expr);
  } else {
    __ctalkExceptionInternal (messages[i], undefined_receiver_x,
			      messages[i] -> name,0);
    return NULL;
  }

  set_subexpr_obj (messages, subexpr_start, subexpr_end,
		   subexpr_result);

  return subexpr_result;
}

/*
 *  For expressions like *<varname>++.
 *
 *  Treats the expression the same as (*<varname>)++, i.e.,
 *  as if we called eval_subexpr, but without the parens.
 */
OBJECT *eval_rassoc_rcvr_as_subexpr_b (MESSAGE_STACK messages,
				       int rcvr_end_idx,
				       int pfx_idx,
				       int is_arg_expr) {

  int subexpr_scope = LOCAL_VAR,
    stack_top,
    subexpr_start,
    subexpr_end,
    n_subexpr_toks;
  char token_buf[MAXMSG];
  OBJECT *subexpr_rcvr,
    *subexpr_result, *var;
  METHOD *method = NULL;        /* Avoid a warning. */
  int post_expr_ptr;

  stack_top = e_message_ptr;

  subexpr_start = pfx_idx;
  subexpr_end = rcvr_end_idx;

  toks2str (messages, subexpr_start, subexpr_end, token_buf);

  subexpr_rcvr = get_subexpr_rcvr (messages, subexpr_start, subexpr_end,
				   &subexpr_scope);

  if (subexpr_rcvr) {
    subexpr_result = eval_expr (token_buf, subexpr_rcvr -> __o_class, 
				method, subexpr_rcvr, subexpr_scope,
				is_arg_expr);
  } else {
    __ctalkExceptionInternal (messages[pfx_idx], undefined_receiver_x,
			      messages[pfx_idx] -> name,0);
    return NULL;
  }

  set_subexpr_obj (messages, subexpr_start, subexpr_end,
		   subexpr_result);

  return subexpr_result;
}

/*
 *  For now, rely on the token type of a function label as
 *  LABEL.  Later on, it may be a good idea to add a check for
 *  a method based on the receiver class.  This depends on
 *  whether the function is within a subexpression, however,
 *  to determine the order in which the tokens are evaluated.
 */

int is_c_fn_syntax (MESSAGE_STACK messages, int fn_label_ptr) {

  if ((messages[fn_label_ptr] -> tokentype == LABEL) &&
      (messages[next_msg (messages, fn_label_ptr)] -> tokentype == OPENPAREN))
    return TRUE;

  return FALSE;
}

/*
 *  These are API-level wrappers for __get_expr_parser_ptr () and
 *  __expr_parser_at ().
 */
int __ctalkGetExprParserPtr (void) {
  return __get_expr_parser_ptr ();
}

EXPR_PARSER *__ctalkGetExprParserAt (int ptr) {
  return __expr_parser_at (ptr);
}

char *expr_text (char *s) {
  s = collect_tokens (e_messages, P_MESSAGES, e_message_ptr + 1);
  return s;
}

OBJECT *current_expression_result (void) {
  if (expr_parser_ptr <= MAXARGS) {
    return expr_parsers[expr_parser_ptr] -> e_result;
  }
  return NULL;
}

void cleanup_expr_parser (void) {
  /* On SPARC/Solaris, can cause a bus error on error 
     exit... needs further checking... */
#if ! defined (__sparc__)
  int i;
  MESSAGE *m;
  for (i = e_message_ptr + 1; i <= P_MESSAGES; i++) {
    if ((m = e_messages[i]) != NULL) {
      if (IS_OBJECT(m -> obj) &&
	  ((m -> obj -> scope & CREATED_PARAM) ||
	   (m -> obj -> scope & TYPECAST_OBJECT)) &&
	  (m -> obj -> nrefs == 0))
	__ctalkDeleteObject (m -> obj);
    }
  }
#endif
}
