/* $Id: xmenu.c,v 1.34 2021/03/09 20:22:45 rkiesling Exp $ -*-c-*-*/

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

#if defined (HAVE_XFT_H) && defined (HAVE_XRENDER_H)
#include "xrender.h"
#endif
extern Display *display;   /* Defined in x11lib.c. */
extern GC create_pane_win_gc (Display *d, Window w, OBJECT *pane);
extern void __save_pane_to_vars (OBJECT *, GC, int, int);
extern char *ascii[8193];

#define N_TRANSIENTS 63
#define M_TRANSIENT pmenurecs[pmenurecs_ptr+1]
static W_TRANSIENT *pmenurecs[N_TRANSIENTS+1] =  {NULL,};
static int pmenurecs_ptr = N_TRANSIENTS;

static void push_pmenurec (OBJECT *pane_object, Display *d_l) {
  pmenurecs[pmenurecs_ptr] = __xalloc (sizeof (struct _dc));
  pmenurecs[pmenurecs_ptr] -> pane = pane_object;
  pmenurecs[pmenurecs_ptr] -> d_p = d_l;
  pmenurecs[pmenurecs_ptr] -> d_p_screen =
    DefaultScreen (pmenurecs[pmenurecs_ptr] -> d_p);
  pmenurecs[pmenurecs_ptr] -> d_p_root =
    RootWindow (pmenurecs[pmenurecs_ptr] -> d_p,
		pmenurecs[pmenurecs_ptr] -> d_p_screen);
  pmenurecs[pmenurecs_ptr] -> d_p_screen_depth =
    DefaultDepth (pmenurecs[pmenurecs_ptr] -> d_p,
		  pmenurecs[pmenurecs_ptr] -> d_p_screen);
  --pmenurecs_ptr;
}

static W_TRANSIENT *pop_pmenurec (void) {
  W_TRANSIENT *w;
  ++pmenurecs_ptr;
  w = pmenurecs[pmenurecs_ptr];
  pmenurecs[pmenurecs_ptr] = NULL;
  return w;
}

Display *menu_dpy (void) {
  if (pmenurecs_ptr < N_TRANSIENTS) {
    if ((M_TRANSIENT != NULL) && (M_TRANSIENT -> d_p != NULL)) {
      return M_TRANSIENT -> d_p;
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

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
    fprintf (stderr, "__ctalkX11MapMenu: Could not find "
	     "\"displayPtr\" instance variable.\n");
    exit (ERROR);
  }
  menu_dpy = (Display *)SYMVAL(displayPtr -> instancevars -> __o_value);
  push_pmenurec (menu_object, menu_dpy); 

  if ((xWindowID = __ctalkGetInstanceVariable (menu_object,
						"xWindowID", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11MapMenu: Could not find "
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

extern XRENDERDRAWREC surface;

void xr_make_surface (Display *, Drawable);
bool get_color (char *, XftColor *);
void save_color (char *, XftColor *);

int __ctalkX11MenuDrawLine (void *d, int drawable_id,
			    unsigned long gc_ptr,
			    int x_start, int y_start,
			    int x_end, int y_end,
			    int pen_width, int alpha,
			    char *color) {
#if defined (HAVE_XFT_H) && defined (HAVE_XRENDER_H)

  XftColor rcolor;
  XPointDouble poly[4];
  int dx, dy;
  double pen_half_width;
  
  xr_make_surface (d, drawable_id);
  if (!get_color (color, &rcolor)) {
    if (XftColorAllocName (d, 
			   DefaultVisual (d, DefaultScreen (d)),
			   DefaultColormap (d, DefaultScreen (d)),
			   color, &rcolor)) {
      rcolor.color.alpha = alpha;
      save_color (color, &rcolor);
    } else {
      return ERROR;
    }
  }

  if (rcolor.color.alpha != alpha)
    rcolor.color.alpha = alpha;
  surface.fill_picture = XftDrawSrcPicture (surface.draw, &rcolor);
  if (surface.fill_picture == 0) {
    surface.drawable = 0;
    return ERROR;
  }

  dx = x_start - x_end;
  dy = y_start - y_end;
  pen_half_width = ((double)pen_width / 2.0);

  /* This is (almost) from xlib_draw_line in xrender.c */
  if (abs(dx) > abs(dy)) {
    poly[0].x = ((double)x_start);
    poly[0].y = ((double)y_start) + pen_half_width;
    poly[1].x = ((double)x_end);
    poly[1].y = ((double)y_end) + pen_half_width;
    poly[2].x = ((double)x_end);
    poly[2].y = ((double)y_end) - pen_half_width;
    poly[3].x = ((double)x_start);
    poly[3].y = ((double)y_start) - pen_half_width;
  } else {
    poly[0].x = ((double)x_start) + pen_half_width;
    poly[0].y = ((double)y_start);
    poly[1].x = ((double)x_end) + pen_half_width;
    poly[1].y = ((double)y_end);
    poly[2].x = ((double)x_end) - pen_half_width;;
    poly[2].y = ((double)y_end);
    poly[3].x = ((double)x_start) - pen_half_width;
    poly[3].y = ((double)y_start);
  }
  
  XRenderCompositeDoublePoly (d,
			      PictOpOver,
			      surface.fill_picture,
			      surface.picture,
			      surface.mask_format,
			      0, 0, 0, 0, poly, 4, EvenOddRule);

    return SUCCESS;

#else /* #if defined (HAVE_XFT_H) && defined (HAVE_XRENDER_H) */

  XGCValues xgcv;

  xgcv.foreground = lookup_pixel_d (d, color);
  /* xgcv.background = self bgPixel; *//***/
  xgcv.line_width = pen_width;
  XChangeGC (self displayPtr, self xGC,
	     GCForeground|GCLineWidth, &xgcv);
#if 0/***/
  XChangeGC (self displayPtr, self xGC,
	     GCForeground|GCBackground|GCLineWidth, &xgcv);
#endif  
  
  XDrawLine (self displayPtr, self paneBuffer xID, self xGC,
	     x_start, y_start,
	     x_end, y_end);

#endif   /* #if defined (HAVE_XFT_H) && defined (HAVE_XRENDER_H) */
  return SUCCESS;
}

extern XftFont *selected_font;
static XftFont *xfont = NULL;  
static XftDraw *xftdraw = NULL;
static Display *xftdraw_dpy;
static Drawable xftdraw_drawable;
static char ftFont[MAXMSG];
static char textColor[MAXLABEL];
static XftColor xftcolor;

int __ctalkX11MenuDrawString (void *d, unsigned int w, int x, int y,
			      int text_x_size, int text_y_size,
			      int box_x_size, int box_y_size,
			      char *str, char *p_color) {
  Display *d_l = (Display *)d;
  XColor xColor_exact, xColor_screen;
  XRenderColor xrcolor;
  int y_vcenter = 0;
  
  if (xftdraw == NULL) {  
    xftdraw =
      XftDrawCreate ((Display *)d_l,
		     (Drawable)w,
		     DefaultVisual (d_l, DefaultScreen (d_l)),
		     DefaultColormap (d_l, DefaultScreen (d_l)));
    xftdraw_dpy = d_l;
    xftdraw_drawable = w;
  } else if (d_l != xftdraw_dpy || w != xftdraw_drawable) {
    if (xftdraw)
      XftDrawDestroy (xftdraw);
    xftdraw =
      XftDrawCreate ((Display *)d_l,
		     (Drawable)w,
		     DefaultVisual (d_l, DefaultScreen (d_l)),
		     DefaultColormap (d_l, DefaultScreen (d_l)));
    xftdraw_dpy = d_l;
    xftdraw_drawable = w;
  }

  if (xfont == NULL) {
    __ctalkXftSelectFontFromFontConfig (ftFont);
    xfont = selected_font;
  }

  if (strcmp (textColor, p_color)) {
    XAllocNamedColor (d_l, DefaultColormap (d_l, DefaultScreen (d_l)),
		      p_color, &xColor_exact, &xColor_screen);
    xrcolor.red = xColor_screen.red << 8;
    xrcolor.green = xColor_screen.green << 8;
    xrcolor.blue = xColor_screen.blue << 8;
    xrcolor.alpha = 0xffff;
    XftColorAllocValue (d_l, DefaultVisual (d_l, DefaultScreen (d_l)),
			DefaultColormap (d_l, DefaultScreen (d_l)),
			&xrcolor, &xftcolor);
    strcpy (textColor, p_color);
  }



  y_vcenter = (box_y_size / 2) - (text_y_size / 2);
  XftDrawString8 (xftdraw, &xftcolor, xfont, x, y - y_vcenter,
		  (unsigned char *)str, strlen (str));

#if 0  /* Leave for now in case we can't save a color somewhere. */
  XftColorFree (d_l, DefaultVisual (d_l, DefaultScreen (d_l)),
		DefaultColormap (d_l, DefaultScreen (d_l)),
		&xftcolor);
#endif  
}

int __ctalkX11WithdrawMenu (OBJECT *menu_object_param) {
  OBJECT *menu_object, *displayPtr, *xWindowID;
  Display *menu_dpy;
  Window menu_win_id;
  W_TRANSIENT *w;
  
  while (pmenurecs_ptr < N_TRANSIENTS)  {

    w = pop_pmenurec ();

    menu_object = w -> pane;

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
    __xfree (MEMADDR(w));
  }
  return SUCCESS;
}

unsigned long lookup_pixel_d (Display *, char *);

int __ctalkX11CreatePopupMenu (OBJECT *self_object, int p_x, int p_y) {
  Window menu_win_id;
  XSetWindowAttributes set_attributes;
  GC gc;
  XGCValues xgcv;
  OBJECT *displayPtr, *items, *size, *y, *x, *t, *t_val,
    *resources;
    
  static int wm_event_mask;
  int x_org, y_org, x_size, y_size, border_width;
  Display *d_l;
  int n_items;
  char bgColorName[MAXLABEL];

  wm_event_mask = WM_CONFIGURE_EVENTS | WM_INPUT_EVENTS;

  if ((d_l = XOpenDisplay (getenv ("DISPLAY"))) == NULL) {
    return ERROR;
  }
  /* push_pmenurec (self_object, d_l); */
  
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
  if ((x = __ctalkGetInstanceVariable (size, "x", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11CreatePopupMenu: Could not find "
	     "\"size x\" instance variable.\n");
    exit (ERROR);
  }
  if ((resources = __ctalkGetInstanceVariable (self_object, "resources", TRUE))
      == NULL) {
    fprintf (stderr, "__ctalkX11CreatePopupMenu: Could not find "
	     "\"resources\" instance variable.\n");
    exit (ERROR);
  }
  for (t = resources -> instancevars; t; t = t -> next) {
    if (str_eq (t -> __o_name, "font")) {
      t_val = *(OBJECT **)t -> instancevars -> __o_value;
      strcpy (ftFont, t_val -> __o_value);
    } else if (str_eq (t -> __o_name, "backgroundColor")) {
      strcpy (bgColorName, t_val -> __o_value);
    }
  }


  border_width = 0;
  set_attributes.backing_store = Always;
  set_attributes.save_under = true;
  set_attributes.override_redirect = true;

  menu_win_id = XCreateWindow
    (d_l, DefaultRootWindow (d_l), 
     0, 0, INTVAL(x -> __o_value), INTVAL(y -> __o_value),
     border_width,
     DefaultDepth (d_l, DefaultScreen (d_l)),
     CopyFromParent, CopyFromParent, 
     CWBackingStore|CWSaveUnder|CWOverrideRedirect,
     &set_attributes);

  XSelectInput(d_l, menu_win_id, wm_event_mask);

  gc = create_pane_win_gc (d_l, menu_win_id, self_object);
  xgcv.background = lookup_pixel_d (d_l, bgColorName);
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
			      int text_x_size, int text_y_size,
			      int box_x_size, int box_y_size,
			      char *str) {
  fprintf (stderr, "__ctalkX11MenuDrawString: This program requires "
	   "the X Window System.  Exiting.\n");
  exit (ERROR);
}

int __ctalkX11MenuDrawLine (void *d, int drawable_id,
			    unsigned long gc_ptr,
			    int x_start, int y_start,
			    int x_end, int y_end,
			    int pen_width, int alpha,
			    char *color) {
  fprintf (stderr, "__ctalkX11MenuDrawLine: This program requires "
	   "the X Window System.  Exiting.\n");
  exit (ERROR);
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
