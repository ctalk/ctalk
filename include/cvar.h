/* $Id: cvar.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2015, 2018 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _OBJECT_H
#include "object.h"
#endif

#ifndef _PVAL_H
#include "pval.h"
#endif

#ifndef _CVAR_H
#define _CVAR_H

/*
 *  C99 translation limit.
 */
#define MAX_DECLARATORS 12

#define IS_SIGN_LABEL(s) (!strcmp (s, "signed") ||	\
  !strcmp (s, "unsigned"))
#define IS_SIGN_LABEL2(s) (str_eq (s, "signed") ? 1 : \
			  (str_eq (s, "unsigned") ? 2 : 0))
#define IS_TYPE_LENGTH(s) (!strcmp (s, "long") || \
                           !strcmp (s, "short"))
#define IS_SCALAR_TYPE(s) (!strcmp (s, "char") || \
                           !strcmp (s, "double") || \
                           !strcmp (s, "float") || \
                           !strcmp (s, "int") || \
                           !strcmp (s, "_Bool") || \
                           !strcmp (s, "_Complex") || \
                           !strcmp (s, "_Imaginary"))
#define IS_AGGREGATE_TYPE(s) (str_eq (s, "struct") ? 1 : \
			      (str_eq (s, "enum") ? 2 :	 \
			       (str_eq (s, "union") ? 3 : 0)))

#define CVAR_SIG 0x838383

typedef struct _cvar CVAR;

typedef enum {
  auto_sc_decl,
  const_sc_decl,
  extern_sc_decl,
  inline_sc_decl,
  register_sc_decl,
  static_sc_decl,
  volatile_sc_decl,
  typedef_sc_decl         /* Include typedef here even though it's not
			     a storage class. */
} STORAGE_CLASS;
  

#define CVAR_ATTR_STRUCT             (1 << 0)
#define CVAR_ATTR_STRUCT_INCOMPLETE  (1 << 1)
#define CVAR_ATTR_STRUCT_DECL        (1 << 2)
#define CVAR_ATTR_STRUCT_MBR         (1 << 3)
#define CVAR_ATTR_STRUCT_PTR         (1 << 4)
#define CVAR_ATTR_FN_DECL            (1 << 5)
#define CVAR_ATTR_FN_PTR_DECL        (1 << 6)
#define CVAR_ATTR_FN_PARAM           (1 << 7)
#define CVAR_ATTR_FN_PROTOTYPE       (1 << 8)
#define CVAR_ATTR_TYPEDEF            (1 << 9)
#define CVAR_ATTR_EXTERN             (1 << 10)
#define CVAR_ATTR_TYPEDEF_INCOMPLETE (1 << 11)
#define CVAR_ATTR_ENUM               (1 << 12)
#define CVAR_ATTR_FN_NO_PARAMS       (1 << 13)
#define CVAR_ATTR_ARRAY_DECL         (1 << 14)
#define CVAR_ATTR_STRUCT_TAG         (1 << 15)
#define CVAR_ATTR_STRUCT_PTR_TAG     (1 << 16)
#define CVAR_ATTR_ELLIPSIS           (1 << 17)
/* This is the CVAR whose declaration gets output to the _output_ TUI
   - so only a single GLOBAL declaration goes to the compiler - then
   we can elide "extern" declarations of the same CVAR when
   compiling other input files. */
#define CVAR_ATTR_TUI_DECL           (1 << 18)
#define CVAR_ATTR_CVARTAB_ENTRY      (1 << 19)
/* this is the attribute for the container struct that contains
   a float aggregate for use with an argblk - struct can be aliased. */
#define CVAR_ATTR_FP_ARGBLK          (1 << 20)
/* used so we can selectively delete CVARs when multiple
   closely spaced eval_expr calls follow a single set of
   register CVAR calls (rt_cvar.c and rt_expr.c). */
#define CVAR_ATTR_EVALED             (1 << 21)

#define CVAR_TYPE_CHAR               (1 << 1)
#define CVAR_TYPE_CONST              (1 << 2)
#define CVAR_TYPE_DOUBLE             (1 << 3)
#define CVAR_TYPE_ENUM               (1 << 4)
#define CVAR_TYPE_EXTERN             (1 << 5)
#define CVAR_TYPE_FLOAT              (1 << 6)
#define CVAR_TYPE_INLINE             (1 << 7)
#define CVAR_TYPE_INT                (1 << 8)
#define CVAR_TYPE_LONG               (1 << 9)
#define CVAR_TYPE_REGISTER           (1 << 10)
#define CVAR_TYPE_SHORT              (1 << 11)
#define CVAR_TYPE_SIGNED             (1 << 12)
#define CVAR_TYPE_STATIC             (1 << 13)
#define CVAR_TYPE_STRUCT             (1 << 14)
#define CVAR_TYPE_UNION              (1 << 15)
#define CVAR_TYPE_UNSIGNED           (1 << 16)
#define CVAR_TYPE_VOID               (1 << 17)
#define CVAR_TYPE_VOLATILE           (1 << 18)
#define CVAR_TYPE_BOOL               (1 << 19)
#define CVAR_TYPE_COMPLEX            (1 << 20)
#define CVAR_TYPE_IMAGINARY          (1 << 21)
#define CVAR_TYPE_LONGLONG           (1 << 22)
#define CVAR_TYPE_TYPEDEF            (1 << 23)
#ifdef __GNUC__
#define CVAR_TYPE_GNU_COMPLEX        (1 << 24)
#define CVAR_TYPE_GNU_INLINE         (1 << 25)
#define CVAR_TYPE_GNU_THREAD         (1 << 26)
#endif
#define CVAR_TYPE_FILE               (1 << 27)
#define CVAR_TYPE_OBJECT             (1 << 28)
#define CVAR_TYPE_LONGDOUBLE         (1 << 29)

struct _cvar {
  int sig;
  char decl[MAXLABEL];
  char type[MAXLABEL];
  char qualifier[MAXLABEL];
  char qualifier2[MAXLABEL];
  char qualifier3[MAXLABEL];
  char qualifier4[MAXLABEL];
  char storage_class[MAXLABEL];
  char name[MAXLABEL];
  int n_derefs,
    initializer_size;
  int attrs, 
    type_attrs,
    attr_data;
  bool is_unsigned;
  bool evaled;
  int scope;
  VAL val;                /* Used at run time to evaluate expressions
			     and for enum values.                     */
  struct _cvar *members;
  struct _cvar *params;
  struct _cvar *next;
  struct _cvar *prev;
};

/* Used in place of CVARs when we can set the declarations by
   assigning pointers instead of using strcpy. */
typedef struct _param_cvar PARAMCVAR;

struct _param_cvar {
  int sig;
  char *decl;
  char *type;
  char *qualifier;
  char *qualifier2;
  char *qualifier3;
  char *qualifier4;
  char *storage_class;
  char *name;
  int n_derefs,
    initializer_size;
  int attrs, 
    type_attrs;
  bool is_unsigned;
  int scope;
  VAL val;
  struct _cvar *members;
  struct _cvar *params;
  struct _cvar *next;
  struct _cvar *prev;
};

typedef struct _cfunc {
  char decl[MAXLABEL];
  char return_type[MAXLABEL];
  char qualifier_type[MAXLABEL];
  char qualifier2_type[MAXLABEL];
  char storage_class[MAXLABEL];
  int is_unsigned;
  int return_derefs;
  int is_ptr_decl;
  int is_prototype_decl;
  int return_type_attrs;
  CVAR *params;
  struct _cfunc *next;
  struct _cfunc *prev;
} CFUNC;

typedef struct _varname {
  char *name;
  int n_derefs,
    attrs,
    array_initializer_size;
} VARNAME;

typedef struct _fn_declarator {
  char *name;
  int attr;
} FN_DECLARATOR;

typedef enum {
  aggregate_expr_type_null = 0,
  aggregate_expr_type_struct,
  aggregate_expr_type_array
} AGGREGATE_EXPR_TYPE;

typedef CVAR** CVARREF_T;
#define CVARREF(__o) (&(__o))

#define IS_CVAR(x) ((x) && (x)->sig==CVAR_SIG)

typedef struct _expr_fn_rec EXPR_FN_REC;

struct _expr_fn_rec {
  int stack_idx;
  CFUNC *cfunc;
};

#endif /* _CVAR_H */
