/* $Id: stack.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
#include <string.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

/*
 *  These are leftovers from the versions of ctalk which had
 *  the preprocessor in the same program.
 */

MESSAGE *p_messages[P_MESSAGES+1]; /* Param message stack.        */
int p_message_ptr = P_MESSAGES;    /* Param stack pointer.        */

extern MESSAGE *messages[P_MESSAGES+1];  /* Declared in parser.c. */
extern int messageptr;

extern MESSAGE *fn_messages[P_MESSAGES+2];  /* Declared in fnbuf.c. */
extern int fn_message_ptr;

extern MESSAGE *m_messages[N_MESSAGES+1];   /* Declared in method.c. */
extern int m_message_ptr;

extern MESSAGE *method_buf_messages[P_MESSAGES+1];/* Declared in methodbuf.c.*/
extern int method_buf_message_ptr;

extern MESSAGE *tmpl_messages[N_MESSAGES + 1]; /* Declared in fn_tmpl.c. */
extern int tmpl_messageptr;

extern MESSAGE *c_messages[N_MESSAGES + 1];  /* Declared in cparse.c. */
extern int c_message_ptr;

extern MESSAGE *var_messages[N_VAR_MESSAGES + 1]; /* Declared in cvars.c. */
extern int var_messageptr;

extern MESSAGE *argblk_messages[N_MESSAGES + 1]; /* Declared in argblk.c. */
extern int argblk_messageptr; 

extern MESSAGE *r_messages[N_R_MESSAGES + 1]; /* Declared in mthdrf.c. */
extern int r_messageptr;

int p_message_push (MESSAGE *m) {
  if (p_message_ptr == 0) {
    _warning (_("p_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("p_message_push %d. %s."), p_message_ptr, m -> name);
#endif
  p_messages[p_message_ptr--] = m;
  return p_message_ptr + 1;
}

MESSAGE *p_message_pop (void) {
  MESSAGE *m;
  if (p_message_ptr == P_MESSAGES) {
    _warning (_("p_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("p_message_pop %d. %s."), p_message_ptr, 
	 p_messages[p_message_ptr+1] -> name);
#endif
  if (p_messages[p_message_ptr + 1] && 
      IS_MESSAGE(p_messages[p_message_ptr + 1])) {
    m = p_messages[p_message_ptr + 1];
    p_messages[++p_message_ptr] = NULL;
    return m;
  } else {
    p_messages[++p_message_ptr] = NULL;
    return NULL;
  }
}

int stack_start (MESSAGE_STACK stack_param) {
  if (stack_param == p_messages)
    return P_MESSAGES;
  else if (stack_param == messages)
    return P_MESSAGES;
  else if (stack_param == m_messages)
    return N_MESSAGES;
  else if (stack_param == c_messages)
    return N_MESSAGES;
  else if (stack_param == var_messages)
    return N_VAR_MESSAGES;
  else if (stack_param == argblk_messages)
    return N_MESSAGES;
  else if (stack_param == fn_messages)
    return P_MESSAGES;
  else if (stack_param == method_buf_messages)
    return P_MESSAGES;
  else if (stack_param == r_messages)
    return N_R_MESSAGES;
  else if (stack_param == tmpl_messages)
    return N_MESSAGES;
  else
    _error (_("stack_start: unimplemented message stack."));
  /* Avoid warning message. */
  return ERROR;

}

/*
 *  Return the index of the next stack entry.
 */
int get_stack_top (MESSAGE_STACK stack_param) {
  if (stack_param == messages)
    return messageptr;
  else if (stack_param == m_messages)
    return m_message_ptr;
  else if (stack_param == p_messages)
    return p_message_ptr;
  else if (stack_param == fn_messages)
    return fn_message_ptr;
  else if (stack_param == method_buf_messages)
    return method_buf_message_ptr;
  else if (stack_param == c_messages)
    return c_message_ptr;
  else if (stack_param == var_messages)
    return var_messageptr;
  else if (stack_param == argblk_messages)
    return argblk_messageptr;
  else if (stack_param == r_messages)
    return r_messageptr;
  else if (stack_param == tmpl_messages)
    return tmpl_messageptr;
  else
    _error (_("get_stack_top: unimplemented message stack."));
  /* Avoid warning message. */
  return ERROR;
}

