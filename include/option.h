/* $Id: option.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef _OPTION_H
#define _OPTION_H

#ifndef _OBJECT_H
#include "object.h"       /* VAL typedef. */
#endif

typedef enum {
  opt_arg_none,
  opt_arg_int,
  opt_arg_str
} OPTION_ARG_TYPE;

typedef struct _option_desc {
  char opt_name[MAXLABEL];
  char opt_desc[MAXMSG];
  OPTION_ARG_TYPE opt_arg_type;
} OPTION_DESC;

typedef struct _option_val {
  char opt_name[MAXLABEL];
  VAL opt_value;
} OPTION_VAL;

#endif  /* ifndef _OPTION_H */
