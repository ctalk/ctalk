/* $Id: chkstate.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

#include <stdio.h>
#include <string.h>
#include "ctpp.h"

#define A_IDX_(a,b,ncols) (((a)*(ncols))+(b))

int check_state_old (int stack_idx, MESSAGE **messages, int *states, 
		 int cols) {
  int i, j;
  int token;
  int next_idx = stack_idx;

  token = messages[stack_idx] -> tokentype;

  for (i = 0; ; i++) {

  nextrans:
     if (states[A_IDX_(i,0,cols)] == ERROR)
       return ERROR;

     if (states[A_IDX_(i,0,cols)] == token) {

       for (j = 1, next_idx = stack_idx - 1; ; j++, next_idx--) {
	
	 while (states[A_IDX_(i,j,cols)]) {
	   if (!messages[next_idx] || !IS_MESSAGE (messages[next_idx]))
	     return ERROR;
	   while ((messages[next_idx]->tokentype == WHITESPACE) ||
		  (messages[next_idx]->tokentype == NEWLINE)) {
	     next_idx--;
	     /* TO DO - 
		Find a way to notify the calling function whether 
		the message is the end of the stack, or determine
		the end of the stack by the calling function.
	     */
	     if (!messages[next_idx] || !IS_MESSAGE (messages[next_idx]))
	       return ERROR;
	   }
	   if (states[A_IDX_(i,j,cols)] != messages[next_idx]->tokentype) {
	     ++i;
	     goto nextrans;
	   }
	   else
	     ++j;
	 }
	 return i;
       }
     }
  }
}

int check_state (int stack_idx, MESSAGE **messages, int *states, 
		 int cols) {
  register int i;
  int token;
  int next_idx = stack_idx;
  int last_idx;
  register int *states_ptr;

  states_ptr = states;
  token = messages[stack_idx] -> tokentype;

  last_idx = get_stack_top (messages);

  for (i = 0; ; i++) {
  nextrans:
    if (states_ptr[i+i] == ERROR)
      return ERROR;
    if (states_ptr[i+i] == token) {
      for (next_idx = stack_idx - 1; next_idx > last_idx; next_idx--) {
	while (M_ISSPACE(messages[next_idx])) {
	  next_idx--;
	  if (next_idx == last_idx) return ERROR;
	}
	if (states_ptr[i+i+1] != messages[next_idx]->tokentype) {
	  ++i;
 	  goto nextrans;
	} else {
	  return i;
	}
      }
    }
  }
  return ERROR;
}
      
