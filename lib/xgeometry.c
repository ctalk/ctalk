/* $Id: xgeometry.c,v 1.15 2020/03/29 00:06:48 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2014, 2018, 2019  Robert Kiesling, rk3314042@gmail.com.
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

extern Display *display;   /* Defined in x11lib.c. */

#if X11LIB_FRAME
int __ctalkX11ParseGeometry (char *s, int *x, int *y, int *w, int *h) {
  return SUCCESS;
}
int __ctalkX11SetSizeHints (int x, int y, int width, int height,
			    int geomflags) {
  return SUCCESS;
}
void __ctalkX11GetSizeHints (int, int *x, int *y, int *width, int *height, 
			     int *win_gravity, int *flags) {
}

void __ctalkX11FreeSizeHints (void) {
}

void __ctalkX11SubWindowGeometry (OBJECT *parentpane, char *geomspec
				  int *x_out, int *y_out,
				  int *width_out, int *height_out);

#else /* X11LIB_FRAME */

static int strtol_geom_error (char *s) {
  int i;
  errno = 0;
  i = strtol (s, NULL, 10);
  if (errno)
    return -1;
  else
    return i;
}

/*
 *  Parse a subpane geometry string, and return the individual dimensions
 *  in x_out, y_out, width_out, and height_out.  Subpane geometries have
 *  the form:
 *
 *    width[%]xheight[%]+x[%]+y[%]
 *
 *  The dimensions are given in pixels, unless a percent sign follows
 *  a dimension.  In that case the dimension is calculated as a fractional
 *  percentage of the parent pane's width or height.  Geometry strings may
 *  contain a combination of absolute and relative dimensions.
 */

#define DIMENBUFSIZE 0xff

void __ctalkX11SubWindowGeometry (OBJECT *parentpane, char *geomspec,
				  int *x_out, int *y_out,
				  int *width_out, int *height_out) {
  char *p, *q, *r, xbuf[DIMENBUFSIZE], ybuf[DIMENBUFSIZE], widthbuf[DIMENBUFSIZE], heightbuf[DIMENBUFSIZE];
  OBJECT *pt, *var, *subvar;
  int pct;

  if (parentpane -> attrs & OBJECT_IS_VALUE_VAR)
    pt = parentpane -> __o_p_obj;
  else
    pt = parentpane;

  if ((r = strchr (geomspec, '+')) == NULL) {
    /* width and height only */

    *x_out = DIM_UNDEF;
    *y_out = DIM_UNDEF;

      p = geomspec;
    if ((r = strchr (geomspec, '%')) == NULL) {
      /* absolute dimensions only */

      if ((q = strchr (p, 'x')) == NULL)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
      memset (widthbuf, 0, DIMENBUFSIZE);
      strncpy (widthbuf, p, q - p);
      if ((*width_out = strtol_geom_error (widthbuf)) < 0)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
    
      ++q; p = q;
      memset (heightbuf, 0, DIMENBUFSIZE);
      strcpy (heightbuf, p);
      if ((*height_out = strtol_geom_error (heightbuf)) < 0)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);

    } else {
    }
  } else { /* if ((r = strchr (geomspec, '+')) == NULL) */

    if ((r = strchr (geomspec, '%')) == NULL) {
      /* absolute dimensions only */

      p = geomspec;
      if ((q = strchr (p, 'x')) == NULL)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
      memset (widthbuf, 0, DIMENBUFSIZE);
      strncpy (widthbuf, p, q - p);
      if ((*width_out = strtol_geom_error (widthbuf)) < 0)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
    
      ++q; p = q;
      if ((q = strchr (p, '+')) == NULL)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
      memset (heightbuf, 0, DIMENBUFSIZE);
      strcpy (heightbuf, p);
      if ((*height_out = strtol_geom_error (heightbuf)) < 0)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);

      ++q; p = q;
      if ((q = strchr (p, '+')) == NULL)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
      memset (xbuf, 0, DIMENBUFSIZE);
      strncpy (xbuf, p, q - p);
      if ((*x_out = strtol_geom_error (xbuf)) < 0)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);

      ++q;
      memset (ybuf, 0, DIMENBUFSIZE);
      strcpy (ybuf, q);
      if ((*y_out = strtol_geom_error (ybuf)) < 0)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);

    } else { /*   if ((r = strchr (geomspec, '%')) == NULL) { */
      p = geomspec;
      if ((q = strchr (p, 'x')) == NULL)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
      memset (widthbuf, 0, DIMENBUFSIZE);
      strncpy (widthbuf, p, q - p);
      if ((r = strchr (widthbuf, '%')) == NULL) {
	if ((*width_out = strtol_geom_error (widthbuf)) < 0)
	  _error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		  geomspec);
      } else {
	var = __ctalkGetInstanceVariable (pt, "size", TRUE);
	subvar = __ctalkGetInstanceVariable (var, "x", TRUE);
	widthbuf[strlen (widthbuf) - 1] = 0;
	if ((pct = strtol_geom_error (widthbuf)) < 0)
	  _error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		  geomspec);
	*width_out = (INTVAL(subvar -> __o_value) * pct) / 100;
      }

      ++q; p = q;
      if ((q = strchr (p, '+')) == NULL)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
      memset (heightbuf, 0, DIMENBUFSIZE);
      strncpy (heightbuf, p, q - p);
      if ((r = strchr (heightbuf, '%')) == NULL) {
	if ((*height_out = strtol_geom_error (heightbuf)) < 0)
	  _error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		  geomspec);
      } else {
	var = __ctalkGetInstanceVariable (pt, "size", TRUE);
	subvar = __ctalkGetInstanceVariable (var, "y", TRUE);
	heightbuf[strlen (heightbuf) - 1] = 0;
	if ((pct = strtol_geom_error (heightbuf)) < 0)
	  _error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		  geomspec);
	*height_out = (INTVAL(subvar -> __o_value) * pct) / 100;
      }

      ++q; p = q;
      if ((q = strchr (p, '+')) == NULL)
	_error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		geomspec);
      memset (xbuf, 0, DIMENBUFSIZE);
      strncpy (xbuf, p, q - p);
      if ((r = strchr (xbuf, '%')) == NULL) {
	if ((*x_out = strtol_geom_error (xbuf)) < 0)
	  _error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		  geomspec);
      } else {
	var = __ctalkGetInstanceVariable (pt, "size", TRUE);
	subvar = __ctalkGetInstanceVariable (var, "x", TRUE);
	xbuf[strlen (xbuf) - 1] = 0;
	if ((pct = strtol_geom_error (xbuf)) < 0)
	  _error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		  geomspec);
	*x_out = (INTVAL(subvar -> __o_value) * pct) / 100;
      }
      
      p = ++q;
      strcpy (ybuf, p);
      if ((r = strchr (ybuf, '%')) == NULL) {
	if ((*y_out = strtol_geom_error (ybuf)) < 0)
	  _error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		  geomspec);
      } else {
	var = __ctalkGetInstanceVariable (pt, "size", TRUE);
	subvar = __ctalkGetInstanceVariable (var, "y", TRUE);
	ybuf[strlen (ybuf) - 1] = 0;
	if ((pct = strtol_geom_error (ybuf)) < 0)
	  _error ("ctalk: bad geometry specification: \"%s.\" Exiting.\n",
		  geomspec);
	*y_out = (INTVAL(subvar -> __o_value) * pct) / 100;
      }
    }
  } /* if ((r = strchr (geomspec, '+')) == NULL) */
}

int __ctalkX11ParseGeometry (char *geometry_string, int *x, int *y, 
			int *width, int *height) {
  int x_return, y_return, flags;
  unsigned int width_return, height_return;

  flags = XParseGeometry (geometry_string, 
			  &x_return, &y_return, &width_return, &height_return);

  if (flags & XValue) *x = x_return; else *x = 0;
  if (flags & YValue) *y = y_return; else *y = 0;
  if (flags & WidthValue) *width = width_return; else *width = 0;
  if (flags & HeightValue) 
    *height = height_return; 
  else
    *height = 0;

  return flags;
}

int __geomFlags = -1;
XSizeHints *size_hints = NULL;
XWMHints *wm_hints = NULL;

int __ctalkX11SetSizeHints (int x, int y, int width, int height,
			    int geomflags) {
  size_hints = XAllocSizeHints ();
  memset (size_hints, 0, sizeof (XSizeHints));

  __geomFlags = geomflags;

  size_hints -> win_gravity = NorthWestGravity;
  if (geomflags & XValue) {
    if (geomflags & XNegative) {
      size_hints -> win_gravity = NorthEastGravity;
    }
    size_hints -> x = x;
  } else {
    size_hints -> x = 0;
  }

  if (geomflags & YValue) {
    if (geomflags & YNegative) {
      if (size_hints -> win_gravity == NorthEastGravity) {
	size_hints -> win_gravity = SouthEastGravity;
      } else {
	size_hints -> win_gravity = SouthWestGravity;
      }
    }
    size_hints -> y = y;
  } else {
    size_hints -> y = 0;
  }

  if (width == 0) 
    size_hints -> width = size_hints -> base_width = DEFAULT_WIDTH;
  else
    size_hints -> width = size_hints -> base_width = width;
  if (height == 0)
    size_hints -> height = size_hints -> base_height = DEFAULT_HEIGHT;
  else
    size_hints -> height = size_hints -> base_height = height;

  if (!XValue || !YValue)
    size_hints -> flags = PWinGravity|USSize|USPosition;
  else
    size_hints -> flags = PWinGravity|USSize|PPosition;

  return SUCCESS;
}

void __ctalkX11GetSizeHints (int win_id, int *x, int *y, 
			     int *width, int *height, 
			     int *win_gravity, int *flags) {
  XSizeHints hints_return;
  if (size_hints) {
    if (display) {
      XGetWMNormalHints (display, (Window)win_id, &hints_return, 
			 (long int *)size_hints);
      *x = hints_return.x;
      *y = hints_return.y;
      if (hints_return.flags & PBaseSize) {
	*width = hints_return.base_width;
	*height = hints_return.base_height;
      } else if (hints_return.flags & USSize) {
	*width = hints_return.width;
	*height = hints_return.height;
      }
      *win_gravity = hints_return.win_gravity;
      *flags = hints_return.flags;
    } else {
      *x = size_hints -> x;
      *y = size_hints -> y;
      *width = size_hints -> base_width;
      *height = size_hints -> base_height;
      *win_gravity = size_hints -> win_gravity;
      *flags = size_hints -> flags;
    }
  }

}

/* weighted to the upper half, left 2/3 of the display */
static int rand_win_x (void) {
  int display_width = DisplayWidth (display, DefaultScreen (display));
  srand (time (NULL));
  return rand () % display_width * 0.6;
}
static int rand_win_y (void) {
  int display_height = DisplayHeight (display, DefaultScreen (display));
  srand (time (NULL));
  return rand () % display_height / 2;
}

/* Called by __ctalkCreateX11MainWindow and
   __ctalkCreateGLXMainWindow. */
void set_size_hints_internal (OBJECT *pane_object, int *x_org_out,
			      int *y_org_out, int *x_size_out,
			      int *y_size_out) {
  int pane_x, pane_y, pane_width, pane_height;
  if (__geomFlags <= 0) {
    size_hints = XAllocSizeHints ();
    pane_x = __pane_x_org (pane_object);
    pane_y = __pane_y_org (pane_object);
    pane_width = __pane_x_size (pane_object);
    pane_height = __pane_y_size (pane_object);
    size_hints -> x = (pane_x) ? pane_x : 0;
    size_hints -> y = (pane_y) ? pane_y : 0;
    size_hints -> width = size_hints -> base_width = (pane_width) ? pane_width : 250;
    size_hints -> height = size_hints -> base_height = (pane_height) ? pane_height : 250;
    if (!pane_x || !pane_y) {
      size_hints -> flags = USSize|USPosition;
      size_hints -> x = rand_win_x ();
      size_hints -> y = rand_win_y ();
    } else {
      size_hints -> flags = USSize|USPosition;
    }
    *x_org_out = size_hints -> x;
    *y_org_out = size_hints -> y;
    *x_size_out = size_hints -> base_width;
    *y_size_out = size_hints -> base_height;
  } else {
    if (size_hints) {
      *x_org_out = size_hints -> x;
      *y_org_out = size_hints -> y;
      *x_size_out = size_hints -> base_width;
      *y_size_out = size_hints -> base_height;
    } else {
      *x_org_out = __pane_x_org (pane_object);
      *y_org_out = __pane_y_org (pane_object);
      *x_size_out = __pane_x_size (pane_object);
      *y_size_out = __pane_y_size (pane_object);
    }
    if (__geomFlags & XNegative) {
      *x_org_out = DisplayWidth (display, DefaultScreen (display)) 
       	+ *x_org_out - *x_size_out;
      size_hints -> x = *x_org_out;
    }
    if (__geomFlags & YNegative) {
      *y_org_out = DisplayHeight (display, DefaultScreen (display)) 
       	+ *y_org_out - *y_size_out;
      size_hints -> y = *y_org_out;
    }
  }
}

void __ctalkX11FreeSizeHints (void) {
  if (size_hints)
    XFree (size_hints);
}
#endif /* X11LIB_FRAME */
#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

static void __gui_support_error (void) {
  x_support_error ();
}

int __ctalkX11ParseGeometry (char *s, int *x, int *y, int *w, int *h) {
  x_support_error (); return ERROR;
}
int __ctalkX11SetSizeHints (int x, int y, int width, int height,
			    int geomflags) {
  x_support_error (); return ERROR;
}
void __ctalkX11GetSizeHints (int win_id, int *x, int *y, int *width, 
			     int *height, 
			     int *win_gravity, int *flags) {
  x_support_error ();
}

void __ctalkX11FreeSizeHints (void) {
  x_support_error ();
}

void __ctalkX11SubWindowGeometry (OBJECT *parentpane, char *geomspec,
				  int *x_out, int *y_out,
				  int *width_out, int *height_out) {
    x_support_error ();
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
