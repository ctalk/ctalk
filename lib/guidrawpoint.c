/* $Id: guidrawpoint.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016, 2018-2019  
    Robert Kiesling, rk3314042@gmail.com.
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
 *  __ctalkX11PaneDrawPoint is here for backward compatibility but is
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
int __ctalkX11PaneDrawPoint (OBJECT *self, OBJECT *pane, OBJECT *pen) {
  return __ctalkGUIPaneDrawPoint (self, pane, pen);
}
int __ctalkGUIPaneDrawPoint (OBJECT *self, OBJECT *pane, OBJECT *pen) {
  return SUCCESS;
}
int __ctalkX11PaneDrawPointBasic (int drawable_id,
				  unsigned long int gc_ptr,
				   int x_center, int y_center,
				   int pen_width,
				   int alpha,
				   char *pen_color) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */
int __ctalkX11PaneDrawPointBasic (int drawable_id,
				  unsigned long int gc_ptr,
				  int x_center, int y_center,
				  int pen_width,
				  int alpha,
				  char *pen_color) {
  char d_buf[MAXLABEL];
  char intbuf4[MAXLABEL], intbuf5[MAXLABEL], intbuf6[MAXLABEL];

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if (!shm_mem)
    return ERROR;

  strcatx (d_buf,
	   ascii[x_center], ":",
	   ascii[y_center], ":",
	   ascii[pen_width], ":", ctitoa ((unsigned int)alpha, intbuf6),
	   ":", ctitoa ((unsigned int)drawable_id, intbuf4),
	   ":", ctitoa ((unsigned int)drawable_id, intbuf5),
	   ":", pen_color,
	   NULL);
  
  make_req (shm_mem, PANE_DRAW_POINT_REQUEST,
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

int __ctalkX11PaneDrawPoint (OBJECT *self, OBJECT *pane, OBJECT *pen) {
  return __ctalkGUIPaneDrawPoint (self, pane, pen);
}
int __ctalkGUIPaneDrawPoint (OBJECT *self, OBJECT *point, OBJECT *pen) {
  OBJECT *self_object, *point_object, *pen_object, *pen_width_object,
    *point_x_var, *point_y_var, *pen_color_object, *pen_alpha_object,
    *parentDrawable_object;
  int panebuffer_xid, panebackingstore_xid;
  OBJECT *win_id_value, *gc_value;
  char d_buf[MAXLABEL], intbuf1[MAXLABEL], intbuf2[MAXLABEL],
    intbuf3[MAXLABEL];
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if (shm_mem == NULL) {
    return ERROR;
  }

  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  parentDrawable_object = __ctalkGetInstanceVariable
    (self_object, "parentDrawable", FALSE);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  pen_object = (IS_VALUE_INSTANCE_VAR (pen) ? pen->__o_p_obj : pen);
  point_object = (IS_VALUE_INSTANCE_VAR (point) ? point->__o_p_obj : point);
  pen_width_object = 
    __ctalkGetInstanceVariable (pen_object, "width", TRUE);
  pen_color_object = 
    __ctalkGetInstanceVariable (pen_object, "colorName", TRUE);
  pen_alpha_object = 
    __ctalkGetInstanceVariable (pen_object, "alpha", TRUE);
  point_x_var = 
    __ctalkGetInstanceVariable (point_object, "x", TRUE);
  point_y_var = 
    __ctalkGetInstanceVariable (point_object, "y", TRUE);

  if (IS_OBJECT(parentDrawable_object)) {
    /* the receiver is a X11Bitmap object. */
    panebuffer_xid = panebackingstore_xid =
      atoi (win_id_value -> __o_value);
  } else {
    __get_pane_buffers (self_object, &panebuffer_xid,
			&panebackingstore_xid);
  }

  strcatx (d_buf,
	   ascii[*(int *)point_x_var->instancevars->__o_value], ":",
	   ascii[*(int *)point_y_var->instancevars->__o_value], ":",
	   ascii[*(int *)pen_width_object->instancevars->__o_value], ":",
	   ctitoa (*(int *)pen_alpha_object->instancevars->__o_value,
		   intbuf3), ":",
	   ctitoa (panebuffer_xid, intbuf1), ":",
	   ctitoa (panebackingstore_xid, intbuf2), ":",
	   pen_color_object->instancevars->__o_value,
	   NULL);
	   
  if (IS_OBJECT(parentDrawable_object)) {
    /* Again, the receiver is a X11Bitmap object. */
    make_req (shm_mem, PANE_DRAW_POINT_REQUEST,
	      INTVAL(parentDrawable_object -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);
  } else  {
    make_req (shm_mem, PANE_DRAW_POINT_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);
  }
#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = *(int *)win_id_value->__o_value;
  XSendEvent (display,
	      *(int *)win_id_value -> __o_value;
 	      False, 0L, &send_event);
#endif
  wait_req (shm_mem);
  return SUCCESS;
}
#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11PaneDrawPoint (OBJECT *self, OBJECT *pane, OBJECT *pen) {
  x_support_error (); return ERROR;
}
int __ctalkGUIPaneDrawPoint (OBJECT *self, OBJECT *pane, OBJECT *pen) {
  x_support_error (); return ERROR;
}
int __ctalkX11PaneDrawPointBasic (int drawable_id,
				  unsigned long int gc_ptr,
				   int x_center, int y_center,
				   int pen_width,
				   int alpha,
				   char *pen_color) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
