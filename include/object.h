/* $Id: object.h,v 1.1.1.1 2020/12/13 14:51:03 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2015, 2017-2020 Robert Kiesling, rk3314042@gmail.com.
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
#define _OBJECT_H

#ifndef _CTALK_LIB

#include "list.h"

#ifndef MAXLABEL
#define MAXLABEL 0x100
#endif

#ifndef MAXARGS
#define MAXARGS 512
#endif

/*
 *  Object scope constants.
 */
#define GLOBAL_VAR                  (1 << 0)
#define LOCAL_VAR                   (1 << 1)
#define ARG_VAR                     (1 << 2)
#define RECEIVER_VAR                (1 << 3)
#define PROTOTYPE_VAR               (1 << 4)
#define CREATED_PARAM               (1 << 6)
#define CVAR_VAR_ALIAS_COPY         (1 << 7)
#define SUBEXPR_CREATED_RESULT      (1 << 8)
#define VAR_REF_OBJECT              (1 << 9)
#define METHOD_USER_OBJECT          (1 << 10)
#define TYPECAST_OBJECT             (1 << 11)
#define SUBSCRIPT_OBJECT_ALIAS      (1 << 13)

#define CREATED_CVAR_SCOPE (LOCAL_VAR | CVAR_VAR_ALIAS_COPY)
/* This should work as long as we decide that when the varentry is
   NULL, then the object is still just a temporary object. */
#define HAS_CREATED_CVAR_SCOPE(o)  \
  (IS_OBJECT(o) && \
  ((o) -> scope & LOCAL_VAR) &&					\
  ((o) -> scope & CVAR_VAR_ALIAS_COPY) &&			\
   (((o) -> __o_vartags) ? ((o) -> __o_vartags -> tag == NULL) : \
    ((o) -> __o_vartags == NULL)))

#if 0
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !(FALSE)
#endif
#endif

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
/*  True and False are compatible with Xlib.h. */
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


/* If changing these, also change in ctalklib. */
#define METHOD_RETURN "OBJECT * "
#define METHOD_RETURN_LENGTH 9
#define METHOD_C_PARAM "(void)"
#define METHOD_C_PARAM_LENGTH 6 

#define INSTANCE_SELECTOR "_instance_"
#define CLASS_SELECTOR    "_class_"

#ifndef NULLSTR
#define NULLSTR "(null)"
#define NULLSTR_LENGTH 6
#endif

/*
 *  Attributes for run-time method calls.
 */
#define METHOD_SUPER_ATTR  (1 << 0)

#define METHOD_ERR_FMT "\"%s,\" (class %s)"

typedef enum {
  null_context,
  lvalue_context,
  receiver_context,
  argument_context,     /* Argument to a method. */
  c_argument_context,   /* Argument to a C operator. */
  c_context
} OBJECT_CONTEXT;

typedef enum {
  arg_null,
  arg_const_tok,
  arg_const_expr,
  arg_obj_tok,
  arg_method_param_tok,
  arg_c_var_tok,
  arg_obj_expr,
  arg_c_fn_expr,
  arg_c_writable_fn_expr,
  arg_c_var_expr,
  arg_c_fn_const_expr,
  arg_c_global_arg,
  arg_rt_expr,
  arg_c_sizeof_expr,
  arg_compound_method
} ARG_CLASS;

typedef enum {
  rt_expr_null,
  rt_expr_fn,
  rt_expr_user_fn,
  rt_c_fn,
  rt_self_instvar_simple_expr,
  rt_bool_multiple_subexprs,
  rt_prefix_rexpr,
  rt_mixed_c_ctalk
} RT_EXPR_CLASS;

typedef enum {
  rt_cleanup_null,
  rt_cleanup_obj_ref,
  rt_cleanup_local_obj_init,
  rt_cleanup_exit_delete
} RT_CLEANUP_MODE;

typedef enum {
  i_or_c_instance,
  i_or_c_class
} I_OR_C;

typedef struct _method METHOD;

typedef struct _object OBJECT;

typedef struct _varentry VARENTRY;

#ifndef __have_vartag_typedef
typedef struct _vartag VARTAG;
#define __have_vartag_typedef
#endif

#define PARAM_C_PARAM (1 << 0)
#define PARAM_VARARGS_PARAM (1 << 1)
#define PARAM_PREFIX_METHOD_ATTRIBUTE (1 << 2)
#define PARAM_PFO (1 << 3)   /* pointer to function returning object. */

#define PARAM_SIG 0xf1f1f1
#define IS_PARAM(x) ((x) && (x)->sig == PARAM_SIG)

typedef struct _param PARAM;

struct _param {
  int sig;
  char class[MAXLABEL];
  char name[MAXLABEL];
  int attrs;
  int is_ptr,
    is_ptrptr,
    n_derefs;
};

typedef struct _tagparam TAGPARAM;

struct _tagparam {
  int sig;
  VARENTRY *parent_tag;
  char class[MAXLABEL];
  char name[MAXLABEL];
};

#define ARG_SIG 0xf2f2f2
#define IS_ARG(a) ((a) && (a) -> sig == ARG_SIG)

typedef struct _arg ARG;
struct _arg {
  int sig;
  OBJECT *obj;
  int call_stack_frame;
};

#ifndef _CVAR_H
#include "cvar.h"
#endif

#define METHOD_PREFIX_ATTR (1 << 0)
#define METHOD_VARARGS_ATTR (1 << 1)
#define METHOD_NOINIT_ATTR (1 << 2)
#define METHOD_ARGBLK_ATTR (1 << 3)
#define METHOD_CONTAINS_ARGBLK_ATTR (1 << 4)

#define METHOD_SIG 0xf2f2f2
#define IS_METHOD(x) ((x) && (x)->sig==METHOD_SIG)

/* If we don't know or don't care how many parameters a 
   method needs when finding a method. */
#define ANY_ARGS -1

#ifdef LIB_BUILD
#define M_ARGS_DECLARED(m) (((m) -> attrs & RT_DATA_IS_NR_ARGS_DECLARED) ? \
			    m -> attr_data : ANY_ARGS)
#else
#define M_ARGS_DECLARED(m) (((m) -> attrs & DATA_IS_NR_ARGS_DECLARED) ? \
			    m -> attr_data : ANY_ARGS)
#endif

/*
 *  When changing this, also change in ctalklib, ctldjgpp, message.h,
 *  and chash.h. 
 */
#ifndef __llvm__
#define GNUC_PACKED_STRUCT (defined(__linux__) && defined(__i386__) &&	\
			    defined(__GNUC__) && (__GNUC__ >= 3))
#endif

struct _method {
  int sig;
  char name[MAXLABEL];
  char selector[MAXLABEL];
  char returnclass[MAXLABEL];
  OBJECT *rcvr_class_obj;
  OBJECT *(*cfunc)();      /* Method prototype.            */
  char *src;
  PARAM *params[MAXARGS];
  int n_params;
  int varargs;
  int prefix;
  int no_init;
  int n_args;
  int primitive;
  int attrs;
  int error_line;
  int error_column;
  int arg_frame_top;
  int rcvr_frame_top;
  bool imported;
  bool queued;
  ARG *args[MAXARGS];
  union {
    VARENTRY *vars;
    OBJECT *objs; 
  } local_objects[MAXARGS];
  int nth_local_ptr;
  LIST *user_objects,
    *user_object_ptr;
  int n_user_objs;
  CVAR *local_cvars;
  void *db;    /* reserved for future debug data */
  struct _method *next;
  struct _method *prev;
};

#define M_LOCAL_VAR_LIST(__m__) \
  ((__m__)->local_objects[(__m__)->nth_local_ptr].vars)
#define M_LOCAL_OBJ_LIST(__m__) \
  ((__m__)->local_objects[(__m__)->nth_local_ptr].objs)

/* if changing, also change in classes/ctalklib.in */
#define INTVAL(x) *(int *)(x)
#define UINTVAL(x) *(unsigned int *)(x)
#define SETINTVARS(x, i) (*(int *)(x) -> __o_value) =	\
    (*(int *)(x) -> instancevars -> __o_value) = (i)
#define BOOLVAL(x) *(int *)(x)
#define LLVAL(x) *(long long int *)(x)
#define SYMVAL(x) (*(uintptr_t *)(x))
#define SYMTOOBJ(x) (*(OBJECT **)(x))

/* Object attributes */
#define OBJECT_IS_VALUE_VAR            (1 << 0)
#define OBJECT_VALUE_IS_C_CHAR_PTR_PTR (1 << 1)
#define OBJECT_IS_NULL_RESULT_OBJECT   (1 << 2)
#define OBJECT_HAS_PTR_CX              (1 << 3)
#define OBJECT_IS_GLOBAL_COPY          (1 << 4)
#define OBJECT_IS_I_RESULT             (1 << 5)
#define OBJECT_IS_STRING_LITERAL       (1 << 6)
/* This means that the object is a collection member of __o_p_obj,
   not an instance variable. Mostly used for Key objects. This
   tells functions like __ctalk_arg () that this is the topmost
   object we want to push onto the arg stack, that it's not an
   instance variable, and we don't want to push the entire
   parent collection onto the argument stack.
*/
#define OBJECT_IS_MEMBER_OF_PARENT_COLLECTION (1 << 7)
#define OBJECT_HAS_LOCAL_TAG (1 << 8)  /* A temporary tag added by one of
					  the basicNew methods. */
#define OBJECT_IS_LITERAL_CHAR (1 <<  9)
#define OBJECT_IS_CONSTANT_INT_OR_LONG (1 << 10)
#define OBJECT_IS_CONSTANT_LONGLONG (1 << 11)
#define OBJECT_IS_CONSTANT_FLOAT (1 << 12)
#define OBJECT_VALUE_IS_MEMORY_VECTOR (1 << 13)
#define OBJECT_IS_DEREF_RESULT     (1 << 14)
#define OBJECT_IS_FN_ARG_OBJECT    (1 << 15) /* Front end only. */
#define OBJECT_REF_IS_CVAR_PTR_TGT (1 << 16)  /* see cvar_object* (rt_cvar.c) */
#define ZERO_LENGTH_STR_INIT       (1 << 17) /* makes strings more friendly*/  
#define INT_BUF_SIZE_INIT          (1 << 18)
#define BOOL_BUF_SIZE_INIT         (1 << 19)
#define CHAR_BUF_SIZE_INIT         (1 << 20)
#define LONGLONG_BUF_SIZE_INIT     (1 << 21)
#define SYMBOL_BUF_SIZE_INIT       (1 << 22)  
#define OBJECT_VALUE_IS_BIN_INT    (1 << 23)
#define OBJECT_VALUE_IS_BIN_BOOL   (1 << 24)
#define OBJECT_VALUE_IS_BIN_LONGLONG (1 << 25)
#define OBJECT_VALUE_IS_BIN_SYMBOL (1 << 26)  
/* only set when deleting objects during app exit */
#define OBJECT_IS_BEING_CLEANED_UP (1 << 27)
  
struct _object {
#ifndef __sparc__
  int sig;
#else
  char sig[16];
#endif
  char __o_name[MAXLABEL];
  char __o_classname[MAXLABEL];
  OBJECT *__o_class;
  char __o_superclassname[MAXLABEL];
  OBJECT *__o_superclass;
  OBJECT *__o_p_obj;
  VARTAG *__o_vartags;
  char *__o_value;
  METHOD *instance_methods,
    *class_methods;
  int scope;
  int nrefs;
  struct _object *classvars;
  struct _object *instancevars;    
  struct _object *next;
  struct _object *prev;
  int attrs;
#if GNUC_PACKED_STRUCT
  char pad[196];
} __attribute__ ((packed));
#else
};
#endif

#define VARENTRY_SIG 0xa2a2a2
#define IS_VARENTRY(x) ((x) && (x)->sig == VARENTRY_SIG)
struct _varentry {
  int sig;
  TAGPARAM *var_decl;
  OBJECT *var_object;
  OBJECT *orig_object_rec;  /* Tells us if the label is still 
			       unaliased. Local variables for now only.*/
  void *i, *i_post, *i_temp;
  int del_cnt;  /* Set in __delete_method_local_objs () only for now.  Read
		   only where needed so far....  */
  bool is_local;
  struct _varentry *next, *prev;
};

/* Also defined in vartag.h */
#ifndef __have_vartag_def
#define __have_vartag_def
struct _vartag {
  int sig;
  VARENTRY *tag;
  VARENTRY *from;        /* If we alias an object, this is the VARENTRY
                            of the original object. */
  struct _vartag *next, *prev;
};
#endif

#ifndef VARTAG_SIG
#define VARTAG_SIG 0xf2f2f2
#endif
#ifndef IS_VARTAG
#define IS_VARTAG(vt) ((vt) && (vt)->sig == VARTAG_SIG)
#endif
#ifndef IS_EMPTY_VARTAG
#define IS_EMPTY_VARTAG(vt) ((vt) && ((vt)->sig == VARTAG_SIG) && \
			    ((vt) -> tag == NULL))
#endif
#ifndef HAS_VARTAGS
#define HAS_VARTAGS(obj) ((obj) && ((obj) -> __o_vartags!=NULL) && \
			((obj) -> __o_vartags -> sig==VARTAG_SIG))
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

#if 0
/* Moved to rt_obj.c and rt_methd.c */
static inline VARENTRY *last_varentry (VARENTRY *start) {
  VARENTRY *v;
  for (v = start; v -> next; v = v -> next)
    ;
  return v;
}
#endif

#include "list.h"

typedef struct _new_method NEWMETHOD;

struct _new_method {
  METHOD *method;
  METHOD *argblks[MAXARGS+1];
  LIST *templates;
  int argblk_ptr;
  int n_param_newlines;
  int source_line;
  char *source_file;
};

typedef struct _subscript SUBSCRIPT;
struct _subscript {
  int start_idx,
    end_idx;
  int block_start_idx,
    block_end_idx;
  OBJECT *obj;
};

/*
 *  Used in lib/rtobjref.c functions.
 */
typedef OBJECT** OBJREF_T;

/*
 *  Used to dereference arguments to the lib/objref.c function calls.
 */
#define OBJREF(__o) (&(__o))

typedef enum {
  m_null_context,
  m_obj_context,
  m_bool_context,
  m_int_context,
  m_long_context,
  m_double_context,
  m_char_context,
  m_char_ptr_context
} METHOD_C_CONTEXT;

#define MAX_USER_OBJECT_RESOURCES 8192

/* The macros don't work with all GCC versions. 
   So just use the C function versions.  These are generally only
   called when the program exits, anyway.
*/ 

#define DELETE_OBJECT_LIST delete_object_list_internal

#if 0
#define DELETE_OBJECT_LIST(__l) \
  { OBJECT *__o, *__o_prev; \
    for (__o = (__l); \
	 __o && __o -> next; \
	 __o = __o -> next) { \
      if (!__o -> next || !IS_OBJECT(__o->next))\
	break;\
    } \
    if (__o == (__l)) { \
      if (__l) __ctalkDeleteObject (__l); \
      (__l) = NULL; \
    } else { \
      while (__o != (__l)) { \
        __o_prev = __o -> prev; \
        if (__o) __ctalkDeleteObject ( __o); \
        __o = __o_prev; \
        if (!__o) break; \
        __o -> next = NULL; \
      } \
      if (__l) { __ctalkDeleteObject (__l); (__l) = NULL; } \
    } \
  } \

#endif

#define DELETE_VAR_LIST(__l) \
  { VARENTRY *__v, *__v_prev; \
    for (__v = (__l); __v && __v -> var_object && IS_OBJECT(__v -> var_object) && __v -> next; __v = __v -> next) \
      ; \
    if (__v == (__l)) { \
      if (__l && __l -> var_object) {\
	__objRefCntZero (OBJREF(__l -> var_object));	\
	__ctalkDeleteObject (__l -> var_object);	\
      } \
      delete_varentry (__l); \
      (__l) = NULL; \
    } else { \
      while (__v != (__l)) { \
        __v_prev = __v -> prev; \
        if (__v && __v -> var_object) { 		      \
	__objRefCntZero (OBJREF (__v -> var_object)); \
	__ctalkDeleteObject ( __v -> var_object);     \
	} \
        delete_varentry (__v); \
        __v = __v_prev; \
        if (!__v) break; \
        __v -> next = NULL; \
      } \
      if (__l && __l -> var_object) \
	{ __objRefCntZero (OBJREF(__l -> var_object)); \
	  __ctalkDeleteObject (__l -> var_object);     \
         delete_varentry (__l); \
       } \
    } \
  } \

/* Used by fn_param_declarations (), records the start and end
   stack indexes of each parameter, so we don't have to retokenize
   the parameter again when we evaluate it. */
typedef struct _paramlist_entry PARAMLIST_ENTRY;

struct _paramlist_entry {
  char buf[MAXLABEL];
  int start_ptr, end_ptr;
};

/* If we want to delete CVARs in the final term of a compound statement,
   use one of these as the argument to __ctalkToCInteger. */

#define OBJTOC_OBJECT_DELETE  0
#define OBJTOC_OBJECT_KEEP    (1 << 0)
#define OBJTOC_DELETE_CVARS   (1 << 1)

/*
 *  Sets/gets back pointers of objects stored in a method's
 *  user object pool.  Because objects' instance_methods and
 *  class_methods members are only used by class objects, we
 *  can re-purpose them for this use in individual, instantiated
 *  objects.  This allows us to access the pool data whenever
 *  we want to clear an object's entry from a method pool 
 *  whenever we want to delete an object in some other routine,
 *  like when we instantiate new, local objects whenever we call
 *  a method.
 */
#define POOL_SET_METHOD_P(o,m) (o -> instance_methods = m)
#define POOL_SET_RT_FN_P(o,r) (o -> instance_methods = (METHOD *)r)
#define POOL_SET_LINK_P(o,l) (o -> class_methods = (METHOD *)l)
#define POOL_METHOD_P(o)(o -> instance_methods)
#define POOL_RT_FN_P(o)((RT_INFO *)o -> instance_methods)
#define POOL_LINK_P(o)((LIST *)o -> class_methods)

#endif   /* _CTALK_LIB */

#endif   /* _OBJECT_H */

