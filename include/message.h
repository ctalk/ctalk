/* $Id: message.h,v 1.2 2021/01/10 14:51:59 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015, 2018 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _MESSAGE_H
#define _MESSAGE_H

/*
 *  Attributes for messages.
 */

#ifdef LIB_BUILD
/* 
 *  Attributes for the run time libraries.
 */
#define RT_OBJ_IS_INSTANCE_VAR             (1 << 0)
#define RT_VALUE_OBJ_IS_INSTANCE_VAR       (1 << 1)
#define RT_OBJ_IS_CLASS_VAR                (1 << 2)
#define RT_VALUE_OBJ_IS_CLASS_VAR          (1 << 3)
#define RT_CVAR_AGGREGATE_TOK              (1 << 4)
#define RT_TOK_OBJ_IS_CREATED_PARAM        (1 << 5)
#define RT_TOK_VALUE_OBJ_IS_RECEIVER       (1 << 6)
#define RT_TOK_VALUE_OBJ_IS_SUPERCLASS     (1 << 7)
#define RT_TOK_OBJ_IS_ARG_W_FOLLOWING_EXPR (1 << 8)
#define RT_TOK_IS_AGGREGATE_MEMBER_LABEL   (1 << 9)
#define RT_TOK_IS_TYPECAST_EXPR            (1 << 10) /*==TOK_IS_TYPECAST_EXPR*/
/*
 *  When a ++ or -- tok gets updated to METHODMSGLABEL, 
 *  add this attribute to the message.  Also affects which
 *  object is the receiver object of subsequent postfix ops.
 */
#define RT_TOK_IS_POSTFIX_MODIFIED         (1 << 11)
#define RT_DATA_IS_NR_ARGS_DECLARED        (1 << 12)
#define RT_TOK_VALUE_OBJ_IS_NULL_RESULT    (1 << 13)
#define RT_TOK_HAS_LVAL_PTR_CX             (1 << 14)
#define RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS   (1 << 15)
#define RT_TOK_IS_PREFIX_OPERATOR          (1 << 16) /*==TOK_IS_PREFIX_OP...*/
#define RT_TOK_IS_VARTAB_ID                (1 << 17)
#define RT_TOK_IS_SELF_KEYWORD             (1 << 19)
#define RT_TOK_IS_PRINTF_ARG               (1 << 21) /* ==TOK_IS_PRINTF_ARG */
#define RT_DATA_IS_CACHED_METHOD           (1 << 22)

#define TOK_IS_MEMBER_VAR(m) (((m) -> attrs & RT_OBJ_IS_INSTANCE_VAR) || \
			      ((m) -> attrs & RT_OBJ_IS_CLASS_VAR))
#define M_OBJ_IS_VAR(m) (((m) -> attrs & RT_OBJ_IS_INSTANCE_VAR) || \
			 ((m) -> attrs & RT_OBJ_IS_CLASS_VAR))
#define M_VALUE_OBJ_IS_VAR(m) (((m) -> attrs & RT_VALUE_OBJ_IS_INSTANCE_VAR)||\
			       ((m) -> attrs & RT_VALUE_OBJ_IS_CLASS_VAR))
#define M_VALUE_OBJ_IS_SUPERCLASS(m) ((m) -> attrs & \
				      RT_TOK_VALUE_OBJ_IS_SUPERCLASS)

#else  /* LIB_BUILD */
/*
 *  Message attributes for the compiler.
 */
#define OBJ_IS_INSTANCE_VAR            (1 << 0)
#define VALUE_OBJ_IS_INSTANCE_VAR      (1 << 1)
#define OBJ_IS_CLASS_VAR               (1 << 2)
#define VALUE_OBJ_IS_CLASS_VAR         (1 << 3)
#define TOK_IS_UNRESOLVED              (1 << 4)
#define TOK_IS_METHOD_ARG              (1 << 5)
#define TOK_IS_RT_EXPR                 (1 << 6)
#define RCVR_OBJ_IS_SUBEXPR            (1 << 7)
#define RCVR_OBJ_IS_CONSTANT           (1 << 8)
#define TOK_IS_POSTFIX_OPERATOR        (1 << 9)
#define TOK_IS_TYPECAST_EXPR           (1 << 10) /*==RT_TOK_IS_TYPECAST_EXPR*/
#define RCVR_OBJ_IS_C_ARRAY_ELEMENT    (1 << 11)
#define RCVR_TOK_IS_C_STRUCT_EXPR      (1 << 12)
#define RCVR_OBJ_IS_C_STRUCT_ELEMENT   (1 << 13)
#define TOK_IS_DECLARED_C_VAR          (1 << 14)
#define TOK_CVAR_REGISTRY_IS_OUTPUT    (1 << 15)
#define TOK_IS_PREFIX_OPERATOR         (1 << 16) /*==RT_TOK_IS_PREFIX_OP...*/
#define TOK_IS_CLASS_TYPECAST          (1 << 17)
#define TOK_IS_INSTANCE_METHOD_KEYWORD (1 << 18)
#define TOK_IS_CLASS_METHOD_KEYWORD    (1 << 19)
#define TOK_OBJ_IS_CREATED_CVAR_ALIAS  (1 << 20)
#define TOK_IS_PRINTF_ARG              (1 << 21) /*==RT_TOK_IS_PRINTF_ARG */
#define OBJ_IS_SINGLE_TOK_ARG_ACCESSOR (1 << 22)
#define TOK_IS_FN_START                (1 << 23)
#define TOK_SELF                       (1 << 24)
#define TOK_SUPER                      (1 << 25)
#define TOK_IS_TMP_FN_RESULT           (1 << 26)
#define TOK_HAS_CVARTAB_AGG_WRAPPER    (1 << 27)

#define TOK_IS_MEMBER_VAR(m) (((m) -> attrs & OBJ_IS_INSTANCE_VAR) ||	\
			      ((m) -> attrs & OBJ_IS_CLASS_VAR))
#define M_OBJ_IS_VAR(m) (((m) -> attrs & OBJ_IS_INSTANCE_VAR) || \
   ((m) -> attrs & OBJ_IS_CLASS_VAR))
#define M_VALUE_OBJ_IS_VAR(m) (((m) -> attrs & VALUE_OBJ_IS_INSTANCE_VAR) || \
   ((m) -> attrs & VALUE_OBJ_IS_CLASS_VAR))

#endif /* LIB_BUILD */

/*
 *  When changing this, also change in ctalklib, ctldjgpp, and object.h. 
 */
#ifndef __llvm__
#define GNUC_PACKED_STRUCT (defined(__linux__) && defined(__i386__) &&	\
			    defined(__GNUC__) && (__GNUC__ >= 3))
#endif

/* When changing this definition, ensure that copy_message ()
   is still valid. */

typedef struct _message MESSAGE;

struct _message {
  int sig;
  char *name;
  char *value;
  OBJECT *value_obj;
  OBJECT *obj;
  int tokentype; 
  int evaled;
  int output;
  int error_line;
  int error_column;
  long long int attrs;
  long int attr_data;
  long int attr_method;
  OBJECT *receiver_obj;
  struct _message *receiver_msg;
#if GNUC_PACKED_STRUCT
} __attribute__ ((packed));
#else
};
#endif


/* Attributes used by re_lexical, and  __ctalkMatchText. */
/* Also defined in regex.h. */
#ifndef META_BROPEN
#define META_BROPEN             (1 << 0)
#endif
#ifndef META_BRCLOSE
#define META_BRCLOSE            (1 << 1)
#endif
#ifndef META_CHAR_CLASS
#define META_CHAR_CLASS         (1 << 2)
#endif
#ifndef META_CHAR_LITERAL_ESC
#define META_CHAR_LITERAL_ESC   (1 << 3)
#endif

#define MINIMUM_MESSAGE_HANDLING

#ifndef __need_MESSAGE_STACK
#define __need_MESSAGE_STACK
typedef MESSAGE ** MESSAGE_STACK;
#endif

typedef struct _argstr ARGSTR;

struct _argstr {
  char *arg;
  MESSAGE_STACK m_s;
  int start_idx, end_idx;
  bool leading_typecast;
  bool class_typecast;
  char *typecast_expr;
  int typecast_start_idx, typecast_end_idx;
  int class_typecast_start_idx, class_typecast_end_idx;
};

typedef struct _msinfo {
  MESSAGE_STACK messages;
  int stack_start;
  int stack_ptr;
  int tok;
} MSINFO;

#endif   /* _MESSAGE_H */


