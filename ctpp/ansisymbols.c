/* $Id: ansisymbols.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

/* 
 *   Implements the C99 preprocessor symbols __FILE__, __LINE__,
 *   __DATE__, __TIME__, and (initial version) __func__.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include "ctpp.h"
#include "phash.h"
#include "typeof.h"

extern INCLUDE *includes[MAXARGS + 1];  /* Declared in preprocess.c.    */
extern int include_ptr;

extern char source_file[FILENAME_MAX];  /* Declared in main.c.          */

extern HASHTAB ansisymbols;             /* Declared in hash.c.          */

static DEFINITION *ansi_new_definition (void);

void ansi_symbol_init (void) {

  DEFINITION *d;
  time_t utctime;
  struct tm *t;

  /* __FILE__ macro */
  d = ansi_new_definition ();
  strcpy (d -> name, "__FILE__");
  if (include_ptr <= MAXARGS) {
    strcpy (d -> value, includes[include_ptr] -> path);
  } else {
    strcpy (d -> value, source_file);
  }
   _hash_put (ansisymbols, d, d -> name);

  /* __LINE__ macro. */
  d = ansi_new_definition ();
  strcpy (d -> name, "__LINE__");
  SNPRINTF (d -> value, MAXMSG, "%d", 1);
  _hash_put (ansisymbols, d, d -> name);

  utctime = time (&utctime);
  t = localtime (&utctime);

  /* __DATE__ macro */
  d = ansi_new_definition ();
  strcpy (d -> name, "__DATE__");
  SNPRINTF (d -> value, MAXMSG, 
	    "\"%s %2d %4d\"", mon(t -> tm_mon), t -> tm_mday,
	   t -> tm_year + 1900);
  _hash_put (ansisymbols, d, d -> name);

  /* __TIME__ macro */
  d = ansi_new_definition ();
  strcpy (d -> name, "__TIME__");
  SNPRINTF (d -> value, MAXMSG, 
	    "\"%02d:%02d:%02d\"", t -> tm_hour, t -> tm_min,
	   t -> tm_sec);
  _hash_put (ansisymbols, d, d -> name);

  /* __func__ macro - Use the GNU terminology.  */
  d = ansi_new_definition ();
  strcpy (d -> name, "__func__");
  strcpy (d -> value, "\"At top level.\"");
  _hash_put (ansisymbols, d, d -> name);

}

void ansi__LINE__ (int n) {

  DEFINITION *d;

  if ((d = (DEFINITION *)_hash_get (ansisymbols, "__LINE__")) == NULL) {
    _error ("ansi__LINE__: undefined symbol");
    return;
  }

  SNPRINTF (d -> value, MAXMSG, "%d", n);
}

int get_ansi__LINE__ (void) {
  DEFINITION *d;

  if ((d = (DEFINITION *)_hash_get (ansisymbols, "__LINE__")) == NULL) {
    _error ("ansi__LINE__: undefined symbol");
    return 0;
  }

  return atoi (d -> value);
}

void ansi__FILE__ (char *name) {

  DEFINITION *d;

  if ((d = (DEFINITION *)_hash_get (ansisymbols, "__FILE__")) == NULL) {
    _error ("ansi__FILE__: undefined symbol");
    return;
  }

  SNPRINTF (d -> value, MAXMSG, "\"%s\"", name);

}

void ansi__func__ (char *name) {
  DEFINITION *d;

  if ((d = (DEFINITION *)_hash_get (ansisymbols, "__func__")) == NULL)
    _error ("ansi__func__: undefined symbol");

  strcpy (d -> value, name);

}

static DEFINITION *ansi_new_definition (void) {

  DEFINITION *d;

  if ((d = 
       (DEFINITION *)__xalloc (sizeof (struct _macro_symbol)))
      == NULL)
    _error ("add_symbol: %s.", strerror (errno));

  d -> sig = MACRODEF_SIG;

  return d;
}

