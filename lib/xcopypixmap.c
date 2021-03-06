/* $Id: xcopypixmap.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014 - 2019  Robert Kiesling, rk3314042@gmail.com.
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
extern char *shm_mem;
extern int mem_id;

#if X11LIB_FRAME
int __ctalkX11CopyPixmapBasic (int dest_drawable_id,
			       unsigned long int dest_gc_ptr,
			       int src_drawable_id, 
			       int src_x_org, int src_y_org,
			       int src_width, int src_height,
			       int dest_x_org, int dest_y_org) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

int __ctalkX11CopyPixmapBasic (int dest_drawable_id,
			       unsigned long int dest_gc_ptr,
			       int src_drawable_id, 
			       int src_x_org, int src_y_org,
			       int src_width, int src_height,
			       int dest_x_org, int dest_y_org) {

  char d_buf[MAXLABEL];
  char intbuf1[MAXLABEL], intbuf2[MAXLABEL], intbuf3[MAXLABEL],
    intbuf4[MAXLABEL], intbuf5[MAXLABEL], intbuf6[MAXLABEL],
  intbuf7[MAXLABEL];

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if (!shm_mem)
    return ERROR;

  strcatx (d_buf, ctitoa ((unsigned int)src_drawable_id, intbuf1),
	   ":", ctitoa ((unsigned int)src_x_org, intbuf2),
	   ":", ctitoa ((unsigned int)src_y_org, intbuf3),
	   ":", ctitoa ((unsigned int)src_width, intbuf4),
	   ":", ctitoa ((unsigned int)src_height, intbuf5),
	   ":", ctitoa ((unsigned int)dest_x_org, intbuf6),
	   ":", ctitoa ((unsigned int)dest_y_org, intbuf7), 
	   ":", NULL);

  make_req (shm_mem, PANE_COPY_PIXMAP_REQUEST,
   	    dest_drawable_id, dest_gc_ptr, d_buf);
#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = dest_drawable_id;
  XSendEvent (display, dest_drawable_id, False, 0L, &send_event);
#endif
  wait_req (shm_mem);

  return SUCCESS;
}
#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
static void __gui_support_error (void) {
  _error ("__ctalkGUI ... () functions require Graphical User Interface support.\n");
}
int __ctalkX11CopyPixmapBasic (int dest_drawable_id,
			       unsigned long int dest_gc_ptr,
			       int src_drawable_id, 
			       int src_x_org, int src_y_org,
			       int src_width, int src_height,
			       int dest_x_org, int dest_y_org) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
