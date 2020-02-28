/* $Id: guiputxstr.c,v 1.2 2020/02/28 22:39:04 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014, 2016-2019  Robert Kiesling, rk3314042@gmail.com.
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
 *  Requires a X11FreeTypeFont object as the selected font.
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

#if X11LIB_FRAME
int __ctalkX11PanePutTransformedStr (OBJECT *self_object, int x, int y, char *s) {
  return SUCCESS;
}
int __ctalkGUIPanePutTransformedStr (OBJECT *self_object, int x, int y, char *s) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

OBJECT *__x11_pane_font_id_value_object (OBJECT *);

int __ctalkX11PanePutTransformedStr (OBJECT *self, int x, int y, char *s) {
  return __ctalkGUIPanePutTransformedStr (self, x, y, s);
}

#ifdef HAVE_XFT_H

int __ctalkGUIPanePutTransformedStr (OBJECT *self, int x, int y, char *s) {
  OBJECT *self_object, *win_id_value, *gc_value, *font_id_value;
  OBJECT *fontDesc_var, *fontDesc_value_var, *displayptr_var;
  int panebuffer_xid, panebackingstore_xid;
  char d_buf[MAXMSG], f_buf[MAXLABEL], fname_buf[MAXLABEL],
    /* w_buf[MAXLABEL],*/ pat_buf[MAXMSG], *pat;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  font_id_value = __x11_pane_font_id_value_object (self_object);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr",
					       TRUE);
  if ((fontDesc_var = 
       __ctalkGetInstanceVariable (self_object, "fontDescStr", TRUE)) 
      == NULL)
    return 0;
  if  ((fontDesc_value_var = fontDesc_var -> instancevars) == NULL)
    return 0;
  gc_value = __x11_pane_win_gc_value_object (self_object);
  if (atoi (font_id_value -> __o_value) == 0) {
    __ctalkDecimalIntegerToASCII ((int)fixed_font, f_buf);
    strcpy (fname_buf, FIXED_FONT_XLFD);
  } else {
    strcpy (f_buf, font_id_value -> __o_value);
    strcpy (fname_buf, fontDesc_value_var -> __o_value);
  }
  if (__ctalkXftInitialized ()) {

    if ((pat = __xft_selected_pattern_internal ()) == NULL)
      return 0;

    strcpy (pat_buf, pat);

    sprintf (d_buf, "%s:%d:%d:%s", pat_buf, x, y, s);

    __get_pane_buffers (self_object, &panebuffer_xid, 
			&panebackingstore_xid);
    if (panebuffer_xid) {
#if 1 /***/
      make_req (shm_mem, PANE_PUT_STR_REQUEST_FT,
		panebuffer_xid,
		SYMVAL(gc_value -> __o_value), d_buf);
#else
      make_req (shm_mem,
		SYMVAL(displayptr_var -> instancevars -> __o_value),
		PANE_PUT_STR_REQUEST_FT,
		panebuffer_xid,
		SYMVAL(gc_value -> __o_value), d_buf);
#endif      
    } else {
#if 1 /***/
      make_req (shm_mem, PANE_PUT_STR_REQUEST_FT,
		INTVAL(win_id_value -> __o_value),
		SYMVAL(gc_value -> __o_value), d_buf);
#else
      make_req (shm_mem,
		SYMVAL(displayptr_var -> instancevars -> __o_value),
		PANE_PUT_STR_REQUEST_FT,
		INTVAL(win_id_value -> __o_value),
		SYMVAL(gc_value -> __o_value), d_buf);
#endif      
    }

  } else { /* if (__ctalkXftInitialized ()) */

    sprintf (d_buf, "%d:%d:%s", x, y, s);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO    
    fprintf (stderr, "__ctalkX11LibPutTransformedStr: d_buf  = %s.\n", d_buf);
#endif
    __get_pane_buffers (self_object, &panebuffer_xid, 
			&panebackingstore_xid);
    if (panebuffer_xid) {
#if 1 /***/
      make_req (shm_mem, PANE_PUT_STR_REQUEST, panebuffer_xid,
		SYMVAL(gc_value -> __o_value), d_buf);
#else
      make_req (shm_mem,
		SYMVAL(displayptr_var -> instancevars -> __o_value),
		PANE_PUT_STR_REQUEST, panebuffer_xid,
		SYMVAL(gc_value -> __o_value), d_buf);
#endif      
    } else {
#if 1 /***/
      make_req (shm_mem, PANE_PUT_STR_REQUEST,
		INTVAL(win_id_value -> __o_value),
		SYMVAL(gc_value -> __o_value), d_buf);
#else
      make_req (shm_mem,
		SYMVAL(displayptr_var -> instancevars -> __o_value),
		PANE_PUT_STR_REQUEST,
		INTVAL(win_id_value -> __o_value),
		SYMVAL(gc_value -> __o_value), d_buf);
#endif      
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
  fprintf (stderr, "__ctalkX11LibPutTransformedStr: shm_mem = %s, %s, %s.\n", shm_mem, 
	   &shm_mem[SHM_GC], &shm_mem[SHM_DATA]);
#endif
  return SUCCESS;
}

#else /* #ifdef HAVE_XFT_H */

int __ctalkGUIPanePutTransformedStr (OBJECT *self, int x, int y, char *s) {
  OBJECT *self_object, *win_id_value, *gc_value, *font_id_value,
    *displayptr_var;
  OBJECT *fontDesc_var, *fontDesc_value_var;
  int panebuffer_xid, panebackingstore_xid;
  char d_buf[MAXLABEL], f_buf[MAXLABEL], fname_buf[MAXLABEL],
    w_buf[MAXLABEL];
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  font_id_value = __x11_pane_font_id_value_object (self_object);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr",
					       TRUE);
  if ((fontDesc_var = 
       __ctalkGetInstanceVariable (self_object, "fontDescStr", TRUE)) 
      == NULL)
    return 0;
  if ((fontDesc_value_var = fontDesc_var -> instancevars) == NULL)
    return 0;
  gc_value = __x11_pane_win_gc_value_object (self_object);
  if (atoi (font_id_value -> __o_value) == 0) {
    __ctalkDecimalIntegerToASCII ((int)fixed_font, f_buf);
    strcpy (fname_buf, FIXED_FONT_XLFD);
  } else {
    strcpy (f_buf, font_id_value -> __o_value);
    strcpy (fname_buf, fontDesc_value_var -> __o_value);
  }
  sprintf (d_buf, "%d:%d:%s", x, y, s);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO    
  fprintf (stderr, "__ctalkX11LibPutTransformedStr: d_buf  = %s.\n", d_buf);
#endif
  __get_pane_buffers (self_object, &panebuffer_xid, 
		      &panebackingstore_xid);
  if (panebuffer_xid) {
#if 1 /***/
    make_req (shm_mem, PANE_PUT_STR_REQUEST, panebuffer_xid,
	      SYMVAL(gc_value -> __o_value), d_buf);
#else
    make_req (shm_mem,
	      SYMVAL(displayptr_var -> instancevars -> __o_value),
	      PANE_PUT_STR_REQUEST, panebuffer_xid,
	      SYMVAL(gc_value -> __o_value), d_buf);
#endif    
  } else {
#if 1 /***/
    make_req (shm_mem, PANE_PUT_STR_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);
#else
    make_req (shm_mem,
	      SYMVAL(displayptr_var -> instancevars -> __o_value),
	      PANE_PUT_STR_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);
#endif    
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
  fprintf (stderr, "__ctalkX11LibPutTransformedStr: shm_mem = %s, %s, %s.\n", shm_mem, 
	   &shm_mem[SHM_GC], &shm_mem[SHM_DATA]);
#endif
  return SUCCESS;
}

#endif /* #ifdef HAVE_XFT_H */

#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int __ctalkX11PanePutTransformedStr (OBJECT *self_object, int x, int y, char *s) {
  x_support_error ();
  return 0; /* notreached */
}
int __ctalkGUIPanePutTransformedStr (OBJECT *self_object, int x, int y, char *s) {
  x_support_error ();
  return 0; /* notreached */
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
