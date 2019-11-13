/* $Id: pending.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

#ifndef _PENDING_H
#define _PENDING_H

#define PENDING_SIG 0x1a1a1a
#define IS_PENDING(x) ((x)->sig==PENDING_SIG)

typedef struct _pending {            /* Statements that need to be output */
  int sig;
  int n;                             /* later in the generated code.      */
  int parser_lvl;
  char stmt[MAXMSG];
  /*char *ptr;*/                         /* if we can use the original string */
  struct _pending *next;             /* in fileout_str */
  struct _pending *prev;
} PENDING;

typedef struct _pending_fn_arg {     /* Statements that need to be output */
  int n;                             /* later in the generated code.      */
  char tok[MAXLABEL];                /* The token we want to replace.     */
  char stmt[1024];
  struct _pending_fn_arg *next;
  struct _pending_fn_arg *prev;
} PENDING_FN_ARG;

#endif 

