/* $Id: classlib.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2014 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _CLASSLIB_H

#ifndef _LIST_H
#include "list.h"
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 255
#endif

#define METHOD_PROTO_SIG 0x646464

typedef struct _method_proto {
  int sig;
  int n_params;
  bool varargs, prefix;
  char *src;
  char *alias;
  struct _method_proto *next, *prev;
} METHOD_PROTO;

#define CLASSLIB_SIG 0x545454

typedef struct _classlib {
  int sig;
  char path[FILENAME_MAX];
  char name[FILENAME_MAX];
  METHOD_PROTO *proto, *proto_head;
  char included_from_filename[FILENAME_MAX];
  int included_from_line;        /* Line in included_from file where the
                                    inclusion occurred.                  */
  int file_size;
  struct _classlib *next, *prev; /* Used in library cache.               */
} CLASSLIB;

#define C_LIB_INCLUDE lib_includes[lib_includes_ptr+1]

#define _CLASSLIB_H
#endif /* #ifndef __CLASSLIB_H */
