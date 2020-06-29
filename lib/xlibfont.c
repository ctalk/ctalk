/* $Id: xlibfont.c,v 1.2 2020/06/29 03:03:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2017-2019  Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include "x11defs.h"
#include <X11/Xutil.h>
#include "xlibfont.h"
extern Display *display;   /* Defined in x11lib.c. */

#if X11LIB_FRAME
int clear_font_descriptors_internal (void) {
  return SUCCESS;
}
int load_xlib_fonts_internal (void *d, char *xlfd) {
  return SUCCESS;
}
#else /* X11LIB_FRAME */

extern char *shm_mem;
extern int mem_id;
static bool shmem_attached = false;

char **fixed_font_set;
XLIBFONT xlibfont = {NULL,};

/* used when loading new xlfds. */
static char old_xlfd[MAXLABEL];
static bool have_normal_xlfd = false;
static bool have_bold_xlfd = false;
static bool have_italic_xlfd = false;
static bool have_bold_italic_xlfd = false;

void clear_font_descriptors (void *d) {
  if (xlibfont.normal) {__xfree (MEMADDR(xlibfont.normal)); }
  if (xlibfont.bold) {__xfree (MEMADDR(xlibfont.bold)); }
  if (xlibfont.italic) {__xfree (MEMADDR(xlibfont.italic)); }
  if (xlibfont.bolditalic) {__xfree (MEMADDR(xlibfont.bolditalic)); }
  xlibfont.selectedfont = NULL;
  xlibfont.selected_xfs = NULL;
  if (d) {
    if (xlibfont.normal_xfs) { XFreeFont (d, xlibfont.normal_xfs); }
    if (xlibfont.bold_xfs) { XFreeFont (d, xlibfont.bold_xfs); }
    if (xlibfont.italic_xfs) { XFreeFont (d, xlibfont.italic_xfs); }
    if (xlibfont.bolditalic_xfs) { XFreeFont (d, xlibfont.bolditalic_xfs);
    }
  }
  memset (&xlibfont, 0, sizeof (XLIBFONT));
}

int load_xlib_fonts_internal (void *d, char *xlfd) {
  int n_user_fonts, nth_font;
  char **user_font_set, buf[0xff];

  if (str_eq (xlfd, old_xlfd))
    return SUCCESS;

  if (!shmem_attached) {
    if ((shm_mem = (char *)get_shmem (mem_id)) == NULL) {
      _exit (0);
    }
    shmem_attached = true;
  }

  if (strstr (xlfd, "fixed")) {
    /* Make sure we look up a reasonable descriptor. */
    user_font_set = XListFonts
      (d, FIXED_FONT_XLFD, MAXARGS, &n_user_fonts);
  } else {
    user_font_set = XListFonts (d, xlfd, MAXARGS, &n_user_fonts);
  }

  strcpy (old_xlfd, xlfd);
  clear_font_descriptors (d);
  have_normal_xlfd = have_bold_xlfd = have_italic_xlfd =
    have_bold_italic_xlfd = false;

  for (nth_font = 0; nth_font < n_user_fonts; nth_font++) {
    if (have_normal_xlfd && have_bold_xlfd && have_italic_xlfd &&
	have_bold_italic_xlfd)
      break;
    /* Just check the weight and slant arguments for now. */
    if (strstr (user_font_set [nth_font], "medium-r")) {
      if (!have_normal_xlfd) {
	if ((xlibfont.normal = strdup (user_font_set[nth_font])) != NULL)
	  xlibfont.normal_xfs = XLoadQueryFont (d, xlibfont.normal);
	have_normal_xlfd = true;
      }
    } else if (strstr (user_font_set[nth_font], "-bold-r")) {
      if (!have_bold_xlfd) {
	if ((xlibfont.bold = strdup (user_font_set[nth_font])) != NULL)
	  xlibfont.bold_xfs = XLoadQueryFont (d, xlibfont.bold);
	have_bold_xlfd = true;
      }
    } else if (strstr (user_font_set[nth_font], "medium-o") ||
	       strstr (user_font_set[nth_font], "medium-i")) {
      /* use either italic or oblique here and for bold-italic. */
      if (!have_italic_xlfd) {
	if ((xlibfont.italic = strdup (user_font_set[nth_font])) != NULL)
	  xlibfont.italic_xfs = XLoadQueryFont (d,
						xlibfont.italic);
	have_italic_xlfd = true;
      }
    } else if (strstr (user_font_set[nth_font], "bold-o") ||
	       strstr (user_font_set[nth_font], "bold-i")) {
      if (!have_bold_italic_xlfd) {
	if ((xlibfont.bolditalic = strdup (user_font_set[nth_font]))
	    != NULL)
	  xlibfont.bolditalic_xfs =
	    XLoadQueryFont (d, xlibfont.bolditalic);
	have_bold_italic_xlfd = true;
      }
    }
  }
  XFreeFontNames (user_font_set);
  if ((xlibfont.selectedfont = xlibfont.normal) != NULL) { /***/
    xlibfont.selected_xfs = xlibfont.normal_xfs;
    /* there might need to be more of this. */
    ctitoa (xlibfont.selected_xfs -> fid, buf);
    strcpy (&shm_mem[SHM_FONT_FID], buf); 
    strcpy (&shm_mem[SHM_FONT_XLFD], xlibfont.selectedfont);
  }
  return SUCCESS;
}

int load_xlib_fonts_internal_1t (void *d, char *xlfd) {
  int n_user_fonts, nth_font;
  char **user_font_set, buf[0xff];

  if (str_eq (xlfd, old_xlfd))
    return SUCCESS;

#if 0
  if (!shmem_attached) {
    if ((shm_mem = (char *)get_shmem (mem_id)) == NULL) {
      _exit (0);
      shmem_attached = true;
    }
  }
#endif  

  if (strstr (xlfd, "fixed")) {
    /* Make sure we look up a reasonable descriptor. */
    user_font_set = XListFonts
      (d, FIXED_FONT_XLFD, MAXARGS, &n_user_fonts);
  } else {
    user_font_set = XListFonts (d, xlfd, MAXARGS, &n_user_fonts);
  }

  strcpy (old_xlfd, xlfd);
  clear_font_descriptors (d);
  have_normal_xlfd = have_bold_xlfd = have_italic_xlfd =
    have_bold_italic_xlfd = false;

  for (nth_font = 0; nth_font < n_user_fonts; nth_font++) {
    if (have_normal_xlfd && have_bold_xlfd && have_italic_xlfd &&
	have_bold_italic_xlfd)
      break;
    /* Just check the weight and slant arguments for now. */
    if (strstr (user_font_set [nth_font], "medium-r")) {
      if (!have_normal_xlfd) {
	if ((xlibfont.normal = strdup (user_font_set[nth_font])) != NULL)
	  xlibfont.normal_xfs = XLoadQueryFont (d, xlibfont.normal);
	have_normal_xlfd = true;
      }
    } else if (strstr (user_font_set[nth_font], "-bold-r")) {
      if (!have_bold_xlfd) {
	if ((xlibfont.bold = strdup (user_font_set[nth_font])) != NULL)
	  xlibfont.bold_xfs = XLoadQueryFont (d, xlibfont.bold);
	have_bold_xlfd = true;
      }
    } else if (strstr (user_font_set[nth_font], "medium-o") ||
	       strstr (user_font_set[nth_font], "medium-i")) {
      /* use either italic or oblique here and for bold-italic. */
      if (!have_italic_xlfd) {
	if ((xlibfont.italic = strdup (user_font_set[nth_font])) != NULL)
	  xlibfont.italic_xfs = XLoadQueryFont (d,
						xlibfont.italic);
	have_italic_xlfd = true;
      }
    } else if (strstr (user_font_set[nth_font], "bold-o") ||
	       strstr (user_font_set[nth_font], "bold-i")) {
      if (!have_bold_italic_xlfd) {
	if ((xlibfont.bolditalic = strdup (user_font_set[nth_font]))
	    != NULL)
	  xlibfont.bolditalic_xfs =
	    XLoadQueryFont (d, xlibfont.bolditalic);
	have_bold_italic_xlfd = true;
      }
    }
  }
  xlibfont.selectedfont = xlibfont.normal;
  xlibfont.selected_xfs = xlibfont.normal_xfs;
  XFreeFontNames (user_font_set);
  /* this probably needs to be updated after the X client is
     started */
#if 0
  ctitoa (xlibfont.selected_xfs -> fid, buf);
  strcpy (&shm_mem[SHM_FONT_FID], buf); 
  strcpy (&shm_mem[SHM_FONT_XLFD], xlibfont.selectedfont);
#endif  
  return SUCCESS;
}

#ifdef HAVE_XFT_H

#include <fontconfig/fontconfig.h>
#include <X11/Xft/Xft.h>
#include FT_FREETYPE_H
#include "xftfont.h"

extern unsigned short fgred, fggreen, fgblue, fgalpha;

void sync_ft_font (bool sync_color_too) {
  char buf[0xff];
  if (!shm_mem)
    return;

  strcpy (&shm_mem[SHM_FONT_FT_FAMILY], __ctalkXftSelectedFamily ());
  shm_mem[SHM_FONT_FT_WEIGHT] = (unsigned short)__ctalkXftSelectedWeight ();
  shm_mem[SHM_FONT_FT_SLANT] = (unsigned short)__ctalkXftSelectedSlant ();
  shm_mem[SHM_FONT_FT_DPI] = (unsigned short)__ctalkXftSelectedDPI ();
  sprintf (buf, "%.1lf", __ctalkXftSelectedPointSize ());
  strcpy (&shm_mem[SHM_FONT_FT_PT_SIZE], buf);
  if (sync_color_too) {
    shm_mem[SHM_FONT_FT_RED] = fgred;
    shm_mem[SHM_FONT_FT_GREEN] = fggreen;
    shm_mem[SHM_FONT_FT_BLUE] = fgblue;
    shm_mem[SHM_FONT_FT_ALPHA] = fgalpha;
  }
}

int __ctalkSelectXFontFace (void *d, int drawable_id,
			    unsigned long int gc_ptr, int face) {
  char d_buf[MAXLABEL]; 
  char intbuf[MAXLABEL];
  int slant_def, weight_def, dpi_def;
  double ptsize_def;

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  if (!shm_mem)
    return ERROR;

  if (__ctalkXftInitialized ()) {
    sync_ft_font (false);
    switch (face)
      {
      case X_FACE_REGULAR:
	slant_def = XFT_SLANT_ROMAN;
	weight_def = XFT_WEIGHT_MEDIUM;
	break;
      case X_FACE_BOLD:
	slant_def = XFT_SLANT_ROMAN;
	weight_def = XFT_WEIGHT_BOLD;
	break;
      case X_FACE_ITALIC:
	slant_def = XFT_SLANT_ITALIC;
	weight_def = XFT_WEIGHT_MEDIUM;
	break;
      case X_FACE_BOLD_ITALIC:
	__ctalkXftSelectFont
	  ("", XFT_SLANT_ITALIC, XFT_WEIGHT_BOLD, 0, 0.0);
	slant_def = XFT_SLANT_ITALIC;
	weight_def = XFT_WEIGHT_BOLD;
	break;
      }

    ptsize_def = strtod (&shm_mem[SHM_FONT_FT_PT_SIZE], NULL);
    dpi_def = (int)shm_mem[SHM_FONT_FT_DPI];
    __ctalkXftSelectFont
      (&shm_mem[SHM_FONT_FT_FAMILY], slant_def, weight_def,
       dpi_def, ptsize_def);
    sync_ft_font (false);

    /* In case a program just selects a typeface for a single
       piece of text, send it to the server side directly. */
    sync_ft_font (true);
    strcatx (d_buf, ctitoa (face, intbuf), NULL);

    make_req (shm_mem, d, PANE_XLIB_FACE_REQUEST_FT,
	      drawable_id, gc_ptr, d_buf);

    wait_req (shm_mem);

  } else {

    strcatx (d_buf, ctitoa (face, intbuf), NULL);

    make_req (shm_mem, d, PANE_XLIB_FACE_REQUEST,
	      drawable_id, gc_ptr, d_buf);

    wait_req (shm_mem);
  }
#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = d;
  send_event.xgraphicsexpose.drawable = drawable_id;
  XSendEvent (d, drawable_id, False, 0L, &send_event);
#endif

  return SUCCESS;

}

int __ctalkX11UseFontBasic (void *d, int drawable_id, unsigned long int gc_ptr,
			    char *xlfd) {
  char d_buf[MAXLABEL];
  GC gc;
  XGCValues v;
  Display *l_d = NULL;
  int r;
  XFontStruct *xfs;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if (__client_pid () < 0) {
    if ((gc = (GC) gc_ptr) == NULL)
      return ERROR;
    if (d == NULL) {
      if ((l_d = XOpenDisplay (getenv ("DISPLAY"))) == NULL) {
	_error ("ctalk: This program requires the X Window System. Exiting.\n");
      }
      load_xlib_fonts_internal_1t (l_d, xlfd);
      v.font = xlibfont.selected_xfs -> fid;
      r = XChangeGC (l_d, gc, GCFont, &v);
      XCloseDisplay (l_d);
    } else {
      load_xlib_fonts_internal_1t (d, xlfd);
      v.font = xlibfont.selected_xfs -> fid;
      r = XChangeGC (d, gc, GCFont, &v);
    }
  } else {
    if (!shm_mem)
      return ERROR;

    sprintf (d_buf, ":%ld:%s", GCFont, xlfd);

    if (dialog_dpy ()) {

      __xlib_change_gc (d, drawable_id, (GC)gc_ptr, d_buf);

    } else {

      make_req (shm_mem, d, PANE_CHANGE_GC_REQUEST,
		drawable_id, gc_ptr, d_buf);
#ifdef GRAPHICS_WRITE_SEND_EVENT
      send_event.xgraphicsexpose.type = GraphicsExpose;
      send_event.xgraphicsexpose.send_event = True;
      send_event.xgraphicsexpose.display = display;
      send_event.xgraphicsexpose.drawable = drawable_id;
      XSendEvent (display, drawable_id, False, 0L, &send_event);
#endif
      wait_req (shm_mem);

    }
    return SUCCESS;
  }
}

#else /* HAVE_XFT_H */

int __ctalkSelectXFontFace (void *d, int drawable_id,
			    unsigned long int gc_ptr, int face) {
  char d_buf[MAXLABEL],
    intbuf[MAXLABEL];

#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  if (!shm_mem)
    return ERROR;

  strcatx (d_buf, ctitoa (face, intbuf), NULL);

  make_req (shm_mem, d, PANE_XLIB_FACE_REQUEST,
	    drawable_id, gc_ptr, d_buf);

#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = d;
  send_event.xgraphicsexpose.drawable = drawable_id;
  XSendEvent (d, drawable_id, False, 0L, &send_event);
#endif
  wait_req (shm_mem);

  return SUCCESS;

}

int __ctalkX11UseFontBasic (void *d, int drawable_id, unsigned long int gc_ptr,
			    char *xlfd) {
  char d_buf[MAXLABEL];
  GC gc;
  XGCValues v;
  Display *l_d = NULL;
  int r;
  XFontStruct *xfs;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  if (__client_pid () < 0) {
    if ((gc = (GC) gc_ptr) == NULL)
      return ERROR;
    if (d == NULL) {
      if ((l_d = XOpenDisplay (getenv ("DISPLAY"))) == NULL) {
	_error ("ctalk: This program requires the X Window System. Exiting.\n");
      }
      load_xlib_fonts_internal_1t (l_d, xlfd);
      v.font = xlibfont.selected_xfs -> fid;
      r = XChangeGC (l_d, gc, GCFont, &v);
      XCloseDisplay (l_d);
    } else {
      load_xlib_fonts_internal_1t (d, xlfd);
      v.font = xlibfont.selected_xfs -> fid;
      r = XChangeGC (d, gc, GCFont, &v);
    }
  } else {
    if (!shm_mem)
      return ERROR;

    sprintf (d_buf, ":%ld:%s", GCFont, xlfd);

    if (dialog_dpy ()) {

      __xlib_change_gc (d, drawable_id, (GC)gc_ptr, d_buf);

    } else {

      make_req (shm_mem, d, PANE_CHANGE_GC_REQUEST,
		drawable_id, gc_ptr, d_buf);
#ifdef GRAPHICS_WRITE_SEND_EVENT
      send_event.xgraphicsexpose.type = GraphicsExpose;
      send_event.xgraphicsexpose.send_event = True;
      send_event.xgraphicsexpose.display = display;
      send_event.xgraphicsexpose.drawable = drawable_id;
      XSendEvent (display, drawable_id, False, 0L, &send_event);
#endif
      wait_req (shm_mem);

    }
    return SUCCESS;
  }
}

#endif /* HAVE_XFT_H */


#endif /* X11LIB_FRAME */

#else /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */

static void gui_support_error (void) {
  x_support_error ();
}
int load_xlib_fonts_internal (void *d, char *xlfd) {
  x_support_error (); return ERROR;
}

int __ctalkSelectXFontFace (void *d, int drawable_id,
			    unsigned long int gc_ptr,
			    int face) {
  x_support_error (); return ERROR;
}
int __ctalkX11UseFontBasic (void *d, int drawable_id, unsigned long int gc_ptr,
			    char *xlfd) {
  x_support_error (); return ERROR;
}
#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */
