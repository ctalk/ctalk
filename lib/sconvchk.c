/* $Id: sconvchk.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012 Robert Kiesling, rk3314042@gmail.com.
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
 *  Scalar conversion with checking for range and 
 *  valid characters.  After calling chkatoi,
 *  check errno.
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#define CHKATOI_WARNING 0

int chkatoi (const char *s) {
  char *nptr = NULL;
  int r;
  errno = 0;
  r = (int)strtol (s, &nptr, 10);
#if CHKATOI_WARNING  
  if ((nptr == s) && errno) {
    fprintf (stderr, "chkatoi: %s.\n", strerror (errno));
  }
#endif
  return r;
}

int chkptrstr (const char *__s) {
  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && isxdigit((int)__s[2])) && 
      (__s[3] && isxdigit((int)__s[3])) && 
      (__s[4] && isxdigit((int)__s[4])) && 
      (__s[5] && isxdigit((int)__s[5])) && 
      (__s[6] && isxdigit((int)__s[6]))) {
    return 1;
  }
  return 0;
}

