/* $Id: bitmap.c,v 1.6 2020/03/03 02:25:05 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015, 2019  Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include <X11/Xlib.h>
#include "x11defs.h"

extern Display *display;

#define RECTANGLE_GCV_MASK (GCFunction|GCFillStyle|\
			    GCForeground|GCBackground) 
int __xlib_clear_pixmap (Drawable d, unsigned int width, 
			 unsigned int height, GC gc){
  XGCValues rectangle_gcv, old_gcv;
  XGetGCValues (display, gc, DEFAULT_GCV_MASK, &old_gcv);
  rectangle_gcv.function = GXcopy;
  rectangle_gcv.fill_style = FillSolid;
  rectangle_gcv.background = old_gcv.background;
  rectangle_gcv.foreground = old_gcv.background;
  XChangeGC (display, gc, RECTANGLE_GCV_MASK, &rectangle_gcv);
  XFillRectangle (display, d, gc,
		  0, 0, width, height);
  XChangeGC (display, gc, DEFAULT_GCV_MASK, &old_gcv);
  return SUCCESS;
}

/***/
int __xlib_clear_rectangle_basic (Display *disp, Drawable d, int x, int y, 
				  unsigned int width, unsigned int height,
				  GC gc) {
  XGCValues rectangle_gcv, old_gcv;
  XGetGCValues (disp, gc, DEFAULT_GCV_MASK, &old_gcv);
  rectangle_gcv.function = GXset;
  rectangle_gcv.fill_style = FillSolid;
  rectangle_gcv.background = old_gcv.background;
  rectangle_gcv.foreground = old_gcv.background;
  XChangeGC (disp, gc, RECTANGLE_GCV_MASK, &rectangle_gcv);
  XFillRectangle (disp, d, gc,
 		  x, y, width, height);
  XChangeGC (disp, gc, DEFAULT_GCV_MASK, &old_gcv);
  return SUCCESS;
}

int __ctalkX11CreatePixmap (void *d,
			    int parent,
			    int width,
			    int height, 
			    int depth) {
  Pixmap p;
  GC gc;
  XGCValues v;
  p = XCreatePixmap ((Display *)d, (Drawable)parent, width, height, depth);
  v.function = GXcopy;
  v.fill_style = FillSolid;
  v.background = v.foreground = 
    BlackPixel ((Display *)d, DefaultScreen (display));
  gc = XCreateGC ((Display *)d, p, 
		  GCForeground|\
		  GCBackground|\
		  GCFunction|\
		  GCFillStyle, &v);
  XFillRectangle ((Display *)d, p, gc,
		  0, 0, width, height);
  XFreeGC ((Display *)d, gc);
  return (int)p;
}

void __ctalkX11DeletePixmap (int id) {
  XFreePixmap (display, (Drawable)id);
}

void __ctalkX11FreeGC (unsigned long int gc_ptr) {
  XFreeGC (display, (GC)gc_ptr);
}

void * __ctalkX11CreateGC (void *d, int drawable) {
  GC gc;
  XGCValues v;
  gc = XCreateGC ((Display *)d, (Drawable)drawable, 0, &v);
  return (void *)gc;
}

int __ctalkX11CreatePaneBuffer (OBJECT *pane,
				int width, int height, 
				int depth) {
  OBJECT *pane_object, *paneBuffer_object, *paneBackingStore_object,
    *xWindowID_object;
  OBJECT *xGC_object;
  GC gc;
  int win_id;
  char buf[MAXMSG];
  Pixmap p;
  pane_object = (IS_VALUE_INSTANCE_VAR(pane) ? pane -> __o_p_obj :
		 pane);
  if ((xGC_object = 
	__ctalkGetInstanceVariable (pane_object, "xGC", TRUE))
       == NULL)
    return ERROR;

  errno = 0;
  gc = (GC)strtoul (xGC_object->instancevars->__o_value, NULL, 16);
  if (errno != 0) {
    strtol_error (errno, "__ctalkX11CreatePaneBuffer ()", 
		  xGC_object->instancevars->__o_value);
  }
	  
  if ((paneBuffer_object = 
       __ctalkGetInstanceVariable (pane_object, "paneBuffer", TRUE))
      == NULL)
    return ERROR;
  if ((xWindowID_object = 
       __ctalkGetInstanceVariable (pane_object, "xWindowID", TRUE))
      == NULL)
    return ERROR;
  if ((paneBackingStore_object = 
	__ctalkGetInstanceVariable (pane_object, "paneBackingStore", TRUE))
       == NULL)
    return ERROR;
  win_id = atoi (xWindowID_object->instancevars->__o_value);
  p = XCreatePixmap (display, win_id, width, height, depth);
  __xlib_clear_pixmap (p, width, height, gc);
  __ctalkDecimalIntegerToASCII ((int)p, buf);
  __ctalkSetObjectValueVar (paneBuffer_object, buf);
  p = XCreatePixmap (display, win_id, width, height, depth);
  __xlib_clear_pixmap (p, width, height, gc);
  __ctalkDecimalIntegerToASCII((int)p, buf);
  __ctalkSetObjectValueVar (paneBackingStore_object, buf);
  return SUCCESS;
}

int __ctalkX11FreePaneBuffer (OBJECT *pane) {
  OBJECT *pane_object, *paneBuffer_object, *paneBackingStore_object;
  pane_object = (IS_VALUE_INSTANCE_VAR(pane) ? pane -> __o_p_obj :
		 pane);
  if ((paneBuffer_object = 
       __ctalkGetInstanceVariable (pane_object, "paneBuffer", TRUE))
      == NULL)
    return ERROR;
  if ((paneBackingStore_object = 
	__ctalkGetInstanceVariable (pane_object, "paneBackingStore", TRUE))
       == NULL)
    return ERROR;
  XFreePixmap (display, 
	       atoi(paneBackingStore_object->instancevars->__o_value));
  XFreePixmap (display, 
	       atoi(paneBuffer_object->instancevars->__o_value));
  return SUCCESS;
}
int __ctalkX11ResizePaneBuffer (OBJECT *pane,
				int width, int height) {
  OBJECT *pane_object, *paneBuffer_object, *paneBackingStore_object,
    *xWindowID_object;
  OBJECT *xGC_object;
  int win_id;
  char buf[MAXMSG];
  unsigned int p_old;
  Pixmap p;
  GC gc;
  pane_object = (IS_VALUE_INSTANCE_VAR(pane) ? pane -> __o_p_obj :
		 pane);
  if ((paneBuffer_object = 
       __ctalkGetInstanceVariable (pane_object, "paneBuffer", TRUE))
      == NULL)
    return ERROR;
  if ((xWindowID_object = 
       __ctalkGetInstanceVariable (pane_object, "xWindowID", TRUE))
      == NULL)
    return ERROR;
  if ((paneBackingStore_object = 
	__ctalkGetInstanceVariable (pane_object, "paneBackingStore", TRUE))
       == NULL)
    return ERROR;
  if ((xGC_object = 
	__ctalkGetInstanceVariable (pane_object, "xGC", TRUE))
       == NULL)
    return ERROR;
  win_id = atoi (xWindowID_object->instancevars->__o_value);
  p = XCreatePixmap (display, win_id, width, height, CopyFromParent);

  errno = 0;
  p_old = strtoul (paneBuffer_object->instancevars->__o_value, NULL, 10);
  if (errno != 0) {
    strtol_error (errno, "__ctalkX11ResizePaneBuffer ()", 
		  paneBuffer_object->instancevars->__o_value);
  }

  errno = 0;
  gc = (GC)strtoul (xGC_object->instancevars->__o_value, NULL, 10);
  if (errno != 0) {
    strtol_error (errno, "__ctalkX11ResizePaneBuffer ()", 
		  xGC_object->instancevars->__o_value);
  }
  XCopyArea (display, (Pixmap)p_old, p, gc, 
	     0, 0, 
	     width, height,
	     0, 0);
  __ctalkDecimalIntegerToASCII ((int)p, buf);
  __ctalkSetObjectValueVar (paneBuffer_object, buf);
  XFreePixmap (display, (Pixmap)p_old);

  errno = 0;
  p_old = strtoul (paneBackingStore_object->instancevars->__o_value, NULL, 10);
  if (errno != 0) {
    strtol_error (errno, "__ctalkX11ResizePaneBuffer ()", 
		  paneBackingStore_object->instancevars->__o_value);
  }
  p = XCreatePixmap (display, win_id, width, height, CopyFromParent);
  XCopyArea (display, (Pixmap)p_old, p, gc, 
	     0, 0, 
	     width, height,
	     0, 0);
  __ctalkDecimalIntegerToASCII ((int)p, buf);
  __ctalkSetObjectValueVar (paneBackingStore_object, buf);
  XFreePixmap (display, (Pixmap)p_old);
  return SUCCESS;
}

#if 0 /***/
/* Used? */
int __ctalkX11ClearBufferRectangle (OBJECT *pane,
				    int x, int y,
				    int width, int height) {
  OBJECT *pane_object, *paneBuffer_object, *paneBackingStore_object,
    *xWindowID_object, *xGC_object, *displayptr_var;
  GC gc;
  pane_object = (IS_VALUE_INSTANCE_VAR(pane) ? pane -> __o_p_obj :
		 pane);
  if ((xGC_object = 
       __ctalkGetInstanceVariable (pane_object, "xGC", TRUE))
      == NULL)
    return ERROR;
  if ((paneBuffer_object = 
       __ctalkGetInstanceVariable (pane_object, "paneBuffer", TRUE))
      == NULL)
    return ERROR;
  errno = 0;
  gc = (GC)strtoul (xGC_object->instancevars->__o_value, NULL, 16);
  if (errno != 0) {
    strtol_error (errno, "__ctalkX11CreatePaneBuffer ()", 
		  xGC_object->instancevars->__o_value);
  }
  __xlib_clear_rectangle_basic 
    (display, atoi(paneBuffer_object->instancevars->__o_value), 
     x, y, width, height, gc);
  if ((paneBackingStore_object = 
	__ctalkGetInstanceVariable (pane_object, "paneBackingStore", TRUE))
       == NULL)
    return ERROR;
  __xlib_clear_rectangle_basic 
    (display, atoi(paneBackingStore_object->instancevars->__o_value), 
     x, y, width, height, gc);
  if ((xWindowID_object = 
       __ctalkGetInstanceVariable (pane_object, "xWindowID", TRUE))
      == NULL)
    return ERROR;
  __xlib_clear_rectangle_basic 
    (display, atoi(xWindowID_object->instancevars->__o_value), 
     x, y, width, height, gc);
  return SUCCESS;

}
#endif

int __get_pane_buffers (OBJECT *pane_object, int *panebuffer_xid,
			int *panebackingstore_xid) {
  OBJECT *paneBuffer_instance_var,
    *paneBackingStore_instance_var,
    *paneBuffer_xID_instance_var,
    *paneBackingStore_xID_instance_var;
  *panebuffer_xid = *panebackingstore_xid = 0;
  
  for (paneBuffer_instance_var = pane_object -> instancevars; 
       paneBuffer_instance_var; 
       paneBuffer_instance_var = paneBuffer_instance_var -> next) {
    if (str_eq (paneBuffer_instance_var -> __o_name, PANEBUFFER_INSTANCE_VAR))
      break;
  }

  if (!paneBuffer_instance_var)
    return ERROR;

  for (paneBackingStore_instance_var = pane_object -> instancevars; 
       paneBackingStore_instance_var; 
       paneBackingStore_instance_var = paneBackingStore_instance_var -> next) {
    if (str_eq (paneBackingStore_instance_var -> __o_name, 
		PANEBACKINGSTORE_INSTANCE_VAR))
      break;
  }

  if (!paneBackingStore_instance_var)
    return ERROR;

  for (paneBuffer_xID_instance_var = paneBuffer_instance_var -> instancevars; 
       paneBuffer_xID_instance_var; 
       paneBuffer_xID_instance_var = paneBuffer_xID_instance_var -> next) {
    if (str_eq (paneBuffer_xID_instance_var -> __o_name, 
		X11BITMAP_ID_INSTANCE_VAR)) {
      *panebuffer_xid = *(int *)paneBuffer_xID_instance_var ->
	instancevars -> __o_value;
      break;
    }
  }

  for (paneBackingStore_xID_instance_var = 
	 paneBackingStore_instance_var -> instancevars; 
       paneBackingStore_xID_instance_var; 
       paneBackingStore_xID_instance_var = 
	 paneBackingStore_xID_instance_var -> next) {
    if (str_eq (paneBackingStore_xID_instance_var -> __o_name, 
		X11BITMAP_ID_INSTANCE_VAR)) {
      *panebackingstore_xid =
	*(int *)paneBackingStore_xID_instance_var -> instancevars -> __o_value;
      break;
    }
  }

  return SUCCESS;
}

#else

int __ctalkX11CreatePaneBuffer (OBJECT *pane_object,
				  int width, int height, int depth) {
  x_support_error (); return ERROR;
}

int __ctalkX11FreePaneBuffer (OBJECT *pane_object) {
  x_support_error (); return ERROR;
}

int __ctalkX11ResizePaneBuffer (OBJECT *pane_object,
				int width, int height) {
  x_support_error (); return ERROR;
}

int __ctalkX11ClearBufferRectangle (OBJECT *pane_object,
				    int x, int y,
				    int width, int height) {
  x_support_error (); return ERROR;
}
void __ctalkX11DeletePixmap (int id) {
  x_support_error ();
}

void __ctalkX11FreeGC (unsigned long int gc_ptr) {
  x_support_error ();
}

void *__ctalkX11CreateGC (void *d, int drawable) {  
  x_support_error (); return NULL;
}
int __ctalkX11CreatePixmap (void *d,
			    int parent,
			    int width,
			    int height, 
			    int depth) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
