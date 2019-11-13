/* $Id: pparser.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of ctalk.
  Copyright © 2005-2007 Robert Kiesling, rkiesling@users.sourceforge.net
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

#ifndef _PEXCEPT_H
#include "pexcept.h"
#endif

#ifndef _PPARSER_H
#define _PPARSER_H

#ifndef MAXMSG
#define MAXMSG 8192
#endif

#ifndef MAXARGS
#define MAXARGS 512
#endif

/* 
 *  Define the maximum filename length if the system hasn't 
 *  already defined it.
 */
#ifndef FILENAME_MAX
#define FILENAME_MAX 255
#endif

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

typedef enum {
  abs_path,            /* File delmited by quotes - '"'.         */
  inc_path             /* File delimited by angle braces - "<>". */
} INCLUDE_PATH_TYPE;

/*
 *    Preprocessor conditional expressions.
 */
typedef enum {
  null_expr,
  cond_expr,
  cond_expr_n,
  cond_expr_ifdef,
  cond_expr_ifndef,
  cond_expr_elif,
  cond_expr_endif,
  cond_expr_else
} COND;

typedef enum {
  if_val_undef = -1,
  if_val_false = 0,
  if_val_true = 1,
  if_val_already_true = 2
} COND_VAL;

typedef struct {
  COND expr;
  COND_VAL val;
} COND_EXPR;

typedef enum {      /* Which of the operands to ## to concatenate, */
  concat_null = 0,  /* and literalize.                             */
  concat_op_1,
  concat_op_2,
  concat_both,
  concat_literal
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


typedef struct _macro_arg {
  char name[MAXLABEL];
  CONCAT_MODE paste_mode;
} MACRO_ARG;

typedef struct _macro_symbol {
  int sig;
  char name[MAXLABEL];
  char value[MAXMSG];
  MACRO_ARG *m_args[MAXARGS];
  struct _macro_symbol *next;
  struct _macro_symbol *prev;
} DEFINITION;

typedef struct {
  char fn[FILENAME_MAX];
  int error_line, 
    error_column,
    parser_lvl;
} ERROR_LOCATION;

#endif

