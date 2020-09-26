/* $Id: strsecure.c,v 1.2 2020/09/26 11:00:52 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2018 Robert Kiesling, rk3314042@gmail.com.
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

#define MAXMSG 8192

char *xstrcpy (char *d, const char *s) {
  return strcpy (d, s);
}

char *xstrncpy (char *d, const char *s, size_t n) {
  return strncpy (d, s, n);
}

char *xstrcat (char *d, const char *s) {
  return strcat (d, s);
}

char *xstrncat (char *d, const char *s, size_t n) {
  return strncat (d, s, n);
}

int xsprintf (char *s, const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  return vsprintf (s, fmt, ap);
}

void *xmemcpy (void *d, const void *s, size_t n) {
  return memcpy (d, s, n);
}

void *xmemset (void *s, int c, size_t n) {
  return memset (s, c, n);
}

void *xmemmove (void *d, const void *s, size_t n) {
  return memmove (d, s, n);
}

