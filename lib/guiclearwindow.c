/* $Id: guiclearwindow.c,v 1.2 2020/12/14 13:20:16 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2017-2020  Robert Kiesling, rk3314042@gmail.com.
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

extern Display *display;   /* Defined in x11lib.c. */
extern char *shm_mem;
extern int mem_id;

extern int __xlib_clear_window (Display *, Window, GC);

#if X11LIB_FRAME
int __ctalkX11PaneClearWindow (OBJECT *self_object) {
  return SUCCESS;
}
int __ctalkGUIPaneClearWindow (OBJECT *self_object) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */
int __ctalkX11PaneClearWindow (OBJECT *self) {
  return __ctalkGUIPaneClearWindow (self);
}
int __ctalkGUIPaneClearWindow (OBJECT *self) {
  OBJECT *self_object, *win_id_value, *gc_value,
    *displayPtr_var;
  Display *l_d;
  Window win_id;
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  win_id = INTVAL(win_id_value -> __o_value);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  displayPtr_var = __ctalkGetInstanceVariable (self_object,
					       "displayPtr", TRUE);
  if (dialog_dpy ()) {
    l_d = (Display *)SYMVAL(displayPtr_var -> instancevars -> __o_value);
    win_id = INTVAL(win_id_value -> __o_value);
    __xlib_clear_window (l_d, win_id, (GC)SYMVAL(gc_value -> __o_value));
  } else {
    make_req (shm_mem, SYMVAL(displayPtr_var -> instancevars -> __o_value),
	      PANE_CLEAR_WINDOW_REQUEST,
	      INTVAL(win_id_value ->  __o_value),
	      SYMVAL(gc_value -> __o_value), "");
    wait_req (shm_mem);
  }

  return SUCCESS;
}
#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
static void __gui_support_error (void) {
  x_support_error ();
}
int __ctalkX11PaneClearWindow (OBJECT *self_object) {
  x_support_error ();
  return 0;  /* notreached */
}
int __ctalkGUIPaneClearWindow (OBJECT *self_object) {
  x_support_error ();
  return 0; /* notreached */
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
