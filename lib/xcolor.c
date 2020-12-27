/* $Id: xcolor.c,v 1.8 2020/12/27 19:25:21 rkiesling Exp $ -*-c-*-*/

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

extern Display *display;   /* Defined in x11lib.c. */

#if X11LIB_FRAME

int __ctalkX11NamedColor (char *colorname, int *red_out, int *green_out,
			  int *blue_out, unsigned long int *pixel_out) {
  return SUCCESS;
}

#else /* #if X11LIB_FRAME */

int lookup_color (Display *d, XColor *color, char *name) {

  XColor exact_color;
  Colormap default_cmap;

  default_cmap = DefaultColormap (d, DefaultScreen (d));
  
  if (XAllocNamedColor 
      (d, default_cmap, name, &exact_color, color)) {
    return SUCCESS;
  } else {
    return ERROR;
  }
}

unsigned long lookup_pixel (char *color) {
  XColor c;
  if (!lookup_color (display, &c, color)) {
    return c.pixel;
  } else {
    return BlackPixel (display, DefaultScreen (display));
  }
}

/* As above, but we use our own Display in case the program is
   still initializing. */
unsigned long lookup_pixel_d (Display *d, char *color) {
  XColor c;

  if (d == NULL)
    return BlackPixel (display, DefaultScreen (display));

  if (!lookup_color (d, &c, color)) {
    return c.pixel;
  } else {
    return BlackPixel (d, DefaultScreen (d));
  }
}

int lookup_rgb (char *color, unsigned short int *r_return,
		unsigned short int *g_return,
		unsigned short int *b_return) {
  XColor c;
  if (!lookup_color (display, &c, color)) {
    *r_return= c.red; *g_return = c.green; *b_return = c.blue;
    return c.pixel;
  } else {
    *r_return = 0xffff; *g_return  = 0xffff; *b_return = 0xffff;
    return BlackPixel (display, DefaultScreen (display));
  }
}

/* For use when the app has not yet started the X Event client,
   we can still get color values, like during program initialization.
*/
#define LOCAL_DISPLAY (display==NULL)

int __ctalkX11NamedColor (char *colorname, int *red_out, int *green_out,
			  int *blue_out, unsigned long int *pixel_out) {
  XColor screen_def, exact_def;
  Display *l_d;
  int r, ret;;

  /* if (LOCAL_DISPLAY) { */
    l_d = XOpenDisplay (getenv ("DISPLAY"));
#if 0
} else {
    l_d = display;
  }
#endif

  r = XLookupColor (l_d,
		    DefaultColormap (l_d, DefaultScreen (l_d)),
		    colorname,
		    &exact_def, &screen_def);
  if (r > 0) {
    XAllocNamedColor (l_d,
		      DefaultColormap (l_d, DefaultScreen (l_d)),
		      colorname,
		      &screen_def, &exact_def);
    *red_out = screen_def.red;
    *green_out = screen_def.green;
    *blue_out = screen_def.blue;
    *pixel_out = screen_def.pixel;
    ret = SUCCESS;
  } else {
    *red_out = *green_out = *blue_out = 0;
    *pixel_out = BlackPixel (l_d, DefaultScreen (l_d));
    ret = ERROR;
  }
/* if (LOCAL_DISPLAY) { */
    XCloseDisplay (l_d);
/* } */
  return ret;
}

int __ctalkX11RGBColor (char *rgbspec, int *red_out, int *green_out,
			int *blue_out, unsigned long int *pixel_out) {
  XColor parse_exact_color, alloc_screen_color;
  char sbuf[MAXLABEL], rbuf[16], gbuf[16], bbuf[16];
  int r, g, b;
  Display *l_d;

  /* if (LOCAL_DISPLAY) { *//***/
    l_d = XOpenDisplay (getenv ("DISPLAY"));
    /* } *//***/

  if (*rgbspec == '#') {
    /* the fn doesn't rescale the spec's values, it simply formats
       them into a rgb:hh/hh/hh format that XParseColor understands. */
    switch (strlen (&rgbspec[1]))
      {
      case 3:
	strcpy (rbuf, &rgbspec[1]); rbuf[1] = 0;
	strcpy (gbuf, &rgbspec[2]); gbuf[1] = 0;
	strcpy (bbuf, &rgbspec[3]); bbuf[1] = 0;
	r = strtol (rbuf, NULL, 16);
	g = strtol (gbuf, NULL, 16);
	b = strtol (bbuf, NULL, 16);
	sprintf (sbuf, "rgb:%01x/%01x/%01x", r, g, b);
	break;
      case 6:
	strcpy (rbuf, &rgbspec[1]); rbuf[2] = 0;
	strcpy (gbuf, &rgbspec[3]); gbuf[2] = 0;
	strcpy (bbuf, &rgbspec[5]); bbuf[2] = 0;
	r = strtol (rbuf, NULL, 16);
	g = strtol (gbuf, NULL, 16);
	b = strtol (bbuf, NULL, 16);
	sprintf (sbuf, "rgb:%02x/%02x/%02x", r, g, b);
	break;
      case 9:
	strcpy (rbuf, &rgbspec[1]); rbuf[3] = 0;
	strcpy (gbuf, &rgbspec[4]); gbuf[3] = 0;
	strcpy (bbuf, &rgbspec[7]); bbuf[3] = 0;
	r = strtol (rbuf, NULL, 16);
	g = strtol (gbuf, NULL, 16);
	b = strtol (bbuf, NULL, 16);
	sprintf (sbuf, "rgb:%03x/%03x/%03x", r, g, b);
	break;
      case 12:
	strcpy (rbuf, &rgbspec[1]); rbuf[4] = 0;
	strcpy (gbuf, &rgbspec[5]); gbuf[4] = 0;
	strcpy (bbuf, &rgbspec[9]); bbuf[4] = 0;
	r = strtol (rbuf, NULL, 16);
	g = strtol (gbuf, NULL, 16);
	b = strtol (bbuf, NULL, 16);
	sprintf (sbuf, "rgb:%04x/%04x/%04x", r, g, b);
	break;
      default:
	/* if (LOCAL_DISPLAY) { *//***/
	  XCloseDisplay (l_d);
	  /* }*//***/
	return ERROR;
	break;
      }
  } else if (!strncmp (rgbspec, "rgb:", 4)) {
    strcpy (sbuf, rgbspec);
  } else {
    *red_out = *green_out = *blue_out = 0;
    *pixel_out = BlackPixel (l_d, DefaultScreen (l_d));
    /* if (LOCAL_DISPLAY) { *//***/
      XCloseDisplay (l_d);
      /* }*//***/
    return ERROR;
  }
  XParseColor (l_d, DefaultColormap (l_d, DefaultScreen (l_d)),
	       sbuf, &parse_exact_color);
  memset ((void *)&alloc_screen_color, 0, sizeof (XColor));
  alloc_screen_color.red = parse_exact_color.red;
  alloc_screen_color.green = parse_exact_color.green;
  alloc_screen_color.blue = parse_exact_color.blue;
  XAllocColor (l_d, DefaultColormap (l_d, DefaultScreen (l_d)),
	       &alloc_screen_color);
  *red_out = alloc_screen_color.red;
  *green_out = alloc_screen_color.green;
  *blue_out = alloc_screen_color.blue;
  *pixel_out = alloc_screen_color.pixel;
  if (LOCAL_DISPLAY) {
    XCloseDisplay (l_d);
  }
  return SUCCESS;
}


#endif /* X11LIB_FRAME */

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

int __ctalkX11NamedColor (char *colorname, int *red_out, int *green_out,
			  int *blue_out, unsigned long int *pixel_out) {
  _error ("__ctalkX11NamedColor: This program requires the X Window System. "
	  "Exiting.\n");
  return ERROR;
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
