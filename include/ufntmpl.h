/* $Id: ufntmpl.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

/*
 *  A template name for a user function has the following prefix with
 *  the first alphabetic character uppercased, following
 *  the convention of the C library templates.  User templates also
 *  are prefixed with a decimal extent, and are not cached.  Ctalk
 *  creates a new template for each use of the function, with the
 *  arguments in the original expression.
 */
#define UFN_PFX "c"

/*
 *  These templates are not preprocessed!
 */

/*
 *  This should be similar to the methodReturnInteger macro
 *  in ctalklib.
 */
#define INT_FN_RETURN_TMPL "\n\
OBJECT *%s (%s) { \n\
  { char __b[8192]; OBJECT *result;\n\
    __ctalkDecimalIntegerToASCII((%s),__b); \n\
    result = __ctalkCreateObjectInit (\"result\", \n\
    \"Integer\", \"Magnitude\", (1 << 1), __b); \n\
    __ctalkRegisterUserObject (result); \n\
    return result; \n\
  } \n\
} \n\
"

#define INT_FN_RETURN_TMPL_FROM_CVAR "\n\
OBJECT *%s (%s) { \n\
  { char __b[8192]; OBJECT *result;\n\
    __ctalkDecimalIntegerToASCII((%s),__b); \n\
    __ctalkTemplateCallerCVARCleanup (); \n\
    result = __ctalkCreateObjectInit (\"result\", \n\
    \"Integer\", \"Magnitude\", (1 << 1), __b); \n\
    __ctalkRegisterUserObject (result); \n\
    return result; \n\
  } \n\
} \n\
"

#define CHAR_FN_RETURN_TMPL "\n\
OBJECT *%s (%s) { \n\
  { char __b[2]; OBJECT *result;\n\
    __b[0] = (char)%s; \n\
    __b[1] = 0; \n\
    result =  __ctalkCreateObjectInit (\"result\", \n\
    \"Character\", \"Magnitude\", (1 << 1), __b); \n\
    __ctalkRegisterUserObject (result); \n\
    return result; \n\
  } \n\
} \n\
"

#define CHAR_FN_RETURN_TMPL_FROM_CVAR "\n\
OBJECT *%s (%s) { \n\
  { char __b[2]; OBJECT *result;\n\
    __b[0] = (char)%s; \n\
    __b[1] = 0; \n\
    __ctalkTemplateCallerCVARCleanup (); \n\
    result =  __ctalkCreateObjectInit (\"result\", \n\
    \"Character\", \"Magnitude\", (1 << 1), __b); \n\
    __ctalkRegisterUserObject (result); \n\
    return result; \n\
  } \n\
} \n\
"

#define CHAR_PTR_FN_RETURN_TMPL "\n\
OBJECT *%s (%s) { \n\
  { char __b[8192], *__b_ptr; OBJECT *result;\n\
    __b_ptr = %s; \n\
    if (__b_ptr==(void *)0) { \n\
      result = __ctalkCreateObjectInit (\"result\", \n\
      \"String\", \"Character\", (1 << 1), \"0x0\"); \n\
    } else {\n\
      strcpy (__b, __b_ptr); \n\
      result = __ctalkCreateObjectInit (\"result\", \n\
      \"String\", \"Character\", (1 << 1), __b); \n\
    } \n\
    __ctalkRegisterUserObject (result); \n\
    return result; \n\
  } \n\
} \n\
"

#define CHAR_PTR_FN_RETURN_TMPL_FROM_CVAR "\n\
OBJECT *%s (%s) { \n\
  { char __b[8192], *__b_ptr; OBJECT *result;\n\
    __b_ptr = %s; \n\
    if (__b_ptr==(void *)0) { \n\
      result = __ctalkCreateObjectInit (\"result\", \n\
      \"String\", \"Character\", (1 << 1), \"0x0\"); \n\
    } else {\n\
      strcpy (__b, __b_ptr); \n\
      result = __ctalkCreateObjectInit (\"result\", \n\
      \"String\", \"Character\", (1 << 1), __b); \n\
    } \n\
    __ctalkTemplateCallerCVARCleanup (); \n\
    __ctalkRegisterUserObject (result); \n\
    return result; \n\
  } \n\
} \n\
"

#define DOUBLE_FN_RETURN_TMPL "\n\
OBJECT *%s (%s) { \n\
  { char __b[8192]; OBJECT *result;\n\
    __ctalkDoubleToASCII ((%s), __b); \n\
    result = __ctalkCreateObjectInit (\"result\", \n\
    \"Float\", \"Magnitude\", (1 << 1), __b); \n\
    __ctalkRegisterUserObject (result); \n\
    return result; \n\
  } \n\
} \n\
"

#define DOUBLE_FN_RETURN_TMPL_FROM_CVAR "\n\
OBJECT *%s (%s) { \n\
  { char __b[8192]; OBJECT *result;\n\
    __ctalkDoubleToASCII ((%s), __b); \n\
    result = __ctalkCreateObjectInit (\"result\", \n\
    \"Float\", \"Magnitude\", (1 << 1), __b); \n\
    __ctalkTemplateCallerCVARCleanup (); \n\
    __ctalkRegisterUserObject (result); \n\
    return result; \n\
  } \n\
} \n\
"

#define VOID_FN_RETURN_TMPL "\n\
OBJECT *%s (%s) { \n\
  %s; \n\
  return ((void *)0); \n\
} \n\
"


