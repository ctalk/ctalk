/* $Id: rcvrexpr.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2019 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "ctrlblk.h"

/*
 *  Check for an expression like:
 *
 *     (<rcvr> <instvar|method>+) <method>
 *
 *  When called from resolve, msi -> tok should be pointing
 *  at <rcvr>.
 *
 *  This is still pretty basic for now - only a syntax check and
 *  a check for a method label, and only in receiver_context.
 */
bool rcvr_expr_in_parens (MSINFO *ms, int *expr_start_out,
			  int *expr_end_out) {
  int open_paren_idx, close_paren_idx, post_tok_idx;
  OBJECT_CONTEXT cx;
  if ((open_paren_idx = prevlangmsg (ms -> messages, ms -> tok))
      != ERROR) {
    if (M_TOK(ms -> messages[open_paren_idx]) != OPENPAREN) {
      return false;
    }
  } else {
    return false;
  }
  *expr_start_out = open_paren_idx;
  
  if ((cx = object_context (ms -> messages, ms -> tok)) != receiver_context)
    return false;

  if ((close_paren_idx = match_paren (ms -> messages, open_paren_idx,
				      ms -> stack_ptr))
      == ERROR) {
    return false;
  }

  if ((post_tok_idx = nextlangmsg (ms -> messages, close_paren_idx))
      == ERROR)
    return false;

  if (M_TOK(ms->messages[post_tok_idx]) != LABEL)
    return false;

  if (is_method_name (M_NAME(ms -> messages[post_tok_idx])) ||
      is_proto_selector (M_NAME(ms -> messages[post_tok_idx]))) {
    *expr_end_out = post_tok_idx;
    return true;
  }

  return false;
}

void output_rcvr_expr_in_parens (MSINFO *ms, int start_idx, int end_idx) {
  int i, end_idx_out;
  char *toks, expr_out[MAXMSG];
  
  toks = collect_tokens (ms -> messages, start_idx, end_idx);
  rt_expr (ms -> messages,start_idx, &end_idx_out, expr_out);
  __xfree (MEMADDR(toks));

  for (i = start_idx; i >= end_idx; i--) {
    ++(ms -> messages[i] -> evaled);
    ++(ms -> messages[i] -> output);
  }
}
