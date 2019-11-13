/* $Id: ctrlblk.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

#ifndef _CTRLBLK_H

#include "list.h"

/* 
 *  Return values of ctrl_block_state ().  Note that these are 
 *  in addition to the global states ctrlblk_pred and ctrlblk_blk.
 */
#define CTRLBLK_PRED 1   /*  Within control block predicate. */
#define CTRLBLK_BLK  2   /*  Within code block.              */
#define PRED_START   3   /*  At start of predicate.          */
#define PRED_END     4   /*  At end of predicate.            */
#define BLK_START    5   /*  At start of code block.         */
#define BLK_END      6   /*  At end of code block.           */
#define ELSE_BLK     7   /*  Within else block.              */
#define ELSE_START   8   /*  At start of else clause.        */
#define ELSE_END     9   /*  At end of else clause.          */

#define C_CTRL_BLK (ctrlblks[ctrlblk_ptr + 1])

typedef enum {
  stmt_break = 0,
  stmt_case = 1,
  stmt_do = 2,
  stmt_else = 3,
  stmt_for = 4,
  stmt_goto = 5,
  stmt_if = 6,
  stmt_switch = 7,
  stmt_while = 8,
  stmt_default = 9
} STMT_CLASS;

typedef struct {
  STMT_CLASS stmt_type;
  int id;                   /* Used as extent for loop labels.  */
  int level;                /* Ensures that we don't delete a control block
                               when the parser recurses within one. */
  int keyword_ptr,
    keyword2_ptr,           /* Second keyword if necessary - "while" */
                            /* for do... while loops, "else," if     */
                            /* necessary in, "if," statements.       */
    pred_start_ptr,
    pred_end_ptr,
    blk_start_ptr,
    blk_end_ptr;
  bool braces;               /* True if the block is enclosed in */
			    /* braces.                          */
  int else_start_ptr,       /* Start and end of else clauses.   */
    else_end_ptr;

  bool pred_args_d;          /* True if the value of a predicate */
                            /* object can change while          */
                            /* iterating.  Because loops may    */
                            /* independently change the value   */
                            /* of a variable, any reference to  */
                            /* variables within the             */
                            /* block causes pred_args_d to be   */
                            /* set.                             */

  bool pred_expr_evaled;

  LIST *pred_args;          /* List of statements that set the  */
                            /* predicate arguments on each      */
                            /* iteration.                       */

  int for_init_start,       /* Stack pointers of for statement  */
    for_init_end,           /* predicates.                      */
    for_term_start,
    for_term_end,
    for_inc_start,
    for_inc_end;
  int pred_state,           /* Saved states when control blocks */
    blk_state,              /* are nested.                      */
    else_state;
  LIST
    *for_init_args,         /* For loop init predicate calls,   */
    *for_init_methods,
    *for_init_cleanup,
    *for_inc_args,          /* and increment predicates.        */
    *for_inc_methods,
    *for_inc_cleanup;
  bool for_term_methods;    /* True if we're using Ctalk in a for loop
			       termination clause. */

} CTRLBLK;

#define _CTRLBLK_H
#endif
