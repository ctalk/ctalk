/* $Id: rt_expr.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015, 2017-2018
    Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _RT_EXPR_H
#define _RT_EXPR_H

typedef struct {
  int lvl;
  int rcvr_frame_top;
  int msg_frame_start,
    msg_frame_top;
  int call_stack_level;
  METHOD *e_methods[MAXARGS];
  int e_method_ptr;
  MESSAGE_STACK m_s;
  OBJECT *e_result;
  char *expr_str;
  bool is_arg_expr;
  int entry_eval_status;
} EXPR_PARSER;

typedef enum {
  fmt_arg_null = 0,
  fmt_arg_char_ptr,
  fmt_arg_ptr,
  fmt_arg_char,
  fmt_arg_double,
  fmt_arg_int,
  fmt_arg_long_int,
  fmt_arg_long_long_int,
} FMT_ARG_CTYPE;

#define C_EXPR_PARSER expr_parsers[expr_parser_ptr]

#endif /* #ifndef _RT_EXPR_H */
