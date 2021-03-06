/* $Id: xgeometry.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include "x11defs.h"
#include <X11/Xutil.h>

extern Display *display;   /* Defined in x11lib.c. */

#if X11LIB_FRAME
int __ctalkX11ParseGeometry (char *s, int *x, int *y, int *w, int *h) {
  return SUCCESS;
}
int __ctalkX11SetSizeHints (int x, int y, int width, int height,
			    int geomflags) {
  return SUCCESS;
}
void __ctalkX11GetSizeHints (int, int *x, int *y, int *width, int *height, 
			     int *win_gravity, int *flags) {
}

void __ctalkX11FreeSizeHints (void) {
}
#else /* X11LIB_FRAME */

int __ctalkX11ParseGeometry (char *geometry_string, int *x, int *y, 
			int *width, int *height) {
  int x_return, y_return, flags;
  unsigned int width_return, height_return;

  flags = XParseGeometry (geometry_string, 
			  &x_return, &y_return, &width_return, &height_return);

  if (flags & XValue) *x = x_return; else *x = 0;
  if (flags & YValue) *y = y_return; else *y = 0;
  if (flags & WidthValue) *width = width_return; else *width = 0;
  if (flags & HeightValue) 
    *height = height_return; 
  else
    *height = 0;

  return flags;
}

int __geomFlags = -1;
XSizeHints *size_hints = NULL;
XWMHints *wm_hints = NULL;

int __ctalkX11SetSizeHints (int x, int y, int width, int height,
			    int geomflags) {
  size_hints = XAllocSizeHints ();
  memset (size_hints, 0, sizeof (XSizeHints));

  __geomFlags = geomflags;

  size_hints -> win_gravity = NorthWestGravity;
  if (geomflags & XValue) {
    if (geomflags & XNegative) {
      size_hints -> win_gravity = NorthEastGravity;
    }
    size_hints -> x = x;
  } else {
    size_hints -> x = 0;
  }

  if (geomflags & YValue) {
    if (geomflags & YNegative) {
      if (size_hints -> win_gravity == NorthEastGravity) {
	size_hints -> win_gravity = SouthEastGravity;
      } else {
	size_hints -> win_gravity = SouthWestGravity;
      }
    }
    size_hints -> y = y;
  } else {
    size_hints -> y = 0;
  }

  if (width == 0) 
    size_hints -> base_width = DEFAULT_WIDTH;
  else
    size_hints -> base_width = width;
  if (height == 0)
    size_hints -> base_height = DEFAULT_HEIGHT;
  else
    size_hints -> base_height = height;

  if (!XValue || !YValue)
    size_hints -> flags = PWinGravity|PBaseSize|USPosition;
  else
    size_hints -> flags = PWinGravity|PBaseSize|PPosition;

  return SUCCESS;
}

void __ctalkX11GetSizeHints (int win_id, int *x, int *y, 
			     int *width, int *height, 
			     int *win_gravity, int *flags) {
  XSizeHints hints_return;
  if (size_hints) {
    if (display) {
      XGetWMNormalHints (display, (Window)win_id, &hints_return, 
			 (long int *)size_hints);
      *x = hints_return.x;
      *y = hints_return.y;
      *width = hints_return.base_width;
      *height = hints_return.base_height;
      *win_gravity = hints_return.win_gravity;
      *flags = hints_return.flags;
    } else {
      *x = size_hints -> x;
      *y = size_hints -> y;
      *width = size_hints -> base_width;
      *height = size_hints -> base_height;
      *win_gravity = size_hints -> win_gravity;
      *flags = size_hints -> flags;
    }
  }

}

/* Called by __ctalkCreateX11MainWindow and
   __ctalkCreateGLXMainWindow. */
void set_size_hints_internal (OBJECT *pane_object, int *x_org_out,
			      int *y_org_out, int *x_size_out,
			      int *y_size_out) {
  int pane_x, pane_y, pane_width, pane_height;
  if (__geomFlags <= 0) {
    size_hints = XAllocSizeHints ();
    pane_x = __pane_x_org (pane_object);
    pane_y = __pane_y_org (pane_object);
    pane_width = __pane_x_size (pane_object);
    pane_height = __pane_y_size (pane_object);
    size_hints -> x = (pane_x) ? pane_x : 0;
    size_hints -> y = (pane_y) ? pane_y : 0;
    size_hints -> base_width = (pane_width) ? pane_width : 250;
    size_hints -> base_height = (pane_height) ? pane_height : 250;
    if (!pane_x || !pane_y)
      size_hints -> flags = PBaseSize|PPosition;
    else
      size_hints -> flags = PBaseSize|USPosition;
    *x_org_out = size_hints -> x;
    *y_org_out = size_hints -> y;
    *x_size_out = size_hints -> base_width;
    *y_size_out = size_hints -> base_height;
  } else {
    if (size_hints) {
      *x_org_out = size_hints -> x;
      *y_org_out = size_hints -> y;
      *x_size_out = size_hints -> base_width;
      *y_size_out = size_hints -> base_height;
    } else {
      *x_org_out = __pane_x_org (pane_object);
      *y_org_out = __pane_y_org (pane_object);
      *x_size_out = __pane_x_size (pane_object);
      *y_size_out = __pane_y_size (pane_object);
    }
    if (__geomFlags & XNegative) {
      *x_org_out = DisplayWidth (display, DefaultScreen (display)) 
       	+ *x_org_out - *x_size_out;
      size_hints -> x = *x_org_out;
    }
    if (__geomFlags & YNegative) {
      *y_org_out = DisplayHeight (display, DefaultScreen (display)) 
       	+ *y_org_out - *y_size_out;
      size_hints -> y = *y_org_out;
    }
  }
}

void __ctalkX11FreeSizeHints (void) {
  if (size_hints)
    XFree (size_hints);
}
#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

static void __gui_support_error (void) {
  x_support_error ();
}

int __ctalkX11ParseGeometry (char *s, int *x, int *y, int *w, int *h) {
  x_support_error (); return ERROR;
}
int __ctalkX11SetSizeHints (int x, int y, int width, int height,
			    int geomflags) {
  x_support_error (); return ERROR;
}
void __ctalkX11GetSizeHints (int win_id, int *x, int *y, int *width, 
			     int *height, 
			     int *win_gravity, int *flags) {
  x_support_error ();
}

void __ctalkX11FreeSizeHints (void) {
  x_support_error ();
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
