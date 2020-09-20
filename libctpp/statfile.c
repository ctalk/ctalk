/* $Id: statfile.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "ctpp.h"

#ifdef __GNUC__
void _error (char *, ...) __attribute__ ((noreturn));
#else
void _error (char *, ...);
#endif

int file_exists (char *path) {

  struct stat statbuf;
  int r;
  
  if ((r = stat (path, &statbuf)) == 0)
    return 1;
  else
    return 0;
}

int file_size (char *path) {
  struct stat statbuf;
  int r;

  if ((r = stat (path, &statbuf)) != 0)
    _error ("%s: %s.", path, strerror (errno));

  return statbuf.st_size;
}

int is_dir (char *path) {
  struct stat statbuf;
  int r;
  
  if ((r = stat (path, &statbuf)) == 0) {
    if (S_ISDIR (statbuf.st_mode)) 
      return 1;
  }

  return 0;
}

char *which (char *prog) {
  static char pname[MAXMSG];
  char path_var[MAXMSG],
    dname[FILENAME_MAX],
    *p, *q;
  DIR *d;
  struct dirent *d_ent;

  if (getenv ("PATH"))
    strcpy (path_var, getenv ("PATH"));
  else
    return NULL;

  p = path_var;
  while (p) {
    memset ((void *)dname, 0, FILENAME_MAX * sizeof (char));
    if ((q = index (p, ':')) != NULL)
      strncpy (dname, p, q - p);
    else
      strcpy (dname, p);

    if (is_dir (dname)) {

      if ((d = opendir (dname)) == NULL)
	_error ("which: %s.\n", strerror (errno));

      while ((d_ent = readdir (d)) != NULL)
	if (!strcmp (d_ent -> d_name, prog)) {
	  sprintf (pname, "%s/%s", dname, d_ent -> d_name);
	  closedir (d);
	  return pname;
	}

      closedir (d);
    }

    p = (q) ? q + 1 : NULL;
  }

  return NULL;
}
