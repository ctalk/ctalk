/* $Id: fsecure.c,v 1.2 2019/11/28 13:30:29 rkiesling Exp $ */

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

FILE *xfopen (const char *path, const char *mode) {
  return fopen (path, mode);
}

int xfprintf (FILE *s, const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  return vfprintf (s, fmt, ap);
}
