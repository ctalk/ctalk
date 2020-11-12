/* $Id: rt_obj.c,v 1.6 2020/11/12 20:06:59 rkiesling Exp $ */

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
#include <stdint.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"


extern VARENTRY *__ctalk_dictionary;
extern VARENTRY *__ctalk_last_object;

extern OBJECT *__ctalk_classes;
extern OBJECT *__ctalk_last_class;

extern I_PASS interpreter_pass;             /* Declared in rtinfo.c.   */
extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

extern OBJECT *__ctalk_argstack[MAXARGS+1];
extern int __ctalk_arg_ptr;

extern int object_become_delete;
extern int object_become_copy;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

extern int eval_status;

extern RT_INFO rtinfo;

extern OBJECT *__create_object_internal (char *name, char *class, 
					 char *superclass,
					 char *value, int scope, int attrs);

extern DEFAULTCLASSCACHE *rt_defclasses;

static inline VARENTRY *last_varentry (VARENTRY *start) {
  VARENTRY *v;
  if (start) {
    for (v = start; v -> next; v = v -> next)
      ;
    return v;
  } else {
    return NULL;
  }
}

static inline OBJECT *active_tag_obj_v (VARENTRY *v) {
  OBJECT *o, *o_class, *o_superclass;
  if ((o = (IS_OBJECT (v -> var_object) ? v -> var_object :
	    v -> orig_object_rec)) != NULL) {
    make_tag_for_varentry_active (o, v);
    if (IS_OBJECT(o)) {
      return o;
    }
  } else {
    o_class = __ctalkGetClass (v -> var_decl -> class);
    o_class = __ctalkGetClass (v -> var_decl -> class);
    o_superclass = o_class -> __o_superclass ? 
      o_class -> __o_superclass : NULL;
    o = __ctalkCreateObjectInit (NULLSTR, o_class -> __o_name,
				 ((o_superclass) ? o_superclass -> __o_name :
				  NULL),
				 CREATED_PARAM, "0");
    o -> attrs |= OBJECT_IS_NULL_RESULT_OBJECT;
    return o;
  }
  return NULL;
}

static inline OBJECT *active_tag_obj (OBJECT *o) {
  if (HAS_VARTAGS(o) &&
      IS_VARENTRY(o -> __o_vartags -> tag)) {
    return active_tag_obj_v (o -> __o_vartags -> tag);
  } else {
    return o;
  }
}

static OBJECT *object_from_argblk_caller (const char *__name,
					  const char *__classname) {
  METHOD *__m;
  VARENTRY *__v;
  RT_FN *rt_fn;
  int i;
  
  if (__ctalkIsArgBlockScope ()) {
    if ((__m = __ctalkBlockCallerMethod ()) != NULL) {
      for (i = 0; i < __m -> n_params; i++) {
	if (str_eq (__m -> params[i] -> name, (char *)__name))
	  return __m -> args[__m -> n_params - i - 1] -> obj;
      }
      if (M_LOCAL_VAR_LIST(__m)) {
	for (__v = M_LOCAL_VAR_LIST(__m); __v; __v = __v -> next) {
	  if (__classname) {
	    if (str_eq (__v -> var_decl -> name, (char *)__name) && 
		str_eq (__v -> var_decl -> class, (char *)__classname))
	      return active_tag_obj_v (__v);
	  } else {
	    if (str_eq (__v -> var_decl -> name, (char *)__name))
	      return active_tag_obj_v (__v);
	  }
	}
      }
    } else {
      if ((rt_fn = __ctalkBlockCallerFn ()) != NULL) {
	for (__v = rt_fn -> local_objects.vars; __v; __v = __v -> next) {
	  if (__classname) {
	    if (str_eq (__v -> var_decl -> name, (char *)__name) && 
		str_eq (__v -> var_decl -> class, (char *)__classname))
	      return active_tag_obj_v (__v);
	  } else {
	    if (str_eq (__v -> var_decl -> name, (char *)__name))
	      return active_tag_obj_v (__v);
	  }
	}
      }
    }
  }
  return NULL;
}
  
static OBJECT *object_from_argblk_caller_fn (const char *__name,
					     const char *__classname) {
  RT_FN *rt_fn;
  VARENTRY *__v;
  if (__ctalkIsArgBlockScope ()) {
    if ((rt_fn = __ctalkBlockCallerFn ()) != NULL) {
      for (__v = rt_fn -> local_objects.vars; __v; __v = __v -> next) {
	if (__classname) {
	  if (str_eq ((char *)__name, __v -> var_decl -> name) && 
	      str_eq ((char *)__classname, __v -> var_decl -> class))
	    return active_tag_obj_v (__v);
	} else {
	  if (str_eq ((char *)__name, __v -> var_decl -> name))
	    return active_tag_obj_v (__v);
	}
      }
    }
  }
  return NULL;
}

/* FOR NOW, these two functions deal with just a single-variable 
   varlist, called from save_local_objects_to_extra_b */
static void cleanup_local_object_cache (void) {
  RT_INFO *r;
  OBJECT *o_tmp, *o_tmp_orig;
  VARENTRY *v;
  r = __call_stack[__call_stack_ptr+1];
  v = r -> local_object_cache[r -> local_obj_cache_ptr - 1];
  o_tmp = v -> var_object;
  unlink_varentry (o_tmp);
  o_tmp -> __o_vartags -> tag = NULL;
  __ctalkRegisterExtraObjectInternal (o_tmp, r -> method);
  if (v -> orig_object_rec != o_tmp) {
    o_tmp_orig = v -> orig_object_rec;
    unlink_varentry (o_tmp_orig);
    __ctalkRegisterExtraObjectInternal (o_tmp_orig, r -> method);
  }
  delete_varentry (r -> local_object_cache[r -> local_obj_cache_ptr - 1]);
  r -> local_object_cache[r -> local_obj_cache_ptr - 1] = NULL;
}

static OBJECT *obj_from_local_object_cache (const char *name,
					    const char *classname) {
  RT_INFO *r;
  VARENTRY *v;
  r = __call_stack[__call_stack_ptr+1];
  v = r -> local_object_cache[r -> local_obj_cache_ptr - 1];
  if (str_eq (v -> var_decl -> name, (char *)name)) {
    if (classname) {
      if (str_eq (v -> var_decl -> class, (char *)classname)) {
	return v -> var_object;
      } else {
	return NULL;
      }
    } else {
      return v -> var_object;
    }
  }
  return NULL;
}

OBJECT *__ctalk_get_object_return (const char *name,
				   const char *classname) {
  RT_INFO *r;
  OBJECT *return_obj;
  r = __call_stack[__call_stack_ptr + 1];
  if (r -> local_obj_cache_ptr > 0) {
    /* See the comments above for what this can do. */
    if ((return_obj = obj_from_local_object_cache (name, classname))
	!= NULL) {
      cleanup_local_object_cache ();
      return return_obj;
    } else {
      cleanup_local_object_cache ();
    }
    --r -> local_obj_cache_ptr;
  }
  return __ctalk_get_object (name, classname);
}

/* 
 *   TO DO - For now, if the class name is NULL, simply return 
 *   the first object that matches the name.  Add a 
 *   contextualization facility that allows the language to 
 *   qualify an ambiguous object with its class name.
 *
 */
OBJECT *__ctalk_get_object (const char *__name, const char *__classname) {

  OBJECT *__o, *result;
  VARENTRY *__v;
  METHOD *__m;
  RT_FN *rt_fn;
  int i;
  int arg_n;

  if (!IS_C_LABEL(__name))
    return NULL;

  /*
   *  Check for "self" keyword and parameter replacements.
   */

  if (str_eq ((char *)__name, "self")) {
    __o =  __ctalk_receivers[__ctalk_receiver_ptr+1];
    if (__classname) {
      if (str_eq (__o -> CLASSNAME, (char *)__classname))
	return __o;
      else
	return NULL;
    } else {
      return __o;
    }
  } else if (str_eq ((char *)__name, "super")) {
    // This is the super that works in arg block scope.
    // The super that modifies methods is in rt_expr.c.
    if (__ctalkIsArgBlockScope ()) {
      OBJECT *super_rcvr;
      if ((super_rcvr = __call_stack[__call_stack_ptr+3] -> rcvr_obj)
	  != NULL)
	return super_rcvr;
      if (__call_stack[__call_stack_ptr+3] -> _rt_fn &&
	  ((super_rcvr = __call_stack[__call_stack_ptr+2] -> rcvr_obj)
	   != NULL))
	return super_rcvr;

      _warning ("Keyword, \"super:\" Can't find receiver object.\n");

      return NULL;
	
    } else {
      return NULL;
    }
  } else if ((__call_stack_ptr < MAXARGS) &&
	     (__call_stack[__call_stack_ptr+1] -> inline_call)) {
    result = object_from_argblk_caller (__name, __classname);
    if (IS_OBJECT(result)) {
      return result;
    } else {
      result = object_from_argblk_caller_fn (__name, __classname);
      if (IS_OBJECT(result)) {
	return result;
      }
    }
  } else if ((__call_stack_ptr < MAXARGS) &&
	     ((rt_fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    for (__v = rt_fn -> local_objects.vars; __v; __v = __v -> next) {
      if (__classname) {
	if (str_eq ((char *)__name, __v -> var_decl -> name) && 
	    str_eq ((char *)__classname, __v -> var_decl -> class))
	  return active_tag_obj_v (__v);
      } else {
	if (str_eq ((char *)__name, __v -> var_decl -> name))
	  return active_tag_obj_v (__v);
      }
    }
  }

  if ((result = __ctalkGetClass (__name)) != NULL)
    return result;
  
  if (__call_stack_ptr < MAXARGS) {
    if ((__m = __call_stack[__call_stack_ptr + 1] -> method) != NULL) {
      int __nth_arg_frame_offset, __nth_arg;
      OBJECT *__arg_object;
      /*
       *  Handle args for methods called recursively by 
       *  checking each method arg against the argument 
       *  stack's. current frame.  Note that the 
       *  method's arg_frame_top may not always be the
       *  same relatively (sometimes it might point to the
       *  next arg index _after_ the call), so we don't use 
       *  it as a  bounds when checking the actual arguments 
       *  on the __arg_stack.
       *
       *  If there's not arg_frame_top given, then simply
       *  check the first args[0... n_params].
       */
      if (__m -> n_params && 
	  /* (__m -> arg_frame_top >= __arg_stack_ptr) && */
	  (__m -> arg_frame_top >= __ctalk_arg_ptr) &&
	  (__m -> arg_frame_top <= MAXARGS) &&
	  (!__m -> varargs)) {
	__nth_arg_frame_offset = 
	  (__m -> n_args == __m -> n_params) ?
	  0 : ((__m -> n_args / __m -> n_params) - 1);
      next_arg_frame:
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    __nth_arg = 
	      (__m->n_params-i-1)+__nth_arg_frame_offset;
	    if (__nth_arg < 0)
	      goto look_at_variables;
	    __arg_object = 
	      (((__nth_arg >= 0) && __m->args[__nth_arg]) ?
	       __m -> args[__nth_arg]->obj : NULL);
	    if (IS_OBJECT(__arg_object)) {
	      eval_status |= EVAL_STATUS_NAMED_PARAMETER;
	      return active_tag_obj (__arg_object);
	    }
	    __nth_arg_frame_offset -= __m -> n_params;
	    goto next_arg_frame;
	  }
	}
      } else  if (__m -> varargs) {
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    if (__m -> args[__m -> n_params - i - 1]) {
	       return __m -> args[__m -> n_params - i - 1] -> obj; 
	    } else {
	      _warning 
		("__ctalk_get_object: Method %s : wrong number of arguments.\n", 
		 __m -> name);
	      return null_result_obj (__m, ARG_VAR);
	    }
	  }
	}
      } else {
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    if (__m -> args[__m -> n_args - __m -> n_params + i]) {
	      return __m -> args[__m -> n_args - __m->n_params + i] -> obj;
	    } else {
	      _warning 
		("__ctalk_get_object: Method %s : wrong number of arguments.\n", 
		 __m -> name);
	      
	      return null_result_obj (__m, ARG_VAR);
	    }
	  }
	}
      }

    look_at_variables:
      if (M_LOCAL_VAR_LIST(__m)) {
	for (__v = M_LOCAL_VAR_LIST(__m); 
	     IS_VARENTRY(__v); __v = __v -> next) {
	  if (IS_OBJECT(__v -> var_object)) {
	    if (__classname) {
	      if (str_eq (__v -> var_decl -> name, (char *)__name) && 
		  str_eq (__v -> var_decl -> class, (char *)__classname))
		return active_tag_obj_v (__v);
	    } else {
	      if (str_eq (__v -> var_decl -> name, (char *)__name))
		return active_tag_obj_v (__v);
	    }
	  } else {
	    /*
	     *  May not be exactly the correct return value in all
	     *  cases, but it's safer to let the caller cope with
	     *  it.
	     */
	    if (__classname) {
	      if (str_eq (__v -> var_decl -> name, (char *)__name) && 
		  str_eq (__v -> var_decl -> class, (char *)__classname))
		return NULL;
	    } else {
	      if (!strncmp (__v -> var_decl -> name, (char *)__name,
			    strlen (__name)))
		return NULL;
	    }
	  }
	}
      }
    } /* if ((__m = __call_stack[__call_stack_ptr + 1] -> method) != NULL) */

    /*
     *  Check for a global object.
     */
    
    if (__ctalk_dictionary) {
      for (__v = __ctalk_dictionary; __v -> next; __v = __v -> next)
	;
      do {
	if (__classname) {
	  if (str_eq (__v -> var_decl -> name, (char *)__name) &&
	      str_eq (__v -> var_decl -> class, (char *)__classname))
	    return active_tag_obj_v (__v);
	} else {
	  if (str_eq (__v -> var_decl -> name, (char *)__name))
	    return active_tag_obj_v (__v);
	}
	if ((__v = __v -> prev) == NULL)
	  break;
      } while (TRUE);
    }

    /*
     *  Need a check for a class variable here.
     */
    if ((__o = __ctalk_find_classvar ((char *)__name, 
				      (char *)__classname)) != NULL)
      return __o;

    return NULL;
  }
  
  if (__ctalk_dictionary) {
    __v = __ctalk_last_object;
    do {
      if (__classname) {
	if (str_eq (__v -> var_decl -> name, (char *)__name) &&
	    str_eq (__v -> var_decl -> class, (char *)__classname))
	  return active_tag_obj_v (__v);
      } else {
	if (str_eq (__v -> var_decl -> name, (char *)__name))
	  return active_tag_obj_v (__v);
      }
    } while ((__v = __v -> prev) != NULL);
  }

  for (__v = __ctalk_dictionary; __v; __v = __v -> next) {
    if (__classname) {
      if (str_eq (__v -> var_decl -> name, (char *)__name) &&
	  str_eq (__v -> var_decl -> class, (char *)__classname))
	return active_tag_obj_v (__v);
    } else {
      if (str_eq (__v -> var_decl -> name, (char *)__name))
	return active_tag_obj_v (__v);
    }
  }

  /*
   *  Finally, check for a class variable.  This is occasionally needed.
   */

  if ((__o = __ctalk_find_classvar ((char *)__name, 
				    (char *)__classname)) != NULL)
    return __o;

  return NULL;
}

/*
 *  Like __ctalk_get_object but called when we don't have a
 *  class for the object.
 */
OBJECT *__ctalk_get_arg_tok (const char *__name) {

  OBJECT *__o, *result;
  VARENTRY *__v;
  METHOD *__m;
  RT_FN *rt_fn;
  int i;
  int arg_n;

  if (!IS_C_LABEL(__name))
    return NULL;

  /*
   *  Check for "self" keyword and parameter replacements.
   */

  if (str_eq ((char *)__name, "self")) {
    return __ctalk_receivers[__ctalk_receiver_ptr+1];
  } else if (str_eq ((char *)__name, "super")) {
    // This is the super that works in arg block scope.
    // The super that modifies methods is in rt_expr.c.
    if (__ctalkIsArgBlockScope ()) {
      OBJECT *super_rcvr;
      if ((super_rcvr = __call_stack[__call_stack_ptr+3] -> rcvr_obj)
	  != NULL)
	return super_rcvr;
      if (__call_stack[__call_stack_ptr+3] -> _rt_fn &&
	  ((super_rcvr = __call_stack[__call_stack_ptr+2] -> rcvr_obj)
	   != NULL))
	return super_rcvr;

      _warning ("Keyword, \"super:\" Can't find receiver object.\n");

      return NULL;
	
    } else {
      return NULL;
    }
  } else if ((__call_stack_ptr < MAXARGS) &&
	     (__call_stack[__call_stack_ptr+1] -> inline_call)) {
    result = object_from_argblk_caller (__name, NULL);
    if (IS_OBJECT(result)) {
      return result;
    } else {
      result = object_from_argblk_caller_fn (__name, NULL);
      if (IS_OBJECT(result)) {
	return result;
      }
    }
  } else if ((__call_stack_ptr < MAXARGS) &&
	     ((rt_fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    for (__v = rt_fn -> local_objects.vars; __v; __v = __v -> next) {
      if (str_eq ((char *)__name, __v -> var_decl -> name))
	return active_tag_obj_v (__v);
    }
  }

  if ((result = __ctalkGetClass (__name)) != NULL)
    return result;
  
  if (__call_stack_ptr < MAXARGS) {
    if ((__m = __call_stack[__call_stack_ptr + 1] -> method) != NULL) {
      int __nth_arg_frame_offset, __nth_arg;
      OBJECT *__arg_object;
      /*
       *  Handle args for methods called recursively by 
       *  checking each method arg against the argument 
       *  stack's current frame.  Note that the 
       *  method's arg_frame_top may not always be the
       *  same relatively (sometimes it might point to the
       *  next arg index _after_ the call), so we don't use 
       *  it as a  bounds when checking the actual arguments 
       *  on the __arg_stack.
       *
       *  If there's not arg_frame_top given, then simply
       *  check the first args[0... n_params].
       */
      if (__m -> n_params && 
	  (__m -> arg_frame_top >= __ctalk_arg_ptr) &&
	  (__m -> arg_frame_top <= MAXARGS) &&
	  (!__m -> varargs)) {
	__nth_arg_frame_offset = 
	  (__m -> n_args == __m -> n_params) ?
	  0 : ((__m -> n_args / __m -> n_params) - 1);
      next_arg_frame:
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    __nth_arg = 
	      (__m->n_params-i-1)+__nth_arg_frame_offset;
	    if (__nth_arg < 0)
	      goto look_at_variables;
	    __arg_object = 
	      (((__nth_arg >= 0) && __m->args[__nth_arg]) ?
	       __m -> args[__nth_arg]->obj : NULL);
	    if (IS_OBJECT(__arg_object)) {
	      eval_status |= EVAL_STATUS_NAMED_PARAMETER;
	      return active_tag_obj (__arg_object);
	    }
	    __nth_arg_frame_offset -= __m -> n_params;
	    goto next_arg_frame;
	  }
	}
      } else  if (__m -> varargs) {
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    if (__m -> args[__m -> n_params - i - 1]) {
	       return __m -> args[__m -> n_params - i - 1] -> obj; 
	    } else {
	      _warning 
		("__ctalk_get_object: Method %s : wrong number of arguments.\n", 
		 __m -> name);
	      return null_result_obj (__m, ARG_VAR);
	    }
	  }
	}
      } else {
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    if (__m -> args[__m -> n_args - __m -> n_params + i]) {
	      return __m -> args[__m -> n_args - __m->n_params + i] -> obj;
	    } else {
	      _warning 
		("__ctalk_get_object: Method %s : wrong number of arguments.\n", 
		 __m -> name);
	      return null_result_obj (__m, ARG_VAR);
	    }
	  }
	}
      }

    look_at_variables:
      if (M_LOCAL_VAR_LIST(__m)) {
	for (__v = M_LOCAL_VAR_LIST(__m); 
	     IS_VARENTRY(__v); __v = __v -> next) {
	  if (IS_OBJECT(__v -> var_object)) {
	    if (str_eq (__v -> var_decl -> name, (char *)__name))
	      return active_tag_obj_v (__v);
	  } else {
	    /*
	     *  May not be exactly the correct return value in all
	     *  cases, but it's safer to let the caller cope with
	     *  it.
	     */
	    if (!strncmp (__v -> var_decl -> name, (char *)__name,
			  strlen (__name)))
	      return NULL;
	  }
	}
      }
    } /* else if ((__m = __call_stack[__call_stack_ptr + 1] -> method) != NULL) ... */
  }
  
  if (__ctalk_dictionary) {
    __v = __ctalk_last_object;
    do {
      if (str_eq (__v -> var_decl -> name, (char *)__name)) {
	return active_tag_obj_v (__v);
      }
    } while ((__v = __v -> prev) != NULL);
  }

  if ((__o = __ctalk_find_classvar ((char *)__name, 
				    NULL)) != NULL) {
    return __o;
  }

  return NULL;
}

/*
 *  Like __ctalk_get_object but called only by eval_label_token,
 *  when we don't have a class for the object, and we've already
 *  checked for self.
 */
OBJECT *__ctalk_get_eval_expr_tok (const char *__name) {

  OBJECT *__o, *result;
  VARENTRY *__v;
  METHOD *__m;
  RT_FN *rt_fn;
  int i;
  int arg_n;

  if (str_eq ((char *)__name, "super")) {
    // This is the super that works in arg block scope.
    // The super that modifies methods is in rt_expr.c.
    if (__ctalkIsArgBlockScope ()) {
      OBJECT *super_rcvr;
      if ((super_rcvr = __call_stack[__call_stack_ptr+3] -> rcvr_obj)
	  != NULL)
	return super_rcvr;
      if (__call_stack[__call_stack_ptr+3] -> _rt_fn &&
	  ((super_rcvr = __call_stack[__call_stack_ptr+2] -> rcvr_obj)
	   != NULL))
	return super_rcvr;

      _warning ("Keyword, \"super:\" Can't find receiver object.\n");

      return NULL;
	
    } else {
      return NULL;
    }
  } else if ((__call_stack_ptr < MAXARGS) &&
	     (__call_stack[__call_stack_ptr+1] -> inline_call)) {
    result = object_from_argblk_caller (__name, NULL);
    if (IS_OBJECT(result)) {
      return active_tag_obj (result);
    } else {
      result = object_from_argblk_caller_fn (__name, NULL);
      if (IS_OBJECT(result)) {
	return active_tag_obj (result);
      }
    }
  } else if ((__call_stack_ptr < MAXARGS) &&
	     ((rt_fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    for (__v = rt_fn -> local_objects.vars; __v; __v = __v -> next) {
      if (str_eq ((char *)__name, __v -> var_decl -> name))
	return active_tag_obj_v (__v);
    }
  }

  if ((result = __ctalkGetClass (__name)) != NULL)
    return result;
  
  if (__call_stack_ptr < MAXARGS) {
    if ((__m = __call_stack[__call_stack_ptr + 1] -> method) != NULL) {
      int __nth_arg_frame_offset, __nth_arg;
      OBJECT *__arg_object;
      /*
       *  Handle args for methods called recursively by 
       *  checking each method arg against the argument 
       *  stack's current frame.  Note that the 
       *  method's arg_frame_top may not always be the
       *  same relatively (sometimes it might point to the
       *  next arg index _after_ the call), so we don't use 
       *  it as a  bounds when checking the actual arguments 
       *  on the __arg_stack.
       *
       *  If there's not arg_frame_top given, then simply
       *  check the first args[0... n_params].
       */
      if (__m -> n_params && 
	  (__m -> arg_frame_top >= __ctalk_arg_ptr) &&
	  (__m -> arg_frame_top <= MAXARGS) &&
	  (!__m -> varargs)) {
	__nth_arg_frame_offset = 
	  (__m -> n_args == __m -> n_params) ?
	  0 : ((__m -> n_args / __m -> n_params) - 1);
      next_arg_frame:
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    __nth_arg = 
	      (__m->n_params-i-1)+__nth_arg_frame_offset;
	    if (__nth_arg < 0)
	      goto look_at_variables;
	    __arg_object = 
	      (((__nth_arg >= 0) && __m->args[__nth_arg]) ?
	       __m -> args[__nth_arg]->obj : NULL);
	    if (IS_OBJECT(__arg_object)) {
	      eval_status |= EVAL_STATUS_NAMED_PARAMETER;
	      return active_tag_obj (__arg_object);
	    }
	    __nth_arg_frame_offset -= __m -> n_params;
	    goto next_arg_frame;
	  }
	}
      } else  if (__m -> varargs) {
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    if (__m -> args[__m -> n_params - i - 1]) {
	       return __m -> args[__m -> n_params - i - 1] -> obj; 
	    } else {
	      _warning 
		("__ctalk_get_object: Method %s : wrong number of arguments.\n", 
		 __m -> name);
	      return null_result_obj (__m, ARG_VAR);
	    }
	  }
	}
      } else {
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    if (__m -> args[__m -> n_args - __m -> n_params + i]) {
	      return __m -> args[__m -> n_args - __m->n_params + i] -> obj;
	    } else {
	      _warning 
		("__ctalk_get_object: Method %s : wrong number of arguments.\n", 
		 __m -> name);
	      return null_result_obj (__m, ARG_VAR);
	    }
	  }
	}
      }

    look_at_variables:
      if (M_LOCAL_VAR_LIST(__m)) {
	for (__v = M_LOCAL_VAR_LIST(__m); 
	     IS_VARENTRY(__v); __v = __v -> next) {
	  if (IS_OBJECT(__v -> var_object)) {
	    if (str_eq (__v -> var_decl -> name, (char *)__name))
	      return active_tag_obj_v (__v);
	  } else {
	    /*
	     *  May not be exactly the correct return value in all
	     *  cases, but it's safer to let the caller cope with
	     *  it.
	     */
	    if (!strncmp (__v -> var_decl -> name, (char *)__name,
			  strlen (__name)))
	      return NULL;
	  }
	}
      }
    } /* else if ((__m = __call_stack[__call_stack_ptr + 1] -> method) != NULL) ... */
  }
  
  if (__ctalk_dictionary) {
    __v = __ctalk_last_object;
    do {
      if (str_eq (__v -> var_decl -> name, (char *)__name)) {
	return active_tag_obj_v (__v);
      }
    } while ((__v = __v -> prev) != NULL);
  }

  if ((__o = __ctalk_find_classvar ((char *)__name, 
				    NULL)) != NULL) {
    return __o;
  }

  return NULL;
}

/*
 *  Like __ctalk_get_object (), except for use with methods like
 *  "new," which means its safe to skip checking for class objects,
 *  class variables, etc., etc.
 *
 *  Called by __ctalk_arg to find non-primitive constructor arguments.
 */
OBJECT *__ctalk_get_constructor_arg_tok (const char *__name) {

  OBJECT *__o, *__arg_object;
  VARENTRY *__v;
  METHOD *__m;
  RT_FN *rt_fn;
  int i, __nth_arg_frame_offset, __nth_arg;

  /*
   *  If we're in a method, try to return a parameter, then local
   *  object. 
   */
  if (__call_stack_ptr < MAXARGS) { /* skip during initialization, tho' */

    if ((__m = __call_stack[__call_stack_ptr +  1] -> method) != NULL) {
      /*
       *  Handle args for methods called recursively by 
       *  checking each method arg against the argument 
       *  stack's. current frame.  Note that the 
       *  method's arg_frame_top may not always be the
       *  same relatively (sometimes it might point to the
       *  next arg index _after_ the call), so we don't use 
       *  it as a  bounds when checking the actual arguments 
       *  on the __arg_stack.
       *
       *  If there's not arg_frame_top given, then simply
       *  check the first args[0... n_params].
       */
      if (__m -> n_params && 
	  (__m -> arg_frame_top >= __ctalk_arg_ptr) &&
	  (__m -> arg_frame_top <= MAXARGS) &&
	  (!__m -> varargs)) {
	__nth_arg_frame_offset = 
	  (__m -> n_args == __m -> n_params) ?
	  0 : ((__m -> n_args / __m -> n_params) - 1);
      next_arg_frame:
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    __nth_arg = 
	      (__m->n_params-i-1)+__nth_arg_frame_offset;
	    if (__nth_arg < 0)
	      goto look_at_variables;
	    __arg_object = 
	      (((__nth_arg >= 0) && __m->args[__nth_arg]) ?
	       __m -> args[__nth_arg]->obj : NULL);
	    if (IS_OBJECT(__arg_object)) {
	      eval_status |= EVAL_STATUS_NAMED_PARAMETER;
	      if (HAS_VARTAGS(__arg_object) &&
		  IS_VARENTRY(__arg_object -> __o_vartags -> tag)) {
		return active_tag_obj_v (__arg_object -> __o_vartags -> tag);
	      } else {
		return __arg_object;
	      }
	    }
	    __nth_arg_frame_offset -= __m -> n_params;
	    goto next_arg_frame;
	  }
	}
      } else {
	for (i = 0; i < __m -> n_params; i++) {
	  if ((__m -> params[i] != NULL) && 
	      str_eq (__m -> params[i] -> name, (char *)__name)) {
	    if (__m -> args[__m -> n_params - i - 1]) {
	      return __m -> args[__m -> n_params - i - 1] -> obj;
	    } else {
	      _warning 
	     ("__ctalk_get_constructor_arg_tok: Method %s : wrong number of arguments.\n", 
		 __m -> name);
	      return NULL;
	    }
	  }
	}
      }

    look_at_variables:
      if (IS_CONSTRUCTOR(__m) &&
	  str_eq (__m->params[0]->name, (char *)__name)) {
	if (M_LOCAL_VAR_LIST(__m))
	  return M_LOCAL_VAR_LIST(__m) -> var_object;
      }
      if (M_LOCAL_VAR_LIST(__m)) {
	for (__v = M_LOCAL_VAR_LIST(__m); 
	     __v && IS_OBJECT(__v->var_object); 
	     __v = __v -> next) {
	  if (str_eq (__v -> var_decl -> name, (char *)__name))
	    return active_tag_obj_v (__v);
	}
      }
    }
    
    if ((__call_stack_ptr < MAXARGS) &&
	((rt_fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
      for (__v = rt_fn -> local_objects.vars; __v; __v = __v -> next) {
	if (str_eq ((char *)__name, __v -> var_decl -> name))
	  return active_tag_obj_v (__v);
      }
    }

  } /* if (__call_stack_ptr < MAXARGS) */

  if (__ctalk_dictionary) {
    __v = __ctalk_last_object;
    do {
      if (str_eq (__v -> var_decl -> name, (char *)__name)) {
	return active_tag_obj_v (__v);
      }
    } while ((__v = __v -> prev) != NULL);
  }
  
#if 0
  /* This shouldn't be needed, but _if_ there is a case where a
     class variable is a constructor argument, then eval_expr
     can parse it after we return to __ctalk_arg, instead of
     calling this every time we have a miss. */
  
  if ((__o = __ctalk_find_classvar ((char *)__name, NULL)) != NULL)
    return __o;
#endif  

  return NULL;
}

/*
 *  This function should not need to check for, "self," and, "super,"
 *  because at this time it is only called from 
 *  __ctalkDeleteObjectInternal () and __ctalk_set_local ().
 */
int __ctalk_remove_object (OBJECT *o) {

  if (IS_CLASS_OBJECT(o)) {
    __ctalkRemoveClass (o);
  } else {
    if (__ctalk_last_object && (o == __ctalk_last_object -> var_object)) {
      if (object_become_delete) {
	__ctalk_last_object -> var_object = NULL;
      } else {
	__ctalk_last_object = __ctalk_last_object -> prev;
      }
    }
    if (__ctalk_dictionary && (o == __ctalk_dictionary -> var_object)) {
      if (object_become_delete) {
	__ctalk_dictionary -> var_object = NULL;
      } else {
	__ctalk_dictionary = __ctalk_dictionary -> next;
      }
    }
    if (o -> prev) o -> prev -> next = o -> next;
    if (o -> next) o -> next -> prev = o -> prev;
    o -> next = o -> prev = NULL;
  }

  return SUCCESS;
}

OBJECT *__ctalk_self_internal (void) {

  if (__ctalk_receiver_ptr == MAXARGS)
    return NULL;
  return __ctalk_receivers[__ctalk_receiver_ptr+1];

}

OBJECT *__ctalk_self_internal_value (void) {

  if (__ctalk_receiver_ptr == MAXARGS)
    return NULL;
  return (IS_OBJECT(__ctalk_receivers[__ctalk_receiver_ptr+1] -> instancevars) ?
	  __ctalk_receivers[__ctalk_receiver_ptr+1] -> instancevars :
	  __ctalk_receivers[__ctalk_receiver_ptr+1]);

}

void __ctalk_set_global (char *name, char *classname) {

  OBJECT *obj;
  VARENTRY *v;

  if ((obj = __ctalk_get_object (name, classname)) == NULL)
    _error ("__ctalk_set_global (): Undefined object %s (Class %s).\n", 
	    name, ((classname) ? classname : NULLSTR));

  (void)__objRefCntSet (OBJREF(obj), 1);
  __ctalkSetObjectScope (obj, GLOBAL_VAR);

}


#define __THIS_METHOD (__call_stack[__call_stack_ptr+1]->method)
void __ctalk_set_local (OBJECT *obj) {

  RT_FN *rt_fn;
  VARENTRY *v, *v_ptr;

  if (!IS_OBJECT(obj))
    return;

  __ctalkSetObjectScope (obj, LOCAL_VAR);

  (void)__objRefCntSet(OBJREF(obj), 1);

  /*
   *  If we're in a method, retrieve the method, and add the object
   *  there.
   */
  if (__THIS_METHOD) {
    __ctalk_remove_object (obj);
    obj -> next = obj -> prev = NULL;
    if (obj -> __o_vartags -> tag) {
      if (obj -> __o_vartags -> tag -> next)
	obj -> __o_vartags -> tag -> next -> prev = 
	  obj -> __o_vartags -> tag -> prev;
      if (obj -> __o_vartags -> tag -> prev)
	obj -> __o_vartags -> tag -> prev -> next = 
	  obj -> __o_vartags -> tag -> next;
      delete_varentry (obj -> __o_vartags -> tag);
      obj -> __o_vartags -> tag = NULL;
    }

    v = new_local_varentry (obj);
    if (M_LOCAL_VAR_LIST(__THIS_METHOD)) {
      v_ptr = last_varentry (M_LOCAL_VAR_LIST(__THIS_METHOD));
      v_ptr -> next = v;
      v -> prev = v_ptr;
    } else {
      M_LOCAL_VAR_LIST(__THIS_METHOD) = v;
    }
    return;
  }
  
  /*
   *  If there's no receiver object, then we're in a C function.
   */

  if ((__call_stack_ptr < MAXARGS) &&
      ((rt_fn = __call_stack[__call_stack_ptr+1]->_rt_fn) == NULL)) 
    return;

  __ctalk_remove_object (obj);

  if (obj -> __o_vartags -> tag) {
    if (obj -> __o_vartags -> tag -> next)
      obj -> __o_vartags -> tag -> next -> prev = obj -> __o_vartags -> tag -> prev;
    if (obj -> __o_vartags -> tag -> prev)
      obj -> __o_vartags -> tag -> prev -> next = obj -> __o_vartags -> tag -> next;
    delete_varentry (obj -> __o_vartags -> tag);
    obj -> __o_vartags -> tag = NULL;
  }

  obj -> next = obj -> prev = NULL;

  if (rt_fn -> local_objects.vars) {
    VARENTRY *v_1;
    v = new_varentry (obj);

    for (v_1 = rt_fn -> local_objects.vars; v_1 -> next; v_1 = v_1 -> next)
      ;
    v_1 -> next = v;
    v -> prev = v_1;

  } else {
    v = new_varentry (obj);
    rt_fn -> local_objects.vars = v;
  }
}

void __ctalk_set_local_by_name (char *objname) {

  RT_FN *rt_fn;
  VARENTRY *v, *v_ptr, *v_obj;
  OBJECT *obj = NULL;

  for (v_obj = __ctalk_last_object; v_obj; v_obj = v_obj -> prev) {
    if (!v_obj -> var_decl) {
      _error ("__ctalk_set_local: bad tag\n");
    }
    if (str_eq (v_obj -> var_decl -> name, objname)) {
      obj = v_obj -> var_object;
      break;
    }
  }

  if (!IS_OBJECT(obj))
    return;

  __ctalkSetObjectScope (obj, LOCAL_VAR);

  (void)__objRefCntSet(OBJREF(obj), 1);

  /*
   *  If we're in a method, retrieve the method, and add the object
   *  there.
   */
  if (__THIS_METHOD) {
    __ctalk_remove_object (obj);
    obj -> next = obj -> prev = NULL;
    if (obj -> __o_vartags -> tag) {
      if (obj -> __o_vartags -> tag -> next)
	obj -> __o_vartags -> tag -> next -> prev = 
	  obj -> __o_vartags -> tag -> prev;
      if (obj -> __o_vartags -> tag -> prev)
	obj -> __o_vartags -> tag -> prev -> next = 
	  obj -> __o_vartags -> tag -> next;
      delete_varentry (obj -> __o_vartags -> tag);
      obj -> __o_vartags -> tag = NULL;
    }

    v = new_local_varentry (obj);
    if (M_LOCAL_VAR_LIST(__THIS_METHOD)) {
      v_ptr = last_varentry (M_LOCAL_VAR_LIST(__THIS_METHOD));
      v_ptr -> next = v;
      v -> prev = v_ptr;
    } else {
      M_LOCAL_VAR_LIST(__THIS_METHOD) = v;
    }
    return;
  }
  
  /*
   *  If there's no receiver object, then we're in a C function.
   */

  if ((__call_stack_ptr < MAXARGS) &&
      ((rt_fn = __call_stack[__call_stack_ptr+1]->_rt_fn) == NULL)) 
    return;

  __ctalk_remove_object (obj);

  if (obj -> __o_vartags -> tag) {
    if (obj -> __o_vartags -> tag -> next)
      obj -> __o_vartags -> tag -> next -> prev =
	obj -> __o_vartags -> tag -> prev;
    if (obj -> __o_vartags -> tag -> prev)
      obj -> __o_vartags -> tag -> prev -> next =
	obj -> __o_vartags -> tag -> next;
    /* check if the tag hasn't already been deleted */
    if (obj -> __o_vartags -> tag -> var_decl)
      delete_varentry (obj -> __o_vartags -> tag);
    obj -> __o_vartags -> tag = NULL;
  }

  obj -> next = obj -> prev = NULL;

  if (rt_fn -> local_objects.vars) {
    VARENTRY *v_1;
    v = new_varentry (obj);

    for (v_1 = rt_fn -> local_objects.vars; v_1 -> next; v_1 = v_1 -> next)
      ;
    v_1 -> next = v;
    v -> prev = v_1;

  } else {
    v = new_varentry (obj);
    rt_fn -> local_objects.vars = v;
  }
}

OBJECT *match_instancevar (OBJECT *varlist, OBJECT *var) {
  OBJECT *o, *p;
  for (o = varlist; o; o = o -> next) {
    if (o == var) {
      return o;
    } else {
      if (o -> instancevars)
	if ((p = match_instancevar (o -> instancevars, var)) != NULL)
	  return p;
    }
  }

  return NULL;
}

OBJECT *match_classvar (OBJECT *varlist, OBJECT *var) {
  OBJECT *o, *p;
  for (o = varlist; o; o = o -> next) {
    if (o == var) {
      return o;
    } else {
      if (o -> classvars)
	if ((p = match_classvar (o -> classvars, var)) != NULL)
	  return p;
    }
  }

  return NULL;
}

OBJECT *match_instancevar_name (OBJECT *varlist, OBJECT *var) {
  OBJECT *o, *p;
  for (o = varlist; o; o = o -> next) {
    if (str_eq (o -> __o_name, var -> __o_name)) {
      return o;
    } else {
      if (o -> instancevars)
	if ((p = match_instancevar_name (o -> instancevars, var)) != NULL)
	  return p;
    }
  }

  return NULL;
}

/*
 *  TODO - Should be generalized.
 */
OBJECT *__ctalkInstanceVariableObject (OBJECT *var) {

  OBJECT *r;
  VARENTRY *v;

  for (v = __ctalk_dictionary; v; v = v -> next) {
    if ((r = match_instancevar (v -> var_object -> instancevars, var)) != NULL)
      return v -> var_object;
  }
  return NULL;
}

OBJECT *__ctalkClassVariableObject (OBJECT *var) {

  OBJECT *o, *r;
  VARENTRY *v;

  for (v = __ctalk_dictionary; v; v = v -> next) {
    if ((r = match_classvar (v -> var_object -> classvars, var)) != NULL)
      return v -> var_object;
  }
  for (o = __ctalk_classes; o; o = o -> next) {
    if ((r = match_classvar (o -> classvars, var)) != NULL)
      return o;
  }
  
  return NULL;
}

/*
 *  Like __ctalkInstanceVariableObject, but checks for 
 *  instance variable membership of objects on the receiver
 *  stack.
 */

OBJECT *__ctalkInstanceVariableReceiver (OBJECT *possible_var) {

  int i;
  OBJECT *rcvr, *v;

  for (i = __ctalk_receiver_ptr  + 1; i <= MAXARGS; i++) {
    rcvr = __ctalk_receivers[i];
    for (v = rcvr -> instancevars; v; v = v -> next) {
      if (v == possible_var)
	return rcvr;
    }
  }
  return possible_var;
}

static void parent_bin_int_init (OBJECT *new_value_var) {
  if ((new_value_var -> __o_class -> attrs & INT_BUF_SIZE_INIT) &&
      !(new_value_var -> __o_p_obj -> __o_class -> attrs &
	OBJECT_VALUE_IS_BIN_INT)) {
    /* give the parent object a binary int initialization, if it
       is an instance variable with the class of the parent
       object indicating the variable's class membership */
    __xfree (MEMADDR(new_value_var -> __o_p_obj -> __o_value));
    new_value_var -> __o_p_obj -> __o_value = __xalloc (INTBUFSIZE);
    memcpy ((void *)new_value_var -> __o_p_obj -> __o_value,
	    new_value_var -> __o_value, sizeof (int));
    new_value_var -> attrs |= OBJECT_VALUE_IS_BIN_INT;
    new_value_var -> __o_p_obj -> attrs |= OBJECT_VALUE_IS_BIN_INT;
  }

}

int copy_object_lvl = 0;
int max_copy_lvl_reached = 0;

/*
 *  This is the original version - makes no extra provision
 *  for a value and following instance variables to be copied
 *  individually.
 */

static OBJECT *__copy_object_internal (OBJREF_T, OBJREF_T);

static OBJECT *__copy_object_internal_from_parent (OBJREF_T src_ptr, OBJREF_T dest_ptr) {
  OBJECT *p_var, *p_var2, *new_var, *reffed_obj;

  if (copy_object_lvl >= MAXARGS) {
    fprintf (stderr, "__ctalkCopyObject: %s: Maximum object nesting level reached.\n", (*src_ptr)->__o_name);
    __warning_trace ();
    max_copy_lvl_reached = TRUE;
    --copy_object_lvl;
    return NULL;
  }

  if (max_copy_lvl_reached)
    return NULL;

  ++copy_object_lvl;
  if (*src_ptr == NULL)  {
    _warning ("__ctalk_copy_object_internal: NULL source object.\n");
    return NULL;
  }
  *dest_ptr = __create_object_internal ((*src_ptr) -> __o_name, 
					(*src_ptr) -> CLASSNAME, 
					_SUPERCLASSNAME(*src_ptr),
					(*src_ptr) -> __o_value,
					(*src_ptr) -> scope,
					(*src_ptr) -> attrs);

  for (p_var = (*src_ptr) -> instancevars; p_var; p_var = p_var -> next) {
    /*
     *  Don't use __ctalkAddInstanceVariable here because some
     *  classes (e.g. List) have duplicate instance variable names.
     *
     *  Can't use IS_VALUE_INSTANCE_VAR macro when __o_p_obj is
     *  not always set.  This function is also used by the front
     *  end.
     */
    if (__copy_object_internal_from_parent (OBJREF(p_var), OBJREF(new_var))) {
      new_var -> __o_p_obj = (*dest_ptr);
      if (IS_OBJECT((*dest_ptr)->instancevars)) {
	for (p_var2 = (*dest_ptr)->instancevars; p_var2 && p_var2 -> next;
	     p_var2 = p_var2 -> next) {
	  if (!p_var2 -> next)
	    break;
	}
	p_var2 -> next = new_var;
	new_var -> prev = p_var2;
	(void)__objRefCntSet (OBJREF(new_var), p_var -> nrefs);

	parent_bin_int_init (new_var);
      } else {

	(*dest_ptr)->instancevars = new_var;
	(void)__objRefCntSet (OBJREF(new_var), (*src_ptr) -> nrefs);

	parent_bin_int_init (new_var);
      }
    } else {
      --copy_object_lvl;
      return NULL;
    }
  }
  --copy_object_lvl;
  return *dest_ptr;
}

static OBJECT *__copy_object_internal (OBJREF_T src_ptr, OBJREF_T dest_ptr) {
  OBJECT *p_var, *p_var2, *new_var, *reffed_obj;

  if (copy_object_lvl >= MAXARGS) {
    fprintf (stderr, "__ctalkCopyObject: %s: Maximum object nesting level reached.\n", (*src_ptr)->__o_name);
    __warning_trace ();
    max_copy_lvl_reached = TRUE;
    --copy_object_lvl;
    return NULL;
  }

  if (max_copy_lvl_reached)
    return NULL;

  ++copy_object_lvl;
  if (*src_ptr == NULL)  {
    _warning ("__ctalk_copy_object_internal: NULL source object.\n");
    return NULL;
  }
  *dest_ptr = __create_object_internal ((*src_ptr) -> __o_name, 
				   (*src_ptr) -> CLASSNAME, 
				   _SUPERCLASSNAME(*src_ptr),
					(*src_ptr) -> __o_value,
					(*src_ptr) -> scope,
					(*src_ptr) -> attrs);
  (*dest_ptr) -> nrefs = (*src_ptr) -> nrefs;

  if (!IS_VALUE_INSTANCE_VAR (*src_ptr)) {
    for (p_var = (*src_ptr) -> instancevars; p_var; p_var = p_var -> next) {
      /*
       *  Don't use __ctalkAddInstanceVariable here because some
       *  classes (e.g. List) have duplicate instance variable names.
       *
       *  Can't use IS_VALUE_INSTANCE_VAR macro when __o_p_obj is
       *  not always set.  This function is also used by the front
       *  end.
       */
      if (__copy_object_internal_from_parent 
	  (OBJREF(p_var), OBJREF(new_var))) {
	new_var -> __o_p_obj = (*dest_ptr);
	if (IS_OBJECT((*dest_ptr)->instancevars)) {
	  for (p_var2 = (*dest_ptr)->instancevars; p_var2 && p_var2 -> next;
	       p_var2 = p_var2 -> next) {
	    if (!p_var2 -> next)
	      break;
	  }
	  p_var2 -> next = new_var;
	  new_var -> prev = p_var2;
	  (void)__objRefCntSet (OBJREF(new_var), p_var -> nrefs);
	} else {
	  (*dest_ptr)->instancevars = new_var;
	  (void)__objRefCntSet (OBJREF(new_var), (*src_ptr) -> nrefs);
	  if ((new_var -> attrs & OBJECT_IS_VALUE_VAR) &&
	      (new_var -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL)) {
	    free ((*dest_ptr) -> __o_value); /* not __xfree */
	    (*dest_ptr) -> __o_value = __xalloc (PTRBUFSIZE);
	    memcpy ((void *)(*dest_ptr) -> __o_value,
		    (void *)p_var -> __o_value,
		    PTRBUFSIZE);
	    (*dest_ptr) -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
	  }
	}
      } else {
	--copy_object_lvl;
	return NULL;
      }
    }
  } else { /* if (!IS_VALUE_INSTANCE_VAR (*src_ptr)) { */
    p_var2 = *dest_ptr;
    for (p_var = (*src_ptr) -> next; p_var; p_var = p_var -> next) {
      if (__copy_object_internal (OBJREF(p_var), OBJREF(new_var))) {
	new_var -> __o_p_obj = (*dest_ptr) -> __o_p_obj;
	p_var2 -> next = new_var;
	p_var2 = new_var;
      }
    }
  }
  --copy_object_lvl;
  return *dest_ptr;
}

/*
 *  Return True if reffed contains o.  Look in objects named by
 *  reference strings here also.
 */

int reffed_ref_lvl = 0;
int reffed_ref_lvl_reached = 0;

static int reffed_ref_is_circular_a (OBJECT *o, OBJECT *reffed) {
  OBJECT *t, *value_reffed_obj;
  int r;
  if (reffed_ref_lvl_reached) {
    --reffed_ref_lvl;
    return TRUE;
  }
  ++reffed_ref_lvl;
  if (reffed_ref_lvl >= MAXARGS) {
    reffed_ref_lvl_reached = TRUE;
    return TRUE;
  }
  for (t = reffed -> instancevars; t; t = t -> next) {
    if (t -> instancevars) {
      if ((r = reffed_ref_is_circular_a (o, t)) != FALSE) {
	--reffed_ref_lvl;
	return r;
      }
    }
    if ((value_reffed_obj = obj_ref_str_2 (t -> __o_value, t)) != NULL) {
      if ((r = reffed_ref_is_circular_a (o, value_reffed_obj)) != FALSE) {
	--reffed_ref_lvl;
	return r;
      }
    }
    if (t == o) {
      --reffed_ref_lvl;
      return TRUE;
    }
  }
  --reffed_ref_lvl;
  return FALSE;
}

static int reffed_ref_is_circular (OBJECT *o, OBJECT *reffed) {
  int r;
  reffed_ref_lvl = reffed_ref_lvl_reached = 0;
  r = reffed_ref_is_circular_a (o, reffed);
  reffed_ref_lvl = reffed_ref_lvl_reached = 0;
  return r;
}

OBJECT *__ctalkCopyObject (OBJREF_T src_ptr, OBJREF_T dest_ptr) {
  OBJECT *result;
  copy_object_lvl = max_copy_lvl_reached = 0;
  result = __copy_object_internal (src_ptr, dest_ptr);
  copy_object_lvl = max_copy_lvl_reached = 0;
  return result;
}

void __ctalkSetObjectScope (OBJECT *__o, int scope) {

  OBJECT *__var;
  if (IS_OBJECT(__o)) {
    for (__var = __o -> instancevars; __var; __var = __var -> next) {
      if (!IS_OBJECT(__var))
	return;
      __ctalkSetObjectScope (__var, scope);
    }
    __o -> scope = scope;
  }
}

void __ctalkSetObjectAttr (OBJECT *__o, unsigned int attr) {

  OBJECT *__var;
  if (IS_OBJECT(__o)) {
    for (__var = __o -> instancevars; __var; __var = __var -> next) 
      __ctalkSetObjectAttr (__var, attr);
    __o -> attrs = attr;
  }
}

void __ctalkObjectAttrAnd (OBJECT *__o, unsigned int attr) {

  OBJECT *__var;
  if (IS_OBJECT(__o)) {
    for (__var = __o -> instancevars; __var; __var = __var -> next) 
      __ctalkObjectAttrAnd (__var, attr);
    __o -> attrs &= attr;
  }
}

void __ctalkObjectAttrOr (OBJECT *__o, unsigned int attr) {

  OBJECT *__var;
  if (IS_OBJECT(__o)) {
    for (__var = __o -> instancevars; __var; __var = __var -> next) 
      __ctalkObjectAttrOr (__var, attr);
    __o -> attrs |= attr;
  }
}

/* extern int __local_object_init; */ /***/

int object_is_deletable (OBJECT *obj, OBJECT *result_obj, 
			 OBJECT *subexpr_obj){

  if (obj == result_obj || obj == subexpr_obj)
    return FALSE;

#if 0 /***/
  if ((obj -> scope & LOCAL_VAR) &&
      (__local_object_init == TRUE)) {
    /*    if ((obj != result_obj) &&
	(obj != subexpr_obj)) {
      } */
    if (obj -> scope & METHOD_USER_OBJECT)
      return FALSE;
    else
      return TRUE;
  }
#endif  
  if ((obj -> scope == CREATED_PARAM) &&
      !is_receiver (obj) &&
      !is_arg (obj) &&
      (obj -> instancevars != result_obj) && 
      !(obj ->  attrs & OBJECT_IS_VALUE_VAR)) {
    return TRUE;
  }
  if ((obj -> scope & CREATED_PARAM) && (obj -> nrefs == 0) && 
      (obj -> instancevars != result_obj) &&
      !(obj -> attrs & OBJECT_IS_VALUE_VAR)) {
    return TRUE;
  }
  /* Strange but true... */
  if ((obj -> scope & CVAR_VAR_ALIAS_COPY) &&
      !(obj -> scope & LOCAL_VAR) &&
      !(obj -> attrs & OBJECT_IS_VALUE_VAR)) {
    if (obj -> scope & METHOD_USER_OBJECT) {
      return FALSE;
    } else {
      if ((obj -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) ||
	  (obj -> __o_p_obj && obj -> __o_p_obj -> attrs & 
	   OBJECT_IS_MEMBER_OF_PARENT_COLLECTION)) {
	return FALSE;
      } else {
	return TRUE;
      }
    }
  }
  /* Also strange but true... */
  if ((obj -> scope & CVAR_VAR_ALIAS_COPY) &&
      (obj -> scope & SUBEXPR_CREATED_RESULT) && 
      !is_arg (obj) && 
      !is_receiver (obj)) {
    if (obj -> scope & METHOD_USER_OBJECT) {
      return FALSE;
    } else {
      return TRUE;
    }
  }
  return FALSE;
}

static bool __get_object_from_tag (OBJECT *o) {
  OBJECT *p;
  if (IS_OBJECT (o)) {
    if (o -> __o_vartags) {
      if (!IS_EMPTY_VARTAG (o -> __o_vartags)) {
#if 1
	if (IS_VARENTRY(o -> __o_vartags -> tag) &&
	    IS_PARAM (o -> __o_vartags -> tag -> var_decl)) {
#else
	if (IS_PARAM (o -> __o_vartags -> tag -> var_decl)) {
#endif
	  return (bool) __ctalk_get_object 
	    (o -> __o_vartags -> tag -> var_decl -> name,
	     o -> __o_vartags -> tag -> var_decl -> class);
	}
      }
    }
  }
  if (IS_OBJECT (o -> __o_p_obj) && 
      !(o -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION)) {
    p = o -> __o_p_obj;
    if (IS_OBJECT (p)) {
      if (p -> __o_vartags) {
	if (!IS_EMPTY_VARTAG (p -> __o_vartags)) {
	  if (IS_PARAM (p -> __o_vartags -> tag -> var_decl)) {
	    return (bool) __ctalk_get_object 
	      (p -> __o_vartags -> tag -> var_decl -> name,
	       p -> __o_vartags -> tag -> var_decl -> class);
	  }
	}
      }
    }
  }

  return false;
}

int object_is_decrementable (OBJECT *obj, OBJECT *result_obj,
			     OBJECT *subexpr_obj) {

  if (!IS_OBJECT(obj))
    return FALSE;

  if (IS_CLASS_OBJECT(obj))
    return  FALSE;

  if (is_receiver (obj) || is_arg (obj))
    return FALSE;

  if (obj == result_obj || obj == subexpr_obj)
    return FALSE;

  if (obj -> scope & CVAR_VAR_ALIAS_COPY) {
    /*
     *  This should handle most of the cases of arguments that 
     *  are also method user objects saved by 
     *  __ctalkSaveCVARResource.
     */
    if (obj -> scope & METHOD_USER_OBJECT) {
      return FALSE;
    }  else {
      if (obj -> scope & VAR_REF_OBJECT) {
	return FALSE;
      } else {
	return TRUE;
      }
    }
  }
  if (obj -> __o_p_obj && (obj -> scope & CVAR_VAR_ALIAS_COPY) &&
      (obj -> __o_p_obj != result_obj) &&
      (obj -> __o_p_obj != subexpr_obj) && 
      !is_arg (obj -> __o_p_obj) &&
      !is_receiver (obj -> __o_p_obj)) {
    if (obj -> __o_p_obj -> scope & METHOD_USER_OBJECT) {
      return FALSE;
    }  else {
      if (obj -> __o_p_obj -> scope & VAR_REF_OBJECT) {
	return FALSE;
      } else {
	return TRUE;
      }
    }
  }

  if ((obj -> __o_p_obj) && 
      (obj -> nrefs <= obj -> __o_p_obj -> nrefs))
    return FALSE;
  if (is_p_obj (obj, result_obj) ||
      is_p_obj (obj, subexpr_obj))
    return FALSE;
  if ((obj -> scope & METHOD_USER_OBJECT) &&
      (obj -> nrefs <= 1))
    return FALSE;
  if ((obj -> scope & VAR_REF_OBJECT) &&
      (obj -> nrefs <= 1))
    return FALSE;
  if ((obj -> scope & TYPECAST_OBJECT) &&
      (obj != result_obj) &&
      (obj != subexpr_obj))
    return TRUE;
  if (obj -> scope & ARG_VAR) {
    if (obj -> scope & VAR_REF_OBJECT)
      return FALSE;
  }

  if (IS_VARTAG (obj -> __o_vartags)) {
    if (!obj -> __o_vartags -> tag) {
      if (IS_OBJECT (obj) && 
	  !__ctalk_get_object(obj->__o_name, obj->CLASSNAME)) {
	if (obj -> scope & VAR_REF_OBJECT) {
	  return FALSE;
	}
	if (obj -> scope & SUBEXPR_CREATED_RESULT) {
	  return TRUE;
	}
      }
    } else {
      if (IS_OBJECT (obj) && 
	  !__get_object_from_tag (obj)) {
	return TRUE;
      }
    }
  } else { /* if (IS_VARTAG (obj -> __o_vartags)) */
    /* Same as above - place in own fn. */
    if (IS_OBJECT (obj) && 
	!__ctalk_get_object(obj->__o_name, obj->CLASSNAME)) {
      if (obj -> scope & VAR_REF_OBJECT) {
	return FALSE;
      }
    }
  }  /* if (IS_VARTAG (obj -> __o_vartags)) */
  if (IS_CLASS_OBJECT(obj)) {
    OBJECT *__o_class;
    if ((__o_class = obj -> __o_class) != NULL) {
      if (__o_class != obj) {
	if (obj -> scope & CVAR_VAR_ALIAS_COPY) {
	  return FALSE;
	} else {
	  if (obj -> scope & ARG_VAR) {
	    if ((obj == result_obj) || (obj == subexpr_obj)) {
	      return FALSE;
	    } else {
	      return TRUE;
	    }
	  } else{
	    return TRUE;
	  }
	}
      }
    }
  }
  return FALSE;
}

VARENTRY *new_local_varentry (OBJECT *var_object) {
  VARENTRY *v;
  v = new_varentry (var_object);
  v -> orig_object_rec = var_object;
  return v;
}

VARENTRY *new_varentry (OBJECT *var_object) {
  VARENTRY *v;
  TAGPARAM *p;
  v = (VARENTRY *)__xalloc (sizeof (struct _varentry));
  p = new_tagparam ();
  strcpy (p -> class, var_object -> CLASSNAME);
  strcpy (p -> name, var_object -> __o_name);
  v -> sig = VARENTRY_SIG;
  v -> var_decl = p;
  p -> parent_tag = v;
  v -> var_object = var_object;
  if (IS_EMPTY_VARTAG(var_object -> __o_vartags)) {
    var_object -> __o_vartags -> tag = v;
  } else {
    /* TODO - */
    /* Do we need to make the new VARENTRY active here? */
    add_tag (var_object, v);
  }
  v -> i = v -> i_post = v -> i_temp = I_UNDEF;
  return v;
}

void delete_varentry (VARENTRY *v) {
  if (IS_VARENTRY(v)) {
    if (v -> var_decl) {
      __xfree (MEMADDR(v -> var_decl));
    }
    __xfree (MEMADDR(v));
  }
}

void reset_varentry_i (VARENTRY *v) {
  if (v) {
    v -> i = v -> i_post = v -> i_temp = I_UNDEF;
  }
}

OBJECT *__ctalkReplaceVarEntry (VARENTRY *v, OBJECT *new_object) {
  _error ("__ctalkReplaceVarEntry () has been superceded.  Please contact "
	  "the authors for more information.\n");
}

void __delete_operand_result (OBJREF_T operand_result_p, 
			      OBJREF_T subexpr_result_p) {
  if (IS_OBJECT((*operand_result_p)) &&
      ((*operand_result_p) -> scope & SUBEXPR_CREATED_RESULT)) {
    (*operand_result_p) -> scope &= ~SUBEXPR_CREATED_RESULT;
    (void)__objRefCntDec (operand_result_p);
    if ((*operand_result_p) -> nrefs <= 0) {
      __ctalkDeleteObjectInternal ((*operand_result_p));
      if ((*operand_result_p) == (*subexpr_result_p)) {
	(*operand_result_p) = (*subexpr_result_p) = NULL;
      } else {
	(*operand_result_p) = NULL;
      }
    } else {
      __ctalkDeleteObjectInternal ((*operand_result_p));
    }
  }
}

void delete_reference_in_user_object (OBJECT *user_object) {
  OBJECT *r;
  if (IS_OBJECT(user_object -> instancevars)) {
    user_object -> instancevars -> __o_value[0] = 0;
  }
  user_object -> __o_value[0] = 0;
}

static int method_pool_max = METHOD_POOL_MAX;

void __ctalkSetMethodPoolMax (int new_size) {
  method_pool_max = new_size;
}

int __ctalkMethodPoolMax (void) {
  return method_pool_max;
}

int __ctalkRegisterUserObject (OBJECT *o) {
  METHOD *m;
  OBJECT *o_1 = NULL, *o_top = NULL;
  RT_FN *r;
  LIST *l, *l_1;

  if (!IS_OBJECT(o))
    return SUCCESS;

  if (IS_CLASS_OBJECT(o))
    return SUCCESS;

  if (o -> scope & GLOBAL_VAR)
    return SUCCESS;

  if (o -> scope & METHOD_USER_OBJECT) {
    (void)__objRefCntInc (OBJREF(o));
    return SUCCESS;
  }

  if ((o_top = top_parent_object (o)) != NULL) {
    if (IS_VARTAG(o_top -> __o_vartags) || IS_CLASS_OBJECT(o_top))
      return SUCCESS;
  }

  if ((__call_stack_ptr < MAXARGS) &&
      (m = __call_stack[__call_stack_ptr + 1] -> method) != NULL) {
    if (!is_arg (o))
      __objRefCntSet (OBJREF(o), 1);
    __ctalkSetObjectScope (o, o -> scope | METHOD_USER_OBJECT);
    if (m -> user_objects == NULL) {
      m -> user_objects = m -> user_object_ptr = new_list ();
      m -> user_objects -> data = o;
      m -> n_user_objs = 1;
      POOL_SET_METHOD_P(o,m);
      POOL_SET_LINK_P(o,m -> user_objects);
    } else {


      if (m -> n_user_objs >= method_pool_max) {
	OBJECT *o_del;
	LIST *l_del;
	l_del = list_unshift (&(m -> user_objects));
	m -> user_objects = l_del -> next;
	m -> user_objects -> prev = NULL;
	o_del = l_del -> data;

	--m -> n_user_objs;
	__xfree (MEMADDR(l_del));

	if (IS_OBJECT(o_del)) {

	  /* check here if we have a cross-linked object. */
	  if (IS_OBJECT (o_del -> instancevars)) {
	    if (o_del -> instancevars -> __o_p_obj != o_del)
	      o_del -> instancevars = NULL;
	  } else {
	    o_del -> instancevars = NULL;
	  }

	  if (o != o_del) {  /* Strange but true. */
	    if (o_del -> scope & VAR_REF_OBJECT) {
	      if (o_del -> attrs & OBJECT_REF_IS_CVAR_PTR_TGT) {
		__objRefCntZero(OBJREF(o_del));
		__ctalkDeleteObjectInternal (o_del);
	      } else if (o_del -> scope & CVAR_VAR_ALIAS_COPY) {
		__ctalkDeleteObject (o_del);
	      } else if (o_del -> nrefs > 1) {
		__objRefCntDec (OBJREF(o_del));
	      }
	    } else {
	      if (!(o_del -> attrs & OBJECT_IS_GLOBAL_COPY)) {
		__objRefCntZero(OBJREF(o_del));
		delete_reference_in_user_object (o_del);
		__ctalkDeleteObjectInternal (o_del);
	      }
	    }
	  }
	}
      }

      l_1 = new_list ();
      l_1 -> data = o;
      /***/
      POOL_SET_METHOD_P(o,m);
      POOL_SET_LINK_P(o,l_1);
      m -> user_object_ptr -> next = l_1;
      l_1 -> prev = m -> user_object_ptr;
      m -> user_object_ptr = l_1;
      ++m -> n_user_objs;
    }
  } else {
    if ((__call_stack_ptr < MAXARGS) &&
	((r = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
      (void)__objRefCntSet (OBJREF(o), 1);
      __ctalkSetObjectScope (o, o -> scope | METHOD_USER_OBJECT);
      if (r -> user_objects == NULL) {
	r-> user_objects = r -> user_object_ptr = new_list ();
	r -> user_objects -> data = o;
	r -> n_user_objs = 1;
	POOL_SET_RT_FN_P(o, r);
	POOL_SET_LINK_P(o, r -> user_objects);
      } else {

	if (r -> n_user_objs >= method_pool_max) {
	  LIST *l_del;
	  OBJECT *o_del;
	  l_del = list_unshift (&(r -> user_objects));
	  o_del = l_del -> data;
	  r -> user_objects = l_del -> next;
	  r -> user_objects = r -> user_object_ptr = NULL;
	  r -> n_user_objs = 0;
	  __xfree (MEMADDR(l_del));

	  if (IS_OBJECT(o_del)) {

	    if (IS_OBJECT (o_del -> instancevars)) {
	      if (o_del -> instancevars -> __o_p_obj != o_del)
		o_del -> instancevars = NULL;
	    } else {
	      o_del -> instancevars = NULL;
	    }

	    if  (o_del != o) { /* Again, strange but true. */
	      if (o_del -> scope & VAR_REF_OBJECT) {
		if (o_del -> attrs & OBJECT_REF_IS_CVAR_PTR_TGT) {
		  __objRefCntZero(OBJREF(o_del));
		  __ctalkDeleteObject (o_del);
		} else if (o_del -> scope & CVAR_VAR_ALIAS_COPY) {
		  __ctalkDeleteObject (o_del);
		} else if (o_del -> nrefs > 1) {
		  __objRefCntDec (OBJREF(o_del));
		}
	      } else {
		__objRefCntZero(OBJREF(o_del));
		delete_reference_in_user_object (o_del);
		__ctalkDeleteObjectInternal (o_del);
	      }
	    }
	    --r -> n_user_objs;
	  }
	}

	l = new_list ();
	l -> data = o;
	POOL_SET_RT_FN_P(o,r);
	POOL_SET_LINK_P(o,l);
	l -> prev = r -> user_object_ptr;
	r -> user_object_ptr = l;
	++r -> n_user_objs;
	return SUCCESS;
      }
    }
  }
  return SUCCESS;
}

static void cleanup_fn_pool_obj (VARENTRY *v_del) {
  OBJECT *o_del;
  if (IS_OBJECT(v_del -> var_object)) {
    o_del = v_del -> var_object;
	  
    if (IS_OBJECT (o_del -> instancevars)) {
      if (o_del -> instancevars -> __o_p_obj != o_del)
	o_del -> instancevars = NULL;
    } else {
      o_del -> instancevars = NULL;
    }
    __ctalkDeleteObject (o_del);
  }
  if ((IS_OBJECT(v_del -> orig_object_rec)) &&
      (v_del -> orig_object_rec != v_del -> var_object)) {
    o_del = v_del -> orig_object_rec;
    
    if (IS_OBJECT (o_del -> instancevars)) {
      if (o_del -> instancevars -> __o_p_obj != o_del)
	o_del -> instancevars = NULL;
    } else {
      o_del -> instancevars = NULL;
    }
    __ctalkDeleteObject (o_del);
  }
}

static LIST *fn_pool = NULL;
static LIST *fn_pool_head = NULL;
static int n_fn_pool_objs = 0;

int register_function_objects (VARENTRY *obj_list) {
  METHOD *m;
  OBJECT *o_1 = NULL, *o_top = NULL, *o_del;
  RT_FN *r;
  LIST *l, *l_1, *l_del;
  VARENTRY *v, *v_del;

  for (v = obj_list; v; v = v -> next) {

    if (!IS_OBJECT(v -> var_object))
      continue;

    if (IS_CLASS_OBJECT(v -> var_object))
      continue;

    if (v -> var_object -> scope & GLOBAL_VAR)
      continue;

    if (v -> var_object -> scope & METHOD_USER_OBJECT) {
      (void)__objRefCntInc (OBJREF(v -> var_object));
      continue;
    }

    if ((o_top = top_parent_object (v -> var_object)) != NULL) {
      if (IS_VARTAG(o_top -> __o_vartags) || IS_CLASS_OBJECT(o_top))
	continue;
    }

    __ctalkSetObjectScope (v -> var_object,
			   v -> var_object -> scope | METHOD_USER_OBJECT);
    if (fn_pool == NULL) {
      fn_pool = fn_pool_head = new_list ();
      fn_pool -> data  = v;
      n_fn_pool_objs = 1;
    } else {
      
      if (n_fn_pool_objs >= method_pool_max) {
	l_del = list_unshift (&(fn_pool));
	v_del = l_del -> data;
	fn_pool = l_del -> next;
	--n_fn_pool_objs;
	__xfree (MEMADDR(l_del));
	cleanup_fn_pool_obj (v_del);
	delete_varentry (v_del);
      }

      l = new_list ();
      l -> data = v;
      fn_pool_head -> next = l;
      l -> prev = fn_pool_head;
      fn_pool_head = fn_pool_head -> next;
      ++n_fn_pool_objs;
      return SUCCESS;
    }
  } /*   for (v = obj_list; v; v = v -> next) */
  return SUCCESS;
}

void cleanup_fn_objects (void) {
  VARENTRY *v_del;
  LIST *l_del, *l_del_prev;
  OBJECT *o_del;

  if (!fn_pool)
    return;

  if (fn_pool_head == fn_pool) {
    v_del = fn_pool_head -> data;
    __xfree (MEMADDR(fn_pool_head));
    cleanup_fn_pool_obj (v_del);
    delete_varentry (v_del);
  } else {
    l_del = fn_pool_head;
    while (l_del != fn_pool) {
      l_del_prev = l_del -> prev;
      v_del = l_del -> data;
      cleanup_fn_pool_obj (v_del);
      delete_varentry (v_del);
      __xfree (MEMADDR(l_del));
      l_del = l_del_prev;
    }
    v_del = fn_pool -> data;
    __xfree (MEMADDR(fn_pool));
    cleanup_fn_pool_obj (v_del);
    delete_varentry (v_del);
  }
}

int __ctalkRegisterExtraObjectInternal (OBJECT *o, METHOD *m) {
  OBJECT *o_1, *o_top;
  int n_resource_objs;
  LIST *l;


  if (!o)
    return SUCCESS;

  if (IS_CLASS_OBJECT(o))
    return SUCCESS;

  if (o -> scope & GLOBAL_VAR)
    return SUCCESS;

  if (o -> scope & METHOD_USER_OBJECT) {
    (void)__objRefCntInc (OBJREF(o));
    return SUCCESS;
  }

  if (((o_top = top_parent_object (o)) != NULL) &&
      ((o_top -> __o_vartags -> tag) || IS_CLASS_OBJECT(o_top)))
    return SUCCESS;

  if (m -> user_objects == NULL) {
    __ctalkSetObjectScope (o, o -> scope | METHOD_USER_OBJECT);
    m -> user_objects = m -> user_object_ptr = new_list ();
    m -> user_objects -> data = o;
    m -> n_user_objs = 1;
  } else {
    if (m -> n_user_objs >= method_pool_max) {
      OBJECT *o_del;
      LIST *l_del;
      l_del = list_unshift (&(m -> user_objects));
      o_del = l_del -> data;
      m -> user_objects = l_del -> next;
      m -> user_objects -> prev = NULL;
      if ((o_del -> scope & CVAR_VAR_ALIAS_COPY) ||
	  (o_del -> attrs & OBJECT_REF_IS_CVAR_PTR_TGT)) {
	(void)__objRefCntZero(OBJREF(o_del));
	__ctalkDeleteObject (o_del);
      } else {
	(void)__objRefCntDec(OBJREF(o_del));
	__ctalkDeleteObjectInternal (o_del);
      }
      __xfree (MEMADDR(l_del));
      --m -> n_user_objs;
    }
    __ctalkSetObjectScope (o, o -> scope | METHOD_USER_OBJECT);
    l = new_list ();
    l -> data = o;
    m -> user_object_ptr -> next = l;
    l -> prev = m -> user_object_ptr;
    m -> user_object_ptr = l;
    ++m -> n_user_objs;
    return SUCCESS;
  }
  return SUCCESS;
}

int __ctalkRegisterExtraObject (OBJECT *o) {
  METHOD *m;
  OBJECT *o_1, *o_top;
  LIST *l;

  if (!o)
    return SUCCESS;

  if (IS_CLASS_OBJECT(o))
    return SUCCESS;

  if (o -> scope & GLOBAL_VAR)
    return SUCCESS;

  if (o -> scope & METHOD_USER_OBJECT) {
    (void)__objRefCntInc (OBJREF(o));
    return SUCCESS;
  }

  o -> scope |= METHOD_USER_OBJECT;

  if (((o_top = top_parent_object (o)) != NULL) &&
      ((o_top -> __o_vartags -> tag) || IS_CLASS_OBJECT(o_top)))
    return SUCCESS;

  if ((m = __call_stack[__call_stack_ptr + 1] -> method) != NULL) {
    if (m -> user_objects == NULL) {
      m -> user_objects = m -> user_object_ptr = new_list ();
      m -> user_objects -> data = o;
      m -> n_user_objs = 1;
    } else {
      if (m -> n_user_objs >= method_pool_max) {
	OBJECT *o_del;
	LIST *l_del;
	l_del = list_unshift (&(m -> user_objects));
	o_del = l_del -> data;
	m -> user_objects = l_del -> next;
	m -> user_objects -> prev = NULL;
	(void)__objRefCntDec(OBJREF(o_del));
	__ctalkDeleteObjectInternal (o_del);
	__xfree (MEMADDR(l_del));
	--m -> n_user_objs;
      }
      l = new_list ();
      m -> user_object_ptr -> next = l;
      l -> prev = m -> user_object_ptr;
      m -> user_object_ptr -> next -> data = o;
      m -> user_object_ptr = m -> user_object_ptr -> next;
      ++m -> n_user_objs;
      return SUCCESS;
    }
  }
  return SUCCESS;
}

int is_p_obj (OBJECT *p_obj, OBJECT *c_obj) {
  OBJECT *o;
  if (!IS_OBJECT(c_obj) ||
      (c_obj -> __o_p_obj == NULL) ||
      !IS_OBJECT(c_obj -> __o_p_obj))
    return FALSE;
  for (o = c_obj -> __o_p_obj; IS_OBJECT(o); o = o -> __o_p_obj) {
    if (o == p_obj)
      return TRUE;
  }
  return FALSE;
}

int parent_ref_is_circular (OBJECT *o, OBJECT *ref) {
  OBJECT *t;
  if (ref -> attrs & OBJECT_IS_BEING_CLEANED_UP)
    return TRUE;
  for (t = o -> __o_p_obj; t; t = t -> __o_p_obj) {
    if (t == ref) {
      return FALSE;
    } else if (t -> attrs & OBJECT_IS_BEING_CLEANED_UP) {
      if (!ref -> __o_vartags)
	/* first check if the reffed object is simply a copy -
	   it won't (shouldn't) have a tag */
	return FALSE;
      if (ref -> __o_vartags && IS_EMPTY_VARTAG(ref -> __o_vartags))
	return FALSE;
      *o -> __o_value = '\0';
      __ctalkSetObjectScope (ref, ref -> scope & ~VAR_REF_OBJECT);
      return TRUE;
    }
  }
  return FALSE;
}

OBJECT *top_parent_object (OBJECT *o) {
  OBJECT *t;
  if (!o -> __o_p_obj)
    return NULL;

  t = o -> __o_p_obj;

  while (t && t -> __o_p_obj)
    t = t -> __o_p_obj;

  return t;
}

static METHOD *saved_e_methods[MAXARGS] = { 0, };

void save_local_objects_to_extra (void) {
  int i;
  METHOD *m;
  VARENTRY *v, *v_prev;
  OBJECT *o_tmp, *o_tmp_orig;
  for (i = 0; (i < MAXARGS) && saved_e_methods[i]; i++) {
    m = saved_e_methods[i];
    saved_e_methods[i] = NULL;
    if (m -> nth_local_ptr) {
      while (m -> nth_local_ptr > 0) {
	v = last_varentry (M_LOCAL_VAR_LIST(m));
	if (v == M_LOCAL_VAR_LIST(m)) {
	  if (M_LOCAL_VAR_LIST(m) &&
	      IS_OBJECT(M_LOCAL_VAR_LIST(m) -> var_object)) {
	    o_tmp = M_LOCAL_VAR_LIST(m) -> var_object;
	    unlink_varentry_2 (o_tmp, M_LOCAL_VAR_LIST(m));
	    o_tmp -> __o_vartags -> tag = NULL;
	    __ctalkRegisterExtraObjectInternal (o_tmp, m);
	    if (M_LOCAL_VAR_LIST(m) -> orig_object_rec != o_tmp) {
	      if ((o_tmp_orig = M_LOCAL_VAR_LIST(m) -> orig_object_rec)
		  != NULL) {
		unlink_varentry_2 (o_tmp_orig, M_LOCAL_VAR_LIST(m));
		M_LOCAL_VAR_LIST(m) -> orig_object_rec = NULL;
		__ctalkRegisterExtraObjectInternal (o_tmp_orig, m);
	      }
	    }
	  }
	  delete_varentry (M_LOCAL_VAR_LIST(m));
	  M_LOCAL_VAR_LIST(m) = NULL;
	} else {
	  while (v != M_LOCAL_VAR_LIST(m)) {
	    v_prev = v -> prev;
	    if (v && v -> var_object) {
	      o_tmp = v -> var_object;
	      if (o_tmp -> __o_vartags) {
		unlink_varentry_2 (v -> var_object, v);
		o_tmp -> __o_vartags -> tag = NULL;
	      }
	      __ctalkRegisterExtraObjectInternal (o_tmp, m);
	    }
	    if (v -> orig_object_rec !=v -> var_object) {
	      if ((o_tmp_orig = v -> orig_object_rec) != NULL) {
		unlink_varentry_2 (o_tmp_orig, v);
		o_tmp_orig -> __o_vartags -> tag = NULL;
		__ctalkRegisterExtraObjectInternal (o_tmp_orig, m);
	      }
	    }
	    delete_varentry (v);
	    v = v_prev;
	    if (!v) break;
	    v -> next = NULL;
	  }
	  if (M_LOCAL_VAR_LIST(m) && M_LOCAL_VAR_LIST(m) -> var_object) {
	    o_tmp = M_LOCAL_VAR_LIST(m) -> var_object;
	    unlink_varentry_2 (M_LOCAL_VAR_LIST(m) -> var_object, 
			       M_LOCAL_VAR_LIST (m));
	    o_tmp -> __o_vartags -> tag = NULL;
	    __ctalkRegisterExtraObjectInternal (o_tmp, m);
	  }
	  if (M_LOCAL_VAR_LIST(m) -> orig_object_rec != o_tmp) {
	    if ((o_tmp_orig = M_LOCAL_VAR_LIST(m) -> orig_object_rec)
		!= NULL) {
	      unlink_varentry_2 (M_LOCAL_VAR_LIST(m) -> orig_object_rec,
				 M_LOCAL_VAR_LIST(m));
	      o_tmp_orig -> __o_vartags -> tag = NULL;
	      __ctalkRegisterExtraObjectInternal (o_tmp_orig, m);
	    }
	  }
	  delete_varentry (M_LOCAL_VAR_LIST(m));
	  M_LOCAL_VAR_LIST(m) = NULL;
	}
	--m -> nth_local_ptr;
      }
    }
  }
}

/* similar to above, but it saves the varlist to the RTINFO's
   local_object_cache, where it can be checked and 
   retrieved by __ctalk_get_object_return when the program
   actually returns from the method. for an example, refer to
   Collection : integerAt.  this needs to evolve as we find more
   cases where it applies. */
void save_local_objects_to_extra_b (void) {
  int i;
  METHOD *m;
  VARENTRY *v, *v_prev;
  OBJECT *o_tmp, *o_tmp_orig;
  RT_INFO *r;
  for (i = 0; (i < MAXARGS) && saved_e_methods[i]; i++) {
    m = saved_e_methods[i];
    saved_e_methods[i] = NULL;
    if (m -> nth_local_ptr) {
      while (m -> nth_local_ptr > 0) {
	v = last_varentry (M_LOCAL_VAR_LIST(m));
	if (v == M_LOCAL_VAR_LIST(m)) {
	  r = __call_stack[__call_stack_ptr+1];
	  r -> local_object_cache[r -> local_obj_cache_ptr++] =
	    M_LOCAL_VAR_LIST(m);
#if 0
	  if (M_LOCAL_VAR_LIST(m) &&
	      IS_OBJECT(M_LOCAL_VAR_LIST(m) -> var_object)) {
	    o_tmp = M_LOCAL_VAR_LIST(m) -> var_object;
	    unlink_varentry_2 (o_tmp, M_LOCAL_VAR_LIST(m));
	    o_tmp -> __o_vartags -> tag = NULL;
	    __ctalkRegisterExtraObjectInternal (o_tmp, m);
	    if (M_LOCAL_VAR_LIST(m) -> orig_object_rec != o_tmp) {
	      if ((o_tmp_orig = M_LOCAL_VAR_LIST(m) -> orig_object_rec)
		  != NULL) {
		unlink_varentry_2 (o_tmp_orig, M_LOCAL_VAR_LIST(m));
		M_LOCAL_VAR_LIST(m) -> orig_object_rec = NULL;
		__ctalkRegisterExtraObjectInternal (o_tmp_orig, m);
	      }
	    }
	  }
	  delete_varentry (M_LOCAL_VAR_LIST(m));
#endif	  
	  M_LOCAL_VAR_LIST(m) = NULL;
	} else {
	  while (v != M_LOCAL_VAR_LIST(m)) {
	    v_prev = v -> prev;
	    if (v && v -> var_object) {
	      o_tmp = v -> var_object;
	      if (o_tmp -> __o_vartags) {
		unlink_varentry_2 (v -> var_object, v);
		o_tmp -> __o_vartags -> tag = NULL;
	      }
	      __ctalkRegisterExtraObjectInternal (o_tmp, m);
	    }
	    if (v -> orig_object_rec !=v -> var_object) {
	      if ((o_tmp_orig = v -> orig_object_rec) != NULL) {
		unlink_varentry_2 (o_tmp_orig, v);
		o_tmp_orig -> __o_vartags -> tag = NULL;
		__ctalkRegisterExtraObjectInternal (o_tmp_orig, m);
	      }
	    }
	    delete_varentry (v);
	    v = v_prev;
	    if (!v) break;
	    v -> next = NULL;
	  }
	  if (M_LOCAL_VAR_LIST(m) && M_LOCAL_VAR_LIST(m) -> var_object) {
	    o_tmp = M_LOCAL_VAR_LIST(m) -> var_object;
	    unlink_varentry_2 (M_LOCAL_VAR_LIST(m) -> var_object, 
			       M_LOCAL_VAR_LIST (m));
	    o_tmp -> __o_vartags -> tag = NULL;
	    __ctalkRegisterExtraObjectInternal (o_tmp, m);
	  }
	  if (M_LOCAL_VAR_LIST(m) -> orig_object_rec != o_tmp) {
	    if ((o_tmp_orig = M_LOCAL_VAR_LIST(m) -> orig_object_rec)
		!= NULL) {
	      unlink_varentry_2 (M_LOCAL_VAR_LIST(m) -> orig_object_rec,
				 M_LOCAL_VAR_LIST(m));
	      o_tmp_orig -> __o_vartags -> tag = NULL;
	      __ctalkRegisterExtraObjectInternal (o_tmp_orig, m);
	    }
	  }
	  delete_varentry (M_LOCAL_VAR_LIST(m));
	  M_LOCAL_VAR_LIST(m) = NULL;
	}
	--m -> nth_local_ptr;
      }
    }
  }
}

void delete_extra_local_objects (EXPR_PARSER *p) {
  int i;
  METHOD *m;
  VARENTRY *v, *v_prev;
  for (i = 0; (i < MAXARGS) && p -> e_methods[i]; i++) {
    m = p -> e_methods[i];
    if (m -> nth_local_ptr) {
      while (m -> nth_local_ptr > 0) {
	v = last_varentry(M_LOCAL_VAR_LIST(m));
	if (v == M_LOCAL_VAR_LIST(m)) {
	  if (M_LOCAL_VAR_LIST(m) && M_LOCAL_VAR_LIST(m) -> var_object) {
	    if (M_LOCAL_VAR_LIST(m) -> var_object == 
		p -> e_result) {
	      M_LOCAL_VAR_LIST(m) -> var_object -> __o_vartags -> tag = NULL;
	      __ctalkRegisterExtraObjectInternal 
		(M_LOCAL_VAR_LIST(m) -> var_object, m);
	    } else {
	      if (M_LOCAL_VAR_LIST(m) -> var_object == 
		  p -> e_result) {
		M_LOCAL_VAR_LIST(m) -> var_object -> __o_vartags -> tag = NULL;
		__ctalkRegisterExtraObjectInternal 
		  (M_LOCAL_VAR_LIST(m) -> var_object, m);
	      } else {
		__ctalkDeleteObject (M_LOCAL_VAR_LIST(m) -> var_object);
	      }
	    }
	  }
	  delete_varentry (M_LOCAL_VAR_LIST(m));
	  M_LOCAL_VAR_LIST(m) = NULL;
	} else {
	  while (v != M_LOCAL_VAR_LIST(m)) {
	    v_prev = v -> prev;
	    if (v && v -> var_object) {
	      if (v -> var_object == current_expression_result ()) {
		v -> var_object -> __o_vartags -> tag = NULL;
		__ctalkRegisterExtraObjectInternal (v -> var_object,
						    m);
	      } else {
		/* don't delete instance vars declared elsewhere 
		 and assigned or reffed here, or an object that
		 has a vartag from a declaration somewhere else */
		if (!IS_OBJECT(v -> var_object -> __o_p_obj)) {
		  goto del_ex_cleanup_skip_delete;
		} else if (!(IS_VARENTRY(v -> var_object ->__o_vartags) &&
			     IS_VARTAG(v -> var_object -> __o_vartags -> tag))){
		  goto del_ex_cleanup_skip_delete;
		}
	      }
	    }
	    __ctalkDeleteObject ( v -> var_object);
	  del_ex_cleanup_skip_delete:
	    delete_varentry (v);
	    v = v_prev;
	    if (!v) break;
	    v -> next = NULL;
	  }
	  if (M_LOCAL_VAR_LIST(m) && M_LOCAL_VAR_LIST(m) -> var_object) {
	    if (M_LOCAL_VAR_LIST(m) -> var_object == 
		current_expression_result ()) {
	      M_LOCAL_VAR_LIST(m) -> var_object -> __o_vartags -> tag = NULL;
	      __ctalkRegisterExtraObjectInternal 
		(M_LOCAL_VAR_LIST(m) -> var_object, m);
	    } else {
	      if (M_LOCAL_VAR_LIST(m) -> var_object -> scope &
		  LOCAL_VAR) {
		M_LOCAL_VAR_LIST(m) -> var_object =
		  M_LOCAL_VAR_LIST(m) -> orig_object_rec;
		M_LOCAL_VAR_LIST(m) -> orig_object_rec = NULL;
	      } else {
		__ctalkDeleteObject (M_LOCAL_VAR_LIST(m) -> var_object);
	      }
	    }
	    delete_varentry (M_LOCAL_VAR_LIST(m));
	    M_LOCAL_VAR_LIST(m) = NULL;
	  }
	}
	--m -> nth_local_ptr;
      }
    }
  }
}

void save_e_methods (EXPR_PARSER *p) {
  int i;
  for (i = 0; (i < MAXARGS) && p -> e_methods[i]; i++) {
    saved_e_methods[i] = p -> e_methods[i];
  }
}

/*
 *  We can actually use the argument to double check the method
 *  on the call stack if necessary.
 */
int save_e_methods_2 (METHOD *m) {
  int i;
  if (__call_stack[__call_stack_ptr + 1] -> _successive_call) {
    for (i = 0; i < MAXARGS; i++) {
      if (saved_e_methods[i] == NULL) {
	saved_e_methods[i] = __call_stack[__call_stack_ptr+1] -> method;
	return TRUE;
      }
    }
  }
  return FALSE;
}

/* Works for all constant classes so far except String, which needs the
   quotes escaped first. */
int const_created_param (MESSAGE_STACK messages, int idx,
				       METHOD *method,
				       int scope,
				       MESSAGE *m, OBJECT *class_obj) {

  int param_scope = scope & ~VAR_REF_OBJECT;

  if (method == NULL) {
    if (M_TOK(messages[idx]) == INTEGER ||
	M_TOK(messages[idx]) == CHAR ||
	M_TOK(messages[idx]) == FLOAT ||
	M_TOK(messages[idx]) == DOUBLE ||
	M_TOK(messages[idx]) == LONG ||
	M_TOK(messages[idx]) == LONGLONG) {
      m -> obj =
	__ctalkCreateObjectInit (M_NAME(messages[idx]),
				 class_obj -> __o_name,
				 _SUPERCLASSNAME(class_obj),
				 CREATED_PARAM,
				 M_NAME(messages[idx]));
      __ctalkSetObjectScope (m -> obj, m -> obj -> scope | param_scope);
      m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
      return SUCCESS;
    }
  }

  if ((m -> obj = create_param (m -> name, method, class_obj,
				messages, idx)) == NULL) {

    m -> obj = exception_null_obj (messages, idx);
    m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;

    return ERROR;

  } else {

    __ctalkSetObjectScope (m -> obj, m -> obj -> scope | param_scope);

    errno = 0;
    if (str_eq (class_obj -> __o_name, INTEGER_CLASSNAME) ||
	str_eq (class_obj -> __o_name, LONGINTEGER_CLASSNAME)) {
      switch (radix_of (m -> name))
	{
	case hexadecimal:
	  (void)ctitoa (strtol (m -> name, NULL, 16), m -> obj -> __o_value);
	  __ctalkSetObjectValueVar  (m -> obj, m -> obj -> __o_value);
	  break;
	case octal:
	  (void)ctitoa (strtol (m -> name, NULL, 8), m -> obj -> __o_value);
	  __ctalkSetObjectValueVar  (m -> obj, m -> obj -> __o_value);
	  break;
	case binary:
	  *(int *)m -> obj -> __o_value = ascii_bin_to_dec (m -> name);
	  *(int *)m -> obj -> instancevars -> __o_value =
	    *(int *)m -> obj -> __o_value;
	  break;
	case decimal:
	default:
	  break;
	}

      switch (errno)
	{
	case EINVAL:
	  _warning ("const_created_param: Invalid number %s (Object %s).\n",
		    m -> obj -> __o_value, m -> obj -> __o_name);
	  break;
	case ERANGE:
	  _warning ("const_created_param: "
		    "Value %s is out of range. (Object %s).\n",
		    m -> obj -> __o_value, m -> obj -> __o_name);
	  break;
	default:
	  break;
	}
    }

    m -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
    
  }

  return SUCCESS;
}

/*
 *  If the argument token matches a parameter, use the corresponding
 *  argument.  Otherwise, if the argument token has a class that
 *  matches a lexical type, use that.  Otherwise, use the class of the
 *  method parameter.
 */

OBJECT *create_param (char *name, METHOD *method, OBJECT *arg_class,
		      MESSAGE_STACK messages, int err_idx) {

  PARAM *p;
  OBJECT *param_object, *param_class,
    *rcvr, *rcvr_class, *(*methd_fn)();
  int i;

  param_object = NULL;

  if (method == NULL) {  /* Expression from __ctalkEvalExpr (). */
    if ((methd_fn = rtinfo.method_fn) != NULL) {

      if (__ctalk_receiver_ptr >= 512) {
	__ctalkExceptionInternal (messages[err_idx], invalid_receiver_x,
				  name, 0);
	return NULL;
      }

      if ((rcvr = __ctalk_receivers[__ctalk_receiver_ptr + 1]) == 
	  NULL)
	return NULL;

      rcvr_class = rcvr->__o_class;
      if (rcvr_class == rcvr) {
	if ((method = 
	     __ctalkFindClassMethodByFn (&rcvr, methd_fn, FALSE)) 
	    != NULL) {
	  for (i = 0; i < method -> n_params; i++) {
	    if (!strcmp (method -> params[i] -> name, name)) {
	      return method -> args[i] -> obj;
	    }
	  }
	}
      } else {
	if ((method = 
	     __ctalkFindInstanceMethodByFn (&rcvr, methd_fn, FALSE)) 
	    != NULL) {
	  for (i = 0; i < method -> n_params; i++) {
	    if (!strcmp (method -> params[i] -> name, name)) {
	      if (method -> args[i]) {
		return method -> args[i] -> obj;
	      } else {
		goto param_from_arg_class;
	      }
	    }
	  }
	}
      }
    }
  }

 param_from_arg_class:
  if (arg_class) {
    if (arg_class -> attrs & INT_BUF_SIZE_INIT) {
      if (isdigit (*name) || *name  == '-' || *name == '+') {
	param_object =
	  __ctalkCreateObjectInit (name, arg_class -> __o_name,
				   _SUPERCLASSNAME(arg_class),
				   CREATED_PARAM,
				   name);
      } else {
	arg_class -> attrs &= ~INT_BUF_SIZE_INIT;
	arg_class -> instancevars -> attrs &= ~INT_BUF_SIZE_INIT;
	/* For temporary label parameters, we still need the normal
	   initialization. */
	param_object =
	  __ctalkCreateObjectInit (name, arg_class -> __o_name,
				   _SUPERCLASSNAME(arg_class),
				   CREATED_PARAM,
				   name);
	arg_class -> attrs |= INT_BUF_SIZE_INIT;
	arg_class -> instancevars -> attrs |= INT_BUF_SIZE_INIT;
	__ctalkSetObjectAttr (param_object,
			      param_object -> attrs &
			      OBJECT_VALUE_IS_BIN_INT);
      }
    } else if (arg_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
      if (isdigit (*name) || *name  == '-' || *name == '+') {
	param_object =
	  __ctalkCreateObjectInit (name, arg_class -> __o_name,
				   _SUPERCLASSNAME(arg_class),
				   CREATED_PARAM,
				   name);
      } else {
	arg_class -> attrs &= ~SYMBOL_BUF_SIZE_INIT;
	arg_class -> instancevars -> attrs &= ~SYMBOL_BUF_SIZE_INIT;
	/* For temporary label parameters, we still need the normal
	   initialization. */
	param_object =
	  __ctalkCreateObjectInit (name, arg_class -> __o_name,
				   _SUPERCLASSNAME(arg_class),
				   CREATED_PARAM,
				   name);
	arg_class -> attrs |= SYMBOL_BUF_SIZE_INIT;
	arg_class -> instancevars -> attrs |= SYMBOL_BUF_SIZE_INIT;
	__ctalkSetObjectAttr (param_object,
			      param_object -> attrs &
			      OBJECT_VALUE_IS_BIN_SYMBOL);
      }
    } else {
      param_object =
	__ctalkCreateObjectInit (name, arg_class -> __o_name,
				 _SUPERCLASSNAME(arg_class),
				 CREATED_PARAM,
				 name);
    }
  } else {
    if ((p = has_param_def (method)) != NULL) {
      if ((param_class = __ctalkGetClass (p -> class)) != NULL) {
#ifdef DEBUG_DYNAMIC_C_ARGS
	_warning ("create_param (): method %s (param class %s) not found.\n",
		  m -> name, p -> class);
#endif
	if (param_class -> attrs & INT_BUF_SIZE_INIT) {
	  if (isdigit (*name) || *name  == '-' || *name == '+') {
	    param_object =
	      __ctalkCreateObjectInit (name, param_class -> __o_name,
				       _SUPERCLASSNAME(param_class),
				       CREATED_PARAM,
				       name);
	  } else {
	    /* here, too, for temporary parameters we need the
	       normal object init */
	    param_class -> attrs &= ~INT_BUF_SIZE_INIT;
	    param_class -> instancevars -> attrs &= ~INT_BUF_SIZE_INIT;
	    param_object =
	      __ctalkCreateObjectInit (name, param_class -> __o_name,
				       _SUPERCLASSNAME(param_class),
				       CREATED_PARAM,
				       name);
	    param_class -> attrs |= INT_BUF_SIZE_INIT;
	    param_class -> instancevars -> attrs |= INT_BUF_SIZE_INIT;
	  }
	} else {
	  param_object =
	    __ctalkCreateObjectInit (name, param_class -> __o_name,
				     _SUPERCLASSNAME(param_class),
				     CREATED_PARAM,
				     name);
	}
      }
    }
  }
  return param_object;
}

OBJECT *create_instancevar_param (char *name, OBJECT *recv_class) {

  OBJECT *instvar, *instvar_copy;

  if (!IS_OBJECT(recv_class))
    return NULL;

  if ((instvar = __ctalkGetInstanceVariable (recv_class, 
					     name, FALSE)) != NULL) {
    __ctalkCopyObject (OBJREF(instvar), OBJREF(instvar_copy));
    return instvar_copy;
  }

  return create_instancevar_param (name, recv_class -> __o_superclass);

}

/* should be used only by Integer : = */
OBJECT *_store_int (OBJECT *rcvr, OBJECT *arg) {
  char buf[MAXLABEL];
  if (arg -> attrs & OBJECT_VALUE_IS_BIN_INT) {
    /* Putting this first should be faster */
    INTVAL(rcvr -> __o_value) = INTVAL(arg -> __o_value);
    if (IS_OBJECT(rcvr -> instancevars)) {
      INTVAL(rcvr -> instancevars -> __o_value) =
	(IS_OBJECT(arg -> instancevars) ?
	 (*(int *)(arg -> instancevars -> __o_value)) :
	 (*(int *)(arg -> __o_value)));
    }
  } else if (arg -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG) {
    if (LLVAL(arg -> __o_value) > INT32_MAX) {
      _warning ("ctalk: LongInteger value %s truncated to Integer.\n",
		arg -> __o_name);
    }
    INTVAL(rcvr -> __o_value) = (int)LLVAL(arg -> __o_value);
    if (IS_OBJECT(rcvr -> instancevars)) {
      INTVAL(rcvr -> instancevars -> __o_value) =
	(int)(IS_OBJECT(arg -> instancevars) ?
	      (LLVAL(arg -> instancevars -> __o_value)) :
	      (LLVAL(arg -> __o_value)));
    }
  } else if (rcvr -> attrs & OBJECT_HAS_PTR_CX) {
    __ctalkAliasObject (rcvr, arg);
  } else {
    __ctalkDecimalIntegerToASCII (__ctalkToCInteger(arg, 1), buf);
    *(int *)(rcvr -> __o_value) = atoi (buf);
    if (IS_OBJECT(rcvr -> instancevars)) {
      *(int *)(rcvr -> instancevars -> __o_value) = atoi (buf);
    }
  }
  return rcvr;
}
