/* $Id: errorloc.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012  Robert Kiesling, rk3314042@gmail.com.
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
#include "lex.h"
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"

int error_line = 1;               /* Line in source file.                 */
int error_column = 1;             /* Column in source file.               */

extern RT_INFO rtinfo;           /* Declared in rtinfo.c.                */

static ERROR_LOCATION error_location[MAXARGS+1];
static int error_location_ptr;

void error_reset (void) {
  error_line = error_column = 1;
}

void init_error_location (void) {
  error_location_ptr = MAXARGS;
}

int get_error_location_ptr (void) {
  return error_location_ptr;
}

int error_line_at (int ptr) {
  return error_location[ptr].error_line;
}

int error_column_at (int ptr) {
  return error_location[ptr].error_column;
}

char *error_file_at (int ptr) {
  return error_location[ptr].fn;
}

void save_error_location (int new_line, int new_column, char *fn) {
  error_location[error_location_ptr].error_line = error_line;
  error_location[error_location_ptr].error_column = error_column;
  error_line = new_line;
  error_column = new_column;
  if (fn && *fn) {
    strcpy (error_location[error_location_ptr].fn, fn);
  } else {
    if (error_location_ptr < MAXARGS) {
	strcpy (error_location[error_location_ptr].fn, 
		error_location[error_location_ptr + 1].fn);
    } else {
      strcpy (error_location[error_location_ptr].fn, 
	      rtinfo.source_file);
    }
  }
  --error_location_ptr;
}

void restore_error_location (void) {
  ++error_location_ptr;
  error_line = error_location[error_location_ptr].error_line;
  error_column = error_location[error_location_ptr].error_column;
  __ctalkRtSaveSourceFileName (error_location[error_location_ptr].fn);
  error_location[error_location_ptr].error_line = 
    error_location[error_location_ptr].error_column = 0;
  strcpy (error_location[error_location_ptr].fn, "");
}


