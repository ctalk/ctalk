/* $Id: rtxalloc.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

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
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include "list.h"

#ifdef CHAR_BIT
#if CHAR_BIT == 8
#define _CHAR_8_BIT_
#endif
#endif

/*
 * Prototypes from ctalk.h.
 */
LIST *new_list (void);
void list_add (LIST *, LIST *);
LIST *list_remove (LIST **, LIST **);

#ifdef __GNUC__
void _error (char *, ...) __attribute__ ((noreturn));
#else
void _error (char *, ...);
#endif
void _warning (char *fmt, ...);

bool char_size_1 = true;

#ifdef __GNUC__
void *__xalloc (size_t size) {
#else
void *__xalloc (unsigned int size) {
#endif
  static void *p;
  if (char_size_1) {
    if ((p = calloc (1, size)) == NULL)
      _error ("__xalloc(char_8): Block size %d: %s.\n", size, strerror (errno));
  } else {
  if ((p = calloc (1, size * sizeof (char))) == NULL)
    _error ("__xalloc(char_sizeof): Block size %d: %s.\n", size, strerror (errno));
  }
  return p;
}

void __xfree (void **__p) {
  if (__p == NULL || *__p == NULL) {
#ifdef WARN_NULL_POINTER_FREES
    _warning ("__xfree: Attempt to free a NULL pointer.\n");
#endif
    return;
  }
  free (*__p);
  *__p = NULL;
}

void *__xstrdup (char *s) {
  void *p;
  if ((p = calloc (strlen(s) + 1, sizeof(char))) == NULL)
    _error ("__xalloc: %s.\n", strerror (errno));
  strcpy ((char *)p, s);
  return p;
}

void *__xrealloc (void **s, int n) {
  void *p;
#ifdef CHAR_8_BIT
  if ((p = __xalloc (n + 1)) == NULL)
    _error ("__xrealloc: %s.\n", strerror (errno));
#else
  if ((p = __xalloc ((n + 1) * sizeof(char))) == NULL)
    _error ("__xrealloc: %s.\n", strerror (errno));
#endif  
  strncpy ((char *)p, *s, 
	   ((n > strlen ((char *)*s)) ? strlen ((char *)*s) : n));
  __xfree (s);
  *s = p;
  return p;
}

void __ctalkFree (void *p) {
  __xfree (&p);
}
