/* $Id: guidrawrectangle.c,v 1.13 2020/03/26 02:58:39 rkiesling Exp $ -*-c-*-*/

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
 *  __ctalkX11PaneDrawRectangle is here for backward compatibility but is
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
extern DIALOG_C *dpyrec;

extern char *shm_mem;
extern int mem_id;

extern char *ascii[8193];             /* from intascii.h */

int __xlib_draw_rectangle (Display *, Drawable, GC, char *);


#if X11LIB_FRAME
int __ctalkX11PaneDrawRectangle (OBJECT *self, OBJECT *pane, OBJECT *pen,
				 int fill) {
  return SUCCESS;
}
int __ctalkX11PaneDrawRoundedRectangle (OBJECT *self, OBJECT *pane, OBJECT *pen,
					int fill, in corner_radius) {
  return SUCCESS;
}
int __ctalkGUIPaneDrawRectangle (OBJECT *self, OBJECT *pane, OBJECT *pen,
				 int fill) {
  return SUCCESS;
}
int __ctalkGUIPaneDrawRoundedRectangle (OBJECT *self, OBJECT *pane, OBJECT *pen,
					int fill, int corner_radius) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

int __ctalkX11PaneDrawRectangle (OBJECT *self, OBJECT *rectangle, OBJECT *pen,
				 int fill) {
  return __ctalkGUIPaneDrawRectangle (self, rectangle, pen, fill);
}
int __ctalkX11PaneDrawRoundedRectangle (OBJECT *self, OBJECT *rectangle,
					OBJECT *pen,
					int fill,
					int corner_radius) {
  return __ctalkGUIPaneDrawRoundedRectangle (self, rectangle, pen,
					     fill, corner_radius);
}

int __ctalkGUIPaneDrawRectangle (OBJECT *self, OBJECT *rectangle, OBJECT *pen,
				 int fill) {
  OBJECT *self_object, *rectangle_object, *pen_object; 
  OBJECT *rect_top, *rect_top_start, *rect_top_start_x, *rect_top_start_y;
  OBJECT *rect_right, *rect_right_end, *rect_right_end_x, *rect_right_end_y;
  OBJECT *pen_width_object, *pen_color_object;
  OBJECT *displayptr_var;
  int panebuffer_xid, panebackingstore_xid;
  char d_buf[MAXLABEL];
  char intbuf1[MAXLABEL], intbuf2[MAXLABEL], intbuf3[MAXLABEL];
  Display *l_d;
  OBJECT *win_id_value, *gc_value;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  pen_object = (IS_VALUE_INSTANCE_VAR (pen) ? pen->__o_p_obj : pen);
  rectangle_object = (IS_VALUE_INSTANCE_VAR (rectangle) ? 
		      rectangle->__o_p_obj : rectangle);


  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr", TRUE);
  l_d = (Display *)SYMVAL(displayptr_var -> instancevars -> __o_value);

  pen_width_object = 
    __ctalkGetInstanceVariable (pen_object, "width", TRUE);
  if ((pen_color_object = 
       __ctalkGetInstanceVariable (pen_object, "colorName", TRUE)) == NULL)
    return ERROR;
  rect_top = 
    __ctalkGetInstanceVariable (rectangle_object, "top", TRUE);
  rect_top_start = 
    __ctalkGetInstanceVariable (rect_top, "start", TRUE);
  rect_top_start_x = 
    __ctalkGetInstanceVariable (rect_top_start, "x", TRUE);
  rect_top_start_y = 
    __ctalkGetInstanceVariable (rect_top_start, "y", TRUE);
  rect_right = 
    __ctalkGetInstanceVariable (rectangle_object, "right", TRUE);
  rect_right_end = 
    __ctalkGetInstanceVariable (rect_right, "end", TRUE);
  rect_right_end_x = 
    __ctalkGetInstanceVariable (rect_right_end, "x", TRUE);
  rect_right_end_y = 
    __ctalkGetInstanceVariable (rect_right_end, "y", TRUE);

  /* panebackingstore_id is not used */
  __get_pane_buffers (self_object, 
		      &panebuffer_xid,
		      &panebackingstore_xid);

  strcatx (d_buf,
	   ascii[(*(unsigned int *)rect_top_start_x->instancevars->
		  __o_value < 8192) ?
		 *(unsigned int *)rect_top_start_x -> instancevars
		 -> __o_value : 0], ":",
	   ascii[(*(unsigned int *)rect_top_start_y->instancevars->
		  __o_value < 8192) ?
		 *(unsigned int *) rect_top_start_y -> instancevars
		 -> __o_value: 0], ":",
	   ascii[(*(unsigned int *)rect_right_end_x->instancevars
		  ->__o_value < 8192) ?
		 *(unsigned int *)rect_right_end_x -> instancevars
		 -> __o_value : 0], ":",
	   ascii[(*(unsigned int *)rect_right_end_y->instancevars
		  ->__o_value < 8192) ?
		 *(unsigned int *)rect_right_end_y -> instancevars
		 -> __o_value: 0], ":",
	   ascii[(*(unsigned int *)pen_width_object->instancevars->
		  __o_value < 8192) ?
		 *(unsigned int *)pen_width_object -> instancevars ->
		 __o_value : 0], ":",
	   ascii[fill], ":",
	   ctitoa (panebuffer_xid, intbuf2), ":",
	   ascii[0], ":",   /* radius */
	   pen_color_object->instancevars->__o_value,
	   NULL); /***/
	   
  if (DIALOG(l_d)) {
    __xlib_draw_rectangle (l_d, INTVAL(win_id_value -> __o_value),
			   (GC)SYMVAL(gc_value -> __o_value), d_buf);
  } else {

    make_req (shm_mem,
	      SYMVAL(displayptr_var -> instancevars->__o_value),
	      PANE_DRAW_RECTANGLE_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);

#ifdef GRAPHICS_WRITE_SEND_EVENT
    send_event.xgraphicsexpose.type = GraphicsExpose;
    send_event.xgraphicsexpose.send_event = True;
    send_event.xgraphicsexpose.display = display;
    send_event.xgraphicsexpose.drawable = *(int *)win_id_value -> __o_value;
    XSendEvent (display,
		*(int *)win_id_value -> __o_value,
		False, 0L, &send_event);
#endif
    wait_req (shm_mem);
  }
  return SUCCESS;
}

int __ctalkGUIPaneDrawRoundedRectangle (OBJECT *self, OBJECT *rectangle,
					OBJECT *pen,
				 int fill, int corner_radius) {
  OBJECT *self_object, *rectangle_object, *pen_object; 
  OBJECT *rect_top, *rect_top_start, *rect_top_start_x, *rect_top_start_y;
  OBJECT *rect_right, *rect_right_end, *rect_right_end_x, *rect_right_end_y;
  OBJECT *pen_width_object, *pen_color_object;
  int panebuffer_xid, panebackingstore_xid;
  char d_buf[MAXLABEL];
  char intbuf1[MAXLABEL], intbuf2[MAXLABEL], intbuf3[MAXLABEL];
  OBJECT *win_id_value, *gc_value, *displayptr_var;
  Display *l_d;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  pen_object = (IS_VALUE_INSTANCE_VAR (pen) ? pen->__o_p_obj : pen);
  rectangle_object = (IS_VALUE_INSTANCE_VAR (rectangle) ? 
		      rectangle->__o_p_obj : rectangle);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr",
					       TRUE);
  l_d = (Display *)SYMVAL(displayptr_var -> instancevars -> __o_value);


  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);

  pen_width_object = 
    __ctalkGetInstanceVariable (pen_object, "width", TRUE);
  if ((pen_color_object = 
       __ctalkGetInstanceVariable (pen_object, "colorName", TRUE)) == NULL)
    return ERROR;
  rect_top = 
    __ctalkGetInstanceVariable (rectangle_object, "top", TRUE);
  rect_top_start = 
    __ctalkGetInstanceVariable (rect_top, "start", TRUE);
  rect_top_start_x = 
    __ctalkGetInstanceVariable (rect_top_start, "x", TRUE);
  rect_top_start_y = 
    __ctalkGetInstanceVariable (rect_top_start, "y", TRUE);
  rect_right = 
    __ctalkGetInstanceVariable (rectangle_object, "right", TRUE);
  rect_right_end = 
    __ctalkGetInstanceVariable (rect_right, "end", TRUE);
  rect_right_end_x = 
    __ctalkGetInstanceVariable (rect_right_end, "x", TRUE);
  rect_right_end_y = 
    __ctalkGetInstanceVariable (rect_right_end, "y", TRUE);

  /* panebackingstore_id is not used */
  __get_pane_buffers (self_object, 
		      &panebuffer_xid,
		      &panebackingstore_xid);

  strcatx (d_buf,
	   ascii[(*(unsigned int *)rect_top_start_x->instancevars->
		  __o_value < 8192) ?
		 *(unsigned int *)rect_top_start_x -> instancevars
		 -> __o_value : 0], ":",
	   ascii[(*(unsigned int *)rect_top_start_y->instancevars->
		  __o_value < 8192) ?
		 *(unsigned int *) rect_top_start_y -> instancevars
		 -> __o_value: 0], ":",
	   ascii[(*(unsigned int *)rect_right_end_x->instancevars
		  ->__o_value < 8192) ?
		 *(unsigned int *)rect_right_end_x -> instancevars
		 -> __o_value : 0], ":",
	   ascii[(*(unsigned int *)rect_right_end_y->instancevars
		  ->__o_value < 8192) ?
		 *(unsigned int *)rect_right_end_y -> instancevars
		 -> __o_value: 0], ":",
	   ascii[(*(unsigned int *)pen_width_object->instancevars->
		  __o_value < 8192) ?
		 *(unsigned int *)pen_width_object -> instancevars ->
		 __o_value : 0], ":",
	   ascii[fill], ":",
	   ctitoa (panebuffer_xid, intbuf2), ":",
	   ascii[corner_radius], ":",
	   pen_color_object->instancevars->__o_value,
	   NULL); /***/
	   
  if (DIALOG(l_d)) {
    __xlib_draw_rectangle (l_d, INTVAL(win_id_value -> __o_value),
			   (GC)SYMVAL(gc_value -> __o_value), d_buf);
  } else {
    make_req (shm_mem,
	      SYMVAL(displayptr_var -> instancevars -> __o_value),
	      PANE_DRAW_RECTANGLE_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);

#ifdef GRAPHICS_WRITE_SEND_EVENT
    send_event.xgraphicsexpose.type = GraphicsExpose;
    send_event.xgraphicsexpose.send_event = True;
    send_event.xgraphicsexpose.display = display;
    send_event.xgraphicsexpose.drawable = *(int *)win_id_value -> __o_value;
    XSendEvent (display,
		*(int *)win_id_value -> __o_value,
		False, 0L, &send_event);
#endif
    wait_req (shm_mem);
  }
  return SUCCESS;
}

#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11PaneDrawRectangle (OBJECT *self, OBJECT *pane, OBJECT *pen,
				 int fill) {
  x_support_error (); return ERROR;
}

int __ctalkX11PaneDrawRoundedRectangle (OBJECT *self, OBJECT *pane, OBJECT *pen,
					int fill, int corner_radius) {
  x_support_error (); return ERROR;
}

int __ctalkGUIPaneDrawRectangle (OBJECT *self, OBJECT *pane, OBJECT *pen,
				 int fill) {
  x_support_error (); return ERROR;
}

int __ctalkGUIPaneDrawRoundedRectangle (OBJECT *self, OBJECT *pane, OBJECT *pen,
					int fill, int corner_radius) {
  x_support_error (); return ERROR;
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
