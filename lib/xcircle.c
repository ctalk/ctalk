/* $Id: xcircle.c,v 1.7 2020/04/04 16:49:43 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014-2016, 2018-2020  Robert Kiesling, rk3314042@gmail.com.
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
extern DIALOG_C *dpyrec;   /* Declared in xdialog.c. */
extern char *shm_mem;
extern int mem_id;

extern char *ascii[8193];             /* from intascii.h */

#if X11LIB_FRAME

int __ctalkX11PaneDrawCircleBasic (void *d, int drawable_id,
				   unsigned long int gc_ptr,
				   int x_center, int y_center,
				   int radius,
				   int fill,
				   int pen_width,
				   int alpha,
				   char *pen_color,
				   char *bg_color) {
  return SUCCESS;
}
int __ctalkGUIPaneDrawCircleBasic (void *d, int drawable_id,
				   unsigned long int gc_ptr,
				   int x_center, int y_center,
				   int radius,
				   int fill,
				   int pen_width,
				   int alpha,
				   char *pen_color,
				   char *bg_color) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

int __xlib_draw_circle (Display *d, Drawable, GC, char *);

int __ctalkX11PaneDrawCircleBasic (void *d, int drawable_id,
				   unsigned long int gc_ptr,
				   int x_center, int y_center,
				   int radius,
				   int fill, int pen_width,
				   int alpha,
				   char *pen_color,
				   char *bg_color) {
  char d_buf[MAXLABEL];
  char alphabuf[MAXLABEL];

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if (!shm_mem)
    return ERROR;

  strcatx (d_buf, ascii[x_center],
	   ":", ascii[y_center],
	   ":", ascii[radius],
	   ":", ascii[fill],
	   ":", ascii[pen_width],
	   ":", ctitoa ((unsigned int)alpha, alphabuf),
	   ":", pen_color, ":", bg_color, NULL);

  if (DIALOG(d)) {
    __xlib_draw_circle (d, drawable_id, (GC)gc_ptr, d_buf);
  } else {
    make_req (shm_mem, d, PANE_DRAW_CIRCLE_REQUEST,
	      drawable_id, gc_ptr, d_buf);
  }

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
int __ctalkGUIPaneDrawCircleBasic (void *d, int drawable_id,
				   unsigned long int gc_ptr,
				   int x_center, int y_center,
				   int radius,
				   int fill,
				   int pen_width,
				   int alpha,
				   char *color,
				   char *bg_color) {
  
  return __ctalkX11PaneDrawCircleBasic (d, drawable_id, gc_ptr,
					x_center, y_center,
					radius,
					fill,
					pen_width,
					alpha,
					color,
					bg_color);
}

#endif /* X11LIB_FRAME */

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

int __ctalkX11PaneDrawCircleBasic (void *d, int drawable_id,
				   unsigned long int gc_ptr,
				   int x_center, int y_center,
				   int radius,
				   int fill,
				   int pen_width,
				   int alpha,
				   char *pen_color, char *fill_color) {  
  x_support_error (); return ERROR;
}
int __ctalkGUIPaneDrawCircleBasic (void *d, int drawable_id,
				   unsigned long int gc_ptr,
				   int x_center, int y_center,
				   int radius,
				   int fill,
				   int pen_width,
				   int alpha, 
				   char *pen_color,
				   char *fill_color) {
  x_support_error (); return ERROR;
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
