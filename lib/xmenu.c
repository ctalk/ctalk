/* $Id: xmenu.c,v 1.5 2020/12/28 23:05:13 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014, 2018 - 2020  Robert Kiesling, rk3314042@gmail.com.
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include "x11defs.h"
#include <X11/Xutil.h>

extern Display *display;   /* Defined in x11lib.c. */
extern GC create_pane_win_gc (Display *d, Window w, OBJECT *pane);

int __ctalkX11CreatePopupMenu (OBJECT *self_object, int x, int y) {
  Window menu_win_id;
  XSetWindowAttributes set_attributes;
  GC gc;
  OBJECT *displayPtr, *mainWinPtr, *mainWin, *titleStr;
  static int wm_event_mask;
  int x_org, y_org, x_size, y_size, border_width;
  Display *d_l;

  wm_event_mask = WM_CONFIGURE_EVENTS | WM_INPUT_EVENTS;

  if ((d_l = XOpenDisplay (getenv ("DISPLAY"))) == NULL) {
    return ERROR;
  }

  mainWinPtr = __ctalkGetInstanceVariable (self_object,
					   "mainWindowPtr", TRUE);
  mainWin = *(OBJECT **)mainWinPtr -> instancevars -> __o_value;
  
  titleStr = __ctalkPaneResource (self_object, "titleText", TRUE);

  /* border_width = __x11_pane_border_width (self_object); *//***/
  border_width = 0;
  set_attributes.backing_store = Always;
  set_attributes.save_under = true;
  set_attributes.override_redirect = true;

  menu_win_id = XCreateWindow (d_l, DefaultRootWindow (d_l), 
			  x_org, y_org, x_size, y_size,
			  border_width,
			  DefaultDepth (display, DefaultScreen (display)),
			  CopyFromParent, CopyFromParent, 
			  CWBackingStore|CWSaveUnder|CWOverrideRedirect,
			  &set_attributes);

  XSelectInput(d_l, menu_win_id, wm_event_mask);

  gc = create_pane_win_gc (d_l, menu_win_id, self_object);

  /* XMoveResizeWindow (d_l, menu_win_id, x_org, y_org, x_size, y_size); *//***/

  return menu_win_id;
}

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

int __ctalkX11CreatePopupMenu (OBJECT *self_object, int x, int y) {
  fprintf (stderr, "__ctalkX11CreatePopupMenu: This program requires "
	   "the X Window System.  Exiting.\n");
  exit (ERROR);
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
