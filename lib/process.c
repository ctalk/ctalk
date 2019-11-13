/* $Id: process.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2015 Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"

int exec_bin (char *);
int exec_bin_to_buf (char *, OBJECT *);
int spawn_bin (char *, int);

int __ctalkExec (char *cmdline, OBJECT *outputStr) {
  /* outputStr should be a String object that will contain
     the subprocesses output - in methods, we will probably assign
     the string object to a Symbol, so the library can work
     with the complete object. */
#if defined (__linux__) || defined (__unix__) || defined (__MACH__)
  if (outputStr == NULL) {
    return exec_bin (cmdline);
  } else {
    return exec_bin_to_buf (cmdline, outputStr);
  }
#else
  printf ("__ctalkExec: subprocesses are not (yet) supported on this "
	  "architecture.\n");
#endif
}

int __ctalkSpawn (char *cmd, int restrict_param) {
#if defined (__linux__) || defined (__unix__) || defined (__MACH__)
  return spawn_bin (cmd, restrict_param);
#else
  printf ("__ctalkSpawn: subprocesses are not (yet) supported on this "
	  "architecture.\n");
#endif
}
