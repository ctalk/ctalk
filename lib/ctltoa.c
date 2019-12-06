/* $Id: ctltoa.c,v 1.2 2019/12/06 22:59:13 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2019 Robert Kiesling, rk3314042@gmail.com.
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

#include <string.h>
#include <stdbool.h>

int strcatx2 (char *, ...);

extern void __reverse(char []);

void __ctalkLongToDecimalASCII (long int n, char s[])
{
  long int i;
  long int sign;

  if ((sign = n) < 0)  /* record sign */
    n = -n;          /* make n positive */
  i = 0;
  do {       /* generate digits in reverse order */
    s[i++] = n % 10 + '0';   /* get next digit */
  } while ((n /= 10) > 0);     /* delete it */
  if (sign < 0)
    s[i++] = '-';
  s[i] = '\0';
  __reverse(s);
  strcatx2 (s, "l", NULL);
} 

extern char hexl[]; /* declared in ctlltoa.c */
extern char hexu[];

char *__ctalkLongToHexASCII (long int n, char s[], bool uppercase) {
  int i;
  long long int sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
      if (uppercase)
	s[i++] = hexu[n % 16];
      else
	s[i++] = hexl[n % 16];
    } while ((n /= 16) > 0);     /* delete it */
    if (uppercase)
      s[i++] = 'X', s[i++] = '0';
    else
      s[i++] = 'x', s[i++] = '0';
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    __reverse(s);
    return s;
} 
