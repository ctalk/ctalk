/* $Id: rt_argblk.c,v 1.6 2020/09/16 23:08:19 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012  Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"

/*
 *  Inline calls do not (yet) save and restore the rtinfo struct,
 *  so these functions look on the call stack directly.
 */

extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

extern DEFAULTCLASSCACHE *rt_defclasses;

/*
 *  C_RT_INFO is the block's call stack entry.  INLINE_CALLER_RT_INFO
 *  for the outermost block is two entries up. The map call is the 
 *  previous entry, and the entry before the map call is the method 
 *  where the block is defined.
 *
 *  The frame of the calling method or function is
 *    __call_stack_ptr + 1 + (2 * nested_argument_block_levels).
 */
#define C_RT_INFO __call_stack[__call_stack_ptr+1]  /* From lib/rtinfo.c. */
#define INLINE_CALLER_RT_INFO __call_stack[__call_stack[__call_stack_ptr+1]->_block_frame_top]

int __ctalkEnterArgBlockScope (void) {

  int blk_frame_offset;

  if (C_RT_INFO->inline_call) {
    blk_frame_offset = 1;
    blk_frame_offset += 2;
    C_RT_INFO->block_scope = True;
    C_RT_INFO->_block_frame_top = __call_stack_ptr + blk_frame_offset;

    while (__call_stack[__call_stack_ptr + blk_frame_offset] -> 
	   block_scope == True) {
      blk_frame_offset += 2;
      C_RT_INFO->block_scope = True;
      C_RT_INFO->_block_frame_top = __call_stack_ptr + blk_frame_offset;
    }
  } else {
    _warning ("__ctalkEnterArgBlockScope: Not an inline method call.\n");
    _warning ("__ctalkEnterArgBlockScope: See the documentation for inline methods\n");
    _warning ("__ctalkEnterArgBlockScope: and the __ctalkInlineMethod library\n");
    _warning ("__ctalkEnterArgBlockScope: function.\n");
    return ERROR;
  }
  return SUCCESS;
}
/*
 *  This can be a no-op for now.
 */
int __ctalkExitArgBlockScope (void) {
  return SUCCESS;
}

int __ctalkIsArgBlockScope (void) {
  if (__call_stack_ptr >= MAXARGS)
    return FALSE;
  return (int)C_RT_INFO->block_scope;
}

int __ctalkBlockCallerFrame (void) {
  if (__call_stack_ptr >= MAXARGS)
    return FALSE;
  return C_RT_INFO->_block_frame_top;
}

METHOD *__ctalkBlockCallerMethod (void) {
  if (__call_stack_ptr >= MAXARGS)
    return NULL;
  return ((INLINE_CALLER_RT_INFO->method) ? 
	  INLINE_CALLER_RT_INFO -> method : NULL);
}

RT_FN *__ctalkBlockCallerFn (void) {
  if (__call_stack_ptr >= MAXARGS)
    return NULL;
  return ((INLINE_CALLER_RT_INFO->_rt_fn) ? 
	  INLINE_CALLER_RT_INFO -> _rt_fn : NULL);
}

static void set_conditional_expr_value (EXPR_PARSER *p,
					int start,
					int end, OBJECT *result) {
  int i, i_2;
  OBJECT *del_obj;
  for (i = start; i >= end; i--) {
      if (!IS_OBJECT(p -> m_s[i] -> obj)) {
	p -> m_s[i] -> obj = result;
      }
      if (IS_OBJECT(p -> m_s[i] -> value_obj)) {
	/* We might need to take these case-by-case. */
	if (p -> m_s[i] -> value_obj -> attrs & OBJECT_IS_I_RESULT) {
	  del_obj = p -> m_s[i] -> value_obj;
	  for (i_2 = start; i_2 >= end; i_2--) {
	    p -> m_s[i_2] -> value_obj = NULL;
	  }
	  __ctalkDeleteObject (del_obj);
	}
      }
      p -> m_s[i] -> value_obj = result;
      ++(p -> m_s[i] -> evaled);
    }
}

void subexpr_conditional (EXPR_PARSER *p, int subexpr_start,
			  int subexpr_end, int question_tok_ptr) {
  int colon_tok_ptr, i;
  int true_result_start, true_result_end;
  int false_result_start, false_result_end;
  char *s_true, *s_false;
  OBJECT *true_result, *false_result, *result;

  for (i = question_tok_ptr - 1; i >  p -> msg_frame_top; --i) {
    if (M_TOK(p -> m_s[i]) == COLON) {
      colon_tok_ptr = i;
      break;
    }
  }
  true_result_start = __ctalkNextLangMsg (p -> m_s, question_tok_ptr,
					  p -> msg_frame_top);
  true_result_end = __ctalkPrevLangMsg (p -> m_s, colon_tok_ptr,
					p -> msg_frame_start);
  false_result_start = __ctalkNextLangMsg (p -> m_s, colon_tok_ptr,
					   p -> msg_frame_top);
  if (M_TOK(p -> m_s[false_result_start]) == OPENPAREN) {
    false_result_end = __ctalkMatchParen (p -> m_s,
					  false_result_start,
					  p -> msg_frame_top);
  } else {
    /***/
    /* TODO - work on rules if there's an unparenthesized expression
       with tokens following it. */
    false_result_end = p -> msg_frame_top;
  }
  
  s_true = collect_tokens (p -> m_s, true_result_start, true_result_end);

  s_false = collect_tokens (p -> m_s, false_result_start, false_result_end);

  if ((p -> m_s[subexpr_start] -> value_obj -> attrs & OBJECT_VALUE_IS_BIN_BOOL) ||
      (p -> m_s[subexpr_start] -> value_obj -> attrs & OBJECT_VALUE_IS_BIN_INT)) {
    if (INTVAL(p -> m_s[subexpr_start] -> value_obj
	       -> instancevars -> __o_value)) {
      result = __ctalkEvalExpr (s_true);
    } else {
      result = __ctalkEvalExpr (s_false);
    }
    set_conditional_expr_value (p, subexpr_start, false_result_end, result);
  } else if (p -> m_s[subexpr_start] -> value_obj -> __o_class ==
	     rt_defclasses -> p_string_class) {
    if (p -> m_s[subexpr_start] -> value_obj -> instancevars -> __o_value[0]) {
      result = __ctalkEvalExpr (s_true);
    } else {
      result = __ctalkEvalExpr (s_false);
    }
    set_conditional_expr_value (p, subexpr_start, false_result_end, result);
  } else if (p -> m_s[subexpr_start] -> value_obj -> __o_class ==
	     rt_defclasses -> p_character_class) {
    if (p -> m_s[subexpr_start] -> value_obj -> instancevars -> __o_value[0]) {
      result = __ctalkEvalExpr (s_true);
    } else {
      result = __ctalkEvalExpr (s_false);
    }
    set_conditional_expr_value (p, subexpr_start, false_result_end, result);
  } else {
    _warning ("Unhandled predicate class, \"%s,\" in expression:\n\n\t%s\n\n"
	      "Please contact the ctalk authors.\n",
	      p -> m_s[subexpr_start] -> value_obj -> __o_class -> __o_name,
	      p -> expr_str);
  }

  __xfree (MEMADDR(s_true));
  __xfree (MEMADDR(s_false));

}

/* For expressions of the form 
 *
 *  (
 */
void question_conditional (EXPR_PARSER *p, int cond_op_idx) {
  int i, colon_idx, pred_end_idx, pred_start_idx, true_start, true_end,
    false_start, false_end;
  OBJECT *pred_object, *result_object = NULL;


  if (p -> m_s[cond_op_idx] -> evaled)  /* Probably been evaled 
					   by the fn just above. 
					   The '?' operator should
					   not have its eval count
					   bumped, anywhere else,
					   so far. */
    return;

  for (i = cond_op_idx - 1; i >= p -> msg_frame_top; --i) {
    if (M_TOK(p -> m_s[i]) == COLON) {
      colon_idx = i;
      break;
    }
  }

  pred_end_idx = __ctalkPrevLangMsg (p -> m_s, cond_op_idx,
				     p -> msg_frame_start);
  for (i = pred_end_idx; i <= p -> msg_frame_start; ++i) {
    if (M_VALUE_OBJ(p -> m_s[i]) != M_VALUE_OBJ(p -> m_s[pred_end_idx])) {
      break;
    } else {
      pred_start_idx = i;
    }
  }

  true_start = __ctalkNextLangMsg (p -> m_s, cond_op_idx,
				       p -> msg_frame_top);
  for (i = true_start; i >= p -> msg_frame_top; --i) {
    if (M_VALUE_OBJ(p -> m_s[i]) != M_VALUE_OBJ(p -> m_s[true_start])) {
      break;
    } else {
      true_end = i;
    }
  }

  false_start = __ctalkNextLangMsg (p -> m_s, colon_idx, p -> msg_frame_top);

  for (i = false_start; i >= p -> msg_frame_top; --i) {
    if (M_VALUE_OBJ(p -> m_s[i]) != M_VALUE_OBJ(p -> m_s[false_start])) {
      break;
    } else {
      false_end = i;
    }
  }

  if ((pred_object = M_VALUE_OBJ(p -> m_s[pred_end_idx])) != NULL) {
    if (pred_object -> instancevars -> attrs & OBJECT_VALUE_IS_BIN_BOOL ||
	pred_object -> instancevars -> attrs & OBJECT_VALUE_IS_BIN_INT) {
      if (INTVAL(pred_object -> instancevars -> __o_value)) {
	result_object = M_VALUE_OBJ(p -> m_s[true_start]);
      } else {
	result_object = M_VALUE_OBJ(p -> m_s[false_start]);
      }
    } else if (pred_object -> __o_class == rt_defclasses -> p_string_class) {
      if (pred_object -> instancevars -> __o_value[0]) {
	result_object = M_VALUE_OBJ(p -> m_s[true_start]);
      } else {
	result_object = M_VALUE_OBJ(p -> m_s[false_start]);
      }
    } else if (pred_object -> __o_class == rt_defclasses -> p_character_class) {
      if (pred_object -> instancevars -> __o_value[0]) {
	result_object = M_VALUE_OBJ(p -> m_s[true_start]);
      } else {
	result_object = M_VALUE_OBJ(p -> m_s[false_start]);
      }
    } else {
      _warning ("Unhandled predicate class, \"%s,\" in expression:\n\n\t%s\n\n"
		"Please contact the ctalk authors.\n",
		pred_object -> __o_class -> __o_name,
		p -> expr_str);
    }
  }

  for (i = pred_start_idx; i >= false_end; i--) {
    if (i < pred_end_idx && i >= false_end) {
      if (IS_OBJECT(p -> m_s[i] -> value_obj)) {
	if (p -> m_s[i] -> value_obj != result_object) {
	  if (p -> m_s[i] -> value_obj -> attrs & OBJECT_IS_I_RESULT) {
	    __ctalkDeleteObject (p -> m_s[i] -> value_obj);
	  }
	}
      }
      p -> m_s[i] -> value_obj = result_object;
    } else {
      p -> m_s[i] -> value_obj = result_object;
    }
    ++(p -> m_s[i]) -> evaled;
  }
}
