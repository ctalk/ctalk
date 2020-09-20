/* $Id: linux-gnu-x86.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012 Robert Kiesling, rk3314042@gmail.com.
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
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 *  Built-in #defines for linux-gnu with Intel x86 CPU.
 *  The builtins[] array is simply a set of symbol/value
 *  pairs.  builtins[0,2,4,...n] are the symbols, 
 *  builtins[1,3,5,...n] are the values.
 *
 *  You must also declare the variables host_os and host_cpu
 *  with the correct target values for your system in 
 *  builtins.c.
 *
 *  Cpp can display the definitions that GNU C expects for a 
 *  particular system.  Use the command, "echo ' ' | cpp -dM -"
 *  to print the target system's built-in definitions.
 */

#include <stdio.h>

static char *builtins[] = {
  /* symbol */        /* value */
  "__linux__",        "1",
  "__linux",          "1",
  "linux",            "1",
  "__i386__",         "1",
  "__i386",           "1",
  "i386",             "1",
  "__unix__",         "1",
  "__unix",           "1",
  "unix",             "1",
  /* GNU cpp defines this, gcc doesn't. */
  "__STDC__",         "1",
  "STDC",             "1",
/* stddef.h uses these to define typedefs. */
#    ifdef __DJGPP__
  "__SIZE_TYPE__",    "long unsigned int",
#    else
  "__SIZE_TYPE__",    "unsigned int",
#    endif
  NULL, NULL
};
