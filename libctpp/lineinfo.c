/* $Id: lineinfo.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <string.h>
#include "ctpp.h"
#include "typeof.h"
#include "prtinfo.h"

extern int error_line,       /* Declared in errorloc.c. */
  error_column;
extern RT_INFO rtinfo;       /* Declared in rtinfo.c.   */

int line_info_line = FALSE;

MESSAGE *l_messages[P_MESSAGES+1];
int l_message_ptr = P_MESSAGES;

int l_message_push (MESSAGE *m) {
  if (l_message_ptr == 0) {
    _warning (_("l_message_push: stack overflow.\n"));
    return ERROR;
  }
  l_messages[l_message_ptr--] = m;
  return l_message_ptr + 1;
}

MESSAGE *l_message_pop (void) {
  MESSAGE *m;
  if (l_message_ptr == P_MESSAGES) {
    _warning (_("l_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
  if (l_messages[l_message_ptr + 1] && 
      IS_MESSAGE(l_messages[l_message_ptr + 1])) {
    m = l_messages[l_message_ptr + 1];
    l_messages[++l_message_ptr] = NULL;
    return m;
  } else {
    l_messages[++l_message_ptr] = NULL;
    return NULL;
  }
}

int line_info_tok (char *l) {

  int i, stack_start, stack_top, error;

  stack_start = l_message_ptr;
  line_info_line = TRUE;
  stack_top = tokenize_reuse (l_message_push, l);
  line_info_line = FALSE;

  error = set_line_info (l_messages, stack_start, stack_top);

  for (i = stack_top + 1; i <= stack_start; i++)
    reuse_message (l_message_pop ());

  return error;
}

int set_line_info (MESSAGE_STACK messages, int start, int end) {

  int i;
  char l_fn[FILENAME_MAX];
  int l_err_line, l_flag;
  int lasttoken = -1, 
    error = -1;
  MESSAGE *m;

  for (i = start, error = FALSE, l_err_line = 0, l_flag = 0; i >= end; i--) {

    m = messages[i];

    if (m -> tokentype == WHITESPACE)
      continue;
    if (m -> tokentype == NEWLINE)
      break;

    switch (m -> tokentype)
      {
      case PREPROCESS:
	break;
      case INTEGER:
	switch (lasttoken)
	  {
	  case PREPROCESS:
	    l_err_line = atoi (m -> name);
	    break;
	  case LITERAL:
	    l_flag = atoi (m -> name);
	    break;
	  default:
	    _warning ("set_line_info: Badly formed line marker.");
	    error = TRUE;
	    break;
	  }
	break;
      case LITERAL:
	strcpy (l_fn, m -> name);
	break;
      default:
	_warning ("set_line_info: Badly formed line marker.");
	error = TRUE;
	break;
      }

    lasttoken = m -> tokentype;
  }

  if (!error) {

    /*
     *   Trim the quotes from the LITERAL token if necessary.
     */
    if (*l_fn == '\"') {
      char buf[FILENAME_MAX];
      substrcpy (buf, l_fn, 1, strlen (l_fn) - 2);
      __source_file (buf);
    } else {
      __source_file (l_fn);
    }

    switch (l_flag)
      {
      case 0:
      case 1:
	error_line = l_err_line;
	error_column = 1;
	break;
      case 2:
 	error_line = l_err_line;
 	error_column = 1;
	break;
      default:
	_warning ("set_line_info: unimplemented flag %d.", l_flag);
	error = TRUE;
	break;
      }
  }

  return error;
}
