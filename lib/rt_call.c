/* $Id: rt_call.c,v 1.2 2020/09/18 21:25:12 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016, 2019 - 2020  Robert Kiesling, 
     rk3314042@gmail.com.
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

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

extern RT_INFO *__call_stack[MAXARGS+1];    /* Declared in rtinfo.c. */
extern int __call_stack_ptr;

static OBJECT *__e_result;

OBJECT *__ctalkCallMethodFn (METHOD *m) {

  OBJECT *(*fn)();
  __e_result = NULL;
  fn = m -> cfunc;
  __e_result = (*fn)();
  return __e_result;
}


/* Note that the return is true for a miss and false for success */
bool sym_ptr_expr (EXPR_PARSER *p, 
		  int open_paren_idx,
		  int close_paren_idx, 
		  int *last_paren_idx_out) {
  int param_open_paren_idx;
  int param_close_paren_idx;
  int prev_idx;
  int pfx_idx;
  int label_idx;
  int pre_call_stack_ptr;
  void (*fn_addr) ();
  OBJECT *symbol_obj, *symbol_value_obj;

  if ((param_open_paren_idx = __ctalkNextLangMsg (p -> m_s,
						  close_paren_idx,
						  p -> msg_frame_top))
      != ERROR) {
    if (M_TOK(p -> m_s[param_open_paren_idx]) != OPENPAREN) {
      return true;
    }
  } else {
    return true;
  }
  
  if ((prev_idx = __ctalkPrevLangMsg (p -> m_s, open_paren_idx,
				      p -> msg_frame_start)) != ERROR) {
    /* We've already evaluated the expression via eval_subexpr (). */
    if (M_TOK(p -> m_s[prev_idx]) == OPENPAREN) {
      *last_paren_idx_out = __ctalkMatchParen (p -> m_s, prev_idx,
						 p -> msg_frame_top);
      return false;
    }
  } else {
    /* ... there doesn't need to be a token before the opening parens. */
  }

  if ((pfx_idx = __ctalkNextLangMsg (p -> m_s, open_paren_idx, p -> msg_frame_top))
      == ERROR)
    return true;
  if (M_TOK(p -> m_s[pfx_idx]) != MULT)
    return true;

  if ((label_idx = __ctalkNextLangMsg (p -> m_s, pfx_idx, p -> msg_frame_top))
      == ERROR)
    return true;

  if (M_TOK(p -> m_s[label_idx]) != LABEL)
    return true;

  if ((symbol_obj = M_VALUE_OBJ(p -> m_s[label_idx])) == NULL)
    return true;

  if (!is_class_or_subclass (symbol_obj,  rt_defclasses -> p_symbol_class)) {
    _warning ("In expression, \"(*%s)(),\"  \"%s,\" is not a Symbol object expression.\n",
	      symbol_obj -> __o_name, symbol_obj -> __o_name);
    return true;
  }

  if ((param_close_paren_idx = __ctalkNextLangMsg (p -> m_s,
						   param_open_paren_idx,
						   p -> msg_frame_top))
      == ERROR)
    return true;

  symbol_value_obj = ((symbol_obj -> instancevars) ? 
		      symbol_obj -> instancevars :
		      symbol_obj);
  pre_call_stack_ptr = __call_stack_ptr;

  fn_addr = *(void **)symbol_value_obj -> __o_value;
  if (fn_addr != NULL) {
    (*fn_addr)();
  } else {
    _warning ("In expression, \"(*%s)(),\"  \"%s,\" is NULL.\n",
	      symbol_obj -> __o_name, symbol_obj -> __o_name);
  }

  if (pre_call_stack_ptr > __call_stack_ptr) {
    /* If the function placed an entry on the call stack and didn't
       remove it, remove it here.  TODO - Check for nested calls
       (more than one new stack_entry), and the full state being
       saved in rtinfo.c if possible. */
    RT_INFO *r;
    r = __call_stack[++__call_stack_ptr];
    __call_stack[__call_stack_ptr] = NULL;
    __xfree (MEMADDR(r));
  }
  *last_paren_idx_out = param_close_paren_idx;
  return false;
}
