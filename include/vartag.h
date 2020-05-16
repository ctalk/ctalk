/* $Id: vartag.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of ctalk.
  Copyright © 2005-2007 Robert Kiesling, rkiesling@users.sourceforge.net
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef _VARTAG_H
#define _VARTAG_H

#include "object.h"

#ifndef __have_vartag_typedef
typedef struct _vartag VARTAG;
#define __have_vartag_typedef
#endif

/* Also defined in object.h. */
#ifndef __have_vartag_def
#define __have_vartag_def
struct _vartag {
  int sig;
  VARENTRY *tag;
  VARENTRY *from;        /* If we alias an object, this is the VARENTRY
                            of the original object. */
  struct _vartag *next;
  struct _vartag *prev;
};
#endif

#ifndef VARTAG_SIG
#define VARTAG_SIG 0xfff0
#endif
#ifndef IS_VARTAG
#define IS_VARTAG(x) ((x) && (x)->sig == VARTAG_SIG)
#endif

#ifndef HAS_VARTAGS
#define HAS_VARTAGS(x) ((x) && (x -> __o_vartags!=NULL) && \
			((x)->sig==VARTAG_SIG))
#endif

#ifndef IS_EMPTY_VARTAG
#define IS_EMPTY_VARTAG(x) ((x) && ((x)->sig == VARTAG_SIG) && \
			    ((x) -> tag == NULL))
#endif

#ifndef I_UNDEF
#define I_UNDEF ((void *)-1)
#endif

#ifndef TAG_REF_PREFIX
#define TAG_REF_PREFIX 0
#endif
#ifndef TAG_REF_POSTFIX
#define TAG_REF_POSTFIX 1
#endif
#ifndef TAG_REF_TEMP
#define TAG_REF_TEMP 2
#endif

#endif /* _VARTAG_H */

