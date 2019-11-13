/* $Id: lextype.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2014  Robert Kiesling, rk3314042@gmail.com.
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
#include <sys/types.h>
#include <ctype.h>
#include <limits.h>
#include "ctpp.h"
#include "typeof.h"

/*
 *    Convenience functions for lexical ().
 *    The range checking is not very complicated,
 *    but it should be portable.
 *    
 */

int _range_check (const char *s, int lextype) {

  int r_type = lextype;
  long long int val;

  sscanf (s, "%lli", &val);

  if (val > INT_MAX) r_type = LONG_T;
  if (val > LONG_MAX) r_type = LONGLONG_T;

  return r_type;
}

int _unsigned_range_check (const char *s, int lextype) {

  int r_type = lextype;
  long long int val;

  sscanf (s, "%lli", &val);

  if (val > UINT_MAX) r_type = ULONG_T;
  if (val > ULONG_MAX) r_type = LONGLONG_T;

  return r_type;
}

int _lextype (const char *s) {

  MESSAGE *m;
  int c_type;
  long long idx;
  int lexical_type;

  c_type = ERROR;

  /* Lexically analyze a single token. */

  m = new_message ();
  idx = 0;

  lexical_type = lexical ((char *)s, &idx, m);

  delete_message (m);

  switch (lexical_type)
    {
    case LONGLONG:
      c_type = _range_check (s, LONGLONG_T);
      break;
    case LONG:
      c_type = _range_check (s, LONG_T);
      break;
    case ULONG:
      c_type = _unsigned_range_check (s, ULONG_T);
      break;
    case INTEGER:
      c_type = _range_check (s, INTEGER_T);
      break;
    case UINTEGER:
      c_type = _unsigned_range_check (s, UINTEGER_T);
      break;
    case DOUBLE:
      c_type = DOUBLE_T;
      break;
    case LITERAL:
      c_type = LITERAL_T;
      break;
    case LITERAL_CHAR:
      c_type = LITERAL_CHAR_T;
      break;
    case LABEL:
      c_type = PTR_T;
    default:
      break;
    }

  return c_type;

}
