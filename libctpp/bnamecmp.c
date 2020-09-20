/* $Id: bnamecmp.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

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

#include <string.h>

/* From ctpp.h. */
char *substrcpy (char *, char *, int, int);

#if defined (__sparc__) && defined (__GNUC__)
#define index strchr
#define rindex strrchr
#endif

#ifndef MAXLABEL
#define MAXLABEL 255
#endif

/*
 * TO DO -
 * 1. This needs to be speeded up.
 *
 * 2. Compare only the first 63 characters, as specified in C99.
 */

/*
 * Match s1 and s2 up to the parameter list - the first '('.
 *
 * Return < 0 if s1 < s2, 0 if s1 == s2, > 0 if s1 > s2. 
 */

int basename_cmp (char *s1, char *s2) {

  char *p_1, *p_2;        /* Start of arguments.  */
  char b1[MAXLABEL], b2[MAXLABEL];    /* Basenames.           */

  p_1 = index (s1, '(');
  p_2 = index (s2, '(');

  if ((p_1 && p_2) &&
      ((p_1 - s1) != (p_2 - s2)))
    return -1;

  /* Substrcpy takes care of trailing nulls, strncpy doesn't. */
  if (p_1)
    substrcpy (b1, s1, 0, p_1 - s1);
  if (p_2)
    substrcpy (b2, s2, 0, p_2 - s2);

  return strcmp (((p_1) ? b1 : s1), ((p_2) ? b2 : s2));
}

/*
 *  Only check for macro base names; i.e., the label
 *  before the argument list.
 */
char *mbasename (char *s) {
  char *p;
  static char bname[8192];  /* MAXMSG */
  
  if ((p = index (s, '(')) != NULL)
    substrcpy (bname, s, 0, p - s);
  else
    strcpy (bname, s);
  return bname;
}
