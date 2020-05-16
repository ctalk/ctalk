/* $Id: fileglob.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2017, 2019  Robert Kiesling, rk3314042@gmail.com.
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
#include <termios.h>
#include <unistd.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "parser.h"

extern DEFAULTCLASSCACHE *rt_defclasses;

#if defined (HAVE_GLOB) && defined (HAVE_GLOB_H)

#include <glob.h>

int __ctalkGlobFiles (char *pattern, OBJECT *list_out) {

  glob_t globbuf;
  int i, r;
  OBJECT *list_head, *key_object, *string_object;
  char buf[MAXLABEL];

  globbuf.gl_offs = 0;
  globbuf.gl_flags = 0;
  r = glob (pattern, 0, NULL, &globbuf);

  /* Point list_head at the last of the list's instance vars. */
  for (list_head = list_out -> instancevars; list_head && list_head -> next;
       list_head = list_head -> next)
    ;

  for (i = 0; i < globbuf.gl_pathc; ++i) {
    string_object = create_object_init_internal
      ("fileglob_match", rt_defclasses -> p_string_class,
       list_out -> scope | VAR_REF_OBJECT, globbuf.gl_pathv[i]);
    __objRefCntSet (OBJREF(string_object), 1);

    key_object = create_object_init_internal
      ("glob_key", rt_defclasses -> p_key_class, list_out -> scope, "");
    *(OBJECT **)key_object -> __o_value =
      *(OBJECT **)key_object -> instancevars -> __o_value = string_object;
	 
    __objRefCntSet (OBJREF(key_object), list_out -> nrefs);
    key_object -> attrs |= OBJECT_IS_MEMBER_OF_PARENT_COLLECTION;

    list_head -> next = key_object;
    key_object -> prev = list_head;
    list_head = key_object;
  }

  globfree (&globbuf);
  return r;
}

#else /* #if defined (HAVE_GLOB) && defined (HAVE_GLOB_H) */

int __ctalkGlobFiles (char *pattern, OBJECT *list_out) {
  printf ("Error - This system's C libraries do not support file globbing.\n");
  return ERROR;
}


#endif /* #if defined (HAVE_GLOB) && defined (HAVE_GLOB_H) */
