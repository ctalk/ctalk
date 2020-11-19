/* $Id: rtobjref.c,v 1.6 2020/11/19 14:05:45 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2014, 2017, 2019  Robert Kiesling, rk3314042@gmail.com.
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

/*
 *  Functions to handle object reference counts.  The arguments to 
 *  the function calls should use the OBJREF() macro, which is defined
 *  in object.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

/*
 *  OBJREF_T is defined in object.h.
 */

/* We can use a macro to check the contents of (possible)
   pointers,since we don't need to worry about regionalization.... */
#define ct_xdigit(c) (((c)>= '0' && (c) <= '9') || \
		   ((c) >= 'a' && (c) <= 'f') ||	\
		   ((c) >= 'A' && (c) <= 'F'))
/* The macro is untested with Sparc/Sun, tho'.  So if it causes
   problems, just comment out the macro above and uncomment this 
   line -  */
/*#define ct_xdigit isxdigit */

void __refObj (OBJREF_T ref1, OBJREF_T ref2) {
  OBJECT *var;
  if (IS_OBJECT (*ref1)) {
    (void)__objRefCntDec (ref1);
  }
  *ref1 = *ref2;
  (void)__objRefCntInc(ref2);
}

static void __obj_ref_cnt_store (OBJREF_T obj_p, int nrefs) {
  OBJECT *vars;

  if (IS_OBJECT (*obj_p)) {
    (*obj_p) -> nrefs = nrefs;
    for (vars = (*obj_p) -> instancevars; vars; vars = vars -> next)
      __obj_ref_cnt_store (OBJREF (vars), nrefs);
  } else {
#ifdef DEBUG_INVALID_OBJ_REFS
    _warning ("__objRefCntSet: Not an object.\n");
#endif
  }
}

void __objRefCntInc (OBJREF_T obj_p) {

  if (*obj_p == NULL)
    return;

  __obj_ref_cnt_store (obj_p, (*obj_p) -> nrefs + 1);
}

void __objRefCntDec (OBJREF_T obj_p) {

  if (*obj_p == NULL)
    return;
  if ((*obj_p) -> nrefs == 0)
    return;

  __obj_ref_cnt_store (obj_p, (*obj_p) -> nrefs - 1);

}

void __objRefCntSet (OBJREF_T obj_p, int nrefs) {
  OBJECT *vars;

  /* 
     This line helps prevent any call from zapping an object.  
     If we want an object to be expressly deleted, call 
     __objRefCntZero () before __ctalkDeleteObject* () - which 
     is what the Object::delete method does.
  */
  if (nrefs == 0) return;

  if (!IS_OBJECT (*obj_p)) {
#ifdef DEBUG_INVALID_OBJ_REFS
    _warning ("__objRefCntSet: Not an object.\n");
#endif
  } else {
    (*obj_p) -> nrefs = nrefs;
    for (vars = (*obj_p) -> instancevars; vars; vars = vars -> next)
      (void)__objRefCntSet (OBJREF (vars), nrefs);
  }
}

void __objRefCntZero (OBJREF_T obj_p) {
  OBJECT *vars;

  if (!IS_OBJECT (*obj_p)) {
#ifdef DEBUG_INVALID_OBJ_REFS
    _warning ("__objRefCntSet: Not an object.\n");
#endif
  } else {
    (*obj_p) -> nrefs = 0;
    for (vars = (*obj_p) -> instancevars; vars; vars = vars -> next) {
      if (IS_OBJECT(vars)) {
	(void)__objRefCntZero (OBJREF (vars));
	if (!IS_OBJECT (vars -> next)) {
	  break;
	}
      } else {
	break;
      }
    }
  }
}

#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
#define OBJREF_IS_OBJECT(x) (((long int)(x) > 0) && IS_OBJECT(x))
#else
#define OBJREF_IS_OBJECT(x) (((int)(x) > 0) && IS_OBJECT(x))
#endif

OBJECT *obj_ref_str (char *__s) {
  OBJECT *__r;
  if (__s == NULL)
    return NULL;

  /* See the comments with the definition of ct_xdigit, above. 
   (We really, really do need to check every digit of a
   possible pointer.) */

#ifdef __x86_64

# ifdef __APPLE__

  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) && 
      (__s[7] && ct_xdigit((int)__s[7])) &&
      (__s[8] && ct_xdigit((int)__s[8])) &&
      (__s[9] && ct_xdigit((int)__s[9])) &&
      (__s[10] && ct_xdigit((int)__s[10])) &&
      (__s[11] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (OBJREF_IS_OBJECT(__r)) {
	return __r;
      }
    }
  }

# else /* __APPLE__ */

  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) && 
      (__s[7] && ct_xdigit((int)__s[7])) &&
      (__s[8] && ct_xdigit((int)__s[8])) &&
      (__s[9] && ct_xdigit((int)__s[9])) &&
      (__s[10] && ct_xdigit((int)__s[10])) &&
      (__s[11] && ct_xdigit((int)__s[11])) &&
      (__s[12] && ct_xdigit((int)__s[12])) &&
      (__s[13] && ct_xdigit((int)__s[13])) &&
      (__s[14] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (OBJREF_IS_OBJECT(__r)) {
	return __r;
      }
    }
  }

# endif /* __APPLE__ */

#else /* __x86_64 */

#if ((defined (__sparc__) && defined (__GNUC__)) ||	\
     (defined (__APPLE__) && defined (__GNUC__)))
  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) && 
      (__s[7] && ct_xdigit((int)__s[7])) &&
      (__s[8] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (OBJREF_IS_OBJECT(__r)) {
	return __r;
      }
    }
  }
  /*
   *  In case Solaris or OS X uses different addressing.
   */
  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) &&
      (__s[7] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (OBJREF_IS_OBJECT(__r)) {
	return __r;
      }
    }
  }
#else

  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) && 
      (__s[7] && ct_xdigit((int)__s[7])) && 
      (__s[8] && ct_xdigit((int)__s[8])) &&
      (__s[9] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (((int)__r == INT_MAX) || (__r == I_UNDEF))
	return NULL;
      if (IS_OBJECT(__r))
	return __r;
    }
  }

#endif
#endif /* __x86_64 */  

  return NULL;
}

static OBJECT *obj_ref_chk_str (OBJECT *h_obj, uintptr_t t_obj,
				uintptr_t mask, char *s) { /***/
  OBJECT *r;
  char objstr[0xff];
  if ((t_obj & mask) == ((uintptr_t)h_obj & mask)) {
    if (((uintptr_t)h_obj & mask) == 0) {
      return NULL;
    }
    if ((r = (OBJECT *)__ctalkStrToPtr (s)) != NULL) {
      sprintf (objstr, "%p", h_obj);
      if (strlen (s) != strlen (objstr)) {
	return NULL;
      }
      if (!OBJREF_IS_OBJECT(r)) {
	r = NULL;
      }
    }
  } else {
    r = NULL;
  }
  return r;
}

static bool obj_ref_chk_str_seg (OBJECT *h_obj, uintptr_t t_obj,
				 uintptr_t mask) {
  if (((uintptr_t)h_obj != 0) &&
      ((uintptr_t)t_obj != 0) &&
      (((uintptr_t)h_obj & mask) == ((uintptr_t)t_obj & mask))) {
    return true;
  }
  return false;
}

/* Similar to obj_ref_str, but the obj parameter provides the
   fn with a heap location so we can guage whether a number
   refers to a valid memory location.
*/
OBJECT *obj_ref_str_2 (char *__s, OBJECT *obj) {
  OBJECT *__r;
  char objstr[0xff];
  if (__s == NULL)
    return NULL;

  /* See the comments with the definition of ct_xdigit, above. 
   (We really, really do need to check every digit of a
   possible pointer.) */

#ifdef __x86_64

# ifdef __APPLE__
  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) && 
      (__s[7] && ct_xdigit((int)__s[7])) &&
      (__s[8] && ct_xdigit((int)__s[8])) &&
      (__s[9] && ct_xdigit((int)__s[9])) &&
      (__s[10] && ct_xdigit((int)__s[10])) &&
      (__s[11] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (OBJREF_IS_OBJECT(__r)) {
	return __r;
      }
    }
  }

# else /* __APPLE__ */

#if 1 /***/
  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) && 
      (__s[7] && ct_xdigit((int)__s[7])) &&
      (__s[8] && ct_xdigit((int)__s[8])) &&
      (__s[9] && ct_xdigit((int)__s[9])) &&
      (__s[10] && ct_xdigit((int)__s[10])) &&
      (__s[11] && ct_xdigit((int)__s[11])) &&
      (__s[12] && ct_xdigit((int)__s[12])) &&
      (__s[13] && ct_xdigit((int)__s[13])) &&
      (__s[14] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (obj_ref_chk_str_seg (obj, (uintptr_t)__r, 0xff0000000000) ||
	  obj_ref_chk_str_seg (obj, (uintptr_t)__r, 0xff000000000) ||
	  obj_ref_chk_str_seg (obj, (uintptr_t)__r, 0xff00000000) ||
	  obj_ref_chk_str_seg (obj, (uintptr_t)__r, 0xff0000000) ||
	  obj_ref_chk_str_seg (obj, (uintptr_t)__r, 0xff000000)) {
	/* if (OBJREF_IS_OBJECT(__r)) { */ /***/
	sprintf (objstr, "%p", obj);
	if (strlen (objstr) != strlen (__s)) {
	  return NULL;
	}
	if (*(int *)__r == 0xd3d3d3) {
	  return __r;
	}
      }
    }
  } else {
#endif    
    /* We might have a byte alignment that results in a negative char 
       or some other overflow - check. */
    uintptr_t tmp;
    if (!IS_OBJECT(obj))
      return NULL;
    errno = 0;
    __r = NULL;
    tmp = strtoul (__s, NULL, 16);
    if (errno == 0 && tmp != 0) {
      if (((uintptr_t)obj & 0xff0000000000) == 0) {
	if (((uintptr_t)obj & 0xff000000000) == 0) {
	  if (((uintptr_t)obj & 0xff00000000) == 0) {
	    if (((uintptr_t)obj & 0xff0000000) == 0) {
	      if (((uintptr_t)obj & 0xff000000) == 0) {
		if (((uintptr_t)obj & 0xff00000) == 0) {
		  return NULL;
		} else {
		  return obj_ref_chk_str (obj, tmp, 0xff00000, __s);
		}
	      } else {
		return obj_ref_chk_str (obj, tmp, 0xff000000, __s);
	      }
	    } else {
	      return obj_ref_chk_str (obj, tmp, 0xff0000000, __s);
	    }
	  } else {
	    return obj_ref_chk_str (obj, tmp, 0xff00000000, __s);
	  }
	} else {
	  return obj_ref_chk_str (obj, tmp, 0xff000000000, __s);
	}
      } else {
	return obj_ref_chk_str (obj, tmp, 0xff0000000000, __s);
      }
    }
    return __r;
#if 1 /***/
  }
#endif  

# endif /* __APPLE__ */

#else /* __x86_64 */

#if ((defined (__sparc__) && defined (__GNUC__)) ||	\
     (defined (__APPLE__) && defined (__GNUC__)))
  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) && 
      (__s[7] && ct_xdigit((int)__s[7])) &&
      (__s[8] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (OBJREF_IS_OBJECT(__r)) {
	return __r;
      }
    }
  }
  /*
   *  In case Solaris or OS X uses different addressing.
   */
  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) &&
      (__s[7] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (OBJREF_IS_OBJECT(__r)) {
	return __r;
      }
    }
  }
#else

  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x')) && 
      (__s[2] && ct_xdigit((int)__s[2])) && 
      (__s[3] && ct_xdigit((int)__s[3])) && 
      (__s[4] && ct_xdigit((int)__s[4])) && 
      (__s[5] && ct_xdigit((int)__s[5])) && 
      (__s[6] && ct_xdigit((int)__s[6])) && 
      (__s[7] && ct_xdigit((int)__s[7])) && 
      (__s[8] && ct_xdigit((int)__s[8])) &&
      (__s[9] == '\0')) {
    if ((__r = (OBJECT *)__ctalkStrToPtr (__s)) != NULL) {
      if (((int)__r == INT_MAX) || (__r == I_UNDEF))
	return NULL;
      if (IS_OBJECT(__r))
	return __r;
    }
  }

#endif
#endif /* __x86_64 */  

  return NULL;
}

void *generic_ptr_str (char *__s) {
  void *__r;
  int i;
  if ((__s[0] && (__s[0] == '0')) && 
      (__s[1] && (__s[1] == 'x'))) {
    for (i = 2; __s[i]; ++i) {
      if (!ct_xdigit ((int)__s[i])) {
	return NULL;
      }
    }
  } else {
    return NULL;
  }
  if ((__r = __ctalkStrToPtr (__s)) != NULL) {
    return __r;
  }
  return NULL;
}

void *__ctalkGenericPtrFromStr (char *s) {
  return generic_ptr_str (s);
}

/*
 *  Like __ctalkGenericPtrFromStr (), but also performs
 *  system-specific checking for the FILE *'s validity.  The fns
 *  fileno () and fstat () also set errno.  Returns a void *
 *  in order to keep the header files simple - an app or method 
 *  needs to do the cast of the return value to a FILE *.
 */
void *__ctalkFilePtrFromStr (char *s) {
  FILE *f;
  int r;
  struct stat statbuf;

  if ((f = generic_ptr_str (s)) != NULL) {
    if (fileno (f) != -1) {
      if ((r = fstat (fileno (f), &statbuf)) == 0) {
	return (void *)f;
      }
    }
  }
  return NULL;
}
