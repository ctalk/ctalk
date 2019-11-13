/* $Id: tempname.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef MAXLABEL
#define MAXLABEL 0x100
#endif

static char letters[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			 "abcdefghijklmnopqrstuvwxyz"
			 "0123456789"};

char *random_string (void) {
  int i;
  static char name[MAXLABEL];

  for (i = 0 ; i <= 8; ++i) {
    name[i] = letters[rand () % 62];
  }
  name[9] = 0;
  return name;
}

static char tempname_pidbuf[64] = "";

char *__tempname (char *d_path, char *pfx) {

  static char pname[FILENAME_MAX];
  struct stat statbuf;
  int r;

  if (*tempname_pidbuf == 0) {
    sprintf (tempname_pidbuf, "%d", getpid ());
  }

  while (1) {
    sprintf (pname, "%s/%s%s.%s", d_path, pfx, tempname_pidbuf,
	     random_string ());
    if ((r = stat (pname, &statbuf)) != 0) {
      return pname;
    }
  }

  return NULL;
}
