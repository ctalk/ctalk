/* $Id: rt_refobj.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2016 Robert Kiesling, rk3314042@gmail.com.
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
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "ctrlblk.h"

OBJECT *_makeRef (char *objname) {
  OBJECT *o;
  /* This handles, "self," "super," and everything
     else the similarly to other types of expressions. */
  if ((o = __ctalkEvalExprU (objname)) != NULL) {
    if (IS_VARTAG (o -> __o_vartags)) {
      if (IS_VARENTRY (o -> __o_vartags -> tag)) {
	return (OBJECT *) o -> __o_vartags -> tag -> var_decl;
      } else {
	return o;
      }
    } else {
      return o;
    }
  } else {
    _warning ("_makeRef: Object, \"%s,\" reference is NULL.\n",
	      objname);
    return NULL;
  }
}

OBJECT *_getRef (OBJECT *c_var_alias) {
  TAGPARAM *p;
  VARENTRY *p_parent;
  /* If makeRef couldn't find a tag, it assigned the object
     directly, so check for it here. */
  if (IS_OBJECT(c_var_alias))
    return c_var_alias;
  p = (TAGPARAM *)c_var_alias;
  if (IS_PARAM(p)) {
    if ((p_parent = p -> parent_tag) != NULL) {
      return (p_parent -> var_object ? p_parent -> var_object :
	      p_parent -> orig_object_rec);
    }
  }
  _warning ("ctalk: NULL parent tag in _getRef.\n");
  return NULL;
}

/*
 *   this replaces an OBJREF cast (which is either &<cvar_obj_ptr>
 *   or &(<cvar_obj_ptr>), which replaces the addressof operator, too). 
 *   See the comments in src/refobj.c. 

 *   Some compilers print a warning - ecch.
 */
OBJECT **_getRefRef (OBJECT *c_var_alias) {
  TAGPARAM *p;
  VARENTRY *p_parent;
  static OBJECT **return_val;
  /* See the comment above. */
  if (IS_OBJECT(c_var_alias)) {
    return_val = &c_var_alias;
    return return_val;
  }
  p = (TAGPARAM *)c_var_alias;
  if (IS_PARAM(p)) {
    if ((p_parent = p -> parent_tag) != NULL) {
      return (p_parent -> var_object ? &p_parent -> var_object :
	      &p_parent -> orig_object_rec);
    }
  }
  _warning ("ctalk: NULL parent tag in _getRefRef.\n");
  return NULL;
}
