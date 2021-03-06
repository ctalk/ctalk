/* $Id: guiclearrectangle.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright � 2005-2012, 2015-2019  Robert Kiesling, rk3314042@gmail.com.
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
 *  __ctalkX11 ... are here for backward compatibility but are
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
int __ctalkX11PaneClearRectangle (OBJECT *self_object, int x, int y,
				   int width, int height) {
  return SUCCESS;
}
int __ctalkGUIPaneClearRectangle (OBJECT *self_object, int x, int y,
				   int width, int height) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

int __ctalkX11ClearRectangleBasic (int drawable_id,
				   unsigned long int gc_ptr, 
				   int x, int y,
				   int width, int height) {
  char d_buf[MAXLABEL];
  char cr_intbuf1[MAXLABEL], cr_intbuf2[MAXLABEL], cr_intbuf3[MAXLABEL],
    cr_intbuf4[MAXLABEL], cr_intbuf5[MAXLABEL], cr_intbuf6[MAXLABEL];

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  strcatx (d_buf, 
	   ascii[x], ":",
	   ascii[y], ":",
	   ascii[width], ":",
	   ascii[height], ":",
	   ctitoa (drawable_id, cr_intbuf5), ":",
	   ctitoa (drawable_id, cr_intbuf6), ":",
	   NULL);
  make_req (shm_mem, PANE_CLEAR_RECTANGLE_REQUEST,
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

int __ctalkX11PaneClearRectangle (OBJECT *self, int x, int y,
				   int width, int height) {
  return __ctalkGUIPaneClearRectangle (self, x, y, width, height);
}

int __ctalkGUIPaneClearRectangle (OBJECT *self, int x, int y,
				   int width, int height) {

  OBJECT *self_object, *win_id_value, *gc_value;
  int panebuffer_xid, panebackingstore_xid;
  char d_buf[MAXLABEL];
  char cr_intbuf1[MAXLABEL], cr_intbuf2[MAXLABEL], cr_intbuf3[MAXLABEL],
    cr_intbuf4[MAXLABEL], cr_intbuf5[MAXLABEL], cr_intbuf6[MAXLABEL];

  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  __get_pane_buffers (self_object, &panebuffer_xid,
		      &panebackingstore_xid);
  if (panebuffer_xid && panebackingstore_xid) {

    strcatx (d_buf,
	     ctitoa (x, cr_intbuf1), ":",
	     ctitoa (y, cr_intbuf2), ":",
	     ctitoa (width, cr_intbuf3), ":",
	     ctitoa (height, cr_intbuf4), ":",
	     ctitoa (panebuffer_xid, cr_intbuf5), ":",
	     ctitoa (panebackingstore_xid, cr_intbuf6), ":",
	     NULL);
    make_req (shm_mem, PANE_CLEAR_RECTANGLE_REQUEST,
	      panebuffer_xid,
	      SYMVAL(gc_value -> __o_value), d_buf);
  } else {
    strcatx (d_buf,
	     ctitoa (x, cr_intbuf1), ":",
	     ctitoa (y, cr_intbuf2), ":",
	     ctitoa (width, cr_intbuf3), ":",
	     ctitoa (height, cr_intbuf4), ":0:0:",
	     NULL);
    make_req (shm_mem, PANE_CLEAR_RECTANGLE_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);
  }
  wait_req (shm_mem);
  return SUCCESS;
}

#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
static void __gui_support_error (void) {
  x_support_error ();
}
int __ctalkX11PaneClearRectangle (OBJECT *self_object, int x, int y,
				  int width, int height) {
  x_support_error (); return ERROR;
}
int __ctalkGUIPaneClearRectangle (OBJECT *self_object, int x, int y,
				  int width, int height) {
  x_support_error (); return ERROR;
}
int __ctalkX11ClearRectangleBasic (int drawable_id,
				   unsigned long int gc_ptr, 
				   int x, int y,
				   int width, int height) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
