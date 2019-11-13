/* $Id: infiles.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "classlib.h"
#include "symbol.h"

char *source_files[MAXARGS]; /* Source files from the command args.  */
int n_input_files, input_idx;

int is_input_file (char *s) {
  int i;
  for (i = 0; i < n_input_files; i++)
    if (!strcmp (s, source_files[i]))
      return TRUE;
  return FALSE;
}

static CLASSLIB *source_infile_declarations[MAXARGS];
static int source_infile_declarations_ptr = 0;

void save_infile_declarations (CLASSLIB *c) {
  source_infile_declarations[source_infile_declarations_ptr++] = c;
}
