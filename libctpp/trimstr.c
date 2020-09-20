/* $Id: trimstr.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

/* 
 *  String handling routines.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Prototype */
char *substrcpy (char *, char *, int, int);

#define FALSE 0
#define TRUE !(FALSE)
#define MAXLABEL 255

void trim_leading_whitespace (char *s) {

  int i;
  int i_charstart, in_word;
  int length;
  static char buf[MAXLABEL];

  strcpy (buf, s);

  length = strlen (buf);

  for (i = 0, i_charstart = 0, in_word = FALSE; 
       (i < length) && (in_word == FALSE); 
       i++)
    if (!isspace ((int) buf[i]) && !in_word) {
	i_charstart = i;
	in_word = TRUE;
    }

  strcpy (s, &buf[i_charstart]);
}

void trim_trailing_whitespace (char *s) {

  int i;
  int i_inchar;
  int i_char_end;
  int length;

  length = strlen (s);

  for (i = 0, i_inchar = FALSE, i_char_end = -1; i < length; i++) {
    if (!isspace ((int) s[i]))
	i_inchar = TRUE;

    if (i_inchar && isspace ((int) s[i])) {
      i_inchar = FALSE;
      i_char_end = i;
    }
  }
  if (i_char_end >= 0)
    s[i_char_end] = 0;

}

