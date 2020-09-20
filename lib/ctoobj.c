/* $Id: ctoobj.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016-2019 Robert Kiesling, rk3314042@gmail.com.
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
 *   Translate C function calls to objects, run-time functions.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

extern DEFAULTCLASSCACHE *rt_defclasses;

OBJECT *__ctalkCIntToObj (int i) {

  OBJECT *obj;

  obj = create_object_init_internal ("intResult",
				     rt_defclasses -> p_integer_class,
				     CREATED_PARAM, "");
  SETINTVARS(obj,i);
  __ctalkRegisterUserObject (obj);
  return obj;
}

OBJECT *__ctalkCBoolToObj (bool b) {

  OBJECT *obj;

  obj = create_object_init_internal ("booleanResult",
				     rt_defclasses -> p_boolean_class,
				     CREATED_PARAM, "");
  BOOLVAL(obj -> __o_value) = 
    BOOLVAL(obj -> instancevars -> __o_value) = b ? 1 : 0;
  __ctalkRegisterUserObject (obj);
  return obj;
}

OBJECT *__ctalkCSymbolToObj (void *i) {

  OBJECT *obj;

  obj = create_object_init_internal ("symbolResult",
				     rt_defclasses -> p_symbol_class,
				     CREATED_PARAM, "");
  *(OBJECT **)obj -> __o_value =
    *(OBJECT **)obj -> instancevars -> __o_value = i;
  __ctalkRegisterUserObject (obj);
  return obj;
}

OBJECT *__ctalkCLongLongToObj (long long int l) {

  OBJECT *obj;

  obj = create_object_init_internal ("longlongResult",
				     rt_defclasses -> p_longinteger_class,
				     CREATED_PARAM, "");
  *(long long *)obj -> __o_value =
    *(long long *)obj -> instancevars -> __o_value = l;
  __ctalkRegisterUserObject (obj);
  return obj;
}

OBJECT *__ctalkCDoubleToObj (double d) {

  char buf[MAXMSG];
  OBJECT *obj;

  sprintf (buf, "%f", d);
  obj = create_object_init_internal (buf,
				     rt_defclasses -> p_float_class,
				     CREATED_PARAM, buf);
  __ctalkRegisterUserObject (obj);
  return obj;
}

OBJECT *__ctalkCCharPtrToObj (char *s) {

  OBJECT *obj;
  obj = create_object_init_internal (s, rt_defclasses -> p_string_class,
				     CREATED_PARAM, s);
  __ctalkRegisterUserObject (obj);
  return obj;
}

int __ctalkIsObject (void *v) {
  return IS_OBJECT((OBJECT *)v);
}

OBJECT *__ctalkCVARReturnClass (CVAR *fn_cvar) {

  CVAR *typedef_cvar;

  if (fn_cvar -> type_attrs & CVAR_TYPE_CHAR) {
    switch (fn_cvar -> n_derefs)
      {
      case 2: return rt_defclasses -> p_array_class; break;
      case 1: return rt_defclasses -> p_string_class; break;
      case 0: return rt_defclasses -> p_character_class; break;
      default: break;
      }
  } else if (fn_cvar -> type_attrs & CVAR_TYPE_LONGLONG) {
    return rt_defclasses -> p_longinteger_class;
  } else if ((fn_cvar -> type_attrs & CVAR_TYPE_LONG) ||
	     (fn_cvar -> type_attrs & CVAR_TYPE_INT)) {
    return rt_defclasses -> p_integer_class;
  } else if ((fn_cvar -> type_attrs & CVAR_TYPE_FLOAT) ||
	     (fn_cvar -> type_attrs & CVAR_TYPE_DOUBLE)) {
    return rt_defclasses -> p_float_class;
  } else if ((typedef_cvar = __ctalkGetTypedef (fn_cvar -> type)) != NULL) {
      __ctalkCVARReturnClass (typedef_cvar); 
  } else {
    _warning ("Unimplemented type %s in cvar_return_class (). Return class defaulting to %s.\n",
	      fn_cvar -> type, DEFAULT_CLIB_RETURNCLASS);
    return __ctalkGetClass (DEFAULT_CLIB_RETURNCLASS);
  }
  return NULL;
}

