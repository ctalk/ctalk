/* $Id: guirefresh.c,v 1.1.1.1 2020/07/17 07:41:39 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2017-2020  Robert Kiesling, rk3314042@gmail.com.
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
 *  __ctalkX11PaneRefresh is here for backward compatibility but is
 *  deprecated.
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

extern char *ascii[8193];             /* from intascii.h */

#if X11LIB_FRAME
int __ctalkX11PaneRefresh (OBJECT *self) {
  return __ctalkGUIPaneRefresh (self);
}
int __ctalkGUIPaneRefresh (OBJECT *self) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

int __xlib_refresh_window (Display *, Drawable, GC, char *);

int __ctalkX11PaneRefresh (OBJECT *self, 
			   int src_x_org, int src_y_org,
			   int src_width, int src_height,
			   int dest_x_org, int dest_y_org) {
  return __ctalkGUIPaneRefresh 
    (self, src_x_org, src_y_org, src_width, src_height,
     dest_x_org, dest_y_org);
}
int __ctalkGUIPaneRefresh (OBJECT *self, 
			   int src_x_org, int src_y_org,
			   int src_width, int src_height,
			   int dest_x_org, int dest_y_org) {
  OBJECT *self_object, *win_id_value, *gc_value, *displayptr_var;
  int panebuffer_xid, panebackingstore_xid;
  char d_buf[MAXLABEL];
  char intbuf1[MAXLABEL];
  memset (d_buf, 0, MAXLABEL);
  Display *l_d;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr", TRUE);
  __get_pane_buffers (self_object, &panebuffer_xid,
		      &panebackingstore_xid);
  if (!panebuffer_xid)
    return ERROR;

  strcatx (d_buf,
	   ctitoa (panebuffer_xid, intbuf1), ":",
	   ascii[src_x_org], ":",
	   ascii[src_y_org], ":",
	   ascii[src_width], ":",
	   ascii[src_height], ":",
	   ascii[dest_x_org], ":",
	   ascii[dest_y_org], NULL);

  l_d = (Display *)SYMVAL(displayptr_var -> instancevars -> __o_value);

  if (dialog_dpy ()) {
    __xlib_refresh_window (l_d,
			   INTVAL(win_id_value -> __o_value),
			   (GC)SYMVAL(gc_value -> __o_value), d_buf);
  } else {
    make_req (shm_mem,
	      SYMVAL(displayptr_var -> instancevars -> __o_value),
	      PANE_REFRESH_REQUEST,
	      INTVAL(win_id_value -> __o_value),
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
  }
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO    
  fprintf (stderr, "shm_mem = %s, %s, %s.\n", shm_mem, 
	   &shm_mem[SHM_GC], &shm_mem[SHM_DATA]);
#endif
  return SUCCESS;
}
#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11PaneRefresh (OBJECT *self, int src_x_org, int src_y_org,
			   int src_width, int src_height, int dest_x_org,
			   int dest_y_org) {
  x_support_error (); return ERROR;
}
int __ctalkGUIPaneRefresh (OBJECT *self, int src_x_org, int src_y_org,
			   int src_width, int src_height, int dest_x_org,
			   int dest_y_org) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
