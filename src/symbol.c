/* $Id: symbol.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015 Robert Kiesling, rk3314042@gmail.com.
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
 *  This has all been superceded ... this file can probably go away
 *  now.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "symbol.h"

#if 0
static SYMBOL *unresolved_symbols = NULL;
static SYMBOL *unresolved_classes = NULL; /* List of unresolved classes.  */
#endif

#if 0
int add_unresolved_class (char *name) {

  SYMBOL *s, *t;

  s = new_symbol ();
  strcpy (s -> name, name);

  if (unresolved_classes == NULL) {
    unresolved_classes = s;
  } else {
    for (t = unresolved_classes; t -> next; t = t -> next)
      ;
    t -> next = s;
    s -> prev = t;
  }
  return SUCCESS;
}
#endif

#if 0
int add_unresolved_symbol (char *name) {

  SYMBOL *s, *t;

  s = new_symbol ();
  strcpy (s -> name, name);

  if (unresolved_symbols == NULL) {
    unresolved_symbols = s;
  } else {
    for (t = unresolved_symbols; t -> next; t = t -> next)
      ;
    t -> next = s;
    s -> prev = t;
  }
  return SUCCESS;
}
#endif

#if 0
SYMBOL *new_symbol (void) {
  SYMBOL *s;
  s = __xalloc (sizeof (SYMBOL));
  return s;
}
#endif

#if 0
void delete_symbol (SYMBOL *s) {
  __xfree (MEMADDR(s));
}
#endif
