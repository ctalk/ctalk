/* $Id: lineinfo.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <unistd.h>
#include <ctype.h>
#include "ctpp.h"
#include "typeof.h"

/*
 *  Once again, there seems to be a problem with line markers
 *  in DJGPP, which generates warning messages when returning
 *  to a file from more than one included files.  For now,
 *  simply omitting the flag in DJGPP line markers should
 *  avoid the warning messages.
 */

/*
 *   This covers for an incompatibility between versions
 *   of GCC and glibc limits.h, because of incompatibility 
 *   between different sets of library headers.
 */

#define INT_MAX 2147483647

char *fmt_line_info (int line, char *file, int flag, int new_line,
		     char *buf_out) {

  char hashbuf[3];

  if (new_line)
    strcpy (hashbuf, "\n#");
  else
    strcpy (hashbuf, "#");

#ifdef __DJGPP__
      sprintf (buf_out, "%s %d \"%s\"\n", hashbuf, line, file);
#else
  if (flag) {
      sprintf (buf_out, "%s %d \"%s\" %d\n", hashbuf, line, file, flag);
  } else {
      sprintf (buf_out, "%s %d \"%s\"\n", hashbuf, line, file);
  }
#endif
  return buf_out;
}

/*
 *  Handle #line preprocessing directives.  The directive
 *  can take three forms:
 *   1. #line <int linenum>
 *   2. #line <int linenum> <literal string filename>
 *   3. #line <tokens>
 *
 *   The last form must expand to one of the first two forms.
 */

extern int error_line;     /* Declared in errorloc.c. */

int line_directive (MESSAGE_STACK messages, int keyword_idx) {

  int end_of_p_stmt_orig, 
    end_of_p_stmt_new,
    linenum_idx, 
    filename_idx,
    line_orig,
    line_d,
    stack_end,
    i;
  MESSAGE *m;
  enum {
    linenum_null,
    linenum_line,
    linenum_file,
    linenum_token
  } state;

  end_of_p_stmt_orig = end_of_p_stmt_new = end_of_p_stmt (messages, keyword_idx);
  i = nextlangmsg (messages, keyword_idx);

  if (messages[i] -> tokentype != INTEGER) {
    expand_line_args (messages, i);
  }

  state = linenum_null;
  line_orig = messages[keyword_idx] -> error_line;
  stack_end = get_stack_top (messages);

  for (linenum_idx = -1, filename_idx = -1; i >= end_of_p_stmt_new; i--) {

    if (messages[i] -> tokentype == WHITESPACE) continue;

    m = messages[i];

    switch (m -> tokentype)
      {
      case INTEGER:
	if (state != linenum_null) {
	  new_exception (parse_error_x);
	} else {
	  state = linenum_line;
	  linenum_idx = i;
	}
	break;
      case LITERAL:
	if (state != linenum_line) {
	  new_exception (parse_error_x);
	} else {
	  state = linenum_file;
	  filename_idx = i;
	}
	break;
      default:
	break;
      }
  }

  if (linenum_idx == -1 || messages[linenum_idx] -> tokentype != INTEGER) {
    new_exception (line_range_x);
    return 0;
  } else {

    if (linenum_idx != -1) {
      /*
       *  Check that the line number is within the
       *  limits of a signed integer, and do nothing
       *  if it isn't - line_range_x skips the 
       *  input line.
       */
      error_line = atoi (messages[linenum_idx] -> name);

      if (error_line < 1 || error_line > INT_MAX) {
	new_exception (line_range_x);
	error_line = line_orig;
	return 0;
      }
      
      line_d = error_line - line_orig;
      /*
       *  After the loop above, i should be left pointing at 
       *  the newline at the end of the #line directive args.
       *  The line number should be updated beginning on the 
       *  following line.  This should only be a slight problem 
       *  if the end of the preprocessor line contains a run of 
       *  newlines.
       *
       *  TO DO - This adjustment is for the input file.
       *  Check that we're not in an include file.
       */
      i = nextlangmsg (messages, i);
      for ( ; i > stack_end; i--)
	messages[i] -> error_line += line_d;
    } /* if (linenum_idx != -1) */

  }

  if (filename_idx != -1) {
    if (messages[filename_idx] -> tokentype != LITERAL) {
      new_exception (parse_error_x);
    } else {
      __source_file (messages[filename_idx] -> name);
    }
  }
    

  return end_of_p_stmt_orig - end_of_p_stmt_new;
}

int expand_line_args (MESSAGE_STACK messages, int arg_idx) {

  int i,end_of_p_stmt_new;
  VAL result;

  end_of_p_stmt_new = end_of_p_stmt (messages, arg_idx);

  macro_sub_parse (messages, arg_idx, &end_of_p_stmt_new, &result);

  /*
   *  Evaluate the argument expression separately, because
   *  macro_parse () only evaluates expressions that are arguments
   *  of preprocessing directives.
   */

  eval_constant_expr (messages, arg_idx, &end_of_p_stmt_new, &result);

  /*
   *  Perform a macro substitution up to the newline or
   *  literal filename.  The result of the line number 
   *  expression will be in messages[i] -> value.
   *  Result may not hold the value of the line number,
   *  if, there was some way that the filename argument 
   *  contained an expression.
   *
   *  The program only needs to paste the line number value into
   *  the first argument token; all the rest of the expression
   *  it replaces with whitespace.
   */
  for (i = arg_idx; i >= end_of_p_stmt_new; i--) {
    if (messages[i] -> tokentype == LITERAL ||
	messages[i] -> tokentype == NEWLINE)
      break;
    if (i == arg_idx) {
      strcpy (messages[i] -> name, messages[i] -> value);
      messages[i] -> tokentype = INTEGER;
    } else {
      strcpy (messages[i] -> name, " ");
      messages[i] -> tokentype = WHITESPACE;
    }
  }

  return SUCCESS;
}
