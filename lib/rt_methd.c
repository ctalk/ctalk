/* $Id: rt_methd.c,v 1.1.1.1 2020/12/13 14:51:03 rkiesling Exp $ */

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
#include <stdlib.h>
#include <string.h>
#if defined(__sparc__) && defined(__svr4__)
#include <strings.h>       /* Prototype of bcmp. */
#endif
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "typeof.h"

extern int method_object_message_call;  /* Declared in rt_vmthd.c */

extern RT_INFO rtinfo, *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;
extern VARENTRY *__ctalk_dictionary;

extern OBJECT *__ctalk_argstack[MAXARGS+1];
extern int __ctalk_arg_ptr;

extern OBJECT *__ctalk_classes;
extern OBJECT *__ctalk_last_class;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

typedef struct _methodsrchinfo {
  OBJECT *method_srch_obj;
  OBJECT *rcvr_obj;
  char *object_name;
  OBJECT *(*cfunc)();
  char *method_name;
  OBJECT *new_rcvr_return;
} METHODSRCHINFO;

static inline VARENTRY *last_varentry (VARENTRY *start) {
  VARENTRY *v;
  for (v = start; v -> next; v = v -> next)
    ;
  return v;
}

/* This is a last resort - if the method is an assign operator
   ("=") and the receiver object has the original object in the
   varentry, then use the varentry's orig_obj_rec object to search
   for it, and return the actual (orig_obj_rec) receiver to 
   __ctalk_method. This should allow a little leeway in case
   someone writes a method that uses the '=' operator semantics
   differently than expected - for repeated calls with the "="
   method. */
static METHOD *assign_method_orig_obj (METHODSRCHINFO *msi) {

  OBJECT *new_mthd_srch_obj;
  METHOD *method;
  
  msi -> new_rcvr_return = NULL;

  if (str_eq (msi -> method_name, "=")) {
    if (IS_VARTAG(msi -> rcvr_obj -> __o_vartags)) {
      if (IS_VARENTRY(msi -> rcvr_obj -> __o_vartags -> tag)) {
	if ((msi -> new_rcvr_return = (msi -> rcvr_obj -> __o_vartags
				       -> tag -> orig_object_rec)) != NULL) {
	  new_mthd_srch_obj = msi -> new_rcvr_return;
	  if (((method = 
		__ctalkFindInstanceMethodByFn (&new_mthd_srch_obj,
					       msi -> cfunc, FALSE))
	       != NULL) ||
	      ((method = 
		__ctalkFindClassMethodByFn (&new_mthd_srch_obj,
					    msi -> cfunc, FALSE))
	       != NULL)) {
	    return method;
	  }
	}
      }
    }
  }
  return NULL;
}

static METHOD *__ctalk_method_find_method_by_fn (METHODSRCHINFO *msi) {
	
  static METHOD *method = NULL;
  OBJECT *p_value_obj;

  if (((method = 
       __ctalkFindInstanceMethodByFn (&msi -> method_srch_obj,
				      msi -> cfunc, FALSE))
       == NULL) &&
      ((method = 
	__ctalkFindClassMethodByFn (&msi -> method_srch_obj,
				    msi -> cfunc, FALSE))
       == NULL)) {
    if (msi -> rcvr_obj -> instancevars) {
      msi -> method_srch_obj = msi -> rcvr_obj -> instancevars;
      if (((method = 
	    __ctalkFindInstanceMethodByFn (&msi -> method_srch_obj, 
					   msi -> cfunc, FALSE))
	   == NULL) &&
	  ((method = 
	    __ctalkFindClassMethodByFn (&msi -> method_srch_obj, 
					msi -> cfunc, FALSE))
	   == NULL)) {
	/*
	 *  Experimental - if the receiver is the member of a collection, 
	 *  then look for the method in the parent object's class.  Then, 
	 *  if the parent object is an instance or class variable, look 
	 *  for the method in the parent object's value class.
	 */
	if (IS_OBJECT (msi -> rcvr_obj -> __o_p_obj)) {
	  msi -> method_srch_obj = msi -> rcvr_obj -> __o_p_obj;
	  if (((method = 
		__ctalkFindInstanceMethodByFn (&msi -> method_srch_obj, 
					       msi -> cfunc, FALSE))
	       == NULL) &&
	      ((method = 
		__ctalkFindClassMethodByFn (&msi -> method_srch_obj, 
					    msi -> cfunc, FALSE))
	       == NULL)) {
	    if ((p_value_obj = msi -> rcvr_obj -> __o_p_obj -> instancevars)
		!= NULL) {
	      msi -> method_srch_obj = p_value_obj;
	      if (((method = 
		    __ctalkFindInstanceMethodByFn (&msi -> method_srch_obj, 
						   msi -> cfunc, FALSE))
		   == NULL) &&
		  ((method = 
		    __ctalkFindClassMethodByFn (&msi -> method_srch_obj, 
						msi -> cfunc, FALSE))
		   == NULL)) {
		if ((method = assign_method_orig_obj (msi)) == NULL) {
		  _warning ("__ctalk_method: Undefined method, \"%s.\"\n"
			    "Receiver, \"%s,\" (Class \"%s\").\n",
			    msi -> method_name,
			    msi -> rcvr_obj -> __o_name,
			    IS_OBJECT(msi -> rcvr_obj -> instancevars) ?
			    msi -> rcvr_obj -> instancevars -> __o_class
			    -> __o_name :
			    msi -> rcvr_obj -> __o_class -> __o_name);
		  return NULL;
		} else {
		  return method;
		}
	      } 
	    }
	  }  else {
	    _warning ("__ctalk_method: Undefined method.\n"
		      "\tReceiver, \"%s,\" (class %s). Name, \"%s.\"\n",
		      msi -> method_srch_obj -> __o_name,
		      msi -> method_srch_obj -> CLASSNAME, 
		      msi -> object_name);
	    return NULL;
	  }
	} 
      }
    }
  }

  return method;
}

static char *declared_rcvr_value = NULL;
static char *derived_rcvr_value_orig = NULL;

static bool result_still_not_modified (OBJECT *rcvr, OBJECT *result) {
  if (IS_OBJECT (result)) {
    if (!str_rcvr_mod () || 
	((result -> __o_p_obj ? result -> __o_p_obj : result)
	 != rcvr)) {
      return true;
    }
  }
  return false;
}

static OBJECT *find_rcvr_obj (char *__object_name,
			      char *__method_name,
			      int *have_derived_value ) {
  OBJECT *__receiver_obj;
  OBJECT *derived_rcvr;
  void *c_i_rcvr = I_UNDEF;

  if ((__receiver_obj = __ctalk_get_arg_tok (__object_name)) == NULL) 
    return NULL;
  if ((c_i_rcvr = active_i (__receiver_obj)) != I_UNDEF) {
    if (is_string_object (__receiver_obj)) {
      /* This is kludgy but easy, and almost method local. */
      derived_rcvr = create_param_i (__receiver_obj, c_i_rcvr);
      declared_rcvr_value = 
	strdup (__receiver_obj -> instancevars ->  __o_value);
      __ctalkSetObjectValueVar (__receiver_obj,
				derived_rcvr -> instancevars 
				-> __o_value);
      derived_rcvr_value_orig =
	strdup (derived_rcvr -> instancevars -> __o_value);
      *have_derived_value = true;
      __ctalkDeleteObject (derived_rcvr);
      set_rcvr_mod_catch ();
    } else {
      if (is_key_object (__receiver_obj)) {
	if (c_i_rcvr == NULL) {
	  /* This might also be valid if i is something besides
	     NULL or I_UNDEF... */
	  if (!reset_i_if_lval (__receiver_obj, __method_name)) {
	    if ((__receiver_obj = create_param_i (__receiver_obj, 
						  c_i_rcvr))
		== NULL)
	      /* have_derived_value == true && __receiver_obj == NULL
		 means we've iterated off the end of a collection
		 or something similar, so just return from __ctalk_method. */
	      *have_derived_value = true;
	      return NULL;
	  }
	} else {
	  if ((__receiver_obj = create_param_i (__receiver_obj, c_i_rcvr))
	      == NULL) {
	    /* here, too. */
	    *have_derived_value = true;
	    return NULL;
	  }
	}
      }
    }
  }
  return __receiver_obj;
}

OBJECT *__ctalk_method (char *__object_name, OBJECT *(*__cfunc)(), 
			char *__method_name) {

  OBJECT *__receiver_obj, *__method_rcvr_obj_, *__receiver_obj_post_call;
  OBJECT *__class_obj_;
  OBJECT *__result_obj_ = NULL, *__result_obj_post_call = NULL;
  OBJECT *derived_rcvr, *const_rcvr_class = NULL;
  METHOD *method, *rcvr_class_method;
  CVAR *c_rcvr = NULL;
  int i;
  int cvar_rcvr_obj_is_created = FALSE;
  OBJECT *(*__eff_cfunc)();
  void *c_i, *c_i_rcvr = I_UNDEF;
  int have_derived_value = false;
  METHODSRCHINFO msi;

  /*
   *  If the object name refers to a parameter, then it is only
   *  stored the method, and we have to find the actual argument
   *  by looking for the parameter name.  Method arguments are 
   *  not stored on the receiver stack, until just before we 
   *  call this method.
   *
   *  If the object name is a C lexical type, then simply create
   *  an object. 
   */

  __eff_cfunc = __cfunc;

  if ((__receiver_obj = find_rcvr_obj (__object_name,
				      __method_name,
				       &have_derived_value)) != NULL) {
    goto have_receiver;
  } else {
    if (have_derived_value) {
      /* the receiver worked out to a NULL - just return. */
      return NULL;
    }

    if ((c_rcvr = get_method_arg_cvars (__object_name)) != NULL) {
      __receiver_obj = cvar_object (c_rcvr, &cvar_rcvr_obj_is_created);
      if (cvar_rcvr_obj_is_created)
	__ctalkSetObjectScope (__receiver_obj, (RECEIVER_VAR|CREATED_PARAM));
      goto have_receiver;
    }
  }
  
  if (lextype_is_LITERAL_T (__object_name)) {
    const_rcvr_class = rt_defclasses -> p_string_class;
  } else if (lextype_is_INTEGER_T (__object_name)) {
    const_rcvr_class = rt_defclasses -> p_integer_class;
  } else if (lextype_is_LITERAL_CHAR_T (__object_name)) {
    const_rcvr_class = rt_defclasses -> p_character_class;
  } else if (lextype_is_LONGLONG_T (__object_name)) {
    const_rcvr_class = rt_defclasses -> p_longinteger_class;
  } else if (lextype_is_FLOAT_T (__object_name)) {
    const_rcvr_class = rt_defclasses -> p_float_class;
  }
  if (const_rcvr_class)
    __receiver_obj = create_object_init_internal
      (__object_name, const_rcvr_class, RECEIVER_VAR|CREATED_PARAM,
       __object_name);

 have_receiver:
  if (!IS_OBJECT(__receiver_obj)) {
    return NULL;
  }
  __class_obj_ = __receiver_obj->__o_class;
  __method_rcvr_obj_ = __receiver_obj;

  msi.method_srch_obj = __method_rcvr_obj_;
  msi.rcvr_obj = __receiver_obj;
  msi.object_name = __object_name;
  msi.cfunc = __cfunc;
  msi.method_name = __method_name;
  msi.new_rcvr_return = NULL;

  method = __ctalk_method_find_method_by_fn (&msi);

  if (IS_OBJECT(msi.new_rcvr_return)) {
    __receiver_obj = __method_rcvr_obj_ = msi.new_rcvr_return;
  }

  /* 
   *   If the receiver object was replaced by __ctalkAliasObject (),
   *   then check the receiver object's varentry for the orig_object_rec
   *   and use that to look up the method.
  */
  if (!method) {
    if (__method_rcvr_obj_ -> __o_vartags &&
	!IS_EMPTY_VARTAG(__method_rcvr_obj_ -> __o_vartags)) {
      if ((__method_rcvr_obj_ -> __o_vartags -> tag) &&
	  __method_rcvr_obj_ -> __o_vartags -> tag -> orig_object_rec && 
	  (__method_rcvr_obj_ -> __o_vartags -> tag -> var_object != 
	   __method_rcvr_obj_ -> __o_vartags -> tag -> orig_object_rec)) {
	__method_rcvr_obj_ = 
	  __method_rcvr_obj_ -> __o_vartags -> tag -> orig_object_rec;

	msi.rcvr_obj = __method_rcvr_obj_;
	method = __ctalk_method_find_method_by_fn (&msi);

	if (!method) {
	  __method_rcvr_obj_ = 
	    __method_rcvr_obj_ -> __o_vartags -> tag -> var_object;

	  method = __ctalk_method_find_method_by_fn (&msi);

	}
	if (!method) {
	  _warning ("rt_methd: Method %s (receiver %s) not found.\n",
		    __method_name, __receiver_obj -> __o_name);
	  return NULL;
	}
	__eff_cfunc = method -> cfunc;
      }
    }
  }

  if (!method) {
    if (((method = 
	  __ctalkFindInstanceMethodByName (&__method_rcvr_obj_, 
					   __method_name, FALSE, ANY_ARGS)) 
	 == NULL) &&
	((method = 
	  __ctalkFindClassMethodByName (&__method_rcvr_obj_, 
					__method_name, FALSE, ANY_ARGS))  
	 == NULL)) {
      _warning ("__ctalk_method: Undefined method, \"%s.\"\n"
		"\tReceiver, \"%s,\" (class %s). Name, \"%s.\"\n",
		__method_name,
		__method_rcvr_obj_ -> __o_name,
		__method_rcvr_obj_ -> CLASSNAME, 
		__object_name);
      return NULL;
    } else {
    __eff_cfunc = method -> cfunc;
    }
  }

  if (method && ! method -> varargs) {
    /* if the expression contains a class cast, then the arguments
       could be in the method of the receiver's class - so transfer
       them over. *Need More Examples* */
    if (method -> n_params == 1) {
      if (method -> n_args < method -> n_params) {
	if (((rcvr_class_method = 
	      __ctalkFindInstanceMethodByName (&__method_rcvr_obj_, 
					       __method_name, FALSE,
					       ANY_ARGS)) 
	     != NULL) ||
	    ((rcvr_class_method = 
	      __ctalkFindClassMethodByName (&__method_rcvr_obj_, 
					    __method_name, FALSE, ANY_ARGS))  
	     != NULL)) {
	  if (rcvr_class_method -> n_args == rcvr_class_method -> n_params) {
	    for (i = 0; i < method -> n_params; ++i) {
	      method -> args[i] = rcvr_class_method -> args[i];
	      rcvr_class_method -> args[i] = NULL;
	      ++method -> n_args;
	      --rcvr_class_method -> n_args;
	    }
	  }
	}
      }
    }
  }
	     
  method -> arg_frame_top = __ctalk_arg_ptr + 1;
  method -> rcvr_frame_top = __ctalk_receiver_ptr + 1;

  __save_rt_info (__receiver_obj, __class_obj_, NULL, method, __cfunc, False);

  __ctalk_receiver_push_ref (__receiver_obj);

  __result_obj_ = (__eff_cfunc)();

  __receiver_obj_post_call = __ctalk_receiver_pop_deref ();
  
  /* The attribute should be set on the original receiver only. */
  if (__receiver_obj -> attrs & OBJECT_HAS_PTR_CX)
    __ctalkSetObjectAttr (__receiver_obj,
			  __receiver_obj -> attrs & ~OBJECT_HAS_PTR_CX);

  __ctalk_methodReturn (__receiver_obj, method, __result_obj_);

  if (IS_CONSTRUCTOR (method)) {
    OBJECT *__arg_object;
    VARENTRY *t_local, *v_next;
    int call_level;
    /*
     *  The interpreter adds a __ctalk_set_local () call in the
     *  caller's init block, so it isn't needed here.
     *
     *  The object must be removed from the method, so it 
     *  isn't bollixed the next time the method is called.
     *
     *  If the argument is still on the stack, remove it.
     */
    if (IS_OBJECT(__result_obj_))
      __ctalk_dictionary_add (__result_obj_);

    __arg_object = __ctalk_arg_pop ();
    __ctalk_arg_push (__arg_object);
    if (__result_obj_ == __arg_object) {
      if (IS_CLASS_OBJECT(method -> rcvr_class_obj) &&
	  (method -> rcvr_class_obj == rt_defclasses -> p_object_class)) {
	__ctalk_arg_pop ();
      }
      if (method -> n_args > 0) {
	if (IS_ARG(method->args[method->n_args-1]))
	  __ctalkDeleteArgEntry (method -> args[method -> n_args - 1]);
      }
      method -> args[method -> n_args - 1] = NULL;
      --(method -> n_args);
    }

    for (t_local = M_LOCAL_VAR_LIST(method); 
	 t_local; t_local = t_local -> next) {
      if (t_local -> var_object == __result_obj_) {
	if (__result_obj_ == M_LOCAL_VAR_LIST(method)->var_object) {
	  v_next = M_LOCAL_VAR_LIST(method) -> next;
	  delete_varentry (M_LOCAL_VAR_LIST(method));
	  /* TODO - Should this always be the case? */
	  __result_obj_ -> __o_vartags -> tag = NULL;
	  if (__result_obj_ -> __o_vartags -> next) {
	    VARTAG *t_tmp = __result_obj_ -> __o_vartags -> next;
	    delete_vartag (__result_obj_ -> __o_vartags,
			   __result_obj_ -> attrs & OBJECT_HAS_LOCAL_TAG);
	    t_tmp -> prev = NULL;
	    __result_obj_ -> __o_vartags = t_tmp;
	  }
	  M_LOCAL_VAR_LIST(method) = t_local = v_next;
	  goto varentry_delete_done;
	} else {
	  if (t_local -> prev) t_local -> prev -> next = t_local -> next;
	  if (t_local -> next) t_local -> next -> prev = t_local -> prev;
	  if (t_local == M_LOCAL_VAR_LIST(method)) {
	    delete_varentry (t_local);
	    M_LOCAL_VAR_LIST(method) = NULL;
	    goto varentry_delete_done;
	  } else {
	    VARENTRY *t_local_next;
	    VARTAG *t_tmp;
	    if (IS_VARTAG(t_local -> var_object -> __o_vartags -> next)) {
	      t_tmp = t_local -> var_object -> __o_vartags;
	      t_local -> var_object -> __o_vartags =
		t_local -> var_object -> __o_vartags -> next;
	      t_local -> var_object -> __o_vartags -> prev
		= t_tmp -> prev;
	      __xfree (MEMADDR(t_tmp));
	    }
	    t_local_next = t_local -> next;
	    delete_varentry (t_local);
	    if (!t_local_next)
	      goto varentry_delete_done;
	  }
	}
      }
      if (!t_local) break;
      if (!t_local -> next) break;
    }
  varentry_delete_done:

    /*
     *  If the calling method is also a constructor, then 
     *  set the result object's class and superclass to 
     *  the calling method.  Also set the value variable's
     *  class and superclass.
     *
     *  The caller is two call frames up from the call 
     *  stack pointer.
     *
     *  If the caller is a _rt_fn, then skip.
     */
    if ((call_level = __call_stack_ptr) < (MAXARGS - 1) &&
	(__call_stack[call_level+2] -> _rt_fn == NULL)) {
      METHOD *calling_method;
      OBJECT *calling_rcvr_class_obj;
      calling_rcvr_class_obj = 
	__call_stack[call_level + 2] -> rcvr_class_obj;
      if ((calling_method = 
	   __ctalkGetInstanceMethodByFn (calling_rcvr_class_obj, 
					 __call_stack[call_level+2]->method_fn,
					 FALSE)) != NULL) {
	if (IS_CONSTRUCTOR(calling_method)) {
#ifdef USE_CLASSNAME_STR
	  strcpy (__result_obj_ -> __o_classname, 
		  calling_rcvr_class_obj -> __o_name);
#endif
#ifdef USE_SUPERCLASSNAME_STR
	  strcpy (__result_obj_ -> __o_superclassname, 
		  _SUPERCLASSNAME(calling_rcvr_class_obj));
#endif
	  __result_obj_->__o_class = calling_rcvr_class_obj -> __o_class;
	  __result_obj_->__o_superclass = 
	    calling_rcvr_class_obj -> __o_superclass;
	}
      }    
    }
  }

  /*
   *  Decrementing the reference count here doesn't actually
   *  work very well.
   */
  if (method -> n_args) {
    if (method -> varargs) {
      int arg_frame, n_args_tmp;
      arg_frame = method->args[method->n_args-1]->call_stack_frame;
      n_args_tmp = method -> n_args;
      for (i = 0; i < method -> n_args; i++) {
	if ((IS_ARG(method->args[i])) && 
	    (method->args[i]->call_stack_frame == arg_frame)) {
	  __ctalkDeleteArgEntry (method->args[i]); 
	  method -> args[i] = NULL;
	  n_args_tmp--;
	}
      }
      method -> n_args = n_args_tmp;
    } else {
      for (i = 0; i < method -> n_args; i++) {
	if (method->args[i] && IS_ARG(method->args[i]))
	  __ctalkDeleteArgEntry (method->args[i]);
	method -> args[i] = NULL;
      }
      method -> n_args = 0;
    }
  }

  if (__receiver_obj == __receiver_obj_post_call) {
    if ((__receiver_obj -> scope & RECEIVER_VAR) &&
	(__receiver_obj -> scope & CREATED_PARAM)) {
      __ctalkDeleteObjectInternal (__receiver_obj);
    }
  } 

  if (have_derived_value && !str_rcvr_mod ()) {
    if (str_eq (__receiver_obj -> instancevars -> __o_value,
		derived_rcvr_value_orig)) {
      /* If the method didn't change the value, then we should be
	 okay with leaving the pointers where they are for this
	 call. */
      __ctalkSetObjectValueVar (__receiver_obj, declared_rcvr_value);
    } else if (*__receiver_obj -> instancevars -> __o_value == '\0'
	       && *derived_rcvr_value_orig == '\0') {
      /* this is also equivalent, 'tho str_eq doesn't work with
	 two empty strings */
      __ctalkSetObjectValueVar (__receiver_obj, declared_rcvr_value);
    } else {
      /* However, if the method changed the value, then we 
	 probably want to keep it, so reset the tag. */
      reset_primary_tag (__receiver_obj -> __o_vartags);
    }
  }
  if (have_derived_value) {
    /* __xfree () sets the pointers to NULL. */
    if (declared_rcvr_value != NULL)
      __xfree (MEMADDR(declared_rcvr_value));
    if (derived_rcvr_value_orig != NULL)
      __xfree (MEMADDR(derived_rcvr_value_orig));
  }

  if (save_e_methods_2 (method)) 
    init_extra_objects ();

  if (result_still_not_modified (__receiver_obj, __result_obj_)) {
    if ((c_i = active_i (__result_obj_)) != I_UNDEF) {
      __result_obj_ = create_param_i (__result_obj_, c_i);
      if (IS_OBJECT(__result_obj_)) {
	if ((__result_obj_ -> nrefs == 0) && 
	    (__result_obj_ -> attrs & OBJECT_IS_I_RESULT)) {
	  __ctalkRegisterUserObject (__result_obj_);
	}
      }
    }
  }

  if (need_rcvr_mod_catch ())
    clear_rcvr_mod_catch ();

  if (need_postfix_fetch_update () && IS_OBJECT(__result_obj_)) {
    /* This is used for Key objects right now. */
    __ctalkCopyObject (OBJREF(__result_obj_), 
		       OBJREF(__result_obj_post_call));
    __ctalkSetObjectAttr (__result_obj_post_call,
			   __result_obj_post_call -> attrs |
			   OBJECT_IS_I_RESULT);
    /* If the result is a method user object already, make sure
       we can register this one separately, or just delete it. */
    if (__result_obj_post_call -> scope & METHOD_USER_OBJECT) {
      __ctalkSetObjectScope (__result_obj_post_call,
			     __result_obj_post_call -> scope &
			     ~METHOD_USER_OBJECT);
    }
    __ctalkRegisterUserObject (__result_obj_post_call);
    make_postfix_current (__result_obj_);
    clear_postfix_fetch_update ();
  }

  __restore_rt_info ();
  if (c_rcvr) {
    if (cvar_rcvr_obj_is_created && (__receiver_obj != __result_obj_)) {
      __ctalkDeleteObject (__receiver_obj);
    } else if (HAS_CREATED_CVAR_SCOPE(__receiver_obj) &&
	       str_eq (__receiver_obj -> __o_name, c_rcvr -> name) &&
	       (__receiver_obj != __result_obj_)) {
      __ctalkDeleteObject (__receiver_obj);
    }
    delete_method_arg_cvars ();
  }

  if (IS_OBJECT(__result_obj_post_call))
    return __result_obj_post_call;
  else
    return __result_obj_;
}

#define ARG(m,obj) {if ((obj) -> nrefs == 0)  \
		       __ctalkSetObjectScope ((obj), ARG_VAR); \
                       __add_arg_object_entry_frame (m, obj); \
                       __ctalk_arg_push_ref (obj);}

OBJECT *__ctalkInlineMethod (OBJECT *rcvr, METHOD *m, int n_args, ...) {
  OBJECT *__result_object, *__rcvr_class_object, 
    *__method_class_object, *(*__cfunc)();
  int __save_stack_ptr, __s;
  va_list ap;
  int n;
  OBJECT *arg1_obj, *arg2_obj, *arg3_obj, *arg4_obj, *arg5_obj,
    *arg6_obj;

  if (!IS_OBJECT(rcvr))
    return NULL;

  if (n_args > 0) {
    va_start(ap, n_args);
    for (n = 0; n < n_args; ++n) {
      switch (n) 
	{
	case 0: arg1_obj = (OBJECT *)va_arg (ap, char *); break;
	case 1: arg2_obj = (OBJECT *)va_arg (ap, char *); break;
	case 2: arg3_obj = (OBJECT *)va_arg (ap, char *); break;
	case 3: arg4_obj = (OBJECT *)va_arg (ap, char *); break;
	case 4: arg5_obj = (OBJECT *)va_arg (ap, char *); break;
	case 5: arg6_obj = (OBJECT *)va_arg (ap, char *); break;
	default:
	  _warning ("__ctalkInlineMethod (): Too many arguments.\n");
	  break;
	}
    }
    va_end(ap);
  }

  __cfunc = m -> cfunc;
  __rcvr_class_object = rcvr->__o_class;
  /*
   *  At the present the only use of a the method class is
   *  within an inline function, where the method's receiver class
   *  might be different than the class of the actual receiver.
   *  In these cases, the method's class is the receiver class of
   *  the previous call stack entry.
   */
  __method_class_object = m -> rcvr_class_obj;
  __save_stack_ptr = __call_stack_ptr;
  __save_rt_info (rcvr, __rcvr_class_object, __method_class_object, 
		  m, __cfunc, True);
  __ctalk_receiver_push_ref (rcvr);

  switch (n_args)
    {
    case 0: __result_object = (__cfunc)(); break;
    case 1: 
      ARG(m, arg1_obj);
      __result_object = (__cfunc)(); 
      __ctalk_arg_cleanup (__result_object);
      break;
    case 2:
      ARG(m,arg2_obj); ARG(m,arg1_obj);
      __result_object = (__cfunc)();
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object);
      break;
    case 3: 
      ARG(m,arg3_obj); ARG(m,arg2_obj); ARG(m,arg1_obj);
      __result_object = (__cfunc)();
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object);
      break;
    case 4: 
      ARG(m,arg4_obj); ARG(m,arg3_obj); ARG(m,arg2_obj); ARG(m,arg1_obj);
      __result_object = (__cfunc)();
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object);
      __ctalk_arg_cleanup (__result_object);
      break;
    case 5: 
      ARG(m,arg5_obj); ARG(m,arg4_obj); ARG(m,arg3_obj); ARG(m,arg2_obj);
      ARG(m,arg1_obj);
      __result_object = (__cfunc)();
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object);
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object);
      break;
    case 6: 
      ARG(m,arg6_obj); ARG(m,arg5_obj); ARG(m,arg4_obj); ARG(m,arg3_obj);
      ARG(m,arg2_obj); ARG(m,arg1_obj);
      __result_object = (__cfunc)();
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object);
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object); 
      __ctalk_arg_cleanup (__result_object);
      break;
    default:  /* The warning message is generated above. */
      __result_object = (__cfunc)(); 
      break;
    }

  __ctalk_receiver_pop_deref ();
  /*
   *  Methods that don't have a method return macro can sometimes
   *  have a __ctalk_exitFn () call, so check if we've already
   *  removed the call info.
   */
  if ((__s = __call_stack_ptr) < __save_stack_ptr)
    __restore_rt_info ();

  return __result_object;
}

static inline int __rt_match_method (METHOD *m, char *name, 
				     int n_params_wanted) {

  if (n_params_wanted < 0) {

    if (str_eq (m -> name, name) && !m -> prefix)
      return TRUE;

  } else {

    if (m -> varargs) {

      if (str_eq (m -> name, name))
   	return TRUE;

    } else {

      if (str_eq (m -> name, name) && 
   	  (m -> n_params == n_params_wanted) && 
   	  !m -> prefix)
   	return TRUE;

    }
  }

  return FALSE;
}

static METHOD *get_instance_method_class (OBJECT *class, const char *name, 
				 int n_args_wanted) {
  METHOD *m;
  for (m = class -> instance_methods; m; m = m -> next)
    if (__rt_match_method (m, (char *)name, n_args_wanted))
      return m;

  if (class -> __o_superclass)
    return get_instance_method_class (class -> __o_superclass,
				      name, n_args_wanted);
  else
    return NULL;
}

static METHOD *get_class_method_class (OBJECT *class, const char *name, 
				 int n_args_wanted) {
  METHOD *m;
  for (m = class -> class_methods; m; m = m -> next)
    if (__rt_match_method (m, (char *)name, n_args_wanted))
      return m;

  if (class -> __o_superclass)
    return get_class_method_class (class -> __o_superclass,
				      name, n_args_wanted);
  else
    return NULL;
}

METHOD *__ctalkGetInstanceMethodByName (OBJECT *o, 
					const char *name, int warn, 
					int n_args_wanted) {
  OBJECT *class, *value_var;
  METHOD *m;

  if (!IS_OBJECT(o)) return NULL;

  if (!name)
    return NULL;

  value_var = ((o -> instancevars) ? o -> instancevars : o);

  if ((class=
       ((IS_CLASS_OBJECT(value_var)) ? value_var : value_var->__o_class))
      != NULL) {
    if ((m = get_instance_method_class (class, name, n_args_wanted))
	!= NULL)
      return m;
  }


  /* Don't repeat if it isn't necessary. */
  if (value_var == o)
    return NULL;

  class=((IS_CLASS_OBJECT(o)) ? o : o->__o_class);
  if (!class) {
    if (warn) {
      _warning ("__ctalkGetInstanceMethodByName: Undefined class.\n",
		o -> CLASSNAME);
    }
    return NULL;
  }

  return get_instance_method_class (class, name, n_args_wanted);

}

METHOD *__ctalkGetPrefixMethodByName (OBJECT *o, 
					const char *name, int warn) {
  OBJECT *class, *value_var;
  METHOD *m;

  if (!IS_OBJECT(o)) return NULL;

  value_var = ((o -> instancevars) ? o -> instancevars : o);

  if ((class=
       ((IS_CLASS_OBJECT(value_var)) ? value_var : value_var->__o_class))
      != NULL) {
    for (m = class -> instance_methods; m; m = m -> next) {
      if (str_eq (m -> name, (char *)name) && m -> prefix) 
	return m;
    }
    for (class = class -> __o_superclass; 
	 class; class = class -> __o_superclass) {
      for (m = class -> instance_methods; m; m = m -> next) {
	if (str_eq (m -> name, (char *)name) && m -> prefix) 
	  return m;
      }
    }
  }

  class=((IS_CLASS_OBJECT(o)) ? o : o->__o_class);
  if (!class) {
    if (warn) {
      _warning ("__ctalkGetInstanceMethodByName: Undefined class.\n",
		o -> CLASSNAME);
    }
    return NULL;
  }
  for (m = class -> instance_methods; m; m = m -> next) {
    if (str_eq (m -> name, (char *)name) && m -> prefix) 
      return m;
  }
  for (class = class -> __o_superclass; 
       class; class = class -> __o_superclass) {
    for (m = class -> instance_methods; m; m = m -> next) {
      if (str_eq (m -> name, (char *)name) && m -> prefix) 
	return m;
    }
  }
  return NULL;
}

METHOD *__ctalkGetInstanceMethodByFn (OBJECT *rcvr, OBJECT *(*fn)(void), 
				      int warn) {

  OBJECT *class;
  METHOD *m;

  if (!IS_OBJECT(rcvr))
    return NULL;

  class = rcvr -> __o_class;

  for (m = class -> instance_methods; m; m = m -> next) {
    if (m -> cfunc == fn)
      return m;
    if (!m -> next)
      break;
  }
  
  if (class -> __o_superclass)
    return __ctalkGetInstanceMethodByFn (class -> __o_superclass, fn, warn);
  else
    return NULL;
}

METHOD *__ctalkGetClassMethodByName (OBJECT *o, const char *name, int warn,
				     int n_args_wanted) {

  OBJECT *class, *value_var;
  METHOD *m;

  if (!IS_OBJECT(o))
    return NULL;

  if (!name)
    return NULL;

  if (o -> __o_class == rt_defclasses -> p_expr_class) {
    if ((value_var = o -> instancevars) == NULL) {
      if (warn) {
	_warning ("__ctalkGetClassMethodByName: %s: Undefined value.\n",
		  o -> __o_name);
      }
      return NULL;
    }
    class = value_var -> __o_class;
   } else {
     class = o -> __o_class;
   }

   if (!class) {
     if (warn) {
       _warning ("__ctalkGetClassMethodByName: Undefined class %s.\n",
 		o -> CLASSNAME);
     }
     return NULL;
   }

   return get_class_method_class (class, (char *)name, n_args_wanted);

}

METHOD *__ctalkGetClassMethodByFn (OBJECT *rcvr, OBJECT *(*fn)(void), 
				   int warn) {

  OBJECT *class;
  METHOD *m;

  if (!IS_OBJECT(rcvr))
    return NULL;

  class = rcvr -> __o_class;

  for (m = class -> class_methods; m; m = m -> next) {
    if (m -> cfunc == fn)
      return m;
    if (!m -> next)
      break;
  }
  
  if (class -> __o_superclass)
    return __ctalkGetClassMethodByFn (class -> __o_superclass, fn, warn);
  else
    return NULL;
}

METHOD *__ctalkFindPrefixMethodByName (OBJECT **rcvr, 
					 const char *name, int warn) {

  int i;
  METHOD *m = NULL;
  OBJECT *superclass;

  if ((m = __ctalkGetPrefixMethodByName (*rcvr, name, warn)) == NULL) {
    for (superclass = (*rcvr) -> __o_superclass; superclass;
	 superclass = superclass -> __o_superclass) {
      if ((m = __ctalkGetPrefixMethodByName (superclass, name, 
					     warn)) != NULL) {
	break;
      }
    }
  }

  if (m) return m;

  for (i = __ctalk_receiver_ptr + 1; !m && (i <= MAXARGS); i++) {
    if ((m = 
	 __ctalkGetPrefixMethodByName (__ctalk_receivers[i], 
				       name, 
				       warn))
	!= NULL) {
      *rcvr = __ctalk_receivers[i];
    }
  }

  if (!m) {
    if ((!(*rcvr) -> instancevars) ||
	((*rcvr) -> instancevars && 
	 (*rcvr) -> instancevars -> attrs & OBJECT_IS_VALUE_VAR)) {
      OBJECT *collection_value;
      collection_value = *rcvr;
      m = __ctalkGetPrefixMethodByName (collection_value, name, warn);
    }
  }

  if (warn && !m)
    _warning
      ("__ctalkFindPrefixMethodByName: %s: %s (Class %s) does not understand %s.\n",
       __argvFileName (), (*rcvr) -> __o_name, (*rcvr) -> CLASSNAME, name);

  return m;
}

METHOD *__ctalkFindInstanceMethodByName (OBJECT **rcvr, 
					 const char *name, int warn, 
					 int n_args_wanted) {

  int i;
  METHOD *m = NULL;

  if ((m = __ctalkGetInstanceMethodByName (*rcvr, name, warn,
					   n_args_wanted)) == NULL) {
    for (i = __ctalk_receiver_ptr + 1; !m && (i <= MAXARGS); i++) {
      if ((m = 
	   __ctalkGetInstanceMethodByName (__ctalk_receivers[i], name, 
					   warn, n_args_wanted))
	  != NULL) {
	*rcvr = __ctalk_receivers[i];
      }
    }
  }

  if (!m) {
    if ((!(*rcvr) -> instancevars) ||
	((*rcvr) -> instancevars && 
	 (*rcvr) -> instancevars -> attrs & OBJECT_IS_VALUE_VAR)) {
      OBJECT *collection_value;
      collection_value = *rcvr;
      m = __ctalkGetInstanceMethodByName (collection_value, name, warn,
					  n_args_wanted);
    }
  }

  if (warn && !m) {
    _warning
      ("__ctalkFindInstanceMethodByName: %s: %s (Class %s) does not understand %s.\n",
       __argvFileName (), (*rcvr) -> __o_name, (*rcvr) -> CLASSNAME, name);
    if (__ctalkGetExceptionTrace ()) 
      __warning_trace ();
  }

  return m;
}

METHOD *__ctalkFindInstanceMethodByFn (OBJECT **rcvr, OBJECT *(*fn)(void), 
				       int warn) {

  int i;
  METHOD *m = NULL;

  if ((m = __ctalkGetInstanceMethodByFn (*rcvr, fn, warn)) == NULL) {

    for (i = __ctalk_receiver_ptr + 1; !m && (i <= MAXARGS); i++) {
      if ((m = 
	   __ctalkGetInstanceMethodByFn (__ctalk_receivers[i], fn, warn))
	  != NULL) {
	*rcvr = __ctalk_receivers[i];
      }
    }
  }

if (warn && !m)
    _warning
      ("__ctalkFindInstanceMethodByFn: %s: %s (Class %s): undefined method.\n",
       __argvFileName (), (*rcvr) -> __o_name, (*rcvr) -> CLASSNAME);

  return m;
}

METHOD *__ctalkFindClassMethodByName (OBJECT **rcvr, 
				      const char *name, int warn,
				      int n_args_wanted) {

  int i;
  METHOD *m = NULL;

  if ((m = __ctalkGetClassMethodByName (*rcvr, name, warn,
					n_args_wanted)) == NULL) {
    for (i = __ctalk_receiver_ptr + 1; !m && (i <= MAXARGS); i++) {
      if ((m = 
	   __ctalkGetClassMethodByName (__ctalk_receivers[i], name, warn,
					n_args_wanted))
	  != NULL) {
	*rcvr = __ctalk_receivers[i];
      }
    }

  }

  if (warn && !m)
    _warning
      ("__ctalkFindClassMethodByName: %s: %s (Class %s) does not understand %s.\n",
       __argvFileName (), (*rcvr) -> __o_name, (*rcvr) -> CLASSNAME, name);

  return m;
}

METHOD *__ctalkFindClassMethodByFn (OBJECT **rcvr, OBJECT *(*fn)(void), 
				    int warn) {

  int i;
  METHOD *m = NULL;

  if ((m = __ctalkGetClassMethodByFn (*rcvr, fn, warn)) == NULL) {
    for (i = __ctalk_receiver_ptr + 1; !m && (i <= MAXARGS); i++) {
      if ((m = 
	   __ctalkGetClassMethodByFn (__ctalk_receivers[i], fn, warn))
	  != NULL) {
	*rcvr = __ctalk_receivers[i];
      }
    }

  }

  if (warn && !m)
    _warning
      ("__ctalkFindClassMethodByFn: %s: %s (Class %s): undefined method.\n",
       __argvFileName (), (*rcvr) -> __o_name, (*rcvr) -> CLASSNAME);

  return m;
}

METHOD *__ctalkFindMethodByName (OBJECT **rcvr_p, const char *name, int warn,
				 int n_args_wanted) {

  METHOD *m = NULL;

  if (IS_CLASS_OBJECT (*rcvr_p)) {
    if ((m = __ctalkFindClassMethodByName (rcvr_p, name, warn, 
					   n_args_wanted)) != NULL) {
      return m;
    }
  }

  if (((m = __ctalkFindInstanceMethodByName (rcvr_p, name, warn,
					     n_args_wanted)) == NULL) &&
      ((m = __ctalkFindClassMethodByName (rcvr_p, name, warn,
					  n_args_wanted)) == NULL) &&
      ((m = __ctalkFindPrefixMethodByName (rcvr_p, name, warn)) == NULL))
    if (warn)
_warning ("__ctalkFindMethodByName: Object %s (Class %s) does not understand method %s.\n",
  (*rcvr_p) -> __o_name, (*rcvr_p) -> CLASSNAME, name);

return m;
}

static void __super_move_args  (OBJECT *rcvr,
				const char *method_name) {

  OBJECT *class_obj;
  METHOD *method, *superclass_method;
  int i, arg_ptr;

  class_obj = rcvr -> __o_class;


  /* These are constructors, so n_args_wanted should be 1, which
     the fn can use later if it needs stricter arg checking. */
  method = __ctalkGetInstanceMethodByName (rcvr, method_name, TRUE,
					   ANY_ARGS);
  superclass_method = 
    __ctalkGetInstanceMethodByName (class_obj -> __o_superclass, 
				    method_name, TRUE, ANY_ARGS);


  for (i = 0, arg_ptr = __ctalk_arg_ptr + 1; 
       i < method -> n_args; i++, arg_ptr++) {
    if (method -> arg_frame_top && (arg_ptr >= method -> arg_frame_top))
      break;
    superclass_method -> args[i] = method -> args[method -> n_args - i - 1];
    method -> args[method -> n_args - i - 1] = NULL;
    ++superclass_method -> n_args;
    --method -> n_args;
  }

}

/* The arguments are stored in the method only. 
   The prototype allows us to add an arbitrary number 
   of arguments to the function call.
   Note that at this time, _ctalk_define_method () does
   not use the primitive method prototype (*func)(OBJECT **args).
*/
int primitive_method_call_attrs;

static OBJECT *Object_obj = NULL;

void __ctalk_primitive_method (char *receiver_name, char *name, int attrs) {

  OBJECT *receiver = NULL,  /* Avoid warnings. */
    *receiver_orig = NULL; 
  METHOD *method, *m;
  int i;
  OBJECT *(*cfunc)();

  primitive_method_call_attrs = attrs;
  if (attrs & METHOD_SUPER_ATTR) {
    receiver_orig = __ctalkGetClass (receiver_name);
    receiver = receiver_orig -> __o_superclass;
    __super_move_args (receiver_orig, name);
  } else {
    receiver = __ctalkGetClass (receiver_name);
  }

  if (Object_obj == NULL)
    Object_obj = get_class_by_name (OBJECT_CLASSNAME);

  method = NULL;
  for (m = Object_obj -> instance_methods; m; m = m -> next) {
    if (str_eq (m -> name, name)) {
      method = m;
      break;
    }
  }
  /* Fall back to a full search if we didn't find a primitive method. */
  if (!method) {
    method = __ctalkGetInstanceMethodByName (receiver, name, FALSE, ANY_ARGS);
    if (!method) {
      char text[0xffff];
      sprintf (text, "Method, \"%s,\" receiver %s (class %s).",
	       name, receiver -> __o_name, receiver -> CLASSNAME);
      __ctalkExceptionInternal (NULL, undefined_method_x, text, 0);
      return;
    }
  }

  method -> arg_frame_top = __ctalk_arg_ptr + 1;

  __save_rt_info (receiver, receiver -> __o_class, NULL, 
		  method, method -> cfunc, False);

  __ctalk_receiver_push (receiver);
  cfunc = method -> cfunc;
  (void)(cfunc) (method -> args);

  __restore_rt_info ();
  __ctalk_receiver_pop ();

  for (i = 0; i < method -> n_params; i++) {
    if (IS_ARG(method -> args[i])) {
      if (IS_OBJECT (ARG_OBJECT(method -> args[i]))) {
	__ctalkDeleteObjectInternal (ARG_OBJECT(method -> args[i]));
      }
      __ctalkDeleteArgEntry (method->args[i]);
      method -> args[i] = NULL;
    }
    (void)__ctalk_arg_pop ();
  }
  method -> n_args = 0;
}

int __ctalkDefineInstanceMethod (char *classname, char *name, 
			 OBJECT *(*cfunc)(), int required_args, int attrs) {
  METHOD *tm;
  OBJECT *class;
  char hbuf[MAXMSG];
  
  if ((class = __ctalkGetClass (classname)) == NULL)
    _error ("__ctalk_define_method: %s: Undefined class %s", 
	    rtinfo.source_file, classname);

  if (class -> instance_methods == NULL) {
    class -> instance_methods = (METHOD *)__xalloc (sizeof (struct _method));
    class -> instance_methods -> sig = METHOD_SIG;
    strcpy (class -> instance_methods -> name, name);
    class->instance_methods-> n_params = required_args;
    /* 
       This is redundant with __xalloc () which initializes the memor to zero already.
      
    class->instance_methods-> n_args = 0;
    class -> instance_methods -> n_user_objs = 0;
    */
    class->instance_methods-> cfunc = cfunc;
    class->instance_methods-> rcvr_class_obj = class;
    if (attrs & METHOD_PREFIX_ATTR)
      class->instance_methods->prefix = TRUE;
    if (attrs & METHOD_VARARGS_ATTR) 
      class->instance_methods->varargs = TRUE;
    if (attrs & METHOD_NOINIT_ATTR)
      class->instance_methods->varargs = TRUE;
  } else {
    for (tm = class -> instance_methods; tm -> next; tm = tm -> next)
      ;
    tm -> next = (METHOD *)__xalloc (sizeof (struct _method));
    tm -> next -> sig = METHOD_SIG;
    strcpy (tm -> next -> name, name);
    tm -> next -> n_params = required_args;
    tm -> next -> rcvr_class_obj = class;
    if (attrs & METHOD_PREFIX_ATTR)
      tm -> next -> prefix = TRUE;
    if (attrs & METHOD_VARARGS_ATTR)
      tm -> next -> varargs = TRUE;
    if (attrs & METHOD_NOINIT_ATTR)
      tm -> next -> varargs = TRUE;
    /*
    tm -> next -> n_args = 0;
    tm -> next -> n_user_objs = 0;
    */
    tm -> next -> cfunc = cfunc;
    tm -> next -> prev = tm;
  }
  hash_instance_method (classname, name);
  return SUCCESS;
}

int __ctalkDefineClassMethod (char *classname, char *name, 
			     OBJECT *(*cfunc)(), int required_args) {
  static METHOD *m;
  METHOD *tm;
  OBJECT *class;
  
  if ((class = __ctalkGetClass (classname)) == NULL)
    _error ("__ctalk_define_method: %s: Undefined class %s", 
	    rtinfo.source_file, classname);

  m = (METHOD *)__xalloc (sizeof (struct _method));

  m -> sig = METHOD_SIG;
  strcpy (m -> name, name);
  m -> n_params = required_args;
  m -> n_args = 0;
  m -> cfunc = cfunc;
  /*
   *  Very kludgy, but needed for templates.
   */
  if (strstr (name, "printf") ||
      strstr (name, "Printf") ||
      strstr (name, "scanf") ||
      strstr (name, "Scanf"))
    m -> varargs = TRUE;

  if (class -> class_methods == NULL) {
    class -> class_methods = m;
  } else {
    for (tm = class -> class_methods; tm -> next; tm = tm -> next)
      ;
    tm -> next = m;
    m -> prev = tm;
  }
  hash_class_method (classname, name);
  return SUCCESS;
}

int __ctalkDefineTemplateMethod (char *classname, char *name, 
				 OBJECT *(*cfunc)(), int required_args,
				 int n_args) {
  OBJECT *class;
  METHOD *tm;

  __ctalkDefineClassMethod (classname, name, cfunc, required_args);

  /*
   *  method -> n_args is actually set in __rt_method_args.
   *  eval_expr only needs method -> n_params.
   */
   if ((class = __ctalkGetClass (classname)) == NULL)
     _error ("__ctalkDefineTemplateMethod: %s: Undefined class %s",
 	    rtinfo.source_file, classname);

   for (tm = class -> class_methods; tm; tm = tm -> next) {
     if (tm -> cfunc == cfunc) {
       tm -> n_args = 0;
     }
   }
  return SUCCESS;
}

/*
 *   TO DO - These functions get called every time the method
 *   is called.  Like all other method init functions, make 
 *   sure that it is called only as often as necessary.
 *
 *   TO DO - rtinfo.rcvr_class_obj should always be accurate, but 
 *   make sure that it doesn't change here.
 */

void __ctalkMethodReturnClass (char *classname) {

  METHOD *m;

  if (((m = __ctalkFindInstanceMethodByFn (&rtinfo.rcvr_class_obj, 
					   rtinfo.method_fn,
					   FALSE))
       != NULL) ||
      ((m = __ctalkFindClassMethodByFn (&rtinfo.rcvr_class_obj, 
					rtinfo.method_fn,
					FALSE)) != NULL)) {
    strcpy (m -> returnclass, classname);
  } else {
    _warning ("%s: __ctalk_methodReturnClass: undefined instance method.\n",
	      __argvFileName ());
  }
}

void __ctalkInstanceMethodInitReturnClass (char *rcvr_class, 
					   char *method_name, 
					   OBJECT *(*c_fn)(), 
					   char *return_class,
					   int n_params) {
  OBJECT *rcvr_class_obj;
  METHOD *m;

  for (rcvr_class_obj = __ctalk_classes; 
       rcvr_class_obj; rcvr_class_obj = rcvr_class_obj -> next)
    if (str_eq (rcvr_class_obj -> __o_name, (char *)rcvr_class))
      break;

  if (!rcvr_class_obj) {
    _warning ("__ctalkInstanceMethodInitReturnClass: Undefined class %s.\n", 
	      rcvr_class);
    return;
  }

  for (m = rcvr_class_obj -> instance_methods; m; m = m -> next)
    if (m -> cfunc == c_fn)
      break;

  if (!m || !IS_METHOD(m)) {
    _warning ("__ctalkInstanceMethodInitReturnClass: Undefined method %s (class %s).\n", 
	      method_name, rcvr_class);
    return;
  }

  strcpy (m -> returnclass, return_class);

}

void __ctalkClassMethodInitReturnClass (char *rcvr_class, 
					   char *method_name, 
					char *return_class,
					int n_params) {
  OBJECT *rcvr_class_obj;
  METHOD *m;

  if ((rcvr_class_obj = __ctalkGetClass (rcvr_class)) == NULL)
    _warning ("Undefined class %s.\n", rcvr_class);

  if ((m = __ctalkFindClassMethodByName(OBJREF (rcvr_class_obj),
					method_name, TRUE,
					n_params)) == NULL)
    _warning ("Undefined method %s (class %s).\n", 
	      method_name, rcvr_class);
  if (m)
    strcpy (m -> returnclass, return_class);
}

static bool is_reffed_outside_method (OBJECT *obj) {
  if ((obj -> scope & VAR_REF_OBJECT) ||
      (obj -> scope & METHOD_USER_OBJECT) ||
      IS_OBJECT(obj -> __o_p_obj))
    return true;
  else
    return false;
}

static int is_declared_outside_method (OBJECT *tgt) {
  VARENTRY *v, *v_t;
  int i;
  /*
   *  Start looking in the scope before the current method 
   *  == __call_stack_ptr + 2.
   */
  for (i = __call_stack_ptr + 2; i <= MAXARGS; i++) {
    if (__call_stack[i] -> method) {
      v = __call_stack[i] -> method -> local_objects[0].vars;
      for (v_t = v; v_t; v_t = v_t -> next)
	if (v_t -> var_object == tgt)
	  return TRUE;
    } else {
      v = __call_stack[i] -> _rt_fn -> local_objects.vars;
      for (v_t = v; v_t; v_t = v_t -> next)
	if (v_t -> var_object == tgt)
	  return TRUE;
    }
  }
  for (v_t = __ctalk_dictionary; v_t; v_t = v_t -> next)
    if (v_t -> var_object == tgt)
      return TRUE;
  return FALSE;
}

static int is_declared_outside_method_2 (OBJECT *tgt) {
  return is_declared_outside_method (tgt) || is_receiver (tgt) ||
    is_arg (tgt);
}

static inline void __delete_LOCAL_VAR_scope (OBJECT *o) {
  if (!(o -> attrs & OBJECT_HAS_LOCAL_TAG)) {
    __ctalkSetObjectScope (o, o -> scope & ~LOCAL_VAR);
  }
}

static inline void __delete_local_object_internal (VARENTRY *__v) {

  if (!IS_OBJECT (__v -> var_object))
    return;

  if (IS_VALUE_INSTANCE_VAR(__v -> var_object)) {
    __v -> var_object = NULL;
    return;
  }

  if (__v -> orig_object_rec == __v -> var_object) {
    if (!is_declared_outside_method_2 (__v -> var_object)) {
      if (!is_reffed_outside_method (__v -> var_object)) {
	__ctalkDeleteObject ( __v -> var_object);
      } else {
	__objRefCntDec (OBJREF (__v -> var_object));
	__delete_LOCAL_VAR_scope (__v -> var_object);
	if (IS_VARTAG(__v -> var_object -> __o_vartags))
	  __v -> var_object -> __o_vartags -> tag = NULL;
	__ctalkRegisterUserObject (__v -> var_object);
      }
    }
  } else { /* if (__v -> orig_object_rec == __v -> var_object) */
    if (!is_declared_outside_method_2 (__v -> var_object)) {
      if (!is_reffed_outside_method (__v -> var_object)) {
	__ctalkDeleteObject ( __v -> var_object);
      } else {
	__objRefCntDec (OBJREF (__v -> var_object));
	__delete_LOCAL_VAR_scope (__v -> var_object);
	if (IS_VARTAG(__v -> var_object -> __o_vartags) &&
	    !IS_EMPTY_VARTAG(__v -> var_object -> __o_vartags))
	  __v -> var_object -> __o_vartags -> tag = NULL;
	__ctalkRegisterUserObject (__v -> var_object);
      }
    }

    if (IS_OBJECT (__v -> orig_object_rec)) {
      if (!is_declared_outside_method_2 (__v -> orig_object_rec)) {
	if (!is_reffed_outside_method (__v -> orig_object_rec)) {
	  __ctalkDeleteObject ( __v -> orig_object_rec);
	} else {
	  __objRefCntDec (OBJREF (__v -> orig_object_rec));
	  __delete_LOCAL_VAR_scope (__v -> orig_object_rec);
	  if (IS_VARTAG(__v -> orig_object_rec -> __o_vartags) &&
	      !IS_EMPTY_VARTAG(__v -> orig_object_rec -> __o_vartags))
	    __v -> orig_object_rec -> __o_vartags -> tag = NULL;
	  __ctalkRegisterUserObject (__v -> orig_object_rec);
	}
      }
    }

  } /* if (__v -> orig_object_rec == __v -> var_object) */
}

/*
 *  Initialize local objects during a method or function init by 
 *  deleting old objects, if any.  If we're calling from a method,
 *  the local objects are stored in the METHOD struct.  If calling
 *  from a function, they are stored in __ctalk_dictionary, where
 *  we delete all objects that are not GLOBAL_VAR in scope.
 */
/* int __local_object_init = FALSE; *//***/
extern int __cleanup_deletion;
void __ctalk_initLocalObjects (void) {

  METHOD *__m;

  if ((__m = __call_stack[__call_stack_ptr+1] -> method) == NULL) {
    _warning ("__ctalk_initLocalObjects: Unknown method.\n");
    return;
  }
  if (is_successive_method_call (__m)) {
    register_successive_method_call ();
  }
  /*
   *  Local objects must absolutely be deleted here before the
   *  method creates new local objects.
   */
  __cleanup_deletion = TRUE;
  if (M_LOCAL_VAR_LIST(__m)) {
    VARENTRY *__v, *__v_prev;

    __v = last_varentry (M_LOCAL_VAR_LIST(__m));

    while (__v != M_LOCAL_VAR_LIST(__m)) {
      __v_prev = __v -> prev;
      __delete_local_object_internal (__v);
      delete_varentry (__v);
      __v = __v_prev;
      if (!__v) break;
    }
    if (M_LOCAL_VAR_LIST(__m) && M_LOCAL_VAR_LIST(__m) -> var_object)
      __delete_local_object_internal (__v);
    if (M_LOCAL_VAR_LIST(__m))
      delete_varentry (M_LOCAL_VAR_LIST(__m));
    M_LOCAL_VAR_LIST(__m) = NULL;
    /* __local_object_init = FALSE; *//***/
  }
  __cleanup_deletion = FALSE;
}

/*
 *  This should replace save_local_objects_to_extra () whereever we
 *  deal with successive method calls, because it observes scoping
 *  more correctly as methods return to the parent scope.  This also 
 *  helps prevent cross-linking of method user objects, so we should
 *  try work it in whenever necessary or possible.
 */
void init_extra_objects (void) {

  METHOD *__m;

  if ((__m = __call_stack[__call_stack_ptr+1] -> method) == NULL) {
    _warning ("__ctalk_initLocalObjects: Unknown method.\n");
    return;
  }
  /*
   *  Local objects must absolutely be deleted here before the
   *  method creates new local objects.
   */
  if (M_LOCAL_VAR_LIST(__m)) {
    __cleanup_deletion = TRUE;
    VARENTRY *__v, *__v_prev;

    __v = last_varentry (M_LOCAL_VAR_LIST(__m));
    
    while (__v != M_LOCAL_VAR_LIST(__m)) {
      __v_prev = __v -> prev;
      __delete_local_object_internal (__v);
      delete_varentry (__v);
      __v = __v_prev;
      if (!__v) break;
    }
    if (M_LOCAL_VAR_LIST(__m) && M_LOCAL_VAR_LIST(__m) -> var_object) { 
      __delete_local_object_internal (__v);
      delete_varentry (M_LOCAL_VAR_LIST(__m));
      M_LOCAL_VAR_LIST(__m) = NULL;
    }
    /* __local_object_init = FALSE; *//***/
    __cleanup_deletion = FALSE;
    --__m -> nth_local_ptr;
  }
}

/*
 *  __ctalkInstanceMethodParam () only needs to fill in the parameters
 *  immediately after the method is defined.  n_params is already
 *  filled in.  Because arguments get evaluated and then pushed in
 *  reverse order, doing the same for the parameter definitions makes
 *  things easier to follow later.
 *
 *  TO DO - The function should not depend on __ctalk_define_method ()
 *  to set n_params.  It should be a parameter in this function also.
 */

void __ctalkInstanceMethodParam (char *rcvrClass, 
				 char *method_name,
				 OBJECT *(*c_fn)(), 
				 char *paramClass, char *paramName, 
				 int isPointer) {

  OBJECT *classObj;
  METHOD *m;
  PARAM *p;
  int i;

  if ((classObj = __ctalkGetClass (rcvrClass)) == NULL) {
    _warning ("__ctalkInstanceMethodParam: Undefined class %s.\n",
	      rcvrClass);
    return;
  }

  for (m = classObj -> instance_methods; m; m = m -> next)
    if (m -> cfunc == c_fn)
      break;

  if (!m || !IS_METHOD (m)) {
    _warning ("__ctalkInstanceMethodParam: Undefined method %s (Class %s).\n",
	      method_name, rcvrClass);
    return;
  }

  p = new_param ();
  strcpy (p -> class, paramClass);
  strcpy (p -> name, paramName);
  if (!strcmp (p -> name, "..."))
    m -> varargs = TRUE;
  p -> is_ptr = isPointer;

  for (i = m -> n_params - 1; ; i--) {
    if (! m -> params[i] || ! IS_PARAM (m -> params[i])) {
      m -> params[i] = p;
       return; 
    }
    if (i < 0) {
      _warning ("__ctalkInstanceParam: too many parameters.\n");
       return;
    }
  }
}

void __ctalkClassMethodParam (char *rcvrClass, 
			      char *method_name,
			      OBJECT *(*c_fn)(), 
			      char *paramClass, char *paramName, 
			      int isPointer) {

  OBJECT *classObj;
  METHOD *m;
  PARAM *p;
  int i;

  if ((classObj = __ctalkGetClass (rcvrClass)) == NULL) {
    _warning ("__ctalk_methodParam: Undefined class %s.\n",
	      rcvrClass);
    return;
  }

  for (m = classObj -> class_methods; m; m = m -> next)
    if (m -> cfunc == c_fn)
      break;

  if (!m || !IS_METHOD (m)) {
    _warning ("__ctalkClassMethodParam: Undefined method %s (Class %s).\n",
	      method_name, rcvrClass);
    return;
  }

  p = new_param ();
  strcpy (p -> class, paramClass);
  strcpy (p -> name, paramName);
  if (!strcmp (p -> name, "..."))
    m -> varargs = TRUE;
  p -> is_ptr = isPointer;

  for (i = m -> n_params - 1; ; i--) {
    if (! m -> params[i] || ! IS_PARAM (m -> params[i])) {
      m -> params[i] = p;
       return; 
    }
    if (i < 0) {
      _warning ("__ctalkClassMethodParam: too many parameters.\n");
       return;
    }
  }
}

/*
 *  An empty argument list is the token immediately following
 *  a method that is a comma, semicolon, closing parenthesis,
 *  or closing square brace.
 *
 *  The function also checks that the starting token of the 
 *  argument list immediately follows a method token.
 */

int _is_empty_arglist (MESSAGE_STACK messages, int arglist_start, 
		       int stack_start) {

  int m_prev_token;

  if ((m_prev_token = 
       __ctalkPrevLangMsg (messages, arglist_start, stack_start)) == ERROR)
    _error ("Parser error.");

  if ((messages[m_prev_token] -> tokentype == METHODMSGLABEL) &&
      METHOD_ARG_TERM_MSG_TYPE (messages[arglist_start]))
    return TRUE;
  else
    return FALSE;
}

/*
 *  Find the end of an argument list, recursing if necessary
 *  for functions and methods as arguments.  
 *
 *  If the argument list begins with a parenthesis, simply
 *  find the matching closing parenthesis.
 *
 *  The parameter, n_args, should come from the method, or -1 
 *  if the number of args is unknown.  
 * 
 *  Empty argument lists get handled immediately.  They generate
 *  warning messages in method_args () if method -> n_args > 0.
 */

int __ctalkArglistLimit (MESSAGE_STACK messages, int arglist_start, int n_args,
			 int stack_top, int stack_start) {

  MESSAGE *m;
  int i,
    end_list,
    n_parens,
    lookahead,
    start_token;

  if (!n_args && _is_empty_arglist (messages, arglist_start, stack_start))
    return arglist_start;

  for (i = arglist_start, n_parens = 0, end_list = -1, start_token = -1; 
       (i > stack_top) && (end_list == -1); i--) {

    m = messages[i];

    if (M_ISSPACE(m))
      continue;

    if (start_token == -1)
      start_token = m -> tokentype;

    switch (m -> tokentype)
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	/*
	 *  If the first token is a closing parenthesis, then
	 *  the argument list is empty.
	 */
	if (i == arglist_start)
	  end_list = arglist_start;

	  /* 
	   *  If the next token ends an argument, then this parenthesis
	   *  ends the argument list.  Otherwise, continue to scan forward.
	   */
	if (n_parens) {
	  --n_parens;
	  /*
	   *  If a matching closing parenthesis, then check whether
	   *  there are still tokens to the argument.
	   */
	  if (!n_parens) {
	    if ((lookahead = __ctalkNextLangMsg (messages, i, stack_top)) 
		== ERROR) {
	      _warning ("Argument syntax error.");
	      end_list = arglist_start;
	    }
	    switch (messages[lookahead] -> tokentype)
	      {
	      case CLOSEPAREN:
	      case ARGSEPARATOR:
	      case SEMICOLON:
		end_list = i;
		break;
	      }
	  }
	} else {
	  /*
	   *  If we encounter a closing parenthesis without an opening
	   *  parenthesis, then the argument list ends at the previous
	   *  token.
	   */
	  if (!n_parens)
	    end_list = i + 1;
	}
	break;
      case SEMICOLON:
	end_list = i + 1;
	break;
      default:
	break;
      }
  }

  return end_list;
}

/*
 *  Check that a set of parentheses encloses an _entire_expression_;
 *  that is, we don't cut off the first and last parentheses of 
 *  an expression like (1 + 1) - (2 + 2). 
 * 
 *  To simplify this, start_ptr must point to the first opening 
 *  parenthesis, and end_ptr must point to the last token of
 *  the expression.
 */

int _external_parens (MESSAGE_STACK messages, int start_ptr, int end_ptr) {

  int i,
    n_parens;
  bool have_argseparator = false;

  for (i = start_ptr, n_parens = 0; i >= end_ptr; i--) {

    switch (messages[i] -> tokentype)
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (!n_parens && (i != end_ptr))
	  return FALSE;
	if (!n_parens && (i == end_ptr)) {
 	  if (have_argseparator) {
	    /* It's an argument list... */
	    return TRUE;
	  } else {
	    /* ... or it's a single term. */
	    return FALSE;
	  }
	}
	break;
      case ARGSEPARATOR:
	have_argseparator = true;
	break;
      default:
	break;
      }
  }

  return FALSE;
}

OBJECT *__ctalk_methodReturn (OBJECT *rcvr, METHOD *m, OBJECT *return_obj) {
  return NULL;
}

METHOD *__this_method (void) {
  OBJECT *(*method_fn)(void);
  OBJECT *rcvr_class_obj;
  METHOD *method;

  if ((method_fn = rtinfo.method_fn) != NULL) {
    /*
     *  If there is a receiver class object, then use 
     *  that to find the method.
     */
    if ((rcvr_class_obj = rtinfo.rcvr_class_obj) != NULL) {
      if (((method = __ctalkFindInstanceMethodByFn (OBJREF (rcvr_class_obj), 
				   method_fn, FALSE)) == NULL) &&
	  ((method = __ctalkFindClassMethodByFn (OBJREF (rcvr_class_obj), 
						 method_fn, FALSE)) == NULL)){
	_warning ("Undefined method in this_method ().\n");
	return NULL;
      }
      return method;
    } else {
      /*
       * Otherwise, perform a search of the classes.
       */
      for (rcvr_class_obj = __ctalk_classes; rcvr_class_obj; 
	   rcvr_class_obj = rcvr_class_obj -> next) {
	for (method = rcvr_class_obj -> instance_methods; method;
	     method = method -> next) {
	  if (method -> cfunc == method_fn)
	    return method;
	}
      }
      for (rcvr_class_obj = __ctalk_classes; rcvr_class_obj; 
	   rcvr_class_obj = rcvr_class_obj -> next) {
	for (method = rcvr_class_obj -> class_methods; method;
	     method = method -> next) {
	  if (method -> cfunc == method_fn)
	    return method;
	}
      }
    }
  }
  _warning ("Undefined method in this_method ().\n");
  return NULL;
}

int __ctalkIsInstanceMethod (OBJECT *self, char *methodname) {
  OBJECT *class_object;
  METHOD *m;
  if (!self || EMPTY_STR(methodname))
    return FALSE;
  class_object = IS_VALUE_INSTANCE_VAR(self) ? self -> __o_class :
    self -> instancevars -> __o_class;
  for (m = class_object -> instance_methods; m; m = m -> next) {
    if (!strcmp (methodname, m -> name))
      return TRUE;
  }
  return FALSE;
}

int __ctalkIsClassMethod (OBJECT *self, char *methodname) {
  OBJECT *class_object;
  METHOD *m;
  class_object = IS_VALUE_INSTANCE_VAR(self) ? self -> __o_class :
    self -> instancevars -> __o_class;
  for (m = class_object -> class_methods; m; m = m -> next) {
    if (!strcmp (methodname, m -> name))
      return TRUE;
  }
  return FALSE;
}

bool is_method_param (char *name) {
  METHOD *method;
  int i;
  if (__ctalkIsInlineCall ()) {
    if ((method = __call_stack[__call_stack_ptr + 3] -> method) != NULL) {
      for (i = 0; i < method -> n_params; ++i) {
	if (str_eq (name, method -> params[i] -> name))
	  return true;
      }
    }
  } else {
    if ((method = __ctalkRtGetMethod ()) != NULL) {
      for (i = 0; i < method -> n_params; ++i) {
	if (str_eq (name, method -> params[i] -> name))
	  return true;
      }
    }
  }
  return false;
}

static METHOD *match_instance_method_greedy (OBJECT *class, char *name,
					     METHOD **mset,
					     int *n_methods) {
  METHOD *t;
  for (t = class -> instance_methods; t; t = t -> next) {
    if (str_eq (t -> name, name)) {
      mset[*n_methods] = t;
      ++*n_methods;
    }
  }
  if (*n_methods > 0)
    return mset[0];
  else
    return NULL;
}

static METHOD *match_class_method_greedy (OBJECT *class, char *name,
					  METHOD **mset,
					  int *n_methods) {
  METHOD *t;
  for (t = class -> class_methods; t; t = t -> next) {
    if (str_eq (t -> name, name)) {
      mset[*n_methods] = t;
      ++*n_methods;
    }
  }
  if (*n_methods > 0)
    return mset[0];
  else
    return NULL;
}

METHOD *get_method_greedy (OBJECT *rcvr, char *name, METHOD **mset,
			   int *n_methods) {
  METHOD *m_ret = NULL, *m_match;
  OBJECT *class, *c;

  if (rcvr == NULL)
    return NULL;
  
  *n_methods = 0;
  class = rcvr -> instancevars ? rcvr -> instancevars -> __o_class :
    rcvr -> __o_class;

  for (c = class; c; c = c -> __o_superclass) {
    if ((m_match = match_instance_method_greedy (c, name, mset, n_methods))
	!= NULL) {
      return m_match;
    }
  }

  for (c = class; c; c = c -> __o_superclass) {
    if ((m_match = match_class_method_greedy (c, name, mset, n_methods))
	!= NULL) {
      return m_match;
    }
  }

  /* If we have an inline method call (eg "map*") check the receiver's
     class */
  if (!IS_OBJECT(__call_stack[__call_stack_ptr + 1] -> rcvr_class_obj))
    return NULL;
  if ((class = __call_stack[__call_stack_ptr + 1] ->
       rcvr_class_obj -> __o_class) == NULL) {
    return NULL;
  }

  for (c = class; c; c = c -> __o_superclass) {
    if ((m_match = match_instance_method_greedy (c, name, mset, n_methods))
	!= NULL) {
      return m_match;
    }
  }

  for (c = class; c; c = c -> __o_superclass) {
    if ((m_match = match_class_method_greedy (c, name, mset, n_methods))
	!= NULL) {
      return m_match;
    }
  }

  return NULL;
}
			     
