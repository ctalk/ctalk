/* $Id: chash.h,v 1.2 2019/11/11 20:21:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright � 2005-2014  Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _CHASH_H
#define _CHASH_H

/* #define HASH_SIG     0xFF00 */

typedef struct _h_list HLIST;
/*
 *  Defined other places, so check if changing this.
 */
#ifndef __llvm__
#define GNUC_PACKED_STRUCT (defined(__linux__) && defined(__i386__) &&	\
			    defined(__GNUC__) && (__GNUC__ >= 3))
#endif

struct _h_list {
  char s_key[MAXLABEL];
  void *data;
  struct _h_list *next,
    *prev;
  /* offsetof says 268 for the prev member, above. */
#if GNUC_PACKED_STRUCT
  char pad[244];
} __attribute__ ((packed));
#else
};
#endif


typedef struct _hashbucket{
  int sig;
  HLIST *mbrs,
    *mbrs_head;
} HASHBUCKET;

typedef HASHBUCKET **HASHTAB;

#define IS_HLIST(x) ((x)->sig == HASH_SIG)

#endif /* _CHASH_H */
