/* $Id: strcatx.c,v 1.2 2020/04/22 10:32:11 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2016, 2018 Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"

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
    while (*t++ = *s++)
      ;
    --t;
    s = va_arg (ap, char *);
  }
  va_end (ap);
  *t = 0;
  return (int) (t - destbuf);
}

/* Like strcatx, but concatenates to the existing contents of destbuf. */
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
    while (*t++ = *s++)
      ;
    --t;
    s = va_arg (ap, char *);
  }
  va_end (ap);
  *t = 0;
  return (int) (t - destbuf);
}

#ifdef __GNUC__
inline char *toks2str (MESSAGE_STACK messages, int start, int end, char *buf_out) {
#else
char *toks2str (MESSAGE_STACK messages, int start, int end, char *buf_out) {
#endif  
  int i;
  char *q, *p;
  p = buf_out;
  for (i = start; i >= end; i--) {
    q = messages[i] -> name;
    while (*p++ = *q++)
      ;
    --p;
  }
  return buf_out;
}
