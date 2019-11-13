/* $Id: xrender.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2016  Robert Kiesling, rk3314042@gmail.com.
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
 *  xrender.c - routines for which we use the XRender extension
 *  (except for the Xft font routines, which has its own file, too).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)

#if defined (HAVE_XFT_H) && defined (HAVE_XRENDER_H)
#include "x11defs.h"
#include "xrender.h"
#else
#include "x11defs.h"
#endif

extern Display *display;   /* Defined in x11lib.c. */
extern int display_width, display_height;

#if X11LIB_FRAME

int __xlib_draw_line (Drawable drawable_arg, GC gc, char *data) {
  return SUCCESS;
}

int __xlib_draw_circle (Drawable drawable_arg, GC gc, char *data) {
  return SUCCESS;
}

int __xlib_draw_point (Drawable drawable_arg, GC gc, char *data) {
  return SUCCESS;
}
void __ctalkX11UseXRender (bool b) {
  return;
}

bool __ctalkX11UsingXRender (void) {
  return false;
}

#else /* X11LIB_FRAME */

/* These are in x11lib.c. */
extern int lookup_pixel (char *);
extern int lookup_rgb (char *, unsigned short int *,
		       unsigned short int *, unsigned short int *);

#ifdef GC_RANGE_CHECK

bool gc_check (GC *);

#endif

#define LINE_GCV_MASK (GCFunction|GCForeground|GCFillStyle|GCLineWidth) 

static bool select_xrender = true;

static bool have_useful_xrender = false;
int xrender_event_base = -1, xrender_error_base = -1;

void __ctalkX11UseXRender (bool b) {
  select_xrender = b;
}

bool __ctalkX11UsingXRender (void) {
  return have_useful_xrender;
}

static int line_data_error (char *data) {
#ifndef WITHOUT_X11_WARNINGS
  printf ("__xlib_draw_line: Invalid message %s.\n", data);
  printf ("__xlib_draw_line: (To disable these messages, build Ctalk with\n");
  printf ("__xlib_draw_line: the --without-x11-warnings option.)\n");
#endif
  return ERROR;
}

static int __xlib_draw_line_scan (char *data, 
				  int *line_start_x, 
				  int *line_start_y, 
				  int *line_end_x,
				  int *line_end_y,
				  int *pen_width,
				  unsigned short int *pen_alpha,
				  int *panebuffer_id, 
				  int *panebackingstore_id,
				  char **color_name) {
  /* if (strchr (data, '|')) {
    printf ("__xlib_draw_line_scan: %s\n", data);
    return ERROR;
    } */
  char *p, *q;
  p = data;
  q = strchr (p, ':');
  if (!q) { return line_data_error (data); }
  errno = 0;
  *line_start_x = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_line", data); return ERROR;
  }
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return line_data_error (data); }
  *line_start_y = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_line", data); return ERROR;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return line_data_error (data); }
  *line_end_x = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_line", data); return ERROR;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return line_data_error (data); }
  *line_end_y = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_line", data); return ERROR;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {  return line_data_error (data); }
  *pen_width = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_line", data); return ERROR;
  }

  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return line_data_error (data); }
  *pen_alpha = strtol (p, &q, 0);
  if (errno) {
    strtol_error (errno, "__xlib_draw_line", data); return ERROR;
  }

  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return line_data_error (data); }
  *panebuffer_id = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_line", data); return ERROR;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return line_data_error (data); }
  *panebackingstore_id = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_line", data); return ERROR;
  }
    
  *color_name = ++q;
  return SUCCESS;
}

#define CIRCLE_GCV (GCForeground)

static int circle_data_error (char *data) {
#ifndef WITHOUT_X11_WARNINGS
  printf ("__xlib_draw_circle: Invalid message %s.\n", data);
  printf ("__xlib_draw_circle: (To disable these messages, build Ctalk with\n");
  printf ("__xlib_draw_circle: the --without-x11-warnings option.)\n");
#endif
  return ERROR;
}

static int __xlib_draw_circle_scan (char *data, 
				   int *center_x, 
				   int *center_y, 
				   int *radius,
				     int *fill, 
				     int *pen_width,
				     unsigned short int *alpha,
				     char **color_name,
				     char **bg_color_name) {
  char *p, *q;
  p = data;
  q = strchr (p, ':');
  if (!q) { return circle_data_error (data); }
  errno = 0;
  *center_x = strtol (p, &q, 10);
  if (errno) {
    return ERROR;
  }
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return circle_data_error (data); }
  *center_y = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_circle_scan", data);
    return ERROR;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return circle_data_error (data); }
  *radius = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_circle_scan", data);
    return ERROR;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return circle_data_error (data); }
  *fill = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_circle_scan", data);
    return ERROR;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return circle_data_error (data); }
  *pen_width = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_circle_scan", data);
    return ERROR;
  }

  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) { return circle_data_error (data); }
  *alpha = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_draw_circle_scan", data);
    return ERROR;
  }

  *color_name = ++q;
  
  q = strchr (*color_name, ':');
  if (!q) { return circle_data_error (data); }
  *q = 0;
  *bg_color_name = ++q;
  return SUCCESS;
}

#define POINT_GCV_MASK (GCFunction | GCForeground | GCFillStyle) 

static void __xlib_draw_point_scan (char *data, 
				    int *center_x, int *center_y, 
				    int *pen_width,
				    unsigned short *pen_alpha,
				    int *panebuffer_id, 
				    int *panebackingstore_id,
				    char **color_name) {
  char *p, *q;
  p = data;
  q = strchr (p, ':');
  if (!q) {
    fprintf (stderr, "__xlib_draw_point (): Invalid message %s.\n", data);
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
  errno = 0;
  *center_x = strtol (p, &q, 10);
  if (errno) {
#ifndef WITHOUT_X11_WARNINGS
    strtol_error (errno, "__xlib_draw_point", data);
    printf ("__xlib_draw_point: (To disable these messages, build Ctalk with\n");
    printf ("__xlib_draw_point: the --without-x11-warnings option.)\n");
#endif
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    fprintf (stderr, "__xlib_draw_point (): Invalid message %s.\n", data);
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
  *center_y = strtol (p, &q, 10);
  if (errno) {
#ifndef WITHOUT_X11_WARNINGS
    strtol_error (errno, "__xlib_draw_point", data);
    printf ("__xlib_draw_point: (To disable these messages, build Ctalk with\n");
    printf ("__xlib_draw_point: the --without-x11-warnings option.)\n");
#endif
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    fprintf (stderr, "__xlib_draw_point (): Invalid message %s.\n", data);
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
  *pen_width = strtol (p, &q, 10);
  if (errno) {
#ifndef WITHOUT_X11_WARNINGS
    strtol_error (errno, "__xlib_draw_point", data);
    printf ("__xlib_draw_point: (To disable these messages, build Ctalk with\n");
    printf ("__xlib_draw_point: the --without-x11-warnings option.)\n");
#endif
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    fprintf (stderr, "__xlib_draw_point (): Invalid message %s.\n", data);
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
  *pen_alpha = strtol (p, &q, 10);
  if (errno) {
#ifndef WITHOUT_X11_WARNINGS
    strtol_error (errno, "__xlib_draw_point", data);
    printf ("__xlib_draw_point: (To disable these messages, build Ctalk with\n");
    printf ("__xlib_draw_point: the --without-x11-warnings option.)\n");
#endif
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    fprintf (stderr, "__xlib_draw_point (): Invalid message %s.\n", data);
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
  *panebuffer_id = strtol (p, &q, 10);
  if (errno) {
#ifndef WITHOUT_X11_WARNINGS
    strtol_error (errno, "__xlib_draw_point", data);
    printf ("__xlib_draw_point: (To disable these messages, build Ctalk with\n");
    printf ("__xlib_draw_point: the --without-x11-warnings option.)\n");
#endif
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    fprintf (stderr, "__xlib_draw_point (): Invalid message %s.\n", data);
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
  *panebackingstore_id = strtol (p, &q, 10);
  if (errno) {
#ifndef WITHOUT_X11_WARNINGS
    strtol_error (errno, "__xlib_draw_point", data);
    printf ("__xlib_draw_point: (To disable these messages, build Ctalk with\n");
    printf ("__xlib_draw_point: the --without-x11-warnings option.)\n");
#endif
    *center_x = *center_y = *pen_width = *panebuffer_id = 
      *panebackingstore_id = **color_name = 0;
    return;
  }
    
  *color_name = ++q;
}

extern int lookup_pixel (char *color);

static double degree_to_radian (double d) {
  double radians;
  radians = d * (3.1416 / 180);
  return radians;
}

static double opp_side (double deg, double hyp) {
  return hyp * sin(degree_to_radian (deg));
}
static double adj_side (double deg, double hyp) {
  return hyp * cos(degree_to_radian (deg));
}

#if defined (HAVE_XRENDER_H) && defined (HAVE_XFT_H)

bool xrender_version_check (void) {
  int major_version, minor_version;

  if (select_xrender) {
    if (XRenderQueryExtension (display, &xrender_event_base, 
			       &xrender_error_base)) {
      XRenderQueryVersion (display, &major_version, &minor_version);
      if ((major_version > 0) ||
	  (major_version == 0 && minor_version >= 10)) {
	have_useful_xrender = true;
	return true;
      }
    }
  }
  have_useful_xrender = false;
  return false;

}

static void clear_draw_rec (XRENDERDRAWREC *r) {
  if (r -> draw != NULL) {
    XftDrawDestroy (r -> draw);
    r -> draw = NULL;
  }
  if (r -> fill_picture != 0) {
    XRenderFreePicture (display, r -> fill_picture);
    r -> fill_picture = 0;
  }
  if (r -> picture != 0) {
    XRenderFreePicture (display, r -> picture);
    r -> picture = 0;
  }
  r -> drawable = 0;
}

HASHTAB color_index_hash = NULL;

XRNAMEDCOLOR colors[MAXXRCOLORS+1];
int color_ptr = 0;
int last_color = -1;

void save_color (char *colorname, XftColor *rcolor) {
  char s_color_ptr[16];
  if (color_ptr > MAXXRCOLORS)
    return;
  strcpy (colors[color_ptr].name, colorname);
  colors[color_ptr].pixel = rcolor -> pixel;
  colors[color_ptr].xrcolor.red  = rcolor -> color.red;
  colors[color_ptr].xrcolor.green  = rcolor -> color.green;
  colors[color_ptr].xrcolor.blue  = rcolor -> color.blue;
  colors[color_ptr].xrcolor.alpha  = rcolor -> color.alpha;

  if (color_index_hash == NULL)
    _new_hash (&color_index_hash);
  ctitoa (color_ptr, s_color_ptr);
  _hash_put (color_index_hash, strdup (s_color_ptr), colorname);
  ++color_ptr;
}

bool get_color (char *colorname, XftColor *color_out) {
  int i;
  char *p_color_index;

  p_color_index = (char *)_hash_get (color_index_hash, colorname);
  if (p_color_index != NULL) {
    errno = 0;
    i = strtol (p_color_index, NULL, 10);
    if (!errno) {
      color_out -> pixel = colors[i].pixel;
      color_out -> color.red = colors[i].xrcolor.red;
      color_out -> color.green = colors[i].xrcolor.green;
      color_out -> color.blue = colors[i].xrcolor.blue;
      color_out -> color.alpha = colors[i].xrcolor.alpha;
      last_color = i;
      return true;
    } else {
      strtol_error (errno, "get_color", p_color_index);
    }
  }

  color_out -> pixel = BlackPixel (display, DefaultScreen (display));
  color_out -> color.red = 0xffff; color_out -> color.blue = 0xffff;
  color_out -> color.green = 0xffff; color_out -> color.alpha = 0xffff;
  last_color = -1;
  return false;
}

XRENDERDRAWREC surface = {0, NULL, 0, 0, NULL, NULL, NULL};

int __xlib_draw_line (Drawable drawable_arg, GC gc, char *data) {
  XGCValues line_gcv, old_gcv;
  int line_start_x, line_start_y, line_end_x, line_end_y, pen_width;
  int panebuffer_id, panebackingstore_id, actual_drawable;
  int r;
  char *colorname;

  XftColor rcolor;
  XPointDouble poly[4];
  double pen_half_width;
  unsigned short int pen_alpha;
  int dx, dy;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if (__xlib_draw_line_scan (data, &line_start_x, &line_start_y,
			     &line_end_x, &line_end_y, &pen_width,
			     &pen_alpha, 
			     &panebuffer_id, &panebackingstore_id,
			     &colorname) < 0)
    return ERROR;

  if (!panebuffer_id && !panebackingstore_id) {
    actual_drawable = drawable_arg;
  } else {
    actual_drawable = panebuffer_id;
  }

  if (have_useful_xrender) {

    if (actual_drawable != surface.drawable) {
      surface.draw = XftDrawCreate (display, actual_drawable,
			    DefaultVisual (display,
					   DefaultScreen (display)),
			    DefaultColormap (display,
					     (DefaultScreen (display))));
      surface.picture = XftDrawPicture (surface.draw);
      surface.mask_format = XRenderFindStandardFormat (display, PictStandardA8);
      surface.drawable = actual_drawable;
    }
    if (!get_color (colorname, &rcolor)) {
      if (XftColorAllocName (display, 
			     DefaultVisual (display, DefaultScreen (display)),
			     DefaultColormap (display, DefaultScreen (display)),
			     colorname, 
			     &rcolor)) {
	rcolor.color.alpha = pen_alpha;
	save_color (colorname, &rcolor);
      } else {
	return ERROR;
      }
    }
    if (rcolor.color.alpha != pen_alpha)
      rcolor.color.alpha = pen_alpha;
    surface.fill_picture = XftDrawSrcPicture (surface.draw, &rcolor);
    if (surface.fill_picture == 0) {
      surface.drawable = 0;
      return ERROR;
    }

    pen_half_width = ((double)pen_width / 2.0);
    if (pen_half_width < 1.0)
      pen_half_width = 1.0;

    dx = line_start_x - line_end_x;
    dy = line_start_y - line_end_y;

    /* cheap but effective... :) */
    if (abs(dx) > abs(dy)) {
      poly[0].x = ((double)line_start_x);
      poly[0].y = ((double)line_start_y) + pen_half_width;
      poly[1].x = ((double)line_end_x);
      poly[1].y = ((double)line_end_y) + pen_half_width;
      poly[2].x = ((double)line_end_x);
      poly[2].y = ((double)line_end_y) - pen_half_width;
      poly[3].x = ((double)line_start_x);
      poly[3].y = ((double)line_start_y) - pen_half_width;
    } else {
      poly[0].x = ((double)line_start_x) + pen_half_width;
      poly[0].y = ((double)line_start_y);
      poly[1].x = ((double)line_end_x) + pen_half_width;
      poly[1].y = ((double)line_end_y);
      poly[2].x = ((double)line_end_x) - pen_half_width;;
      poly[2].y = ((double)line_end_y);
      poly[3].x = ((double)line_start_x) - pen_half_width;
      poly[3].y = ((double)line_start_y);
    }
  
    XRenderCompositeDoublePoly (display,
				PictOpOver,
				surface.fill_picture,
				surface.picture,
				surface.mask_format,
				0, 0, 0, 0, poly, 4, EvenOddRule);

    return SUCCESS;
  } else { /* if (have_useful_xrender) */

    line_gcv.function = GXcopy;
    line_gcv.foreground = lookup_pixel (colorname);
    line_gcv.fill_style = FillSolid;
    line_gcv.line_width = pen_width;
    XGetGCValues (display, gc, DEFAULT_GCV_MASK, &old_gcv);
    XChangeGC (display, gc, LINE_GCV_MASK, &line_gcv);

    XDrawLine (display, actual_drawable, gc,
	       line_start_x,
	       line_start_y,
	       line_end_x,
	       line_end_y);
    XChangeGC (display, gc, DEFAULT_GCV_MASK, &old_gcv);
    return SUCCESS;
  }  /* if (have_useful_xrender) */
}

/* Don't allocate on the stack.... :| */
static XPoint path[360 * 2];
static XPointDouble d_path[360];

int __xlib_draw_circle (Drawable w, GC gc, char *data) {
  XGCValues gcv, old_gcv;
  int center_x, center_y, radius, pen_width, fill, r;
  double i, p, opp, adj;
  char *colorname, *bg_color_name;
  int path_idx;
  XftColor rcolor;
  double pen_half_width;
  unsigned short int pen_alpha;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if (__xlib_draw_circle_scan (data, &center_x, &center_y, &radius,
			       &fill, &pen_width, &pen_alpha,
			       &colorname, &bg_color_name) < 0)
    return ERROR;

  /* Do some range checking so we can catch any garbage in the data.... */
  if (radius < 0 || pen_width < 0)
    return ERROR;
  if (display_width < display_height) {
    if (radius > display_width) {
      return ERROR;
    }
    if (pen_width > display_width) {
      return ERROR;
    }
  } else {
    if (radius > display_height) {
      return ERROR;
    }
    if (pen_width > display_height) {
      return ERROR;
    }
  }
  if (pen_width > radius) {
    /* return ERROR; */
    pen_width = radius;
  }

  if (have_useful_xrender) {

    if (w != surface.drawable) {
      surface.draw = XftDrawCreate (display, w,
			    DefaultVisual (display,
					   DefaultScreen (display)),
			    DefaultColormap (display,
					     (DefaultScreen (display))));
      surface.picture = XftDrawPicture (surface.draw);
      surface.mask_format = XRenderFindStandardFormat
	(display, PictStandardA8);
      surface.drawable = w;
    }
    if (!get_color (colorname, &rcolor)) {
      if (XftColorAllocName (display, 
			     DefaultVisual (display, DefaultScreen (display)),
			     DefaultColormap (display, DefaultScreen (display)),
			     colorname, 
			     &rcolor)) {
	rcolor.color.alpha = pen_alpha;
	save_color (colorname, &rcolor);
      } else {
	return ERROR;
      }
    }
    if (rcolor.color.alpha != pen_alpha)
      rcolor.color.alpha = pen_alpha;
    surface.fill_picture = XftDrawSrcPicture (surface.draw, &rcolor);
    if (surface.fill_picture == 0) {
      surface.drawable = 0;
      return ERROR;
    }

    pen_half_width = ((double)pen_width / 2.0);
    if (pen_half_width < 1.0)
      pen_half_width = 1.0;

    if (fill) {

      path_idx = 0;
      for (i = 0; i < 360.0; i += 1.0 ){
	opp = opp_side (i, ((double)radius + pen_half_width));
	adj = adj_side (i, ((double)radius + pen_half_width));
	d_path[path_idx].x = opp + center_x;
	d_path[path_idx].y = adj + center_y;
	++path_idx;
      }

      XRenderCompositeDoublePoly (display,
				  PictOpOver,
				  surface.fill_picture,
				  surface.picture,
				  surface.mask_format,
				  0, 0, 0, 0, d_path, 360,
				  WindingRule);
      return SUCCESS;
    } else { /* if (fill) */

      path_idx = 0;
      for (i = 0; i < 360.0; i += 1.0 ){
	opp = opp_side (i, ((double)radius + pen_half_width));
	adj = adj_side (i, ((double)radius + pen_half_width));
	d_path[path_idx].x = opp + center_x;
	d_path[path_idx].y = adj + center_y;
	++path_idx;
      }

      XRenderCompositeDoublePoly (display,
				  PictOpOver,
				  surface.fill_picture,
				  surface.picture,
				  surface.mask_format,
				  0, 0, 0, 0, d_path,
				  360, 
				  WindingRule);
      if (!get_color (bg_color_name, &rcolor)) {
	if (XftColorAllocName (display, 
			       DefaultVisual (display, 
					      DefaultScreen (display)),
			       DefaultColormap (display, 
						DefaultScreen (display)),
			       bg_color_name, 
			       &rcolor)) {
	  rcolor.color.alpha = pen_alpha;
	  save_color (colorname, &rcolor);
	} else {
	  return ERROR;
	}
      }
      if (rcolor.color.alpha != pen_alpha)
	rcolor.color.alpha = pen_alpha;
      surface.fill_picture = XftDrawSrcPicture (surface.draw, &rcolor);
      if (surface.fill_picture == 0) {
	surface.drawable = 0;
	return ERROR;
      }
      path_idx = 0;
      for (i = 0; i < 360.0; i += 1.0 ){
	opp = opp_side (i, ((double)radius - pen_half_width));
	adj = adj_side (i, ((double)radius - pen_half_width));
	d_path[path_idx].x = opp + center_x;
	d_path[path_idx].y = adj + center_y;
	++path_idx;
      }

      XRenderCompositeDoublePoly (display,
				  PictOpOver,
				  surface.fill_picture,
				  surface.picture,
				  surface.mask_format,
				  0, 0, 0, 0, d_path,
				  360, 
				  WindingRule);
      return SUCCESS;
    } /* if (fill) */
  } else { /* if (have_useful_xrender) */
    if (fill) {
      XGetGCValues (display, gc, CIRCLE_GCV, &old_gcv);
      gcv.foreground = lookup_pixel (colorname);
      XChangeGC (display, gc, CIRCLE_GCV, &gcv);
      path_idx = 0;
      for (i = 0; i <= 360.0; i += 0.5 ){
	opp = opp_side (i, ((double)radius));
	adj = adj_side (i, ((double)radius));
	path[path_idx].x = (int)opp + center_x;
	path[path_idx].y = (int)adj + center_y;
	++path_idx;
      }
      XFillPolygon (display, w, gc, path, (360 * 2), Convex,
		    CoordModeOrigin);
      XChangeGC (display, gc, CIRCLE_GCV, &old_gcv);
    } else {
      XGetGCValues (display, gc, CIRCLE_GCV, &old_gcv);
      gcv.foreground = lookup_pixel (colorname);
      XChangeGC (display, gc, CIRCLE_GCV, &gcv);
      /* This turns out to be more efficient that drawing the points
	 in one call (or pen_width * calls) using XDrawPoints. */
      if (pen_width > 1) {
	for (p = (0 - (pen_width / 2)); p < (pen_width / 2); ++p) {
	  for (i = 0; i <= 360.0; i += 0.2 ){
	    opp = opp_side (i, ((double)radius + p));
	    adj = adj_side (i, ((double)radius + p));
	    XDrawPoint (display, w, gc, (int)opp + center_x, 
			(int)adj +  center_y);
	  }
	}
      } else {
	for (i = 0; i <= 360.0; i += 0.2 ){
	  opp = opp_side (i, ((double)radius + p));
	  adj = adj_side (i, ((double)radius + p));
	  XDrawPoint (display, w, gc, (int)opp + center_x, 
		      (int)adj +  center_y);
	}
      }
      XChangeGC (display, gc, CIRCLE_GCV, &old_gcv);
    }
    return SUCCESS;
  } /*   /* if (have_useful_xrender) */
}

int __xlib_draw_point (Drawable drawable_arg, GC gc, char *data) {
  XGCValues point_gcv, old_gcv;
  XftColor rcolor;
  int center_x, center_y, pen_width;
  unsigned short pen_alpha;
  int panebuffer_id, panebackingstore_id, actual_drawable;
  int r;
  int path_idx;
  double opp, adj, radius, i;
  static char *colorname;
  
#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  __xlib_draw_point_scan (data, &center_x, &center_y, &pen_width,
			  &pen_alpha,
			  &panebuffer_id, &panebackingstore_id,
			  &colorname);
  if (!panebuffer_id && !panebackingstore_id) {
    actual_drawable = drawable_arg;
  } else {
    actual_drawable = panebuffer_id;
  }
  
  /* i.e., if we don't have a buffered window, don't try to use
     xrender */
  if (have_useful_xrender && panebuffer_id) {
    if (actual_drawable != surface.drawable) {
      surface.draw = XftDrawCreate (display, actual_drawable,
			    DefaultVisual (display,
					   DefaultScreen (display)),
			    DefaultColormap (display,
					     (DefaultScreen (display))));
      surface.picture = XftDrawPicture (surface.draw);
      surface.mask_format = XRenderFindStandardFormat
	(display, PictStandardA8);
      surface.drawable = actual_drawable;
    }

    if (!get_color (colorname, &rcolor)) {
      if (XftColorAllocName (display, 
			     DefaultVisual (display, DefaultScreen (display)),
			     DefaultColormap (display, DefaultScreen (display)),
			     colorname, 
			     &rcolor)) {
	save_color (colorname, &rcolor);
      } else {
	return ERROR;
      }
    }
    if (rcolor.color.alpha != pen_alpha)
      rcolor.color.alpha = pen_alpha;
    /* this gets re-used if possible. */
    surface.fill_picture = XftDrawSrcPicture (surface.draw, &rcolor);
    if (pen_width < 2) {
      radius = 1.0;
    } else {
      radius = pen_width / 2;
    }
    path_idx = 0;
    if (radius < 6.0) {
      for (i = 0; i < 360.0; i += 60.0 ){
	opp = opp_side (i, radius);
	adj = adj_side (i, radius);
	d_path[path_idx].x = opp + center_x;
	d_path[path_idx].y = adj + center_y;
	++path_idx;
      }
    } else {
      for (i = 0; i < 360.0; i += 1.0 ){
	opp = opp_side (i, radius);
	adj = adj_side (i, radius);
	d_path[path_idx].x = opp + center_x;
	d_path[path_idx].y = adj + center_y;
	++path_idx;
      }
    }

    XRenderCompositeDoublePoly (display,
				PictOpOver,
				surface.fill_picture,
				surface.picture,
				surface.mask_format,
				0, 0, 0, 0, d_path,
				((radius < 6.0) ? 6 : 360),
				WindingRule);
    return SUCCESS;
  } else {

    XGetGCValues (display, gc, DEFAULT_GCV_MASK, &old_gcv);
    point_gcv.function = GXcopy;
    point_gcv.foreground = lookup_pixel (colorname);
    point_gcv.fill_style = FillSolid;
    XChangeGC (display, gc, POINT_GCV_MASK, &point_gcv);
    if (pen_width < 2) {
      XDrawPoint (display, actual_drawable, gc,
		  center_x, center_y);
    } else {
      XFillArc (display, actual_drawable, gc,
		center_x, center_y,
		pen_width, pen_width,
		0, 360 * 64);
    }
    XChangeGC (display, gc, DEFAULT_GCV_MASK, &old_gcv);
  }
  return SUCCESS;
}

#else /* HAVE_XRENDER_H  && HAVE_XFT_H*/

int __xlib_draw_line (Drawable drawable_arg, GC gc, char *data) {
  XGCValues line_gcv, old_gcv;
  int line_start_x, line_start_y, line_end_x, line_end_y, pen_width;
  unsigned short int pen_alpha;
  int panebuffer_id, panebackingstore_id, actual_drawable;
  int r;
  static char *colorname;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if (__xlib_draw_line_scan (data, &line_start_x, &line_start_y,
			     &line_end_x, &line_end_y, &pen_width,
			     &pen_alpha, 
			     &panebuffer_id, &panebackingstore_id,
			     &colorname) < 0)
    return ERROR;

  if (!panebuffer_id && !panebackingstore_id) {
    actual_drawable = drawable_arg;
  } else {
    actual_drawable = panebuffer_id;
  }

  line_gcv.function = GXcopy;
  line_gcv.foreground = lookup_pixel (colorname);
  line_gcv.fill_style = FillSolid;
  line_gcv.line_width = pen_width;
  XGetGCValues (display, gc, DEFAULT_GCV_MASK, &old_gcv);
  XChangeGC (display, gc, LINE_GCV_MASK, &line_gcv);

  XDrawLine (display, actual_drawable, gc,
	     line_start_x,
	     line_start_y,
	     line_end_x,
	     line_end_y);
  XChangeGC (display, gc, DEFAULT_GCV_MASK, &old_gcv);
  return SUCCESS;

}

int __xlib_draw_circle (Drawable w, GC gc, char *data) {
  XGCValues gcv, old_gcv;
  int center_x, center_y, radius, pen_width, fill;
  int r;
  double i, p, opp, adj;
  char *colorname, *bg_color_name;
  XPoint path[360 * 2];
  int path_idx;
  unsigned short int pen_alpha;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if (__xlib_draw_circle_scan (data, &center_x, &center_y, &radius,
			       &fill, &pen_width, &pen_alpha, &colorname,
			       &bg_color_name) < 0)
    return ERROR;

  /* Do some range checking so we can catch any garbage in the data.... */
  if (radius < 0 || pen_width < 0)
    return ERROR;
  if (display_width < display_height) {
    if (radius > display_width) {
      return ERROR;
    }
    if (pen_width > display_width) {
      return ERROR;
    }
  } else {
    if (radius > display_height) {
      return ERROR;
    }
    if (pen_width > display_height) {
      return ERROR;
    }
  }
  if (pen_width > radius) {
    return ERROR;
  }
  if (fill) {
    XGetGCValues (display, gc, CIRCLE_GCV, &old_gcv);
    gcv.foreground = lookup_pixel (colorname);
    XChangeGC (display, gc, CIRCLE_GCV, &gcv);
    path_idx = 0;
    for (i = 0; i <= 360.0; i += 0.5 ){
      opp = opp_side (i, ((double)radius));
      adj = adj_side (i, ((double)radius));
      path[path_idx].x = (int)opp + center_x;
      path[path_idx].y = (int)adj + center_y;
      ++path_idx;
    }
    XFillPolygon (display, w, gc, path, (360 * 2), Convex,
		  CoordModeOrigin);
    XChangeGC (display, gc, CIRCLE_GCV, &old_gcv);
  } else {
    XGetGCValues (display, gc, CIRCLE_GCV, &old_gcv);
    gcv.foreground = lookup_pixel (colorname);
    XChangeGC (display, gc, CIRCLE_GCV, &gcv);
    /* This turns out to be more efficient that drawing the points
       in one call (or pen_width * calls) using XDrawPoints. */
    if (pen_width > 1) {
      for (p = (0 - (pen_width / 2)); p < (pen_width / 2); ++p) {
	for (i = 0; i <= 360.0; i += 0.2 ){
	  opp = opp_side (i, ((double)radius + p));
	  adj = adj_side (i, ((double)radius + p));
	  XDrawPoint (display, w, gc, (int)opp + center_x, 
		      (int)adj +  center_y);
	}
      }
    } else {
      for (i = 0; i <= 360.0; i += 0.2 ){
	opp = opp_side (i, ((double)radius + p));
	adj = adj_side (i, ((double)radius + p));
	XDrawPoint (display, w, gc, (int)opp + center_x, 
		    (int)adj +  center_y);
      }
    }
    XChangeGC (display, gc, CIRCLE_GCV, &old_gcv);
  }

  return SUCCESS;
}

int __xlib_draw_point (Drawable drawable_arg, GC gc, char *data) {
  XGCValues point_gcv, old_gcv;
  int center_x, center_y, pen_width;
  unsigned short int pen_alpha;
  int panebuffer_id, panebackingstore_id, actual_drawable;
  int r;
  static char *colorname;
  
#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  __xlib_draw_point_scan (data, &center_x, &center_y, &pen_width,
			  &pen_alpha, &panebuffer_id, &panebackingstore_id,
			  &colorname);
  if (!panebuffer_id && !panebackingstore_id) {
    actual_drawable = drawable_arg;
  } else {
    actual_drawable = panebuffer_id;
  }

  XGetGCValues (display, gc, DEFAULT_GCV_MASK, &old_gcv);
  point_gcv.function = GXcopy;
  point_gcv.foreground = lookup_pixel (colorname);
  point_gcv.fill_style = FillSolid;
  XChangeGC (display, gc, POINT_GCV_MASK, &point_gcv);
  if (pen_width < 2) {
    XDrawPoint (display, actual_drawable, gc,
		center_x, center_y);
  } else {
    XFillArc (display, actual_drawable, gc,
	      center_x, center_y,
	      pen_width, pen_width,
	      0, 360 * 64);
  }
  XChangeGC (display, gc, DEFAULT_GCV_MASK, &old_gcv);
  return SUCCESS;
}

#endif /* HAVE_XRENDER_H  && HAVE_XFT_H */

#endif /* X11LIB_FRAME */

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

static void __gui_support_error (void) {
  x_support_error ();
}

int __xlib_draw_line (unsigned int w, unsigned int gc, char *data) {
  x_support_error (); return ERROR;
}

int __xlib_draw_circle (unsigned int w, unsigned int gc, char *data) {
  x_support_error (); return ERROR;
}

int __xlib_draw_point (unsigned int w, unsigned int gc, char *data) {
  x_support_error (); return ERROR;
}

void __ctalkX11UseXRender (bool b) {
  x_support_error ();
}

bool __ctalkX11UsingXRender (void) {
  x_support_error (); return false;
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
