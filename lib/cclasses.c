/* $Id: cclasses.c,v 1.4 2020/03/31 23:08:31 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2016, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

/*
  Class stuff for the compiler, located here so it's availabel
  to the library routines also.
*/

OBJECT *classes = NULL;       /* Class dictionary and list head pointer.  */
OBJECT *last_class;

extern I_PASS interpreter_pass; /* Declared in rtinfo.c. */

/*  
 *    Default class cache for the compiler.  This gives us another way 
 *    to compare objects quickly.  The classes here should match the
 *    classes in include/defcls.h. 
*/

DEFAULTCLASSCACHE *ct_defclasses = NULL;

void init_ct_default_class_cache (void) {
  ct_defclasses = (DEFAULTCLASSCACHE *)__xalloc 
    (sizeof (struct _defaultclasscache));
}

void delete_ct_default_class_cache (void) {
  __xfree (MEMADDR(ct_defclasses));
}

void cache_class_ptr (OBJECT *class_obj) {
  if (str_eq (class_obj -> __o_name, OBJECT_CLASSNAME)) {
    ct_defclasses -> p_object_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, COLLECTION_CLASSNAME)) {
    ct_defclasses -> p_collection_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, ARRAY_CLASSNAME)) {
    ct_defclasses -> p_array_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, EXPR_CLASSNAME)) {
    ct_defclasses -> p_expr_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, CFUNCTION_CLASSNAME)) {
    ct_defclasses -> p_cfunction_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, MAGNITUDE_CLASSNAME)) {
    ct_defclasses -> p_magnitude_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, CHARACTER_CLASSNAME)) {
    ct_defclasses -> p_character_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, STRING_CLASSNAME)) {
    ct_defclasses -> p_string_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, FLOAT_CLASSNAME)) {
    ct_defclasses -> p_float_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, INTEGER_CLASSNAME)) {
    ct_defclasses -> p_integer_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, LONGINTEGER_CLASSNAME)) {
    ct_defclasses -> p_longinteger_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, SYMBOL_CLASSNAME)) {
    ct_defclasses -> p_symbol_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, BOOLEAN_CLASSNAME)) {
    ct_defclasses -> p_boolean_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, STREAM_CLASSNAME)) {
    ct_defclasses -> p_stream_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, FILESTREAM_CLASSNAME)) {
    ct_defclasses -> p_filestream_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, KEY_CLASSNAME)) {
    ct_defclasses -> p_key_class = class_obj;
  }
}

OBJECT *get_class_object (char *name) {

  OBJECT *o;

  if ((interpreter_pass == preprocessing_pass) ||
      (interpreter_pass == var_pass))
    return NULL;

  if (!classes) return NULL;
  if (!name) return NULL;

  for (o = classes; IS_OBJECT(o); o = o -> next) {
#if 0
    if (!IS_OBJECT(o)) {
      return NULL;
    }
#endif    
    if (str_eq (o -> __o_name, name))
      return o;
    if (!o || !o -> next)
      break;
  }

  return NULL;
}

HASHTAB class_names;

bool is_class_name (char *s) {
  if (_hash_get (class_names, s))
    return true;
  else
    return false;
}

void get_class_names (char **include_dirs, int n_dirs) {
  char *s;
  int i;
  DIR *d;
  struct dirent *d_ent;
  struct stat statbuf;
  char path[FILENAME_MAX];

  _new_hash (&class_names);
  for (i = 0; i < n_dirs; ++i) {
    s = include_dirs[i];
    if ((d = opendir (s)) != NULL) {
      while ((d_ent = readdir (d)) != NULL) {
	if ((d_ent -> d_name[0] == '.') ||
	    /* Emacs temp files. */
	    (d_ent -> d_name[0] == '#')) {
	  continue;
	}
	strcatx (path, s, "/", d_ent -> d_name, NULL);
	if (!stat (path, &statbuf)) {
	  if (S_ISREG(statbuf.st_mode)) {
	    _hash_put (class_names, d_ent -> d_name, d_ent -> d_name);
	  }
	} else {
	  _error ("Class file %s:  %s.\n", d_ent -> d_name, strerror(errno));
	}
      }
    }
  }
}

