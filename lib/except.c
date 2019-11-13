/* $Id: except.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2017  Robert Kiesling, rk3314042@gmail.com.
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
 *  Exception handling layer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "except.h"
/* #include "../classes/ctalklib" */

extern I_PASS interpreter_pass;           /* Declared in rtinfo.c. */
extern RT_INFO *__call_stack[MAXARGS+1]; 
extern int __call_stack_ptr;
extern PARSER *parsers[MAXARGS+1];
extern int current_parser_ptr;

extern OBJECT *__ctalk_classes;
extern OBJECT *__ctalk_last_class;

extern VARENTRY *__ctalk_dictionary;
extern VARENTRY *__ctalk_last_object;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

static I_EXCEPTION *x_list = NULL;
static I_EXCEPTION *x_list_ptr = NULL;

JMP_ENV jmp_env;
int exception_return;

#define ERROR_LOC_FMT "%s:%d: "

/*
 *  Ctalk-specific exceptions.  Note that searches of the array 
 *  stop at no_x, so it needs to be at the end.
 */

#define X_HANDLER_FOR(__x) (&x_handlers[(int)(__x)])

X_HANDLER x_handlers[] = {
  {no_x, __ctalkExceptionNotifyInternal, "Success", X_HANDLE_IMMED},
  {cplusplus_header_x, __ctalkExceptionNotifyInternal, "C++ header file.", X_HANDLE_IMMED},
  {mismatched_paren_x, __ctalkExceptionNotifyInternal, "Mismatched parentheses.", X_HANDLE_IMMED},
  {false_assertion_x, __ctalkExceptionNotifyInternal, "Assertion %s failed.", X_HANDLE_IMMED},
  {file_is_directory_x, __ctalkExceptionNotifyInternal, "%s is not a file.", X_HANDLE_IMMED},
  {file_already_open_x, __ctalkExceptionNotifyInternal, "File is already open.", X_HANDLE_IMMED},
  {undefined_param_class_x, __ctalkExceptionNotifyInternal, "Undefined parameter class %s.", X_HANDLE_IMMED},
  {parse_error_x, __ctalkExceptionNotifyInternal, "Parser error.", X_HANDLE_IMMED},
  {invalid_operand_x, __ctalkExceptionNotifyInternal, "Invalid operand: %s.", X_HANDLE_IMMED},
  {ambiguous_operand_x, __ctalkExceptionNotifyInternal, "Ambiguous operand: %s.", X_HANDLE_IMMED},
  {ptr_conversion_x, __ctalkExceptionNotifyInternal, "Not a pointer value.", X_HANDLE_IMMED},
  {undefined_class_x, __ctalkExceptionNotifyInternal, "Undefined class, %s.", X_HANDLE_IMMED},
  {undefined_class_or_receiver_x, __ctalkExceptionNotifyInternal, "Undefined class or receiver, %s.", X_HANDLE_IMMED},
  {undefined_method_x, __ctalkUndefinedMethodReferenceException, "Undefined method, \"%s\".", X_HANDLE_DEFER},
  {method_used_before_define_x, __ctalkUndefinedMethodReferenceException, "Method %s used before it is defined.", X_HANDLE_DEFER},
  {self_without_receiver_x, __ctalkExceptionNotifyInternal, "Keyword, \"self,\" used without receiver. %s", X_HANDLE_IMMED},
  {undefined_label_x, __ctalkExceptionNotifyInternal, "Undefined label, \"%s.\"", X_HANDLE_IMMED},
  {undefined_type_x, __ctalkExceptionNotifyInternal, "Undefined type, \"%s.\"", X_HANDLE_IMMED},
  {undefined_receiver_x, __ctalkExceptionNotifyInternal, "Undefined receiver, \"%s.\"", X_HANDLE_IMMED},
  {unknown_file_mode_x, __ctalkExceptionNotifyInternal, "Unknown file mode, \"%s\"", X_HANDLE_IMMED},
  {invalid_variable_declaration_x, __ctalkExceptionNotifyInternal, "Invalid variable declaration.", X_HANDLE_IMMED},
  {wrong_number_of_arguments_x, __ctalkExceptionNotifyInternal, "Method %s: wrong number of arguments.", X_HANDLE_IMMED},
  {signal_event_x, __ctalkExceptionNotifyInternal, "Signal: %s.", X_HANDLE_IMMED},
  {invalid_receiver_x, __ctalkExceptionNotifyInternal, "Invalid receiver: %s.", X_HANDLE_IMMED},
  {not_a_tty_x, __ctalkExceptionNotifyInternal, "Not a tty: %s.", X_HANDLE_IMMED},
  {user_exception_x, __ctalkExceptionNotifyInternal, "%s", X_HANDLE_IMMED},
  /*
   *  System error exception text is filled in in 
   *  __ctalkSystemErrExceptionInternal, etc. below.
   */
  {eperm_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enoent_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {esrch_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {eintr_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {eio_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enxio_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {e2big_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enoexec_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {ebadf_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {echild_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {echild_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {eagain_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enomem_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {eaccess_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {efault_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enotblk_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {ebusy_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {eexist_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {exdev_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enodev_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enotdir_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {eisdir_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {einval_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enfile_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {emfile_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enotty_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {etxtbsy_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {efbig_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {enospc_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {espipe_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {erofs_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {emlink_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {epipe_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {edom_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {erange_x, __ctalkExceptionNotifyInternal, "", X_HANDLE_IMMED},
  {no_x, __ctalkExceptionNotifyInternal, "Success", X_HANDLE_IMMED}
};

/* 
 *  Exceptions from errno events generally use the string provided by
 *  strerror () when the notification is issued, unless the exception
 *  uses another handler, specified below.  Used by 
 *  __ctalkSysErrExceptionInternal ().
 */
static EXCEPTION errno_exceptions[] = {
  /*
   *  Not used so we can index all the exceptions sequentially.
   *  __ctalkSysErrExceptionInternal and 
   *  __ctalkCriticalSysErrExceptionInternal adjust for this 
   *  also, below.
   */ 
/*   success_x, */       /* 0. */
  eperm_x,         /* 1. EPERM */
  enoent_x,        /* 2. ENOENT */
  esrch_x,         /* 3. ESRCH */
  eintr_x,         /* 4. EINTR */
  eio_x,           /* 5. EIO */
  enxio_x,         /* 6. ENXIO */
  e2big_x,         /* 7. E2BIG */
  enoexec_x,       /* 8. ENOEXEC */
  ebadf_x,         /* 9. EBADF */
  echild_x,        /* 10. ECHILD */
  eagain_x,        /* 11. EAGAIN */
  enomem_x,        /* 12. ENOMEM */
  eaccess_x,       /* 13. EACCESS */
  efault_x,        /* 14. EFAULT */
  enotblk_x,       /* 15. ENOTBLK */
  ebusy_x,         /* 16. EBUSY */
  eexist_x,        /* 17. EEXIST */
  exdev_x,         /* 18. EXDEV */
  enodev_x,        /* 19. ENODEV */
  enotdir_x,       /* 20. ENOTDIR */
  eisdir_x,        /* 21. EISDIR */
  einval_x,        /* 22. EINVAL */
  enfile_x,        /* 23. ENFILE */
  emfile_x,        /* 24. EMFILE */
  enotty_x,        /* 25. ENOTTY */
  etxtbsy_x,       /* 26. ETXTBSY */
  efbig_x,         /* 27. EFBIG */
  enospc_x,        /* 28. ENOSPC */
  espipe_x,        /* 29. ESPIPE */
  erofs_x,         /* 30. EROFS */
  emlink_x,        /* 31. EMLINK */
  epipe_x,         /* 32. EPIPE */
  edom_x,          /* 33. EDOM */
  erange_x,        /* 34. ERANGE */
  edeadlk_x,       /* 35. EDEADLCK */
  enametoolong_x,  /* 36. ENAMETOOLONG */
  enolck_x,        /* 37. ENOLCK */
  enosys_x,        /* 38. ENOSYS */
  enotempty_x,     /* 39. ENOTEMPY */
  eloop_x,         /* 40. ELOOP */
  ewouldblock_x,   /* 41. EWOULDBLOCK = EAGAIN */
  enomsg_x,        /* 42. ENOMSG */
  eidrm_x,         /* 43. EIDRM */
  echrng_x,        /* 44. ECHRNG */
  el2nsync_x,      /* 45. EL2SYNC */
  el3hlt_x,        /* 46. EL3HLT */
  el3rst_x,        /* 47. EL3RST */
  elnrng_x,        /* 48. ELNRNG */
  eunatch_x,       /* 49. EUNATCH */
  enocsi_x,        /* 50. ENOCSI */
  el2hlt_x,        /* 51. EL2HLT */
  ebade_x,         /* 52. EBADE */
  ebadr_x,         /* 53. EBADR */
  exfull_x,        /* 54. EXFULL */
  enoano_x,        /* 55. ENOANO */
  ebadrqc_x,       /* 56. EBADRQC */
  ebadslt_x,       /* 57. EBADSLT */
  edeadlock_x,     /* 58. EDADLOCK = EDEADLK */
  ebfont_x,        /* 59. EBFONT */
  enostr_x,        /* 60. ENOSTR */
  enodata_x,       /* 61. ENODATA */
  etime_x,         /* 62. ETIME */
  enosr_x,         /* 63. ENOSR */
  enonet_x,        /* 64. ENONET */
  enopkg_x,        /* 65. ENOPKG */
  eremote_x,       /* 66. EREMOTE */
  enolink_x,       /* 67. ENOLINK */
  eadv_x,          /* 68. EADV */
  esrmnt_x,        /* 69. ESRMNT */
  ecomm_x,         /* 70. ECOMM */
  eproto_x,        /* 71. EPROTO */
  emultihop_x,     /* 72. EMULTIHOP */
  edotdot_x,       /* 73. EDOTDOT */
  ebadmsg_x,       /* 74. EBADMSG */
  eoverflow_x,     /* 75. EOVERFLOW */
  enotuniq_x,      /* 76. ENOTUNIQ */
  ebadfd_x,        /* 77. EBADFD */
  eremchg_x,       /* 78. EREMCHG */
  elibacc_x,       /* 89. ELIBCHG */
  elibbad_x,       /* 80. ELIBBAD */
  elibscn_x,       /* 81. ELIBSCN */
  elibmax_x,       /* 82. ELIBMAX */
  elibexex_x,      /* 83. ELIBEXEC */
  eilseq_x,        /* 84. EILSEQ */
  erestart_x,      /* 85. ERESTART */
  estrpipe_x,      /* 86. ESTRPIPE */
  eusers_x,        /* 87. EUSERS */
  enotsock_x,      /* 88. ENOTSOCK */
  edestaddrreq_x,  /* 89. EDESTADDRREQ */
  emsgsize_x,      /* 90. EMSGSIZE */
  eprototype_x,    /* 91. EPROTOTYPE */
  enoprotopt_x,    /* 92. ENOPROTOOPT */
  eprotonotsuppport_x,  /* 93. EPROTONOTSUPPORT */
  esocktnosupport_x,    /* 94. ESOCKNOTSUPPORT */
  eopnotsupp_x,         /* 95. EOPNOTSUPP */
  epfnosupport_x,       /* 96. EPFNOSUPPORT */
  eafnosupport_x,       /* 97. EAFNOSUPPORT */
  eaddrinuse_x,         /* 98. EADDRINUSE */
  eaddrnotavail_x,      /* 99. EADDRNOTAVAIL */
  enetdown_x,           /* 100. ENETDOWN */
  enetunreach_x,        /* 101. ENETUNREACH */
  enetreset_x,          /* 102. ENETRESET */
  econnaborted_x,       /* 103. ECONNABORTED */
  econnreset_x,         /* 104. ECONNRESET */
  enobufs_x,            /* 105. ENOBUFS */
  eisconn_x,            /* 106. EISCONN */
  enotconn_x,           /* 107. ENOTCONN */
  eshutdown_x,          /* 108. ESHUTDOWN */
  etoomanyrefs_x,       /* 109. ETOOMANYREFS */
  etimeout_x,           /* 110. ETIMEOUT */
  econnrefused_x,       /* 111. ECONNREFUSED */
  ehostdown_x,          /* 112. EHOSTDOWN */
  ehostunreach_x,       /* 113. EHOSTUNREACH */
  ealready_x,           /* 114. EALREADY */
  einprogress_x,        /* 115. EINPROGRESS */
  estale_x,             /* 116. ESTALE */
  euclean_x,            /* 117. EUCLEAN */
  enotnam_x,            /* 118. ENOTNAM */
  enavail_x,            /* 119. EAVAIL */
  eisnam_x,             /* 120. EISNAM */
  eremoteio_x,          /* 121. EREMOTEIO */
  edquot_x,             /* 122. EDQUOT */
  enomedium_x,          /* 123. ENOMEDIUM */
  emediumtype_x         /* 124. EMEDIUMTYPE */
};

I_EXCEPTION *__get_x_list (void) {
  return x_list;
}

int __ctalkExceptionInternal (MESSAGE *m_orig, EXCEPTION ex, 
				    char *text, int visible_line) {
  I_EXCEPTION *i;
  i = __new_exception ();

  switch (interpreter_pass)
    {
    case run_time_pass:
      i -> parser_lvl = __get_expr_parser_level ();
      __save_call_stack_to_exception (i);
      break;
    case preprocessing_pass:
    case var_pass:
      break;
    case parsing_pass:
    case library_pass:
    case method_pass:
    case c_fn_pass:
      i -> parser_lvl = parsers[current_parser_ptr] -> level;
      break;
    default:
      break;
    }
  if (m_orig) {
    i -> error_line = m_orig -> error_line;
    i -> visible_line = visible_line;
    i -> error_col = m_orig -> error_column;
    if (m_orig -> receiver_obj) i -> rtinfo.rcvr_obj = m_orig -> receiver_obj;
  }
  i -> _exception = ex;
  if (text) {
    strcpy (i -> text, text);
  } else {
    *i -> text = 0;
  }

  if (!x_list) {
    x_list = x_list_ptr = i;
  } else {
    x_list_ptr -> next = i;
    i -> prev = x_list_ptr;
    x_list_ptr = i;
  }

  return TRUE;
}

/*
 *  This is the run-time portion of the above, and also sets the
 *  is_critical flag and saves a snapshot of the caller's RTINFO.
 */
int __ctalkCriticalExceptionInternal (MESSAGE *m_orig, EXCEPTION ex, 
				    char *text) {
  I_EXCEPTION *i;
  i = __new_exception ();

  switch (interpreter_pass)
    {
    case run_time_pass:
      i -> parser_lvl = __get_expr_parser_level ();
      __save_call_stack_to_exception (i);
      break;
    default:
      _warning ("__ctalkCriticalExceptionInternal should only be called by run-time code.\n");
      __xfree (MEMADDR(i));
      return ERROR;
      break;
    }
  i -> _exception = ex;
  if (text) {
    strcpy (i -> text, text);
  } else {
    *i -> text = 0;
  }

  i -> is_critical = TRUE;
  /*
   *  __call_stack_ptr + 1 is the context of the method that called
   *  this function.  __call_stack_ptr + 2 is the method where the
   *  exception occurred, so save that context.  
   *  
   *  NOTE: The _source file_ is at __call_stack_ptr + 1.
   */
  if (__call_stack_ptr + 2 <= MAXARGS) {
    i -> rtinfo.rcvr_obj = __call_stack[__call_stack_ptr + 2]->rcvr_obj;
    i -> rtinfo.rcvr_class_obj = 
      __call_stack[__call_stack_ptr + 2]->rcvr_class_obj;
    i -> rtinfo.method_fn = __call_stack[__call_stack_ptr + 2]->method_fn;
    if (text)
      strcpy (i -> text, text);
    strcpy (i -> rtinfo.source_file, 
	    __call_stack[__call_stack_ptr + 1]->source_file);
  } else {
    _warning ("__ctalkCriticalExceptionInternal: Unable to find a method calling context.  Did you call this function directly?\n");
  }

  if (!x_list) {
    x_list = x_list_ptr = i;
  } else {
    x_list_ptr -> next = i;
    i -> prev = x_list_ptr;
    x_list_ptr = i;
  }

  return TRUE;
}

int __ctalkHandleRunTimeExceptionInternal (void) {
  I_EXCEPTION *i;
  X_HANDLER *x_h;

  for (i = x_list; i; i = i -> next) {
    if (i -> parser_lvl >= __get_expr_parser_level ()) {
      if ((x_h = X_HANDLER_FOR (i -> _exception)) != NULL)
	(x_h -> handler) (i);

      if (i == x_list) {
	if (!i -> next) 
	  x_list = x_list_ptr = NULL;
	else
	  x_list = i -> next;
	_delete_exception (i);
	if (!x_list) return SUCCESS;
      } else {
	if (i -> prev) i -> prev -> next = i -> next;
	if (i -> next) i -> next -> prev = i -> prev;
	if (x_list_ptr == i) x_list_ptr = i -> prev;
	_delete_exception (i);
      }
    }
  }
  return SUCCESS;
}

int __ctalkHandleRunTimeException (void) {
  I_EXCEPTION *i;
  X_HANDLER *x_h;

  if (__ctalkHaveCallerException ())
    return SUCCESS;

  for (i = x_list; i; i = i -> next) {
    if (i) {
      if ((x_h = X_HANDLER_FOR (i -> _exception)) != NULL) {
	/*
	 *  Don't print "Success."
	 */
	if (x_h->handler) {
	  restore_stdin_stdout_termios ();
	  (x_h -> handler) (i);
	}
      }
      
      if (i == x_list) {
	if (!i -> next) 
	  x_list = x_list_ptr = NULL;
	else
	  x_list = i -> next;
	_delete_exception (i);
	return SUCCESS;
      } else {
	if (i -> prev) i -> prev -> next = i -> next;
	if (i -> next) i -> next -> prev = i -> prev;
	if (x_list_ptr == i) x_list_ptr = i -> prev;
	_delete_exception (i);
      }
    }
  }
  return SUCCESS;
}

char *__ctalkGetRunTimeException (void) {
  I_EXCEPTION *i;
  X_HANDLER *x_h;
  static char msgbuf[MAXMSG];

  strcpy (msgbuf, "Success.");

  for (i = x_list; i; i = i -> next) {
    if (i) {
      if ((x_h = X_HANDLER_FOR (i -> _exception)) != NULL) {
 	sprintf (msgbuf, x_h -> msgfmt, i -> text);
      } else {
	strcpy (msgbuf, i -> text);
      }
      if (i == x_list) {
	if (!i -> next) 
	  x_list = x_list_ptr = NULL;
	else
	  x_list = i -> next;
	_delete_exception (i);
      } else {
	if (i -> prev) i -> prev -> next = i -> next;
	if (i -> next) i -> next -> prev = i -> prev;
	if (x_list_ptr == i) x_list_ptr = i -> prev;
	_delete_exception (i);
      }
    }
  }
  return msgbuf;
}

char *__ctalkPeekRunTimeException (void) {
  I_EXCEPTION *i;
  X_HANDLER *x_h;
  static char msgbuf[MAXMSG];

  strcpy (msgbuf, "Success.");

  for (i = x_list; i; i = i -> next) {
    if (i)
      if ((x_h = X_HANDLER_FOR (i -> _exception)) != NULL)
	strcpy (msgbuf, i -> text);
  }
  return msgbuf;
}

int __ctalkHandleInterpreterExceptionInternal (MESSAGE *m) {

  I_EXCEPTION *i;
  X_HANDLER *x_h;
  
  for (i = x_list; i; i = i -> next) {
    if ((i -> error_line == m -> error_line) &&
	(i -> error_col == m -> error_column) &&
	(parsers[current_parser_ptr] -> level == i -> parser_lvl)) {

      if ((x_h = X_HANDLER_FOR (i -> _exception)) != NULL) {
	switch (x_h -> _handle_mode)
	  {
	  case X_HANDLE_DEFER:
	  case X_HANDLE_IMMED:
	    (x_h -> handler) (i);
	    if (i == x_list) {
	      if (!i -> next) {
		x_list = x_list_ptr = NULL;
		_delete_exception (i);
		return SUCCESS;
	      } else {
		x_list = i -> next;
		_delete_exception (i);
	      }
	    } else {
	      if (i -> prev) i -> prev -> next = i -> next;
	      if (i -> next) i -> next -> prev = i -> prev;
	      if (x_list_ptr == i) { 
		x_list_ptr = i -> prev;
		x_list_ptr -> next = NULL;
		_delete_exception (i);
		return SUCCESS;
	      } else {
		_delete_exception (i);
	      }
	    }
	    break;
	  }
      }
    }
  }
  return SUCCESS;
}

I_EXCEPTION *__ctalkTrapExceptionInternal (MESSAGE *m) {

  I_EXCEPTION *i;

  switch (interpreter_pass)
    {
    case run_time_pass:
      for (i = x_list; i; i = i -> next) {
	if (i -> parser_lvl >= __get_expr_parser_level ())
	  return i;
      }
      return NULL;
      break;
    case var_pass:
      for (i = x_list; i; i = i -> next) {
	if ((i -> error_line == m -> error_line) &&
	    (i -> error_col == m -> error_column))
	  return i;
      }
      break;
    case parsing_pass:
    case library_pass:
    case method_pass:
    case c_fn_pass:
      for (i = x_list; i; i = i -> next) {
	if ((i -> error_line == m -> error_line) &&
	    (parsers[current_parser_ptr] -> level == i -> parser_lvl))
	  return i;
      }
      break;
    default:
      break;
    }
  return NULL;
}

int __ctalkDeleteExceptionInternal (I_EXCEPTION *x) {

  I_EXCEPTION *i;

  for (i = x_list; i; i = i -> next) {
    if (x == i) {
      if (x == x_list) {
	x_list = x -> next;
	if (x -> next) x -> next -> prev = NULL;
      } else {
	if (x -> next) x -> next -> prev = x -> prev;
	if (x -> prev) x -> prev -> next = x -> next;
      }
      _delete_exception (x);
    }
  }

  return SUCCESS;
}

int __ctalkDeleteLastExceptionInternal (void) {

  I_EXCEPTION *i;

  if ((i = x_list_ptr) != NULL) {

    if (x_list == x_list_ptr) {
      x_list = x_list_ptr = NULL;
    } else {
      x_list_ptr = x_list_ptr -> prev;
      x_list_ptr -> next = NULL;
    }
    _delete_exception (i);
  }

  return SUCCESS;
}

OBJECT *__ctalkExceptionNotifyInternal (I_EXCEPTION *i) {

  X_HANDLER *x_h;
  char msgfmtbuf[MAXMSG];

  /*
   *  Ctalk exceptions.
   */
  if ((x_h = X_HANDLER_FOR (i -> _exception)) != NULL) {
    if (*(x_h -> msgfmt)) {
      if (strstr (x_h -> msgfmt, "%s")) {
	sprintf (msgfmtbuf, x_h -> msgfmt, i -> text);
      } else {
	sprintf (msgfmtbuf, x_h -> msgfmt);
      }
    } else {
      strcpy (msgfmtbuf, i -> text);
    }
    _warning ("%s:%d: %s\n", __source_filename (), 
	      ((i->visible_line) ? i->visible_line : i -> error_line), 
	      msgfmtbuf);
    if (__ctalkGetExceptionTrace ())
      __ctalkPrintExceptionTrace ();
  } else {
    /*
     *  Errno exceptions.
     */
  }
  return NULL;
}

I_EXCEPTION *__new_exception (void) {
  I_EXCEPTION *i;

  if ((i = (I_EXCEPTION *)__xalloc (sizeof (I_EXCEPTION))) == NULL)
    _error ("__new_exception: %s.\n", strerror (errno));
  return i;
}

/*
 *  errnum - 1 because esuccess isn't used in the exception table.
 */
int __ctalkSysErrExceptionInternal (MESSAGE *m_orig, int errnum, char *text) {
  char s[MAXMSG];
  strcatx (s, strerror (errnum), ": ", text, NULL);
  return __ctalkCriticalExceptionInternal (m_orig, errno_exceptions[errnum-1], s);
  return SUCCESS;
}

/*
 *  Like __ctalkTrapExceptionInternal, except that it is called
 *  from methods and ignores the expression parser level.
 */
I_EXCEPTION *__ctalkTrapException (void) {

  I_EXCEPTION *i;

  switch (interpreter_pass)
    {
    case run_time_pass:
      if ((i = x_list) == NULL)
	  return NULL;
      break;
    default:
      return NULL;
      break;
    }
  return i;
}

/*
 *  Convenience function for __ctalkTrapException ().
 */
int __ctalkPendingException (void) {
  I_EXCEPTION *i;
  return (((i = __ctalkTrapException ()) != NULL) ? TRUE : FALSE);
}

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;

OBJECT *__ctalkUndefinedMethodReferenceException (I_EXCEPTION *i) {

  char msgfmtbuf[0xffff];

  switch (i -> _exception) 
    {
    case method_used_before_define_x:
      switch (interpreter_pass)
	{
	case method_pass:
	  sprintf (msgfmtbuf, "In method %s (Class %s):",
		   new_methods[new_method_ptr+1]->method->name, 
		   rcvr_class_obj -> __o_name);
	       
	  _warning ("%s:%d: %s\n", __source_filename (), i -> error_line, msgfmtbuf);
	  sprintf (msgfmtbuf, "%s used before it is defined.", i -> text);
	  _warning ("%s:%d: %s\n", __source_filename (), i -> error_line, msgfmtbuf);
	  break;
	default:
	  sprintf (msgfmtbuf, "Method %s used before it is defined.",
		   i -> text);
	  _warning ("%s:%d: %s\n", __source_filename (), i -> error_line, msgfmtbuf);
	  break;
	}  /* switch (interpreter_pass) */
      break;
    case undefined_method_x:
      switch (interpreter_pass)
	{
	case method_pass:
	  sprintf (msgfmtbuf, "In method %s (Class %s):",
		   new_methods[new_method_ptr+1]->method->name, 
		   rcvr_class_obj -> __o_name);
	       
	  _warning ("%s:%d: %s\n", __source_filename (), i -> error_line, msgfmtbuf);
	  SNPRINTF (msgfmtbuf, MAXMSG, "%s%s\n", 
		   ERROR_LOC_FMT,
		   X_HANDLER_FOR(i -> _exception)->msgfmt);
	  _warning (msgfmtbuf, 
		    __source_filename (),
		    i -> error_line,
		    i -> text);
  	  break;
	default:
	  SNPRINTF (msgfmtbuf, MAXMSG, "%s%s\n", 
		   ERROR_LOC_FMT,
		   X_HANDLER_FOR(i -> _exception)->msgfmt);
	  _warning (msgfmtbuf, 
		    __source_filename (),
		    i -> error_line,
		    i -> text);
	  break;
	}  /* switch (interpreter_pass) */
      break;
    default:  /* Avoid warnings. */
      break;
    }

  if (__ctalkGetExceptionTrace ())
    __ctalkPrintExceptionTrace ();

  /*
   *  See the comment in lib/rtnwmthd.c.
   */
#ifdef __APPLE__
extern int dummy (void);
  dummy ();
#endif

  return SUCCESS;
}

static int print_var_level;

static char *__pp_space (void) {
  static char buf[MAXMSG];
  int i;
  
  if (print_var_level == 1)
    return "";
  
  *buf = 0;
  for (i = 2; i <= print_var_level; i++)
    strcatx2 (buf, "            ", NULL);

  return buf;
}

static void __dump_instance_variable (OBJECT *o) {
  OBJECT *v, *v1;
  char sp_buf[MAXMSG];

  ++print_var_level; 

  strcpy (sp_buf, __pp_space ());

  if (o -> instancevars) {
    for (v = o -> instancevars; v; v = v -> next) {
      fprintf (stderr, "%sINSTANCE VARIABLE: %s\n", sp_buf, v -> __o_name);
      fprintf (stderr, "%s            Class: %s\n", sp_buf, v -> CLASSNAME);
      fprintf (stderr, "%s            Value: %s\n", sp_buf, v -> __o_value);

      for (v1 = v -> instancevars; v1; v1 = v1 -> next)
	__dump_instance_variable (v1);
    }
  } else {
    fprintf (stderr, "%sINSTANCE VARIABLE: %s\n", sp_buf, o -> __o_name);
    fprintf (stderr, "%s            Class: %s\n", sp_buf, o -> CLASSNAME);
    fprintf (stderr, "%s            Value: %s\n", sp_buf, o -> __o_value);
  }
  --print_var_level;
}

void __dump_object (OBJECT *o) {

  fprintf (stderr, "OBJECT: %s\n", o -> __o_name);
  fprintf (stderr, "Class: %s\n", o -> CLASSNAME);
  fprintf (stderr, "Value: %s\n", o -> __o_value);

  __dump_instance_variable (o);
}

int __ctalkPrintObject (OBJECT *o) {
  __dump_object (o);
  return SUCCESS;
}

int __ctalkPrintObjectByName (char *name) {

  OBJECT *o, *p;
  METHOD *m;

  print_var_level = 0;

  if (strcmp (name, "self")) {

    for (p = __ctalk_classes; p; p = p -> next) {
      
      for (m = p -> class_methods; m; m = m -> next) {
	/* 
	 *  TO DO!  Check this.
	 */
	for (o = m -> local_objects[m -> nth_local_ptr].objs; 
	     o; o = o -> next) {
	  if (!strcmp (name, o -> __o_name)) 
	    __dump_object (o);
	}
      }

      /*
       *  HERE TOO!
       */
      for (m = p -> instance_methods; m; m = m -> next) {
	for (o = m -> local_objects[m -> nth_local_ptr].objs; 
	     o; o = o -> next) {
	  if (!strcmp (name, o -> __o_name)) 
	    __dump_object (o);
	}
      }

      for (o = p -> classvars; o; o = o -> next) {
	if (!strcmp (name, o -> __o_name)) 
	  __dump_object (o);
      }

      for (o = p -> instancevars; o; o = o -> next) {
	if (!strcmp (name, o -> __o_name)) 
	  __dump_object (o);
      }

    }
  } else {
    __dump_object (__ctalk_receivers[__ctalk_receiver_ptr + 1]);
  }
  return SUCCESS;
}

int _delete_exception (I_EXCEPTION *x) {
  int j;
  for (j = x -> call_stack_idx + 1; j <= MAXARGS; j++) {
    if (x -> x_call_stack[j]) _delete_rtinfo (x -> x_call_stack[j]);
  }
  __xfree (MEMADDR(x));
  return SUCCESS;
}

void __ctalkPrintExceptionTrace (void) {
  I_EXCEPTION *i;
  int idx;
  if (__ctalkHaveCallerException ())
    return;
  for (i = x_list; i; i = i -> next) {
    for (idx = i -> call_stack_idx + 1; idx <= MAXARGS; idx++) {
      if (i -> x_call_stack[idx] -> method) {
	if (!strstr (i->x_call_stack[idx]->method->name, ARGBLK_LABEL))
	  fprintf (stderr, "\tFrom %s : %s\n", 
		   i->x_call_stack[idx]->method->rcvr_class_obj->__o_name,
		   i->x_call_stack[idx]->method->name);
      } else {
	if (i -> x_call_stack[idx] -> _rt_fn) {
	  if (*i -> x_call_stack[idx] -> _rt_fn -> name)
	    fprintf (stderr, "\tFrom %s ()\n", 
		     i -> x_call_stack[idx] -> _rt_fn -> name);
	  else
	    fprintf (stderr, "\tFrom %s\n", "<cfunc>");
	}
      }
    }
  }
}

char * __ctalkPeekExceptionTrace (void) {
  I_EXCEPTION *i;
  static char s[0xFFFF], line[MAXMSG];
  int idx;
  *s = '\0';
  for (i = x_list; i; i = i -> next) {
    for (idx = i -> call_stack_idx + 1; idx <= MAXARGS; idx++) {
      if (i -> x_call_stack[idx] -> method) {
	sprintf (line, "\tFrom %s : %s\n", 
		 i->x_call_stack[idx]->method->rcvr_class_obj->__o_name,
		 i->x_call_stack[idx]->method->name);
	strcatx2 (s, line, NULL);
      } else {
	sprintf (line, "\tFrom %s\n", "<cfunc>");
	strcatx2 (s, line, NULL);
      }
    }
  }
  return s;
}

void *__ctalkExceptionInstalledHandler (OBJECT *o) {
  OBJECT *handler_instance_var;
  if ((handler_instance_var = 
       __ctalkGetInstanceVariable (o, "handlerMethod", TRUE))
      != NULL) {
    if (handler_instance_var->instancevars->__o_value[0] != '0') {
      return __ctalkGenericPtrFromStr 
	(handler_instance_var->instancevars->__o_value);
    }
  }
  return NULL;
}

int __ctalkHaveCallerException (void) {
  int call_stack_idx;
  RT_INFO *r;
  OBJECT *o;
  VARENTRY *v;
  for (call_stack_idx = __call_stack_ptr + 2; 
       call_stack_idx <= MAXARGS; 
       call_stack_idx++) {
    r = __call_stack[call_stack_idx];
    if (r -> method) {
      for (v = M_LOCAL_VAR_LIST(r -> method); 
	   v && IS_OBJECT(v->var_object); v = v -> next) {
	o = v -> var_object;
	if (!strcmp ( o -> CLASSNAME, "Exception") ||
	    __ctalkIsSubClassOf (o -> CLASSNAME, "Exception")) {
	  if (__ctalkExceptionInstalledHandler (o)) {
	    return TRUE;
	  }
	}
      }
    } else {
      for (v = r -> _rt_fn -> local_objects.vars; 
	   v && IS_OBJECT(v->var_object); v = v -> next) {
	o = v -> var_object;
	if (!strcmp ( o -> CLASSNAME, "Exception") ||
	    __ctalkIsSubClassOf (o -> CLASSNAME, "Exception")) {
	  if (__ctalkExceptionInstalledHandler (o)) {
	    return TRUE;
	  }
	}
      }
    }
  }
  if (__ctalk_dictionary) {
    for (v = __ctalk_dictionary ;
	 v && IS_OBJECT(v->var_object); v = v -> next) {
      o = v -> var_object;
      if (!strcmp ( o -> CLASSNAME, "Exception") ||
	  __ctalkIsSubClassOf (o -> CLASSNAME, "Exception")) {
	if (__ctalkExceptionInstalledHandler (o)) {
	  return TRUE;
	}
      }
    }
  }
  return FALSE;
}
