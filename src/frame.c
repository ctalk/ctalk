/* $Id: frame.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015 Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include "object.h"
#include "message.h"
#include "frame.h"
#include "ctalk.h"

extern int parser_level;            /* Declared in parser.c.                */

FRAME *frames[MAXFRAMES+1];  /* Frame stack and frame stack pointer. */
int frame_pointer;

/* 
 *   Return the current frame.    
 */
FRAME *this_frame (void) {  
  return frames[frame_pointer];
}

/* 
 * Return the frame for a specific frame pointer.
 */
FRAME *frame_at (int ptr) {
  if (ptr == ERROR)
    return NULL;
  else
    return frames[ptr];
}

/* 
 *   Return the frame pointer.
 */
int get_frame_pointer (void) {
  return frame_pointer;
}

/*
 *   Initialize the frame pointer.
 */
int init_frames (void) {
  frame_pointer = MAXFRAMES + 1;
  return SUCCESS;
}

/*
 *   Returns true if the frame's scope is GLOBAL_VAR.
 */
int is_global_frame () {

  FRAME *f;

  if ((f = frame_at (parser_frame_ptr ())) != NULL) {
    if (f -> scope == GLOBAL_VAR) {
      return TRUE;
    } else {
      return FALSE;
    }
  }
  return ERROR;
}

/*
 *   Create a frame and initialize its scope and stack indexes.
 */
int new_frame (int scope, int parser_level) {

  FRAME *f;
  int message_ptr;

  message_ptr = get_messageptr ();
  /* The message at message_ptr has not been pushed yet.  Use the
     previous message. */

  if (message_ptr > P_MESSAGES)
    _error ("New_frame: var stack underflow: message pointer %d.",
	    message_ptr);

  if ((f = (FRAME *) calloc (1, sizeof (struct _frame))) == NULL)
    _error ("New_frame: %s.", strerror (errno));

  f -> scope = scope;
  f -> level = parser_level;

  f -> message_frame_top = message_ptr;

  frames[--frame_pointer] = f;

#ifdef STACK_TRACE
  debug ("------------------------------");
  debug ("level: %d, frame: %d: %d.", 
	 f -> level,
	 frame_pointer,
	 f -> scope,
	 f -> message_frame_top);
#endif

  return SUCCESS;
}

int delete_frame (void) {

  FRAME *f;

  if (frame_pointer > MAXFRAMES) {
    _warning ("delete_frame: stack underflow: %d.", frame_pointer);
    return ERROR;
  }


  f = frames[frame_pointer];
  __xfree (MEMADDR(f));
  frames[frame_pointer] = NULL;
#ifdef STACK_TRACE
  debug ("Exit frame: %d.\n------------------------------", 
	  frame_pointer);
#endif  
  ++frame_pointer;
  return SUCCESS;
}

int message_frame_top (void) {
  return message_frame_top_n (frame_pointer);
}

int message_frame_top_n (int ptr) {
  if ((ptr == MAXFRAMES + 1) || (ptr < frame_pointer))
    return ERROR;
  return frames[ptr] -> message_frame_top;
}


