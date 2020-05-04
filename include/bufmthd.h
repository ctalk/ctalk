/* $Id: bufmthd.h,v 1.4 2020/05/04 22:09:08 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2020 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef __BUFMTHD_H
#define __BUFMTHD_H

struct _mb_list {
  int sig;
  struct _mb_list *next;
  char data[MAXMSG - 8]; /* sizeof (int) + sizeof (void *) */
};

typedef struct _mb_list MBLIST;

typedef struct {
  METHOD *method;
  MBLIST *src;
  MBLIST *src_head;
  MBLIST *init;
  MBLIST *init_head;
  MBLIST *cvar_tab_init, *cvar_tab_init_head;
  MBLIST *cvar_tab_members, *cvar_tab_members_head;
} BUFFERED_METHOD;

#endif 

