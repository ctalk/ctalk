/* $Id: edittext.c,v 1.33 2020/01/31 08:12:52 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2018-2020  Robert Kiesling, rk3314042@gmail.com.
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "xlibfont.h"

#ifdef HAVE_XFT_H

#include <fontconfig/fontconfig.h>
#include <X11/Xft/Xft.h>
#include FT_FREETYPE_H
#include "xftfont.h"
#include "x11defs.h"

extern XFTFONT ft_font;

#else /* HAVE_XFT_H */

#include "x11defs.h"

#endif /* HAVE_XFT_H */

extern char *shm_mem;
extern int mem_id;

extern char *ascii[8193];             /* from intascii.h */

extern bool monospace;

extern Display *display;
extern XLIBFONT xlibfont;

#define TAB_WIDTH 8
#define SHM_BLKSIZE 10240  /* If changing, also change in x11lib.c, etc... */

extern bool natural_text;

/* Height of FIXED_FONT_XLFD: "*-fixed-*-*-*-*-*-120-*" - defined in
   x11defs.h. */
#define DEFAULT_LINE_HEIGHT 13
#define DEFAULT_LMARGIN 2
/* (win_width  / FIXED_FONT_XLFD -> max_bounds.width) - 
   FIXED_FONT_XLFD -> max_bounds.width */
#define DEFAULT_RMARGIN 30
#undef CR
#define CR 13
#undef LF
#define LF 10
#define DELETE_KEYSYM   0x7f
#define SCROLLMARGIN 2

#define FORMAT_BEGIN        "<format"
#define FORMAT_BEGIN_LENGTH 7

/* these are our possible shiftState values */
/* if changing these, also change in X11TextEditorPane class */
#define shiftStateShift (1 << 0)
#define shiftStateCtrl  (1 << 1)

static int c_line_width = -1;  /* Dimensions calculated for the */
static int c_scrollmargin;     /* client process.               */

static int line_width;         /* These are the layout dimen-   */
static int left_margin;        /* sions for the server process. */
static int right_margin;
static int line_height = -1;
static int win_width;
static int win_height;
static Drawable win_xid;
static char fgcolorname[0xff];
static char bgcolorname[0xff];
static char selectionbgcolorname[0xff];
static bool user_line_width = false, user_left_margin = false,
  user_right_margin = false, user_line_height = false,
  user_win_width = false, user_win_height = false,
  user_fg_color = false, user_bg_color = false,
  user_selection_bg_color = false, user_win_xid = false;

static bool need_init = true;
/* Used in case we need to resize the Xft drawing surfaces. */
static bool have_format = false;
static bool reformat = false;

int point_x, point_y, point_ret;

#define EDITINTSET(o,i) (INTVAL((o) -> __o_value) = \
			 INTVAL((o) -> instancevars -> __o_value) = (i))

/* 
 *  So far, the only format is the global layout.
 */
static void edit_setformat (char *text) {
  char *p, *q, *r;
  char *key, keybuf[255];
  char *val_end, valbuf[255];
  q = text;

  if (have_format = false) {
    have_format = true;
  } else {
    reformat = true;
  }

  while ((p = strchr (q, '=')) != NULL) {
    r = p - 1;
    while (isalnum ((int)*r)) {
      --r;
    }
    key = r + 1;
    memset (keybuf, 0, 255);
    strncpy (keybuf, key, p - key);

    r = p + 1;
    if ((val_end = strchr (r, '>')) != NULL) {
      memset (valbuf, 0, 255);
      strncpy (valbuf, r, val_end - r);
    }

    q = val_end + 1;

    if (str_eq (keybuf, "lineWidth")) {
      line_width = atoi (valbuf);
      user_line_width = true;
    } else if (str_eq (keybuf, "leftMargin")) {
      left_margin = atoi (valbuf);
      user_left_margin = true;
    } else if (str_eq (keybuf, "rightMargin")) {
      right_margin = atoi (valbuf);
      user_right_margin = true;
    } else if (str_eq (keybuf, "lineHeight")) {
      line_height = atoi (valbuf);
      user_line_height = true;
    } else if (str_eq (keybuf, "winWidth")) {
      win_width = atoi (valbuf);
      user_win_width = true;
    } else if (str_eq (keybuf, "winHeight")) {
      win_height = atoi (valbuf);
      user_win_height = true;
    } else if (str_eq (keybuf, "foregroundColor")) {
      strcpy (fgcolorname, valbuf);
      user_fg_color = true;
    } else if (str_eq (keybuf, "backgroundColor")) {
      strcpy (bgcolorname, valbuf);
      user_bg_color = true;
    } else if (str_eq (keybuf, "selectionBackgroundColor")) {
      strcpy (selectionbgcolorname, valbuf);
      user_selection_bg_color = true;
    } else if (str_eq (keybuf, "xWindowID")) {
      win_xid = (Drawable)atoi(valbuf);
      user_win_xid = true;
    }
  }
}

static char *edit_text_read (char *fn) {
  struct stat statbuf;
  long long int fsize, bytes_read;
  char *text;
  FILE *f;

  if (stat (fn, &statbuf)) {
    fprintf (stderr, "edit_text_read (stat): %s: %s.\n",
	     fn, strerror (errno));
    exit (EXIT_FAILURE);  /* TODO! We need to exit the other process. */
  }
  fsize = statbuf.st_size;

  if ((f = fopen (fn, "r")) == NULL) {
    fprintf (stderr, "edit_text_read (fopen): %s: %s.\n",
	     fn, strerror (errno));
    exit (EXIT_FAILURE);  /* TODO! We need to exit the other process. */
  }

  text = malloc (fsize + 1);
  memset (text, '\0', fsize + 1);

  if ((bytes_read = fread (text, sizeof (char), fsize, f)) != fsize) {
    fprintf (stderr, "edit_text_read (fread): %s: %s.\n",
	     fn, strerror (errno));
    fprintf (stderr, "%s\n", text);
    exit (EXIT_FAILURE);  /* TODO! We need to exit the other process. */
  }

  if (fclose (f)) {
    fprintf (stderr, "edit_text_read (fclose): %s: %s.\n",
	     fn, strerror (errno));
    exit (EXIT_FAILURE);  /* TODO! We need to exit the other process. */
  }

  /* The file gets unlinked in __ctalkX11TextFromData. */

  return text;
}

typedef struct _linerec LINEREC;

struct _linerec {
  struct _linerec *next, *prev;
  char *text;
  int start;
};

static LINEREC *new_line (void) {
  LINEREC *l;
  if ((l = (LINEREC *)__xalloc (sizeof (struct _linerec))) == NULL)
    _error ("new_line: %s.", strerror (errno));

  return l;
}

static void linerec_push (LINEREC **l1, LINEREC **l2) {
  LINEREC *t;

  for (t = *l1; t && t -> next; t = t -> next) 
    ;
  if (t) {
    t -> next = *l2;
    (*l2) -> prev = t;
    (*l2) -> next = NULL;
  } else {
    *l1 = *l2;
  }
}

static void delete_line (LINEREC *l) {
  if (l -> text) __xfree (MEMADDR(l -> text));
  __xfree (MEMADDR(l));
}

static void delete_lines (LINEREC **l) {
  LINEREC *t1, *t2;

  if (! *l)
    return;

  for (t1 = *l; t1 -> next; t1 = t1 -> next)
    ;

  if (t1 == *l) {
    delete_line (*l);
    *l = NULL;
    return;
  }

   while (t1 != *l) {
     t2 = t1 -> prev;
     delete_line (t1);
     if (t2 == *l)
       break;
     t1 = t2;
   }
   delete_line (*l);
   *l = NULL;
}

#define INIT_LINE_BUF memset (linebuf, 0, 8192)

static void split_text (char *text, LINEREC **lines, int line_width_param) {
  int i, j, j_2, new_line_start = 0;
  char linebuf[8192];
  LINEREC *t;
  bool have_soft_break_space;

  INIT_LINE_BUF;
  *lines = NULL;
  for (j = 0, i = 0; ; ++i, ++j) {
    switch (text[i])
      {
      case CR:
      case LF:
	linebuf[j] = '\0';
	if (*lines == NULL) {
	  *lines = new_line ();
	  (*lines) -> start = 0;
	  (*lines) -> text = strdup (linebuf);
	} else {
	  t = new_line ();
	  t -> text = strdup (linebuf);
	  t -> start = new_line_start;
	  linerec_push (lines, &t);
	}
	INIT_LINE_BUF;
	j = -1;
	new_line_start = i + 1;
	break;
      case 0:  /* The final line. */
	linebuf[j] = text[i];
	if (*lines == NULL) {
	  *lines = new_line ();
	  (*lines) -> start = 0;
	  (*lines) -> text = strdup (linebuf);
	} else {
	  t = new_line ();
	  t -> text = strdup (linebuf);
	  t -> start = new_line_start;
	  linerec_push (lines, &t);
	}
	return;
      default:
	linebuf[j] = text[i];
	if (j > line_width_param) {
	  have_soft_break_space = false;
	  if (!isspace(linebuf[j])) {
	    for (j_2 = j; j_2 >= 0; j_2--) {
	      if (isspace (linebuf[j_2])) {
		++j_2;
		have_soft_break_space = true;
		break;
	      }
	    }
	  } else {
	    /* Add the space characters to the end of the line
	       before the break. */
	    while (isspace(text[i])) {
	      linebuf[j] = text[i];
	      if (isspace(text[i+1])) {
		++i, ++j;
	      } else {
		break;
	      }
	    }
	    have_soft_break_space = false;
	  }
	  if (have_soft_break_space) {
	    linebuf[j_2] = '\0';
	    i -= ((j + 1) - j_2);
	  } else {
	    linebuf[j] = '\0';
	  }
	  if (*lines == NULL) {
	    *lines = new_line ();
	    (*lines) -> start = 0;
	    (*lines) -> text = strdup (linebuf);
	  } else {
	    t = new_line ();
	    t -> text = strdup (linebuf);
	    t -> start = new_line_start;
	    linerec_push (lines, &t);
	  }
	  new_line_start = i + 1;
	  INIT_LINE_BUF;
	  j = -1;
	}
	break;
      }
  }

}

static void internal_defaults (void) {
  /* These get set if a program hasn't sent any format tags. */
  /* Note that there's no default window width, because it
     wouldn't be used with our DEFAULT_LMARGIN if the app hasn't
     set it (or unset). */
  if (!user_line_width)
    line_width = DEFAULT_RMARGIN - DEFAULT_LMARGIN;
  if (!user_left_margin)
    left_margin = DEFAULT_LMARGIN;
  if (!user_right_margin)
    right_margin = DEFAULT_RMARGIN;
  if (!user_line_height)
    line_height = DEFAULT_LINE_HEIGHT;
}

static OBJECT *buflength_instance_var = NULL;
static OBJECT *textlength_instance_var = NULL;
static OBJECT *point_instance_var = NULL;
static OBJECT *text_instance_var = NULL;
static OBJECT *linewidth_instance_var = NULL;
static OBJECT *viewheightlines_instance_var = NULL;
static OBJECT *viewstartline_instance_var = NULL;
static OBJECT *lineheight_instance_var = NULL;
static OBJECT *size_instance_var = NULL;
static OBJECT *size_x_instance_var = NULL;
static OBJECT *size_y_instance_var = NULL;
static OBJECT *fontvar_instance_var = NULL;
static OBJECT *fontvar_maxwidth_instance_var = NULL;
static OBJECT *fontvar_lbearing_instance_var = NULL;
static OBJECT *fontvar_fontid_instance_var = NULL;
static OBJECT *fontvar_fontdesc_instance_var = NULL;
static OBJECT *fontvar_height_instance_var = NULL;
static OBJECT *ft_fontvar_instance_var = NULL;
static OBJECT *ft_fontvar_height_instance_var = NULL;
static OBJECT *ft_fontvar_ascent_instance_var = NULL;
static OBJECT *ft_fontvar_descent_instance_var = NULL;
static OBJECT *ft_fontvar_maxadvance_instance_var = NULL;
static OBJECT *selecting_instance_var = NULL;
static OBJECT *s_start_instance_var = NULL;
static OBJECT *s_end_instance_var = NULL;
static OBJECT *scrollmargin_instance_var = NULL;
static OBJECT *view_x_offset_instance_var = NULL;
static OBJECT *rightmargin_instance_var = NULL;
static OBJECT *win_xid_instance_var = NULL;
#define DEFAULT_BUFSIZE 8192
static int bufsize = 0;
#define XFT (INTVAL(fontvar_fontid_instance_var -> __o_value) == 0)

static void buffer_size_check (OBJECT *text_var, int textlength) {
  if (textlength >= bufsize) {
    bufsize *= 2;
    __xrealloc ((void **)&(text_var -> __o_value), bufsize);
    __xrealloc ((void **)&(text_var -> instancevars -> __o_value), bufsize);
  }
}

#define TEXT text_instance_var -> __o_value
#define TEXT_VAR text_instance_var -> instancevars -> __o_value

static void buf_init (OBJECT *editorpane_object) {
  char intbuf[0xff];
  int newtextlength;
  if (!buflength_instance_var)
    buflength_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "bufLength", TRUE);
  if (!textlength_instance_var)
    textlength_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "textLength", TRUE);
  if (!point_instance_var)
    point_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "point", TRUE);
  if (!text_instance_var)
    text_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "text", TRUE);
  if (!linewidth_instance_var)
    linewidth_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "lineWidth", TRUE);
  if (!viewstartline_instance_var)
    viewstartline_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "viewStartLine", TRUE);
  if (!viewheightlines_instance_var)
    viewheightlines_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "viewHeight", TRUE);
  if (!size_instance_var)
    size_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "size", TRUE);
  if (!size_x_instance_var)
    size_x_instance_var =
      __ctalkGetInstanceVariable (size_instance_var, "x", TRUE);
  if (!size_y_instance_var)
    size_y_instance_var =
      __ctalkGetInstanceVariable (size_instance_var, "y", TRUE);
  if (!fontvar_instance_var)
    fontvar_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "fontVar", TRUE);
  if (!fontvar_maxwidth_instance_var)
    fontvar_maxwidth_instance_var =
      __ctalkGetInstanceVariable (fontvar_instance_var,
				  "maxWidth", TRUE);
  if (!fontvar_lbearing_instance_var)
    fontvar_lbearing_instance_var =
      __ctalkGetInstanceVariable (fontvar_instance_var,
				  "maxLBearing", TRUE);
  if (!fontvar_fontid_instance_var)
    fontvar_fontid_instance_var =
      __ctalkGetInstanceVariable (fontvar_instance_var,
				  "fontId", TRUE);
  if (!fontvar_fontdesc_instance_var)
    fontvar_fontdesc_instance_var =
      __ctalkGetInstanceVariable (fontvar_instance_var,
				  "fontDesc", TRUE);
  if (!fontvar_height_instance_var)
    fontvar_height_instance_var =
      __ctalkGetInstanceVariable (fontvar_instance_var,
				  "height", TRUE);
  if (!ft_fontvar_instance_var)
    ft_fontvar_instance_var =
      __ctalkGetInstanceVariable (editorpane_object,
				  "ftFontVar", TRUE);
  if (!ft_fontvar_height_instance_var)
    ft_fontvar_height_instance_var =
      __ctalkGetInstanceVariable (ft_fontvar_instance_var,
				  "height", TRUE);
  if (!ft_fontvar_ascent_instance_var)
    ft_fontvar_ascent_instance_var =
      __ctalkGetInstanceVariable (ft_fontvar_instance_var,
				  "ascent", TRUE);
  if (!ft_fontvar_descent_instance_var)
    ft_fontvar_descent_instance_var =
      __ctalkGetInstanceVariable (ft_fontvar_instance_var,
				  "descent", TRUE);
  if (!ft_fontvar_maxadvance_instance_var)
    ft_fontvar_maxadvance_instance_var =
      __ctalkGetInstanceVariable (ft_fontvar_instance_var,
				  "maxAdvance", TRUE);
      
  if (!scrollmargin_instance_var) {
    scrollmargin_instance_var = __ctalkGetInstanceVariable
      (editorpane_object, "scrollMargin", TRUE);
    c_scrollmargin =
      INTVAL(scrollmargin_instance_var -> instancevars -> __o_value);
  }
  if (!selecting_instance_var) {
    selecting_instance_var = __ctalkGetInstanceVariable
      (editorpane_object, "selecting", TRUE);
  }
  if (!s_start_instance_var) {
    s_start_instance_var = __ctalkGetInstanceVariable
      (editorpane_object, "sStart", TRUE);
  }
  if (!s_end_instance_var) {
    s_end_instance_var = __ctalkGetInstanceVariable
      (editorpane_object, "sEnd", TRUE);
  }
  if (!view_x_offset_instance_var) {
    view_x_offset_instance_var = __ctalkGetInstanceVariable
      (editorpane_object, "viewXOffset", TRUE);
  }
  if (!rightmargin_instance_var) {
    rightmargin_instance_var = __ctalkGetInstanceVariable
      (editorpane_object, "rightMargin", TRUE);
  }
  if (!win_xid_instance_var) {
    win_xid_instance_var = __ctalkGetInstanceVariable
      (editorpane_object, "xWindowID", TRUE);
  }
  
  if ((newtextlength = strlen (text_instance_var -> __o_value))
      < DEFAULT_BUFSIZE) {
    __xrealloc ((void **)&(text_instance_var -> __o_value), DEFAULT_BUFSIZE);
    __xrealloc ((void **)&(text_instance_var -> instancevars -> __o_value),
		DEFAULT_BUFSIZE);
    bufsize = DEFAULT_BUFSIZE;
  } else {
    bufsize = newtextlength * 2;
    __xrealloc ((void **)&(text_instance_var -> __o_value), bufsize);
    __xrealloc ((void **)&(text_instance_var -> instancevars -> __o_value),
		bufsize);
  }
  EDITINTSET(buflength_instance_var, bufsize);
  EDITINTSET(textlength_instance_var, newtextlength);
  /***/
  if (INTVAL(point_instance_var -> __o_value) > newtextlength) {
    EDITINTSET(point_instance_var, newtextlength);
  }

  need_init = false;
}

#define SELECTING (INTVAL(s_start_instance_var->__o_value) != 0)

/* Adjusts the column to the end of the target line if the
   column is greater than the target line's length. */
static int shorter_line_adj (LINEREC *current, LINEREC *target,
			     int current_col) {
  int new_col = ((strlen (target -> text) < current_col) ?
		 (target -> start + strlen (target -> text)) :
		 (target -> start + current_col));
  return new_col;
}

/* declared in x11lib.c */
extern bool edittext_resize_notify;

#ifndef XK_Return
/* Some systems don't automatically define X keysyms.
   Make sure we define it here.
*/
#define XK_MISCELLANY
#include <X11/keysymdef.h>
#endif

#ifdef HAVE_XFT_H

extern XftFont *selected_font;  /* The selected outline font ptr. */
extern bool monospace;

static int calc_line_width (void) {
  int line_size_x;
  line_size_x = INTVAL(size_x_instance_var -> __o_value) -
    INTVAL(view_x_offset_instance_var -> __o_value) -
    INTVAL(rightmargin_instance_var -> __o_value);;
  if (c_line_width == -1 || edittext_resize_notify) {
    if ((c_line_width = INTVAL(linewidth_instance_var
			       -> __o_value)) == 0) {
      if (__ctalkXftInitialized ()) {
	c_line_width = line_size_x /
	  selected_font -> max_advance_width;
      } else {
	c_line_width = line_size_x /
	  INTVAL(fontvar_maxwidth_instance_var -> instancevars -> __o_value);
      }
    }
    edittext_resize_notify = false;
  }
  return c_line_width;
}

static int calc_viewlines (void) {
  int c_viewlines;
  if (__ctalkXftInitialized ()) {
    c_viewlines = INTVAL(size_y_instance_var -> __o_value) /
      (selected_font -> ascent + selected_font -> descent);
  } else {
    return INTVAL(viewheightlines_instance_var -> __o_value);
  }
  return c_viewlines;
}

#else
static int calc_line_width (void) {
  int line_size_x;
  line_size_x = INTVAL(size_x_instance_var -> __o_value) -
    INTVAL(view_x_offset_instance_var -> __o_value) -
    INTVAL(rightmargin_instance_var -> __o_value);;
  if (c_line_width == -1) {
    if ((c_line_width = INTVAL(linewidth_instance_var
			    -> __o_value)) == 0) {
      c_line_width = line_size_x /
	INTVAL(fontvar_maxwidth_instance_var -> instancevars -> __o_value);
    }
  }
  return c_line_width;
}

static int calc_viewlines (void) {
  return INTVAL(viewheightlines_instance_var -> __o_value);
}

#endif

static int __text_index_from_click (char *text,
				    int click_x, int click_y,
				    int char_cell_width,
				    int char_cell_height) {
  double d_click_x, d_click_y,
    d_cell_width, d_cell_height,
    d_col_x, d_row_y;
  int i_col_x, i_row_y;
  int l_line_width, l_point, line_no, l_point_row;
  int char_width, half_width, l_margin;
  LINEREC *text_lines = NULL, *l;
  Display *d_l;
  XFontStruct *xfs;
  int i_click_x, l_l_margin, i_x, str_px, l_viewstart;
  
  if (XFT) {
    d_click_y = click_y;
    d_cell_height = char_cell_height;
    d_row_y = (d_click_y / d_cell_height) - 0.5f;
    i_row_y = (int)d_row_y;

    l_margin = INTVAL(view_x_offset_instance_var -> __o_value);
    d_click_x = click_x - l_margin;
    d_cell_width = char_cell_width;
    d_col_x = (d_click_x / d_cell_width);
    d_col_x = trunc (d_col_x); 
    i_col_x = (int)d_col_x;
    l_point = INTVAL(point_instance_var -> __o_value);
    l_line_width = calc_line_width ();
    split_text (text, &text_lines, l_line_width);
    l_point = -1;
    l_viewstart = INTVAL(viewstartline_instance_var -> __o_value);
    for (line_no = 0, l = text_lines; l; l = l -> next, ++line_no) {
      if (line_no < l_viewstart)
	continue;
      if ((line_no - l_viewstart) == i_row_y) {
	l_point_row = l -> start;
	if (i_col_x > strlen (l -> text)) {
	  /* if past the end of the text line, calculate the
	     index at the end of the line. */
	  l_point = l_point_row + strlen (l -> text);
	  return l_point;
	} else {
	  l_point = l_point_row + i_col_x;
	  return l_point;
	}
	break;
      }
    }
  } else {
    d_l = XOpenDisplay (getenv ("DISPLAY"));
    xfs = XLoadQueryFont (d_l, fontvar_fontdesc_instance_var -> __o_value);
    l_l_margin = INTVAL(view_x_offset_instance_var -> __o_value);
    i_click_x = click_x - l_l_margin;
    char_width = INTVAL(fontvar_maxwidth_instance_var -> __o_value);
    half_width = char_width / 2;

    l_line_width = calc_line_width ();
    split_text (text, &text_lines, l_line_width);
    l_point = -1;
    d_click_y = click_y;
    d_cell_height = char_cell_height;
    d_row_y = (d_click_y / d_cell_height) - 0.5f;
    i_row_y = (int)d_row_y;
    l_viewstart = INTVAL(viewstartline_instance_var -> __o_value);
    for (line_no = 0, l = text_lines; l; l = l -> next, ++line_no) {
      if (line_no < l_viewstart)
	continue;
      if ((line_no - l_viewstart) == i_row_y) {
	l_point_row = l -> start;
	str_px = 0;
	for (i_x = 0; l -> text[i_x]; ++i_x) {
	  char c = l -> text[i_x];
	  str_px += xfs -> per_char[c].width;
	  if (str_px >= i_click_x) {
	    XFreeFont (d_l, xfs);
	    XCloseDisplay (d_l);
	    return l_point_row + i_x;
	  }
	}
      }
    }
  }
  if (l_point == -1) {
    /* past the last row */
    for (l = text_lines; l -> next; l = l -> next)
      ;
      l_point_row = l -> start;
      if (i_col_x > strlen (l -> text)) {
	l_point = l_point_row + strlen (l -> text);
      } else {
	l_point = l_point_row + strlen (l -> text);
      }
  }
  delete_lines (&text_lines);
  return l_point;
}

int __edittext_index_from_pointer (OBJECT *editorpane_object,
				int pointer_x, int pointer_y) {
  int char_cell_width, char_cell_height;
  
  if (need_init)
    buf_init (editorpane_object);

  if (XFT) {
    /* We're using outline fonts. */
    char_cell_width =
      INTVAL(ft_fontvar_maxadvance_instance_var -> __o_value);
    char_cell_height = INTVAL(ft_fontvar_ascent_instance_var -> __o_value) +
      INTVAL(ft_fontvar_descent_instance_var -> __o_value);
  } else {
    char_cell_width =
      INTVAL(fontvar_maxwidth_instance_var -> __o_value);
    char_cell_height =
      INTVAL(fontvar_height_instance_var -> __o_value);
  }

  return __text_index_from_click
    (text_instance_var -> __o_value, pointer_x, pointer_y,
     char_cell_width, char_cell_height);

}

int __edittext_row_col_from_mark (OBJECT *editorpane_object,
				  int mark_x_px, int mark_y_px,
				  int *row_out, int *col_out) {
  LINEREC *l, *text_lines = NULL;
  int char_cell_width, char_cell_height, l_point, l_line_width,
    line_no;

  if (need_init)
    buf_init (editorpane_object);
  if (XFT) {
    /* We're using outline fonts. */
    char_cell_width =
      INTVAL(ft_fontvar_maxadvance_instance_var -> __o_value);
    char_cell_height = INTVAL(ft_fontvar_ascent_instance_var -> __o_value) +
      INTVAL(ft_fontvar_descent_instance_var -> __o_value);
  } else {
    char_cell_width =
      INTVAL(fontvar_maxwidth_instance_var -> __o_value);
    char_cell_height =
      INTVAL(fontvar_height_instance_var -> __o_value);
  }
  l_point = __text_index_from_click
    (text_instance_var -> __o_value, mark_x_px, mark_y_px,
     char_cell_width, char_cell_height);
  l_line_width = calc_line_width ();
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);
  if (text_lines == NULL)
    return SUCCESS;

  line_no = 0;
  for (l = text_lines; l; l = l -> next) {
    if (l -> next) {
      if (l_point >= l -> start && l_point < l -> next -> start) {
	*col_out = 0;
	*row_out = line_no;
	break;
      }
    } else {
      *col_out = 0;
      *row_out = line_no;
      break;
    }
    ++line_no;
  }

  delete_lines (&text_lines);
  return SUCCESS;
}

int __edittext_point_to_click (OBJECT *editorpane_object,
				int click_x, int click_y) {
  int char_cell_width, char_cell_height;
  int l_point;
  
  if (need_init)
    buf_init (editorpane_object);
  if (XFT) {
    /* We're using outline fonts. */
    char_cell_width =
      INTVAL(ft_fontvar_maxadvance_instance_var -> __o_value);
    char_cell_height = INTVAL(ft_fontvar_ascent_instance_var -> __o_value) +
      INTVAL(ft_fontvar_descent_instance_var -> __o_value);
  } else {
    char_cell_width =
      INTVAL(fontvar_maxwidth_instance_var -> __o_value);
    char_cell_height =
      INTVAL(fontvar_height_instance_var -> __o_value);
  }
  l_point = __text_index_from_click
    (text_instance_var -> __o_value, click_x, click_y,
     char_cell_width, char_cell_height);
  EDITINTSET(point_instance_var, l_point);

  return SUCCESS;
}

static void swap_selection_ends (int *start, int *end) {
  int selection_tmp;
  selection_tmp = *start;
  *start = *end;
  *end = selection_tmp;
}

/* Don't use without calling buf_init first. */
static void delete_selection (void) {
  int selection_start, selection_end, selection_length, i;
  int l_buflength, l_textlength, l_point;
  char *insbuf;
  selection_start = INTVAL(s_start_instance_var -> __o_value);
  selection_end = INTVAL(s_end_instance_var -> __o_value);
  l_buflength = INTVAL(buflength_instance_var -> instancevars -> __o_value);
  l_textlength = INTVAL(textlength_instance_var -> instancevars -> __o_value);
  l_point = INTVAL(point_instance_var -> instancevars -> __o_value);
  insbuf =  __xalloc (l_buflength);
  if (selection_start > selection_end)
    swap_selection_ends (&selection_start, &selection_end);
  selection_length = selection_end - selection_start;

  strncpy (insbuf, TEXT_VAR, selection_start);
  strcat (insbuf, &(TEXT_VAR[selection_end + 1]));
  strcpy (TEXT, insbuf);
  strcpy (TEXT_VAR, insbuf);

  if (l_point < selection_start) {
    /* do nothing */
  } else if (l_point >= selection_start && l_point <= selection_end) {
    /* move to character after selection */
    l_point = selection_end - selection_length;
  } else {
    /* point is after the selection */
    l_point -= selection_length;
  }
  l_textlength -= selection_length;

  EDITINTSET(point_instance_var, l_point);
  EDITINTSET(textlength_instance_var, l_textlength);
  EDITINTSET(s_start_instance_var, 0);
  EDITINTSET(s_end_instance_var, 0);
  EDITINTSET(selecting_instance_var, 0);

  buffer_size_check (text_instance_var, l_textlength);

  __xfree (MEMADDR(insbuf));

}

int __edittext_set_selection_owner (OBJECT *editorpane_object) {
  Drawable win_id;
  if (need_init)
    buf_init (editorpane_object);
  win_id = (Drawable)INTVAL(win_xid_instance_var -> __o_value);
  if (XSetSelectionOwner (display, XA_PRIMARY, win_id, CurrentTime) == win_id){
    fprintf (stderr, "XSetSelectionOwner failed!\n");
    return ERROR;
  } else {
    fprintf (stderr, "XSetSelectionOwner succeeded!\n");
    return SUCCESS;
  }
}

int __edittext_insert_str_at_click (OBJECT *editorpane_object,
				    int click_x, int click_y,
				    char *str) {
  int char_cell_width, char_cell_height, i;
  int l_point, l_textlength, l_buflength;
  char *insbuf, intbuf[0xff];
  
  if (need_init)
    buf_init (editorpane_object);
  if (XFT) {
    /* We're using outline fonts. */
    char_cell_width =
      INTVAL(ft_fontvar_maxadvance_instance_var -> __o_value);
    char_cell_height = INTVAL(ft_fontvar_ascent_instance_var -> __o_value) +
      INTVAL(ft_fontvar_descent_instance_var -> __o_value);
  } else {
    char_cell_width =
      INTVAL(fontvar_maxwidth_instance_var -> __o_value);
    char_cell_height =
      INTVAL(fontvar_height_instance_var -> __o_value);
  }
  l_point = __text_index_from_click
    (text_instance_var -> __o_value, click_x, click_y,
     char_cell_width, char_cell_height);

  l_textlength = INTVAL(textlength_instance_var -> instancevars -> __o_value);
  l_buflength = INTVAL(buflength_instance_var -> instancevars -> __o_value);

  insbuf =  __xalloc (l_buflength);
  for (i = 0; str[i]; ++i) {
    strcpy (insbuf,	&(TEXT_VAR[l_point]));
    TEXT_VAR[l_point] = str[i];
    strcpy (&(TEXT_VAR[l_point + 1]), insbuf);
    strcpy (insbuf,	&(TEXT[l_point]));
    TEXT[l_point] = str[i];
    strcpy (&(TEXT[l_point + 1]), insbuf);
    ++l_textlength;
    ++l_point;
  }
  EDITINTSET(textlength_instance_var, l_textlength);
  buffer_size_check (text_instance_var, l_textlength);
  return SUCCESS;
}

int __edittext_insert_str_at_point (OBJECT *editorpane_object, char *str) {
  int i, l_point, l_textlength, l_buflength;
  char *insbuf, intbuf[0xff];
  bool point_change = false;
  if (need_init)
    buf_init (editorpane_object);

  if (str == NULL)
    return SUCCESS;

  l_point = INTVAL(point_instance_var -> instancevars -> __o_value);
  l_textlength = INTVAL(textlength_instance_var -> instancevars -> __o_value);
  l_buflength = INTVAL(buflength_instance_var -> instancevars -> __o_value);

  insbuf =  __xalloc (l_buflength);
  for (i = 0; str[i]; ++i) {
    strcpy (insbuf,	&(TEXT_VAR[l_point]));
    TEXT_VAR[l_point] = str[i];
    strcpy (&(TEXT_VAR[l_point + 1]), insbuf);
    strcpy (insbuf,	&(TEXT[l_point]));
    TEXT[l_point] = str[i];
    strcpy (&(TEXT[l_point + 1]), insbuf);
    ++l_point;
    ++l_textlength;
    point_change = true;
  }
  if (point_change)
    EDITINTSET(point_instance_var, l_point);
  EDITINTSET(textlength_instance_var, l_textlength);

  buffer_size_check (text_instance_var, l_textlength);
  
  __xfree (MEMADDR(insbuf));

  return SUCCESS;
}

/***/
unsigned int __edittext_xk_keysym (int keycode, int shift_state,
				   int keypress) {
  Display *d_local;
  unsigned int keysym;

  d_local = XOpenDisplay (getenv ("DISPLAY"));

  keysym = get_x11_keysym_2 (d_local, keycode, shift_state, keypress);

  XCloseDisplay (d_local);

  return keysym;
}

int __edittext_insert_at_point (OBJECT *editorpane_object,
				int keycode, int shift_state,
				int keypress) {
  LINEREC *text_lines = NULL, *l;
  int l_viewstart, l_viewlines, line_no, l_scrollmargin,
    l_viewheight, l_col, l_line_width;
  int keysym;
  int l_point, l_textlength, l_buflength;
  Display *d_local;
  char *insbuf;
  bool point_change = false;

  if (need_init)
    buf_init (editorpane_object);
  
  d_local = XOpenDisplay (getenv ("DISPLAY"));

  keysym = get_x11_keysym_2 (d_local, keycode, shift_state, keypress);

  XCloseDisplay (d_local);

  switch (keysym)
    {
    case XK_Return: keysym = LF; break;
    case '0': if (shift_state & shiftStateShift) keysym = ')'; break;
    case '1': if (shift_state & shiftStateShift) keysym = '!'; break;
    case '2': if (shift_state & shiftStateShift) keysym = '@'; break;
    case '3': if (shift_state & shiftStateShift) keysym = '#'; break;
    case '4': if (shift_state & shiftStateShift) keysym = '$'; break;
    case '5': if (shift_state & shiftStateShift) keysym = '%'; break;
    case '6': if (shift_state & shiftStateShift) keysym = '^'; break;
    case '7': if (shift_state & shiftStateShift) keysym = '&'; break;
    case '8': if (shift_state & shiftStateShift) keysym = '*'; break;
    case '9': if (shift_state & shiftStateShift) keysym = '('; break;
    case '`': if (shift_state & shiftStateShift) keysym = '~'; break;
    case '-': if (shift_state & shiftStateShift) keysym = '_'; break;
    case '=': if (shift_state & shiftStateShift) keysym = '+'; break;
    case '[': if (shift_state & shiftStateShift) keysym = '{'; break;
    case ']': if (shift_state & shiftStateShift) keysym = '}'; break;
    case '\\': if (shift_state & shiftStateShift) keysym = '|'; break;
    case ';': if (shift_state & shiftStateShift) keysym = ':'; break;
    case '\'': if (shift_state & shiftStateShift) keysym = '"'; break;
    case ',': if (shift_state & shiftStateShift) keysym = '<'; break;
    case '.': if (shift_state & shiftStateShift) keysym = '>'; break;
    case '/': if (shift_state & shiftStateShift) keysym = '?'; break;
    default:
      if (islower (keysym) && (shift_state & shiftStateShift)) {
	keysym = toupper (keysym);
      }
    }

  l_point = INTVAL(point_instance_var -> instancevars -> __o_value);
  l_textlength = INTVAL(textlength_instance_var -> instancevars -> __o_value);
  l_buflength = INTVAL(buflength_instance_var -> instancevars -> __o_value);

  insbuf =  __xalloc (l_buflength);

  /* 
     NOTE:  We perform these operations on *both* the value
     of the text instance var and its value var - this keeps all of
     the measurements consistent for all of the editing operations. 
  */
  if (TEXT_VAR[l_point] == 0) {
    /* insert/delete at end of text. */
    if (keysym == XK_BackSpace || keysym == DELETE_KEYSYM) {
      if (SELECTING) {
	delete_selection ();
      } else {
	if (l_point > 0) {
	  --l_point;
	  --l_textlength;
	  TEXT_VAR[l_point] = '\0';
	  TEXT[l_point] = '\0';
	  point_change = true;
	}
      }
    } else {
      TEXT_VAR[l_point] = keysym;
      TEXT[l_point] = keysym;
      ++l_point;
      ++l_textlength;
      point_change = true;
    }
  } else {
    switch (keysym)
      {
      case XK_BackSpace:
	if (SELECTING) {
	  delete_selection ();
	} else {
	  if (l_point > 0) {
	    strcpy (insbuf, &(TEXT_VAR[l_point]));
	    strcpy (&(TEXT_VAR[l_point - 1]), insbuf);
	    strcpy (insbuf, &(TEXT[l_point]));
	    strcpy (&(TEXT[l_point - 1]), insbuf);
	    --l_point;
	    --l_textlength;
	    point_change = true;
	  }
	}
	break;
      case DELETE_KEYSYM:
	/* Delete at end of text, above, happens if we're at the end
	   of the buffer. */
	if (SELECTING) {
	  /* this is untested so far */
	  delete_selection ();
	} else {
	  strcpy (insbuf, &(TEXT_VAR[l_point]));
	  strcpy (&(TEXT_VAR[l_point]), &insbuf[1]);
	  strcpy (insbuf, &(TEXT[l_point]));
	  strcpy (&(TEXT[l_point]), &insbuf[1]);
	  --l_textlength;
	}
	break;
      default:
	strcpy (insbuf,	&(TEXT_VAR[l_point]));
	TEXT_VAR[l_point] = keysym;
	strcpy (&(TEXT_VAR[l_point + 1]), insbuf);
	strcpy (insbuf,	&(TEXT[l_point]));
	TEXT[l_point] = keysym;
	strcpy (&(TEXT[l_point + 1]), insbuf);
	++l_point;
	++l_textlength;
	point_change = true;
	break;
      }
  }
  if (point_change) {
    EDITINTSET(point_instance_var, l_point);
  }
  EDITINTSET(textlength_instance_var, l_textlength);

  buffer_size_check (text_instance_var, l_textlength);
  
  __xfree (MEMADDR(insbuf));

  /* Make sure that the point is in the visible region. */
  /* going into __edittext_recenter */
#if 0 /***/
  l_viewstart = INTVAL(viewstartline_instance_var -> __o_value);
  l_scrollmargin = INTVAL(scrollmargin_instance_var -> __o_value);
  l_viewlines = calc_viewlines ();
  l_line_width = calc_line_width ();
  l_viewheight = INTVAL(viewheightlines_instance_var -> __o_value);

  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);

  if (text_lines == NULL)
    return SUCCESS;

  line_no = 0;
  for (l = text_lines; l; l = l -> next) {
    if (l -> next) {
      if (l_point >= l -> start && l_point < l -> next -> start) {
	l_col = l_point - l -> start;
	/* point_new = shorter_line_adj (l, l -> next, l_col); *//***/
	/* EDITINTSET(point_instance_var, point_new); *//***/
	/* cursor_line = line_no; *//***/
	break;
      }
    } else {
      return SUCCESS;
    }
    ++line_no;
  }

  if ((line_no <= (l_viewstart + l_scrollmargin)) ||
      (line_no >= (l_viewstart + l_viewheight))) {
    EDITINTSET(viewstartline_instance_var,
	       (line_no - (l_viewheight / 2)));
  }

  delete_lines (&text_lines);
#endif  
  return keysym;
}

int __edittext_prev_char (OBJECT *editorpane_object) {
  int l_point;

  if (need_init)
    buf_init (editorpane_object);
  
  l_point = INTVAL(point_instance_var -> instancevars -> __o_value);

  if (l_point > 0) {
    --l_point;
    EDITINTSET(point_instance_var, l_point);
  }
  return 0;
}
			    
int __edittext_next_char (OBJECT *editorpane_object) {
  int l_point, l_textlength;

  if (need_init)
    buf_init (editorpane_object);
  
  l_point = INTVAL(point_instance_var -> instancevars -> __o_value);
  l_textlength = INTVAL(textlength_instance_var -> instancevars -> __o_value);

  if (l_point < l_textlength) {
    ++l_point;
    EDITINTSET(point_instance_var, l_point);
  }

  return 0;
}

int __edittext_prev_line (OBJECT *editorpane_object) {
  LINEREC *l, *text_lines = NULL;
  int l_point, l_viewstart, point_new, l_col, line_no, point_line,
    l_line_width;
  
  if (need_init)
    buf_init (editorpane_object);
  
  l_point = INTVAL(point_instance_var -> __o_value);
  l_viewstart = INTVAL(viewstartline_instance_var -> __o_value);
  
  l_line_width = calc_line_width ();
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);

  if ((text_lines == NULL) ||
      (text_lines -> next == NULL && text_lines -> prev == NULL))
    return SUCCESS;

  for (line_no = 0, l = text_lines; l; l = l -> next, ++line_no) {
    if (l -> next) {
      if (l_point >= l -> start && l_point < l -> next -> start) {
	if (l -> prev) {
	  l_col = l_point - l -> start;
	  point_new = shorter_line_adj (l, l -> prev, l_col);
	  point_line = line_no;
	  EDITINTSET(point_instance_var, point_new);
	}
	break;
      }
    } else {  /* Last line */
      if (l -> prev) {
	l_col = l_point - l -> start;
	point_line = line_no;
	point_new = shorter_line_adj (l, l -> prev, l_col);
	EDITINTSET(point_instance_var,point_new);
      }
      break;
    }
  }

  if (l_viewstart == 0) {
    delete_lines (&text_lines);
    return SUCCESS;
  }

  if (point_line < l_viewstart + c_scrollmargin) {
    --l_viewstart;
    EDITINTSET(viewstartline_instance_var, l_viewstart);
  }

  delete_lines (&text_lines);
  return SUCCESS;
}

int __edittext_next_line (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l;
  int l_point, point_new, l_col, l_viewstart, l_viewlines,
    line_no, cursor_line, l_viewstart_old, l_line_width;
  
  if (need_init)
    buf_init (editorpane_object);
  
  l_point = INTVAL(point_instance_var -> __o_value);
  l_viewstart = INTVAL(viewstartline_instance_var -> __o_value);
  l_viewlines = calc_viewlines ();
  l_line_width = calc_line_width ();
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);

  if (text_lines == NULL)
    return SUCCESS;

  line_no = 0;
  for (l = text_lines; l; l = l -> next) {
    if (l -> next) {
      if (l_point >= l -> start && l_point < l -> next -> start) {
	l_col = l_point - l -> start;
	point_new = shorter_line_adj (l, l -> next, l_col);
	EDITINTSET(point_instance_var, point_new);
	cursor_line = line_no;
	break;
      }
    } else {
      return SUCCESS;
    }
    ++line_no;
  }

  if (cursor_line < (l_viewstart + l_viewlines - 2)) {
    delete_lines (&text_lines);
    return SUCCESS;
  }

  /* Scroll down if needed and move the cursor back
     into the view. */
  l_viewstart_old = l_viewstart;
  while (cursor_line >= (l_viewstart + l_viewlines - c_scrollmargin)) {
    ++l_viewstart;
  }
  
  EDITINTSET(viewstartline_instance_var, l_viewstart);
  EDITINTSET(point_instance_var, l_viewstart);
  delete_lines (&text_lines);
  return SUCCESS;
}

int __edittext_line_start (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l;
  int l_point, l_line_width, point_new;

  if (need_init)
    buf_init (editorpane_object);
  l_point = INTVAL(point_instance_var -> __o_value);
  l_line_width = calc_line_width ();
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);
  if (text_lines == NULL)
    return SUCCESS;

  for (l = text_lines; l; l = l -> next) {
    if (l -> next) {
      if (l_point >= l -> start && l_point < l -> next -> start) {
	point_new = l -> start;
	break;
      }
    } else { /* last line */
      point_new = l -> start;
    }
  }
  EDITINTSET(point_instance_var, point_new);
  
  delete_lines (&text_lines);
  return SUCCESS;
}

int __edittext_line_end (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l;
  int l_point, i, l_line_width, point_new;
  
  if (need_init)
    buf_init (editorpane_object);
  l_point = INTVAL(point_instance_var -> __o_value);
  l_line_width = calc_line_width ();
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);
  for (l = text_lines; l; l = l -> next) {
    if (l -> next) {
      if (l_point >= l -> start && l_point < l -> next -> start) {
	point_new = l -> next -> start - 1;
	break;
      }
    } else { /* last line */
      if (*l -> text) {
	for (i = 0; l -> text[i] != 0; ++i)
	  ;
	point_new = l -> start + i;
      } else {
	point_new = l_point;
      }
    }
  }
  EDITINTSET(point_instance_var, point_new);

  delete_lines (&text_lines);
  return SUCCESS;
}

int __edittext_next_page (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l, *point_line;
  int l_viewheightlines, l_scrollmargin, l_viewstartline,
    scrolldistance, point_new, text_line_no, view_line_no,
    l_point, i, l_col, l_line_width;

  if (need_init)
    buf_init (editorpane_object);
  l_viewheightlines =
    INTVAL(viewheightlines_instance_var -> instancevars -> __o_value);
  l_scrollmargin =
    INTVAL(scrollmargin_instance_var -> instancevars -> __o_value);
  l_viewstartline =
    INTVAL(viewstartline_instance_var -> instancevars -> __o_value);
  l_point =
    INTVAL(point_instance_var -> instancevars -> __o_value);
  l_line_width = calc_line_width ();
  scrolldistance = l_viewheightlines - (l_scrollmargin * 2);
  
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);

  if (text_lines == NULL) {
    return SUCCESS;
  } else if (text_lines -> next == NULL) {
    delete_lines (&text_lines);
    return SUCCESS;
  }

  for (text_line_no = 0, view_line_no = 0, l = text_lines; l;
       l = l -> next, ++text_line_no) {
    if (text_line_no >= l_viewstartline) {
      if (l -> next) {
	if (l_point >= l -> start && l_point < l -> next -> start) {
	  point_line = l;
	  l_col = l_point - l -> start;
	  break;
	}
	++view_line_no;
      } else { /* last line */
	point_line = l;
	l_col = l_point - l -> start;
      }
    }
  }

  for (l = point_line, i = 0;
       (i < scrolldistance) && l -> next;
       l = l -> next, ++i)
    ;
  point_new = l -> start + l_col;
  EDITINTSET(point_instance_var, point_new);
  if (l -> next == NULL) /* last line */
    l_viewstartline +=
      ((i - l_scrollmargin < 0) ? 0 : i - l_scrollmargin);
  else
    l_viewstartline += scrolldistance;
  EDITINTSET(viewstartline_instance_var, l_viewstartline);
  delete_lines (&text_lines);

  return SUCCESS;
}

int __edittext_recenter (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l, *point_line;
  int l_line_width, l_viewheightlines, l_viewstartline,
    l_scrollmargin, point_line_no, l_point;

  if (need_init)
    buf_init (editorpane_object);

  l_line_width = calc_line_width ();
  l_viewheightlines =
    INTVAL(viewheightlines_instance_var -> instancevars -> __o_value);
  l_viewstartline =
    INTVAL(viewstartline_instance_var -> instancevars -> __o_value);
  l_point =
    INTVAL(point_instance_var -> instancevars -> __o_value);

  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);
  if (text_lines == NULL) {
    return SUCCESS;
  } else if (text_lines -> next == NULL) {
    delete_lines (&text_lines);
    return SUCCESS;
  }

  for (point_line_no = 0, l = text_lines; l;
       l = l -> next, ++point_line_no) {
    if (l -> next) {
      if ((l_point >= l -> start) && (l_point < l -> next -> start)) {
	point_line = l;
	break;
      }
    } else if (l_point >= l -> start) { /* last line */
      break;
    }
  }

  /* point is already within the view */
  if ((point_line_no >=  l_viewstartline) &&
      (point_line_no < l_viewstartline + l_viewheightlines)) {
    delete_lines (&text_lines);
    return SUCCESS;
  }
  
  l_viewstartline = (point_line_no - (l_viewheightlines / 2));

  EDITINTSET(viewstartline_instance_var, l_viewstartline);
  
  delete_lines (&text_lines);
  return SUCCESS;
}

int __edittext_scroll_down (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l;
  int l_point, l_col, l_viewstart, l_viewlines,
    line_no, cursor_line, l_line_width;

  if (need_init)
    buf_init (editorpane_object);
  
  l_viewstart = INTVAL(viewstartline_instance_var -> __o_value);
  l_viewlines = calc_viewlines ();
  l_line_width = calc_line_width ();
  l_point =
    INTVAL(point_instance_var -> instancevars -> __o_value);
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);

  if (text_lines == NULL)
    return SUCCESS;

  line_no = 0;
  for (l = text_lines; l; l = l -> next) {
    if (l -> next) {
      if (l_point >= l -> start && l_point < l -> next -> start) {
	l_col = l_point - l -> start;
	/* point_new = shorter_line_adj (l, l -> next, l_col); *//***/
	/* EDITINTSET(point_instance_var, point_new); *//***/
	/* cursor_line = line_no; *//***/
	break;
      }
    } else {
      return SUCCESS;
    }
    ++line_no;
  }

  /* Scroll down if needed and move the cursor back
     into the view. */
  ++l_viewstart;
  
  EDITINTSET(viewstartline_instance_var, l_viewstart);
  delete_lines (&text_lines);
  return SUCCESS;
}

int __edittext_scroll_up (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l;
  int l_point, l_col, l_viewstart, l_viewlines,
    line_no, cursor_line, l_line_width;

  if (need_init)
    buf_init (editorpane_object);
  
  l_viewstart = INTVAL(viewstartline_instance_var -> __o_value);
  l_viewlines = calc_viewlines ();
  l_line_width = calc_line_width ();
  l_point =
    INTVAL(point_instance_var -> instancevars -> __o_value);
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);

  if (text_lines == NULL)
    return SUCCESS;

  line_no = 0;
  for (l = text_lines; l; l = l -> next) {
    if (l -> next) {
      if (l_point >= l -> start && l_point < l -> next -> start) {
	l_col = l_point - l -> start;
	/* point_new = shorter_line_adj (l, l -> next, l_col); *//***/
	/* EDITINTSET(point_instance_var, point_new); *//***/
	/* cursor_line = line_no; *//***/
	break;
      }
    } else {
      return SUCCESS;
    }
    ++line_no;
  }

  /* Scroll down if needed and move the cursor back
     into the view. */
  if (l_viewstart > 0)
    --l_viewstart;
  
  EDITINTSET(viewstartline_instance_var, l_viewstart);
  delete_lines (&text_lines);
  return SUCCESS;
}

int __edittext_prev_page (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l, *point_line;
  int l_viewheightlines, l_scrollmargin, l_viewstartline,
    scrolldistance, point_new, text_line_no, view_line_no,
    l_point, i, l_col, l_line_width;

  if (need_init)
    buf_init (editorpane_object);
  l_viewheightlines =
    INTVAL(viewheightlines_instance_var -> instancevars -> __o_value);
  l_scrollmargin =
    INTVAL(scrollmargin_instance_var -> instancevars -> __o_value);
  l_viewstartline =
    INTVAL(viewstartline_instance_var -> instancevars -> __o_value);
  l_point =
    INTVAL(point_instance_var -> instancevars -> __o_value);
  l_line_width = calc_line_width ();
  scrolldistance = l_viewheightlines - (l_scrollmargin * 2);
  
  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);

  if (text_lines == NULL) {
    return SUCCESS;
  } else if (text_lines -> next == NULL) {
    delete_lines (&text_lines);
    return SUCCESS;
  }

  for (text_line_no = 0, view_line_no = 0, l = text_lines; l;
       l = l -> next, ++text_line_no) {
    if (text_line_no >= l_viewstartline) {
      if (l -> next) {
	if (l_point >= l -> start && l_point < l -> next -> start) {
	  point_line = l;
	  l_col = l_point - l -> start;
	  break;
	}
	++view_line_no;
      } else { /* last line */
	point_line = l;
	l_col = l_point - l -> start;
      }
    }
  }

  for (l = point_line, i = 0;
       (i < scrolldistance) && l -> prev;
       l = l -> prev, ++i)
    ;
  point_new = l -> start + l_col;
  EDITINTSET(point_instance_var, point_new);
  l_viewstartline -= scrolldistance;
  if (l_viewstartline < 0)
    l_viewstartline = 0;
  EDITINTSET(viewstartline_instance_var, l_viewstartline);
  delete_lines (&text_lines);

  return SUCCESS;
}

int __edittext_delete_char (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL;
  char *insbuf;
  int l_point, l_textlength, l_buflength, l_line_width;

  if (need_init)
    buf_init (editorpane_object);

  l_point = INTVAL(point_instance_var -> instancevars -> __o_value);
  l_textlength = INTVAL(textlength_instance_var -> instancevars -> __o_value);
  l_buflength = INTVAL(buflength_instance_var -> instancevars -> __o_value);
  l_line_width = calc_line_width ();
  

  insbuf = __xalloc (l_buflength);

  if (TEXT_VAR[l_point] != 0) {
    strcpy (insbuf, &(TEXT_VAR[l_point]));
    strcpy (&(TEXT_VAR[l_point]), &insbuf[1]);
    strcpy (insbuf, &(TEXT[l_point]));
    strcpy (&(TEXT[l_point]), &insbuf[1]);
    --l_textlength;
    EDITINTSET(textlength_instance_var, l_textlength);
  }

  __xfree (MEMADDR(insbuf));
  return SUCCESS;
}

int __edittext_text_start (OBJECT *editorpane_object) {
  char intbuf[64];
  if (need_init)
    buf_init (editorpane_object);
  EDITINTSET(point_instance_var, 0);
  EDITINTSET(viewstartline_instance_var, 0);

  return SUCCESS;
}

int __edittext_text_end (OBJECT *editorpane_object) {
  LINEREC *text_lines = NULL, *l;
  int n_lines, scrolldistance, l_viewstartline, l_viewheightlines,
    l_scrollmargin, l_line_width;
  if (need_init)
    buf_init (editorpane_object);
  l_viewheightlines =
    INTVAL(viewheightlines_instance_var -> instancevars -> __o_value);
  l_scrollmargin =
    INTVAL(scrollmargin_instance_var -> instancevars -> __o_value);
  l_viewstartline =
    INTVAL(viewstartline_instance_var -> instancevars -> __o_value);
  l_line_width = calc_line_width ();
  scrolldistance = l_viewheightlines - l_scrollmargin;

  split_text (text_instance_var -> __o_value, &text_lines, l_line_width);

  for (l = text_lines, n_lines = 0; l; l = l -> next, ++n_lines)
    ;

  l_viewstartline = n_lines - scrolldistance;
  if (l_viewstartline < 0)
    l_viewstartline = 0;

  EDITINTSET(point_instance_var, strlen(text_instance_var -> __o_value));
  EDITINTSET(viewstartline_instance_var, l_viewstartline);

  delete_lines (&text_lines);
  return SUCCESS;
}

static int cursor_char_width = -1;
static XFontStruct *cursor_xfs;

static char cursor_space[] = " ";

static LINEREC *calc_point_cursor (LINEREC *lines,
				   int point,
				   int view_start,
				   int *point_x,
				   int *point_y) {
  LINEREC *l, *point_line;
  int line_no;

  line_no = 1;
  for (l = lines; l; l = l -> next) {
    if (point >= l -> start) {
      if (l -> next == NULL) {
	point_line = l;
	break;
      } else if (point < l -> next -> start) {
	point_line = l;
	break;
      }
    }
    ++line_no;
  }
  *point_y = line_no - view_start;
  *point_x = point - point_line -> start;
  return point_line;
}

static char xa_primary_buf[0xffff];

#ifdef HAVE_XFT_H

XftDraw *ftDraw = NULL;
Pixmap cursor = 0;
Pixmap selectionchar = 0;
GC xft_cursor_gc, xft_selection_gc = NULL;
XGCValues xft_cursor_xgcv, xft_selection_xgcv;
XColor cursor_screen, cursor_exact;
XColor selection_screen, selection_exact;
XftDraw *ftCursor, *ftSelection;
XRenderColor cursorFGColor, selectionBGColor, selectionFGColor;
XftColor ftC, ftS, ftSfg;

static void init_ft_surfaces (Drawable w) {
  if (ftDraw == NULL) {
    ftDraw = 
      XftDrawCreate (display, w,
		     DefaultVisual (display, DefaultScreen (display)),
		     DefaultColormap (display, DefaultScreen (display)));
  }
  if (selectionchar == 0) {
    selectionchar = XCreatePixmap
      (display, w,
       selected_font -> max_advance_width,
       selected_font -> height,
       DefaultDepth (display, DefaultScreen (display)));
    XAllocNamedColor (display,
		      DefaultColormap (display, DefaultScreen (display)),
		      selectionbgcolorname, &selection_screen,
		      &selection_exact);
    xft_selection_xgcv.background = selection_screen.pixel;
    xft_selection_gc = XCreateGC (display, selectionchar, GCBackground,
				  &xft_selection_xgcv);
    ftSelection = 
      XftDrawCreate (display, selectionchar,
		     DefaultVisual (display, DefaultScreen (display)),
		     DefaultColormap (display, DefaultScreen (display)));
    __ctalkXftSetForegroundFromNamedColor (selectionbgcolorname);
    selectionBGColor.red = (unsigned short)__ctalkXftFgRed ();
    selectionBGColor.green = (unsigned short)__ctalkXftFgGreen ();
    selectionBGColor.blue = (unsigned short)__ctalkXftFgBlue ();
    selectionBGColor.alpha = (unsigned short)0xffff;
    XftColorAllocValue(display,
		       DefaultVisual (display, DefaultScreen (display)),
		       DefaultColormap (display, DefaultScreen (display)),
		       &selectionBGColor, &ftS);
    __ctalkXftSetForegroundFromNamedColor (fgcolorname);
    selectionFGColor.red = (unsigned short)__ctalkXftFgRed ();
    selectionFGColor.green = (unsigned short)__ctalkXftFgGreen ();
    selectionFGColor.blue = (unsigned short)__ctalkXftFgBlue ();
    selectionFGColor.alpha = (unsigned short)0xffff;
    XftColorAllocValue(display,
		       DefaultVisual (display, DefaultScreen (display)),
		       DefaultColormap (display, DefaultScreen (display)),
		       &selectionFGColor, &ftSfg);
  }
  if (cursor == 0) {
    cursor = XCreatePixmap (display, w,
			    selected_font -> max_advance_width,
			    selected_font -> height,
			    DefaultDepth (display,
					  DefaultScreen (display)));
    XAllocNamedColor (display,
		      DefaultColormap (display, DefaultScreen (display)),
		      fgcolorname, &cursor_screen, &cursor_exact);
    xft_cursor_xgcv.background = cursor_screen.pixel;
    xft_cursor_gc = XCreateGC (display, cursor, GCBackground,
			       &xft_cursor_xgcv);
    ftCursor = 
      XftDrawCreate (display, cursor,
		     DefaultVisual (display, DefaultScreen (display)),
		     DefaultColormap (display, DefaultScreen (display)));
    __ctalkXftSetForegroundFromNamedColor (bgcolorname);
    cursorFGColor.red = (unsigned short)__ctalkXftFgRed ();
    cursorFGColor.green = (unsigned short)__ctalkXftFgGreen ();
    cursorFGColor.blue = (unsigned short)__ctalkXftFgBlue ();
    cursorFGColor.alpha = (unsigned short)0xffff;
    XftColorAllocValue(display,
		       DefaultVisual (display, DefaultScreen (display)),
		       DefaultColormap (display, DefaultScreen (display)),
		       &cursorFGColor, &ftC);
  }
}

int __edittext_get_primary_selection (OBJECT *editorpane_object,
				      void **buf_out, int *size_out) {
  char handle_basename_path[FILENAME_MAX], intbuf[0xff],
    data_path[FILENAME_MAX], info_path[FILENAME_MAX];
  OBJECT *win_id_var, *content_var, *sstart_var, *send_var;
  Display *d_l;
  Drawable win_id, selection_win;
  int r, s_start, s_end, data_length = 0;
  FILE *f_info, *f_dat;

  /***/
  if ((win_id_var = __ctalkGetInstanceVariable (editorpane_object, "xWindowID",
						TRUE)) != NULL) {
    if ((d_l = XOpenDisplay (getenv ("DISPLAY"))) != NULL) {
      win_id = (Drawable)INTVAL(win_id_var -> __o_value);
      if ((selection_win = XGetSelectionOwner (d_l, XA_PRIMARY)) == win_id) {
	content_var = __ctalkGetInstanceVariable (editorpane_object, "text",
						  TRUE);
	sstart_var = __ctalkGetInstanceVariable (editorpane_object, "sStart", TRUE);
	send_var = __ctalkGetInstanceVariable (editorpane_object, "sEnd", TRUE);
	s_start = INTVAL(sstart_var -> __o_value);
	s_end = INTVAL(send_var -> __o_value);
	if (s_start == 0 && s_end == 0) {
	  *buf_out = NULL;
	  *size_out = 0;
	  return SUCCESS;
	}
	if (s_end < s_start) {
	  *size_out = s_start - s_end + 1;
	  *buf_out = (void *)__xalloc (*size_out);
	  memset (*buf_out, 0, *size_out);
	  strncpy (*buf_out, &content_var -> __o_value[s_end], s_start - s_end + 1);
	} else {
	  *size_out = s_end - s_start + 1;
	  *buf_out = (void *)__xalloc (*size_out);
	  memset (*buf_out, 0, *size_out);
	  strncpy (*buf_out, &content_var -> __o_value[s_start], s_end - s_start + 1);
	}
	return SUCCESS;
      }
      XCloseDisplay (d_l);
    }
  }

  strcatx (handle_basename_path, P_tmpdir, "/text", ctitoa (getpid (), intbuf),
	   NULL);
  memset ((void *)shm_mem, 0, SHM_BLKSIZE);
  make_req (shm_mem, PANE_GET_PRIMARY_SELECTION_REQUEST, 0, 0,
	    handle_basename_path);
  wait_req (shm_mem);

  strcatx (info_path, handle_basename_path, ".inf", NULL);
  strcatx (data_path, handle_basename_path, ".dat", NULL);

  if ((f_info = fopen (info_path, "r")) != NULL) {
    memset (intbuf, 0, 0xff);
    r = fread (intbuf, sizeof (char), 0xff, f_info);
    fclose (f_info);
    data_length = atoi (intbuf);
    unlink (info_path);
  }
  if (data_length > 0) {
    *buf_out = (void *)__xalloc (data_length + 1);
    if ((f_dat = fopen (data_path, "r")) != NULL) {
      r = fread (*buf_out, sizeof (char), data_length, f_dat);
      fclose (f_dat);
      unlink (data_path);
    }
  } else {
    *buf_out = NULL;
    *size_out = 0;
  }

  return SUCCESS;
}

int __xlib_get_primary_selection (Drawable pixmap, GC gc, char
				  *handle_basename_path) {
  Window owner;
  int fmt_return;
  unsigned long nitems, bytes_left, dummy;
  unsigned char *data;
  Atom type_return;
  char info_path[FILENAME_MAX], data_path[FILENAME_MAX],
    bytes_left_buf[0xff];
  FILE *f_inf, *f_dat;


  strcatx (info_path, handle_basename_path, ".inf", NULL);
  strcatx (data_path, handle_basename_path, ".dat", NULL);
  if ((owner = XGetSelectionOwner (display, XA_PRIMARY)) == None)
    return SUCCESS;

  XConvertSelection (display, XA_PRIMARY, XA_STRING, 0, owner,
		     (Time)time (NULL));

  XFlush (display);
  XGetWindowProperty (display, owner, XA_STRING, 0, 0, False,
		      AnyPropertyType, &type_return, &fmt_return,
		      &nitems, &bytes_left, &data);

  if (bytes_left) {
    if (!XGetWindowProperty (display, owner, XA_STRING, 0,
			     bytes_left, 0, AnyPropertyType,
			     &type_return, &fmt_return,
			     &nitems, &dummy, &data)) {
      if ((f_inf = fopen (info_path, "w")) != NULL) {
	ctitoa (bytes_left, bytes_left_buf);
	fwrite (bytes_left_buf, sizeof (char), strlen (bytes_left_buf), f_inf);
	fclose (f_inf);
      }
      if ((f_dat = fopen (data_path, "w")) != NULL) {
	fwrite (data, sizeof (char), bytes_left, f_dat);
	fclose (f_dat);
      }
    }
  }

  return SUCCESS;
}

int __xlib_clear_selection (XEvent *e) {
  *xa_primary_buf = 0;
  XSetSelectionOwner (display, XA_PRIMARY, None, CurrentTime);
  XFlush (display);
  return SUCCESS;
}

int __xlib_send_selection (XEvent *e) {
  XEvent ne;

  XChangeProperty 
    (display, e -> xselectionrequest.requestor,
     e -> xselectionrequest.property,
     XA_STRING, 8, PropModeReplace,
     (unsigned char *)xa_primary_buf, strlen (xa_primary_buf));
  ne.xselection.property = e -> xselectionrequest.property;
  ne.xselection.type = SelectionNotify;
  ne.xselection.display = e -> xselectionrequest.display;
  ne.xselection.requestor = e -> xselectionrequest.requestor;
  ne.xselection.selection = e -> xselectionrequest.selection;
  ne.xselection.target = e -> xselectionrequest.target;
  ne.xselection.time = e -> xselectionrequest.time;
  XSendEvent (display, e -> xselectionrequest.requestor, 
	      0, 0, &ne);
  XFlush (display);
  return SUCCESS;
}

int __xlib_render_text (Drawable pixmap, GC gc, char *fn) {
  char *text, *content;
  char starting_xlfd[255];
  int line_y, cursor_y, view_start_y, view_end_y, line_x, i_x;
  LINEREC *text_lines = NULL, *l;
  int r, point, visible_line;
  int selection_start, selection_end;
  XGCValues xgcv_content, xgcv_point;
  GC cursor_gc, selection_gc;
  XRenderColor fgColor;
  XftColor ftFg;
  /***/
  XColor fg_screen, fg_exact;
  XGCValues xgcv_foreground;
  XColor l_selection_screen, l_selection_exact;
  bool xft = false;
  Drawable selection_owner;

  internal_defaults ();

  text = edit_text_read (fn);

  if (!strncmp (text, FORMAT_BEGIN, FORMAT_BEGIN_LENGTH)) {
    edit_setformat (text);
    free (text);
    return 0;
  } 

  if ((r = sscanf (text, "%d:%d:%d:%d:", &point, &view_start_y,
		   &selection_start, &selection_end)) != 4) {
    fprintf
      (stderr,"ctalk: bad point: starting at the begining of buffer.\n");
    content = text;
  } else {
    content = strchr (text, ':'), ++content;
    content = strchr (content, ':'), ++content;
    content = strchr (content, ':'), ++content;
    content = strchr (content, ':'), ++content;
  }
  
  if (__ctalkXftInitialized ()) {
    xft = true;
    if (ftDraw == NULL) {
      init_ft_surfaces (pixmap);
    } else if (reformat) {
      XftDrawChange (ftDraw, pixmap);
    }
    if (line_width == 0) {
      line_width = (win_width - left_margin - right_margin) /
	selected_font -> max_advance_width;
    }
  }

  if (point > strlen (content)) {
    point = strlen (content);
  }
  split_text (content, &text_lines, line_width);

  if (xft) {
    __ctalkXftSetForegroundFromNamedColor (fgcolorname);
    fgColor.red = (unsigned short)__ctalkXftFgRed ();
    fgColor.green = (unsigned short)__ctalkXftFgGreen ();
    fgColor.blue = (unsigned short)__ctalkXftFgBlue ();
    fgColor.alpha = (unsigned short)0xffff;
    XftColorAllocValue(display,
		       DefaultVisual (display, DefaultScreen (display)),
		       DefaultColormap (display, DefaultScreen (display)),
		       &fgColor, &ftFg);
    line_height = selected_font -> ascent + selected_font -> descent;
  } else {
    /***/
    XAllocNamedColor (display,
		      DefaultColormap (display, DefaultScreen (display)),
		      fgcolorname, &fg_screen, &fg_exact);
    xgcv_foreground.foreground = fg_screen.pixel;
    XChangeGC (display, gc, GCForeground, &xgcv_foreground);
  }
  line_y = line_height;
  visible_line = 0;
  view_end_y = view_start_y + (win_height / line_height);

  for (l = text_lines; l; l = l -> next) {
    if (visible_line >= view_start_y && visible_line <= view_end_y) {
      if (xft) {
	XftDrawString8 (ftDraw, &ftFg, selected_font,
			left_margin, line_y,
			(unsigned char *)l -> text,
			strlen (l -> text));
      } else {
	XDrawImageString (display, pixmap, gc,
			  left_margin, line_y, 
			  (char *)l -> text, strlen ((char *)l -> text));
      }
      line_y += line_height;
    }
    ++visible_line;
  }

  /* Draw the point cursor. */
  l = calc_point_cursor (text_lines, point, view_start_y, &point_x, &point_y);
  cursor_y = point_y * line_height;
  if (xft) {
    if (cursor_char_width == -1) {
      cursor_char_width = selected_font -> max_advance_width;
    }
    XftDrawRect (ftCursor, &ftFg, 0, 0, cursor_char_width,
		 selected_font -> height);
    if (point_x >= strlen (l -> text)) {
      XftDrawString8 (ftCursor, &ftC, selected_font,
		      0, selected_font -> ascent,
		      " ", 1);
    } else {
      XftDrawString8 (ftCursor, &ftC, selected_font,
		      0, selected_font -> ascent,
		      (char *)(l -> text) + point_x, 1);
    }
    XCopyArea (display, cursor, pixmap, gc, 0, 0,
	       cursor_char_width, selected_font -> height,
	       left_margin + (cursor_char_width * point_x),
	       cursor_y - selected_font -> ascent);
  } else {
    XGetGCValues (display, gc, GCForeground|GCBackground|GCFont,
		  &xgcv_content);
    xgcv_point.foreground = xgcv_content.background;
    xgcv_point.background = xgcv_content.foreground;
    xgcv_point.font = xgcv_content.font;
    cursor_gc = XCreateGC (display, pixmap,
			   GCForeground|GCBackground|GCFont,
			   &xgcv_point);
    if (cursor_char_width == -1) {
      cursor_xfs = XQueryFont (display, xgcv_content.font);
      cursor_char_width = cursor_xfs -> max_bounds.width;
    }
    if (point_x >= strlen (l -> text)) {
      XDrawImageString (display, pixmap, cursor_gc,
			left_margin + (cursor_char_width * point_x),
			cursor_y, cursor_space, 1);
    } else {
      XDrawImageString (display, pixmap, cursor_gc,
			left_margin + (cursor_char_width * point_x),
			cursor_y, (char *)(l -> text) + point_x, 1);
    }
    XFreeGC (display, cursor_gc);
  } /* if (!xft) { */
   
  if (selection_start != 0) {
    if (selection_start > selection_end) {
      swap_selection_ends (&selection_start, &selection_end);
    }
    XSetSelectionOwner (display, XA_PRIMARY, win_xid,
			CurrentTime);
    XFlush (display);
    if ((selection_owner = XGetSelectionOwner (display, XA_PRIMARY))
	!= win_xid) {
      /* TODO *** Request the selection here, with retries. */
      fprintf (stderr, "ctalk: failed to get X selection owner = %u.\n",
	       (unsigned int)selection_owner);
    } else {
      memset (xa_primary_buf, 0, 0xffff);
      strncpy (xa_primary_buf, &content[selection_start],
	       (selection_end - selection_start) + 1);
    }
    if (xft) {
      if (cursor_char_width == -1) {
	cursor_char_width = selected_font -> max_advance_width;
      }
      line_y = line_height;
      visible_line = 0;
#if 1 /***/
      for (l = text_lines; l && (visible_line <= view_end_y); l = l -> next) {
	if (visible_line >= view_start_y) {
#else
       for (l = text_lines; l; l = l -> next) {
	if (visible_line >= view_start_y && visible_line <= view_end_y) {
#endif	  
	  line_x = l -> start;
	  for (i_x = 0; l -> text[i_x]; ++i_x) {
	    if ((line_x + i_x >= selection_start) &&
		(line_x + i_x <= selection_end)) {
	      XftDrawRect (ftSelection, &ftS, 0, 0, cursor_char_width,
			   selected_font -> height);
	      XftDrawString8 (ftSelection, &ftSfg, selected_font,
			      0, selected_font -> ascent,
			      (unsigned char *)&l -> text[i_x], 1);
	      XCopyArea (display, selectionchar, pixmap, gc, 0, 0,
			 cursor_char_width, selected_font -> height,
			 left_margin + (cursor_char_width * i_x),
			 line_y - selected_font -> ascent);
	    }
	  }
	  line_y += line_height;
	}
	++visible_line;
      }
    } else { /* if (xft)  */
      XGetGCValues (display, gc, GCForeground|GCBackground|GCFont,
		   &xgcv_content);
      xgcv_point.foreground = xgcv_content.foreground;
      xgcv_point.font = xgcv_content.font;
      XAllocNamedColor (display,
			DefaultColormap (display, DefaultScreen (display)),
			selectionbgcolorname, &l_selection_screen,
			&l_selection_exact);
      xgcv_point.background = l_selection_screen.pixel;
      selection_gc = XCreateGC (display, pixmap,
				GCForeground|GCBackground|GCFont,
				&xgcv_point);
      if (cursor_char_width == -1) {
	cursor_xfs = XQueryFont (display, xgcv_content.font);
	cursor_char_width = cursor_xfs -> max_bounds.width;
      }
      line_y = line_height;
      visible_line = 0;
      for (l = text_lines; l && (visible_line <= view_end_y); l = l -> next) {
	if (visible_line >= view_start_y) {
#if 0 /***/
      for (l = text_lines; l; l = l -> next) {
	if (visible_line >= view_start_y && visible_line <= view_end_y) {
#endif	  
	  line_x = l -> start;
	  for (i_x = 0; l -> text[i_x]; ++i_x) {
	    if ((line_x + i_x >= selection_start) &&
		(line_x + i_x <= selection_end)) {
	      if (xft) {
		XftDrawString8 (ftDraw, &ftFg, selected_font,
				left_margin, line_y,
				(unsigned char *)l -> text,
				strlen (l -> text));
	      } else {
		XDrawImageString (display, pixmap, selection_gc,
				  left_margin + (i_x * cursor_char_width),
				  line_y, 
				  (char *)&(l -> text[i_x]), 1);
	      }
	    }
	  }
	  line_y += line_height;
	}
	++visible_line;
      }

      XFreeGC (display, selection_gc);
    } /* if (xft) */
  }

  free (text);
  
  delete_lines (&text_lines);
  return SUCCESS;
}

#else  /* #ifdef HAVE_XFT_H */

int __edittext_get_primary_selection (OBJECT *editorpane_object,
				      void **buf_out, int *size_out) {
  char handle_basename_path[FILENAME_MAX], intbuf[0xff],
    data_path[FILENAME_MAX], info_path[FILENAME_MAX];
  int r, s_start, s_end, data_length = 0;
  OBJECT *win_id_var, *content_var, *sstart_var, *send_var;
  Display *d_l;
  Drawable win_id, selection_win;
  FILE *f_info, *f_dat;


  /***/
  if ((win_id_var = __ctalkGetInstanceVariable (editorpane_object, "xWindowID",
						TRUE)) != NULL) {
    if ((d_l = XOpenDisplay (getenv ("DISPLAY"))) != NULL) {
      win_id = (Drawable)INTVAL(win_id_var -> __o_value);
      if ((selection_win = XGetSelectionOwner (d_l, XA_PRIMARY)) == win_id) {
	content_var = __ctalkGetInstanceVariable (editorpane_object, "text",
						  TRUE);
	sstart_var = __ctalkGetInstanceVariable (editorpane_object, "sStart", TRUE);
	send_var = __ctalkGetInstanceVariable (editorpane_object, "sEnd", TRUE);
	s_start = INTVAL(sstart_var -> __o_value);
	s_end = INTVAL(send_var -> __o_value);
	if (s_start == 0 && s_end == 0) {
	  *buf_out = NULL;
	  *size_out = 0;
	  return SUCCESS;
	}
	if (s_end < s_start) {
	  *size_out = s_start - s_end + 1;
	  *buf_out = (void *)__xalloc (*size_out);
	  memset (*buf_out, 0, *size_out);
	  strncpy (*buf_out, &content_var -> __o_value[s_end], s_start - s_end + 1);
	} else {
	  *size_out = s_end - s_start + 1;
	  *buf_out = (void *)__xalloc (*size_out);
	  memset (*buf_out, 0, *size_out);
	  strncpy (*buf_out, &content_var -> __o_value[s_start], s_end - s_start + 1);
	}
	return SUCCESS;
      }
      XCloseDisplay (d_l);
    }
  }

  strcatx (handle_basename_path, P_tmpdir, "/text", ctitoa (getpid (), intbuf),
	   NULL);
  memset ((void *)shm_mem, 0, SHM_BLKSIZE);
  make_req (shm_mem, PANE_GET_PRIMARY_SELECTION_REQUEST, 0, 0,
	    handle_basename_path);
  wait_req (shm_mem);

  strcatx (info_path, handle_basename_path, ".inf", NULL);
  strcatx (data_path, handle_basename_path, ".dat", NULL);

  if ((f_info = fopen (info_path, "r")) != NULL) {
    memset (intbuf, 0, 0xff);
    r = fread (intbuf, sizeof (char), 0xff, f_info);
    fclose (f_info);
    data_length = atoi (intbuf);
    unlink (info_path);
  }
  if (data_length > 0) {
    *buf_out = (void *)__xalloc (data_length + 1);
    if ((f_dat = fopen (data_path, "r")) != NULL) {
      r = fread (*buf_out, sizeof (char), data_length, f_dat);
      fclose (f_dat);
      unlink (data_path);
    }
  } else {
    *buf_out = NULL;
    *size_out = 0;
  }

  return SUCCESS;
}

int __xlib_get_primary_selection (Drawable pixmap, GC gc, char
				  *handle_basename_path) {
  Window owner;
  int fmt_return;
  unsigned long nitems, bytes_left, dummy;
  unsigned char *data;
  Atom type_return;
  char info_path[FILENAME_MAX], data_path[FILENAME_MAX],
    bytes_left_buf[0xff];
  FILE *f_inf, *f_dat;


  strcatx (info_path, handle_basename_path, ".inf", NULL);
  strcatx (data_path, handle_basename_path, ".dat", NULL);
  if ((owner = XGetSelectionOwner (display, XA_PRIMARY)) == None)
    return SUCCESS;

  XConvertSelection (display, XA_PRIMARY, XA_STRING, 0, owner,
		     (Time)time (NULL));

  XFlush (display);
  XGetWindowProperty (display, owner, XA_STRING, 0, 0, False,
		      AnyPropertyType, &type_return, &fmt_return,
		      &nitems, &bytes_left, &data);

  if (bytes_left) {
    if (!XGetWindowProperty (display, owner, XA_STRING, 0,
			     bytes_left, 0, AnyPropertyType,
			     &type_return, &fmt_return,
			     &nitems, &dummy, &data)) {
      if ((f_inf = fopen (info_path, "w")) != NULL) {
	ctitoa (bytes_left, bytes_left_buf);
	fwrite (bytes_left_buf, sizeof (char), strlen (bytes_left_buf), f_inf);
	fclose (f_inf);
      }
      if ((f_dat = fopen (data_path, "w")) != NULL) {
	fwrite (data, sizeof (char), bytes_left, f_dat);
	fclose (f_dat);
      }
    }
  }

  return SUCCESS;
}

static char xa_primary_buf[0xffff];

int __xlib_clear_selection (XEvent *e) {
  *xa_primary_buf = 0;
  XSetSelectionOwner (display, XA_PRIMARY, None, CurrentTime);
  XFlush (display);
  return SUCCESS;
}

int __xlib_send_selection (XEvent *e) {
  XEvent ne;

  XChangeProperty 
    (display, e -> xselectionrequest.requestor,
     e -> xselectionrequest.property,
     XA_STRING, 8, PropModeReplace,
     (unsigned char *)xa_primary_buf, strlen (xa_primary_buf));
  ne.xselection.property = e -> xselectionrequest.property;
  ne.xselection.type = SelectionNotify;
  ne.xselection.display = e -> xselectionrequest.display;
  ne.xselection.requestor = e -> xselectionrequest.requestor;
  ne.xselection.selection = e -> xselectionrequest.selection;
  ne.xselection.target = e -> xselectionrequest.target;
  ne.xselection.time = e -> xselectionrequest.time;
  XSendEvent (display, e -> xselectionrequest.requestor, 
	      0, 0, &ne);
  XFlush (display);
  return SUCCESS;
}

int __xlib_render_text (Drawable pixmap, GC gc, char *fn) {
  char *text, *content;
  char starting_xlfd[255];
  int selection_start, selection_end;
  int line_y, cursor_y, view_start_y, view_end_y, line_x, i_x;
  LINEREC *text_lines = NULL, *l;
  int r, point, visible_line;
  XGCValues xgcv_content, xgcv_point;
  XColor l_selection_screen, l_selection_exact;
  GC cursor_gc, selection_gc;

  internal_defaults ();

  text = edit_text_read (fn);

  if (!strncmp (text, FORMAT_BEGIN, FORMAT_BEGIN_LENGTH)) {
    edit_setformat (text);
    free (text);
    return 0;
  } 

  if ((r = sscanf (text, "%d:%d:", &point, &view_start_y)) != 2) {
    fprintf
      (stderr,"ctalk: bad point: starting at the begining of buffer.\n");
    content = text;
  } else {
    content = strchr (text, ':');
    ++content;
    content = strchr (content, ':');
    ++content;
  }
  
  split_text (content, &text_lines, line_width);
  free (text);

  view_end_y = view_start_y + (win_height / line_height);

  line_y = line_height;
  visible_line = 0;
  for (l = text_lines; l; l = l -> next) {
    if (visible_line >= view_start_y && visible_line <= view_end_y) {
      XDrawImageString (display, pixmap, gc,
			left_margin, line_y, 
			(char *)l -> text, strlen ((char *)l -> text));
      line_y += line_height;
    }
    ++visible_line;
  }

  /* Draw the point cursor. */
  l = calc_point_cursor (text_lines, point, view_start_y, &point_x, &point_y);
  cursor_y = point_y * line_height;
  XGetGCValues (display, gc, GCForeground|GCBackground|GCFont,
		&xgcv_content);
  xgcv_point.foreground = xgcv_content.background;
  xgcv_point.background = xgcv_content.foreground;
  xgcv_point.font = xgcv_content.font;
  cursor_gc = XCreateGC (display, pixmap,
			 GCForeground|GCBackground|GCFont,
			 &xgcv_point);
  if (cursor_char_width == -1) {
    cursor_xfs = XQueryFont (display, xgcv_content.font);
    cursor_char_width = cursor_xfs -> max_bounds.width;
  }
  if (point_x >= strlen (l -> text)) {
    XDrawImageString (display, pixmap, cursor_gc,
		      left_margin + (cursor_char_width * point_x),
		      cursor_y, cursor_space, 1);
  } else {
    XDrawImageString (display, pixmap, cursor_gc,
		      left_margin + (cursor_char_width * point_x),
		      cursor_y, (char *)(l -> text) + point_x, 1);
  }
  XFreeGC (display, cursor_gc);
   
  if (selection_start != 0) {
    if (selection_start > selection_end) {
      /* If the selection start is after the selection end, swap
	 the endpoints. */
      int selection_tmp;
      selection_tmp = selection_start;
      selection_start = selection_end;
      selection_end = selection_tmp;
    }
    XGetGCValues (display, gc, GCForeground|GCBackground|GCFont,
		  &xgcv_content);
    xgcv_point.foreground = xgcv_content.foreground;
    xgcv_point.font = xgcv_content.font;
    XAllocNamedColor (display,
		      DefaultColormap (display, DefaultScreen (display)),
		      selectionbgcolorname, &l_selection_screen,
		      &l_selection_exact);
    xgcv_point.background = l_selection_screen.pixel;
    selection_gc = XCreateGC (display, pixmap,
			      GCForeground|GCBackground|GCFont,
			      &xgcv_point);
    if (cursor_char_width == -1) {
      cursor_xfs = XQueryFont (display, xgcv_content.font);
      cursor_char_width = cursor_xfs -> max_bounds.width;
    }
    line_y = line_height;
    visible_line = 0;
    for (l = text_lines; l && (visible_line <= view_end_y); l = l -> next) {
      if (visible_line >= view_start_y) {
#if 0 /***/
    for (l = text_lines; l; l = l -> next) {
      if (visible_line >= view_start_y && visible_line <= view_end_y) {
#endif	
	line_x = l -> start;
	for (i_x = 0; l -> text[i_x]; ++i_x) {
	  if ((line_x + i_x >= selection_start) &&
	      (line_x + i_x <= selection_end)) {
	    XDrawImageString (display, pixmap, selection_gc,
			      left_margin + (i_x * cursor_char_width),
			      line_y, 
			      (char *)&(l -> text[i_x]), 1);
	  }
	}
	line_y += line_height;
      }
      ++visible_line;
    }

    XFreeGC (display, selection_gc);
  }

  delete_lines (&text_lines);
  return SUCCESS;
}

#endif /* #ifdef HAVE_XFT_H */


#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

int __edittext_insert_at_point (OBJECT *editorpane_object,
				int keycode, int shift_state,
				int keypress) {
  fprintf (stderr, "__edittext_insert_at_point: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int __xlib_render_text (int pixmap, unsigned long int gc, char *fn) {
  fprintf (stderr, "__xlib_render_text: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int __edittext_prev_char (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_prev_char: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_next_char (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_next_char: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int __edittext_prev_line (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_prev_line: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int __edittext_next_line (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_next_line: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_line_start (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_line_start: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_line_end (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_line_end: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_next_page (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_next_page: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int __edittext_prev_page (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_prev_page: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_delete_char (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_delete_char: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_text_start (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_text_start: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_text_end (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_text_end: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_point_to_click (OBJECT *editorpane_object,
				int click_x, int click_y) {
  fprintf (stderr, "__edittext_point_to_click: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}
int __edittext_index_from_pointer (OBJECT *editorpane_object,
				int pointer_x, int pointer_y) {
  fprintf (stderr, "__edittext_index_from_pointer: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int __edittext_get_primary_selection (OBJECT *editorpane_object,
				      void ** buf_out, int *size_out) {
  fprintf (stderr, "__edittext_insert_selection: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}  

int __edittext_row_col_from_mark (OBJECT *editorpane_object,
				  int mark_x, int mark_y,
				  int *row_out, int *col_out) {
  fprintf (stderr, "__edittext_insert_selection: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int edittext_scroll_down (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_insert_selection: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int edittext_scroll_up (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_insert_selection: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

int __edittext_recenter (OBJECT *editorpane_object) {
  fprintf (stderr, "__edittext_insert_selection: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

unsigned int __edittext_xk_keysym (int keycode, int shift_state,
				   int keypress) {
  fprintf (stderr, "__edittext_insert_selection: This function requires "
	   "the X Window System.\n");
  exit (EXIT_FAILURE);
}

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
