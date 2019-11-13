/* $Id: val.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017  Robert Kiesling, rk3314042@gmail.com.
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

/* Functions to check and set VAL's. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "lex.h"
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "typeof.h"

extern I_PASS interpreter_pass;   /* Declared in rtinfo.c. */

int m_print_val (MESSAGE *m, VAL *val) {
  switch (val -> __type)
      {
      case INTEGER_T:
	__ctalkDecimalIntegerToASCII (val -> __value.__i, m -> value);
	break;
      case LONG_T:
	sprintf (m -> value, "%ld", val -> __value.__l);
	break;
      case LONGLONG_T:
	sprintf (m -> value, "%lld", val -> __value.__ll);
	break;
      case DOUBLE_T:
      case FLOAT_T:
	sprintf (m -> value, "%f", val -> __value.__d);
	break;
#ifndef __APPLE__
      case LONGDOUBLE_T:
	sprintf (m -> value, "%Lf", val -> __value.__ld);
#endif
	break;
      case PTR_T:
	if (IS_OBJECT ((OBJECT *)val -> __value.__ptr)) {
	  OBJECT *o, *o_value;
	  o = (OBJECT *)val -> __value.__ptr;
	  if ((o_value = o -> instancevars) == NULL)
	    o_value = o;

	  if (strlen (o_value -> __o_value) >= MAXLABEL) {
	    if ((m -> value = realloc (m -> value, 
				       strlen ((char *)m -> value) + 1)) 
		== NULL) 
	      _error ("m_print_val: allocation error.\n");
	  }
	  strcpy (m -> value, o_value -> __o_value);
	} else {
	  if (strlen ((char *)val -> __value.__ptr) >= MAXLABEL) {
	    if ((m -> value = realloc (m -> value, 
				       strlen ((char *)m -> value) + 1)) 
		== NULL) 
	      _error ("m_print_val: allocation error.\n");
	  }
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
	  sprintf (m -> value, "%#lx", (unsigned long int)val -> __value.__ptr);
#else
	  htoa (m -> value, (unsigned int)val->__value.__ptr);
#endif
	}
	break;
      default:
	_warning ("Unknown type %d in m_print_val.\n", val -> __type);
	break;
      }
  return val -> __type;
}
    
int is_val_true (VAL *val) {
  switch (val -> __type)
    {
    case PTR_T:
      return (val -> __value.__ptr != NULL) ? TRUE : FALSE;
      break;
    case INTEGER_T:
      return (val -> __value.__i != 0) ? TRUE : FALSE;
      break;
    case BOOLEAN_T:
      return (val -> __value.__b != 0) ? TRUE : FALSE;
      break;
    case DOUBLE_T:
    case FLOAT_T:
      return (val -> __value.__d != 0.0) ? TRUE : FALSE;
      break;
    case LONG_T:
      return (val -> __value.__l != 0l) ? TRUE : FALSE;
      break;
    case LONGLONG_T:
      return (val -> __value.__ll != 0ll) ? TRUE : FALSE;
      break;
#ifndef __APPLE__
    case LONGDOUBLE_T:
      return (val -> __value.__ld != 0.0l) ? TRUE : FALSE;
#endif
      break;
    case 0:           /* Unitialized __value. */
      return FALSE;
      break;
    }
  _warning ("Unknown type %d in is_val_true.\n", val -> __type);
  return FALSE;
}

/*
 *  NOTE - Numeric_value is used only during expression parsing
 *  by the preprocessor, at this time, so an empty valbuf indicates 
 *  an undefined symbol, which we can evaluate silently to 
 *  FALSE unless we specify otherwise.
 */
int numeric_value (char *valbuf, VAL *val) {

  int r;
  RADIX radix;

  if (interpreter_pass == preprocessing_pass) {
    if (!strlen (valbuf)) {
      val -> __type = INTEGER_T;
      val -> __value.__i = 0;
#ifdef DEBUG_UNDEFINED_PREPROCESSOR_SYMBOLS
      debug ("Undefined preprocessor symbol, \"%s.\"", m -> name);
#endif
      return val -> __type;
    }
  }

  if (lextype_is_INTEGER_T_conv (valbuf, &(val->__value.__i), &radix)) {
    val -> __type = INTEGER_T;
    if (radix == hexadecimal) {
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
#else
      if ((long int *)val -> __value.__i == 
	  (long int *)val -> __value.__ptr)
#endif
	val -> __type = PTR_T;
    }
  } else if (lextype_is_LONGLONG_T_conv (valbuf, &(val->__value.__ll),
					 &radix)) {
    val -> __type = LONGLONG_T;
    if (radix == hexadecimal) {
      if ((long long int *)val -> __value.__l == 
	  (long long int *)val -> __value.__ptr)
	val -> __type = PTR_T;
    }
  } else if (lextype_is_FLOAT_T_conv (valbuf, &(val -> __value.__d))) {
    val -> __type = FLOAT_T;
    if (errno != 0)
      strtol_error (errno, "numeric_value ()", valbuf);
  } else if (lextype_is_DOUBLE_T_conv (valbuf, &(val -> __value.__d))) {
    if (errno != 0)
      strtol_error (errno, "numeric_value ()", valbuf);
    val -> __type = DOUBLE_T;
  }
#ifndef __APPLE__
  else if (lextype_is_LONGDOUBLE_T_conv (valbuf, &(val ->__value.__ld))) {
    if (errno != 0)
      strtol_error (errno, "numeric_value ()", valbuf);
    val -> __type = LONGDOUBLE_T;
  }
#endif /* __APPLE__ */

  return val -> __type;
}

int val_eq (VAL *val1, VAL *val2) {

  int bool_result = FALSE;

  if (val1 -> __type == val2 -> __type) {
    switch (val1 -> __type) 
      {
      case INTEGER_T:
	if (val1 -> __value.__i == val2 -> __value.__i)
	  bool_result = TRUE;
	break;
      case LONG_T:
	if (val1 -> __value.__l == val2 -> __value.__l)
	  bool_result = TRUE;
	break;
      case LONGLONG_T:
	if (val1 -> __value.__ll == val2 -> __value.__ll)
	  bool_result = TRUE;
	break;
      case DOUBLE_T:
	if (val1 -> __value.__d == val2 -> __value.__d)
	  bool_result = TRUE;
	break;
#ifndef __APPLE__
      case LONGDOUBLE_T:
	if (val1 -> __value.__ld == val2 -> __value.__ld)
	  bool_result = TRUE;
#endif
	break;
      case PTR_T:
	if (val1 -> __value.__ptr == val2 -> __value.__ptr)
	  bool_result = TRUE;
	break;
    }
  }

  return bool_result;
}

char *hex_from_numeric_val (char *s) {

  static char buf[MAXMSG];
  char tmpbuf[MAXLABEL];

  if (lextype_is_INTEGER_T (s)) {
    htoa (buf, (unsigned) atoi (s));
  } else if (lextype_is_LONGLONG_T (s)) {
# if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
      /*
       *  SPARC uses atol right now.
       */
      sprintf (buf, "0x%lx", atol(s));
# else
      sprintf (buf, "0x%llx", atoll(s));
# endif
  } else if (lextype_is_LITERAL_CHAR_T (s)) {
    strcpy (tmpbuf, s);
    TRIM_CHAR (tmpbuf);
    sprintf (buf, "0x%x", (int) *tmpbuf);
  } else if (lextype_is_DOUBLE_T (s)) {
    __ctalkExceptionInternal (NULL, ptr_conversion_x, "",0);
  }

  return buf;
}
