/* $Id: val.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2014, 2016 Robert Kiesling, rk3314042@gmail.com.
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
#include "ctpp.h"
#include "typeof.h"

void m_print_val (MESSAGE *m, VAL *val) {
  switch (val -> __type)
      {
      case BOOLEAN_T: /***/
	sprintf (m -> value, "%d", val -> __value.__b ? 1 : 0);
	break;
      case INTEGER_T:
	sprintf (m -> value, "%d", val -> __value.__i);
	break;
      case UINTEGER_T:
	sprintf (m -> value, "%u", val -> __value.__u);
	break;
      case LONG_T:
	sprintf (m -> value, "%ld", val -> __value.__l);
	break;
      case ULONG_T:
	sprintf (m -> value, "%lu", val -> __value.__ul);
	break;
      case LONGLONG_T:
	sprintf (m -> value, "%lld", val -> __value.__ll);
	break;
      case DOUBLE_T:
	sprintf (m -> value, "%f", val -> __value.__d);
	break;
#ifndef __APPLE__
      case LONGDOUBLE_T:
	sprintf (m -> value, "%Lf", val -> __value.__ld);
	break;
#endif
      case PTR_T:
	if (val -> __value.__ptr) {
	  if (strlen ((char *)val -> __value.__ptr) >= MAXLABEL) {
	    if ((m -> value = realloc (m -> value, 
				       strlen ((char *)m -> value) + 1)) 
		== NULL) 
	      _error ("m_print_val: allocation error.\n");
	  }
	  sprintf (m -> value, "%p", val -> __value.__ptr);
	}
	break;
      default:
	_warning ("Unknown type %d in m_print_val.\n", val -> __type);
	break;
      }
}
    
int d_print_val (DEFINITION *d, VAL *val) {
  switch (val -> __type)
      {
      case INTEGER_T:
	sprintf (d -> value, "%d", val -> __value.__i);
	break;
      case UINTEGER_T:
	sprintf (d -> value, "%d", val -> __value.__u);
	break;
      case LONG_T:
	sprintf (d -> value, "%ld", val -> __value.__l);
	break;
      case ULONG_T:
	sprintf (d -> value, "%ld", val -> __value.__ul);
	break;
      case LONGLONG_T:
	sprintf (d -> value, "%lld", val -> __value.__ll);
	break;
      case DOUBLE_T:
	sprintf (d -> value, "%f", val -> __value.__d);
	break;
#ifndef __APPLE__
      case LONGDOUBLE_T:
	sprintf (d -> value, "%Lf", val -> __value.__ld);
	break;
#endif
      case PTR_T:
	sprintf (d -> value, "%p", val -> __value.__ptr);
	break;
      default:
	_warning ("Unknown type %d in d_print_val.\n", val -> __type);
	break;
      }
  return val -> __type;
}
    
int copy_val (VAL *s, VAL *d) {
  d -> __type = s -> __type;
  switch (s -> __type)
    {
    case INTEGER_T:
      d -> __value.__i = s -> __value.__i;
      break;
    case UINTEGER_T:
      d -> __value.__u = s -> __value.__u;
      break;
    case LONG_T:
      d -> __value.__l = s -> __value.__l;
      break;
    case ULONG_T:
      d -> __value.__ul = s -> __value.__ul;
      break;
    case LONGLONG_T:
      d -> __value.__ll = s -> __value.__ll;
      break;
    case DOUBLE_T:
      d -> __value.__d = s -> __value.__d;
      break;
#ifndef __APPLE__
    case LONGDOUBLE_T:
      d -> __value.__ld = s -> __value.__ld;
      break;
#endif
    case PTR_T:
      d -> __value.__ptr = s -> __value.__ptr;
      break;
    default:
      _warning ("Unknown type %d in copy_val.\n", s -> __type);
      break;
    }
  return SUCCESS;
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
    case UINTEGER_T:
      return (val -> __value.__u != 0) ? TRUE : FALSE;
      break;
    case BOOLEAN_T:
      return (val -> __value.__b != 0) ? TRUE : FALSE;
      break;
    case DOUBLE_T:
      return (val -> __value.__d != 0.0) ? TRUE : FALSE;
      break;
    case LONG_T:
      return (val -> __value.__l != 0l) ? TRUE : FALSE;
      break;
    case ULONG_T:
      return (val -> __value.__ul != 0ul) ? TRUE : FALSE;
      break;
    case LONGLONG_T:
      return (val -> __value.__ll != 0ll) ? TRUE : FALSE;
      break;
#ifndef __APPLE__
    case LONGDOUBLE_T:
      return (val -> __value.__ld != 0.0l) ? TRUE : FALSE;
      break;
#endif
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
int numeric_value (const char *valbuf, VAL *val) {

  if (!strlen (valbuf)) {
    val -> __type = INTEGER_T;
    val -> __value.__i = 0;
#ifdef DEBUG_UNDEFINED_PREPROCESSOR_SYMBOLS
    debug ("Undefined preprocessor symbol, \"%s.\"", m -> name);
#endif
    return val -> __type;
  }

  switch (_lextype (valbuf))
  {
  case INTEGER_T:
    val -> __type = INTEGER_T;
    switch (radix_of (valbuf))
      {
      case decimal:
	(void)sscanf (valbuf, "%d", &(val-> __value.__i));
	break;
      case octal:
	(void)sscanf (valbuf, "%o", &val-> __value.__i);
	break;
      case hexadecimal:
	(void)sscanf (valbuf, "%x", &val-> __value.__i);
	break;
      case binary:
	val -> __value.__i = ascii_bin_to_dec ((char *)valbuf);
	break;
      }
    break;
  case UINTEGER_T:
    val -> __type = UINTEGER_T;
    switch (radix_of (valbuf))
      {
      case decimal:
	(void)sscanf (valbuf, "%u", &(val-> __value.__u));
	break;
      case octal:
	(void)sscanf (valbuf, "%o", &val-> __value.__u);
	break;
      case hexadecimal:
	(void)sscanf (valbuf, "%x", &val-> __value.__u);
	break;
      case binary:
	val -> __value.__u = (unsigned)
	  ascii_bin_to_dec ((char *)valbuf);
	break;
      }
    break;
  case LONG_T:
    val -> __type = LONG_T;
    switch (radix_of (valbuf))
      {
      case decimal:
	(void)sscanf (valbuf, "%ld", &val-> __value.__l);
	break;
      case octal:
	(void)sscanf (valbuf, "%lo", &val-> __value.__l);
	break;
      case hexadecimal:
	(void)sscanf (valbuf, "%lx", &val-> __value.__l);
	break;
      case binary:
	val -> __value.__l = (long) ascii_bin_to_dec ((char *)valbuf);
	break;
      }
    break;
  case ULONG_T:
    val -> __type = ULONG_T;
    switch (radix_of (valbuf))
      {
      case decimal:
	(void)sscanf (valbuf, "%ld", &val-> __value.__ul);
	break;
      case octal:
	(void)sscanf (valbuf, "%lo", &val-> __value.__ul);
	break;
      case hexadecimal:
	(void)sscanf (valbuf, "%lx", &val-> __value.__ul);
	break;
      case binary:
	val -> __value.__ul = (unsigned long) ascii_bin_to_dec ((char *)valbuf);
	break;
      }
    break;
  case LONGLONG_T:
    val -> __type = LONGLONG_T;
    switch (radix_of (valbuf))
      {
      case decimal:
	(void)sscanf (valbuf, "%lld", &val-> __value.__ll);
	break;
      case octal:
	(void)sscanf (valbuf, "%llo", &val-> __value.__ll);
	break;
      case hexadecimal:
	(void)sscanf (valbuf, "%llx", &val-> __value.__ll);
	break;
      case binary:
	val -> __value.__ll = (long long) ascii_bin_to_dec ((char *)valbuf);
	break;
      }
    break;
  case FLOAT_T:
  case DOUBLE_T:
    (void)sscanf (valbuf, "%lf", &val-> __value.__d);
    val -> __type = DOUBLE_T;
    break;
#ifndef __APPLE__
  case LONGDOUBLE_T:
    (void)sscanf (valbuf, "%Lf", &val-> __value.__ld);
    val -> __type = LONGDOUBLE_T;
    break;
#endif
  case BOOLEAN_T:
    (void)sscanf (valbuf, "%d", (int *) &val-> __value.__b);
    val -> __type = BOOLEAN_T;
    break;
  case PTR_T:
    val -> __value.__ptr = (char *)valbuf;
    val -> __type = PTR_T;
    break;
  case LITERAL_T:
    val -> __value.__ptr = (char *)valbuf;
    val -> __type = LITERAL_T;
    break;
  default:
    val -> __type = INTEGER_T;
    val -> __value.__b = FALSE;
#ifdef DEBUG_CODE
    _warning ("Undefined type of %s in numeric___value.\n", valbuf);
#endif
    break;
  }

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
      case UINTEGER_T:
	if (val1 -> __value.__u == val2 -> __value.__u)
	  bool_result = TRUE;
	break;
      case LONG_T:
	if (val1 -> __value.__l == val2 -> __value.__l)
	  bool_result = TRUE;
	break;
      case ULONG_T:
	if (val1 -> __value.__ul == val2 -> __value.__ul)
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
	break;
#endif
      case PTR_T:
	if (val1 -> __value.__ptr == val2 -> __value.__ptr)
	  bool_result = TRUE;
	break;
    }
  }

  return bool_result;
}

