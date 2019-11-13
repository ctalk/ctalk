/* $Id: pattern.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"

/*
 *  Returns the length of the format string after the '%', or FALSE.
 */
int is_printf_fmt (char *s) {

  char *t, *t0;
  enum {
    printf_fmt_null,
    printf_fmt_percent,
    printf_fmt_arg_n,
    printf_fmt_flag,
    printf_fmt_field_width,
    printf_fmt_precision,
    printf_fmt_length_qualifier,
    printf_fmt_fmt_chr
  } printf_fmt_state;

  /* strcpy (buf, s);
  if (*buf == '"') TRIM_LITERAL(buf);
  if (*buf == '\'') TRIM_CHAR_BUF(buf); */

  for (t0 = s; (*t0 && (*t0 != '%')); t0++)
    ;
  if (*t0 == '\0') return FALSE;
  printf_fmt_state = printf_fmt_null;
  for (t = t0; *t; t++) {

    if (printf_fmt_state == printf_fmt_null) {
      if (!IS_STDARG_FMT_CHAR (*t)) continue;
      if (iscntrl ((int) *t)) continue;
    }

    switch (*t)
      {
      case '%':
	if (printf_fmt_state == printf_fmt_null) {
	  printf_fmt_state = printf_fmt_percent;
	} else {
	  if (printf_fmt_state == printf_fmt_percent)
	    /* A "%%" string. */
	    return FALSE;
	}
	break;
      case ' ':
      case ',':
      case '-':
      case '+':
      case '#':
      case '0':
	if ((printf_fmt_state == printf_fmt_percent) ||
	    (printf_fmt_state == printf_fmt_arg_n))
	  printf_fmt_state = printf_fmt_flag;
	break;
      case '\t':
      case '\r':
      case '\f':
      case '\n':
	return FALSE;
	break;
      case 'd':   /* Signed conversion characters. */
      case 'i':
      case 'f':
      case 'F':
      case 'e':
      case 'E':
      case 'g':
      case 'G':
	if ((printf_fmt_state == printf_fmt_percent) ||
	    (printf_fmt_state == printf_fmt_arg_n) ||
	    (printf_fmt_state == printf_fmt_flag) ||
	    (printf_fmt_state == printf_fmt_length_qualifier) ||
	    (printf_fmt_state == printf_fmt_field_width))
	  return t - t0 + 1;
	break;
      case 'o':
      case 'u':
      case 'x':
      case 'X':
      case 'a':
      case 'A':
      case 'c':
      case 'C':
      case 'w':
      case 's':
      case 'S':
      case 'p':
      case 'n':
	if ((printf_fmt_state == printf_fmt_percent) ||
	    (printf_fmt_state == printf_fmt_arg_n) ||
	    (printf_fmt_state == printf_fmt_flag) ||
	    (printf_fmt_state == printf_fmt_length_qualifier) ||
	    (printf_fmt_state == printf_fmt_field_width))
	  return t - t0 + 1;
	break;
      case 'h':
	if ((printf_fmt_state == printf_fmt_percent) ||
	    (printf_fmt_state == printf_fmt_arg_n) ||
	    (printf_fmt_state == printf_fmt_flag)) 
	  printf_fmt_state = printf_fmt_length_qualifier;
	if (*(t + 1) == 'h')
	  ++t;
      case 'l':
	if ((printf_fmt_state == printf_fmt_percent) ||
	    (printf_fmt_state == printf_fmt_arg_n) ||
	    (printf_fmt_state == printf_fmt_field_width) ||
	    (printf_fmt_state == printf_fmt_flag)) 
	  printf_fmt_state = printf_fmt_length_qualifier;
	if (*(t + 1) == 'l')
	  ++t;
      case 'j':
      case 'z':
      case 't':
      case 'L':
	if ((printf_fmt_state == printf_fmt_percent) ||
	    (printf_fmt_state == printf_fmt_arg_n) ||
	    (printf_fmt_state == printf_fmt_flag) ||
	    (printf_fmt_state == printf_fmt_field_width))
	  printf_fmt_state = printf_fmt_length_qualifier;
	break;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '.':
	if (printf_fmt_state == printf_fmt_percent) {
	  if (*(t + 1) == '$') {
	    printf_fmt_state = printf_fmt_arg_n;
	    ++t;
	  } else {
	    printf_fmt_state = printf_fmt_field_width;
	  }
	}
	break;
      default:
	return FALSE;
	break;
      }
  }
  return FALSE;
}
