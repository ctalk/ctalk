/* $Id: list.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of ctalk.
  Copyright © 2005-2007 Robert Kiesling, rkiesling@users.sourceforge.net
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

#ifndef _LIST_H
#define _LIST_H

typedef struct _list LIST;

struct _list {
  int sig;
  void *data;
  struct _list *next;
  struct _list *prev;
};

#define LIST_SIG 0xfff0

#ifndef IS_LIST
#define IS_LIST(x) ((x) && (x)->sig==LIST_SIG)
#endif

#endif /* _LIST_H */

