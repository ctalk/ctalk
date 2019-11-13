/* $Id: macprint.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "ctpp.h"

extern HASHTAB macrodefs;             /* Declared in hash.c.           */
extern HASHTAB ansisymbols;

extern int traditional_opt;           /* From libctpp/rtinfo.c         */

void output_symbols (void) {

  HLIST *l;
  DEFINITION *d;

  if ((l = _hash_first (&macrodefs)) == NULL)
    return;
  
  d = (DEFINITION *) l -> data;
  fprintf (stdout, "#define %s %s \n", d -> name, d -> value);
    
  while ((l = _hash_next (&macrodefs)) != NULL) {
    d = (DEFINITION *) l -> data;
    fprintf (stdout, "#define %s %s \n", d -> name, d -> value);
  }
}

void output_symbol_names (void) {

  HLIST *l;
  DEFINITION *d;

  if ((l = _hash_first (&macrodefs)) == NULL)
    return;
  
  d = (DEFINITION *) l -> data;
  fprintf (stdout, "#define %s %s \n", d -> name, d -> value);
    
  while ((l = _hash_next (&macrodefs)) != NULL) {
    d = (DEFINITION *) l -> data;
    fprintf (stdout, "#define %s \n", d -> name);
  }
}

void output_unused_symbols (void) {
  HLIST *l;
  DEFINITION *d;

  if ((l = _hash_first (&macrodefs)) == NULL)
    return;
  
  d = (DEFINITION *) l -> data;
  _warning ("%s:%d: Unused macro definition %s.\n",
	    l -> source_file, l -> error_line, d -> name);

  while ((l = _hash_next (&macrodefs)) != NULL) {
    if (!l -> hits) {
      d = (DEFINITION *) l -> data;
      _warning ("%s:%d: Unused macro definition %s.\n",
		l -> source_file, l -> error_line, d -> name);
    }
  }
}

extern char defines_fname[FILENAME_MAX];  /* Declared in d_opt.c. */

int write_defines (void) {

  FILE *d_fname;
  HLIST *l;
  DEFINITION *d;

  if ((d_fname = fopen (defines_fname, "w")) == NULL)
    _error ("write_defines (): %s: %s.\n", defines_fname, strerror (errno));

  if ((l = _hash_first (&macrodefs)) == NULL)
    return ERROR;
  d = (DEFINITION *) l -> data;
  fprintf (d_fname, "#define %s %s \n", d -> name, d -> value);

  while ((l = _hash_next (&macrodefs)) != NULL) {
    d = (DEFINITION *) l -> data;
    fprintf (d_fname, "#define %s %s \n", d -> name, d -> value);
  }
  fclose (d_fname);

  return SUCCESS;
}
