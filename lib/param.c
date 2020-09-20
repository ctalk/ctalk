/* $Id: param.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015, 2017  Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

extern RT_INFO rtinfo;

PARAM *new_param (void) {
  PARAM *p;

  if ((p = (PARAM *)__xalloc (sizeof (PARAM))) == NULL)
    _error ("new_param: %s\n", strerror (errno));

  p -> sig = PARAM_SIG;
  return p;
}

TAGPARAM *new_tagparam (void) {
  TAGPARAM *p;

  if ((p = (TAGPARAM *)__xalloc (sizeof (TAGPARAM))) == NULL)
    _error ("new_param: %s\n", strerror (errno));

  p -> sig = PARAM_SIG;
  return p;
}

/*
 *  When looking for a param name, find the method 
 *  that is currently executing by searching for the
 *  method by function in the receiver's class and 
 *  and its superclasses, then looking up the parameter
 *  name, and returning the corresponding argument.
 */

OBJECT *__ctalk_getParamByName (char *name) {

  OBJECT *rcvr_obj,
    *rcvr_class_obj;
  METHOD *this_method;
  int i;

  if (__ctalk_receiver_ptr == MAXARGS)
    return NULL;

  rcvr_obj = __ctalk_receivers[__ctalk_receiver_ptr + 1];

  if (!IS_OBJECT (rcvr_obj))
    _error ("__ctalk_getParamByName: Unknown receiver.\n");

  if ((rcvr_class_obj = rcvr_obj->__o_class) == NULL)
    _error ("__ctalk_getParamByName: Unknown receiver class %s.\n",
	    rcvr_obj -> CLASSNAME);

  if (((this_method = 
       __ctalkGetInstanceMethodByFn (rcvr_obj, rtinfo.method_fn, TRUE))
      == NULL) &&
      ((this_method =
	__ctalkGetClassMethodByFn (rcvr_obj, rtinfo.method_fn, FALSE)) 
       == NULL))
    return NULL;

  for (i = 0; i < this_method -> n_params; i++)
    if (!strcmp (name, this_method -> params[i] -> name))
      return this_method -> args[i] -> obj;

  return NULL;
}

