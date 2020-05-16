/* $Id: message.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015, 2017-2019
      Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include <stddef.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

MESSAGE *new_message (void) {
  MESSAGE *m;

  if ((m = (MESSAGE *)__xalloc (sizeof (MESSAGE))) == NULL)
    _error ("new_message: %s\n", strerror (errno));

  if ((m -> name = (char *)__xalloc (MAXLABEL)) == NULL)
    _error ("new_message: %s\n", strerror (errno));
    
  if ((m -> value = (char *)__xalloc (MAXLABEL)) == NULL)
    _error ("new_message: %s\n", strerror (errno));
    

  m -> sig = 0xd2d2d2;
  return m;
}

void delete_message (MESSAGE *m) {
  if (m && IS_MESSAGE (m)) {
    if (m -> name) __xfree (MEMADDR(m -> name));
    if (m -> value) __xfree (MEMADDR(m -> value));
    __xfree (MEMADDR(m));
    m = NULL;
  } else {
    _warning ("delete_message: Bad message.");
  }
}

/*
 *  Deprecated.  Use dup_message (), below, instead.
 */

void copy_message (MESSAGE *d, MESSAGE *s) {

  if (s -> name) { 
    __xfree (MEMADDR(d -> name)); 
    d -> name = strdup (s -> name); 
  }
  if (s -> value) { 
    __xfree (MEMADDR(d -> value)); 
    d -> value = strdup (s -> value); 
  }
  if (s -> value_obj) d -> value_obj = s -> value_obj;
  if (s -> obj)       d -> obj = s -> obj;
  if (s -> receiver_obj)       d -> receiver_obj = s -> receiver_obj;
  if (s -> receiver_msg)  d -> receiver_msg = s -> receiver_msg;
  d -> tokentype = s -> tokentype;
  d -> evaled = s -> evaled;
  d -> output = s -> output;
  d -> attrs = s -> attrs;
  d -> error_line = s -> error_line;
  d -> error_column = s -> error_column;
  
}

MESSAGE *dup_message (MESSAGE *s) {

  MESSAGE *m_new;

  m_new = new_message ();

  if (s -> name)
    strcpy (m_new -> name, s -> name);

  if (s -> value)
    strcpy (m_new -> value, s -> value);

  if (s -> value_obj) m_new -> value_obj = s -> value_obj;
  if (s -> obj)       m_new -> obj = s -> obj;
  if (s -> receiver_obj)       m_new -> receiver_obj = s -> receiver_obj;
  if (s -> receiver_msg)  m_new -> receiver_msg = s -> receiver_msg;
  m_new -> tokentype = s -> tokentype;
  m_new -> evaled = s -> evaled;
  m_new -> output = s -> output;
  m_new -> error_line = s -> error_line;
  m_new -> error_column = s -> error_column;
  
  return m_new;
}

/*
 *  TO DO - Resize_message normally should be called only during
 *  tokenization, but copy_message () might need to check 
 *  for buffer size as well.
 */
MESSAGE *resize_message (MESSAGE *m, int size) {

  if ((m -> name = (char *)__xrealloc ((void **)&(m -> name),
				       size + 1)) == NULL)
    _error ("resize_message: %s\n", strerror (errno));
    
  if ((m -> value = (char *)__xrealloc ((void **)&(m -> value),
					size + 1)) == NULL)
    _error ("resize_message: %s\n", strerror (errno));

  return m;
}

int __rt_get_stack_top (MESSAGE_STACK m) {
  if (m == _get_e_messages ()) 
    return P_MESSAGES;
  _warning ("_rt_get_stack_top: unknown stack.\n");
  return ERROR;
}

int __rt_get_stack_end (MESSAGE_STACK m) {
  if (m == _get_e_messages ()) 
    return _get_e_message_ptr ();
  _warning ("_rt_get_stack_end: unknown stack.\n");
  return ERROR;
}

/*
 *  Return the stack index of the next non-whitespace and 
 *  non-newline message.
 */
int __ctalkNextLangMsg (MESSAGE_STACK messages, int this_msg, int stack_end) { 
  int i = this_msg - 1;
  while ( i >= stack_end ) {
    /* This is necessary when stack_end != stack_ptr. */
    if (!messages[i] || !IS_MESSAGE (messages[i]))
      return ERROR;
    if (!M_ISSPACE(messages[i]))
      return i;
    --i;
  }
  return ERROR;
}

int __ctalkPrevLangMsg (MESSAGE_STACK messages, int this_msg, int stack_start) {
  int i = this_msg + 1;
  while ( i <= stack_start) {
    if (!M_ISSPACE(messages[i]))
      return i;
    ++i;
  }
  return ERROR;
}


static MESSAGE *reused_messages[P_MESSAGES+1];
static int rm_ptr = 0;

void reuse_message (MESSAGE *m) {
  if (rm_ptr >= P_MESSAGES) {
    delete_message (m);
  } else {
    /* We should keep these so we can diagnose any issues
       more easily. */
    /* memset (m -> name, 0, MAXLABEL);
       memset (m -> value, 0, MAXLABEL); */
    m -> obj = m -> value_obj = m -> receiver_obj = NULL;
    m -> receiver_msg = NULL;
    m -> evaled = m -> output = 0;
    m -> attr_method = m -> attr_data = 0l;
    m -> attrs = 0L;
    reused_messages[rm_ptr++] = m;
  }
}


MESSAGE *get_reused_message (void) {
  MESSAGE *m;

  if (rm_ptr == 0) {
    return new_message ();
  } else {
    m = reused_messages[--rm_ptr];
    reused_messages[rm_ptr] = NULL;
  }
  return m;
}

void cleanup_reused_messages (void) {
  while (--rm_ptr >= 0) {
    if (reused_messages[rm_ptr])
      delete_message (reused_messages[rm_ptr]);
  }
}
