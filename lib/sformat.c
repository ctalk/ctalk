/* $Id: sformat.c,v 1.2 2020/09/18 21:25:13 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017, 2019, 2020  Robert Kiesling, 
    rk3314042@gmail.com.
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "typeof.h"
#include "defcls.h"

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

static char __fmtbuf[0xFFFF];
static char __tmpbuf[0xFFFF];

#ifndef TRIM_CHAR_BUF
#define TRIM_CHAR_BUF(s) \
  { \
    while ((s[0] == '\'')&&(s[1] != '\0')) \
       substrcpy (s, s, 1, strlen (s) - 2); \
  } \

#endif
#ifndef TRIM_LITERAL_BUF
#define TRIM_LITERAL_BUF(s) \
  { \
    while ((s[0] == '\"')&&(s[1] != '\0')) \
       substrcpy (s, s, 1, strlen (s) - 2); \
  } \

#endif

#define ARG_VAL_OBJ(__o) ((__o)->instancevars ? \
  (__o)->instancevars : (__o))

char *__scalar_fmt_conv (char *fmt, char *arg, OBJECT *arg_obj) {

  char fmt_conv_chr;

  strcpy (__tmpbuf, arg);

  if (ARG_VAL_OBJ(arg_obj) -> __o_class == rt_defclasses -> p_string_class) {
    if (*arg == '\'') {
      TRIM_CHAR_BUF(__tmpbuf);
    } else {
      if (*arg == '\"') {
	TRIM_LITERAL_BUF(__tmpbuf);
      }
    }

    fmt_conv_chr = fmt[strlen(fmt) - 1];
    if ((*fmt == '%') && (fmt_conv_chr == 'f' || fmt_conv_chr == 'd' ||
			  fmt_conv_chr == 'i' || fmt_conv_chr == 'u' ||
			  fmt_conv_chr == 'o')) {
      sprintf (__fmtbuf, fmt, *__tmpbuf);
      return __fmtbuf;
    } else if ((*fmt == '%') && (fmt_conv_chr == 'x' || fmt_conv_chr == 'X')) {
      sprintf (__fmtbuf, fmt, __tmpbuf);
      return __fmtbuf;
    } else {
      return __tmpbuf;
    }

  }

  if (is_class_or_subclass (ARG_VAL_OBJ(arg_obj),
			    rt_defclasses -> p_character_class)) {
    if (*arg == '\'') {
      TRIM_CHAR_BUF(__tmpbuf);
    } else {
      if (*arg == '\"') {
	TRIM_LITERAL_BUF(__tmpbuf);
      }
    }

    fmt_conv_chr = fmt[strlen(fmt) - 1];
    if ((*fmt == '%') && (fmt_conv_chr == 'f' || fmt_conv_chr == 'd' ||
			  fmt_conv_chr == 'i' || fmt_conv_chr == 'u' ||
			  fmt_conv_chr == 'o' || fmt_conv_chr == 'x' ||
			  fmt_conv_chr == 'X')) {
      sprintf (__fmtbuf, fmt, *__tmpbuf);
      return __fmtbuf;
    } else {
      return __tmpbuf;
    }

  }
  if (is_class_or_subclass (ARG_VAL_OBJ(arg_obj),
			    rt_defclasses -> p_longinteger_class)) {
    /*
     *  Solaris workaround for what seems  to be a bug in  sprintf, 
     *  below.
     *  TODO - Could use a truncation warning.
     *  Also seems to be a bug in Linux on SPARC -- 
     *  Needs further testing.
     */
#if defined(__sparc__)
    int __arg_val;
#else
#if defined (__APPLE__)
    long int __arg_val;
#else
    long long int __arg_val;
#endif
#endif
    if (*arg == '\'') {
      TRIM_CHAR_BUF(__tmpbuf);
    } else {
      if (*arg == '\"') {
	TRIM_LITERAL_BUF(__tmpbuf);
      }
    }
     if (strstr (fmt, "%s")) {
       sprintf (__fmtbuf, fmt, __tmpbuf);
     } else {
#ifndef __ppc__  /* i.e., little-endian */
       sprintf (__fmtbuf, fmt, LLVAL(arg_obj -> __o_value));
#else 
       /* i.e., big-endian */
       /* this should be much more thorough if we ever need it. */
       switch (fmt[strlen (fmt) - 1])
	 {
	 case 'c':
	   sprintf (__fmtbuf, fmt, (char)LLVAL(arg_obj -> __o_value));
	   break;
	 case 'd':
	   sprintf (__fmtbuf, fmt, (int)LLVAL(arg_obj -> __o_value));
	   break;
	 case 'u':
	   sprintf (__fmtbuf, fmt, (unsigned)LLVAL(arg_obj -> __o_value));
	   break;
	 }
#endif
     }
    return __fmtbuf;
  }
  if (is_class_or_subclass (ARG_VAL_OBJ(arg_obj),
			    rt_defclasses -> p_integer_class)) {
    int __arg_val;
    if (*arg == '\'') {
      TRIM_CHAR_BUF(__tmpbuf);
    } else {
      if (*arg == '\"') {
	TRIM_LITERAL_BUF(__tmpbuf);
      }
    }
    if (strstr (fmt, "%s")) {
      sprintf (__fmtbuf, fmt, __tmpbuf);
      return __fmtbuf;
    } else {
      if ((ARG_VAL_OBJ(arg_obj) -> attrs & OBJECT_VALUE_IS_BIN_INT) ||
	  (ARG_VAL_OBJ(arg_obj) -> __o_class -> attrs & INT_BUF_SIZE_INIT)) {
	sprintf (__fmtbuf, fmt, *(int *)(ARG_VAL_OBJ(arg_obj) -> __o_value));
	return __fmtbuf;
      } else {
	__arg_val = atoi (__tmpbuf);
	sprintf (__fmtbuf, fmt, __arg_val);
	return __fmtbuf;
      }
    }
  }
  if (is_class_or_subclass (ARG_VAL_OBJ(arg_obj),
			    rt_defclasses -> p_float_class)) {
    double __arg_val;
    if (*arg == '\'') {
      TRIM_CHAR_BUF(__tmpbuf);
    } else {
      if (*arg == '\"') {
	TRIM_LITERAL_BUF(__tmpbuf);
      }
    }
    errno = 0;
    __arg_val = strtold (__tmpbuf, NULL);
    if (errno != 0) {
      strtol_error (errno, "__scalar_fmt_conv ()", __tmpbuf);
      return NULL;
    }
    sprintf (__fmtbuf, fmt, __arg_val);
    return __fmtbuf;
  }
  if (is_class_or_subclass (ARG_VAL_OBJ(arg_obj),
			    rt_defclasses -> p_symbol_class)) {
    void *__arg_val;
    if (*arg == '\'') {
      TRIM_CHAR_BUF(__tmpbuf);
    } else {
      if (*arg == '\"') {
	TRIM_LITERAL_BUF(__tmpbuf);
      }
    }
    errno = 0;
    __arg_val = (void *)strtoul (__tmpbuf, NULL, 16);
    if (errno != 0) {
      strtol_error (errno, "__scalar_fmt_conv ()", __tmpbuf);
      return NULL;
    }
    sprintf (__fmtbuf, fmt, __arg_val);
    return __fmtbuf;
  }

  return __tmpbuf;
}

void *__ctalkStrToPtr (char *s) {
  static void *__p;
  errno = 0;
  __p = (void *)strtoul (s, NULL, 16);
  if (errno != 0) {
    strtol_error (errno, "__ctalkStrToPtr ()", s);
    return NULL;
  }
  return __p;
}

void __ctalkObjValPtr (OBJECT *o, void *p) {
  char buf[MAXLABEL];
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
  sprintf (buf, "%#lx", (unsigned long int)p);
#else
  htoa (buf, (unsigned int)p);
#endif
  if (IS_VALUE_INSTANCE_VAR(o)) {
    __ctalkSetObjectValue (o, buf);
  } else {
    __ctalkSetObjectValueVar (o, buf);
  }
}

bool is_zero_q (char *s) {
  int i;
  if (*s == '\0')
    return true;

  if (str_eq (s, NULLSTR))
    return true;

  for (i = 0; s[i]; ++i) {
    if (s[i] >= '0' && s[i] <= '9') {
      if (s[i] != '0') {
	return false;
      }
    } else {
      if (s[i] == '.' || s[i] == 'x' || s[i] == 'X' || s[i] == 'l' ||
	  s[i] == 'L')
	continue;
      else
	return false;
    }
    
  }
  return true;
}

bool str_is_zero_q (char *s) {
  int i;

  if (!s)
    return true;

  if (*s == '\0')
    return true;

  if (str_eq (s, NULLSTR))
    return true;

  if (s[0] == '\0')
    return true;
  if (s[0] == '0' && s[1] == '\0')
    return false;

  for (i = 0; s[i]; ++i) {
    if (s[i] >= '0' && s[i] <= '9') {
      if (s[i] != '0') {
	return false;
      }
    } else {
      if (s[i] == '.' || s[i] == 'x' || s[i] == 'X' || s[i] == 'l' ||
	  s[i] == 'L')
	continue;
      else
	return false;
    }
    
  }
  return true;
}
