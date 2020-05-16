/* $Id: glutlib.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2015  Robert Kiesling, rk3314042@gmail.com.
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

#define SUCCESS 0
#define ERROR -1

#ifdef HAVE_GLUT_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#ifdef __APPLE__
#include <stdlib.h>
#include <X11/Xlib.h>
#include "/System/Library/Frameworks/GLUT.framework/Headers/glut.h"
#else
#include <X11/Xlib.h>
#include <GL/glut.h>
#endif

#define GLUT_DEFAULT_WINDOW_WIDTH 640
#define GLUT_DEFAULT_WINDOW_HEIGHT 480

int __ctalkGLUTVersion (void) {
  return GLUT_API_VERSION;
}

static int __glutlib_win_id;

int __ctalkGLUTCreateMainWindow (char *title) {
  __glutlib_win_id = glutCreateWindow (title);

  return __glutlib_win_id;
}

int __ctalkGLUTInitWindowGeometry (int x, int y, int width, int height) {
  if ((x > -1) && (y > -1)) {
    glutInitWindowPosition (x, y);
  }
  /* Our minimim window size is 50x25 */
  if ((width > 50) || (height > 25)) {
    glutInitWindowSize (width, height);
    return SUCCESS;
  } else {
    /* Our default window size is 640 x 480. */
    glutInitWindowSize (GLUT_DEFAULT_WINDOW_WIDTH, 
			GLUT_DEFAULT_WINDOW_HEIGHT);
    return SUCCESS;
  }
  return ERROR;
}

int __ctalkGLUTInit (int argc, char **argv) {
  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
  return SUCCESS;
}

int __ctalkGLUTRun (void) {
  glutMainLoop (); /* Doesn't return. */
  return SUCCESS;
}


static int find_children (Display *d, Window w, char *wm_name) {
  Window root_return, parent_return;
  Window *children_return;
  unsigned int nchildren_return, i;
  Window child_window, target;
  char wm_name_return[256];

  XQueryTree (d, w, &root_return, &parent_return,
	      &children_return, &nchildren_return);
  for (i = 0; i < nchildren_return; ++i) {
    child_window = children_return[i];
    XFetchName (d, child_window, (char **)wm_name_return);
    if (((char **)wm_name_return)[0] == NULL) {
      if ((target = find_children (d, child_window, wm_name)) != 0) {
	return target;
      }
    } else if (!strcmp (((char **)wm_name_return)[0], wm_name)) {
      return child_window;
    }
  }
  return 0;
}

int __ctalkGLUTWindowID (char *wm_name) {
  Display *d;
  char *e;
  Window root_window;
  
  if ((e = getenv ("DISPLAY"))  == NULL) {
    printf ("__ctalkGLUTWindowID: Can't get DISPLAY evironment variable.\n");
    return -1;
  }
  d = XOpenDisplay (e);
  root_window = RootWindow (d, DefaultScreen (d));

  return find_children (d, root_window, wm_name);
}

void glut_default_display_callback (void) {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glutSwapBuffers ();
}

void glut_default_reshape_callback (int w, int h) {
  float ar = (float)w / (float)h;
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho (-5.0 * ar, 5.0 * ar, -5.0, 5.0, 5.0, -5.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

#define GLUT_DISPLAY_CALLBACK          0
#define GLUT_RESHAPE_CALLBACK          1
#define GLUT_IDLE_CALLBACK             2
#define GLUT_OVERLAY_DISPLAY_CALLBACK  3
#define GLUT_KEYBOARD_CALLBACK         4
#define GLUT_MOUSE_CALLBACK            5
#define GLUT_MOTION_CALLBACK           6
#define GLUT_PASSIVE_MOTION_CALLBACK   7
#define GLUT_VISIBILITY_CALLBACK       8
#define GLUT_ENTRY_CALLBACK            9
#define GLUT_SPECIAL_CALLBACK          10
#define GLUT_SPACEBALL_MOTION_CALLBACK 11
#define GLUT_SPACEBALL_ROTATE_CALLBACK 12
#define GLUT_SPACEBALL_BUTTON_CALLBACK 13
#define GLUT_BUTTONBOX_CALLBACK        14
#define GLUT_DIALS_CALLBACK            15
#define GLUT_TABLET_MOTION_CALLBACK    16
#define GLUT_TABLET_BUTTON_CALLBACK    17
#define GLUT_MENU_STATUS_CALLBACK      18
#define GLUT_MENU_STATE_CALLBACK       19
#define GLUT_TIMER_CALLBACK            20

static void *__glut_callbacks[] = {
  glut_default_display_callback,  /* GLUT_DISPLAY_CALLBACK */
  glut_default_reshape_callback,  /* GLUT_RESHAPE_CALLBACK */
  NULL,                           /* GLUT_IDLE_CALLBACK */
  NULL,                           /* GLUT_OVERLAY_DISPLAY_CALLBACK */
  NULL,                           /* GLUT_KEYBOARD_CALLBACK */
  NULL,                           /* GLUT_MOUSE_CALLBACK */
  NULL,                           /* GLUT_MOTION_CALLBACK */
  NULL,                           /* GLUT_PASSIVE_MOTION_CALLBACK */
  NULL,                           /* GLUT_VISIBILITY_CALLBACK */
  NULL,                           /* GLUT_ENTRY_CALLBACK */
  NULL,                           /* GLUT_SPECIAL_CALLBACK */
  NULL,                           /* GLUT_SPACEBALL_MOTION_CALLBACK */
  NULL,                           /* GLUT_SPACEBALL_ROTATE_CALLBACK */
  NULL,                           /* GLUT_SPACEBALL_BUTTON_CALLBACK */
  NULL,                           /* GLUT_BUTTONBOX_CALLBACK */
  NULL,                           /* GLUT_DIALS_CALLBACK */
  NULL,                           /* GLUT_TABLET_MOTION_CALLBACK */
  NULL,                           /* GLUT_TABLET_BUTTON_CALLBACK */
  NULL,                           /* GLUT_MENU_STATUS_CALLBACK */
  NULL,                           /* GLUT_MENU_STATE_CALLBACK */
  NULL                            /* GLUT_TIMER_CALLBACK */
};
  
static int __glut_timer_msec = 0;
static int __glut_timer_arg = 0;

void __ctalkGLUTInstallCallbacks (void) {
  glutDisplayFunc (__glut_callbacks[GLUT_DISPLAY_CALLBACK]);
  glutReshapeFunc (__glut_callbacks[GLUT_RESHAPE_CALLBACK]);
  if (__glut_callbacks[GLUT_IDLE_CALLBACK])
    glutIdleFunc (__glut_callbacks[GLUT_IDLE_CALLBACK]);
  if (__glut_callbacks[GLUT_OVERLAY_DISPLAY_CALLBACK])
    glutOverlayDisplayFunc (__glut_callbacks[GLUT_OVERLAY_DISPLAY_CALLBACK]);
  if (__glut_callbacks[GLUT_KEYBOARD_CALLBACK])
    glutKeyboardFunc (__glut_callbacks[GLUT_KEYBOARD_CALLBACK]);
  if (__glut_callbacks[GLUT_MOUSE_CALLBACK])
    glutMouseFunc (__glut_callbacks[GLUT_MOUSE_CALLBACK]);
  if (__glut_callbacks[GLUT_MOTION_CALLBACK])
    glutMotionFunc (__glut_callbacks[GLUT_MOTION_CALLBACK]);
  if (__glut_callbacks[GLUT_PASSIVE_MOTION_CALLBACK])
    glutPassiveMotionFunc (__glut_callbacks[GLUT_PASSIVE_MOTION_CALLBACK]);
  if (__glut_callbacks[GLUT_VISIBILITY_CALLBACK])
    glutVisibilityFunc (__glut_callbacks[GLUT_VISIBILITY_CALLBACK]);
  if (__glut_callbacks[GLUT_ENTRY_CALLBACK])
    glutEntryFunc (__glut_callbacks[GLUT_ENTRY_CALLBACK]);
  if (__glut_callbacks[GLUT_SPECIAL_CALLBACK])
    glutSpecialFunc (__glut_callbacks[GLUT_SPECIAL_CALLBACK]);
  if (__glut_callbacks[GLUT_SPACEBALL_MOTION_CALLBACK])
    glutSpaceballMotionFunc (__glut_callbacks[GLUT_SPACEBALL_MOTION_CALLBACK]);
  if (__glut_callbacks[GLUT_SPACEBALL_ROTATE_CALLBACK])
    glutSpaceballRotateFunc (__glut_callbacks[GLUT_SPACEBALL_ROTATE_CALLBACK]);
  if (__glut_callbacks[GLUT_SPACEBALL_BUTTON_CALLBACK])
    glutSpaceballButtonFunc (__glut_callbacks[GLUT_SPACEBALL_BUTTON_CALLBACK]);
  if (__glut_callbacks[GLUT_BUTTONBOX_CALLBACK])
    glutButtonBoxFunc (__glut_callbacks[GLUT_BUTTONBOX_CALLBACK]);
  if (__glut_callbacks[GLUT_DIALS_CALLBACK])
    glutDialsFunc (__glut_callbacks[GLUT_DIALS_CALLBACK]);
  if (__glut_callbacks[GLUT_TABLET_MOTION_CALLBACK])
    glutTabletMotionFunc (__glut_callbacks[GLUT_TABLET_MOTION_CALLBACK]);
  if (__glut_callbacks[GLUT_TABLET_BUTTON_CALLBACK])
    glutTabletButtonFunc (__glut_callbacks[GLUT_TABLET_BUTTON_CALLBACK]);
  if (__glut_callbacks[GLUT_MENU_STATUS_CALLBACK])
    glutMenuStatusFunc (__glut_callbacks[GLUT_MENU_STATUS_CALLBACK]);
  if (__glut_callbacks[GLUT_MENU_STATE_CALLBACK])
    glutMenuStateFunc (__glut_callbacks[GLUT_MENU_STATE_CALLBACK]);

  if (__glut_callbacks[GLUT_TIMER_CALLBACK])
    glutTimerFunc (__glut_timer_msec, __glut_callbacks[GLUT_TIMER_CALLBACK], 
		   __glut_timer_arg);
  
}


int __ctalkGLUTInstallDisplayFn (void (*fn)()) {
  __glut_callbacks[GLUT_DISPLAY_CALLBACK] = fn;
  return SUCCESS;
}

int __ctalkGLUTInstallReshapeFn (void (*fn)(int, int)) {
  __glut_callbacks[GLUT_RESHAPE_CALLBACK] = fn;
  return SUCCESS;
}

int __ctalkGLUTInstallIdleFn (void (*fn)()) {
  __glut_callbacks[GLUT_IDLE_CALLBACK] = fn;
  return SUCCESS;
}

int __ctalkGLUTInstallOverlayDisplayFunc(void (*fn)()) {
  __glut_callbacks[GLUT_OVERLAY_DISPLAY_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallKeyboardFunc (void (*fn)(unsigned char c, int x, int y)) {
  __glut_callbacks[GLUT_KEYBOARD_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallMouseFunc (void (*fn)(int button, int state,
					    int x, int y)) {
  __glut_callbacks[GLUT_MOUSE_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallMotionFunc (void (*fn)(int x, int y)) {
  __glut_callbacks[GLUT_MOTION_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallPassiveMotionFunc (void (*fn)(int x, int y)) {
  __glut_callbacks[GLUT_PASSIVE_MOTION_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallVisibilityFunc (void (*fn)(int state)) {
  __glut_callbacks[GLUT_VISIBILITY_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallEntryFunc (void (*fn)(int state)) {
  __glut_callbacks[GLUT_ENTRY_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallSpecialFunc (void (*fn)(int key, int x, int y)) {
  __glut_callbacks[GLUT_SPECIAL_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallSpaceballMotionFunc (void (*fn)(int x, int y, int z)) {
  __glut_callbacks[GLUT_SPACEBALL_MOTION_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallSpaceballRotateFunc (void (*fn)(int x, int y, int z)) {
  __glut_callbacks[GLUT_SPACEBALL_ROTATE_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallSpaceballButtonFunc (void (*fn)(int button, int state)) {
  __glut_callbacks[GLUT_SPACEBALL_BUTTON_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallButtonBoxFunc (void (*fn)(int button, int state)) {
  __glut_callbacks[GLUT_BUTTONBOX_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallDialsFunc (void (*fn)(int dial, int value)) {
  __glut_callbacks[GLUT_DIALS_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallTabletMotionFunc (void (*fn)(int x, int y)) {
  __glut_callbacks[GLUT_TABLET_MOTION_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallTabletButtonFunc (void (*fn)(int button, int state,
						   int x, int y)) {
  __glut_callbacks[GLUT_TABLET_BUTTON_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallMenuStatusFunc (void (*fn)(int status,
					   int x, int y)) {
  __glut_callbacks[GLUT_MENU_STATUS_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallMenuStateFunc (void (*fn)(int status)) {
  __glut_callbacks[GLUT_MENU_STATE_CALLBACK] = fn;
  return SUCCESS;
}
int __ctalkGLUTInstallTimerFunc (int msec, void (*fn)(int value), int value) {
  __glut_timer_msec = msec;
  __glut_timer_arg = value;
  __glut_callbacks[GLUT_TIMER_CALLBACK] = fn;
  return SUCCESS;
}

void __ctalkGLUTFullScreen (void) {
  glutFullScreen ();
}

void __ctalkGLUTSphere (double radius, int slices, int stacks, int fill) {
  if (fill)
    glutSolidSphere (radius, slices, stacks);
  else
    glutWireSphere (radius, slices, stacks);
}

void __ctalkGLUTCube (double size, int fill) {
  if (fill)
    glutSolidCube (size);
  else
    glutWireCube (size);
}

void __ctalkGLUTCone (double base, double height, int slices, int stacks,
		      int fill) {
  if (fill)
    glutSolidCone (base, height, slices, stacks);
  else
    glutWireCone (base, height, slices, stacks);
}

void __ctalkGLUTTorus (double inner_radius, double outer_radius,
		       int nsides, int rings, int fill) {
  if (fill)
    glutSolidTorus (inner_radius, outer_radius, nsides, rings);
  else
    glutWireTorus (inner_radius, outer_radius, nsides, rings);
}

void __ctalkGLUTDodecahedron (int fill) {
  if (fill)
    glutSolidDodecahedron ();
  else
    glutWireDodecahedron ();
}
void __ctalkGLUTOctahedron (int fill) {
  if (fill)
    glutSolidOctahedron ();
  else
    glutWireOctahedron ();
}
void __ctalkGLUTTetrahedron (int fill) {
  if (fill)
    glutSolidTetrahedron ();
  else
    glutWireTetrahedron ();
}
void __ctalkGLUTIcosahedron (int fill) {
  if (fill)
    glutSolidIcosahedron ();
  else
    glutWireIcosahedron ();
}
void __ctalkGLUTTeapot (double size, int fill) {
  if (fill)
    glutSolidTeapot (size);
  else
    glutWireTeapot (size);
}

void __ctalkGLUTPosition (int x, int y) {
  glutPositionWindow (x, y);
}
void __ctalkGLUTReshape (int w, int h) {
  glutReshapeWindow (w, h);
}

#define HEXASCIITOBINARY(d) (\
  (d >= '0' && d <= '9') ? d - 48 :				\
  ((d >= 'A' && d <= 'F') ? d - 55 :				\
   ((d >= 'a' && d <= 'f') ? d - 87 : 0)))

void __ctalkXPMToGLTexture (char **xpm_data, unsigned int alpha,
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
  int rrr, ggg, bbb;
  int rrrr, gggg, bbbb;

  if ((r = sscanf (xpm_data[0], "%d %d %d %d", 
		   &width, &height, &n_colors, &chars_per_pixel))
      != 4) {
    printf ("__ctalkXPMToGLTexture: %s: parse error.\n", xpm_data[0]);
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
		  /* 24-bit color. */
		  rrggbb = strtol (q + 1, NULL, 16);
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
		pix = (rrggbb << 8) + 255;
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

#else /* HAVE_GLUT_H */

int __ctalkGLUTInit (int argc, char **argv) {
  return ERROR;
}

int __ctalkGLUTRun (void) {
  return ERROR;
}

int __ctalkGLUTInitWindowGeometry (int x, int y, int width, int height) {
  return ERROR;
}

int __ctalkGLUTCreateMainWindow (char *title) {
  return ERROR;
}

int __ctalkGLUTVersion (void) {
  return ERROR;
}
void __ctalkGLUTInstallCallbacks (void) {
  return;
}
int __ctalkGLUTInstallDisplayFn (void (*fn)()) {
  return ERROR;
}
int __ctalkGLUTInstallReshapeFn (void (*fn)(int, int)) {
  return ERROR;
}
int __ctalkGLUTInstallIdleFn (void (*fn)(int, int)) {
  return ERROR;
}
int __ctalkGLUTInstallOverlayDisplayFunc(void (*fn)()) {
  return ERROR;
}
int __ctalkGLUTInstallKeyboardFunc (void (*fn)(unsigned char c, int x, int y)) {
  return ERROR;
}
int __ctalkGLUTInstallMouseFunc (void (*fn)(int button, int state,
					    int x, int y)) {
  return ERROR;
}
int __ctalkGLUTInstallMotionFunc (void (*fn)(int x, int y)) {
  return ERROR;
}
int __ctalkGLUTInstallPassiveMotionFunc (void (*fn)(int x, int y)) {
  return ERROR;
}
int __ctalkGLUTInstallVisibilityFunc (void (*fn)(int state)) {
  return ERROR;
}
int __ctalkGLUTInstallEntryFunc (void (*fn)(int state)) {
  return ERROR;
}
int __ctalkGLUTInstallSpecialFunc (void (*fn)(int key, int x, int y)) {
  return ERROR;
}
int __ctalkGLUTInstallSpaceballMotionFunc (void (*fn)(int x, int y, int z)) {
  return ERROR;
}
int __ctalkGLUTInstallSpaceballRotateFunc (void (*fn)(int x, int y, int z)) {
  return ERROR;
}
int __ctalkGLUTInstallSpaceballButtonFunc (void (*fn)(int button, int state)) {
  return ERROR;
}
int __ctalkGLUTInstallButtonBoxFunc (void (*fn)(int button, int state)) {
  return ERROR;
}
int __ctalkGLUTInstallDialsFunc (void (*fn)(int dial, int value)) {
  return ERROR;
}
int __ctalkGLUTInstallTabletMotionFunc (void (*fn)(int x, int y)) {
  return ERROR;
}
int __ctalkGLUTInstallTabletButtonFunc (void (*fn)(int button, int state,
						   int x, int y)) {
  return ERROR;
}
int __ctalkGLUTInstallMenuStatusFunc (void (*fn)(int status,
					   int x, int y)) {
  return ERROR;
}
int __ctalkGLUTInstallMenuStateFunc (void (*fn)(int status)) {
  return ERROR;
}
int __ctalkGLUTInstallTimerFunc (int msec, void (*fn)(int value), int value) {
  return ERROR;
}
void __ctalkGLUTFullScreen (void) {
  return;
}
void __ctalkGLUTSphere (double radius, int slices, int stacks, int fill) {
  return;
}
void __ctalkGLUTCube (double size, int fill) {
  return;
}
void __ctalkGLUTCone (double base, double height, int slices, int stacks,
		      int fill) {
  return;
}
void __ctalkGLUTTorus (double inner_radius, double outer_radius,
		       int nsides, int rings, int fill) {
  return;
}
void __ctalkGLUTDodecahedron (int fill) {
  return;
}
void __ctalkGLUTOctahedron (int fill) {
  return;
}
void __ctalkGLUTTetrahedron (int fill) {
  return;
}
void __ctalkGLUTIcosahedron (int fill) {
  return;
}
void __ctalkGLUTTeapot (double size, int fill) {
  return;
}
void __ctalkGLUTPosition (int x, int y) {
  return;
}
void __ctalkGLUTReshape (int w, int h) {
  return;
}
void __ctalkXPMToGLTexture (char **xpm_data, unsigned int alpha,
			    int *width_out, int *height_out,
			    void **texel_data_out) {
  return;
}
int __ctalkGLUTWindowID (char *wm_name) {
  return;
}
#endif /* _HAVE_GLUT_H_ */
