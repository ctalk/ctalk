/* $Id: message.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2014, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include "ctpp.h"

extern int keepcomments_opt;    /* Declared in rtinfo.c. */

MESSAGE *new_message (void) {
  MESSAGE *m;

  if ((m = (MESSAGE *)calloc (1, sizeof (MESSAGE))) == NULL)
    _error ("new_message: %s\n", strerror (errno));

  if ((m -> name = (char *)calloc (MAXLABEL, sizeof (char))) == NULL)
    _error ("new_message: %s\n", strerror (errno));
    
  if ((m -> value = (char *)calloc (MAXLABEL, sizeof (char))) == NULL)
    _error ("new_message: %s\n", strerror (errno));
    

/*   strcpy (m -> sig, "MESSAGE"); */
  m -> sig = MESSAGE_SIG;
  m -> evaled = 0;
  m -> output = 0;
  return m;
}

void delete_message (MESSAGE *m) {
 if (m && IS_MESSAGE (m)) {
    if (m -> name) free (m -> name);
    if (m -> value) free (m -> value);
    free (m);
  } else {
    _warning ("delete_message: Bad message.");
  }
}

/*
 *  Deprecated.  Use dup_message (), below, instead.
 */

void copy_message (MESSAGE *d, MESSAGE *s) {

  if (s -> name) { 
    free (d -> name); 
    d -> name = strdup (s -> name); 
  }
  if (s -> value) { 
    free (d -> value); 
    d -> value = strdup (s -> value); 
  }
  d -> tokentype = s -> tokentype;
  d -> evaled = s -> evaled;
  d -> output = s -> output;
  d -> error_line = s -> error_line;
  d -> error_column = s -> error_column;
  
}

MESSAGE *dup_message (const MESSAGE *s) {

  MESSAGE *m_new;

  m_new = new_message ();

  if (strlen (s -> name) > MAXLABEL) {
    if (s -> name) {
        free (m_new -> name);
        m_new -> name = strdup (s -> name);
    }
  } else {
    if (s -> name)
      strcpy (m_new -> name, s -> name);
  }

  if (strlen (s -> value) > MAXLABEL) {
    free (m_new -> value);
    m_new -> value = strdup (s -> value);
  } else {
    if (s -> value)
      strcpy (m_new -> value, s -> value);
  }

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

  if ((m -> name = (char *)realloc (m -> name, (size + 1) * sizeof (char)))
      == NULL)
    _error ("resize_message: %s\n", strerror (errno));
    
  if ((m -> value = (char *)realloc (m -> value, (size + 1) * sizeof (char)))
      == NULL)
    _error ("resize_message: %s\n", strerror (errno));

  return m;
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
    m -> evaled = m -> output = 0;
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
