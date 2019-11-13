/* $Id: pexcept.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _PEXCEPT_H
#define _PEXCEPT_H

#ifndef _PRTINFO_H
#include "prtinfo.h"
#endif

#ifndef MAXMSG
#define MAXMSG 8192
#endif

typedef enum {
  success_x,
  cplusplus_header_x,     /* Preprocessor exceptions. */
  mismatched_paren_x,
  false_assertion_x,
  undefined_symbol_x,
  undefined_keyword_x,
  deprecated_keyword_x,
  redefined_symbol_x,
  parse_error_x,
  invalid_operand_x,
  elif_w_o_if_x,
  else_w_o_if_x,
  endif_w_o_if_x,
  elif_after_ifdef_x,
  undefined_label_x,
  assignment_in_constant_expr_x,
  missing_arg_list_x,
  line_range_x,
  argument_parse_error_x
} EXCEPTION;

typedef struct _i_exception {
  int parser_lvl,                  /* Location of interpreter exception. */
    error_line,
    error_col;
  RT_INFO rtinfo;                  /* Location of run-time exception. */
  EXCEPTION exception;
  char text[MAXMSG];
  struct _i_exception *next,
    *prev;
} I_EXCEPTION;

typedef struct _x_handler {
  EXCEPTION exception;
  char *(*handler)(struct _i_exception *);
  char msgfmt[MAXMSG];
} X_HANDLER;

#endif /* #ifndef _PEXCEPT_H */
