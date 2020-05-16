/* $Id: substrcpy.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

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

#include <string.h>

/*
 *  substrcpy (char *dest, char *src, int start, int count);
 *    dest - The destination string.
 *    src  - The source string.
 *    start - The starting character to copy, starting from 0.
 *    count - The number of characters to copy.
 *
 */

static char *_g_dest, *_g_src;
static int _g_start, _g_count;

#if __GNUC__ >= 3
char *substrcpy (char * __restrict__ dest, char * __restrict__ src, 
  		 int start, int count) {
#else
char *substrcpy (char * dest, char * src, int start, int count) {
#endif

  char *substrsrc;
  char *d = dest;
  int c, srclen;

  srclen = strlen (src);
  if ((start < 0) || (start > srclen)) return src;
  if (count < 0) return src;

  for ( c = 0, substrsrc = src + start;
	c < count && *substrsrc;
	c++ )
    *d++ = *substrsrc++;

  *d = 0;

  _g_src = src;
  _g_dest = dest;
  _g_start = start;
  _g_count = count;

  return dest;
}

/*
 *  substrcat (char *dest, char *src, int start, int count);
 *    dest - The destination string.
 *    src  - The source string.
 *    start - The starting character to concatenate, starting from 0.
 *    count - The number of characters to concatenate.
 *
 */

char *substrcat (char * __restrict__ dest, char * __restrict__ src, 
		 int start, int count) {

  char *d, *substrsrc;
  int c;

  for (c = 0, substrsrc = src + start, d = dest + strlen (dest);
       c < count && *substrsrc;
       c++)
    *d++ = *substrsrc++;
  *d = 0;

  return dest;
}
