/* $Id: parser.h,v 1.4 2020/11/27 12:18:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _LIST_H
#include "list.h"
#endif

#ifndef _CVAR_H
#include "cvar.h"
#endif

#ifndef _EXCEPT_H
#include "except.h"
#endif

#ifndef _PARSER_H
#define _PARSER_H

#ifndef MAXMSG
#define MAXMSG 8192
#endif

/* 
 *  Define the maximum filename length if the system hasn't 
 *  already defined it.
 */
#ifndef FILENAME_MAX
#define FILENAME_MAX 255
#endif

/*
 *  Need to include 
 *  extern PARSER *parsers[MAXARGS+1];           
 *  extern int current_parser_ptr;
 *
 *  In the source module.
 */
#ifndef CURRENT_PARSER
#define CURRENT_PARSER ((current_parser_ptr >= MAXARGS + 1) ? NULL : \
			parsers[current_parser_ptr])
#endif
#define FRAME_START_IDX (frame_at (CURRENT_PARSER -> frame) -> \
                         message_frame_top)
#define HAVE_FRAME  ((frame_at (CURRENT_PARSER -> frame)) ? TRUE : FALSE)
#define FRAME_SCOPE (frame_at (CURRENT_PARSER -> frame) -> scope)

typedef enum {
  none,
  and,
  or,
  not,
  defined,
  eq,
  ge,
  le,
  gt,
  lt
} BOOLEAN_OP;

/*
 *   Interpreter passes.
 */
typedef enum {
  run_time_pass,      /* The target program is being run and run-time lib
		         is being used.                                      */
  preprocessing_pass, /* Preprocess.c functions preprocess (), etc.          */
  var_pass,           /* Functions called from parse_vars ().                */
  parsing_pass,       /* Main source file - parse () called from main.c.     */
  library_pass,       /* Class files - parse () called from library_search().*/
  method_pass,        /* Method source - parse () called from new_method (). */
  c_fn_pass,          /* C function args - parse () called by c_fn_args ().  */
  expr_check,         /* Check the validity of an expression.                */
  typedef_var_pass
} I_PASS;

typedef enum {
  add,
  subtract,
  mult,
  divide,
  asl,
  asr
} MATH_OP;

typedef enum { op_null_context,
	       op_unary_context, 
	       op_binary_context,
	       op_cast_context,
	       op_not_an_op
} OP_CONTEXT;

typedef enum {
  decl_null_context,
  decl_var_context,
  decl_param_list_context
} DECLARATION_CONTEXT;

#define PARSER_SIG 0xf6f6f6

typedef struct {
  int sig;
  OBJECT *vars;          /* Local objects.     */
  OBJECT *methodobjects; /* Other objects created for method statements. */
                         /*  Here so the parser can clean up parameter   */
                         /*  objects quickly at the end of a method.     */
  CVAR *cvars;           /* Local C variables. */
  OBJECT *classes;       /* Local classes.     */
  LIST *init;            /* Init statements.   */
  LIST *init_head;
  int level;
  int frame;
  int top_frame;
  int last_frame;
  int block_level;
  int need_init;         /* False only when between function declaration
			    and first non-variable declaration statement,
			    when init block is output. */
  int need_main_init;    /* True only when parsing main () declaration.  */
  int need_main_exit;
  int pred_state,        /* Saved control block states when recursing.   */
    blk_state,
    else_state;
  EXCEPTION _p_exception;
  I_PASS pass;
} PARSER;

typedef enum {
  abs_path,
  inc_path
} INCLUDE_PATH_TYPE;

/*
 *    Preprocessor conditional expressions.
 */
typedef enum {
  null_expr,
  cond_expr,
  cond_expr_n,
  cond_expr_elif,
  cond_expr_endif,
  cond_expr_else
} COND;

typedef enum {
  if_val_undef = -1,
  if_val_false = 0,
  if_val_true = 1
} COND_VAL;

typedef struct {
  COND expr;
  COND_VAL val;
} COND_EXPR;

typedef enum {      /* Which of the operands to ## to concatenate. */
  concat_op_1,
  concat_op_2,
  concat_both
} CONCAT_MODE;

typedef struct {
  char name[FILENAME_MAX];
  char path[FILENAME_MAX]; /* GNU's __FILE__returns the path. */
  INCLUDE_PATH_TYPE path_type;
  int error_line;         /* Source line of #include directive. */
  int s_start, s_end;
  int result_ptr;
  COND expr;              /* Saved conditional expr state.     */
  COND_EXPR if_vals[MAXARGS+1];
  int message_ptr;        /* The preprocessor stack pointer.   */
  char *buf;
} INCLUDE;


typedef struct _macro_symbol {
  char sig[8];
  char name[MAXLABEL];
  char value[MAXMSG];
  char *args[MAXARGS];
  struct _macro_symbol *next;
  struct _macro_symbol *prev;
} DEFINITION;

typedef struct {
  char fn[FILENAME_MAX];
  int error_line, 
    error_column,
    parser_lvl;
} ERROR_LOCATION;

typedef struct _struct_decl {
  char storage_class[MAXLABEL];
  char name[MAXLABEL];
  VARNAME vars[MAXARGS];
  int n_vars;
  int attrs;
  char *type_decls[MAX_DECLARATORS];
  int type_decls_ptr;
  CVAR *members,     /* Saved members when recursing into an */
		      /* inner struct.                        */
    *members_ptr;
  int block_start,
    block_end;
} STRUCT_DECL;

typedef struct _fn_decl {
  char storage_class[MAXLABEL];
  char name[MAXLABEL];
  VARNAME vars[MAXARGS];
  int n_vars;
  int attrs;
  char *type_decls[MAX_DECLARATORS];
  int type_decls_ptr;
  CVAR *params,     /* Saved members when recursing into an */
		      /* inner struct.                        */
    *params_ptr;
} FN_DECL;

typedef struct _arg_term {
  MESSAGE_STACK messages;
  int start, end;
} ARG_TERM;

typedef enum {
  null_message,
  instance_method_message,
  class_method_message,
  instance_var_message,
  class_var_message
} MESSAGE_CLASS;

#define SELF_LVAL_FN_EXPR_TEMPLATE "\n\
       {\n\
          %s %s = %s;\n\
          %s\n\
          %s;\n\
       }\n"

#define ARG_FN_EXPR_TEMPLATE "\n\
       {\n\
          %s %s = %s;\n\
          %s\
       }\n"

#define RVALTEMPLATEMAX 0xffff

#endif

