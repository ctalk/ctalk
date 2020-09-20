/* $Id: glewlib.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2017-2019 Robert Kiesling, rk3314042@gmail.com.
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
#include <object.h>
#include <message.h>
#include <ctalk.h>

bool has_ARB = false;
bool has_GLEW_2_0 = false;

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)

#ifdef HAVE_GLEW_H

#include <GL/glew.h>

static bool initialized = false;

int __ctalkInitGLEW (void) {
  GLenum status;
  
  if (!initialized) {
    status = glewInit ();
    if (status != GLEW_OK) {
      fprintf (stderr, "ctalk: GLEW Error: %s\n",
	       glewGetErrorString (status));
      return ERROR;
    } else {
      initialized = true;
      if (!GLEW_VERSION_2_0) {
	has_GLEW_2_0 = false;
      } else {
	has_GLEW_2_0 = true;
      }
      if (!GLEW_ARB_vertex_shader && !GLEW_ARB_fragment_shader) {
	has_ARB = false;
      } else {
	has_ARB = true;
      }
    }
  }
  return SUCCESS;
}

bool __ctalkGLEW20 (void) { return has_GLEW_2_0; }
bool __ctalkARB (void) {return has_ARB; }


#else /* HAVE_GLEW_H */

void glew_support_error (void) {
  printf ("ctalk: The library does not support the GLEW libraries. "
	  "Refer to the README file in the Ctalk source code package.\n");
}

int __ctalkInitGLEW (void) {
  glew_support_error ();
  return ERROR;
}

bool __ctalkGLEW20 (void) {
  glew_support_error ();
  return has_GLEW_2_0;
}
bool __ctalkARB (void) {
  glew_support_error ();
  return has_ARB;
}

#endif /* HAVE_GLX_H */

#endif /* ! defined (DJGPP) && ! defined (WITHOUT_X11) */ 
