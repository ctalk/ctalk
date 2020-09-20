/* $Id: x11lib.c,v 1.2 2020/09/18 21:25:13 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2020  Robert Kiesling, rk3314042@gmail.com.
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
 *  Notes - This code should still be handled gently.  I've tried to
 *  avoid memory leaks, but it is possible to lose objects in the 
 *  X11TerminalStream methods, and probably elsewhere, if the code 
 *  is pushed.
 *
 *  Also, for Solaris 8, and probably Solaris 9, a program can
 *  generate a "too many open files" error.  The (possible - I haven't
 *  tried it) solution for this is the same as for servers like
 *  apache, mysql, and postgres - increase the system's maximum file
 *  descriptors from 256 to 65535.  Supposedly you can add lines
 *  similar to the following to the /etc/system file; for example: 
 *
 *    set rlim_fd_max=65535
 *
 *  Also see the ulimit manual page.  You should not need to worry
 *  about how much memory is in the data segment, however, because
 *  with this release, Ctalk sets the brk limit depending on how
 *  much memory the system can provide it.  And please let me know 
 *  if you see this message anyway.
 *
 *  Solaris 10 has several workarounds for this.  You should probably
 *  update if this is an issue.  Actually, a X application can run out 
 *  of file descriptors under any OS if it gets too large.
 *
 *  Linux is touchy about signal handling.  In particular, the default
 *  handlers do not seem to be reliably restartable.  If an application 
 *  receives a SIGUSR2 (though I'm not sure why the X libs should raise 
 *  this signal) or a SIGABRT, the only solution now is to kill the 
 *  application.  SA_RESETHAND seems to work best for all platforms.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"
#include "x11defs.h"

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/ucontext.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "xlibfont.h"

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
#include <sys/resource.h>
#endif

#define X11LIB_PRINT_COPIOUS_DEBUG_INFO 0
#define X11LIB_FRAME 0
#define MAX_RESIZE_RETRIES 3

Display *display = NULL;
static int screen, screen_depth;
int display_width, display_height;
static Window root;
static bool wm_xfce, wm_kwin, wm_xquartz;

extern char **fixed_font_set;
Font fixed_font = 0;
int n_fixed_fonts;

extern XLIBFONT xlibfont; /*  declared in xlibfont.c */

extern bool textcx_word_wrap;

extern char *ascii[8193];             /* from intascii.h */

#if !X11LIB_FRAME
static int frame_pane_sock_fd;
#endif

char *shm_mem;
int mem_id = 0;

Atom wm_delete_window;

static int __ctalkX11IOErrorHandler (Display *);
static int __ctalkX11ErrorHandler (Display *, XErrorEvent *);
static int (*default_io_handler) (Display *);
static int (*default_error_handler) (Display *, XErrorEvent *);
static OBJECT *__x11_pane_win_depth_value_object (OBJECT *);
static OBJECT *pane_pt_var (OBJECT *, char *, char *);
OBJECT *__x11_pane_font_id_value_object (OBJECT *);

/* In xrender.c */
int __xlib_draw_circle (Display *d, Drawable, GC, char *);
int __xlib_draw_rectangle (Display *, Drawable, GC, char *);
int __xlib_draw_line (Display *, Drawable, GC, char *);
int __xlib_draw_point (Display *, Drawable, GC, char *);
/* In edittext.c */
int __xlib_render_text (Display *, Drawable, GC, char *);
int __xlib_get_primary_selection (Display *, Drawable, GC, char *);
int __xlib_send_selection (XEvent *);
int __xlib_clear_selection (XEvent *);
int __xlib_set_primary_selection_owner (Display *, Drawable);
int __xlib_set_primary_selection_text (char *);

int __have_bitmap_buffers (OBJECT *);
void request_client_shutdown (void) {
  shm_mem[SHM_SHUTDOWN] = 't';
}

static int client_shutdown_requested (char *s) {
  return (s[SHM_SHUTDOWN] == 't') ? TRUE : FALSE;
}

/* a range check to help make sure that we didn't receive garbage
   as the GC address. Simply returns true for systems that it hasn't
   been tested on. */
#ifdef GC_RANGE_CHECK

bool gc_check (GC *gc_ptr) {

  if (sizeof (int) != 4)
    return true;

#ifdef __x86_64
  /* TODO - This needs checking. */
  return true;
#else  
#ifdef linux
  return (((unsigned int)* gc_ptr & 0xff000000) > 0);
#elif defined (__APPLE__)
  return (((unsigned int)* gc_ptr & 0xfff00000) > 0);
#else
  return true;
#endif
#endif   /* #ifdef __x86_64 */

}

#endif /* GC_RANGE_CHECK */

sigjmp_buf env;

void segv_sighandler (int signo) {
  siglongjmp (env, ERROR);
}

void set_client_sighandlers (void) {
  struct sigaction segv_act, segv_old_act;
  sigset_t sigmask;
  segv_act.sa_handler = segv_sighandler;
  sigemptyset (&sigmask);
  sigaddset (&sigmask, SIGSEGV);
  segv_act.sa_mask = sigmask;
  segv_act.sa_flags = SA_RESETHAND;
  sigaction (SIGSEGV, &segv_act, &segv_old_act);
}

void unset_client_sighandlers (void) {
  struct sigaction segv_act, segv_old_act;
  sigset_t sigmask;
  sigemptyset (&sigmask);
  sigaddset (&sigmask, SIGSEGV);
  segv_act.sa_handler = SIG_DFL;
  segv_act.sa_flags = SA_RESETHAND;
  sigaction (SIGSEGV, &segv_act, &segv_old_act);
}

/*
 *  We should let the signal handlers in class Application
 *  handle signals.  So these functions should not be needed.
 */
#if defined(__sparc__) && defined(__svr4__)
void x11_sig_handler (int sig, siginfo_t *sip, ucontext_t *ucp) {
#else /* Linux */ 
void x11_sig_handler (int sig, siginfo_t *sip, void *ucp) {
#endif
  struct sigaction old_action;
  sigemptyset (&(old_action.sa_mask));
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
  fprintf (stderr, "x11_sig_handler: Caught signal %d\n", sig);
#endif
  kill (0, sig);
}

static bool is_wm_xquartz (void) {
  struct stat s;
  /* Just look for the MacOS Frameworks directory... */
  if (!stat ("/System/Library/Frameworks", &s)) {
    return true;
  }
  return false;
}

static bool is_wm_xfce (Display *d) {
  /* Xfce4 names its desktop window "xfdesktop," so we
     simply look for the window name. */
  Window *children_return = NULL, root_return, parent_return;
  unsigned int nchildren_return;
  int i;
  char *window_name_return = NULL;
  bool retval = false;

  XQueryTree (d, root, &root_return, &parent_return,
	      &children_return, &nchildren_return);
  for (i = 0; (i < nchildren_return) && !retval; i++) {
    if (XFetchName(d, children_return[i], &window_name_return)) {
      if (str_eq (window_name_return, "xfdesktop")) {
	retval = true;
      }
    }
  }
  if (children_return)
    XFree (children_return);
  if (window_name_return)
    XFree (window_name_return);
  return retval;
}

static bool is_kwin_desktop (Display *d) {
  Atom kde_full_session, type_return;
  int format_return;
  unsigned long nitems_return, bytes_after_return;
  unsigned char *prop_return;
  if ((kde_full_session = XInternAtom (d, "KDE_FULL_SESSION",
				       true)) != 0) {
    XGetWindowProperty (d, root, kde_full_session, 0LL, 16,
			false, XA_STRING, &type_return, &format_return,
			&nitems_return, &bytes_after_return,
			&prop_return);
    if (str_eq ((char *)prop_return, "true"))
      return true;
  }
  return false;
}

/*
 *  Although we do *not* simply want to terminate the input client
 *  process, resetting the signal handlers seems to work best to avoid
 *  unchecked signal propagation on all platforms.
 */
static void x11_sig_init (void) {
  struct sigaction action, old_action;
  action.sa_flags = SA_SIGINFO|SA_RESETHAND;
  action.sa_sigaction = x11_sig_handler;
  sigemptyset (&(action.sa_mask));
  sigaction (SIGHUP, &action, &old_action);
  /*
   *  Some systems complain a lot if a call like XPending is
   *  interrupted by the user, so just use the system's CTRL-C
   *  handler.
   */
/*   sigaction (SIGINT, &action, &old_action); */
sigaction (SIGQUIT, &action, &old_action);
sigaction (SIGABRT, &action, &old_action);
sigaction (SIGPIPE, &action, &old_action);
sigaction (SIGTERM, &action, &old_action);
//  sigaction (SIGUSR1, &action, &old_action);
//  sigaction (SIGUSR2, &action, &old_action);
//  sigaction (SIGCHLD, &action, &old_action);
}

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
    return BlackPixel (display, screen);
  }
}

/* As above, but we use our own Display in case the program is
   still initializing. */
unsigned long lookup_pixel_d (Display *d, char *color) {
  XColor c;

  if (d == NULL)
    return BlackPixel (display, screen);

  if (!lookup_color (d, &c, color)) {
    return c.pixel;
  } else {
    return BlackPixel (d, screen);
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
    return BlackPixel (display, screen);
  }
}

extern int __xlib_clear_rectangle_basic (Display *, Drawable,
					 int, int, int, int, GC);

static void __xlib_put_str_scan (char *data, int *x, int *y, 
				 char **str) {
  char *p, *q;
  p = data;
  q = strchr (p, ':');
  if (!q) {
    fprintf (stderr, "__xlib_put_str (): Invalid message %s.\n", data);
    *x = *y = **str = 0;
    return;
  }
  errno = 0;
  *x = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_put_str", data);
    *x = *y = **str = 0;
    return;
  }
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    fprintf (stderr, "__xlib_put_str (): Invalid message %s.\n", data);
    *x = *y = **str = 0;
    return;
  }
  *y = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_put_str", data);
    *x = *y = **str = 0;
    return;
  }
  *str = ++q;
}

int __xlib_put_str (Display *d, Drawable w, GC gc, char *s) {
  int n;
  static Font c_font;
  static XFontStruct *fontinfo = NULL;
  static char *c_term;
  int x, y, width;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  __xlib_put_str_scan (s, &x, &y, &c_term);

#if X11LIB_PRINT_COPIOUS_DEBUG_INFO
#ifndef WITHOUT_X11_WARNINGS
  fprintf (stderr, "font_id (2) = %d, c_font = %d, c_font_name = %s\n", 
	   font_id, (int)c_font, c_font_name);
#endif
#endif

#if 0
  /* Due to changes in the X libraries, clearing the region behind the
     string may no longer be needed...  will try with more examples */
  if (xlibfont.selectedfont && xlibfont.selected_xfs) {
    width = XTextWidth (xlibfont.selected_xfs, c_term,
			strlen (c_term));
    __xlib_clear_rectangle_basic (d, w, x,
				  y - xlibfont.selected_xfs -> ascent,
				  width, 
				  xlibfont.selected_xfs -> ascent +
				  xlibfont.selected_xfs -> descent,
				  gc);
  } else if (xlibfont.selectedfont) {
    c_font = XLoadFont (d, xlibfont.selectedfont);
    fontinfo = XQueryFont (d, c_font);
    width = XTextWidth (fontinfo, c_term,
			strlen (c_term));
    __xlib_clear_rectangle_basic (d, w, x, y - fontinfo -> ascent, 
				  width, 
				  fontinfo -> ascent + fontinfo -> descent,
				  gc);
    XFreeFont (d, fontinfo);
  } else {
    c_font = XLoadFont (d, FIXED_FONT_XLFD);
    fontinfo = XQueryFont (d, c_font);
    width = XTextWidth (fontinfo, c_term,
			strlen (c_term));
    __xlib_clear_rectangle_basic (d, w, x, y - fontinfo -> ascent, 
				  width, 
				  fontinfo -> ascent + fontinfo -> descent,
				  gc);
    XFreeFont (d, fontinfo);
  }
#endif    
    
    /*
     *  VT100 erase line - has the effect of clearing the 
     *  line.
     */
    if (strcmp (c_term, "\x1b" "[2K"))
      XDrawImageString (d, w, gc, x, y, 
			c_term, 
			strlen (c_term));
  return SUCCESS;
}

#ifdef HAVE_XFT_H

#include <fontconfig/fontconfig.h>
#include <X11/Xft/Xft.h>
#include FT_FREETYPE_H
#include "xftfont.h"
#include "xrender.h" 



/* prototypes declared here so we don't need to add the gui types
   to ctalk.h */
extern XFTFONT ft_font;          /* declared in xftlib.h */
extern XftFont *selected_font;  /* THE selected font from xftlib.h */
extern bool new_color_spec (XRenderColor *);
extern void save_new_color_spec (XRenderColor *);
XftColor g_ftFg = {-1,};

XRENDERDRAWREC ft_str = {0, NULL, 0, 0, NULL, NULL, NULL};

int __xlib_put_str_ft (Display *d, Drawable w, GC gc, char *s) {
  int n;

  XftDraw *ftDraw = NULL;
  Colormap ftColorMap;
  XRenderColor fgColor;

  static char *c_term;
  int x, y, width, height;
  int str_x, str_y, str_width, str_height;
  char *fdsc;
  char family[256];
  int weight, slant, dpi;
  double pt_size;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if (shm_mem == NULL) {
    if ((shm_mem = get_shmem (mem_id)) == NULL) {
      fprintf (stderr, "ctalk: Couldn't get shared memory: %s.\n",
	       strerror (errno));
      return 0;
    }
  }

  if ((fdsc = strchr (s, ':')) == NULL)
    return ERROR;

  if ((n = sscanf (fdsc, ":%d:%d:", &x, &y)) != 2 )
    return ERROR;
  { /* We need to find the *third* colon. */
    char *_s, *_q;
    _q = strchr (fdsc + 1, ':'); /* we can skip past the first colon. */
    ++_q; _s = _q;
    _q = strchr (_s, ':');
    c_term = ++_q;
  }

  load_ft_font_faces_internal
    (&shm_mem[SHM_FONT_FT_FAMILY],
     strtod (&shm_mem[SHM_FONT_FT_PT_SIZE], NULL),
     shm_mem[SHM_FONT_FT_SLANT],
     shm_mem[SHM_FONT_FT_WEIGHT],
     shm_mem[SHM_FONT_FT_DPI]);

  fgColor.red = (unsigned short)shm_mem[SHM_FONT_FT_RED];
  fgColor.green = (unsigned short)shm_mem[SHM_FONT_FT_GREEN];
  fgColor.blue = (unsigned short)shm_mem[SHM_FONT_FT_BLUE];
  fgColor.alpha = (unsigned short)shm_mem[SHM_FONT_FT_ALPHA];

#define DEFAULT_VISUAL DefaultVisual(d, DefaultScreen (d))
  if (ft_str.draw == NULL || w != ft_str.drawable) {
    if (ft_str.draw != NULL)
      XftDrawDestroy (ft_str.draw);
    
    ft_str.drawable = w;
    ft_str.draw = XftDrawCreate (d, w,
				 DEFAULT_VISUAL,
				 DefaultColormap (d, (DefaultScreen (d))));
  }
  if (new_color_spec (&fgColor) || g_ftFg.pixel == -1) {
    ftColorMap =  DefaultColormap(d, DefaultScreen (d));
    XftColorAllocValue(d, DEFAULT_VISUAL, ftColorMap, &fgColor, &g_ftFg);
    save_new_color_spec (&fgColor);
  }
  /*
   *  VT100 erase line - has the effect of clearing the 
   *  line.
   */

  if (strcmp (c_term, "\x1b" "[2K"))
    XftDrawString8 (ft_str.draw, &g_ftFg, ft_font.normal, x, y,
		      (unsigned char *)c_term, 
		      strlen (c_term));

  if (dialog_dpy ()) {
    /* The next call may be a completely new server connection. */
    XftDrawDestroy (ft_str.draw);
    ft_str.draw = NULL;
    ft_str.drawable = 0;
    g_ftFg.pixel = -1;
  }

  return SUCCESS;
}

#endif /* HAVE_XFT_H */

#ifndef WITHOUT_X11_WARNINGS
#define WITHOUT_X11_WARNINGS
#endif

int __xlib_set_wm_name_prop (Display *d, Drawable drawable, GC gc, char *s) {
  XTextProperty text_prop, text_prop_return;
  Atom wm_name;
  char *s_1;
  if ((wm_name = XInternAtom (display, "WM_NAME", False)) == None) {
#ifndef WITHOUT_X11_WARNINGS 
    printf ("__xlib_set_wm_name_prop: WM_NAME property not found.\n");
    printf ("__xlib_set_wm_name_prop: (To disable these messages, build Ctalk with\n");
    printf ("__xlib_set_wm_name_prop: the --without-x11-warnings option.)\n");
#endif
    return ERROR;
  }  else {
    s_1 = strdup (s);
    XStringListToTextProperty (&s_1, 1, &text_prop);
    text_prop_return.value = NULL;
    do {
      XSetTextProperty (d, drawable, &text_prop, wm_name);
      if (text_prop_return.value)
	XFree (text_prop_return.value);
    } while (!XGetTextProperty (d, drawable, 
				&text_prop_return, wm_name));
    if (text_prop_return.value)
      XFree (text_prop_return.value);
    XFree (text_prop.value);
    __xfree (MEMADDR(s_1));
  }
  XFlush (d);
  return SUCCESS;
}

int __xlib_clear_window (Display *d, Drawable drawable, GC gc) {
   XClearWindow (d, drawable);
   return SUCCESS;
}

static void xlib_clear_rectangle_invalid_message (char *data) {
#ifndef WITHOUT_X11_WARNINGS
    printf ("__xlib_clear_rectangle: Invalid message %s.\n", data);
    printf ("__xlib_clear_rectangle: (To disable these messages, "
	    "build Ctalk with\n");
    printf ("__xlib_clear_rectangle: the --without-x11-warnings option.)\n");
#endif
}

static void __xlib_clear_rectangle_scan (char *data, 
					 int *x, int *y, 
					 int *width, int *height,
					 int *panebuffer_id, 
					 int *panebackingstore_id) {
  char *p, *q;
  p = data;
  q = strchr (p, ':');
  if (!q) {
    xlib_clear_rectangle_invalid_message (data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
  errno = 0;
  *x = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_clear_rectangle", data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    xlib_clear_rectangle_invalid_message (data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
  *y = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_clear_rectangle", data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    xlib_clear_rectangle_invalid_message (data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
  *width = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_clear_rectangle", data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    xlib_clear_rectangle_invalid_message (data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
  *height = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_clear_rectangle", data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    xlib_clear_rectangle_invalid_message (data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
  *panebuffer_id = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_clear_rectangle", data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
    
  errno = 0;
  p = ++q;
  q = strchr (p, ':');
  if (!q) {
    xlib_clear_rectangle_invalid_message (data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
  *panebackingstore_id = strtol (p, &q, 10);
  if (errno) {
    strtol_error (errno, "__xlib_clear_rectangle", data);
    *x = *y = *width = *height = *panebuffer_id = *panebackingstore_id = 0;
    return;
  }
    
}

int __xlib_clear_rectangle (Display *d, Drawable drawable, GC gc, char *data) {
  int r, x, y, width, height;
  int panebuffer_id, panebackingstore_id;
  XGCValues gcv, old_gcv;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  __xlib_clear_rectangle_scan (data, &x, &y, &width, &height,
			       &panebuffer_id, &panebackingstore_id);

  if (!panebuffer_id && !panebackingstore_id) {
    XClearArea (d, drawable, x, y, width, height, True);
  } else {
#define CLEAR_RECTANGLE_GCV_MASK (GCFunction|GCForeground|GCBackground|GCFillStyle)
    XGetGCValues (d, gc,
		  CLEAR_RECTANGLE_GCV_MASK, &old_gcv);
    gcv.background = old_gcv.background;
    gcv.foreground = old_gcv.background;
    gcv.fill_style = FillSolid;
    gcv.function = GXcopy;
    XChangeGC (d, gc, CLEAR_RECTANGLE_GCV_MASK, &gcv);

    XFillRectangle (d, (Drawable)panebuffer_id, gc,
 		    x, y, width, height);

    XChangeGC (d, gc, CLEAR_RECTANGLE_GCV_MASK, &old_gcv);

  }
  return SUCCESS;
}

extern int __xlib_clear_pixmap (Drawable, unsigned int, unsigned int,
				GC gc);
int __xlib_resize_window (Display *d, Drawable drawable, GC gc, char *data) {
  unsigned int width, height;
  int depth, r;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if ((r = sscanf (data, "%u:%u:%d", 
		   &width, &height, &depth)) != 3) { 
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, "__xlib_resize_window (sscanf width, height): %s : r = %d.\n", strerror (errno), r);
#endif
    return ERROR;
  }
  r = XResizeWindow (d, drawable, width, height);
  /* The C library uses the opposite convention - 0 for success,
     non-zero for error. Changes this to the C convention */
  r = (r == 0) ? 1 : 0;
  ctitoa (r, data);
  return r;
}

int __xlib_move_window (Display *d, Drawable drawable, GC gc, char *data) {
  int x, y, r;
  if ((r = sscanf (data, "%d:%d", &x, &y)) != 2) {
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, "__xlib_move_window (sscanf x, y, width, height): %s : r = %d.\n", strerror (errno), r);
#endif
    return ERROR;
  }
  XMoveWindow (d, drawable, x, y);
  return SUCCESS;
}

int __xlib_refresh_window (Display *d, Drawable drawable, GC gc, char *data) {
  int r;
  long unsigned int buffer_id;
  unsigned int src_x_org, src_y_org, width, height,
    dest_x_org, dest_y_org;
  XGCValues v;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if ((r = sscanf (data, "%lu:%u:%u:%u:%u:%u:%u", 
		   &buffer_id, 
		   &src_x_org, &src_y_org,
		   &width, &height,
		   &dest_x_org, &dest_y_org)) != 7) {
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, "__xlib_refresh_window: %s : r = %d.\n", 
	     strerror (errno), r);
#endif
    return ERROR;
  }
  
  /* v.function = GXcopy;
     XChangeGC (d, gc, GCFunction, &v); */
  
  XCopyArea (d, (Drawable)buffer_id, drawable, gc, 
	     src_x_org, src_y_org,
	     width, height,
	     dest_x_org, dest_y_org);
  return SUCCESS;
}

int __xlib_use_cursor (Display *d, Drawable drawable, GC gc, char *data) {
  int r;
  unsigned int cursor;

  errno = 0;
  cursor = strtoul (data, NULL, 10);
  if (errno)
    strtol_error (errno, "__xlib_use_cursor", data);
  XDefineCursor (d, drawable, (Cursor)cursor);
  return SUCCESS;
}

int __xlib_change_gc (Display *d, Drawable drawable, GC gc, char *data) {
  int r;
  unsigned long int valuemask;
  Font f;
  char value[MAXLABEL];
  XGCValues v;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if (d == NULL) {
    _warning ("ctalk: __xlib_change_gc: Display connection is NULL.\n");
    __warning_trace ();
    return ERROR;
  }

  if ((r = sscanf (data, ":%ld:%s", &valuemask, value)) != 2) {
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, 
	     "__xlib_change_gc: %s : r = %d.\n",
	     strerror (errno), r);
    _warning ("__xlib_change_gc: "
	      "(To disable these messages, build Ctalk with\n");
    _warning ("__xlib_change_gc: "
	      "the --without-x11-warnings option.)\n");
#endif
    return ERROR;
  }
  switch (valuemask)
    {
    case GCFont:
      load_xlib_fonts_internal (d, value);
      /* xlibfont.selectedfont is set in xlibfont.c */
      if (xlibfont.selected_xfs) {
	v.font = xlibfont.selected_xfs -> fid;
	r = XChangeGC (d, gc, valuemask, &v);
      } else {
	fprintf (stderr, "__ctalkX11UseFontBasic: Couldn't load font, \"%s.\"\n", value);
      }
      break;
    case GCBackground:
      v.background = lookup_pixel_d (d, value);
      r = XChangeGC (d, gc, valuemask, &v);
      break;
    case GCForeground:
      v.foreground = lookup_pixel_d (d, value);
      r = XChangeGC (d, gc, valuemask, &v);
      break;
    }
  return SUCCESS;
}

int __xlib_resize_pixmap (Display *d, Drawable drawable, GC gc,
			  char *data) {
  int r;
  Pixmap old_pixmap, new_pixmap;
  unsigned int old_width, old_height,
    new_width, new_height;
  int depth;
  
#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if ((r = sscanf (data, "%d:%u:%u:%u:%u:%d", 
		   (int *) &old_pixmap, &old_width,
		   &old_height, &new_width, &new_height, &depth)) 
      != 6) {
#ifndef WITHOUT_X11LIB_WARNINGS
    fprintf (stderr, 
	     "__xlib_resize_pixmap: %s : r = %d.\n",
	     strerror (errno), r);
#endif
    return ERROR;
  }
  if ((new_pixmap = XCreatePixmap (d, drawable, 
				   new_width, new_height,
				   depth)) == 0) {
    strcpy (data, "0");
    return ERROR;
  }
  __xlib_clear_pixmap (new_pixmap, new_width, new_height, gc);
  XFreePixmap (d, old_pixmap);
  __ctalkDecimalIntegerToASCII (new_pixmap, data);
  return SUCCESS;
}

static char *xpm_data = NULL;
static char **xpm_lines = NULL;

static int __read_xpm_data (char *handle_path) {

  FILE *f;
  int r;
  struct stat statbuf;
  long long int file_size, bytes_read;
  char *p, *q;
  char *line_buf_ptr;
  int width, height, n_colors, chars_per_pixel;
  char linebuf[MAXMSG];
  int i;

  if ((f = fopen (handle_path, "r")) == NULL) {

    printf ("__read_xpm_data (fopen): %s: %s.\n",
	    handle_path, strerror (errno));

    return ERROR;
  }

  if ((r = stat (handle_path, &statbuf)) != 0) {
    printf ("__read_xpm_data (stat): %s: %s.\n",
	    handle_path, strerror (errno));

    return ERROR;
  }

  file_size = statbuf.st_size;

  /* freed in the calling fn. */
  if ((xpm_data = (char *)__xalloc (file_size + 1)) == NULL) {
    printf ("__read_xpm_data (malloc): %s: %s.\n",
	    handle_path, strerror (errno));
    return ERROR;
  }

  if ((bytes_read = fread ((void *)xpm_data, sizeof (char), 
			   file_size, f)) != file_size) {
    printf ("__read_xpm_data (malloc): %s: %s.\n",
	    handle_path, strerror (errno));
    return ERROR;
  }

  p = xpm_data;
  if ((q = strchr (p, '\n')) == NULL) {
    printf ("__xlib_xpm_from_data: parse error.\n");
    return ERROR;
  }
  memset (linebuf, 0, MAXMSG);
  strncpy (linebuf, p, q - p);

  if ((r = sscanf (linebuf, "%d %d %d %d", 
		   &width, &height, &n_colors, &chars_per_pixel))
      != 4) {
    printf ("__read_xpm_data: parse error.\n");
    return ERROR;
  }

  if ((xpm_lines = (char **)malloc ((n_colors + height + 2) * 
				    sizeof (char *)))
      == NULL) {
    printf ("__read_xpm_data (malloc): %s.\n",
	    strerror (errno));
    return ERROR;
  }

  memset (xpm_lines, 0, sizeof (xpm_lines));

  p = xpm_data;
  for (i = 0; i <= (n_colors + height); i++) {

    if ((q = strchr (p, '\n')) == NULL) {
      printf ("__read_xpm_data parse error.\n");
      return ERROR;
    }

    if ((line_buf_ptr = (char *)malloc (((width * chars_per_pixel) + 2) 
					* sizeof (char)))
	== NULL) {
      printf ("__read_xpm_data (malloc): %s.\n",
	      strerror (errno));
      return ERROR;
    }

    memset (line_buf_ptr, 0, (width + 1) * sizeof (char));
    strncpy (line_buf_ptr, p, q - p);

    xpm_lines[i] = line_buf_ptr;

    p = q + 1;
  }


  if ((r = fclose (f)) != 0) {
    printf ("__read_xpm_data (fclose): %s: %s.\n",
	    handle_path, strerror (errno));
    return ERROR;
  }

  return SUCCESS;
}

static unsigned long rgb_to_pix (Display *d, 
				 int r, int g, int b) {
  XColor s;
  s.red = r;
  s.green = g;
  s.blue = b;
  (void)XAllocColor 
    (d, DefaultColormap 
     (d, DefaultScreen (d)),
     &s);
   return s.pixel;
}

#define HEXASCIITOBINARY(d) (\
  (d >= '0' && d <= '9') ? d - 48 :				\
  ((d >= 'A' && d <= 'F') ? d - 55 :				\
   ((d >= 'a' && d <= 'f') ? d - 87 : 0)))

int __xlib_xpm_from_data (Display *d, Drawable drawable, GC gc, char *data) {
  int r;
  char handle_path[FILENAME_MAX];
  char *p, *q;
  char linebuf[MAXMSG];
  int width, height, n_colors, chars_per_pixel;
  int x, y, x_out, y_out, nth_color,
    have_color;
  int user_x_org, user_y_org;
  int color_1_row, color_n_row;
  char old_color_pixels[4] = "";
  int rr = 0, gg = 0, bb = 0;
  int rrr = 0, ggg = 0, bbb = 0;
  int rrrr = 0, gggg = 0, bbbb = 0;
  int rrggbb;
  unsigned long pix;
  XGCValues gcv;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc))
    return ERROR;

#endif

  if ((r = sscanf (data, ":%d:%d:%s", 
		   &user_x_org, &user_y_org, handle_path)) != 3) {
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, 
	     "__xlib_xpm_from_data: %s : r = %d: data = %s.\n",
	     strerror (errno), r, data);
#endif
    return ERROR;
  }

  if (__read_xpm_data (handle_path) < 0)
    return ERROR;


  if ((r = sscanf (xpm_lines[0], "%d %d %d %d", 
		   &width, &height, &n_colors, &chars_per_pixel))
      != 4) {
#ifndef WITHOUT_X11_WARNINGS
    printf ("__xlib_xpm_from_data: parse error.\n");
#endif
    goto xpm_from_pixmap_done;
  }

  color_1_row = 1;
  color_n_row = n_colors;

  for (y_out = 0, y = color_n_row + 1; y <= (color_n_row + height);
       ++y_out, y++) {
    for (x = 0, x_out = 0; x_out < width; ++x_out, x += chars_per_pixel) {
      if (!strncmp (&xpm_lines[y][x], old_color_pixels, chars_per_pixel)) {
   	// Re-use a color if it's the same as the previous pixel.
	XDrawPoint (d, drawable, gc, x_out + user_x_org, 
		    y_out + user_y_org);
      } else {
   	for (nth_color = color_1_row, have_color = FALSE;
   	     (nth_color <= color_n_row) && !have_color; nth_color++) {
   	  if (!strncmp (&xpm_lines[y][x], xpm_lines[nth_color], chars_per_pixel)) {
   	    char *p, *q;
   	    if ((p = strchr ((char *)xpm_lines[nth_color], 'c')) != NULL) {
   	      if ((q = strchr (p, '#')) != NULL) {
		if (strlen (q) == 7) {
		  /* 24-bit color. */
		  rr = (HEXASCIITOBINARY(*(q + 1)) << 4) + 
		    HEXASCIITOBINARY(*(q + 2));
		  gg = (HEXASCIITOBINARY(*(q + 3)) << 4) + 
		    HEXASCIITOBINARY(*(q + 4));
		  bb = (HEXASCIITOBINARY(*(q + 5)) << 4) + 
		    HEXASCIITOBINARY(*(q + 6));
		} else if (strlen (q) == 10) {
		  /* 32-bit color - no live examples of this yet, so
		     it's untested.  See the comment in glutlib.c. */
		  rrr = ((HEXASCIITOBINARY(*(q + 1)) << 6) +
			 (HEXASCIITOBINARY(*(q + 2)) << 4) +
			 (HEXASCIITOBINARY(*(q + 3))));
		  ggg = ((HEXASCIITOBINARY(*(q + 4)) << 6) +
			 (HEXASCIITOBINARY(*(q + 5)) << 4) +
			 (HEXASCIITOBINARY(*(q + 6))));
		  bbb = ((HEXASCIITOBINARY(*(q + 7)) << 6) +
			 (HEXASCIITOBINARY(*(q + 8)) << 4) +
			 (HEXASCIITOBINARY(*(q + 9))));
		  rr = rrr >> 2;
		  gg = ggg >> 2;
		  bb = bbb >> 2;
		} else if (strlen (q) == 13) {
		  /* 48-bit color. */
		  rrrr = ((HEXASCIITOBINARY(*(q + 1)) << 8) +
			  (HEXASCIITOBINARY(*(q + 2)) << 6) +
			  (HEXASCIITOBINARY(*(q + 3)) << 4) +
			  (HEXASCIITOBINARY(*(q + 4))));
		  gggg = ((HEXASCIITOBINARY(*(q + 5)) << 8) +
			  (HEXASCIITOBINARY(*(q + 6)) << 6) +
			  (HEXASCIITOBINARY(*(q + 7)) << 4) +
			  (HEXASCIITOBINARY(*(q + 8))));
		  bbbb = ((HEXASCIITOBINARY(*(q + 9)) << 8) +
			  (HEXASCIITOBINARY(*(q + 10)) << 6) +
			  (HEXASCIITOBINARY(*(q + 11)) << 4) +
			  (HEXASCIITOBINARY(*(q + 12))));
		  rr = rrrr >> 4;
		  gg = gggg >> 4;
		  bb = bbbb >> 4;
		}
   		have_color = TRUE;
		pix = rgb_to_pix (d, rr << 8, gg << 8, bb << 8);
		gcv.foreground = pix;
		XChangeGC (d, gc, GCForeground, &gcv);
		XDrawPoint (d, drawable, gc, 
			    x_out + user_x_org, 
			    y_out + user_y_org);

		switch (chars_per_pixel)
		  {
		  case 1:
		    old_color_pixels[0] = xpm_lines[y][x];
		    old_color_pixels[1] = '\0';
		    break;
		  case 2:
		    old_color_pixels[0] = xpm_lines[y][x];
		    old_color_pixels[1] = xpm_lines[y][x+1];
		    old_color_pixels[2] = '\0';
		    break;
		  case 3:
		    old_color_pixels[0] = xpm_lines[y][x];
		    old_color_pixels[1] = xpm_lines[y][x+1];
		    old_color_pixels[2] = xpm_lines[y][x+2];
		    old_color_pixels[3] = '\0';
		    break;
		  }
   	      } else {
   		// A named color.  TODO
   	      }
   	    }
   	  }
   	}
      }
    } // for (x = 0, x_out = 0; x_out < width; ++x_out, x += chars_per_pixel)
  } // for (y_out = 0, y = color_n_row + 1; y <= (color_n_row + height); ...


 xpm_from_pixmap_done:

  if (xpm_data) {
    __xfree (MEMADDR(xpm_data));
  }

  if (xpm_lines) {
    int __i;

    for (__i = 0; __i <= (n_colors + height); ++__i) 
      __xfree (MEMADDR(xpm_lines[__i]));

    __xfree (MEMADDR(xpm_lines));
  }

  return SUCCESS;
}


int __xlib_set_resources (Display *d, Drawable drawable, GC gc,
			  char *data) {
  XClassHint *class_hints;
  int r;
  char *q, *q1;
  static char resource_name[1024];
  static char resource_class[1024];

  if ((q = strchr (data, ':')) == NULL) {
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, 
	     "__xlib_set_resources: %s : data format error.\n",
	     data);
#endif
    return ERROR;
  }

  ++q;

  if ((q1 = strchr (q, ':')) == NULL) {
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, 
	     "__xlib_set_resources: %s : data format error.\n",
	     data);
#endif
    return ERROR;
  }

  memset (resource_name, 0, 1024);
  memset (resource_class, 0, 1024);
  strncpy (resource_name, q, q1 - q);
  ++q1;
  class_hints = XAllocClassHint ();

  class_hints -> res_name = strdup (resource_name);
  class_hints -> res_class = strdup (q1);

  XSetClassHint (display, (Window)drawable, class_hints);

  XFlush (display);

  __xfree (MEMADDR(class_hints -> res_name));
  __xfree (MEMADDR(class_hints -> res_class));
  XFree (class_hints);

  return SUCCESS;
}

int __xlib_change_window_background (Display *d, Drawable drawable, GC gc,
				     char *data) {
  int r;
  char color[MAXLABEL];
  if ((r = sscanf (data, ":%s", color)) != 1) {
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, 
	     "__xlib_change_window_background: %s : r = %d data: %s.\n",
	     strerror (errno), r, data);
#endif
    return ERROR;
  }
  if (color) 
    XSetWindowBackground 
      (d, drawable, lookup_pixel (color));
  return SUCCESS;
}

int __xlib_copy_pixmap (Display *d, Drawable dest_drawable, GC dest_gc,
			char *data) {
  Pixmap src_drawable;
  int r;
  unsigned int src_drawable_uint, 
    src_x_org, src_y_org, src_width, src_height, dest_x_org,
    dest_y_org;
  GC copyGC;
  XGCValues gcv, gcv_save;

#ifdef GC_RANGE_CHECK

  if (!gc_check (&dest_gc))
    return ERROR;

#endif

  if ((r = sscanf (data, "%u:%u:%u:%u:%u:%u:%u:", 
		   &src_drawable_uint, 
		   &src_x_org, &src_y_org, &src_width, &src_height,
		   &dest_x_org, &dest_y_org)) != 7) {
#ifndef WITHOUT_X11_WARNINGS
    fprintf (stderr, 
	     "__xlib_copy_pixmap: %s : r = %d data: %s.\n",
	     strerror (errno), r, data);
#endif
    return ERROR;
  }

  src_drawable = (Pixmap)src_drawable_uint;

  gcv.function = GXcopy;
  copyGC = XCreateGC (d, dest_drawable, GCFunction, &gcv);

  r = XCopyArea (d, src_drawable, (Pixmap)dest_drawable, copyGC,
	     (int)src_x_org, (int)src_y_org, 
	     src_width, src_height, 
	     (int)dest_x_org, (int)dest_y_org);

  XFreeGC (d, copyGC);
  return SUCCESS;
}

int __xlib_change_face_request (Display *d, Drawable w, GC gc, char *data) {
  int face_id, r;
  XGCValues v;
  XFontStruct *xfs;
  char buf[0xff];

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc)) {
    return ERROR;
  }

#endif

  errno = 0;
  face_id = strtol (data, NULL, 10);
  if (errno) {
    fprintf (stderr, "ctalk: %s.\n", strerror (errno));
    return ERROR;
  }

  switch (face_id)
    {
    case X_FACE_REGULAR:
      if (xlibfont.normal) {
	xlibfont.selectedfont = xlibfont.normal;
	xlibfont.selected_xfs = xlibfont.normal_xfs;
      }
      break;
    case X_FACE_BOLD:
      if (xlibfont.bold) {
	xlibfont.selectedfont = xlibfont.bold;
	xlibfont.selected_xfs = xlibfont.bold_xfs;
      }
      break;
    case X_FACE_ITALIC:
      if (xlibfont.italic) {
	xlibfont.selectedfont = xlibfont.italic;
	xlibfont.selected_xfs = xlibfont.italic_xfs;
      }
      break;
    case X_FACE_BOLD_ITALIC:
      if (xlibfont.bolditalic) {
	xlibfont.selectedfont = xlibfont.bolditalic;
	xlibfont.selected_xfs = xlibfont.bolditalic_xfs;
      }
      break;
    }

  if (xlibfont.selectedfont && xlibfont.selected_xfs) {
    v.font = xlibfont.selected_xfs -> fid;
    r = XChangeGC (d, gc, GCFont, &v);
    if (r) {
      ctitoa (xlibfont.selected_xfs -> fid, buf);
      strcpy (&shm_mem[SHM_FONT_FID], buf); 
      strcpy (&shm_mem[SHM_FONT_XLFD], xlibfont.selectedfont);
    }
  } else if (xlibfont.selectedfont) {
    xfs = XLoadQueryFont (d, xlibfont.selectedfont);
    v.font = xfs -> fid;
    r = XChangeGC (d, gc, GCFont, &v);
    if (r) {
      ctitoa (xlibfont.selected_xfs -> fid, buf);
      strcpy (&shm_mem[SHM_FONT_FID], buf); 
      strcpy (&shm_mem[SHM_FONT_XLFD], xlibfont.selectedfont);
    }
    XFreeFont (d, xfs);
    if (r) {
      return SUCCESS;
    } else {
      return ERROR;
    }
  } else {
    return ERROR;
  }
}

extern void sync_ft_font (bool);

#ifdef HAVE_XFT_H

extern XftFont *selected_font;
extern XftFont *__select_font_for_family (int p_slant,
					  int p_weight, int p_dpi,
					  double p_size);


int __xlib_change_font_request_ft (Display *d, Drawable w, GC gc, char *data) {
  int face_id, r, slant_def, weight_def;
  int dpi_def;
  double ptsize_def;
  XGCValues v;
  XFontStruct *xfs;
  XftFont *f;
  char buf[0xff], slant_buf[0xff], weight_buf[0xff];

#ifdef GC_RANGE_CHECK

  if (!gc_check (&gc)) {
    return ERROR;
  }

#endif

  if (shm_mem == NULL) {
    if ((shm_mem = get_shmem (mem_id)) == NULL) {
      fprintf (stderr, "ctalk: Couldn't get shared memory: %s.\n",
	       strerror (errno));
      return 0;
    }
  }

  errno = 0;
  face_id = strtol (data, NULL, 10);
  if (errno) {
    fprintf (stderr, "ctalk: %s.\n", strerror (errno));
    return ERROR;
  }
  
  switch (face_id)
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
      if ((f = __select_font_for_family (XFT_SLANT_ITALIC,
					 XFT_WEIGHT_BOLD, 0, 0.0))
	  != NULL) {
	selected_font = f;
	slant_def = XFT_SLANT_ITALIC;
	weight_def = XFT_WEIGHT_BOLD;
	sync_ft_font (false);
      }
      break;
    }

  ptsize_def = strtod (&shm_mem[SHM_FONT_FT_PT_SIZE], NULL);
  dpi_def = (int)shm_mem[SHM_FONT_FT_DPI];
  if ((f = __select_font_for_family (slant_def, weight_def,
				     dpi_def, ptsize_def)) != NULL) {
    selected_font = f;
    sync_ft_font (false);
  }
  
  return SUCCESS;
}
#endif /* HAVE_XFT_H */
 
int __xlib_handle_client_request (char *shm_mem_2) {
  unsigned int w;
  int r;
  GC gc;
  Display *d;
  
  if (! *shm_mem_2) goto x_event_cleanup;

  set_client_sighandlers ();

  if (sigsetjmp (env, true))
    goto x_event_cleanup;

  w = (shm_mem_2[SHM_DRAWABLE] << 28) +
    (shm_mem_2[SHM_DRAWABLE+1] << 24) +
    (shm_mem_2[SHM_DRAWABLE+2] << 20) +
    (shm_mem_2[SHM_DRAWABLE+3] << 16) +
    (shm_mem_2[SHM_DRAWABLE+4] << 12) +
    (shm_mem_2[SHM_DRAWABLE+5] << 8) +
    (shm_mem_2[SHM_DRAWABLE+6] << 4) +
    shm_mem_2[SHM_DRAWABLE+7];
  
#ifdef __x86_64
  /* the uintptr_t casts indicate that we want this type width */
  d = (Display *)((((uintptr_t)shm_mem_2[SHM_DISPLAY]) << 44) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+1]) << 40) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+2]) << 36) + 
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+3]) << 32) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+4]) << 28) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+5]) << 24) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+6]) << 20) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+7]) << 16) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+8]) << 12) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+9]) << 8) +
	    (((uintptr_t)shm_mem_2[SHM_DISPLAY+10]) << 4) +
	    shm_mem_2[SHM_DISPLAY+11]);
  gc = (GC)((((uintptr_t)shm_mem_2[SHM_GC]) << 44) +
	    (((uintptr_t)shm_mem_2[SHM_GC+1]) << 40) +
	    (((uintptr_t)shm_mem_2[SHM_GC+2]) << 36) + 
	    (((uintptr_t)shm_mem_2[SHM_GC+3]) << 32) +
	    (((uintptr_t)shm_mem_2[SHM_GC+4]) << 28) +
	    (((uintptr_t)shm_mem_2[SHM_GC+5]) << 24) +
	    (((uintptr_t)shm_mem_2[SHM_GC+6]) << 20) +
	    (((uintptr_t)shm_mem_2[SHM_GC+7]) << 16) +
	    (((uintptr_t)shm_mem_2[SHM_GC+8]) << 12) +
	    (((uintptr_t)shm_mem_2[SHM_GC+9]) << 8) +
	    (((uintptr_t)shm_mem_2[SHM_GC+10]) << 4) +
	    shm_mem_2[SHM_GC+11]);

#else  
  d = (Display *)((shm_mem_2[SHM_DISPLAY] << 28) +
		  (shm_mem_2[SHM_DISPLAY + 1] << 24) +
		  (shm_mem_2[SHM_DISPLAY + 2] << 20) +
		  (shm_mem_2[SHM_DISPLAY + 3] << 16) +
		  (shm_mem_2[SHM_DISPLAY + 4] << 12) +
		  (shm_mem_2[SHM_DISPLAY + 5] << 8) +
		  (shm_mem_2[SHM_DISPLAY + 6] << 4) +
		  shm_mem_2[SHM_DISPLAY + 7]);
  gc = (GC)((shm_mem_2[SHM_GC] << 28) +
	   (shm_mem_2[SHM_GC+1] << 24) +
	   (shm_mem_2[SHM_GC+2] << 20) +
	   (shm_mem_2[SHM_GC+3] << 16) +
	   (shm_mem_2[SHM_GC+4] << 12) +
	   (shm_mem_2[SHM_GC+5] << 8) +
	   (shm_mem_2[SHM_GC+6] << 4) +
	   shm_mem_2[SHM_GC+7]);
#endif
  
  switch (shm_mem_2[SHM_REQ]) 
    {
    case PANE_PUT_STR_REQUEST:
      __xlib_put_str (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
#ifdef HAVE_XFT_H
    case PANE_PUT_STR_REQUEST_FT:
      __xlib_put_str_ft (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
#endif
    case PANE_WM_TITLE_REQUEST:
      __xlib_set_wm_name_prop (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_DRAW_POINT_REQUEST:
      __xlib_draw_point (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_DRAW_LINE_REQUEST:
      __xlib_draw_line (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_DRAW_RECTANGLE_REQUEST:
      __xlib_draw_rectangle (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_CLEAR_WINDOW_REQUEST:
      __xlib_clear_window (d, (Drawable)w, gc);
      break;
    case PANE_CLEAR_RECTANGLE_REQUEST:
      __xlib_clear_rectangle (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_RESIZE_REQUEST:
      __xlib_resize_window (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      strcpy (&shm_mem_2[SHM_RETVAL], &shm_mem_2[SHM_DATA]);
      break;
    case PANE_MOVE_REQUEST:
      __xlib_move_window (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_REFRESH_REQUEST:
      __xlib_refresh_window (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_CURSOR_REQUEST:
      __xlib_use_cursor (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_CHANGE_GC_REQUEST:
      __xlib_change_gc (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_SET_WINDOW_BACKGROUND_REQUEST:
      __xlib_change_window_background 
	(d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_RESIZE_PIXMAP_REQUEST:
      __xlib_resize_pixmap (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      strcpy (&shm_mem_2[SHM_RETVAL], &shm_mem_2[SHM_DATA]);
      break;
    case PANE_XPM_FROM_DATA_REQUEST:
      __xlib_xpm_from_data (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_SET_RESOURCE_REQUEST:
      __xlib_set_resources (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_COPY_PIXMAP_REQUEST:
      __xlib_copy_pixmap (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
	break;
    case PANE_DRAW_CIRCLE_REQUEST:
      __xlib_draw_circle (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_XLIB_FACE_REQUEST:
      __xlib_change_font_request_ft (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
#ifdef HAVE_XFT_H
    case PANE_XLIB_FACE_REQUEST_FT:
      __xlib_change_font_request_ft (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
#endif
    case PANE_TEXT_FROM_DATA_REQUEST:
      __xlib_render_text (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_GET_PRIMARY_SELECTION_REQUEST:
      __xlib_get_primary_selection (d, (Drawable)w, gc, &shm_mem_2[SHM_DATA]);
      break;
    case PANE_SET_PRIMARY_SELECTION_OWNERSHIP_REQUEST:
      r = __xlib_set_primary_selection_owner (d, (Drawable)w);
      shm_mem_2[SHM_RETVAL] = ((r == SUCCESS) ? '0' : '1');
      break;
    case PANE_SET_PRIMARY_SELECTION_TEXT:
      __xlib_set_primary_selection_text (&shm_mem_2[SHM_DATA]);
      break;
    } 
 x_event_cleanup:
  unset_client_sighandlers ();
  shm_mem_2[SHM_REQ] = '\0';
  /* detach_shmem (mem_handle, shm_mem_2); */
  return SUCCESS;
}

static char *__x11_pane_socket_path (int pid) {
  static char path[FILENAME_MAX];
  char intbuf[MAXLABEL];
  strcatx (path, P_tmpdir, "/ctalk.x.", ctitoa (pid, intbuf), NULL);
  return path;
}

static int __x11_pane_stream_socket_open (void) {
  int fd;
  struct sockaddr_un addr;
  char path[FILENAME_MAX];
  if ((fd = socket (AF_UNIX, SOCK_DGRAM, 0)) == -1) {
    fprintf (stderr, "__x11_stream_socket: %s.\n", 
	     strerror (errno));
    return ERROR;
  }
  strcpy (path, __x11_pane_socket_path ((int)getpid ()));
  strncpy (addr.sun_path, path, sizeof (addr.sun_path) - 1);
  addr.sun_family = AF_UNIX;
  if (bind (fd, (struct sockaddr *) &addr, 
	    sizeof (struct sockaddr_un))) {
    fprintf (stderr, "__x11_stream_bind: %s. Exiting.\n", 
	     strerror (errno));
    exit (EXIT_FAILURE);
  }
  return fd;
}

static int __x11_pane_stream_socket_ready (int pid) {
  char path[FILENAME_MAX];
  struct stat statbuf;
  int r;
  strcpy (path, __x11_pane_socket_path (pid));
  /*
   *  If not open, wait half a sec and check again.
   */
  if ((r = stat (path, &statbuf)) == ERROR) {
    usleep (500000L);
    if ((r = stat (path, &statbuf)) == ERROR) {
      return ERROR;
    }
  } else {
    return SUCCESS;
  }
  return ERROR;
}

/* 
 *  Apple needs the server and client sockets to
 *  be different, regardless of whether they're in
 *  different processes.  Linux doesn't need that,
 *  so we just use the parent's socket fd.
 */
static int __x11_pane_socket_connect (int parent_sock_fd) {
  int r, client_sock_fd, sock_return = 0;
  struct sockaddr_un addr;
  strcpy (addr.sun_path, __x11_pane_socket_path ((int)getppid ()));
  addr.sun_family = AF_UNIX;
#ifdef __APPLE__
  addr.sun_len = strlen (addr.sun_path) + 1;
  client_sock_fd = socket (AF_UNIX, SOCK_DGRAM, 0);
  if ((r = connect (client_sock_fd, 
	       (struct sockaddr *)&addr, 
		    sizeof (struct sockaddr_un))) == ERROR) {
    fprintf (stderr, "__x11_pane_socket_connect: %s: %s\n", 
	     addr.sun_path, strerror (errno));
    sock_return = -1;
  } else {
    sock_return = client_sock_fd;
  }
#else
  if ((r = connect (parent_sock_fd, 
		    (struct sockaddr *)&addr,
		    sizeof (addr.sun_family) +
		    strlen (addr.sun_path))) == ERROR) {
    fprintf (stderr, "__x11_pane_socket_connect: %s: %s\n", 
	     addr.sun_path, strerror (errno));
    sock_return = -1;
  } else {
    sock_return = parent_sock_fd;
  }
#endif    
  return sock_return;
}

static Display *__x11_open_display (void) {
  char *d_env = NULL;

  if (display != NULL) 
    return display;

  if ((d_env = getenv ("DISPLAY")) == NULL) {
    printf ("This program requires the X Window System. Exiting.\n");
    exit (1);
  }
  if ((display = XOpenDisplay (d_env)) != NULL) {
    default_io_handler = XSetIOErrorHandler (__ctalkX11IOErrorHandler);
    default_error_handler = XSetErrorHandler (__ctalkX11ErrorHandler);
    /* 
     *  See the comments above.
     */
    setsid ();
    x11_sig_init ();
    screen = DefaultScreen (display);
    root = RootWindow (display, screen);
    screen_depth = DefaultDepth (display, screen);
    display_width = DisplayWidth (display, screen);
    display_height = DisplayHeight (display, screen);
    fixed_font_set = XListFonts (display, FIXED_FONT_XLFD, 10,
 				 &n_fixed_fonts);
    if (n_fixed_fonts) {
      fixed_font = XLoadFont (display, fixed_font_set[0]);
      XFreeFontNames (fixed_font_set);
    } else {
#ifndef WITHOUT_X11LIB_WARNINGS
      _warning ("__x11_open_display: Couldn't find fixed font.\n");
      _warning ("__x11_open_display: (To disable these messages, build Ctalk with\n");
      _warning ("__x11_open_display: the --without-x11-warnings option.)\n");
#endif
    }
    wm_xfce = is_wm_xfce (display);
    wm_kwin = is_kwin_desktop (display);
    wm_xquartz = is_wm_xquartz ();

    return display;
  }
  return NULL;
}

void *__ctalkX11Display (void) { 
#if !X11LIB_FRAME
  if (display)
    return (void *)display; 
  else
    return (void *)__x11_open_display ();
#else
 return NULL;
#endif
}

/*
 *  We need to do this here because the X headers 
 *  define the DefaultColormap sub-macros out of 
 *  order.  Arrgh.
 */
int __ctalkX11Colormap (void) {
#if !X11LIB_FRAME
  if (!display)
    __x11_open_display ();
  return DefaultColormap 
    (display, DefaultScreen (display));
#else
  return 0;
#endif
}

static void release_fixed_font (void) {
  if (n_fixed_fonts) { 
    /*
     *  This should not be needed on most displays.
     */
/*     XUnloadFont (display, fixed_font); */
/*     XFreeFontNames (fixed_font_set); */
    fixed_font = 0;
    n_fixed_fonts = 0;
  }
}

Font get_user_font (OBJECT *self) {
  OBJECT *self_object, *fontDesc_var, *fontDesc_value_var;
  Font user_font;
  int n_user_fonts;
  char **user_font_set;
  Display *l_d;
  OBJECT *displayPtr_instvar;
  self_object = IS_VALUE_INSTANCE_VAR(self) ? self -> __o_p_obj :
    self;
  if ((fontDesc_var = 
       __ctalkGetInstanceVariable (self_object, "fontDescStr", TRUE)) 
      == NULL)
    return 0;
  if ((fontDesc_value_var = fontDesc_var -> instancevars) == NULL)
    return 0;
  if (!strcmp (fontDesc_value_var -> __o_value, NULLSTR))
    return 0;

  if ((displayPtr_instvar = __ctalkGetInstanceVariable (self_object,
							"displayPtr",
							TRUE)) == NULL)
    return ERROR;

  l_d = (Display *)SYMVAL(displayPtr_instvar -> instancevars -> __o_value);
  user_font_set = XListFonts (l_d, fontDesc_value_var->__o_value, 10,
			      &n_user_fonts);

  if (n_user_fonts) {
    user_font = XLoadFont (l_d, user_font_set[0]);
    XFreeFontNames (user_font_set);
    return user_font;
    } else {
#ifndef WITHOUT_X11_WARNINGS
    _warning ("__x11_open_display: Couldn't find font %s.\n",
	      fontDesc_value_var->__o_value);
    _warning ("__x11_open_display: (To disable these messages, build Ctalk with\n");
    _warning ("__x11_open_display: the --without-x11-warnings option.)\n");
#endif
    }
  return 0;
}

#if 0
/* Not used currently */
static void release_pane_gc (OBJECT *self) {
  OBJECT *gc_value_var;
  GC gc;
  gc_value_var = __x11_pane_win_gc_value_object (self);
  gc = (GC)generic_ptr_str (gc_value_var->__o_value);
  XFreeGC (display, gc);
  gc_value_var->__o_value[0] = '\0'; 
}
#endif

#if 0
/* Also not used currently */
static void destroy_pane_window (OBJECT * self) {
  OBJECT *win_id_value_var;
  Window w;
  win_id_value_var = __x11_pane_win_id_value_object (self);
  w = atoi (win_id_value_var -> __o_value);
  XDestroyWindow (display, w);
}
#endif

#if X11LIB_FRAME

int __ctalkX11InputClient (OBJECT *streamobject, int parent_fd, int mem_handle, int main_win_id) {
  XEvent e;
  int events_waiting, handle_count, client_sock_fd;
  char *s;

  if ((client_sock_fd = __x11_pane_socket_connect (parent_fd)) == ERROR) {
    fprintf (stderr, "ctalk: Couldn't open X11 client socket.  Exiting.\n");
    _exit (1);
  }

  while (1) {
    while ((events_waiting = XPending (display)) > 0) {
      XNextEvent (display, &e);
    }
    if ((s = (char *)get_shmem (mem_handle)) == NULL)
      return ERROR;
    if (client_shutdown_requested (s)) {
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "Process %d releasing shared memory.\n",
	       (int) getpid ());
#endif
      handle_count = detach_shmem (mem_id, s);

#ifdef HAVE_XFT_H
      if (ft_str.draw != NULL)
	XftDestroy (ft_str.draw);
#endif      
      
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "Shared memory attached to %d processes.\n",
	       handle_count);
#endif
      return SUCCESS;
    } else {
      handle_count = detach_shmem (mem_id, s);
    }
  }
  return SUCCESS;
}
#endif /* X11LIB_FRAME */

#if !X11LIB_FRAME
static OBJECT *__stream_queue_object (OBJECT *streamobject) {
  OBJECT *p_obj;
  static OBJECT *queue_object;
  if (!strcmp (streamobject -> __o_name, "value") &&
      streamobject -> __o_p_obj) {
    p_obj = streamobject -> __o_p_obj;
  } else {
    p_obj = streamobject;
  }
  if ((queue_object = 
       __ctalkGetInstanceVariable (p_obj, "inputQueue", FALSE))
      == NULL)
    return NULL;
  return queue_object;
}
#endif

#if !X11LIB_FRAME
static OBJECT *__x11_stream_pid_value_object (OBJECT *streamobject) {
  static OBJECT *pid_object,
    *pid_value_object;
  OBJECT *p_obj;
  if (!strcmp (streamobject -> __o_name, "value") &&
      streamobject -> __o_p_obj) {
    p_obj = streamobject -> __o_p_obj;
  } else {
    p_obj = streamobject;
  }
  if ((pid_object = 
       __ctalkGetInstanceVariable (p_obj, "inputPID", FALSE))
      == NULL)
    return NULL;
  if ((pid_value_object = pid_object -> instancevars) == NULL)
    return NULL;
  return pid_value_object;
}

static OBJECT *__x11_stream_client_sock_fd_value_object (OBJECT *streamobject) {
  static OBJECT *sock_fd_object,
    *client_sock_fd_value_object;
  OBJECT *p_obj;

  if (!IS_OBJECT(streamobject))
    return NULL;
  if (IS_VALUE_INSTANCE_VAR(streamobject)) {
    p_obj = streamobject -> __o_p_obj;
  } else {
    p_obj = streamobject;
  }

  if ((sock_fd_object = 
       __ctalkGetInstanceVariable (p_obj, "clientFD", FALSE))
      == NULL)
    return NULL;
  if ((client_sock_fd_value_object = sock_fd_object -> instancevars)
      == NULL)
    return NULL;
  return client_sock_fd_value_object;
}
#endif

#define D_EVENT 0
#define D_1 64
#define D_2 128
#define D_3 192
#define D_4 256
#define D_5 320

static OBJECT *get_event_instancevar (OBJECT *obj, char *varname) {
  OBJECT *o;
  for (o = obj -> instancevars; o; o = o -> next)
    if (str_eq (o -> __o_name, varname))
      return o;
  return NULL;
}

int __ctalkX11MakeEvent (OBJECT *eventobject_value_var, OBJECT *inputqueue) {
  fd_set rfds;
  struct timeval tv;
  char nbuf[MAXMSG];
  char valbuf[MAXLABEL];
  int r, n_read;
  OBJECT *ev_parent;
  OBJECT *ev_instancevar;
  OBJECT *inputqueue_head;
  OBJECT *key_object;
  int i;

  ev_parent = eventobject_value_var -> __o_p_obj;

  FD_ZERO(&rfds);
  FD_SET(frame_pane_sock_fd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 500;

 read_again:
  if ((r = select (frame_pane_sock_fd + 1, &rfds, NULL, NULL, &tv)) > 0) {
    if ((n_read = read (frame_pane_sock_fd, (void *)nbuf, MAXMSG)) > 0) {
      if (n_read < 320)
	return -1;

      nbuf[n_read] = '\0';

      ev_instancevar = get_event_instancevar (ev_parent, "eventClass");
      __ctalkSetObjectValueVar (ev_instancevar, nbuf + D_EVENT);

      ev_instancevar = get_event_instancevar (ev_parent, "xEventData1");
      __ctalkSetObjectValueVar (ev_instancevar, nbuf + D_1);
      
      ev_instancevar = get_event_instancevar (ev_parent, "xEventData2");
      __ctalkSetObjectValueVar (ev_instancevar, nbuf + D_2);

      ev_instancevar = get_event_instancevar (ev_parent, "xEventData3");
      __ctalkSetObjectValueVar (ev_instancevar, nbuf + D_3);

      ev_instancevar = get_event_instancevar (ev_parent, "xEventData4");
      __ctalkSetObjectValueVar (ev_instancevar, nbuf + D_4);

      ev_instancevar = get_event_instancevar (ev_parent, "xEventData5");
      __ctalkSetObjectValueVar (ev_instancevar, nbuf + D_5);

      inputqueue_head = inputqueue -> instancevars;
      if (inputqueue_head -> next != NULL)
	for (; inputqueue_head -> next; 
	     inputqueue_head = inputqueue_head -> next)
	  ;

      key_object = __ctalkCreateObjectInit ("keyObject", "Key", "Symbol",
					    inputqueue_head -> scope,
					    NULLSTR);
      __objRefCntSet (OBJREF(key_object), inputqueue_head -> nrefs);
      __ctalkSetObjectAttr (key_object, OBJECT_IS_MEMBER_OF_PARENT_COLLECTION);
#ifdef __x86_64
      htoa (valbuf, (unsigned long long int)eventobject_value_var->__o_p_obj);
#else      
      htoa (valbuf, (unsigned int)eventobject_value_var->__o_p_obj);
#endif      
      __ctalkSetObjectValueVar (key_object, valbuf);
      inputqueue_head -> next = key_object;
      key_object -> prev = inputqueue_head;

    }
  }
  
  return 0;
}

static int child_pid = -1;

int __client_pid (void) {
  return child_pid;
}

int __ctalkOpenX11InputClient (OBJECT *streamobject) {
#if !X11LIB_FRAME
  OBJECT *pid_value_object, *client_sock_fd_value_object;
  OBJECT *x11_pane_stream_object, *x11_pane_stream_queue_list;
  int main_win_id;
  OBJECT *pane_win_id_value_object;
#endif

  if ((display = __x11_open_display ()) == NULL)
    return -1;

  if ((mem_id = mem_setup ()) == ERROR) 
    return ERROR;

  if (shm_mem == NULL) {
    if ((shm_mem = get_shmem (mem_id)) == NULL) {
      fprintf (stderr, "ctalk: Couldn't get shared memory: %s.\n",
	       strerror (errno));
      return 0;
    }
  }
  INTVAL(&shm_mem[SHM_EVENT_READY]) = 0;
  INTVAL(&shm_mem[SHM_EVENT_MASK]) = 0;
  INTVAL(&shm_mem[SHM_LOCK_XID]) = 0;
  
#if !X11LIB_FRAME
  x11_pane_stream_object = streamobject;
  if ((x11_pane_stream_queue_list = __stream_queue_object (streamobject))
      == NULL) {
    x11_pane_stream_object = NULL;
    release_fixed_font ();
    /*
     *  Delete GC and window here also, *if any*.
     */
    XCloseDisplay (display);
    display = NULL;
    return -3;
  }
#endif

  pane_win_id_value_object = 
    __x11_pane_win_id_value_object (streamobject->__o_p_obj);
  main_win_id = *(int *)pane_win_id_value_object -> __o_value;

#ifdef HAVE_XRENDER_H
  xrender_version_check ();
#endif

  /*
   *  Handle this error in the method.
   */
  if ((frame_pane_sock_fd = __x11_pane_stream_socket_open ()) == ERROR)
    return ERROR;

  switch (child_pid = fork ())
    {
    case 0:
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "Process %d started.  Parent PID %d.\n",
	       (int)getpid(), (int)getppid ());
#endif
      XFlush (display);
      __ctalkX11InputClient (streamobject, frame_pane_sock_fd, mem_id, 
			     main_win_id);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "Process %d exit.\n", (int)getpid ());
#endif
      _exit (0);
      break;
    case -1:
      fprintf (stderr, "__ctalkOpenX11InputClient: %s\n", 
	       strerror (errno));
      _exit (1);
      break;
    default:
      if (__x11_pane_stream_socket_ready (getpid ())) {
	fprintf (stderr, "__ctalkOpenX11InputClient: socket not ready.\n");
	return ERROR;
      }
      if ((shm_mem = (char *)get_shmem (mem_id)) == NULL)
	_exit (0);

#if !X11LIB_FRAME
      if ((pid_value_object = __x11_stream_pid_value_object (streamobject))
	  != NULL) {
	*(int *)pid_value_object -> __o_value = child_pid;
      }
      if ((client_sock_fd_value_object = 
	   __x11_stream_client_sock_fd_value_object (streamobject)) != NULL) {
	*(int *)client_sock_fd_value_object -> __o_value =
	  frame_pane_sock_fd;
	if (IS_OBJECT(client_sock_fd_value_object -> __o_p_obj))
	*(int *)client_sock_fd_value_object -> __o_p_obj -> __o_value =
	  frame_pane_sock_fd;
	  
      }
#endif
      break;
    }
  return SUCCESS;
}

/*
 *  We have to be careful here and in __ctalkX11CloseParentPane 
 *  and __ctalkX11CloseClient that we don't destroy objects 
 *  asynchronously.
 */
int __ctalkCloseX11Pane (OBJECT *self) {
  OBJECT *gc_value_var;
  GC gc;
  OBJECT *win_id_value_var;
  Window w;

  gc_value_var = __x11_pane_win_gc_value_object (self);

  if ((gc = (GC)generic_ptr_str (gc_value_var->__o_value)) != NULL) {
    XFreeGC (display, gc);
    gc_value_var->__o_value[0] = '\0'; 
  }

  win_id_value_var = __x11_pane_win_id_value_object (self);
  if ((w = *(int *)win_id_value_var -> __o_value) != (Window)0)
    XDestroyWindow (display, w);

  if (__have_bitmap_buffers (self)) {
    __ctalkX11FreePaneBuffer (self);
  }  

  return SUCCESS;
}

int __ctalkX11CloseParentPane (OBJECT *self) {
  OBJECT *pane_win_id_value_object;
  int main_win_id;
  pane_win_id_value_object = 
    __x11_pane_win_id_value_object (self);
  if ((main_win_id = *(int *)pane_win_id_value_object -> __o_value) == 0)
    return ERROR;
    
  release_fixed_font ();
  request_client_shutdown ();
  display = NULL;
  return SUCCESS;
}

int __ctalkX11CloseClient (OBJECT *self) {
  int waitstatus;
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
  fprintf (stderr, "__ctalkX11CloseClient: Waiting for client shutdown....\n");
#endif
  request_client_shutdown ();
  wait (&waitstatus);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
  fprintf (stderr, "...waited.\n");
#endif
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "App releasing shared memory.\n");
#endif
      release_shmem (mem_id, shm_mem);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "Shared memory released.\n");
#endif
  delete_shmem_info (mem_id);
#if !X11LIB_FRAME
  if (shutdown (frame_pane_sock_fd, SHUT_RD))
    fprintf (stderr, "__ctalkX11CloseClient: shutdown: %s.\n",
	     strerror (errno));
  if (unlink (__x11_pane_socket_path ((int)getpid())))
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
    fprintf (stderr, "__ctalkX11CloseClient: unlink: %s : %s.\n",
	     __x11_pane_socket_path ((int)getpid()), strerror (errno));
#endif
#endif

  return SUCCESS;
}


#if X11LIB_FRAME
int __ctalkCreateX11MainWindow (OBJECT *self_object) {
   return SUCCESS;
}
int __ctalkCreateX11SubWindow (OBJECT *parent, OBJECT *self) {
  return SUCCESS;
}
int __ctalkMapX11Window (OBJECT *self_object) {
  return SUCCESS;
}
int __ctalkRaiseX11Window (OBJECT *self_object) {
  return SUCCESS; 
}
int __ctalkX11SetWMNameProp (OBJECT *self_object, char *title) {
  return SUCCESS;
}
int __ctalkX11UseFontBasic (void * d, int drawable_id, unsigned long int gc_ptr,
			    char *xlfd) {
  return SUCCESS;
}
int __ctalkX11ResizeWindow (OBJECT *self, int width, int height) {
  return SUCCESS;
}
int __ctalkX11ResizePixmap (int parent_visual, int pixmap,
			    unsigned long int gc,
			    int old_width, int old_height,
			    int new_width, int new_height,
			    int depth) {
  return SUCCESS;
}
int __ctalkX11MoveWindow (OBJECT *self, int x, int y) {
  return SUCCESS;
}

int __ctalkX11FontCursor (OBJECT *self, int cursor_id) {
  return SUCCESS;
}

int __ctalkX11UseCursor (OBJECT *pane_object, OBJECT *cursor_object) {
  return SUCCESS;
}

int __ctalkX11DisplayHeight (void) { return SUCCESS; }
int __ctalkX11DisplayWidth (void) { return SUCCESS; }

#endif /* X11LIB_FRAME */

#if !X11LIB_FRAME
/*
 *  Only tests paneBuffer variable.  Might need to test
 *  paneBackingStore variable also if the buffers are not set 
 *  as a unit.
 */
int __have_bitmap_buffers (OBJECT *self_object) {
  OBJECT *paneBuffer_var, *paneBuffer_xID_var;
  if ((paneBuffer_var =
       __ctalkGetInstanceVariable 
       ((IS_VALUE_INSTANCE_VAR(self_object) ? 
	 self_object -> __o_p_obj : self_object), 
	"paneBuffer", TRUE))
      == NULL)
    return ERROR;
  if ((!strcmp (paneBuffer_var->instancevars->CLASSNAME, "Bitmap")) ||
      (__ctalkIsSubClassOf (paneBuffer_var->instancevars->CLASSNAME,
			    "Bitmap") > 0)) {
    if ((paneBuffer_xID_var = 
	 __ctalkGetInstanceVariable (paneBuffer_var,
				     "xID", FALSE)) != NULL) {
      if (strcmp (paneBuffer_xID_var->instancevars->__o_value, "0") &&
	  strcmp (paneBuffer_xID_var->instancevars->__o_value, NULLSTR))
	return TRUE;
    } else {
      if (strcmp (paneBuffer_var->instancevars->__o_value, "0") &&
	  strcmp (paneBuffer_var->instancevars->__o_value, NULLSTR))
	return TRUE;
    }
  }
  return FALSE;
}

static bool have_event_mask = false;

int read_event (int *ev_type_out, unsigned int *win_out,
		unsigned int data[], int event_mask) {

  if (!INTVAL(&shm_mem[SHM_EVENT_READY]))
    return -1;

  if (event_mask == 0) {
    *ev_type_out = INTVAL(&shm_mem[SHM_EVENT_TYPE]);
    *win_out = UINTVAL(&shm_mem[SHM_EVENT_WIN]);
    data[0] = UINTVAL(&shm_mem[SHM_EVENT_DATA1]);
    data[1] = UINTVAL(&shm_mem[SHM_EVENT_DATA2]);
    data[2] = UINTVAL(&shm_mem[SHM_EVENT_DATA3]);
    data[3] = UINTVAL(&shm_mem[SHM_EVENT_DATA4]);
    data[4] = UINTVAL(&shm_mem[SHM_EVENT_DATA5]);
    data[5] = UINTVAL(&shm_mem[SHM_EVENT_DATA6]);
    INTVAL(&shm_mem[SHM_EVENT_READY]) = 0;
  } else {
    if (!have_event_mask) {
      /* should not reach this if event_mask is zero */
      INTVAL(&shm_mem[SHM_EVENT_MASK]) = event_mask;
      have_event_mask = true;
    }
    *ev_type_out = INTVAL(&shm_mem[SHM_EVENT_TYPE]);
    if (*ev_type_out & event_mask) {
      *win_out = UINTVAL(&shm_mem[SHM_EVENT_WIN]);
      data[0] = UINTVAL(&shm_mem[SHM_EVENT_DATA1]);
      data[1] = UINTVAL(&shm_mem[SHM_EVENT_DATA2]);
      data[2] = UINTVAL(&shm_mem[SHM_EVENT_DATA3]);
      data[3] = UINTVAL(&shm_mem[SHM_EVENT_DATA4]);
      data[4] = UINTVAL(&shm_mem[SHM_EVENT_DATA5]);
      data[5] = UINTVAL(&shm_mem[SHM_EVENT_DATA6]);
      INTVAL(&shm_mem[SHM_EVENT_READY]) = 0;
    } else {
      INTVAL(&shm_mem[SHM_EVENT_READY]) = 0;
    }
  }
  return SUCCESS;
}

#define EV_REUSE_MAX 0xffff
/* uncomment to print the EV_CH cache levels on a terminal */
/* #define SHOW_EV_CACHE_USE */

typedef struct ev_ch {
  struct ev_ch *next;
  int event_class,
    eventdata1,
    eventdata2,
    eventdata3,
    eventdata4,
    eventdata5,
    eventdata6;
  unsigned int win_id;
} EV_CH;

EV_CH *ev_buf = NULL, *ev_buf_ptr = NULL;

EV_CH *ev_reuse_s[EV_REUSE_MAX];
int ev_reuse_ptr = 0; 
int ev_cnt = 0;
 
static EV_CH *stashed_ev (void) {
  EV_CH *e;
  if (ev_reuse_ptr > 0) {
    e = ev_reuse_s[--ev_reuse_ptr];
    ev_reuse_s[ev_reuse_ptr] = NULL;
  } else {
    e = __xalloc (sizeof (EV_CH));
  }
  return e;
}

static void stash_ev (EV_CH *e) {
  if (ev_reuse_ptr >= EV_REUSE_MAX) {
    __xfree (MEMADDR(e));
  } else {
    ev_reuse_s[ev_reuse_ptr++] = e;
  }
}

static EV_CH *ev_unshift (void) {
  EV_CH *e;

  if (ev_buf == NULL)
    return NULL;

  e = ev_buf;
  if (ev_buf -> next == NULL) {
    ev_buf = ev_buf_ptr = NULL;
  } else {
    ev_buf = ev_buf -> next;
  }
  --ev_cnt;
  return e;
}

static void ev_push (int eventclass, unsigned int win_id,
		     int eventdata1, int eventdata2, int eventdata3,
		     int eventdata4, int eventdata5, int eventdata6) {
  EV_CH *e;
  if (ev_buf == NULL) {
    e = ev_buf = ev_buf_ptr = stashed_ev ();
    e -> next = NULL;
  } else {
    e = stashed_ev ();
    ev_buf_ptr -> next = e;
    ev_buf_ptr = e;
    e -> next = NULL;
  }
  e -> event_class = eventclass;
  e -> win_id = win_id;
  e -> eventdata1 = eventdata1; e -> eventdata2 = eventdata2;
  e -> eventdata3 = eventdata3; e -> eventdata4 = eventdata4;
  e -> eventdata5 = eventdata5; e -> eventdata6 = eventdata6;
  ++ev_cnt;
}

static void event_to_client (int eventclass,
			     unsigned int win_id,
			     int eventdata1,
			     int eventdata2, int eventdata3,
			     int eventdata4, int eventdata5,
			     int eventdata6) {
  int wait_retries = 0;

  if (eventclass == MOTIONNOTIFY) {
    /* Use the version of event_to_client that discards events. */

    while (INTVAL(&shm_mem[SHM_EVENT_READY])) {
      if (++wait_retries > 20)
	return;
      usleep (100);
    }

    INTVAL(&shm_mem[SHM_EVENT_TYPE]) = eventclass;
    UINTVAL(&shm_mem[SHM_EVENT_WIN]) = win_id;
    INTVAL(&shm_mem[SHM_EVENT_DATA1]) = eventdata1;
    INTVAL(&shm_mem[SHM_EVENT_DATA2]) = eventdata2;
    INTVAL(&shm_mem[SHM_EVENT_DATA3]) = eventdata3;
    INTVAL(&shm_mem[SHM_EVENT_DATA4]) = eventdata4;
    INTVAL(&shm_mem[SHM_EVENT_DATA5]) = eventdata5;
    INTVAL(&shm_mem[SHM_EVENT_DATA6]) = eventdata6;

    INTVAL(&shm_mem[SHM_EVENT_READY]) = TRUE;
    return;
  } else if (eventclass == BUTTONRELEASE || eventclass == BUTTONPRESS) {
    INTVAL(&shm_mem[SHM_EVENT_TYPE]) = eventclass;
    UINTVAL(&shm_mem[SHM_EVENT_WIN]) = win_id;
    INTVAL(&shm_mem[SHM_EVENT_DATA1]) = eventdata1;
    INTVAL(&shm_mem[SHM_EVENT_DATA2]) = eventdata2;
    INTVAL(&shm_mem[SHM_EVENT_DATA3]) = eventdata3;
    INTVAL(&shm_mem[SHM_EVENT_DATA4]) = eventdata4;
    INTVAL(&shm_mem[SHM_EVENT_DATA5]) = eventdata5;
    INTVAL(&shm_mem[SHM_EVENT_DATA6]) = eventdata6;

    INTVAL(&shm_mem[SHM_EVENT_READY]) = TRUE;
    return;
  }

  if (INTVAL(&shm_mem[SHM_EVENT_MASK]) > 0) {
    if (!(INTVAL(&shm_mem[SHM_EVENT_MASK]) & eventclass)) {
      return;
    }
  }

#ifdef SHOW_EV_CACHE_USE
  fprintf (stderr, "%d %d\n", ev_cnt, ev_reuse_ptr);
#endif  

  if (INTVAL(&shm_mem[SHM_EVENT_READY])) {
    ev_push (eventclass, win_id, eventdata1, eventdata2,
	       eventdata3, eventdata4, eventdata5, eventdata6);
    return;
  }

  INTVAL(&shm_mem[SHM_EVENT_TYPE]) = eventclass;
  UINTVAL(&shm_mem[SHM_EVENT_WIN]) = win_id;
  INTVAL(&shm_mem[SHM_EVENT_DATA1]) = eventdata1;
  INTVAL(&shm_mem[SHM_EVENT_DATA2]) = eventdata2;
  INTVAL(&shm_mem[SHM_EVENT_DATA3]) = eventdata3;
  INTVAL(&shm_mem[SHM_EVENT_DATA4]) = eventdata4;
  INTVAL(&shm_mem[SHM_EVENT_DATA5]) = eventdata5;
  INTVAL(&shm_mem[SHM_EVENT_DATA6]) = eventdata6;

  INTVAL(&shm_mem[SHM_EVENT_READY]) = TRUE;

}

 
static void generate_expose (Window w, int x, int y, int width, int height) {
  XEvent e;
  e.type = Expose;
  e.xexpose.serial = 0;
  e.xexpose.send_event = True;
  e.xexpose.display = display;
  e.xexpose.window = e.xconfigure.window;
  e.xexpose.x = e.xconfigure.x;
  e.xexpose.y = e.xconfigure.y;
  e.xexpose.width = e.xconfigure.width;
  e.xexpose.height = e.xconfigure.height;
  e.xexpose.count = 0;
  XSendEvent (display, e.xconfigure.window, True,
	      ExposureMask,  &e);
}

static void resize_event_to_client (int x, int y,
				     int width, int height,
				     int border_width,
				     XRectangle *prev,
				     int win_id,
				     int *eventclass) {
  event_to_client (RESIZENOTIFY, win_id,
		   x, y, width, height, border_width, 0);
  prev -> width = width;
  prev -> height = height;
  *eventclass = 0;
}

static void copy_xconfig_event (XEvent *e1, XEvent *e2) {
  e2 -> type = e1 -> type;
  e2 -> xconfigure.serial = e1 -> xconfigure.serial;
  e2 -> xconfigure.send_event = e1 -> xconfigure.send_event;
  e2 -> xconfigure.display = e1 -> xconfigure.display;
  e2 -> xconfigure.event = e1 -> xconfigure.event;
  e2 -> xconfigure.window = e1 -> xconfigure.window;
  e2 -> xconfigure.x = e1 -> xconfigure.x;
  e2 -> xconfigure.y = e1 -> xconfigure.y;
  e2 -> xconfigure.width = e1 -> xconfigure.width;
  e2 -> xconfigure.height = e1 -> xconfigure.height;
  e2 -> xconfigure.border_width = e1 -> xconfigure.border_width;
  e2 -> xconfigure.above = e1 -> xconfigure.above;
  e2 -> xconfigure.override_redirect = e1 -> xconfigure.override_redirect;
}

static void copy_xexpose_event (XEvent *e1, XEvent *e2) {
  e2 -> type = e1 -> type;
  e2 -> xexpose.serial = e1 -> xexpose.serial;
  e2 -> xexpose.send_event = e1 -> xexpose.send_event;
  e2 -> xexpose.display = e1 -> xexpose.display;
  e2 -> xexpose.window = e1 -> xexpose.window;
  e2 -> xexpose.x = e1 -> xexpose.x;
  e2 -> xexpose.y = e1 -> xexpose.y;
  e2 -> xexpose.width = e1 -> xexpose.width;
  e2 -> xexpose.height = e1 -> xexpose.height;
  e2 -> xexpose.count = e1 -> xexpose.count;
}

#if 0
static int kwin_event_loop (int parent_fd, int mem_handle, int main_win_id) {
  struct timespec req, rem;
  int events_waiting, handle_count;
  char *s;
  int eventclass, eventdata1, eventdata2, eventdata3, eventdata4,
    eventdata5;
  char buf[MAXMSG], e_class[64], e1[64], e2[64], e3[64], e4[64], 
    e5[64];
  XEvent e_expose, e, e_config, e_next, e_next_2, e_peek;
  XRectangle prev_d;
  bool resizing_config = false, buttonpressed = false, grabbed;

 next_req: while (1) {
    __xlib_handle_client_request (mem_handle);
    while ((events_waiting = XPending (display)) > 0) {
      XNextEvent (display, &e);

      /* When resizing, KWin sends a series of three-event
	 sequences:
	 Expose
	 ConfigureNotify (send_event == true)
	 ConfigureNotify (send_event == false)
	 
	 Try to act only on the last of these sequences. */
      if (e.type == Expose) {
	XNextEvent (display, &e_next);
	if (e_next.type == ConfigureNotify &&
	    e_next.xconfigure.send_event == true) {
	  XNextEvent (display, &e_next_2);
	  if (e_next_2.type == ConfigureNotify &&
	      e_next_2.xconfigure.send_event == false) {
	    XPeekEvent (display, &e_peek);
	    if (e_peek.type != Expose) {
	      copy_xconfig_event (&e_next_2, &e);
	      goto kwin_configure;
	    } else {
	      continue;
	    }
	  } else {
	    copy_xconfig_event (&e_next_2, &e);
	  }
	} else {
	  copy_xconfig_event (&e_next, &e);
	}
      }
      
      switch (e.type)
	{
	case ButtonPress:
	  if (e.xbutton.window == main_win_id) {
	    eventclass = BUTTONPRESS;
	    eventdata1 = e.xbutton.x;
	    eventdata2 = e.xbutton.y;
	    eventdata3 = e.xbutton.state;
	    eventdata4 = e.xbutton.button;
	    eventdata5 = 0;
	    buttonpressed = TRUE;
	  }
	  break;
	case ButtonRelease:
	  if (e.xbutton.window == main_win_id) {
	    eventclass = BUTTONRELEASE;
	    eventdata1 = e.xbutton.x;
	    eventdata2 = e.xbutton.y;
	    eventdata3 = e.xbutton.state;
	    eventdata4 = e.xbutton.button;
	    eventdata5 = 0;
	    buttonpressed = FALSE;
	  }
	  break;
	case KeyPress:
	  eventclass = KEYPRESS;
	  eventdata1 = e.xkey.x;
	  eventdata2 = e.xkey.y;
	  eventdata3 = e.xkey.state;
	  eventdata4 = e.xkey.keycode;
	  eventdata5 = 
	    get_x11_keysym (e.xkey.keycode, e.xkey.state, TRUE);
	  break;
	case KeyRelease:
	  eventclass = KEYRELEASE;
	  eventdata1 = e.xkey.x;
	  eventdata2 = e.xkey.y;
	  eventdata3 = e.xkey.state;
	  eventdata4 = e.xkey.keycode;
	  eventdata5 = 
	    get_x11_keysym (e.xkey.keycode, e.xkey.state, FALSE);
	  break;
	case FocusIn:
	case FocusOut:
	  event_to_client (FOCUSCHANGENOTIFY,
			   e.xfocus.window, e.xfocus.type,
			   e.xfocus.mode, e.xfocus.detail, 0, 0, 0);
	  eventclass = 0;
	  continue;
	  break;
	case PropertyNotify:
	  break;
  	case MotionNotify:
	  while (XCheckTypedWindowEvent 
		 (display, main_win_id, MotionNotify, &e))
	    ;
	  event_to_client (MOTIONNOTIFY,
			   e.xmotion.window, e.xmotion.x, e.xmotion.y,
			   e.xmotion.state, e.xmotion.is_hint, 0, 0);
	  eventclass = 0;
	  continue;
  	  break;
  	case MapNotify:
	  while (XCheckTypedWindowEvent 
		 (display, main_win_id, MapNotify, &e))
	    ;
  	  eventclass = MAPNOTIFY;
	  eventdata1 = e.xmap.event;
	  eventdata2 = e.xmap.window;
	  eventdata3 = eventdata4 = eventdata5 = eventdata6 = 0;
  	  break;
   	case Expose:
	  if (e.xexpose.serial == 0) {
	    eventclass = EXPOSE;
	    eventdata1 = e.xexpose.x;
	    eventdata2 = e.xexpose.y;
	    eventdata3 = e.xexpose.width;
	    eventdata4 = e.xexpose.height;
	    eventdata5 = e.xexpose.count;
	  }
   	  break;
	case ConfigureNotify:
	kwin_configure:
	  if (e.xconfigure.window == main_win_id) {
	    if ((e.xconfigure.x && e.xconfigure.y) &&
		((prev_d.x != e.xconfigure.x) ||
		 (prev_d.y != e.xconfigure.y))) {
	      eventclass = MOVENOTIFY;
	      prev_d.x = e.xconfigure.x;
	      prev_d.y = e.xconfigure.y;
	      event_to_client (eventclass, e.xconfigure.x, 
			       e.xconfigure.y, 
			       e.xconfigure.width,
			       e.xconfigure.height,
			       e.xconfigure.border_width, 0);
	      if ((prev_d.width != e.xconfigure.width) ||
		  (prev_d.height != e.xconfigure.height)) {
		resize_event_to_client
		  (e.xconfigure.x, 
		   e.xconfigure.y, 
		   e.xconfigure.width,
		   e.xconfigure.height,
		   e.xconfigure.border_width,
		   &prev_d, &eventclass);
	      }
	    } else if ((prev_d.width != e.xconfigure.width) ||
		       (prev_d.height != e.xconfigure.height)) {
	      resize_event_to_client
		(e.xconfigure.x, e.xconfigure.y,
		 e.xconfigure.width, e.xconfigure.height,
		 e.xconfigure.border_width, &prev_d, &eventclass);
	    }
	    event_to_client (EXPOSE, e.xconfigure.x, 
			     e.xconfigure.y, 
			     e.xconfigure.width,
			     e.xconfigure.height, 0, 0);

	  }
	  break;
	case ClientMessage:
	  if(e.xclient.data.l[0] == wm_delete_window) {
	    eventclass = WINDELETE;
	    eventdata1 = eventdata2 = eventdata3 = eventdata4 = 
	      eventdata5 = 0;
	    
	  }
	  break;
	case VisibilityNotify:
	  break;
	case CirculateNotify:
	case CirculateRequest:
	case ResizeRequest:
	case EnterNotify:
	case LeaveNotify:
	  eventclass = 0;
	  break;
	default:
	  eventclass = 0;
	  break;
	}
      if (eventclass) {

	event_to_client (eventclass, eventdata1, 
			 eventdata2, eventdata3,
			 eventdata4, eventdata5, 0);

	eventclass = eventdata1 = eventdata2 = 0;
      }
    }
    if ((s = (char *)get_shmem (mem_id)) == NULL)
      return ERROR;
    if (client_shutdown_requested (s)) {
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "Process %d releasing shared memory.\n",
	       (int) getpid ());
#endif
      handle_count = detach_shmem (mem_id, s);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "Shared memory attached to %d processes.\n",
	       handle_count);
#endif
      if (shutdown (parent_fd, SHUT_WR))
	fprintf (stderr, "__ctalkX11InputClient: shutdown: %s.\n",
		 strerror (errno));
      clear_font_descriptors (display);
      return SUCCESS;
    } else {
      handle_count = detach_shmem (mem_id, s);
    }
    req.tv_sec = rem.tv_sec = rem.tv_nsec = 0;
    req.tv_nsec = 10000000;
    while (1) {
      if (nanosleep (&req, &rem)) {
	if (errno == EINTR) {
	  req.tv_nsec = rem.tv_nsec;
	  req.tv_sec = rem.tv_sec = rem.tv_nsec = 0;
	} else {
	  printf ("__ctalkX11InputClient: nanosleep: %s.\n",
		  strerror (errno));
	  return ERROR;
	}
      } else {
	break;
      }
    }
  }
  return SUCCESS;
}
#endif


/* Uncomment this if you want the library to print a lot of event
   information on a xterm */
/* #define TRACK_EVENTS */

int __ctalkX11InputClient (OBJECT *streamobject, int parent_fd, int mem_handle, int main_win_id) {
  XEvent e, e_config, e_expose;
  XRectangle prev_d;
  int events_waiting, handle_count;
  int eventclass, event_win,
    eventdata1, eventdata2, eventdata3, eventdata4,
    eventdata5;
  int client_sock_fd, wresult;
  char *s;
  char buf[MAXMSG], e_class[64], e1[64], e2[64], e3[64], e4[64], 
    e5[64];
  int buttonpressed = FALSE;
  int retval = SUCCESS;
  struct timespec req, rem;
  Atom compiz_resize_atom = None;
  bool compiz_resizing = false, xfce_resizing = false, resizing = false,
    grabbed = false;
  eventclass = eventdata1 = eventdata2 = eventdata3 = eventdata4 = 
    eventdata5 = 0;
  memset ((void *)&prev_d, '\0', sizeof (XRectangle));
  e_config.type == -1;
  char *shm_mem_2;
  EV_CH *ev;

#if 0
  if (wm_kwin)
    return kwin_event_loop (parent_fd, mem_handle, main_win_id);
#endif

  if ((shm_mem_2 = get_shmem (mem_handle)) == NULL) return -1;

  if ((client_sock_fd = __x11_pane_socket_connect (parent_fd)) == ERROR) {
    fprintf (stderr, "ctalk: Couldn't open X11 client socket.  Exiting.\n");
    _exit (1);
  }

  while (1) {
    __xlib_handle_client_request (shm_mem_2);
    while ((events_waiting = XPending (display)) > 0) {
      XNextEvent (display, &e);
      if (compiz_resizing) {
	if (e.type == ConfigureNotify) {
	  copy_xconfig_event (&e, &e_config);
	  continue;
	}
      }
      if (e.type == Expose && e.xexpose.count > 0)
	continue;

      switch (e.type)
	{
	case ButtonPress:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> ButtonPress\n");
#endif	  
	  buttonpressed = TRUE;
	  event_to_client (BUTTONPRESS,
			   e.xbutton.window,
			   e.xbutton.x,
			   e.xbutton.y,
			   e.xbutton.state,
			   e.xbutton.button, 0, 0);
	  continue;
	  break;
	case ButtonRelease:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> ButtonRelease\n");
#endif	  
	  buttonpressed = FALSE;
	  event_to_client (BUTTONRELEASE,
			   e.xbutton.window,
			   e.xbutton.x,
			   e.xbutton.y,
			   e.xbutton.state,
			   e.xbutton.button, 0, 0);
	  continue;
	  break;
	case KeyPress:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> KeyPress\n");
#endif	  
	  event_to_client (KEYPRESS, e.xkey.window,
			   e.xkey.x, e.xkey.y, e.xkey.state,
			   e.xkey.keycode,
			   get_x11_keysym (e.xkey.keycode,
					   e.xkey.state, true),
			   e.xkey.time);
	  continue;
	  break;
	case KeyRelease:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> KeyRelease\n");
#endif	  
	  event_to_client (KEYRELEASE, e.xkey.window,
			   e.xkey.x, e.xkey.y, e.xkey.state,
			   e.xkey.keycode,
			   get_x11_keysym (e.xkey.keycode,
					   e.xkey.state, false),
			   e.xkey.time);
	  continue;
	  break;
	case FocusIn:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> FocusIn\n");
#endif	  
	  if (e.xfocus.mode == NotifyUngrab) {
	    grabbed = false;
	    if (resizing) {
	      resizing = false;
	      resize_event_to_client
		(e_config.xconfigure.x, 
		 e_config.xconfigure.y, 
		 e_config.xconfigure.width,
		 e_config.xconfigure.height,
		 e_config.xconfigure.border_width,
		 &prev_d, e_config.xconfigure.window,
		 &eventclass);
	      eventclass = EXPOSE;
	      event_to_client (eventclass,
			       e_config.xconfigure.window,
			       e_expose.xexpose.x,
			       e_expose.xexpose.y,
			       e_expose.xexpose.width,
			       e_expose.xexpose.height,
			       e_expose.xexpose.count, 0);
	      eventclass = 0;
	      continue;
	    }
	  }
	  event_to_client (WMFOCUSCHANGENOTIFY,
			   e.xfocus.window, e.xfocus.type,
			   e.xfocus.mode, e.xfocus.detail, 0, 0, 0);
	  eventclass = 0;
	  continue;
	  break;
	case FocusOut:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> FocusOut\n");
#endif	  
	  if (e.xfocus.mode == NotifyGrab) {
	    grabbed = true;
	  }
	  event_to_client (WMFOCUSCHANGENOTIFY,
			   e.xfocus.window, e.xfocus.type,
			   e.xfocus.mode, e.xfocus.detail, 0, 0, 0);
	  eventclass = 0;
	  continue;
	  break;
	case EnterNotify:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> EnterNotify\n");
#endif	  
	  event_to_client (ENTERWINDOWNOTIFY,
			   e.xcrossing.window, e.xcrossing.subwindow,
			   e.xcrossing.mode, e.xcrossing.detail,
			   e.xcrossing.time, 0, 0);
	  eventclass = 0;
	  continue;
	  break;
	case LeaveNotify:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> LeaveNotify\n");
#endif	  
	  event_to_client (LEAVEWINDOWNOTIFY,
			   e.xcrossing.window, e.xcrossing.subwindow,
			   e.xcrossing.mode, e.xcrossing.detail,
			   e.xcrossing.time, 0, 0);
	  eventclass = 0;
	  continue;
	  break;
	case PropertyNotify:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> PropertyNotify\n");
#endif	  
	  if (compiz_resize_atom == None) {
	    compiz_resize_atom =
	      XInternAtom (display, "_COMPIZ_RESIZE_INFORMATION", True);
	  }
	  if (e.xproperty.atom == compiz_resize_atom) {
	    if (e.xproperty.state) {
	      compiz_resizing = false;
	      if (e_config.type != -1) {
		XWindowAttributes xwa;
		copy_xconfig_event (&e_config, &e);
		/* don't laugh.  it works */
		XGetWindowAttributes (display, main_win_id, &xwa);
		e.xconfigure.width = xwa.width;
		e.xconfigure.height = xwa.height;
		goto do_configure;
	      }
	    } else {
	      compiz_resizing = true;
	    }
	  }
	  break;
  	case MotionNotify:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> MotionNotify\n");
#endif	  
	  while (XCheckTypedWindowEvent 
		 (display, main_win_id, MotionNotify, &e))
	    ;
	  event_to_client (MOTIONNOTIFY,
			   e.xmotion.window,
			   e.xmotion.x,
			   e.xmotion.y,
			   e.xmotion.state,
			   e.xmotion.is_hint,
			   0, 0);
	  continue;
  	  break;
  	case MapNotify:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> MapNotify\n");
#endif	  
	  while (XCheckTypedWindowEvent 
		 (display, main_win_id, MapNotify, &e))
	    ;
	  event_to_client (MAPNOTIFY,
			   e.xmap.window,
			   e.xmap.event,
			   e.xmap.window,
			   0, 0, 0, 0);
  	  break;
   	case Expose:
#ifdef TRACK_EVENTS
	  fprintf (stderr, "Expose\n");
#endif	  

	  if (resizing) {
	    /* resizing is set for cinammon, and we just save the
	       most recent event until the resizing is completed. */
	    copy_xexpose_event (&e, &e_expose);
	    continue;
	  }

	  /* compiz needs this here */
	  while (XCheckTypedWindowEvent 
		 (display, main_win_id, Expose, &e))
	    ;
	  if (buttonpressed)  {
	    continue;
	  }

	  event_to_client (EXPOSE,
			   e.xexpose.window, e.xexpose.x, e.xexpose.y,
			   e.xexpose.width, e.xexpose.height, 0, 0);
	  eventclass = 0;
	  continue;
   	  break;
	case ConfigureNotify:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> ConfigureNotify\n");
#endif	  
	  /* 
	   * Here we have to separate the event into (possible) move 
	   * and resize events.  If the event's x and y are 0, then it's 
	   * only a size change.  Otherwise, it's a position change, and
	   * we should also check for a new size also.  Different
	   * window managers handle ConfigureNotifies a little differently,
	   * so if we handle all of the possibilities here, then we don't
	   * need to worry as much about what type of window manager 
	   * is sending the events.
	   */
	  

	  if (grabbed) {
	    /* cinnamon notifies us of a FocusIn/Out ungrab or grab,
	       so we save the event until the ungrab, and set 
	       resizing for everything else during the resize
	       operation. */
	    if ((prev_d.width != e.xconfigure.width) ||
		(prev_d.height != e.xconfigure.height)) {
	      resizing = true;
	      copy_xconfig_event (&e, &e_config);
	      continue;
	    }
	  }

	  if (wm_xfce) {
	    /* with xfce we only need to keep the last
	       ConfigureNotify event of the resizing 
	       operation. */
	    XEvent e_next;
	    xfce_resizing = true;
	    while (xfce_resizing) {
	      XPeekEvent (display, &e_next);
	      if (e_next.type != ConfigureNotify &&
		  e_next.type != Expose) {
	      resize_event_to_client
		(e.xconfigure.x, 
		 e.xconfigure.y, 
		 e.xconfigure.width,
		 e.xconfigure.height,
		 e.xconfigure.border_width,
		 &prev_d, e.xconfigure.window, &eventclass);
		xfce_resizing = false;
	      }
	      while (XCheckTypedWindowEvent 
		     (display, main_win_id, ConfigureNotify, &e))
		;
	    }
	    continue;
	  }

	  /* These work fine with mwm. */

	  if (buttonpressed)  {
	    while (XCheckTypedWindowEvent 
		   (display, main_win_id, Expose, &e))
	      ;
	    continue;
	  }

	  if (e.xconfigure.window == main_win_id) {
	    /* Anything else *should* just be registered on the
	       window itself. */
	  do_configure:
	    if (e.xconfigure.x && e.xconfigure.y) {
	      if ((prev_d.x != e.xconfigure.x) ||
		  (prev_d.y != e.xconfigure.y)) {
		eventclass = MOVENOTIFY;
		prev_d.x = e.xconfigure.x;
		prev_d.y = e.xconfigure.y;
		event_to_client (eventclass,
				 e.xconfigure.window,
				 e.xconfigure.x, 
				 e.xconfigure.y, 
				 e.xconfigure.width,
				 e.xconfigure.height,
				 e.xconfigure.border_width, 0);

		/* This is new. Works fine with xfce. */
		if ((prev_d.width != e.xconfigure.width) ||
		    (prev_d.height != e.xconfigure.height)) {
		  while (XCheckTypedWindowEvent 
			 (display, main_win_id, ConfigureNotify, &e))
		    ;
		  resize_event_to_client
		    (e.xconfigure.x, 
		     e.xconfigure.y, 
		     e.xconfigure.width,
		     e.xconfigure.height,
		     e.xconfigure.border_width,
		     &prev_d, e.xconfigure.window, &eventclass);
		}
		eventclass = 0;
		continue;
	      } else if ((prev_d.width != e.xconfigure.width) ||
			 (prev_d.height != e.xconfigure.height)) {
		resize_event_to_client
		  (e.xconfigure.x, e.xconfigure.y,
		   e.xconfigure.width, e.xconfigure.height,
		   e.xconfigure.border_width, &prev_d,
		   e.xconfigure.window, &eventclass);
	      }
	    } else {
	      if ((prev_d.width != e.xconfigure.width) ||
		  (prev_d.height != e.xconfigure.height)) {
		XEvent e_expose;
		while (XCheckTypedWindowEvent 
		       (display, main_win_id, ConfigureNotify, &e))
		  ;
		/* compiz still uses this occasionally, too */
		while (XCheckTypedWindowEvent 
		       (display, main_win_id, Expose, &e_expose))
		  ;
		eventclass = RESIZENOTIFY;
		resize_event_to_client
		  (e.xconfigure.x, e.xconfigure.y,
		   e.xconfigure.width, e.xconfigure.height,
		   e.xconfigure.border_width, &prev_d,
		   e.xconfigure.window, &eventclass);
		continue;
	      }
	    }
	  }
	  break;
	case ClientMessage:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> ClientMessage\n");
#endif	  
	  if(e.xclient.data.l[0] == wm_delete_window) {
	    eventclass = WINDELETE;
	    event_win = e.xclient.window;
	    eventdata1 = eventdata2 = eventdata3 = eventdata4 = 
	      eventdata5 = 0;
	  }
	  break;
	case SelectionClear:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> SelectionClear\n");
#endif	  
	  /* We can do the actual releasing of the X primary selection
	     in the server thread, but we send an event back to the app's
	     thread so the app can update its state if necessary. */
	  __xlib_clear_selection (&e);
	  event_to_client (SELECTIONCLEAR,
			   e.xselectionclear.window,
			   0, 0, 0, 0, 0, 0);
	  continue;
	  break;
	case SelectionRequest:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> SelectionRequest\n");
#endif	  
	  /* we can do all of this in the server thread, too,
	     completely */
	  __xlib_send_selection (&e);
	  continue;
	  break;
	case CirculateNotify:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> CirculateNotify\n");
#endif	  
	case CirculateRequest:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> CirculateRequest\n");
#endif	  
	case ResizeRequest:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> ResizeRequest\n");
#endif	  
	case VisibilityNotify:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> VisibilityNotify\n");
#endif	  
	  eventclass = 0;
	  break;
	case NoExpose:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> NoExpose\n");
#endif	  
	  break;
	default:
#ifdef TRACK_EVENTS
	  fprintf (stderr, ">>> default event: %d\n", e.type);
#endif	  
	  eventclass = 0;
	  break;
	}
      if (eventclass) {
	event_to_client (eventclass, event_win,eventdata1, eventdata2,
			 eventdata3, eventdata4, eventdata5, 0);
	eventclass = eventdata1 = eventdata2 = 0;
      }
    }
    if ((ev = ev_unshift ()) != NULL) {
      event_to_client (ev -> event_class,
		       ev -> win_id, ev -> eventdata1,
		       ev -> eventdata2, ev -> eventdata3,
		       ev -> eventdata4, ev -> eventdata5,
		       ev -> eventdata6);
      stash_ev (ev);
    }
    if ((s = (char *)get_shmem (mem_id)) == NULL) {
      retval = ERROR;
      goto client_loop_done;
    }
    if (client_shutdown_requested (s)) {
      handle_count = detach_shmem (mem_id, s);
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO  
      fprintf (stderr, "Process %d releasing shared memory.\n",
	       (int) getpid ());
      fprintf (stderr, "Shared memory attached to %d processes.\n",
	       handle_count);
#endif
      if (shutdown (client_sock_fd, SHUT_WR))
	fprintf (stderr, "__ctalkX11InputClient: shutdown: %s.\n",
		 strerror (errno));
      clear_font_descriptors (display);
      retval = SUCCESS;
      goto client_loop_done;
    } else {
      handle_count = detach_shmem (mem_id, s);
    }
    req.tv_sec = rem.tv_sec = rem.tv_nsec = 0;
    req.tv_nsec = 10000000;
    while (1) {
      if (nanosleep (&req, &rem)) {
	if (errno == EINTR) {
	  req.tv_nsec = rem.tv_nsec;
	  req.tv_sec = rem.tv_sec = rem.tv_nsec = 0;
	} else {
	  printf ("__ctalkX11InputClient: nanosleep: %s.\n",
		  strerror (errno));
	  retval = ERROR;
	  goto client_loop_done;
	}
      } else {
	break;
      }
    }
  }
 client_loop_done:
  detach_shmem (mem_handle, shm_mem_2);
  return SUCCESS;
}

OBJECT *__x11_pane_win_id_value_object (OBJECT *paneobject) {
  static OBJECT *pane_self_object, 
    *winid_var_object,
    *winid_var_value_object;

  if (!IS_OBJECT(paneobject)) return NULL;
  if (!strcmp (paneobject -> __o_name, "value") &&
      paneobject -> __o_p_obj)
    pane_self_object = paneobject -> __o_p_obj;
  else
    pane_self_object = paneobject;

  if ((winid_var_object = 
       __ctalkGetInstanceVariable (pane_self_object, "xWindowID", FALSE))
      != NULL) {
    winid_var_value_object = winid_var_object -> instancevars;
  } else if ((winid_var_object = 
	      __ctalkGetInstanceVariable (pane_self_object,
					  "xID", FALSE))
	     != NULL) {
    winid_var_value_object = winid_var_object -> instancevars;
  }

  if (!IS_OBJECT(winid_var_value_object)) {
    _warning ("ctalk:  %s: "
  	      "Could not find xID or xWindowID instance variable.\n",
	      pane_self_object -> __o_name);
    __warning_trace ();
    return NULL;
  }
  return winid_var_value_object;
}

OBJECT *__x11_pane_win_gc_value_object (OBJECT *paneobject) {
  static OBJECT *pane_self_object, 
    *wingc_var_object,
    *wingc_var_value_object;

  if (!IS_OBJECT(paneobject)) return NULL;

  if (!strcmp (paneobject -> __o_name, "value") &&
      paneobject -> __o_p_obj)
    pane_self_object = paneobject -> __o_p_obj;
  else
    pane_self_object = paneobject;
  if ((wingc_var_object = 
       __ctalkGetInstanceVariable (pane_self_object, "xGC", TRUE))
      == NULL)
    return NULL;
  if ((wingc_var_value_object = wingc_var_object -> instancevars)
      == NULL)
    return NULL;

  return wingc_var_value_object;
}

/* Get a dimension instance var, named by xyvarname, of the point
   instance var named by ptvarname */

static OBJECT *pane_pt_var (OBJECT *paneobject, char *ptvarname,
			     char *xyvarname) {
  OBJECT *pane_self_object, *pt_var, *xy_var;
  if (!IS_OBJECT(paneobject)) return NULL;
  if ((paneobject -> attrs & OBJECT_IS_VALUE_VAR) &&
      paneobject -> __o_p_obj)
    pane_self_object = paneobject -> __o_p_obj;
  else
    pane_self_object = paneobject;
  if ((pt_var = 
       __ctalkGetInstanceVariable (pane_self_object, ptvarname, TRUE))
      == NULL)
    return NULL;
  if ((xy_var =
       __ctalkGetInstanceVariable (pt_var, xyvarname, TRUE)) == NULL)
    return NULL;
  return xy_var -> instancevars;
}

static OBJECT *__x11_pane_win_depth_value_object (OBJECT *paneobject) {
  static OBJECT *pane_self_object, 
    *depth_var_object,
    *depth_var_value_object;

  if (!IS_OBJECT(paneobject)) return NULL;
  if (!strcmp (paneobject -> __o_name, "value") &&
      paneobject -> __o_p_obj)
    pane_self_object = paneobject -> __o_p_obj;
  else
    pane_self_object = paneobject;
  if ((depth_var_object = 
       __ctalkGetInstanceVariable (pane_self_object, "depth", TRUE))
      == NULL)
    return NULL;
  if ((depth_var_value_object = depth_var_object) == NULL)
    return NULL;

  return depth_var_value_object;
}

OBJECT *__x11_pane_font_id_value_object (OBJECT *paneobject) {
  static OBJECT *pane_self_object, *pane_font_object,
    *pane_font_fontid_object, *pane_font_fontid_value_object; 
    
  if (!IS_OBJECT(paneobject)) return NULL;
  pane_self_object = (IS_VALUE_INSTANCE_VAR(paneobject) ?
		      paneobject -> __o_p_obj : paneobject);
  if ((pane_font_object = 
       __ctalkGetInstanceVariable (pane_self_object, "fontVar", TRUE))
      == NULL)
    return NULL;
  if ((pane_font_fontid_object = 
       __ctalkGetInstanceVariable (pane_font_object, "fontId", TRUE)) 
      == NULL)
    return NULL;
  if ((pane_font_fontid_value_object =
       pane_font_fontid_object -> instancevars) == NULL)
    return NULL;
  return pane_font_fontid_value_object;
}

OBJECT *__x11_pane_display_value_object (OBJECT *paneobject) {
  static OBJECT *pane_self_object, *pane_display_object,
    *pane_display_value_object;
  if (!IS_OBJECT(paneobject)) return NULL;
  pane_self_object = (IS_VALUE_INSTANCE_VAR(paneobject) ?
		      paneobject -> __o_p_obj : paneobject);
  if ((pane_display_object = 
       __ctalkGetInstanceVariable (pane_self_object, "displayPtr", TRUE))
      == NULL)
    return NULL;

  if ((pane_display_value_object =
       pane_display_object -> instancevars) == NULL)
    return NULL;

  return pane_display_value_object;
  
}

/* Returns a default of 1 if the instance var isn't found. */
int __x11_pane_border_width (OBJECT *paneobject) {
  static OBJECT *pane_self_object, *pane_borderwidth_object,
    *pane_borderwidth_value_object; 
  if (!IS_OBJECT(paneobject)) return 1;
  pane_self_object = (IS_VALUE_INSTANCE_VAR(paneobject) ?
		      paneobject -> __o_p_obj : paneobject);
  if ((pane_borderwidth_object = 
       __ctalkGetInstanceVariable (pane_self_object, "borderWidth", TRUE))
      == NULL)
    return 1;

  if ((pane_borderwidth_value_object =
       pane_borderwidth_object -> instancevars) == NULL)
    return 1;

  return *(int *)pane_borderwidth_value_object -> __o_value;
}

void __save_window_geometry (OBJECT *pane_object, XSizeHints *size_hints) {
  OBJECT *origin_instvar, *size_instvar, *origin_x_instvar, *origin_y_instvar,
    *size_x_instvar, *size_y_instvar;
  origin_instvar = __ctalkGetInstanceVariable
    (pane_object, "origin", TRUE);
  origin_x_instvar = __ctalkGetInstanceVariable
    (origin_instvar, "x", TRUE);
  origin_y_instvar = __ctalkGetInstanceVariable
    (origin_instvar, "y", TRUE);
  size_instvar = __ctalkGetInstanceVariable
    (pane_object, "size", TRUE);
  size_x_instvar = __ctalkGetInstanceVariable
    (size_instvar, "x", TRUE);
  size_y_instvar = __ctalkGetInstanceVariable
    (size_instvar, "y", TRUE);
  *(int *)origin_x_instvar -> __o_value =
  *(int *)origin_x_instvar -> instancevars -> __o_value = size_hints -> x;
  *(int *)origin_y_instvar -> __o_value =
    *(int *)origin_y_instvar -> instancevars -> __o_value = size_hints -> y;
  *(int *)size_x_instvar -> __o_value =
    *(int *)size_x_instvar -> instancevars -> __o_value = size_hints -> width;
  *(int *)size_y_instvar -> __o_value =
    *(int *)size_y_instvar -> instancevars -> __o_value = size_hints -> height;
}

void __save_pane_to_vars (OBJECT *pane_object, GC gc,
			  int win_id, int screen_depth) {
  OBJECT *win_gc_value_obj, *win_id_value_obj,
    *win_depth_obj;
  char buf[MAXLABEL];

  win_id_value_obj =
    __x11_pane_win_id_value_object (pane_object);
  win_gc_value_obj = 
    __x11_pane_win_gc_value_object (pane_object);
  win_depth_obj = 
    __x11_pane_win_depth_value_object (pane_object);
  
  *(uintptr_t *)win_gc_value_obj -> __o_value = (uintptr_t)gc;
  if (IS_OBJECT(win_gc_value_obj -> __o_p_obj))
    *(uintptr_t *)win_gc_value_obj -> __o_p_obj -> __o_value =
      (uintptr_t)gc;
  
  *(unsigned int *)win_id_value_obj -> __o_value = win_id;
  if (IS_OBJECT(win_id_value_obj -> __o_p_obj))
    *(unsigned int *)win_id_value_obj -> __o_p_obj -> __o_value = win_id;

  *(int *)win_depth_obj -> __o_value = screen_depth;
  if (IS_OBJECT(win_depth_obj -> instancevars))
    *(int *)win_depth_obj -> instancevars -> __o_value = screen_depth;
}
			  

/* Declared in xgeometry.c */
extern XSizeHints *size_hints;
extern int __geomFlags;

GC create_pane_win_gc (Display *d, Window w, OBJECT *pane) {
  /* Also sets the window background if backgroundColor is set,
     white otherwise.  */
  int l_screen;
  static GC gc;
  XGCValues gcv;
  OBJECT *bgColorVar, *fgColor, *resources, *bgResource;
  XColor bg_color, fg_color;

  l_screen = DefaultScreen (d);
  if ((bgColorVar = __ctalkGetInstanceVariable (pane, "backgroundColor", FALSE))
      == NULL) {
    bgColorVar = __ctalkGetInstanceVariable (pane, "background", FALSE);
  }
  fgColor = __ctalkGetInstanceVariable (pane, "foregroundColor", TRUE);
  
  if (((bgResource = __ctalkPaneResource (pane, "backgroundColor", FALSE))
      != NULL) ||
      ((bgResource = __ctalkPaneResource (pane, "background", FALSE))
       !=  NULL)) {
    lookup_color (d, &bg_color, bgResource -> __o_value);
    gcv.background = bg_color.pixel;
    XSetWindowBackground (d, w, bg_color.pixel);
  } else if (*bgColorVar -> __o_value &&
	     !str_eq (bgColorVar -> __o_value, NULLSTR)) {
    lookup_color (d, &bg_color, bgColorVar -> __o_value);
    gcv.background = bg_color.pixel;
    XSetWindowBackground (d, w, bg_color.pixel);
  } else {
    gcv.background = WhitePixel (d, l_screen);
    XSetWindowBackground (d, w, WhitePixel (display, l_screen));
  }

  if (*fgColor -> __o_value && !str_eq (fgColor -> __o_value, NULLSTR)) {
    lookup_color (d, &fg_color, fgColor -> __o_value);
    gcv.foreground = fg_color.pixel;
  } else {
    gcv.foreground = BlackPixel (d, l_screen);
  }
  if ((gcv.font = get_user_font (pane)) == 0) {
    if (n_fixed_fonts && !fixed_font)
      fixed_font = XLoadFont (d, fixed_font_set[0]);
    if (fixed_font)
      gcv.font = fixed_font;
  }
  gcv.fill_style = FillSolid;
  gcv.function = GXcopy;
  gcv.graphics_exposures = false;
  gc = XCreateGC (d, w, DEFAULT_GCV_MASK|GCGraphicsExposures, &gcv);

  return gc;
}

int __ctalkCreateX11MainWindow (OBJECT *self_object) {
  Window win_id, root_return;
  XSetWindowAttributes set_attributes;
  XGCValues gcv;
  GC gc;
  XWMHints wm_hints;
  char buf[MAXLABEL];
  static int wm_event_mask;
  int geom_ret, x_return, y_return;
  int pane_x, pane_y, pane_width, pane_height, border_width;
  unsigned int width_return, height_return, depth_return, border_width_return;
  int x_org, y_org, x_size, y_size;
  OBJECT *displayPtr_value;

  wm_event_mask = WM_CONFIGURE_EVENTS | WM_INPUT_EVENTS;

  if (!__x11_open_display ())
    return ERROR;

  border_width = __x11_pane_border_width (self_object);
  displayPtr_value = __x11_pane_display_value_object (self_object);
  SYMVAL(displayPtr_value -> __o_value) = (uintptr_t)display;
  set_attributes.backing_store = Always;
  set_size_hints_internal (self_object, &x_org, &y_org, &x_size, &y_size);

  win_id = XCreateWindow (display, root, 
			  x_org, y_org, x_size, y_size,
			  border_width, screen_depth,
			  CopyFromParent, CopyFromParent, 
			  CWBackingStore,
			  &set_attributes);
  wm_hints.flags = (InputHint|StateHint);
  wm_hints.input = TRUE;;
  wm_hints.initial_state = NormalState;
  XSetWMHints (display, win_id, &wm_hints);
  
  wm_delete_window = XInternAtom (display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (display, win_id, &wm_delete_window, 1);

  if (size_hints) {
    XSetWMNormalHints (display, win_id, size_hints);
    geom_ret = XGetGeometry (display, win_id, &root_return,
     			     &x_return, &y_return,
     			     &width_return, &height_return,
     			     &border_width_return,
     			     &depth_return);
    size_hints -> x = x_return;
    size_hints -> y = y_return;
    size_hints -> base_width = width_return;
    size_hints -> base_height = height_return;
    __save_window_geometry (self_object, size_hints);
  }

  XSetWindowBorder (display, win_id, BlackPixel(display, screen));
  XSelectInput(display, win_id, wm_event_mask);

  gc = create_pane_win_gc (display, win_id, self_object);

  __xlib_set_wm_name_prop 
    ((Display *)SYMVAL(displayPtr_value -> __o_value), win_id, gc, 
     basename_w_extent(__argvFileName ()));

  __save_pane_to_vars (self_object, gc, win_id,
		       DefaultDepth (display, DefaultScreen (display)));
  return win_id;
}

int __ctalkCreateX11MainWindowTitle (OBJECT *self_object, char *title) {
  Window win_id, root_return;
  XSetWindowAttributes set_attributes;
  XGCValues gcv;
  GC gc;
  XWMHints wm_hints;
  char buf[MAXLABEL];
  static int wm_event_mask;
  int geom_ret, x_return, y_return;
  int pane_x, pane_y, pane_width, pane_height, border_width;
  unsigned int width_return, height_return, depth_return, border_width_return;
  int x_org, y_org, x_size, y_size;
  OBJECT *displayPtr_value;

  wm_event_mask = WM_CONFIGURE_EVENTS | WM_INPUT_EVENTS;

  if (!__x11_open_display ())
    return ERROR;

  border_width = __x11_pane_border_width (self_object);
  displayPtr_value = __x11_pane_display_value_object (self_object);
  SYMVAL(displayPtr_value -> __o_value) = (uintptr_t)display;
  set_attributes.backing_store = Always;
  set_size_hints_internal (self_object, &x_org, &y_org, &x_size, &y_size);
  win_id = XCreateWindow (display, root, 
			  x_org, y_org, x_size, y_size,
			  border_width, screen_depth,
			  CopyFromParent, CopyFromParent, 
			  CWBackingStore,
			  &set_attributes);
  wm_hints.flags = (InputHint|StateHint);
  wm_hints.input = TRUE;
  wm_hints.initial_state = NormalState;
  XSetWMHints (display, win_id, &wm_hints);
  
  wm_delete_window = XInternAtom (display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (display, win_id, &wm_delete_window, 1);

  if (size_hints) {
    XSetWMNormalHints (display, win_id, size_hints);
    geom_ret = XGetGeometry (display, win_id, &root_return,
     			     &x_return, &y_return,
     			     &width_return, &height_return,
     			     &border_width_return,
     			     &depth_return);
    size_hints -> x = x_return;
    size_hints -> y = y_return;
    size_hints -> base_width = width_return;
    size_hints -> base_height = height_return;
  }

  XSetWindowBorder (display, win_id, BlackPixel(display, screen));
  XSelectInput(display, win_id, wm_event_mask);

  gc = create_pane_win_gc (display, win_id, self_object);

  __xlib_set_wm_name_prop
    ((Display *)SYMVAL(IS_OBJECT(displayPtr_value -> instancevars) ?
		       displayPtr_value -> instancevars -> __o_value :
		       displayPtr_value -> __o_value),
     win_id, gc,
     ((title) ? title : basename_w_extent (__argvFileName())));
       
  __save_pane_to_vars (self_object, gc, win_id,
		       DefaultDepth (display, DefaultScreen (display)));
  return win_id;
}

int __ctalkCreateX11SubWindow (OBJECT *parent, OBJECT *self) {
  OBJECT *parent_object;
  /* Here, self is the subpane object */
  OBJECT *self_object, *self_origin_x_var, *self_origin_y_var,
    *self_size_x_var, *self_size_y_var;
  OBJECT *parent_win_id_value_object, *self_win_id_value_object,
    *win_depth_value_obj;
  OBJECT *self_gc_value_object;
  OBJECT *bgcolor_obj, *fgcolor_obj;
  OBJECT *self_displayPtr_value, *parent_displayPtr_value;
  Window parent_id, self_id;
  XSetWindowAttributes self_attributes;
  XGCValues gcv;
  GC gc;
  char buf[MAXLABEL];
  Display *p_display;
  
  self_object = IS_VALUE_INSTANCE_VAR(self) ? self -> __o_p_obj : self;
  parent_object = NULL;
  if (IS_OBJECT(parent -> instancevars)) {
    if (parent -> instancevars -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
      parent_object = *(OBJECT **)parent -> instancevars -> __o_value;
    }
  } else if (IS_VALUE_INSTANCE_VAR(parent)) {
    if (parent -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL) {
      parent_object = *(OBJECT **)parent -> __o_value;
    }
  }
  if (!IS_OBJECT(parent_object)) {
    parent_object = IS_VALUE_INSTANCE_VAR(parent) ? 
      parent -> __o_p_obj : parent;
  }
  parent_win_id_value_object =
    __x11_pane_win_id_value_object (parent_object);
  self_win_id_value_object =
    __x11_pane_win_id_value_object (self_object);
  self_gc_value_object = 
    __x11_pane_win_gc_value_object (self_object);
  win_depth_value_obj =
    __x11_pane_win_depth_value_object (self_object);
  self_displayPtr_value = __x11_pane_display_value_object (self_object);
  parent_displayPtr_value = __x11_pane_display_value_object (parent_object);
  p_display = (Display *)SYMVAL(parent_displayPtr_value -> __o_value);
  SYMVAL(self_displayPtr_value -> __o_value) = (uintptr_t)p_display;
  parent_id = (Window)*(int *)parent_win_id_value_object -> __o_value;
  self_origin_x_var = pane_pt_var (self_object, "origin", "x");
  self_origin_y_var = pane_pt_var (self_object, "origin", "y");
  self_size_x_var = pane_pt_var (self_object, "size", "x");
  self_size_y_var = pane_pt_var (self_object, "size", "y");
  if (!p_display) {
    _warning ("__ctalkX11CreateSubWindow: Could not open display, \"%s.\"\n",
	      getenv ("DISPLAY"));
    exit (EXIT_FAILURE);
  }
  if (parent_id == 0) {
    _warning ("__ctalkX11CreateSubWindow: Invalid parent window.\n");
    return ERROR;
  }
  self_attributes.backing_store = Always;
  self_id = XCreateWindow (p_display, parent_id, 
			   INTVAL(self_origin_x_var -> __o_value),
			   INTVAL(self_origin_y_var -> __o_value),
			   INTVAL(self_size_x_var -> __o_value),
			   INTVAL(self_size_y_var -> __o_value),
			   0, /* border_width */
			   CopyFromParent,
			   CopyFromParent, CopyFromParent, 
			   CWBackingStore, &self_attributes);

  XSelectInput(p_display, self_id, (WM_CONFIGURE_EVENTS|WM_INPUT_EVENTS));

  gc = create_pane_win_gc (p_display, self_id, self_object);

  __save_pane_to_vars (self_object, gc, self_id,
		       DefaultDepth (p_display, DefaultScreen (p_display)));
  
  *(int *)win_depth_value_obj -> __o_value =
    DefaultDepth (p_display, DefaultScreen (p_display));
  *(int *)win_depth_value_obj -> instancevars -> __o_value =
    DefaultDepth (p_display, DefaultScreen (p_display));
  return self_id;
}

int __ctalkMapX11Window (OBJECT *self_object) {
  unsigned int win_id;
  OBJECT *win_id_value_obj;
  win_id_value_obj =
    __x11_pane_win_id_value_object (self_object);
  win_id = *(int *)win_id_value_obj -> __o_value;
  XMapWindow (display, (Window)win_id);
  XMapSubwindows (display, (Window)win_id);
  return SUCCESS;
}

int __ctalkRaiseX11Window (OBJECT *self_object) {
  unsigned int win_id;
  OBJECT *win_id_value_obj;
  win_id_value_obj =
    __x11_pane_win_id_value_object (self_object);
  win_id = INTVAL(win_id_value_obj -> __o_value);
  XRaiseWindow (display, (Window)win_id);
  return SUCCESS; 
}

int __ctalkX11SetWMNameProp (OBJECT *self_object, char *title) {
  OBJECT *win_id_value, *gc_value, *displayptr_var;

  if (mem_id == 0) {
#ifndef WITHOUT_X11_WARNINGS
    printf ("__ctalkX11SetWMNameProp: No connection to display server.\n");
    printf ("__ctalkX11SetWMNameProp: (To disable these messages, build Ctalk with\n");
    printf ("__ctalkX11SetWMNameProp: the --without-x11-warnings option.)\n");
#endif
    return ERROR;
  }

  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr", TRUE);
  make_req (shm_mem,
	    SYMVAL(displayptr_var -> instancevars -> __o_value),
	    PANE_WM_TITLE_REQUEST,
	    INTVAL(win_id_value -> __o_value),
	    SYMVAL(gc_value -> __o_value), title);
  return SUCCESS;
}

#if 0
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
#endif
 
static int __x11_resize_request_internal (void *d, int width, int height, int depth,
					  int win_id_value,
					  GC gc_value) {

  char d_buf[MAXMSG], d_buf_2[MAXLABEL], d_buf_3[MAXLABEL];
  int r;
  Display *l_d;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif

  strcatx (d_buf, ascii[width], ":",
	   ascii[height], ":",
	   ascii[depth], NULL);

  if ((l_d = dialog_dpy ()) != NULL) {
    r = __xlib_resize_window (l_d, win_id_value, (GC)gc_value, d_buf);
  } else {

    make_req (shm_mem, d, PANE_RESIZE_REQUEST,
	      win_id_value, gc_value, d_buf);

#ifdef GRAPHICS_WRITE_SEND_EVENT
    send_event.xgraphicsexpose.type = GraphicsExpose;
    send_event.xgraphicsexpose.send_event = True;
    send_event.xgraphicsexpose.display = display;
    send_event.xgraphicsexpose.drawable = atoi(win_id_value);
    XSendEvent (display, atoi(win_id_value), False, 0L, &send_event);
#endif
    wait_req (shm_mem);
    errno = 0;
    r = strtol (&shm_mem[SHM_RETVAL], NULL, 10);
    if (errno != 0) {
      strtol_error (errno, "__x11_resize_request_internal",
		    &shm_mem[SHM_RETVAL]);
      return ERROR;
    }
  }
  return r;
}

/* cursor_id's are defined in x11defs.h and X11Cursor class */
/* It might be sometime necessary to include the Display * in
   the X11Cursor object, if (when) the dialogs also become 
   non-modal. */
int __ctalkX11FontCursor (OBJECT *self, int cursor_id) {
  char buf[255];
  Cursor cursor;
  Display *l_d;
  if (display == NULL) {
    __x11_open_display ();
    l_d = display;
  } else if ((l_d = dialog_dpy ()) == NULL) {
    l_d = display;
  }
  switch (cursor_id)
    {
    case CURSOR_GRAB_MOVE:
      cursor = XCreateFontCursor (l_d, XC_fleur);
      break;
    case CURSOR_SCROLL_ARROW:
      cursor = XCreateFontCursor (l_d, XC_sb_v_double_arrow);
      break;
    case CURSOR_WATCH:
      cursor = XCreateFontCursor (l_d, XC_watch);
      break;
    case CURSOR_ARROW:
      cursor = XCreateFontCursor (l_d, XC_arrow);
      break;
    case CURSOR_XTERM:
      cursor = XCreateFontCursor (l_d, XC_xterm);
      break;
    default:
      fprintf (stderr, "__ctalkX11FontCursor: unknown cursor %d.\n",
	       cursor_id);
      return ERROR;
      break;
    }
  
  INTVAL(self -> instancevars -> __o_value) = cursor;
  return SUCCESS;
}

/* 
   If cursor_object is NULL, use the default (normally the parent
   window's) cursor.
*/
int __ctalkX11UseCursor (OBJECT *pane_object, OBJECT *cursor_object) {
  OBJECT *win_id_value, *gc_value, *displayvar_value;
  Cursor cursor;
  char d_buf[MAXMSG];
  Display *l_d;
  win_id_value = __x11_pane_win_id_value_object (pane_object);
  gc_value = __x11_pane_win_gc_value_object (pane_object);
  displayvar_value = __ctalkGetInstanceVariable (pane_object, "displayPtr",
						 TRUE);
  l_d = (Display *)SYMVAL(displayvar_value -> instancevars -> __o_value);

  if (cursor_object) {
    cursor = INTVAL(cursor_object -> instancevars -> __o_value);
  } else {
    cursor = None;
  }
  sprintf (d_buf, "%lu", cursor);

  if (dialog_dpy ()) {
    __xlib_use_cursor (l_d, INTVAL(win_id_value -> __o_value),
		       (GC)SYMVAL(gc_value -> __o_value), d_buf);
  } else {
    make_req (shm_mem, l_d,
	      PANE_CURSOR_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);

    wait_req (shm_mem);
  }
  return SUCCESS;
}

int __ctalkX11ResizePixmap (void *d, int parent_visual, 
			    int old_pixmap, 
			    unsigned long int gc,
			    int old_width, int old_height,
			    int new_width, int new_height,
			    int depth, int *new_pixmap_return) {
  char d_buf[MAXMSG];
  char intbuf1[MAXLABEL];

  strcatx (d_buf,
	   ctitoa (old_pixmap, intbuf1), ":",
	   ascii[old_width], ":",
	   ascii[old_height], ":",
	   ascii[new_width], ":",
	   ascii[new_height], ":",
	   ascii[depth], ":", NULL);

  if (dialog_dpy ()) {
    __xlib_resize_pixmap (d, parent_visual, (GC)gc, d_buf);
    *new_pixmap_return = strtoul (d_buf, 0, 10);
  } else {
    make_req (shm_mem, d, PANE_RESIZE_PIXMAP_REQUEST, 
	      parent_visual, gc, d_buf);

    wait_req (shm_mem);
    errno = 0;
    *new_pixmap_return = strtoul (&shm_mem[SHM_RETVAL], NULL, 10);
    if (errno != 0) {
      strtol_error (errno, "__ctalkX11ResizePixmap", &shm_mem[SHM_RETVAL]);
      return ERROR;
    }
  }
  return SUCCESS;
}

bool edittext_resize_notify = false;

/*
 *  This fn will eventually be replaced by something that
 *  doesn't hard-code instance variables.
 */
int __ctalkX11ResizeWindow (OBJECT *self, int width, int height,
			    int depth) {
  OBJECT *self_object, *win_id_value, *gc_value;
  OBJECT *pane_size_point_object, *pane_size_point_y_object, 
    *pane_size_point_x_object, *displayPtr_var;
  int old_x_size, old_y_size, n_resize_retries;
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  displayPtr_var = __ctalkGetInstanceVariable (self_object, "displayPtr", TRUE);
  if ((pane_size_point_object =
       __ctalkGetInstanceVariable (self_object, "size", TRUE))
      == NULL)
    return ERROR;
  if ((pane_size_point_y_object =
       __ctalkGetInstanceVariable (pane_size_point_object, "y", TRUE))
      == NULL)
    return ERROR;
  if ((pane_size_point_x_object =
       __ctalkGetInstanceVariable (pane_size_point_object, "x", TRUE))
      == NULL)
    return ERROR;
  old_x_size = INTVAL(pane_size_point_x_object -> instancevars -> __o_value);
  old_y_size = INTVAL(pane_size_point_y_object -> instancevars -> __o_value);
  edittext_resize_notify = true;
  n_resize_retries = 0;
 try_resize_request:
  if (!__x11_resize_request_internal 
      ((void *)SYMVAL(displayPtr_var -> instancevars -> __o_value),
       width, height, depth,
       INTVAL(win_id_value -> __o_value),
       (GC)SYMVAL(gc_value -> __o_value))) {
    return 1;
  } else {
#ifndef WITHOUT_X11LIB_WARNINGS
    printf ("__ctalkX11ResizeWindow: Invalid resize_request return.\n");
    printf ("__ctalkX11ResizeWindow: (To disable these messages, build Ctalk with\n");
    printf ("__ctalkX11ResizeWindow: the --without-x11-warnings option.)\n");
#endif

    if (++n_resize_retries < MAX_RESIZE_RETRIES) {
      /*
       *  There's a possibility, if there are invalid resource
       *  id's  in the return buffers, that a valid Pixmap
       *  buffer can be lost, resulting in a memory 
       *  leak.  Mostly applies to OS X, which seems 
       *  to have trouble sometimes detaching shared
       *  memory correctly.  So we retry the resize anyway, 
       *  creating completely new buffers.  OS X, at least
       *  does not seem to require this function to generate
       *  a synthetic event to indicate that the window 
       *  must be redrawn completely.
       */
      goto try_resize_request;
    } else {
      return ERROR;
    }
  }
}

int __ctalkX11MoveWindow (OBJECT *self, int x, int y) {
  OBJECT *self_object, *win_id_value, *gc_value, *displayptr_var;
  char d_buf[MAXLABEL];
#ifdef GRAPHICS_WRITE_SEND_EVENT
  XEvent send_event;
#endif
  self_object = (IS_VALUE_INSTANCE_VAR (self) ? self->__o_p_obj : self);
  win_id_value = __x11_pane_win_id_value_object (self_object);
  gc_value = __x11_pane_win_gc_value_object (self_object);
  displayptr_var = __ctalkGetInstanceVariable (self_object, "displayPtr",
					       TRUE);

  strcatx (d_buf, ascii[x], ":", ascii[y], ":", NULL);

  if (dialog_dpy ()) {
    __xlib_move_window ((Display *)SYMVAL(displayptr_var -> instancevars 
					  -> __o_value),
			INTVAL(win_id_value -> __o_value),
			(GC)SYMVAL(gc_value -> __o_value),
			d_buf);
  } else {
    make_req (shm_mem,
	      SYMVAL(displayptr_var -> instancevars -> __o_value),
	      PANE_MOVE_REQUEST,
	      INTVAL(win_id_value -> __o_value),
	      SYMVAL(gc_value -> __o_value), d_buf);
  }

#ifdef GRAPHICS_WRITE_SEND_EVENT
  send_event.xgraphicsexpose.type = GraphicsExpose;
  send_event.xgraphicsexpose.send_event = True;
  send_event.xgraphicsexpose.display = display;
  send_event.xgraphicsexpose.drawable = atoi(win_id_value->__o_value);
  XSendEvent (display, atoi(win_id_value->__o_value),
 	      False, 0L, &send_event);
#endif
  wait_req (shm_mem);
  return SUCCESS;
}

/*
 *  The I/O error handler lets us shutdown correctly if 
 *  WM_DELETE_WINDOW happens to kill the app (the parent
 *  process of __ctalkX11InputClient) when it deletes the 
 *  window.
 */
int __ctalkX11IOErrorHandler (Display *d) {
  int pending;

#if X11LIB_PRINT_COPIOUS_DEBUG_INFO
  fprintf (stderr, "Ctalk x11lib.c. PID %d\n", (int)getpid ());
  fprintf (stderr, "Display = %p. Pending = %d\n", d, pending);
#endif
  if (d) {
    while ((pending = XPending (d)) > 0)
      XSync (d, TRUE);
    XCloseDisplay (d);
  }
  (*default_io_handler)(d);
  __ctalkErrorExit ();
#if X11LIB_PRINT_COPIOUS_DEBUG_INFO
  fprintf (stderr, "Display = %p. Pending = %d\n", d, pending);
  (*default_io_handler)(d);
#endif
  return SUCCESS;
}

int __ctalkX11ErrorHandler (Display *d, XErrorEvent *e) {
#ifndef WITHOUT_X_PROTOCOL_ERRORS  
  char error_text[MAXMSG];
  printf ("__ctalkX11ErrorHandler: X11 Protocol Error %d\n", 
	  (int)e -> error_code);
  XGetErrorText (d, (int)e -> error_code, error_text, MAXMSG);
  printf ("__ctalkX11ErrorHandler: %s\n", error_text);
  printf ("__ctalkX11ErrorHandler: (To disable these messages, use the\n");
  printf ("__ctalkX11ErrorHandler: --without-x-protocol-errors option when building Ctalk.)\n");
#endif /* WITHOUT_X_PROTOCOL_ERRORS */
  return SUCCESS;
}

int __ctalkX11DisplayWidth (void) {
  int width;
  char *d_env;
  Display *local_display;
  if (display == NULL) {
    d_env = getenv ("DISPLAY");
    if ((local_display = XOpenDisplay (d_env)) != NULL) {
      width = DisplayWidth (local_display, DefaultScreen (local_display));
      XCloseDisplay (local_display);
    } else {
      width = -1;
    }
  } else {
    width = display_width;
  }
  return width;
}

int __ctalkX11DisplayHeight (void) {
  int height;
  char *d_env;
  Display *local_display;
  if (display == NULL) {
    d_env = getenv ("DISPLAY");
    if ((local_display = XOpenDisplay (d_env)) != NULL) {
      height = DisplayHeight (local_display, DefaultScreen (local_display));
      XCloseDisplay (local_display);
    } else {
      height = -1;
    }
  } else {
    height = display_height;
  }
  return height;
}

#endif /* !X11LIB_FRAME */

#else /* #if defined (DJGPP) || defined (WITHOUT_X11) */

void x_support_error () {
  _error ("To run graphics programs, you must configure and install "
	  "Ctalk with X Window System support. "
	  "Type \"./configure --help\" in the "
	  "top-level source directory for a list of "
	  "configuration options.\n");
}

int __ctalkX11InputClient (OBJECT *streamobject, int parent_fd, int mem_handle, int main_win_id) {
  x_support_error (); return ERROR;
}
int __ctalkCreateX11MainWindow (OBJECT *self_object) {
  x_support_error (); return ERROR;
}
int __ctalkCreateX11SubWindow (OBJECT *parent, OBJECT *self) {
  x_support_error (); return ERROR;
}
int __ctalkMapX11Window (OBJECT *self_object) {
  x_support_error (); return ERROR;
}
int __ctalkRaiseX11Window (OBJECT *self_object) {
  x_support_error(); return ERROR;
}
int __ctalkX11SetWMNameProp (OBJECT *self_object, char *title) {
  x_support_error(); return ERROR;
}
int __ctalkX11ResizeWindow (OBJECT *self, int width, int height) {
  x_support_error(); return ERROR;
}
int __ctalkX11MoveWindow (OBJECT *self, int x, int y) {
  x_support_error(); return ERROR;
}
int __ctalkX11DisplayHeight (void) {
  x_support_error (); return ERROR;
}
int __ctalkX11DisplayWidth (void) {
  x_support_error (); return ERROR;
}
int __ctalkX11UseCursor (OBJECT *pane_object, OBJECT *cursor_object) {
  x_support_error (); return ERROR;
}
int __ctalkX11FontCursor (OBJECT *self, int cursor_id) {
  x_support_error (); return ERROR;
}
int __ctalkX11CloseParentPane (OBJECT *self) {
  x_support_error (); return ERROR;
}
int __ctalkCloseX11Pane (OBJECT *self) {
  x_support_error (); return ERROR;
}
int __ctalkCreateX11MainWindowTitle (OBJECT *self_object, char *title) {
  x_support_error (); return ERROR;
}
int __ctalkX11Colormap (void) {
  x_support_error (); return ERROR;
}
void *__ctalkX11Display (void) {
  x_support_error (); return NULL;
}
int __ctalkX11ResizePixmap (void *d, int parent_visual, 
			    int old_pixmap, 
			    unsigned long int gc,
			    int old_width, int old_height,
			    int new_width, int new_height,
			    int depth, int *new_pixmap_return) {
  x_support_error (); return ERROR;
}
int __ctalkOpenX11InputClient (OBJECT *streamobject) {
  x_support_error (); return ERROR;
}

int read_event (int *ev_type_out, unsigned int *win_out, unsigned int data[],
		int event_mask) {
  x_support_error (); return ERROR;
}

#endif  /* #if defined (DJGPP) || defined (WITHOUT_X11) */
