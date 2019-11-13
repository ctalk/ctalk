/* $Id: rt_error.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2011  Robert Kiesling, rk3314042@gmail.com.
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
 *   Format and output error and warning messages.
 */

#ifndef MAXMSG             /* Defined in ctalk.h. */
#define MAXMSG 8192
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

extern int no_warnings_opt;         /* -w option.                  */
extern int warnings_to_errors_opt;  /* -Werror option.             */

extern char *_format_str (char *, char *, va_list);
extern void _error_out (char *);
extern void cleanup (int);

/* Just use stdout with fprintf, which communicates with ctalks 
   popen () call on all the platforms that we're compatible with.
*/
void _error (char *fmt, ...) {
  va_list ap;
  char buf[MAXMSG], fmtbuf[MAXMSG];
  va_start (ap, fmt);
  sprintf (buf, "%s", _format_str (fmtbuf, fmt, ap));
  fprintf (stdout, "%s", buf);
  fflush (stdout);
  cleanup (1);
  exit (1);
}

void _warning (char *fmt, ...) {
  va_list ap;
  char buf[MAXMSG], fmtbuf[MAXMSG];
  if (no_warnings_opt) return;
  if (warnings_to_errors_opt) _error (fmt);
  va_start (ap, fmt);
  sprintf (buf, "%s", _format_str (fmtbuf, fmt, ap));
  fprintf (stderr, "%s", buf);
  fflush (stderr);
}

