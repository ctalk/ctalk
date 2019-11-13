/* $Id: pmessage.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

#ifndef _PMESSAGE_H
#define _PMESSAGE_H

/* When changing this definition, ensure that copy_message () and 
   dup_message () are still valid. */

typedef struct _message {
  int sig;
  char *name;
  char *value;
  int tokentype; 
  int evaled;
  int output;
  int error_line;
  int error_column;
} MESSAGE;


#ifndef MAXARGS
#define MAXARGS 512
#endif

#ifndef N_MESSAGES 
#define N_MESSAGES (MAXARGS * 120)
#endif

#ifndef P_MESSAGES
#define P_MESSAGES (N_MESSAGES * 30)  /* Enough to include all ISO headers. */
#endif

#ifndef __need_MESSAGE_STACK

#define __need_MESSAGE_STACK
typedef MESSAGE ** MESSAGE_STACK;

#endif

struct _messageb {
  MESSAGE_STACK messages[P_MESSAGES+1];
  int start, 
    ptr;
  int (*push_fn)(MESSAGE *);
  MESSAGE *(*pop_fn) (void);
};

typedef struct _messageb MESSAGEB, *P_MESSAGEB;

#endif   /* _PMESSAGE_H */


