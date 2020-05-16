/* $Id: become.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

#include <stdio.h>
#include <string.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

extern VARENTRY *__ctalk_dictionary;
extern VARENTRY *__ctalk_last_object;

extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

extern OBJECT *__ctalk_argstack[MAXARGS+1];
extern int __ctalk_arg_ptr;

extern DEFAULTCLASSCACHE *rt_defclasses;

extern bool is_constant_arg;  /* declared in rt_expr.c */

 int object_become_delete = FALSE;
 int object_become_copy = FALSE;

 /* 
  *  These functions are called from within Object : become,
  *  which is called within an argument block, so the call 
  *  stack looks like this:
  *  Stack ptr + 1 is Object : become.
  *  Stack ptr + 2 is inline block.
  *  Stack ptr + 3 is inline block's map method (or other method) caller.
  *  Stack ptr + 4 is method where the block is declared and map is called 
  *  and a local variable is declared.
  */
 static RT_FN *get_calling_fn_w_block_call (void) { 
   RT_FN *r;
   if ((__call_stack_ptr + 1) >= MAXARGS) return NULL;
   if (__call_stack[__call_stack_ptr+2] && 
       __call_stack[__call_stack_ptr+2]->inline_call) {
     if (__call_stack[__call_stack_ptr+4]) {
       if ((r = __call_stack[__call_stack_ptr+4]->_rt_fn) != NULL)
	 return r;
       else
	 return NULL;
     } else {
       return NULL;
     }
   }
   return (__call_stack[__call_stack_ptr+2]->_rt_fn ? 
	   __call_stack[__call_stack_ptr+2]->_rt_fn :
	   NULL); 
 }

 static METHOD *get_calling_method_w_block_call (void) { 
   METHOD *m;
   int stack_walkup = 2;
   if ((__call_stack_ptr + 1) >= MAXARGS) return NULL;
   if (__call_stack[__call_stack_ptr+stack_walkup] && 
       __call_stack[__call_stack_ptr+stack_walkup]->inline_call) {
     stack_walkup += 2;
     while (__call_stack[__call_stack_ptr+stack_walkup] &&
	    __call_stack[__call_stack_ptr+stack_walkup] -> inline_call)
       stack_walkup += 2;
     if (__call_stack[__call_stack_ptr+stack_walkup]) {
       if ((m = __call_stack[__call_stack_ptr+stack_walkup]->method) != NULL)
	 return m;
       else
	 return NULL;
     } else {
       return NULL;
     }
   }
   return (__call_stack[__call_stack_ptr+2]->method ? 
	   __call_stack[__call_stack_ptr+2]->method :
	   NULL); 
 }

 int __ctalkIsCallersReceiver (void) {
   /* Object itself */
   if (__call_stack[__call_stack_ptr + 1] -> rcvr_obj == 
       __call_stack[__call_stack_ptr + 2] -> rcvr_obj)
     return TRUE;
   return FALSE;
 }

 int __ctalkInstanceVarIsCallersReceiver (void) {
   if (__call_stack[__call_stack_ptr + 1] -> rcvr_obj && 
       __call_stack[__call_stack_ptr+1]->rcvr_obj->__o_p_obj &&
       __ctalkGetInstanceVariable 
       (__call_stack[__call_stack_ptr+1]->rcvr_obj->__o_p_obj,
	__call_stack[__call_stack_ptr+1]->rcvr_obj -> __o_name, FALSE))
     return TRUE;
   return FALSE;
 }

 /*
  *  Look first for an actual object, then the entry declaration
  *  if there is no object.
  */
 static OBJECT *__varlist_match_object (VARENTRY *__v, 
					char *name, char *classname) {
   VARENTRY *__v_2;
   for (__v_2 = __v; __v_2; __v_2 = __v_2 -> next) {
     if (IS_OBJECT(__v_2 -> var_object)) {
       if (!strcmp (__v_2 -> var_object->__o_name, name) && 
	   !strcmp (__v_2 -> var_object->CLASSNAME, classname))
	 return __v_2 -> var_object;
     }
   }
   for (__v_2 = __v; __v_2; __v_2 = __v_2 -> next) {
     if (!strcmp (__v_2 -> var_decl->name, name) && 
	 !strcmp (__v_2 -> var_decl->class, classname))
       return __v_2 -> var_object;
   }
   return NULL;
 }


 OBJECT *__ctalkGetCallingFnObject (char *name, char *classname) {
   RT_FN *fn;
   if ((fn = get_calling_fn_w_block_call ()) != NULL) {
     return __varlist_match_object (fn -> local_objects.vars,
				    name, classname);
   }
   return NULL;
 }

 OBJECT *__ctalkGetCallingMethodObject (char *name, char *classname) {
   METHOD *m;
   if ((m = get_calling_method_w_block_call ()) != NULL)
     return __varlist_match_object (M_LOCAL_VAR_LIST(m), 
				    name, classname);
   return NULL;
 }

 static OBJECT *__receiver_become (OBJECT *__new) {
   OBJECT *old_rcvr;
   object_become_delete = TRUE;
   old_rcvr = __ctalk_receiver_pop_deref ();
   if (old_rcvr -> nrefs == 0) {
     if (!IS_EMPTY_VARTAG(old_rcvr -> __o_vartags)) {
       old_rcvr -> __o_vartags -> tag -> var_object = NULL;
     }
     __ctalkDeleteObjectInternal (old_rcvr);
   }
   __ctalk_receiver_push_ref (__new);
   if (__call_stack[__call_stack_ptr + 1] -> rcvr_obj) {
     __call_stack[__call_stack_ptr + 1] -> rcvr_obj = __new;
     __call_stack[__call_stack_ptr + 1] -> rcvr_class_obj = 
       __new -> __o_class;
   }
   object_become_delete = FALSE;
   return __new;
 }

 static OBJECT *__become_this_object (VARENTRY *v, 
				      OBJECT *old, OBJECT *new) {
   OBJECT *new_copy;
   int old_nrefs;
   object_become_delete = TRUE;
   object_become_copy = TRUE;
   __ctalkCopyObject (OBJREF(new), OBJREF(new_copy));
   __ctalkSetObjectScope (new_copy, old -> scope);
   if (old -> __o_vartags -> tag) {
     new_copy -> __o_vartags -> tag = old -> __o_vartags -> tag;
   }
   old_nrefs = old -> nrefs;
   (void)__receiver_become (new_copy);
   __refObj (OBJREF(v -> var_object), OBJREF(new_copy));
   if (old_nrefs > 1) {  /* Wasn't deleted by __receiver_become */
     if (old -> scope & LOCAL_VAR) {
       if (old -> nrefs <= 1) {
	 if (is_receiver (old)) {
	   int __ptr;
	   for (__ptr = __ctalk_receiver_ptr + 1; __ptr <= MAXARGS; __ptr++){
	     if (__ctalk_receivers[__ptr] == old)
	       __ctalk_receivers[__ptr] = NULL;
	   }
	 }
	 __ctalkDeleteObject (old);
       } else {
	 if (old != v -> orig_object_rec) {
	   /*
	    *  Or even if it is sometimes... watch this.
	    */
	   __ctalkDeleteObject (old);
	 }
       }
     } else {
       __ctalkDeleteObject (old);
     }
     /*
      *  This is still not certain to be absolutely correct,
      *  depending on whether the old object is the varentry's
      *  previous var_object.
      */
     if (v -> orig_object_rec == old) {
       if (IS_OBJECT (old)) {
	 __ctalkDeleteObject (old);
       }
       v -> orig_object_rec = v -> var_object;
     }
   }
   strcpy (v -> var_decl -> class, new_copy -> CLASSNAME);
   object_become_delete = FALSE;
   object_become_copy = FALSE;
   return new_copy;
 }

 static OBJECT *__become_this_instance_variable (VARENTRY *v, 
				      OBJECT *old, OBJECT *new) {
   OBJECT *new_copy;
   int old_nrefs;
   object_become_delete = TRUE;
   __ctalkCopyObject (OBJREF(new), OBJREF(new_copy));
   __ctalkSetObjectScope (new_copy, old -> scope);
   old_nrefs = old -> nrefs;
   if (old_nrefs > 1) {  /* Wasn't deleted by __receiver_become */
     if (old -> scope & LOCAL_VAR) {
       if (old -> nrefs <= 1)
	 __ctalkDeleteObject (old);
     } else {
       if (old -> nrefs <= 0)
	 __ctalkDeleteObjectInternal (old);
     }
   }
   strcpy (new_copy -> __o_name, old -> __o_name);

#ifdef USE_CLASSNAME_STR
   strcpy (new_copy -> __o_classname, old -> __o_classname);
#endif
#ifdef USE_SUPERCLASSNAME_STR
   strcpy (new_copy -> __o_superclassname, _SUPERCLASSNAME(old));
#endif

   new_copy -> __o_class = old -> __o_class;
   new_copy -> __o_superclass = old -> __o_superclass;
   if (old -> prev) {
     old -> prev -> next = new_copy;
     new_copy ->prev = old -> prev;
   }
   if (old -> next) {
     old -> next -> prev = new_copy;
     new_copy -> next = old -> next;
   }
   object_become_delete = FALSE;
   return new_copy;
 }

 OBJECT *__ctalkCallingMethodObjectBecome (OBJECT *__old, OBJECT *__new) {
   METHOD *m;
   VARENTRY *__v;
   if ((m = get_calling_method_w_block_call ()) != NULL) {
     for (__v = M_LOCAL_VAR_LIST(m); __v; __v = __v -> next) {
       if (__v -> var_object == __old) {
	 if (__new) {
	   return __become_this_object (__v, __old, __new);
	 } else {
	   OBJECT *null_object;
	   null_object =
	     create_object_init_internal
	     (NULLSTR, rt_defclasses -> p_integer_class, __old -> scope,
	      NULL);
	   return __become_this_object (__v, __old, null_object);
	 }
       }
     }
   }
   return NULL;
 }

 OBJECT *__ctalkCallingFnObjectBecome (OBJECT *__old, OBJECT *__new) {
   RT_FN *fn;
   VARENTRY *__v;
   if ((fn = get_calling_fn_w_block_call ()) != NULL) {
     for (__v = fn -> local_objects.vars; __v; __v = __v -> next) {
       if (__v -> var_object == __old) {
	 if (__new) {
	   return __become_this_object (__v, __old, __new);
	 } else {
	   OBJECT *null_object;
	   null_object =
	     create_object_init_internal
	     (NULLSTR, rt_defclasses -> p_integer_class, __old -> scope,
	      NULL);
	   return __become_this_object (__v, __old, null_object);
	 }
       }
     }
   }
   return NULL;
 }

 OBJECT *__ctalkCallingReceiverBecome (OBJECT *__old, OBJECT *__new) {
   METHOD *m;
   RT_FN *fn;
   VARENTRY *__v;
   OBJECT *__old_i_var;
   if (__call_stack[__call_stack_ptr + 2] &&
       (__call_stack[__call_stack_ptr + 2] -> rcvr_obj)) {
     if ((m = __call_stack[__call_stack_ptr + 3] -> method) != NULL) {
       for (__v = M_LOCAL_VAR_LIST(m); __v; __v = __v -> next) {
	 if (!strcmp (__v -> var_decl -> name, __old -> __o_name) && 
	     !strcmp (__v -> var_decl -> class, __old -> CLASSNAME)) {
	   if (__v -> var_object -> __o_vartags -> tag)
	     __new -> __o_vartags -> tag = __v;
	   __ctalkDeleteObjectInternal (__v -> var_object);
	   __refObj (OBJREF(__v -> var_object), OBJREF(__new));
	   strcpy (__v -> var_decl -> class, __new -> CLASSNAME);
	   return __new;
	 } else {
	   if ((__old_i_var = 
		__ctalkGetInstanceVariable (__v -> var_object, __old->__o_name,
					    FALSE)) != NULL) {
	     return __become_this_instance_variable (__v, __old, __new);
	   }
	 }
       }
     } else {
       /* Replacing an instance variable. */
       if ((m = __call_stack[__call_stack_ptr + 2] -> method) != NULL) {
	 for (__v = M_LOCAL_VAR_LIST(m); __v; __v = __v -> next) {
	   if ((__old_i_var = 
		__ctalkGetInstanceVariable (__v -> var_object, __old->__o_name,
					    FALSE)) != NULL) {
	     return __become_this_object (__v, __old, __new);
	   }
	 }
       }
         }


    /* Fall through. */
    if ((fn = __call_stack[__call_stack_ptr + 3] -> _rt_fn) != NULL) {
      for (__v = fn -> local_objects.vars; __v; __v = __v -> next) {
	if (__v -> var_object == __old) {
	  return __become_this_object (__v, __old, __new);
	} else {
	  if ((__old_i_var = 
	       __ctalkGetInstanceVariable (__v -> var_object, 
					   __old->__o_name,
					   FALSE)) != NULL) {
	    return __become_this_object (__v, __old, __new);
	  }
	}
      }
    }
  }
  
  return NULL;
}

OBJECT *__ctalkCallingInstanceVarBecome (OBJECT *__old, OBJECT *__new) {
  OBJECT *__old_i_var, *parent_object, *new_copy;
  int rcvr_ptr;
  int old_nrefs;
  object_become_delete = TRUE;
  object_become_copy = TRUE;
  if ((__old_i_var = 
       __ctalkGetInstanceVariable (__old->__o_p_obj,
				   __old->__o_name,
				   FALSE)) != NULL) {
    if ((parent_object = __old_i_var -> __o_p_obj) != NULL) {
      if (__new) {
	__ctalkCopyObject(OBJREF(__new), OBJREF(new_copy));
      } else {
	OBJECT *null_object;
	null_object = create_object_init_internal
	  (NULLSTR, rt_defclasses -> p_integer_class,
	   __old -> scope, NULLSTR);
	__ctalkCopyObject(OBJREF(null_object), OBJREF(new_copy));
      }
      old_nrefs = __old -> nrefs;
      strcpy (new_copy -> __o_name, __old -> __o_name);
#ifdef USE_CLASSNAME_STR
      strcpy (new_copy -> __o_classname, __old -> CLASSNAME);
#endif
#ifdef USE_SUPERCLASSNAME_STR
      strcpy (new_copy -> __o_superclassname, _SUPERCLASSNAME(__old));
#endif
      new_copy -> __o_class = __old -> __o_class;
      new_copy -> __o_superclass = __old -> __o_superclass;
      if (__old -> prev) {
	__old -> prev -> next = new_copy;
	new_copy -> prev = __old -> prev;
      }
      if (__old -> next) {
	__old -> next -> prev = new_copy;
	new_copy -> next = __old -> next;
      }
      (void)__objRefCntInc(OBJREF(new_copy));
      new_copy -> __o_p_obj = __old -> __o_p_obj;
      if (__old -> __o_p_obj && 
	  __old -> __o_p_obj -> instancevars == __old)
	new_copy -> __o_p_obj -> instancevars = new_copy;
      (void)__objRefCntDec(OBJREF(__old));
      --old_nrefs;

      if (__call_stack[__call_stack_ptr+1]->rcvr_obj == __old) {
	__call_stack[__call_stack_ptr+1]->rcvr_obj = new_copy;
	__call_stack[__call_stack_ptr+1]->rcvr_class_obj = new_copy -> __o_class;
      }
      for (rcvr_ptr = __ctalk_receiver_ptr + 1; rcvr_ptr <= MAXARGS;
	   rcvr_ptr++) {
	if (__ctalk_receivers[rcvr_ptr] == __old) {
	  (void)__objRefCntDec(OBJREF(__ctalk_receivers[rcvr_ptr]));
	  --old_nrefs;
	  if (__old -> nrefs == 0) {
	    __ctalk_receivers[rcvr_ptr] = NULL;
	    become_expr_parser_cleanup (__old, new_copy);
	    __ctalkDeleteObject (__old);
	    old_nrefs = -1;
	  }
	  (void)__objRefCntInc(OBJREF(new_copy));
	  __ctalk_receivers[rcvr_ptr] = new_copy;
	}
      }

      if (old_nrefs >= 0) {
	__ctalkDeleteObject (__old);
      }

      object_become_delete = FALSE;
      object_become_copy = FALSE;
      return new_copy;
    }
  }
  object_become_delete = FALSE;
  object_become_copy = FALSE;
  return NULL;
}

OBJECT *__ctalkReceiverReceiverBecome (OBJECT *__new) {
  OBJECT *old_rcvr;
  if (!__new)
    return NULL;
  old_rcvr = __ctalk_receiver_pop_deref ();
  old_rcvr = __ctalk_receiver_pop_deref ();
  if (old_rcvr) {
    if (old_rcvr -> __o_vartags -> tag)
      __new -> __o_vartags -> tag = old_rcvr -> __o_vartags -> tag;
    if (old_rcvr -> nrefs <= 0)
      __ctalkDeleteObject (old_rcvr);
  }

  __ctalk_receiver_push_ref (__new);
  __ctalk_receiver_push_ref (__new);

  if (__call_stack[__call_stack_ptr + 1] -> rcvr_obj) {
    __call_stack[__call_stack_ptr + 1] -> rcvr_obj = __new;
    __call_stack[__call_stack_ptr + 1] -> rcvr_class_obj = 
      __new -> __o_class;
  }

  if (__call_stack[__call_stack_ptr + 2] -> rcvr_obj) {
    __call_stack[__call_stack_ptr + 2] -> rcvr_obj = __new;
    __call_stack[__call_stack_ptr + 2] -> rcvr_class_obj = 
      __new -> __o_class;
  }

  return __new;
}


OBJECT *__ctalkGlobalObjectBecome (OBJECT *__old, OBJECT *__new) {
  VARENTRY *__v;
  if (__ctalk_dictionary != NULL) {
    for (__v = __ctalk_dictionary; __v; __v = __v -> next) {
      if (__v -> var_object == __old) {
	__become_this_object (__v, __old, __new);
      }
    }
  }
  return NULL;
}

int become_expr_parser_cleanup (OBJECT *old, OBJECT *new) {
  int stack_start = P_MESSAGES;
  int stack_end = _get_e_message_ptr ();
  int idx;
  MESSAGE *m;
  MESSAGE_STACK __e_messages = _get_e_messages ();
  for (idx = stack_end + 1; idx <= stack_start; idx++) {
    m = __e_messages[idx];
    if (m -> obj == old) m -> obj = new;
    if (m -> value_obj == old) m -> value_obj = new;
    if (m -> receiver_obj == old) m -> receiver_obj = new;
  }
  return FALSE;
}

#if 0
/* This is a no-op for now. */
static void ctalk_alias_decrement (VARENTRY *v, OBJECT *o) {
  if (v -> var_object -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) {
    if (v -> var_object -> nrefs > 1) {
      __objRefCntDec (OBJREF(v -> var_object));
    }
  } else {
    __objRefCntDec (OBJREF(v -> var_object));
  }
}
#endif

static void ctalk_alias_delete (VARENTRY *v, OBJECT *o) {
  /* __ctalkDeleteObject restores the tag to its original state. */
  if (!(v -> var_object -> attrs & 
	OBJECT_IS_MEMBER_OF_PARENT_COLLECTION)) {
    __ctalkDeleteObject (v -> var_object); 
  }
}

static int is_aliased_elsewhere (VARENTRY *v) {
  if ((v -> var_object -> attrs & OBJECT_IS_STRING_LITERAL) && 
      !(v -> var_object -> scope & METHOD_USER_OBJECT) &&
      !(v -> var_object -> scope & VAR_REF_OBJECT)) {
    switch (n_tags (v -> var_object))
      {
      case 0:
	return FALSE;
	break;
      case 1:
	/*
	 *  If the object is not aliased somewhere else.
	 */
	if (v -> var_object -> __o_vartags -> tag == v)
	  return FALSE;
	break;
      default:
	break;
      }
  }
  return TRUE;
}

/*
 *  Check when we try to assign a constant argument.
 *  If necessary, promote the constant object to the 
 *  receiver's class. 
 *
 *  Note that is_constant_arg gets set in eval_expr (so far
 *  the only place), for single token expressions.
 */
static void constant_target_to_declared_class (OBJECT *target, 
				      OBJECT *declared_object) {
  if (!is_constant_arg)
    return;

  if (target -> __o_class == declared_object -> __o_class)
    return;

#ifdef USE_CLASSNAME_STR
  strcpy (target -> __o_classname, declared_object -> CLASSNAME);
#endif
#ifdef USE_SUPERCLASSNAME_STR
  strcpy (target -> __o_superclassname, 
	  _SUPERCLASSNAME(declared_object));
#endif
  target -> __o_class = declared_object -> __o_class;
  target -> __o_superclass = declared_object -> __o_superclass;
  /* Also change the value variable if the one or other
     receiver isn't already. TODO - This might need more checking
     if the expression is really convoluted. */
  if (IS_OBJECT(target -> instancevars) &&
      IS_OBJECT(declared_object -> instancevars)) {
    if (target -> instancevars -> __o_class !=
	declared_object -> instancevars -> __o_class) {

#ifdef USE_CLASSNAME_STR
      strcpy (target -> instancevars -> __o_classname, 
	      declared_object -> instancevars ->  CLASSNAME);
#endif
#ifdef USE_SUPERCLASSNAME_STR
      strcpy (target -> instancevars ->  __o_superclassname, 
	      _SUPERCLASSNAME(declared_object -> instancevars));
#endif
      target -> instancevars -> __o_class = 
	declared_object -> instancevars ->  __o_class;
      target -> instancevars -> __o_superclass = 
	declared_object -> instancevars -> __o_superclass;
      /* TODO? This might need the values of the original object's
	 instance vars also.... */
      __ctalkInstanceVarsFromClassObject (target);
    }
  } else {
    /* If we're already the value variable, then only add vars
       if the original object has further instance variables....
       Needs further checking. */
    if (IS_OBJECT(declared_object -> instancevars)) {
      __ctalkInstanceVarsFromClassObject (target);
    }
  }
}

static METHOD *calling_method_for_args (void) {
  int stack_idx;
  if (__call_stack[__call_stack_ptr+2] -> inline_call) {
    stack_idx = __call_stack_ptr + 4;
    while (__call_stack[stack_idx] -> inline_call) {
      stack_idx += 2;
      if (stack_idx > (MAXARGS - 1))
	return NULL;
    }
    return __call_stack[stack_idx] -> method;
  } else {
    return __call_stack[__call_stack_ptr+2] -> method;
  }
  return NULL;
}

/*
 *  Assigns an object to the receiver's VARENTRY, so we
 *  can refer to it by the receiver's declaration.
 *  If the receiver doesn't have a declaration (e.g., it's
 *  an instance variable), then return ERROR.
 *  NOTE: We should check for instance and class variable 
 *  sequences, however - do it NOW.
 *
 *  This method of matching the __rcvr_arg with the correct
 *  VARENTRY works okay as long as it's consistent with the
 *  way __ctalk_arg (), __ctalk_method (), and everything else
 *  looks up the receiver object.
 */
int __ctalkAliasObject (OBJECT *__rcvr_arg, OBJECT *__target) {
  RT_FN *r;
  METHOD *m;
  VARENTRY *v_list, *v, *v_arg;
  VARENTRY *dictionary_list_ptr_tmp = NULL; /* This temporary saves the 
					       __ctalk_last_object dictionary
					       list pointer if necessary -
					       it can get changed when
					    __ctalkDeleteObjectInternal ()
					    calls __ctalk_remove_object (). */
  OBJECT *rcvr, *rcvr_tmp;
  bool need_new_rcvr = false;
  bool is_orig_object = false;
  int n_th_param;
  METHOD *calling_method;

  if (IS_VALUE_INSTANCE_VAR(__rcvr_arg))
    rcvr = __rcvr_arg -> __o_p_obj;
  else
    rcvr = __rcvr_arg;

  if (assign_string_by_value ()) {
    __ctalkSetObjectValueVar 
      (__rcvr_arg, 
       ((__target -> instancevars) ? 
	__target -> instancevars -> __o_value :
	__target -> __o_value));
    reset_primary_tag (__rcvr_arg -> __o_vartags);
    clear_string_assign_by_value ();
    if (__target -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
      OBJECT *o = (OBJECT *)SYMVAL(__target -> __o_value); 
      if (IS_OBJECT (o)) {
	__ctalkSetObjectAttr (__rcvr_arg, __rcvr_arg -> attrs |
			      OBJECT_VALUE_IS_BIN_SYMBOL);
      }
    }
    return SUCCESS;
  }

  if (__rcvr_arg == __target) {
    if (is_string_object (__rcvr_arg)) {
      /* String and subclasses' arguments already have their
	 C values derived, so simply reset the i pointers. */
      if (!IS_CLASS_OBJECT(__rcvr_arg))
	reset_varentry_i (__rcvr_arg -> __o_vartags -> tag);
      return SUCCESS;
    } else {
      if ((v_arg = arg_active_varentry ()) != NULL) {
	if (v_arg -> i != I_UNDEF) {
	  /* Is this sufficient? */
	  /* ... so far, but only if it's not the first
	     assignment on the tag. Otherwise, we add the
	     tag below. (We probably won't be in this
	     clause, either.) */
	  if (__rcvr_arg -> __o_vartags &&
	      !IS_EMPTY_VARTAG (__rcvr_arg -> __o_vartags)) {
	       __rcvr_arg -> __o_vartags -> tag -> i = v_arg -> i;
	  }
	}
	return SUCCESS;
      }
    }
  }

  if ((r = get_calling_fn_w_block_call ()) != NULL) {
    v_list = r -> local_objects.vars;
  } else {
    if ((m = get_calling_method_w_block_call ()) != NULL) {
      v_list = M_LOCAL_VAR_LIST (m);
    }
  }

  for (v = NULL; v_list && !v; v_list = v_list -> next) {
    if (v_list -> var_object == rcvr)
      v = v_list;
  }

  /* if ((calling_method = __call_stack[__call_stack_ptr+2] -> method) != NULL) { */
  if ((calling_method = calling_method_for_args ()) != NULL) {
    /* look for a parameter in the calling method */
    if (!v && (calling_method -> n_params > 0)) {
      for (n_th_param = 0;
	   n_th_param < calling_method -> n_params;
	   ++n_th_param) {
	if (str_eq (calling_method -> params[n_th_param] -> name,
		    rcvr -> __o_name)) {
	  if ((rcvr_tmp = calling_method ->
	       args[calling_method -> n_params - 1 - n_th_param] -> obj)
	      != NULL) {
	    if (HAS_VARTAGS(rcvr_tmp) &&
		IS_VARENTRY(rcvr_tmp->__o_vartags->tag)) {
	      v = rcvr_tmp -> __o_vartags -> tag;
	      break;
	    }
	  }
	} else if (calling_method -> args[n_th_param] -> obj ==
		   __rcvr_arg) {
	  rcvr_tmp = calling_method -> args[n_th_param] -> obj;
	  if (HAS_VARTAGS(rcvr_tmp) &&
	      IS_VARENTRY(rcvr_tmp->__o_vartags->tag)) {
	    v = rcvr_tmp -> __o_vartags -> tag;
	    break;
	  }
	}
      }
    }
  }

  if (!v) { /* Look for a global varentry. */
    if (__ctalk_dictionary != NULL) {
      for (v_list = __ctalk_dictionary; v_list && !v; v_list = v_list -> next) {
	if (v_list -> var_object == rcvr) {
	  v = v_list;
	  if (v == __ctalk_last_object) 
	    dictionary_list_ptr_tmp = v;
	}
      }
    }
  }

  if (!v)
    if ((v = reset_if_re_assigning_collection (__rcvr_arg, __target,
					       OBJREF(rcvr))) != NULL) {
      need_new_rcvr = true;
    }

  if (!v) { /* If we still haven't found the varentry, return with error. */
    return ERROR;
  }

  if (v -> orig_object_rec == NULL) {
    v -> orig_object_rec = v -> var_object;
    is_orig_object = true;
  } else if (v -> orig_object_rec == v -> var_object) {
    is_orig_object = true;
  }

  rcvr_tmp = __ctalk_receiver_pop ();
  __ctalk_receiver_push (rcvr_tmp);
  if (rcvr_tmp == v -> var_object) {
    (void)__ctalk_receiver_pop_deref ();
    need_new_rcvr = true;
    /* In case we deref a collection key object whose reference count
       hasn't yet been incremented. */
    if (rcvr -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) {
      if (rcvr -> nrefs < 1) {
	__objRefCntSet (OBJREF(rcvr), 1);
      }
    }
  }

  remove_tag (v -> var_object, v);

  /*
   *  If either the new or old __target's scope is ARG_VAR, and it's
   *  the last occurrence of the constant, then we can delete it.
   *  Also note that we need to replace the receiver object, but we
   *  don't delete something on the argument stack, just its VARENTRY
   *  if it's this one.
   */
  if (need_new_rcvr && v ->  var_object -> scope == ARG_VAR &&
      IS_EMPTY_VARTAG(v -> var_object -> __o_vartags) &&
      (__rcvr_arg != __target)) {
    ctalk_alias_delete (v, v -> var_object);
  } else if (!is_aliased_elsewhere (v)) {
    ctalk_alias_delete (v, v -> var_object);
  } else if (need_new_rcvr && v -> var_object -> scope == ARG_VAR &&
	     v -> var_object -> nrefs <= 2) {
    /* The number of tags does not always correlate with
       the reference count of an object, but we'll watch this. */
    ctalk_alias_delete (v, v -> var_object);
  }

  if ((__target -> attrs & OBJECT_IS_I_RESULT) &&
      (__target -> __o_vartags -> tag == NULL) && 
      (__target -> __o_vartags -> from != NULL)) {

    /*  This should only apply to Strings at the moment. */

    if (IS_OBJECT (__target -> __o_vartags -> from -> var_object)) {
      /* this is probably safer. */
	__ctalkSetObjectValueVar (v -> var_object, 
				  __target -> __o_vartags -> from -> var_object
				  -> __o_value);
	derived_i (v, __target);

      if (need_postfix_fetch_update ()) {
	__ctalkSetObjectValueVarB (v -> var_object, (char *)v -> i);
	v -> i = I_UNDEF;
      }

      /* Delete the derived target if possible. */
      if (!is_arg (__target)) {
	__ctalkDeleteObject (__target);
      }
      __target = NULL;

      __objRefCntInc (OBJREF(v -> var_object));
    } else {
      v -> var_object = __target;
      __objRefCntInc (OBJREF(__target));
    }
  } else {
    if (IS_OBJECT(v -> var_object)) {
      /* If there's a leftover object here, then this is a good
	 time to stash it. */
      if (v ->  var_object -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) {
	if (!IS_EMPTY_VARTAG(v -> var_object -> __o_vartags)) 
	  remove_tag (v -> var_object, v);
	__ctalkRegisterUserObject (v -> var_object);
      }
    }
    /* Make sure (again) that we keep the original object. */
    if (v -> orig_object_rec == NULL) {
      v -> orig_object_rec = v -> var_object;
    }
    v -> var_object = __target;
    __objRefCntInc (OBJREF(__target));
    constant_target_to_declared_class  (__target, 
					IS_OBJECT(v -> orig_object_rec) ?
					v -> orig_object_rec :
					v -> var_object);
  }

  if (__target) {
    if (is_string_object (__target)) {
      /* Again, String objects have their C value already derived. */
      /* Note: If the __target has a OBJECT_HAS_LOCAL_TAG attribute,
	 it's probably safe to leave the attribute and the local tag
	 in place. */
      add_tag (__target, v);
    } else {
      VARENTRY *v_arg;
      add_tag (__target, v);
      /* If the __target has a from tag, then it's a derived object,
	 and it should be safe to reset the primary tag (now the tag
	 of the lvalue). */
      if (IS_VARTAG (__target -> __o_vartags) && 
	  IS_VARENTRY(__target -> __o_vartags -> from)) {
	reset_primary_tag (__target -> __o_vartags);
      }
      if ((v_arg = arg_active_varentry ()) != NULL) {
	if (v_arg -> i != I_UNDEF) {
	  v -> i = v_arg -> i;
	}
      }
    }
  }

  if (dictionary_list_ptr_tmp == v)
    __ctalk_last_object = v;

  if (__target)
    __objRefCntSet (OBJREF(__target), __target -> nrefs + 1);

  if (need_new_rcvr) {
    if (__target) {
      __ctalk_receiver_push_ref (__target);
    } else {
      __ctalk_receiver_push_ref (v -> var_object);
    }
  }

  if (need_postfix_fetch_update ())
    clear_postfix_fetch_update ();

  return SUCCESS;
}

/*
 *  Like __ctalkAliasObject (), but it returns -1 if __rcvr_arg
 *  is not actually the receiver.
 */
int __ctalkAliasReceiver (OBJECT *__rcvr_arg, OBJECT *__target) {
  OBJECT *rcvr_tmp;
  rcvr_tmp = __ctalk_receiver_pop ();
  __ctalk_receiver_push (rcvr_tmp);
  if (rcvr_tmp != __rcvr_arg)
    return ERROR;

  return __ctalkAliasObject (__rcvr_arg, __target);
}
