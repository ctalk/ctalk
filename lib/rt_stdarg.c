/* $Id: rt_stdarg.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015 - 2017, 2019
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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "typeof.h"

extern DEFAULTCLASSCACHE *rt_defclasses;

extern RADIX radix_of (char *);

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

/*
 *  This uses the stdarg.h format, which normally requires that the
 *  first argument be a char *fmt.
 *
 *  TO DO - Perform scalar-symbol translations, and scalar and 
 *          pointer args in the same call.  
 *          __ctalkCPrintFmtToCtalkFmt performs this translation
 *          if called first.
 *  TO DO - Argument type mismatches should raise exceptions.
 *  TO DO - For functions with stream arguments (fscanf, sscanf, etc)
 *          allow catalog of stream types and number.
 */

char fmt_tokens[512][8192];
int fmt_tok_idx;

char ptr_args[512][8192];
int ptr_arg_idx;

OBJECT *__ctalkLibcFnWithMethodVarArgs (int (*fn)(), METHOD *method,
					char *return_class_name) {
  char fmtbuf[MAXMSG], retbuf[MAXMSG];
  OBJECT *fmt_value_obj, *return_obj;
  int retval = 0, lextype;  /* Avoid a warning. */
  RADIX r;
  STDARG_CALL_INFO stdarg_call_info;
  OBJECT *arg0_obj = NULL;        /* Avoid a warning. */
  int k;
  bool is_INTEGER_T;
  OBJECT *read_value_obj;
  
#if defined (__APPLE__) && defined (__ppc__)
  printf ("This version of Darwin uses version-specific stdargs functions.\n");
  printf ("(Try using the stdarg function (e.g., printf, scanf, fscanf, fprintf, sscanf,\nsprintf) in a C expression.)\n");
  exit (1);
#endif

  stdarg_call_info.libcfn = fn;

  if ((stdarg_call_info.libcfn == (int (*)())fscanf) || 
      (stdarg_call_info.libcfn == (int (*)())sscanf) || 
      (stdarg_call_info.libcfn == (int (*)())fprintf) || 
      (stdarg_call_info.libcfn == (int (*)())sprintf) ||
      (stdarg_call_info.libcfn == (int (*)())xsprintf)) {
    stdarg_call_info.fmt_arg_n = 1;
    if ((arg0_obj = method -> args[0] -> obj -> instancevars) == NULL)
      arg0_obj = method -> args[0] -> obj;
    if ((stdarg_call_info.libcfn == (int (*)())fscanf) ||
	(stdarg_call_info.libcfn == (int (*)())sscanf)) {
      is_INTEGER_T = lextype_is_INTEGER_T (arg0_obj -> __o_value);
      r = radix_of (arg0_obj -> __o_value);
      if (is_INTEGER_T &&
	  ((r == hexadecimal) || (r == decimal))) {
	errno = 0;
	stdarg_call_info.arg0 = (void *)strtoul 
	  (arg0_obj -> __o_value, NULL, 16);
	if (errno != 0)
	  strtol_error (errno, "__ctalkLibcFnWithMethodArgs ()",
			arg0_obj -> __o_value);
      } else {
	stdarg_call_info.arg0 = arg0_obj -> __o_value;
      }
    } else {
      stdarg_call_info.arg0 = arg0_obj -> __o_value;
    }
  } else {
    stdarg_call_info.fmt_arg_n = 0;
  }

  if ((stdarg_call_info.libcfn == (int (*)())fprintf) || 
      (stdarg_call_info.libcfn == (int (*)())sprintf) ||
      (stdarg_call_info.libcfn == (int (*)())xsprintf)) {
    stdarg_call_info.stream_write_fn = TRUE;
  } else {
    stdarg_call_info.stream_write_fn = FALSE;
  }

  if ((fmt_value_obj = 
       __ctalkGetInstanceVariable 
       (method->args[stdarg_call_info.fmt_arg_n] -> obj, 
	"value", FALSE))
      == NULL)
    fmt_value_obj = method -> args[stdarg_call_info.fmt_arg_n] -> obj;

  if (fmt_value_obj -> __o_class != rt_defclasses -> p_string_class) {
    _warning ("__ctalkLibcFnWithMethodArgs: Argument type mismatch.\n");
    return NULL;
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
	 __ctalkGetInstanceVariable (method->args[k]->obj, 
				     "value", FALSE)) == NULL)
      read_value_obj = method -> args[k] -> obj;
    strcpy (ptr_args[ptr_arg_idx++], read_value_obj -> __o_value);
  }

  switch (stdarg_call_info.fmt_arg_n)
    {
    case 0:
      retval = __call_fn_w_args_fmtarg0 (fmtbuf, method, &stdarg_call_info);
      break;
    case 1:
      retval = __call_fn_w_args_fmtarg1 (fmtbuf, method, &stdarg_call_info);
      break;
    }

  /*
   *  All functions in the C library that use stdargs return int.
   */

  if (!strcmp (return_class_name, INTEGER_CLASSNAME)) {
    __ctalkDecimalIntegerToASCII (retval, retbuf);
    if (retval > 0)
      args_to_method_args (method, &stdarg_call_info);
    return_obj =
      create_object_init_internal
      ("result", rt_defclasses -> p_integer_class, LOCAL_VAR, retbuf);
  } else {
 _warning ("__ctalkLibCFnWithMethodVarArgs: Unimplemented return class %s.\n",
	   return_class_name);
 return_obj = NULL;
  }

  return return_obj;
}

int __call_fn_w_args_fmtarg0 (char *fmt, METHOD *method, 
			      STDARG_CALL_INFO *stdarg_call_info) {

  int i, j, retval = 0;   /* Avoid a warning. */

  if (stdarg_call_info -> libcfn == (int (*)())printf) {
    for (i = 0, j = 0; (i <= fmt_tok_idx) && fmt_tokens[i]; i++) {
      if (strchr (fmt_tokens[i], '%')) {
	retval += printf (fmt_tokens[i], ptr_args[j++]);
      } else {
	retval += printf (fmt_tokens[i]);
      }
    }
    return retval;
  }

  if (stdarg_call_info -> libcfn == (int (*)())scanf) {
    for (i = 0, j = 0; (i <= fmt_tok_idx) && fmt_tokens[i]; i++) {
      if (strchr (fmt_tokens[i], '%')) {
	retval += scanf (stdarg_call_info->arg0, fmt_tokens[i], ptr_args[j++]);
      } else {
	retval += scanf (stdarg_call_info->arg0, fmt_tokens[i]);
      }
    }
    return retval;
  }
  return ERROR;
}

int __call_fn_w_args_fmtarg1 (char *fmt, METHOD *method, 
			      STDARG_CALL_INFO *stdarg_call_info) {

  int i, j, retval;
  char tmparg0buf[MAXMSG];
  char *arg0_buf;

  retval = 0;

  if (stdarg_call_info -> libcfn == (int (*)())sprintf ||
      stdarg_call_info -> libcfn == (int (*)())xsprintf) {
    arg0_buf = stdarg_call_info -> arg0;
    *arg0_buf = '\0';
    for (i = 0, j = 0; (i <= fmt_tok_idx) && fmt_tokens[i]; i++) {
      if (strchr (fmt_tokens[i], '%')) {
	retval += sprintf (tmparg0buf, fmt_tokens[i], ptr_args[j++]);
      } else {
	retval += sprintf (tmparg0buf, fmt_tokens[i]);
      }
      strcatx2 (arg0_buf, tmparg0buf, NULL);
    }
    return retval;
  }

  if (stdarg_call_info -> libcfn == (int (*)())fprintf) {
    FILE *arg0_file;
    errno = 0;
    arg0_file = (FILE *)strtoul (stdarg_call_info -> arg0, NULL, 16);
    if (errno != 0)
      strtol_error (errno, "__call_fn_w_args_fmtarg1 ()", 
		    stdarg_call_info -> arg0);
    for (i = 0, j = 0; (i <= fmt_tok_idx) && fmt_tokens[i]; i++) {
      if (strchr (fmt_tokens[i], '%')) {
	retval += fprintf (arg0_file, fmt_tokens[i], ptr_args[j++]);
      } else {
	retval += fprintf (arg0_file, fmt_tokens[i]);
      }
    }
    return retval;
  }


  if (stdarg_call_info -> libcfn == (int (*)())fscanf) {
    arg0_buf = stdarg_call_info -> arg0;
    *arg0_buf = '\0';
    for (i = 0, j = 0; (i <= fmt_tok_idx) && fmt_tokens[i]; i++) {
      if (strchr (fmt_tokens[i], '%')) {
	retval += fscanf (stdarg_call_info->arg0, fmt_tokens[i], ptr_args[j++]);
      } else {
	retval += fscanf (stdarg_call_info->arg0, fmt_tokens[i]);
      }
    }
    return retval;
  }

  if (stdarg_call_info -> libcfn == (int (*)())sscanf) {
    int idx = 0, arg0_idx = 0;
    arg0_buf = stdarg_call_info -> arg0;
    for (i = 0, j = 0; (i <= fmt_tok_idx) && fmt_tokens[i]; i++) {
      if (strchr (fmt_tokens[i], '%')) {
	retval += sscanf (&arg0_buf[arg0_idx], fmt_tokens[i], ptr_args[j]);
	arg0_idx += strlen (ptr_args[j]);
	++j;
      } else {
	retval += sscanf (&arg0_buf[arg0_idx], fmt_tokens[i]);
	arg0_idx += strlen (fmt_tokens[i]);
      }
      /*
       *  Skip any whitespace after the input tokens. 
       */
      if (isspace((int)arg0_buf[idx])) {
	while (isspace((int)arg0_buf[idx++]))
	  ;
	--idx;  /* Back off for the next iteration. */
      }
    }
    return retval;
  }

  return ERROR;

}

void args_to_method_args (METHOD *method, STDARG_CALL_INFO *stdarg_call_info) {

  int i, n_th_arg;
  OBJECT *arg_val_obj;

  if (!stdarg_call_info -> stream_write_fn) {
    for (i = fmt_tok_idx; i > 0; i--) {
      n_th_arg = i + stdarg_call_info -> fmt_arg_n;
      /*
       *  Skip trailing tokens.
       */
      if (n_th_arg > (method->n_args-1))
	continue;
      /*
       *  fmt_arg_n should be 0 or 1, depending on whether there is a 
       *  stream argument preceding the format in the argument list.
       */
      if ((arg_val_obj = 
	   __ctalkGetInstanceVariable 
	   (method->args[n_th_arg]->obj, "value", FALSE)) != NULL) {
	__ctalkSetObjectValue (arg_val_obj, ptr_args[i - 1]);
      } else {
	_warning ("args_to_method_args: Argument mismatch.\n");
	return;
      }
    }
  }

}

int tokenize_fmt (char *fmt) {

  int src_idx, dst_idx, fmt_length;

  memset (fmt_tokens, '\0', 512 * 8192);

  for (src_idx = 0, dst_idx = 0, fmt_tok_idx = 0; fmt[src_idx];)
    {
      switch (fmt[src_idx])
	{
	case '%':
	  if ((src_idx > 0) && (fmt[src_idx - 1] == '%')) {
	    fmt_tokens[fmt_tok_idx][dst_idx++] = fmt[src_idx++]; 
	    fmt_tokens[fmt_tok_idx][dst_idx] = 0;
	  } else if ((fmt_length = is_printf_fmt (fmt, &fmt[src_idx])) != 0) {
	    substrcpy (fmt_tokens[fmt_tok_idx++], fmt, src_idx, fmt_length);
	    src_idx += fmt_length;
	  } else {
	    fmt_tokens[fmt_tok_idx][dst_idx++] = fmt[src_idx++]; 
	    fmt_tokens[fmt_tok_idx][dst_idx] = 0;
	  }
	  break;
	default:
	  fmt_tokens[fmt_tok_idx][dst_idx++] = fmt[src_idx++]; 
	  fmt_tokens[fmt_tok_idx][dst_idx] = 0;
	  if ((fmt[src_idx] == '%') &&
	      ((fmt[src_idx+1] != '%') && (fmt[src_idx-1] != '%'))) { 
	    /* lookahead */
	    ++fmt_tok_idx;
	    dst_idx = 0;
	  }
	  break;
	}
    }

  return SUCCESS;
}


