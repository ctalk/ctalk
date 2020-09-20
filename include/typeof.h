/* $Id: typeof.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of ctalk.
  Copyright © 2005-2007 Robert Kiesling, rkiesling@users.sourceforge.net.
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

#ifndef _TYPEOF_H
#define _TYPEOF_H

#define INTEGER_T         1
#define LONG_T            2
#define LONGLONG_T        3
#define DOUBLE_T          4
#define FLOAT_T           5
#define LONGDOUBLE_T      6
#define LITERAL_T         7
#define LITERAL_CHAR_T    8
#define OBJECT_T          9
#define PTR_T             10
#define STRUCT_T          11
#define STRUCT_MBR_T      12
#define ARRAY_MBR_T       13
#define BOOLEAN_T         14
#define UINTEGER_T        15
#define ULONG_T           16

#define IS_C_TYPE(i) ((i == INTEGER_T) || \
                      (i == UINTEGER_T) || \
                      (i == LONG_T) || \
                      (i == ULONG_T) || \
                      (i == LONGLONG_T) || \
                      (i == DOUBLE_T) || \
                      (i == FLOAT_T) || \
                      (i == LONGDOUBLE_T) || \
                      (i == LITERAL_T) || \
                      (i == LITERAL_CHAR_T) || \
                      (i == OBJECT_T) || \
                      (i == PTR_T) || \
                      (i == STRUCT_MBR_T) || \
                      (i == ARRAY_MBR_T) || \
                      (i == STRUCT_T))

#endif   /* _TYPEOF_H */
