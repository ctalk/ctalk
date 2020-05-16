/* $Id: strcatx.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2016 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdarg.h>

/* Returns the length of destbuf.  Only the last argument may be
   NULL ... */
#ifdef __GNUC__
inline int strcatx (char *destbuf, ...) {
#else
int strcatx (char *destbuf, ...) {
#endif  
  va_list ap;
  register char *s, *t = destbuf;

  va_start (ap, destbuf);

  s = va_arg (ap, char *);
  while (s) {
    do { *t++ = *s++; } while (*s);
    s = va_arg (ap, char *);
  }
  va_end (ap);
  *t = 0;
  return (int) (t - destbuf);
}

/* Like strcatx, but does not overwrite the existing contents of destbuf. */
#ifdef __GNUC__
inline int strcatx2 (char *destbuf, ...) {
#else
int strcatx2 (char *destbuf, ...) {
#endif  
  va_list ap;
  register char *s, *t;

  t = destbuf;
  while (*t)
    t++;

  va_start (ap, destbuf);

  s = va_arg (ap, char *);
  while (s) {
    do { *t++ = *s++; } while (*s);
    s = va_arg (ap, char *);
  }
  va_end (ap);
  *t = 0;
  return (int) (t - destbuf);
}

