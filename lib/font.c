/* $Id: font.c,v 1.2 2020/09/19 02:21:15 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016  Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include <X11/Xlib.h>

char *__get_actual_xlfd (char *xlfd) {
  Display *display;
  char *d_env = NULL;
  char **font_set = NULL;
  static char actual_xlfd[MAXMSG];
  int n_fonts = 0;
  if ((display = XOpenDisplay (d_env)) == NULL) {
    _warning ("__get_actual_xlfd: Could not open display %s.\n",
 	      getenv ("DISPLAY"));
    return NULL;
  }
  font_set = XListFonts (display, xlfd, 10, &n_fonts);
  XCloseDisplay (display);
  if (n_fonts) {
    strcpy (actual_xlfd, font_set[0]);
    XFreeFontNames (font_set);
    return actual_xlfd;
  }
  return NULL;
}

/* Try to provide some reasonable default fonts." */
static char *fixed_fonts[3] = {
    "-*-fixed-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-fixed-medium-r-*-*-*-140-*-*-*-*-*-*",
    "-*-fixed-medium-r-*-*-*-100-*-*-*-*-*-*"};

int __ctalkX11QueryFont (OBJECT *font, char *xlfd) {
  Display *display;
  char *d_env = NULL;
  char **font_set;
  char buf[MAXMSG];
  int n_fonts;
  XFontStruct *font_info;
  char l_xlfd[MAXMSG];
  int f_retries = 0;

  if (str_eq (xlfd, "fixed")) {
    strcpy (l_xlfd, fixed_fonts[f_retries]);
  } else {
    strcpy (l_xlfd, xlfd);
  }

  OBJECT *font_object, *fontId_object, *ascent_object, *descent_object,
    *maxWidth_object, *height_object, *fontDesc_object, *lbearing_object,
    *rbearing_object;
  font_object = (IS_VALUE_INSTANCE_VAR (font) ? font->__o_p_obj : font);
  if ((display = XOpenDisplay (d_env)) == NULL) {
    _warning ("__ctalkX11QueryFont: Could not open display %s.\n",
  	      getenv ("DISPLAY"));
    return ERROR;
  }
 retry_font_query:
  font_set = XListFonts (display, l_xlfd, 10, &n_fonts);
  if (n_fonts) {
    font_info = XLoadQueryFont (display, l_xlfd);
    if (font_info -> per_char == NULL) {
      _warning ("__ctalkX11QueryFont: Character data for %s not found. "
		"Using fixed font.\n", xlfd);
    XFreeFontNames (font_set);
    strcpy (l_xlfd, fixed_fonts[f_retries]);
    ++f_retries;
    if (f_retries > 2) {
      _error ("__ctalkX11QueryFont: Could not find a resonable fixed font. "
	      "Exiting.\n");
    }
    goto retry_font_query;
    }
    if ((fontId_object = 
	 __ctalkGetInstanceVariable (font_object, "fontId", TRUE)) != NULL) {
      SETINTVARS(fontId_object, font_info -> fid);
    }
    if ((ascent_object = 
	 __ctalkGetInstanceVariable (font_object, "ascent", TRUE)) != NULL) {
      SETINTVARS(ascent_object, font_info -> ascent);
    }
    if ((descent_object = 
	 __ctalkGetInstanceVariable (font_object, "descent", TRUE)) != NULL) {
      SETINTVARS(descent_object, font_info -> descent);
    }
    if ((maxWidth_object = 
	 __ctalkGetInstanceVariable (font_object, "maxWidth", TRUE)) != NULL) {
      SETINTVARS(maxWidth_object, font_info -> per_char['M'].width);
    }
    if ((lbearing_object = 
	 __ctalkGetInstanceVariable (font_object, "maxLBearing", TRUE))
	!= NULL) {
      SETINTVARS(lbearing_object, font_info -> max_bounds.lbearing);
    }
    if ((rbearing_object = 
	 __ctalkGetInstanceVariable (font_object, "maxRBearing", TRUE))
	!= NULL) {
      SETINTVARS(rbearing_object, font_info -> max_bounds.rbearing);
    }
    if ((height_object = 
	 __ctalkGetInstanceVariable (font_object, "height", TRUE)) != NULL) {
      SETINTVARS(height_object,
	(font_info -> max_bounds.ascent + font_info -> max_bounds.descent));
    }
    if ((fontDesc_object = 
	 __ctalkGetInstanceVariable (font_object, "fontDesc", TRUE)) != NULL){
      __ctalkSetObjectValueVar (fontDesc_object, font_set[0]);
    }
    XFreeFontNames(font_set);
    XFreeFont (display, font_info);
  } else {
    _warning ("__ctalkX11QueryFont: Could not find font %s. "
	      "Using fixed font.\n", xlfd);
    XFreeFontNames (font_set);
    strcpy (l_xlfd, fixed_fonts[f_retries]);
    ++f_retries;
    if (f_retries > 2) {
      _error ("__ctalkX11QueryFont: Could not find a resonable fixed font. "
	      "Exiting.\n");
    }
    goto retry_font_query;
  }
  XCloseDisplay (display);
  return SUCCESS;
}

int __ctalkX11TextWidth (char *xlfd, char *text) {
  Display *display;
  char *d_env = NULL;
  XFontStruct *font_info;
  int text_width;
  if ((display = XOpenDisplay (d_env)) == NULL) {
    _warning ("__ctalkX11TextWidth: Could not open display %s.\n",
  	      getenv ("DISPLAY"));
    return ERROR;
  }
  if (xlfd == NULL || str_eq (xlfd, NULLSTR)) {
    font_info = XLoadQueryFont (display, fixed_fonts[0]);
  } else {
    if ((font_info = XLoadQueryFont (display, xlfd)) == NULL) {
      font_info = XLoadQueryFont (display, fixed_fonts[0]);
    }
  }
  text_width = XTextWidth (font_info, text, strlen (text));
  XFreeFont (display, font_info);
  XCloseDisplay (display);
  return text_width;
}

#else
char *__get_actual_xlfd (char *xlfd) {
  return SUCCESS;
}

int __ctalkX11QueryFont (OBJECT *font, char *xlfd) {
  return SUCCESS;
}

int __ctalkX11TextWidth (char *xlfd, char *text) {
   return SUCCESS;
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
