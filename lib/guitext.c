/* $Id: guitext.c,v 1.4 2020/02/29 02:54:05 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014-2019  Robert Kiesling, rk3314042@gmail.com.
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
/* Nothing for now. */
#else /* X11LIB_FRAME */

/* Returns a string containing the name of the file or whatever
   handle transfers the data to the server. */
static char *__write_text_data (char *data) {
  int width, height, n_colors, colors_per_pixel;
  int r, data_length;
  int i;
  static char handle_path[FILENAME_MAX];
  char intbuf[MAXLABEL];
  FILE *f;

  strcatx (handle_path, P_tmpdir, "/text", ctitoa (getpid (), intbuf),
	   NULL);

  if ((f = fopen (handle_path, "w")) == NULL) {
    fprintf (stderr, "__write_text_to_data (fopen): %s: %s.\n",
	     handle_path, strerror (errno));
    return NULL;
  }

  data_length = strlen (data);
  if ((r = fwrite (data, sizeof (char), data_length, f)) != data_length) {
    fprintf (stderr, "__write_text_to_data (fwrite): %s: %s.\n",
	     handle_path, strerror (errno));
  }

  if ((r = fclose (f)) != 0) {
    fprintf (stderr, "__write_text_to_data (fclose): %s: %s.\n",
	     handle_path, strerror (errno));
  }

  return handle_path;
}

int __ctalkX11TextFromData (void *d, int drawable_id,
			    unsigned long int gc_ptr, char *data) {
  char *h;
  int r;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if ((h = __write_text_data (data)) == NULL)
    return ERROR;

  if (!shm_mem)
    return ERROR;
  memset ((void *)shm_mem, 0, SHM_BLKSIZE);
  make_req (shm_mem, (uintptr_t)d, PANE_TEXT_FROM_DATA_REQUEST,
   	    drawable_id, gc_ptr, h);
  
#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = drawable_id;
  XSendEvent (display, drawable_id, False, 0L, &send_event);
#endif
  wait_req (shm_mem);

  if ((r = unlink (h)) != 0) {
    fprintf (stderr, "__ctalkX11TextFromData (unlink): %s.\n",
	     strerror (errno));
  }

  return SUCCESS;
}

#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11TextFromData (void *d, int drawable_id,
			    unsigned long int gc_ptr, 
			   char *data) {
  x_support_error (); return ERROR;
}
void *__ctalkX11GetPrimarySelection (void *data_in_out,
				     int *size_in_out) {
  x_support_error (); return NULL;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
