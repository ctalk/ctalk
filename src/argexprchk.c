/* $Id: argexprchk.c,v 1.2 2020/09/19 01:08:26 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015, 2020 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

OBJECT *arg_expr_object_2 (MESSAGE_STACK messages, int start_idx, 
			   int end_idx) {
  OBJECT *arg_object;
  ARGSTR tmp_argstr;
  int expr_end;
  MSINFO ms;

  ms.messages = messages;
  ms.tok = start_idx;
  ms.stack_start = stack_start (messages);
  ms.stack_ptr = get_stack_top (messages);
  expr_end = find_expression_limit (&ms);
  tmp_argstr.arg = collect_tokens (messages, start_idx, expr_end);
  tmp_argstr.start_idx = start_idx;
  tmp_argstr.end_idx = expr_end;
  tmp_argstr.m_s = messages;
  arg_object = create_arg_EXPR_object (&tmp_argstr);
  __xfree (MEMADDR(tmp_argstr.arg));
  return arg_object;
}

