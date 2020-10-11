/* $Id: tempname.c,v 1.2 2020/10/11 16:50:21 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright � 2005-2012, 2015-2016, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>

#ifndef MAXLABEL
#define MAXLABEL 0x100
#endif

/* prototypes */
char *htoa (char *, unsigned int);
int strcatx (char *, ...);
char *ctitoa (int n, char s[]);

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

char tmpname_pidbuf[64] = "";

/* buf_out should be at least FILENAME_MAX long. */
char *__tempname (char *pfx, char *buf_out) {

  struct stat statbuf;

  while (1) {
    if (*tmpname_pidbuf == '\0')
      strcatx (buf_out, P_tmpdir, "/", pfx,
	       ctitoa (getpid (), tmpname_pidbuf), ".",
	       random_string (), NULL);
    else
      strcatx (buf_out, P_tmpdir, "/", pfx, tmpname_pidbuf, ".",
	       random_string (), NULL);
    if ((stat (buf_out, &statbuf) == -1) && (errno == ENOENT))
       return buf_out;
  }
  return NULL;
}
