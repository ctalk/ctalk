/* $Id: radixof.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

#include <ctype.h>
#include <string.h>
#include "ctpp.h"

#define IS_SIGN(c,d) ((c == '-' || c == '+') && \
		      (isdigit (d) || d == '.'))

RADIX radix_of (const char *buf) {

  RADIX radix = decimal;
  int i;
  
  for (i = 0; i < strlen (buf); i++)
    if (!isxdigit ((int)buf[i]) && 
	(buf[i] != (char)'l') && (buf[i] != (char)'L') &&
	(buf[i] != (char)'u') && (buf[i] != (char)'U') &&
	(buf[i] != (char)'x') && (buf[i] != (char)'X'))
      return -1;

  if (buf[0] && buf[1] && !IS_SIGN (buf[0],buf[1])) {
    if ((buf[0] == (char)'0') && (buf[1] != (char)'.')) {
      if (buf[1] == (char) 'x' || buf[1] == (char) 'X') {
	radix = hexadecimal;
      } else if (buf[1] == (char) 'b' || buf[1] == (char) 'B') {
	radix = binary;
      } else if (buf[1] && (buf[1] >= '0' && buf[1] <= '7')) {
	radix = octal;
      }
    }
  } else {
    if (buf[1] && buf[2]) {
      if ((buf[1] == (char)'0') && (buf[2] != (char)'.')) {
	if (buf[2] == (char) 'x' || buf[2] == (char) 'X') {
	  radix = hexadecimal;
	} else if (buf[2] == (char) 'b' || buf[2] == (char) 'B') {
	  radix = binary;
	} else if (buf[2] && (buf[2] >= '0' && buf[2] <= '7')) {
	  radix = octal;
	}
      }
    }
  }
  return radix;
}
