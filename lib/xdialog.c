

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
#include "xpms/info64x64.xpm"
#include "xpms/stop64x64.xpm"
#include "xpms/warning64x64.xpm"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include "x11defs.h"
#include <X11/Xutil.h>

DIALOG_C *dpyrec = NULL;

Atom wm_delete_dialog;
extern Font fixed_font;
extern char **fixed_font_set;
extern int n_fixed_fonts;

extern char *shm_mem;
extern int mem_id;

int lookup_color (Display *, XColor *, char *);
Font get_user_font (OBJECT *);
int __xlib_set_wm_name_prop (Display *, Drawable, GC, char *);
void __save_pane_to_vars (OBJECT *, GC, int, int);
void x_support_error ();
int __x11_pane_border_width (OBJECT *);


static int open_dialog_display_connection (void) {
  if (dpyrec == NULL || dpyrec -> d_p == NULL) {
    dpyrec = __xalloc (sizeof (struct _dc));
    if ((dpyrec -> d_p = XOpenDisplay (getenv ("DISPLAY"))) == NULL) {
      return ERROR;
    }
    dpyrec -> d_p_screen = DefaultScreen (dpyrec -> d_p);
    dpyrec -> d_p_root = RootWindow (dpyrec -> d_p, dpyrec -> d_p_screen);
    dpyrec -> d_p_screen_depth = DefaultDepth (dpyrec -> d_p, dpyrec -> d_p_screen);
  }
  return SUCCESS;
}

static void dialog_xy_from_parent (OBJECT *self, OBJECT *mainWin,
				   int *x_org, int *y_org) {
  OBJECT *size_var, *size_x_var, *size_y_var;
  OBJECT *mw_origin_var, *mw_origin_x_var, *mw_origin_y_var,
    *mw_size_var, *mw_size_x_var, *mw_size_y_var;
  int self_width, self_height;
  int p_x_org, p_y_org, p_x_size, p_y_size;
  /* hefe, "self" is just a dummy argument,  the geometry string
     is already the parent's geometry in absolute pixels */

  size_var = __ctalkGetInstanceVariable (self, "size", TRUE);
  size_x_var = __ctalkGetInstanceVariable (size_var, "x", TRUE);
  size_y_var = __ctalkGetInstanceVariable (size_var, "y", TRUE);
  self_width = INTVAL(size_x_var -> instancevars -> __o_value);
  self_height = INTVAL(size_y_var -> instancevars -> __o_value);

  mw_origin_var = __ctalkGetInstanceVariable (mainWin, "origin", TRUE);
  mw_origin_x_var = __ctalkGetInstanceVariable (mw_origin_var, "x", TRUE);
  mw_origin_y_var = __ctalkGetInstanceVariable (mw_origin_var, "y", TRUE);
  mw_size_var = __ctalkGetInstanceVariable (mainWin, "size", TRUE);
  mw_size_x_var = __ctalkGetInstanceVariable (mw_size_var, "x", TRUE);
  mw_size_y_var = __ctalkGetInstanceVariable (mw_size_var, "y", TRUE);
  p_x_org = INTVAL(mw_origin_x_var -> instancevars -> __o_value);
  p_y_org = INTVAL(mw_origin_y_var -> instancevars -> __o_value);
  p_x_size = INTVAL(mw_size_x_var -> instancevars -> __o_value);
  p_y_size = INTVAL(mw_size_y_var -> instancevars -> __o_value);

  *x_org = (p_x_size / 2) - (self_width / 2) + p_x_org;
  /* vertical alignment is slightly above the window's 
     center */
  *y_org = (p_y_size / 2) - (self_height/ 2) + p_y_org -
    (p_y_size * .1);
  /* don't position the origin above the top of the display or left
     of the left edge */
  if (*x_org < 0) *x_org = 0;
  if (*y_org < 0) *y_org = 0;
}

extern GC create_pane_win_gc (Display *d, Window w, OBJECT *pane);

int __ctalkX11CreateDialogWindow (OBJECT *self_object) {
  Window win_id, root_return;
  XSetWindowAttributes set_attributes;
  XGCValues gcv;
  GC gc;
  OBJECT *displayPtr, *mainWinPtr, *mainWin, *titleStr;
  XWMHints wm_hints;
  XSizeHints *size_hints;
  char buf[MAXLABEL];
  static int wm_event_mask;
  int geom_ret, x_return, y_return;
  int pane_x, pane_y, pane_width, pane_height, border_width;
  unsigned int width_return, height_return, depth_return, border_width_return;
  int x_org, y_org, x_size, y_size;

  wm_event_mask = WM_CONFIGURE_EVENTS | WM_INPUT_EVENTS;

  if (open_dialog_display_connection () < 0) {
    return ERROR;
  } else {
    displayPtr = __ctalkGetInstanceVariable (self_object,
					     "displayPtr", TRUE);
    SYMVAL(displayPtr -> instancevars -> __o_value) = (uintptr_t)dpyrec -> d_p;
  }
  mainWinPtr = __ctalkGetInstanceVariable (self_object,
					   "mainWindowPtr", TRUE);
  mainWin = *(OBJECT **)mainWinPtr -> instancevars -> __o_value;
  
  titleStr = __ctalkPaneResource (self_object, "titleText", TRUE);

  border_width = __x11_pane_border_width (self_object);
  set_attributes.backing_store = Always;
  set_attributes.save_under = true;

  x_org = __pane_x_org (self_object);
  y_org = __pane_y_org (self_object);
  if (x_org == DIM_UNDEF || y_org == DIM_UNDEF) 
    dialog_xy_from_parent (self_object, mainWin, &x_org, &y_org);
  x_size = __pane_x_size (self_object);
  y_size = __pane_y_size (self_object);

  win_id = XCreateWindow (dpyrec -> d_p, dpyrec -> d_p_root, 
			  x_org, y_org, x_size, y_size,
			  border_width, dpyrec -> d_p_screen_depth,
			  CopyFromParent, CopyFromParent, 
			  CWBackingStore|CWSaveUnder,
			  &set_attributes);

  wm_hints.flags = (InputHint|StateHint);
  wm_hints.input = TRUE;
  wm_hints.initial_state = NormalState;
  XSetWMHints (dpyrec -> d_p, win_id, &wm_hints);
  
  size_hints = XAllocSizeHints ();
  size_hints -> x = x_org;
  size_hints -> y = y_org;
  size_hints -> base_width = x_size;
  size_hints -> base_height = y_size;
  size_hints -> flags = PWinGravity|USSize|USPosition;
  XSetWMNormalHints (dpyrec -> d_p, win_id, size_hints);
  XFree (size_hints);

  wm_delete_dialog = XInternAtom (dpyrec -> d_p, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (dpyrec -> d_p, win_id, &wm_delete_dialog, 1);

  XSetWindowBorder (dpyrec -> d_p, win_id, BlackPixel(dpyrec -> d_p, dpyrec -> d_p_screen));
  XSelectInput(dpyrec -> d_p, win_id, wm_event_mask);

  gc = create_pane_win_gc (dpyrec -> d_p, win_id, self_object);

  __xlib_set_wm_name_prop 
    (dpyrec -> d_p, win_id, gc, 
     titleStr -> instancevars -> __o_value);
#if 0
     basename_w_extent(__argvFileName ()));
#endif  

  __save_pane_to_vars (self_object, gc, win_id,
		       DefaultDepth (dpyrec -> d_p, DefaultScreen (dpyrec -> d_p)));
  dpyrec -> mapped = true;
  XMoveResizeWindow (dpyrec -> d_p, win_id, x_org, y_org, x_size, y_size);

  return win_id;
}

void __enable_dialog (OBJECT *self) {
  OBJECT *mainWinPtr, *mainWin, *self_xID;
  OBJECT *origin_var, *origin_x_var, *origin_y_var;
  Window self_win;
  int x_org, y_org;

  origin_var = __ctalkGetInstanceVariable (self, "origin", TRUE);
  origin_x_var = __ctalkGetInstanceVariable (origin_var, "x", TRUE);
  origin_y_var = __ctalkGetInstanceVariable (origin_var, "y", TRUE);
  self_xID = __ctalkGetInstanceVariable (self, "xWindowID", TRUE);
  self_win = INTVAL(self_xID -> instancevars -> __o_value);
  mainWinPtr = __ctalkGetInstanceVariable (self, "mainWindowPtr", TRUE);
  mainWin = *(OBJECT **)mainWinPtr -> instancevars -> __o_value;

  dialog_xy_from_parent (self, mainWin, &x_org, &y_org);

  INTVAL(origin_x_var -> instancevars -> __o_value) =
    INTVAL(origin_x_var -> __o_value) = x_org;
  INTVAL(origin_y_var -> instancevars -> __o_value) =
    INTVAL(origin_y_var -> __o_value) = y_org;

  dpyrec -> mapped = true;

  XMoveWindow (dpyrec -> d_p, self_win, x_org, y_org);

}

int __ctalkCloseX11DialogPane (OBJECT *self) {

  dpyrec -> mapped = false;

  return SUCCESS;
}

char **__ctalkIconXPM (int icon_id) {
  switch (icon_id) {
  case ICON_NONE:
    return (char **)NULL;
    break;
  case ICON_STOP:
    return stop64x64_xpm;
    break;
  case ICON_CAUTION:
    return warning64x64_xpm;
    break;
  case ICON_INFO:
    return info64x64_xpm;
    break;
  default:
    _warning ("ctalk: Unknown dialog icon ID: %d.\n", icon_id);
    return (char **)NULL;
    break;
  } 
  return (char **)NULL;
}

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

int __ctalkX11CreateDialogWindow (OBJECT *self_object) {
  x_support_error ();
  return 0;  /* notreached */
}
int __ctalkCloseX11DialogPane (OBJECT *self) {
  x_support_error ();
  return 0;  /* notreached */
}

void __enable_dialog (OBJECT *self) {
  x_support_error ();
}

char **__ctalkIconXPM (int icon_id) {
  x_support_error ();
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
