/* $Id: rt_cvar.c,v 1.1.1.1 2020/07/26 05:50:11 rkiesling Exp $ */

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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "cvar.h"

CVAR *global_cvars = NULL;
CVAR *local_cvars = NULL;
CVAR *typedefs = NULL;
CVAR *typedefs_ptr = NULL;
CVAR *externs = NULL;
CVAR *externs_ptr = NULL;
CVAR *method_arg_cvars = NULL;
CVAR *method_arg_cvars_head = NULL;
CFUNC *functions = NULL,
  *functions_ptr = NULL;
void _new_cvar (CVARREF_T);

HASHTAB typedefhash = NULL;

extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;
extern RT_INFO rtinfo;

extern int eval_status;  /* declared in rt_expr.c */

extern DEFAULTCLASSCACHE *rt_defclasses;

static inline void longlong_int_conv (CVAR *c, void *var,
				      bool have_array_member) {
  switch (c -> n_derefs - have_array_member)
    {
    case 0:
      c -> val.__type = LONGLONG_T;
    
      if (var != NULL)
	c -> val.__value.__ll = *(long long int *)var;
      else
	c -> val.__value.__ll = 0;
      break;
    case 1:
      c -> val.__type = PTR_T;
    
      if (var != NULL)
	c -> val.__value.__ptr = (long long int *)var;
      else
	c -> val.__value.__ptr = NULL;
      if (c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    default:
      c -> val.__type = LONGLONG_T;
	
      if (var != NULL)
	c -> val.__value.__ll = *(long long int *)var;
      else
	c -> val.__value.__ll = 0;
      if (c -> n_derefs && c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    }
}

static inline void short_int_conv (CVAR *c, void *var,
				   bool have_array_member) {
  switch (c -> n_derefs - have_array_member)
    {
    case 0:
      c -> val.__type = INTEGER_T;
      if (var == NULL) {
	c -> val.__value.__i = 0;
      } else {
	if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	  c -> val.__value.__i = *(unsigned short int *)var;
	} else {
	  c -> val.__value.__i = *(short int *)var;
	}
      }
      break;
    default:
      _warning ("short_int_conv: Dereferenced short ints are not "
		"(yet) supported.\n");
      break;
    }
}

static inline void int_conv (CVAR *c, void *var, bool have_array_member) {
  switch (c -> n_derefs - have_array_member)
    {
    case 0:
      c -> val.__type = INTEGER_T;
    
      if (var != NULL)
	c -> val.__value.__i = *(uintptr_t *)var;
      else
	c -> val.__value.__i = 0;
      break;
    case 1:
      c -> val.__type = PTR_T;
    
      if (var != NULL)
	c -> val.__value.__ptr = (uintptr_t *)var;
      else
	c -> val.__value.__ptr = NULL;
      if (c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    case 2:
      c -> val.__type = PTR_T;
      if (var != NULL)
	c -> val.__value.__ptr = (uintptr_t **)var;
      else
	c -> val.__value.__ptr = NULL;
      if (c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    default:
      c -> val.__type = INTEGER_T;
	
      if (var != NULL)
	c -> val.__value.__i = *(uintptr_t *)var;
      else
	c -> val.__value.__i = 0;
      if (c -> n_derefs && c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    }
}

static inline bool typedef_int_conv (CVAR *c, void *var,
				     bool have_array_member) {

  if (c -> type_attrs == CVAR_TYPE_TYPEDEF) {
    /* this is transitional */
    if (str_eq (c -> type, "int")) {
      if (str_eq (c -> qualifier, "long")) {
	if (str_eq (c -> qualifier2, "long")) {
	  c -> val.__value.__ll = *(long long int *)var;
	  c -> val.__type = LONGLONG_T;
	  return true;
	} else {
	  c -> val.__value.__l = *(long int *)var;
	  c -> val.__type = LONG_T;
	  return true;
	}
      } else {
	c -> val.__value.__i = *(uintptr_t *)var;
	c -> val.__type = INTEGER_T;
	return true;
      }
    }
  } else if (c -> type_attrs & CVAR_TYPE_TYPEDEF) {
    if (c -> type_attrs & CVAR_TYPE_SHORT) {
      short_int_conv (c, var, have_array_member);
      return true;
    } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
      longlong_int_conv (c, var, have_array_member);
      return true;
    } else if ((c -> type_attrs & CVAR_TYPE_INT) ||
	       (c -> type_attrs & CVAR_TYPE_LONG) ||
	       (c -> type_attrs & CVAR_TYPE_SHORT)) {
      int_conv (c, var, have_array_member);
      return true;
    }
  }
  return false;
}


static inline void char_conv (CVAR *c, void *var, bool have_array_member) {
    switch (c -> n_derefs - have_array_member)
      {
	/*
	 *  Promote a character to an int.
	 */
      case 0:
	c -> val.__type = INTEGER_T;
	c -> val.__value.__i = *(char *)var;
	break;
      case 1:
      case 2:
	c -> val.__type = PTR_T;
	c -> val.__value.__ptr = var;
	break;
      }

}

static inline void longdouble_conv (CVAR *c, void *var,
				      bool have_array_member) {
  switch (c -> n_derefs - have_array_member)
    {
    case 0:
      c -> val.__type = LONGDOUBLE_T;
    
#if !(defined(__APPLE__) && defined(__ppc__))
      if (var != NULL)
	c -> val.__value.__ld = *(long double *)var;
      else
	c -> val.__value.__ld = 0;
      break;
#else
      if (var != NULL)
	c -> val.__value.__d = *(double *)var;
      else
	c -> val.__value.__d = 0;
      break;
#endif      
    case 1:
      c -> val.__type = PTR_T;
    
      if (var != NULL)
	c -> val.__value.__ptr = (long double *)var;
      else
	c -> val.__value.__ptr = NULL;
      if (c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    default:
#ifndef __APPLE__
      c -> val.__type = LONGDOUBLE_T;
      if (var != NULL)
	c -> val.__value.__ld = *(long double *)var;
      else
	c -> val.__value.__ld = 0;
#else
      c -> val.__type = DOUBLE_T;
      if (var != NULL)
	c -> val.__value.__d = *(long double *)var;
      else
	c -> val.__value.__d = 0;
#endif      
      if (c -> n_derefs && c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    }
}

static inline void double_conv (CVAR *c, void *var,
				      bool have_array_member) {
  switch (c -> n_derefs - have_array_member)
    {
    case 0:
      c -> val.__type = DOUBLE_T;
      if (var != NULL)
	c -> val.__value.__d = *(double *)var;
      else
	c -> val.__value.__d = 0.0;
      break;
    case 1:
      c -> val.__type = PTR_T;
      if (var != NULL)
	c -> val.__value.__ptr = (double *)var;
      else
	c -> val.__value.__ptr = NULL;
      if (c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    default:
      c -> val.__type = DOUBLE_T;
      if (var != NULL)
	c -> val.__value.__d = *(double *)var;
      else
	c -> val.__value.__d = 0.0;
      if (c -> n_derefs && c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    }
}

static inline void float_conv (CVAR *c, void *var,
			       bool have_array_member) {
  switch (c -> n_derefs - have_array_member)
    {
    case 0:
      c -> val.__type = FLOAT_T;
      if (var != NULL)
	c -> val.__value.__d = *(float *)var;
      else
	c -> val.__value.__d = 0.0;
      break;
    case 1:
      c -> val.__type = PTR_T;
      if (var != NULL)
	c -> val.__value.__ptr = (float *)var;
      else
	c -> val.__value.__ptr = NULL;
      if (c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    default:
      c -> val.__type = FLOAT_T;
      if (var != NULL)
	c -> val.__value.__d = *(float *)var;
      else
	c -> val.__value.__d = 0.0;
      if (c -> n_derefs && c -> initializer_size) {
	c -> val.__deref_ptr = var;
      }
      break;
    }
}

/* These might need to use different htoa prototypes for
   a while.... */
#define CVHTOA(buf,r) {(void)htoa(buf, (uintptr_t)(r));}

static void cvar_print_val (char *s, VAL *val) {
  switch (val -> __type)
      {
      case INTEGER_T:
	ctitoa (val -> __value.__i, s);
	break;
      case UINTEGER_T:
	sprintf (s, "%u", val -> __value.__u);
	break;
      case LONG_T:
	ctitoa (val -> __value.__i, s);
	break;
      case ULONG_T:
	sprintf (s, "%lu", val -> __value.__ul);
	break;
      case LONGLONG_T:
	sprintf (s, "%lld", val -> __value.__ll);
	break;
      case DOUBLE_T:
	sprintf (s, "%f", val -> __value.__d);
	break;
#ifndef __APPLE__
      case LONGDOUBLE_T:
	sprintf (s, "%Lf", val -> __value.__ld);
	break;
#endif
      case PTR_T:
	if (val -> __value.__ptr) {
	  CVHTOA(s,val->__value.__ptr);
	}
	break;
      default:
	_warning ("Unknown type %d in cvar_print_val.\n", val -> __type);
	break;
      }
}

static bool is_abbrev_subscript_reg (char *array_expr, CVAR *c) {
  if (strchr (array_expr, '[') &&
      (c -> initializer_size == 0) &&
      (c -> attrs & CVAR_ATTR_ARRAY_DECL) &&
      (c -> n_derefs > 0)) {
    /* in case the registry's name doesn't include a subscript */
    if (!strncmp (array_expr, c -> name, strlen (c -> name))) {
      return true;
    }
  }
  return false;
}

static bool is_cvartab_entry (CVAR *c) {
  int i;
  for (i = __call_stack_ptr + 1; i <= MAXARGS; ++i) {
    if (IS_METHOD (__call_stack[i] -> method)) {
      if (IS_OBJECT (__call_stack[i] -> rcvr_class_obj)) {
	if (strstr (c -> name, __call_stack[i] -> rcvr_class_obj -> __o_name) &&
	    strstr (c -> name, __call_stack[i] -> method -> name)) {
	  return true;
	}
      }
    } else if (__call_stack[i] -> _rt_fn != NULL) {
      if (strstr (c ->  name, __call_stack[i] -> _rt_fn -> name)) {
	return true;
      }
    }
  }
  return false;
}

static int scope_from_object (void *d_ptr) {
  OBJECT *o = (OBJECT *)d_ptr;

  if (o == NULL || o == (OBJECT *)0xffffffff)
    return 0;

  if (IS_OBJECT (o)) {
    return o -> scope;
  } else {
    return 0;
  }
}

/*
 *  For the prototype, see src/output.c.
 */
int __ctalk_register_c_method_arg (char *decl, char *type, 
				   char *qualifier, char *qualifier2,
				   char *storage_class, char *name, 
				   int type_attrs, int n_derefs, 
				   int initializer_size, int scope, 
				   int attrs,
				   void *var) {
  CVAR *c;
  bool have_array_member;
  int sfo;

  _new_cvar (CVARREF(c));

  strcpy (c -> type, type);
  strcpy (c -> name, name);

  if (strcmp (decl, NULLSTR))
    strcpy (c -> decl, decl);
  if (strcmp (qualifier, NULLSTR))
    strcpy (c -> qualifier, qualifier);
  if (strcmp (qualifier2, NULLSTR))
    strcpy (c -> qualifier2, qualifier2);
#ifdef CVAR_STORAGE_CLASS
  if (strcmp (storage_class, NULLSTR))
     strcpy (c -> storage_class, storage_class);
#endif

  c -> type_attrs = type_attrs;
  c -> n_derefs = n_derefs;
  c -> initializer_size = initializer_size;
  c -> attrs = attrs;
  if (scope & SUBSCRIPT_OBJECT_ALIAS) {
    c -> scope = scope & !SUBSCRIPT_OBJECT_ALIAS;
    have_array_member = 1; 
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    c -> scope = scope;
    have_array_member = 0;
    if ((sfo = scope_from_object (var)) != 0) {
      c -> scope |= sfo;
    }
  } else {
    c -> scope = scope;
    have_array_member = 0;
  }

  if (typedef_int_conv (c, var, have_array_member)) {
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
    longlong_int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_SHORT) {
    short_int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (((c -> type_attrs & CVAR_TYPE_INT) ||
	      (c -> type_attrs & CVAR_TYPE_LONG)) &&
	     !(c -> type_attrs & CVAR_TYPE_DOUBLE)) { /***/
#if 0 /***/
  } else if ((c -> type_attrs & CVAR_TYPE_INT) ||
	     (c -> type_attrs & CVAR_TYPE_LONG)) {
#endif    
    int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_CHAR) {
    char_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  }

  if ((c -> type_attrs & CVAR_TYPE_LONGDOUBLE) ||
      ((c -> type_attrs & CVAR_TYPE_LONG) &&
       (c -> type_attrs & CVAR_TYPE_DOUBLE))){ /***/
#if 0 /***/
  if (c -> type_attrs & CVAR_TYPE_LONGDOUBLE) {
#endif    
    longdouble_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
    double_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_FLOAT) {
    float_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    c -> val.__type = OBJECT_T;
    c -> val.__value.__ptr = (OBJECT *)var;
  } else {
    /*
     *   At the moment we should only need to store a generic
     *   pointer.
     */
    c -> val.__type = PTR_T;
    c -> val.__value.__ptr = var;
  }

    

  add_method_arg_cvar (c);

  return SUCCESS;
}

int __ctalk_register_c_method_arg_b (char *decl, char *type, 
				   char *qualifier, 
				   char *storage_class, char *name, 
				   int type_attrs, int n_derefs, 
				   int initializer_size, int scope, 
				   int attrs,
				   void *var) {
  CVAR *c;
  bool have_array_member;
  int sfo;

  _new_cvar (CVARREF(c));

  strcpy (c -> type, type);
  strcpy (c -> name, name);

  if (strcmp (decl, NULLSTR))
    strcpy (c -> decl, decl);
  if (strcmp (qualifier, NULLSTR))
    strcpy (c -> qualifier, qualifier);
#ifdef CVAR_STORAGE_CLASS
  if (strcmp (storage_class, NULLSTR))
    strcpy (c -> storage_class, storage_class);
#endif
  c -> type_attrs = type_attrs;
  c -> n_derefs = n_derefs;
  c -> initializer_size = initializer_size;
  c -> attrs = attrs;
  if (scope & SUBSCRIPT_OBJECT_ALIAS) {
    c -> scope = scope & !SUBSCRIPT_OBJECT_ALIAS;
    have_array_member = 1; 
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    c -> scope = scope;
    have_array_member = 0;
    if ((sfo = scope_from_object (var)) != 0) {
      c -> scope |= sfo;
    }
  } else {
    c -> scope = scope;
    have_array_member = 0;
  }

  if (typedef_int_conv (c, var, have_array_member)) {
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
    longlong_int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (((c -> type_attrs & CVAR_TYPE_INT) ||
	      (c -> type_attrs & CVAR_TYPE_LONG)) &&
	     !(c -> type_attrs & CVAR_TYPE_DOUBLE)) { /***/
    /* CVAR_TYPE_LONG|CVAR_TYPE_DOUBLE == long double, handled below. */
    int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_CHAR) {
    char_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  }

  if ((c -> type_attrs & CVAR_TYPE_LONGDOUBLE) ||
      ((c -> type_attrs & CVAR_TYPE_LONG) &&
       (c -> type_attrs & CVAR_TYPE_DOUBLE))){ /***/
    longdouble_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
    double_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_FLOAT) {
    float_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    c -> val.__type = OBJECT_T;
    c -> val.__value.__ptr = (OBJECT *)var;
  } else {
    /*
     *   At the moment we should only need to store a generic
     *   pointer.
     */
    c -> val.__type = PTR_T;
    c -> val.__value.__ptr = var;
  }

  add_method_arg_cvar (c);

  return SUCCESS;
}

int __ctalk_register_c_method_arg_c (char *decl, char *type, 
				   char *storage_class, char *name, 
				   int type_attrs, int n_derefs, 
				   int initializer_size, int scope, 
				   int attrs,
				   void *var) {
  CVAR *c;
  bool have_array_member;
  int sfo;

  _new_cvar (CVARREF(c));

  strcpy (c -> type, type);
  strcpy (c -> name, name);

  if (strcmp (decl, NULLSTR))
    strcpy (c -> decl, decl);
#ifdef CVAR_STORAGE_CLASS
  if (strcmp (storage_class, NULLSTR))
    strcpy (c -> storage_class, storage_class);
#endif
  c -> type_attrs = type_attrs;
  c -> n_derefs = n_derefs;
  c -> initializer_size = initializer_size;
  c -> attrs = attrs;
  if (scope & SUBSCRIPT_OBJECT_ALIAS) {
    c -> scope = scope & !SUBSCRIPT_OBJECT_ALIAS;
    have_array_member = 1; 
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    c -> scope = scope;
    have_array_member = 0;
    if ((sfo = scope_from_object (var)) != 0) {
      c -> scope |= sfo;
    }
  } else {
    c -> scope = scope;
    have_array_member = 0;
  }

  if (typedef_int_conv (c, var, have_array_member)) {
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
    longlong_int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (((c -> type_attrs & CVAR_TYPE_INT) ||
	      (c -> type_attrs & CVAR_TYPE_LONG)) &&
	     !(c -> type_attrs & CVAR_TYPE_DOUBLE)) { /***/
#if 0 /***/
  } else if ((c -> type_attrs & CVAR_TYPE_INT) ||
	     (c -> type_attrs & CVAR_TYPE_LONG)) {
#endif    
    int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_CHAR) {
    char_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  }

  if ((c -> type_attrs & CVAR_TYPE_LONGDOUBLE) ||
      ((c -> type_attrs & CVAR_TYPE_LONG) &&
       (c -> type_attrs & CVAR_TYPE_DOUBLE))){ /***/
#if 0 /***/
  if (c -> type_attrs & CVAR_TYPE_LONGDOUBLE) {
#endif    
    longdouble_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
    double_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_FLOAT) {
    float_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    c -> val.__type = OBJECT_T;
    c -> val.__value.__ptr = (OBJECT *)var;
  } else {
    /*
     *   At the moment we should only need to store a generic
     *   pointer.
     */
    c -> val.__type = PTR_T;
    c -> val.__value.__ptr = var;
  }

  add_method_arg_cvar (c);

  return SUCCESS;
}

int __ctalk_register_c_method_arg_d (char *type, 
				   char *storage_class, char *name, 
				   int type_attrs, int n_derefs, 
				   int initializer_size, int scope, 
				   int attrs,
				   void *var) {
  CVAR *c;
  bool have_array_member;
  int sfo;

  _new_cvar (CVARREF(c));

  strcpy (c -> type, type);
  strcpy (c -> name, name);

#ifdef CVAR_STORAGE_CLASS
  if (strcmp (storage_class, NULLSTR))
    strcpy (c -> storage_class, storage_class);
#endif
  c -> type_attrs = type_attrs;
  c -> n_derefs = n_derefs;
  c -> initializer_size = initializer_size;
  c -> attrs = attrs;
  if (scope & SUBSCRIPT_OBJECT_ALIAS) {
    c -> scope = scope & !SUBSCRIPT_OBJECT_ALIAS;
    have_array_member = 1;
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    c -> scope = scope;
    have_array_member = 0;
    if ((sfo = scope_from_object (var)) != 0) {
      c -> scope |= sfo;
    }
  } else {
    c -> scope = scope;
    have_array_member = 0;
  }

  if (typedef_int_conv (c, var, have_array_member)) {
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
    longlong_int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (((c -> type_attrs & CVAR_TYPE_INT) ||
	      (c -> type_attrs & CVAR_TYPE_LONG)) &&
	     !(c -> type_attrs & CVAR_TYPE_DOUBLE)) { /***/
#if 0 /***/
  } else if ((c -> type_attrs & CVAR_TYPE_INT) ||
	     (c -> type_attrs & CVAR_TYPE_LONG)) {
#endif    
    int_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  } else if (c -> type_attrs & CVAR_TYPE_CHAR) {
    char_conv (c, var, have_array_member);
    add_method_arg_cvar (c);
    return SUCCESS;
  }

  if ((c -> type_attrs & CVAR_TYPE_LONGDOUBLE) ||
      ((c -> type_attrs & CVAR_TYPE_LONG) &&
       (c -> type_attrs & CVAR_TYPE_DOUBLE))){ /***/
#if 0 /***/
  if (c -> type_attrs & CVAR_TYPE_LONGDOUBLE) {
#endif    
    longdouble_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
    double_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_FLOAT) {
    float_conv (c, var, have_array_member);
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    c -> val.__type = OBJECT_T;
    c -> val.__value.__ptr = (OBJECT *)var;
  } else {
    /*
     *   At the moment we should only need to store a generic
     *   pointer.
     */
    c -> val.__type = PTR_T;
    c -> val.__value.__ptr = var;
  }

  add_method_arg_cvar (c);

  return SUCCESS;
}

/*
 *  The order of precedence for adding and removing cvars is:
 *  1. Method local cvar.
 *  2. Function local cvar.
 *  3. Global cvar.
 */

CVAR *remove_method_arg_cvar (char *name) {

  CVAR *t = NULL;                /* Avoid a warning. */
  OBJECT *(*method_fn)(void);
  METHOD *method;
  RT_FN *fn;

  if ((method_fn = rtinfo.method_fn) != NULL) {

    if ((method = __ctalkRtGetMethod ()) == NULL) {
      if ((__call_stack_ptr < MAXARGS) &&
	  (__call_stack[__call_stack_ptr+1]->_rt_fn == NULL)) {
	_warning ("Undefined method or function in remove_method_arg_cvar.\n");
	return NULL;
      } else {
	goto remove_cvar_check_fn;
      }
    }

    if (method -> local_cvars) {
      if ((method -> local_cvars -> next == NULL) &&
	  (method -> local_cvars -> prev == NULL)) {
	if (str_eq (method -> local_cvars -> name, name)) {
	  t = method -> local_cvars;
	  method -> local_cvars = NULL;
	  return t;
	}
      } else {
	for (t = method -> local_cvars; t; t = t -> next) {
	  if (str_eq (t -> name, name)) {
	    if (t -> prev)
	      t -> prev -> next = (t -> next ? t -> next : NULL);
	    if (t -> next)
	      t -> next -> prev = (t -> prev ? t -> prev : NULL);
	    return t;
	  }
	}
      }
    }
  }

 remove_cvar_check_fn:
  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    if (fn -> local_cvars) {
      if ((fn -> local_cvars -> next == NULL) &&
	  (fn -> local_cvars -> prev == NULL)) {
	if (str_eq (fn -> local_cvars -> name, name)) {
	  t = fn -> local_cvars;
	  fn -> local_cvars = NULL;
	  return t;
	}
      } else {
	for (t = fn -> local_cvars; t; t = t -> next) {
	  if (str_eq (t -> name, name)) {
	    if (t -> prev)
	      t -> prev -> next = (t -> next ? t -> next : NULL);
	    if (t -> next)
	      t -> next -> prev = (t -> prev ? t -> prev : NULL);
	    return t;
	  }
	}
      }
    }
    return NULL;
  }


  if (method_arg_cvars) {

    if ((method_arg_cvars -> next == NULL) &&
	(method_arg_cvars -> prev == NULL)) {
      if (str_eq (name, t -> name)) {
	t = method_arg_cvars;
	method_arg_cvars = NULL;
	return t;
      } else {
	return NULL;
      }
    } else {
      for (t = method_arg_cvars; t && t -> next; t = t -> next) {
	if (str_eq (t -> name, name)) {
	  if (t -> prev)
	    t -> prev -> next = (t -> next ? t -> next : NULL);
	  if (t -> next)
	    t -> next -> prev = (t -> prev ? t -> prev : NULL);
	  return t;
	}
      }
    }
  }
  return NULL;
}

int add_method_arg_cvar (CVAR *c) {

  CVAR *t;
  METHOD *method;
  RT_FN *fn;

  if ((method = __ctalkRtGetMethod ()) != NULL) {

    if (!method -> local_cvars) {
      method -> local_cvars = c;
    } else {
      for (t = method -> local_cvars; t -> next; t = t -> next)
	;
      t -> next = c;
      c -> prev = t;
    }

    return SUCCESS;
  }

  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {

    if (fn -> local_cvars == NULL) {
      fn -> local_cvars = c;
    } else {
    for (t = fn -> local_cvars; t -> next; t = t -> next)
      ;
    t -> next = c;
    c -> prev = t;
    }
    return SUCCESS;
  }

  if (!method_arg_cvars) {
    method_arg_cvars = method_arg_cvars_head = c;
  } else {
    method_arg_cvars_head -> next = c;
    c -> prev = method_arg_cvars_head;
    method_arg_cvars_head = c;
  }

  return SUCCESS;

}


static CVAR *derived_aggregate_cvar (void) {
  METHOD *this_method;
  CVAR *c_mbr;
  if ((this_method = __ctalkRtGetMethod ()) == NULL)
    return NULL;

  for (c_mbr = this_method -> local_cvars;
       c_mbr && c_mbr->next; c_mbr = c_mbr->next)
    ;
  return c_mbr;
}

OBJECT *OBJECT_mbr_class (OBJECT *deref_rcvr_class, char *mbr_name) {
  if (str_eq (mbr_name, "__o_name") ||
      str_eq (mbr_name, "__o_classname") ||
      str_eq (mbr_name, "__o_superclassname")) {
    return rt_defclasses -> p_string_class;
  } else if (str_eq (mbr_name, "__o_value")) {
    if (deref_rcvr_class -> attrs & INT_BUF_SIZE_INIT) {
      return rt_defclasses -> p_integer_class;
    } else {
      return rt_defclasses -> p_string_class;
    }
  } else if (str_eq (mbr_name, "__o_class") ||
	     str_eq (mbr_name, "__o_superclass") ||
	     str_eq (mbr_name, "__o_p_obj") ||
	     str_eq (mbr_name, "instancevars") ||
	     str_eq (mbr_name, "classvars") ||
	     str_eq (mbr_name, "next") ||
	     str_eq (mbr_name, "prev")) {
    return rt_defclasses -> p_object_class;
  } else if (str_eq (mbr_name, "instance_methods") ||
	     str_eq (mbr_name, "class_methods") ||
	     str_eq (mbr_name, "scope") ||
	     str_eq (mbr_name, "nrefs") ||
	     str_eq (mbr_name, "attrs") ||
	     str_eq (mbr_name, "tag")) {
    return rt_defclasses -> p_integer_class;
  } else {
    _warning ("ctalk: Object member, \"%s\" not defined.\n", mbr_name);
    return rt_defclasses -> p_object_class;
  }
	     
  return NULL;
}

/*
 *  For now, objects and methods are returned as int values.
 */
static CVAR *get_object_cvar_struct_mbr (CVAR *c, char *expr_wo_space) {
  CVAR *c_derived_type = NULL;
  char *mbr_ptr, warning_msg[MAXMSG];
  METHOD *this_method;
  OBJECT *obj;
  int baselength;

  if (!(c -> type_attrs & CVAR_TYPE_TYPEDEF)) {
    c_derived_type = __ctalkGetTypedef (c -> type);
    if (!c_derived_type)
      return NULL;
  } else {
    c_derived_type = c;
  }

  if ((this_method = __ctalkRtGetMethod ()) == NULL)
    return NULL;

  obj = (OBJECT *)c->val.__value.__ptr;
  if ((mbr_ptr = strrchr (expr_wo_space, '>')) != NULL) {
    ++mbr_ptr;
#if 0
    /* This might be needed later. */
    baselength = 
      strspn (mbr_ptr, 
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
    mbr_ptr[baselength] = 0;
#endif    

    if (str_eq (mbr_ptr, "__o_name")) {
      __ctalk_register_c_method_arg ("(null)", "char", "(null)",
				     "(null)", "(null)", 
				     expr_wo_space,
				     CVAR_TYPE_CHAR, 1, 0, obj -> scope, 0,
				     (void *)obj->__o_name);
      return derived_aggregate_cvar ();
    }
      if (str_eq (mbr_ptr, "__o_classname") ||
	  str_eq (mbr_ptr, "CLASSNAME")) {
	__ctalk_register_c_method_arg 
	  ("(null)", "char", "(null)",
	   "(null)", "(null)", 
	   expr_wo_space,
	   CVAR_TYPE_CHAR, 1, 0, obj -> scope, 0,
	   /* This could be made more orthognal - it's a little
	      strange. */
	   (void *) (IS_CLASS_OBJECT(obj) ? obj -> __o_class->__o_classname
		     : obj ->CLASSNAME));
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "__o_class")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 0, 0, obj -> scope, 0,
				       (void *)&obj->__o_class);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "__o_superclassname") ||
	  str_eq (mbr_ptr, "SUPERCLASSNAME")) {
	__ctalk_register_c_method_arg ("(null)", "char", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_CHAR, 1, 0, obj -> scope, 0,
				       _SUPERCLASSNAME(obj));
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "__o_superclass")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 1, 0, obj -> scope, 0,
				       (void *)&obj->__o_superclass);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "__o_p_obj")) {
	__ctalk_register_c_method_arg ("(null)", "OBJECT", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_STRUCT, 1, 0,
				       obj -> scope, 0,
				       (void *)obj->__o_p_obj);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "__o_value")) {
	__ctalk_register_c_method_arg ("(null)", "char", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,				       CVAR_TYPE_CHAR, 1, 0, obj -> scope, 0,
				       (void *)obj->__o_value);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "instance_methods")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 0, 0, obj -> scope, 0,
				       (void *)&obj->instance_methods);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "class_methods")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 0, 0, obj -> scope, 0,
				       (void *)&obj->class_methods);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "instancevars")) {
	__ctalk_register_c_method_arg ("(null)", "OBJECT", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_STRUCT, 1, 0,
				       obj -> scope, 0,
				       (void *)obj->instancevars);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "classvars")) {
	__ctalk_register_c_method_arg ("(null)", "OBJECT", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_STRUCT, 1, 0,
				       obj -> scope, 0,
				       (void *)obj->classvars);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "scope")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 0, 0, obj -> scope, 0,
				       (void *)&obj->scope);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "nrefs")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 0, 0, obj -> scope, 0,
				       (void *)&obj->nrefs);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "next")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 1, 0, obj -> scope, 0,
				       (void *)&obj->next);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "prev")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 1, 0, obj -> scope, 0,
				       (void *)&obj->prev);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "attrs")) {
	__ctalk_register_c_method_arg ("(null)", "int", "(null)",
				       "(null)", "(null)", 
				       expr_wo_space,
				       CVAR_TYPE_INT, 0, 0, obj -> scope, 0,
				       (void *)&obj->attrs);
	return derived_aggregate_cvar ();
      }
      if (str_eq (mbr_ptr, "tag")) {
	char tag[MAXLABEL], *name;
	if (obj -> __o_vartags &&
	    obj -> __o_vartags -> tag) {
	  name = obj -> __o_vartags -> tag -> var_decl -> name;
	}
	if (obj -> __o_vartags -> tag) {
	  __ctalk_register_c_method_arg 
	    ("(null)", "char", "(null)",
	     "(null)", "(null)", 
	     tag,
	     0, 1, 0, obj -> scope, 0,
	     (void *)&obj->__o_vartags -> tag -> var_decl -> name);
	} else {
	  __ctalk_register_c_method_arg ("(null)", "char", "(null)",
					 "(null)", "(null)", 
					 (*name ? name : NULLSTR),
					 CVAR_TYPE_CHAR, 1, 0,
					 obj -> scope, 0,
					 NULLSTR);
	}
	return derived_aggregate_cvar ();
      }
      sprintf (warning_msg,
	       "get_object_cvar_struct_member: unknown expression %s.\n",
	       expr_wo_space);
      __ctalkExceptionInternal (NULL, invalid_operand_x, 
				warning_msg, 0);
  }
  return NULL;
}

#define THIS_METHOD (__call_stack[__call_stack_ptr+1]->method)
#define THIS_RT_FN (__call_stack[__call_stack_ptr+1]->_rt_fn)

/* the original version, which has all of the extra stuff
   for embedded spaces and aggregate expressions. */
static CVAR *get_method_arg_cvars_fold (const char *s) {

  CVAR *c;
  CVAR *c_mbr;
  CVAR *__c_prev;
  METHOD *method;
  RT_FN *fn;
  char struct_buf[MAXMSG], c_name_buf[MAXMSG],
    s_buf[MAXMSG], *p, *s_term_p = NULL;
  char *q, *s_term_q = NULL;
  int name_length;

  remove_whitespace ((char *)s, s_buf);

  /* p points to the end of the initial struct label,
     q points to the start of the last "." or "->" operator,
     so we can try to find a match on either the first or the
     last identifier, or the entire expression. */
  if ((q = terminal_struct_op (s_buf)) != NULL) {
    if (*q == '.') {
      s_term_q = q + 1;
    } else if (*q == '-' && *(q + 1) == '>') {
      s_term_q = q + 2;
    }
  }

  if ((p = strstr (s_buf, ".")) != NULL) {
    s_term_p = p + 1;
  } else {
    if ((p = strstr (s_buf, "->")) != NULL) {
      s_term_p = p + 2;
    } else {
      s_term_p = NULL;
    }
  }

  if ((method = __ctalkRtGetMethod ()) != NULL) {
    if (method -> local_cvars) {
      for (c = method -> local_cvars; c && c -> next; c = c -> next)
	;
      while (1) {
	__c_prev = c -> prev;
	remove_whitespace (c -> name, c_name_buf);
	/* this is a little different from
	   get_method_arg_cvars_fold_not_evaled, but we can
	   return immediately. */
	elide_cvartab_struct_alias_2 (c_name_buf, struct_buf);
	if (*struct_buf != 0) {
	  /* don't use the leading '*' here - eval_expr calls this
	   on the first label token */
	  strcpy (c_name_buf, &struct_buf[1]);
	  eval_status |= EVAL_STATUS_CVARTAB_OBJECT_PTR;
	}
	if (!strcmp (c_name_buf, s_buf))
	  return c;
	if ((c -> type_attrs & CVAR_TYPE_CHAR) && (c -> n_derefs == 2)) {
	  /* If the cvar has only the basename of an array member,
	     we have to check for a following subscript.  This is
	     for char **. */
	  name_length = strlen (c_name_buf);
	  if (name_length < strlen (s_buf)) {
	    q = &s_buf[name_length];
	    while (isspace ((int) *q))
	      ++q;
	    if (*q == '[')
	      return c;
	  }
	}
	if ((c_mbr = get_object_cvar_struct_mbr (c, s_buf)) != NULL) {
	  return c_mbr;
	}
	if (p) {
	  if (!strncmp (c_name_buf, s_buf, p - s_buf)) {
	    return c;
	  }
	}
	if (s_term_q) {
	  if (str_eq (c_name_buf, s_term_p)) {
	    if (c -> attrs & CVAR_ATTR_STRUCT_MBR) {
	      return c;
	    }
	  }
	}
	if (is_abbrev_subscript_reg (s_buf, c))
	  return c;
	if (!__c_prev)
	  break;
	c = __c_prev;
      }
    }
  }

  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    if (fn -> local_cvars) {
      for (c = fn -> local_cvars; c && c -> next; c = c -> next)
	;
      while (1) {
	__c_prev = c -> prev;
	remove_whitespace (c -> name, c_name_buf);
	if (!strcmp (c_name_buf, s_buf))
	  return c;
	if ((c_mbr = get_object_cvar_struct_mbr (c, s_buf)) != NULL) {
	  return c_mbr;
	}
	if (p) {
	  if (!strncmp (c_name_buf, s_buf, p - s_buf)) {
	    return c;
	  }
	}
	if  (s_term_p) {
	  if (!strcmp (c_name_buf, s_term_p)) {
	    if (c -> attrs & CVAR_ATTR_STRUCT_MBR) {
	      return c;
	    }
	  }
	}
	if (is_abbrev_subscript_reg (s_buf, c))
	  return c;
	if (!__c_prev)
	  break;
	c = __c_prev;
      }
    }
  }

  if (method) {
    if ((p = index (s_buf, '[')) != NULL) {
      *p = '\0';
      for (c = method -> local_cvars; c; c = c -> next) {
	remove_whitespace (c -> name, c_name_buf);
	if (!strcmp (c_name_buf, s_buf)) {
	  if (c -> attrs & CVAR_ATTR_STRUCT_MBR) {
	    return c;
	  }
	}
      }
    }
  }

  if (fn) {
    if ((p = index (s_buf, '[')) != NULL) {
      *p = '\0';
      for (c = fn -> local_cvars; c; c = c -> next) {
	remove_whitespace (c ->  name, c_name_buf);
	if (!strcmp (c_name_buf, s_buf)) {
	  return c;
	}
      }
    }
  }

  if (!method_arg_cvars)
    return NULL;

  for (c = method_arg_cvars; c; c = c -> next) {
    remove_whitespace (c -> name, c_name_buf);
    if (!strcmp (c_name_buf, s_buf))
      return c;
  }

  return NULL;
}

/* see the comments in get_method_arg_cvars_fold also. */
static CVAR *get_method_arg_cvars_fold_not_evaled (const char *s,
						   int expr_parser_lvl) {

  CVAR *c;
  CVAR *c_mbr;
  CVAR *__c_prev;
  METHOD *method;
  RT_FN *fn;
  char struct_buf[MAXMSG], c_name_buf[MAXMSG], s_buf[MAXMSG],
    *p, *s_term_p = NULL;
  char *q, *s_term_q = NULL;
  int name_length;

  remove_whitespace ((char *)s, s_buf);

  if ((q = terminal_struct_op (s_buf)) != NULL) {
    if (*q == '.') {
      s_term_q = q + 1;
    } else if (*q == '-' && *(q + 1) == '>') {
      s_term_q = q + 2;
    }
  }

  if ((p = strstr (s_buf, ".")) != NULL) {
    s_term_p = p + 1;
  } else {
    if ((p = strstr (s_buf, "->")) != NULL) {
      s_term_p = p + 2;
    } else {
      s_term_p = NULL;
    }
  }

  if ((method = __ctalkRtGetMethod ()) != NULL) {
    if (method -> local_cvars) {
      for (c = method -> local_cvars; c && c -> next; c = c -> next)
	;
      while (1) {
	if (c -> evaled &&
	    (expr_parser_lvl >= c -> attr_data)) {
	  if (c -> prev == NULL) {
	    break;
	  } else {
	    c = c -> prev;
	    continue;
	  }
	}
	__c_prev = c -> prev;
	remove_whitespace (c -> name, c_name_buf);
	elide_cvartab_struct_alias_2 (c_name_buf, struct_buf);
	if (*struct_buf != 0) {
	  strcpy (c_name_buf, &struct_buf[1]);
	  eval_status |= EVAL_STATUS_CVARTAB_OBJECT_PTR;
	}
	if (!strcmp (c_name_buf, s_buf))
	  return c;
	if ((c -> type_attrs & CVAR_TYPE_CHAR) && (c -> n_derefs == 2)) {
	  name_length = strlen (c_name_buf);
	  if (name_length < strlen (s_buf)) {
	    q = &s_buf[name_length];
	    while (isspace ((int) *q))
	      ++q;
	    if (*q == '[')
	      return c;
	  }
	}
	if ((c_mbr = get_object_cvar_struct_mbr (c, s_buf)) != NULL) {
	  return c_mbr;
	}
	if (p) {
	  if (!strncmp (c_name_buf, s_buf, p - s_buf)) {
	    return c;
	  }
	}
	if (s_term_q) {
	  if (str_eq (c_name_buf, s_term_p)) {
	    if (c -> attrs & CVAR_ATTR_STRUCT_MBR) {
	      return c;
	    }
	  }
	}
	if (is_abbrev_subscript_reg (s_buf, c))
	  return c;
	if (!__c_prev)
	  break;
	c = __c_prev;
      }
    }
  }

  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    if (fn -> local_cvars) {
      for (c = fn -> local_cvars; c && c -> next; c = c -> next)
	;
      while (1) {
	if (c -> evaled &&
	    (expr_parser_lvl >= c -> attr_data)) {
	  if (c -> prev == NULL) {
	    break;
	  } else {
	    c = c -> prev;
	    continue;
	  }
	}
	__c_prev = c -> prev;
	remove_whitespace (c -> name, c_name_buf);
	if (!strcmp (c_name_buf, s_buf))
	  return c;
	if ((c_mbr = get_object_cvar_struct_mbr (c, s_buf)) != NULL) {
	  return c_mbr;
	}
	if (p) {
	  if (!strncmp (c_name_buf, s_buf, p - s_buf)) {
	    return c;
	  }
	}
	if  (s_term_p) {
	  if (!strcmp (c_name_buf, s_term_p)) {
	    if (c -> attrs & CVAR_ATTR_STRUCT_MBR) {
	      return c;
	    }
	  }
	}
	if (is_abbrev_subscript_reg (s_buf, c))
	  return c;
	if (!__c_prev)
	  break;
	c = __c_prev;
      }
    }
  }

  if (method) {
    if ((p = index (s_buf, '[')) != NULL) {
      *p = '\0';
      for (c = method -> local_cvars; c; c = c -> next) {
	remove_whitespace (c -> name, c_name_buf);
	if (!strcmp (c_name_buf, s_buf)) {
	  if (c -> attrs & CVAR_ATTR_STRUCT_MBR) {
	    return c;
	  }
	}
      }
    }
  }

  if (fn) {
    if ((p = index (s_buf, '[')) != NULL) {
      *p = '\0';
      for (c = fn -> local_cvars; c; c = c -> next) {
	remove_whitespace (c ->  name, c_name_buf);
	if (!strcmp (c_name_buf, s_buf)) {
	  return c;
	}
      }
    }
  }

  if (!method_arg_cvars)
    return NULL;

  for (c = method_arg_cvars; c; c = c -> next) {
    remove_whitespace (c -> name, c_name_buf);
    if (!strcmp (c_name_buf, s_buf))
      return c;
  }

  return NULL;
}

CVAR *get_method_arg_cvars (const char *s) {

  CVAR *c, *__c_prev;
  METHOD *method;
  RT_FN *fn;

  if (__call_stack_ptr >= MAXARGS)
    return NULL;

  if (strpbrk (s, " \t\n\f-.["))
    return get_method_arg_cvars_fold (s);

  if (__call_stack[__call_stack_ptr+1]) {
    if (__call_stack[__call_stack_ptr+1] -> method) {
      if (THIS_METHOD -> local_cvars) {
	for (c = THIS_METHOD -> local_cvars; c && c -> next; c = c -> next)
	  ;
	while (1) {
	  __c_prev = c -> prev;
	  if (str_eq (c -> name, (char *)s))
	    return c;
	  if (!__c_prev)
	    break;
	  c = __c_prev;
	}
      }
    } else if ((__call_stack_ptr < MAXARGS)
	       && __call_stack[__call_stack_ptr + 1] -> _rt_fn) {
      if (THIS_RT_FN -> local_cvars) {
	for (c = THIS_RT_FN -> local_cvars; c && c -> next; c = c -> next)
	  ;
	while (1) {
	  __c_prev = c -> prev;
	  if (str_eq (c -> name, (char *)s))
	    return c;
	  if (!__c_prev)
	    break;
	  c = __c_prev;
	}
      }
    }
  }
  if (!method_arg_cvars)
    return NULL;

  for (c = method_arg_cvars; c; c = c -> next) {
    if (str_eq (c -> name, (char *)s))
      return c;
  }

  return NULL;
}

CVAR *get_method_arg_cvars_not_evaled (const char *s, int expr_parser_lvl) {

  CVAR *c, *__c_prev;
  METHOD *method;
  RT_FN *fn;

  if (__call_stack_ptr >= MAXARGS)
    return NULL;

  if (strpbrk (s, " \t\n\f-.["))
    return get_method_arg_cvars_fold_not_evaled (s, expr_parser_lvl);

  if (__call_stack[__call_stack_ptr+1]) {
    if (__call_stack[__call_stack_ptr+1] -> method) {
      if (THIS_METHOD -> local_cvars) {
	for (c = THIS_METHOD -> local_cvars; c && c -> next; c = c -> next)
	  ;
	while (1) {
	  if (c -> evaled &&
	      (expr_parser_lvl >= c -> attr_data)) {
	    /* i.e., we can still use the CVAR in sub-parsers */
	    if ((c = c -> prev) == NULL)
	      break;
	    else
	      continue;
	  }
	  __c_prev = c -> prev;
	  if (str_eq (c -> name, (char *)s))
	    return c;
	  if (!__c_prev)
	    break;
	  c = __c_prev;
	}
      }
    } else if ((__call_stack_ptr < MAXARGS)
	       && __call_stack[__call_stack_ptr + 1] -> _rt_fn) {
      if (THIS_RT_FN -> local_cvars) {
	for (c = THIS_RT_FN -> local_cvars; c && c -> next; c = c -> next)
	  ;
	while (1) {
	  if (c -> evaled &&
	      (expr_parser_lvl >= c -> attr_data)) {
	    if ((c = c -> prev) == NULL)
	      break;
	    else
	      continue;
	  }
	  __c_prev = c -> prev;
	  if (str_eq (c -> name, (char *)s))
	    return c;
	  if (!__c_prev)
	    break;
	  c = __c_prev;
	}
      }
    }
  }
  if (!method_arg_cvars)
    return NULL;

  for (c = method_arg_cvars; c; c = c -> next) {
    if (str_eq (c -> name, (char *)s))
      return c;
  }

  return NULL;
}

/*
 *  API version of the function above.
 */

CVAR *__ctalkGetCArg (OBJECT *o) {

  CVAR *c;
  OBJECT *(*method_fn)(void), *arg_value_obj;
  METHOD *method;
  RT_FN *fn;

  arg_value_obj = ((o -> instancevars) ? o -> instancevars : o);

  if ((method_fn = rtinfo.method_fn) != NULL) {

    if ((method = __this_method ()) == NULL) {
      _warning ("Undefined method in get_method_arg_cvar.\n");
      return NULL;
    }

    for (c = method -> local_cvars; c; c = c -> next)
      if (!strcmp (c -> name, arg_value_obj -> __o_value))
 	return c;

  }

  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
     for (c = fn -> local_cvars; c; c = c -> next) {
       if (!strcmp (c -> name, arg_value_obj -> __o_value))
 	return c;
     }
   }

  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr + 1] -> _rt_fn) != NULL)) {
     for (c = fn -> local_cvars; c; c = c -> next) {
       if (!strcmp (c -> name, arg_value_obj -> __o_value))
 	return c;
     }
   }

  return NULL;
}

/* returns true if the variable is registered multiple
   times in a call stack frame.  This means we give it
   an extra reference count so it isn't deleted by
   some cleanup routine until their are no more duplicate
   CVARs in the call stack. */
static bool multiple_cvars_in_frame (char *varname) {
  RT_FN *fn;
  METHOD *method;
  CVAR *c;
  int n = 0;
  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    for (c = fn -> local_cvars; c; c = c -> next) {
      if (str_eq (c -> name, varname)) {
	++n;
      }
    }
  } else if ((method = __ctalkRtGetMethod ()) != NULL) {
    for (c = method -> local_cvars; c; c = c -> next) {
      if (str_eq (c -> name, varname)) {
	++n;
      }
    }
  }
  return (n > 1);
}

/* Refer to the comments in object_from_int_CVAR. */
static OBJECT *object_from_longlong_CVAR (CVAR *c, int *obj_is_created,
					  int d_scope) {
  long long int *i, i_2, r_ptr;
  OBJECT *o, *o_ref, *o_ref2;
  char buf[0xff];

  switch (c -> n_derefs)
    {
    case 0:
      o = create_object_init_internal
	(c -> name, rt_defclasses -> p_longinteger_class, d_scope, "");
      *(long long *)o -> __o_value =
	*(long long *)o -> instancevars -> __o_value =
	c -> val.__value.__ll;
      *obj_is_created = TRUE;
      return o;
      break;
    case 1:
      if (c -> n_derefs && c -> initializer_size) {
	if (c -> attrs & CVAR_ATTR_ARRAY_DECL) {
	  /* i.e., an expression with a subscript */
	  o = create_object_init_internal
	    (c -> name, rt_defclasses -> p_longinteger_class, d_scope, "");
	  *(long long *)o -> __o_value =
	    *(long long *)o -> instancevars -> __o_value =
	    c -> val.__value.__ll;
	  *obj_is_created = TRUE;
	  return o;
	} else {
	  o = create_object_init_internal
	    (c -> name, rt_defclasses -> p_longinteger_class, d_scope, "");
	  *(unsigned long *)o -> __o_value =
	    *(unsigned long *)o -> instancevars -> __o_value =
	    (unsigned long) c-> val.__deref_ptr;
	  *obj_is_created = TRUE;
	  return o;
	}
      } else if (c -> attrs & CVAR_ATTR_ARRAY_DECL) {
	o = create_object_init_internal
	  (c -> name, rt_defclasses -> p_longinteger_class, d_scope, "");
	*(long long *)o -> __o_value =
	  *(long long *)o -> instancevars -> __o_value =
	  c -> val.__value.__ll;
	*obj_is_created = TRUE;
	return o;
      }
#if 0 /* Leave this here - in case we need it again. */
      
      o_ref = __ctalkCreateObjectInit (c -> name, 
				       LONGINTEGER_CLASSNAME,
				       LONGINTEGER_SUPERCLASSNAME,
				       d_scope|VAR_REF_OBJECT,
				       buf);
      o_ref -> attrs |= OBJECT_REF_IS_CVAR_PTR_TGT;
      CVHTOA(buf,o_ref);
      o = __ctalkCreateObjectInit (c -> name, 
				   SYMBOL_CLASSNAME,
				   SYMBOL_SUPERCLASSNAME,
				   d_scope,
				   buf);
      o -> attrs |= OBJECT_REF_IS_CVAR_PTR_TGT;
      *obj_is_created = TRUE;
#endif      
      break;
    case 2:
      i_2 = (long long int)((long long int *)c -> val.__value.__ptr)[0];
      if (c -> initializer_size > 0) {
	o = create_object_init_internal
	  (c -> name, rt_defclasses -> p_longinteger_class, d_scope, "");
	*(long long *)o -> __o_value =
	  *(long long *)o -> instancevars -> __o_value = i_2;
      } else {
	o_ref = create_object_init_internal
	  (c -> name, rt_defclasses -> p_longinteger_class,
	   d_scope|VAR_REF_OBJECT, "");
	*(long long *)o_ref -> __o_value =
	  *(long long *)o_ref -> instancevars -> __o_value = i_2;
	o_ref2 = create_object_init_internal
	  (c -> name, rt_defclasses -> p_symbol_class,
	   d_scope, "");
	*(OBJECT **)o_ref2 -> __o_value =
	  *(OBJECT **)o_ref2 -> instancevars -> __o_value =
	  o_ref;
	o = create_object_init_internal
	  (c -> name, rt_defclasses -> p_symbol_class,
	   d_scope, "");
	*(OBJECT **)o -> __o_value =
	  *(OBJECT **)o -> instancevars -> __o_value =
	  o_ref2;
      }
      *obj_is_created = TRUE;
      break;
    }
  __ctalkSetObjectAttr (o, o -> attrs | OBJECT_REF_IS_CVAR_PTR_TGT);
  return o;
}

static OBJECT *object_from_int_CVAR (CVAR *c, int *obj_is_created,
				     int d_scope) {
  int *i, r_ptr; 
  uintptr_t i_2;
#ifdef __x86_64
  long long int li;
#endif  
  OBJECT *o, *o_ref, *o_ref2;
  char buf[0xff];

  switch (c -> n_derefs)
    {
    case 0:
      o = create_object_init_internal
	(c -> name, rt_defclasses -> p_integer_class, d_scope, "");
      SETINTVARS(o, c -> val.__value.__i);
      *obj_is_created = TRUE;
      break;
    case 1:
      /* if this is a simplified reference to
	 an array start, treat it the same
	 as an int *. This is all we need to
	 do with arrays with a prefix operator
	 now anyway.
      */
      if (c -> n_derefs && c -> initializer_size) {
	if (c -> attrs & CVAR_ATTR_ARRAY_DECL) {
	  o = create_object_init_internal
	    (c -> name, rt_defclasses -> p_integer_class, d_scope, "");
	  SETINTVARS(o, c -> val.__value.__i);
	  *obj_is_created = TRUE;
	  return o;
	} else {
	  o = create_object_init_internal
	    (c -> name, rt_defclasses -> p_integer_class, d_scope, "");
	  *(unsigned long *)o -> __o_value =
	    *(unsigned long *)o -> instancevars -> __o_value =
	    (unsigned long)c -> val.__deref_ptr;
	*obj_is_created = TRUE;
	return o;
	}
      } else if (c -> attrs & CVAR_ATTR_ARRAY_DECL) {
	o = create_object_init_internal
	  (c -> name, rt_defclasses -> p_integer_class, d_scope, "");
	SETINTVARS(o, c -> val.__value.__i);
	*obj_is_created = TRUE;
	return o;
      }
      /* save the object in the first declared method we
	 find - this is usually needed to prevent cvartab
	 objects from getting lost after an inline call. 
	 The attribute means we can delete the object
	 completely during the method pool cleaning.
      */
      o_ref -> attrs |= OBJECT_REF_IS_CVAR_PTR_TGT;
      /* *** Do this separately... */
#ifdef __x86_64
      li = (long long int)((long long int **)c -> val.__value.__ptr)[0];
      i_2 = (uintptr_t)li;
#else      
      i_2 = (uintptr_t)((uintptr_t **)c -> val.__value.__ptr)[0];
#endif
 
      /*  __ctalkDecimalIntegerToASCII (i_2, buf); *//***/
      if (c -> initializer_size > 0) {
#if 0 /***/
	o = __ctalkCreateObjectInit (c -> name, 
				     INTEGER_CLASSNAME,
				     INTEGER_SUPERCLASSNAME,
				     d_scope,
				     buf);
#endif	
      } else {
#if 0 /***/
	o_ref = __ctalkCreateObjectInit (c -> name, 
					 INTEGER_CLASSNAME,
					 INTEGER_SUPERCLASSNAME,
					 d_scope|VAR_REF_OBJECT,
					 buf);
	CVHTOA(buf,o_ref);
	o_ref2 = __ctalkCreateObjectInit (c -> name, 
					  SYMBOL_CLASSNAME,
					  SYMBOL_SUPERCLASSNAME,
					  d_scope,
					  buf);
	CVHTOA(buf,o_ref2);
	o = __ctalkCreateObjectInit (c -> name, 
				     SYMBOL_CLASSNAME,
				     SYMBOL_SUPERCLASSNAME,
				     d_scope,
				     buf);
#endif	
      }
      *obj_is_created = TRUE;
      break;
    }
  __ctalkSetObjectAttr (o, o -> attrs | OBJECT_REF_IS_CVAR_PTR_TGT);
  return o;
}

/* this works equally well for OBJECT_T and PTR_T values (so far). */
static OBJECT *object_from_CVAR (CVAR *c, OBJECT **o_out, int *obj_is_created) {
  OBJECT *o_from_var;
  *o_out = NULL;
  *obj_is_created = FALSE;
  switch (c -> n_derefs)
    {
    case 1:
      o_from_var = (OBJECT *)c -> val.__value.__ptr;
      if (IS_OBJECT (o_from_var)) {
	if (multiple_cvars_in_frame (c -> name)) {
	  __objRefCntInc (OBJREF(o_from_var));
	}
	*o_out = o_from_var;
      } else if ((o_from_var = obj_ref_str 
		  ((char *)c -> val.__value.__ptr))  != NULL) {
	*o_out = create_object_init_internal
	  (c -> name, rt_defclasses -> p_symbol_class, CREATED_PARAM,
	   (char *)c -> val.__value.__ptr);
	*obj_is_created = TRUE;
      } else if (c -> val.__value.__ptr == NULL) {
	*o_out = create_object_init_internal
	  (c -> name, rt_defclasses -> p_object_class, CREATED_PARAM, "0x0");
	*obj_is_created = TRUE;
      } else {
	*o_out = create_object_init_internal
	  (NULLSTR, rt_defclasses -> p_integer_class, CREATED_PARAM, "0");
	*obj_is_created = TRUE;
      }
      break;
    case 2:
      /* this could be an OBJECT ** in a cvartab */
      if (c -> type_attrs & CVAR_TYPE_OBJECT) {
	*o_out = *(OBJECT **)c -> val.__value.__ptr;
	*obj_is_created = FALSE;
      } else if ((c -> attrs & CVAR_ATTR_CVARTAB_ENTRY) &&
		 str_eq (c -> type, "OBJECT")) {
	*o_out = *(OBJECT **)c -> val.__value.__ptr;
      }
      break;
    }
  return *o_out;
}

OBJECT *cvar_object (CVAR *c, int *obj_is_created) {
  static OBJECT *o = NULL;         /* Avoid a warning. */
  OBJECT *o_from_var;
  OBJECT *o_ref, *o_ref2;
  char buf[MAXLABEL], *p;
  int d_scope;
#ifdef __x86_64
  long long int li;
#endif  
  uintptr_t *__i, i_2, r_ptr;

  *obj_is_created = FALSE;

#if 0
  /* We don't (yet) need to take the actual object's scope into
     account when creating a copy that's local to this expression. */
  d_scope = c -> scope | CREATED_CVAR_SCOPE;
#else
  d_scope = CREATED_CVAR_SCOPE;
#endif

  switch (c -> val.__type) 
    {
    case INTEGER_T:
    case LONG_T:
      /*
       *  Char types can also occur here.
       */
      if (c -> type_attrs & CVAR_TYPE_CHAR) {
	if (c -> val.__value.__i > UCHAR_MAX) {
	  printf ("ctalk: Warning: value of %s truncated to Character.\n",
		  c -> name);
	}
	buf[0] = (char)c -> val.__value.__i & 0xff;
	buf[1] = 0;
	o = create_object_init_internal
	  (c -> name, rt_defclasses -> p_character_class, d_scope, buf);
	*obj_is_created = TRUE;
      } else {
	/*
	 *  Here we might need to encode the initializer size into
	 *  the value of the newly created object, if we should
	 *  ever need to perform pointer math with them.  
	 *  for now, however, prefix operators only need the address of the 
	 *  first element.
	 */
	if (c -> n_derefs && c -> initializer_size) {
	  if (c -> attrs & CVAR_ATTR_ARRAY_DECL) {
	    __ctalkDecimalIntegerToASCII (c->val.__value.__i, buf);
	    o = create_object_init_internal
	      (c -> name, rt_defclasses -> p_integer_class, d_scope, buf);
	    *obj_is_created = TRUE;
	    return o;
	  } else {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
	    sprintf (buf, "%#lx", (unsigned long int)c -> val.__deref_ptr);
#else
	    (void)htoa (buf, (unsigned int)c -> val.__deref_ptr);
#endif
	    o = create_object_init_internal
	      (c -> name, rt_defclasses -> p_integer_class, d_scope, buf);
	    *obj_is_created = TRUE;
	    return o;
	  }
	} else {
	  return object_from_int_CVAR (c, obj_is_created, d_scope);
	}
      }
      break;
    case LONGLONG_T:
      return object_from_longlong_CVAR (c, obj_is_created, d_scope);
      break;
    case FLOAT_T:
    case DOUBLE_T:
      if (c -> n_derefs && c -> initializer_size) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
	sprintf (buf, "%#lx", (unsigned long int)c -> val.__deref_ptr);
#else
	(void)htoa (buf, (unsigned int)c -> val.__deref_ptr);
#endif
      } else {
#ifdef DJGPP
      sprintf (buf, "%lf", c -> val.__value.__d);
#else
      snprintf (buf, MAXLABEL, "%lf", c -> val.__value.__d);
#endif
      }
      o = create_object_init_internal
	(c -> name, rt_defclasses -> p_float_class, d_scope, buf);
      *obj_is_created = TRUE;
      break;
#if !(defined(__APPLE__) && defined(__ppc__))
    case LONGDOUBLE_T: /***/
      if (c -> n_derefs && c -> initializer_size) {
	/* untested for now */
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
	sprintf (buf, "%#lx", (unsigned long int)c -> val.__deref_ptr);
#else
	(void)htoa (buf, (unsigned int)c -> val.__deref_ptr);
#endif
      } else {
	snprintf (buf, MAXLABEL, "%Lf", c -> val.__value.__ld);
      }
      o = create_object_init_internal
	(c -> name, rt_defclasses -> p_float_class, d_scope, buf);
      *obj_is_created = TRUE;
      break;
#endif      /* !(defined(__APPLE__) && defined(__ppc__)) */
    case OBJECT_T:
      return object_from_CVAR (c, &o, obj_is_created);
      break;
    case PTR_T:
#ifdef DJGPP
      (void)htoa (buf, (unsigned int)c -> val.__ptr);
#else

# if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      snprintf (buf, MAXLABEL, "%#lx", 
		(unsigned long int)c -> val.__value.__ptr);
# else
      (void)htoa (buf, (unsigned int)c -> val.__value.__ptr);
# endif
#endif
      if (!chkptrstr (buf)) {
	errno = 0;
	if (strtol(buf, NULL, 0) != 0) {
	  _warning 
	    ("cvar_object: Variable \"%s\" does not have a valid address.\n",
	     c -> name);
	}
	o = create_object_init_internal
	  (c -> name, rt_defclasses -> p_integer_class, d_scope, NULL);
	*obj_is_created = TRUE;
	return o;
	if (errno) {
	  _warning ("cvar_object: %s: %s.\n", c -> name, strerror (errno));
	}
      }

      if (c -> n_derefs == 2 && str_eq (c -> type, "OBJECT")) 
	o = object_from_CVAR (c, &o, obj_is_created);
      else
	o = (OBJECT *)c -> val.__value.__ptr;
      if (!IS_OBJECT(o)) {
	/* clean this up later */
	if (c -> type_attrs & CVAR_TYPE_CHAR) {
	switch (c -> n_derefs)
	  {
	  case 0:
	    /*
	     *  Character class are handled as INTEGER_T values,
	     *  above.
	     */
	      break;
	    case 1:
	      o = create_object_init_internal
		(c -> name, rt_defclasses -> p_string_class, d_scope, "");
		if (c -> attrs & CVAR_ATTR_CVARTAB_ENTRY) {
		  /* unref it here, at least for now. */
		  o -> __o_value[0] = ((char *)c -> val.__value.__ptr)[0];
		  o -> instancevars -> __o_value[0] =
		    ((char *)c -> val.__value.__ptr)[0];
		  o -> __o_value[1] = '\0';
		  o -> instancevars -> __o_value[1] = '\0';
		} else {
		  __xfree (MEMADDR(o -> __o_value));
		  o -> __o_value = strdup ((char *)c -> val.__value.__ptr);
		  __xfree (MEMADDR(o -> instancevars -> __o_value));
		  o -> instancevars -> __o_value =
		    strdup ((char *)c -> val.__value.__ptr);
		}
		*obj_is_created = TRUE;
	      break;
	    case 2:
	      if ((c -> attrs & CVAR_ATTR_CVARTAB_ENTRY) ||
		  /* We still need this for now. *** TODO - remove ? */
		  is_cvartab_entry (c)) {
		/* the extra deref is just that -
		   attach to a Symbol object. */
		o_ref = create_object_init_internal
		  (c -> name, rt_defclasses -> p_string_class,
		   d_scope|VAR_REF_OBJECT, (char *)c -> val.__value.__ptr);
		o = create_object_init_internal
		  (c -> name, rt_defclasses -> p_symbol_class, d_scope, "");
		SYMVAL(o -> __o_value) =
		  SYMVAL(o -> instancevars -> __o_value) = (uintptr_t)o_ref;
		  
		*obj_is_created = TRUE;
	      } else {
		/*
		 *  Array members do not automatically translate 
		 *  from cvars - they need to be atPut manually.
		 */
		o = create_object_init_internal
		  (c -> name, rt_defclasses -> p_symbol_class, d_scope, "");
		memcpy (o -> __o_value, &c -> val.__value.__ptr,
			sizeof (char **));
		memcpy (o -> instancevars -> __o_value,
			&c -> val.__value.__ptr,
			sizeof (char **));
		o -> attrs |= OBJECT_VALUE_IS_C_CHAR_PTR_PTR;
		o -> instancevars -> attrs 
		  |= OBJECT_VALUE_IS_C_CHAR_PTR_PTR;
		*obj_is_created = TRUE;
	      }
	      break;
	    default:
	      break;
	    }
	} else if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
	  double d;
	  switch (c -> n_derefs)
	    {
	    case 0:
	      sprintf (buf, "%lf", c -> val.__value.__d);
	      o = create_object_init_internal
		(c -> name, rt_defclasses -> p_float_class, d_scope, buf);
	      *obj_is_created = TRUE;
	      return o;
	      break;
	    case 1:
		sprintf (buf, "%lf", *(double *)c -> val.__value.__ptr);
		o_ref = create_object_init_internal
		  (c -> name, rt_defclasses -> p_float_class,
		   d_scope|VAR_REF_OBJECT, buf);
		o = create_object_init_internal
		  (c -> name, rt_defclasses -> p_symbol_class,
		   d_scope, "");
		*(OBJECT **)o -> __o_value =
		  *(OBJECT **)o -> instancevars -> __o_value = o_ref;
		o -> attrs |= OBJECT_REF_IS_CVAR_PTR_TGT;
		*obj_is_created = TRUE;
		return o;
	      break;
	    default:
	      break;
	    }
	} else {
	  if (c -> type_attrs & CVAR_TYPE_FILE) {
	    /*
	     *  Unsigned ints do not have "0x" prefixes, so
	     *  we don't have to delete an existing prefix.
	     */
# if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
	    sprintf (buf, "%#lx", (unsigned long int)c -> val.__value.__ptr);
#else
	    (void)htoa (buf, (unsigned int)c -> val.__value.__ptr);
#endif
	    o = create_object_init_internal
	      (c -> name, rt_defclasses -> p_filestream_class,
	       d_scope, buf);
	    *obj_is_created = TRUE;
	  } else {
	      if (!strcmp (c -> type, "void")) {
		switch (c -> n_derefs)
		  {
		  case 0:
		    _warning ("void variable, \"%s,\" is not a pointer. Handling by reference anyway.\n", c -> name);
		    /* Fall through */
		  default:
#if 0 /***/
# if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
		    sprintf (buf, "%#lx", 
			     (unsigned long int)c -> val.__value.__ptr);
#else
		    (void)htoa (buf, (unsigned int)c -> val.__value.__ptr);
#endif /***/
#endif		    
		    o = create_object_init_internal
		      (c -> name, rt_defclasses -> p_symbol_class,
		       d_scope, "");
		    *(long int *)o -> __o_value =
		      *(long int *)o -> instancevars -> __o_value =
		      (long int)c -> val.__value.__ptr;
		    *obj_is_created = TRUE;
		    break;
		  } 
	      } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
		long long int ll;
		switch (c -> n_derefs)
		  {
		  case 0:
		    o = create_object_init_internal
		      (c -> name, rt_defclasses -> p_longinteger_class,
		       d_scope, "");
		    *(long long *)o -> __o_value =
		      *(long long *)o -> instancevars -> __o_value =
		      c -> val.__value.__ll;
		    *obj_is_created = TRUE;
		    break;
		  case 1:
		    ll = *(long long *)c -> val.__value.__ptr;
		    o_ref = create_object_init_internal
		      (c -> name, rt_defclasses -> p_longinteger_class,
		       d_scope|VAR_REF_OBJECT, "");
		    *(long long *)o_ref -> __o_value =
		      *(long long *)o_ref -> instancevars -> __o_value
		      = ll;
		      o = create_object_init_internal
			(c -> name, rt_defclasses -> p_symbol_class,
			 d_scope, "");
		    SYMVAL(o -> __o_value) =
		      SYMVAL(o -> instancevars -> __o_value) =
		      (uintptr_t)o_ref;
		    *obj_is_created = TRUE;
		    break;
		  }
	      } else if (c -> type_attrs & CVAR_TYPE_INT) {
		switch (c -> n_derefs)
		    {
		    case 0:
		      /* This is sort of an odd behaviour - shows
			 up in Darwin -- we'll see if this needs
			 more work. */
		      o = create_object_init_internal
			(c -> name, rt_defclasses -> p_integer_class,
			 d_scope, "");
		      SETINTVARS(o_ref, (int)c -> val.__value.__ptr);
		      *obj_is_created = TRUE;
		      break;
		    case 1:
		      /* if this is a simplified reference to
			 an array start, treat it the same
			 as a long int *. */
		      i_2 = ((uintptr_t *)c -> val.__value.__ptr)[0];
		      /* causes an intermittent leak - try to fix
			 when not optimizing */
		      o_ref = create_object_init_internal
			(c -> name, rt_defclasses -> p_integer_class,
			 d_scope|VAR_REF_OBJECT, "");
		      SETINTVARS(o_ref, i_2);
		      o_ref -> attrs |= OBJECT_REF_IS_CVAR_PTR_TGT;
		      o = create_object_init_internal
			(c -> name, rt_defclasses -> p_symbol_class,
			 d_scope, "");
		      SYMVAL(o -> __o_value) =
			SYMVAL(o -> instancevars -> __o_value) =
			(uintptr_t)o_ref;
		      o -> attrs |= OBJECT_REF_IS_CVAR_PTR_TGT;
		      *obj_is_created = TRUE;
		      break;
		    case 2:
		      if (c -> initializer_size > 0) {
			i_2 = (uintptr_t)((uintptr_t **)c ->
					  val.__value.__ptr)[0];
			o = create_object_init_internal
			  (c -> name, rt_defclasses -> p_integer_class,
			   d_scope, "");
			SETINTVARS(o, i_2);
		      } else {
			i_2 = **(int **)c -> val.__value.__ptr;
			o_ref = create_object_init_internal
			  (c -> name, rt_defclasses -> p_integer_class,
			   d_scope|VAR_REF_OBJECT, "");
			INTVAL(o_ref -> __o_value) = 
			  INTVAL(o_ref -> instancevars -> __o_value) = i_2;
			o_ref2 = create_object_init_internal
			  (c -> name, rt_defclasses -> p_symbol_class,
			   d_scope, "");
			SYMVAL(o_ref2 -> __o_value) =
			  SYMVAL(o_ref2 -> instancevars -> __o_value) =
			  (uintptr_t)o_ref;
			o = create_object_init_internal
			  (c -> name, rt_defclasses -> p_symbol_class,
			   d_scope, "");
			SYMVAL(o -> __o_value) =
			  SYMVAL(o -> instancevars -> __o_value) =
			  (uintptr_t)o_ref2;
		      }
		      *obj_is_created = TRUE;
		      break;
		    }
	      } else {
		  char buf[MAXLABEL];
		  strcatx (buf, c -> type, " ", c -> name, NULL);
		  __ctalkExceptionInternal (NULL, undefined_type_x,
					    buf, 0);
		  o = create_object_init_internal
		    (c -> name, rt_defclasses -> p_string_class,
		     d_scope, NULL);
		  *obj_is_created = TRUE;
		}
	      }
	}
      }
      break;
    }
    return o;
}

OBJECT *cvar_object_mark_evaled (CVAR *c, int *obj_is_created,
				 int expr_parser_ptr) {
  OBJECT *o = cvar_object (c, obj_is_created);
  if (!c -> evaled) {
    c -> evaled = true;
    /* the CVAR is still valid for any subexprs parsed again by
       eval_subexpr, so record the level of the topmost parser. */
    c -> attr_data = expr_parser_ptr;
  }
  return o;
}

int delete_method_arg_cvars (void) {

  METHOD *method;
  RT_FN *fn;

  if ((method = __ctalkRtGetMethod ()) != NULL) {
    if (method -> local_cvars) {
      CVAR *s, *t;
      if ((method -> local_cvars -> next == NULL) &&
	  (method -> local_cvars -> prev == NULL)) {
	_delete_cvar (method -> local_cvars);
	method -> local_cvars = NULL;
      } else {
	t = method -> local_cvars;
	while (1) {
	  s = t -> next;
	  _delete_cvar (t);
	  if (!s) break;
	  t = s;
	}
      }
      method -> local_cvars = NULL;
    }
    return SUCCESS;
  }

  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    CVAR *s, *t;
    if (fn -> local_cvars) {
      t = fn -> local_cvars;
      while (1) {
	s = t -> next;
	_delete_cvar (t);
	if (!s) break;
	t = s;
      }
      fn -> local_cvars = NULL;
    }
  }

  return SUCCESS;
}

/* #define CHECK_UNUSED_CVARS */

int delete_method_arg_cvars_evaled (void) {

  METHOD *method;
  RT_FN *fn;
  CVAR *s, *t;
#ifdef CHECK_UNUSED_CVARS
  bool have_cvars = false;
  bool cvar_delete = false;
#endif  

  if ((method = __ctalkRtGetMethod ()) != NULL) {
    if (method -> local_cvars) {
#ifdef CHECK_UNUSED_CVARS
      have_cvars = true;
#endif      
      if ((method -> local_cvars -> next == NULL) &&
	  (method -> local_cvars -> prev == NULL) &&
	  method -> local_cvars -> evaled) {
#ifdef CHECK_UNUSED_CVARS
	cvar_delete = true;
#endif	
	_delete_cvar (method -> local_cvars);
	method -> local_cvars = NULL;
      } else if (method -> local_cvars -> next == method -> local_cvars) {
	if (method -> local_cvars -> evaled) {
#ifdef CHECK_UNUSED_CVARS
	  cvar_delete = true;
#endif	  
	  _delete_cvar (method -> local_cvars);
	  method -> local_cvars = NULL;
	}
      } else {
	t = method -> local_cvars;
	while (1) {
	  s = t -> next;
	  if (t -> evaled) {
	    if (t -> next) t -> next -> prev = t -> prev;
	    if (t -> prev) t -> prev -> next = t -> next;
	    if (t == method -> local_cvars) {
	      method -> local_cvars = t -> next;
	      if (method -> local_cvars)
		method -> local_cvars -> prev = NULL;
	    }
#ifdef CHECK_UNUSED_CVARS
	    cvar_delete = true;
#endif	    
	    _delete_cvar (t);
	  }
	  if (s == NULL)
	    break;
	  t = s;
	}
      }
    }
#ifdef CHECK_UNUSED_CVARS
    if (have_cvars && !cvar_delete) {
      printf (">>>>>>> UNDELETED CVARS\n");
    }
#endif    
    return SUCCESS;
  }

  if ((__call_stack_ptr < MAXARGS) &&
      ((fn = __call_stack[__call_stack_ptr+1]->_rt_fn) != NULL)) {
    if (fn -> local_cvars) {
#ifdef CHECK_UNUSED_CVARS
      have_cvars = true;
#endif      
      if ((fn -> local_cvars -> next == NULL) &&
	  (fn -> local_cvars -> prev == NULL) &&
	  fn -> local_cvars -> evaled) {
#ifdef CHECK_UNUSED_CVARS
	cvar_delete = true;
#endif	
	_delete_cvar (fn -> local_cvars);
	fn -> local_cvars = NULL;
      } else {
	t = fn -> local_cvars;
	while (1) {
	  s = t -> next;
	  if (t -> evaled) {
	    if (t -> next) t -> next -> prev = t -> prev;
	    if (t -> prev) t -> prev -> next = t -> next;
	    if (t == fn -> local_cvars) {
	      fn -> local_cvars = t -> next;
	      if (fn -> local_cvars)
		fn -> local_cvars -> prev = NULL;
	    }
#ifdef CHECK_UNUSED_CVARS
	    cvar_delete = true;
#endif	    
	    _delete_cvar (t);
	  }
	  if (s == NULL)
	    break;
	  t = s;
	}
      }
    }
  }

#ifdef CHECK_UNUSED_CVARS
  if (have_cvars && !cvar_delete) {
    printf (">>>>>>> UNDELETED CVARS\n");
  }
#endif  
  method_arg_cvars = NULL;
  return SUCCESS;
}

void _new_cvar (CVARREF_T cvar_ref) {
  *cvar_ref = __xalloc (sizeof(CVAR));
  (*cvar_ref) -> sig = CVAR_SIG;
}

/*
 *  TO DO - If necessary, delete all links of a 
 *  multiple declaration.
 */
void _delete_cvar (CVAR *c) {

  CVAR *t;

  if (!IS_CVAR (c)) return;

  if (IS_CVAR(c -> members)) {
    for (t = c -> members; t -> next; t = t -> next)
      ;
    while (t -> prev) {
      t = t -> prev;
      _delete_cvar (t -> next);
    }
    _delete_cvar (t);
  }

  __xfree (MEMADDR(c));

}

CVAR *__ctalkCopyCVariable (CVAR *c) {

  CVAR *t, *p, *mbr, *mbr_ptr = NULL;  /* Avoid a warning. */

  _new_cvar (CVARREF(t));
# if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
  memcpy ((void *)t, (void *)c, (long int)&((CVAR *)0)->val);
#else
  memcpy ((void *)t, (void *)c, (int)&((CVAR *)0)->val);
#endif

  for (p = c -> members; p; p = p -> next) {
    mbr = __ctalkCopyCVariable (p);
    if (! t -> members) {
      t -> members = mbr_ptr = mbr;
    } else {
      mbr_ptr -> next = mbr;
      mbr -> prev = mbr_ptr;
      mbr_ptr = mbr_ptr -> next;
    }
  }
  return t;
}

/*
 *  If the label following an aggregate type is a method
 *  message, set the label token type to METHODMSGLABEL.
 *
 *  Call with rcvr_ptr pointing to the last token of the 
 *  aggregate variable.
 */
int __resolve_aggregate_method (MESSAGE_STACK messages, int rcvr_ptr) {

  int stack_end,
    stack_start,
    lookahead;

  stack_end = __rt_get_stack_end (messages);
  stack_start = __rt_get_stack_top (messages);

  if ((lookahead = __ctalkNextLangMsg (messages, rcvr_ptr, stack_end))
      != ERROR) {
    if (messages[lookahead] -> tokentype == LABEL) {
      if (__ctalk_isMethod_2 (messages[lookahead] -> name, 
			    messages, lookahead, stack_start)) {
	messages[lookahead] -> tokentype = METHODMSGLABEL;
	return TRUE;
      }
    }
  }
  return FALSE;
}

OBJECT *__caller_tmp_cvar_object = NULL;
OBJECT *__caller_tmp_cvar_object_ptr = NULL;

extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

OBJECT *__ctalkGetTemplateCallerCVAR (char *name) {

  RT_FN *__cfn;
  CVAR *__lcvar = NULL;  /* Avoid a warning. */
  OBJECT *__t, *__t_2;
  OBJECT *__s = NULL;
  int __calling_fn_or_method_ptr;
  METHOD *__calling_method;
  char valbuf[MAXLABEL];
  char name_wo_spaces[MAXMSG], *s, *t;

  __ctalkTemplateCallerCVARCleanup ();

  __calling_fn_or_method_ptr = __call_stack_ptr + 2;

  if (__calling_fn_or_method_ptr > MAXARGS)
    return NULL;

  if ((__call_stack_ptr < (MAXARGS - 1)) &&
      ((__cfn = __call_stack[__call_stack_ptr+2]->_rt_fn) != NULL)) {
    for (__lcvar = __cfn -> local_cvars; __lcvar && (__s == NULL); 
	 __lcvar = __lcvar -> next) {
      if (str_eq (__lcvar -> name, name)) {
	__s = __ctalkCVARReturnClass (__lcvar);
	goto found_lcvar;
      }
    }
  } else if ((__calling_method = 
	      __call_stack[__calling_fn_or_method_ptr]->method) != NULL) {
    for (__lcvar = __calling_method -> local_cvars; 
	 __lcvar; __lcvar = __lcvar -> next) {
      if (str_eq (__lcvar -> name, name)) {
	__s = __ctalkCVARReturnClass (__lcvar);
	goto found_lcvar;
      }
    }
  }

  /* Also check the name without spaces if we've squeezed them out
     when compiling */
  s = name;
  t = name_wo_spaces;
  while (*s) {
    if (isspace ((int)*s)) {
      ++s;
      continue;
    } else {
      *t++ = *s++;
    }
  }
  *t = 0;
  
  if ((__call_stack_ptr < (MAXARGS - 1)) &&
      ((__cfn = __call_stack[__call_stack_ptr+2]->_rt_fn) != NULL)) {
    for (__lcvar = __cfn -> local_cvars; __lcvar && (__s == NULL); 
	 __lcvar = __lcvar -> next) {
      if (str_eq (__lcvar -> name, name_wo_spaces)) {
	__s = __ctalkCVARReturnClass (__lcvar);
	goto found_lcvar;
      }
    }
  } else if ((__calling_method = 
	      __call_stack[__calling_fn_or_method_ptr]->method) != NULL) {
    for (__lcvar = __calling_method -> local_cvars; 
	 __lcvar; __lcvar = __lcvar -> next) {
      if (str_eq (__lcvar -> name, name_wo_spaces)) {
	__s = __ctalkCVARReturnClass (__lcvar);
	goto found_lcvar;
      }
    }
  }

  /*  If we haven't found the CVAR, compare the base name with the 
      expression if possible. This can be the case with a few subscript
      variable expressions. */

  if (strchr (name, '[')) {
    if (__cfn != NULL) {
      for (__lcvar = __cfn -> local_cvars; __lcvar && (__s == NULL); 
	   __lcvar = __lcvar -> next) {
	if (strlen (__lcvar -> name) < strlen (name)) {
	  if (!strncmp (__lcvar -> name, name, strlen (__lcvar -> name))) {
	    __s = __ctalkCVARReturnClass (__lcvar);
	    goto found_lcvar;
	  }
	}
      }
    } else if (__calling_method != NULL) {
      for (__lcvar = __calling_method -> local_cvars; 
	   __lcvar; __lcvar = __lcvar -> next) {
	if (strlen (__lcvar -> name) < strlen (name)) {
	  if (!strncmp (__lcvar -> name, name, strlen (__lcvar -> name))) {
	    __s = __ctalkCVARReturnClass (__lcvar);
	    goto found_lcvar;
	  }
	}
      }
    }
  }

  if (!__s && !__lcvar) {
    _warning ("__ctalkGetTemplateCallerCVAR: "
	      "calling method or function CVAR %s not found.\n", 
	      name);
    return NULL;
  }

 found_lcvar:

  cvar_print_val (valbuf, &(__lcvar -> val));
  /*
   *  If reference is not an object, check for a char *.
   */
  if ((__t_2 = obj_ref_str (valbuf)) != NULL) {
    __ctalkCopyObject (OBJREF(__t_2), OBJREF(__t));
  } else {
    if ((__lcvar -> type_attrs & CVAR_TYPE_CHAR) &&
 	(__lcvar -> n_derefs == 1)) {
      char *__r;
      errno = 0;
      __r = (char *)strtoul (valbuf, NULL, 16);
      if (errno != 0) {
	strtol_error (errno, "__ctalkGetTemplateCallerCVAR ()", valbuf);
      }
	  
       __t = __ctalkCreateObjectInit (__lcvar -> name, __s -> __o_name,
  				     __s -> __o_superclass -> __o_name,
  				     LOCAL_VAR,
  				     __r);
    } else {
      __t = __ctalkCreateObjectInit (__lcvar -> name, __s -> __o_name,
				     __s -> __o_superclass -> __o_name,
				     LOCAL_VAR,
				     valbuf);

    }
  }
  if (!__caller_tmp_cvar_object_ptr) {
    __caller_tmp_cvar_object = 
      __caller_tmp_cvar_object_ptr = __t;
  } else {
    __caller_tmp_cvar_object_ptr -> next = __t;
    __t -> prev = __caller_tmp_cvar_object_ptr;
    __caller_tmp_cvar_object_ptr = __t;
  }
  return __caller_tmp_cvar_object;
}

void __ctalkTemplateCallerCVARCleanup (void) {

  if (__caller_tmp_cvar_object) {
    if (__caller_tmp_cvar_object == __caller_tmp_cvar_object_ptr) {
      __ctalkDeleteObjectInternal (__caller_tmp_cvar_object);
      __caller_tmp_cvar_object = __caller_tmp_cvar_object_ptr = NULL;
    } else {
      _warning ("__ctalkTemplateCallerCVARCleanup: Multiple CVAR objects.\n");
    }
  }
}

int cvar_object_ref_cleanup (MESSAGE_STACK messages, int idx,
			     int stack_start, int stack_top) {
  MESSAGE *m_idx;
  OBJECT *__t_obj = NULL;
  m_idx = messages[idx];
  if (m_idx -> obj && (m_idx -> obj -> scope & CVAR_VAR_ALIAS_COPY)) {
    __t_obj = m_idx -> obj;
  } else {
    if (m_idx -> value_obj && 
	(m_idx -> value_obj -> scope & CVAR_VAR_ALIAS_COPY)) {
      __t_obj = m_idx -> value_obj;
    }
  }

  if (IS_OBJECT(__t_obj)) {
    if ((__t_obj == messages[stack_start] -> obj) ||
	(__t_obj == messages[stack_start] -> value_obj))
      return SUCCESS;
    if (obj_ref_str(__t_obj -> __o_value)) {
      OBJECT *__r;
      errno = 0;
      __r = (OBJECT *)strtoul(__t_obj -> __o_value, NULL, 16);
      if (errno != 0) {
	strtol_error (errno, "cvar_object_ref_cleanup (1)", 
		      __t_obj -> __o_value);
      }
      __ctalkDeleteObject(__r);
      __t_obj -> __o_value[0] = '\0';
    }
    if(__t_obj -> instancevars && 
       obj_ref_str(__t_obj->instancevars->__o_value)) {
      OBJECT *__r;
      errno = 0;
      __r = (OBJECT *)strtoul(__t_obj -> instancevars -> __o_value, NULL, 16);
      if (errno != 0) {
	strtol_error (errno, "cvar_object_ref_cleanup (2)", 
		      __t_obj -> __o_value);
      }
      __ctalkDeleteObject(__r);
      __t_obj -> instancevars ->__o_value[0] = '\0';
    }
      
    __ctalkDeleteObjectInternal (__t_obj);
  }
  return SUCCESS;
}

bool is_cvar (char *name) {
  /* called by eval_expr to check for a valid label, 
     this also checks against our ((OBJECT *)*struct) template */
  METHOD *method;
  RT_FN *rt_fn;
  CVAR *c;

  if ((method = __ctalkRtGetMethod ()) != NULL) {
    for (c = method -> local_cvars; c; c = c -> next) {
      if (strstr (c -> name, name)) {
	return true;
      }
    }
  } else if ((rt_fn = __call_stack[__call_stack_ptr+1] -> _rt_fn) != NULL) {
    for (c = rt_fn -> local_cvars; c; c = c -> next) {
      if (strstr (c -> name, name)) {
	return true;
      }
    }
  }
  return false;
}

void write_val_CVAR (OBJECT *rcvr_obj, METHOD *method) {
  CVAR *c, *__c_prev;
  int inc = 0;
  uintptr_t *mbr;
  long int *l_mbr;
  long long int *ll_mbr;
  if (__call_stack_ptr == (MAXARGS - 1)) {
    if (__call_stack[__call_stack_ptr + 1] -> _rt_fn &&
	__call_stack[__call_stack_ptr + 1] -> _rt_fn -> local_cvars) {
      for (c = __call_stack[__call_stack_ptr + 1] -> _rt_fn -> local_cvars;
	   c && c -> next; c = c -> next)
	;
      while (1) {
	__c_prev = c -> prev;
	if (str_eq (c -> name, rcvr_obj -> __o_name))
	  goto have_write_cvar;
	if (!__c_prev)
	  break;
	c = __c_prev;
      }
    }
  } else if (__call_stack[__call_stack_ptr+2]) {
    /* we use call_stack_ptr + 2 becuase we're still in the
       target method's expr_parser frame */
    if (__call_stack[__call_stack_ptr+2] -> method) {
      if (__call_stack[__call_stack_ptr+2] -> method -> local_cvars) {
	for (c = __call_stack[__call_stack_ptr + 2] -> method -> local_cvars;
	     c && c -> next; c = c -> next)
	  ;
	while (1) {
	  __c_prev = c -> prev;
	  if (str_eq (c -> name, rcvr_obj -> __o_name))
	    goto have_write_cvar;
	  if (!__c_prev)
	    break;
	  c = __c_prev;
	}
      } else if (__call_stack[__call_stack_ptr+1] -> inline_call &&
		 __call_stack[__call_stack_ptr + 1] -> method &&
		 __call_stack[__call_stack_ptr + 1] -> method -> local_cvars) {
	for (c = __call_stack[__call_stack_ptr + 1] -> method -> local_cvars;
	     c && c -> next; c = c -> next)
	  ;
	while (1) {
	  __c_prev = c -> prev;
	  if (str_eq (c -> name, rcvr_obj -> __o_name))
	    goto have_write_cvar;
	  if (!__c_prev)
	    break;
	  c = __c_prev;
	}
      }
    } else if ((__call_stack_ptr < MAXARGS)
	       && __call_stack[__call_stack_ptr + 2] -> _rt_fn) {
      if (__call_stack[__call_stack_ptr + 2] -> _rt_fn -> local_cvars) {
	for (c = __call_stack[__call_stack_ptr + 2] -> _rt_fn -> local_cvars;
	     c && c -> next; c = c -> next)
	  ;
	while (1) {
	  __c_prev = c -> prev;
	  if (str_eq (c -> name, rcvr_obj -> __o_name))
	    goto have_write_cvar;
	  if (!__c_prev)
	    break;
	  c = __c_prev;
	}
      }
    }
  }
  if (!method_arg_cvars)
    return;

  for (c = method_arg_cvars; c; c = c -> next) {
    if (str_eq (c -> name, rcvr_obj -> __o_name))
      goto have_write_cvar;
  }

  if (c == NULL)
    return;

 have_write_cvar:

  if (str_eq (method -> name, "++"))
    inc = 1;
  else if (str_eq (method -> name, "--"))
    inc = -1;

  if (c -> type_attrs & CVAR_TYPE_INT &&
      c -> type_attrs & CVAR_TYPE_LONGLONG) {
    switch (c -> n_derefs)
      {
      case 0:
	c -> val.__value.__ll += (long long int)inc;
	break;
      case 1:
	(*(long long int *)(c -> val.__value.__ptr)) += (long long int)inc;
	break;
      case 2:
	/* TODO - This should be updated to use two Symbols in the
	   object, etc.  See, CVAR_TYPE_INT, below, etc. */
	ll_mbr = (long long int *)c -> val.__value.__ptr;
	*ll_mbr += inc;
	break;
      }
  } else if (c -> type_attrs & CVAR_TYPE_INT &&
	     c -> type_attrs & CVAR_TYPE_LONG) {
    switch (c -> n_derefs)
      {
      case 0:
	c -> val.__value.__l += (long int)inc;
	break;
      case 1:
	(*(long int *)(c -> val.__value.__ptr)) += (long int)inc;
	break;
      case 2:
	l_mbr = *(long int **)c -> val.__value.__ptr;
	*l_mbr += inc;
	break;
      }
  } else if (c -> type_attrs & CVAR_TYPE_INT) {
    switch (c -> n_derefs)
      {
      case 0:
	c -> val.__value.__i += inc;
	break;
      case 1:
	(*(uintptr_t *)(c -> val.__value.__ptr)) += inc;
	break;
      case 2:
	mbr = *(uintptr_t **)c -> val.__value.__ptr;
	*mbr += inc;
	break;
      }
  }
}

extern MESSAGE *e_messages[P_MESSAGES+1]; /* Declared in rt_expr.c. */
extern int e_message_ptr;
extern EXPR_PARSER *expr_parsers[MAXARGS+1];
extern int expr_parser_ptr;


void unref_vartab_var (int *idx, CVAR *c, OBJECT *m_obj) {
  OBJECT *o_d;
  MESSAGE *m;
  int i;

  m = e_messages[*idx];
  if (c -> type_attrs & CVAR_TYPE_CHAR) {
    switch (c -> n_derefs)
      {
      case 2:
	o_d = *(OBJECT **)m_obj -> __o_value;
	  __ctalkSetObjectScope (o_d, m_obj -> scope);
	m_obj -> __o_value[0] = m_obj -> instancevars -> __o_value[0]
	  = 0;
	__ctalkDeleteObject (m_obj);
	m -> obj = o_d;
	goto remove_deref_tok;
	break;
      case 1:
	o_d = create_object_init_internal
	  (m_obj -> __o_name, rt_defclasses -> p_character_class,
	   m_obj -> scope, m_obj -> __o_value);
	__ctalkDeleteObject (m_obj);
	m -> obj = o_d;
	goto remove_deref_tok;
	break;
      case 0:
	goto remove_deref_tok;
	break;
      default:
	break;
      }
  } else if (c -> type_attrs & CVAR_TYPE_INT) {
    /* This is (so far) sufficient for any unreffing we need to
       do; i.e., we don't need to worry about width or storage
       class in the type_attrs.  For an example of where we
       didn't set them, see get_param (cvars.c). */
    switch (c -> n_derefs)
      {
      case 0:
	goto remove_deref_tok;
	break;
      case 1:
	if ((o_d = *(OBJECT **)(m_obj -> __o_value)) != NULL) {
	  __ctalkSetObjectScope (o_d, m_obj -> scope);
	  m_obj -> __o_value[0] = m_obj -> instancevars -> __o_value[0]
	    = 0;
	  __ctalkDeleteObject (m_obj);
	  m -> obj = o_d;
	}
	goto remove_deref_tok;
	break;
      case 2:
	if ((o_d = *(OBJECT **)(m_obj -> __o_value)) != NULL) {
	  __ctalkSetObjectScope (o_d, m_obj -> scope);
	  m_obj -> __o_value[0] = m_obj -> instancevars -> __o_value[0]
	    = 0;
	  __ctalkDeleteObject (m_obj);
	  m -> obj = o_d;
	}
	goto remove_deref_tok;
	break;
      }
  } else if (c -> type_attrs & CVAR_TYPE_OBJECT) {
    /* An OBJECT * can still come out of cvar_object already
       unreffed */
    switch (c -> n_derefs)
      {
      case 1:
	goto remove_deref_tok;
	break;
      case 2:
	goto remove_deref_tok;
	break;
      }

  } else if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
    /* these, too, can already be unreffed by cvar_object */
    switch (c -> n_derefs)
      {
      case 0:
	goto remove_deref_tok;
	break;
      }
  }
  printf ("ctalk: unref_vartab_var: unhandled variable: %s\n",
	  vartab_var_basename (c -> name));
  return;
  
 remove_deref_tok:
  /* jump to the label here because we want for the moment to isolate cases
     that we *do* handle */
  if (M_TOK(e_messages[*idx+1]) == ASTERISK) {
    reuse_message (e_messages[*idx+1]);
    for (i = *idx; i >= e_message_ptr; --i)
      e_messages[i+1] = e_messages[i];
    ++expr_parsers[expr_parser_ptr] -> msg_frame_top;
    ++*idx;
    ++e_message_ptr;
  }
			  
}

/***/
void cvar_for_OBJECT_deref_typecast (MESSAGE_STACK messages,
				     int open_paren_idx,
				     int terminal_mbr_idx,
				     int expr_parser_lvl) {
  char *__s, s2[MAXMSG], c2[MAXMSG];
  CVAR *c, *c_1, *c_prev;
  OBJECT *cvar_alias;
  int cvar_object_is_created, i_2;
  METHOD *m;
  __s = collect_tokens (e_messages, open_paren_idx, terminal_mbr_idx);
  remove_whitespace (__s, s2);
  if ((m = __ctalkRtGetMethod ()) != NULL) {
    c_1 = NULL;
    for (c = m -> local_cvars; c && c -> next;
	 c = c -> next)
      ;
    while (1) {
      if (c -> evaled &&
	  (expr_parser_ptr >= c -> attr_data)) {
	if (c -> prev == NULL) {
	  break;
	} else {
	  c = c -> prev;
	  continue;
	}
      }
      c_prev = c -> prev;
      remove_whitespace (c -> name, c2);
      if (str_eq (c2, s2)) {
	c_1 = c;
	break;
      } else {
	c = c_prev;
      }
    }
    
    if (c_1) {
      cvar_alias = cvar_object_mark_evaled
	(c_1, &cvar_object_is_created, expr_parser_ptr);
      __objRefCntInc (OBJREF(cvar_alias));
      for (i_2 = open_paren_idx; i_2 >= terminal_mbr_idx; --i_2) {
	if (!IS_OBJECT(e_messages[i_2] -> obj)) {
	  e_messages[i_2] -> obj = cvar_alias;
	  e_messages[i_2] -> attrs |=
	    RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS;
	}
	e_messages[i_2] -> value_obj = cvar_alias;
	e_messages[i_2] -> evaled += 1;
      }
    }
  }
  __xfree (MEMADDR(__s));
}
				     
