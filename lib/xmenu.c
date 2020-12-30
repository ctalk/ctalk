/* $Id: xmenu.c,v 1.13 2020/12/30 16:04:07 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014, 2018 - 2020  Robert Kiesling, rk3314042@gmail.com.
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
#include <time.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include "x11defs.h"
#include <X11/Xutil.h>

#include <fontconfig/fontconfig.h>
#include <X11/Xft/Xft.h>
#include FT_FREETYPE_H
#include "xftfont.h"

extern Display *display;   /* Defined in x11lib.c. */
extern GC create_pane_win_gc (Display *d, Window w, OBJECT *pane);
extern void __save_pane_to_vars (OBJECT *, GC, int, int);

static Display *scr_click_dpy = NULL;

bool screen_click (int *x_return, int *y_return, unsigned int *p_mask_return) {
  Window root_return, child_return;
  int root_x_return, root_y_return, win_x_return, win_y_return;
  unsigned int mask_return;

  if (!scr_click_dpy) {
    scr_click_dpy = XOpenDisplay (getenv ("DISPLAY"));
  }

  XQueryPointer (scr_click_dpy, DefaultRootWindow (scr_click_dpy),
		 &root_return, &child_return,
		 &root_x_return,
		 &root_y_return,
		 &win_x_return, &win_y_return,
		 &mask_return);
  *x_return = root_x_return;
  *y_return = root_y_return;
  *p_mask_return = mask_return;
  if ((mask_return & Button1Mask) ||
      (mask_return & Button2Mask) ||
      (mask_return & Button3Mask)) {
    return true;
  }
  return false;
}

bool point_is_in_rect (int x, int y, int x_org, int y_org,
		       int x_size, int y_size)  {
  if ((x >= x_org && x <= (x_org + x_size)) &&
      (y >= y_org && y <= (y_org + y_size)))
    return true;
  else
    return false;
}

int __ctalkX11MapMenu (OBJECT *menu_object, int p_dpy_x, int p_dpy_y) {
  OBJECT *displayPtr, *xWindowID;
  Display *menu_dpy;
  Window menu_win_id;
  
  if ((displayPtr = __ctalkGetInstanceVariable (menu_object,
						"displayPtr", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11WithdrawMenu: Could not find "
	     "\"displayPtr\" instance variable.\n");
    exit (ERROR);
  }
  menu_dpy = (Display *)SYMVAL(displayPtr -> instancevars -> __o_value);

  if ((xWindowID = __ctalkGetInstanceVariable (menu_object,
						"xWindowID", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11WithdrawMenu: Could not find "
	     "\"displayPtr\" instance variable.\n");
    exit (ERROR);
  }
  menu_win_id = INTVAL(xWindowID -> instancevars -> __o_value);

  XMoveWindow (menu_dpy, menu_win_id, p_dpy_x, p_dpy_y);
  XMapWindow (menu_dpy, menu_win_id);
  XMapSubwindows (menu_dpy, menu_win_id);
  XRaiseWindow (menu_dpy, menu_win_id);

  return SUCCESS;
}

extern XftFont *selected_font;  /* from xftlib.c */
static XftDraw *ftDraw = NULL;

int __ctalkX11MenuDrawString (void *d, unsigned int w, int x, int y,
			      char *str, char *p_color) {
  Display *d_l = (Display *)d;
  XColor xColor_exact, xColor_screen;
  XRenderColor xrcolor;
  XftColor xftcolor;

  
  XftDraw *xftdraw =
    XftDrawCreate ((Display *)d_l,
		   (Drawable)w,
		   DefaultVisual (d_l, DefaultScreen (d_l)),
		   DefaultColormap (d_l, DefaultScreen (d_l)));
  XftFont *xfont =
    XftFontOpen (d_l, DefaultScreen (d_l),
		 XFT_FAMILY, XftTypeString, "DejaVu",
		 XFT_SIZE, XftTypeDouble, 10.0, NULL);

  XAllocNamedColor (d_l, DefaultColormap (d_l, DefaultScreen (d_l)),
		    p_color, &xColor_exact, &xColor_screen);
  xrcolor.red = xColor_screen.red << 8;
  xrcolor.green = xColor_screen.green << 8;
  xrcolor.blue = xColor_screen.blue << 8;
  xrcolor.alpha = 0xffff;
  XftColorAllocValue (d_l, DefaultVisual (d_l, DefaultScreen (d_l)),
		      DefaultColormap (d_l, DefaultScreen (d_l)),
		      &xrcolor, &xftcolor);
  XftDrawString8 (xftdraw, &xftcolor, xfont, x, y,
		  (unsigned char *)str, strlen (str));
  XftColorFree (d_l, DefaultVisual (d_l, DefaultScreen (d_l)),
		DefaultColormap (d_l, DefaultScreen (d_l)),
		&xftcolor);

#if 0
  XftColor ftFg;
  XRenderColor fgColor;
  Display *d_l;

  d_l = (Display *)d;

  if (!ftDraw) {
    ftDraw = 
      XftDrawCreate (d_l, (Drawable)w,
		     DefaultVisual (d_l, DefaultScreen (d_l)),
		     DefaultColormap (d_l, DefaultScreen (d_l)));
  }

  load_ft_font_faces_internal ("sans-serif", 12.0, 100, 100, 72);

  __ctalkXftSetForegroundFromNamedColor (color);
  fgColor.red = (unsigned short)__ctalkXftFgRed ();
  fgColor.green = (unsigned short)__ctalkXftFgGreen ();
  fgColor.blue = (unsigned short)__ctalkXftFgBlue ();
  fgColor.alpha = (unsigned short)0xffff;
  XftColorAllocValue(d_l,
		     DefaultVisual (d_l, DefaultScreen (d_l)),
		     DefaultColormap (d_l, DefaultScreen (d_l)),
		     &fgColor, &ftFg);
  XftDrawString8 (ftDraw, &ftFg, selected_font,
		  x, y, (unsigned char *)str,
		  strlen (str));
#endif  
}

int __ctalkX11WithdrawMenu (OBJECT *menu_object) {
  OBJECT *displayPtr, *xWindowID;
  Display *menu_dpy;
  Window menu_win_id;
  
  if ((displayPtr = __ctalkGetInstanceVariable (menu_object,
						"displayPtr", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11WithdrawMenu: Could not find "
	     "\"displayPtr\" instance variable.\n");
    exit (ERROR);
  }
  menu_dpy = (Display *)SYMVAL(displayPtr -> instancevars -> __o_value);

  if ((xWindowID = __ctalkGetInstanceVariable (menu_object,
						"xWindowID", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11WithdrawMenu: Could not find "
	     "\"displayPtr\" instance variable.\n");
    exit (ERROR);
  }
  menu_win_id = INTVAL(xWindowID -> instancevars -> __o_value);

  XUnmapWindow (menu_dpy, menu_win_id);
  XFlush (menu_dpy);
  return SUCCESS;
}

int __ctalkX11CreatePopupMenu (OBJECT *self_object, int x, int p_y) {
  Window menu_win_id;
  XSetWindowAttributes set_attributes;
  GC gc;
  XGCValues xgcv;
  OBJECT *displayPtr, *items, *size, *y, *t;
  static int wm_event_mask;
  int x_org, y_org, x_size, y_size, border_width;
  Display *d_l;
  int n_items;

  wm_event_mask = WM_CONFIGURE_EVENTS | WM_INPUT_EVENTS;

  if ((d_l = XOpenDisplay (getenv ("DISPLAY"))) == NULL) {
    return ERROR;
  }
  
  if ((displayPtr = __ctalkGetInstanceVariable (self_object,
						"displayPtr", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11CreatePopupMenu: Could not find "
	     "\"displayPtr\" instance variable.\n");
    exit (ERROR);
  }
  SYMVAL(displayPtr -> instancevars -> __o_value) = (uintptr_t)d_l;

  if ((items = __ctalkGetInstanceVariable (self_object,
					   "items", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11CreatePopupMenu: Could not find "
	     "\"items\" instance variable.\n");
    exit (ERROR);
  }

  if ((size = __ctalkGetInstanceVariable (self_object,
					   "size", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11CreatePopupMenu: Could not find "
	     "\"size\" instance variable.\n");
    exit (ERROR);
  }

  if ((y = __ctalkGetInstanceVariable (size, "y", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11CreatePopupMenu: Could not find "
	     "\"size y\" instance variable.\n");
    exit (ERROR);
  }

  n_items = 0;
  for (t = items -> instancevars -> next; t; t = t -> next)
    ++n_items;

  INTVAL (y -> instancevars -> __o_value) =
    INTVAL(y -> __o_value) = (n_items + 1) * 16;

  border_width = 0;
  set_attributes.backing_store = Always;
  set_attributes.save_under = true;
  set_attributes.override_redirect = true;

  menu_win_id = XCreateWindow
    (d_l, DefaultRootWindow (d_l), 
     x, p_y, 100, INTVAL(y -> __o_value),
     border_width,
     DefaultDepth (d_l, DefaultScreen (d_l)),
     CopyFromParent, CopyFromParent, 
     CWBackingStore|CWSaveUnder|CWOverrideRedirect,
     &set_attributes);

  XSelectInput(d_l, menu_win_id, wm_event_mask);

  gc = create_pane_win_gc (d_l, menu_win_id, self_object);
  xgcv.background = WhitePixel (d_l, DefaultScreen (d_l));
  xgcv.foreground = BlackPixel (d_l, DefaultScreen (d_l));
  xgcv.function = GXcopy;
  XChangeGC (d_l, gc, GCFunction|GCBackground|GCForeground, &xgcv);

  __save_pane_to_vars (self_object, gc, menu_win_id,
		       DefaultDepth (d_l,
				     DefaultScreen (d_l)));
  return menu_win_id;
}

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

int __ctalkX11CreatePopupMenu (OBJECT *self_object, int x, int y) {
  fprintf (stderr, "__ctalkX11CreatePopupMenu: This program requires "
	   "the X Window System.  Exiting.\n");
  exit (ERROR);
}

int __ctalkX11WithdrawMenu (OBJECT *self_object) {
  fprintf (stderr, "__ctalkX11WithdrawMenu: This program requires "
	   "the X Window System.  Exiting.\n");
  exit (ERROR);
}

int __ctalkX11MapMenu (OBJECT *menu_object, int p_dpy_x, int p_dpy_y) {
  fprintf (stderr, "__ctalkX11MapMenu: This program requires "
	   "the X Window System.  Exiting.\n");
  exit (ERROR);
}

int __ctalkX11MenuDrawString (void *d, unsigned int w, int x, int y,
			      char *str) {
  fprintf (stderr, "__ctalkX11MenuDrawString: This program requires "
	   "the X Window System.  Exiting.\n");
  exit (ERROR);
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
