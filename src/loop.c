/* $Id: loop.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2018 Robert Kiesling, rk3314042@gmail.com.
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

#define LOOPBLOCKMAX 0xffff

#define FOR_INIT_BLOCK "\n\
{\n\
  %s\n\
  %s\n\
  %s\n\
}\n"

#define FOR_INC_BLOCK FOR_INIT_BLOCK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "parser.h"
#include "ctrlblk.h"

extern CTRLBLK *ctrlblks[MAXARGS + 1];  /* Declared in control.c.         */
extern int ctrlblk_ptr;
extern bool ctrlblk_pred, ctrlblk_blk;

int do_predicate;                       /* Tell ctrlblk_state () that 
					   we should not change the control
					   block state while parsing a 
					   do predicate in its own frame. */

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

void init_loops (void) {
  do_predicate = FALSE;
}

static int for_init_label_is_receiver (CTRLBLK *c, MESSAGE *m) {
  LIST *l;
  for (l = c -> for_init_methods; l; l = l -> next) {
    if (strstr (M_NAME(m), (char *)l -> data))
      return TRUE;
  }
  return FALSE;
}

/*
 * Elides only the clause and a leading comma op.
 * 
 */
static void mark_for_init_clause_as_output (MESSAGE_STACK messages,
					    int rcvr_idx,
					    CTRLBLK *c) {
  int clause_start_delim_idx;
  int clause_start;
  int i;

  clause_start_delim_idx = prevlangmsg (messages, rcvr_idx);

  if (M_TOK(messages[clause_start_delim_idx]) == ARGSEPARATOR)
    clause_start = clause_start_delim_idx;
  else
    clause_start = rcvr_idx;

  for (i = clause_start; i >= c -> for_init_end; i--) {
    if (M_TOK(messages[i]) == SEMICOLON)
      break;
    ++messages[i] -> evaled;
    ++messages[i] -> output;
    if (M_TOK(messages[i]) == ARGSEPARATOR)
      break;
  }

}

int loop_pred_end (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  LIST *l, *l_method, *l_cleanup;
  int i;
  MESSAGE *m;
  char arg_push_buf[MAXMSG],
    method_buf[MAXMSG],
    arg_pop_buf[MAXMSG],
    blk_buf[LOOPBLOCKMAX],
    expr_buf_out[MAXMSG];

  if (ctrlblk_ptr == MAXARGS)
    return ERROR;

  c = ctrlblks[ctrlblk_ptr + 1];

  /*
   *  Insert the initialization predicates of a for loop.
   *
   *  TODO - All of the = methods use one arg - if there's
   *  ever a case where the initialization method uses more
   *  than one arg, then this is going to need an overhaul.
   */

  if (c -> stmt_type == stmt_for) {
    if (c -> for_init_methods != NULL) {

      /*
       *  Collate the initialization method calls.
       */
      for (l = c -> for_init_args, l_method = c -> for_init_methods, 
	     l_cleanup = c -> for_init_cleanup;
	   l; l = l -> next, l_method = l_method -> next, 
	     l_cleanup = l_cleanup -> next) {
	strcpy (arg_push_buf, (char *)l -> data);
	strcpy (method_buf, (char *)l_method -> data);
	/* Add a semicolon here because we've diverted the statement
	   out of the actual C context. */
	strcatx2 (method_buf, ";\n", NULL);
	strcpy (arg_pop_buf, (char *)l_cleanup -> data);
        sprintf (blk_buf, FOR_INIT_BLOCK, arg_push_buf, method_buf, arg_pop_buf);
        fileout (blk_buf, FALSE, c -> keyword_ptr);
      }

      /* Only elide the comma op if the previous clause was
	 diverted and we don't need the delimiter any more. */
      for (i = c -> for_init_start; i >= c -> for_init_end; i--) {
	m = messages[i];
	int __prev_tok, __prev_tok_2;
	if (M_TOK(m) == LABEL) {
	  if (for_init_label_is_receiver (c, m)) {
	    mark_for_init_clause_as_output (messages, i, c);
	  } else {
	    __prev_tok = prevlangmsg (messages, i);
	    __prev_tok_2 = prevlangmsg (messages, __prev_tok);
	    if (M_TOK(messages[__prev_tok]) ==  ARGSEPARATOR) {
	      if (messages[__prev_tok_2] -> output) {
		++messages[__prev_tok] -> evaled;
		++messages[__prev_tok] -> output;
	      }
	    }
	  }
	}
      }

      /* *Also* - check for a trailing comma op directly followed
	 by a semicolon. Any more complicated and this will probably
	 need a MESSAGE attribute, and separate function anyway.
      */
      int __next_tok, __next_tok_2, comma_done;
      for (__next_tok = c -> for_init_start, comma_done = FALSE; 
	   (__next_tok >= c -> for_init_end) && !comma_done; 
	   --__next_tok) {
	if (M_TOK(messages[__next_tok]) == ARGSEPARATOR) {
	  for (__next_tok_2 = __next_tok - 1; 
	       (__next_tok_2 >= c -> pred_end_ptr) && !comma_done; 
	       --__next_tok_2) {
	    if (M_ISSPACE(messages[__next_tok_2]))
	      continue;
	    if (messages[__next_tok_2] -> output)
	      continue;
	    if (M_TOK(messages[__next_tok_2]) == SEMICOLON) {
	      ++messages[__next_tok] ->  evaled;
	      ++messages[__next_tok] ->  output;
	      comma_done = TRUE;
	    } 
	  }
	}
      }

    }
  }

  if (c -> stmt_type == stmt_do) {
    if (! c -> pred_expr_evaled && 
	expr_has_objects (messages, 
			  c -> pred_start_ptr - 1, 
			  c -> pred_end_ptr + 1)) {
      fileout 
	(fmt_default_ctrlblk_expr 
	 (messages, c -> pred_start_ptr - 1, 
	  c -> pred_end_ptr + 1, FALSE, expr_buf_out), FALSE, 
	       c -> pred_start_ptr - 1);
      c -> pred_expr_evaled = TRUE;
    }
  }
  return SUCCESS;
}

int loop_block_start (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  static char blk_buf[MAXMSG];
  char expr_buf_out[MAXMSG];
  int i;

  if (ctrlblk_ptr == MAXARGS)
    return ERROR;

  c = ctrlblks[ctrlblk_ptr + 1];

  /*
   *  If, while, and do statements only need braces around the loop
   *  block in case the interpreter places function calls within
   *  the block.
   */

  if ((c -> stmt_type == stmt_if)) {
    if (!c -> braces)
      fileout ("\n{\n", FALSE, c -> blk_start_ptr);
  }
      
  if (c -> stmt_type == stmt_while) {
    if ((!c -> braces) && (M_TOK(messages[c -> blk_start_ptr]) != SEMICOLON))
      fileout ("\n{\n", FALSE, ptr);

    /*
     *   Check that the expression contains objects or if the
     *   expression has a C variable and operator before the
     *   object, in which case the expression is evaluated and
     *   output in default_method () in rexpr.c.
     */
    if (! c -> pred_expr_evaled && 
	expr_has_objects (messages, 
			  c -> pred_start_ptr - 1, 
			  c -> pred_end_ptr + 1)) {
      /* Should this be ctrlblk_pred_rt_expr? 
	 No - it causes timing errors in the output buffering,
	 but we could make a unique fn if necessary.
      */
      fileout 
	(fmt_default_ctrlblk_expr 
	 (messages, c -> pred_start_ptr - 1, 
	  c -> pred_end_ptr + 1, FALSE, expr_buf_out), FALSE, 
	 c -> pred_start_ptr - 1);

      for (i = c -> pred_start_ptr - 1; i >= c -> pred_end_ptr + 1; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
      c -> pred_expr_evaled = TRUE;

    } /* if (expr_has_objects (messages, c -> pred_start_ptr - 1, ...  */
  } /* if (c -> stmt_type == stmt_while) */

  if (c -> stmt_type == stmt_do) {
    if (!c -> braces) 
       fileout ("\n{\n", FALSE, c -> blk_start_ptr + 1);
  }

  if (c -> stmt_type == stmt_for) {

    if (c -> braces == FALSE) {           /* Insert opening brace if  */
      fileout ("\n{\n", FALSE, ptr);  /*  necessary.     */
    }

    if (c -> for_term_methods) { /* Otherwise we don't need to frob
				    the termination clause if it contains
				    only C statements. */
      for_term_rt_expr  (messages, c -> for_term_start,
			 &(c ->for_term_end), blk_buf);
      for (i = c -> for_term_start; i >= c -> for_term_end; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }

    }
  }

  return SUCCESS;
}

static bool preceding_else (MESSAGE_STACK messages, int if_ptr) {
  int prev_msg;
  if ((prev_msg = prevlangmsgstack (messages, if_ptr)) != ERROR) {
    if (str_eq (M_NAME(messages[prev_msg]), "else")) {
      return true;
    }
  }
  return false;
}

/*
 *   Here check all of the loops if they are nested.  If nested
 *   loops don't have braces, then the loops' blk_end_ptr will
 *   be the same - the semicolon that terminates the loop body
 *   statement.
 *
 *   "Else," clauses if in, "if... else," statements get braces
 *   here also if necessary.
 */

int loop_block_end (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  char end_buf[MAXMSG];
  int blk_ptr;
  char arg_push_buf[MAXMSG],
    method_buf[MAXMSG],
    arg_pop_buf[MAXMSG],
    blk_buf[LOOPBLOCKMAX];
  LIST *l;

  if (ctrlblk_ptr == MAXARGS)
    return ERROR;

  for (blk_ptr = ctrlblk_ptr + 1; blk_ptr <= MAXARGS; blk_ptr++) {

    c = ctrlblks[blk_ptr];

    if (c -> blk_end_ptr != ptr)
      continue;

    if (c -> level < CURRENT_PARSER -> level)
      break;

    /*
     *  If statements without braces also need braces
     *  around an else clause.  The, "else," keyword and
     *  the block are in the next frame, so scan for them individually.
     */

    if (c -> stmt_type == stmt_if) {
      if (c -> braces == FALSE) {
  
        /*
         *  If there's an, "else," clause, anchor the
         *  closing brace on the else keyword.  Otherwise,
         *  anchor it on the end of the statement.
         *
         *  Also anchor the opening brace of the, "else,"
         *  clause on the, "else," keyword.
         *
         *  Anchoring the braces in this manner allows
         *  the code generation in method_args () and
         *  elsewhere to only worry about placing library
         *  calls at the beginning of the source statement's 
         *  frame.
         *
         *  If_else_end (), below, handles the end of an,
         *  "else," clause, in case one of the method functions
         *  needs to anchor a library call on the close of
         *  a single-line, "else," block, which is the
         *  semicolon that terminates the statement.
         */

        if (c -> else_start_ptr == 0) {
          strcpy (end_buf, "\n}\n\n");
          fileout (end_buf, FALSE, ptr - 1);
        } else {
          strcpy (end_buf, "\n}\n ");
          fileout (end_buf, FALSE, c -> keyword2_ptr);

          strcpy (end_buf, " {\n");
          fileout (end_buf, FALSE, c -> keyword2_ptr - 1);
        }      

	/* If there's an else preceding the if, without
	   braces, then we've interpolated braces between
	   the else and the if, so add an extra closing brace
	   to match the brace after the else. E.g.,

	   if ( ... )
	     <expr>
           else if ( ... )
             <expr>

	   because we parse the second "if" as a separate
           control block, we've inserted braces before and after
           the else, e.g.,

           if ( ... ) {
             <expr>
           } else {    <----- this brace is extra.
            if ( ... ) {
              <expr>
            } }        <------ so we add a second brace here. 
	*/

	if (preceding_else (messages, c -> keyword_ptr)) {
          strcpy (end_buf, "\n}\n\n");
          fileout (end_buf, FALSE, ptr - 1);
	}
      }
    }

    if (c -> stmt_type == stmt_while) {
      /*
       *  Also handle case where loop is empty.
       */
      if (c -> braces == FALSE && (c -> blk_start_ptr != c -> blk_end_ptr)) {
        fileout ("\n}\n", FALSE, c -> blk_end_ptr - 1);
      }
    }

    if (c -> stmt_type == stmt_do) {
      *end_buf = 0;
      if (c -> pred_args_d) {
        strcpy (end_buf, "\n  {\n");
        for (l = c -> pred_args, *method_buf = 0; l; l = l -> next) {
  	  strcatx2 (end_buf, " ", (char *)l -> data, NULL);
          if (!index ((char *)l -> data, ';'))
            strcatx2 (end_buf, ";\n", NULL);
        }
        strcatx2 (end_buf, "  }", NULL);
      }

      if (c -> braces == FALSE) {
        if (*end_buf) {
          strcatx2 (end_buf, "\n}\n\n", NULL);
        } else {
          strcpy (end_buf, "\n}\n\n");
        }
        fileout (end_buf, FALSE, ptr - 1);
      }
    }

    if (c -> stmt_type == stmt_for) {
      if (c -> for_inc_methods != NULL) {

        for (l = c -> for_inc_args, *arg_push_buf = 0; l; l = l -> next)
  	  strcatx2 (arg_push_buf, (char *)l -> data, NULL);
        /*
         * Same kludge as in the init block above, because the method's
         * context registers as C context.
         */
        for (l = c -> for_inc_methods, *method_buf = 0; l; l = l -> next) {
	  strcatx2 (method_buf, (char *)l -> data, ";\n", NULL);
        }
        for (l = c -> for_inc_cleanup, *arg_pop_buf = 0; l; l = l -> next)
	  strcatx2 (arg_pop_buf, (char *)l -> data, NULL);

        sprintf (blk_buf, FOR_INC_BLOCK, arg_push_buf, 
                 method_buf, arg_pop_buf);
        fileout (blk_buf, FALSE, 
	       ((c -> braces) ? ptr + 1: ptr - 1));

	strcpy (end_buf, ((c -> braces) ? "\n\n" : "\n}\n"));
        fileout (end_buf, FALSE, ptr - 1);
        loop_linemarker (messages, ptr - 1);    
      } else {
      /*
       *  Output a closing brace if necessary even if there are
       *  no predicates.
       */
        if (c -> braces == FALSE) {
	  strcpy (end_buf, "\n}\n\n");
          fileout (end_buf, FALSE, ptr - 1);
          loop_linemarker (messages, ptr - 1);    
        }
      }
    }
  }

  return SUCCESS;
}

int if_else_end (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  int blk_ptr;

  if (ctrlblk_ptr == MAXARGS)
    return ERROR;

  for (blk_ptr = ctrlblk_ptr + 1; blk_ptr <= MAXARGS; blk_ptr++) {

    c = ctrlblks[blk_ptr];

    if (c -> else_end_ptr != ptr)
      continue;

    if (c -> level < CURRENT_PARSER -> level)
      break;

    if ((c -> braces == FALSE) && (c -> else_end_ptr)) {
      fileout ("\n}\n\n", FALSE, c -> else_end_ptr - 1);
    }
  }

  return SUCCESS;
}
