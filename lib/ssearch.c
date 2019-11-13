/* $Id: ssearch.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2014  Robert Kiesling, rk3314042@gmail.com.
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

static inline bool q_str_match (char *buf, char *pat, int len) {
  /* Still haven't found a good matching algorithm.... */
  if (!strncmp (buf, pat, len))
    return true;
  else
    return false;
}

int __ctalkSearchBuffer (char *pattern, char *buffer, long long *offsets) {

  long long int buffer_length;
  long long int pattern_length;
  long long int searchable_length;
  int n_offsets;
  long long int i;

  buffer_length = strlen (buffer);
  pattern_length = strlen (pattern);

  n_offsets = 0;
  offsets[n_offsets] = -1L;

  if (buffer_length < pattern_length)
    return n_offsets;

  searchable_length = buffer_length - pattern_length;

  for (i = 0; i <= searchable_length; ++i) {
    if (q_str_match (&buffer[i], pattern, pattern_length)) {
      offsets[n_offsets++] = (int)i;
      offsets[n_offsets] = -1L;
    }
  }

  return n_offsets;
}

