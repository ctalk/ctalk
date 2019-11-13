/* $Id: methodgt.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2015-2018 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "classlib.h"
#include "symbol.h"


/*
 *  Should actually be *_ge ... match any method with a parameter list
 *  >= the number of arguments in an expression, so we can do a greedy
 *  argumentlist match in functions like method_arglist_limit_2 ().
 *
 */

extern int method_from_proto; /* Declared in mthdref.c */

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c.   */
extern int new_method_ptr;

extern CLASSLIB *lib_includes[MAXARGS + 1];  /* Declared in class.c.  */
extern int lib_includes_ptr;
extern CLASSLIB *input_declarations[MAXARGS+1];
extern int input_declarations_ptr;

extern MESSAGE *var_messages[N_VAR_MESSAGES + 1];
extern int var_messageptr;

struct proto_set {
  int params;
  bool varargs;
};

static struct proto_set proto_params[MAXARGS] = {{0, }};
static int proto_params_ptr = 0;

#define INPUT_DECLARATION input_declarations[input_declarations_ptr+1]

static int super_method_lookup = FALSE;
static OBJECT *super_orig_rcvr;

int get_proto_params_ptr (void) { return proto_params_ptr; }
int get_nth_proto_param (int n) { return proto_params[n].params; }
bool get_nth_proto_varargs (int n) { return proto_params[n].varargs; }

static int cmp_proto_set_params (const void *i1, const void *i2) {
  struct proto_set *p1 = (struct proto_set *)i1, 
    *p2 = (struct proto_set *)i2;
  if (p1 -> params < p2 -> params) return -1;
  if (p1 -> params > p2 -> params) return 1;
  return 0;
}

static inline int match_method_gt (METHOD *m, char *name, int n_params_wanted) {

#define GET_METHOD_WITH_PARAM_COUNT

#ifdef GET_METHOD_WITH_PARAM_COUNT
  if (n_params_wanted < 0) {

    if (str_eq (m -> name, name) && !m -> prefix)
      return TRUE;

  } else {

    if (m -> varargs) {

      if (str_eq (m -> name, name))
   	return TRUE;

    } else {

      if (str_eq (m -> name, name) && 
   	  (m -> n_params >= n_params_wanted) && 
   	  !m -> prefix)
   	return TRUE;

    }
  }

#else

    if (str_eq (m -> name, name) && !m -> prefix)
      return TRUE;

#endif /* GET_METHOD_WITH_PARAM_COUNT */

  return FALSE;
}

METHOD *get_instance_method_gt (MESSAGE *m_org, OBJECT *o, char *name, 
			     int n_params_wanted, int warn) {

  OBJECT *class;
  METHOD *m, *requested_method = NULL;

  if (!IS_OBJECT(o)) return NULL;
  if (!IS_CLASS_OBJECT (o)) 
    class = class_object_search (o -> CLASSNAME, FALSE);
  else 
    class = o;

  if (!IS_OBJECT(class)) return NULL;

  for (m = class -> instance_methods; m; m = m -> next) {
    if (match_method_gt (m, name, n_params_wanted)) {
      requested_method = m;
      break;
    }

    if (!m -> next)
      break;
  }
  
  if (requested_method)
    return requested_method;

  if (o->__o_superclass) {
    if ((requested_method = 
	 get_instance_method (((m_org) ? m_org : NULL), 
			      o->__o_superclass, name, 
			      n_params_wanted, warn)) != NULL) 
      /*
       *  Here the function must check for a method of the 
       *  same class that is forward declared, but also has
       *  a method of the same name in a superclass.  
       *  Note that this case should not apply to methods
       *  that are part of a superclass lookup, and should
       *  not apply to recursive calls, which should be 
       *  handled above.
       */
      if (is_method_proto (class, name) &&
	  !method_proto_is_output (name) &&
	  !method_from_proto &&
	  !super_method_lookup && 
	  (strcmp (name, new_methods[new_method_ptr+1]->method->name))) {

	/* Create the method from its prototype, then check again. */
	method_from_prototype (name);

	for (m = class -> instance_methods; m; m = m -> next) {
	  if (match_method_gt (m, name, n_params_wanted)) {
	    requested_method = m;
	    break;
	  }

	  if (!m -> next)
	    break;
	}

      }
    return requested_method;
  } else {
    OBJECT *__t;
    if ((__t = 
	 class_object_search (class -> __o_superclassname, FALSE)) 
	!= NULL) {
      if (IS_OBJECT(__t)) {
	if ((requested_method = 
	     get_instance_method (((m_org) ? m_org : NULL), 
				  __t, name, n_params_wanted, warn)) != NULL) 
	  return requested_method;
      }
    }
  }

  /* If the method still isn't found, try to find a prototype, then
     look in the the class library once more. */
  if (requested_method) {
    return requested_method;
  } else {
    library_search (class -> __o_name, FALSE);
    for (m = class -> instance_methods; m; m = m -> next) {
      if (match_method_gt (m, name, n_params_wanted)) {
	requested_method = m;
	break;
      }

      if (!m -> next)
	break;
    }
  }

  if (requested_method) {
    return requested_method;
  } else {
    if (warn) {
      if (super_method_lookup) {
	warning (m_org, "Method %s (Receiver %s, Class %s) not found.\n", 
		 name, 
		 (IS_OBJECT(super_orig_rcvr) ? 
		  super_orig_rcvr -> __o_name : NULLSTR),
		 (IS_OBJECT(super_orig_rcvr) ? 
		  super_orig_rcvr -> CLASSNAME: class -> __o_name));
      } else {
	warning (m_org, "Method %s (Class %s) not found.\n", 
		 name, class -> __o_name);
      }
    }
  }

  return NULL;

}

METHOD *get_class_method_gt (MESSAGE *m_org, OBJECT *o, char *name, 
			  int n_args_wanted, int warn) {

  OBJECT *class;
  METHOD *m, *requested_method = NULL;

  if (!IS_OBJECT(o)) return NULL;

  if (!IS_CLASS_OBJECT(o))
    class = class_object_search (o -> CLASSNAME, FALSE);
  else 
    class = o;

  for (m = class -> class_methods; m; m = m -> next) {
    if (match_method_gt (m, name, n_args_wanted)) {
      requested_method = m;
      break;
    }

    if (!m -> next)
      break;
  }
  
  if (requested_method)
    return requested_method;

  if (o->__o_superclass) {
    if ((requested_method = 
	 get_class_method (m_org, o -> __o_superclass, name, 
			   n_args_wanted, warn)) != NULL) 
      return requested_method;
  } else {
    if ((o->__o_superclass = 
	 class_object_search (class -> __o_superclassname,
			      FALSE)) != NULL) {
      if ((requested_method = 
	   get_class_method (m_org, o -> __o_superclass, 
			     name, n_args_wanted, warn)) != NULL) 
	return requested_method;
    }
  }

  /* If the method still isn't found, try to find it in the class
     library once more. */
  if (requested_method) {
    return requested_method;
  } else {
    library_search (class -> __o_name, FALSE);
    for (m = class -> class_methods; m; m = m -> next) {
      if (match_method_gt (m, name, n_args_wanted)) {
	requested_method = m;
	break;
      }
      if (!m -> next)
	break;
    }
  }

  if (requested_method) {
    return requested_method;
  } else {
    if (warn)
      warning (m_org, "Method %s (Class %s) not found.\n", name, 
	       class -> __o_name);
  }

  return NULL;

}

static int n_params (char *src, int *varargs) {
  int start_idx, end_idx;
  int idx, param_start_idx, param_end_idx;
  int n_params;

  start_idx = stack_start (var_message_stack ());
  end_idx = tokenize_reuse (var_message_push, src);

  param_end_idx = param_start_idx = -1;
  for (idx = start_idx; 
       (idx >= end_idx) & (param_end_idx == -1); 
       idx--) {
    if (M_TOK(var_messages[idx]) == OPENPAREN) {
      param_start_idx = idx;
      param_end_idx = match_paren (var_messages, param_start_idx, end_idx);
    }
  }

  fn_params (var_message_stack (), param_start_idx, param_end_idx);

  if (param_0_is_void ()) {
    *varargs = FALSE;
    n_params = 0;
  } else {
    *varargs = get_vararg_status ();
    n_params = get_param_ptr () + 1;
  }

  REUSE_MESSAGES(var_messages, var_messageptr, start_idx);

  return n_params;
}


static inline void set_proto_param_set (METHOD_PROTO *mp) {
  proto_params[proto_params_ptr].params = mp -> n_params;
  proto_params[proto_params_ptr].varargs = mp -> varargs;
}

int is_method_proto_max_params (char *classname, char *selector) { 

  METHOD_PROTO *m;
  int n_params_max, varargs;
  char instancemethod_key[MAXMSG];
  char classmethod_key[MAXMSG];
  int instancemethod_key_length,
    classmethod_key_length;

  instancemethod_key_length = strcatx 
    (instancemethod_key, classname, "_instance_",
     selector, ":::", NULL);
  classmethod_key_length = strcatx 
    (instancemethod_key, classname, "_class_",
     selector, ":::", NULL);

  n_params_max = 0;
  memset (proto_params, 0, MAXARGS * sizeof (struct proto_set));
  proto_params_ptr = 0;

  if (lib_includes_ptr < MAXARGS) {
    if (!strcmp (C_LIB_INCLUDE->name, classname)) {
      for (m = C_LIB_INCLUDE-> proto; m; m = m -> next) {

	if (!strncmp (m -> src, instancemethod_key, 
		      instancemethod_key_length)) {
	  if (m -> n_params == -1) {
	    m -> n_params = n_params (m -> src, &varargs);
	    m -> varargs = (varargs) ? true : false;
	  }
	  set_proto_param_set (m);
	  if (m -> n_params > n_params_max)
	    n_params_max = m -> n_params;
	  ++proto_params_ptr;
	}

	if (!strncmp (m -> src, classmethod_key, 
		      classmethod_key_length)) {
	  if (m -> n_params == -1) {
	    m -> n_params = n_params (m -> src, &varargs);
	    m -> varargs = (varargs) ? true : false;
	  }
	  set_proto_param_set (m);
	  if (m -> n_params > n_params_max)
	    n_params_max = m -> n_params;
	  ++proto_params_ptr;
	}
      }
    }
  }

  if (input_declarations_ptr < MAXARGS) {
    for (m = INPUT_DECLARATION->proto; m; m = m -> next) {
      if (str_eq (INPUT_DECLARATION -> name, classname)) {

	if (!strncmp (m -> src, instancemethod_key, 
		      instancemethod_key_length)) {
	  if (m -> n_params == -1) {
	    m -> n_params = n_params (m -> src, &varargs);
	    m -> varargs = (varargs) ? true : false;
	  }
	  set_proto_param_set (m);
	  if (m -> n_params > n_params_max)
	    n_params_max = m -> n_params;
	  ++proto_params_ptr;
	}

	if (!strncmp (m -> src, classmethod_key, 
		      classmethod_key_length)) {
	  if (m -> n_params == -1) {
	    m -> n_params = n_params (m -> src, &varargs);
	    m -> varargs = (varargs) ? true : false;
	  }
	  set_proto_param_set (m);
	  if (m -> n_params > n_params_max)
	    n_params_max = m -> n_params;

	  ++proto_params_ptr;
	}
      }
    }
  }

  qsort (proto_params, proto_params_ptr, sizeof(struct proto_set), 
	 cmp_proto_set_params);
  return n_params_max;
}

