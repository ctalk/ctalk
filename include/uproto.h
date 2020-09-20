/* $Id: uproto.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */
/*
  Copyright © 2005-2015 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _OBJECT_H
#include "object.h"
#endif

#ifndef _UPROTO_H
#define _UPROTO_H

typedef struct _uproto UPROTO;

struct _uproto {
  char classname[MAXLABEL];
  char name[MAXLABEL];
  char alias[MAXLABEL];
  char returnclass[MAXLABEL];
  LIST *params;
};

#endif /* _UPROTO_H */
