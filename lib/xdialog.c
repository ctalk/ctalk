/* $Id: xdialog.c,v 1.1 2020/02/27 18:37:37 rkiesling Exp $ -*-c-*-*/

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

#if 0 /***/
extern Display *display;   /* Defined in x11lib.c. */
extern char *shm_mem;
extern int mem_id;
#endif

#if X11LIB_FRAME

#if  0 /***/
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
  OBJECT *self_object, *win_id_value, *gc_value;
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  make_req (shm_mem, PANE_CLEAR_WINDOW_REQUEST,
	    INTVAL(win_id_value ->  __o_value),
	    SYMVAL(gc_value -> __o_value), "");
  wait_req (shm_mem);
  return SUCCESS;
}

#endif /* #if 0 */

#endif /* X11LIB_FRAME */

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

#if 0 /***/

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

#endif /* #if 0 */

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
