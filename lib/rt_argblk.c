/* $Id: rt_argblk.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012  Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"

/*
 *  Inline calls do not (yet) save and restore the rtinfo struct,
 *  so these functions look on the call stack directly.
 */

extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

/*
 *  C_RT_INFO is the block's call stack entry.  INLINE_CALLER_RT_INFO
 *  for the outermost block is two entries up. The map call is the 
 *  previous entry, and the entry before the map call is the method 
 *  where the block is defined.
 *
 *  The frame of the calling method or function is
 *    __call_stack_ptr + 1 + (2 * nested_argument_block_levels).
 */
#define C_RT_INFO __call_stack[__call_stack_ptr+1]  /* From lib/rtinfo.c. */
#define INLINE_CALLER_RT_INFO __call_stack[__call_stack[__call_stack_ptr+1]->_block_frame_top]

int __ctalkEnterArgBlockScope (void) {

  int blk_frame_offset;

  if (C_RT_INFO->inline_call) {
    blk_frame_offset = 1;
    blk_frame_offset += 2;
    C_RT_INFO->block_scope = True;
    C_RT_INFO->_block_frame_top = __call_stack_ptr + blk_frame_offset;

    while (__call_stack[__call_stack_ptr + blk_frame_offset] -> 
	   block_scope == True) {
      blk_frame_offset += 2;
      C_RT_INFO->block_scope = True;
      C_RT_INFO->_block_frame_top = __call_stack_ptr + blk_frame_offset;
    }
  } else {
    _warning ("__ctalkEnterArgBlockScope: Not an inline method call.\n");
    _warning ("__ctalkEnterArgBlockScope: See the documentation for inline methods\n");
    _warning ("__ctalkEnterArgBlockScope: and the __ctalkInlineMethod library\n");
    _warning ("__ctalkEnterArgBlockScope: function.\n");
    return ERROR;
  }
  return SUCCESS;
}
/*
 *  This can be a no-op for now.
 */
int __ctalkExitArgBlockScope (void) {
  return SUCCESS;
}

int __ctalkIsArgBlockScope (void) {
  if (__call_stack_ptr >= MAXARGS)
    return FALSE;
  return (int)C_RT_INFO->block_scope;
}

int __ctalkBlockCallerFrame (void) {
  if (__call_stack_ptr >= MAXARGS)
    return FALSE;
  return C_RT_INFO->_block_frame_top;
}

METHOD *__ctalkBlockCallerMethod (void) {
  if (__call_stack_ptr >= MAXARGS)
    return NULL;
  return ((INLINE_CALLER_RT_INFO->method) ? 
	  INLINE_CALLER_RT_INFO -> method : NULL);
}

RT_FN *__ctalkBlockCallerFn (void) {
  if (__call_stack_ptr >= MAXARGS)
    return NULL;
  return ((INLINE_CALLER_RT_INFO->_rt_fn) ? 
	  INLINE_CALLER_RT_INFO -> _rt_fn : NULL);
}
