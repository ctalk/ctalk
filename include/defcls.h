/* $Id: defcls.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

#ifndef _DEFCLS_H

#define OBJECT_CLASSNAME "Object"
#define OBJECT_SUPERCLASSNAME NULL

#define COLLECTION_CLASSNAME "Collection"
#define COLLECTION_SUPERCLASSNAME "Object"

#define ARRAY_CLASSNAME "Array"
#define ARRAY_SUPERCLASSNAME "Collection"

#define EXPR_CLASSNAME "Expr"
#define EXPR_SUPERCLASSNAME "Object"

#define CFUNCTION_CLASSNAME "CFunction"
#define CFUNCTION_SUPERCLASSNAME "Expr"

#define MAGNITUDE_CLASSNAME "Magnitude"
#define MAGNITUDE_SUPERCLASSNAME "Object"

#define CHARACTER_CLASSNAME "Character"
#define CHARACTER_SUPERCLASSNAME "Magnitude"

#define STRING_CLASSNAME "String"
#define STRING_SUPERCLASSNAME "Character"

#define FLOAT_CLASSNAME "Float"
#define FLOAT_SUPERCLASSNAME "Magnitude"

#define INTEGER_CLASSNAME "Integer"
#define INTEGER_SUPERCLASSNAME "Magnitude"

/* this helps avoid warnings if we encounter a "%ld" printf format */
#define INTEGER_CLASSNAME_L "Integer_L"
#define INTEGER_SUPERCLASSNAME_L "Magnitude"

#define LONGINTEGER_CLASSNAME "LongInteger"
#define LONGINTEGER_SUPERCLASSNAME "Magnitude"

#define SYMBOL_CLASSNAME "Symbol"
#define SYMBOL_SUPERCLASSNAME "Object"

#define BOOLEAN_CLASSNAME "Boolean"
#define BOOLEAN_SUPERCLASSNAME "Object"

#define STREAM_CLASSNAME "Stream"
#define STREAM_SUPERCLASSNAME "Collection"

#define FILESTREAM_CLASSNAME "FileStream"
#define FILESTREAM_SUPERCLASSNAME "Stream"

#define KEY_CLASSNAME "Key"
#define KEY_SUPERCLASSNAME "Symbol"

#define VECTOR_CLASSNAME "Vector"
#define VECTOR_SUPERCLASSNAME "Symbol"

#define DEFAULT_CLIB_RETURNCLASS "Integer"

#ifndef DEFAULTCLASSCACHE_DEFINED
struct _defaultclasscache {
  OBJECT *p_object_class;
  OBJECT *p_collection_class;
  OBJECT *p_array_class;
  OBJECT *p_expr_class;
  OBJECT *p_cfunction_class;
  OBJECT *p_magnitude_class;
  OBJECT *p_character_class;
  OBJECT *p_string_class;
  OBJECT *p_float_class;
  OBJECT *p_integer_class;
  OBJECT *p_longinteger_class;
  OBJECT *p_symbol_class;
  OBJECT *p_boolean_class;
  OBJECT *p_stream_class;
  OBJECT *p_filestream_class;
  OBJECT *p_key_class;
};

typedef struct _defaultclasscache DEFAULTCLASSCACHE;

#define DEFAULTCLASSCACHE_DEFINED
#endif /* #ifndef DEFAULTCLASSCACHE_DEFINED */

#ifndef DEFAULTCLASS_CMP
#define DEFAULTCLASS_CMP(o,d,s) ((IS_OBJECT(o) && IS_OBJECT(d)) ?	\
				 ((o) -> __o_class == (d)) :		\
				 str_eq ((o) -> __o_classname, (s)))
#endif

#define _DEFCLS_H
#endif /* #ifndef __DEFCLS_H */
