/* $Id: xresource.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014 - 2018  Robert Kiesling, rk3314042@gmail.com.
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
int __ctalkX11SetResource (int drawable_id, 
			   char *resource_name,
			   char *resource_class) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

int __ctalkX11SetResource (int drawable_id, 
			   char *resource_name,
			   char *resource_class) {
  char d_buf[MAXLABEL], gc_buf[MAXLABEL]; 
  int i;
  char *h, handle_buf[FILENAME_MAX];
  int r;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  XClassHint class_hints;

  class_hints.res_name = strdup (resource_name);
  class_hints.res_class = strdup (resource_class);

  XSetClassHint (display, (Window)drawable_id, &class_hints);

  XFlush (display);

  __xfree (MEMADDR(class_hints.res_name));
  __xfree (MEMADDR(class_hints.res_class));

  if (!shm_mem)
    return ERROR;

  /* The GC is not used in this call. */
  strcatx (d_buf, ":", resource_name, ":", resource_class, NULL);
  make_req (shm_mem, PANE_SET_RESOURCE_REQUEST,
   	    drawable_id, 65535, d_buf);
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
#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11SetResource (int drawable_id, 
			   char *resource_name,
			   char *resource_class) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
