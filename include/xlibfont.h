/* $Id: xlibfont.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2014, 2016 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _XLIBFONT_H_

#include <X11/Xlib.h>

typedef struct {
  char *normal;
  XFontStruct *normal_xfs;
  char *bold;
  XFontStruct *bold_xfs;
  char *italic;
  XFontStruct *italic_xfs;
  char *bolditalic;
  XFontStruct *bolditalic_xfs;
  char *selectedfont;
  XFontStruct *selected_xfs;
} XLIBFONT;

#define _XLIBFONT_H_
#endif /* _XLIBFONT_H_ */
