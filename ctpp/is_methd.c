/* $Id: is_methd.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

#include <string.h>
#include "ctpp.h"
#include "typeof.h"

/*
 *  We need to find only the start of the method declaration, 
 *  so look for a, "instanceMethod," keyword as the next label token.
 */
int is_instance_method_declaration_start (MESSAGE_STACK messages, int start, int end) {

  int i, next_ptr, prev_ptr;
  MESSAGE *m_tok;

  for (i = start; i > end; i--) {
    
    m_tok = messages[i];

    switch (m_tok -> tokentype) 
      {
      case LABEL:
	if ((next_ptr = nextlangmsg (messages, i)) != ERROR) {
	  if (!strcmp (messages[next_ptr] -> name, "instanceMethod"))
	    return TRUE;
	}
	if (!strcmp (m_tok -> name, "instanceMethod")) {
	  if ((prev_ptr = prevlangmsg (messages, i)) != ERROR) {
	    if (messages[prev_ptr] -> tokentype == LABEL)
	      return TRUE;
	  }
	}
	break;
      default:
	return FALSE;
	break;
      }
  }
  return FALSE;
}

