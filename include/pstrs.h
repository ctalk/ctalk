/* $Id: pstrs.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-Fundamental-*- */

/*
  This file is part of Ctalk.
  Copyright © 2016 Robert Kiesling, rk3314042@gmail.com.
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

/* pstrs.h - Source-level string functions for ctpp. */

#ifndef _PSTRS_H

#if __GNUC__

// Works unless both strings are emtpy

static inline bool str_eq (char *s, char *t) __attribute__((always_inline));
static inline bool str_eq (char *s, char *t) {
  while (*s++  == *t++) {
    if (*s == '\0' && *t == '\0') {
      return true;
    }
  }
  return false;
}

#else

static bool str_eq (const char *s, const char *t) {
  while (*s++  == *t++) {
    if (*s == '\0' && *t == '\0') {
      return true;
    }
  }
  return false;
}

#endif

#define  _PSTRS_H
#endif /* _STRS_H_ */