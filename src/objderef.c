/* $Id: objderef.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2018 Robert Kiesling, rk3314042@gmail.com.
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

/*
 *  Handles the compiler stuff for evaluating object member
 *  expressions; e.g., myObject -> __o_name, myObject -> instancevars,
 *  etc.
 *
 *  So far we only put the expressions into __ctalkEvalExpr calls
 *  when they're printf arguments ... it saves some extra stuff in
 *  the run-time. 
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "classlib.h"
#include "symbol.h"

extern I_PASS interpreter_pass;         /* Declared in lib/rtinfo.c.  */

/* Should be called with first_tok_idx pointing to the first
   label of the expression. */
bool objderef_check_expr (MESSAGE_STACK messages, int first_tok_idx,
			   int *last_tok_idx, int stack_top_idx) {
  int i, prev_tok;
  MESSAGE *m;

  prev_tok = -1;

  for (i = first_tok_idx; i > stack_top_idx; --i) {
    if (M_ISSPACE(messages[i]))
      continue;

    m = messages[i];

    switch (M_TOK(m))
      {
      case LABEL:
	if (i < first_tok_idx) {
	  if (!is_OBJECT_member (M_NAME(m))) {
	    /* It's okay to have a different label if the
	       struct expression is already complete - we
	       just return the index of the previous label
	       (i.e., the last valid label of the OBJECT *struct). 
	    */
	    if (M_TOK(messages[prev_tok]) == LABEL) {
	      *last_tok_idx = prev_tok;
	      return true;
	    } else {
	      return false;
	    }
	  }
	}
	prev_tok = i;
	break;
      case DEREF:
      case PERIOD:
	prev_tok = i;
	break;
      default:
	if (i == first_tok_idx || prev_tok == first_tok_idx) {
	  *last_tok_idx = ERROR;
	  return false;
	}
	*last_tok_idx = prev_tok;
	return true;
	break;
      }
  }

  return false;
}

/* Declared in pattypes.c. */
extern bool ptr_fmt_is_alt_int_fmt;

bool objderef_handle_printf_arg (MSINFO *ms) {
  int i, expr_end_idx;
  char expr_buf[MAXMSG], expr_buf_out[MAXMSG], expr_buf_out_2[MAXMSG];
  

  if (interpreter_pass == expr_check)
    return false;

  if (!objderef_check_expr (ms -> messages, ms -> tok, &expr_end_idx, 
			    ms -> stack_ptr))
    return false;

  if (!is_fmt_arg_2 (ms))
    return false;
    
  toks2str (ms -> messages, ms -> tok, expr_end_idx, expr_buf);

  if (!str_eq (M_NAME(ms -> messages[expr_end_idx]), "__o_p_obj") &&
      !str_eq (M_NAME(ms -> messages[expr_end_idx]), "classvars") &&
      !str_eq (M_NAME(ms -> messages[expr_end_idx]), "instancevars")) {
    fmt_eval_expr_str (expr_buf, expr_buf_out);
    fmt_printf_fmt_arg_ms (ms, expr_buf_out, expr_buf_out_2);
    fileout (expr_buf_out_2, 0, ms -> tok);
  } else {

    /* 
       Handle __o_p_obj, instancevars, and classvars separately. 
    */
    fmt_eval_u_expr_str (expr_buf, expr_buf_out);

    if (fmt_arg_type (ms -> messages, ms -> tok, ms -> stack_start)
	== fmt_arg_ptr) {
      if (ptr_fmt_is_alt_int_fmt) {
	strcatx (expr_buf_out_2, ALT_PTR_FMT_CAST,
		 expr_buf_out, NULL);
	fileout (expr_buf_out_2, 0, ms -> tok);
      } else {
	fileout (expr_buf_out, 0, ms -> tok);
      }
    } else {
      fileout (expr_buf_out, 0, ms -> tok);
    }

  }
  for (i = ms -> tok; i >= expr_end_idx; --i) {
    ++(ms -> messages[i] -> evaled);
    ++(ms -> messages[i] -> output);
  }
  return true;

}
