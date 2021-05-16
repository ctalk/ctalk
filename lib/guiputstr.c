/* $Id: guiputstr.c,v 1.1.1.1 2021/04/03 11:26:06 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016-2020 Robert Kiesling, rk3314042@gmail.com.
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
 *  __ctalkX11PanePutStr and __ctalkGUIPanePutStr are here for 
 *  compatibility, but they rely on hard-coded instance variable names.  
 *  Try to use __X11PanePutStrBasic in new code.
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
extern Font fixed_font;

extern char *ascii[8193];             /* from intascii.h */


#if X11LIB_FRAME
int __ctalkX11PanePutStr (OBJECT *self_object, int x, int y, char *s) {
  return SUCCESS;
}
int __ctalkGUIPanePutStr (OBJECT *self_object, int x, int y, char *s) {
  return SUCCESS;
}
int __ctalkX11PanePutStrBasic (void *d, int visual_id, unsigned long int gc_ptr,
			       int x, int y, char *s) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

#ifdef HAVE_XFT_H

int __xlib_put_str (Display *, Drawable, GC, char *);
int __xlib_put_str_ft (Display *, Drawable, GC, char *);
void sync_ft_font (bool);

int __ctalkX11PanePutStrBasic (void *d, int drawable_id, unsigned long int gc_ptr,
			       int x, int y, char *s) {
  char d_buf[MAXLABEL];
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  if (!shm_mem)
    return ERROR;
  if (__ctalkXftInitialized ()) {
    
    strcatx (d_buf, ":", (((x >= 0) && (x <= 8192)) ? ascii[x] : ascii[0]),
	     ":", (((y >= 0) && (y <= 8192)) ? ascii[y] : ascii[0]),
	     ":", s, NULL);

    sync_ft_font (false);
    if (dialog_dpy ()) {
      __xlib_put_str_ft (d, drawable_id, (GC)gc_ptr, d_buf);
      return SUCCESS;
    } else {
      make_req (shm_mem, d, PANE_PUT_STR_REQUEST_FT,
		drawable_id, gc_ptr, d_buf);
      wait_req (shm_mem);
    }

  } else { /* if (__ctalkXftInitialized ()) */

    strcatx (d_buf, ascii[x], ":", ascii[y], ":", s, NULL);

    if (dialog_dpy ()) {
      __xlib_put_str (d, drawable_id, (GC)gc_ptr, d_buf);
      return SUCCESS;
    } else {
      make_req (shm_mem, d, PANE_PUT_STR_REQUEST,
		drawable_id, gc_ptr, d_buf);
      wait_req (shm_mem);
    }
  }  /* if (__ctalkXftInitialized ()) */
#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = drawable_id;
  XSendEvent (display, drawable_id, False, 0L, &send_event);
#endif

  return SUCCESS;
}

#else /* #ifdef HAVE_XFT_H */

int __xlib_change_gc (Display *, Drawable, GC, char *);
int __xlib_put_str_ft (Display *, Drawable, GC, char *);

int __ctalkX11PanePutStrBasic (void *d, int drawable_id, unsigned long int gc_ptr,
			       int x, int y, char *s) {
  char d_buf[MAXLABEL];
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  if (!shm_mem)
    return ERROR;

  strcatx (d_buf,
	     ascii[x], ":", ascii[y], ":", s, NULL);

  make_req (shm_mem, d, PANE_PUT_STR_REQUEST,
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

#endif /* #ifdef HAVE_XFT_H */

OBJECT *__x11_pane_font_id_value_object (OBJECT *);

int __ctalkX11PanePutStr (OBJECT *self, int x, int y, char *s) {
  return __ctalkGUIPanePutStr (self, x, y, s);
}

#ifdef HAVE_XFT_H

int __ctalkGUIPanePutStr (OBJECT *self, int x, int y, char *s) {
  OBJECT *self_object, *win_id_value, *gc_value, *displayptr_var;
  int panebuffer_xid, panebackingstore_xid;
  char d_buf[MAXMSG], *pat;
  char intbuf1[MAXLABEL], intbuf2[MAXLABEL];
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr",
					       TRUE);
  if (__ctalkXftInitialized ()) {

    if ((pat = __xft_selected_pattern_internal ()) == NULL)
      return 0;

    strcatx (d_buf, 
	     pat, ":",
	     ctitoa (x, intbuf1), ":",
	     ctitoa (y, intbuf2), ":",
	     s, NULL);
	     

    __get_pane_buffers (self_object, &panebuffer_xid, 
			&panebackingstore_xid);
    if (panebuffer_xid) {

      make_req (shm_mem,
		SYMVAL(displayptr_var -> instancevars -> __o_value),
		PANE_PUT_STR_REQUEST_FT, panebuffer_xid,
		SYMVAL(gc_value -> __o_value), d_buf);

    } else {

      make_req (shm_mem,
		SYMVAL(displayptr_var -> instancevars -> __o_value),
		PANE_PUT_STR_REQUEST_FT,
		INTVAL(win_id_value -> __o_value),
		SYMVAL(gc_value -> __o_value), d_buf);
    }

  } else { /* if (__ctalkXftInitialized ()) */

    strcatx (d_buf,
	     ctitoa (x, intbuf1), ":",
	     ctitoa (y, intbuf2), ":",
	     s, NULL);

#if X11LIB_PRINT_COPIOUS_DEBUG_INFO    
    fprintf (stderr, "__ctalkX11LibPutStr: d_buf  = %s.\n", d_buf);
#endif
    __get_pane_buffers (self_object, &panebuffer_xid, 
			&panebackingstore_xid);
    if (panebuffer_xid) {

      make_req (shm_mem,
		SYMVAL(displayptr_var -> instancevars -> __o_value),
		PANE_PUT_STR_REQUEST,
		panebuffer_xid,
		*(uintptr_t *)gc_value -> __o_value, d_buf);

    } else {
      make_req (shm_mem,
		SYMVAL(displayptr_var -> instancevars -> __o_value),
		PANE_PUT_STR_REQUEST,
		*(int *)win_id_value -> __o_value,
		*(uintptr_t *)gc_value -> __o_value, d_buf);
    }

  } /* if (__ctalkXftInitialized ()) */

#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = atoi(win_id_value->__o_value);
  XSendEvent (display, atoi(win_id_value->__o_value),
 	      False, 0L, &send_event);
#endif
  wait_req (shm_mem);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO    
  fprintf (stderr, "__ctalkX11LibPutStr: shm_mem = %s, %s, %s.\n", shm_mem, 
	   &shm_mem[SHM_GC], &shm_mem[SHM_DATA]);
#endif
  return SUCCESS;
}

#else /* #ifdef HAVE_XFT_H */

int __ctalkGUIPanePutStr (OBJECT *self, int x, int y, char *s) {
  OBJECT *self_object, *win_id_value, *gc_value, *displayptr_var;
  int panebuffer_xid, panebackingstore_xid;
  char d_buf[MAXLABEL];
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr",
					       TRUE);

  strcatx (d_buf, ascii[x], ":", ascii[y], ":", s, NULL);
	   
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO    
  fprintf (stderr, "__ctalkX11LibPutStr: d_buf  = %s.\n", d_buf);
#endif
  __get_pane_buffers (self_object, &panebuffer_xid, 
		      &panebackingstore_xid);
  if (panebuffer_xid) {
    make_req (shm_mem,
	      SYMVAL(displayptr_var -> instancevars -> __o_value),
	      PANE_PUT_STR_REQUEST, panebuffer_xid,
	      SYMVAL(gc_value -> __o_value), d_buf);
  } else {
    make_req (shm_mem,
	      SYMVAL(displayptr_var -> instancevars -> __o_value),
	      PANE_PUT_STR_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);
  }
#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = atoi(win_id_value->__o_value);
  XSendEvent (display, atoi(win_id_value->__o_value),
 	      False, 0L, &send_event);
#endif
  wait_req (shm_mem);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO    
  fprintf (stderr, "__ctalkX11LibPutStr: shm_mem = %s, %s, %s.\n", shm_mem, 
	   &shm_mem[SHM_GC], &shm_mem[SHM_DATA]);
#endif
  return SUCCESS;
}

#endif /* #ifdef HAVE_XFT_H */

#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11PanePutStr (OBJECT *self_object, int x, int y, char *s) {
  x_support_error ();
  return ERROR;  /* notreached */
}
int __ctalkGUIPanePutStr (OBJECT *self_object, int x, int y, char *s) {
  x_support_error (); return ERROR;
}
int __ctalkX11PanePutStrBasic (void *d, int visual_id, unsigned long int gc_ptr,
			       int x, int y, char *s) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
