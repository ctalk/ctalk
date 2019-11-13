/* $Id: ppexcept.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

#include <stdlib.h>
#include "ctpp.h"
#include "typeof.h"

EXCEPTION parse_exception = success_x;
int exception_notify = FALSE;

extern int warnundefsymbols_opt;     /* Declared in rtinfo.c.              */
extern int warn_all_opt;
extern int warn_dup_symbol_wo_args_opt;

extern DEFINITION *last_symbol;    /* Last symbol defined by preprocessor. */

static DEFINITION *last_symbol_env;/* Last symbol defined before 
				      preprocessing.                       */

extern bool constant_expr;      /* Declared in cparse.c.                */

void new_exception (EXCEPTION e) {
  parse_exception = e;
  exception_notify = FALSE;
}

int handle_preprocess_exception (MESSAGE_STACK messages, int idx, 
				 int start, int end) {

  DEFINITION *t;
  int retval = TRUE;

  switch (parse_exception) 
    {
    case cplusplus_header_x:
      parse_exception = success_x;
      /*
       * Delete the symbols defined by the system header.
       */
      if (last_symbol) {
	for (t = last_symbol; ; t = t -> prev) {
	  if (t == last_symbol_env)
	    break;
	  if (t -> prev) {
	    last_symbol = t -> prev;
	    t -> prev -> next = NULL;
	  } else {
	    last_symbol = NULL;
	  }
	  free (t);
	  if (t == last_symbol_env)
	    break;
	}
      }
      break;
    case mismatched_paren_x:
      if (!exception_notify) {
	warning (messages[idx], "Mismatched parentheses.");
	exception_notify = TRUE;
      }
      retval = SKIP;
      break;
    case argument_parse_error_x:
      if (!exception_notify) {
	warning (messages[idx], "Argument parse error.");
	exception_notify = TRUE;
      }
      retval = FALSE;
      break;
    case false_assertion_x:
      warning (messages[idx], "Assertion, \"%s,\" failed.",
	       messages[nextlangmsg (messages, idx)] -> name);
      abort ();
      break;
    case undefined_symbol_x:
      if (warnundefsymbols_opt)
	warning (messages[idx], "Undefined symbol %s.", messages[idx] -> name);
      parse_exception = success_x;
      exception_notify = TRUE;
      retval = FALSE;
      break;
    case assignment_in_constant_expr_x:
      break;
    case missing_arg_list_x:
      if (!exception_notify && warn_dup_symbol_wo_args_opt) {
	warning (messages[idx], "Missing macro argument."); 
	exception_notify = TRUE;
      }
      retval = FALSE;
      break;
    case undefined_keyword_x:
      if (!exception_notify) {
	warning (messages[idx], "Unknown macro keyword, \"%s.\"", 
		 messages[idx] -> name); 
	exception_notify = TRUE;
      }
      retval = SKIP;
      break;
    case deprecated_keyword_x:
      if (!exception_notify && warn_all_opt) {
      warning (messages[idx], "Deprecated macro keyword %s - ignoring.",
	       messages[idx] -> name);
      exception_notify = TRUE;
      }
      retval = SKIP;
      break;
    case redefined_symbol_x:
      if (!exception_notify) {
	warning (messages[idx], "Redefinition of symbol %s.",
		 messages[idx] -> name);
	exception_notify = TRUE;
      }
      retval = FALSE;
      break;
    case else_w_o_if_x:
      if (!exception_notify) {
	warning (messages[idx], "#else without #if.",
		 messages[idx] -> name);
	exception_notify = TRUE;
      }
      retval = FALSE;
      break;
    case endif_w_o_if_x:
      if (!exception_notify) {
	warning (messages[idx], "#endif without #if.",
		 messages[idx] -> name);
	exception_notify = TRUE;
      }
      retval = FALSE;
      break;
    case elif_w_o_if_x:
      if (!exception_notify) {
	warning (messages[idx], "#elif without #if.",
		 messages[idx] -> name);
	exception_notify = TRUE;
      }
      retval = FALSE;
      break;
    case elif_after_ifdef_x:
      if (warn_all_opt)
	warning (messages[idx],
	 "#elif after #ifdef or #ifndef is an extension to standard C.");
      exception_notify = TRUE;
      retval = FALSE;
      parse_exception = success_x;
      break;
    case line_range_x:
      if (!exception_notify) {
	warning (messages[idx], "Line number out of range.",
		 messages[idx] -> name);
	exception_notify = TRUE;
      }
      retval = SKIP;
      break;
    case parse_error_x:
    case undefined_label_x:
    case success_x:
    default:
      retval = FALSE;
      break;
    }
  return retval;
}

void set_preprocess_env (void) {
  constant_expr = True;
  last_symbol_env = last_symbol;
}



