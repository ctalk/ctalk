/* $Id: typecheck.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2017-2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdarg.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "cvar.h"

/* this does not check whether the operand is an object. */
bool is_unary_minus (MESSAGE_STACK messages,
				     int op_idx) {
  int lookahead, lookback;
  MESSAGE *m_next, *m_prev;
  if (M_TOK(messages[op_idx]) == MINUS) {
    if ((lookahead = nextlangmsgstack_pfx (messages, op_idx)) != ERROR) {
      m_next = messages[lookahead];
      if (IS_CONSTANT_TOK(M_TOK(m_next)) ||  /* just check syntax here -
						we check the type of the
						operand later. this helps
						prevent segfaults if
						the expression is 
						incorrect. */
	  (M_TOK(m_next) == LABEL)) {
	if ((lookback = prevlangmsgstack_pfx (messages, op_idx)) != ERROR) {
	  m_prev = messages[lookback];
	  if (IS_C_OP_CHAR(m_prev -> name[0]) ||
	      IS_SEPARATOR(m_prev -> name[0])) {
	    return true;
	  }
	} else {
	  /* We're at the beginning of the stack. */
	  return true;
	}
      }
    }
  }
  return false;
}

bool is_object_prefix (MESSAGE_STACK messages, int tok_idx) {
  int lookahead, lookback;
  if (IS_C_UNARY_MATH_OP(M_TOK(messages[tok_idx])) ||
      is_unary_minus (messages, tok_idx)) {
    if ((lookahead = nextlangmsg (messages, tok_idx)) != ERROR) {
      if (M_TOK(messages[lookahead]) == LABEL) {
	if (is_object_or_param (M_NAME(messages[lookahead]), NULL)) {
	  if ((lookback = prevlangmsgstack (messages, tok_idx)) != ERROR) {
	    if (IS_C_OP_CHAR(messages[lookback] -> name[0]) ||
		IS_SEPARATOR(messages[lookback] -> name[0])) {
	      return true;
	    }
	  } else {
	    /* we're at the beginning of the stack. */
	    return true;
	  }
	} else if (messages[lookahead] -> attrs & TOK_SELF ||
		   messages[lookahead] -> attrs & TOK_SUPER) {
	  if ((lookback = prevlangmsgstack (messages, tok_idx)) != ERROR) {
	    if (IS_C_OP_CHAR(messages[lookback] -> name[0]) ||
		IS_SEPARATOR(messages[lookback] -> name[0])) {
	      return true;
	    }
	  } else {
	    /* we're at the beginning of the stack. */
	    return true;
	  }
	} else {
	  return false;
	}
      }
    }
  } 
  return false;
}

int operand_type_check (MESSAGE_STACK messages, int op_idx) {

  if (IS_C_UNARY_MATH_OP(M_TOK(messages[op_idx])) ||
      is_unary_minus (messages, op_idx)) {
    unary_type_check (messages, op_idx);
  } else if (IS_C_BINARY_MATH_OP(M_TOK(messages[op_idx]))) {
    binary_type_check (messages, op_idx);
  }

  return SUCCESS;
}

int unary_type_check (MESSAGE_STACK messages, int op_idx) {
  return SUCCESS;
}

enum op_type_class {
  op_type_null,
  op_type_object
};


/*
 *  Still very much in the beginning stages... doesn't do
 *  anything yet.
 */
int binary_type_check (MESSAGE_STACK messages, int op_idx) {
  int r, op1_idx = -1, op2_idx = -1;
  MESSAGE *m_op1, *m_op2;
  OBJECT *op1_object, *op2_object;
  enum op_type_class op1_class = op_type_null, op2_class = op_type_null;

  if ((r = operands (messages, op_idx, &op1_idx, &op2_idx)) == ERROR) {
    warning (messages[op_idx], "Parser error.");
    return ERROR;
  }

  if (op1_idx == -1 || op2_idx == -1) {
    error (messages[op_idx], "Invalid operand(s).");
  }

  m_op1 = messages[op1_idx];
  m_op2 = messages[op2_idx];

  switch (M_TOK(m_op1))
    {
    case LABEL:
      if ((op1_object = get_object (M_NAME(m_op1), NULL)) != NULL) {
	op1_class = op_type_object;
      } 
      break;
    }

  switch (M_TOK(m_op2))
    {
    case LABEL:
      if ((op2_object = get_object (M_NAME(m_op1), NULL)) != NULL) {
	op2_class = op_type_object;
      } 
      break;
    }

  /*
   *  We haven't found a way yet to analyze the operand.
   */
  if ((op1_class == op_type_null) || (op2_class == op_type_null))
    return SUCCESS;

  return SUCCESS;
}
