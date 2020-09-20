/* $Id: rtnwmthd.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2015  Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "parser.h"

NEWMETHOD *new_methods[MAXARGS+1];
int new_method_ptr;
OBJECT *rcvr_class_obj;

/*
 *  Fool Darwin's linker into putting the above variables into
 *  the same library namespace as except.c.

 *
 *  Much later... this might not be needed now that there's a 
 *  real function in the source module.  
 */
#ifdef __APPLE__
int dummy () { return 0; }
#endif

NEWMETHOD *create_newmethod (void) {
  static NEWMETHOD *n;

  if ((n = __xalloc (sizeof (struct _new_method))) == NULL) {
    _error ("create_newmethod (__xalloc): %s.\n", strerror (errno));
  }

  memset (n, 0, sizeof (struct _new_method));
  
  n -> argblk_ptr = MAXARGS;

  return n;
}

NEWMETHOD *create_newmethod_init (METHOD *m) {
  static NEWMETHOD *n;

  if ((n = __xalloc (sizeof (struct _new_method))) == NULL) {
    _error ("create_newmethod (__xalloc): %s.\n", strerror (errno));
  }

  memset (n, 0, sizeof (struct _new_method));
  
  n -> method = m;
  n -> argblk_ptr = MAXARGS;
  
  return n;
}

void delete_newmethod (NEWMETHOD *n) {

  int i;

  for (i = n -> argblk_ptr + 1; i <= MAXARGS; i++) 
    if (n -> argblks[i])
      __xfree (MEMADDR(n -> argblks[i]));
  if (n -> templates)
    delete_list (&(n -> templates));
  if (n -> source_file)
    __xfree (MEMADDR(n -> source_file));
  __xfree (MEMADDR(n));
}
