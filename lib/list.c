/* $Id: list.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _LIST_H
#include "list.h"
#endif
#include "object.h"
#include "message.h"
#include "ctalk.h"

#ifdef __GNUC__
void _error (char *, ...) __attribute__ ((noreturn));
#else
void _error (char *, ...);
#endif

extern void *__xalloc (int);
extern void __xfree(void **);
/* Used by xfree<n> */
#ifndef MEMADDR
#define MEMADDR(x) ((void **)&(x))
#endif

/*
 *  Add list member t to list l.
 */

void list_add (LIST *l, LIST *t) {

  LIST *tmp;

  if (l) {
    for (tmp = l; tmp && tmp -> next; tmp = tmp -> next)
      ;
    tmp -> next = t;
    t -> prev = tmp;
  } else {
    l = t;
  }
}

LIST *list_remove (LIST **l, LIST **p) {
  LIST *t = NULL;
  for (t = *l; t; t = t -> next) {
    if (t == *p) {
      if (t -> prev) {
	t -> prev -> next = t -> next;
      }
      if (t -> next) {
	t -> next -> prev = t -> prev;
      }
      if (t == *l)  {
	(*l) = (*l) -> next;
	if (*l && (*l) -> prev) (*l) -> prev = NULL;
      }
      break;
    }
  }
  return t;
}

/*
 *  Create a new LIST structure.
 */

LIST *new_list (void) {

  LIST *l;

  if ((l = (LIST *)__xalloc (sizeof (struct _list))) == NULL)
    _error ("new_list: %s.", strerror (errno));
  l -> sig = LIST_SIG;

  return l;
}

/*
 *  Add an element to the end of the list.
 */

void list_push (LIST **l1, LIST **l2) {
  LIST *t;

  for (t = *l1; t && t -> next; t = t -> next) 
    ;
  if (t) {
    t -> next = *l2;
    (*l2) -> prev = t;
    (*l2) -> next = NULL;
  } else {
    *l1 = *l2;
  }
}

/*
 *  Remove the first element of list l and return it.
 */

LIST *list_unshift (LIST **l) {
  LIST *t;

  if (!*l) return (LIST *)NULL;
  t = *l;
  if (t -> next) t -> next -> prev = (*l) -> prev;
  *l = (t -> next) ? t -> next : NULL;
  return t;
}

void delete_list_element (LIST *l) {
  if (l -> data) __xfree (MEMADDR(l -> data));
  __xfree (MEMADDR(l));
}

void delete_list (LIST **l) {
  LIST *t1, *t2;

  if (! *l)
    return;

  for (t1 = *l; t1 -> next; t1 = t1 -> next)
    ;

  if (t1 == *l) {
    delete_list_element (*l);
    return;
  }

   while (t1 != *l) {
     t2 = t1 -> prev;
     delete_list_element (t1);
     if (t2 == *l)
       break;
     t1 = t2;
   }
   delete_list_element (*l);
}

void delete_object_list_element (LIST *l) {
  OBJECT *o;
  if (IS_OBJECT(l -> data)) {
    o = (OBJECT *)l -> data;
    /* In case we get a cross-linking. */
    if (is_arg(o -> instancevars) || is_receiver (o -> instancevars))
      o -> instancevars = NULL;
    if (!is_arg (o) && !is_receiver (o)) {
      __ctalkDeleteObject (o);
    }
  }
  __xfree (MEMADDR(l));
}

void delete_object_list (LIST **l) {
  LIST *t1, *t2;

  if (! *l)
    return;

  for (t1 = *l; t1 -> next; t1 = t1 -> next)
    ;

  if (t1 == *l) {
    delete_object_list_element (*l);
    return;
  }

   while (t1 != *l) {
     t2 = t1 -> prev;
     delete_object_list_element (t1);
     if (t2 == *l)
       break;
     t1 = t2;
   }
   delete_object_list_element (*l);
}
