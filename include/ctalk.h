/* $Id: ctalk.h,v 1.6 2020/06/04 01:20:06 rkiesling Exp $ -*-Fundamental-*- */

/*
  This file is part of Ctalk.
  Copyright © 2005-2020 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _CTALK_H
#define _CTALK_H

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

#ifndef _STDINT_H
#include <stdint.h>
#endif

#ifndef _STDARG_H
#include <stdarg.h>
#endif

#ifndef _STDIO_H
#include <stdio.h>
#endif

#ifndef _PARSER_H
#include "parser.h"
#endif

#ifndef _FRAME_H
#include "frame.h"
#endif

#ifndef _CVAR_H
#include "cvar.h"
#endif

#ifndef _EXCEPT_H
#include "except.h"
#endif

#ifndef _LEX_H
#include "lex.h"
#endif

#ifndef _LIST_H
#include "list.h"
#endif

#ifndef _OPTION_H
#include "option.h"
#endif

#ifndef _CLASSLIB_H
#include "classlib.h"
#endif

#ifndef _OBJECT_H
#include "object.h"
#endif

#ifndef __BUFMTHD_H
#include "bufmthd.h"
#endif

#ifndef _CTRLBLK_H
#include "ctrlblk.h"
#endif

#ifndef _PENDING_H
#include "pending.h"
#endif

#ifndef _RTINFO_H
#include "rtinfo.h"
#endif

#ifndef _RT_EXPR_H
#include "rt_expr.h"
#endif

#ifndef _SYMBOL_H
#include "symbol.h"
#endif

#ifndef _DEFCLS_H
#include "defcls.h"
#endif

#ifndef _FNTMPL_H
#include "fntmpl.h"
#endif

#ifndef _STDARGCALL_H
#include "stdargcall.h"
#endif

#ifndef __OBJTOC_H
#include "objtoc.h"
#endif

#ifndef _ARGBLK_H
#include "argblk.h"
#endif

#ifndef _STRS_H
#include  "strs.h"
#endif

#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
#include <X11/Xlib.h>
#endif

#ifndef _CHASH_H
# ifndef N_HASH_BUCKETS
# define N_HASH_BUCKETS 0xfff
# endif
# include "chash.h"
#endif

#ifndef MAXLABEL
#define MAXLABEL 0x100
#endif


#ifndef MAXMSG
#define MAXMSG 8192
#endif

#define SUCCESS 0
#define ERROR -1

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !(FALSE)
#endif

#ifndef MAXARGS
#define MAXARGS 512
#endif

#ifdef __SIZEOF_INT__
#define INTBUFSIZE __SIZEOF_INT__ + 1
#else
#warning __SIZEOF_INT__ not defined.  You might need to check your compiler documentation.
#define INTBUFSIZE 5
#endif
#define BOOLBUFSIZE INTBUFSIZE

#ifdef __SIZEOF_LONG_LONG__
#define LLBUFSIZE __SIZEOF_LONG_LONG__ + 1
#else
#warning __SIZEOF_LONG_LONG__ not defined.  You might need to check your compiler documentation.
#define LLBUFSIZE 9
#endif

#ifdef __SIZEOF_POINTER__
#define PTRBUFSIZE __SIZEOF_POINTER__ + 1
#else
#warning __SIZEOF_POINTER__ not defined.  You might need to check your compiler documentation.
#ifdef __x86_64
#define PTRBUFSIZE 9
#else
#define PTRBUFSIZE 5
#endif
#endif


/*
 * Used in __ctalkDeleteObjectInternal () for levels of cleanup when
 * the rt needs to get rid of an object - mostly used for initializing
 * local objects in methods.
 */
#ifndef MAX_CLEANUP_LEVELS
#define MAX_CLEANUP_LEVELS (MAXARGS * 10)
#endif


#ifndef MAXUSERDIRS
#define MAXUSERDIRS MAXARGS
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 4096
#endif

/*
 *  Using a return_valptrs list is slower and less reliable, but it
 *  may be necessary if the app contains a very large amount of data 
 *  in its return values.
 */
/* #define RETURN_VALPTRS 1 */

/*
 *  Disk block size for reading and writing streams.  Linux uses
 *  1K blocks, and this should work nearly as well with other
 *  OS block sizes, although without optimization.
 */

#ifndef IO_BLKSIZE
#define IO_BLKSIZE 1024
#endif

#ifdef __DJGPP__
# ifndef S_SPLINT_S
# define SNPRINTF(_buf,_size,_fmt,...) sprintf (_buf,_fmt,__VA_ARGS__)
# else
# define SNPRINTF(_buf,_size,_fmt) 
# endif
#else
#define SNPRINTF(_buf,_size,_fmt,...) snprintf (_buf,_size,_fmt,__VA_ARGS__)
#endif

#if defined(__DJGPP__) || defined(__CYGWIN__)
#define CTPP_BIN "ctpp.exe"
#define FILE_WRITE_MODE "w"
#define FILE_READ_MODE "r"
#define FILE_APPEND_MODE "a"
#else
/*
 *  Include the "b" suffix for UNIX systems that
 *  normally perform text-mode file I/O.
 */
#define CTPP_BIN "ctpp"
#define FILE_WRITE_MODE "wb"
#define FILE_READ_MODE "rb"
#define FILE_APPEND_MODE "ab"
#endif

/*
 *  Used in class libraries also, so change the definitions there
 *  also if necessary.  Used mainly for *scanf.  The *printf 
 *  function formats are compatible using the "%#x" format.
 */
#define FMT_0XHEX(__p) "%#x",((unsigned int)__p)

#ifndef N_MESSAGES 
#define N_MESSAGES (MAXARGS * 120)
#endif

#ifndef N_VAR_MESSAGES
#define N_VAR_MESSAGES (N_MESSAGES * 100)
#endif

/* Used in mthdref.c. */
#ifndef N_R_MESSAGES
#define N_R_MESSAGES (N_MESSAGES * 100)
#endif

#ifndef P_MESSAGES
#define P_MESSAGES (N_MESSAGES * 30)  /* Enough to include all ISO headers. */
#endif

#ifndef __need_MESSAGE_STACK
#define __need_MESSAGE_STACK
typedef MESSAGE ** MESSAGE_STACK;
#endif

#ifndef NULL
#define NULL ((void *)0)
#define NULL_LENGTH 11
#endif

#ifndef NULLSTR
#define NULLSTR "(null)"
#define NULLSTR_LENGTH 6
#endif

/* Used by xfree<n> */
#ifndef MEMADDR
#define MEMADDR(x) ((void **)&(x))
#endif

#define STR_VAL(s) _STR_VAL(s)
#define _STR_VAL(s) #s

#define EMPTY_STR(s) (((s) == NULL) || (*(s) == 0))

#define IS_C_LABEL(s) (((*s) >= 'a' && (*s) <= 'z') || \
	 ((*s) == '_') || ((*s) >= 'A' && (*s) <= 'Z'))

#ifndef __sparc__
#define IS_OBJECT(x) (((x) != NULL) && (*(int*)(x) == 0xd3d3d3))
#else
#define IS_OBJECT(x) ((x) && !memcmp ((void *)x, "OBJECT", 6))
#endif
/* 
   Define these if you still want to identify classes in the run-time
   library by the __o_classname and __o_superclassname strings. 
   (The compiler still uses them, though.) 
*/

/* #define USE_CLASSNAME_STR
#define USE_SUPERCLASSNAME_STR */
#define CLASSNAME __o_class->__o_name
#define SUPERCLASSNAME __o_superclass->__o_name

/* NOTE: Use this only with OBJECT *'s. */
#define _SUPERCLASSNAME(o) ((IS_OBJECT((o)->__o_class) && \
	IS_OBJECT((o)->__o_class->__o_superclass)) ? \
	(o) -> __o_class -> __o_superclass -> __o_name : "")

#define IS_MESSAGE(x) ((x) && (x) -> sig == 0xd2d2d2)
#define IS_HEXSTR(x) (!strncmp (x, "16r", 3))
#define IS_MACRODEF(x) (!memcmp ((void *)x, "MACRODEF", 8))
#define IS_METHOD_MSG(m) (IS_MESSAGE(m) && (m -> tokentype == METHODMSGLABEL))
#define IS_VALUE_INSTANCE_VAR(__o) (IS_OBJECT(__o) && \
	((__o) -> attrs & OBJECT_IS_VALUE_VAR))
#define IS_STRUCT_OR_UNION(c) ((c -> type_attrs & CVAR_TYPE_STRUCT) ||\
	(c -> type_attrs & CVAR_TYPE_UNION) ||\
	(c -> attrs & CVAR_ATTR_STRUCT))
#define VAL_OBJECT(x) ((x -> value && !IS_OBJECT(x -> value)) ? \
                       ((OBJECT *)x -> value) : (OBJECT *)x)
#define M_NAME(m) (m -> name)
#define M_VAL(m)  (m -> value)
#define M_TOK(m) (m -> tokentype)
#define M_OBJ(m) (m -> obj)
#define M_VALUE_OBJ(m) ((IS_OBJECT(m -> value_obj) ? \
                         m -> value_obj : (IS_OBJECT(m -> obj)\
                         ? m -> obj : NULL)))
#define M_SENDER(m) (m -> sender)
#define M_RECEIVER(m) (m -> receiver)

#define ARG_OBJECT(a) (IS_ARG(a) ? (a)->obj : NULL) 
#define ARG_NAME(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->__o_name : NULL) 
#define ARG_CLASSNAME(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->__o_classname : "") 
#define ARG_SUPERCLASSNAME(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->__o_superclassname : "") 
#define ARG_CLASS(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->__o_class : NULL) 
#define ARG_SUPERCLASS(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->__o_superclass : NULL) 
#define ARG_NREFS(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->nrefs : 0) 
#define ARG_SCOPE(a) ((IS_ARG(a) && IS_OBJECT((a)->obj)) ? \
  (a)->obj->scope : 0) 

#define TRIM_LITERAL(s) (substrcpy (s, s, 1, strlen (s) - 2))
#define TRIM_CHAR(c)    ((c[0] == '\'' && c[1]!=0) ? \
			 substrcpy (c,c,1,strlen (c)-2) : c)
#define TRIM_CHAR_BUF(s) \
  { \
    while ((s[0] == '\'')&&(s[1] != '\0')) \
       substrcpy (s, s, 1, strlen (s) - 2); \
  } \
/* If Character constant's value is enclosed in single quotes,
   e.g. 'A', then return the beginning of the actual character. */
#define CHAR_CONSTANT_VALUE(s) ((s)[0] == '\'' ? &(s)[1] : (s))

#define _TOK_OBJ_VALUE_OBJ(__o) ((__o)->instancevars ? (__o)->instancevars : \
			     (__o))

#define METHOD_ARG_MSG_TYPE(m) ((m -> tokentype == OPENPAREN) || \
                                (m -> tokentype == CHAR) || \
                                (m -> tokentype == EQ) || \
                                (m -> tokentype == BOOLEAN_EQ) || \
                                (m -> tokentype == GT) || \
                                (m -> tokentype == GE) || \
                                (m -> tokentype == ASR) || \
                                (m -> tokentype == ASR_ASSIGN) || \
                                (m -> tokentype == LT) || \
                                (m -> tokentype == LE) || \
                                (m -> tokentype == ASL) || \
                                (m -> tokentype == ASL_ASSIGN) || \
                                (m -> tokentype == PLUS) || \
                                (m -> tokentype == PLUS_ASSIGN) || \
                                (m -> tokentype == INCREMENT) || \
                                (m -> tokentype == MINUS) || \
                                (m -> tokentype == MINUS_ASSIGN) || \
                                (m -> tokentype == DECREMENT) || \
                                (m -> tokentype == DEREF) || \
                                (m -> tokentype == MULT) || \
                                (m -> tokentype == MULT_ASSIGN) || \
                                (m -> tokentype == DIVIDE) || \
                                (m -> tokentype == DIV_ASSIGN) || \
                                (m -> tokentype == DIVIDE) || \
                                (m -> tokentype == BIT_AND) || \
                                (m -> tokentype == BIT_AND_ASSIGN) || \
                                (m -> tokentype == BIT_COMP) || \
                                (m -> tokentype == LOG_NEG) || \
                                (m -> tokentype == INEQUALITY) || \
                                (m -> tokentype == BIT_OR) || \
                                (m -> tokentype == BOOLEAN_OR) || \
                                (m -> tokentype == BIT_OR_ASSIGN) || \
                                (m -> tokentype == BIT_XOR) || \
                                (m -> tokentype == BIT_XOR_ASSIGN) || \
                                (m -> tokentype == PERIOD) || \
                                (m -> tokentype == ELLIPSIS) || \
                                (m -> tokentype == LITERALIZE) || \
                                (m -> tokentype == MACRO_CONCAT) || \
                                (m -> tokentype == CONDITIONAL) || \
                                (m -> tokentype == COLON) || \
                                (m -> tokentype == LABEL) || \
                                (m -> tokentype == METHODLABEL) || \
                                (m -> tokentype == LITERAL) || \
                                (m -> tokentype == LITERAL_CHAR) || \
                                (m -> tokentype == INTEGER) || \
                                (m -> tokentype == LONG) || \
                                (m -> tokentype == LONGLONG) || \
                                (m -> tokentype == FLOAT))

#define METHOD_ARG_TERM_MSG_TYPE(m) ((m -> tokentype == SEMICOLON) || \
                                     (m -> tokentype == CLOSEPAREN) || \
                                     (m -> tokentype == ARGSEPARATOR) || \
                                     (m -> tokentype == ARRAYCLOSE))
#define CONSTANT_TOK(m) ((m -> tokentype == LITERAL) || \
                         (m -> tokentype == LITERAL_CHAR) || \
                         (m -> tokentype == INTEGER) || \
                         (m -> tokentype == LONG) || \
                         (m -> tokentype == FLOAT) || \
                         (m -> tokentype == LONGLONG))

/*
 *  Need to include 
 *  extern PARSER *parsers[MAXARGS+1];           
 *  extern int current_parser_ptr;
 *
 *  In the source module.
 */
#ifndef CURRENT_PARSER
#define CURRENT_PARSER (parsers[current_parser_ptr])
#endif
#define THIS_FRAME (frame_at (CURRENT_PARSER -> frame))
#define NEXT_FRAME (frame_at ((CURRENT_PARSER -> frame) - 1))
#define NEXT_FRAME_START (frame_at (parser_at (parser_ptr ()) -> frame - 1) -> message_frame_top)

#define HAVE_FRAMES ((interpreter_pass == parsing_pass) || \
                     (interpreter_pass == library_pass) || \
                     (interpreter_pass == method_pass) || \
                     (interpreter_pass == expr_check) || \
                     (interpreter_pass == c_fn_pass))


#define LINE_SPLICE(messages, i) (((i) < stack_start ((messages))) && \
                                  ((messages)[(i)+1]->name[0] == '\\'))

#define IS_CONSTRUCTOR(m) ((m) && IS_METHOD(m) && !strcmp ((m) -> name, "new"))

#define IS_CONSTRUCTOR_LABEL(__s) (!strcmp ((__s), "new"))

#define IS_PRIMITIVE_METHOD(m) (!strcmp (m -> name, "instanceMethod") || \
                                !strcmp (m -> name, "classMethod") || \
                                !strcmp (m -> name, "instanceVariable") || \
                                !strcmp (m -> name, "classVariable") || \
                                !strcmp (m -> name, "class"))

#define MAIN_ARGS "int argc, char **argv, char **envp"

#define VAR_IS_STRUCT(v) (!strcmp (v -> type, "struct") || \
                          !strcmp (v -> qualifier, "struct") || \
                          !strcmp (v -> type, "union") || \
                          !strcmp (v -> qualifier, "union"))

#define VAR_IS_FN(v) ((v -> attrs & CVAR_ATTR_FN_DECL) || \
                      (v -> attrs & CVAR_ATTR_FN_PTR_DECL) || \
                      (v -> attrs & CVAR_ATTR_FN_PROTOTYPE))

#define EXPR_CHECK (interpreter_pass == expr_check)

/*
 *  Value to return if a function is the argument of another function
 *  and doesn't need a ctoobj translation.
 */
#define FN_IS_ARG_RETURN_VAL 1
/* 
 *  Value to return if the function is a method argument.
 */
#define FN_IS_METHOD_ARG 2

#ifdef __GNUC__
# ifdef __APPLE__
#  ifdef __ppc__
#  define IS_DEFINED_LABEL(s) (is_c_keyword(s) || \
                       is_ctalk_keyword(s) || \
                       is_gnu_extension_keyword(s) || \
                       global_var_is_declared(s) || \
                       get_local_var(s) || \
                       get_class_variable(s, NULL, FALSE) || \
                       get_function(s) || \
                       typedef_is_declared(s) || \
                       is_enum_member(s) || \
                       is_struct_member(s) || \
                       is_gnuc_builtin_type (s) || \
                       is_apple_ppc_math_builtin (s) || \
		       is_darwin_ctype_fn (s) || \
                       is_fn_param(s))
#  else /* __ppc__ */    /* (i.e., __i386__) */
#  define IS_DEFINED_LABEL(s) (is_c_keyword(s) || \
                       is_ctalk_keyword(s) || \
                       is_gnu_extension_keyword(s) || \
                       global_var_is_declared(s) || \
                       get_local_var(s) || \
                       get_class_variable(s, NULL, FALSE) || \
                       get_function(s) || \
                       typedef_is_declared(s) || \
                       is_enum_member(s) || \
                       is_struct_member(s) || \
                       is_gnuc_builtin_type (s) || \
                       is_apple_i386_math_builtin (s) || \
		       is_darwin_ctype_fn (s) || \
                       is_fn_param(s))
#  endif
# else /* __APPLE__ */
# define IS_DEFINED_LABEL(s) (is_c_keyword(s) || \
                       is_ctalk_keyword(s) || \
                       is_gnu_extension_keyword(s) || \
                       global_var_is_declared(s) || \
                       get_local_var(s) || \
                       get_class_variable(s, NULL, FALSE) || \
                       get_function(s) || \
                       typedef_is_declared(s) || \
                       is_enum_member(s) || \
                       is_struct_member(s) || \
                       is_gnuc_builtin_type (s) || \
                       is_fn_param(s))
# endif /* __APPLE__ */
#else /* __GNUC__ */
#define IS_DEFINED_LABEL(s) (is_c_keyword(s) || \
                       is_ctalk_keyword(s) || \
                       is_gnu_extension_keyword(s) || \
                       get_global_var(s) || \
                       get_local_var(s) || \
                       get_class_variable(s, NULL, FALSE) || \
                       get_function(s) || \
                       get_typedef(s) || \
                       is_enum_member(s) || \
                       is_struct_member(s) || \
                       is_fn_param(s))
#endif /* __GNUC__ */

# define IS_DEFINED_RECEIVER(s) (global_var_is_declared(s) || \
                       get_local_var(s) || \
		       str_eq (s, "self") || str_eq (s, "super") || \
                       get_class_variable(s, NULL, FALSE) || \
                       is_struct_member(s) || \
		       is_method_parameter_s (s) || \
                       is_fn_param(s))

#define IS_DEFINED_FN(s) (get_function(s) || \
                          template_name(s) || \
                          ctalk_lib_fn_name(s,0))

/*
 *  Every char in escape sequences defined by C99 that 
 *  we don't want to escape the backslash in front of.
 */
#define ESC_SEQUENCE_CHAR(c) ((c == '?') || \
			      (c == 'a') || \
			      (c == 'b') || \
			      (c == 'f') || \
			      (c == 'n') || \
			      (c == 'r') || \
			      (c == 't') || \
			      (c == 'u') || \
			      (c == 'U') || \
			      (c == 'v') || \
			      (c == 'x') || \
			      (c == '0') || \
			      (c == '1') || \
			      (c == '2') || \
			      (c == '3') || \
			      (c == '4') || \
			      (c == '5') || \
			      (c == '6') || \
			      (c == '7'))

#define REGISTER_C_METHOD_ARG "__ctalk_register_c_method_arg"
#define REGISTER_C_METHOD_ARG_B "__ctalk_register_c_method_arg_b"
#define REGISTER_C_METHOD_ARG_C "__ctalk_register_c_method_arg_c"
#define REGISTER_C_METHOD_ARG_D "__ctalk_register_c_method_arg_d"
#define METHOD_ARG_ACCESSOR_FN "__ctalk_arg_internal"
#define METHOD_ARG_ACCESSOR_FN_LENGTH 20
#define METHOD_ARG_VALUE_ACCESSOR_FN "__ctalk_arg_value_internal"
#define EVAL_EXPR_FN           "__ctalkEvalExpr"
#define EVAL_EXPR_FN_LENGTH 15
#define EVAL_EXPR_FN_U          "__ctalkEvalExprU"
#define SELF_ACCESSOR_FN       "__ctalk_self_internal"
#define SELF_VALUE_ACCESSOR_FN       "__ctalk_self_internal_value"
#define SRC_FILE_SAVE_FN       "__ctalkRtSaveSourceFileName"
#define ARGBLK_ENTER_SCOPE_FN  "__ctalkEnterArgBlockScope"
#define ARGBLK_EXIT_SCOPE_FN   "__ctalkExitArgBlockScope"
#define ARGBLK_RETURN_STMT     "return ((void *)0)"
#define BOOL_CONSTANT_RETURN_FN "__ctalkRegisterBoolReturn"
#define INT_CONSTANT_RETURN_FN "__ctalkRegisterIntReturn"
#define CHAR_PTR_CONSTANT_RETURN_FN "__ctalkRegisterCharPtrReturn"
#define CHAR_CONSTANT_RETURN_FN "__ctalkRegisterCharReturn"
#define FLOAT_CONSTANT_RETURN_FN "__ctalkRegisterFloatReturn"
#define LLINT_CONSTANT_RETURN_FN "__ctalkRegisterLongLongIntReturn"
#define OBJECT_TOK_RETURN_FN	 "__ctalk_get_object"
#define CVAR_TOK_RETURN_FN	 "__ctalkSaveCVARResource"
#define CVAR_ARRAY_TOK_RETURN_FN "__ctalkSaveCVARArrayResource"
#define OBJECT_MBR_RETURN_FN	 "__ctalkSaveOBJECTMemberResource"
#define FIXUPROOT                "@ctalkfixup"
#define CONSTRUCTOR_ARG_FN       "__ctalkCreateArgA"
#define CONSTRUCTOR_METHOD       "new"
#ifdef __x86_64
#define ALT_PTR_FMT_CAST         "(unsigned long int)"
#else
#define ALT_PTR_FMT_CAST         "(unsigned int)"
#endif
#define TMP_FN_BLK_PFX           "__tmp_fn_blk_"
#define TMP_FN_BLK_PFX_LENGTH    12
#define REGISTER_USER_FN              "__ctalkRegisterUserFunctionName"

#define SUBSCRIPT_VAR_FMT  "__ctsubscript_%d"
#define TMP_SUBSCRIPT_FMT  "%s = __ctalk_to_c_int (__ctalkEvalExpr (\"%s\"));"
#define TMPL_CVAR_ACCESSOR_FN "__ctalkGetTemplateCallerCVAR"
#define TMPL_CVAR_CLEANUP_FN  "__ctalkTemplateCallerCVARCleanup"
#define ARGBLK_LABEL "__ctblk"

#define DELETE_MESSAGES(__s,__i,__l) \
   while (++__i <= __l) {\
     delete_message (__s[__i]);\
     __s[__i] = NULL;\
   }\
   __i = __l;\

#define REUSE_MESSAGES(__s,__i,__l) \
   while (++__i <= __l) {\
     reuse_message (__s[__i]);\
     __s[__i] = NULL;\
   }\
   __i = __l;\

#define TEMPLATE_CACHE_FMT "%s %s"

#define PREFIX_METHOD_ALIAS_SUFFIX "<>"

#define IS_CLASS_OF(s,c) (!strcmp ((s), (c)) ||	\
                               is_subclass_of((s), (c)))

#define IS_COLLECTION_SUBCLASS_OBJ(o) \
  (!strcmp ((o)->__o_name, COLLECTION_CLASSNAME) || \
   is_subclass_of ((o)->__o_name, COLLECTION_CLASSNAME))

#define IS_STREAM_SUBCLASS_OBJ(o) \
  (!strcmp ((o)->__o_name, STREAM_CLASSNAME) || \
     is_subclass_of ((o)->__o_name, STREAM_CLASSNAME))

/*
  Template stuff
 */
#ifndef TEMPLATE_CACHE_FMT
#define TEMPLATE_CACHE_FMT "%s %s"
#endif
#ifndef CLIBDIR
#define CLIBDIR "libc"
#endif
#ifndef USERTEMPLATEDIR
#define USERTEMPLATEDIR "templates"
#endif
#ifndef USERTEMPLATEFILE
#define USERTEMPLATEFILE "fnnames"
#endif
#ifndef USERDIR
#define USERDIR ".ctalk"
#endif

/*
 *  WHITESPACE tokens include the characters of isspace(3),
 *  except for newlines (\n and \r), which have their own token 
 *  type.
 */
#define M_ISSPACE(m) (((m) -> tokentype == WHITESPACE) || \
                      ((m) -> tokentype == NEWLINE))

#define DEFAULT_LIBC_RETURNCLASS INTEGER_CLASSNAME

#define PARAM_C_ARG_ATTR  "__c_arg__"
#define PARAM_PREFIX_METHOD_ATTR  "__prefix__"

#define STRCAT2(s,s1,s2) strcat (s, (s1)); strcat (s,(s2));

#define IS_CLASS_OBJECT(__o) ((__o) && ((__o)->__o_class==(__o)))
#define HAS_INSTANCEVAR_CLASS(__o) (__o && __o->__o_p_obj && \
                                    (__o->__o_class == \
                                     __o->__o_p_obj->__o_class))


#define TOK_HAS_CLASS_TYPECAST(m) \
  (m -> receiver_msg && \
  (m -> receiver_msg -> attrs & TOK_IS_CLASS_TYPECAST))
/*
 *  The Solaris index and rindex are interchangeable
 *  with strchr and strrchr.
 */
#if defined (__sparc__) && defined (__GNUC__)
#define index strchr
#define rindex strrchr
#endif

#ifdef __DJGPP__
#define PATHSEPCHAR ';'
#define DIRSEPCHAR '\\'
#else 
#define PATHSEPCHAR ':'
#define DIRSEPCHAR '/'
#endif

/*
 * Internationalization
 */
#ifndef _
#define _(String) (String)
#endif

#ifndef N_
#define N_(String) (String)
#endif

#ifndef textdomain
#define textdomain(Domain)
#endif

#ifndef bindtextdomain
#define bindtextdomain(Package,Directory)
#endif

#ifndef NEXT_OFFSET
#define NEXT_OFFSET (offsetof (OBJECT, next))
#endif

/* 
 *    Debugging 
 *    Define DEBUG_CODE and any of symbols below.
 */
#define DEBUG_CODE

/* 
 *    #define STACK_TRACE if you want the interpreter to print 
 *    huge amounts of information about the stack values during 
 *    processing.  
 */
#undef STACK_TRACE
/*
 *    ARG_TRACE prints the method argument stack.
 */
#undef ARG_TRACE
/*
 *    #define CLASSLIB_TRACE if you want to watch 
 *    class library searches.
 */
#undef CLASSLIB_TRACE
/*
 *    TRACE_PREPROCESS_INCLUDES prints the include level
 *    of each #included file.
 */
#undef TRACE_PREPROCESS_INCLUDES
/*
 *    CHECK_PREPROCESS_CONDITIONALS prints the level of
 *    nested conditionals before and after every #include
 */
#undef CHECK_PREPROCESS_IF_LEVEL
/* 
 *    DEBUG_SYMBOLS prints information about the symbols
 *    defined by the preprocessor.  If a symbol is not 
 *    defined, print a warning - GNU cpp silently returns
 *    false for undefined symbols.
 */
#undef DEBUG_SYMBOLS
/*
 *    Issue a warning for undefined preprocessor symbols.
 *    GNU cpp returns false without issuing a warning when
 *    evaluating an undefined symbol.
 */
#undef DEBUG_UNDEFINED_PREPROCESSOR_SYMBOLS
/*
 *    DEBUG_TYPEDEFS dumps a list of the typedefs.
 */
#undef DEBUG_TYPEDEFS
/*    DEBUG_C_VARIABLES causes the interpreter to perform more
 *    type checking.
 */
#undef DEBUG_C_VARIABLES
/*
 *    FN_STACK_TRACE prints a stack trace of the function output
 *    buffer.
 */
#undef FN_STACK_TRACE
/*
 *    MALLOC_TRACE enables the glibc mtrace () function.  You
 *    must also set the environment variable MALLOC_TRACE to
 *    the name of the file that will contain the trace data.
 */
#undef MALLOC_TRACE
/*
 *    DEBUG_OBJECT_DESTRUCTION adds extra run-time warnings 
 *    in lib/rtnewobj.c.
 */
#undef DEBUG_OBJECT_DESTRUCTION
/*
 *    DEBUG_DYNAMIC_C_ARGS issues warnings if the run-time 
 *    library encounters a C variable for which a class 
 *    has not yet been loaded.  Basic C types should not
 *    need this warning.
 */
#undef DEBUG_DYNAMIC_C_ARGS
/*
 *    DEBUG_INVALID_OBJ_REFS prints warnings if __objRefCntInc and
 *    __objRefCntDec receive invalid objects.
 */
#undef DEBUG_INVALID_OBJ_REFS
/*
 *    DEBUG_UNDEFINED_PARAMETER_CLASSES prints warnings if a method
 *    tries to use a parameter for which a class has not yet been
 *    defined.
 */
#undef DEBUG_UNDEFINED_PARAMETER_CLASSES
/*
 *    MEM_WARNINGS can print all sort of information about memory
 *    issues.
 */
#define MEM_WARNINGS

/* This should only need to be defined when diagnosing the compiler. */
/* #define SYMBOL_SIZE_CHECK */

/* Prototypes */

/* arg.c */
int arg_is_constant (MESSAGE_STACK, int, int);
int c_param_expr_arg (MSINFO *);
int complex_arglist_limit (MESSAGE_STACK, int);
int compound_method_limit (MESSAGE_STACK, int, int, int, int);
ARG *create_arg (void);
ARG *create_arg_init (OBJECT *);
void delete_arg (ARG *);
char *fmt_c_fn_obj_args_expr (MESSAGE_STACK, int, char *);
int method_expr_is_c_fmt_arg (MESSAGE_STACK, int, int, int);
int method_arg_limit (MESSAGE_STACK, int);
int method_arglist_limit (MESSAGE_STACK, int, int, int);
int method_arglist_limit_2 (MESSAGE_STACK, int, int, int, int);
int method_arglist_n_args (MESSAGE_STACK, int);
OBJECT *fn_arg_expression (OBJECT *, METHOD *, MESSAGE_STACK, int);
char *obj_arg_to_c_expr (MESSAGE_STACK, int, OBJECT *, int);
int obj_expr_is_fn_arg (MESSAGE_STACK, int, int, int *);
int obj_expr_is_fn_arg_ms (MSINFO *, int *);
int resolve_arg2 (MESSAGE_STACK, int);
int split_args_argstr (MESSAGE_STACK, int, int, ARGSTR [], int *);
int split_args_idx (MESSAGE_STACK, int, int, int *, int *);
char *stdarg_fmt_arg_expr (MESSAGE_STACK, int, METHOD *, char *);
char *writable_arg_rt_arg_expr (MESSAGE_STACK, int, int, char *);
char *format_method_arg_accessor (int, char *, bool, char *);

/* argblk.c */
int argblk_end (MESSAGE_STACK, int);
int argblk_message_push (MESSAGE *);
MESSAGE_STACK argblk_message_stack (void);
void argblk_push (ARGBLK *);
ARGBLK *argblk_pop (void);
void buffer_argblk_stmt (char *);
int create_argblk (MESSAGE_STACK, int, int);
int get_argblk_message_ptr (void);
int is_argblk_ctrl_struct (MESSAGE_STACK, int);
int is_argblk_expr (MESSAGE_STACK, int);
int is_argblk_expr2 (MESSAGE_STACK, int);
void init_arg_blocks (void);
ARGBLK *new_arg_blk (void);
int argblk_super_expr (MSINFO *);
ARGBLK *current_argblk (void);
bool argblk_cvar_is_fn_argument (MESSAGE_STACK, int, CVAR *);
void register_argblk_cvar_from_basic_cvar (MESSAGE_STACK, int, int, CVAR *);
char *fmt_register_argblk_cvar_from_basic_cvar (MESSAGE_STACK, int, CVAR *,
                                                char *);
char *fmt_register_argblk_cvar_from_basic_cvar_2 (MESSAGE *,
						CVAR *,
						char *);
int argblk_fp_containers (MESSAGE_STACK, int, CVAR *);

/* argexprchk.c */
OBJECT *arg_expr_object_2 (MESSAGE_STACK messages, int start_idx, int end_idx);

/* ccompat.c */
void ccompat_init (void);
char *find_gcc_target_dir (void);
void gnu_attributes (MESSAGE_STACK, int);
int gnu_cpp_symbols (void);
int init_gcc_paths (void);
void print_libdirs (void);
#ifdef __GNUC__
void gnuc_fix_empty_extern (MESSAGE_STACK, int);
#endif

/* check.c */
void classlib_check (MESSAGE_STACK, int);

/* class.c */
int add_class_object (OBJECT *);
OBJECT *class_object_search (char *, int);
void delete_class_library (void);
OBJECT *get_class_object (char *);
int get_lib_includes_ptr (void);
int init_default_classes (void);
void init_library_include_stack (void);
int is_pending_class (char *);
CLASSLIB *lib_include_at (int ptr);
char *library_pathname (void);
int library_search (char *, int);
CLASSLIB *pop_input_declaration (void);
CLASSLIB *pop_library_include (void);
int prev_include (char *);
void push_input_declaration (char *, char *, int);
void push_library_include (char *, char *, int);
int add_instance_variables (OBJECT *, OBJECT *);
int instance_variables_from_class_definition (OBJECT *);
bool is_subclass_of (char *, char *);
int has_class_declaration (char *, char *);
void save_decl_list (void);
void restore_decl_list (void);

/* collection.c */
int collection_needs_rt_eval (OBJECT *, METHOD *);
int collection_rt_expr (METHOD *, MESSAGE_STACK, int, int, OBJECT_CONTEXT);
COLLECTION_CONTEXT collection_context (MESSAGE_STACK, int);
bool is_collection_initializer (int);

/* complexmethd.c */
char *complex_expr_class (MESSAGE_STACK, int);
int complex_method_statement (OBJECT *, MESSAGE_STACK, int);
int complex_self_method_statement (OBJECT *, MESSAGE_STACK, int);
int complex_var_or_method_message (OBJECT *, MESSAGE_STACK, int);
int is_overloaded_aggregate_op (MESSAGE_STACK, int);
MESSAGE_CLASS message_class (MESSAGE_STACK, int);
void register_argblk_c_vars_1 (MESSAGE_STACK, int, int);
char *fmt_register_argblk_c_vars_1 (MESSAGE_STACK, int, int);
char *fmt_register_argblk_c_vars_2 (MESSAGE *, CVAR *, char *);
void super_argblk_rcvr_expr (MESSAGE_STACK, int, OBJECT *);

/* constrcvr.c */
int handle_cvar_arg_before_terminal_method_a (MESSAGE_STACK, int);
void handle_subexpr_rcvr (int);
int have_subexpr_rcvr_class_postfix (MESSAGE_STACK, int);
int is_aggregate_term_a (MESSAGE_STACK, int);
int is_aggregate_term_b (MESSAGE_STACK, int);
int method_call_constant_tok_expr_a (MESSAGE_STACK, int);
int method_call_subexpr_postfix_a (MESSAGE_STACK, int);
int method_call_subexpr_postfix_b (MESSAGE_STACK, int);
int register_cvar_arg_expr_a (MESSAGE_STACK, int);
int register_cvar_rcvr_expr_a (MESSAGE_STACK, int, CVAR *, char *);
int register_cvar_rcvr_expr_b (MESSAGE_STACK, int);
int set_cvar_rcvr_postfix_attrs_a (MESSAGE_STACK, int, int, CVAR *, char *);
void constrcvr_unhandled_case_warning (MESSAGE_STACK, int);

/* control.c */
/* This is a no-op for now. */
/* int case_stmt (MESSAGE_STACK, int); */
int ctrl_block_state (int);
int control_structure (MESSAGE_STACK, int);
int ctrlblk_cleanup_args (METHOD *,int);
int ctrlblk_method_call (OBJECT *, METHOD *, int);
int ctrlblk_push_args (OBJECT *, METHOD *, OBJECT *);
int ctrlblk_register_c_var (char *, int);
CTRLBLK *ctrlblk_pop (void);
void ctrlblk_push (CTRLBLK *);
int ctrl_blk_state (int);
void delete_control_block (CTRLBLK *);
int do_stmt (MESSAGE_STACK, int);
int if_else_end (MESSAGE_STACK, int);
int for_stmt (MESSAGE_STACK, int);
int get_ctrlblk_ptr (void);
STMT_CLASS get_ctrl_class (char *);
int if_stmt (MESSAGE_STACK, int);
void init_control_blocks (void);
bool is_if_pred_start (MESSAGE_STACK, int);
bool is_if_subexpr_start (MESSAGE_STACK, int);
bool is_while_pred_start (MESSAGE_STACK, int);
bool is_while_pred_start_2 (MESSAGE_STACK, int);
int leading_unary_op (MESSAGE_STACK, int);
void loop_linemarker (MESSAGE_STACK, int);
CTRLBLK *new_ctrl_blk (void);
int require_class (MESSAGE_STACK, int);
int switch_stmt (MESSAGE_STACK, int);
int while_stmt (MESSAGE_STACK, int);
int expr_paren_check (MESSAGE_STACK, int);
int handle_blk_continue_stmt (MESSAGE_STACK, int);
void handle_blk_break_stmt (MESSAGE_STACK, int);
int ctrlblk_pred_start_idx (void);
int ctrlblk_pred_end_idx (void);
bool major_logical_op_check (MESSAGE_STACK, int, int);
int goto_stmt (MESSAGE_STACK, int);
char *handle_rt_prefix_rexpr (MESSAGE_STACK, int, int, int, char *);

/* cparse.c */
MESSAGE_STACK c_message_stack (void);
int escaped_line_end (MESSAGE **, int, int);
int eval_constant_expr (MESSAGE_STACK, int, int *, VAL *);
int find_arg_end (MESSAGE_STACK, int);
int find_declaration_end (MESSAGE_STACK, int, int);
int find_instancevar_declaration_end (MESSAGE_STACK, int);
int find_function_close (MESSAGE_STACK, int, int);
int find_n_subscripts (MESSAGE_STACK, int, int *);
int find_struct_members (MESSAGE_STACK, int);
int find_subscript_declaration_end (MESSAGE_STACK, int);
int find_subscript_initializer_size (MESSAGE_STACK, int);
int fn_closure (MESSAGE_STACK, int, int);
int fn_has_gnu_attribute (MESSAGE_STACK, int);
int fn_is_declaration (MESSAGE_STACK, int);
int fn_param_cvars (MESSAGE_STACK, int, int);
int fn_params (MESSAGE_STACK, int, int);
int get_cvar_attrs (void);
PARAMCVAR get_param_n (int );
int get_param_ptr (void);
bool get_struct_decl (void);
int get_vararg_status (void);
int have_declaration (char *);
int have_ref_prefix (MESSAGE_STACK, int);
int have_type_prefix (MESSAGE_STACK, int);
int initializer_end (MESSAGE_STACK, int);
int is_c_fn_declaration_msg (MESSAGE_STACK, int, int);
bool is_c_function_declaration (char *);
int is_c_function_prototype_declaration (MESSAGE_STACK, int);
int is_c_identifier (char *);
int is_c_prefix_op (MESSAGE_STACK, int);
bool is_c_var_declaration_msg (MESSAGE_STACK, int, int, int);
bool is_c_param_declaration_msg (MESSAGE_STACK, int, int);
int math (MESSAGE *, MATH_OP, VAL *, VAL *, VAL *);
int param_is_fnptr (MESSAGE_STACK, int, int);
int parse_include_directive (MESSAGE_STACK, int, int, INCLUDE *);
void reset_struct_decl (void);
void restore_fn_declaration (void);
void restore_struct_declaration (void);
void save_fn_declaration (MESSAGE_STACK, int);
void save_struct_declaration (MESSAGE_STACK, int);
int typedef_var (MESSAGE_STACK, int, int);
bool param_0_is_void (void);

/* ctoobj.c */
char *fmt_c_to_obj_call (MESSAGE_STACK, int, OBJECT *, METHOD *, OBJECT *,
     char *, ARGSTR *);
char *fmt_c_to_symbol_obj_call (OBJECT *, METHOD *, OBJECT *, char *);
int generate_c_to_obj_call (MESSAGE_STACK, int, OBJECT *,
    METHOD *, OBJECT *);
char *cfunc_return_class (CFUNC *);
int c_char_ptr_to_obj_call (OBJECT *, METHOD *, OBJECT *);
int c_double_to_obj_call (OBJECT *, METHOD *, OBJECT *);
int c_int_to_obj_call (OBJECT *, METHOD *, OBJECT *);
int c_longlong_to_obj_call (OBJECT *, METHOD *, OBJECT *);
char *fn_return_class (char *);

/* cvars.c */
int add_incomplete_type (MESSAGE_STACK, int, int);
int add_typedef_from_cvar (CVAR *);
int add_typedef_from_parser (void);
int add_variable_from_cvar (CVAR *);
void buffer_fn_args (PENDING *, char *);
OBJECT *c_constant_object (MESSAGE_STACK, int);
int function_not_shadowed_by_method (MESSAGE_STACK, int);
int is_global_constructor (MESSAGE_STACK, int);
int is_incomplete_type (char *);
CVAR *is_incomplete_type_2 (char *, char *);
CFUNC *cvar_to_cfunc (CVAR *);
char *declaration_expr_from_cvar (CVAR *, char *);
void delete_local_c_vars (void);
void dump_typedefs (MESSAGE *);
CVAR *get_fn_declaration (char *);
CFUNC *get_function (char *);
CVAR *get_global_struct_defn (char *);
CVAR *get_global_var (char *);
CVAR *get_local_struct_defn (char *);
CVAR *get_local_var (char *);
char *get_type_val2 (void);
int get_params (void);
CVAR *get_struct_from_member (void);
CVAR *get_typedef (char *);
int global_var_is_declared (char *);
CVAR *have_struct (char *);
int typedef_is_declared (char *);
int is_c_derived_type (char *);
bool is_enum_member (char *);
bool is_fn_param (char *);
int is_param_of (CFUNC *, char *);
int is_struct_element (MESSAGE_STACK, int);
bool is_struct_member (char *s);
int is_struct_member_tok (MESSAGE_STACK, int);
int match_type (VAL *, VAL *);
CVAR *parser_to_cvars (void);
int parse_vars (char *);
int parse_vars_and_prototypes (MESSAGE_STACK, int, int);
int resolve_incomplete_types (void);
int struct_def_from_typedef (CVAR *);
CVAR *struct_member_from_expr (char *, CVAR *);
CVAR *struct_member_from_expr_b (MESSAGE_STACK, int, int, CVAR *);
int var_cmp (MESSAGE *, CVAR *, CVAR *);
int var_cmp2 (CVAR *, CVAR *);
int var_message_push (MESSAGE *);
int get_var_messageptr (void);
void unlink_global_cvar (CVAR *);
MESSAGE_STACK var_message_stack (void);
MESSAGE *var_message_stack_at (int);
void init_cvars (void);
CVAR *struct_defn_from_struct_decl (char *);
CVAR *mbr_from_struct_or_union_cvar (CVAR *, char *);
bool is_this_fn_param (char *);
CVAR *is_this_fn_param_2 (char *);

/* cvartab.c */
void method_cvar_tab_entry (MESSAGE_STACK, int, CVAR *);
void function_cvar_tab_entry (MESSAGE_STACK, int, CVAR *);
void function_param_cvar_tab_entry (MESSAGE_STACK, int, CVAR *);
int new_method_has_contain_argblk_attr (void);
int method_cvar_alias_basename (METHOD *, CVAR *, char *);
int function_contains_argblk (MESSAGE_STACK, int);
int function_cvar_alias_basename (CVAR *, char *);
CVAR *get_var_from_cvartab_name (char *);
bool need_cvar_argblk_translation (CVAR *);
OBJECT *handle_cvar_argblk_translation (MESSAGE_STACK, int, int, CVAR *);
OBJECT *argblk_CVAR_name_to_msg (MESSAGE *, CVAR *);

/* c_rval.c */
int format_self_lval_C_expr (MESSAGE_STACK, int);
int format_self_lval_fn_expr (MESSAGE_STACK, int);
int is_self_expr_as_fn_lvalue (MESSAGE *, int, int, int);
int is_self_as_fn_lvalue (MESSAGE *, int, int, int);
int is_self_expr_as_C_expr_lvalue (MESSAGE *, int, int, int);
int obj_is_fn_expr_lvalue (MESSAGE_STACK, int, int);
int format_obj_lval_fn_expr (MESSAGE_STACK, int);
int obj_expr_is_fn_expr_lvalue (MESSAGE_STACK, int, int);
int cvar_rcvr_arg_expr_limit (MESSAGE_STACK, int);
int cvar_array_is_arg_expr_rcvr (MESSAGE_STACK, int);
int cvar_plain_is_arg_expr_rcvr (MESSAGE_STACK, int);
int cvar_struct_is_arg_expr_rcvr (MESSAGE_STACK, int);
int cvar_struct_ptr_is_arg_expr_rcvr (MESSAGE_STACK, int);
char *format_fn_call_method_expr_block (MESSAGE_STACK, int, int *, char *);
void eval_params_inline (MESSAGE_STACK, int, int, int, CFUNC *,
     char *);

/* enum.c */
CVAR *enum_decl (MESSAGE_STACK, int);
char *enum_initializer (MESSAGE_STACK, int, int *, char *);

/* eval_arg.c */
OBJECT *eval_arg (METHOD *, OBJECT *, ARGSTR *, int);

/* expr.c */
int is_constant_expr (MESSAGE_STACK, int);

/* errmsgs.c */
void method_args_wrong_number_of_arguments_1 
     (MESSAGE_STACK, int, METHOD *, METHOD *, char *, I_PASS);
void method_args_wrong_number_of_arguments_2 
     (MESSAGE_STACK, int, METHOD *, METHOD *, char *, int, char *, I_PASS);
void method_args_ambiguous_argument_1 
     (MESSAGE_STACK, int, METHOD *, METHOD *, int, char *, I_PASS);
void undefined_arg_first_label_warning (MESSAGE_STACK, int, int, int,
					OBJECT *, CVAR *);
void self_within_constructor_warning (MESSAGE *);
void arglist_syntax_error_msg (MESSAGE_STACK, int);
void undefined_label_after_instance_variable_warning (MESSAGE_STACK, int);
void rval_fn_extra_tok_warning (MESSAGE_STACK, int, int);
void object_in_initializer_warning (MESSAGE_STACK, int, METHOD *, OBJECT *);
void prev_constant_tok_warning (MESSAGE_STACK, int, int, int);
void still_prototyped_constructor_warning (MESSAGE *, char *, char *);
void invalid_class_variable_declaration_error (MESSAGE_STACK, int);
void var_shadows_method_warning (MESSAGE_STACK messages, int);
void unsupported_return_type_warning_a (MESSAGE_STACK, int);
void object_does_not_understand_msg_warning_a (MESSAGE *, char *,
					       char *, char *);
void undefined_method_follows_class_object (MESSAGE *);
void arg_shadows_a_c_variable_warning (MESSAGE *, char *);
void resolve_undefined_param_class_error (MESSAGE *, char *, char *);
void undefined_self_fmt_arg_class (MESSAGE_STACK, int, int);
void return_expr_undefined_label (MESSAGE_STACK, int, int, int);
void primitive_arg_shadows_a_parameter_warning (MESSAGE_STACK, int);
void assignment_in_constant_expr_warning (MESSAGE_STACK, int, int);
bool postfix_following_method_warning (MESSAGE_STACK, int);
void prefix_inc_before_method_expr_warning (MESSAGE_STACK, int);
bool rcvr_cx_postfix_warning (MESSAGE_STACK, int);
bool is_OBJECT_ptr_array_deref (MESSAGE_STACK, int);
bool subscripted_int_in_code_block_error (MESSAGE_STACK, int, CVAR *);
void object_follows_object_error (MESSAGE_STACK, int, int);
void method_shadows_c_keyword_warning_a (MESSAGE_STACK, int, int);
void self_outside_method_error (MESSAGE_STACK, int);
void object_follows_a_constant_warning (MESSAGE_STACK, int, int);
void unknown_format_conversion_warning_ms (MSINFO *);
void unknown_format_conversion_warning (MESSAGE_STACK, int);
void self_instvar_expr_unknown_label (MESSAGE_STACK, int, int);
void instancevar_wo_rcvr_warning (MESSAGE_STACK, int, bool, int);
char *collect_errmsg_expr (MESSAGE_STACK, int);
void undefined_blk_method_warning (MESSAGE *, MESSAGE *, MESSAGE *);
OBJECT *resolve_rcvr_is_undefined (MESSAGE *, MESSAGE *);


/* error.c */
#ifdef DEBUG_CODE
void debug (char *, ...);
#endif
void error (MESSAGE *, char *, ...);
void fn_check_expr (MESSAGE_STACK, int, int, int);
char *source_filename (void);
void warning (MESSAGE *, char *, ...);
void warning_text (char *, MESSAGE *, char *, ...);
void unresolved_eval_delay_warning_1 (MESSAGE *,
     MESSAGE_STACK, int, char *, char *, char *,char *);
void unresolved_eval_delay_warning_2 (MESSAGE *, char *, char *, 
				      char *, char *, char *);
void warn_unknown_c_type (MESSAGE *, char *);
void method_shadows_instance_variable_warning_1 (MESSAGE *, char *,
        char *, char *, char *);
void undefined_method_exception (MESSAGE *r, MESSAGE *);
bool function_shadows_class_warning_1 (MESSAGE_STACK, int);
void missing_separator_error (MESSAGE_STACK, int);
void warn_unresolved_labels (void);
void ssroe_class_mismatch_warning (MESSAGE *, char *, char *);
#ifdef SYMBOL_SIZE_CHECK
void check_symbol_size (char *);
#endif

/* extern.c */
int extern_declaration (MESSAGE_STACK, int);

/* fn_param.c */
int fn_param_declarations (MESSAGE_STACK, int, int, PARAMLIST_ENTRY **, int *);
int fn_prototype_param_declarations (MESSAGE_STACK, int, int, 
    PARAMLIST_ENTRY**, int *);
bool fn_decl_varargs (void);

/* fn_tmpl.c */
int add_method_return (char *, char *);
int buffer_fn_template (char *);
int c_tmpl_fn_args (MESSAGE_STACK, OBJECT *, int);
void cache_fn_tmpl_name (char *, char *);
void cleanup_template_cache (void);
char *clib_fn_expr (MESSAGE_STACK, int, int, OBJECT *);
char *clib_fn_rt_expr (MESSAGE_STACK, int);
int find_fn_arg_parens (MESSAGE_STACK, int, int *, int *);
char *find_tmpmethodname (char *);
char *fn_tmpl_is_cached (char *);
char *fn_param_prototypes_from_cfunc (CFUNC *, char *);
void generate_fn_template_init (char *, char *);
int get_tmpl_messageptr (void);
void init_c_fn_class (void);
void init_fn_templates (void);
char *preprocess_template (FN_TMPL *);
MESSAGE_STACK tmpl_message_stack (void);
char *template_params (char *, char *);
char *template_return_class (char *);
MESSAGE *tmpl_message_pop (void);
int tmpl_message_push (MESSAGE *);
MESSAGE *tmpl_message_stack_at (int);
int unbuffer_fn_templates (void);
FN_TMPL *cache_template (char *);
void init_fn_template_cache (void);
FN_TMPL *new_fn_template (void);
int preprocess_template_file (char *);
FN_TMPL *get_template (char *);
char *insert_c_methd_return_type (char *, char *);
char *fmt_template_rt_expr (FN_TMPL *, char *);
char *user_fn_expr_args_from_params (MESSAGE_STACK, int, char *);
void template_call_from_CFunction_receiver (MESSAGE_STACK, int);
void premethod_output_template (char *cfn_name);

/* fnbuf.c */
int get_fn_start_line (void);
char *get_fn_name (void);
void buffer_function_statement (char *, int);
void begin_function_buffer (void);
void end_function_buffer (void);
int fn_message_push (MESSAGE *);
int fn_return_at_close (MESSAGE_STACK, int, int);
MESSAGE **fn_message_stack (void);
MESSAGE *fn_message_pop (void);
int get_fn_messageptr (void);
int input_is_c_header (void);
int buffer_output (char *);
int main_args (char *, int);
void function_vartab_statement (char *);
void function_vartab_init_statement (char *);
void unbuffer_function_vartab (void);
bool chk_C_return_alone (MESSAGE_STACK, int);

/* fninit.c */
void generate_fn_return (MESSAGE_STACK, int, int);
void generate_init (int);
void fn_init (char *, int);
void global_init_statement (char *, int);
void output_global_init (void);

/* frame.c */
int delete_frame (void);
FRAME *frame_at (int);
int get_frame_pointer (void);
int init_frames (void);
int is_global_frame ();
int message_frame_top (void);
int message_frame_top_n (int);
int new_frame (int, int);
FRAME *this_frame (void);

/* ifexpr.c */
int ctrlblk_pred_rt_expr_self (MESSAGE_STACK, int);
void parse_ctrlblk_subexprs (MESSAGE_STACK);
char *handle_ctrlblk_subexprs (MESSAGE_STACK, char *);
char *ctrlblk_pred_rt_subexpr (MESSAGE_STACK, int, int, char *, char *);
int ctrlblk_pred_rt_expr (MESSAGE_STACK, int);
void check_major_boolean_parens (MESSAGE_STACK, int);
char *ctblk_eval_major_bool_subexprs (MESSAGE_STACK, int, int);
char *ctblk_handle_rt_c_first_operand_subexpr (MESSAGE_STACK, int, int, int,
                 			     char *);

/* include.c */
char *find_library_include (char *, int);
void init_library_paths (void);
void init_lib_cache_path (void);
void init_user_home_path (void);
char *home_libcache_path (void);
char *ctalklib_path (void);
void init_user_template_path (void);
void get_ctalkdefs_h_lines (void);

/* lib/ansitime.c */
char *mon (int);

/* lib/become.c */
int become_expr_parser_cleanup (OBJECT *, OBJECT *);
int __ctalkIsCallersReceiver (void);
OBJECT *__ctalkGetCallingFnObject (char *, char *);
OBJECT *__ctalkGetCallingMethodObject (char *, char *);
OBJECT *__ctalkCallingFnObjectBecome (OBJECT *, OBJECT *);
OBJECT *__ctalkCallingInstanceVarBecome (OBJECT *, OBJECT *);
OBJECT *__ctalkCallingMethodObjectBecome (OBJECT *, OBJECT *);
OBJECT *__ctalkCallingReceiverBecome (OBJECT *, OBJECT *);
int __ctalkInstanceVarIsCallersReceiver (void);
OBJECT *__ctalkReceiverReceiverBecome (OBJECT *);
OBJECT *__ctalkGlobalObjectBecome (OBJECT *, OBJECT *);
int __ctalkAliasObject (OBJECT *, OBJECT *);
int __ctalkAliasReceiver (OBJECT *, OBJECT *);

/* lib/bin2dec.c */
int ascii_bin_to_dec (char *);

/* lib/bitmap.c */
void *__ctalkX11CreateGC (void *, int);
int __ctalkX11CreatePaneBuffer (OBJECT *, int, int, int);
int __ctalkX11CreatePixmap (void *, int, int, int, int);
void __ctalkX11DeletePixmap (int id);
void __ctalkX11FreeGC (unsigned long int ptr);
int __ctalkX11FreePaneBuffer (OBJECT *);
int __ctalkX11ResizePaneBuffer (OBJECT *, int, int);
int __get_pane_buffers (OBJECT *, int *, int *);

/* lib/bnamecmp.c */
int basename_cmp (const char *, const char *);

/* lib/cclasses.c */
OBJECT *get_class_object (char *);
void cache_class_ptr (OBJECT *);
void init_ct_default_class_cache (void);
void delete_ct_default_class_cache (void);
void get_class_names (char **, int);
bool is_class_name (char *);

/* lib/chash.c */
void _hash_symbol_line (int);
void _hash_symbol_column (int);
void _hash_symbol_source_file (char *);
void _new_hash (HASHTAB *);
HLIST *_new_hlist (void);
void *_hash_first (HASHTAB);
void *_hash_first_obj (HASHTAB);
void *_hash_get (HASHTAB, const char *);
void *_hash_get_obj (HASHTAB h, char *s_key);
void *_hash_next (HASHTAB);
void *_hash_next_obj (HASHTAB);
void  _hash_put (HASHTAB, void *, char *);
int _hash_put_obj (HASHTAB h, void *e, char *s_key, void *);
void *_hash_remove (HASHTAB, const char *);
void _hash_all_initialize (void);
void *_hash_all (HASHTAB);
void _delete_hash (HASHTAB);


/* lib/clibtmpl.c */
char *c99_name (char *, int);
int libc_fn_needs_writable_args (char *);
char *get_clib_template_fn (char *, char *);
char *get_clib_template_file(char *);
char *get_user_template_name (char *);
void cleanup_user_templates (void);
int get_user_template_names (void);
char *template_name (char *);
char *user_template_name (char *);

/* lib/ctatoll.c */
#ifndef HAVE_ATOLL
long long int atoll (const char *);
#endif

/* lib/ctdtoa.c */
char *__ctalkDoubleToASCII (double, char *);
char *__ctalkFloatToASCII (float , char *);
char* __ctalkLongDoubleToASCII (long double, char *);

/* lib/ctitoa.c */
void __reverse(char *);
char *__ctalkDecimalIntegerToASCII (int, char *);
char *ctitoa (int, char *);
char __ctalkDecimalIntegerToChar (int, char *);

/* lib/ctlltoa.c */
char *__ctalkLongLongToDecimalASCII (long long int, char *);
char *__ctalkLongLongToHexASCII (long long int, char *, bool);

/* lib/ctltoa.c */
char *__ctalkLongToDecimalASCII (long int, char *);
char *__ctalkLongToHexASCII (long int, char *, bool);

/* lib/ctoobj.c */
OBJECT *__ctalkCCharPtrToObj (char *);
OBJECT *__ctalkCIntToObj (int);
OBJECT *__ctalkCBoolToObj (bool);
OBJECT *__ctalkCSymbolToObj (void *);
OBJECT *__ctalkCDoubleToObj (double);
OBJECT *__ctalkCLongLongToObj (long long int);
void output_mixed_c_to_obj_arg_block (MESSAGE_STACK, int, ARGSTR *, METHOD *);
void output_mixed_c_to_obj_fmt_arg (MESSAGE_STACK, int, int);
OBJECT *__ctalkCVARReturnClass (CVAR *);
int __ctalkIsObject (void *);

/* lib/ctsleep.c */
void __ctalkSleep (int);

/* lib/cttmpl.c */
char *ctalk_lib_fn_name (char *, int);

/* lib/dblib.c */
void __inspector_trace (int);
void __receiver_trace (int);
void __arg_trace (int);
void __inspect_locals (int);
void __inspect_globals (void);
OBJECT *__inspect_get_arg (int);
OBJECT *__inspect_get_receiver (int);
OBJECT *__inspect_get_global (char *);
OBJECT *__inspect_get_local (int, char *);
void __inspect_brief_help (void);
void __inspect_long_help (void);
void __inspect_init (void);

/* lib/edittext.c */
int __textedit_insert (OBJECT *, int, int, int);
int __edittext_prev_char (OBJECT *);
int __edittext_next_char (OBJECT *);
int __edittext_prev_line (OBJECT *);
int __edittext_next_line (OBJECT *);
int __edittext_next_page (OBJECT *);
int __edittext_prev_page (OBJECT *);
int __edittext_delete_char (OBJECT *);
int __edittext_text_end (OBJECT *);
int __edittext_text_start (OBJECT *);
int __edittext_point_to_click (OBJECT *, int, int);
int __edittext_index_from_pointer (OBJECT *, int, int);
int __edittext_insert_at_point (OBJECT *, int, int, int);
int __edittext_get_primary_selection (OBJECT *, void **, int *);
int __edittext_insert_str_at_point (OBJECT *, char *);
int __edittext_set_selection_owner (OBJECT *);
int __edittext_insert_str_at_click (OBJECT *, int, int, char *);
int __edittext_row_col_from_mark (OBJECT *, int, int, int *, int *);
int __edittext_scroll_down (OBJECT *);
int __edittext_scroll_up (OBJECT *);
int __edittext_recenter (OBJECT *);
unsigned int __edittext_xk_keysym (int, int, int);
int __entrytext_get_primary_selection (OBJECT *, void **, int *);

/* lib/err_out.c */
void _error_out (char *);

/* lib/errorloc.c */
int error_column_at (int);
char *error_file_at (int);
int error_line_at (int);
void error_reset (void);
int get_error_location_ptr (void);
void init_error_location (void);
void restore_error_location (void);
void save_error_location (int, int, char *);

/* lib/escstr.c */
char *escape_str (char *, char *);
char *escape_str_quotes (char *, char *);
char *escape_pattern_quotes (char *, char *);
char *esc_expr_and_pattern_quotes (char *, char *);
char *unescape_str_quotes (char *, char *);
char *esc_expr_quotes (char *, char *);
void de_newline_buf (char *s);
int __ctalkIntFromCharConstant (char *);
int docstring_to_line_count (MESSAGE *);

/* lib/event.c */
int __ctalkNewSignalEventInternal (int, int, char *);
int __ctalkNewApplicationEventInternal (int, char *);

/* lib/except.c */
int __ctalkCriticalExceptionInternal (MESSAGE *, EXCEPTION, char *);
int __ctalkCriticalSysErrExceptionInternal (MESSAGE *, int, char *);
int __ctalkDeleteExceptionInternal (I_EXCEPTION *);
void *__ctalkExceptionInstalledHandler (OBJECT *);
OBJECT *__ctalkExceptionNotifyInternal (I_EXCEPTION *);
char * __ctalkPeekExceptionTrace (void);
char *__ctalkGetRunTimeException (void);
int __ctalkHandleInterpreterExceptionInternal (MESSAGE *);
int __ctalkHandleRunTimeException (void);
int __ctalkHandleRunTimeExceptionInternal (void);
int __ctalkHaveCallerException (void);
int __ctalkExceptionInternal (MESSAGE *, EXCEPTION, char *, int);
char *__ctalkPeekRunTimeException (void);
void __ctalkPrintExceptionTrace (void);
int __ctalkPrintObject (OBJECT *);
int __ctalkPrintObjectByName (char *);
int __ctalkSysErrExceptionInternal (MESSAGE *, int, char *);
I_EXCEPTION *__ctalkTrapException (void);
I_EXCEPTION *__ctalkTrapExceptionInternal (MESSAGE *);
int __ctalkPendingException (void);
OBJECT *__ctalkUndefinedMethodReferenceException (I_EXCEPTION *);
int _delete_exception (I_EXCEPTION *);
I_EXCEPTION *__get_x_list (void);
I_EXCEPTION *__new_exception (void);
int __ctalkDeleteLastExceptionInternal (void);

/* lib/exec_bin.c */
int exec_bin (char *);
int exec_bin_to_buf (char *, OBJECT *);

/* lib/fileglob.c */
int __ctalkGlobFiles (char *, OBJECT *);

/* lib/fmtargtype.c */
FMT_ARG_CTYPE fmt_arg_type (MESSAGE_STACK, int, int);
FMT_ARG_CTYPE fmt_arg_type_ms (MSINFO *);


/* lib/font.c */
char *__get_actual_xlfd (char *);
int __ctalkX11QueryFont (OBJECT *, char *);
int __ctalkX11TextWidth (char *xlfd, char *text);

/* lib/fsecure.c */
FILE *xfopen (const char *, const char *);
int xfprintf (FILE *, const char *, ...);
int xfscanf (FILE *, const char *, ...);

/* lib/ftlib.c */
int __ctalkGLXUseFTFont (char *);
void __ctalkGLXDrawTextFT (char *, float, float);
int __ctalkGLXFreeFTFont (void);
void __ctalkGLXPixelHeightFT (int);
void __ctalkGLXNamedColor (char *, float *, float *, float *);
bool __ctalkUsingFtFont (void);
double __ctalkGLXTextWidthFT (char *);
void __ctalkGLXAlphaFT (float);

/* lib/glewlib.c */
int __ctalkInitGLEW (void);
bool __ctalkGLEW20 (void);
bool __ctalkARB (void);


/* GLUT */
/* lib/glutlib.c */   
int __ctalkGLUTVersion (void);
int __ctalkGLUTCreateMainWindow (char *);
int __ctalkGLUTInitWindowGeometry (int, int, int, int);
int __ctalkGLUTInit (int, char **);
int __ctalkGLUTWindowID (char *);
int __ctalkGLUTRun (void);
int __ctalkGLUTInstallCallbacks (void);
int __ctalkGLUTInstallDisplayFn (void (*)());
int __ctalkGLUTInstallReshapeFn (void (*fn)(int, int));
int __ctalkGLUTInstallIdleFn (void (*fn)());
int __ctalkGLUTInstallOverLayDisplayFunc(void (*fn)());
int __ctalkGLUTInstallKeyboarfFunc (void (*fn)(char c, int x, int y));
int __ctalkGLUTInstallMouseFunc (void (*fn)(int button, int state,
					    int x, int y));
int __ctalkGLUTInstallMotionFunc (void (*fn)(int x, int y));
int __ctalkGLUTInstallPassiveMotionFunc (void (*fn)(int x, int y));
int __ctalkGLUTInstallVisibilityFunc (void (*fn)(int state));
int __ctalkGLUTInstallEntryFunc (void (*fn)(int state));
int __ctalkGLUTInstallSpecialFunc (void (*fn)(int key, int x, int y));
int __ctalkGLUTInstallSpaceballMotionFunc (void (*fn)(int x, int y, int z));
int __ctalkGLUTInstallSpaceballRotateFunc (void (*fn)(int x, int y, int z));
int __ctalkGLUTInstallSpaceballButtonFunc (void (*fn)(int button, int state));
int __ctalkGLUTInstallButtonBoxFunc (void (*fn)(int button, int state));
int __ctalkGLUTInstallDialsFunc (void (*fn)(int dial, int value));
int __ctalkGLUTInstallTabletMotionFunc (void (*fn)(int x, int y));
int __ctalkGLUTInstallTabletButtonFunc (void (*fn)(int button, int state,
						   int x, int y));
int __ctalkGLUTInstallMenuStatusFunc (void (*fn)(int status,
						 int x, int y));
int __ctalkGLUTInstallMenuStateFunc (void (*fn)(int status));
int __ctalkGLUTInstallTimerFunc (int msec, void (*fn)(int value), int value);
void __ctalkGLUTFullScreen (void);
void __ctalkGLUTSphere (double, int, int, int);
void __ctalkGLUTCube (double, int);
void __ctalkGLUTCone (double, double, int, int, int);
void __ctalkGLUTTorus (double, double, int, int, int);
void __ctalkGLUTDodecahedron (int);
void __ctalkGLUTOctahedron (int);
void __ctalkGLUTTetrahedron (int);
void __ctalkGLUTIcosahedron (int);
void __ctalkGLUTTeapot (double, int);
void __ctalkGLUTPosition (int, int);
void __ctalkGLUTReshape (int, int);
void __ctalkXPMToGLTexture (char **, unsigned short int, int *,
                            int *, void **);

/* lib/glxlib.c */
int __ctalkCreateGLXMainWindow (OBJECT *);
int __ctalkMapGLXWindow (OBJECT *);
int __ctalkCloseGLXPane (OBJECT *);
int __ctalkGLXSwapBuffers (OBJECT *);
int __ctalkGLXUseXFont (OBJECT *, char *);
int __ctalkGLXFreeXFont (void);
int __ctalkGLXDrawText (char *);
int __ctalkGLXWindowPos2i (int, int);
void __ctalkXPMToGLXTexture (char **, unsigned int, int *, int *, void **);
int __ctalkGLXWinXOrg (void);
int __ctalkGLXWinYOrg (void);
int __ctalkGLXWinXSize (void);
int __ctalkGLXWinYSize (void);
int __ctalkGLXTextWidth (char *);
void __glx_resize (int, int);
void __ctalkGLXFullScreen (OBJECT *);
void __glx_get_win_config (int *, int *, int *, int *);
char *__ctalkGLXExtensionsString (void);
bool __ctalkGLXExtensionSupported (char *);
float __ctalkGLXRefreshRate (void);
int __ctalkGLXSwapControl (int);
float __ctalkGLXFrameRate (void);

/* lib/guiclearrectangle.c */
int __ctalkX11ClearRectangleBasic (void *, int, unsigned long int, int, int, int, int);
int __ctalkX11PaneClearRectangle (OBJECT *self_object, int x, int y,
				   int width, int height);
int __ctalkGUIPaneClearRectangle (OBJECT *self_object, int x, int y,
				   int width, int height);

/* lib/guiclearwindow.c */
int __ctalkGUIPaneClearWindow (OBJECT *);

/* lib/guidrawline.c */
int __ctalkGUIPaneDrawLine (OBJECT *, OBJECT *, OBJECT *);
int __ctalkX11PaneDrawLine (OBJECT *, OBJECT *, OBJECT *);
int __ctalkGUIPaneDrawLineBasic (void *, int, unsigned long int, int, int, int, int,
				 int, int, char *);
int __ctalkX11PaneDrawLineBasic (void *, int, unsigned long int, int, int, int, int,
				 int, int, char *);


/* lib/guidrawpoint.c */
int __ctalkGUIPaneDrawPoint (OBJECT *, OBJECT *, OBJECT *);
int __ctalkX11PaneDrawPoint (OBJECT *, OBJECT *, OBJECT *);
int __ctalkX11PaneDrawPointBasic (void *, int, unsigned long int, int, int, int, int,
				  char *);


/* lib/guidrawrectangle.c */
int __ctalkGUIPaneDrawRectangle (OBJECT *, OBJECT *, OBJECT *, int);
int __ctalkX11PaneDrawRectangle (OBJECT *, OBJECT *, OBJECT *, int);
int __ctalkGUIPaneDrawRoundedRectangle (OBJECT *, OBJECT *, OBJECT *, int, int);
int __ctalkX11PaneDrawRoundedRectangle (OBJECT *, OBJECT *, OBJECT *, int, int);
int __ctalkX11PaneDrawRectangleBasic (void *, int, unsigned long int,
				  int, int, int, int, int, int, char *, int);
int __ctalkGUIPaneDrawRectangleBasic (void *, int, unsigned long int,
				  int, int, int, int, int, int, char *, int);

/* lib/guiputstr.c */
int __ctalkX11PanePutStr (OBJECT *, int, int, char *);
int __ctalkGUIPanePutStr (OBJECT *, int, int, char *);
int __ctalkX11PanePutStrBasic (void *, int, unsigned long int, int, int, char *);

/* lib/guiputxstr.c */
int __ctalkX11PanePutTransformedStr (OBJECT *, int, int, char *);
int __ctalkGUIPanePutTransformedStr (OBJECT *, int, int, char *);

/* lib/guirefresh.c */
int __ctalkGUIPaneRefresh (OBJECT *, 
    			  int, int, int, int, int, int);

/* lib/guisetbackground.c */
int __ctalkX11SetBackground (OBJECT *, char *);
int __ctalkGUISetBackground (OBJECT *, char *);
int __ctalkX11SetBackgroundBasic (void *, int, unsigned long int, char *);

/* lib/guisetforeground.c */
int __ctalkX11SetForegroundBasic (void *,int, unsigned long int, char *);

/* lib/guitext.c */
int __ctalkX11TextFromData (void *, int, unsigned long int, char *);

/* lib/guixpm.c */
int __ctalkX11XPMFromData (void *, int, unsigned long int, int, int, char **);
int __ctalkX11XPMInfo (void *, char **, int *, int *, int *, int *);

/* lib/infiles.c */
int is_input_file (char *);
void save_infile_declarations (CLASSLIB *);

/* lib/htoa.c */
char *htoa (char *, uintptr_t);
char *__ctalkHexIntegerToASCII (uintptr_t, char *);
int __ctalkReferenceObject (OBJECT *, OBJECT *);

/* lib/lex.c */
char *collect_expression (MSINFO *, int *);
char *collect_expression_buf (MSINFO *, int *, char *);
char *collect_tokens (MESSAGE_STACK, int, int);
char *collect_tokens_buf (MESSAGE_STACK, int, int, char *);
int expr_outer_parens (MESSAGE_STACK, int, int, int, int, int *, int *);
int find_expression_limit (MSINFO *);
void lexical (const char *, long long *, MESSAGE *);
int prefix (MESSAGE_STACK, int, int);
int tokenize (int (*)(MESSAGE *), char *);
int tokenize_reuse (int (*)(MESSAGE *), char *);
int tokenize_no_error (int (*)(MESSAGE *), char *);

/* lib/lextype.c */
int lextype_of_class (char *);
bool lextype_is_PTR_T (char *);
bool lextype_is_LITERAL_T (char *);
bool lextype_is_LITERAL_CHAR_T (char *);
bool lextype_is_INTEGER_T (char *);
bool lextype_is_INTEGER_T_conv (char *, int *, RADIX *);
bool lextype_is_LONGLONG_T (char *);
bool lextype_is_LONGLONG_T_conv (char *, long long int *, RADIX *);
bool lextype_is_FLOAT_T (char *);
bool lextype_is_FLOAT_T_conv (char *, double *);
bool lextype_is_DOUBLE_T (char *);
bool lextype_is_DOUBLE_T_conv (char *, double *);
#if defined(__APPLE__) && defined(__POWERPC__)
bool lextype_is_LONGDOUBLE_T_conv (char *, double *);
#else
bool lextype_is_LONGDOUBLE_T_conv (char *, long double *);
#endif

/* lib/lineinfo.c */
int end_of_p_stmt (MESSAGE_STACK, int);
MESSAGE *l_message_pop (void);
int l_message_push (MESSAGE *);
int line_info_tok (char *);
int set_line_info (MESSAGE_STACK, int, int);

/* lib/localtime.c */
#ifdef __DJGPP__
void __ctalkLocalTime (time_t, int *, int *, int *, int *, int *, int *, 
    int *, int *, int *);
#else
void __ctalkLocalTime (long int, int *, int *, int *, int *, int *, int *, 
     int *, int *, int *);
#endif

/* lib/lsort.c */
int __ctalkSort (OBJECT *, bool);
int __ctalkSortByName (OBJECT *, bool);

/* lib/keyword.c */
int is_apple_i386_math_builtin (const char *);
int is_apple_ppc_math_builtin (const char *);
int is_c_data_type (const char *);
int is_c_data_type_attr (const char *);
int is_c_keyword (const char *);
bool is_c_c_keyword (const char *);
int is_ctrl_keyword (const char *);
int is_ctalk_keyword (const char *);
char *is_c_storage_class (const char *);
int is_gnu_extension_keyword (const char *);
int is_gnuc_builtin_type (const char *);
int is_macro_keyword (const char *);
int is_OBJECT_member (const char *);
#ifdef __APPLE__
bool is_darwin_ctype_fn (const char *);
#endif

/* lib/message.c */
MESSAGE *dup_message (MESSAGE *);
MESSAGE *new_message (void);
void delete_message (MESSAGE *);
void copy_message (MESSAGE *, MESSAGE *);
MESSAGE *resize_message (MESSAGE *, int);
int __rt_get_stack_end (MESSAGE_STACK );
int __rt_get_stack_top (MESSAGE_STACK);
int __ctalkNextLangMsg (MESSAGE_STACK, int, int);
int __ctalkPrevLangMsg (MESSAGE_STACK, int, int);
void reuse_message (MESSAGE *);
MESSAGE *get_reused_message (void);
void cleanup_reused_messages (void);

/* lib/getopts.c */
int getopts (char **, int);

/* lib/ismethd.c */
void initialize_ismethd_hashes (void);
void hash_instance_method (char *, char *);
void hash_class_method (char *, char *);
int __ctalk_isMethod_2 (char *, MESSAGE_STACK, int, int);
int __ctalk_isMethod_3 (EXPR_PARSER *, char *, int);
int __ctalk_op_isMethod_2 (char *, MESSAGE_STACK, int, int);
#if 0
/* needed? */
bool rt_method_name (char *, char *);
#endif

/* lib/ismethd_compile.c */
int __ctalk_isMethod (char *, MESSAGE_STACK, int, int, int);
int __ctalk_op_isMethod (char *, MESSAGE_STACK, int, int, int);


/* lib/list.c */
void list_add (LIST *, LIST *);
void delete_list (LIST **);
void delete_list_element (LIST *);
void delete_object_list (LIST **);
void delete_object_list_element (LIST *);
LIST *new_list (void);
LIST *list_remove (LIST **, LIST **);
void list_push (LIST **, LIST **);
LIST *list_unshift (LIST **);

/* lib/objtoc.c */
char __ctalk_to_c_char (OBJECT *);
double __ctalk_to_c_double (OBJECT *);
int __ctalk_to_c_int (OBJECT *);
long long int __ctalk_to_c_longlong (OBJECT *, int);
char *__ctalk_to_c_char_ptr (OBJECT *);
void *__ctalk_to_c_ptr (OBJECT *);
void *__ctalk_to_c_ptr_u (OBJECT *);
void *__ctalkToCArrayElement (OBJECT *);
char *__ctalkArrayElementToCCharPtr (OBJECT *);
char __ctalkArrayElementToCChar (OBJECT *);
void *__ctalkArrayElementToCPtr (OBJECT *);
double __ctalkArrayElementToCDouble (OBJECT *);
int __ctalkArrayElementToCInt (OBJECT *);
long long int __ctalkArrayElementToCLongLongInt (OBJECT *);
double __ctalkToCDouble (OBJECT *);
int __ctalkToCIntArrayElement (OBJECT *);
char __ctalkToCCharacter (OBJECT *, int);
char *__ctalkToCCharPtr (OBJECT *, int);
int __ctalkToCInteger (OBJECT *o, int);
long int __ctalkToCLongInteger (OBJECT *o, int);
#ifdef RETURN_VALPTRS
void delete_return_valptrs (void);
#endif
LIST *vec_is_registered (void *);
void register_mem_vec (void *);
void delete_mem_vec_entry (void *);
void remove_mem_vec (LIST *);

/* lib/panelib.c */
int **__ctalkCreateWinBuffer (int, int, int);
int __ctalkDeleteWinBuffer (OBJECT *);
int __ctalkANSIClearPaneLine (OBJECT *, int);
int __ctalkANSITerminalPaneMapWindow (OBJECT *);
int __ctalkANSITerminalPaneUnMapWindow (OBJECT *);
int __ctalkANSITerminalPanePutChar (int, int, char);
int __ctalkANSITerminalPaneRefresh (void);
int __ctalkCopyPaneStreams (OBJECT *, OBJECT *);
int __pane_x_org (OBJECT *);
int __pane_y_org (OBJECT *);
int __pane_x_size (OBJECT *);
int __pane_y_size (OBJECT *);

/* lib/process.c */
int __ctalkExec (char *, OBJECT *);
int __ctalkSpawn (char *, int);

/* lib/rt_call.c */
OBJECT *__ctalkCallMethodFn (METHOD *);
bool sym_ptr_expr (EXPR_PARSER *, int, int, int *);

/* lib/rt_op.c */
int _rt_operands (EXPR_PARSER *, int, int *, int *);
int _rt_unary_operand (EXPR_PARSER *, int, int *);
OBJECT *_rt_unary_math (MESSAGE_STACK, int, OBJECT *);

/* lib/param.c */
PARAM *new_param (void);
TAGPARAM *new_tagparam (void);

/* lib/pattern.c */
int is_printf_fmt (char *, char *);

/* lib/pattypes.c */
int is_first_ctrlblk_pred_tok (MESSAGE_STACK, int, int);
bool is_fmt_arg (MESSAGE_STACK, int, int, int);
bool is_fmt_arg_2 (MSINFO *);
int is_fn_style_label (MESSAGE_STACK, int, int);
int is_function_call (MESSAGE_STACK, int, int, int);
int is_leading_ctrlblk_prefix_op_tok (MESSAGE_STACK, int, int);
int is_leading_prefix_op (MESSAGE_STACK, int, int);
int is_printf_fmt_msg (MESSAGE_STACK, int, int, int);
int is_in_rcvr_subexpr (MESSAGE_STACK, int, int, int);
int is_simple_subscript (MESSAGE_STACK, int, int, int);
int is_struct_or_union_expr (MESSAGE_STACK, int, int, int);
int struct_end (MESSAGE_STACK, int, int);
int is_subscript_expr (MESSAGE_STACK, int, int);
int is_trailing_postfix_op (MESSAGE_STACK, int, int);
int have_label_terminator (MESSAGE_STACK, int, int);
int obj_expr_is_arg (MESSAGE_STACK, int, int, int *);
int obj_expr_is_arg_ms (MSINFO *, int *);
int prefix_op_is_sizeof (MESSAGE_STACK, int, int);
int rcvr_is_start_of_expr (MESSAGE_STACK, int, int);
int method_contains_argblk (MESSAGE_STACK, int, int);
bool op_is_prefix_op (EXPR_PARSER *, int);
void find_self_unary_expr_limits (MESSAGE_STACK, int, int *, int *, 
     int, int);
int open_paren_is_arglist_start (MESSAGE_STACK, int, int);
int find_leading_tok_idx (MESSAGE_STACK, int, int, int);
void find_self_unary_expr_limits_2 (MESSAGE_STACK, int, int *, int);
int self_used_in_simple_object_deref (MESSAGE_STACK, int, int, int);
bool tok_precedes_assignment_op (EXPR_PARSER *, int);
int prefix_value_obj_is_term_result (EXPR_PARSER *, int);
bool is_c_assignment_op_label (MESSAGE *);
int match_array_brace (MESSAGE_STACK, int, int);
int match_block_open_brace (MESSAGE_STACK, int, int);
int tok_paren_level (MESSAGE_STACK, int, int);
int last_subscript_tok (MESSAGE_STACK, int, int);
bool struct_expr_terminal_array_member (char *);
int match_array_brace_rev (MESSAGE_STACK, int, int, int *);
bool is_single_token_cast (EXPR_PARSER *, int, int *);
char *terminal_struct_op (char *);
int parse_subscript_expr (MESSAGE_STACK, int, int, SUBSCRIPT subs[],
                          int *);
bool is_switch_closing_brace (MESSAGE_STACK, int, int);
bool rcvr_has_ptr_cx (EXPR_PARSER *, int, int);
bool prev_tok_is_symbol (MESSAGE_STACK, int);
char *elide_cvartab_struct_alias (char *, char *);
char *elide_cvartab_struct_alias_2 (char *, char *);
#ifdef __APPLE__
bool is_apple_inline_chk (MSINFO *ms);
#endif

/* lib/radixof.c */
RADIX radix_of (char *);
int __ctalkCharRadixToDecimal (char *);
char *__ctalkCharRadixToDecimalASCII (char *);
char *__ctalkIntRadixToDecimalASCII (char *);
char   *__ctalkCharRadixToCharASCII (char *);
char   __ctalkCharRadixToChar (char *);
int __ctalkIntRadixToDecimal (char *);

/* lib/read.c */
int is_binary_file (char *);
#ifdef __DJGPP__
unsigned long int read_file (char *, char *);
unsigned long int p_read_file (char **, char *);
#else
size_t read_file (char *, char *);
size_t p_read_file (char **, char *);
#endif
void __ctalkConsoleReadLine (OBJECT *, char *);

/* lib/re_lex.c */
int re_lexical (const char *, long long *, MESSAGE *);
int re_tokenize (int (*)(MESSAGE *), char *);

/* lib/rtobjvar.c */
OBJECT *__ctalkAddClassVariable (OBJECT *, char *, OBJECT *);
OBJECT *__ctalkAddInstanceVariable (OBJECT *, char *, OBJECT *);
OBJECT *__ctalkDefineClassVariable (char *, char *, char *, char *);
OBJECT *__ctalkDefineInstanceVariable (char *, char *, char *, char *);
int __ctalkDeleteObjectList (OBJECT *);
OBJECT *__ctalkFindMemberClass (OBJECT *);
OBJECT *__ctalkGetInstanceVariableComplex (OBJECT *, char *, int);
OBJECT *__ctalkGetInstanceVariableByName (char *, char *, int);
OBJECT *__ctalkFindClassVariable (char *, int);
OBJECT *__ctalkGetClassVariable (OBJECT *, char *, int);
OBJECT *__ctalkGetInstanceVariable (OBJECT *, char *, int);
int __ctalkInstanceVarsFromClassObject (OBJECT *);
int __ctalkIsClassVariableOf (OBJECT *, const char *);
int __ctalkIsInstanceVariableOf (OBJECT *, const char *);
int __is_instvar_of_method_return (EXPR_PARSER *, int);
void __ctalkStringifyName (OBJECT *, OBJECT *);

/* lib/rttmpobj.c */
int _cleanup_temporary_objects (OBJECT *,OBJECT *,OBJECT *,RT_CLEANUP_MODE);
void _cleanup_temporary_objects_all_instances (OBJECT *, RT_CLEANUP_MODE,
                                               EXPR_PARSER *, int, int*);
int delete_all_objs_expr_frame (EXPR_PARSER *, int, int *);
int delete_all_value_objs_expr_frame (MESSAGE_STACK, int, int, int, int *);
void clean_up_message_objects (MESSAGE_STACK, MESSAGE *, OBJECT *, OBJECT *,
     OBJECT *, int, int, int, RT_CLEANUP_MODE);
int cvar_alias_rcvr_created_here (MESSAGE_STACK, int, int, OBJECT *, OBJECT *);
bool cleanup_subexpr_created_arg (EXPR_PARSER *, OBJECT *, OBJECT *);

/* lib/rttrace.c */
void __ctalkSetExceptionTrace (int);
int __ctalkGetExceptionTrace (void);
int __save_call_stack_to_exception (I_EXCEPTION *);
void __warning_trace (void);

/* lib/rt_argblk.c */
int __ctalkEnterArgBlockScope (void);
int __ctalkExitArgBlockScope (void);
int __ctalkIsArgBlockScope (void);
int __ctalkBlockCallerFrame (void);
METHOD *__ctalkBlockCallerMethod (void);
RT_FN *__ctalkBlockCallerFn (void);
OBJECT *argblk_rcvr_object (MESSAGE_STACK, int);

/* lib/rt_args.c */
int __add_arg_object_entry_frame (METHOD *, OBJECT *);
void __argstack_error_cleanup (void);
bool is_arg (OBJECT *);
OBJECT *__ctalkArgAt (int);
int __ctalkCreateArg (char *, char *, void *);
int __ctalkCreateArgA (char *, char *, void *);
ARG *__ctalkCreateArgEntry (void);
ARG *__ctalkCreateArgEntryInit (OBJECT *);
void __ctalkDeleteArgEntry (ARG *);
int __ctalkGetArgPtr (void);
int __ctalk_arg (char *, char *, int, void *);
int __ctalk_arg_cleanup (OBJECT *);
int __ctalk_arg_push (OBJECT *);
int __ctalk_arg_push_ref (OBJECT *);
OBJECT *__ctalk_arg_pop (void);
OBJECT *__ctalk_arg_pop_deref (void);
OBJECT *__ctalk_arg_internal (int);
OBJECT *__ctalk_arg_value_internal (int);
#if 0
/* Needed? */
OBJECT *__ctalk_getstack (void *);
#endif
void __argstack_error_cleanup (void);
MESSAGE *__rt_arg_message_pop (void);
int __rt_arg_message_push (MESSAGE *);
OBJECT *__rt_eval_arg (METHOD *, OBJECT *, char *);
int __rt_method_args(METHOD *, MESSAGE_STACK, int, int, int, int);
int __rt_method_arglist_n_args (EXPR_PARSER *, int, METHOD *);
int cvar_alias_arg_min_refcount_check (OBJECT *);
char *ctalk_arg_text (void);
void clear_ctalk_arg_text (void);
void set_ctalk_arg_text (char *);

/* lib/rt_cvar.c */
OBJECT *__obj_from_cvar (MESSAGE *, CVAR *, char *, int);
int add_method_arg_cvar (CVAR *);
CVAR *__ctalkGetCArg (OBJECT *);
OBJECT *cvar_object (CVAR *, int *);
OBJECT *cvar_object_mark_evaled (CVAR *, int *, int);
int cvar_object_ref_cleanup (MESSAGE_STACK, int, int, int);
void _delete_cvar (CVAR *);
void init_cvars (void);
int delete_method_arg_cvars (void);
int delete_method_arg_cvars_evaled (void);
CVAR *get_method_arg_cvars (const char *);
CVAR *get_method_arg_cvars_not_evaled (const char *, int);
CVAR *remove_method_arg_cvar ( char *name);
void _new_cvar (CVARREF_T);
int is_c_type (char *s);
int validate_type_declarations (CVAR *);
/* lib/rt_cvar.c */
int add_method_arg_cvar (CVAR *);
CVAR *__ctalkCopyCVariable (CVAR *);
OBJECT *__ctalkGetTemplateCallerCVAR (char *);
void __ctalkTemplateCallerCVARCleanup (void);
int __ctalk_register_c_method_arg (char *, char *, char *, char *,
				   char *, char *, int, int, int, int, 
				   int, void *);
int __ctalk_register_c_method_arg_b (char *, char *, char *, char *,
				    char *, int, int, int, int, 
				    int, void *);
int __ctalk_register_c_method_arg_c (char *, char *, char *, char *,
    				    int, int, int, int, 
				    int, void *);
int __ctalk_register_c_method_arg_d (char *, char *, char *,
    				    int, int, int, int, 
				    int, void *);
int __resolve_aggregate_method (MESSAGE_STACK, int);
bool is_cvar (char *);
void write_val_CVAR (OBJECT *, METHOD *);
void unref_vartab_var (int *, CVAR *, OBJECT *);
OBJECT *OBJECT_mbr_class (OBJECT *, char *);

/* lib/rt_expr.c */
int __ctalkMatchParen (MESSAGE_STACK, int, int);
OBJECT *create_instancevar_param (char *, OBJECT *);
OBJECT *current_expression_result (void);
EXPR_PARSER *__current_expr_parser (void);
void __delete_expr_parser (EXPR_PARSER *);
OBJECT *exception_null_obj (MESSAGE_STACK, int);
EXPR_PARSER *__expr_parser_at (int);
char *expr_text (char *);
int __get_expr_parser_level (void);
int __get_expr_parser_ptr (void);
EXPR_PARSER *__new_expr_parser (void);
EXPR_PARSER *__pop_expr_parser (void);
OBJECT *__obj_from_msg_val (MESSAGE *m, char *, char *, int);
int __set_aggregate_obj (MESSAGE_STACK, int, int, OBJECT *);
int __set_unary_prefix_expression (MESSAGE_STACK, int, int, int, OBJECT *);
int __set_left_operand_value (MESSAGE_STACK, int, OBJECT *);
int __set_right_operand_value (MESSAGE_STACK, int, OBJECT *);
int _get_e_message_ptr (void);
OBJECT *create_param (char *, METHOD *, OBJECT *, MESSAGE_STACK, int);
int e_message_push (MESSAGE *);
MESSAGE *e_message_pop (void);
OBJECT *eval_expr (char *, OBJECT *, METHOD *, OBJECT *, int, int);
OBJECT *eval_subexpr (MESSAGE_STACK, int, int);
OBJECT *eval_rassoc_rcvr_as_subexpr (MESSAGE_STACK, int, int, int *);
OBJECT *eval_rassoc_rcvr_as_subexpr_b (MESSAGE_STACK, int, int, int);
OBJECT *eval_subexpr_b (MESSAGE_STACK, int, int);
PARAM *has_param_def (METHOD *);
int is_c_fn_syntax (MESSAGE_STACK, int);
OBJECT *null_result_obj (METHOD *, int);
OBJECT *__ctalkEvalExpr (char *);
OBJECT *__ctalkEvalExprU (char *);
EXPR_PARSER *__ctalkGetExprParserAt (int);
int __ctalkGetExprParserPtr (void);
OBJECT *_rt_math_op (MESSAGE_STACK, int, int, int);
OBJECT *_rt_unary_math_op (EXPR_PARSER *, int);
METHOD *get_op_method (void);
int _set_expr_value (MESSAGE_STACK, int, int, int, OBJECT *);
int _set_rt_math_op_expr_value (MESSAGE_STACK, int, int, int, OBJECT *);
void _set_unary_expr_value (EXPR_PARSER *, int, OBJECT *, OBJECT **);
MESSAGE_STACK _get_e_messages (void);
OBJECT *self_expr (METHOD *, int);
OBJECT *last_eval_result (void);
void output_arg_rt_expr (MESSAGE_STACK, int, int, METHOD *);
int __ctalkMatchParenRev (MESSAGE_STACK, int, int);
void reset_last_eval_result (void);
int expr_n_occurrences (METHOD *);

/* lib/rt_error.c */
#ifdef __GNUC__
void _error (char *, ...) __attribute__ ((noreturn));
#else
void _error (char *, ...);
#endif
void _warning (char *, ...);
void __ctalkWarning (char *, ...);
#ifdef __GNUC__
void system_dir_error (char *) __attribute__ ((noreturn));
#else
void system_dir_error (char *);
#endif
void __log_message (char *fmt, ...);
void __log_message_internal (char *);
void __ctalkLogMessage (char *fmt, ...);
void method_wrong_number_of_arguments_warning (MESSAGE *, MESSAGE *, char *);
void method_wrong_number_of_arguments_prefix_error (MESSAGE *, MESSAGE *,
     char *);
char *math_op_does_not_understand_error (MESSAGE_STACK, int, int, int, int);
void strtol_error (int, char *, char *);
char *vartab_var_basename (char *);
void __ctalkRegisterUserFunctionName (char *);

/* lib/sconvchk.c */
int chkatoi (const char *);
int chkptrstr (const char *);

/* lib/shmem.c */
void delete_shmem_info (int);
int mem_setup (void);
void *get_shmem (int handle);
int detach_shmem (int, void *);
void release_shmem (int, void *);

/* lib/rt_signal.c */
int __ctalkDefaultSignalHandler (int);
int __ctalkIgnoreSignal (int);
int __ctalkInstallHandler (int, OBJECT *(*)());
void __ctalkSignalHandlerBasic (int);

/* lib/rt_stdarg.c */
OBJECT *__ctalkLibcFnWithMethodVarArgs (int (*)(), METHOD *, char *);
int __call_fn_w_args_fmtarg0 (char *, METHOD *, STDARG_CALL_INFO *);
int __call_fn_w_args_fmtarg1 (char *, METHOD *, STDARG_CALL_INFO *);
void args_to_method_args (METHOD *, STDARG_CALL_INFO *);
int tokenize_fmt (char *);

/* lib/rtinfo.c */
char *__argvFileName (void);
void __argvName (char *);
char *__ctalkClassLibraryPath (void);
char *__ctalkClassSearchPath (void);
int __ctalkErrorExit (void);
int __ctalkRtSaveSourceFileName (char *);
int __current_call_stack_idx (void);
int __get_call_stack_ptr (void);
char *__source_filename (void);
void __classLibName (char *);
char *__classLibFileName (void);
int __ctalk_exitFn (int);
void __ctalk_initRtFns (void);
int __ctalk_initFn (char *);
bool __ctalkIsInlineCall (void);
bool __ctalkCallerIsInlineCall (void);
void __ctalkRtReceiver (OBJECT *);
METHOD *__ctalkRtGetMethod (void);
OBJECT *__ctalkRtReceiverObject (void);
void __ctalk_rtReceiverClass (OBJECT *);
void __ctalkRtMethodClass (OBJECT *);
OBJECT *__ctalkRtMethodClassObject (void);
OBJECT *__ctalk_rtReceiverClassObject (void);
void __ctalk_rtMethodFn (OBJECT *(*fn)());
void *__ctalkRtGetMethodFn (void);
void __init_time (void);
void cleanup_rt_fns (void);
RT_FN *get_calling_fn (void);
METHOD *get_calling_method (void);
RT_FN *get_fn (void);
int is_successive_method_call (METHOD *);
RT_FN *new_rt_fn (void);
void register_successive_method_call (void);
void _delete_rtinfo (RT_INFO *);
RT_INFO *_new_rtinfo (void);
void __init_data (void);
int __save_rt_info (OBJECT *, OBJECT *, OBJECT *, METHOD *,
		    OBJECT *(*) (), bool);
int __restore_rt_info (void);
void __rt_init_library_paths (void);
bool __ctalkIsArgBlkScope (void);
void get_stdin_stdout_termios (void);
void restore_stdin_stdout_termios (void);
bool str_rcvr_mod (void);
void register_str_rcvr_mod (bool b);
void set_rcvr_mod_catch (void);
void clear_rcvr_mod_catch (void);
bool need_rcvr_mod_catch (void);
void register_postfix_fetch_update (void);
bool need_postfix_fetch_update (void);
void clear_postfix_fetch_update (void);
void register_arg_active_varentry (VARENTRY *);
void clear_arg_active_varentry (void);
VARENTRY *arg_active_varentry (void);
void register_string_assign_by_value (void);
bool assign_string_by_value (void);
void clear_string_assign_by_value (void);
int __ctalkNArgs(void);
char *__ctalkInstallPrefix (void);
int __ctalk_process_exitFn (int);
char *__ctalkDocDir (void);

/* lib/rt_methd.c */
int _external_parens (MESSAGE_STACK, int, int);
int _is_empty_arglist (MESSAGE_STACK, int, int);
int __ctalkArglistLimit (MESSAGE_STACK, int, int, int, int);
int __ctalkDefineClassMethod (char *, char *, OBJECT *(*)(), int);
int __ctalkDefineInstanceMethod (char *, char *, OBJECT *(*)(), int, int);
int __ctalkDefineTemplateMethod (char *, char *, OBJECT *(*)(), int, int);
METHOD *__ctalkFindClassMethodByFn (OBJECT **, OBJECT *(*)(void), int);
METHOD *__ctalkFindClassMethodByName (OBJECT **, const char *, int, int);
METHOD *__ctalkFindInstanceMethodByFn (OBJECT **, OBJECT *(*)(void), int);
METHOD *__ctalkFindInstanceMethodByName (OBJECT **, const char *, int, int);
METHOD *__ctalkFindPrefixMethodByName (OBJECT **, const char *, int);
METHOD *__ctalkGetClassMethodByFn (OBJECT *o, OBJECT *(*)(void), int);
METHOD *__ctalkGetClassMethodByName (OBJECT *, const char *, int, int);
METHOD *__ctalkGetInstanceMethodByFn (OBJECT *o, OBJECT *(*)(void), int);
METHOD *__ctalkGetInstanceMethodByName (OBJECT *, const char *, int, int);
METHOD *__ctalkGetPrefixMethodByName (OBJECT *, const char *, int);
METHOD *__ctalkFindMethodByName (OBJECT **, const char *, int, int);
OBJECT *__ctalkInlineMethod (OBJECT *, METHOD *, int, ...);
OBJECT *__ctalk_method (char *, OBJECT *(*)(), char *);
void __ctalk_initLocalObjects (void);
OBJECT *__ctalk_methodReturn (OBJECT *, METHOD *, OBJECT *);
void __ctalk_primitive_method (char *, char *, int);
void __ctalkClassMethodInitReturnClass (char *, char *, char *, int);
void __ctalkInstanceMethodInitReturnClass (char *, char *, OBJECT *(*)(), char *, int);
void __ctalkInstanceMethodParam (char *, char *, OBJECT *(*)(), char *, char *, int);
void __ctalkClassMethodParam (char *, char *, OBJECT *(*)(), char *, char *, int);
int __ctalkIsClassMethod (OBJECT *, char *);
int __ctalkIsInstanceMethod (OBJECT *, char *);
void __ctalkMethodReturnClass (char *);
METHOD *__this_method (void);
void init_extra_objects (void);
bool is_method_param (char *);
METHOD *get_method_greedy (OBJECT *, char *, METHOD **, int *);

/* lib/rt_obj.c */
OBJECT *__ctalkCopyObject (OBJREF_T, OBJREF_T);
OBJECT *__ctalk_get_object (const char *, const char *);
OBJECT *__ctalk_get_constructor_arg_tok (const char *);
OBJECT *__ctalk_get_arg_tok (const char *);
void __ctalkDeleteObject (OBJECT *);
void __ctalkDeleteObjectInternal (OBJECT *);
OBJECT *__ctalkInstanceVariableObject (OBJECT *);
OBJECT *__ctalkClassVariableObject (OBJECT *);
OBJECT *__ctalkInstanceVariableReceiver (OBJECT *);
int __ctalkRegisterExtraObject (OBJECT *);
int __ctalkRegisterExtraObjectInternal (OBJECT *, METHOD *m);
int __ctalkRegisterUserObject (OBJECT *);
OBJECT *__ctalkReplaceVarEntry (VARENTRY *, OBJECT *);
void __ctalkSetObjectScope (OBJECT *, int);
void __ctalkSetObjectAttr (OBJECT *__o, int attr);
int __ctalk_remove_object (OBJECT *o);
OBJECT *__ctalk_self_internal (void);
OBJECT *__ctalk_self_internal_value (void);
void __ctalk_set_global (char *, char *);
void __ctalk_set_local (OBJECT *);
void __delete_operand_result (OBJREF_T, OBJREF_T);
void delete_extra_local_objects (EXPR_PARSER *);
void save_local_objects_to_extra (void);
void delete_varentry (VARENTRY *);
void reset_varentry_i (VARENTRY *);
OBJECT *match_instancevar (OBJECT *, OBJECT *);
OBJECT *match_instancevar_name (OBJECT *, OBJECT *);
VARENTRY *new_local_varentry (OBJECT *);
VARENTRY *new_varentry (OBJECT *);
int object_is_decrementable (OBJECT *, OBJECT *, OBJECT *);
int object_is_deletable (OBJECT *, OBJECT *, OBJECT *);
int parent_ref_is_circular (OBJECT *, OBJECT *);
OBJECT *top_parent_object (OBJECT *);
void save_e_methods (EXPR_PARSER *);
int is_p_obj (OBJECT *, OBJECT *);
int save_e_methods_2 (METHOD *);
int const_created_param (MESSAGE_STACK, int, METHOD *, int, MESSAGE *,
    			 OBJECT *);
OBJECT *create_param (char *, METHOD *, OBJECT *, MESSAGE_STACK, 
       int);
OBJECT *create_instancevar_param (char *, OBJECT *);
void __ctalk_set_local_by_name (char *);
int register_function_objects (VARENTRY *);
void cleanup_fn_objects (void);
OBJECT *__ctalk_get_eval_expr_tok (const char *);
OBJECT *_store_int (OBJECT *, OBJECT *);

/* lib/rt_prton.c */
int __ctalkSelfPrintOn (void);
int __ctalkObjectPrintOn (OBJECT *);
int __ctalkCallerPrintOnSelf (OBJECT *);

/* lib/rt_rcvr.c */
OBJECT *__constant_rcvr (char *);
void __ctalk_class_initialize (void);
int __ctalk_receiver_push (OBJECT *);
int __ctalk_receiver_push_ref (OBJECT *);
int is_current_receiver (OBJECT *);
bool is_receiver (OBJECT *);
bool instancevar_is_receiver (OBJECT *);
OBJECT *__ctalk_receivers_at (int);
OBJECT *__ctalk_receiver_pop (void);
OBJECT *__ctalk_receiver_pop_deref (void);
int __ctalkGetReceiverPtr (void);
bool __getClassLibRead (void);
void __setClassLibRead (bool);
OBJECT *resolve_self (bool, bool);

/* lib/rt_refobj.c */
OBJECT *_makeRef (char *);
OBJECT *_getRef (OBJECT *);
OBJECT **_getRefRef (OBJECT *);

/* lib/rt_return.c */
OBJECT *__ctalkRegisterBoolReturn (int);
OBJECT *__ctalkRegisterCharReturn (char);
OBJECT *__ctalkRegisterCharPtrReturn (char *);
OBJECT *__ctalkRegisterFloatReturn (double);
OBJECT *__ctalkRegisterIntReturn (int);
OBJECT *__ctalkRegisterLongLongIntReturn (long long int);
OBJECT *__ctalkSaveCVARArrayResource (char *, int, void *);
OBJECT *__ctalkSaveCVARResource (char *);
OBJECT *__ctalkSaveOBJECTMemberResource (OBJECT *);
OBJECT *__ctalkRegisterArgBlkReturn (int, OBJECT *);
void __ctalkArgBlkkSetCallerReturn (OBJECT *);
bool __ctalkNonLocalArgBlkReturn (void);
OBJECT *__ctalkArgBlkReturnVal (void);

/* lib/rt_vmthd.c */
int __ctalkDefinedClassMethodObject (OBJECT *, char *, char *);
int __ctalkDefinedInstanceMethodObject (OBJECT *, char *, char *);
int __ctalkMethodObjectMessage (OBJECT *, OBJECT *);
int __ctalkMethodObjectMessage2Args (OBJECT *, OBJECT *, OBJECT *, OBJECT *);
int __ctalkBackgroundMethodObjectMessage (OBJECT *, OBJECT *);
int __ctalkBackgroundMethodObjectMessage2Args (OBJECT *, OBJECT *, OBJECT *,
       					      OBJECT *);
OBJECT *__ctalk_method_from_object (OBJECT *, OBJECT *(*)(), METHOD *, int,
       				   	   int *);
OBJECT *__ctalk_method_from_object_2_args (OBJECT *, OBJECT *(*)(),
       METHOD *, int, int *);
void delete_processes (void);
int __ctalkProcessWait (int, int *, int *, int *);

/* lib/rtclslib.c */
void __ctalk_dictionary_add (OBJECT *);
OBJECT *__ctalk_define_class (ARG **);
void __ctalkClassDictionaryCleanup (void);
OBJECT *__ctalkFindFirstSubclass (OBJECT *);
OBJECT *__ctalkGetClass (const char *);
OBJECT *__ctalkClassObject (OBJECT *);
bool __ctalkIsSubClassOf (char *, char *);
OBJECT *get_class_library_definition (char *);
void __ctalkRemoveClass (OBJECT *);
OBJECT *__ctalk_find_classvar (char *, char *);
void delete_object_list_internal (OBJECT *);
void hash_class_var (OBJECT *);
void hash_remove_class_var (OBJECT *);
void cleanup_unlink_method_user_object (VARENTRY *);
void varentry_cleanup_reset (VARENTRY *);
void unlink_varentry (OBJECT *);
void unlink_varentry_2 (OBJECT *, VARENTRY *);
bool is_string_object (OBJECT *);
bool is_key_object (OBJECT *);
void delete_user_object_list_member (LIST *);
void __delete_fn_user_objects (RT_FN *);
void init_default_class_cache (void);
void delete_default_class_cache (void);
OBJECT *get_class_by_name (const char *);
void rt_get_class_names (void);
void rt_delete_class_names (void);
bool is_class_library_name (char *);


/* lib/rtnewobj.c */
OBJECT *_dup_object (OBJECT *);
OBJECT *__ctalkCreateObject (char *, char *, char *, int);
OBJECT *__ctalkCreateObjectInit (char *, char *, char *, int, char *);
OBJECT *create_object_init_internal (char *, OBJECT *, int, char *);
OBJECT *__ctalk_new_object (ARG **);
OBJECT *create_class_object_init (char *, char *, char *, int, char *);
int __ctalkSetObjectValue (OBJECT *, char *);
int __ctalkSetObjectValueBuf (OBJECT *, void *);
int __ctalkSetObjectValueAddr (OBJECT *, void *, int);
int __ctalkSetObjectValueVar (OBJECT *, char *);
int __ctalkSetObjectValueVarB (OBJECT *, char *);
int __ctalkSetObjectValueClass (OBJECT *, OBJECT *);
int __ctalkSetObjectName (OBJECT *, char *);
bool is_class_or_subclass (OBJECT *, OBJECT *);
void init_object_size (void);

/* lib/rtnwmthd.c */
NEWMETHOD *create_newmethod (void);
NEWMETHOD *create_newmethod_init (METHOD *);
void delete_newmethod (NEWMETHOD *);

/* lib/rtobjref.c */
void *generic_ptr_str (char *);
void __refObj (OBJREF_T, OBJREF_T);
void __objRefCntInc (OBJREF_T);
void __objRefCntDec (OBJREF_T);
void __objRefCntSet (OBJREF_T, int);
void __objRefCntZero (OBJREF_T);
OBJECT *obj_from_ref_str (char *);
OBJECT *obj_ref_str (char *);
void *__ctalkGenericPtrFromStr (char *);
void *__ctalkFilePtrFromStr (char *);

/* lib/rtsyserr.c */
int __ctalkErrnoInternal (void);

/* lib/rtxalloc.c */
void *__xalloc (int);
void __xfree(void **);
void *__xstrdup (char *);
void *__xrealloc (void **, int);
void __ctalkFree(void *);

/* lib/sformat.c */
void __ctalkObjValPtr (OBJECT *, void *);
void *__ctalkStrToPtr (char *);
char *__scalar_fmt_conv (char *, char *, OBJECT *);
bool is_zero_q (char *);
bool str_is_zero_q (char *);

/* lib/signame.c */
char *__ctalkSystemSignalName (int);

/* lib/ssearch.c */
int __ctalkSearchBuffer (char *, char *, long long *);

/* lib/statfile.c */
char *basename_w_extent (char *);
char *expand_path (char *, char *);
char *pwd (void);
int file_exists (char *);
int file_mtime (char *);
int file_size (char *);
int is_dir (char *);
char *which (char *);
int __ctalkIsDir (char *);
bool is_shell_script (char *);
bool file_has_exec_permissions (char *);

/* lib/strcatx.c */
int strcatx (char *, ...);
int strcatx2 (char *, ...);
char *toks2str (MESSAGE_STACK, int, int, char *);

/* lib/strdupx.c */
void *strdupx (const char *);

/* lib/streq.c */
#ifdef __GNUC__
inline bool str_eq (register char *, register char *);
#else
bool str_eq (const char *, const char *);
#endif

/* lib/strsecure.c */
int xsprintf (char *, const char *, ...);
char *xstrcpy (char *, const char *);
char *xstrncpy (char *, const char *, size_t);
char *xstrcat (char *, const char *);
char *xstrncat (char *, const char *, size_t);
void *xmemcpy (void *, const void *, size_t);
void *xmemset (void *, int, size_t);
void *xmemmove (void *, const void *, size_t);

/* lib/tag.c */
VARTAG *new_vartag (VARENTRY *);
void delete_vartag (VARTAG *, bool);
void delete_vartag_list (OBJECT *);
void add_tag (OBJECT *, VARENTRY *);
void remove_tag (OBJECT *, VARENTRY *);
void make_tag_for_varentry_active (OBJECT *, VARENTRY *);
void __ctalkIncKeyRef (OBJECT *, int, int);
void __ctalkIncStringRef (OBJECT *, int, int);
void *active_i (OBJECT  *);
OBJECT *create_param_i (OBJECT *, void *);
bool is_temporary_i_param (OBJECT *);
void derived_i (VARENTRY *, OBJECT *);
int derived_i_2 (VARENTRY *, OBJECT *);
int n_tags (OBJECT *);
int make_postfix_current (OBJECT *);
bool reset_i_if_lval (OBJECT *, char *);
void __ctalkAddBasicNewTag (OBJECT *);
void reset_primary_tag (VARTAG *);
OBJECT *var_i (OBJECT *, int);
VARENTRY *reset_if_re_assigning_collection (OBJECT *, OBJECT *, OBJECT **);

/* lib/tempio.c */
void init_tmp_files (void);
int create_tmp (void);
char *macro_cache_path (void);
char *new_macro_cache_path (void);
int unlink_tmp (void);
void cleanup_preprocessor_cache (void);
int close_tmp (void);
int write_tmp (char *);
char *get_tmpname (void);
void cleanup_temp_files (int);
char *preprocess_name (char *, int);
char *ctpp_name (char *, int);
char *ctpp_tmp_name (char *, int);
int save_previous_output (char *);
int rename_file (char *, char *);
void remove_tmpfile_entry (void);

/* lib/tempname.c */
char *__rand_ext (void);
int __rand_ext_2 (void);
char *__tempname (char *, char *);

/* lib/termsize.c */
int __ctalkTerminalHeight (void);
int __ctalkTerminalWidth (void);

/* lib/textcx.c */
void wrap_redisplay (unsigned int, void *, int, int, char *);
void wrap_redisplay_ft (void *, void *, void *, int, int, char *);
void __ctalkSetWordWrap (bool);
bool __ctalkGetWordWrap (void);
extern void __ctalkSplitText (char *, OBJECT *);
void __ctalkWrapText (int, unsigned long int, OBJECT *, int, int);

/* lib/trimstr.c */
char *trim_leading_whitespace (char *, char *);
char *trim_trailing_whitespace (char *);
char *trimstr (char *, char *);
char *remove_whitespace (char *, char *);

/* lib/typecast.c */
char *collect_typecast_expr (MESSAGE_STACK, int, int *);
CVAR *__ctalkGetTypedef (char *);
int __ctalkRegisterCTypedef (char *, char *, char *, char *, char *, char *,
			     char *, int, int, int, int);
int __rt_is_typecast (MESSAGE_STACK, int, int);
int __rt_is_typecast_expr (EXPR_PARSER *, int);
int typecast_is_pointer (MESSAGE_STACK, int, int);
int typecast_ptr_expr (MESSAGE_STACK, int, int, int, int);
int typecast_ptr_expr_b (EXPR_PARSER *, int, int);
int typecast_value_expr (MESSAGE_STACK, int, int);
void constant_typecast_expr (MESSAGE_STACK, int, int, int);
int typecast_or_constant_expr (MESSAGE_STACK, int, int, int, EXPR_PARSER *,
    			       METHOD *, int, OBJECT *, bool);
OBJECT *cast_object_to_c_type (MESSAGE_STACK, int, int, OBJECT *);

/* lib/val.c */
int copy_val (VAL *, VAL *);
char *hex_from_numeric_val (char *);
int is_val_true (VAL *);
int m_print_val (MESSAGE *, VAL *);
int numeric_value (char *, VAL *);
int val_eq (VAL *, VAL *);

/* lib/wregex.c */
int __ctalkMatchText (char *, char *, long long int*);
int __ctalkLastMatchLength (void);
char __ctalkGetRS (void);
void __ctalkSetRS (char);
char *__ctalkMatchAt (int);
int __ctalkMatchIndexAt (int);
int __ctalkNMatches (void);
void __ctalkMatchPrintToks (bool);

/* lib/xcircle.c */
int __ctalkX11PaneDrawCircleBasic (void *, int, unsigned long int, int, int, int, int, int, int, char *, char *);
int __ctalkGUIPaneDrawCircleBasic (void *, int, unsigned long int, int, int, int, int, int, int, char *, char *);

/* lib/xcopypixmap.c */
int __ctalkX11CopyPixmapBasic (void *, int, unsigned long int,
                               int, int, int, int, int,
			       int, int);

/* lib/xgeometry.c */
void set_size_hints_internal (OBJECT *, int *, int *, int *, int *);
int __ctalkX11ParseGeometry (char *, int *, int *, int *, int *);
int __ctalkX11SetSizeHints (int, int, int, int, int);
void __ctalkX11GetSizeHints (int, int *, int *, int *, int *, int *, int *);
void __ctalkX11FreeSizeHints (void);
void __ctalkX11SubWindowGeometry (OBJECT *, char *, int *, int *, int *, int *);

/* lib/xlibfont.c */
int load_xlib_fonts_internal (void *, char *);
int load_xlib_fonts_internal_1t (void *, char *);
void clear_font_descriptors (void *);
int __ctalkSelectXFontFace (void *, int, unsigned long int, int);

/* lib/xrender.c */
void __ctalkX11UseXRender (bool);
bool __ctalkX11UsingXRender (void);

/* lib/xresource.c */
int __ctalkX11SetResource (void *, int, char *, char *);
OBJECT *__ctalkPaneResource (OBJECT *, char *, bool);

/* lib/xftlib.c */
int __ctalkXftInitLib (void);
char *__ctalkXftQualifyFontName (char *);
char *__ctalkXftListFontsFirst (char *);
void __ctalkXftListFontsEnd (void);
char *__ctalkXftListFontsNext (void);
int __ctalkXftMajorVersion (void);
int __ctalkXftMinorVersion (void);
int __ctalkXftVersion (void);
int __ctalkXftRevision (void);
int __ctalkXftInitialized (void);
char *__ctalkXftSelectedFontDescriptor (void);
char *__xft_selected_pattern_internal (void);
int __ctalkXftGetStringDimensions (char *, int *, int *, int *, int *, int *);
void __ctalkXftSelectFont (char *, int, int, int, double);
void __ctalkXftSelectFontFromXLFD (char *);
void __ctalkXftSelectFontFromFontConfig (char *font_config_str);
char  *__ctalkXftSelectedFamily (void);
int __ctalkXftSelectedSlant (void);
int __ctalkXftSelectedWeight (void);
int __ctalkXftSelectedDPI (void);
double __ctalkXftSelectedPointSize (void);
int __ctalkXftFgRed (void);
int __ctalkXftFgGreen (void);
int __ctalkXftFgBlue (void);
int __ctalkXftFgAlpha (void);
int __ctalkXftAscent (void);
int __ctalkXftDescent (void);
int __ctalkXftHeight (void);
int __ctalkXftMaxAdvance (void);
void __ctalkXftSetForeground (int, int, int, int);
void __ctalkXftSetForegroundFromNamedColor (char *);
char *__ctalkXftFontPathFirst (char *);
char *__ctalkXftFontPathNext (void);
char *__ctalkXftSelectedFontPath (void);
void __ctalkXftRed (int);
void __ctalkXftGreen (int);
void __ctalkXftBlue (int);
void __ctalkXftAlpha (int);
int load_ft_font_faces_internal (char *, double,
                                 unsigned short int,
				 unsigned short int,
                                 unsigned short int);
void __ctalkXftShowFontLoad (int lvl);
int __ctalkXftVerbosity (void);
char *__ctalkXftDescStr (void);
char *__ctalkXftRequestedFamily (void);
int __ctalkXftRequestedPointSize (void);
int __ctalkXftRequestedSlant (void);
int __ctalkXftRequestedWeight (void);
int __ctalkXftRequestedDPI (void);
bool __ctalkXftIsMonospace (void);


/* lib/x11ksym.c */
int ascii_shift_keysym (unsigned long int);
int ascii_ctrl_keysym (unsigned long int);
int get_x11_keysym (int, int, bool);
int get_x11_keysym_2 (void *, int, int, int);
int __ctalkGetX11KeySym (int, int, int);

/* lib/x11lib.c */
#if ! defined (DJGPP) && ! defined (WITHOUT_X11)
int __ctalkCloseX11Pane (OBJECT *);
int __ctalkX11CloseParentPane (OBJECT *);
int __ctalkX11FontCursor (OBJECT *, int);
int __ctalkCreateX11MainWindow (OBJECT *);
int __ctalkCreateX11SubWindow (OBJECT *, OBJECT *);
int __ctalkMapX11Window (OBJECT *);
int __ctalkOpenX11InputClient (OBJECT *);
int __ctalkX11ResizePixmap (void *, int, int, unsigned long int, int, int, int, int, int, 
    int *);
int __ctalkX11ResizeWindow (OBJECT *, int, int, int);
int __ctalkX11SetBackground (OBJECT *, char *);
int __ctalkX11PaneClearRectangle (OBJECT *, int, int, int, int);
int __ctalkX11PaneClearWindow (OBJECT *self_object);
int __ctalkX11PaneDrawLine (OBJECT *, OBJECT *, OBJECT *);
int __ctalkX11PaneDrawPoint (OBJECT *, OBJECT *, OBJECT *);
int __ctalkX11PaneDrawRectangle (OBJECT *, OBJECT *, OBJECT *, int);
int __ctalkX11PaneRefresh (OBJECT *, int, int, int, int, int, int);
int __ctalkRaiseX11Window (OBJECT *);
int __ctalkX11CloseClient (OBJECT *);
int __ctalkX11Colormap (void);
/* 
   Declare as returning a void * so we don't
   need to define Display * by including
   the X headers if not needed.
*/
void *__ctalkX11Display (void);
int __ctalkX11SetWMNameProp (OBJECT *, char *);
int __ctalkX11UseFontBasic (void *, int, unsigned long int, char *);
int __ctalkX11UseCursor (OBJECT *, OBJECT *);
void *__ctalkX11NextInputEvent (OBJECT *);
int __ctalkX11InputClient (OBJECT *, int, int, int); 
int __have_bitmap_buffers (OBJECT *);
OBJECT *__x11_pane_win_gc_value_object (OBJECT *);
OBJECT *__x11_pane_win_id_value_object (OBJECT *);
int __ctalkX11MakeEvent (OBJECT *, OBJECT *);
int __ctalkX11DisplayHeight (void);
int __ctalkX11DisplayWidth (void);
int __client_pid (void);
#ifdef HAVE_XRENDER_H
bool xrender_version_check (void);
#endif /* #ifdef HAVE_XRENDER_H */
int sizeof_int (void);
/* Note that the prototype for gc_check needs to be in files that
   has the GC typedef. */
#else  /* #if ! defined (DJGPP) && ! defined (WITHOUT_X11) */
void x_support_error (void);
#endif /* #if ! defined (DJGPP) && ! defined (WITHOUT_X11) */
int read_event (int *, unsigned int *, unsigned int [], int);

/* lib/xdialog.c */
int __ctalkX11CreateDialogWindow (OBJECT *);
int __ctalkCloseX11DialogPane (OBJECT *);
extern char **__ctalkIconXPM (int);
extern char **__ctalkEntryIconXPM (int);
extern void __enable_dialog (OBJECT *);

/* libdeps.c */
int cache_ctpp_output_file (char *);
void check_user_args (void);
int ctpp_deps_updated (char *);
void save_opts (char **, int);
void record_opts (void); 
int class_deps_updated (char *);
int method_deps_updated (char *);

/* loop.c */
void init_loops (void);
int loop_block_end (MESSAGE_STACK, int);
int loop_block_start (MESSAGE_STACK, int);
int loop_pred_end (MESSAGE_STACK, int);

/* main.c */
int args (char **, int, int);
char *ctalk_lib_include (void);
void exit_help (void);
int extended_arg (char *);
char *get_class_lib_dir (void);
char *get_package_name (void);
int preprocess (char *, bool, bool);
int preprocess_to_output (char *);
void CTFLAGS_args ();
void cleanup_CTFLAGS_args (void);

/* math.c */
bool eval_bool (MESSAGE_STACK, int, VAL *);
int eval_math (MESSAGE_STACK, int, VAL *);
int handle_sizeof_op (MESSAGE_STACK, int, int *, VAL *);
int perform_add (MESSAGE *, VAL *, VAL *, VAL *);
int perform_divide (MESSAGE *, VAL *, VAL *, VAL *);
int perform_modulus (MESSAGE *, VAL *, VAL *, VAL *);
int perform_multiply (MESSAGE *, VAL *, VAL *, VAL *);
int perform_subtract (MESSAGE *, VAL *, VAL *, VAL *);
int perform_asl (MESSAGE *, VAL *, VAL *, VAL *);
int perform_asr (MESSAGE *, VAL *, VAL *, VAL *);
int perform_bit_and (MESSAGE *, VAL *, VAL *, VAL *);
int perform_bit_comp (MESSAGE *, VAL *, VAL *);
int perform_bit_or (MESSAGE *, VAL *, VAL *, VAL *);
int perform_bit_xor (MESSAGE *, VAL *, VAL *, VAL *);
bool bool_eval (MESSAGE_STACK, int, VAL *);
int math_eval (MESSAGE_STACK, int, VAL *);
bool question_conditional_eval (MESSAGE_STACK, int, int *, VAL *);

/* mcct.c */
bool is_mcct_expr (MESSAGE_STACK, int, int *, int *);
void handle_mcct_expr (MESSAGE_STACK, int, int);
CVAR *ifexpr_is_cvar_not_shadowed (MESSAGE_STACK, int);


/* method.c */
int arg_error_loc (int, int, int, ERROR_LOCATION *);
void cleanup_args (METHOD *, OBJECT *, int);
int compound_method (MESSAGE_STACK, int, int *);
int define_primitive_method (char *, char *, OBJECT *(*)(), int, int);
OBJECT *eval_receiver_token (MESSAGE_STACK, int, OBJECT *);
METHOD *get_class_method (MESSAGE *, OBJECT *, char *, int, int);
METHOD *get_instance_method (MESSAGE *, OBJECT *, char *, int, int);
CVAR *get_instance_method_local_cvar (char *);
OBJECT *get_instance_method_local_object (char *);
METHOD *get_prefix_instance_method (MESSAGE *, OBJECT *, char *, int);
METHOD *get_super_instance_method (MESSAGE_STACK, int, OBJECT *, char *, int);
int get_method_message_ptr (void);
void init_imported_method_queue (void);
int init_method_stack (void);
bool is_class_method (MESSAGE_STACK, int);
bool is_instance_method (MESSAGE_STACK, int);
bool is_instance_method_or_variable (MESSAGE_STACK, int);
bool is_c_constant_instance_method (MESSAGE_STACK, int);
int is_method_name (const char *);
int is_method_parameter (MESSAGE_STACK, int);
bool is_method_parameter_s (char *);
int is_method_selector (const char *);
OBJECT *method_arg_object ( MESSAGE *);
int method_args (METHOD *, int);
int method_args2 (METHOD *, int);
int method_call (int);
int method_declaration_info (char *, char *, char **, int *, int *);
int method_declaration_msg (MESSAGE_STACK, int, int, char *, int *, 
			    char *, int *);
MESSAGE *method_message_pop (void);
int method_message_push (MESSAGE *);
MESSAGE_STACK method_message_stack (void);
METHOD *method_replaces_primitive (METHOD *, OBJECT *);
void output_imported_methods (void);
void queue_method_for_output (OBJECT *, METHOD *);
OBJECT *resolve_arg (METHOD *, MESSAGE_STACK, int, OBJECT *);
char *rt_library_method_call (OBJECT *, METHOD *, MESSAGE_STACK, int, char *);
void save_method_object (OBJECT *);
void save_method_local_objects (void);
void save_method_local_cvars (void);
int store_arg_object (METHOD *, OBJECT *);
int method_arg_accessor_fn (MESSAGE_STACK, int, int, OBJECT_CONTEXT, bool);
int method_arg_accessor_fn_c (MESSAGE_STACK, int, int);
int method_arg_rt_expr (MESSAGE_STACK, int);
int method_fn_arg_rt_expr (MESSAGE_STACK, int);
int saved_method_object (OBJECT *);
int is_struct_or_union (CVAR *);
CVAR *get_struct_defn (char *);
OBJECT *new_method_parameter_class_object (MESSAGE_STACK, int);
int ambiguous_operand_throw (MESSAGE_STACK, int, METHOD *);
void translate_argblk_cvar_arg_1 (MESSAGE *, CVAR *);
CVAR *get_local_local_cvar (char *);
int have_class_cast_on_unresolved_expr (MESSAGE_STACK, int);
bool have_class_cast_on_unresolved_method (MESSAGE_STACK, int);
bool method_has_param_pfo_def (METHOD *);
bool is_new_method_parameter (char *);
bool param_label_shadows_method (char *, char *);

/* methodbuf.c */
void begin_method_buffer (METHOD *);
void buffer_method_statement (char *, int);
void end_method_buffer (void);
int format_method (BUFFERED_METHOD);
void generate_method_init (BUFFERED_METHOD);
int get_method_buf_message_ptr (void);
int method_buf_message_push (MESSAGE *);
MESSAGE **method_buf_message_stack (void);
void method_init_statement (char *);
void unbuffer_method_statements (void);
void method_vartab_statement (char *);
void method_vartab_init_statement (char *);
void generate_vartab (LIST *);
void unbuffer_method_vartab (void);

/* methodgt.c */
int get_proto_params_ptr (void);
int get_nth_proto_param (int);
bool get_nth_proto_varargs (int);
METHOD *get_instance_method_gt (MESSAGE *, OBJECT *, char *, int, int);
METHOD *get_class_method_gt (MESSAGE *, OBJECT *, char *, int, int);
int is_method_proto_max_params (char *, char *);

/* mthdrep.c */
void report_method_argument_mismatch (MESSAGE *, OBJECT *, char *, int);
METHOD *find_first_instance_method (OBJECT *, char *, OBJECT **);
METHOD *find_next_instance_method (OBJECT *, char *, OBJECT **);

/* mthdret.c */
int return_block (MESSAGE_STACK, int, int *);
int tok_after_return_idx (MESSAGE_STACK, int);

/* mthdrf.c */
int method_prototypes (char *, CLASSLIB *);
int method_prototypes_tok (MESSAGE_STACK, int, int, CLASSLIB *);
void input_prototypes (char *);
int is_method_proto (OBJECT *, char *);
int is_superclass_method_proto (char *, char *);
int method_proto_is_output (char *);
OBJECT *method_from_prototype (char *);
OBJECT *method_from_prototype_2 (OBJECT *, char *);
char *method_proto (char *, char *);
int this_method_from_proto (char *, char *);
MESSAGE_STACK r_message_stack (void);
int r_message_push (MESSAGE *);
void init_r_messages (void);
int get_r_message_ptr (void);
bool have_user_prototype (char *, char *);
void add_user_prototype (MESSAGE_STACK, int, int, int, int);
void cleanup_user_prototypes (void);
void init_method_proto (void);
bool is_proto_selector (char *);
bool is_method_proto_name (char *);

/* objderef.c */
bool objderef_check_expr (MESSAGE_STACK, int, int *, int);
bool objderef_handle_printf_arg (MSINFO *);

/* object.c */
int add_object (OBJECT *);
OBJECT *arg_expr_object (char *);
void cleanup_dictionary (void);
OBJECT *constant_token_object (MESSAGE *);
OBJECT *create_object (char *, char *);
OBJECT *create_object_init (char *, char *, char *, char *);
int delete_arg_object (OBJECT *);
void delete_local_objects (void);
void delete_objects (int);
void delete_object (OBJECT *);
OBJECT *find_class_variable (MESSAGE *, char *);
OBJECT *find_class_variable_2 (MESSAGE_STACK, int);
int fn_return (MESSAGE_STACK, int);
OBJECT *get_class_variable (char *, char *, int);
OBJECT *get_global_object (char *, char *);
OBJECT *get_instance_variable (char *, char *, int);
OBJECT *get_local_object (char *, char *);
OBJECT *get_object (char *, char *);
bool is_object_or_param (char *, char *);
int global_var_is_declared_not_duplicated (MESSAGE_STACK, int, char *);
OBJECT *instantiate_self_object (void);
OBJECT *instantiate_self_object_from_class (OBJECT *);
OBJECT *instantiate_object_from_class (OBJECT *, char *);
int is_class_variable_message (MESSAGE_STACK, int);
int is_in_rcvr_subexpr_obj_check (MESSAGE_STACK, int, int);
int is_instance_variable_message (MESSAGE_STACK, int);
int is_self_instance_variable_message (MESSAGE_STACK, int);
bool is_possible_receiver_subexpr_postfix (MSINFO *, int);
int constant_rcvr_idx (MESSAGE_STACK, int);
int label_is_defined (MESSAGE_STACK, int);
int self_is_fmt_arg (int, char *);
OBJECT *self_object (MESSAGE_STACK, int);
OBJECT *get_instance_variable_series (OBJECT *, MESSAGE *, int, int);
OBJECT *get_self_instance_variable_series (MESSAGE *, int, int, int);
char *get_param_instance_variable_series_class (PARAM *, MESSAGE_STACK, 
       int, int);
int get_instance_variable_series_term_idx (OBJECT *, MESSAGE *,
					   int, int);
OBJECT *get_method_param_obj (MESSAGE_STACK, int);
void get_new_method_param_instance_variable_series (OBJECT *,
     MESSAGE_STACK, int, int);
int primitive_arg_shadows_c_symbol (char *);
int is_self_expr_as_fn_lvalue (MESSAGE *, int, int, int);
int is_self_expr_as_C_expr_lvalue (MESSAGE *, int, int, int);
OBJECT *create_arg_EXPR_object (ARGSTR *);
OBJECT *create_arg_CFUNCTION_object (char *);
bool primitive_arg_shadows_method_parameter (char *);

/* objtoc.c */
char *basic_class_from_constant_tok (MESSAGE *);
char *fmt_printf_fmt_arg (MESSAGE_STACK, int, int, char *, char *);
char *fmt_printf_fmt_arg_ms (MSINFO *, char *, char *);
char *get_type_conv_name (char *);
int is_translatable_basic_class (char *);
char *obj_2_c_wrapper (MESSAGE *, OBJECT *, METHOD *, char *, int);
char *obj_2_c_wrapper_trans (MESSAGE_STACK, int, MESSAGE *, OBJECT *, METHOD *, char *, int);
char *array_obj_2_c_wrapper (MESSAGE_STACK, int, OBJECT *, METHOD *, char *);
char *basic_class_from_cvar (MESSAGE *, CVAR *, int);
char *basic_class_from_paramcvar (MESSAGE *, PARAMCVAR *, int);
char *fmt_rt_return (char *, char *, int, char *);
char *fmt_rt_return_chk_fn_arg (char *, char *, int, 
     MESSAGE_STACK, int);
char *basic_class_from_cfunc (MESSAGE *m_orig, CFUNC *c, int n_expr_derefs);
int match_c_type (CVAR *, CVAR *);
char *basic_class_from_fmt_arg (MESSAGE_STACK, int);
char *c_lval_class (MESSAGE_STACK, int);
bool have_unknown_c_type (void);

/* op.c */
OP_CONTEXT op_context (MESSAGE_STACK, int);
int op_precedence (int, MESSAGE_STACK, int);
int operands (MESSAGE_STACK, int, int *, int *);

/*output.c */
void __fileout (char *);
void __fileout_cache (char *);
void __fileout_import (char *);
void fileout (char *s, int, int);
void fmt_line_info (char *, int, char *, int, int);
void fmt_loop_line_info (char *, int line, int);
int fmt_fixup_info (char *, int, char *, int);
char *fmt_c_expr_store_arg_call (OBJECT *, METHOD *, OBJECT *, char *, ARGSTR *);
char *fmt_method_call (OBJECT *, char *, char *, char *);
char *fmt_register_c_method_arg_call (CVAR *, char *, int, char *);
char *fmt_rt_instancevar_call (OBJECT *, char *, char *);
char *fmt_rt_instancevar_call_2 (MESSAGE_STACK, int, int, OBJECT *, 
                                 char *, char *);
char *fmt_store_arg_call (OBJECT *, METHOD *, OBJECT *);
void generate_ctalk_init (void);
void generate_ctalk_init_call (void);
int generate_c_expr_store_arg_call (OBJECT *, METHOD *, OBJECT *, char *, int, ARGSTR *);
int generate_define_class_variable_call (char *, char *, char *, char *);
int generate_define_instance_variable_call (char *, char *, char *, char *);
int generate_method_call (OBJECT *, char *, char *, int);
int generate_method_call_cvar_cleanup (OBJECT *, char *, char *, int);
int generate_instance_method_param_definition_call (char *, char *, char *, char *, char *,
					   int, int);
int generate_class_method_param_definition_call (char *, char *, char *, char *, char *,
					   int, int);
int generate_method_pop_arg_call (int);
int generate_global_method_definition_call (char *,char *,char *, int, int);
int generate_global_object_definition (char *, char *, char *, char *, int);
int generate_method_c_declaration (char *);
int generate_method_call_from_primitive (OBJECT *, METHOD *, char *, int, int);
int generate_method_call_for_prototype (OBJECT *, char *, char *, char *);
int generate_class_method_definition_call (char *, char *, char *, int);
int generate_instance_method_definition_call (char *, char *, char *, int, int);
int generate_class_method_return_class_call (char *, char *, char *, int);
int generate_instance_method_return_class_call (char *, char *, char *, char *, int);
int generate_primitive_method_call (OBJECT *, METHOD *, char *, int);
int generate_primitive_method_call_2 (OBJECT *, METHOD *, char *, int, int);
int generate_primitive_class_definition_call (OBJECT *, METHOD *);
int generate_register_c_method_arg_call (CVAR *, char *, int, int);
int generate_register_typedef_call (CVAR *);
int generate_rt_instancevar_call (MESSAGE_STACK, int, int, OBJECT *, char *, int);
int generate_self_call (int);
int generate_set_global_variable_call (char *, char *);
int generate_set_local_variable_call (char *, char *);
int generate_store_arg_call (OBJECT *, METHOD *, OBJECT *, int);
int generate_store_arg_call2 (OBJECT *, METHOD *, int);
int include_output (char *);
void init_output_vars (void);
void cleanup_output_vars (void);
int method_definition_attributes (METHOD *);
int output_buffer (char *, int);
int output_ctalk_init (int);
int output_frame (MESSAGE_STACK, int, int, int, int);
int queue_init_statement (char *, int, int);
void unbuffer_function_output (void);
char *struct_or_union_expr (MESSAGE_STACK, int, int, int *);
char *subscript_cvar_registration_expr (MESSAGE_STACK, int, int, int *);
void generate_primitive_class_definition_call_from_init (char *, char *);
CVAR *basic_type_of (CVAR *);
int basic_attrs_of (CVAR *, CVAR *);
char *__fmt_c_method_arg_call (char *, char *, int scope, CVAR *, CVAR *,
                               char *);
void output_delete_cvars_call (MESSAGE_STACK, int, int);

/* param.c */
PARAM *new_param (void);
OBJECT *__ctalk_getParamByName (char *);

/* parser.c */
int add_frame (int, int);
int check_state (int, MESSAGE_STACK, int*, int);
void clear_all_messages (void);
void clear_frame_messages  (int);
void delete_parserinfo (PARSER *);
int get_messageptr (void);
int get_parser_output_ptr (void);
int init_message_stack (void);
void init_parser_frame_ptr (void);
void init_parsers (void);
int lasttoken (int);
int lookup_message (MESSAGE *);
MESSAGE_STACK message_stack ();
MESSAGE *message_pop (void);
MESSAGE *message_stack_at (int);
void message_push (MESSAGE *);
PARSER *new_parserinfo (void);
char *parse_string (char *);
char *parse (char *, long long);
PARSER *parser_at (int);
int parser_frame_ptr (void);
int parser_frame_scope (void);
int parser_pass (int, PARSER *);
int parser_ptr (void);
int current_parser_level (void);
PARSER *pop_parser (void);
void push_parser (PARSER *);
int remove_frame (void);
int reset_messageptr (int);
void set_message_value (MESSAGE *, void *);
void set_parser_frame_ptr (int);
int unevaled (int);

/* pexcept.c */
int catch_exception (void (*)(void));
int handle_parse_exception (PARSER *, int);
int i_exception (MESSAGE *, EXCEPTION, char *);
void init_i_exceptions (void);
void save_unresolved_classes (I_EXCEPTION *);
void save_unresolved_symbols (I_EXCEPTION *);
void set_parser_env (void);
void fp_container_adjust (int);

/* prefixop.c */
char *c_sizeof_expr_wrapper (OBJECT *, METHOD *, OBJECT *);
int postfix_arg_cvar_expr_registration (MESSAGE_STACK, int, CVAR *, int, int);
int prefix_arg_cvar_expr_registration (MESSAGE_STACK, int, CVAR *, int, int);
int prefix_rcvr_cvar_expr_registration (MESSAGE_STACK, int);
int postfix_rcvr_cvar_expr_registration (MESSAGE_STACK, int);
int sizeof_arg_needs_rt_eval (MESSAGE_STACK, int);
int unary_op_attributes (MESSAGE_STACK, int, int *, int *);

/* preclass.c */
void save_class_init_info (char *, char *, char *);
void save_instance_var_init_info (char *, char *, char *, char *);
void save_class_var_init_info (char *, char *, char *, char *);
void load_cached_class (char *);
void fill_in_class_info (char *);
void generate_init_from_cache (void);
char *class_cache_path_name (char *);
void init_pre_classes (void);

/* premethod.c */
void init_pre_methods (void);
int pre_method_is_output (char *);
void save_pre_init_info (char *, METHOD *, OBJECT *);
char *get_init_return_class (char *);
void register_init (char *);
void output_pre_method (char *, char *);
int class_or_instance_var_init_is_output_2 (char *, char *, char *);
void set_class_and_instance_var_init_output_2 (char *classname, char *i_or_c,
					       char *varname);
void output_pre_method_templates (char *);
void pre_method_register_selector_as_output (char *);


/* primitives.c */
int class_primitive_method_args (METHOD *, MESSAGE_STACK, int);
OBJECT *define_class (int);
OBJECT *define_class_variable (int);
OBJECT *define_instance_variable (int);
OBJECT *get_rcvr_class_obj (void);
void init_new_method_stack (void);
int is_cached_class (char *);
int method_start_line (void);
OBJECT *new_class_method (int);
OBJECT *new_instance_method (int);
OBJECT *new_object (int);
PARAM *method_param_str (char *);
PARAM *method_param_tok (MESSAGE_STACK, PARAMLIST_ENTRY *);
int param_class_exists (char *);
void set_rcvr_class_obj (OBJECT *);
void check_return_classes (void);
char *make_selector (OBJECT *, METHOD *, char *, I_OR_C);
int new_method_param_newline_count (void);
void add_template_to_method_init (char *);
bool is_instance_var (char *);
bool is_constructor_initializer (MESSAGE_STACK, int);

/* rcvrexpr.c */
bool rcvr_expr_in_parens (MSINFO *, int *, int *);
void output_rcvr_expr_in_parens (MSINFO *, int, int);

/* refobj.c */
bool have_ref (char *);
void delete_local_c_object_references (void);
void delete_global_c_object_references (void);
void delete_local_c_alias_reference (char *);
void delete_global_c_alias_reference (char *);
char *new_object_reference (char *, char *, int, char *);
char *fmt_getRef (char *, char *);
char *fmt_getRefRef (char *, char *);
bool insert_object_ref (MESSAGE_STACK, int);
bool is_rvalue_of_OBJECT_ptr_lvalue (MESSAGE_STACK, int, int *, int *);

/* reg_cvar.c */
int register_c_var (MESSAGE *, MESSAGE_STACK, int, int *);
char *register_c_var_buf (MESSAGE *, MESSAGE_STACK, int,
			  int *, char *);

/* resolve.c */
OBJECT *resolve (int);
CVAR *get_global_var_not_shadowed (char *name);
int prefix_method_attr (MESSAGE_STACK, int);

/* return.c */
int eval_return_expr (MESSAGE_STACK, int);
char *fmt_argblk_return_expr (MESSAGE_STACK, int);

/* rexpr.c */
int check_constant_expr (MESSAGE_STACK, int, int);
int class_variable_expression (MESSAGE_STACK, int);
int default_method (MSINFO *);
int expr_has_objects (MESSAGE_STACK, int, int);
int fn_output_context (MESSAGE_STACK, int, OBJECT *, METHOD *, int, int);
OBJECT_CONTEXT object_context (MESSAGE_STACK, int);
OBJECT_CONTEXT object_context_ms (MSINFO *);
int self_class_or_instance_variable_lookahead (MESSAGE_STACK, int);
int self_class_or_instance_variable_lookahead_2 (MESSAGE_STACK, int);
int need_rt_eval (MESSAGE_STACK, int);
int arg_needs_rt_eval (MESSAGE_STACK, int);
int use_new_c_rval_semantics (MESSAGE_STACK, int);
char *use_new_c_rval_semantics_b (MESSAGE_STACK, int);
int terminal_rexpr (MESSAGE_STACK, int);
int terminal_printf_arg (MESSAGE_STACK, int);
int is_single_token_method_param (MESSAGE_STACK, int, METHOD *);
int lval_idx_from_arg_start (MESSAGE_STACK, int);
int rval_ptr_context_translate (MSINFO *, int);

int postfix_method_expr_a (MESSAGE_STACK, int);
bool obj_rcvr_after_opening_parens (MESSAGE_STACK, int);

/* rt_expr.c */
void cleanup_expr_parser (void);
int eval_keyword_expr (MESSAGE_STACK, int);
char *fmt_default_ctrlblk_expr (MESSAGE_STACK, int, int, int, char *);
char *fmt_rt_expr (MESSAGE_STACK, int, int *, char *);
char *fmt_rt_expr_ms (MSINFO *, int *, char *);
char *fmt_user_fn_rt_expr (MESSAGE_STACK, int, int, char *, char *);
char *fmt_user_fn_rt_expr_b (MESSAGE_STACK, int, int, char *, char *, bool);
int prefix_method_expr_a (MESSAGE_STACK, int, int);
int prefix_method_expr_b (MESSAGE_STACK, int);
char *rt_expr (MESSAGE_STACK, int, int *, char *);
char *rt_expr_return (MESSAGE_STACK, int, int, int *, char *, bool *);
char *for_term_rt_expr (MESSAGE_STACK, int, int *, char *);
void rt_obj_arg (MESSAGE_STACK, int, int*, int, int);
void param_to_fn_arg (MESSAGE_STACK, int);
char *rt_self_expr (MESSAGE_STACK, int, int *, char *);
int undefined_label_check (MESSAGE_STACK, int);
OBJECT *const_expr_class (MESSAGE_STACK, int, int);
void deferred_method_eval_a (MESSAGE_STACK, int, int *);
int method_self_arg_rt_expr (MESSAGE_STACK, int);
int method_param_arg_rt_expr (MESSAGE_STACK, int, METHOD *);
int expr_contains_method_msg (MESSAGE_STACK, int, int);
char *fn_param_return_trans (MESSAGE *, CFUNC *, char *, int);
void argblk_ptrptr_trans_1 (MESSAGE_STACK, int);
int rval_expr_1 (MSINFO *);
char *fmt_rt_return_2 (MESSAGE_STACK, int, int, char *, char *);
char *fmt_instancevar_expr_a (MESSAGE_STACK, int, int, int *);
char *fmt_eval_expr_str (char *, char *);
char *fmt_eval_u_expr_str (char *, char *);
char *fmt_pattern_eval_expr_str (char *, char *);
void handle_self_conditional_fmt_arg (MSINFO *);
int is_method_param_name (char *);
int rte_expr_contains_c_fn_arg_call (MESSAGE_STACK messages,
					    int start, int end);

/* rt_time.c */
int __ctalkUTCTime (void);

/* stack.c */
int p_message_push (MESSAGE *);
MESSAGE *p_message_pop (void);
int stack_start (MESSAGE_STACK);
int get_stack_top (MESSAGE_STACK);

/* subexpr.c */
int _scanback (MESSAGE_STACK, int, int, int, int);
int _scanforward (MESSAGE_STACK, int, int, int, int);
int constant_subexpr (MESSAGE_STACK, int, int *, VAL *);
int copy_val (VAL *, VAL *);
int m_print_val (MESSAGE *, VAL *);
int p_match_paren (MESSAGE_STACK, int, int);
int match_paren (MESSAGE_STACK, int, int);
int match_paren_rev (MESSAGE_STACK, int, int);
int nextlangmsg (MESSAGE_STACK, int);
int nextlangmsgstack (MESSAGE_STACK, int);
int nextlangmsgstack_pfx (MESSAGE_STACK, int);
int prevlangmsg (MESSAGE_STACK, int);
int prevlangmsg_np (MESSAGE_STACK, int);
int prevlangmsgstack (MESSAGE_STACK, int);
int prevlangmsgstack_pfx (MESSAGE_STACK, int);
int re_eval (int, int);
int scanback (MESSAGE_STACK, int, int, int);
int scanback_other (MESSAGE_STACK, int, int, int);
int scanforward (MESSAGE_STACK, int, int, int);
int scanforward_other (MESSAGE_STACK, int, int, int);
int set_subexpr_val (MESSAGE_STACK, int, VAL *);
int un_evaled (int, int);

/* subscr.c */
char *subscript_expr (MESSAGE_STACK, int, int, OBJECT *, METHOD *, char *, CVAR *);
char *subscript_expr_struct (MESSAGE_STACK, int, int, int, int, OBJECT *, METHOD *, char *, CVAR *);
void register_struct_terminal (MESSAGE_STACK, MESSAGE *, int, int, 
			       int, int, int, CVAR *, char *, char *,
			       OBJECT *, OBJECT *, METHOD *);
char *register_struct_terminal_buf (MESSAGE_STACK, MESSAGE *, int, int, 
				    int, int, int, CVAR *, char *, char *,
				    OBJECT *, OBJECT *, METHOD *, char *);
void cvar_register_output (MESSAGE_STACK, int, int, int, int, OBJECT *,
     		  	    METHOD *, CVAR *, char *, char *, 
			  OBJECT *);
char *cvar_register_output_buf (MESSAGE_STACK, int, int, int, int,
				OBJECT *, METHOD *, CVAR *, 
				char *, char *, OBJECT *, char *);
char *register_template_subscript_arg (MESSAGE_STACK, int, int);
bool subscr_rcvr_next_tok_is_method_a (MESSAGE_STACK, int, CVAR *);
void handle_simple_subscr_rcvr (MESSAGE_STACK, int);

/* substrcpy.c */
char *substrcpy (char *, char *, int, int);
char *substrcat (char *, char *, int, int);

/* symbol.c */
#if 0
/* superceded... ? */
int add_unresolved_class (char *);
int add_unresolved_symbol (char *);
SYMBOL *new_symbol (void);
void delete_symbol (SYMBOL *);
#endif

/* typecast_expr.c */
int is_typecast_expr (MESSAGE_STACK, int, int *);
char *basic_class_from_typecast (MESSAGE_STACK, int, int);
bool is_class_typecast (MSINFO *, int);
bool has_typecast_form (MSINFO *ms, int);

/* typecheck.c */
bool is_unary_minus (MESSAGE_STACK, int);
bool is_object_prefix (MESSAGE_STACK, int);
int operand_type_check (MESSAGE_STACK, int);
int unary_type_check (MESSAGE_STACK, int);
int binary_type_check (MESSAGE_STACK, int);

/* ufntmpl.c */
FN_TMPL *create_user_fn_template (MESSAGE_STACK, int);
char *fmt_user_template_rt_expr (MESSAGE_STACK, int, FN_TMPL *);
int register_user_args (char **, int, CFUNC *, int *);
char *template_from_user_cfunc (CFUNC *, char *, char *, char *, int);
int user_fn_arg_declarations (MESSAGE_STACK, int, int, char **, int *);
char *user_fn_expr_args (MESSAGE_STACK, int, char *);
#if 0
    /* Not needed right now. */
char *user_fn_expr_args_from_params (MESSAGE_STACK, int, char *);
#endif
char *user_fn_template (MESSAGE_STACK, int);
void generate_user_fn_template_init (char *, char *);
char *user_fn_template_name (char *);
int user_template_params (MESSAGE_STACK, int);

/* unixsock.c */
int __ctalkUNIXSocketShutdown (int, int);
int __ctalkUNIXSocketOpenReader (char *);
int __ctalkUNIXSocketOpenWriter (char *);
int __ctalkUNIXSocketRead (int, void *);
int __ctalkUNIXSocketWrite (int, void *, int);

#endif /* _CTALK_H */



