/* $Id: mthdret.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

#include <stdio.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

int tok_after_return_idx (MESSAGE_STACK messages, 
			  int return_keyword_idx) {
  int lookahead, i, stack_top, 
    return_expr_end_idx, term_after_idx, tok_after_idx;

  return_expr_end_idx = return_keyword_idx;

  if ((lookahead = nextlangmsg (messages, return_keyword_idx))
      != ERROR) {

    /*
     *  Find the end of the return expression.
     */
    stack_top = get_stack_top (messages);
    if (M_TOK(messages[lookahead]) == OPENPAREN) {
      return_expr_end_idx = match_paren (messages, lookahead, stack_top);
    } else {
      for (i = lookahead; i > stack_top; i--) {
	if (M_TOK(messages[i]) == SEMICOLON)
	  break;
	return_expr_end_idx = i;
      }
    }
    /* SEMICOLON */
    term_after_idx = nextlangmsg (messages, return_expr_end_idx);
    tok_after_idx = nextlangmsg (messages, term_after_idx);
    return tok_after_idx;
  }
  return ERROR;
}
			  

int return_block (MESSAGE_STACK messages, int return_keyword_idx, 
		  int *return_expr_end_idx) {
  int lookahead, i, stack_top;
  
  if ((lookahead = nextlangmsg (messages, return_keyword_idx))
      != ERROR) {

    /*
     *  Find the end of the return expression.
     */
    stack_top = get_stack_top (messages);
    if (M_TOK(messages[lookahead]) == OPENPAREN) {
      *return_expr_end_idx = match_paren (messages, lookahead, stack_top);
    } else {
      for (i = lookahead; i > stack_top; i--) {
	if (M_TOK(messages[i]) == SEMICOLON)
	  break;
	*return_expr_end_idx = i;
      }
    }

  }

  return ERROR;
}
