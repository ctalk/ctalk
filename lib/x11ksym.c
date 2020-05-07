/* $Id: x11ksym.c,v 1.3 2020/05/07 21:36:01 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2019  Robert Kiesling, rk3314042@gmail.com.
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
#include <ctype.h>
#include <stdbool.h>

/* From ctalk.h */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !(FALSE)
#endif
#define ERROR -1

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include "x11defs.h"

extern Display *display;   /* Declared in x11lib.c. */

int shift = 0;
int ctrl = 0;
int alt = 0;
int shiftlock = 0;
int capslock = 0;
int meta_state = 0;

int ascii_shift_keysym (unsigned long int keysym) {
  int keysym_1;
  keysym_1 = keysym;
  if (!shift && !shiftlock && !capslock) {
    if (isalpha ((int)keysym & 0xff)) { 
      keysym_1 = tolower ((int)keysym & 0xff);
    }
  }
  /*
   *  Everything else....
   */
  if (shift || shiftlock || capslock) {
    switch ((char)keysym & 0xff) 
      {
      case 48:  /* 0 ... */
	keysym_1 = ')';
	break;
      case 49: 
	keysym_1 = '!';
	break;
      case 50: 
	keysym_1 = '@';
	break;
      case 51: 
	keysym_1 = '#';
	break;
      case 52: 
	keysym_1 = '$';
	break;
      case 53: 
	keysym_1 = '%';
	break;
      case 54: 
	keysym_1 = '^';
	break;
      case 55: 
	keysym_1 = '&';
	break;
      case 56: 
	keysym_1 = '*';
	break;
      case 57: 
	keysym_1 = '(';
	break;
      case 45:   /* - */
	keysym_1 = '_';
	break;
      case 61:   /* = */
	keysym_1 = '+';
	break;
      case 92:   /* \ */
	keysym_1 = '|';
	break;
      case 96:   /* ` */
	keysym_1 = '~';
	break;
      case 91:   /* [ */
	keysym_1 = '{';
	break;
      case 93:   /* ] */
	keysym_1 = '}';
	break;
      case 59:   /* ; */
	keysym_1 = ':';
	break;
      case 39:   /* ' */
	keysym_1 = '\"';
	break;
      case 44:   /* , */
	keysym_1 = '<';
	break;
      case 46:   /* . */
	keysym_1 = '>';
	break;
      case 47:   /* / */
	keysym_1 = '?';
	break;
      }
  }
  return keysym_1;
}

int ascii_ctrl_keysym (unsigned long int keysym) {
  int keysym_1;
  keysym_1 = keysym;
  if (keysym_1 == XK_Delete) {
    keysym_1 = 127;
  } else {
    if (ctrl) {
      switch ((char) keysym_1 & 0xff) {
      case '[':
	keysym_1 = 27;
	break;
      case '\\':
	keysym_1 = 28;
	break;
      case ']':
	keysym_1 = 29;
	break;
      case '^':
	keysym_1 = 30;
	break;
      case '_':
	keysym_1 = 31;
	break;
      default:
	if (isupper ((int)keysym_1 & 0xff))
	  keysym_1 -= 64;
	if (islower ((int)keysym_1 & 0xff))
	  keysym_1 -= 96;
	break;
      }
    }
  }
  return keysym_1;
}

int get_x11_keysym (int keycode, int shift_state, bool keypress) {
  KeySym *keysyms;
  int n_keysyms;
  unsigned long int keysym0;

  keysyms = XGetKeyboardMapping (display, keycode,
				 1, &n_keysyms);
  keysym0 = keysyms[0];
  XFree (keysyms);
  if (IS_XK_MODIFIER(keysym0)) {
    switch  (keysym0)
      {
      case XK_Shift_L:
      case XK_Shift_R:
	shift = (keypress) ? TRUE : FALSE;
	break;
      case XK_Control_L:
      case XK_Control_R:
	ctrl = (keypress) ? TRUE : FALSE;
	break;
      case XK_Alt_L:
      case XK_Alt_R:
	alt = (keypress) ? TRUE : FALSE;
	break;
      case XK_Meta_L:
      case XK_Meta_R:
	meta_state = (keypress) ? TRUE : FALSE;
	break;
      case XK_Shift_Lock:
	shiftlock = (keypress) ? TRUE : FALSE;
	break;
      case XK_Caps_Lock:
	capslock = (keypress) ? TRUE : FALSE;
	break;
      default:
	break;
      }
  } else {
    switch (keysym0)
      {
      case  XK_Home:
      case  XK_Left:
      case  XK_Up:
      case  XK_Right:
      case  XK_Down:
      case  XK_Page_Up:
      case  XK_Page_Down:
      case  XK_End:
      case  XK_Begin:
	return keysym0;
	break;
      }
  }
  keysym0 = ascii_shift_keysym (keysym0);
  keysym0 = ascii_ctrl_keysym (keysym0);
  return keysym0;
}

/* For case where we need to use the display pointer of a different
   thread. */
int get_x11_keysym_2 (Display *d_param,
		      int keycode, int shift_state, int keypress) {
  KeySym *keysyms;
  int n_keysyms, i;
  unsigned long int keysym0;

  keysyms = XGetKeyboardMapping (d_param, keycode,
				 1, &n_keysyms);
  keysym0 = keysyms[0];
  XFree (keysyms);
  if (IS_XK_MODIFIER(keysym0)) {
    switch  (keysym0)
      {
      case XK_Shift_L:
      case XK_Shift_R:
	shift = (keypress) ? TRUE : FALSE;
	break;
      case XK_Control_L:
      case XK_Control_R:
	ctrl = (keypress) ? TRUE : FALSE;
	break;
      case XK_Alt_L:
      case XK_Alt_R:
	alt = (keypress) ? TRUE : FALSE;
	break;
      case XK_Meta_L:
      case XK_Meta_R:
	meta_state = (keypress) ? TRUE : FALSE;
	break;
      case XK_Shift_Lock:
	shiftlock = (keypress) ? TRUE : FALSE;
	break;
      case XK_Caps_Lock:
	capslock = (keypress) ? TRUE : FALSE;
	break;
      default:
	break;
      }
  } else {
    switch (keysym0)
      {
      case  XK_Home:
      case  XK_Left:
      case  XK_Up:
      case  XK_Right:
      case  XK_Down:
      case  XK_Page_Up:
      case  XK_Page_Down:
      case  XK_End:
      case  XK_Begin:
	return keysym0;
	break;
      }
  }
  keysym0 = ascii_shift_keysym (keysym0);
  keysym0 = ascii_ctrl_keysym (keysym0);
  return keysym0;
}

int __ctalkGetX11KeySym (int keycode, int shift_state,
		     int keypress) {
  return get_x11_keysym (keycode, shift_state, keypress);
}

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

void x_support_error (void);

int ascii_shift_keysym (unsigned long int keysym) {
  x_support_error (); return ERROR;
}
int ascii_ctrl_keysym (unsigned long int keysym) {
  x_support_error (); return ERROR;
}
int get_x11_keysym (int keycode, int shift_state, int keypress) {
  x_support_error (); return ERROR;
}
int __ctalkGetX11KeySym (int keycode, int shift_state,
			 int keypress) {
  x_support_error ();return ERROR;
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
