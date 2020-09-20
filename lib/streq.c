/* $Id: streq.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2017  Robert Kiesling, rk3314042@gmail.com.
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
 *  This is a modified version of str_eq (), from The C Book.
 *  Do not use.  There's an inline version in strs.h.
 */

#if 0
#ifdef __GNUC__
static inline bool str_eq (char *s, char *t) __attribute__((always_inline));
static inline bool str_eq (char *s, char *t) {
#else
bool __str_eq (const char *s1, const char *s2) {
#endif

  while (*s++  == *t++) {
    if (*s == '\0' && *t == '\0') {
      return true;
    }
  }
  return false;
}

#endif
