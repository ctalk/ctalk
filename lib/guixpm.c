/* $Id: guixpm.c,v 1.3 2020/06/21 22:37:30 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014-2020  Robert Kiesling, rk3314042@gmail.com.
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
#define SHM_BLKSIZE 10240  /* If changing, also change in x11lib.c, etc... */

extern Display *display;   /* Defined in x11lib.c. */
extern char *shm_mem;
extern int mem_id;

#if X11LIB_FRAME
int __ctalkX11XPMFromData (void *d, int drawable_id, 
			   unsigned long int gc_ptr, 
			   int x_org, int y_org,
			   char **data) {
  return SUCCESS;
}

int __ctalkX11XPMInfo (void *d, char **data,
		       int *width_ret,
		       int *height_ret,
		       int *n_colors_ret,
		       int *chars_per_per_color_ret) {
  return SUCCESS;
}

#else /* X11LIB_FRAME */

/* Returns a string containing the name of the file or whatever
   handle transfers the data to the server. */
static char *__write_xpm_data (char **data) {
  int width, height, n_colors, colors_per_pixel;
  int r;
  int i;
  static char handle_path[FILENAME_MAX];
  char intbuf[MAXLABEL];
  FILE *f;

  strcatx (handle_path, P_tmpdir, "/xpm", ctitoa (getpid (), intbuf),
	   NULL);

  if ((f = fopen (handle_path, "w")) == NULL) {
    fprintf (stderr, "__write_xpm_to_data (fopen): %s: %s.\n",
	     handle_path, strerror (errno));
    return NULL;
  }

  if ((r = sscanf (data[0], "%d %d %d %d", 
		   &width, &height, &n_colors, &colors_per_pixel))
      != 4) {
    fprintf (stderr, "__write_xpm_to_data: bad data conversion: %s, r = %d\n",
	     data[0], r);
    return NULL;
  }

  for (i = 0; i <= height + n_colors; i++) {
    fputs (data[i], f);
    fputs ("\n", f);
  }

  if ((r = fclose (f)) != 0) {
    fprintf (stderr, "__write_xpm_to_data (fclose): %s: %s.\n",
	     handle_path, strerror (errno));
  }

  return handle_path;
}

int __xlib_xpm_from_data (Display *, Drawable, GC, char *);

int __ctalkX11XPMFromData (void *d, int drawable_id, 
			   unsigned long int gc_ptr, 
			   int x_org, int y_org,
			   char **data) {
  char d_buf[MAXLABEL];
  char *h;
  char intbuf1[MAXLABEL], intbuf2[MAXLABEL];
  int r;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if ((h = __write_xpm_data (data)) == NULL)
    return ERROR;

  if (!shm_mem)
    return ERROR;
  memset ((void *)shm_mem, 0, SHM_BLKSIZE);
  strcatx (d_buf,
	   ":", ctitoa (x_org, intbuf1),
	   ":", ctitoa (y_org, intbuf2),
	   ":", h, NULL);

  if (dialog_dpy ()) {
      __xlib_xpm_from_data (d, drawable_id, (GC)gc_ptr, d_buf);
    } else {
      make_req (shm_mem, d, PANE_XPM_FROM_DATA_REQUEST,
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

#if 0
  if ((r = unlink (h)) != 0) {
    fprintf (stderr, "__ctalkX11XPMFromData (unlink): %s.\n",
	     strerror (errno));
  }
#endif  

  return SUCCESS;
}

int __ctalkX11XPMInfo (void *d, char **data,
		       int *width_ret,
		       int *height_ret,
		       int *n_colors_ret,
		       int *chars_per_color_ret) {

  int r;

  if ((r = sscanf (data[0], "%d %d %d %d", 
		   width_ret, height_ret, n_colors_ret, chars_per_color_ret))
      != 4) {
    fprintf (stderr, "__ctalkX11XPMInfo: %s, r = %d\n",
	     data[0], r);
    return ERROR;
  }

  return SUCCESS;
}


#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11XPMFromData (void *d, int drawable_id,
			   unsigned long int gc_ptr, 
			   int x_org, int y_org,
			   char **data) {
  x_support_error (); return ERROR;
}
int __ctalkX11XPMInfo (void *d, char **data,
		       int *width_ret,
		       int *height_ret,
		       int *n_colors_ret,
		       int *chars_per_per_color_ret) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
