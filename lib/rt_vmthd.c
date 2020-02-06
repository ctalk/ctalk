/* $Id: rt_vmthd.c,v 1.2 2020/02/06 18:27:32 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2019  Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "typeof.h"
#include "list.h"

extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

extern OBJECT *__ctalk_argstack[MAXARGS+1];
extern int __ctalk_arg_ptr;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

extern char *ascii[8193];             /* from intascii.h */

LIST *processes = NULL;

static void register_process (int pid) {
  LIST *l;
  char intbuf[0xff];
  ctitoa (pid, intbuf);
  l = new_list ();
  l -> data = (void *)strdup (intbuf);
  if (processes == NULL) {
    processes = l;
  } else {
    list_add (processes, l);
  }

}

void term_handler (int signo) {
  __ctalk_process_exitFn (1);
  _exit (0);
}

void register_term_handler (void) {
  struct sigaction chld_act, chld_act_old;
  sigset_t sigmask;
  sigemptyset (&sigmask);
  sigaddset(&sigmask, SIGTERM);
  chld_act.sa_handler = term_handler;
  chld_act.sa_flags = SA_RESETHAND;
  sigaction (SIGTERM, &chld_act, &chld_act_old);
}

#ifdef SIGCHLD_HANDLER
void chld_handler (int signo) {
  /* gives us (supposedly) a core dump where a spurious signal
     occurred. */
  kill (0, SIGSEGV);
}

void register_chld_handler (void) {
  struct sigaction chld_act, chld_act_old;
  sigset_t sigmask;
  sigemptyset (&sigmask);
  sigaddset(&sigmask, SIGCHLD);
  chld_act.sa_handler = chld_handler;
  chld_act.sa_flags = SA_RESETHAND;
  sigaction (SIGCHLD, &chld_act, &chld_act_old);
}
#endif

/* COULD THIS BE A MACRO? */
static inline OBJECT*vmthd_str_to_ptr (char *s) {
  OBJECT *__p;
  __p = *(OBJECT **)s;
  return __p;
}

void delete_processes (void) {
  LIST *t, *t_prev;
  int pid;
  if (processes) {
    for (t = processes; t && t -> next; t = t -> next)
      ;
    if (t == processes) {
      pid = atoi ((char *)t -> data);
      kill (pid, SIGTERM);
      __xfree (MEMADDR(t -> data));
      __xfree (MEMADDR(t));
      processes = NULL;
    } else {
      while (t != processes) {
	t_prev = t -> prev;
	pid = atoi ((char *)t -> data);
	kill (pid, SIGTERM);
	__xfree (MEMADDR(t -> data));
	__xfree (MEMADDR(t));
	t = t_prev;
      }
      pid = atoi ((char *)t -> data);
      kill (pid, SIGTERM);
      __xfree (MEMADDR(t -> data));
      __xfree (MEMADDR(t));
      processes = NULL;
    }
  }
}

static int __method_object_init (OBJECT *rcvr_obj, OBJECT *class_obj, 
				 METHOD *actual_method) {
  /*
   *  The instance variables are defined in the class/Method library
   *  file.  
   *  
   *  Note that this function does not yet fill in the parameters
   *  and arguments, nor local_objects, user_objects, local_cvars,
   *  imported, error_line, and error_column members.
   */
  OBJECT *name_instance_var, *selector_instance_var,
    *returnClass_instance_var, *rcvrClassObject_instance_var,
    *methodFn_instance_var, *source_instance_var, 
    *nParams_instance_var, *varargs_instance_var, *nArgs_instance_var,
    *argFrameTop_instance_var, *rcvrFrameTop_instance_var,
    *isInitialized_instance_var;
  char s[MAXLABEL];
  if ((name_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "methodName", TRUE)) == NULL)
    return ERROR;
  __ctalkSetObjectValueVar (name_instance_var, actual_method -> name);
  if (*(actual_method -> selector)) {
    if ((selector_instance_var = 
	 __ctalkGetInstanceVariable (rcvr_obj, "methodSelector", TRUE)) == NULL)
      return ERROR;
    __ctalkSetObjectValueVar (selector_instance_var, 
			      actual_method->selector);
  }
  if (*(actual_method -> returnclass)) {
    if ((returnClass_instance_var = 
	 __ctalkGetInstanceVariable (rcvr_obj, "returnClass", TRUE)) == NULL)
      return ERROR;
    __ctalkSetObjectValueVar (returnClass_instance_var, 
			      actual_method->returnclass);
  }
  if ((rcvrClassObject_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "rcvrClassObject", 
				   TRUE)) == NULL)
    return ERROR;

  SYMVAL(rcvrClassObject_instance_var -> __o_value) =
    SYMVAL(rcvrClassObject_instance_var -> instancevars -> __o_value)
    = (unsigned long)actual_method -> rcvr_class_obj;

  if ((methodFn_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "methodFn", TRUE)) == NULL)
    return ERROR;
  SYMVAL(methodFn_instance_var -> __o_value) =
    SYMVAL(methodFn_instance_var -> instancevars -> __o_value)
    = (unsigned long)actual_method -> cfunc;
  
  if (actual_method -> src != NULL) {
    if ((source_instance_var = 
	 __ctalkGetInstanceVariable (rcvr_obj, "methodSource", TRUE)) == NULL)
      return ERROR;
    __ctalkSetObjectValueVar (methodFn_instance_var, actual_method->src);
  }
  if ((nParams_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "nParams", TRUE)) == NULL)
    return ERROR;

  *(int *)nParams_instance_var -> __o_value = actual_method -> n_params;
  *(int *)nParams_instance_var -> instancevars -> __o_value =
    actual_method -> n_params;

  if ((varargs_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "varargs", TRUE)) == NULL)
    return ERROR;

  *(int *)varargs_instance_var -> __o_value = actual_method -> varargs;
  *(int *)varargs_instance_var -> instancevars -> __o_value =
    actual_method -> varargs;

  if ((nArgs_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "nArgs", TRUE)) == NULL)
    return ERROR;

  *(int *)nArgs_instance_var -> __o_value = actual_method -> n_args;
  *(int *)nArgs_instance_var -> instancevars -> __o_value =
    actual_method -> n_args;

  if ((argFrameTop_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "argFrameTop", TRUE)) == NULL)
    return ERROR;

  *(int *)argFrameTop_instance_var -> __o_value =
    actual_method -> arg_frame_top;
  *(int *)argFrameTop_instance_var -> instancevars -> __o_value =
    actual_method -> arg_frame_top;

  if ((rcvrFrameTop_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "rcvrFrameTop", TRUE)) == NULL)
    return ERROR;

  *(int *)rcvrFrameTop_instance_var -> __o_value =
    actual_method -> rcvr_frame_top;
  *(int *)rcvrFrameTop_instance_var -> instancevars ->__o_value =
    actual_method -> rcvr_frame_top;

  if ((isInitialized_instance_var = 
       __ctalkGetInstanceVariable (rcvr_obj, "isInitialized", TRUE)) == NULL)
    return ERROR;
  __ctalkSetObjectValueVar (isInitialized_instance_var,
			    ascii[TRUE]);

  return SUCCESS;
}

int __ctalkDefinedInstanceMethodObject (OBJECT *rcvr, char *classname, char *name) {
  OBJECT *rcvr_obj, *class_obj;
  METHOD *defined_method, *__m;

  rcvr_obj = (IS_VALUE_INSTANCE_VAR(rcvr) ? rcvr -> __o_p_obj : rcvr);
  if ((class_obj = __ctalkGetClass (classname)) == NULL) {
    _warning ("__ctalkDefinedMethodObject: Undefined class %s.\n",
	      classname);
    return ERROR;
  }
  for (defined_method = NULL, __m = class_obj -> instance_methods; 
       __m && !defined_method; __m = __m -> next) 
    if (!strcmp (__m -> name, name)) 
      defined_method = __m;
  if (!defined_method) {
    _warning ("__ctalkDefinedInstanceMethodObject: Undefined method %s (class %s).\n",
	      name, classname);
    return ERROR;
  }
  return __method_object_init (rcvr_obj, class_obj, defined_method);
}

int __ctalkDefinedClassMethodObject (OBJECT *rcvr, char *classname, char *name) {
  OBJECT *rcvr_obj, *class_obj;
  METHOD *defined_method, *__m;

  rcvr_obj = (IS_VALUE_INSTANCE_VAR(rcvr) ? rcvr -> __o_p_obj : rcvr);
  if ((class_obj = __ctalkGetClass (classname)) == NULL) {
    _warning ("__ctalkDefinedMethodObject: Undefined class %s.\n",
	      classname);
    return ERROR;
  }
  for (defined_method = NULL, __m = class_obj -> class_methods; 
       __m && !defined_method; __m = __m -> next) 
    if (!strcmp (__m -> name, name)) 
      defined_method = __m;
  if (!defined_method) {
    _warning ("__ctalkDefinedClassMethodObject: Undefined method %s (class %s).\n",
	      name, classname);
    return ERROR;
  }
  return __method_object_init (rcvr_obj, class_obj, defined_method);
}

typedef struct {
  OBJECT *methodArgs_instance_var,
    *nArgs_instance_var, *nParams_instance_var,
    *varargs_instance_var, *methodName_instance_var,
    *methodFn_instance_var;
} VMETHOD;

int method_object_message_call = FALSE;
#define __LIST_HEAD(__o) ((__o)->instancevars->next)

int method_object_msg_internal (OBJECT *, OBJECT *, int);
int method_object_msg_internal_2_args (OBJECT *, OBJECT *,
				       OBJECT *, OBJECT *,
				       int);

int __ctalkMethodObjectMessage (OBJECT *rcvr, OBJECT *method_instance) {
  return method_object_msg_internal (rcvr, method_instance, false);
}

int __ctalkMethodObjectMessage2Args (OBJECT *rcvr,
				     OBJECT *method_instance,
				     OBJECT *arg1, OBJECT *arg2) {
  return method_object_msg_internal_2_args (rcvr, method_instance,
					    arg1, arg2, false);
}

int __ctalkBackgroundMethodObjectMessage (OBJECT *rcvr,
					  OBJECT *method_instance) {
  return method_object_msg_internal (rcvr, method_instance, true);
}

int __ctalkBackgroundMethodObjectMessage2Args (OBJECT *rcvr,
					       OBJECT *method_instance,
					       OBJECT *arg1, OBJECT *arg2) {
  return method_object_msg_internal_2_args (rcvr, method_instance,
					    arg1, arg2, true);
}

int method_object_msg_internal (OBJECT *rcvr, OBJECT *method_instance,
				int background) {

  VMETHOD *v;
  OBJECT *rcvr_obj, *method_obj,
    *t, *t_prev, *arg_ref, *rcvr_obj_tmp,
    *(*fn)(), *t_start, *arg_head;
  OBJECT *result_object;
  int i, n_args, n_params, varargs, n_th_arg,
    retval;
  METHOD *target_method;
  char buf[MAXLABEL];

  v = (VMETHOD *)__xalloc (sizeof (VMETHOD));

  if (rcvr == NULL) {
    fprintf (stderr, "__ctalkMethodObjectMessage: Receiver is NULL.\n");
    return ERROR;
  }

  rcvr_obj = IS_VALUE_INSTANCE_VAR(rcvr) ? rcvr->__o_p_obj : rcvr;
  method_obj = IS_VALUE_INSTANCE_VAR(method_instance) ? 
    method_instance->__o_p_obj :
    method_instance;

  v -> methodName_instance_var = method_obj -> instancevars -> next;
  v -> methodArgs_instance_var = v -> methodName_instance_var -> next;
  v -> methodFn_instance_var = v -> methodArgs_instance_var -> next;
  v -> nArgs_instance_var = v -> methodFn_instance_var -> next;
  n_args = *(int *)v -> nArgs_instance_var -> instancevars -> __o_value;
  v -> nParams_instance_var = v -> nArgs_instance_var -> next;
  n_params = *(int *)v -> nParams_instance_var -> instancevars -> __o_value;
  v -> varargs_instance_var = v -> nParams_instance_var -> next;
  varargs = *(int *)v -> varargs_instance_var -> instancevars -> __o_value;
    
  rcvr_obj_tmp = rcvr_obj;
  if ((target_method = __ctalkFindInstanceMethodByName 
       (&rcvr_obj_tmp,
	v -> methodName_instance_var->instancevars->__o_value,
	TRUE, ANY_ARGS)) == NULL)
    return ERROR;

  if ((n_args != n_params) && !varargs)  {
    _warning("Warning: __ctalkMethodObjectMessage: Have %d arguments.\n"
	     "Warning: \tMethod %s requires %d arguments.\n", n_args, 
	     v -> methodName_instance_var -> instancevars -> __o_value,
	     n_params);
    __warning_trace ();
    return ERROR;
  }

  switch (n_args)
    {
    case 1:
      arg_head = __LIST_HEAD(v -> methodArgs_instance_var);
      if ((arg_ref = vmthd_str_to_ptr
	   (arg_head -> instancevars -> __o_value)) != NULL) {
	__add_arg_object_entry_frame (target_method, arg_ref);
	__ctalk_arg_push_ref (arg_ref);
      }
      break;
    case 2:
      arg_head = __LIST_HEAD(v -> methodArgs_instance_var);
      if ((arg_ref = vmthd_str_to_ptr
	   (arg_head -> instancevars -> __o_value)) != NULL) {
	__add_arg_object_entry_frame (target_method, arg_ref);
	__ctalk_arg_push_ref (arg_ref);
      }
      arg_head = arg_head -> next;
      if ((arg_ref = vmthd_str_to_ptr
	   (arg_head -> instancevars -> __o_value)) != NULL) {
	__add_arg_object_entry_frame (target_method, arg_ref);
	__ctalk_arg_push_ref (arg_ref);
      }
      break;
    default:
      for (t = __LIST_HEAD(v -> methodArgs_instance_var);
	   t; t = t -> next) {
	if ((arg_ref = vmthd_str_to_ptr (t -> instancevars -> __o_value)) != NULL) {
	  __add_arg_object_entry_frame (target_method, arg_ref);
	  __ctalk_arg_push_ref (arg_ref);
	}
	if (t -> next == NULL)
	  arg_head = t;
      }
      break;
    }

  errno = 0;
  fn = (OBJECT *(*)())SYMVAL(v -> methodFn_instance_var -> instancevars ->
			     __o_value);
  if (errno != 0) {
    strtol_error (errno, "__ctalkMethodObjectMessage ()", 
		  v -> methodFn_instance_var->instancevars->__o_value);
  }
	  
  
  method_object_message_call = TRUE;
  result_object = __ctalk_method_from_object (rcvr_obj, fn,
					      target_method, background,
					      &retval);
  method_object_message_call = FALSE;
  t = arg_head;
  for (n_th_arg = 0; n_th_arg < n_args; n_th_arg++) {
    if (t) {
      OBJECT *__arg_ref_tmp, *__arg_object_tmp;
      int nrefs_tmp;
      __arg_ref_tmp = vmthd_str_to_ptr (t -> instancevars->__o_value);
      __arg_object_tmp = __ctalk_arg_pop ();
      __ctalk_arg_push (__arg_object_tmp);
      nrefs_tmp = __ctalk_arg_cleanup (result_object);
      if ((__arg_ref_tmp == __arg_object_tmp) && !nrefs_tmp) {
	*t->__o_value = '\0';
	*t->instancevars->__o_value = '\0';
      }
      
      t_prev = t -> prev;
      /*
       *  Arg's ref count is incremented by List : pushItemRef.
       */
      if ((arg_ref = vmthd_str_to_ptr (t -> instancevars->__o_value)) != NULL) {
	__ctalkSetObjectScope (arg_ref, arg_ref->scope & ~VAR_REF_OBJECT);
      }
      /* Don't adjust arg's reference count again when deleting from argument 
	 list. */
      t->instancevars->__o_value[0] = '\0';
      t -> __o_value[0] = '\0';
      __ctalkDeleteObject(t);

      if (t == t_start) {
	/* 
	 *  The LIST_HEAD object's prev pointer is NULL.
	 */
	if (v -> methodArgs_instance_var -> instancevars -> next != t_start) {
	  if (t -> prev) {
	    t -> prev -> next = NULL;
	    t = t -> prev;
	  } else {
	    v -> methodArgs_instance_var -> instancevars -> next = NULL;
	  } 
	} else {
	  v -> methodArgs_instance_var -> instancevars -> next = NULL;
	}
      } else {
	t = t_prev;
	if (t) {
	  t -> next = NULL;
	  if (v -> methodArgs_instance_var -> instancevars -> next == t)
	    v -> methodArgs_instance_var -> instancevars -> next = NULL;
	} else {
	  v -> methodArgs_instance_var -> instancevars -> next = NULL;
	}
      }
    }
  }
  *(int *)v -> nArgs_instance_var -> __o_value = n_args - n_params;
  *(int *)v -> nArgs_instance_var -> instancevars -> __o_value =
    n_args - n_params;
	   
  __xfree (MEMADDR(v));
 return retval;
}

int method_object_msg_internal_2_args (OBJECT *rcvr, OBJECT *method_instance,
				       OBJECT *arg1, OBJECT *arg2,
				       int background) {

  VMETHOD *v;
  OBJECT *rcvr_obj, *method_obj,
    *t, *t_prev, *arg_ref, *rcvr_obj_tmp,
    *(*fn)(), *t_start, *arg_head;
  OBJECT *result_object, *__arg_object_tmp;
  int i, n_args, n_params, varargs, n_th_arg,
    retval, nrefs_tmp;
  METHOD *target_method;
  char buf[MAXLABEL];

  v = (VMETHOD *)__xalloc (sizeof (VMETHOD));

  if (rcvr == NULL) {
    fprintf (stderr, "__ctalkMethodObjectMessage: Receiver is NULL.\n");
    return ERROR;
  }

  rcvr_obj = IS_VALUE_INSTANCE_VAR(rcvr) ? rcvr->__o_p_obj : rcvr;
  method_obj = IS_VALUE_INSTANCE_VAR(method_instance) ? 
    method_instance->__o_p_obj :
    method_instance;

  v -> methodName_instance_var = method_obj -> instancevars -> next;
  v -> methodFn_instance_var = v -> methodName_instance_var -> next -> next;
    
  rcvr_obj_tmp = rcvr_obj;
  if ((target_method = __ctalkFindInstanceMethodByName 
       (&rcvr_obj_tmp,
	v -> methodName_instance_var->instancevars->__o_value,
	FALSE, ANY_ARGS)) == NULL)
    return ERROR;

  __add_arg_object_entry_frame (target_method, arg1);
  __ctalk_arg_push_ref (arg1);
  __add_arg_object_entry_frame (target_method, arg2);
  __ctalk_arg_push_ref (arg2);

  fn = (OBJECT *(*)())SYMVAL(v -> methodFn_instance_var -> instancevars ->
			     __o_value);

  method_object_message_call = TRUE;
  result_object = __ctalk_method_from_object (rcvr_obj, fn,
					      target_method, background,
					      &retval);
  method_object_message_call = FALSE;


  __arg_object_tmp = __ctalk_arg_pop ();
  __ctalk_arg_push (__arg_object_tmp);
  nrefs_tmp = __ctalk_arg_cleanup (result_object);
  __arg_object_tmp = __ctalk_arg_pop ();
  __ctalk_arg_push (__arg_object_tmp);
  nrefs_tmp = __ctalk_arg_cleanup (result_object);

  __xfree (MEMADDR(v));
 return retval;
}

void init_ctalk_process (OBJECT *, METHOD *);

OBJECT *__ctalk_method_from_object (OBJECT *rcvr_obj, OBJECT *(*__cfunc)(),
				    METHOD *target_method,
				    int background, int *ret_out) {
  OBJECT *__method_rcvr_obj_, *__class_obj_, *__result_obj_ = NULL;
  METHOD *method;
  int i, pid;

  __class_obj_ = rcvr_obj->__o_class;
  __method_rcvr_obj_ = rcvr_obj;

  method = target_method;

  /*
   *  If the method object calls a superclass.
   *  TODO - This might need to be reworked.
   */
  if (method -> n_params) {
    if ((method -> n_params % method -> n_args == 0) &&
	(method -> varargs == 0)) {
      if (rcvr_obj -> __o_superclass == 
	  method -> rcvr_class_obj) {
	METHOD *__calling_method;
	if (((__calling_method = __ctalkRtGetMethod ()) != NULL) &&
	    !strcmp (method -> name, __calling_method -> name) &&
	    (__calling_method -> n_args > __calling_method -> n_params)) {
	  OBJECT *__a;
	  int __n_th_param;
	  for (__n_th_param = 0; __n_th_param < method -> n_params; 
	       __n_th_param++) {
	    __a=__calling_method->args[__calling_method->n_args-1]->obj;
	    __ctalkDeleteArgEntry
	      (__calling_method->args[__calling_method->n_args-1]);
	    __calling_method -> args[__calling_method -> n_args - 1] = NULL;
	    __calling_method -> n_args -= 1;
	    (void)__objRefCntDec (OBJREF(__a));
	    __add_arg_object_entry_frame (method, __a);
	  }
	} else {
	  /* Silently ignore? */
	  /* __ctalkExceptionInternal (NULL,  */
	  /* 			  wrong_number_of_arguments_x,  */
	  /* 			  method -> name,0); */
	  /* __ctalkHandleRunTimeExceptionInternal (); */
	  /* return null_result_obj (method, ARG_VAR); */
	}
      }
    }
  }

  method -> arg_frame_top = __ctalk_arg_ptr + 1;
  method -> rcvr_frame_top = __ctalk_receiver_ptr + 1;

  __save_rt_info (rcvr_obj, __class_obj_, NULL, method, __cfunc, False);

  __ctalk_receiver_push_ref (rcvr_obj);

  register_term_handler ();

  if (background) {
    switch (pid = fork ())
      {
      case 0:
	init_ctalk_process (rcvr_obj, method);
	break;
      case -1:
	printf ("ctalk: Could not start process %s : %s: %s.\n",
		rcvr_obj -> __o_class -> __o_name,
		method -> name, strerror (errno));
	break;
      default:
	register_process (pid);
	*ret_out = pid;
	break;
      }
  } else {
    __result_obj_ = (__cfunc)();
    *ret_out = SUCCESS;
  }

  __ctalk_receiver_pop_deref ();

  __ctalk_methodReturn (rcvr_obj, method, __result_obj_);

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
      if (!strcmp (method -> rcvr_class_obj -> __o_name, OBJECT_CLASSNAME)) {
	__ctalk_arg_pop ();
      }
      method -> args[method -> n_args - 1] = NULL;
      --(method -> n_args);
    }

    for (t_local = M_LOCAL_VAR_LIST(method); 
	 t_local; t_local = t_local -> next) {
      if (t_local -> var_object == __result_obj_) {
	if (__result_obj_ == M_LOCAL_VAR_LIST(method) -> var_object) {
	  v_next = M_LOCAL_VAR_LIST(method) -> next;
	  delete_varentry (M_LOCAL_VAR_LIST(method));
	  M_LOCAL_VAR_LIST(method) = t_local = v_next;
	} else {
	  if (t_local -> prev) t_local -> prev -> next = t_local -> next;
	  if (t_local -> next) t_local -> next -> prev = t_local -> prev;
	  delete_varentry (t_local);
	}
      }
      if (!t_local) 
	break;
    }

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
	  OBJECT *result_object_value_var;
	  result_object_value_var = __result_obj_ -> instancevars;
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
    for (i = 0; i < method -> n_params; i++) {
      if (IS_ARG(method -> args[method -> n_args - 1])) {
	__ctalkDeleteArgEntry (method -> args[method -> n_args - 1]);
	method->args[method -> n_args - 1] = NULL;
	--method -> n_args;
      }
    }
  }

  __restore_rt_info ();

  return __result_obj_;
}

extern void proc_init_argstack (void); /*  defined in rt_args.c. */

void proc_init_receivers (void) {
  int i;
  for (i = __ctalk_receiver_ptr + 1; i <= MAXARGS; ++i) {
    __ctalk_receivers[i] = NULL;
  }
  __ctalk_receiver_ptr = MAXARGS;
}

void proc_init_call_stack (void) {
  int i;
  for (i = __call_stack_ptr + 1; i <= MAXARGS; ++i) {
    if (__call_stack[i] -> _rt_fn != NULL)
      __xfree (MEMADDR(__call_stack[i]->_rt_fn));
    _delete_rtinfo (__call_stack[i]);
    __call_stack[i] = NULL;
  }
  __call_stack_ptr = MAXARGS;
}

void init_ctalk_process (OBJECT *rcvr_obj, METHOD *method) {
  OBJECT *(*__cfunc)(), *__result_obj_;
  int retval;
  proc_init_receivers ();
  proc_init_argstack ();
  proc_init_call_stack ();
  __cfunc = method -> cfunc;
  __save_rt_info (rcvr_obj, rcvr_obj -> __o_class, NULL,
		  method, __cfunc, False);
  __ctalk_receiver_push_ref (rcvr_obj);
  __result_obj_ = (__cfunc)();
  __ctalk_methodReturn (rcvr_obj, method, __result_obj_);
  __restore_rt_info ();
  /* Don't try to return to the main thread - the stacks aren't
     synced. */
  /* Note that we don't share the processes list with this memory
     space - so we can't remove the process from the processes list
     from here.  We can just try a SIGTERM on all of the processes in
     the process list when main exits. (We can also call
     register_process in this memory space if it's really needed.) 

     Also, the child process won't come here if the parent process
     exits first - in which case the parent sends a SIGTERM to the
     child process(es).  In that case, the child terminates by
     calling _exit (0), in term_handler (), above. */

  retval = __ctalkToCInteger (__result_obj_, FALSE);
  __ctalk_process_exitFn (1);
  if (__result_obj_ == NULL) {
    _exit (SUCCESS);
  } else {
    _exit (retval);
  }
}

#include <sys/wait.h>

int __ctalkProcessWait (int child_pid, int *child_ret_val_out,
			int *signo_out, int *errno_out) {
  int waitstatus, r;
  *signo_out = *child_ret_val_out = *errno_out = 0 ;
  r = waitpid (child_pid, &waitstatus, WNOHANG);
  if (r == child_pid) {
    if (WIFEXITED(waitstatus)) {
      *child_ret_val_out = WEXITSTATUS(waitstatus);
    } else if (WIFSIGNALED(waitstatus)) {
      *signo_out = WTERMSIG(waitstatus);
    }
  } else if (r < 0) {
    *errno_out = errno;
  }
  return r;
}

