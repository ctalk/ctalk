/* $Id: argblk.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2014 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _ARGBLK_H
#define _ARGBLK_H

typedef struct _argblk ARGBLK;

struct _argblk {
  int id,
    rcvr_idx,
    start_idx,
    end_idx,
    parser_level;
  bool has_return;
  char *return_exprs[MAXARGS];
  int return_expr_ptr;
  OBJECT *blk_rcvr_obj;
  LIST *buf;
};

#define C_ARG_BLK (argblks[argblk_ptr+1])
#define C_ARG_BLK_BUFFER (argblks[argblk_ptr+1] -> buf)
#define C_ARG_BLK_BUFFER_ABOVE (argblks[argblk_ptr+2] -> buf)
#define TOP_LEVEL_ARGBLK (argblk_ptr == MAXARGS - 1)
#define ARGBLK_TOK(i) ((argblk_ptr < MAXARGS) && \
			(((i) <= C_ARG_BLK -> start_idx) &&	\
			 ((i) >= C_ARG_BLK -> end_idx)))

#endif   /* _ARGBLK_H */


