/* $Id: xrender.h,v 1.2 2019/11/05 20:19:28 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2014  Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _XRENDER_H

#if defined (HAVE_XFT_H) && defined (HAVE_XRENDER_H)
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

typedef struct _xrenderdrawrec XRENDERDRAWREC;

struct _xrenderdrawrec {
  Drawable drawable;
  XftDraw *draw;
  Picture picture;
  Picture fill_picture;
  XRenderPictFormat  *mask_format;
  struct _xrenderdrawrec *next;
  struct _xrenderdrawrec *prev;
};

#define MAXXRCOLORS 8192

typedef struct _xrnamedcolor XRNAMEDCOLOR;

struct _xrnamedcolor {
  char name[MAXLABEL];
  XRenderColor xrcolor;
  unsigned long int pixel;
};

#endif /* HAVE_XFT_H && HAVE_XRENDER_H */
#define _XRENDER_H
#endif /* _XRENDER_H */
