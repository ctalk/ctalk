/* $Id: guidrawline.c,v 1.3 2020/06/21 17:46:39 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2017-2020
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

char intbuf[MAXARGS][MAXLABEL] = {'\0',};

extern Display *display;   /* Defined in x11lib.c. */

extern char *shm_mem;
extern int mem_id;
extern char *ascii[8193];


extern int __xlib_draw_line (Display *, Drawable, GC, char *);

#if X11LIB_FRAME
int __ctalkX11PaneDrawLine (OBJECT *self, OBJECT *line, OBJECT *pen) {
  return SUCCESS;
}
int __ctalkGUIPaneDrawLine (OBJECT *self, OBJECT *line, OBJECT *pen) {
  return SUCCESS;
}
int __ctalkX11PaneDrawLineBasic (void *d, int drawable_id,
				 unsigned long int gc_ptr,
				 int x_start, int y_start,
				 int x_end, int y_end,
				 int pen_width,
				 int alpha,
				 char *pen_color) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

int __ctalkGUIPaneDrawLineBasic (void *d, int drawable_id,
				 unsigned long int gc_ptr,
				 int x_start, int y_start,
				 int x_end, int y_end,
				 int pen_width,
				 int alpha,
				 char *pen_color) {
  return __ctalkX11PaneDrawLineBasic (d, drawable_id, gc_ptr,
				      x_start, y_start, x_end, y_end,
				      pen_width, alpha, pen_color);
}
int __ctalkX11PaneDrawLineBasic (void *d, int drawable_id,
				 unsigned long int gc_ptr,
				 int x_start, int y_start,
				 int x_end, int y_end,
				 int pen_width,
				 int alpha,
				 char *pen_color) {
  char d_buf[MAXLABEL];
  char intbuf1[MAXLABEL], intbuf2[MAXLABEL], intbuf3[MAXLABEL];

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if (!shm_mem)
    return ERROR;

  strcatx (d_buf,
	   ascii[x_start], ":", ascii[y_start], ":", ascii[x_end],
	   ":", ascii[y_end], ":", ascii[pen_width],
	   ":", ctitoa ((unsigned int)alpha, intbuf1),
	   ":", ctitoa ((unsigned int)drawable_id, intbuf2),
	   ":", ctitoa ((unsigned int)drawable_id, intbuf3),
	   ":", pen_color,
	   NULL);
  
  if (dialog_dpy ()) {
    __xlib_draw_line (d, drawable_id, (GC)gc_ptr, d_buf);
  } else {

    make_req (shm_mem, d, PANE_DRAW_LINE_REQUEST,
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

int __ctalkX11PaneDrawLine (OBJECT *self, OBJECT *line, OBJECT *pen) {
  return __ctalkGUIPaneDrawLine (self, line, pen);
}

int __ctalkGUIPaneDrawLine (OBJECT *self, OBJECT *line, OBJECT *pen) {
  OBJECT *self_object, *pen_object; 
  OBJECT *line_start_var, *line_start_x_var, *line_start_y_var;
  OBJECT *line_end_var, *line_end_x_var, *line_end_y_var;
  OBJECT *pen_width_object, *pen_color_object, *pen_alpha_object; 
  OBJECT *win_id_value, *gc_value;
  OBJECT *line_object, *displayPtr_var;
  int start_x, start_y, end_x, end_y;
  char d_buf[MAXMSG];
  int panebuffer_xid, panebackingstore_xid;
  Display *l_d;

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  line_object = (IS_VALUE_INSTANCE_VAR (line) ? line->__o_p_obj : line);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  pen_object = (IS_VALUE_INSTANCE_VAR (pen) ? pen->__o_p_obj : pen);
  pen_width_object = 
      __ctalkGetInstanceVariable (pen_object, "width", TRUE);
  pen_color_object = 
    __ctalkGetInstanceVariable (pen_object, "colorName", TRUE);
  pen_alpha_object = 
    __ctalkGetInstanceVariable (pen_object, "alpha", TRUE);
  line_start_var = 
    __ctalkGetInstanceVariable (line_object, "start", TRUE);
  line_start_x_var = 
    __ctalkGetInstanceVariable (line_start_var, "x", TRUE);
  line_start_y_var = 
    __ctalkGetInstanceVariable (line_start_var, "y", TRUE);
  line_end_var = 
    __ctalkGetInstanceVariable (line_object, "end", TRUE);
  line_end_x_var = 
    __ctalkGetInstanceVariable (line_end_var, "x", TRUE);
  line_end_y_var = 
    __ctalkGetInstanceVariable (line_end_var, "y", TRUE);
  __get_pane_buffers (self_object, &panebuffer_xid,
		      &panebackingstore_xid);
  displayPtr_var = __ctalkGetInstanceVariable (self_object, "displayPtr",
					       TRUE);
  l_d = (Display *)SYMVAL(displayPtr_var -> instancevars -> __o_value);

  start_x = *(int *)line_start_x_var->instancevars -> __o_value;
  start_y = *(int *)line_start_y_var->instancevars -> __o_value;
  end_x = *(int *)line_end_x_var->instancevars->__o_value;
  end_y = *(int *)line_end_y_var->instancevars->__o_value;

  strcatx (d_buf,
	   ((start_x >= 0) ? ascii[start_x] : ctitoa (start_x, intbuf[0])), ":",
	   ((start_y >= 0) ? ascii[start_y] : ctitoa (start_y, intbuf[1])), ":",
	   ((end_x >= 0) ? ascii[end_x] : ctitoa (end_x, intbuf[2])), ":",
	   ((end_y >= 0) ? ascii[end_y] : ctitoa (end_y, intbuf[3])), ":",
	   ascii[*(int *)pen_width_object->instancevars->__o_value], ":",
	   ctitoa (*(int *)pen_alpha_object->instancevars->__o_value,
		   intbuf[4]), ":",
	   ctitoa (panebuffer_xid, intbuf[5]), ":",
	   ctitoa (panebackingstore_xid, intbuf[6]), ":",
	   pen_color_object->instancevars->__o_value,
	   NULL);

  if (dialog_dpy ()) {
    __xlib_draw_line (l_d, INTVAL(win_id_value -> __o_value),
		      (GC)SYMVAL(gc_value -> __o_value),
		      d_buf);
  } else {

    make_req (shm_mem,
	      SYMVAL(displayPtr_var -> instancevars -> __o_value),
	      PANE_DRAW_LINE_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);

#ifdef GRAPHICS_WRITE_SEND_EVENT
    send_event.xgraphicsexpose.type = GraphicsExpose;
    send_event.xgraphicsexpose.send_event = True;
    send_event.xgraphicsexpose.display = display;
    send_event.xgraphicsexpose.drawable = *(int *)win_id_value->__o_value;
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
static void __gui_support_error (void) {
  x_support_error ();
}
int __ctalkX11PaneDrawLine (OBJECT *self, OBJECT *line, OBJECT *pen) {
  x_support_error (); return ERROR;
}
int __ctalkGUIPaneDrawLine (OBJECT *self, OBJECT *line, OBJECT *pen) {
  x_support_error (); return ERROR;
}
int __ctalkX11PaneDrawLineBasic (void *d, int drawable_id,
				 unsigned long int gc_ptr,
				 int x_start, int y_start,
				 int x_end, int y_end,
				 int pen_width,
				 int alpha,
				 char *pen_color) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
