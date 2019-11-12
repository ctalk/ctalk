/* $Id: ctalkGLUTdefs.h,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-*/

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

#ifdef __APPLE__
/* 
   You might also need to include -I /usr/X11R6/include as a command
   line option. 
*/
/*#include <GL/gl.h>
  #include <GL/glu.h> */
/* 
   Some of these paths may be symlinked, so check if you need to adapt
   this to a particular machine.
*/
/* Here we trick the framework into thinking we're on a UNIX system. YMMV. */
#ifdef __APPLE__
#undef __APPLE__
#define __ctalk_need_apple_
#endif
#ifdef MACOSX
#undef MACOSX
#define __ctalk_need_osx_
#endif
#include "/System/Library/Frameworks/GLUT.framework/Versions/A/Headers/glut.h"
/* 
   Then re-define them after we're done. You may need to check the actual
   definitions to avoid a lot of warnings about macro redefinitions.  Try:
   $ echo ' ' | cpp -dM - | less 
   to find the definitions of the built-in macros. 
 */
#ifdef __ctalk_need_apple_
#define __APPLE__ 1
#undef __ctalk_need_apple_
#endif
#ifdef __ctalk_need_osx_
#define MACOSX
#undef __ctalk_need_osx_
#endif
#else /* #ifdef __APPLE__ */
/* Just about everyone else. */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif /* #ifdef __APPLE__ */
