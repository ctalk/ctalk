/* $Id: phash.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of ctalk.
  Copyright © 2005-2007  Robert Kiesling, rkiesling@users.sourceforge.net.
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

#ifndef _PHASH_H
#define _PHASH_H

typedef struct _h_list HLIST;

struct _h_list {
  int sig;
  char s_key[MAXLABEL];
  char source_file[FILENAME_MAX];
  int error_line,
    error_column;
  int hits;
  void *data;
  struct _h_list *next,
    *prev;
};

typedef struct _hashbucket{
  int sig;
  HLIST *mbrs,
    *mbrs_head;
} HASHBUCKET;

typedef HASHBUCKET **HASHTAB;

#endif /* _PHASH_H */
