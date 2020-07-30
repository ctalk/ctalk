/* $Id: rtinfo.h,v 1.2 2020/07/29 19:48:07 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2016 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _RTINFO_H
#define _RTINFO_H

#ifndef FILENAME_MAX
#define FILENAME_MAX 4096
#endif
#ifndef MAXLABEL
#define MAXLABEL 0x100
#endif

#ifndef _OBJECT_H
#include "object.h"
#endif

#ifndef _CVAR_H
#include "cvar.h"
#endif

#include "list.h"

typedef struct _rtfn {
  char name[MAXLABEL];
  union {
    VARENTRY *vars;
    OBJECT *objs; 
  } local_objects;
  CVAR *local_cvars;
  LIST *user_objects,
    *user_object_ptr;
  int n_user_objs;
  void *db;
  struct _rtfn *next;
  struct _rtfn *prev;
} RT_FN;

typedef struct _rtinfo {
  char source_file[FILENAME_MAX];
  OBJECT *rcvr_obj;
  OBJECT *rcvr_class_obj;
  OBJECT *method_class_obj;
  METHOD *method;
  OBJECT *(*method_fn)(void);
  RT_FN *_rt_fn;
  bool classlib_read;
  bool inline_call;
  bool block_scope;
  int rt_methd_ptr;
  int _arg_frame_top;
  int _block_frame_top;
  int _successive_call;
  VARENTRY *local_object_cache[MAXARGS];
  VARENTRY *arg_active_tag;
  char arg_text[0x2000]; /* MAXMSG */
  int local_obj_cache_ptr;
} RT_INFO;

#define EVAL_STATUS_TERMINAL_TOK         (1 << 0)
#define EVAL_STATUS_INSTANCE_VAR         (1 << 1)
#define EVAL_STATUS_VALUE_VAR            (1 << 2)
#define EVAL_STATUS_CLASS_VAR            (1 << 3)
#define EVAL_STATUS_VAR_REF              (1 << 4)
#define EVAL_STATUS_ASSIGN_ARG           (1 << 5)
/* single token, set by __ctalk_get_object. */
#define EVAL_STATUS_NAMED_PARAMETER      (1 << 6)
#define EVAL_STATUS_DIRECT_SUBEXPR       (1 << 7)
/*
  This tells us to remove the extra '*' from the 
   start of an expression because it's faster if
   we treat the result as an OBJECT *. E.g., 
   myObj -> __o_name, instead of (*myObj) -> __o_name
*/
#define EVAL_STATUS_CVARTAB_OBJECT_PTR  (1 << 7)

#endif  /* #ifndef _RTINFO_H */
