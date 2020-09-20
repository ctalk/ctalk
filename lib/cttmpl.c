/* $Id: cttmpl.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

/*
 *  As implemented, the library functions that return OBJECT *,
 *  which means they can be evaluated at run time.  These
 *  are the functions used in the methods.
 *  Further functions should be added as needed.
 */


static struct _ctalk_names {
  char *ct_lib_name,
    *ct_template_name;
} ctalk_names[] = {
  {"__ctalkAddInstanceVariable__", "ctalkAddInstanceVariable"},
  {"__ctalkCCharPtrToObj", "ctalkCCharPtrToObj"},
  {"__ctalkCDoubleToObj", "ctalkCDoubleToObj"},
  {"__ctalkCIntToObj", "ctalkCIntToObj"},
  {"__ctalkCopyObject", "ctalkCopyObject"},
  {"__ctalkCreateObject", "ctalkCreateObject"},
  {"__ctalkCreateObjectInit", "ctalkCreateObjectInit"},
  {"__ctalkEvalExpr", "ctalkEvalExpr"},
  {"__ctalkExceptionNotifyInternal", "ctalkExceptionNotifyInternal"},
  {"__ctalkFindClassVariable", "ctalkFindClassVariable"},
  {"__ctalkGetInstanceVariable", "ctalkGetInstanceVariable"},
  {"__ctalkGetInstanceVariableByName", "ctalkGetInstanceVariableByName"},
  {NULL, NULL}
};

char *ctalk_lib_fn_name (char *name, int warn) {
  int i;

  for (i = 0; ctalk_names[i].ct_lib_name; i++) {
    if (!strcmp (ctalk_names[i].ct_lib_name, name))
      return ctalk_names[i].ct_template_name;
  }

  for (i = 0; ctalk_names[i].ct_template_name; i++) {
    if (!strcmp (ctalk_names[i].ct_template_name, name))
      return ctalk_names[i].ct_template_name;
  }

  if (warn)
    _warning ("ctalk_name: unknown function %s.\n", name);

  return NULL;
}

