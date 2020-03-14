/* $Id: ctalkdefs.h,v 1.2 2020/01/11 03:27:30 rkiesling Exp $ -*-C-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014-2015, 2017, 2019  Robert Kiesling, rk3314042@gmail.com.
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

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#ifndef _CTALKDEFS_H_

#include <stdbool.h>
/* Other capitalizations of True and False are compatible with Xlib.h. */
#define True 1
#define False 0
/* Defining TRUE as !(FALSE) is too challenging for ctpp in some involved
   rescanning, sooo ...  */
#ifdef TRUE
#undef TRUE
#define TRUE 1
#endif
#ifdef FALSE
#undef FALSE
#define FALSE 0
#endif

#ifdef __x86)64
#define FMT_0XHEX(__x) "%#lx",((unsigned long int)__x)
#else
#define FMT_0XHEX(__x) "%#x",((unsigned int)__x)
#endif
#if defined (__APPLE__) && defined (__ppc__)
#define STR_0XHEX_TO_PTR(__x) "0x%p",((void *)__x)
#else
#define STR_0XHEX_TO_PTR(__x) "0x%p",((void **)__x)
#endif

#define __LIST_HEAD(__o) ((__o)->instancevars->next)

#ifndef __sparc__
#define IS_OBJECT(x) ((x) && (*(int*)(x) == 0xd3d3d3))
#else
int memcmp(const void *s1, const void *s2, size_t n);
#define IS_OBJECT(x) ((x) && !memcmp ((void *)x, "OBJECT", 6))
#endif

#define IS_VALUE_INSTANCE_VAR(__o) ((__o) && IS_OBJECT(__o) && \
                                    (__o)->__o_p_obj && \
                                  IS_OBJECT((__o)->__o_p_obj) && \
                                  ((__o)->__o_p_obj->instancevars==(__o)))

#ifndef OBJREF
#define OBJREF(__o) (&(__o))
#endif

#ifndef NULLSTR
#define NULLSTR "(null)"
#endif

#ifndef TRIM_CHAR
#define TRIM_CHAR(c)    ((c[0] == '\'' && c[1]!=0) ? \
			 substrcpy (c,c,1,strlen (c)-2) : c)
#endif
#ifndef TRIM_CHAR_BUF
#define TRIM_CHAR_BUF(s) \
  { \
    while ((s[0] == '\'') && (s[1] != '\0')) \
      substrcpy (s, s, 1, strlen (s) - 2); \
  } \

#endif

#ifndef TRIM_LITERAL
#define TRIM_LITERAL(s) (substrcpy (s, s, 1, strlen (s) - 2))
#endif


#ifndef MAXMSG
#define MAXMSG 8192
#endif

#ifndef MAXLABEL
#define MAXLABEL 0x100
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef EOF
#define EOF -1
#endif

#define FILEEOF(buf) {buf[0] = EOF; buf[1] = 0;}

#define DIRECTORY_LIB_ERROR -1

#ifndef IS_ARG
#define ARG_SIG 0xf2f2f2
#define IS_ARG(a) ((a) && (a) -> sig == ARG_SIG)
#endif

#ifndef CLASSNAME
#define CLASSNAME __o_class->__o_name
#endif
#ifndef SUPERCLASSNAME
#define SUPERCLASSNAME __o_superclass->__o_name
#endif
#ifndef _SUPERCLASSNAME
/* NOTE: Use this only with OBJECT *'s.  With objects
   constructed with "new" or "basicNew" or something similar
   that uses the Object : -> method, use expressions like
   these.

     <object> -> __o_superclassname

   and

    <object> -> SUPERCLASSNAME

  The Ctalk library translates them similarly to get
  the superclass name (or an empty string if the
  object doesn't have a superclass, like an "Object"
  object.
*/
#define _SUPERCLASSNAME(o) ((IS_OBJECT((o)->__o_class) && \
	IS_OBJECT((o)->__o_class->__o_superclass)) ? \
	(o) -> __o_class -> __o_superclass -> __o_name : "")
#endif

#ifndef ARG_OBJECT
#define ARG_OBJECT(a) (IS_ARG(a) ? (a)->obj : NULL) 
#endif
#ifndef ARG_NAME
#define ARG_NAME(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->__o_name : NULL) 
#endif
#ifndef ARG_CLASSNAME
#define ARG_CLASSNAME(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->CLASSNAME : NULL) 
#endif
#ifndef ARG_SUPERCLASSNAME
#define ARG_SUPERCLASSNAME(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
			       SUPERCLASSNAME((a)->obj) : NULL) 
#endif
#ifndef ARG_CLASS
#define ARG_CLASS(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->__o_class : NULL) 
#endif
#ifndef ARG_SUPERCLASS
#define ARG_SUPERCLASS(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->__o_superclass : NULL) 
#endif
#ifndef ARG_NREFS
#define ARG_NREFS(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->nrefs : 0) 
#endif
#ifndef ARG_SCOPE
#define ARG_SCOPE(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->scope : 0) 
#endif

#ifndef HAS_CREATED_CVAR_SCOPE
#define HAS_CREATED_CVAR_SCOPE(o)  (((o) -> scope & LOCAL_VAR) && \
				    ((o) -> scope & CVAR_VAR_ALIAS_COPY) && \
				    ((o) -> __o_vartags -> tag == NULL))
#endif

#ifndef LOCAL_VAR
#define LOCAL_VAR (1 << 1)
#endif
#ifndef GLOBAL_VAR
#define GLOBAL_VAR (1 << 0)
#endif
#ifndef CREATED_PARAM 
#define CREATED_PARAM (1 << 6)
#endif
#ifndef CVAR_VAR_ALIAS_COPY
#define CVAR_VAR_ALIAS_COPY (1 << 7)
#endif
#ifndef VAR_REF_OBJECT 
#define VAR_REF_OBJECT (1 << 9)
#endif
#ifndef METHOD_USER_OBJECT
#define METHOD_USER_OBJECT (1 << 10)
#endif

/* Object attributes that we need.  These are from the
   definitions in include/object.h. */
#define OBJECT_IS_VALUE_VAR                   (1 << 0)
#define OBJECT_VALUE_IS_C_CHAR_PTR_PTR        (1 << 1)
#define OBJECT_IS_NULL_RESULT_OBJECT          (1 << 2)
#define OBJECT_HAS_PTR_CX                     (1 << 3)
#define OBJECT_IS_GLOBAL_COPY                 (1 << 4)
#define OBJECT_IS_I_RESULT                    (1 << 5)
#define OBJECT_IS_STRING_LITERAL              (1 << 6)
#define OBJECT_IS_MEMBER_OF_PARENT_COLLECTION (1 << 7)
#define OBJECT_HAS_LOCAL_TAG                  (1 << 8)
#define OBJECT_IS_DEREF_RESULT                (1 << 14)


/* ANSI/VT100/Xterm escape sequences. */
#ifndef  VT_REV
#define VT_REV    "\033[7m"     /* Reverse video.                    */
#endif
#ifndef VT_NORM
#define VT_NORM   "\033[0m"     /* Normal video.                     */
#endif
#ifndef VT_CPOS
#define VT_CPOS   "\033[%d;%df" /* Set cursor position: row, column. */
#endif
#ifndef VT_CLS
#define VT_CLS    "\033[2J"     /* Clear the screen.                 */
#endif
#ifndef VT_RESET
#define VT_RESET  "\033[!p"     /* Soft reset.                       */
#endif
#ifndef VT_SGR
#define VT_SGR    "\033[%cm"    /* Set graphics.                     */
#endif
#ifndef VT_SCS_GRAPHICS
#define VT_SCS_GRAPHICS    "\033(0"    /* Set graphics characters.   */
#endif
#ifndef VT_SCS_ASCII
#define VT_SCS_ASCII       "\033(B"    /* Set ASCII characters.      */
#endif
#ifndef VT_C_HIDE
#define VT_C_HIDE       "\033[>5l"  /* Cursor hidden - extension. */
#endif
#ifndef VT_C_SHOW
#define VT_C_SHOW       "\033[>5h"  /* Cursor visible - extension. */
#endif

#ifndef CTRLC
#define CTRLC   0x03
#endif
#ifndef CTRLD
#define CTRLD   0x04
#endif
#ifndef CTRLH
#define CTRLH   0x08
#endif
#ifndef TAB
#define TAB     0x09
#endif
#ifndef CTRLZ
#define CTRLZ   0x1a
#endif
#ifndef ESC
#define ESC     0x1b
#endif
#ifndef CSI2
#define CSI2    0x5b     /* ESC [ sequence is CSI */
#endif
#ifndef DEL
#define DEL     0x7f
#endif

#ifndef KBDCHAR                 /* These are also defined in */
#define KBDCHAR      (1 << 0)   /* TerminalStream class.     */
#endif
#ifndef KBDCUR   
#define KBDCUR       (1 << 1)
#endif

#ifndef STR_IS_NULL
#define STR_IS_NULL(__s) (str_is_zero_q(__s))
#endif

#ifndef I_UNDEF
#define I_UNDEF ((void *)-1)
#endif

#ifndef TAG_REF_PREFIX
#define TAG_REF_PREFIX 0
#endif
#ifndef TAG_REF_POSTFIX
#define TAG_REF_POSTFIX 1
#endif
#ifndef TAG_REF_TEMP
#define TAG_REF_TEMP 2
#endif

/* Used by __xfree ().  If you prefer to call __xfree () direcly, then
   comment out the define above, and use MEMADDR () to cast the buffer
   to a void ** .*/
#ifndef MEMADDR
#define MEMADDR(x) ((void **)&(x))
#endif

/* Used by the methods in LibrarySearch class. */
#define LIBDOCPATH "/usr/local/share/ctalk/libdoc"
#define KEYPAT ">>>"

#define X_FACE_REGULAR     (1 << 0)
#define X_FACE_BOLD        (1 << 1)
#define X_FACE_ITALIC      (1 << 2)
#define X_FACE_BOLD_ITALIC (1 << 3)

/* Note that port numbers < 1024 are reserved. See services(5). */
#ifndef DEFAULT_TCPIP_PORT
#define DEFAULT_TCPIP_PORT 9998
#endif

/* Text justification in X11LabelPane */
#define LABEL_LEFT   1
#define LABEL_CENTER 2
#define LABEL_RIGHT  3

#define _CTALKDEFS_H_
#endif /* #ifdef _CTALKDEFS_H_ */
