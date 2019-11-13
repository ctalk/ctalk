/* $Id: bintodec.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2016 Robert Kiesling, rk3314042@gmail.com.
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

/*
 *  Handles both 100000b and 0b1110 format binary constants.
 */
int ascii_bin_to_dec (char *buf) {

  int i, idx, bin_exp, len;

  len = strlen (buf);

  if (buf[len-1] == 'b' || buf[len-1] == 'B') {
    for (idx = len, bin_exp = 1, i = 0; idx >= 0; idx--) {
      if ((buf[idx] == 'b') || (buf[idx] == 'B') || buf[idx] == '\0')
	continue;
      i += (buf[idx] == '1') ? bin_exp: 0;
      bin_exp *= 2;
    }
  } else {
    for (idx = len, bin_exp = 1, i = 0; idx > 0; idx--) {
      if ((buf[idx] == 'b') || (buf[idx] == 'B') || buf[idx] == '\0')
	continue;
      i += (buf[idx] == '1') ? bin_exp: 0;
      bin_exp *= 2;
    }
  }


  return i;
}
