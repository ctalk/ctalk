/* $Id: rt_return.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017, 2019  
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
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "parser.h"

extern DEFAULTCLASSCACHE *rt_defclasses;

extern RT_INFO rtinfo;
extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

OBJECT *__ctalkSaveOBJECTMemberResource (OBJECT *o) {
  OBJECT *__o_copy;
  if (o && IS_OBJECT (o)) {
    if (o -> nrefs == 0) {
      __ctalkCopyObject(OBJREF(o), OBJREF(__o_copy));
      __ctalkRegisterUserObject(__o_copy);
      return __o_copy;
    } else {
      return o;
    }
  } else {
    return NULL;
  }
}

OBJECT *__ctalkRegisterIntReturn (int i) {
  OBJECT *__o; 
  __o = create_object_init_internal
    ("result", rt_defclasses -> p_integer_class, LOCAL_VAR, "");
  SETINTVARS(__o, i);
  __ctalkRegisterUserObject(__o);
  return __o;
}

OBJECT *__ctalkRegisterLongLongIntReturn (long long int i) {
  OBJECT *__o; 
  __o = create_object_init_internal
    ("result", rt_defclasses -> p_longinteger_class, LOCAL_VAR, "");
  LLVAL(__o -> __o_value) =
    LLVAL(__o -> instancevars -> __o_value) = i;
  __ctalkRegisterUserObject(__o);
  return __o;
}

static OBJECT *boolTrue_internal = NULL,
  *boolFalse_internal = NULL;

static OBJECT *create_bool_return (int i) {
  OBJECT *__o; 
  __o = create_object_init_internal
    ("result", rt_defclasses -> p_boolean_class, LOCAL_VAR, "");
  SETINTVARS(__o, i);
  __ctalkRegisterUserObject(__o);
  return __o;
}

OBJECT *__ctalkRegisterBoolReturn (int i) {

  if (i) {
    if (boolTrue_internal == NULL) {
      boolTrue_internal = __ctalk_find_classvar ("boolTrue", NULL);
      if (boolTrue_internal == NULL) {
	return create_bool_return (i);
      } else {
	return boolTrue_internal;
      }
    } else {
      return boolTrue_internal;
    }
  } else {
    if (boolFalse_internal == NULL) {
      boolFalse_internal = __ctalk_find_classvar ("boolFalse", NULL);
      if (boolFalse_internal == NULL) {
	return create_bool_return (i);
      } else {
	return boolFalse_internal;
      }
    } else {
      return boolFalse_internal;
    }
  }

}

OBJECT *__ctalkRegisterCharPtrReturn (char *s) {
  OBJECT *__o; 
  if (s != NULL) {
    __o = create_object_init_internal
      ("result", rt_defclasses -> p_string_class, LOCAL_VAR, s);
    __ctalkRegisterUserObject(__o);
    if (obj_ref_str (s)) {
      /* __ctalkToCCharPtr uses checks this in order to return
	 the char *, not an OBJECT *. */
      __ctalkSetObjectAttr (__o, OBJECT_IS_STRING_LITERAL);
    }
    return __o;
  } else {
    return NULL;
  }
}

OBJECT *__ctalkRegisterCharReturn (char c) {
  OBJECT *__o; 
  char __b[2];
  __b[0] = c;
  __b[1] = '\0';
  __o = create_object_init_internal
    ("result", rt_defclasses -> p_character_class, LOCAL_VAR, __b);
  __ctalkRegisterUserObject(__o);
  return __o;
}

OBJECT *__ctalkRegisterFloatReturn (double d) {
  OBJECT *__o; 
  char __b[MAXLABEL];
#if defined(__sparc__) && defined(__svr4__)
  sprintf (__b, "%f", d);
#else
  sprintf (__b, "%lf", (double)d);
#endif
  __o = create_object_init_internal
    ("result", rt_defclasses -> p_float_class, LOCAL_VAR, __b);
  __ctalkRegisterUserObject(__o);
  return __o;
}

static OBJECT *CVAR_receiver_return (char *name) {
  char buf[MAXLABEL];
  OBJECT *obj;
  CVAR *c;
  if ((c = get_method_arg_cvars (name)) != NULL) {
    if ((c -> type_attrs & CVAR_TYPE_OBJECT) && (c -> n_derefs == 1)) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      sprintf (buf, "%#lx", (unsigned long int)c -> val.__value.__ptr);
#else
      htoa (buf, (unsigned int)c -> val.__value.__ptr);
#endif
      if ((obj = obj_ref_str (buf)) != NULL) {
	if (is_receiver (obj))
	  return obj;
	if (is_receiver (top_parent_object (obj)))
	  return obj;
	return NULL;
      }
    } else if (str_eq (c -> type, "OBJECT") && (c -> n_derefs == 1)) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      sprintf (buf, "%#lx", (unsigned long int)c -> val.__value.__ptr);
#else
      htoa (buf, (unsigned int)c -> val.__value.__ptr);
#endif
      if ((obj = obj_ref_str (buf)) != NULL) {
	if (is_receiver (obj))
	  return obj;
	if (is_receiver (top_parent_object (obj)))
	  return obj;
	return NULL;
      }
    }
  }
  return NULL;
}

static OBJECT *CVAR_ref_return (char *name) {
  char buf[MAXLABEL];
  OBJECT *obj;
  CVAR *c;
  if ((c = get_method_arg_cvars (name)) != NULL) {
    if ((c -> type_attrs & CVAR_TYPE_OBJECT) && (c -> n_derefs == 1)) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      sprintf (buf, "%#lx", (unsigned long int)c -> val.__value.__ptr);
#else
      htoa (buf, (unsigned int)c -> val.__value.__ptr);
#endif
      if ((obj = obj_ref_str (buf)) != NULL) {
	if (obj -> scope & VAR_REF_OBJECT) 
	  return obj;
      }
    } else if (str_eq (c -> type, "OBJECT") && (c -> n_derefs == 1)) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      sprintf (buf, "%#lx", (unsigned long int)c -> val.__value.__ptr);
#else
      htoa (buf, (unsigned int)c -> val.__value.__ptr);
#endif
      if ((obj = obj_ref_str (buf)) != NULL) {
	if (obj -> scope & VAR_REF_OBJECT) 
	  return obj;
      }
    }
  }
  return NULL;
}

#define THIS_METHOD (__call_stack[__call_stack_ptr+1]->method)
#define THIS_RT_FN (__call_stack[__call_stack_ptr+1]->_rt_fn)
static CVAR *get_method_arg_cvars_remove (const char *s) {

  CVAR *c, *__c_prev, *__c_next;
  METHOD *method;
  RT_FN *fn;

  if (__call_stack_ptr >= MAXARGS)
    return NULL;

  if (__call_stack[__call_stack_ptr+1]) {
    if (__call_stack[__call_stack_ptr+1] -> method) {
      if (THIS_METHOD -> local_cvars) {
	c = THIS_METHOD -> local_cvars;
	while (1) {
	  __c_next = c -> next;
	  if (str_eq (c -> name, (char *)s)) {
	    if (c == THIS_METHOD -> local_cvars) {
	      THIS_METHOD -> local_cvars = THIS_METHOD -> local_cvars -> next;
	    } else {
	      if (c -> next) c -> next -> prev = c -> prev;
	      if (c -> prev) c -> prev -> next = c -> next;
	    }
	    return c;
	  }
	  if (!__c_next)
	    break;
	  c = __c_next;
	}
      }
    } else if ((__call_stack_ptr < MAXARGS)
	       && __call_stack[__call_stack_ptr + 1] -> _rt_fn) {
      if (THIS_RT_FN -> local_cvars) {
	c = THIS_RT_FN -> local_cvars;
	while (1) {
	  __c_next = c -> next;
	  if (str_eq (c -> name, (char *)s)) {
	    if (c == THIS_RT_FN -> local_cvars) {
	      THIS_RT_FN -> local_cvars =
		THIS_RT_FN -> local_cvars -> next;
	    } else {
	      if (c -> next) c -> next -> prev = c -> prev;
	      if (c -> prev) c -> prev -> next = c -> next;
	    }
	    return c;
	  }
	  if (!__c_next)
	    break;
	  c = __c_next;
	}
      }
    }
  }

  return NULL;
}

/*
 *  NOTE - This function can ONLY be called after a call 
 *  to __ctalk_register_c_method_arg (), because it deletes
 *  the last local CVAR registered by the method, when it 
 *  calls __delete_last_method_return_resource_cvar, above.
 */
OBJECT *__ctalkSaveCVARResource (char *name) {
  OBJECT *obj, *o_top;
  CVAR *c;
  int cvar_obj_is_created;

  if ((obj = CVAR_receiver_return (name)) != NULL) {
    c = get_method_arg_cvars_remove (name);
    if (IS_CVAR(c))
      _delete_cvar (c);
    return obj;
  }

  if ((obj = CVAR_ref_return (name)) != NULL) {
    c = get_method_arg_cvars_remove (name);
    if (IS_CVAR (c))
      _delete_cvar (c);
    if ((obj -> nrefs == 0) && obj -> __o_vartags &&
	IS_EMPTY_VARTAG(obj -> __o_vartags)) {
      __ctalkSetObjectScope (obj, obj -> scope & ~METHOD_USER_OBJECT);
      __ctalkRegisterUserObject (obj);
    }
    return obj;
  }

  if ((c = get_method_arg_cvars_remove (name)) != NULL) {
    if ((c -> type_attrs & CVAR_TYPE_OBJECT) && (c -> n_derefs == 1)) {
    obj = (OBJECT *)c -> val.__value.__ptr;
      if (IS_CLASS_OBJECT(obj)) {
	_delete_cvar (c);
	return obj;
      }
      /* Just in case. */
      if (IS_OBJECT (obj)) {
	if (obj -> scope & GLOBAL_VAR) {
	  /* Note - this could also apply to other scopes that
	     are inherited from the original object ... untested
	     so far. */
	  /* check that it really is an original object. */
	  if ((o_top = top_parent_object (obj)) == NULL)  {
	    if (!__ctalk_get_object (obj -> __o_name, obj ->CLASSNAME)) {
	      __ctalkSetObjectScope (obj, CVAR_VAR_ALIAS_COPY);
	    /* sic */
	    } else if (!__ctalk_get_object (obj -> __o_name, 
					    obj -> CLASSNAME)) {
	      __ctalkSetObjectScope (obj, CVAR_VAR_ALIAS_COPY);
	    } else {
	      _delete_cvar (c);
	      return obj;
	    }
	  }
	}
	if (obj -> nrefs == 0) {
	  __ctalkSetObjectScope (obj, obj -> scope & ~METHOD_USER_OBJECT);
	}
	if (!instancevar_is_receiver (obj))
	  __ctalkRegisterUserObject (obj);
	/*
	 *  This call requires that the __ctalkSaveCVARResource
	 *  get called ONLY after a call to 
	 *  __ctalk_register_c_method_arg ().
	 */
	_delete_cvar (c);
	return obj;
      }
    }

    if ((obj = cvar_object (c, &cvar_obj_is_created)) != NULL) {
      if (obj -> nrefs) {
	if (c -> type_attrs & CVAR_TYPE_OBJECT) {
	  __ctalkRegisterUserObject (obj);
	  _delete_cvar (c);
	  return obj;
	} else if (str_eq (c -> type, "OBJECT") &&
		   (c -> n_derefs == 1)) {
	  __ctalkRegisterUserObject (obj);
	  _delete_cvar (c);
	  return obj;
	} else {
	  _warning ("__ctalkSaveCVARResource: Unsupported type %s.\n",
		    c -> type);
	  _delete_cvar (c);
	  return NULL;
	}
      } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
	__ctalkRegisterUserObject (obj);
	_delete_cvar (c);
	return obj;
      } else if ((c -> type_attrs & CVAR_TYPE_INT) ||
		 (c -> type_attrs & CVAR_TYPE_CHAR) ||
		 (c -> type_attrs & CVAR_TYPE_LONGLONG) ||
		 (c -> type_attrs & CVAR_TYPE_LONG) ||
		 (c -> type_attrs & CVAR_TYPE_LONGDOUBLE) ||
		 (c -> type_attrs & CVAR_TYPE_DOUBLE) ||
		 (c -> type_attrs & CVAR_TYPE_FLOAT)) {
	__ctalkRegisterUserObject (obj);
	_delete_cvar (c);
	__ctalkSetObjectScope (obj, obj -> scope | CVAR_VAR_ALIAS_COPY);
	return obj;
      } else if (str_eq (c -> type, "OBJECT") &&
		 (c -> n_derefs == 1)) {
	/* this is here for backward compatibility also. */
	__ctalkRegisterUserObject (obj);
	_delete_cvar (c);
	return obj;
      } else {
	_warning ("__ctalkSaveCVARResource: Unsupported type %s.\n",
		  c -> type);
	_delete_cvar (c);
	return NULL;
      }
    }
  }
  if (IS_CVAR(c))
    _delete_cvar (c);
  return NULL;
}

/*
 * See the comment above for __ctalkSaveCVARResource.
 */
OBJECT *__ctalkSaveCVARArrayResource_internal (char *name, int initializer_size,
				      void *var) {
  OBJECT *array_obj, *mbr_obj, *t, *member_class, *return_class,
    *elem_obj;
  CVAR *c, *c_t;
  int i;
  int *i_array = NULL;
  long long int *ll_array = NULL;
  char *c_array = NULL;
  double *d_array = NULL;
  METHOD *m;
  char mbr_name[MAXLABEL], valbuf[MAXMSG];;

  array_obj = NULL;
  if ((c = get_method_arg_cvars_remove (name)) != NULL) {
    if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
      ll_array = var;
      member_class = get_class_by_name (LONGINTEGER_CLASSNAME);
    } else if ((c -> type_attrs & CVAR_TYPE_INT) ||
	       (c -> type_attrs & CVAR_TYPE_LONG)) {
      i_array = var;
       member_class = get_class_by_name (INTEGER_CLASSNAME);
    } else if (c -> type_attrs & CVAR_TYPE_CHAR) {
      c_array = var;
      m = __ctalkRtGetMethod ();
      if (*(m -> returnclass)) {
	if (!strcmp (m -> returnclass, ARRAY_CLASSNAME)) {
	  member_class = get_class_by_name (CHARACTER_CLASSNAME);
	} else {
	  array_obj = create_object_init_internal
	    (name, rt_defclasses -> p_string_class, LOCAL_VAR,
	     c_array);
	  _delete_cvar (c);
	  __ctalkRegisterUserObject (array_obj);
	  return array_obj;
	}
      } else {
	return_class = rtinfo.rcvr_class_obj;
	if (!strcmp (return_class -> __o_name, ARRAY_CLASSNAME)) {
	  member_class = get_class_by_name (CHARACTER_CLASSNAME);
	} else {
	  member_class = get_class_by_name (STRING_CLASSNAME);
	}
      }
    } else if ((c -> type_attrs &  CVAR_TYPE_LONGDOUBLE) ||
	       (c -> type_attrs & CVAR_TYPE_FLOAT) ||
	       (c -> type_attrs & CVAR_TYPE_DOUBLE)) {
      d_array = var;
      member_class = get_class_by_name (FLOAT_CLASSNAME);
    } else {
      _warning ("__ctalkSaveCVARResource: Array, \"%s,\" type, \"%s,\" not supported directly by class library.\n", name, c -> type);
      return NULL;
    } /* if (!strcmp (c -> type, "int")) */
    array_obj = create_object_init_internal
      (name, rt_defclasses -> p_array_class, LOCAL_VAR, NULL);
    for (i = 0, t = array_obj -> instancevars; i < initializer_size; i++) {
      __ctalkDecimalIntegerToASCII (i, mbr_name);
      if (member_class == rt_defclasses -> p_integer_class) {
	if (i_array != NULL)  {
	  __ctalkDecimalIntegerToASCII (i_array[i], valbuf);
	}
      } else if (member_class == rt_defclasses -> p_longinteger_class) {
	if (ll_array != NULL) {
	  sprintf (valbuf, "%lld", ll_array[i]);
	}
      } else if (member_class == rt_defclasses -> p_character_class) {
	if (c_array != NULL) {
	  sprintf (valbuf, "\'%c\'", c_array[i]);
	}
      } else if (member_class == rt_defclasses -> p_string_class) {
	if (c_array != NULL) {
	  strcpy (valbuf, c_array);
	}
      } else if (member_class == rt_defclasses -> p_float_class)  {
	if (d_array != NULL) {
#if defined(__sparc__) && defined(__svr4__)
	  sprintf (valbuf, "%f", d_array[i]);
#else
	  sprintf (valbuf, "%lf", d_array[i]);
#endif
	}
      }
    elem_obj = __ctalkCreateObjectInit (mbr_name,
				      member_class -> __o_name,
				      _SUPERCLASSNAME(member_class),
				      LOCAL_VAR,
				      valbuf);

      mbr_obj = create_object_init_internal
	(mbr_name, rt_defclasses -> p_key_class, LOCAL_VAR, "");
      *(OBJECT **)mbr_obj -> __o_value =
	*(OBJECT **)mbr_obj -> instancevars -> __o_value =
	elem_obj;

      t -> next = mbr_obj;
      mbr_obj -> prev = t;
      t = mbr_obj;
    }
    _delete_cvar (c);
    return array_obj;
  }
  return NULL;
}

OBJECT *__ctalkSaveCVARArrayResource (char *name, int initializer_size,
				      void *var) {
  OBJECT *array_obj;
  if ((array_obj = __ctalkSaveCVARArrayResource_internal (name, initializer_size,
							  var)) != NULL) {
    __ctalkRegisterUserObject (array_obj);
  }
  return array_obj;
}

OBJECT *argblk_return_object;
OBJECT *argblk_return_code;
bool caller_return_request;

OBJECT *__ctalkRegisterArgBlkReturn (int return_code, OBJECT *return_object) {
  
  if (__call_stack_ptr >= (MAXARGS - 3)) {
    /* The return object will get trashed if the calling fn is main,
       which means we're exiting the program, Which means that
       __ctalk_exitFn deletes the method pools, WHICH means we should
       make a copy of the return object to return as the program's
       return value. Remember that an argblk consumes two slots in the
       call_stack below the calling fn or method. */
    __ctalkCopyObject (OBJREF(return_object), 
		       OBJREF(argblk_return_object));
  } else {
    argblk_return_object = return_object;
  }
  argblk_return_code = __ctalkRegisterIntReturn (return_code);
  return argblk_return_code;
}

void __ctalkArgBlkSetCallerReturn (OBJECT *break_obj) {
  if (IS_OBJECT(break_obj)) {
    if (*(int *)break_obj -> __o_value == -2) {
      caller_return_request = true;
    } else {
      caller_return_request = false;
    }
  } else {
    caller_return_request = false;
  }
}

bool __ctalkNonLocalArgBlkReturn (void) {
  /* reset the caller return request.*/
  if (caller_return_request) {
    caller_return_request = false;
    return true;
  } else {
    return false;
  }

}

OBJECT *__ctalkArgBlkReturnVal (void) {
  return argblk_return_object;
}
