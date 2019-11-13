/* $Id: objtoc.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2018 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef __OBJTOC_H
#define __OBJTOC_H

/*
 *  These match the library function names in lib/objtoc.c
 */
#define CHAR_TRANS_FN        "__ctalk_to_c_char"
#define FLOAT_TRANS_FN       "__ctalk_to_c_double"
#define INT_TRANS_FN         "__ctalkToCInteger"
#define LONGINT_TRANS_FN     "__ctalkToCLongInteger"
#define LONGLONGINT_TRANS_FN "__ctalk_to_c_longlong"
#define STRING_TRANS_FN      "__ctalkToCCharPtr"
#define STRING_TRANS_FN_OLD  "__ctalk_to_c_char_ptr"
#define PTR_TRANS_FN         "__ctalk_to_c_ptr"
#define PTR_TRANS_FN_U       "__ctalk_to_c_ptr_u"

#define ARRAY_TRANS_FN   "__ctalkToCArrayElement"

#define ARRAY_TRANS_CHAR_PTR_CTYPE_FN "__ctalkArrayElementToCCharPtr"
#define ARRAY_TRANS_PTR_CTYPE_FN "__ctalkArrayElementToCPtr"
#define ARRAY_TRANS_CHAR_CTYPE_FN "__ctalkArrayElementToCChar"
#define ARRAY_TRANS_DOUBLE_CTYPE_FN "__ctalkArrayElementToCDouble"
#define ARRAY_TRANS_INT_CTYPE_FN "__ctalkArrayElementToCInt"
#define ARRAY_TRANS_LONG_LONG_INT_CTYPE_FN "__ctalkArrayElementToCLongLongInt"

#define ARRAY_INT_TRANS_FN   "__ctalkToCIntArrayElement"

typedef enum {
  null_collection_context,
  int_collection_context,
  long_long_int_collection_context,
  double_collection_context,
  char_collection_context,
  char_ptr_collection_context,
  ptr_collection_context
} COLLECTION_CONTEXT;

#endif
