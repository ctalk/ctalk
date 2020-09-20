/* $Id: keyword.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2014, 2016  Robert Kiesling, rk3314042@gmail.com.
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

#include <string.h>

#define FALSE 0
#define TRUE !(FALSE)

#define N_CTALK_KEYWORDS 6

int is_ctalk_keyword (const char *s) {
  int i;
  static const char *ctalk_keywords[] = {
    "self",
    "super",
    "value",
    "arg",
    "method",
    "returnObjectClass"
  };

  /* don't let threads worry about handling a closure */
  for (i = 0; i < N_CTALK_KEYWORDS; i++)
    if (*s == *ctalk_keywords[i])
      if (!strcmp (s, ctalk_keywords[i]))
	return TRUE;
    
  return FALSE;
}

#ifdef __GNUC__
#define N_MACRO_KEYWORDS 19
#else
#define N_MACRO_KEYWORDS 14
#endif

int is_macro_keyword (const char *s) {

  int i;
  static const char *macro_keywords[] = {
    "include",
#ifdef __GNUC__
    "include_next",
    "ident",
    "sccs",
    "unassert",
#endif
    "define",
    "undef",
    "if",
    "ifdef",
    "ifndef",
    "else",
    "elif",
    "endif",
    "error",
    "warning",
    "assert",
    "line",
    "pragma",
#ifdef __GNUC__
    "include_here",
#endif
  };

  for (i = 0; i < N_MACRO_KEYWORDS; i++) {
    /* don't let threads worry about handling a closure */
    if (*s == *macro_keywords[i])
      if (!strcmp (s, macro_keywords[i]))
	return i + 1;
  }
  return FALSE;  /* zero */
}

static const char *c_keywords[] = {
  "auto",
  "break",
  "case",
  "char",
  "const",
  "continue",
  "default",
  "do",
  "double",
  "else",
  "enum",
  "extern",
  "float",
  "for",
  "goto",
  "if",
  "inline",
  "int",
  "long",
  "register",
  "restrict",
  "return",
  "short",
  "signed",
  "sizeof",
  "static",
  "struct",
  "switch",
  "typedef",
  "union",
  "unsigned",
  "void",
  "volatile",
  "while",
  "_Bool",
  "_Complex",
  "_Imaginary"
#ifdef __GNUC__
  , "__complex__"
#endif
#if defined (__APPLE__) && defined (__POWERPC__)
  , "__inline"
#endif 
};

#ifdef __GNUC__
# if defined (__APPLE__) && defined (__POWERPC__)
#  define N_C_KEYWORDS 39
# else
#  define N_C_KEYWORDS 38
# endif
# else
# define N_C_KEYWORDS 37
#endif

/* Check if a label is an ANSI C or C99 keyword, including data types.  
   A subset that checks only the data types is below.
*/

int is_c_keyword (const char *s) {

  int i;

  for (i = 0; i < N_C_KEYWORDS; i++) {
    /* don't let threads worry about handling a closure */
    if (*s == *c_keywords[i])
      if (!strcmp (s, c_keywords[i]))
	return TRUE;
  }

  return FALSE;
}

/* 
 *    Check if a label is an ANSI C data type.  This is a subset of
 *    the ANSI C keywords, above.
*/

static const char *c_data_types[] = {
  "char",
  "const",
  "double",
  "enum",
  "extern",
  "float",
  "inline",
  "int",
  "long",
  "register",
  "short",
  "signed",
  "static",
  "struct",
  "union",
  "unsigned",
  "void",
  "volatile",
  "_Bool",
  "_Complex",
  "_Imaginary"
#ifdef __GNUC__
  , "__complex__"
#endif
#if defined (__APPLE__) && defined (__POWERPC__)
  , "__inline"
#endif 
};

#ifdef __GNUC__
# if defined (__APPLE__) && defined (__POWERPC__)
#  define N_C_DATA_TYPES 23
# else
#  define N_C_DATA_TYPES 22
# endif
# else
# define N_C_DATA_TYPES 21
#endif

/* 
 *    Check if a label is an ANSI C data type.  This is a subset of
 *    the ANSI C keywords, above.
*/

int is_c_data_type (const char *s) {

  int i;

  for (i = 0; i < N_C_DATA_TYPES; i++)
    if (*s == *c_data_types[i])
      if (!strcmp (s, c_data_types[i]))
	return TRUE;

  return FALSE;
}

static const char *c_storage_classes[] = {
  "const",
  "extern",
  "inline",
  "register",
  "static",
  "volatile"
#if defined (__APPLE__) && defined (__POWERPC__)
  , "__inline"
#endif 
};


#if defined (__APPLE__) && defined (__POWERPC__)
# define N_C_STORAGE_CLASSES 6
#else
# define N_C_STORAGE_CLASSES 5
#endif

/* 
 *    Check if a label is an ANSI C storage class keyword.  
 *    This is a subset of the ANSI C keywords, above.
 */

int is_c_storage_class (const char *s) {

  int i;

  for (i = 0; i < N_C_STORAGE_CLASSES; i++)
    if (*s == *c_storage_classes[i])
      if (!strcmp (s, c_storage_classes[i]))
	return TRUE;

  return FALSE;
}

#ifdef __GNUC__

static const char *c_extensions[] = {
  "__extension__",                 /* GNU C. */
  "__const",
  "__restrict",
  "__attribute__",
  "__mode__",
}; 

#define N_EXTENSIONS 5

#else

#define N_EXTENSIONS 0

#endif

int is_extension_keyword (char *s) {

  int i;

  for (i = 0; i < N_EXTENSIONS; i++)
    if (!strcmp (s, c_extensions[i]))
      return TRUE;

  return FALSE;
}

static const char *c_ctrl_keywords[] = {
  "break",
  "case",
  "do",
  "else",
  "for",
  "goto",
  "if",
  "switch",
  "while"
};

#define N_CTRL_KEYWORDS 9

/*
 *  Check if a label is a control structure keyword.
 */

int is_ctrl_keyword (const char *s) {

  int i;

  for (i = 0; i < N_CTRL_KEYWORDS; i++) {
    if (*s == *c_ctrl_keywords[i])
      if (!strcmp (s, c_ctrl_keywords[i]))
	return TRUE;
  }

  return FALSE;
}
