/* $Id: lextype.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016, 2018 
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
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

static int __is_subclass_of (char *classname, char *superclassname) {

  OBJECT *class_object, *superclass_object;

  if ((class_object = __ctalkGetClass (classname)) == NULL)
    return FALSE;
    
  for (superclass_object = class_object -> __o_superclass;
       _SUPERCLASSNAME(superclass_object);
       superclass_object = superclass_object -> __o_superclass)
    if (str_eq (superclass_object -> __o_name, superclassname))
      return TRUE;

  return FALSE;
}

int lextype_of_class (char *s) {
  if (!strcmp (s, INTEGER_CLASSNAME) ||
      __is_subclass_of (s, INTEGER_CLASSNAME))
    return INTEGER_T;

  if (!strcmp (s, LONGINTEGER_CLASSNAME) ||
      __is_subclass_of (s, LONGINTEGER_CLASSNAME))
    return LONGLONG_T;
    
  if (!strcmp (s, FLOAT_CLASSNAME) ||
      __is_subclass_of (s, FLOAT_CLASSNAME))
    return FLOAT_T;

  if (!strcmp (s, STRING_CLASSNAME) ||
      __is_subclass_of (s, STRING_CLASSNAME))
    return PTR_T;

  if (!strcmp (s, CHARACTER_CLASSNAME))
    return INTEGER_T;

  if (!strcmp (s, COLLECTION_CLASSNAME) ||
      __is_subclass_of (s, COLLECTION_CLASSNAME))
    return PTR_T;

  if (!strcmp (s, SYMBOL_CLASSNAME) ||
      __is_subclass_of (s, SYMBOL_CLASSNAME))
    return PTR_T;

  if (!strcmp (s, OBJECT_CLASSNAME) ||
      __is_subclass_of (s, OBJECT_CLASSNAME))
    return PTR_T;

/*    _warning ("Unimplemented class %s in lextype_of_class.  Lexical type defaulting to int.\n", s); */
  
  return INTEGER_T;
}

bool lextype_is_PTR_T (char *s) {
  return (IS_C_LABEL(s));
}

bool lextype_is_LITERAL_T (char *s) {
  char *t = s;

  if (IS_C_LABEL(s))
    return false;

  if (*t == '"') {
    ++t;
  literal_t_loop: while (*t != '"') {
      if (*t == 0)
	return false;
      ++t;
    }
    if (*(t + 1) == 0) {
      return true;
    } else {
      ++t;
      goto literal_t_loop;
    }
  }
  return false;
}

bool lextype_is_LITERAL_CHAR_T (char *s) {

  if (IS_C_LABEL(s))
    return false;

  /* Normal char. */
  if (s[0] == '\'' && s[2] == '\'' && s[3] == 0) {
    return true;
  } else {
    /* Esc char sequence. */
    if (s[0] == '\'' &&  s[1] == '\\' && s[3] == '\'' &&
	s[4] == 0) {
      return true;
    }
  }
  return false;
}

bool lextype_is_INTEGER_T (char *s) {
  long int val;
  char *nptr;

  if (IS_C_LABEL(s))
    return false;

  errno = 0;
  val = strtol (s, &nptr, 0);
  if (val == 0 && nptr == s)
    return false;
  if (errno)
    return false;
  else
    return true;
}

bool lextype_is_INTEGER_T_conv (char *s, int *val, RADIX *r) {
  char *nptr;
  int base = 0;

  if (IS_C_LABEL(s))
    return false;

  switch (*r = radix_of (s))
      {
      case decimal:
	base = 10;
	break;
      case octal:
	base = 8;
	break;
      case hexadecimal:
	base = 16;
	break;
      case binary:
	base = 2;
	*val = (int) ascii_bin_to_dec (s);
	return true;
	break;
      }

  errno = 0;
  *val = strtol (s, &nptr, 0);
  if (*val == 0 && nptr == s)
    return false;
  if (errno)
    return false;
  else
    return true;
}

bool lextype_is_LONGLONG_T (char *s) {
  long long int val;
  char *nptr;

  if (IS_C_LABEL(s))
    return false;

  errno = 0;
  val = strtoll (s, &nptr, 0);
  if (val == 0 && nptr == s)
    return false;
  if (errno)
    return false;
  else
    return true;
}

bool lextype_is_LONGLONG_T_conv (char *s, long long int *val,
				 RADIX *r) {
  char *nptr;
  int base = 0;

  if (IS_C_LABEL(s))
    return false;

  switch (*r = radix_of (s))
    {
    case decimal:
      base = 10;
      break;
    case octal:
      base = 8;
      break;
    case hexadecimal:
      base = 16;
      break;
    case binary:
      *val = (long long) ascii_bin_to_dec (s);
      return true;
      break;
    }

  errno = 0;
  *val = strtoll (s, &nptr, base);
  if (*val == 0 && nptr == s)
    return false;
  if (errno)
    return false;
  else
    return true;
}

bool lextype_is_FLOAT_T (char *s) {
  float val;
  char *nptr;

  if (IS_C_LABEL(s))
    return false;

  errno = 0;
  val = strtof (s, &nptr);
  if (val == 0.0 && nptr == s)
    return false;
  if (errno || val == 0.0)
    return false;
  else
    return true;
}

bool lextype_is_FLOAT_T_conv (char *s, double *val) {
  char *nptr;

  if (IS_C_LABEL(s)) {
    *val = 0.0;
    return false;
  }

  errno = 0;
  *val = (double)strtof (s, &nptr);
  if (*val == 0.0 && nptr == s)
    return false;
  if (errno || *val == 0.0)
    return false;
  else
    return true;
}

bool lextype_is_DOUBLE_T (char *s) {
  double val;
  char *nptr;

  if (IS_C_LABEL(s))
    return false;

  errno = 0;
  val = strtod (s, &nptr);
  if (val == 0.0 && nptr == s)
    return false;
  if (errno || val == 0.0)
    return false;
  else
    return true;
}

bool lextype_is_DOUBLE_T_conv (char *s, double *val) {
  char *nptr;

  if (IS_C_LABEL(s)) {
    *val = 0.0;
    return false;
  }

  errno = 0;
  *val = strtod (s, &nptr);
  if (*val == 0.0 && nptr == s)
    return false;
  if (errno || *val == 0.0)
    return false;
  else
    return true;
}

#if defined(__APPLE__) && defined(__POWERPC__)
bool lextype_is_LONGDOUBLE_T_conv (char *s, double *val) {
  return lextype_is_DOUBLE_T_conv (s, val);
}
#else
bool lextype_is_LONGDOUBLE_T_conv (char *s, long double *val) {
  char *nptr;

  if (IS_C_LABEL(s)) {
    *val = 0.0;
    return false;
  }

  errno = 0;
  *val = strtold (s, &nptr);
  if (*val == 0.0 && nptr == s)
    return false;
  if (errno || *val == 0.0)
    return false;
  else
    return true;
}
#endif
