/* $Id: xalloc.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012  Robert Kiesling, rk3314042@gmail.com.
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
#include "ctpp.h"
/*
 * Prototypes from ctalk.h.
 */
#ifdef __GNUC__
void _error (char *, ...) __attribute__ ((noreturn));
#else
void _error (char *, ...);
#endif
void _warning (char *fmt, ...);

void *__xalloc (int size) {
  static void *p;
  if ((p = malloc (size)) == NULL)
    _error ("__xalloc: %s.\n", strerror (errno));
  memset (p, '\0', size);
#ifdef __ALLOC_FREE_CHECK__
  if (__block_is_allocated ((unsigned long int )p)) 
      fprintf (stderr, "Block %#x is already allocated.\n", p);
  __add_xalloc_block (p);
#endif
  return p;
}

void __xfree(void *__p) {
  if (__p == NULL) {
    _warning ("Attempt to free a NULL pointer.\n");
    return;
  }
#ifdef __ALLOC_FREE_CHECK__
  if (!__block_is_allocated ((unsigned long int )__p)) 
      fprintf (stderr, "Block %#x is not allocated before free ().\n", __p);
  __delete_xalloc_block (__p);
#endif
  free (__p);
}

void *__xstrdup (char *s) {
  void *p;
  if ((p = calloc (strlen(s) + 1, sizeof(char))) == NULL)
    _error ("__xalloc: %s.\n", strerror (errno));
  strcpy ((char *)p, s);
#ifdef __ALLOC_FREE_CHECK__
  if (__block_is_allocated ((unsigned long int )p)) 
      fprintf (stderr, "Block %#x is already allocated.\n", p);
  __add_xalloc_block (p);
#endif
  return p;
}

void *__xrealloc (void **s, int n) {
  void *p;
  if ((p = calloc (n + 1, sizeof(char))) == NULL)
    _error ("__xrealloc: %s.\n", strerror (errno));
  strncpy ((char *)p, *s, 
	   ((n > strlen ((char *)*s)) ? strlen ((char *)*s) : n));
  __xfree (*s);
  *s = p;
  return p;
}
