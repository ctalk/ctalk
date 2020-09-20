/* $Id: trimstr.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016  Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"

/* Prototype */
char *substrcpy (char *, char *, int, int);

#define FALSE 0
#define TRUE !(FALSE)
#define MAXLABEL 0x100

char *trim_leading_whitespace (char *s_in, char *s_out) {
  /* Now trims trailing whitespace also. */

  int i;
  int i_charstart, in_word;
  int length = strlen (s_in);
  char *end;

  for (i = 0, i_charstart = 0, in_word = FALSE; 
       (i < length) && (in_word == FALSE); 
       i++)
    if (!isspace ((int) s_in[i]) && !in_word) {
	i_charstart = i;
	in_word = TRUE;
    }

  strcpy (s_out, &s_in[i_charstart]);

  end = &s_out[strlen (s_out)-1];

  if (*end != ' ' && *end != '\t')
    return s_out;

  while (end > s_out) {
    if (*end == ' ' || *end == '\t')
      *end = 0;
    else
      break;
    --end;
  }

  return s_out;
}

char *trim_trailing_whitespace (char *s) {

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

  return s;
}

char *trimstr (char *s_in, char *s_out) {

  char *q, *r;

  for (q = s_in; *q == ' '; q++)
    ;

  for (r = &s_in[strlen(s_in) - 1]; *r == ' '; r--)
    ;

  substrcpy (s_out, s_in, q - s_in, r - q + 1);

  return s_out;
}

char *remove_whitespace (char *s_in, char *s_out) {
  char *q;
  int i;
  for (q = s_in, i = 0; *q; q++) {
    if (!isspace((int)*q))
      s_out[i++] = *q;
  }
  s_out[i] = '\0';
  return s_out;
}
