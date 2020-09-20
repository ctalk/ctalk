/* $Id: glxlib.c,v 1.1.1.1 2020/07/17 07:41:39 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2017-2020 Robert Kiesling, rk3314042@gmail.com.
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
#include <time.h>
#include <object.h>
#include <message.h>
#include <ctalk.h>

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)

#ifdef HAVE_GLX_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#if HAVE_XFT_H
#include <fontconfig/fontconfig.h>
#include <X11/Xft/Xft.h>
#include FT_FREETYPE_H
#include "xftfont.h"
#endif
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/glu.h>
#include <x11defs.h>


#if HAVE_GLWINDOWPOS
extern void glWindowPos2i (int, int);
#endif

extern Display *display;  /* defined in x11lib.c */
Window g_win_id;
extern char *shm_mem;
extern int mem_id;
extern void __save_pane_to_vars (OBJECT *, GC, int, int);
extern int __x11_pane_border_width (OBJECT *);
extern Atom wm_delete_window;


/* Declared in xgeometry.c */
extern XSizeHints *size_hints;
extern int __geomFlags;

static int win_x_org, win_y_org;
static int win_height, win_width;
static int display_width_px, display_height_px;
static bool fullscreen = false;
/* Saved dimensions when the window is full screen. */
static int save_win_x_org, save_win_y_org;
static int save_win_width, save_win_height;

/* Set some reasonable defaults */
/* static GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None}; */

static int att[64] = {0,};

static void __glxlib_visual_attributes (OBJECT *pane_object) {
  OBJECT *visualDepthSize_instance_var,
    *visualDoubleBuffer_instance_var,
    *visualRGBA_instance_var,
    *visualBufferSize_instance_var,
    *visualStereo_instance_var,
    *visualAuxBuffers_instance_var,
    *visualRedSize_instance_var,
    *visualGreenSize_instance_var,
    *visualBlueSize_instance_var,
    *visualAlphaSize_instance_var,
    *visualStencilPlanes_instance_var,
    *visualRedAccumSize_instance_var,
    *visualGreenAccumSize_instance_var,
    *visualBlueAccumSize_instance_var,
    *visualAlphaAccumSize_instance_var;
  int att_ptr = 0;
  int int_val;
  bool bool_val;
  visualDepthSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualDepthSize", TRUE);
  visualRGBA_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualRGBA", TRUE);
  visualDoubleBuffer_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualDoubleBuffer", TRUE);
  visualBufferSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualBufferSize", TRUE);
  visualStereo_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualStereo", TRUE);
  visualAuxBuffers_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualAuxBuffers", TRUE);
  visualRedSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualRedSize", TRUE);
  visualGreenSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualGreenSize", TRUE);
  visualBlueSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualBlueSize", TRUE);
  visualAlphaSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualAlphaSize", TRUE);
  visualStencilPlanes_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualStencilPlanes", TRUE);
  visualRedAccumSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualRedAccumSize", TRUE);
  visualGreenAccumSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualGreenAccumSize", TRUE);
  visualBlueAccumSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualBlueAccumSize", TRUE);
  visualAlphaAccumSize_instance_var =
    __ctalkGetInstanceVariable (pane_object, "visualAlphaAccumSize", TRUE);

  /* GLX_DOUBLEBUFFER <boolean> */
  bool_val = (bool)INTVAL(visualDoubleBuffer_instance_var->__o_value);
  if (bool_val) {
    att[att_ptr++] = GLX_DOUBLEBUFFER;
  }
  /* GLX_RGBA <boolean> */
  bool_val = (bool)INTVAL(visualRGBA_instance_var -> __o_value); 
  if (bool_val) {
    att[att_ptr++] = GLX_RGBA;
  }
  /* GLX_DEPTH_SIZE, <int_val> */
  int_val = INTVAL (visualDepthSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_DEPTH_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_BUFFER_SIZE, <int_val> */
  int_val = INTVAL(visualBufferSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_BUFFER_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_STEREO <boolean> */
  bool_val = (bool)INTVAL(visualStereo_instance_var -> __o_value);
  if (bool_val) {
    att[att_ptr++] = GLX_STEREO;
  }
  /* GLX_AUX_BUFFERS, <int_val> */
  int_val = INTVAL (visualAuxBuffers_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_AUX_BUFFERS;
    att[att_ptr++] = int_val;
  }
  /* GLX_RED_SIZE, <int_val> */
  int_val = INTVAL(visualRedSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_RED_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_GREEN_SIZE, <int_val> */
  int_val = INTVAL(visualGreenSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_GREEN_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_BLUE_SIZE, <int_val> */
  int_val = INTVAL(visualBlueSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_BLUE_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_ALPHA_SIZE, <int_val> */
  int_val = INTVAL(visualAlphaSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_ALPHA_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_STENCIL_SIZE, <int_val> */
  int_val = INTVAL(visualStencilPlanes_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_STENCIL_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_ACCUM_RED_SIZE, <int_val> */
  int_val = INTVAL(visualRedAccumSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_ACCUM_RED_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_ACCUM_GREEN_SIZE, <int_val> */
  int_val = INTVAL(visualGreenAccumSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_ACCUM_GREEN_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_ACCUM_BLUE_SIZE, <int_val> */
  int_val = INTVAL(visualBlueAccumSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_ACCUM_BLUE_SIZE;
    att[att_ptr++] = int_val;
  }
  /* GLX_ACCUM_ALPHA_SIZE, <int_val> */
  int_val = INTVAL(visualAlphaAccumSize_instance_var -> __o_value);
  if (int_val > 0) {
    att[att_ptr++] = GLX_ACCUM_ALPHA_SIZE;
    att[att_ptr++] = int_val;
  }
  att[att_ptr] = None;
}

void __glx_resize (int new_width, int new_height) {
  win_width = new_width, win_height = new_height;
}

void __glx_get_win_config (int *x_org_out, int *y_org_out,
			   int *width_out, int *height_out) {
  *x_org_out = win_x_org;
  *y_org_out = win_y_org;
  *width_out = win_width;
  *height_out = win_height;
}

int __ctalkCreateGLXMainWindow (OBJECT *self_object) {
  XVisualInfo *vi;
  Colormap cmap;
  XSetWindowAttributes swa;
  Window win_id, root_return;
  OBJECT *visualinfo_instance_var, *displayptr_instance_var,
    *colormap_instance_var;
  char buf[MAXLABEL];
  char intbuf[0xff];
  int border_width;
  int geom_ret, x_return, y_return;
  unsigned int width_return, height_return,
    border_width_return, depth_return;

  __glxlib_visual_attributes (self_object);

  if ((vi = glXChooseVisual (display, 0, att)) == NULL) {
    printf ("GLXChooseVisual didn't work\n");
    exit (1);
  }

  cmap = XCreateColormap (display, DefaultRootWindow (display),
			  vi -> visual, AllocNone);
  swa.colormap = cmap;
  swa.event_mask = WM_CONFIGURE_EVENTS|WM_INPUT_EVENTS;

  border_width = __x11_pane_border_width (self_object);
  set_size_hints_internal (self_object, &win_x_org, &win_y_org,
			   &win_width, &win_height);

  win_id = g_win_id =
    XCreateWindow (display, DefaultRootWindow(display), 
		   win_x_org, win_y_org, win_width, win_height,
		   border_width, vi -> depth,
		   InputOutput, vi -> visual, 
		   CWColormap|CWEventMask, &swa);

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

  wm_delete_window = XInternAtom (display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (display, win_id, &wm_delete_window, 1);

  XStoreName (display, win_id, basename_w_extent (__argvFileName ()));

  __save_pane_to_vars (self_object, NULL, win_id,
		       DefaultDepth (display, DefaultScreen (display)));
  visualinfo_instance_var =
    __ctalkGetInstanceVariable (self_object, "visualInfoPtr", TRUE);
  visualinfo_instance_var -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
  *(unsigned long *)visualinfo_instance_var -> __o_value =
    *(unsigned long *)visualinfo_instance_var -> instancevars -> __o_value
    = (uintptr_t)vi;
  displayptr_instance_var =
    __ctalkGetInstanceVariable (self_object, "displayPtr", TRUE);
  displayptr_instance_var -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
  *(unsigned long *)displayptr_instance_var -> __o_value =
    *(unsigned long *)displayptr_instance_var -> instancevars -> __o_value
    = (uintptr_t)display;
  colormap_instance_var =
    __ctalkGetInstanceVariable (self_object, "colormap", TRUE);
  SETINTVARS(colormap_instance_var,
	     DefaultColormap (display, DefaultScreen (display)));
  display_width_px = DisplayWidth (display, DefaultScreen (display));
  display_height_px = DisplayHeight (display, DefaultScreen (display));
  return win_id;
}

int __ctalkMapGLXWindow (OBJECT *self_object) {
  XVisualInfo *vi;
  GLXContext  glc;
  int win_id;
  OBJECT *visualinfo_instance_var, *winid_instance_var,
    *glxcontext_instance_var;

  winid_instance_var =
    __ctalkGetInstanceVariable (self_object, "xWindowID", TRUE);
  if ((win_id = INTVAL(winid_instance_var -> __o_value)) == 0) {
    printf ("__ctalkMapGLXWindow: bad xWindowID value.\n");
    exit (1);
  }

  XMapWindow (display, win_id);

  visualinfo_instance_var =
    __ctalkGetInstanceVariable (self_object, "visualInfoPtr", TRUE);

  if ((vi = (XVisualInfo *)SYMVAL(visualinfo_instance_var -> __o_value))
	== NULL) {
    printf ("__ctalkMapGLXWindow: bad visualInfoPtr value.\n");
    exit (1);
  }
  glc = glXCreateContext (display, vi, NULL, GL_TRUE);
  glXMakeCurrent (display, win_id, glc);
  glxcontext_instance_var =
    __ctalkGetInstanceVariable (self_object, "glxContextPtr", TRUE);
  __ctalkObjValPtr (visualinfo_instance_var, (void *)glc);

  return SUCCESS;
}

int __ctalkCloseGLXPane (OBJECT *self_object) {
  OBJECT *win_id_value_var, *glxcontext_instance_var,
    *xvisualinfo_instance_var;
  Window w;
  GLXContext  glc;
  XVisualInfo *vi;
  int retval = SUCCESS;

  glXMakeCurrent (display, None, NULL);
  glxcontext_instance_var =
    __ctalkGetInstanceVariable (self_object, "glxContextPtr", TRUE);
  if (IS_OBJECT(glxcontext_instance_var)) {
    if ((glc = (GLXContext)generic_ptr_str
	 (glxcontext_instance_var->__o_value)) != NULL) {
      glXDestroyContext (display, glc);
    } 
    glxcontext_instance_var -> __o_value[0] = '\0';
    if (IS_OBJECT(glxcontext_instance_var -> instancevars))
      glxcontext_instance_var -> instancevars -> __o_value[0] = '\0';
  } else {
    retval = -1;
  }
  xvisualinfo_instance_var = 
    __ctalkGetInstanceVariable (self_object, "visualInfoPtr", TRUE);
  if (IS_OBJECT(glxcontext_instance_var)) {
    if ((vi = (XVisualInfo *)generic_ptr_str
	 (xvisualinfo_instance_var->__o_value)) != NULL) {
      XFree (vi);
    } 
    glxcontext_instance_var -> __o_value[0] = '\0';
    if (IS_OBJECT(glxcontext_instance_var -> instancevars))
      glxcontext_instance_var -> instancevars -> __o_value[0] = '\0';
  } else {
    retval = -1;
  }
  
  if ((win_id_value_var = __x11_pane_win_id_value_object (self_object))
      != NULL) {
    if ((w = INTVAL(win_id_value_var -> __o_value)) != (Window)0)
      XDestroyWindow (display, w);
    else
      retval = -1;
  } else {
    retval = -1;
  }

  return retval;
}

int __ctalkGLXSwapBuffers (OBJECT *self_object) {
  Window w;
  OBJECT *win_id_value_var;
  int retval = SUCCESS;

  if ((win_id_value_var = __x11_pane_win_id_value_object (self_object))
      != NULL) {
    if ((w = INTVAL(win_id_value_var -> __o_value)) != (Window)0) 
      glXSwapBuffers (display, w);
    else
      retval = -1;
  } else {
    retval = -1;
  }
  return retval;
}

static XFontStruct *glx_xfs = NULL;
static unsigned int glx_fontbase = 0;

int __ctalkGLXUseXFont (OBJECT *glx_pane_object, char *fontname) {
  unsigned int first, last;
  OBJECT * xLineHeight_instance_var,
    *xMaxCharWidth_instance_var;

  glx_xfs = XLoadQueryFont (display, fontname);
  if (glx_xfs == NULL) {
    printf ("ctalk: Font %s not found.\n", fontname);
    return ERROR;
  }
  first = glx_xfs -> min_char_or_byte2;
  last = glx_xfs -> max_char_or_byte2;
  glx_fontbase = glGenLists ((unsigned int) last + 1);
  if (glx_fontbase == 0) {
    printf ("ctalk: Unable to allocate font %s.\n", fontname);
    if (glx_xfs) {
      XFreeFont (display, glx_xfs);
      glx_xfs = NULL;
    }
    return ERROR;
  }
  glXUseXFont (glx_xfs -> fid, first, last - first + 1,
	       glx_fontbase + first);

  if ((xLineHeight_instance_var =
       __ctalkGetInstanceVariable (glx_pane_object, "xLineHeight", FALSE))
      != NULL) {
    SETINTVARS(xLineHeight_instance_var, (glx_xfs -> max_bounds.ascent +
					  glx_xfs -> max_bounds.descent));
  }
  if ((xMaxCharWidth_instance_var =
       __ctalkGetInstanceVariable (glx_pane_object,
				   "xMaxCharWidth", FALSE)) != NULL) {
    SETINTVARS(xMaxCharWidth_instance_var, glx_xfs -> max_bounds.width);
  }
  return SUCCESS;
}

int __ctalkGLXTextWidth (char *text) {
  if (glx_xfs) {
    return XTextWidth (glx_xfs, text, strlen (text));
  } else {
    return ERROR;
  }
}

int __ctalkGLXFreeXFont (void) {
  if (glx_xfs) {
    XFreeFont (display, glx_xfs);
    glx_xfs = NULL;
    glx_fontbase = 0;
  }
  return SUCCESS;
}

int __ctalkGLXDrawText (char *text) {
  glListBase (glx_fontbase);
  glCallLists (strlen (text), GL_UNSIGNED_BYTE, (unsigned char *)text);
  return SUCCESS;
}

#if HAVE_GLWINDOWPOS
/* glWindowPos2i is actually a MESA extension, so ./configure checks for
   its presence when building the libraries */
int __ctalkGLXWindowPos2i (int x, int y) {
    glWindowPos2i (x, y);
}
#else
int __ctalkGLXWindowPos2i (int x, int y) {
  printf ("ctalk: Your GL implementation does not support the glWindowPos2i "
	  "function.  Exiting.\n");
  exit (EXIT_FAILURE);
}
#endif

#define HEXASCIITOBINARY(d) (					\
  (d >= '0' && d <= '9') ? d - 48 :				\
  ((d >= 'A' && d <= 'F') ? d - 55 :				\
   ((d >= 'a' && d <= 'f') ? d - 87 : 0)))

/* This is identical to __ctalkXPMToGLTexture in glutlib.c,
   it just uses a different name.  It's also generic enough
   that it should work with any OpenGL toolkit. */
void __ctalkXPMToGLXTexture (char **xpm_data, unsigned int alpha,
			    int *width_out, int *height_out,
			    void **texel_data_out) {
  int r;
  char *p, *q;
  int width, height, n_colors, chars_per_pixel;
  int x, y, x_out, y_out, nth_color;
  bool have_color;
  int user_x_org = 0, user_y_org = 0;
  int color_1_row, color_n_row;
  char old_color_pixels[4] = "";
  unsigned pix;
  int idx;
  int rrggbb;
  unsigned short rr, gg, bb;
  int rrr, ggg, bbb;
  int rrrr, gggg, bbbb;

  if ((r = sscanf (xpm_data[0], "%d %d %d %d", 
		   &width, &height, &n_colors, &chars_per_pixel))
      != 4) {
    printf ("__ctalkXPMToGLXTexture: %s: parse error.\n", xpm_data[0]);
    goto xpm_from_pixmap_done;
  }
  *width_out = width;
  *height_out = height;

  *texel_data_out = malloc (width * height * sizeof (int));

  if (*texel_data_out == NULL)
    return;

  color_1_row = 1;
  color_n_row = n_colors;

  for (y_out = 0, y = color_n_row + 1; y <= (color_n_row + height);
       ++y_out, y++) {
    for (x = 0, x_out = 0; x_out < width; ++x_out, x += chars_per_pixel) {
      if (!strncmp (&xpm_data[y][x], old_color_pixels, chars_per_pixel)) {
   	// Re-use a color if it's the same as the previous pixel.

	idx = ((x_out + user_x_org) * width) + 
	(y_out + user_y_org);
	pix = (rrggbb << 8) + 255;
	((int *)*texel_data_out)[idx] = pix;

      } else {
   	for (nth_color = color_1_row, have_color = false;
   	     (nth_color <= color_n_row) && !have_color; nth_color++) {
   	  if (!strncmp (&xpm_data[y][x], xpm_data[nth_color], chars_per_pixel)) {
   	    char *p, *q;
   	    if ((p = strchr ((char *)xpm_data[nth_color], 'c')) != NULL) {
   	      if ((q = strchr (p, '#')) != NULL) {

		if (strlen (q) == 7) {
#ifndef __APPLE__
		  rrggbb = strtoul (q + 1, NULL, 16);
#else		  
		  rr = ((HEXASCIITOBINARY(*(q + 1)) << 4) +
			(HEXASCIITOBINARY(*(q + 2))));
		  gg = ((HEXASCIITOBINARY(*(q + 3)) << 4) +
			(HEXASCIITOBINARY(*(q + 4))));
		  bb = ((HEXASCIITOBINARY(*(q + 5)) << 4) +
			(HEXASCIITOBINARY(*(q + 6))));
		  rrggbb = (bb << 16) + (gg << 8) + rr;
#endif		  
		} else if (strlen (q) == 10) {
		  /* 32-bit color - see the comment below. 
		     I don't have any live examples of this yet, so
		     it's untested. */
		  rrr = ((HEXASCIITOBINARY(*(q + 1)) << 6) +
			 (HEXASCIITOBINARY(*(q + 2)) << 4) +
			 (HEXASCIITOBINARY(*(q + 3))));
		  ggg = ((HEXASCIITOBINARY(*(q + 4)) << 6) +
			 (HEXASCIITOBINARY(*(q + 5)) << 4) +
			 (HEXASCIITOBINARY(*(q + 6))));
		  bbb = ((HEXASCIITOBINARY(*(q + 7)) << 6) +
			 (HEXASCIITOBINARY(*(q + 8)) << 4) +
			 (HEXASCIITOBINARY(*(q + 9))));
		  rrggbb = ((rrr >> 2) << 16) + ((ggg >> 2) << 8) +
		     (bbb >> 2);
		} else if (strlen (q) == 13) {
		  /* If we have a 32- or 48-bit color, translate it 
		     into a 24-bit color so it (plus the alpha channel)
		     fits a 32-bit texel alignment evenly. 
		     With no sampling (which is probably a good idea if
		     we don't know how the color specifications were
		     generated), this seems to compress the
		     range of tones and makes the texture lighter,
		     To reproduce the exact colors in the texture,
		     then a xpm with 3 chars per pixel can accomodate
		     the full  #rrggbb color space, and fits within
		     OpenGL's limit of 10 bits per hue. */
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
		  rrggbb = ((rrrr >> 4) << 16) + 
		    ((gggg >> 4) << 8) + (bbbb >> 4);
		}
		have_color = true;
#ifndef __APPLE__
		pix = (rrggbb << 8) + alpha;
#else		
		pix = (alpha << 24) + rrggbb;
#endif		
		idx = ((x_out + user_x_org) * width) + 
		  (y_out + user_y_org);
		((int *)*texel_data_out)[idx] = pix;

		switch (chars_per_pixel)
		  {
		  case 1:
		    old_color_pixels[0] = xpm_data[y][x];
		    old_color_pixels[1] = '\0';
		    break;
		  case 2:
		    old_color_pixels[0] = xpm_data[y][x];
		    old_color_pixels[1] = xpm_data[y][x+1];
		    old_color_pixels[2] = '\0';
		    break;
		  case 3:
		    old_color_pixels[0] = xpm_data[y][x];
		    old_color_pixels[1] = xpm_data[y][x+1];
		    old_color_pixels[2] = xpm_data[y][x+2];
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

  return;
}

int __ctalkGLXWinXOrg (void) { return win_x_org; }
int __ctalkGLXWinYOrg (void) { return win_y_org; }
int __ctalkGLXWinXSize (void) { return win_width; }
int __ctalkGLXWinYSize (void) { return win_height; }

void __ctalkGLXNamedColor (char *colorname, float *red_out,
			   float *green_out, float *blue_out) {
  XColor screen_color, exact_color;
  float factor = (float)10000 / (float)65535;
  if (XAllocNamedColor (display,
			DefaultColormap (display,
					 DefaultScreen (display)),
			colorname,
			&screen_color, &exact_color)) {
    *red_out = ((float)exact_color.red / 10000.0) * factor;
    *green_out = ((float)exact_color.green / 10000.0) * factor;
    *blue_out = ((float)exact_color.blue / 10000.0) * factor;
  }
}

char g_win_name[8192];

void __ctalkGLXFullScreen (OBJECT *self_object) {
  XEvent e;
  if (fullscreen == false) {
    fullscreen = true;
    save_win_x_org = win_x_org, save_win_y_org = win_y_org,
      save_win_width = win_width, save_win_height = win_height;
    win_x_org = 0, win_y_org = 0,
      win_width = display_width_px,
      win_height = display_height_px;
  } else {
    fullscreen = false;
    win_x_org = save_win_x_org, win_y_org = save_win_y_org,
      win_width = save_win_width, win_height = save_win_height;
  }
  XMoveResizeWindow (display, g_win_id, win_x_org, win_y_org,
		     win_width, win_height);
  e.type = ConfigureNotify;
  e.xconfigure.send_event = true;
  e.xconfigure.x = win_x_org;
  e.xconfigure.y = win_y_org;
  e.xconfigure.width = win_width;
  e.xconfigure.height = win_width;
  e.xconfigure.window = g_win_id;
  XSendEvent (display, g_win_id, True, StructureNotifyMask, &e);

}

static bool tmp_display_open = false;
static GLXContext tmp_glc;
XVisualInfo *tmp_vi;
static int tmp_att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, None};

static Window open_tmp_glx_context (void) {
  Colormap cmap;
  XSetWindowAttributes swa;
  Window win_id;

  if (display == NULL) {
    display = XOpenDisplay (NULL);
    tmp_display_open = true;
  }

  if ((tmp_vi = glXChooseVisual (display, 0, tmp_att)) == NULL) {
    printf ("GLXChooseVisual didn't work\n");
    exit (1);
  }
  cmap = XCreateColormap (display, DefaultRootWindow (display),
			  tmp_vi -> visual, AllocNone);
  swa.colormap = cmap;
  swa.event_mask = WM_CONFIGURE_EVENTS|WM_INPUT_EVENTS;

  win_id = 
    XCreateWindow (display, DefaultRootWindow(display), 
		   1, 1, 1, 1,
		   1, tmp_vi -> depth,
		   InputOutput, tmp_vi -> visual, 
		   CWColormap|CWEventMask, &swa);

  tmp_glc = glXCreateContext (display, tmp_vi, NULL, GL_TRUE);
  glXMakeCurrent (display, win_id, tmp_glc);

  return win_id;
  
}

static void delete_tmp_glx_context (Window win_id) {
  glXDestroyContext (display, tmp_glc);
  tmp_glc = NULL;
  XFree (tmp_vi);
  tmp_vi = NULL;
  XDestroyWindow (display, win_id);
  if (tmp_display_open) {
    XCloseDisplay (display);
    tmp_display_open = false;
  }
}

char *__ctalkGLXExtensionsString (void) {
  Window tmp_win_id;
  char *ext_str;
  tmp_win_id = open_tmp_glx_context ();
  ext_str = (char *)glXQueryExtensionsString
		     (display, DefaultScreen(display));
  delete_tmp_glx_context (tmp_win_id);
  return ext_str;
}

bool __ctalkGLXExtensionSupported (char *extname) {
  Window tmp_win_id;
  bool have_ext;
  tmp_win_id = open_tmp_glx_context ();
  have_ext = (bool)strstr (glXQueryExtensionsString
			   (display, DefaultScreen (display)),
			   extname);
  delete_tmp_glx_context (tmp_win_id);
  return have_ext;
}

/* The next three functions are adapted from glxswapcontrol.c, in
   the MESA Demos package. */

static bool __glx_oml_sync_control_not_supported_displayed = false;

float __ctalkGLXRefreshRate (void) {
  if (__ctalkGLXExtensionSupported ("GLX_OML_sync_control")) {
   PFNGLXGETMSCRATEOMLPROC  get_msc_rate_proc;
   int n, d;
   get_msc_rate_proc = (PFNGLXGETMSCRATEOMLPROC)
     glXGetProcAddressARB( (const GLubyte *) "glXGetMscRateOML" );
   if ( get_msc_rate_proc != NULL ) {
     (*get_msc_rate_proc)( display, glXGetCurrentDrawable(), &n, &d );
     return (float) n / d;
   }
  } else {
    if (!__glx_oml_sync_control_not_supported_displayed) {
      printf ("ctalk: glXGetMscRateOML is not supported.  "
	      "Cannot display the screen refresh rate.\n");
      __glx_oml_sync_control_not_supported_displayed = true;
    }
    return -1.0f;
  }
}

static bool swap_control_not_supported_displayed = false;
int __ctalkGLXSwapControl (int interval) {
  if (__ctalkGLXExtensionSupported ("GLX_MESA_swap_control")) {
    PFNGLXSWAPINTERVALMESAPROC set_swap_interval = NULL;
    set_swap_interval = (PFNGLXSWAPINTERVALMESAPROC)
      glXGetProcAddressARB( (const GLubyte *) "glXSwapIntervalMESA" );
    if (set_swap_interval != NULL) {
      if (interval != 0) {
	(*set_swap_interval) (interval);
      } else {
	(*set_swap_interval) (0);
      }
      return SUCCESS;
    }
  } else {
    if (!swap_control_not_supported_displayed) {
      printf ("ctalk: The GLX_MESA_swap_control extension is not supported.  "
	      "Not setting swap sync.\n");
      swap_control_not_supported_displayed = true;
    }
  }
  return ERROR;
}

static int t_start = -1;
static int frames = 0;
static float fps = 0.0f;

float __ctalkGLXFrameRate (void) {
  int t, seconds;

  t = time (NULL);

  if (t_start < 0)
    t_start = t;

  ++frames;

  if (t - t_start >= 5) {
    seconds = t - t_start;
    fps = frames / seconds;
    t_start = t;
    frames = 0;
  }
  return fps;
}

#else /* HAVE_GLX_H */

static void glx_support_error (void) {
  printf ("This version of Ctalk does not include GLX support. "
	  "Consult the file README in the top Ctalk source directory "
	  "for more information.\n");
}

int __ctalkCreateGLXMainWindow (OBJECT *self_object) {
  glx_support_error ();
  exit (1);
}
int __ctalkMapGLXWindow (OBJECT *self_object) {
  glx_support_error ();
  exit (1);
}
int __ctalkCloseGLXPane (OBJECT *self) {
  glx_support_error ();
  exit (1);
}
int __ctalkGLXSwapBuffers (OBJECT *self_object) {
  glx_support_error ();
  exit (1);
}

int __ctalkGLXUseXFont (OBJECT *glx_pane_object, char *fontname) {
  glx_support_error ();
  exit (1);
}

int __ctalkGLXFreeXFont (void) {
  glx_support_error ();
  exit (1);
}
int __ctalkGLXDrawText (char *text) {
  glx_support_error ();
  exit (1);
}
int __ctalkGLXWindowPos2i (int x, int y) {
  glx_support_error ();
  exit (1);
}
int __ctalkGLXTextWidth (char *text) {
  glx_support_error ();
  exit (1);
}
void __ctalkXPMToGLXTexture (char **xpm_data, unsigned int alpha,
			    int *width_out, int *height_out,
			    void **texel_data_out) {
  glx_support_error ();
  exit (1);
}

int __ctalkGLXWinXOrg (void) { 
  glx_support_error ();
  exit (1);
}
int __ctalkGLXWinYOrg (void) {
  glx_support_error ();
  exit (1);
}
int __ctalkGLXWinXSize (void) {
  glx_support_error ();
  exit (1);
}
int __ctalkGLXWinYSize (void) {
  glx_support_error ();
  exit (1);
}

void __ctalkGLXFullScreen (OBJECT *self_object) {
  glx_support_error ();
  exit (1);
}
void __glx_resize (int new_width, int new_height) {
  glx_support_error ();
  exit (1);
}

void __glx_get_win_config (int *x_org_out, int *y_org_out,
			   int *width_out, int *height_out) {
  glx_support_error ();
  exit (1);
}

char *__ctalkGLXExtensionsString (void) {
  glx_support_error ();
  exit (1);
}

bool __ctalkGLXExtensionSupported (char *extname) {
  glx_support_error ();
  exit (1);
}

float __ctalkGLXRefreshRate (void) {
  glx_support_error ();
  exit (1);
}

void __ctalkGLXNamedColor (char *colorname, float *red_out,
			     float *green_out, float *blue_out) {
  glx_support_error ();
  exit (1);
}

#endif /* HAVE_GLX_H */

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */ 
