/* $Id: htoa.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2016-2019 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdint.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"


extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

#define DEC_TO_HEX_A(i) ((i >= 0 && i <= 9) ? (i + '0') : \
			 ((i >= 10 && i <= 15) ? (i + 'W') : '0'))

char *htoa (char *buf, uintptr_t ptr) {
  char l_buf[16], *q, *p;
  long int n_val;

  if (ptr == 0) {
    strcpy (buf, "0x0");
    return buf;
  }

#ifdef __x86_64
  n_val = (ptr & 0xf00000000000) >> 44; l_buf[0] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf0000000000) >> 40; l_buf[1] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf000000000) >> 36; l_buf[2] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf00000000) >> 32; l_buf[3] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf0000000) >> 28; l_buf[4] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf000000) >> 24; l_buf[5] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf00000) >> 20; l_buf[6] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf0000) >> 16; l_buf[7] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf000) >> 12; l_buf[8] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf00) >> 8; l_buf[9] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf0) >> 4; l_buf[10] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf); l_buf[11] = DEC_TO_HEX_A (n_val);
  l_buf[12] = '\0';
#else /* #ifdef __x86_64 */
  n_val = (ptr & 0xf0000000) >> 28; l_buf[0] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf000000) >> 24; l_buf[1] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf00000) >> 20; l_buf[2] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf0000) >> 16; l_buf[3] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf000) >> 12; l_buf[4] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf00) >> 8; l_buf[5] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf0) >> 4; l_buf[6] = DEC_TO_HEX_A (n_val);
  n_val = (ptr & 0xf); l_buf[7] = DEC_TO_HEX_A (n_val);
  l_buf[8] = '\0';
#endif  /* #ifdef __x86_64 */

  buf[0] = '0'; buf[1] = 'x'; buf[2] = '\0';
  q = l_buf;
  while (*q == '0')
    ++q;
  p = &buf[2];
  while (*p++ = *q++)
    ;
  return buf;
}

char *__ctalkHexIntegerToASCII (uintptr_t ptr, char *buf) {
  return htoa (buf, ptr);
}

int __ctalkReferenceObject (OBJECT *obj, OBJECT *reffed_obj) {
  char buf[MAXLABEL];
  OBJECT *value_object;
  void *val_alias;

  /*  First we have to remove the old value. */
  if ((value_object = *(OBJECT **)
       ((obj -> instancevars) ? 
	(obj -> instancevars -> __o_value) : 
	(obj -> __o_value))) != NULL) {
  (obj -> instancevars) ?
    (obj -> instancevars -> __o_value[0] = 0) :
    (obj -> __o_value[0] = 0);

    if (IS_OBJECT(value_object)) {
      __objRefCntSet (&value_object,
		      value_object -> nrefs - 1);
      if (value_object -> scope & LOCAL_VAR) {
	if (value_object -> nrefs <= 2) {
	  __ctalkSetObjectScope (value_object,
				 value_object -> scope & ~VAR_REF_OBJECT);
	}
	if (value_object -> scope & GLOBAL_VAR) {
	  if (value_object -> nrefs <= 1) {
	    __ctalkSetObjectScope (value_object,
				   value_object -> scope & ~VAR_REF_OBJECT);
	  }
	}
      }
      __ctalkRegisterExtraObject (value_object);
    }
  }

  if (IS_OBJECT(reffed_obj)) {

    /* This doesn't stash objects declared with "new" in the program. */
    __ctalkRegisterExtraObject (reffed_obj);

    if (reffed_obj -> __o_class == rt_defclasses -> p_symbol_class) {

      __ctalkSetObjectValue (obj, 
			     ((reffed_obj -> instancevars) ? 
			      reffed_obj -> instancevars -> __o_value :
			      reffed_obj -> __o_value));
      

    } else {
      
      __ctalkSetObjectScope (reffed_obj, reffed_obj -> scope | VAR_REF_OBJECT);
      __objRefCntInc (OBJREF(reffed_obj));

      if (obj -> instancevars) {
	val_alias = obj -> instancevars -> __o_value;
	memcpy (val_alias, &reffed_obj,	sizeof (void *));
      }
      val_alias = obj -> __o_value;
      memcpy (val_alias, &reffed_obj, sizeof (void *));

    }
  } else {
    if (obj -> instancevars) {
      SYMVAL(obj -> instancevars -> __o_value) = (uintptr_t)0;
    }
    SYMVAL(obj -> __o_value) = (uintptr_t)0;
  }

  return 0;

}
