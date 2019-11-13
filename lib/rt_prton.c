/* $Id: rt_prton.c,v 1.3 2019/10/30 02:10:37 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017, 2019  
    Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "typeof.h"

extern char ptr_args[512][8192];    /* Declared in rt_stdarg.c. */
extern int ptr_arg_idx;
extern char fmt_tokens[512][8192];
extern int fmt_tok_idx;

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

extern OBJECT *__ctalk_argstack[MAXARGS+1];
extern int __ctalk_arg_ptr;

static char tmparg0buf[0xFFFF];

static int __check_printon_args (METHOD *method) {
  int n, n_th_conv_arg;
  for (n = 0, n_th_conv_arg = 0; n < fmt_tok_idx; n++) {
    if (strchr (fmt_tokens[n], '%'))
      ++n_th_conv_arg;
  }
  if (n_th_conv_arg != ptr_arg_idx)
    return ERROR;
  return SUCCESS;
}

#define ARG_VAL_OBJ(__o) ((__o)->instancevars ? \
  (__o)->instancevars : (__o))

int __call_printon_fn_w_args (OBJECT *arg0_obj, char *fmt, METHOD *method, 
			      STDARG_CALL_INFO *stdarg_call_info) {

  int i, j, k, retval;
  int fmtsize;
  int bufsize = MAXMSG;
  char *arg0_buf;

  retval = 0;

  arg0_buf = __xalloc (bufsize + 1);
  fmtsize = strlen (fmt);

  retval = 0;
  if (stdarg_call_info -> libcfn == (int (*)())sprintf) {
    *arg0_buf = '\0';
    for (i = 0, j = 0, k = stdarg_call_info->fmt_arg_n + 1; (i <= fmt_tok_idx) && *fmt_tokens[i]; i++) {

      memset (tmparg0buf, 0, 0xffff);

    if (strstr (fmt_tokens[i], "%%")) {
      /* For lack of anything better.... */
      strncat (tmparg0buf, fmt_tokens[i], 1);
    } else {
      if (strchr (fmt_tokens[i], '%')) {
	strcatx2 (tmparg0buf, 
		__scalar_fmt_conv (fmt_tokens[i], 
				   ptr_args[j++],
				   method -> args[k++] -> obj), NULL);
	++retval;
      } else {
	strcatx2 (tmparg0buf, fmt_tokens[i], NULL);
      }
    }
      fmtsize += strlen (tmparg0buf);
      if (fmtsize >= bufsize) {
	while (fmtsize > bufsize)
	  bufsize *= 2;
	arg0_buf = realloc (arg0_buf, bufsize);
      }
      strcatx2 (arg0_buf, tmparg0buf, NULL);
    }
    __ctalkSetObjectValueVar(arg0_obj, arg0_buf);
    __xfree (MEMADDR(arg0_buf));
    return retval;
  }

  if (stdarg_call_info -> libcfn == (int (*)())fprintf) {
    OBJECT *arg0_val_obj;
    FILE *f_arg0;
    char *f_arg_buf;

    int stat_r, f_arg0_fileno;
    struct stat statbuf;

    if ((f_arg_buf = 
	 (char *)generic_ptr_str (ARG_VAL_OBJ(arg0_obj)->__o_value)) 
	== NULL) {
      _warning ("__call_printon_fn_w_args: invalid file argument.\n");
      retval = ERROR;
      goto fprintf_done;
    }

    arg0_val_obj = arg0_obj -> instancevars ? arg0_obj -> instancevars :
      arg0_obj;
    errno = 0;
    f_arg0 = (FILE *)strtoul (arg0_val_obj -> __o_value, NULL, 16);
    if (errno != 0)
      strtol_error (errno, "__call_prton_fn_w_args (1)",
		    arg0_val_obj -> __o_value);
    if ((f_arg0_fileno = fileno (f_arg0)) == ERROR) {
      _warning ("__call_printon_fn_w_args: invalid file handle.\n");
      retval = ERROR;
      goto fprintf_done;
    }
    if ((stat_r = fstat (f_arg0_fileno, &statbuf)) == ERROR) {
      _warning ("__call_printon_fn_w_args: %s.\n", strerror (errno));
      retval = ERROR;
      goto fprintf_done;
    }

    for (i = 0, j = 0, k = stdarg_call_info->fmt_arg_n + 1; (i <= fmt_tok_idx) && *fmt_tokens[i]; i++) {
      if (strchr (fmt_tokens[i], '%')) {
 	retval += fputs (__scalar_fmt_conv 
			   (fmt_tokens[i], 
			    ptr_args[j++],
			    method -> args[k++] -> obj),
			 f_arg0);
       } else {
	 retval += fputs (fmt_tokens[i], f_arg0);
       }
     }
  fprintf_done:
    __xfree (MEMADDR(arg0_buf));
    return retval;
   }
  return ERROR;
}

int __ctalkSelfPrintOn (void) {

  char fmtbuf[MAXMSG];
  OBJECT *fmt_value_obj;
  int retval = 0; /* Avoid a warning. */
  STDARG_CALL_INFO stdarg_call_info;
  OBJECT *arg0_obj = NULL;        /* Avoid a warning. */
  int k;
  OBJECT *read_value_obj;
  METHOD *method;
  int arg_frame;
  
  method = __ctalkRtGetMethod ();

  if (is_class_or_subclass (method -> rcvr_class_obj,
			    rt_defclasses -> p_string_class)) {
    stdarg_call_info.libcfn = (int (*)())sprintf;
  } else {
    if (!strcmp (method -> rcvr_class_obj -> __o_name, 
		 "WriteFileStream") ||
	__ctalkIsSubClassOf (method -> rcvr_class_obj -> __o_name,
			     "WriteFileStream")) {
      stdarg_call_info.libcfn = (int (*)())fprintf;
    } else {
      _warning 
	("__ctalkSelfPrintOn: Unimplemented receiver class, \"%s.\"\n",
	 method -> rcvr_class_obj -> __o_name);
      return ERROR;
    }
  }

  arg0_obj = __ctalk_receiver_pop ();
  __ctalk_receiver_push (arg0_obj);

  if (method -> n_args == 0)
    return ERROR;

  arg_frame = method->args[method->n_args-1]->call_stack_frame;
  for (stdarg_call_info.fmt_arg_n = method->n_args - 1;
       stdarg_call_info.fmt_arg_n >= 0;) {
    if ((stdarg_call_info.fmt_arg_n > 0) &&
	(method->args[stdarg_call_info.fmt_arg_n-1]->call_stack_frame
	 != arg_frame))
      break;
    if (stdarg_call_info.fmt_arg_n == 0)
      break;
    stdarg_call_info.fmt_arg_n--;
  }

  if ((fmt_value_obj = 
       __ctalkGetInstanceVariable 
       (method->args[stdarg_call_info.fmt_arg_n]->obj, 
	"value", FALSE))
      == NULL)
    fmt_value_obj = method -> args[stdarg_call_info.fmt_arg_n]->obj;

  if (fmt_value_obj -> __o_class != rt_defclasses -> p_string_class) {
    _warning ("__ctalkLibcFnWithMethodArgs: Argument type mismatch.\n");
    return ERROR;
  }

  if (IS_OBJECT(fmt_value_obj)) {
    if (*fmt_value_obj -> __o_value == '"') {
      strcpy (fmtbuf, TRIM_LITERAL(fmt_value_obj -> __o_value));
    } else {
      strcpy (fmtbuf, fmt_value_obj -> __o_value);
    }
  } else {
    return ERROR;
  }

  tokenize_fmt (fmtbuf);

  for (k = stdarg_call_info.fmt_arg_n + 1, ptr_arg_idx = 0; 
       method->args[k]; k++) {
    read_value_obj = method ->  args[k] -> obj -> instancevars ?
      method -> args[k] -> obj -> instancevars :
      method -> args[k] -> obj;
    if (IS_OBJECT (read_value_obj)) {
      if (read_value_obj -> attrs & OBJECT_VALUE_IS_BIN_INT) {
	/* TO DO - This could be a lot faster if ptr_args can just be a
	   pointer */
	ctitoa (*(int *)read_value_obj -> __o_value, ptr_args[ptr_arg_idx++]);
      } else if (read_value_obj -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG) {
	ctitoa (LLVAL(read_value_obj -> __o_value), ptr_args[ptr_arg_idx++]);
      } else if (read_value_obj -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
	htoa (ptr_args[ptr_arg_idx++], SYMVAL(read_value_obj -> __o_value));
      } else {
	strcpy (ptr_args[ptr_arg_idx++], read_value_obj -> __o_value);
      }
    } else {
      return ERROR;
    }
  }

  if (__check_printon_args (method)) {
    _warning ("__ctalkSelfPrintOn: Wrong number of arguments.\n");
    return ERROR;
  }

  retval = __call_printon_fn_w_args (arg0_obj, fmtbuf, method, &stdarg_call_info);

  /*
   *  All functions in the C library that use stdargs return return.
   */

  return retval;
}

int __ctalkObjectPrintOn (OBJECT *prt_obj) {

  char fmtbuf[MAXMSG];
  OBJECT *fmt_value_obj;
  int retval = 0; /* Avoid a warning. */
  STDARG_CALL_INFO stdarg_call_info;
  OBJECT *prt_value_obj;
  int k;
  OBJECT *read_value_obj;
  METHOD *method;
  
  method = __ctalkRtGetMethod ();

  if ((prt_value_obj = __ctalkGetInstanceVariable (prt_obj, "value", FALSE))
      == NULL) {
    prt_value_obj = prt_obj;
  }

  if (is_class_or_subclass (prt_value_obj, rt_defclasses -> p_string_class)) {
    stdarg_call_info.libcfn = (int (*)())sprintf;
  } else {
    if (!strcmp (prt_value_obj -> CLASSNAME, "WriteFileStream") ||
	__ctalkIsSubClassOf (prt_value_obj -> CLASSNAME, 
			     "WriteFileStream")) {
      stdarg_call_info.libcfn = (int (*)())fprintf;
    } else {
      _warning 
	("__ctalkObjectPrintOn: Unimplemented stream class, \"%s.\"\n",
	 prt_value_obj -> CLASSNAME);
      return ERROR;
    }
  }

  stdarg_call_info.fmt_arg_n = 0;

  if ((fmt_value_obj = 
       __ctalkGetInstanceVariable 
       (method->args[stdarg_call_info.fmt_arg_n]->obj, 
				   "value", FALSE))
      == NULL)
    fmt_value_obj = method -> args[stdarg_call_info.fmt_arg_n] -> obj;

  if (fmt_value_obj -> __o_class != rt_defclasses -> p_string_class) {
    _warning ("__ctalkLibcFnWithMethodArgs: Argument type mismatch.\n");
    return ERROR;
  }

  if (*fmt_value_obj -> __o_value == '"') {
    strcpy (fmtbuf, TRIM_LITERAL(fmt_value_obj -> __o_value));
  } else {
    strcpy (fmtbuf, fmt_value_obj -> __o_value);
  }

  tokenize_fmt (fmtbuf);

  for (k = stdarg_call_info.fmt_arg_n + 1, ptr_arg_idx = 0; 
       method->args[k]; k++) {
    if ((read_value_obj = 
	 __ctalkGetInstanceVariable (method->args[k] -> obj, 
				     "value", FALSE)) == NULL)
      read_value_obj = method -> args[k] -> obj;
    if (!IS_OBJECT(read_value_obj)) {
      _warning ("__ctalkObjectPrintOn: Invalid argument object.\n");
      strcpy (ptr_args[ptr_arg_idx++], NULLSTR);
    } else {
      strcpy (ptr_args[ptr_arg_idx++], read_value_obj -> __o_value);
    }
  }

  if (__check_printon_args (method)) {
    _warning ("__ctalkObjectPrintOn: Wrong number of arguments.\n");
    return ERROR;
  }
  retval = __call_printon_fn_w_args (prt_obj, fmtbuf, method, &stdarg_call_info);

  /*
   *  All functions in the C library that use stdargs return int.
   */

  return retval;
}

/*
 *  This function is intended to be used in a method that is
 *  within a non-string method to format the calling method's
 *  variable arguments.  Refer to GLXCanvasPane : 
 *
 *    GLXCanvasPane instanceMethod drawFmtTextFT (Float xOrg,
 *					    Float yOrg,
 *					    String fmt, ...) {
 *        String new s;
 *        // this formats the caller's variable arglist to s
 *        s vPrintOn;
 *        ...
 *    }
 *
 *  where String : printOn would look like this.
 *
 *    String instanceMethod vPrintOn (void) {
 *      __ctalkCallerPrintOnSelf ();
 *    }
 *
 *  So it's analogous to vprintf and related functions.
 *
 */

int __ctalkCallerPrintOnSelf (OBJECT *caller_fmt_arg) {

  char fmtbuf[MAXMSG];
  OBJECT *fmt_value_obj;
  int retval = 0; /* Avoid a warning. */
  STDARG_CALL_INFO stdarg_call_info;
  OBJECT *arg0_obj = NULL;        /* Avoid a warning. */
  int k, n_th_arg;
  OBJECT *read_value_obj;
  METHOD *caller_method, *this_method;
  int arg_frame;
  
  caller_method = get_calling_method ();
  this_method = __ctalkRtGetMethod ();

  if (is_class_or_subclass (this_method -> rcvr_class_obj,
			    rt_defclasses -> p_string_class)) {
    stdarg_call_info.libcfn = (int (*)())sprintf;
  } else {
    if (!strcmp (this_method -> rcvr_class_obj -> __o_name, 
		 "WriteFileStream") ||
	__ctalkIsSubClassOf (caller_method -> rcvr_class_obj -> __o_name,
			     "WriteFileStream")) {
      stdarg_call_info.libcfn = (int (*)())fprintf;
    } else {
      _warning 
	("__ctalkSelfPrintOn: Unimplemented receiver class, \"%s.\"\n",
	 caller_method -> rcvr_class_obj -> __o_name);
      return ERROR;
    }
  }

  arg0_obj = __ctalk_receiver_pop ();
  __ctalk_receiver_push (arg0_obj);

  if (caller_method -> n_args == 0)
    return ERROR;

  for (k = __ctalk_arg_ptr + 2, n_th_arg = caller_method -> n_args - 1; ;
       --n_th_arg, ++k) {
    if (__ctalk_argstack[k] == caller_fmt_arg) {
      stdarg_call_info.fmt_arg_n = n_th_arg;
      break;
    }
  }

  if (IS_OBJECT(caller_method -> args[stdarg_call_info.fmt_arg_n] -> obj
		-> instancevars)) {
    fmt_value_obj = caller_method -> args[stdarg_call_info.fmt_arg_n]
      ->obj -> instancevars;
  } else {
    fmt_value_obj = caller_method -> args[stdarg_call_info.fmt_arg_n]
      ->obj;
  }

  if (fmt_value_obj -> __o_class != rt_defclasses -> p_string_class) {
    _warning ("__ctalkCallerPrintOnSelf: Argument type mismatch.\n");
    return ERROR;
  }

  if (IS_OBJECT(fmt_value_obj)) {
    if (*fmt_value_obj -> __o_value == '"') {
      strcpy (fmtbuf, TRIM_LITERAL(fmt_value_obj -> __o_value));
    } else {
      strcpy (fmtbuf, fmt_value_obj -> __o_value);
    }
  } else {
    return ERROR;
  }

  tokenize_fmt (fmtbuf);

  for (k = stdarg_call_info.fmt_arg_n + 1, ptr_arg_idx = 0; 
       caller_method->args[k]; k++) {
    read_value_obj = caller_method ->  args[k] -> obj -> instancevars ?
      caller_method -> args[k] -> obj -> instancevars :
      caller_method -> args[k] -> obj;
    if (IS_OBJECT (read_value_obj)) {
      if (read_value_obj -> attrs & OBJECT_VALUE_IS_BIN_INT) {
	/* TO DO - This could be a lot faster if ptr_args can just be a
	   pointer */
	ctitoa (*(int *)read_value_obj -> __o_value, ptr_args[ptr_arg_idx++]);
      } else if (read_value_obj -> attrs & OBJECT_VALUE_IS_BIN_LONGLONG) {
	ctitoa (LLVAL(read_value_obj -> __o_value), ptr_args[ptr_arg_idx++]);
      } else if (read_value_obj -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
	htoa (ptr_args[ptr_arg_idx++], SYMVAL(read_value_obj -> __o_value));
      } else {
	strcpy (ptr_args[ptr_arg_idx++], read_value_obj -> __o_value);
      }
    } else {
      return ERROR;
    }
  }

  if (__check_printon_args (caller_method)) {
    _warning ("__ctalkSelfPrintOn: Wrong number of arguments.\n");
    return ERROR;
  }

  retval = __call_printon_fn_w_args (arg0_obj, fmtbuf, caller_method, &stdarg_call_info);

  return retval;
}

