/* $Id: guisetbackground.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

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

#if X11LIB_FRAME
int __ctalkX11SetBackground (OBJECT *self_object) {
  return SUCCESS;
}
int __ctalkGUISetBackground (OBJECT *self_object) {
  return SUCCESS;
}
int __ctalkX11SetBackgroundBasic (int drawable_id,
				  unsigned long int gc_ptr, 
				  char *color) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */
int __ctalkX11SetBackground (OBJECT *self, char *color) {
  return __ctalkGUISetBackground (self, color);
}

int __ctalkX11SetBackgroundBasic (int drawable_id, 
				  unsigned long int gc_ptr, 
				  char *color) {
  char d_buf[MAXLABEL];
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  if (!shm_mem)
    return ERROR;

  sprintf (d_buf, ":%ld:%s", GCBackground, color);
  make_req (shm_mem, PANE_CHANGE_GC_REQUEST,
	    drawable_id, gc_ptr, d_buf);
#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = drawable_id;
  XSendEvent (display, drawable_id, False, 0L, &send_event);
#endif
  wait_req (shm_mem);
  return SUCCESS;
}

/*
 *  This function is now only for panes that have an actual
 *  window, e.g., X11Pane objects.  For other drawables, use
 *  __ctalkX11SetBackgroundBasic.
 */
int __ctalkGUISetBackground (OBJECT *self, char *color) {
  GC gc;
  char d_buf[MAXLABEL], gc_buf[MAXLABEL];
  OBJECT *win_id_value, *gc_value;
  Window win_id;

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  win_id_value = __x11_pane_win_id_value_object (self);
  win_id = (Window) INTVAL(win_id_value -> __o_value);
  if (!win_id) {
    fprintf (stderr, "__ctalkX11SetBackground: Bad window ID. (Did you attach the Pane object first?)\n");
    return ERROR;
  }
  gc_value = __x11_pane_win_gc_value_object (self);
  /*
   *  Make sure that the GC pointer is in hexadecimal format.
   *  (0x00000000....)
   */
  
  strcatx (d_buf, ":", color, NULL);
  make_req (shm_mem, PANE_SET_WINDOW_BACKGROUND_REQUEST, win_id,
	    SYMVAL(gc_value -> __o_value), d_buf);
#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = atoi(win_id_value->__o_value);
  XSendEvent (display, atoi(win_id_value->__o_value),
 	      False, 0L, &send_event);
#endif
  wait_req (shm_mem);
  return SUCCESS;
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
int __ctalkX11SetBackgroundBasic (int drawable_id,
				  unsigned long int gc_ptr,
				  char *color) {
  x_support_error ();
  return 0;  /* notreached */
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
