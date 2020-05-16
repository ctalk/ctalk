/* $Id: rt_rcvr.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2015, 2017, 2019  
    Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"

OBJECT *__ctalk_receivers[MAXARGS+1];
int __ctalk_receiver_ptr;

extern OBJECT *__ctalk_classes;

extern VARENTRY *__ctalk_dictionary;
extern VARENTRY *__ctalk_last_object;

extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;
extern RT_INFO rtinfo;

static inline void __rcvr_ref_cnt_store (OBJREF_T obj_p, int nrefs) {
  OBJECT *vars, *vars_prev;

  if (IS_OBJECT (*obj_p)) {
    (*obj_p) -> nrefs = nrefs;
    for (vars = (*obj_p) -> instancevars, vars_prev = NULL; 
	 vars != NULL; vars = vars -> next) {
      if (!IS_OBJECT(vars)) {
	if (IS_OBJECT (vars_prev)) vars_prev -> next = NULL;
#ifdef DEBUG_MESSAGES
	fprintf (stderr, 
		 "Warning: __rcvr_ref_cnt_store: Invalid receiver instance variable.\n"
		 "Warning: Receiver \"%s\" (class %s),\n"
		 "Warning: variable \"%s\".\n",
		 (*obj_p) -> __o_name, (*obj_p) -> CLASSNAME, 
		 vars -> __o_name);
	__warning_trace ();
#endif
	return;
      }
      __rcvr_ref_cnt_store (OBJREF (vars), nrefs);
      vars_prev = vars;
    }
  } 
}

int __ctalk_receiver_push (OBJECT *__o) {
  if (!IS_OBJECT (__o))
    return ERROR;

  __ctalk_receivers[__ctalk_receiver_ptr--] = __o;
  return __ctalk_receiver_ptr + 1;

}

int __ctalk_receiver_push_ref (OBJECT *__o) {
  if (!IS_OBJECT (__o))
    return ERROR;

  __ctalk_receivers[__ctalk_receiver_ptr--] = __o;
  __rcvr_ref_cnt_store (OBJREF(__o), __o -> nrefs + 1);

  return __ctalk_receiver_ptr + 1;
}

OBJECT *__ctalk_receiver_pop (void) {

  OBJECT *rcvr;

  if (__ctalk_receiver_ptr == MAXARGS)
    return NULL;

  rcvr = __ctalk_receivers[++__ctalk_receiver_ptr];
  __ctalk_receivers[__ctalk_receiver_ptr] = NULL;
  return rcvr;
}

OBJECT *__ctalk_receiver_pop_deref (void) {
  OBJECT *rcvr;

  if (__ctalk_receiver_ptr == MAXARGS)
    return NULL;

  if (__ctalk_receiver_ptr > MAXARGS) {
    _warning ("__ctalk_receiver_pop_deref: stack underflow\n");
    if (__ctalkGetExceptionTrace ()) {
      __warning_trace ();
    }
    return NULL;
  }

  if (__ctalk_receiver_ptr < 0) {
    _warning ("__ctalk_receiver_pop_deref: stack overflow\n");
    if (__ctalkGetExceptionTrace ()) {
      __warning_trace ();
    }
    return NULL;
  }

  rcvr = __ctalk_receivers[++__ctalk_receiver_ptr];
  __ctalk_receivers[__ctalk_receiver_ptr] = NULL;

  if (IS_OBJECT(rcvr)) {
    __rcvr_ref_cnt_store (OBJREF(rcvr), rcvr -> nrefs - 1);
    return rcvr;
  } else {
    return NULL;
  }
}

void __ctalk_class_initialize (void) {
   __ctalk_receiver_ptr = MAXARGS;
   __setClassLibRead (False);
   initialize_ismethd_hashes ();
   __init_time ();
   init_default_class_cache ();
}

OBJECT *__ctalk_receivers_at (int ptr) {
  return __ctalk_receivers[ptr];
}

int __ctalkGetReceiverPtr (void) {
  return __ctalk_receiver_ptr;
}

bool is_receiver (OBJECT *__o) {
  int i;
  for (i = __ctalk_receiver_ptr + 1; i <= MAXARGS; i++)
    if (__o == __ctalk_receivers[i])
      return true;
  return false;
}

/*
 *  Check both the name and the actual address of the object
 *  on the stack.
 */
bool instancevar_is_receiver (OBJECT *__o) {
  int i;
  OBJECT *var;
  for (i = __ctalk_receiver_ptr + 1; i <= MAXARGS; i++)
    if ((var = 
	 __ctalkGetInstanceVariable (__o, __ctalk_receivers[i] -> __o_name, 
				     FALSE)) != NULL) {
      if (var == __ctalk_receivers[i])
	return true;
    }
  return false;
}

int is_current_receiver (OBJECT *__o) {
  if (__o == __ctalk_receivers[__ctalk_receiver_ptr + 1])
      return TRUE;
  return FALSE;
}


/*
 *  Called for now only from __ctalk_arg ().  It is sufficient
 *  sufficient for single-token constants, and at this point 
 *  is not much more than a convenience function.  
 * 
 *  However, if we want to use, "self," to refer to this receiver,
 *  __ctalkEvalExpr () must be able to figure out that the constant
 *  receiver's object was immediately pushed by __ctalk_arg ().
 *
 *  TO DO - 1. Test with complex expressions.  
 *          2. Figure out a way for __ctalkEvalExpr () to determine
 *          that, "self," refers to a receiver constant without
 *          having to set another state.
 */

extern bool is_constant_rcvr;

OBJECT *__constant_rcvr (char *s) {
  OBJECT *o;
  o = __ctalkEvalExpr (s);
  is_constant_rcvr = True;
  return o;
}

OBJECT *resolve_self (bool is_constant_rcvr, bool is_arg_expr) {

  OBJECT *self_obj;

  if (__call_stack[__call_stack_ptr + 1] -> inline_call) {
    self_obj = rtinfo.rcvr_obj;
  } else if ((is_constant_rcvr == True) || (is_arg_expr == True)) {
    self_obj = self_expr (NULL, is_arg_expr);
  } else {
    self_obj = __ctalk_receivers[__ctalk_receiver_ptr + 2];
  }
  
  if (!IS_OBJECT (self_obj)) {
    _warning ("ctalk: Receiver, \"self,\" not found:\n");
    if (!__ctalkRtGetMethod ()) 
      _warning ("ctalk: \"self,\" Used inside a C function.)\n");
    return null_result_obj (NULL, CREATED_PARAM);
  }

  return self_obj;
}
