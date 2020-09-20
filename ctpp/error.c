/* $Id: error.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "ctpp.h"

extern char appname[];                 /* Declared in main.c.         */
extern char source_file[];
extern int error_line, error_column;   /* Declared in errorloc.c.     */

extern INCLUDE *includes[MAXARGS + 1]; /* From preprocess.c.          */
extern int include_ptr;

/* Library prototypes. */
extern char *_format_str (char *, char *, va_list);
extern void _error (char *,...);
extern void _error_out (char *);
extern void _warning (char *,...);

/*
 *  Return the name of the input file or include file.
 */
char *source_filename (void) {

  static char fn[FILENAME_MAX];

  if (include_ptr > MAXARGS) 
    strcpy (fn, source_file);
  else
    strcpy (fn, includes[include_ptr] -> name);

  return fn;
}

/*
 *  Exit with an error.  The _error () function doesn't return, so
 *  perform file cleanup first.
 */

void error (MESSAGE *orig, char *fmt,...) {
  va_list ap;
  char fmtbuf[MAXMSG];

  cleanup (TRUE);

  va_start (ap, fmt);
  _error ("%s:%d. %s\n", source_filename (), orig -> error_line, 
	  _format_str (fmtbuf, fmt, ap));
}

void warning (MESSAGE *orig, char *fmt,...) {
  va_list ap;
  char fmtbuf[MAXMSG];

  va_start (ap, fmt);
  _warning ("%s:%d: warning: %s\n", source_filename (),
	    orig -> error_line, _format_str (fmtbuf, fmt, ap));
}

#ifdef DEBUG_CODE
void debug (char *fmt,...) {
  va_list ap;
  char fmtbuf[MAXMSG];
  va_start (ap, fmt);
  _warning ("%s\n", _format_str (fmtbuf, fmt, ap));
}
#endif

void location_trace (MESSAGE *orig) {

  int i;
  bool first = True;

  if (include_ptr > MAXARGS)          /* Only trace included files. */
    return;

  for (i = include_ptr; i < MAXARGS; i++) {
    /* if (!includes[i] -> name) */ /***/
    if (!includes[i])
      continue;
    _warning ("%s:%d:\t%s %s line %d,\n", 
	      (include_ptr > MAXARGS) ? 
	      source_file : includes[include_ptr] -> name,
	      orig -> error_line,
	      ((first) ? "File included from" : "From"),
	      ((i == MAXARGS) ? source_file : includes[i+1] -> name), 
	      includes[i] -> error_line);
    first = False;
  }
    _warning ("%s:%d:\t%s %s line %d.\n", 
	      ((include_ptr > MAXARGS) ? 
	       source_file : includes[include_ptr] -> name),
	      orig -> error_line,
	      ((first) ? "File included from" : "From"),
	      source_file,
	      includes[MAXARGS] -> error_line);

}

