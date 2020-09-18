/* $Id: objtoc.c,v 1.3 2020/09/18 15:11:42 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2020 Robert Kiesling, rk3314042@gmail.com.
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

/* 
 *  Generate calls that translate objects to their C equivalents.  In
 *  the case of object that do not correspond directly to C data types,
 *  return the value pointer, or at least try to do something 
 *  intelligent.
 *
 *  In this version, C int and char types are synonymous, Float class
 *  translates to double, as in C, and a long double class isn't 
 *  implemented.
 *
 *  Long int and int are synonymous, as in GNU C for most, if not all, 
 *  platforms.
 *
 *  NOTE - Future versions might allow the class library to define
 *  what instance variable to return in a C context.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "list.h"

#define IS_SIGN(c, c2) ((c == '-' || c == '+') && \
			  isxdigit ((int)c2))

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

/*
 *  Static in case a function needs to do a conversion independent
 *  of its value.
 */
static void *valptr;

#define MAX_RETURN_VALPTRS MAXARGS
static LIST *return_valptrs = NULL;
static LIST *return_valptrs_head = NULL;
static int n_return_vals = 0;

static char *buf_from_return_list (int length) {
  LIST *l, *t;
  l = new_list ();
  l -> data = __xalloc (length * 2);
  if (return_valptrs == NULL) {
    return_valptrs = return_valptrs_head = l;
  } else {
    if (n_return_vals >= MAX_RETURN_VALPTRS) {
      t = list_unshift (&return_valptrs);
      return_valptrs = t -> next;
      --n_return_vals;
      __xfree ((void **)MEMADDR(t -> data));
      __xfree (MEMADDR(t));
    }
    list_add (return_valptrs_head, l);
    return_valptrs_head = l;
  }
  ++n_return_vals;
  return (char *)l -> data;
}

void delete_return_valptrs (void) {
  delete_list (&return_valptrs);
}

#define RETURN_BUF_SIZEE 0xffff
#define RETURN_BUF_ENTRY_MAX 0x8888   /* might need tuning */

static char return_buf[RETURN_BUF_SIZEE];
static int return_buf_ptr = 0;
static int prev_s_len = 0;

static void buffer_return_string (char *s) {
  int s_len = strlen (s);
  return_buf_ptr += prev_s_len;
  if (return_buf_ptr + 1 + s_len >= RETURN_BUF_SIZEE) {
    return_buf_ptr = 0;
  } else {
    /* Don't increment if we're just starting. */
    if (return_buf_ptr != 0) {
      ++return_buf_ptr;
      return_buf[return_buf_ptr] = '\0';
    }
  }
  strcpy (&return_buf[return_buf_ptr], s);
  prev_s_len = s_len;
}

char __ctalk_to_c_char (OBJECT *o) {
  char c;
  OBJECT *o_value;
  char buf[MAXLABEL];

  if (!o) {
#ifdef WARN_UNDEFINED_OBJECT_RETURNS
    _warning ("__ctalk_to_c_char: Undefined value.\n");
#endif
    return '\0';
  }

  if ((o_value = o ->  instancevars) != NULL) {
    strcpy (buf, o_value -> __o_value);
    while ((*buf == '\'') && (buf[1] != '\0'))
      TRIM_CHAR (buf);
    if (strlen (buf) > 1) { /* Convert from Integer if possible. */
      c = (char)atoi (buf);
    } else {
      c = buf[0];
    }
  } else if (o -> attrs & OBJECT_IS_VALUE_VAR) {
    if (o -> attrs & OBJECT_VALUE_IS_BIN_INT) {
      c = (char) (*(int *)o -> __o_value);
    } else if (o -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG) {
      c = (char) (*(long long int *)o -> __o_value);
    } else {
      strcpy (buf, o -> __o_value);
      while ((*buf == '\'') && (buf[1] != '\0'))
	TRIM_CHAR (buf);
      if (strlen (buf) > 1) { /* Convert from Integer if possible. */
	c = (char)atoi (buf);
      } else {
	c = buf[0];
      }
    }
  } else {

    if (o -> __o_value) {
      strcpy (buf, o -> __o_value);
    } else {
      strcpy (buf, NULLSTR);
    }

    while ((*buf == '\'') && (buf[1] != '\0'))
    while (*buf == '\'')
      TRIM_CHAR (buf);
    c = buf[0];
  }
  return c;
}

static inline void double_cond_delete (OBJECT *o) {

  if (object_is_decrementable (o, NULL, NULL) != 0) 
    __ctalkDeleteObjectInternal (o -> __o_p_obj ? o -> __o_p_obj : o);

  if (o -> nrefs == 0) {
    OBJECT *o_p = o -> __o_p_obj ? o -> __o_p_obj : o;
    __ctalkDeleteObject (o_p);
  }

  if (IS_OBJECT(o) && (o -> attrs & OBJECT_IS_I_RESULT)) {
    __ctalkDeleteObject (o);
  }

  if (IS_OBJECT(o) && (o -> attrs & OBJECT_IS_NULL_RESULT_OBJECT)) {
    if ((o -> __o_p_obj == NULL) && IS_EMPTY_VARTAG(o -> __o_vartags)) {
      __ctalkDeleteObject (o);
    }
  }
}

double __ctalk_to_c_double (OBJECT *o) {
  double d;
  OBJECT *o_value;

  if (!IS_OBJECT(o)) {
#ifdef WARN_UNDEFINED_OBJECT_RETURNS
    _warning ("__ctalk_to_c_double: Undefined value.\n");
#endif
    return 0.0;
  }

  if (o -> attrs & OBJECT_VALUE_IS_BIN_INT) {
    if (IS_OBJECT(o -> instancevars)) {
      d = (double) (*(int *)o -> instancevars -> __o_value);
    } else {
      d = (double) (*(int *)o -> __o_value);
    }
    double_cond_delete (o);

    return d;
  } else if (o -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG) {
    if (IS_OBJECT(o -> instancevars)) {
      d = (double) LLVAL(o -> instancevars -> __o_value);
    } else {
      d = (double) LLVAL(o -> __o_value);
    }
    double_cond_delete (o);

    return d;
  }

  if ((o_value = o -> instancevars) != NULL)
    d = strtod (o_value -> __o_value, NULL);
  else
    d = strtod (o -> __o_value, NULL);

  double_cond_delete (o);

  return d;
}

/* The double conversions don't generally need a, "keep" parameter,
   because they're not used in conditional predicates, unlike the
   int conversion functions.  That could change, of course. */
double __ctalkToCDouble (OBJECT *o) {
  return __ctalk_to_c_double (o);
}

int __ctalk_to_c_int (OBJECT *o) {
  int i = 0;
  char c;
  OBJECT *o_value;
  char buf[MAXLABEL];

  if (!o) {
#ifdef WARN_UNDEFINED_OBJECT_RETURNS
    _warning ("__ctalk_to_c_int: Undefined value.\n");
#endif
    return 0;
  }

  if (o -> attrs & OBJECT_VALUE_IS_BIN_INT) {
    if (IS_OBJECT(o -> instancevars)) {
      return *(int *)o -> instancevars -> __o_value;
    } else {
      return *(int *)o -> __o_value;
    }
  } else if (o -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG) {
    if (IS_OBJECT(o -> instancevars)) {
      return (int)LLVAL(o -> instancevars -> __o_value);
    } else {
      return (int)LLVAL(o -> __o_value);
    }
  }

  if ((o_value = o -> instancevars) != NULL) {
    strcpy (buf, o_value -> __o_value);
  } else {
    strcpy (buf, o -> __o_value);
    o_value = o;
  }

  while (*buf == '\'')
    TRIM_CHAR (buf);

  if (is_class_or_subclass (o_value, rt_defclasses -> p_integer_class)) {
#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
    if (isdigit ((int)buf[0]) || IS_SIGN(buf[0], buf[1])) {
#else
    if (isdigit (buf[0]) || IS_SIGN(buf[0], buf[1])) {
#endif
      switch (radix_of (buf))
	{
	case hexadecimal:
	  i = strtol (buf, NULL, 16);
	  break;
	case octal:
	  i = strtol (buf, NULL, 8);
	  break;
	case binary:
	  i = ascii_bin_to_dec (buf);
	  break;
	case decimal:
	default:
	  i = strtol (buf, NULL, 0);
	  break;
	}
    } else {
      if (str_eq (buf, NULLSTR)) {
	i = 0;
      } else {
	if (*buf == '\0') {
	  i = 0;
	} else {
	  _warning ("__ctalk_to_c_int: Incompatible return type.\n");
	}
      }
    }
  } else {
      if (o_value -> __o_class == rt_defclasses -> p_character_class) {
      if (str_eq (buf, NULLSTR)) {
	i = 0;
      } else {
	i = (int)buf[0];
      }
    } else {
      /*
       *  Any other class.
       */
      if (EMPTY_STR(buf) || str_eq (buf, NULLSTR)) {
	i = 0;
      } else {
	if ((strstr (buf, "ll") != NULL) || (strstr (buf, "L")  != NULL)) {
	  long long int l;
	  i = (int)strtoll (buf, NULL, 0);
	} else {
	  if (strstr (buf, "l")) {
	    long int l;
	    i = (int)strtol (buf, NULL, 0);
	  } else {
	    i = strtol (buf, NULL, 10);
	  }
	}
      }
    }
  } 

  return i;
}

static inline void int_cond_delete (OBJECT *o, int keep) {

  if (!(keep & OBJTOC_OBJECT_KEEP))
    if (object_is_decrementable (o, NULL, NULL) != 0)
      __ctalkDeleteObjectInternal (o -> __o_p_obj ? o -> __o_p_obj : o);
  /* Fall through. */

  if (IS_OBJECT(o)) {
    if (o -> nrefs == 0) {
      OBJECT *o_p = o -> __o_p_obj ? o -> __o_p_obj : o;
      __ctalkDeleteObject (o_p);
      o_p = NULL;
    } else if (IS_OBJECT(o) && (o -> attrs & OBJECT_IS_I_RESULT)) {
      __ctalkDeleteObject (o);
      o = NULL;
    } else if (IS_OBJECT(o) && (o -> attrs & OBJECT_IS_NULL_RESULT_OBJECT)) {
      if ((o -> __o_p_obj == NULL) && IS_EMPTY_VARTAG(o -> __o_vartags)) {
	__ctalkDeleteObject (o);
	o = NULL;
      }
    } else if (IS_OBJECT(o) && (o -> scope & CVAR_VAR_ALIAS_COPY) &&
	       (o -> nrefs == 1)) {
      __ctalkDeleteObject (o);
      o = NULL;
    }
  }
  if (keep & OBJTOC_DELETE_CVARS)
    delete_method_arg_cvars ();
}

int __ctalkToCInteger (OBJECT *o, int keep) {
  int i;
#ifdef __x86_64
  long long int li;
#endif  
  char c, *c_i;
  OBJECT *o_value;
  char buf[MAXLABEL];

  if (!IS_OBJECT(o)) {
#ifdef WARN_UNDEFINED_OBJECT_RETURNS
    _warning ("__ctalkToCInteger: Invalid object.\n");
#endif
    return 0;
  }

  if ((o -> attrs & OBJECT_VALUE_IS_BIN_INT) ||
      (o -> attrs & OBJECT_VALUE_IS_BIN_BOOL)) {
    o_value = IS_OBJECT(o -> instancevars) ? o -> instancevars : o;
    i = * (int *)o_value -> __o_value;
    int_cond_delete (o, keep);
    return i;
  } else if (o -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG) {
    o_value = IS_OBJECT(o -> instancevars) ? o -> instancevars : o;
    if (LLVAL(o_value -> __o_value) > INT32_MAX) {
      _warning ("ctalk: LongInteger value %s truncated to int.\n",
		o -> __o_name);
    }
    i = (int )LLVAL(o_value -> __o_value);
    int_cond_delete (o, keep);
    return i;
  } else if (o -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
    o_value = IS_OBJECT(o -> instancevars) ? o -> instancevars : o;
    i = (int )SYMVAL(o_value -> __o_value);
    int_cond_delete (o, keep);
    return i;
  }

  c_i = (char *)active_i (o);

  o_value = IS_OBJECT(o -> instancevars) ? o -> instancevars : o;

  /* Anything that has instance variables that ends up here
     should evaluate to true. */
  if (IS_OBJECT(o_value -> next))
    return 1;

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
  if (isdigit ((int)o_value -> __o_value[0]) || 
      IS_SIGN(o_value -> __o_value[0], o_value -> __o_value[1])) {
#else
  if (isdigit (o_value -> __o_value[0]) || 
      IS_SIGN(o_value -> __o_value[0], o_value -> __o_value[1])) {
#endif
    errno = 0;
    switch (radix_of (o_value -> __o_value))
      {
      case hexadecimal:
	i = strtol (o_value -> __o_value, NULL, 16);
	break;
      case octal:
	i = strtol (o_value -> __o_value, NULL, 8);
	break;
      case binary:
	i = ascii_bin_to_dec (o_value->__o_value);
	break;
      case decimal:
      default:
	i = strtol (o_value -> __o_value, NULL, 10);
	break;
      }

    switch (errno)
      {
      case EINVAL:
	_warning ("__ctalkToCInteger (): Invalid number %s (Object %s).\n",
		  o_value -> __o_value, o -> __o_name);
	break;
      case ERANGE:
	_warning ("__ctalkToCInteger (): Value %s is out of range. (Object %s).\n",
		  o_value -> __o_value, o -> __o_name);
	break;
      default:
	break;
      }

  } else {
    if (o_value -> __o_value[0] == '\'') {
      /* I.e., a Character constant. */
      
      if (o_value -> __o_value) {
	strcpy (buf, o_value -> __o_value);
      } else {
	int_cond_delete (o, keep);
	return FALSE;
      }

      if (*buf == '\'') {
	i = __ctalkIntFromCharConstant (buf);
      } else {
	i = (int)buf[0];
      }

    } else {
      if (c_i != I_UNDEF) {
	if (str_eq (c_i, NULLSTR) || 
	    str_eq (c_i, "0x0")) {
	  i = 0;
	} else {
	  if (c_i == NULL || *c_i == '\0') {
	    i = 0;
	  } else {
	    /* Anything else - don't leave i at some random value. */
#ifdef __x86_64
	    li = (long long int)c_i; /* avoids compiler warnings mainly */
	    i = (int)li;
#else	    
	    i = (int)c_i;
#endif	    
	  }
	} 
      } else { /* if (c_i != I_UNDEF) */
	if (str_eq (o_value -> __o_value, NULLSTR) || 
	    str_eq (o_value -> __o_value, "0x0")) {
	  i = 0;
	} else {
	  if (o_value -> __o_value[0] == '\0') {
	    i = 0;
	  } else {
	    /* Anything else - don't leave i at some random value. */
	    i = (int)o_value -> __o_value[0];
	  }
	} 
      } /*  if (c_i != I_UNDEF) */
    }
  }

  int_cond_delete (o, keep);

  return i;
}

long int __ctalkToCLongInteger (OBJECT *o, int keep) {
  long int i;
  char c, *c_i;
  OBJECT *o_value;
  char buf[MAXLABEL];

  if (!IS_OBJECT(o)) {
#ifdef WARN_UNDEFINED_OBJECT_RETURNS
    _warning ("__ctalkToCInteger: Invalid object.\n");
#endif
    return 0;
  }

  c_i = (char *)active_i (o);

  o_value = IS_OBJECT(o -> instancevars) ? o -> instancevars : o;

  if (o_value -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
    i = *(long int *)o_value -> __o_value;
    return i;
  } else if (o_value -> attrs & OBJECT_VALUE_IS_BIN_INT) {
    i = INTVAL(o_value -> __o_value);
    return i;
  }

  /* Anything that has instance variables that ends up here
     should evaluate to true. */
  if (IS_OBJECT(o_value -> next))
    return 1;

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
  if (isdigit ((int)o_value -> __o_value[0]) || 
      IS_SIGN(o_value -> __o_value[0], o_value -> __o_value[1])) {
#else
  if (isdigit (o_value -> __o_value[0]) || 
      IS_SIGN(o_value -> __o_value[0], o_value -> __o_value[1])) {
#endif
    errno = 0;
    switch (radix_of (o_value -> __o_value))
      {
      case hexadecimal:
	i = strtol (o_value -> __o_value, NULL, 16);
	break;
      case octal:
	i = strtol (o_value -> __o_value, NULL, 8);
	break;
      case binary:
	i = ascii_bin_to_dec (o_value->__o_value);
	break;
      case decimal:
      default:
	i = strtol (o_value -> __o_value, NULL, 10);
	break;
      }

    switch (errno)
      {
      case EINVAL:
	_warning ("__ctalkToCInteger (): Invalid number %s (Object %s).\n",
		  o_value -> __o_value, o -> __o_name);
	break;
      case ERANGE:
	_warning ("__ctalkToCInteger (): Value %s is out of range. (Object %s).\n",
		  o_value -> __o_value, o -> __o_name);
	break;
      default:
	break;
      }

  } else {
    if (o_value -> __o_value[0] == '\'') {
      /* I.e., a Character constant. */
      
      if (o_value -> __o_value) {
	strcpy (buf, o_value -> __o_value);
      } else {
	int_cond_delete (o, keep);
	return FALSE;
      }

      if (*buf == '\'') {
	i = (long int)__ctalkIntFromCharConstant (buf);
      } else {
	i = (long int)buf[0];
      }

    } else {
      if (c_i != I_UNDEF) {
	if (str_eq (c_i, NULLSTR) || 
	    str_eq (c_i, "0x0")) {
	  i = 0l;
	} else {
	  if (c_i == NULL || *c_i == '\0') {
	    i = 0l;
	  } else {
	    /* Anything else - don't leave i at some random value. */
	    i = (long int)c_i;
	  }
	} 
      } else { /* if (c_i != I_UNDEF) */
	if (str_eq (o_value -> __o_value, NULLSTR) || 
	    str_eq (o_value -> __o_value, "0x0")) {
	  i = 0l;
	} else {
	  if (o_value -> __o_value[0] == '\0') {
	    i = 0l;
	  } else {
	    /* Anything else - don't leave i at some random value. */
	    i = (long int)o_value -> __o_value[0];
	  }
	} 
      } /*  if (c_i != I_UNDEF) */
    }
  }

  /* int_cond_delete works here, too */
  int_cond_delete (o, keep);

  return i;
}

static inline void longlong_int_cond_delete (OBJECT *o, int keep) {
  if (object_is_decrementable (o, NULL, NULL) != 0) 
    __ctalkDeleteObjectInternal (o -> __o_p_obj ? o -> __o_p_obj : o);

  if (o -> nrefs == 0) {
    OBJECT *o_p = o -> __o_p_obj ? o -> __o_p_obj : o;
    __ctalkDeleteObject (o_p);
  }

  if (IS_OBJECT(o) && (o -> attrs & OBJECT_IS_I_RESULT)) {
    __ctalkDeleteObject (o);
  }

  if (IS_OBJECT(o) && (o -> attrs & OBJECT_IS_NULL_RESULT_OBJECT)) {
    if ((o -> __o_p_obj == NULL) && IS_EMPTY_VARTAG(o -> __o_vartags)) {
      __ctalkDeleteObject (o);
    }
  }
  if (keep & OBJTOC_DELETE_CVARS)
    delete_method_arg_cvars ();
}

long long int __ctalk_to_c_longlong (OBJECT *o, int keep) {
  long long int l = 0ll;
  int i = 0;
  OBJECT *o_value, *o_l;
  if (!o) {
#ifdef WARN_UNDEFINED_OBJECT_RETURNS
    _warning ("__ctalk_to_c_longlong: Undefined value.\n");
#endif
    return 0LL;
  }
  if ((o_value = o -> instancevars) != NULL) {
    o_l = o_value;
  } else {
    o_l = o;
  }

  if (o_l -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG) {
    l = LLVAL(o_l -> __o_value);
    longlong_int_cond_delete (o, keep);
    return l;
  } else if (o_l -> attrs & OBJECT_VALUE_IS_BIN_INT) {
    i = INTVAL(o_l -> __o_value);
    longlong_int_cond_delete (o, keep);
    return (long long)i;
  } else if (o_l -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
    l = (long long)*(OBJECT **)o_l -> __o_value;
    longlong_int_cond_delete (o, keep);
    return l;
  }

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
  if (isdigit ((int)o_l -> __o_value[0]) || 
      IS_SIGN(o_l -> __o_value[0], o_l -> __o_value[1])) {
#else
  if (isdigit (o_l -> __o_value[0]) || 
      IS_SIGN(o_l -> __o_value[0], o_l -> __o_value[1])) {
#endif
    l = strtoll (o_l -> __o_value, NULL, 0);
  } else {
    if (str_eq (o_l  ->  __o_value, NULLSTR)) {
      l = 0ll;
    } else {
      if (*(o_l -> __o_value) == '\0') {
	l = 0ll;
      } else {
	_warning ("__ctalk_to_c_longlong: Incompatible return type %s.\n", 
		  o_l -> __o_value);
	__ctalkPrintExceptionTrace ();
      }
    }
  }

  longlong_int_cond_delete (o, keep);

  return l;
}

#ifndef STR_0XHEX_TO_PTR
#if defined (__APPLE__) && defined (__ppc__)
#define STR_0XHEX_TO_PTR(__x) "0x%p",((void *)__x)
#else
#define STR_0XHEX_TO_PTR(__x) "0x%p",((void **)__x)
#endif
#endif

char *__ctalk_to_c_char_ptr (OBJECT *o) {
  OBJECT *o_value, *o_return;
  if (!IS_OBJECT (o)) return NULL;
  if ((o_value = o -> instancevars) != NULL) {
    if (o_value -> __o_value) {

      if (o_value -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
	return (char *)SYMVAL(o_value -> __o_value);
      } else if ((o_return = obj_ref_str(o_value -> __o_value)) != NULL) {
	return (char *)o_return;
      } else {
	return o_value -> __o_value;
      }

    } else {

      return NULL;

    }
  } else { 

    if (o -> __o_value) {
      if (o -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
	return *(char **)o -> __o_value;
      } else if ((valptr = generic_ptr_str (o -> __o_value)) != NULL) {
	return (char *)valptr;
      } else {
	return o -> __o_value;
      }
    } else {

      return NULL;

    }
  }
}

static void __delete_char_ptr_arg (OBJECT *__o, int keep) {
  OBJECT *__t;
  if (keep == False) {
    if (((__t = __ctalk_get_object(__o->__o_name, __o->CLASSNAME)) 
	 != __o) &&
	!__ctalkFindClassVariable (__o->__o_name, FALSE) &&
	!is_receiver (__o) &&
	!is_arg (__o)) {
      if (__o -> __o_p_obj) {
	if (((__t = __ctalk_get_object
	      (__o->__o_p_obj->__o_name, __o->__o_p_obj->CLASSNAME)) 
	     != __o -> __o_p_obj) &&
	    !__ctalkFindClassVariable (__o->__o_p_obj->__o_name, FALSE) &&
	    !is_receiver (__o->__o_p_obj) &&
	    !is_arg (__o->__o_p_obj)) {
	  __ctalkDeleteObjectInternal (__o -> __o_p_obj ? __o -> __o_p_obj : 
				       __o);
	}
      }
    }

    if (object_is_decrementable (__o, NULL, NULL) != 0) 
      __ctalkDeleteObjectInternal (__o -> __o_p_obj ? __o -> __o_p_obj : __o);

    if (__o -> nrefs == 0) {
      OBJECT *o_p = __o -> __o_p_obj ? __o -> __o_p_obj : __o;
      __ctalkDeleteObject (o_p);
    }
  }

  if (__o -> attrs == OBJECT_IS_I_RESULT) {
    if (!(__o -> scope & METHOD_USER_OBJECT) && !is_arg (__o)) { 
      __ctalkDeleteObject (__o);
    }
  } else if (__o -> scope == CREATED_CVAR_SCOPE) { /***/
    if (!IS_OBJECT(__o -> __o_p_obj) && !is_arg (__o) && !is_receiver (__o)) {
      __ctalkDeleteObject (__o);
    }
  }
}

#if 0
/* this might still be useful. */
 static bool has_LITERAL_attr (OBJECT *o) {
  OBJECT *o_target;
  o_target = o -> attrs & OBJECT_IS_VALUE_VAR ?
    o -> __o_p_obj : o;
  if (o_target -> attrs & OBJECT_IS_STRING_LITERAL)
    return true;
  else
    return false;
}
#endif
 
char *__ctalkToCCharPtr (OBJECT *o, int keep) {
  OBJECT *o_value, *o_return;
  char *v, *c, *i_obj;
  char ibuf[64];

  c = (char *)I_UNDEF;
  /*
   *  Returning an empty string helps prevent segfaults when
   *  the C library is expecting a char *, like when this
   *  fn is used in a printf () argument.
   */
  if (!IS_OBJECT (o)) return "";
  if ((o_value = o -> instancevars) != NULL) {
    if (o_value -> __o_value != NULL) {

      if ((o_value -> __o_class != rt_defclasses -> p_string_class) &&
	  ((o_return = obj_ref_str(o_value -> __o_value)) != NULL)) {
	__delete_char_ptr_arg (o, keep);
	return (char *)o_return;
      } else {
	if (IS_OBJECT (o_value) && (o_value -> __o_value != NULL)) {
#ifdef RETURN_VALPTRS
	  v = buf_from_return_list (strlen (o_value -> __o_value));
	    strcpy (v, o_value -> __o_value);
	    __delete_char_ptr_arg (o, keep);
	    return v;
#else
	    if (o_value -> attrs & OBJECT_VALUE_IS_BIN_INT) {
	      v = buf_from_return_list (32);
	      if (*(int *)o_value -> __o_value) {
		sprintf (v, "%d", *(int *)o_value -> __o_value);
	      } else {
		v[0] = '0'; v[1] = '\0';
	      }
	      return v;
	    } else if (strlen (o_value -> __o_value) > RETURN_BUF_ENTRY_MAX) {
	      v = buf_from_return_list (strlen (o_value -> __o_value));
	      strcpy (v, o_value -> __o_value);
	      __delete_char_ptr_arg (o, keep);
	      return v;
	    } else if (o_value -> attrs & OBJECT_VALUE_IS_BIN_INT) {
	      v = buf_from_return_list (32);
	      if (*(int *)o_value -> __o_value) {
		sprintf (v, "%d", *(int *)o_value -> __o_value);
	      } else {
		v[0] = '0'; v[1] = '\0';
	      }
	      return v;
	    } else {
	      if (o_value -> __o_value == NULL) {
		buffer_return_string (NULLSTR);
	      } else {
		if ((i_obj = (char *)active_i (o)) != (char *)I_UNDEF) {
		  buffer_return_string (i_obj);
		} else {
		  buffer_return_string (o_value -> __o_value);
		}
	      }
	      __delete_char_ptr_arg (o, keep);
	      return &return_buf[return_buf_ptr];
	    }
#endif
	} else {
	  return NULL;
	}
      }
    } else {
      return NULL;
    }
  } else { 

    if (((o_return = obj_ref_str(o -> __o_value)) != NULL) &&
	!(o -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) &&
	!(o -> attrs & OBJECT_VALUE_IS_BIN_INT) &&
	(o_value -> __o_class != rt_defclasses -> p_string_class)) {
      __delete_char_ptr_arg (o, keep);
      return (char *)o_return;
    } else {
      __delete_char_ptr_arg (o, keep);
      if (o -> __o_value) {
#ifdef RETURN_VALPTRS
	if (o -> attrs & OBJECT_VALUE_IS_BIN_INT) {
	  v = buf_from_return_list (32);
	  if (*(int *)o -> __o_value) {
	    sprintf (v, "%d", *(int *)o -> __o_value);
	  } else {
	    v[0] = '0'; v[1] = '\0';
	  }
	  return v;
	} else if (o -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
	  v = buf_from_return_list (32);
	  if (*(int *)o -> __o_value) {
	    sprintf (v, "%p", (void *)*(uintptr_t *)o -> __o_value);
	  } else {
	    v[0] = '0'; v[1] = '\0';
	  }
	  return v;
	} else {
	  v = buf_from_return_list (strlen (o -> __o_value));
	  strcpy (v, o -> __o_value);
	  return v;
	}
#else
	if (o -> attrs & OBJECT_VALUE_IS_BIN_INT) {
	  char ibuf[64];
	  sprintf (ibuf, "%d", *(int *)o -> __o_value);
	  buffer_return_string (ibuf);
	  return &return_buf[return_buf_ptr];
	} else if (o -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
	  sprintf (ibuf, "%p", (void *)*(uintptr_t *)o -> __o_value);
	  buffer_return_string (ibuf);
	  return &return_buf[return_buf_ptr];
	} else if (o -> attrs & OBJECT_VALUE_IS_BIN_BOOL) {
	  if (*(int *)o -> __o_value) {
	    ibuf[0] = '1'; ibuf[1] = '\0';
	  } else {
	    strcpy (ibuf, "0x0");
	  }
	  buffer_return_string (ibuf);
	  return &return_buf[return_buf_ptr];
	} else {
	  buffer_return_string (o -> __o_value);
	  return &return_buf[return_buf_ptr];
	}
#endif
      } else {
	return NULLSTR;
      }
    }
  }
}

/* 
 * Remember, we're returning a pointer here. 
 * TODO - there might be a warning here if the
 * function returns a scalar as a pointer.
 *
 * TODO - Do this for all subclasses of Magnitude,
 * works for String class, too.
 */

void *__ctalk_to_c_ptr (OBJECT *o) {

  OBJECT *o_value;
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
  unsigned long long int retval = 0;
#else
  unsigned long int retval = 0;
#endif
  RADIX radix;
  char ptr_buf[64];
  void *c;

  if (!IS_OBJECT (o)) return NULL;

  if (IS_VALUE_INSTANCE_VAR(o -> instancevars))
    o_value = o -> instancevars;
  else
    o_value = o;

  if ((c = active_i (o)) != I_UNDEF)
    return c;

  if (o_value -> attrs & OBJECT_VALUE_IS_C_CHAR_PTR_PTR) {
    if (o_value -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
      return (void *)*(uintptr_t *)o_value -> __o_value;
    }
  } else if (o_value -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
    if (o_value -> attrs & OBJECT_VALUE_IS_MEMORY_VECTOR) {
      return (void *)o_value -> __o_value;
    } else {
      return (void *) SYMVAL(o_value -> __o_value);
    }
  } else if (o -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) {
    return NULL;
  }

  radix = radix_of (o_value -> __o_value);
  if ((int)radix == -1) {
    if (str_eq (o_value -> __o_class -> __o_name, STRING_CLASSNAME)) {
      return (void *)o_value -> __o_value;
    } else if (str_eq (o_value -> __o_class -> __o_name, VECTOR_CLASSNAME)) {
      return (void *)o_value -> __o_value;
    } else if (o_value -> attrs & OBJECT_VALUE_IS_BIN_INT) {
      return (void *)(*(uintptr_t *)o_value -> __o_value);
    } else if (str_eq (o_value -> __o_value, NULLSTR)) {
      return NULL;
    } else if (o -> scope & LOCAL_VAR) {
      return o;
    } else {
      _error ("ctalk: Unrecognized numeric value in radix_of: %s.\n",
	      o_value -> __o_value);
    }
  }

  switch (radix)
    {
    case hexadecimal:
      if (!chkptrstr (o_value -> __o_value) && 
	  !str_eq (o_value -> __o_value, "0x0")) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
	retval = (unsigned long long int)&(o_value -> __o_value[0]);
#else
	retval = (unsigned long int)&(o_value -> __o_value[0]);
#endif
      } else {
	retval = (unsigned long long int)strtoll 
	  (o_value -> __o_value, NULL, 16);
      }
      break;
    default:
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      retval = (unsigned long long int)&(o_value -> __o_value[0]);
#else
      retval = (unsigned long int)&(o_value -> __o_value[0]);
#endif
      break;
    }

  return (void *)retval;
}

void *__ctalk_to_c_ptr_u (OBJECT *o) {

  OBJECT *o_value;
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
  unsigned long long int retval = 0;
#else
  unsigned long int retval = 0;
#endif
  RADIX radix;
  char ptr_buf[64];
  void *c;

  if (!IS_OBJECT (o)) return NULL;

  if (IS_VALUE_INSTANCE_VAR(o -> instancevars))
    o_value = o -> instancevars;
  else
    o_value = o;

  if ((c = active_i (o)) != I_UNDEF)
    return c;

  radix = radix_of (o_value -> __o_value);
  switch (radix)
    {
    case hexadecimal:
      if (str_eq (o_value -> __o_value, NULLSTR) ||
	  str_eq (o_value -> __o_value, "0") ||
	  str_eq (o_value -> __o_value, "0x0"))  {
	retval = 0;
      } else {
	retval = (unsigned long long int)strtoll 
	  (o_value -> __o_value, NULL, 16);
      }
      break;
    default:
      if (o_value -> __o_value[0] == '\0') {
	retval = 0;
      } else  {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
	retval = (unsigned long long int)&(o_value -> __o_value[0]);
#else
	retval = (unsigned long int)&(o_value -> __o_value[0]);
#endif
      }
      break;
    }

  return (void *)retval;
}

void *__ctalkToCArrayElement (OBJECT *o) {
  OBJECT *value_var;

  if (!IS_OBJECT(o))
    return NULL;

  if ((value_var = o -> instancevars) == NULL)
    value_var = o;

  if (is_class_or_subclass (o, rt_defclasses -> p_character_class) &&
      !is_class_or_subclass (o, rt_defclasses -> p_string_class)) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
    long int c;
#else
    int c;
#endif
    void *char_value;
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
    c = (long int)__ctalk_to_c_char (value_var);
#else
    c = (int)__ctalk_to_c_char (value_var);
#endif
    char_value = (void *)c;
    return char_value;
  }
  /*
   *  Doubles are not compatible here.
   */

      if (is_class_or_subclass (o, rt_defclasses -> p_float_class)) {
    _warning ("warning: Float array element returned as char pointer.\n");
    return value_var -> __o_value;
  }

    if (is_class_or_subclass (o, rt_defclasses -> p_integer_class)) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
    long int i;
#else
    int i;
#endif
    void *int_val;
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
    i = (long int)__ctalk_to_c_int (value_var);
#else
    i = __ctalk_to_c_int (value_var);
#endif
    int_val = (void *)i;
    return int_val;
  }

    if (is_class_or_subclass (o, rt_defclasses -> p_longinteger_class)) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
    long int l;
#else
    int l;
#endif
    void *ll_val;
    l = __ctalk_to_c_longlong (value_var, TRUE);

#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
    if (l > LONG_MAX) {
      _warning
	("warning: LongInteger array element truncated to long int.\n");
    }
    ll_val = (void *)(l & LONG_MAX);
#else
    if (l > INT_MAX) {
      _warning
	("warning: LongInteger array element truncated to int.\n");
    }
    ll_val = (void *)(l & INT_MAX);
#endif
    return (void *)ll_val;
  }

    if (is_class_or_subclass (o, rt_defclasses -> p_string_class))
    return __ctalk_to_c_char_ptr (value_var);

  /* TODO - We could do this a lot more quickly in conditionals. */
    if (is_class_or_subclass (o, rt_defclasses -> p_boolean_class)) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
    long int l;
#else
    int l;
#endif
    if (strstr (value_var -> __o_value, "L") || 
	strstr (value_var -> __o_value, "ll")) {
      void *ll_val;
      l = __ctalk_to_c_longlong (value_var, TRUE);
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      ll_val = (void *)(l & LONG_MAX);
#else
      ll_val = (void *)(l & INT_MAX);
#endif
      return (void *)ll_val;
    } else {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      long int i;
#else
      int i;
#endif
      void *int_val;
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
      i = (long int)__ctalk_to_c_int (value_var);
#else
      i = __ctalk_to_c_int (value_var);
#endif
      int_val = (void *)i;
      return int_val;
    }
  }


  return value_var -> __o_value;
}

char *__ctalkArrayElementToCCharPtr (OBJECT *o) {
  static char *s;
  s = (char *)__ctalkToCArrayElement (o);
  return s;
}

char __ctalkArrayElementToCChar (OBJECT *o) {
  static char c;
  int i;
#ifdef __x86_64
  long long int li;
#endif  

#ifdef __x86_64
  li = (long long int)__ctalkToCArrayElement (o);
  c = (char)li;
#else
  i = (int)__ctalkToCArrayElement (o);
  c = (char)i;
#endif  
  return c;
}

void *__ctalkArrayElementToCPtr (OBJECT *o) {
  return __ctalkToCArrayElement (o);
}

double __ctalkArrayElementToCDouble (OBJECT *o) {
  static double d = 0.0f;
  OBJECT *o_value;
  int r = 0;

  if (is_class_or_subclass (o, rt_defclasses -> p_float_class)) {
    if ((o_value = __ctalkGetInstanceVariableByName 
	 (o -> __o_name, "value", FALSE))
	!= NULL) {

      if (o_value -> __o_value) {
	d = strtod (o_value -> __o_value, NULL);
      } else {
	d = strtod (o -> __o_value, NULL);
      }
    } else {

      d = 0.0f;

    }
  }
  
  if (r == 0) {
    _warning("__ctalkArrayElementToCDouble: Object, \"%s,\""
	     "has invalid value.\n");
  }

  return d;
}

int __ctalkArrayElementToCInt (OBJECT *o) {
  static int i;
#ifdef __x86_64
  long long int li;
#endif  

#ifdef __x86_64
  li = (long long int)__ctalkToCArrayElement (o);
  i = (int)li;
#else  
  i = (int)__ctalkToCArrayElement (o);
#endif  
  return i;
}

#ifdef __x86_64
long long int __ctalkArrayElementToCLongLongInt (OBJECT *o) {
  return (long long int)__ctalkToCArrayElement (o);
}
#else 
 long long int __ctalkArrayElementToCLongLongInt (OBJECT *o) {
  long long int l;
  int i;
  i = (int)__ctalkToCArrayElement (o);
  l = (long long int)i;
  return l;
}
#endif
 
int __ctalkToCIntArrayElement (OBJECT *o) {
  OBJECT *value_var;
  if ((value_var = o -> instancevars) == NULL)
    value_var = o;

  if (value_var -> attrs & OBJECT_VALUE_IS_BIN_INT)
    return *(int *)value_var -> __o_value;

  if (is_class_or_subclass (o, rt_defclasses -> p_integer_class)) {
    return __ctalk_to_c_int (value_var);
  }

  if (is_class_or_subclass (o, rt_defclasses -> p_longinteger_class)) {
    _warning ("warning: LongInteger array element truncated to int.\n");
    return (int) __ctalk_to_c_longlong (value_var, TRUE);
  }

  return 0;
}


/* List of memory blocks pointed to by Vector objects.  Each memory
   pointer is registred only once. When a vector object is deleted,
   its entry is removed from this list, which prevents double
   free's. */
LIST *mem_vecs = NULL;

LIST *vec_is_registered (void *vec) {
  LIST *l;

  for (l = mem_vecs; l; l = l -> next) {
    if (l -> data == vec) {
      return l;
    }
  }
  return NULL;
}

void register_mem_vec (void *vec) {
  LIST *l;
  if (!vec_is_registered (vec)) {
    l = new_list ();
    l -> data = vec;
    if (mem_vecs == NULL) {
      mem_vecs = l;
    } else {
      list_push (&mem_vecs, &l);
    }
  }
}

void remove_mem_vec (LIST *vec) {
  LIST *l;
  if (mem_vecs != NULL) {
    if ((l = list_remove (&mem_vecs, &vec)) != NULL) {
      /* The memory vector itself is freed by the caller. */
      __xfree (MEMADDR(l));
    }
  }
}
