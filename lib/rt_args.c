/* $Id: rt_args.c,v 1.19 2020/05/13 00:13:54 rkiesling Exp $ */

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
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"

extern RT_INFO rtinfo;              /* Declared in rtinfo.c. */
extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;
extern int eval_status;
extern int method_object_message_call;  /* Declared in rt_vmthd.c */

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

void save_method_object_args (METHOD *);

OBJECT *__ctalk_argstack[MAXARGS+1] = {NULL,};
int __ctalk_arg_ptr = MAXARGS;

/* If not in a local that uses a western alphabet, use isspace. */
static inline char  *skip_leading_whitespace (char *s) {
  char *t;
  for (t = s; *t <= ' '; ++t)
    ;
  return t;
}

int __ctalkGetArgPtr (void) {
  return __ctalk_arg_ptr;
}

OBJECT *__ctalkArgAt (int idx) {
  if (idx <= MAXARGS)
    return __ctalk_argstack[idx];
  else
    return NULL;
}

static inline void __arg_ref_cnt_store (OBJREF_T obj_p, int nrefs) {
  OBJECT *vars;

  if (IS_OBJECT (*obj_p)) {
    (*obj_p) -> nrefs = nrefs;
    for (vars = (*obj_p) -> instancevars; vars; vars = vars -> next)
      __arg_ref_cnt_store (OBJREF (vars), nrefs);
  } 
}

static inline int prev_arg_tok (MESSAGE_STACK messages, int idx,
				int stack_start) {
  register int i = idx;
  for (i = idx + 1; i <= stack_start; i++) {
    if (!M_ISSPACE(messages[i]))
      return i;
  }
  return ERROR;
}

static inline int prev_arg_tok_b (EXPR_PARSER *p, int idx) {
  register int i = idx;
  for (i = idx + 1; i <= p -> msg_frame_start; i++) {
    if (!M_ISSPACE(p -> m_s[i]))
      return i;
  }
  return ERROR;
}

static inline int next_arg_tok (MESSAGE_STACK messages, int idx, 
				int stack_end) {
  register int i = idx;
  for (i = idx - 1; i >= stack_end; i--) {
    if (!M_ISSPACE(messages[i]))
      return i;
  }
  return ERROR;
}

static inline int next_arg_tok_b (EXPR_PARSER *p, int idx) { 
  register int i = idx;
  for (i = idx - 1; i >= p -> msg_frame_top; i--) {
    if (!M_ISSPACE(p -> m_s[i]))
      return i;
  }
  return ERROR;
}

/* skips opening parens */
static int prev_op_tok (MESSAGE_STACK messages, int tok_idx,
			int stack_start) {
  register int i = tok_idx;
  for (i = tok_idx + 1; i <= stack_start; i++) {
    if (M_ISSPACE(messages[i])) {
      continue;
    } else if (M_TOK(messages[i]) == OPENPAREN) {
      continue;
    } else {
      return i;
    }
  }
  return ERROR;
}

int __ctalk_arg_push (OBJECT *__o) {
  if (__ctalk_arg_ptr == 0)
    _error ("__ctalk_arg_push: stack_overflow.\n");

  if (!IS_OBJECT(__o)) {
    OBJECT *o_null;
    _warning ("__ctalk_arg_push: NULL argument object.\n");
    o_null = null_result_obj (NULL, ARG_VAR);
    __ctalk_argstack[__ctalk_arg_ptr--] = o_null;
    __arg_ref_cnt_store (OBJREF(o_null), o_null -> nrefs + 1);
    return ERROR;
  }

  __ctalk_argstack[__ctalk_arg_ptr--] = __o;
  return SUCCESS;
}

int __ctalk_arg_push_ref (OBJECT *__o) {
  if (__ctalk_arg_ptr == 0)
    _error ("__ctalk_arg_push: stack_overflow.\n");

  if (!IS_OBJECT(__o)) {
    OBJECT *o_null;
    _warning ("__ctalk_arg_push_ref: NULL argument object.\n");
    o_null = null_result_obj (NULL, ARG_VAR);
    __ctalk_argstack[__ctalk_arg_ptr--] = o_null;
    __arg_ref_cnt_store (OBJREF(o_null), o_null -> nrefs + 1);
    return ERROR;
  } else {
    __ctalk_argstack[__ctalk_arg_ptr--] = __o;
    __arg_ref_cnt_store (OBJREF(__o), __o -> nrefs + 1);
  }
  return SUCCESS;
}

OBJECT *__ctalk_arg_pop (void) {

  OBJECT *__o;
  if (__ctalk_arg_ptr > (MAXARGS + 1))
    _error ("arg_pop: Message stack overflow, arg_ptr = %d.\n", 
	    __ctalk_arg_ptr);
  if (__ctalk_arg_ptr == MAXARGS)
    return NULL;
  __o = __ctalk_argstack[++__ctalk_arg_ptr];
  __ctalk_argstack[__ctalk_arg_ptr] = NULL;
  if (IS_OBJECT(__o))
    return __o;
  else
    return NULL;
}

static void null_argument_warning (int arg_ptr) {
  fprintf (stderr, 
	   "Warning: __ctalk_arg_pop_deref: NULL argument object.\nWarning: Argument pointer = %d.\n",
	   arg_ptr);
  __warning_trace ();
}

OBJECT *__ctalk_arg_pop_deref (void) {

  OBJECT *__o;
  if (__ctalk_arg_ptr > (MAXARGS + 1))
    _error ("arg_pop: Message stack overflow, arg_ptr = %d.\n", 
	    __ctalk_arg_ptr);
  if (__ctalk_arg_ptr == MAXARGS)
    return NULL;
  if ((__o = __ctalk_argstack[++__ctalk_arg_ptr]) != NULL) {
    __ctalk_argstack[__ctalk_arg_ptr] = NULL;
    __arg_ref_cnt_store (OBJREF (__o), __o -> nrefs - 1);
  } else {
    null_argument_warning (__ctalk_arg_ptr - 1);
  }
  return __o;
}

extern bool is_constant_rcvr;
extern bool is_constant_arg;

static char __ctalk_arg_text[MAXMSG] = "";

void set_ctalk_arg_text (char *text) {
  strcpy (__ctalk_arg_text, text);
}

char *ctalk_arg_text (void) {
  return __ctalk_arg_text;
}

void clear_ctalk_arg_text (void) {
  *__ctalk_arg_text = 0;
}

int __ctalk_arg (char *__recv_name, char *__method, int n_params, 
		 void *__arg) {

  OBJECT *__recv_obj = NULL, *__recv_obj_ptr, *__recv_class, *arg_object = NULL;
  OBJECT *arg_p_object = NULL;
  OBJECT *arg_on_stack;
  METHOD *m;
  CVAR *c, *recv_cvar = NULL;
  int arg_is_eval_result = FALSE;
  int recv_cvar_is_created = FALSE;
  int errno_save;
  void *c_i;
  bool single_tok, binary_arg;


  binary_arg = (n_params > MAXARGS);

  if (!binary_arg) /* > MAXARGS is a binary arg */
    single_tok = (strchr ((char *)__arg, ' ') ? false : true);
  else
    single_tok = true;

  /*
   *   The state, is_constant_rcvr, needs to be set 
   *   only by _constant_rcvr.  See the notes in 
   *   __eval_expr () and _constant_rcvr ().
   *   This variable should probaly go away in a 
   *   future release.
   */
  is_constant_rcvr = False;


  /*
   *   Find the receiver depending on whether it is a class
   *   object, a dictionary object, a method parameter, a CVAR, or
   *   a constant expression.  If it's a CVAR and not an OBJECT *,
   *   then it gets its alias if necessary later on.
   */

  if ((__recv_obj = __ctalk_get_arg_tok (__recv_name)) == NULL) 
    if ((recv_cvar = get_method_arg_cvars (__recv_name)) != NULL) {
      if ((__recv_obj = cvar_object (recv_cvar, &recv_cvar_is_created))
	  == NULL) {
	if ((__recv_obj = __constant_rcvr (__recv_name)) 
	    == NULL) 
	  _error ("__ctalk_arg: %s: Undefined receiver %s.\n",
		  rtinfo.source_file, __recv_name);
      }
    } else if ((__recv_obj = __constant_rcvr (__recv_name)) 
	       == NULL) {
      _error ("__ctalk_arg: %s: Undefined receiver %s.\n",
	      rtinfo.source_file, __recv_name);
    }
  if ((__recv_class = __recv_obj->__o_class)== NULL)
    _error ("__ctalk_arg: %s: Undefined receiver %s class %s.\n",
	    rtinfo.source_file, __recv_obj -> __o_name, 
	    __recv_obj -> CLASSNAME);

  if (recv_cvar != NULL && recv_cvar_is_created) {
    __ctalkDeleteObject (__recv_obj);
    /* There hasn't been a case that's needed this yet. */
    __recv_obj = __recv_class;
  }

  /*
   *   If we can't find the method, then there is probably an
   *   argument mismatch.  Check for the method in the calling
   *   method's receiver class.
   */

  __recv_obj_ptr = __recv_obj;
  if (!binary_arg) {
    if ((m = __ctalkFindInstanceMethodByName (&__recv_obj_ptr, __method, FALSE,
					      n_params))
	== NULL) {
      __recv_obj_ptr = __recv_obj;
      if ((m = __ctalkFindClassMethodByName (&__recv_obj_ptr, __method, FALSE,
					     n_params)) == NULL) {
	__recv_obj_ptr = __recv_obj->instancevars;
	if (__recv_obj_ptr && 
	    ((m = __ctalkFindInstanceMethodByName 
	      (&__recv_obj_ptr, __method, FALSE, n_params)) == NULL)) {
	  __recv_obj_ptr = __recv_obj->instancevars;
	  if (__recv_obj_ptr &&
	      ((m = __ctalkFindClassMethodByName
		(&__recv_obj_ptr, __method, TRUE, n_params)) == NULL)) {
	    return ERROR;
	  }
	}
      }
    }
  } else {
    if ((m = __ctalkFindInstanceMethodByName (&__recv_obj_ptr, __method, FALSE,
					      1))
	== NULL) {
      return ERROR;
    }
  }

  if (!binary_arg) {
    /*
     *  Check if the argument is already on the argument stack or is 
     *  in the dictionary.
     */
    if (!strncmp ((char *)__arg, METHOD_ARG_ACCESSOR_FN, 
		  METHOD_ARG_ACCESSOR_FN_LENGTH)) {
      int arg_n;
      METHOD *__calling_method;
      __calling_method = 
	__ctalkFindInstanceMethodByFn (&__recv_obj, 
				       rtinfo.method_fn, TRUE);
      sscanf ((char *)__arg, "__ctalk_arg_internal (%d)", &arg_n);
      arg_object = __ctalk_argstack[__calling_method -> arg_frame_top + arg_n];
      __add_arg_object_entry_frame (m, arg_object);
      __ctalk_arg_push_ref (arg_object);
      return SUCCESS;
    }
  }

  if (single_tok) {
    if (binary_arg) {
      int val = (int)__arg;
      arg_object = create_object_init_internal
	("int_constant", rt_defclasses -> p_integer_class,
	 CREATED_PARAM, "");
      __xfree (MEMADDR(arg_object -> __o_value));
      arg_object -> __o_value = __xalloc (INTBUFSIZE);
      *((int *)arg_object -> __o_value) = val;
      __xfree (MEMADDR(arg_object -> instancevars -> __o_value));
      arg_object -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
      *((int *)arg_object -> instancevars -> __o_value) = val;
      __add_arg_object_entry_frame (m, arg_object);
      __ctalk_arg_push_ref (arg_object);
      return SUCCESS;
    } else if (IS_OBJECT((OBJECT *)__arg)) {
      arg_object = (OBJECT *)__arg;
      arg_p_object = NULL;
      if ((arg_on_stack = __ctalk_arg_pop ()) != NULL) {
	__ctalk_arg_push (arg_on_stack);
	if ((arg_on_stack == arg_object) && !IS_VALUE_INSTANCE_VAR(arg_object)) {
	  arg_p_object = arg_object;
	} else  if (arg_object -> attrs &
		    OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) {
	  arg_p_object = arg_object;
	} else {
	  /***/
	  if ((arg_object == (OBJECT *)__arg) &&
	      (IS_OBJECT(arg_object -> instancevars)) &&
	      (arg_object -> instancevars -> attrs & OBJECT_IS_VALUE_VAR)) {
	    /* I.e., if it's a declared object, instance or class
	       variable, regardless of parent, and is the return value
	       of a __ctalk_arg_internal call. */
	    arg_p_object = arg_object;
	  } else {
	    arg_p_object = 
	      ((arg_object -> __o_p_obj != NULL) ? 
	       arg_object -> __o_p_obj : arg_object);
	  }
	}
      } else {
	if (arg_object -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) {
	  arg_p_object = arg_object;
	} else if (str_eq (arg_object -> __o_name, (char *)__arg)) {
	  arg_p_object = arg_object;
	} else {
	  arg_p_object = 
	    ((arg_object -> __o_p_obj != NULL) ? 
	     arg_object -> __o_p_obj : arg_object);
	}
      }
      if (IS_OBJECT (arg_p_object)) {
	__add_arg_object_entry_frame (m, arg_p_object);
	__ctalk_arg_push_ref (arg_p_object);
      } else {
	__add_arg_object_entry_frame (m, arg_object);
	__ctalk_arg_push_ref (arg_object);
      }
      return SUCCESS;
    } else if (str_eq (__method, CONSTRUCTOR_METHOD)) { /* non-primitive constructors */
      if ((arg_object = __ctalk_get_constructor_arg_tok ((char *)__arg)) != NULL) {
	__add_arg_object_entry_frame (m, arg_object);
	__ctalk_arg_push_ref (arg_object);
	return SUCCESS;
      }
    } else if ((arg_object = __ctalk_get_arg_tok ((char *)__arg)) != NULL) {
      if ((c_i = active_i (arg_object)) != I_UNDEF)
	arg_object = create_param_i (arg_object, c_i);
      __add_arg_object_entry_frame (m, arg_object);
      __ctalk_arg_push_ref (arg_object);
      return SUCCESS;
    }
  }

  if (!arg_object) {

    /* 
     *    If we have to define an object, the primitives need the 
     *    receiver on the stack.  
     */
    __ctalk_receiver_push_ref (__recv_obj);
    if (IS_C_ASSIGNMENT_OP_NAME(__method)) {
      eval_status |= EVAL_STATUS_ASSIGN_ARG;
    } else {
      eval_status = 0;
    }
    errno_save = errno;
    arg_object = eval_expr ((char *)__arg, __recv_class, m, NULL,
			    __recv_obj -> scope, TRUE);
    errno = errno_save;
    arg_is_eval_result = TRUE;
    __ctalk_receiver_pop_deref ();
    delete_method_arg_cvars ();
    reset_last_eval_result ();

    if (!arg_object)
      arg_object = null_result_obj (m, __recv_obj -> scope);

    /* Saving the arg text here helps it from being
       walloped by an eval_expr call. */
    strcpy (__ctalk_arg_text, (char *)__arg);

  } /* if (!arg_object) */

  if (is_constant_rcvr == True) {
    is_constant_rcvr = False;
    __ctalkDeleteObject (__recv_obj);
  }

  /*
   *  Don't look for a parent object of an expression result if:
   *    1. If the object is identified by a terminal token that is an
   *       instance variable.
   *    2. The object that is the result of a terminal token 
   *       evaluation is referenced by a Symbol or Key object.
   */
  if ((eval_status & 
       (EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_INSTANCE_VAR)) ||
      (arg_is_eval_result && (arg_object -> scope & VAR_REF_OBJECT) &&
       (eval_status & 
	(EVAL_STATUS_TERMINAL_TOK|EVAL_STATUS_VAR_REF)))) {
    arg_p_object = arg_object;
  } else {
    if ((arg_on_stack = __ctalk_arg_pop ()) != NULL) {
      __ctalk_arg_push (arg_on_stack);
      if ((arg_on_stack == arg_object) && arg_is_eval_result
	  && !IS_VALUE_INSTANCE_VAR(arg_object)) {
	arg_p_object = arg_object;
      } else  if (arg_object -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) {
	arg_p_object = arg_object;
      } else if (str_eq (arg_object -> __o_name, (char *)__arg)) {
	arg_p_object = arg_object;
      } else if (eval_status & EVAL_STATUS_NAMED_PARAMETER) {
	/* single token, set by __ctalk_get_arg_tok. */
	arg_p_object = arg_object;
      } else {
	arg_p_object = 
	  ((arg_object -> __o_p_obj != NULL) ? 
	   arg_object -> __o_p_obj : arg_object);
      }
    } else {
      if (arg_object -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) {
	arg_p_object = arg_object;
      } else if (!IS_VALUE_INSTANCE_VAR(arg_object) &&
		 match_instancevar_name 
		 (arg_object -> __o_class -> instancevars,
		  arg_object)) {
	arg_p_object = arg_object;
      } else if (str_eq (arg_object -> __o_name, (char *)__arg)) {
	arg_p_object = arg_object;
      } else if (eval_status & EVAL_STATUS_NAMED_PARAMETER) {
	/* single token, set by __ctalk_get_arg_tok. */ 
	arg_p_object = arg_object;
      } else {
	arg_p_object = 
	  ((arg_object -> __o_p_obj != NULL) ? 
	   arg_object -> __o_p_obj : arg_object);}
    } 
  }
  if (arg_p_object -> nrefs == 0)
    __ctalkSetObjectScope (arg_p_object, ARG_VAR);
  __add_arg_object_entry_frame (m, arg_p_object);
  __ctalk_arg_push_ref (arg_p_object);
  if (arg_p_object -> __o_vartags && 
      !IS_EMPTY_VARTAG(arg_p_object -> __o_vartags))
    register_arg_active_varentry (arg_p_object -> __o_vartags -> tag);
  eval_status = 0;
  return SUCCESS;
}

/* 
 *  Used in init blocks with "new" method.
 *  The method cache mechanism checks for this function when it 
 *  looks for classes to load when declaring local objects.
 */
int __ctalkCreateArgA (char *__recv_name, char *__method, void *__arg) {
  return __ctalkCreateArg (__recv_name, __method, __arg);
}

int __ctalkCreateArg (char *__recv_name, char *__method, void *__arg) {

  OBJECT *__recv_class, *arg_object = NULL;
  OBJECT *t;
  METHOD *m = NULL;
  CVAR *c;

  /* See the comments in __ctalk_arg (). */

  /*
   *  Caution! The only place this function is used so far is 
   *  is for creating local method objects, so it assumes that
   *  __recv_name is the name of a class.  Not tested yet with
   *  random receivers from __ctalk_get_object ().
   */
  if ((__recv_class = __ctalkGetClass (__recv_name)) == NULL)
      _error ("__ctalkCreateArg: %s: Undefined receiver class %s.\n",
	      rtinfo.source_file, __recv_name);

  for (t = __recv_class; t && !m; t = t -> __o_superclass)
    for (m = t -> instance_methods; m; m = m -> next) {
      if (str_eq (m -> name, __method))
	break;
    }

  if (!m)
    return ERROR;

#if 0
  /* This isn't currently needed. */
  if (!strncmp ((char *)__arg, "__ctalk_arg_internal", 
		METHOD_ARG_ACCESSOR_FN_LENGTH)) {
    int arg_n;
    METHOD *__calling_method;
    __calling_method = 
      __ctalkFindInstanceMethodByFn (&__recv_class, 
			      rtinfo.method_fn, TRUE);
    sscanf ((char *)__arg, "__ctalk_arg_internal (%d)", &arg_n);
    arg_object = __ctalk_argstack[__calling_method -> arg_frame_top + arg_n];
    __add_arg_object_entry_frame (m, arg_object);
    __ctalk_arg_push_ref (arg_object);
    return SUCCESS;
  }
#endif
  
  if (IS_OBJECT ((OBJECT *)__arg)) {
    arg_object = (OBJECT *)__arg;
    __add_arg_object_entry_frame (m, arg_object);
    __ctalk_arg_push_ref (arg_object);
    return SUCCESS;
  }

  arg_object = 
    __ctalkCreateObjectInit ((char *)__arg,
			     __recv_name,
			     ((__recv_class -> __o_superclass) ? 
			      __recv_class->__o_superclass->__o_name : 
			      NULL),
			     ARG_VAR,
			     NULL);

  /* normally set in __ctalkCreateObjectInit, this is occasionally needed */
  if ((__recv_class -> attrs & INT_BUF_SIZE_INIT) ||
      (__recv_class -> attrs & CHAR_BUF_SIZE_INIT)) {
    arg_object -> attrs |= OBJECT_VALUE_IS_BIN_INT;
    arg_object -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
  } else if (__recv_class -> attrs & LONGLONG_BUF_SIZE_INIT) {
    arg_object -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
    arg_object -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
  } else if (__recv_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
    arg_object -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
    arg_object -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
  }

  __add_arg_object_entry_frame (m, arg_object);
  __ctalk_arg_push_ref (arg_object);

  return SUCCESS;
}

#if 0
/* Needed? */
OBJECT *__ctalk_getstack (void *p) {
  int i;
  for (i = __ctalk_arg_ptr + 1; i <= MAXARGS; i++)
    if (p == __ctalk_argstack[i])
      return p;
  return NULL;
}
#endif

/*
 *   Method args start from 0, which is at 
 *   __ctalk_argstack[__call_stack[__call_stack_ptr+1]->_arg_frame_top].
 */
OBJECT *__ctalk_arg_internal (int n) {
  return __ctalk_argstack[__call_stack[__call_stack_ptr+1]->_arg_frame_top+n];
}

OBJECT *__ctalk_arg_value_internal (int n) {
  OBJECT *arg_obj;
  arg_obj = 
    __ctalk_argstack[__call_stack[__call_stack_ptr+1]->_arg_frame_top+n];
  return ((arg_obj -> instancevars) ? arg_obj -> instancevars : arg_obj);
}

/* 
 *  Called once per arg from within a class library or source module.
 *  Global objects should be deleted explicitly by the application
 *  program.
 *
 *  Returns the number of reference counts of the argument object.
 */

int __ctalk_arg_cleanup (OBJECT *result_obj) {
  OBJECT *arg_object;
  int arg_nrefs, arg_scope;

  if (__ctalk_argstack[__ctalk_arg_ptr+1] == result_obj) {
    if ((arg_object = __ctalk_arg_pop_deref ()) != NULL) {
      if (IS_OBJECT(arg_object)) {
	return arg_object -> nrefs;
      } else {
	return ERROR;
      }
    } else {
      return ERROR;
    }
  }

  clear_arg_active_varentry ();
  if (is_constant_arg)
    is_constant_arg = false;

  if ((arg_object = __ctalk_arg_pop_deref ()) == NULL) 
    return ERROR;
  arg_nrefs = arg_object -> nrefs;
  arg_scope = arg_object -> scope;
  if (arg_object -> nrefs == 0) {
    if (IS_VALUE_INSTANCE_VAR(arg_object)) {
      if (arg_object -> nrefs < arg_object -> __o_p_obj -> nrefs) {
	arg_object -> nrefs = arg_object -> __o_p_obj -> nrefs;
	return arg_object -> nrefs;
      } else {
	__ctalkDeleteObjectInternal (arg_object);
	return 0;
      }
    } else {
      if (arg_object -> scope != GLOBAL_VAR) {
	/* sanity check */
	if (IS_OBJECT(arg_object -> __o_p_obj)) {
	  if (arg_object -> nrefs < arg_object -> __o_p_obj -> nrefs) {
	    __objRefCntSet (OBJREF(arg_object),
			    arg_object -> __o_p_obj -> nrefs);
	  } else {
	    __ctalkDeleteObjectInternal (arg_object);
	  }
	} else {
	  __ctalkDeleteObjectInternal (arg_object);
	}
	return 0;
      } else if (arg_object -> scope == GLOBAL_VAR) {
	__objRefCntSet (OBJREF(arg_object), 1);
	return 1;
      }
    }
  } else {
    if (arg_object -> scope == ARG_VAR) {
      if (!IS_EMPTY_VARTAG(arg_object -> __o_vartags)) {
	if (IS_OBJECT(arg_object -> __o_p_obj) &&
	    (arg_object -> __o_p_obj -> attrs &
	     OBJECT_IS_NULL_RESULT_OBJECT)) {
	  __ctalkDeleteObject (arg_object -> __o_p_obj);
	  return 0;
	} else {
	  if (arg_object -> nrefs > 1) {
	    __ctalkDeleteObjectInternal (arg_object);
	    --arg_nrefs;
	  }
	  return arg_nrefs;
	}
      }
      /*
       *  The arg is going to be dereffed to 0 anyway.
       */
      if (!is_arg (arg_object)) {
	__ctalkDeleteObject(arg_object);
	return 0;
      }
    } else {
      if (arg_object -> scope & VAR_REF_OBJECT) {
	save_local_objects_to_extra ();
	return arg_nrefs;
      }
      if (arg_object -> scope == CREATED_PARAM) {
	if (is_arg (arg_object)) {
	  save_local_objects_to_extra ();
	  return arg_nrefs;
	} else {
	  __ctalkDeleteObject (arg_object);
	  return 0;
	}
      }
      if (arg_object -> scope == CREATED_CVAR_SCOPE) {
	/* If we see this scope here, and not anywhere else, then
	   we're leaving the scope of the argument. 
	   TODO - Unless it's also the result object of a method call,
	   so we should check here.
	*/
	if (!is_receiver (arg_object) &&
	    !is_arg (arg_object)) {
	  if (arg_object -> __o_p_obj) {
	    if (!is_receiver (arg_object -> __o_p_obj) &&
		!is_arg (arg_object -> __o_p_obj)) {
	      __ctalkDeleteObject (arg_object);
	      return 0;
	    }
	  } else if (HAS_VARTAGS(arg_object) &&
		     !IS_EMPTY_VARTAG(arg_object -> __o_vartags) &&
		     arg_object -> __o_vartags -> tag -> var_object ==
		     arg_object) {
	    __ctalkDeleteObjectInternal (arg_object);
	    return 0;
	  } else {
	    __ctalkDeleteObject (arg_object);
	    return 0;
	  }
	}
      }
      if (arg_object -> scope != GLOBAL_VAR) {
	if (arg_object -> nrefs == 0) {
	  if (arg_object -> scope & CREATED_PARAM) {
	    if (IS_OBJECT(arg_object -> instancevars)) {
	      if (vec_is_registered (arg_object -> instancevars -> __o_value)) {
		arg_object -> instancevars -> __o_value = NULL;
	      }
	    }
	    if (vec_is_registered (arg_object -> __o_value)) {
	      arg_object -> __o_value = NULL;
	    }
	  }
	}
	__ctalkDeleteObjectInternal (arg_object);
	--arg_nrefs; if (arg_nrefs < 0) arg_nrefs = 0;
      }


      if (arg_scope & (GLOBAL_VAR|CREATED_PARAM)) {
	if ((arg_nrefs == 0) && IS_OBJECT(arg_object)) {
	  if (IS_OBJECT(arg_object -> instancevars)
	      && vec_is_registered (arg_object -> instancevars -> __o_value)) {
	    arg_object -> instancevars -> __o_value = NULL;
	  }
	  if (vec_is_registered (arg_object -> __o_value)) {
	    arg_object -> __o_value = NULL;
	  }
	  __ctalkDeleteObjectInternal (arg_object);
	} else {
	  if (arg_scope & SUBEXPR_CREATED_RESULT && !is_arg (arg_object)) {
	    __ctalkDeleteObject (arg_object);
	  }
	}
      } else {

	if (!IS_OBJECT(arg_object -> __o_p_obj)) {
	  if (object_is_decrementable(arg_object, NULL, NULL))
	    --arg_nrefs; if (arg_nrefs < 0) arg_nrefs = 0;
	  if (object_is_deletable(arg_object, NULL, NULL))
	    arg_nrefs = 0;
	  _cleanup_temporary_objects (arg_object, NULL, NULL, rt_cleanup_null);
	}
      }
    }
  }
  save_local_objects_to_extra ();
  return arg_nrefs;
}

bool is_arg (OBJECT *__o) {
  int i;
  for (i = __ctalk_arg_ptr + 1; i <= MAXARGS; i++)
    if (__o == __ctalk_argstack[i])
      return TRUE;
  return FALSE;
}

static OBJECT *__ctalk_arg_expr_tok (MESSAGE_STACK, int, int, int,
				     int);

static int __arg_typecast_expr (MESSAGE_STACK messages,
				int method_msg_ptr,
				int start_idx, int end_idx,
				METHOD *method,
				int stack_start, int stack_end) {
  OBJECT *arg_val_obj;
  int cast_end_idx, expr_start_idx;
  if (__rt_is_typecast (messages, start_idx, end_idx)) {
    if (typecast_is_pointer (messages, start_idx, end_idx)) {
      typecast_ptr_expr (messages, start_idx, end_idx,
			 stack_start, stack_end);
    } else {
      typecast_value_expr (messages, start_idx, end_idx);
    }
    if ((arg_val_obj = M_VALUE_OBJ(messages[start_idx])) != NULL) {
      __add_arg_object_entry_frame (method, arg_val_obj);
      __ctalk_arg_push_ref (arg_val_obj);
      return end_idx;
    } else {
      cast_end_idx = __ctalkMatchParen (messages, start_idx, end_idx);
      expr_start_idx = __ctalkNextLangMsg (messages, cast_end_idx, end_idx);
      if (expr_start_idx == end_idx) {
	arg_val_obj = __ctalk_arg_expr_tok (messages, method_msg_ptr, 
					    expr_start_idx,
					    end_idx, true);
	arg_val_obj = cast_object_to_c_type (messages, start_idx,
					     cast_end_idx,
					     arg_val_obj);
	__add_arg_object_entry_frame (method, arg_val_obj);
	__ctalk_arg_push_ref (arg_val_obj);
      }
    }
  } else {
    return FALSE;
  }
  return FALSE;
}

static bool __arg_typecast_expr_2 (MESSAGE_STACK messages, 
				   int arg_start_idx, int arg_end_idx) {
  int inner_paren_idx, lookahead, i;
  OBJECT *typecast_obj;

  for (inner_paren_idx = arg_start_idx; ; --inner_paren_idx) {
    if ((lookahead = __ctalkNextLangMsg (messages, inner_paren_idx,
					 arg_end_idx)) == ERROR) {
      return false;
    }
    if (M_TOK(messages[lookahead]) != OPENPAREN) {
      break;
    }

  }
  if (inner_paren_idx <= arg_end_idx) {
    return false;
  }

  if (__rt_is_typecast (messages, inner_paren_idx, arg_end_idx)) {
    int __close_paren_idx;
    if ((__close_paren_idx = 
	 __ctalkMatchParen (messages, inner_paren_idx, arg_end_idx)) == ERROR) {
      __ctalkExceptionInternal (NULL, mismatched_paren_x,
				M_NAME (messages[lookahead]), 0);
    }
    if (typecast_is_pointer (messages, inner_paren_idx, arg_end_idx)) {
      typecast_ptr_expr (messages, arg_start_idx, __close_paren_idx,
			 arg_start_idx, arg_end_idx);
      typecast_obj = M_VALUE_OBJ(messages[inner_paren_idx]);
      for (i = arg_start_idx; i >= arg_end_idx; --i) {
	if (!IS_OBJECT(messages[i]->obj)) {
	  messages[i] -> obj = typecast_obj;
	} else {
	  messages[i] -> value_obj = typecast_obj;
	}
      }
      return true;
    } else {
      /* Non-pointer typecasts normally just get elided - so return
	 false and then go on and evaluate the rest of the
	 expression. */
      for (i = arg_start_idx; i >= arg_end_idx; --i) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
     return false;
    }
  }

  return false;
}

/*
 *  Return the stack index of the first possible receiver
 *  of this method, or -1 there is not immediate or previous
 *  receiver.  Called only by __rt_method_args, so the only
 *  stack should be e_messages.  The backtrack is only
 *  slightly greedy.  It checks whether the method token is 
 *  valid for the primary receiver of the complete expression,
 *  then for the first token of the argument subexpression only.
 *
 *  TO DO - No compound receivers in this version.
 */

static int __method_backtrack (char *method_name,
			       MESSAGE_STACK messages,
			       int method_idx) {
  EXPR_PARSER *p;
  int stack_end, arg_rcvr_idx, primary_method_idx, primary_rcvr_idx;
  OBJECT *primary_rcvr_obj, *arg_rcvr_obj;
  METHOD *arg_method, *primary_method;

  p = __current_expr_parser ();
  stack_end = __get_expr_parser_ptr ();
  
  if (__ctalk_isMethod_2 (M_NAME(messages[method_idx]),
			messages,
			method_idx,
			p -> msg_frame_start)) {
    if (((arg_rcvr_idx = 
	 __ctalkPrevLangMsg (messages, method_idx, p->msg_frame_start)) 
	!= ERROR) &&
	((primary_method_idx = 
	  __ctalkPrevLangMsg (messages, arg_rcvr_idx, p->msg_frame_start))
	 != ERROR) &&
	((primary_rcvr_idx = 
	  __ctalkPrevLangMsg (messages, primary_method_idx, p->msg_frame_start))
	 != ERROR)) {
      if (__ctalk_isMethod_2 (M_NAME(messages[primary_method_idx]),
			     messages,
			     primary_method_idx,
			     p -> msg_frame_start)) {
	primary_rcvr_obj = M_VALUE_OBJ(messages[primary_rcvr_idx]);
	arg_rcvr_obj = M_OBJ(messages[arg_rcvr_idx]);
	if (__ctalkFindInstanceMethodByName (&primary_rcvr_obj,
					       M_NAME(messages[method_idx]),
					     FALSE, ANY_ARGS)) {
	  /* Incomplete expression. */
	  if (!IS_OBJECT(primary_rcvr_obj) || !IS_OBJECT(arg_rcvr_obj))
	    return arg_rcvr_idx;

	  if ((((arg_method = 
	       __ctalkFindInstanceMethodByName (&arg_rcvr_obj,
						M_NAME(messages[method_idx]),
						FALSE, ANY_ARGS)) != NULL) ||
	       ((arg_method = 
		  __ctalkFindClassMethodByName (&arg_rcvr_obj,
						M_NAME(messages[method_idx]),
						FALSE, ANY_ARGS)) != NULL)) &&
	      (((primary_method =
		__ctalkFindInstanceMethodByName (&primary_rcvr_obj,
					 M_NAME(messages[primary_method_idx]),
						 FALSE, ANY_ARGS)) != NULL) ||
	       ((primary_method =
		 __ctalkFindClassMethodByName (&primary_rcvr_obj,
				       M_NAME(messages[primary_method_idx]),
					       FALSE, ANY_ARGS)) != NULL))) {
	    if (!strcmp (primary_method -> params[0] -> class,
			 arg_method -> returnclass) ||
		__ctalkIsSubClassOf (arg_method -> returnclass,
				     primary_method -> params[0] -> class)) {
	      return arg_rcvr_idx;
	    } else {
	      /*
	       *  This should be enough to recognize Collection subclasses.
	       */
	      if (((!strcmp (arg_method -> returnclass, "Any")) &&
		  __ctalkIsSubClassOf (arg_rcvr_obj -> CLASSNAME,
				       arg_method -> 
				       rcvr_class_obj -> __o_name)) ||
		  !strcmp (arg_rcvr_obj -> CLASSNAME,
			   arg_method -> returnclass) || 
		  __ctalkIsSubClassOf (arg_rcvr_obj -> CLASSNAME,
				       arg_method -> returnclass)) {
		return arg_rcvr_idx;
	      } else {
		if (str_eq (M_NAME(messages[arg_rcvr_idx]), "self") &&
		    __ctalkIsInlineCall ()) {
		  /* This might not always be the longest match, but it
		     is consistent with the rest of the expression parser,
		     It's also more compartmentalized when using
		     self within an argument block. Check first because
		     of the more limited scope.
		  */
		  return arg_rcvr_idx;
		} else {
		  if (!strcmp (primary_method -> returnclass,
			       arg_method -> returnclass) ||
		      __ctalkFindMethodByName (OBJREF(primary_rcvr_obj),
					       M_NAME(messages[method_idx]),
					       FALSE, ANY_ARGS)) {
		    return primary_rcvr_idx;
		  } else {
		    return arg_rcvr_idx; /* Incomplete expression. */
		  }
		}
	      }
	    }
	  } else {
	    return arg_rcvr_idx;  /* Incomplete expression. */
	  }
	}
	/*
	 *  Handle cases where there is only an argument method found.
	 */
	if (((__ctalkFindInstanceMethodByName (&arg_rcvr_obj,
					      M_NAME(messages[method_idx]),
					       FALSE, ANY_ARGS)) != NULL) ||
	    ((__ctalkFindClassMethodByName (&arg_rcvr_obj,
					    M_NAME(messages[method_idx]),
					    FALSE, ANY_ARGS)) != NULL)) {
	  /*
	   *  The method can match the receiver of the argument
	   *  subexpression.
	   *
	   *  TO DO - Check whether superceded by the cases above.
	   */
	  return arg_rcvr_idx;
	}
      } else {
	return arg_rcvr_idx;
      }
    }
  } else {
    /*
     *  Next arg token is not a method.
     */
    return ERROR;
  }
  return ERROR;
}

/*
 * Define a single token evaluation as a macro,
 * so we don't have to define rt_argstrs[][], 
 * for another function.
 */
#define EVAL_SINGLE_ARG_TOK { rt_argstrs[0].arg = \
		    strdup (messages[rt_argstrs[0].start_idx] -> name); \
		  rt_argstrs[0].end_idx = rt_argstrs[0].start_idx; \
		  arg_val_obj = __ctalk_arg_expr (messages, method_msg_ptr,\
						  rt_argstrs[0].arg, \
                                                  rt_argstrs[0].start_idx,\
                                                  rt_argstrs[0].end_idx, \
						  is_arg_expr);	\
		  __xfree (MEMADDR(rt_argstrs[0].arg)); \
                  } \

static int __rt_arglist_limit (METHOD *, MESSAGE_STACK, int, int,
			       int, int, int, int *);
static OBJECT *__ctalk_arg_expr (MESSAGE_STACK, int, char *, int,
				 int, int);

int __rt_method_args(METHOD *method, MESSAGE_STACK messages, 
		     int stack_start, int stack_end, int method_msg_ptr,
		     int is_arg_expr){

  int arglist_start,
    arglist_end,
    i,
    j,
    last_arg_tok_idx,
    next_arg_tok_idx, 
    n_arg_internal_parens,
    last_tok, is_empty; 
  bool paren_delim;
  char argbuf[MAXMSG];
  OBJECT *arg_val_obj;
  MESSAGE *m_arg; 
  struct _rt_argstrs {
    char *arg;
    int start_idx,
      end_idx;
  } rt_argstrs[MAXARGS];
  int rt_argstrptr = 0;
  int arg_frame_for_push = method -> n_args;

  /*
   *  If we're at the end of the expression stack, then the
   *  end of the argument list is the method token index.
   */
  if ((arglist_start = 
       next_arg_tok (messages, method_msg_ptr, stack_end)) == ERROR)
    return method_msg_ptr;

  arglist_end = __rt_arglist_limit (method,
				    messages, stack_start, stack_end, 
				    method_msg_ptr, arglist_start, 
				    method -> n_params, &is_empty);

  if (arglist_start == arglist_end) {
    if (is_empty) {
      /* only a separator after the method. */
      if (method -> n_params)
	_warning ("Method %s: Wrong number of arguments\n.", method -> name);
      return arglist_start;
    }

    arg_val_obj = M_VALUE_OBJ(messages[arglist_start]);

    if (!IS_OBJECT(arg_val_obj)) {
      /* try to reconstruct a trashed object. */
      int __prev_tok;
      OBJECT *__recv_obj;
      if ((__prev_tok = __ctalkPrevLangMsg (messages, method_msg_ptr,
					    stack_start)) != ERROR) {
	__recv_obj = M_VALUE_OBJ(messages[__prev_tok]);
	if (IS_OBJECT(__recv_obj)) {
	  arg_val_obj = eval_expr (M_NAME(messages[arglist_start]),
				   NULL, NULL, __recv_obj -> __o_class,
				   __recv_obj -> scope, FALSE);
	  __objRefCntSet (OBJREF(arg_val_obj),
			  __recv_obj -> nrefs);
	  if (IS_OBJECT(messages[arglist_start]->obj)) {
	    messages[arglist_start] -> value_obj = arg_val_obj;
	  } else {
	    messages[arglist_start] -> obj = arg_val_obj;
	  }
	}
      }
    }

    __add_arg_object_entry_frame (method, arg_val_obj);
    __ctalk_arg_push_ref (arg_val_obj);
    if (messages[method_msg_ptr] -> receiver_msg)
      messages[arglist_start] -> receiver_msg = 
	messages[method_msg_ptr] -> receiver_msg;
    if (messages[method_msg_ptr] -> receiver_obj)
      messages[arglist_start] -> receiver_obj = 
	messages[method_msg_ptr] -> receiver_obj;
    return arglist_start;

  } /* if (arglist_start == arglist_end) */

  /*
   *  Handle cases where the argument is already evaluated.
   */
  if (messages[arglist_start] -> value_obj && 
      (messages[arglist_start] -> value_obj == 
       messages[arglist_end] -> value_obj) &&
      messages[arglist_start] -> evaled) {
    /*
     *  This case needs looking at - a subexpression as an argument
     *  should need to be evaled again in order.  However, if a method
     *  is registered as a user object, we cannot (so far) guarantee
     *  that the object won't be needed again before it is superceded.
     *  So in these cases we make a copy as an ARG_VAR.  There should
     *  not (again, so far)be any cases where an object return value
     *  needs to be passed by reference, so simply making a copy of
     *  the object is workable. 
     *
     *  Need good test cases here.
     */
    if ((M_TOK(messages[arglist_start]) != OPENPAREN) &&
	(M_TOK(messages[arglist_end]) != CLOSEPAREN)) {
      OBJECT *t;
      if (messages[arglist_start] -> attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL) {
	if (messages[arglist_start] -> obj -> scope & METHOD_USER_OBJECT) {
	  t = __ctalkCreateObjectInit 
	    (messages[arglist_start] -> obj -> __o_name,
	     messages[arglist_start] -> obj -> CLASSNAME,
	     _SUPERCLASSNAME(messages[arglist_start] -> obj),
	     ARG_VAR,
	     ((messages[arglist_start] -> obj -> instancevars) ?
	      (messages[arglist_start] -> obj -> instancevars -> __o_value) :
	      (messages[arglist_start] -> obj -> __o_value)));
	  __add_arg_object_entry_frame (method, t);
	  __ctalk_arg_push_ref (t);
	} else {
	  __add_arg_object_entry_frame (method, messages[arglist_start]->obj);
	  __ctalk_arg_push_ref (messages[arglist_start]->obj);
	}
      } else {
	__add_arg_object_entry_frame (method, 
				      messages[arglist_end] -> value_obj);
	__ctalk_arg_push_ref (messages[arglist_end]->value_obj);
      }
      return arglist_end;
    } else if (IS_OBJECT(messages[arglist_start] -> value_obj) &&
	       IS_OBJECT(messages[arglist_end] -> obj) &&
	       (messages[arglist_start] -> value_obj ==
		messages[arglist_end] -> obj) &&
	       messages[arglist_end] -> attrs & RT_OBJ_IS_INSTANCE_VAR) {
      /*
       *   An expression like <obj> <instancevar> can also have this
       *   state (in which case it's already been evaluated).
       */
	__add_arg_object_entry_frame (method, 
				      messages[arglist_start] -> value_obj);
	__ctalk_arg_push_ref (messages[arglist_start]->value_obj);
	return arglist_end;
    } else {
      if ((M_TOK(messages[arglist_start]) == OPENPAREN) &&
	  (M_TOK(messages[arglist_end]) == CLOSEPAREN)) {
	if ((M_VALUE_OBJ(messages[arglist_start]) ==
	     M_VALUE_OBJ(messages[arglist_end])) && 
	    messages[arglist_start] -> evaled) {
	  __add_arg_object_entry_frame 
	    (method, M_VALUE_OBJ(messages[arglist_start]));
	  __ctalk_arg_push_ref (M_VALUE_OBJ(messages[arglist_start]));
	  return arglist_end;
	}
      } else {
	if (IS_C_UNARY_MATH_OP(M_TOK(messages[arglist_start])) &&
	    (M_TOK(messages[arglist_end]) == CLOSEPAREN) &&
	    (M_VALUE_OBJ(messages[arglist_start]) == 
	     M_VALUE_OBJ(messages[arglist_end])) &&
	    (messages[arglist_start] -> evaled)) {
	  __add_arg_object_entry_frame 
	    (method, M_VALUE_OBJ(messages[arglist_start]));
	  __ctalk_arg_push_ref (M_VALUE_OBJ(messages[arglist_start]));
	  return arglist_end;
	} else {
	  if ((M_TOK(messages[arglist_end]) == CLOSEPAREN) &&
	      (M_VALUE_OBJ(messages[arglist_start]) == 
	       M_VALUE_OBJ(messages[arglist_end])) &&
	      messages[arglist_start] -> evaled) {
	    __add_arg_object_entry_frame 
	      (method, M_VALUE_OBJ(messages[arglist_start]));
	    __ctalk_arg_push_ref (M_VALUE_OBJ(messages[arglist_start]));
	    return arglist_end;
	  }
	}
      }
    }
  }

  if (messages[arglist_start] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
    if (IS_OBJECT(messages[arglist_start] -> value_obj)) {
      int i_1;
      bool evaled = true;
      for (i_1 = arglist_start; i_1 > arglist_end; i_1--) {
	if (messages[i_1] -> evaled == 0) {
	  evaled = false;
	}
      }
      if (evaled) {
	__add_arg_object_entry_frame 
	  (method, 
	   messages[arglist_start]->value_obj);
	__ctalk_arg_push_ref (messages[arglist_start]->value_obj);
	return arglist_end;
      }
    }
  }

  if ((messages[arglist_end] -> attrs & 
       (RT_OBJ_IS_INSTANCE_VAR|RT_OBJ_IS_CLASS_VAR)) &&
      (messages[arglist_end] -> receiver_msg == messages[arglist_start])) {
    __add_arg_object_entry_frame 
      (method, 
       messages[arglist_end]->receiver_msg->value_obj);
    __ctalk_arg_push_ref (messages[arglist_end]->receiver_msg->value_obj);
    return arglist_end;
  }

  /*
   *  Adjust the start and end indexes to exclude parentheses.
   * 
   *  However, we also have to check that the parentheses
   *  enclose the entire expression, so we don't also 
   *  exclude parentheses in expressions like (1 + 1) - (2 + 2).
   *
   *  The function also checks for parentheses for each argument,
   *  below.
   */

  if ((messages[arglist_start] -> tokentype == OPENPAREN) &&
      (messages[arglist_end] -> tokentype == CLOSEPAREN) &&
      _external_parens (messages, arglist_start, arglist_end)) {
    paren_delim = True;

    ++messages[arglist_start] -> evaled;
    ++messages[arglist_start] -> output;
    ++messages[arglist_end] -> evaled;
    ++messages[arglist_end] -> output;
    arglist_start = next_arg_tok (messages, arglist_start, stack_end);
    arglist_end = __ctalkPrevLangMsg (messages, arglist_end, stack_start);
  } else {
    paren_delim = False;
  }

  rt_argstrs[0].start_idx = arglist_start;
  rt_argstrs[0].end_idx = arglist_end;


  /*
   *  If the method requires only one argument, and does not
   *  accept a variable number of arguments, evaluate the 
   *  entire expression as a single argument.
   */
  if (!method -> varargs && (method -> n_params == 1)) {

    /*
     *  Here we have to evaluate the argument to determine if 
     *  it is a single token or an expression.
     */

    if (messages[rt_argstrs[0].start_idx] -> tokentype == OPENPAREN) {
      /*
       *  First check for parentheses.
       */
      if ((rt_argstrs[0].end_idx = 
	__ctalkMatchParen (messages, rt_argstrs[0].start_idx, 
			   stack_end)) == ERROR) {
	_warning ("__rt_method_args: mismatched parentheses.");
      } else {
	/*
	 * If there's more expression following a closing parenthesis,
	 * adjust the arglist limit.
	 */
	if ((paren_delim == False) &&
	    (arglist_end < rt_argstrs[0].end_idx)) {
	  rt_argstrs[0].end_idx = arglist_end;
	}
      }
      if (__arg_typecast_expr (messages, method_msg_ptr,
			       rt_argstrs[0].start_idx,
			       rt_argstrs[0].end_idx, method,
			       stack_start, stack_end)) {
	arg_val_obj = messages[rt_argstrs[0].start_idx]->obj;
	return rt_argstrs[0].end_idx;
      } else {
	arg_val_obj = __ctalk_arg_expr_tok (messages, method_msg_ptr, 
					    rt_argstrs[0].start_idx,
					    rt_argstrs[0].end_idx,
					    is_arg_expr);
      }
      goto arg_reevaled_1;
    } else {
      /* Argument is not enclosed in parentheses. */
      if (rt_argstrs[0].start_idx != rt_argstrs[0].end_idx) {
	/*
	 *  Check for an aggregate C variable arg here, too.
	 */
	if (messages[rt_argstrs[0].start_idx] -> attrs & 
	    RT_CVAR_AGGREGATE_TOK) {
	  arg_val_obj = M_VALUE_OBJ(messages[rt_argstrs[0].start_idx]);
	  goto arg_reevaled_1;
	}
	/*
	 * Check for an argument starting with a unary operator.
	 */
	if (messages[rt_argstrs[0].start_idx] -> attrs &
	    RT_TOK_IS_PREFIX_OPERATOR) {
	  if ((arg_val_obj = M_VALUE_OBJ(messages[rt_argstrs[0].start_idx])) 
	      == NULL) {;
	    arg_val_obj = __ctalk_arg_expr_tok (messages, method_msg_ptr,
						rt_argstrs[0].start_idx,
						rt_argstrs[0].end_idx,
						is_arg_expr);
	  }
	  goto arg_reevaled_1;
	}
  	if ((next_arg_tok_idx =
  	     next_arg_tok (messages, rt_argstrs[0].start_idx,
  				 stack_end)) != ERROR) {
 	  int first_rcvr_idx;
 	  if ((first_rcvr_idx = 
 	       __method_backtrack (M_NAME(messages[next_arg_tok_idx]),
 				   messages,
 				   next_arg_tok_idx)) != ERROR) {
	    /* '==' is what we want here. */
 	    if (first_rcvr_idx ==
 		__ctalkPrevLangMsg (messages,
 				    next_arg_tok_idx,
 				    __current_expr_parser () -> msg_frame_start))
 	      {
		int first_rcvr_idx_2, method_idx_2;
		/*  Check the next token for a method. */
		if ((method_idx_2 = 
		     next_arg_tok (messages, rt_argstrs[0].start_idx,
					 stack_end)) != ERROR) {
		  if (__ctalk_isMethod_2 (M_NAME(messages[method_idx_2]),
					messages, method_idx_2, 
				__current_expr_parser () -> msg_frame_start)) {
		    first_rcvr_idx_2 = 
		      __method_backtrack (M_NAME(messages[method_idx_2]),
					  messages,
					  method_idx_2);
		    if (first_rcvr_idx_2 == 
			__ctalkPrevLangMsg (messages, 
					    method_idx_2,
			    __current_expr_parser () -> msg_frame_start)) {
		      arg_val_obj =
			__ctalk_arg_expr_tok (messages, method_msg_ptr,
					      rt_argstrs[0].start_idx,
					      rt_argstrs[0].end_idx,
					      is_arg_expr);
		      goto arg_reevaled_1;
		    } else {
 		      EVAL_SINGLE_ARG_TOK
		      goto arg_reevaled_1;
		    }
		  } else {
		    /*
		     *  Next token is not a method.
		     *  TO DO - check for instance and
		     *  class variable messages.
		     */
 		    EVAL_SINGLE_ARG_TOK
		    goto arg_reevaled_1;
		  }
		} else {
		  /* No next token. */
		  EVAL_SINGLE_ARG_TOK
		  goto arg_reevaled_1;
		}
	      } else {
	      /*  Method matches only the primary receiver. */
 	      EVAL_SINGLE_ARG_TOK
 	      goto arg_reevaled_1;
 	    }
 	  } else {
	    /*
	     *  Instance and class variable messages should need 
	     *  a fixup.
	     */
	    if ((messages[next_arg_tok_idx] -> attrs & RT_OBJ_IS_INSTANCE_VAR) ||
		(messages[next_arg_tok_idx] -> attrs & RT_OBJ_IS_CLASS_VAR)) {
	      rt_argstrs[rt_argstrptr].start_idx = 
		arglist_start;
	      rt_argstrs[rt_argstrptr].end_idx = 
		arglist_end;
	      ++rt_argstrptr;
	      arg_val_obj = 
		__ctalk_arg_expr_tok (messages, method_msg_ptr, 
				      rt_argstrs[0].start_idx,
				      rt_argstrs[0].end_idx,
				      is_arg_expr);
	      goto arg_reevaled_1;
	    } else {
	      if (IS_C_UNARY_MATH_OP
		  (M_TOK(messages[rt_argstrs[rt_argstrptr].start_idx]))) {
		arg_val_obj = 
		  __ctalk_arg_expr_tok (messages, method_msg_ptr, 
					rt_argstrs[rt_argstrptr].start_idx,
					rt_argstrs[rt_argstrptr].end_idx,
					is_arg_expr);
		++rt_argstrptr;
		goto arg_reevaled_1;
	      } else {
		/*
		 *  Sanity check.
		 */
		if (first_rcvr_idx == -1) {
		  if (~messages[rt_argstrs[rt_argstrptr].start_idx] -> attrs &
		      RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) {
		    char *s;
		    char s_2[MAXMSG];
		    s = collect_tokens (messages, 
					rt_argstrs[rt_argstrptr].start_idx,
					rt_argstrs[rt_argstrptr].end_idx);
		    sprintf (s_2, "The expression,\n\t\"%s,\" does not seem to have a receiver", s);
		    __xfree (MEMADDR(s));
		    __ctalkExceptionInternal (NULL, ambiguous_operand_x,
					      s_2, 0);
		    __ctalkHandleRunTimeExceptionInternal ();
		    return ERROR;
		  }
		} else {
		  /* None of the above. */
		  EVAL_SINGLE_ARG_TOK
		    goto arg_reevaled_1;
		}
	      }
	    }
 	  }
  	} else {
  	  arg_val_obj =
	    __ctalk_arg_expr_tok (messages, method_msg_ptr,
				  rt_argstrs[0].start_idx,
				  rt_argstrs[0].end_idx,
				  is_arg_expr);
  	  goto arg_reevaled_1;
  	}
      } else {
  	rt_argstrs[0].end_idx = rt_argstrs[0].start_idx;
	if (IS_OBJECT (M_VALUE_OBJ (messages[rt_argstrs[0].start_idx]))) {
	  arg_val_obj = M_VALUE_OBJ (messages[rt_argstrs[0].start_idx]);
	} else {
	  rt_argstrs[0].arg = 
	    strdup (messages[rt_argstrs[0].start_idx] -> name);
	  if ((rt_argstrs[0].start_idx == rt_argstrs[0].end_idx) &&
	      (M_TOK(messages[rt_argstrs[0].start_idx]) == METHODMSGLABEL)) {
	    arg_val_obj = create_object_init_internal
	      (rt_argstrs[0].arg,
	       rt_defclasses -> p_expr_class, ARG_VAR, NULL);
	  } else {
	    arg_val_obj = __ctalkEvalExpr (rt_argstrs[0].arg);
	  }
	  __xfree (MEMADDR(rt_argstrs[0].arg));
	}
	goto arg_reevaled_1;
      }
    }

  arg_reevaled_1:

    __add_arg_object_entry_frame (method, arg_val_obj);
    __ctalk_arg_push_ref (arg_val_obj);

    last_arg_tok_idx = rt_argstrs[0].end_idx;

    rt_argstrs[0].arg = NULL;
    rt_argstrs[0].start_idx = rt_argstrs[0].end_idx = 0;

    return last_arg_tok_idx;
  }


  /*
   *  Multiple arguments.
   */

  for (i = arglist_start, rt_argstrptr = 0, *argbuf = 0, 
	 n_arg_internal_parens = 0, last_tok = 0; 
       i >= arglist_end; i--) {

    m_arg = messages[i];

    switch (m_arg -> tokentype)
      {
      case SEMICOLON:
      case ARGSEPARATOR:
	if (!n_arg_internal_parens) {
	  if (isspace ((int)argbuf[0])) {
	    rt_argstrs[rt_argstrptr].arg = 
	      strdup (skip_leading_whitespace(argbuf));
	  } else {
	    rt_argstrs[rt_argstrptr].arg = strdup (argbuf);
	  }
	  rt_argstrs[rt_argstrptr++].end_idx = 
	    __ctalkPrevLangMsg (messages, i, stack_start);
	  /*
	   *  Set the start index of the next argument here.
	   */
	  rt_argstrs[rt_argstrptr].start_idx = 
	    next_arg_tok (messages, i, stack_end) ;
	  *argbuf = 0;
	  ++m_arg -> evaled;
	  ++m_arg -> output;
	} else {
	  strcatx2 (argbuf, m_arg -> name, NULL);
	}
	last_tok = M_TOK(m_arg);
	break;
      case NEWLINE:
	if (last_tok != SEMICOLON && last_tok != ARGSEPARATOR)
	  strcatx2 (argbuf, m_arg -> name, NULL);
	last_tok = M_TOK(m_arg);
	break;
      case WHITESPACE:
	if (last_tok != SEMICOLON && last_tok != ARGSEPARATOR)
	  strcatx2 (argbuf, m_arg -> name, NULL);
	last_tok = M_TOK(m_arg);
	break;
      case OPENPAREN:
	++n_arg_internal_parens;
	strcatx2 (argbuf, m_arg -> name, NULL);
	last_tok = M_TOK(m_arg);
	break;
      case CLOSEPAREN:
	if (n_arg_internal_parens) {
	  --n_arg_internal_parens;
	  strcatx2 (argbuf, m_arg -> name, NULL);
	}
	last_tok = M_TOK(m_arg);
	break;
      default:
	strcatx2 (argbuf, m_arg -> name, NULL);
	last_tok = M_TOK(m_arg);
	break;
      }

    if (i == arglist_end) {
      if (isspace ((int)argbuf[0])) {
	rt_argstrs[rt_argstrptr].arg = 
	  strdup (skip_leading_whitespace (argbuf));
      } else {
	rt_argstrs[rt_argstrptr].arg = strdup (argbuf);
      }
      rt_argstrs[rt_argstrptr++].end_idx = arglist_end;
    }
  }

  for (i = 0; i < rt_argstrptr; i++) {
    /* This can be speeded up. */
    if ((rt_argstrs[i].start_idx != rt_argstrs[i].end_idx) ||
	!(messages[rt_argstrs[i].start_idx] -> attrs & 
	  RT_TOK_OBJ_IS_CREATED_PARAM)) {

      if (!((rt_argstrs[i].start_idx == rt_argstrs[i].end_idx) &&
	    messages[rt_argstrs[i].start_idx]->attrs & 
	    RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS)) {

	arg_val_obj = __ctalk_arg_expr (messages, method_msg_ptr, 
					rt_argstrs[i].arg,
					rt_argstrs[i].start_idx,
					rt_argstrs[i].end_idx,
					is_arg_expr);
	for (j = rt_argstrs[i].start_idx; j >= rt_argstrs[i].end_idx; j--) {
	  if (M_VALUE_OBJ (messages[j]) != arg_val_obj) {
	    /* this needs to be the value object, so it doesn't overwrite
	       the tmp parameter created at this parser level. */
	    /***/
	    if (IS_OBJECT(arg_val_obj)) {
	      messages[j] -> value_obj = arg_val_obj;
	      if (arg_val_obj -> scope & SUBEXPR_CREATED_RESULT) {
		messages[j] -> attrs |= RT_TOK_OBJ_IS_CREATED_PARAM;
	      }
	      if (arg_val_obj -> scope == CREATED_CVAR_SCOPE)
		messages[j] -> attrs |= RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS;
	    }
	  }
	}

	if (arg_val_obj) { /***/
	  __add_arg_object_entry_frame (method, arg_val_obj);
	}
	  

      } else { /* ... 	    RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS))  */

	__add_arg_object_entry_frame 
	  (method, 
	   messages[rt_argstrs[i].start_idx]->obj);

      } /* ... 	    RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS))  */

    } else { /* if ((rt_argstrs[i].start_idx != rt_argstrs[i].end_idx) || */
      arg_val_obj = messages[rt_argstrs[i].start_idx] -> obj;
      if (M_VALUE_OBJ (messages[rt_argstrs[i].start_idx]) != arg_val_obj) {
	messages[rt_argstrs[i].start_idx] -> value_obj = arg_val_obj;
      }
      __add_arg_object_entry_frame (method, arg_val_obj);
#if 0

      /* this needs to be outside of the first loop, so it
	 doesn't change the argument stack when we evaluate 
	 each of the method's arguments. */
      __ctalk_arg_push_ref (arg_val_obj);
#endif
    }
  }

  for (i = 0; (i < rt_argstrptr) && method -> args[i]; i++) {
    OBJECT *__arg_obj = method -> args[i + arg_frame_for_push] -> obj;
    if (__ctalk_arg_push_ref (__arg_obj) != ERROR) {
      /*
       *  If an instance variable of a publicly visible object, 
       *  then the entire object needs to have the same reference
       *  count.
       */
      if (__arg_obj -> __o_p_obj && 
	  __arg_obj -> __o_p_obj -> __o_vartags &&
	  (__arg_obj -> __o_p_obj -> __o_vartags -> tag)) {
	(void)__objRefCntSet (OBJREF(__arg_obj -> __o_p_obj), 
			__arg_obj -> nrefs);
      }
    }
  }

  last_arg_tok_idx = rt_argstrs[rt_argstrptr-1].end_idx;

  while (--rt_argstrptr >= 0) {
    __xfree (MEMADDR(rt_argstrs[rt_argstrptr].arg));
    rt_argstrs[rt_argstrptr].arg = NULL;
    rt_argstrs[rt_argstrptr].start_idx = 
      rt_argstrs[rt_argstrptr].end_idx = 0;
  }

  return last_arg_tok_idx;
}

static bool arg_separator_count (MESSAGE_STACK messages,
				 int start_idx,  int end_idx,
				 int n_args_expected) {
  int i, n_argseparators = 0;
  for (i = start_idx; i >= end_idx; i--) {
    if (M_TOK(messages[i]) == ARGSEPARATOR)
      ++n_argseparators;
  }
  if (n_argseparators == (n_args_expected - 1))
    return true;
  else
    return false;
}

static int __rt_arglist_limit (METHOD *method,
			       MESSAGE_STACK messages, int stack_start,
			       int stack_end,
			       int primary_method_idx,
			       int arglist_start, int n_args,
			       int *is_empty) {

   MESSAGE *m, *m_tok_orig;
   int i, i_2,
     end_list,
     n_parens,
     lookahead,
     start_token,
     n_arg_separators,
     separator_paren_level,
     close_paren_idx;
   EXPR_PARSER *p;
   METHOD *arg_method;

   if (!n_args && _is_empty_arglist (messages, arglist_start, stack_start)) {
     *is_empty = TRUE;
     return arglist_start;
   } else {
     *is_empty = FALSE;
   }

   /* Here also, do a check for a simple argument list enclosed in
      parentheses.  See the comment in __rt_method_arglist_n_args (),
      below. */
   
   if (M_TOK(messages[arglist_start]) == OPENPAREN) {
     if ((close_paren_idx = __ctalkMatchParen (messages, arglist_start,
					       stack_end)) != ERROR) {
       if ((lookahead = __ctalkNextLangMsg (messages, close_paren_idx,
					    stack_end)) != ERROR) {
	 if ((M_TOK(messages[lookahead]) == SEMICOLON) ||
	     (M_TOK(messages[lookahead]) == CLOSEPAREN) ||
	     (M_TOK(messages[lookahead]) == COLON) ||
	     (M_TOK(messages[lookahead]) == CLOSEBLOCK)) {
	   if (arg_separator_count (messages, arglist_start, close_paren_idx,
				    n_args)) {
	     return close_paren_idx;
	   } else {
	     goto multiple_args;
	   }
	 } else if (M_TOK(messages[lookahead]) == ARGSEPARATOR) {
	   goto multiple_args;
	 } else if (IS_C_RELATIONAL_OP(M_TOK(messages[lookahead]))) {
	   if (arg_separator_count (messages, arglist_start, close_paren_idx,
				    n_args)) {
	     return close_paren_idx;
	   } else {
	     goto multiple_args;
	   }
	 } else if (M_TOK(messages[lookahead]) == METHODMSGLABEL) {
	   if (arg_separator_count (messages, arglist_start, close_paren_idx,
				    n_args)) {
	     return close_paren_idx;
	   } else {
	     goto multiple_args;
	   }
	 } else if ((M_TOK(messages[lookahead]) == INCREMENT) ||
		    (M_TOK(messages[lookahead]) == DECREMENT)) {
	   if (arg_separator_count (messages, arglist_start, close_paren_idx,
				    n_args)) {
	     return lookahead;
	   } else {
	     goto multiple_args;
	   }
	 } else if (M_TOK(messages[lookahead]) == LABEL) { /***/
	   if (arg_separator_count (messages, arglist_start, close_paren_idx,
				    n_args)) {
	     return lookahead;
	   } else {
	     goto multiple_args;
	   }
	 } else if (IS_C_BINARY_MATH_OP(M_TOK(messages[lookahead]))) {
	   if (arg_separator_count (messages, arglist_start, close_paren_idx,
				    n_args)) {
	     return close_paren_idx;
	   } else {
	     goto multiple_args;
	   }
	 } else if (messages[arglist_start] -> attrs &
		    RT_TOK_IS_TYPECAST_EXPR) {
	   for (i = close_paren_idx - 1; i > stack_end; i--) {
	     if (M_TOK(messages[i]) == ARGSEPARATOR)
	       goto multiple_args;
	     if ((M_TOK(messages[i]) == SEMICOLON) ||
		 (M_TOK(messages[i]) == CLOSEPAREN) ||
		 (M_TOK(messages[i]) == COLON) ||
		 (M_TOK(messages[i]) == CLOSEBLOCK)) {
	       if (arg_separator_count (messages, arglist_start,
					i, n_args)) {
		 return i;
	       }
	     }
	   }
	   if (i == stack_end)
	     return i + 1;  /* end of expression tokens */
	 } else {
	   /* if we have another argument separator before another
	      opening parenthesis,  then there are more arguments. */
	   for (i = lookahead - 1; i > stack_end; i--) {
	     if (M_TOK(messages[i]) == OPENPAREN)
	       break;
	     if (M_TOK(messages[i]) == ARGSEPARATOR)
	       goto multiple_args;
	   }
	   return close_paren_idx;
	 }
       }
     }
   }

 multiple_args:
   n_arg_separators = 0;
   for (i = arglist_start, n_parens = 0, end_list = -1, start_token = -1,
	  separator_paren_level = -1;
        (i > stack_end) && (end_list == -1); i--) {

     /*
      *  With the expression in its own frame, if we reach the end of the
      *  stack, return the last message.
      */
     if (i == stack_end + 1) {
       /* and check for trailing whitespace */
       while (M_ISSPACE(messages[i]))
	 ++i;
       return i;
     }

     m = messages[i];

     if ((m -> tokentype == NEWLINE) ||
	 (m -> tokentype == WHITESPACE))
       continue;

     if (start_token == -1)
       start_token = m -> tokentype;

     switch (m -> tokentype)
       {
       case LABEL:
	 if (m->attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL)
	   return i;
	 break;
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
	     if ((lookahead =
		  next_arg_tok (messages, i, stack_end)) == ERROR) {
	       /* Check for trailing whitespace. */
	       int i_1;
	       end_list = -1;
	       for (i_1 = i - 1; i_1 > stack_end; i_1--) {
		 if (!M_ISSPACE(messages[i_1])) {
		   _warning ("Argument syntax error.");
		   end_list = arglist_start;
		 }
	       }
	       if (end_list == -1)
		 return i;
	       else
		 return end_list;
	     } else {
	       if (IS_C_OP_TOKEN_NOEVAL(M_TOK(messages[lookahead]))) {
		 /*
		  *  Check for an individual argument wrapped in 
		  *  parentheses.  If there are further arguments,
		  *  then we won't have reached the end of the argument
		  *  list.ppppp
		  */
		 if ((M_TOK(messages[lookahead]) == ARGSEPARATOR) &&
		     !n_parens) {
		   if ((n_arg_separators == (n_args - 1)) &&
		       (n_parens < separator_paren_level)) {
		     end_list = i;
		   }
		 }
	       } else if (M_TOK(messages[lookahead]) == METHODMSGLABEL) {
		 continue;
	       }
	     }
	   }
	 } else {
	  /*
	   *  If we encounter a closing parenthesis without an opening
	   *  parenthesis, then the argument list ends at the previous
	   *  token.  We determine whether the label is a method or
	   *  instance variable later in eval_subexpr.
	   */
	   if (!n_parens)
	     end_list = i + 1;
	 }
	 break;
       case METHODMSGLABEL:
	 if (n_parens == 0) {
	   /* If the expression isn't grouped, make sure the
	      method can bind to the previous label or constant
	      for the method to be considered part of the
	      argument. */
	   p = __current_expr_parser ();
	   for (i_2 = i + 1; i_2 <= p -> msg_frame_start; i_2++) {
	     if (!M_ISSPACE(messages[i_2]))
	       break;
	   }
	   if (M_TOK(messages[i_2]) == CLOSEPAREN) {
	     /* We've already checked the number of commas within
		the parens, so there are more commas following. */
	     if (n_arg_separators < (n_args - 1)) {
	       continue;
	     }
	   } else {
	     /* If the method binds to a previous method in the
		expression, then the argument list ends at
		the previous token (methods without params aren't
		checked here */
	     if (messages[i] -> receiver_msg != messages[i_2])
	       end_list = i_2;
	   }
	 }
	 break;
       case SEMICOLON:
	 end_list = i + 1;
	 break;
       case ARGSEPARATOR:
	 ++n_arg_separators;
	 if ((n_arg_separators == 1) && (separator_paren_level == -1))
	   separator_paren_level = n_parens;
	 break;
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
       case MODULUS:
       case CONDITIONAL:     /* No-op for now. */
       case SIZEOF:
       case INCREMENT:
       case DECREMENT:
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
	 /*
	  *  Any operator not within a subexpression should
	  *  end the argument list.  eval_expr () deals with
	  *  specific operator precedences.
	  *
	  *  The function must re-lex the primary method token in 
	  *  case a caller has changed it to METHODMSGLABEL.
	  *
	  *  Does not handle a case where an operator follows a 
	  *  method label.  __rt_method_args adjusts the argument
	  *  list limit using the primary method.
	  */
	 if (!n_parens) {
	   if (IS_C_OP_CHAR(messages[primary_method_idx] -> name[0])) {
	     end_list = 
	       __ctalkPrevLangMsg (messages, i, stack_start);
  	     messages[end_list] -> attrs |=
	       RT_TOK_OBJ_IS_ARG_W_FOLLOWING_EXPR;
	   } else {
	     /* check if the previous tokens could be the
		receiver of this method */
	     int prev_tok = __ctalkPrevLangMsg (messages, i, stack_start);
	     if (IS_OBJECT(messages[prev_tok] -> obj)) {
	       OBJECT *rcvr_ptr = messages[prev_tok] -> obj;
	       if (rcvr_ptr -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
		 /*
		  *  DTRT for an expression like
		  *
		  *    *<Symbol_object>++
		  *
		  *   Where ++ is the higher precedence op,
		  *   but for arglist checking it only needs
		  *   to be considered as part of the complete
		  *   expression.
		  */
		 int prev_tok_2 = prev_op_tok (messages, prev_tok,
						      stack_start);
		 if (M_TOK(messages[prev_tok_2]) == MULT) {
		   /* 
		    *   unref the actual receiver
		    */
		   OBJECT *rcvr_ptr_ptr = *(OBJECT **)rcvr_ptr -> __o_value;
		   if ((arg_method = __ctalkFindMethodByName
			(&rcvr_ptr_ptr,
			 M_NAME(messages[i]),
			 FALSE, ANY_ARGS)) == NULL) {
		     end_list = prev_tok;
		     messages[end_list] -> attrs |=
		       RT_TOK_OBJ_IS_ARG_W_FOLLOWING_EXPR;
		   }
		 } else { /* if (M_TOK(messages[prev_tok_2]) == MULT) */
		   if ((arg_method =
			__ctalkFindMethodByName
			(&rcvr_ptr,
			 M_NAME(messages[i]),
			 FALSE, ANY_ARGS)) == NULL) {
		     end_list = prev_tok;
		     messages[end_list] -> attrs |=
		       RT_TOK_OBJ_IS_ARG_W_FOLLOWING_EXPR;
		   }
		 } /* if (M_TOK(messages[prev_tok_2]) == MULT) */
	       } else {
		 if ((arg_method =
		      __ctalkFindMethodByName
		      (&rcvr_ptr,
		       M_NAME(messages[i]),
		       FALSE, ANY_ARGS)) == NULL) {
		   end_list = prev_tok;
		   messages[end_list] -> attrs |=
		     RT_TOK_OBJ_IS_ARG_W_FOLLOWING_EXPR;
		 } else {
		   if (IS_C_RELATIONAL_OP(M_TOK(messages[i])) &&
		       (n_parens == 0)) {
		     if (n_arg_separators == (method -> n_params - 1)) {
		       end_list = prev_tok;
		       messages[end_list] -> attrs |=
			 RT_TOK_OBJ_IS_ARG_W_FOLLOWING_EXPR;
		     }
		   }
		 }
	       }
	     }
	   }
	 }
	 /*
	  *  Any operator not within a subexpression should
	  *  end the argument list.  eval_expr () deals with
	  *  specific operator precedences.
	  */
       default:
	 break;
       }
   }

   return end_list;
}

static inline OBJECT *arg_object_if_label (MESSAGE *m_tok) {
  OBJECT *o;
  if (M_TOK(m_tok) == LABEL) {
    if (IS_OBJECT(m_tok -> obj) && IS_CLASS_OBJECT (m_tok -> obj)) {
      /* 
	 In case we have a construct like: 
	 
	   self = CFunction cTime ...

	 This might need to be worked out more.
      */
      return M_VALUE_OBJ(m_tok);
    } else {
      if ((o = __ctalk_get_arg_tok (M_NAME(m_tok))) != NULL)
	return o;
    }
  }
  return NULL;
}

/*
 *   Call eval_expr () from within an argument.  The receiver
 *   is the object immediately before the method whose arguments
 *   we're evaluating, and the method argument to eval_expr ()
 *   is NULL.
 */

static OBJECT *__ctalk_arg_expr (MESSAGE_STACK messages, int method_ptr, 
				 char *expr, int arg_start_idx,
				 int arg_end_idx,
				 int is_arg_expr) {
  int rcvr_ptr,
    stack_start,
    i;
  OBJECT *rcvr_obj,
    *rcvr_class_obj,
    *result_obj = NULL;

#if 0 /***/
  if ((messages[arg_start_idx] -> evaled > 1) &&
      IS_OBJECT (messages[arg_start_idx] -> value_obj))
    return messages[arg_start_idx] -> value_obj;
#endif  

  stack_start = __rt_get_stack_top (messages);

  if ((rcvr_ptr = 
       __ctalkPrevLangMsg (messages, method_ptr, stack_start)) == ERROR)
    _error ("Parser error.");

  rcvr_obj = M_VALUE_OBJ (messages[rcvr_ptr]);
  if (!IS_OBJECT (rcvr_obj))
    _error ("Parser error.");

  if (IS_CLASS_OBJECT(rcvr_obj)) {
    rcvr_class_obj = rcvr_obj;
  } else {
    rcvr_class_obj = rcvr_obj -> __o_class;
  }
  if (!IS_OBJECT (rcvr_class_obj))
    _error ("Parser error.");
    
  if (__arg_typecast_expr_2 (messages, arg_start_idx, arg_end_idx)) {
    result_obj = M_VALUE_OBJ(messages[arg_start_idx]);
  } else {

    if (arg_start_idx == arg_end_idx) {
      if (messages[arg_start_idx] -> attrs & RT_TOK_IS_SELF_KEYWORD) {
	result_obj = resolve_self (is_constant_rcvr, is_arg_expr);
      } else if ((result_obj = arg_object_if_label (messages[arg_start_idx]))
		 == NULL) {
	__ctalk_receiver_push_ref (rcvr_obj);
	result_obj = eval_expr (expr, rcvr_class_obj, NULL, NULL, 
				rcvr_obj -> scope & ~METHOD_USER_OBJECT,
				is_arg_expr);
	__ctalk_receiver_pop_deref ();
      }
    } else {
      if (messages[arg_end_idx] -> obj &&
	  messages[arg_end_idx] -> obj -> attrs &
	  OBJECT_REF_IS_CVAR_PTR_TGT) {
	OBJECT *__r;
	/* A special case, where the argument expression is
	  
	     *<var>

	   which is normally the case when there is a cvartab
	   entry; i.e., a CVAR in an arg block. In this simple
	   case, we just extract the referenced object, and
	   use that as the result, which means we can delete
	   the Symbol object that references it.
	*/
	__r = obj_ref_str (messages[arg_end_idx] -> obj -> __o_value);
	if (IS_OBJECT(__r)) {
	  __ctalkSetObjectScope (__r, __r -> scope & ~VAR_REF_OBJECT);
	  __objRefCntSet (OBJREF(__r), 1);
	  messages[arg_end_idx] -> obj -> __o_value[0] = '\0';
	  messages[arg_end_idx] -> obj -> instancevars -> __o_value[0] = '\0';
	  __objRefCntZero (OBJREF(messages[arg_end_idx] -> obj));
	  __ctalkDeleteObject (messages[arg_end_idx] -> obj);
	  messages[arg_end_idx] -> obj = NULL;
	  messages[arg_start_idx] -> value_obj = __r;
	  messages[arg_end_idx] -> value_obj = __r;
	  return __r;
	}
      } 
      __ctalk_receiver_push_ref (rcvr_obj);
      result_obj = eval_expr (expr, rcvr_class_obj, NULL, NULL, 
			      rcvr_obj -> scope & ~METHOD_USER_OBJECT,
			      is_arg_expr);
      __ctalk_receiver_pop_deref ();
    }
  }

  for (i = arg_start_idx; i >= arg_end_idx; --i)
    messages[i] -> evaled += 2;

  return result_obj;
}

static OBJECT *__ctalk_arg_expr_tok (MESSAGE_STACK messages, int method_ptr, 
				     int arg_start_idx,
				     int arg_end_idx,
				     int is_arg_expr) {
  int i;
  char toks[MAXMSG];
  *toks = 0;
  for (i = arg_start_idx; i >= arg_end_idx; --i)
    strcatx2(toks, M_NAME(messages[i]), NULL);
  return __ctalk_arg_expr (messages, method_ptr, toks, arg_start_idx,
			   arg_end_idx, is_arg_expr);
}


ARG *__ctalkCreateArgEntry (void) {
  ARG *a;
  a = __xalloc (sizeof (struct _arg));
  a -> sig = ARG_SIG;
  return a;
}

ARG *__ctalkCreateArgEntryInit (OBJECT *o) {
  ARG *a;
  a = __ctalkCreateArgEntry ();
  a -> obj = o;
  return a;
}

void __ctalkDeleteArgEntry (ARG *a) {
  __xfree (MEMADDR(a));
}

int __add_arg_object_entry_frame (METHOD *m, OBJECT *arg) {
  ARG *a;
  if (IS_OBJECT(arg)) {
    a = __ctalkCreateArgEntryInit (arg);
    (void)__objRefCntInc (OBJREF(arg));
  } else {
    a = __ctalkCreateArgEntry ();
  }
  a -> call_stack_frame = __call_stack_ptr + 1;
  m -> args[m -> n_args++] = a;
  return SUCCESS;
}

void __argstack_error_cleanup (void) {
  while (++__ctalk_arg_ptr <= MAXARGS) {
    if (__ctalk_argstack[__ctalk_arg_ptr]) {
      __ctalkDeleteObjectInternal (__ctalk_argstack[__ctalk_arg_ptr]);
      __ctalk_argstack[__ctalk_arg_ptr] = NULL;
    }
  }
}

static int __rt_method_arg_limit_2 (MESSAGE_STACK messages, int arg_start,
				    int stack_end_idx) {

  int n_parens, n_subscripts, i, i_1, lookahead, lookahead_1,
    prev_lookahead;
  int arg_arglist_limit;
  int arg_close_paren_idx;

  n_parens = n_subscripts = i = stack_end_idx = 0;

  switch (M_TOK(messages[arg_start]))
    {
    case OPENPAREN:

      if ((arg_close_paren_idx = 
	   __ctalkMatchParen (messages, arg_start, stack_end_idx))
	  == ERROR)
	return ERROR;

      for (lookahead = arg_close_paren_idx - 1;
	   lookahead > stack_end_idx; --lookahead) {
	if (!messages[lookahead])
	  return arg_close_paren_idx;
	if (!M_ISSPACE(messages[lookahead]))
	  break;
      }

      /*
       *  If we don't find an argument terminator immediately
       *  after the expression's closing parenthesis, scan
       *  forward until we do.
       */
      switch (M_TOK(messages[lookahead]))
	{
	case CLOSEPAREN:
	case SEMICOLON:
	case ARGSEPARATOR:
	  return arg_close_paren_idx;
	  /* break; */   /* Not reached. */
	case LABEL:
	  /* look for an argseparator after a label or
	     a series of labels that follow a set of
	     parentheses - there should be checks for 
	     other tokens in the future. */
	  prev_lookahead = lookahead;
	  while (lookahead > stack_end_idx) {
	    if ((lookahead = next_arg_tok (messages, lookahead,
					   stack_end_idx)) != ERROR) {
	      if (M_TOK(messages[lookahead]) == ARGSEPARATOR) {
		return prev_lookahead;
	      } else  if (M_TOK(messages[lookahead]) == LABEL) {
		prev_lookahead = lookahead;
	      } else if (M_TOK(messages[lookahead]) == SEMICOLON) {
		return prev_lookahead;
	      }
	    }
	  }
	  break;
	default:

	  if (IS_C_BINARY_MATH_OP(M_TOK(messages[lookahead]))) {
	    if (!n_parens) {

	      int prev_lookahead = lookahead;

 	      if ((lookahead = next_arg_tok 
		   (messages, prev_lookahead, stack_end_idx)) == ERROR)
 		return prev_lookahead;

	      /* If it's a math op, then it's not the first method
		 in the expression, so return the end of the expression,
		 and eval_expr will handle the args in the math phases.
	      */
	      return stack_end_idx;

	    }
	  } else {
	    for (; i > stack_end_idx; i--) {

 	      if ((lookahead = next_arg_tok (messages, i, stack_end_idx)) == ERROR)
 		return ERROR;

	      switch (M_TOK(messages[lookahead]))
		{
		case CLOSEPAREN:
		case SEMICOLON:
		case ARGSEPARATOR:
		  return i;
		  /* break; */  /* Not reached. */
		}
	    }
	  }
	}
      break;
    default:
      /*
       *  Arguments that don't begin with a parenthesis.
       */

      for (i = arg_start; i > stack_end_idx; i--) {

	if (!messages[i])
	  break;

	if (M_ISSPACE(messages[i]))
	  continue;

	for (lookahead = i - 1; lookahead > stack_end_idx; --lookahead) {
	  if (!messages[lookahead])
	    return i;
	  if (!M_ISSPACE(messages[lookahead]))
	    break;
	}
	
	switch (M_TOK(messages[i]))
	  {
	  case LABEL:
	  case METHODMSGLABEL:

	    /* Assumes we're starting at the beginning of an 
	       argument that is not completely enclosed by
	       parentheses. */

	    for (i_1 = i; i_1 > stack_end_idx; i_1--) {

	      if (!messages[i_1])
		break;

	      if (M_ISSPACE(messages[i_1]))
		continue;

	      for (lookahead_1 = i_1 - 1; lookahead_1 > stack_end_idx; --lookahead_1) {
		if (!messages[lookahead_1])
		  return i_1;
		if (!M_ISSPACE(messages[lookahead_1]))
		  break;
	      }

	      switch (M_TOK(messages[lookahead_1]))
		{
		case OPENPAREN:
		  ++n_parens;
		  break;
		case CLOSEPAREN:
		  if (!n_parens)
		    return i_1;
		  --n_parens;
		  break;
		case ARGSEPARATOR:
		  if (!n_parens)
		    return i_1;
		  break;
		case SEMICOLON:
		  return i_1;
		  break;
		}

	      arg_arglist_limit = i_1;
	    }

	  case OPENPAREN:
	    ++n_parens;
	    break;
	  case ARRAYOPEN:
	    ++n_subscripts;
	    break;
	  case ARRAYCLOSE:
	    if (--n_subscripts < 0) return i;
	    break;
	  case CLOSEPAREN:
	    if (--n_parens < 0) return i;
	    break;
	  }
	switch (M_TOK(messages[lookahead])) 
	  {
	  case ARGSEPARATOR:
	    if (!n_parens) return i;
	    break;
	  case CLOSEPAREN:
	    if (!n_parens) return i;
	    break;
	  case ARRAYCLOSE:
	    if (!n_subscripts) return i;
	    break;
	  case SEMICOLON:
	    return i;
	    /* 	    break; */  /* Not reached. */
	  }
      }
      break;
    }
  return ERROR;
}

/*
 * Check for a comma preceding the receiver.
 * Doesn't look count parens within the arglist.
 */
static int __rt_is_comma_before_receiver (MESSAGE_STACK messages, 
				  int method_msg_idx, int stack_start_idx) {
  int i, i_prev;
#if 0
  stack_start_idx = stack_start (messages);
#endif

  if (messages[method_msg_idx] -> receiver_msg) {
    for (i = method_msg_idx; i <= stack_start_idx; i++) 
      if (messages[i] == messages[method_msg_idx] -> receiver_msg)
	break;

    if ((i_prev = __ctalkPrevLangMsg 
	 (messages, i, stack_start_idx)) != ERROR) {
      if (M_TOK(messages[i_prev]) == ARGSEPARATOR) {
	return TRUE;
      } else {
	return FALSE;
      }
    } else {
      return FALSE;
    }

  } else {
    return FALSE;
  }
}

/*
 *  Handle cases like this:
 *
 *    tokenList push (String basicNew "token", tokenbuf);
 *    tokenList push String basicNew "token", tokenbuf;
 *
 *  I.e., make sure that the argument list associates
 *  with "basicNew" and not "push".  This is easy
 *  enough so far if we make it all of the way to
 *  the closing paren of the arglist, or to the end
 *  of the message stack if there are no parens (and
 *  arglist_end == -1 in this case because we didn't
 *  previously scan for it in the calling fn).
 */
static int arglist_internal_method_expr (EXPR_PARSER *p,
					 int rcvr_tok_idx,
					 int arglist_end,
					 METHOD *pri_method) {
  OBJECT *arg_obj;
  METHOD *arg_method;
  int i_2, arg_arglist_end, n_arg_parens, n_arg_method_args,
    lookahead;

  if ((lookahead = next_arg_tok_b (p, rcvr_tok_idx)) == ERROR)
    return ERROR;
  if (M_TOK(p -> m_s[lookahead]) == LABEL) {
    if ((arg_obj = 
	 __ctalk_get_object (M_NAME(p -> m_s[rcvr_tok_idx]),
			     NULL)) != NULL) {
      p -> m_s[rcvr_tok_idx] -> obj = arg_obj;
      if (__ctalk_isMethod_2 (M_NAME(p -> m_s[lookahead]),
			      p -> m_s, lookahead,
			      p -> msg_frame_start)) {
	if (arglist_end == -1) {
	  for (i_2 = lookahead - 1, n_arg_parens = 0,
		 n_arg_method_args = 1;
	       (i_2 > p -> msg_frame_top) && (n_arg_parens >= 0);
	       --i_2) {
	    if (M_TOK(p -> m_s[i_2]) == SEMICOLON)
	      break;
	    switch (M_TOK(p -> m_s[i_2]))
	      {
	      case OPENPAREN:
		++n_arg_parens;
		break;
	      case CLOSEPAREN:
		--n_arg_parens;
		break;
	      case ARGSEPARATOR:
		++n_arg_method_args;
		break;
	      }
	  }
	} else {
	  for (i_2 = lookahead - 1, n_arg_parens = 0,
		 n_arg_method_args = 1;
	       (i_2 > arglist_end) && (n_arg_parens >= 0);
	       --i_2) {
	    switch (M_TOK(p -> m_s[i_2]))
	      {
	      case OPENPAREN:
		++n_arg_parens;
		break;
	      case CLOSEPAREN:
		--n_arg_parens;
		break;
	      case ARGSEPARATOR:
		++n_arg_method_args;
		break;
	      }
	  }
	}
      }
      if ((arg_method = __ctalkFindMethodByName
	   (&arg_obj, M_NAME(p -> m_s[lookahead]),
	    FALSE, n_arg_method_args)) != NULL) {
	/* just return the primary method's param count because... */
	if (arglist_end == -1) {
	  /* we reached the end of the expr's stack */
	  if (i_2 == p -> msg_frame_top) {
	    return pri_method -> n_params;
	  }
	} else {
	/* we've checked all the way to the closing
	   paren of the arg method's arglist */
	  if (i_2 == arglist_end) {
	    return pri_method -> n_params;
	  }
	}
      }
    }
  }
  return -1;
}

static int stack_end_chk = 0;

int __rt_method_arglist_n_args (EXPR_PARSER *p, int method_msg_idx,
				METHOD *m) {
  bool paren_delimiters = false;
  int open_paren, close_paren, lookahead, arglist_start = 0, i,
    arglist_end, paren_level, n_args, n_internal_args, n_commas;
  
  if (m -> n_params == 0)
    return 0;

  if ((open_paren = next_arg_tok_b (p, method_msg_idx))
      == ERROR) {
    /* if we reach here, it's probably an error in the input, and should
       be handled at a higher level. */
    return 0;
  }
  if (M_TOK(p -> m_s[open_paren]) == OPENPAREN) {
    close_paren = __ctalkMatchParen (p -> m_s, open_paren, p -> msg_frame_top);

    /* empty arglist */
    if (next_arg_tok_b (p, open_paren) == close_paren)
      return 0;

    if ((lookahead = next_arg_tok_b (p, close_paren)) != ERROR) {
      /* 
	 check for further operators after the matching paren; e.g.,

	   rcvr mthd (arg1 term) - (another arg1 term), arg2, arg3...
                                 ^
	 and don't interpret the opening paren and its match as
	 enclosing the entire argument list.

	 HOWEVER, we have to check for expressions like the
	 following - 

	   if (rcvr mthd (arglist) == 0)

	 to see if there is a complete argument list between the
	 parens anyway, so we do a comma check to see how many args
	 there are between the parens, where there are no nested paren
	 levels.
      */
      if (IS_C_BINARY_MATH_OP(M_TOK(p -> m_s[lookahead]))) {
	for (n_commas = 0, paren_level = 0,
	       i = open_paren - 1; i > close_paren; i--) {
	  if ((M_TOK(p -> m_s[i]) == ARGSEPARATOR) &&
	      (paren_level == 0)) {
	    ++n_commas;
	  } else if (M_TOK(p -> m_s[i]) == OPENPAREN) {
	    ++paren_level;
	  } else if (M_TOK(p -> m_s[i]) == CLOSEPAREN) {
	    ++paren_level;
	  }
	}
	if (n_commas != m -> n_params - 1)
	  goto not_paren_delimiters;
      }
    }
    
    paren_delimiters = true;

    arglist_start = next_arg_tok_b (p, open_paren);
    arglist_end = prev_arg_tok_b (p, close_paren);

    lookahead = next_arg_tok_b (p, close_paren);
    paren_level = 0;
    for (i = lookahead; i >= p -> msg_frame_top; --i) {
      /* Check for an argument list with only the first argument
	 enclosed in parens - the first arg includes its parens */
      if (p -> m_s[i] == NULL) {
	break;
      } else if (M_ISSPACE(p -> m_s[i])) {
	continue;
      } else if (M_TOK(p -> m_s[i]) == ARGSEPARATOR) {
	/* This clause needs to come before 
	   METHOD_ARG_TERM_MSG_TYPE, so we don't just break
	   if we encounter a comma. */
	paren_delimiters = false;
	arglist_start = next_arg_tok_b (p, method_msg_idx);
      } else if (METHOD_ARG_TERM_MSG_TYPE(p -> m_s[i])) {
	break;
      }
    }

  } else {
    arglist_start = n_args;
  }

  if (paren_delimiters) {
    n_args = 1;
    paren_level = 1;
    for (i = arglist_start; i > arglist_end; i--) {
      if (p -> m_s[i] == NULL)
	break;
      switch (M_TOK(p -> m_s[i]))
	{
	case OPENPAREN:
	  ++paren_level;
	  break;
	case CLOSEPAREN:
	  --paren_level;
	  break;
	case ARGSEPARATOR:
	  if (paren_level == 1)
	    ++n_args;
	  break;
	case LABEL:
	  if ((n_internal_args =
	       arglist_internal_method_expr (p, i, arglist_end, m))
	      > 0)
	    return n_internal_args;
#if 0 /***/
	  { /***/
	    /*
	     *  Handle a case like this:
	     *
	     *    tokenList push (String basicNew "token", tokenbuf);
	     *
	     *  I.e., make sure that the argument list associates
	     *  with "basicNew" and not "push".  This is easy
	     *  enough so far if we make it all of the way to
	     *  the closing paren of the arglist.
	     */
	    OBJECT *arg_obj;
	    METHOD *arg_method;
	    int i_2, arg_arglist_end, n_arg_parens, n_arg_method_args;
	    lookahead = next_arg_tok_b (p, i);
	    if (M_TOK(p -> m_s[lookahead]) == LABEL) {
	      if ((arg_obj = 
		   __ctalk_get_object (M_NAME(p -> m_s[i]), NULL)) != NULL) {
		p -> m_s[i] -> obj = arg_obj;
		if (__ctalk_isMethod_2 (M_NAME(p -> m_s[lookahead]),
					p -> m_s, lookahead,
					p -> msg_frame_start)) {

		  for (i_2 = lookahead - 1, n_arg_parens = 0,
			 n_arg_method_args = 1;
		       (i_2 > arglist_end) && (n_arg_parens >= 0);
		       --i_2) {
		    switch (M_TOK(p -> m_s[i_2]))
		      {
		      case OPENPAREN:
			++n_arg_parens;
			break;
		      case CLOSEPAREN:
			--n_arg_parens;
			break;
		      case ARGSEPARATOR:
			++n_arg_method_args;
			break;
		      }
		  }
		}
		if ((arg_method = __ctalkFindMethodByName
		     (&arg_obj, M_NAME(p -> m_s[lookahead]),
		      FALSE, n_arg_method_args)) != NULL) {
		  /* we've checked all the way to the closing
		     paren of the primary method's arglist,
		     so just return the primary method's param
		     count. */
		  if (i_2 == arglist_end) {
		    return m -> n_params;
		  }
		}
	      }
	    }
	  }
	  break;
#endif	  
	}
    }
    return n_args;
  } else {
  not_paren_delimiters:
    paren_level = 0;
    arglist_start = next_arg_tok_b (p, method_msg_idx);
    if (METHOD_ARG_TERM_MSG_TYPE (p -> m_s[arglist_start]))
      return 0;
    n_args = 1;
    for (i = arglist_start; i >= p -> msg_frame_top; i--) {
      if (p -> m_s[i] == NULL)
	return n_args;
      switch (M_TOK(p -> m_s[i]))
	{
	case OPENPAREN:
	  ++paren_level;
	  break;
	case CLOSEPAREN:
	  --paren_level;
	  break;
	case ARGSEPARATOR:
	  if (paren_level == 0) {
	    ++n_args;
	  }
	  break;
	case LABEL:
	  /* without parens, the fn didn't find arglist_end above */
	  if ((n_internal_args = arglist_internal_method_expr
	       (p, i, -1, m)) != ERROR)
	    return n_internal_args;
	  break;
	default:
	  if (METHOD_ARG_TERM_MSG_TYPE (p -> m_s[arglist_start])) {
	    if (paren_level == 0) {
	      return n_args;
	    }
	  }
	  break;
	}
    }
    return n_args;
  }
}

/* Returns 0 if the object's reference count == 
   2 * <number of argument stack entries>. 
   Returns -1 if the reference count > <n stack entries>.
   Returns 1 if the reference count < <n stack entries>.
*/
int cvar_alias_arg_min_refcount_check (OBJECT *cvar_alias_obj) {
  int i;
  int n_entries;
  for (i = __ctalk_arg_ptr, n_entries = 0; i <= MAXARGS; i++) {
    if (__ctalk_argstack[i] == cvar_alias_obj)
      ++n_entries;
  }

  if ((n_entries * 2) == cvar_alias_obj -> nrefs) 
    return 0;
  if ((n_entries * 2) < cvar_alias_obj -> nrefs) 
    return -1;
  if ((n_entries * 2) > cvar_alias_obj -> nrefs) 
    return 1;

  return 0;
}

/* this is used in rt_vmthd.c by init_process () */
void proc_init_argstack (void) {
  int i;
  for (i = __ctalk_arg_ptr + 1; i <= MAXARGS; ++i) {
    __ctalk_argstack[i] = 0;
  }
  __ctalk_arg_ptr = MAXARGS;
}
