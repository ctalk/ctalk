/* $Id: rt_error.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2014-2015, 2019
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

/*
 *   Format and output error and warning messages.
 */

#ifndef MAXMSG             /* Defined in ctalk.h. */
#define MAXMSG 8192
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

extern void _error_out (char *);
extern void cleanup (int);
extern void cleanup_expr_parser (void);
extern void cleanup_temp_files (int);
extern void cleanup_rt_fns (void);
extern int __ctalk_exitFn (int);
extern void __argstack_error_cleanup (void);

extern I_PASS interpreter_pass; /* Declared in rtinfo.c. */

void _error (char *fmt, ...) {
  va_list ap;
  char buf[MAXMSG];
  va_start (ap, fmt);
  vsprintf (buf, fmt, ap);
  restore_stdin_stdout_termios ();
  _error_out (buf);
  if (interpreter_pass == run_time_pass) {
    cleanup_expr_parser ();
    cleanup_temp_files (1);
    cleanup_rt_fns ();
    __argstack_error_cleanup ();
    __ctalk_exitFn (1);
  }
  exit (EXIT_FAILURE);
}

void _warning (char *fmt, ...) {
  va_list ap;
  char buf[MAXMSG];
  va_start (ap, fmt);
  vsprintf (buf, fmt, ap);
  _error_out (buf);
}

void __ctalkWarning (char *fmt, ...) {
  va_list ap;
  char buf[MAXMSG];
  va_start (ap, fmt);
  vsprintf (buf, fmt, ap);
  _error_out (buf);
}

void system_dir_error (char *d) {

  _warning ("Warning: Could not find system include directory %s.\n", d);
  _warning ("         See the -s option.\n");
  exit (EXIT_FAILURE);
}

void method_wrong_number_of_arguments_warning (MESSAGE *method_msg, 
					       MESSAGE *prev_msg,
					       char *expr) {

  _warning ("Warning: Method, \"%s,\" receiver, \"%s:\"\n",
	    method_msg -> name, prev_msg -> name);
  if (M_ARGS_DECLARED(method_msg) != ANY_ARGS) {
    _warning ("Warning:\n");
    _warning ("\t%s\n", expr);
    _warning ("Warning:\tExpression provides %d argument(s).\n",
	      M_ARGS_DECLARED(method_msg));
    _warning ("Warning:\tMethod matching that prototype not found.\n");
    _warning ("Warning:\tTrying method without required parameters.\n");
  } else {
    _warning ("Warning:\tNot found.\n");
  }
  __warning_trace ();
}

void method_wrong_number_of_arguments_prefix_error (MESSAGE *method_msg,
						    MESSAGE *prev_msg,
						    char *expr) {
  /*
   * The expression parser is still a little lazy in some
   * cases, like when an expression has two of the same 
   * method message in succession.  So this might need
   * to be worked out further if there's more need
   * for other message.
   */
  if (M_ARGS_DECLARED(method_msg) != ANY_ARGS) {
    _warning ("Warning: Method, \"%s,\" receiver, \"%s:\"\n",
	      method_msg -> name, prev_msg -> name);
    _warning ("Warning:\n");
    _warning ("\t%s\n", expr);
    _warning ("Warning:\tExpression provides %d argument(s).\n",
	      M_ARGS_DECLARED(method_msg));
    _warning ("Warning:\tMethod matching that prototype not found.\n");
  }
}

char *math_op_does_not_understand_error (MESSAGE_STACK messages,
					 int op1_ptr,
					 int op_ptr,
					 int stack_start_idx,
					 int stack_end_idx) {
  static char errmsg[MAXMSG];
  OBJECT *op1_value_obj;
  MESSAGE *op1_message, *op_message;
  int op1_is_instance_var = FALSE;
  char *expr;

  op1_message = messages[op1_ptr];
  op_message = messages[op_ptr];

  op1_value_obj = (M_VALUE_OBJ(op1_message) -> instancevars) ?
    M_VALUE_OBJ(op1_message) -> instancevars :
    M_VALUE_OBJ(op1_message);

  if ((op1_message -> attrs & RT_OBJ_IS_INSTANCE_VAR) ||
      (op1_message -> attrs & RT_VALUE_OBJ_IS_INSTANCE_VAR))
    op1_is_instance_var = TRUE;

  expr = collect_tokens (messages, stack_start_idx, stack_end_idx);

  sprintf (errmsg, 
   "rt_math_op (): %s %s (%s) does not understand message %s.\n"
	   "In expression: \n\t%s\n%s",
	   ((op1_is_instance_var) ? "Instance variable" : "Object"),
	   M_VALUE_OBJ(op1_message) -> __o_name, 
	   op1_value_obj -> CLASSNAME, 
	   M_NAME(op_message), 
	   (M_VALUE_OBJ(op_message) ? 
	    M_VALUE_OBJ(op_message) -> __o_name :
	    op_message -> name),
	   expr);
  __xfree (MEMADDR(expr));
  return errmsg;
}

void strtol_error (int errno_arg, char *fn_name, char *data) {
  switch (errno_arg)
    {
    case EINVAL:
    case ERANGE:
      fprintf (stderr, "%s: %s: %s.\n", fn_name, strerror (errno_arg), data);
      break;
    default:
      break;
    }
}

void __ctalkLogMessage (char *fmt,...) {
#if defined (__linux__) || defined (__unix__)
  va_list ap;
  char fmtbuf[MAXMSG];
  va_start (ap, fmt);
  vsprintf (fmtbuf, fmt, ap);
  __log_message_internal (fmtbuf);
#else
  printf ("Syslog support is not (yet) implemented for this "
	  "architecture.\n");
#endif
}

/* Note that the system calls are the same also with MACH/APPLE,
   but the constants mean different things, and - aargh - my old
   OS X doesn't define LOG_CONSOLE :( So it's untested there... ymmv,
   etc. */
void _log_error (char *fmt, ...) {
  va_list ap;
  char buf[MAXMSG];
  va_start (ap, fmt);
  vsprintf (buf, fmt, ap);

  openlog ("ctalk", LOG_NDELAY|LOG_NOWAIT|LOG_PID, LOG_SYSLOG);
  syslog (LOG_SYSLOG|LOG_INFO, "%s", buf);
  closelog ();
}

/* for messages that are already formatted. */
void __log_message_internal (char *msg) {
  openlog ("ctalk", LOG_NDELAY|LOG_NOWAIT|LOG_PID, LOG_SYSLOG);
  syslog (LOG_SYSLOG|LOG_INFO, "%s", msg);
  closelog ();
}

static LIST *user_fns = NULL;

char *vartab_var_basename (char *name) {
  char *q, *q_1, *s_name_p, *e_name_p, *q_3, *q_4, pfx_1[MAXLABEL];
  char *args_part;
  char *s_fn_name, *e_fn_name;
  static char name_ret[MAXLABEL];
  OBJECT *class;
  METHOD *m;
  LIST *fn;
  if ((q = strchr (name, '_')) != NULL) {
    memset (pfx_1, 0, MAXLABEL);
    strncpy (pfx_1, name, q - name);
    if ((class = __ctalkGetClass (pfx_1)) != NULL) {
      /* the vartab is in a method. try to find the end of the selector. */
      q_1 = strchr (q, '_'); ++q_1; s_name_p = strchr (q_1, '_'); ++s_name_p;
      e_name_p = strchr (s_name_p, '_');
    longer_method_name:
      memset (name_ret, 0, MAXLABEL);
      strncpy (name_ret, s_name_p, e_name_p - s_name_p);
      for (m = class -> instance_methods; m; m = m -> next) {
	if (str_eq (name_ret, m -> name)) {
	  ++e_name_p;
	  args_part = strchr (e_name_p, '_');
	  ++args_part;
	  strcpy (name_ret, args_part);
	  return name_ret;
	}
      }
      for (m = class -> class_methods; m; m = m -> next) {
	if (str_eq (name_ret, m -> name)) {
	  ++e_name_p;
	  args_part = strchr (e_name_p, '_');
	  ++args_part;
	  strcpy (name_ret, args_part);
	  return name_ret;
	}
      }
      /* If we haven't found a method name yet, look for a longer
	 match (i.e., with embedded '_' characters). */
      ++e_name_p;
      if ((e_name_p = strchr (e_name_p, '_')) != NULL)
	goto longer_method_name;
    } else {
      s_fn_name = name;
      e_fn_name = strchr (s_fn_name, '_');
    match_longer_fn:
      memset (name_ret, 0, MAXLABEL);
      strncpy (name_ret, s_fn_name, e_fn_name - s_fn_name);

      for (fn = user_fns; fn; fn = fn -> next) {
	if (str_eq (name_ret, (char *)fn -> data)) {
	  return name_ret;
	}
      }
      ++e_fn_name;
      if ((e_fn_name = strchr (e_fn_name, '_')) != NULL) {
	goto match_longer_fn;
      }
    }
  }

  return name;
}

void __ctalkRegisterUserFunctionName (char *name) {
  LIST *l;
  l = new_list ();
  l -> data = strdup (name);
  if (user_fns == NULL) {
    user_fns = l;
  } else {
    list_add (user_fns, l);
  }
}
