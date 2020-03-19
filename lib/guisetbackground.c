/* $Id: guisetbackground.c,v 1.11 2020/03/19 16:10:16 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016-2019  Robert Kiesling, rk3314042@gmail.com.
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

/*
 *  __ctalkX11PaneClearWindow is here for compatibility,
 *  but it is deprecated.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include "x11defs.h"

extern Display *display;   /* Defined in x11lib.c. */
extern char *shm_mem;
extern int mem_id;
unsigned long lookup_pixel_d (Display *, char *);

#if X11LIB_FRAME
int __ctalkX11SetBackground (OBJECT *self_object) {
  return SUCCESS;
}
int __ctalkGUISetBackground (OBJECT *self_object) {
  return SUCCESS;
}
int __ctalkX11SetBackgroundBasic (void *d, int drawable_id,
				  unsigned long int gc_ptr, 
				  char *color) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

extern Display *d_p;
int __xlib_change_gc (Display *, Drawable, GC, char *);

int __ctalkX11SetBackground (OBJECT *self, char *color) {
  return __ctalkGUISetBackground (self, color);
}

int __ctalkX11SetBackgroundBasic (void *d, int drawable_id, 
				  unsigned long int gc_ptr, 
				  char *color) {
  char d_buf[MAXLABEL];
  Display *l_d;
  XGCValues v;
  GC gc;
  int r;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  
  if (!shm_mem)
    return ERROR;

  if (drawable_id == 0)
    _error ("ctalk: __ctalkX11SetBackgroundBasic: Bad window ID. "
	    "(Did you attach the Pane object first?)\n");

  if (__client_pid () < 0) {
    gc = (GC)gc_ptr;
    if (display == NULL) {
      if ((l_d = XOpenDisplay (getenv ("DISPLAY"))) == NULL) {
	_error ("ctalk: This program requires the X Window System. Exiting.\n");
      }
      v.background = lookup_pixel_d (l_d, color);
      r = XChangeGC (l_d, gc, GCBackground, &v);
      XCloseDisplay (l_d);
    } else {
      v.background = lookup_pixel_d (display, color);
      r = XChangeGC (display, gc, GCBackground, &v);
    }
  } else {

    if (!shm_mem)
      return ERROR;

    sprintf (d_buf, ":%ld:%s", GCBackground, color);
    if (DIALOG(d)) {
      __xlib_change_gc (d, drawable_id, (GC)gc_ptr, d_buf);
    } else {
      make_req (shm_mem, d, PANE_CHANGE_GC_REQUEST,
		drawable_id, gc_ptr, d_buf);
#ifdef GRAPHICS_WRITE_SEND_EVENT
      send_event.xgraphicsexpose.type = GraphicsExpose;
      send_event.xgraphicsexpose.send_event = True;
      send_event.xgraphicsexpose.display = display;
      send_event.xgraphicsexpose.drawable = drawable_id;
      XSendEvent (display, drawable_id, False, 0L, &send_event);
#endif
      wait_req (shm_mem);
    }
    return SUCCESS;
  }
}

int __ctalkGUISetBackground (OBJECT *self, char *color) {
  OBJECT *win_id_value, *gc_value, *displayptr_var;

  win_id_value = __x11_pane_win_id_value_object (self);
  gc_value = __x11_pane_win_gc_value_object (self);
  displayptr_var = __ctalkGetInstanceVariable (self, "displayPtr", TRUE);

  return __ctalkX11SetBackgroundBasic
    ((void *)SYMVAL(displayptr_var -> instancevars -> __o_value),
     INTVAL(win_id_value -> __o_value), SYMVAL(gc_value -> __o_value),
     color);
}

#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11SetBackground (OBJECT *self_object, char *color) {
  x_support_error ();
  return 0;  /* notreached */
}
int __ctalkGUISetBackground (OBJECT *self_object, char *color) {
  x_support_error ();
  return 0; /* notreached */
}
int __ctalkX11SetBackgroundBasic (void *d, int drawable_id,
				  unsigned long int gc_ptr,
				  char *color) {
  x_support_error ();
  return 0;  /* notreached */
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
