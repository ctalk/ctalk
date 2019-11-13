/* $Id: dblib.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2015, 2019  Robert Kiesling, rk3314042@gmail.com.
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
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "except.h"

extern RT_INFO *__call_stack[MAXARGS+1];  /* Declared in rtinfo.c. */
extern int __call_stack_ptr;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

char pager_path[FILENAME_MAX];

static char *docdir = DOCDIR;

void __inspect_init (void) {
  char *c;
  if ((c = which ("less")) == NULL) {
    c = which ("more");
  }
  strcpy (pager_path, c);
}


void __inspector_trace (int idx_opt) {
  int idx;
  for (idx = MAXARGS; idx > __call_stack_ptr; idx--) {
    if (__call_stack[idx] -> method) {
      if (!strstr (__call_stack[idx]->method->name, ARGBLK_LABEL))
	printf ("\t%d.\t%s : %s\n", 
		idx,
		__call_stack[idx]->method->rcvr_class_obj->__o_name,
		__call_stack[idx]->method->name);
    } else {
      if (__call_stack[idx] -> _rt_fn) {
	if (*(__call_stack[idx] -> _rt_fn -> name))
	  printf ("\t%d.\t%s ()\n", 
		  idx,
		  __call_stack[idx] -> _rt_fn -> name);
	else
	  printf ("\t%d.\t%s\n", idx, "<cfunc>");
      }
    }
  }
}

void __receiver_trace (int idx_opt) {
  int idx;
  for (idx = MAXARGS; idx > __ctalk_receiver_ptr; idx--) {
    printf ("\t%d.\t%p\t%s <%s> : %s <%s>\n", idx, __ctalk_receivers[idx],
	    __ctalk_receivers[idx] -> __o_name, 
	    __ctalk_receivers[idx] -> CLASSNAME,
	    __ctalk_receivers[idx] -> instancevars ? 
	    __ctalk_receivers[idx] -> instancevars -> __o_value :
	    __ctalk_receivers[idx] -> __o_value,
	    __ctalk_receivers[idx] -> instancevars ? 
	    __ctalk_receivers[idx] -> instancevars -> CLASSNAME :
	    __ctalk_receivers[idx] -> CLASSNAME);
  }
}

void __arg_trace (int idx_opt) {
  int arg_ptr = __ctalkGetArgPtr ();
  int i;
  OBJECT *arg;

  for (i = MAXARGS; i > arg_ptr; i--) {
    arg = __ctalkArgAt (i);
    printf ("\t%d.\t%p\t%s <%s> : %s <%s>\n", 
	    i, arg,
	    arg -> __o_name, 
	    arg -> CLASSNAME,
	    arg -> instancevars ? 
	    arg -> instancevars -> __o_value :
	    arg -> __o_value,
	    arg -> instancevars ? 
	    arg -> instancevars -> CLASSNAME :
	    arg -> CLASSNAME);
  }
}

void __inspect_locals (int frame_idx_opt) {
  RT_INFO *r;
  VARENTRY *var_list, *v;
  
  if ((r = __call_stack[frame_idx_opt]) == NULL) {
    printf ("%s\n", NULLSTR);
    return;
  }

  if (r -> _rt_fn != NULL) {
    var_list = r -> _rt_fn -> local_objects.vars;
  } else {
    if (r -> method != NULL) {
      var_list = M_LOCAL_VAR_LIST(r -> method);
    }
  }

  if (var_list == NULL) {
    printf ("%s\n", NULLSTR);
    return;
  }

  for (v = var_list; v; v = v -> next) {
    printf ("\t%p\t%s <%s> : %s <%s>\n", 
	    v -> var_object,
	    v -> var_object -> __o_name, 
	    v -> var_object -> CLASSNAME,
	    v -> var_object -> instancevars ? 
	    v -> var_object -> instancevars -> __o_value :
	    v -> var_object -> __o_value,
	    v -> var_object -> instancevars ? 
	    v -> var_object -> instancevars -> CLASSNAME :
	    v -> var_object -> CLASSNAME);
  }
}

extern VARENTRY *__ctalk_dictionary;

void __inspect_globals (void) {
  VARENTRY *v;

  if (__ctalk_dictionary == NULL) {
    printf ("%s\n", NULLSTR);
    return;
  }

  for (v = __ctalk_dictionary; v; v = v -> next) {
    printf ("\t%p\t%s <%s> : %s <%s>\n", 
	    v -> var_object,
	    v -> var_object -> __o_name, 
	    v -> var_object -> CLASSNAME,
	    v -> var_object -> instancevars ? 
	    v -> var_object -> instancevars -> __o_value :
	    v -> var_object -> __o_value,
	    v -> var_object -> instancevars ? 
	    v -> var_object -> instancevars -> CLASSNAME :
	    v -> var_object -> CLASSNAME);
  }
}

OBJECT *__inspect_get_arg (int frame) {
  return __ctalkArgAt (frame);
}

OBJECT *__inspect_get_receiver (int frame) {
  return __ctalk_receivers[frame];
}

OBJECT *__inspect_get_global (char *name) {
  VARENTRY *v;
  for (v = __ctalk_dictionary; v; v = v -> next) {
    if (str_eq (v -> var_object -> __o_name, name))
      return v -> var_object;
  }
  return NULL;
}

OBJECT *__inspect_get_local (int frame, char *name) {
  RT_INFO *r;
  VARENTRY *var_list = NULL, *v;
  
  if ((r = __call_stack[frame]) == NULL) {
    goto inspect_get_local_done;
  }

  if (r -> _rt_fn != NULL) {
    var_list = r -> _rt_fn -> local_objects.vars;
  } else {
    if (r -> method != NULL) {
      var_list = M_LOCAL_VAR_LIST(r -> method);
    }
  }

  if (var_list == NULL) {
    goto inspect_get_local_done;
  }

  for (v = var_list; v; v = v -> next) {
    if (str_eq (v -> var_object -> __o_name, name))
      return v -> var_object;
  }
  
 inspect_get_local_done:
  printf ("local variable %s (frame %d): not found.\n", name, frame);
  return NULL;
}

void __inspect_brief_help (void) {
  char cmd[0xff];
  int r;
  setenv ("DBPAGER", pager_path, true);
  strcatx (cmd, "cat ", docdir, "/inspect_brief_help | $DBPAGER", NULL);
  r = system (cmd);
  unsetenv ("DBPAGER");
}

void __inspect_long_help (void) {
  char cmd[0xff];
  int r;
  setenv ("DBPAGER", pager_path, true);
  strcatx (cmd, "cat ", docdir, "/inspect_long_help | $DBPAGER", NULL);
  r = system (cmd);
  unsetenv ("DBPAGER");
}
