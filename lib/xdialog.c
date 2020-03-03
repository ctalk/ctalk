/* $Id: xdialog.c,v 1.6 2020/03/03 02:25:05 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2020  Robert Kiesling, rk3314042@gmail.com.
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
 *  __ctalkX11PaneClearWindow is here for compatibility,
 *  but it is deprecated.
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
#include <X11/Xutil.h>

static Display *d = NULL;   /* Defined in x11lib.c. */
static Window d_root = 0;
static int d_screen = 0, d_screen_depth = 0;
Atom wm_delete_dialog;
extern Font fixed_font;
extern char **fixed_font_set;
extern int n_fixed_fonts;

int lookup_color (Display *, XColor *, char *);
Font get_user_font (OBJECT *);
int __xlib_set_wm_name_prop (Display *, Drawable, GC, char *);
void __save_pane_to_vars (OBJECT *, GC, int, int);
void x_support_error ();
int __x11_pane_border_width (OBJECT *);


#if ! defined (DJGPP) && ! defined (WITHOUT_X11)

#if X11LIB_FRAME

int __ctalkX11CreateDialogWindow (OBJECT *self_object) {
  return SUCCESS;
}

#else /* X11LIB_FRAME */

static int open_dialog_display_connection (void) {
  if (d == NULL) {
    if ((d = XOpenDisplay (getenv ("DISPLAY"))) == NULL) {
      return ERROR;
    }
    d_screen = DefaultScreen (d);
    d_root = RootWindow (d, d_screen);
    d_screen_depth = DefaultDepth (d, d_screen);
  }
  return SUCCESS;
}

int __ctalkX11CreateDialogWindow (OBJECT *self_object) {
  Window win_id, root_return;
  XSetWindowAttributes set_attributes;
  XGCValues gcv;
  GC gc;
  OBJECT *displayPtr;
  XWMHints wm_hints;
  char buf[MAXLABEL];
  static int wm_event_mask;
  int geom_ret, x_return, y_return;
  int pane_x, pane_y, pane_width, pane_height, border_width;
  unsigned int width_return, height_return, depth_return, border_width_return;
  int x_org, y_org, x_size, y_size;
  OBJECT *bgColor;
  XColor bg_color;

  wm_event_mask = WM_CONFIGURE_EVENTS | WM_INPUT_EVENTS;

  if (open_dialog_display_connection () < 0) {
    return ERROR;
  } else {
    displayPtr = __ctalkGetInstanceVariable (self_object,
					     "displayPtr", TRUE);
    SYMVAL(displayPtr -> instancevars -> __o_value) = (uintptr_t)d;
  }
  
  border_width = __x11_pane_border_width (self_object);
  set_attributes.backing_store = Always;
  set_attributes.save_under = true;
  /* set_size_hints_internal (self_object, &x_org, &y_org, &x_size, &y_size); */
  x_org = __pane_x_org (self_object);
  y_org = __pane_y_org (self_object);
  x_size = __pane_x_size (self_object);
  y_size = __pane_y_size (self_object);
  bgColor = __ctalkGetInstanceVariable (self_object,
					"backgroundColor", TRUE);
  win_id = XCreateWindow (d, d_root, 
			  x_org, y_org, x_size, y_size,
			  border_width, d_screen_depth,
			  CopyFromParent, CopyFromParent, 
			  CWBackingStore|CWSaveUnder,
			  &set_attributes);

  wm_hints.flags = (InputHint|StateHint);
  wm_hints.input = TRUE;
  wm_hints.initial_state = NormalState;
  XSetWMHints (d, win_id, &wm_hints);
  
  wm_delete_dialog = XInternAtom (d, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (d, win_id, &wm_delete_dialog, 1);

  XSetWindowBorder (d, win_id, BlackPixel(d, d_screen));
  XSelectInput(d, win_id, wm_event_mask);
  gcv.fill_style = FillSolid;
  gcv.function = GXcopy;
  gcv.foreground = BlackPixel (d, d_screen);

  if (*bgColor -> __o_value) {
    lookup_color (d, &bg_color, bgColor -> __o_value);
    gcv.background = bg_color.pixel;
  } else {
    gcv.background = WhitePixel (d, d_screen);
  }

  if ((gcv.font = get_user_font (self_object)) == 0) {
    if (n_fixed_fonts && !fixed_font)
      fixed_font = XLoadFont (d, fixed_font_set[0]);
    if (fixed_font)
      gcv.font = fixed_font;
  }
  gc = XCreateGC (d, win_id, DEFAULT_GCV_MASK, &gcv);

  if (*bgColor -> __o_value) {
    XSetWindowBackground (d, win_id, bg_color.pixel);
  } else {
    XSetWindowBackground (d, win_id, WhitePixel (d, d_screen));
  }


  __xlib_set_wm_name_prop 
    (d, win_id, gc, 
     basename_w_extent(__argvFileName ()));

  __save_pane_to_vars (self_object, gc, win_id,
		       DefaultDepth (d, DefaultScreen (d)));
  return win_id;
}

#endif /* #if 0 */

#endif /* X11LIB_FRAME */

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

int __ctalkX11CreateDialogWindow (OBJECT *self_object) {
  x_support_error ();
  return 0;  /* notreached */
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
