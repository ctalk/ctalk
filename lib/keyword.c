/* $Id: keyword.c,v 1.2 2019/11/20 21:08:03 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2017  Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include "cvar.h"
#include "message.h"
#include "object.h"
#include "ctalk.h"


#define FALSE 0
#define TRUE !(FALSE)

#define N_CTALK_KEYWORDS 11

int is_ctalk_keyword (const char *s) {
  int i;
  char *ctalk_keywords[] = {
    "self",
    "super",
    "class",
    "require",
    "eval",
    "instanceMethod",
    "classMethod",
    "instanceVariable",
    "classVariable",
    "returnObjectClass",
    "noMethodInit"
  };

  for (i = 0; i < N_CTALK_KEYWORDS; i++) {
    if (str_eq ((char *)s, ctalk_keywords[i]))
      return TRUE;
  }
  return FALSE;
}

#ifdef __GNUC__
#define N_MACRO_KEYWORDS 18
#else
#define N_MACRO_KEYWORDS 14
#endif

int is_macro_keyword (const char *s) {

  int i, is_keyword;
  char *macro_keywords[] = {
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
  };

  for (i = 0, is_keyword = FALSE; 
       (i < N_MACRO_KEYWORDS) && ! is_keyword; 
       i++) {
    if (str_eq ((char *)s, macro_keywords[i]))
      is_keyword = TRUE;
  }
  return is_keyword;
}

static char *c_c_keywords[] = {
  "break",
  "break;", /* see handle_blk_break_stmt () in control.c.  Eech. */
  "case",
  "continue",
  "continue;", /* same as for break; */
  "default",
  "do",
  "else",
  "for",
  "goto",
  "if",
  "inline",
  "return",
  "sizeof",
  "switch",
  "typedef",
  "while"
};

# define N_C_C_KEYWORDS 17

/* Check if a label is an ANSI C or C99 keyword, except for data
   types.
*/

bool is_c_c_keyword (const char *s) {

  int i;

  for (i = 0; i < N_C_C_KEYWORDS; i++) {
    if (str_eq ((char *)s, c_c_keywords[i]))
      return true;
  }

  return false;
}

static char *c_keywords[] = {
  "auto",
  "break",
  "break;", /* see handle_blk_break_stmt () in control.c.  Eech. */
  "case",
  "char",
  "const",
  "__const",
  "continue",
  "continue;", /* same as for break; */
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
  "__signed",
  "sizeof",
  "static",
  "struct",
  "switch",
  "typedef",
  "union",
  "unsigned",
  "void",
  "volatile",
  "__volatile",
  "while",
  "_Bool",
  "_Complex",
  "_Imaginary",
  "__float128"
#ifdef __GNUC__
  , "__complex__",
  "__thread",
  "__inline"
#endif
};

#ifdef __GNUC__
# define N_C_KEYWORDS 46
# else
# define N_C_KEYWORDS 43
#endif

/* Check if a label is an ANSI C or C99 keyword, including data types.  
   A subset that checks only the data types is below.
*/

#ifdef __GNUC__
inline
#endif
int is_c_keyword (const char *s) {

  int i;

  for (i = 0; i < N_C_KEYWORDS; i++) {
    if (str_eq ((char *)s, c_keywords[i]))
      return TRUE;
  }

  return FALSE;
}

/* 
 *    Check if a label is an ANSI C data type.  This is a subset of
 *    the ANSI C keywords, above.
*/

static char *c_data_types[] = {
  "char",
  "const",
  "__const",
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
  "__signed",
  "static",
  "struct",
  "union",
  "unsigned",
  "void",
  "volatile",
  "__volatile",
  "_Bool",
  "_Complex",
  "_Imaginary",
  "__float128"
#ifdef __GNUC__
  , "__complex__",
  "__inline__",
  "__inline",
  "__thread"
#endif
};

#ifdef __GNUC__
# define N_C_DATA_TYPES 29
# else
# define N_C_DATA_TYPES 26
#endif

/* 
 *    Check if a label is an ANSI C data type.  This is a subset of
 *    the ANSI C keywords, above.
*/

#ifdef __GNUC__
inline
#endif
int is_c_data_type (const char *s) {

  int i;

  for (i = 0; i < N_C_DATA_TYPES; i++)
    if (str_eq ((char *)s, c_data_types[i]))
      return TRUE;

  return FALSE;
}

static struct _type_attr_struct {
  char *name;
  int attr;
} type_attrs[] = {
  { "OBJECT",        CVAR_TYPE_OBJECT},
  { "char",          CVAR_TYPE_CHAR},
  { "const",         CVAR_TYPE_CONST},
  { "__const",       CVAR_TYPE_CONST},
  { "double",        CVAR_TYPE_DOUBLE},
  { "enum",          CVAR_TYPE_ENUM},
  { "extern",        CVAR_TYPE_EXTERN},
  { "float",         CVAR_TYPE_FLOAT},
  { "__float128",    CVAR_TYPE_FLOAT},
  { "inline",        CVAR_TYPE_INLINE},
  { "int",           CVAR_TYPE_INT},
  { "long",          CVAR_TYPE_LONG},
  { "register",      CVAR_TYPE_REGISTER},
  { "short",         CVAR_TYPE_SHORT},
  { "signed",        CVAR_TYPE_SIGNED},
  { "__signed",      CVAR_TYPE_SIGNED},
  { "static",        CVAR_TYPE_STATIC},
  { "struct",        CVAR_TYPE_STRUCT},
  { "union",         CVAR_TYPE_UNION},
  { "unsigned",      CVAR_TYPE_UNSIGNED},
  { "void",          CVAR_TYPE_VOID},
  { "volatile",      CVAR_TYPE_VOLATILE},
  { "__volatile",    CVAR_TYPE_VOLATILE},
  { "_Bool",         CVAR_TYPE_BOOL},
  { "_Complex",      CVAR_TYPE_COMPLEX},
  { "_Imaginary",    CVAR_TYPE_IMAGINARY},
  { "FILE",          CVAR_TYPE_FILE},
  { "__FILE",        CVAR_TYPE_FILE},
  { "__IO_FILE",     CVAR_TYPE_FILE}
#ifdef __GNUC__
  , { "__complex__", CVAR_TYPE_GNU_COMPLEX},
  { "__inline__",    CVAR_TYPE_GNU_INLINE},
  { "__inline",    CVAR_TYPE_GNU_INLINE},
  { "__thread",      CVAR_TYPE_GNU_THREAD}
#endif
};

#ifdef __GNUC__
# define N_C_DATA_TYPE_ATTRS 33
#else
# define N_C_DATA_TYPE_ATTRS 29
#endif

int is_c_data_type_attr (const char *s) {
  int i;

  for (i = 0; i < N_C_DATA_TYPE_ATTRS; i++)
    if (str_eq ((char *)s, type_attrs[i].name))
      return type_attrs[i].attr;

  return FALSE;
}

static char *c_storage_classes[] = {
  "const",
  "__const",
  "extern",
  "inline",
  "register",
  "static",
  "volatile",
  "__volatile",
  "__thread"
};

#define N_C_STORAGE_CLASSES 9

/* 
 *    Check if a label is an ANSI C storage class keyword.  
 *    This is a subset of the ANSI C keywords, above.
 */

#ifdef __GNUC__
inline
#endif
char *is_c_storage_class (const char *s) {

  int i, returnval = FALSE;

  for (i = 0; i < N_C_STORAGE_CLASSES; i++)
    if (str_eq ((char *)s, c_storage_classes[i]))
      return c_storage_classes[i];

  return NULL;
}

#ifdef __GNUC__

static char *gnu_c_extensions[] = {
  "__extension__",                 /* GNU C. */
  "__const",
  "__restrict",
  "__attribute__",
  "__mode__",
  "__inline",
  "__inline__",
  "__asm__",
  "__asm",
  "asm",
  "__volatile__"
}; 

#define N_GNU_EXTENSIONS 11

#else

#define N_GNU_EXTENSIONS 0

#endif

#ifdef __GNUC__
inline
#endif
int is_gnu_extension_keyword (const char *s) {

  int i, returnval = FALSE;

  for (i = 0; i < N_GNU_EXTENSIONS; i++)
    if (str_eq ((char *)s, gnu_c_extensions[i]))
      returnval = TRUE;

  return returnval;
}

static char *c_ctrl_keywords[] = {
  "break",
  "case",
  "default",
  "do",
  "else",
  "for",
  "goto",
  "if",
  "return",
  "switch",
  "while"
};

#define N_CTRL_KEYWORDS 11

/*
 *  Check if a label is a control structure keyword.
 */

#ifdef __GNUC__
inline
#endif
int is_ctrl_keyword (const char *s) {

  int i;

  for (i = 0; i < N_CTRL_KEYWORDS; i++) {
    if (str_eq ((char *)s, c_ctrl_keywords[i]))
      return TRUE;
  }

  return FALSE;
}

#ifdef __GNUC__

static char *gnuc_builtin_types[] = {
  "_pthread_descr_struct",
  "__builtin_va_list",
  "__dirstream",
  "__builtin_bswap32",
  "__builtin_bswap64",
  "__uint16_identity",
  "__uint32_identity",
  "__uint64_identity"
#ifdef DJGPP
  ,"signal",
  "__dj_DIR"
#endif
};


#ifdef DJGPP
 #define N_GNUC_BUILTIN_TYPES 10
#else
#define N_GNUC_BUILTIN_TYPES 8
#endif

#endif /* #ifdef __GNUC__ */

#ifdef __GNUC__
inline
#endif
int is_gnuc_builtin_type (const char *s) {

#ifdef __GNUC__
  int i;

  for (i = 0; i < N_GNUC_BUILTIN_TYPES; i++) {
    if (str_eq ((char *)s, gnuc_builtin_types[i]))
      return TRUE;
  }
#endif

  return FALSE;
}

#ifdef __APPLE__
# ifdef __ppc__

# include "osx_ppc_builtins.h"

int is_apple_ppc_math_builtin (const char *s) {
  int i;
  for (i = 0; __osx_ppc_math_builtins[i]; i++) {
    if (str_eq ((char *)s, __osx_ppc_math_builtins[i]))
      return TRUE;
  }
  if (!strncmp (s, "__builtin_", 10))
      return TRUE;
  return FALSE;
}

int is_apple_ppc_libkern_builtin (const char *s) {
  int i;
  for (i = 0; __osx_ppc_libkern_builtins[i]; i++) {
    if (str_eq ((char*)s, __osx_ppc_libkern_builtins[i]))
      return TRUE;
  }
  if (!strncmp (s, "__builtin_", 10))
      return TRUE;
  return FALSE;
}

# else /* __ppc__ */

# include "osx_i386_builtins.h"

/* somewhat of a misnomer - includes all functions defined in the
   include files */
int is_apple_i386_math_builtin (const char *s) {
  int i;
  for (i = 0; __osx_i386_math_builtins[i]; i++) {
    if (str_eq ((char *)s, __osx_i386_math_builtins[i]))
      return TRUE;
  }
  if (!strncmp (s, "__builtin_", 10))
      return TRUE;
  return FALSE;
}

int is_apple_i386_libkern_builtin (const char *s) {
  int i;
  for (i = 0; __osx_i386_libkern_builtins[i]; i++) {
    if (str_eq ((char *)s, __osx_i386_libkern_builtins[i]))
      return TRUE;
  }
  if (!strncmp (s, "__builtin_", 10))
      return TRUE;
  return FALSE;
}

#endif /* __ppc__ */
#endif /* __APPLE__ */

static char *OBJECT_member_names[] = {
  "sig",
  "__o_name",
  "__o_classname",
  "__o_class",
  "__o_superclassname",
  "__o_superclass",
  "__o_p_obj",
  "__o_vartags",
  "__o_value",
  "instance_methods",
  "class_methods",
  "tag",
  "scope",
  "nrefs",
  "classvars",
  "instancevars",
  "next",
  "prev",
  "attrs",
};

#define N_OBJECT_MEMBER_NAMES 19

int is_OBJECT_member (const char *s) {
  int i;
  for (i = 0; i < N_OBJECT_MEMBER_NAMES; i++)
    if (str_eq ((char *)s, OBJECT_member_names[i]))
      return TRUE;
  return FALSE;
}



