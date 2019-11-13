/* $Id: sformat.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2011  Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>

/*    format_string ()
 *
 *    Formats a string using a printf-like format and an argument 
 *    list.  The calling function must call va_start first.
 *    The function recognizes the following format escapes.
 *
 *    %s - Character string.
 *    %p - Pointer, formatted as a hexadecimal number.
 *    %d - Decimal integer.
 */

char *_format_str (char * buf, char *fmt, va_list ap) {

  char *s, *oldbuf;
  int d;

  *buf = 0;

  while (*fmt) {
    oldbuf = strdup (buf);
    if (*fmt == '%') {
      (void) *fmt++;
      switch (*fmt++)
	{
	case 's':
	  s = va_arg (ap, char *);
	  if (s)
	    strcat (buf, s);
	  else 
	    strcat (buf, "(null)");
	  break;
	case 'p':
	  s = va_arg (ap, char *);
	  if (s)
	    sprintf (buf, "%s %p", oldbuf, (void *)s);
	  else
	    sprintf (buf, "%s %p", oldbuf, (void *)NULL);
	case 'd':
	  d = va_arg (ap, int);
	  sprintf (buf, "%s%d", oldbuf, d);
	  break;
	}
    } else {
      sprintf (buf, "%s%c", oldbuf, *fmt++);
    }
    free (oldbuf);
  }

  return buf;
}

