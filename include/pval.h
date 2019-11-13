/* $Id: pval.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _PVAL_H
#define _PVAL_H

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
/* True and False are compatible with Xlib.h. */
#define True 1
#define False 0
#else
#ifndef __CT_BOOLEAN__
#define bool _Bool;
#define true 1
#define false 0
#define True 1
#define False 0
#define TRUE true
#define FALSE false
#define __bool_true_false_are_defined	1
#define __CT_BOOLEAN__
#endif /* #ifndef __CT_BOOLEAN__ */
#endif /* #ifdef HAVE_STDBOOL_H */

typedef struct _val VAL;

struct _val{
  int __type;
  union {
    void *__ptr;
    int __i;
    bool __b;
    double __d;          /* Double and float. */
#ifndef __APPLE__
    long double __ld;
#endif
    long __l;
    long long __ll;
    unsigned int __u;
    unsigned long int __ul;
  } __value;
  void *__deref_ptr;
};

#endif /* #ifndef _PVAL_H */
