/* $Id: rttrace.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2017, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <setjmp.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "except.h"

static int exception_trace = 0;

extern RT_INFO *__call_stack[MAXARGS+1];  /* Declared in rtinfo.c. */
extern int __call_stack_ptr;

int __save_call_stack_to_exception (I_EXCEPTION *x) {
  int i;
  RT_INFO *r;
  x -> call_stack_idx = __call_stack_ptr;
  for (i = x -> call_stack_idx + 1; i <= MAXARGS; i++) {
    r = _new_rtinfo ();
    memcpy ((void *)r, (void *)__call_stack[i], sizeof (struct _rtinfo));
    x -> x_call_stack[i] = r;
  }
  return SUCCESS;
}

void __ctalkSetExceptionTrace (int i) {
  exception_trace = i;
}

int __ctalkGetExceptionTrace (void) {
  return exception_trace;
}

void __warning_trace (void) {
  int idx;
  for (idx = __call_stack_ptr + 1; idx <= MAXARGS; idx++) {
    if (__call_stack[idx] -> method) {
      if (!strstr (__call_stack[idx]->method->name, ARGBLK_LABEL))
	fprintf (stderr, "\tFrom %s : %s\n", 
		 __call_stack[idx]->method->rcvr_class_obj->__o_name,
		 __call_stack[idx]->method->name);
    } else {
      if (__call_stack[idx] -> _rt_fn) {
	if (*(__call_stack[idx] -> _rt_fn -> name))
	  fprintf (stderr, "\tFrom %s ()\n", 
		   __call_stack[idx] -> _rt_fn -> name);
	else
	  fprintf (stderr, "\tFrom %s\n", "<cfunc>");
      }
    }
  }
}

