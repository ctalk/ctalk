/* $Id: except.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

#ifndef _EXCEPT_H
#define _EXCEPT_H

#ifndef _SETJMP_H
#include <setjmp.h>
#endif

#ifndef _OBJECT_H
#include "object.h"
#endif

#ifndef _SYMBOL_H
#include "symbol.h"
#endif

#ifndef _RTINFO_H
#include "rtinfo.h"
#endif

#ifndef MAXMSG
#define MAXMSG 8192
#endif

typedef enum {
  no_x = 0,
  cplusplus_header_x = 1,     /* Preprocessor exceptions. */
  mismatched_paren_x = 2,
  false_assertion_x = 3,
  file_is_directory_x = 4,
  file_already_open_x = 5,
  undefined_param_class_x = 6,
  parse_error_x = 7,
  invalid_operand_x = 8,
  ambiguous_operand_x = 9,
  ptr_conversion_x = 10,
  undefined_class_x = 11,
  undefined_class_or_receiver_x = 12,
  undefined_method_x = 13,
  method_used_before_define_x = 14,
  self_without_receiver_x = 15,
  undefined_label_x = 16,
  undefined_type_x = 17,
  undefined_receiver_x = 18,
  unknown_file_mode_x = 19,
  invalid_variable_declaration_x = 20,
  wrong_number_of_arguments_x = 21,
  signal_event_x = 22,
  invalid_receiver_x = 23,
  not_a_tty_x = 24,
  user_exception_x = 25,
  eperm_x,                /* These are looked up in errno_exceptions[]   */
  enoent_x,               /* in except.c.                                */
  esrch_x,
  eintr_x,
  eio_x,
  enxio_x,
  e2big_x,
  enoexec_x,
  ebadf_x,
  echild_x,
  eagain_x,
  enomem_x,
  eaccess_x,
  efault_x,
  enotblk_x,
  ebusy_x,
  eexist_x,
  exdev_x,
  enodev_x,
  enotdir_x,
  eisdir_x,
  einval_x,
  enfile_x,
  emfile_x,
  enotty_x,
  etxtbsy_x,
  efbig_x,
  enospc_x,
  espipe_x,
  erofs_x,
  emlink_x,
  epipe_x,
  edom_x,
  erange_x,
  edeadlk_x,
  enametoolong_x,
  enolck_x,
  enosys_x,
  enotempty_x,
  eloop_x,
  ewouldblock_x,
  enomsg_x,
  eidrm_x,
  echrng_x,
  el2nsync_x,
  el3hlt_x,
  el3rst_x,
  elnrng_x,
  eunatch_x,
  enocsi_x,
  el2hlt_x,
  ebade_x,
  ebadr_x,
  exfull_x,
  enoano_x,
  ebadrqc_x,
  ebadslt_x,
  edeadlock_x,
  ebfont_x,
  enostr_x,
  enodata_x,
  etime_x,
  enosr_x,
  enonet_x,
  enopkg_x,
  eremote_x,
  enolink_x,
  eadv_x,
  esrmnt_x,
  ecomm_x,
  eproto_x,
  emultihop_x,
  edotdot_x,
  ebadmsg_x,
  eoverflow_x,
  enotuniq_x,
  ebadfd_x,
  eremchg_x,
  elibacc_x,
  elibbad_x,
  elibscn_x,
  elibmax_x,
  elibexex_x,
  eilseq_x,
  erestart_x,
  estrpipe_x,
  eusers_x,
  enotsock_x,
  edestaddrreq_x,
  emsgsize_x,
  eprototype_x,
  enoprotopt_x,
  eprotonotsuppport_x,
  esocktnosupport_x,
  eopnotsupp_x,
  epfnosupport_x,
  eafnosupport_x,
  eaddrinuse_x,
  eaddrnotavail_x,
  enetdown_x,
  enetunreach_x,
  enetreset_x,
  econnaborted_x,
  econnreset_x,
  enobufs_x,
  eisconn_x,
  enotconn_x,
  eshutdown_x,
  etoomanyrefs_x,
  etimeout_x,
  econnrefused_x,
  ehostdown_x,
  ehostunreach_x,
  ealready_x,
  einprogress_x,
  estale_x,
  euclean_x,
  enotnam_x,
  enavail_x,
  eisnam_x,
  eremoteio_x,
  edquot_x,
  enomedium_x,
  emediumtype_x
} EXCEPTION;

#define SUCCESS_X                        (EXCEPTION)0
#define CPLUSPLUS_HEADER_X               (EXCEPTION)1
#define MISMATCHED_PAREN_X               (EXCEPTION)2
#define FALSE_ASSERTION_X                (EXCEPTION)3
#define FILE_IS_DIRECTORY_X              (EXCEPTION)4
#define FILE_ALREADY_OPEN_X              (EXCEPTION)5
#define UNDEFINED_PARAM_CLASS_X          (EXCEPTION)6
#define PARSE_ERROR_X                    (EXCEPTION)7
#define INVALID_OPERAND_X                (EXCEPTION)8
#define AMBIGUOUS_OPERAND_X              (EXCEPTION)9
#define PTR_CONVERSION_X                 (EXCEPTION)10
#define UNDEFINED_CLASS_X                (EXCEPTION)11
#define UNDEFINED_CLASS_OR_RECEIVER_X    (EXCEPTION)12
#define UNDEFINED_METHOD_X               (EXCEPTION)13
#define METHOD_USED_BEFORE_DEFINE_X      (EXCEPTION)14
#define SELF_WITHOUT_RECEIVER_X          (EXCEPTION)15
#define UNDEFINED_LABEL_X                (EXCEPTION)16
#define UNDEFINED_TYPE_X                 (EXCEPTION)17
#define UNDEFINED_RECEIVER_X             (EXCEPTION)18
#define UNKNOWN_FILE_MODE_X              (EXCEPTION)19
#define INVALID_VARIABLE_DECLARATION_X   (EXCEPTION)20
#define WRONG_NUMBER_OF_ARGUMENTS_X      (EXCEPTION)21
#define SIGNAL_EVENT_X                   (EXCEPTION)22
#define INVALID_RECEIVER_X               (EXCEPTION)23
#define NOT_A_TTY_X                      (EXCEPTION)24
#define USER_EXCEPTION_X                 (EXCEPTION)25

/*
 *  Masks for deferred returns.  Mostly for internal use.
 */
#define M_SUCCESS_X                      (0)
#define M_CPLUSPLUS_HEADER_X             (1 << 0)
#define M_MISMATCHED_PAREN_X             (1 << 1)
#define M_FALSE_ASSERTION_X              (1 << 2)
#define M_FILE_IS_DIRECTORY_X            (1 << 3)
#define M_FILE_ALREADY_OPEN_X            (1 << 4)
#define M_UNDEFINED_PARAM_CLASS_X        (1 << 5)
#define M_PARSE_ERROR_X                  (1 << 6)
#define M_INVALID_OPERAND_X              (1 << 7)
#define M_AMBIGUOUS_OPERAND_X            (1 << 8)
#define M_PTR_CONVERSION_X               (1 << 9)
#define M_UNDEFINED_CLASS_X              (1 << 10)
#define M_UNDEFINED_CLASS_OR_RECEIVER_X  (1 << 11)
#define M_UNDEFINED_METHOD_X             (1 << 12)
#define M_METHOD_USED_BEFORE_DEFINE_X    (1 << 13)
#define M_SELF_WITHOUT_RECEIVER_X        (1 << 14)
#define M_UNDEFINED_LABEL_X              (1 << 15)
#define M_UNDEFINED_TYPE_X               (1 << 16)
#define M_UNDEFINED_RECEIVER_X           (1 << 17)
#define M_UNKNOWN_FILE_MODE_X            (1 << 18)
#define M_INVALID_VARIABLE_DECLARATION_X (1 << 19)
#define M_WRONG_NUMBER_OF_ARGUMENTS_X    (1 << 20)
#define M_SIGNAL_EVENT_X                 (1 << 21)
#define M_INVALID_RECEIVER_X             (1 << 22)
#define M_NOT_A_TTY_X                    (1 << 23)
#define M_USER_EXCEPTION_X               (1 << 24)

typedef struct _i_exception {
  int parser_lvl,                  /* Location of interpreter exception. */
    error_line,
    error_col;
  int visible_line;
  RT_INFO rtinfo;                  /* Location of run-time exception. */
  int is_critical,
    is_internal,
    call_stack_idx;
  EXCEPTION _exception;
  SYMBOL *unresolved_classes,
    *unresolved_symbols;
  char text[MAXMSG];
  RT_INFO *x_call_stack[MAXARGS+1];
  struct _i_exception *next,
    *prev;
} I_EXCEPTION;

#define X_HANDLE_IMMED (1 << 0)    /* Handle exception as soon as its trapped. */
#define X_HANDLE_DEFER (1 << 1)    /* Defer exception until handler clears it. */

typedef struct _x_handler {
  EXCEPTION _exception;
  OBJECT *(*handler)(struct _i_exception *);
  char msgfmt[MAXMSG];
  int _handle_mode;
} X_HANDLER;

typedef struct _jmp_env {
  int _e_mask;
  jmp_buf _env;
} JMP_ENV;

#define EXCEPTION_MASK(__mask) (jmp_env._e_mask=(__mask))
#define CATCH_EXCEPTION (setjmp(jmp_env._env))

#endif /* #ifndef _EXCEPT_H */
