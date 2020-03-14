/* $Id: control.c,v 1.3 2019/12/10 02:11:42 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2019 Robert Kiesling, rk3314042@gmail.com.
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

/*
 *  Functions to parse control structures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "ctrlblk.h"

extern int do_predicate;     /* Declared in loop.c.  True if parsing a 
				do predicate in its own frame.         */

extern int nolinemarker_opt; /* Declared in main.c.                    */
extern I_PASS interpreter_pass;
extern char input_source_file[FILENAME_MAX];

extern int error_line,       /* Declared in errorloc.c                 */
  error_column;

extern int fn_defined_by_header; /* Declared in fnbuf.c.               */

extern bool argblk;          /* Declared in argblk.c.                   */
extern ARGBLK *argblks[MAXARGS + 2];
extern int argblk_ptr;

CTRLBLK *ctrlblks[MAXARGS + 1];
int ctrlblk_ptr;

bool ctrlblk_pred,            /* Global states.                         */
  ctrlblk_blk,
  ctrlblk_else_blk;

bool for_init,                /* States of for loop predicates.         */
  for_term,
  for_inc;

static int loop_ext;         /* Unique extent for loop labels.         */

RT_EXPR_CLASS rt_expr_class = rt_expr_null;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

int get_ctrlblk_ptr (void) {
  return ctrlblk_ptr;
}

void init_control_blocks (void) {
  ctrlblk_ptr = MAXARGS;
  ctrlblk_pred = ctrlblk_blk = FALSE;
  for_init = for_term = for_inc = FALSE;
  loop_ext = 0;
}

void ctrlblk_push (CTRLBLK *c) {
  ctrlblks[ctrlblk_ptr--] = c;
}

CTRLBLK *ctrlblk_pop (void) {
  static CTRLBLK *c;
  c = ctrlblks[++ctrlblk_ptr];
  ctrlblks[ctrlblk_ptr] = NULL;

  return c;
}

/*
 *  Delete the predicate lists for do and while loops, and
 *  the control block struct.
 *
 *  TO DO - 
 *  Delete the predicate lists of for loops when they
 *  are finished.
 */

void delete_control_block (CTRLBLK *c) {
  delete_list (&c -> pred_args);
  delete_list (&c -> for_init_args);
  delete_list (&c -> for_init_methods);
  delete_list (&c -> for_init_cleanup);
  delete_list (&c -> for_inc_args);
  delete_list (&c -> for_inc_methods);
  delete_list (&c -> for_inc_cleanup);
  __xfree (MEMADDR(c));
}

CTRLBLK *new_ctrl_blk (void) {
  static CTRLBLK *c;

  if ((c = (CTRLBLK *) malloc (sizeof (CTRLBLK))) == NULL)
    return NULL;
  memset (c, 0, sizeof (CTRLBLK));
  return c;
}

/* 
 *  If the message stack pointer is past the end of the 
 *  innermost control block, also make sure that we haven't
 *  recursed within a control block, by checking the parser
 *  level, before deleting the control block.
 *
 *  If the control block is an if statement, also check 
 *  whether the parser is past the else clause, if any.
 *
 *  NOTE - The control block states should be completely
 *  reset by ctrl_block_state ().  If this turns out 
 *  not to be the case, then remove_ctrl_block () should
 *  return whether it has deleted a block, and then
 *  all of the states should be completely reset.
 */

static void remove_ctrl_block (int ptr) {
 
  if (C_CTRL_BLK -> stmt_type == stmt_if) {
    if (C_CTRL_BLK -> else_start_ptr) {
      if ((ptr < C_CTRL_BLK -> else_end_ptr) &&
	  (C_CTRL_BLK -> level >= CURRENT_PARSER -> level)) {
 	ctrlblk_blk = ctrlblk_pred = ctrlblk_else_blk = FALSE;
	delete_control_block (ctrlblk_pop ());
      }
    } else {
      if ((ptr < C_CTRL_BLK -> blk_end_ptr) &&
	  (C_CTRL_BLK -> level >= CURRENT_PARSER -> level)) {
 	ctrlblk_blk = ctrlblk_pred = ctrlblk_else_blk = FALSE;
	delete_control_block (ctrlblk_pop ());
      }
    }
  } else {
    if (C_CTRL_BLK -> stmt_type == stmt_do) {
      if ((ptr < C_CTRL_BLK -> pred_end_ptr) &&
	  (C_CTRL_BLK -> level >= CURRENT_PARSER -> level)) {
	ctrlblk_blk = ctrlblk_pred = FALSE;
	delete_control_block (ctrlblk_pop ());
      }
    } else {
      if ((ptr < C_CTRL_BLK -> blk_end_ptr) &&
	  (C_CTRL_BLK -> level >= CURRENT_PARSER -> level)) {
	ctrlblk_blk = ctrlblk_pred = FALSE;
	delete_control_block (ctrlblk_pop ());
      }
    }
  }
}      

int ctrl_block_state (int ptr) {

  if (ctrlblk_ptr == MAXARGS)
    return FALSE;

  if (do_predicate || (interpreter_pass == expr_check))
    return FALSE;

  remove_ctrl_block (ptr);
      
  if (ctrlblk_ptr == MAXARGS)
    return FALSE;

  if ((ptr <= C_CTRL_BLK -> pred_start_ptr) &&
      (ptr >= C_CTRL_BLK -> pred_end_ptr))
    ctrlblk_pred = TRUE;
  else
    ctrlblk_pred = FALSE;
      
  if ((ptr <= C_CTRL_BLK -> blk_start_ptr) &&
      (ptr >= C_CTRL_BLK -> blk_end_ptr))
    ctrlblk_blk = TRUE;
  else 
    ctrlblk_blk = FALSE;
  
  if (C_CTRL_BLK -> stmt_type == stmt_if) {
  if ((ptr <= C_CTRL_BLK -> else_start_ptr) &&
      (ptr >= C_CTRL_BLK -> else_end_ptr))
    ctrlblk_else_blk = TRUE;
  else 
    ctrlblk_else_blk = FALSE;
  }

  if (C_CTRL_BLK -> stmt_type == stmt_for) {
    if ((ptr <= C_CTRL_BLK -> for_init_start) &&
	(ptr >= C_CTRL_BLK -> for_init_end)) {
      for_init = TRUE;
      for_term = for_inc = FALSE;
    }
    if ((ptr <= C_CTRL_BLK -> for_term_start) &&
	(ptr >= C_CTRL_BLK -> for_term_end)) {
      for_term = TRUE;
      for_init = for_inc = FALSE;
    }
    if ((ptr <= C_CTRL_BLK -> for_inc_start) &&
	(ptr >= C_CTRL_BLK -> for_inc_end)) {
      for_inc = TRUE;
      for_init = for_term = FALSE;
    }
  } else {
    for_init = for_term = for_inc = FALSE;
  }

  if (ptr == C_CTRL_BLK -> pred_start_ptr)
    return PRED_START;
  if (ptr == C_CTRL_BLK -> pred_end_ptr)
    return PRED_END;
  if (ptr == C_CTRL_BLK -> blk_start_ptr)
    return BLK_START;
  if (ptr == C_CTRL_BLK -> blk_end_ptr)
    return BLK_END;
  if (ptr == C_CTRL_BLK -> else_start_ptr)
    return ELSE_START;
  if (ptr == C_CTRL_BLK -> else_end_ptr)
    return ELSE_END;
  if (ctrlblk_pred)
    return CTRLBLK_PRED;
  if (ctrlblk_blk)
    return CTRLBLK_BLK;
  if (ctrlblk_else_blk)
    return ELSE_BLK;

  remove_ctrl_block (ptr);
      
  return FALSE;

}

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c.*/
extern int new_method_ptr;
extern HASHTAB defined_instancevars; /* declared in primitives.c. */
extern OBJECT *rcvr_class_obj;   /* Declared in lib/rtnwmthd.c and */

static bool if_stmt_label_is_defined (MESSAGE_STACK messages, int ptr) {

  OBJECT *tmp;
  MESSAGE *m;
  METHOD *method;
  int i, prev_idx;
  CVAR *c;

  m = messages[ptr];

  if (m -> attrs & TOK_SELF || m -> attrs & TOK_SUPER) {
    if (new_method_ptr == MAXARGS && !argblk) {
      warning (m, "Label, \"%s,\" used within a C function.",
	       M_NAME(m));
    }
    return true;
  }

  if (((tmp = get_object (m -> name, NULL)) != NULL) ||
      ((tmp = get_class_object (m -> name)) != NULL))
    return true;

  if (((c = get_local_var (messages[ptr] -> name)) != NULL) ||
      ((c = get_global_var (messages[ptr] -> name)) != NULL))
    return true;

  if (is_instance_method (messages, ptr) ||
      is_class_method (messages, ptr))
    return true;

  if (_hash_get (defined_instancevars, M_NAME(messages[ptr])))
    return true;

  if ((interpreter_pass == method_pass) ||
      (interpreter_pass == expr_check)) {
    if (new_method_ptr < MAXARGS) {
      if ((method = new_methods[new_method_ptr + 1] -> method) != NULL) {
	for (i = 0; i < method -> n_params; i++) {
	  if (!strcmp (method -> params[i] -> name, m -> name))
	    return true;
	}
      }
    }
  }

  if (interpreter_pass == method_pass) {
    /* Check for a "self <instancevar>" expression. */
    if ((prev_idx = prevlangmsg (messages, ptr)) != ERROR) {
      if (messages[prev_idx] -> attrs & TOK_SELF) {
	if (IS_OBJECT(rcvr_class_obj)) {
	  OBJECT *var;
	  for (var = rcvr_class_obj -> instancevars; var; var = var -> next) {
	    if (str_eq (var -> __o_name, M_NAME(m))) {
	      return true;
	    }
	  }
	}
      }
    }
  }

  return false;
}

int control_structure (MESSAGE_STACK messages, int ptr) {

  if (M_TOK(messages[ptr]) != LABEL)
    return ERROR;

  /*
   *  Case statements only use integers as 
   *  their arguments,
   */
#if 0
  if (str_eq (messages[ptr] -> name, "case"))
    return case_stmt (messages, ptr);
#endif
  
  if (str_eq (messages[ptr] -> name, "do"))
    return do_stmt (messages, ptr);
  
  if (str_eq (messages[ptr] -> name, "for"))
    return for_stmt (messages, ptr);

  if (str_eq (messages[ptr] -> name, "if"))
    return if_stmt (messages, ptr);

  if (str_eq (messages[ptr] -> name, "switch"))
    return switch_stmt (messages, ptr);
  
  if (str_eq (messages[ptr] -> name, "goto"))
    return goto_stmt (messages, ptr);

  /*
   *  Make sure a, "while," is not part of a, "do .. while."
   */
  if (str_eq (messages[ptr] -> name, "while") &&
      ! messages[ptr] -> evaled && ! messages[ptr] -> output)
      return while_stmt (messages, ptr);

  return ERROR;
}

#if 0
/* This is a no-op for now. */
int case_stmt (MESSAGE_STACK messages, int ptr) {
  return SUCCESS;
}
#endif

int for_predicates (CTRLBLK *c, MESSAGE_STACK messages, int ptr) {

  int i,
    lasttoken,
    lastmsgptr = ptr,        /* Avoid a warning. */
    nexttoken;
  MESSAGE *m, *m_next;
  enum {init, term, inc, null} for_state;
  
  for_state = null;

  for (i = c -> pred_start_ptr, 
	 c -> for_init_start = ERROR, 
	 c -> for_init_end = ERROR,
	 c -> for_term_start = ERROR,
	 c -> for_term_end = ERROR,
	 c -> for_inc_start = ERROR,
	 c -> for_inc_end = ERROR,
	 lasttoken = ERROR;
       i >= c -> pred_end_ptr; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    m = messages[i];

    if ((nexttoken = nextlangmsg (messages, i)) != ERROR)
      m_next = messages[nexttoken];
    else
      m_next = NULL;

    switch (lasttoken)
      {
      case OPENPAREN:
	if (lastmsgptr == c -> pred_start_ptr) {
	  for_state = init;
	  c -> for_init_start = i;
	  /*
	   *  Handle emtpy init predicate.
	   */
	  if (m -> tokentype == SEMICOLON)
	    c -> for_init_end = i;
	}
	break;
      case SEMICOLON:
	switch (for_state) 
	  {
	  case init:
	    for_state = term;
	    c -> for_term_start = i;
	    /*  Handle emtpy predicates. */
	    if (m -> tokentype == SEMICOLON)
	      c -> for_term_end = i;
	    break;
	  case term:
	    c -> for_term_end = prevlangmsg (messages, lastmsgptr);
	    for_state = inc;
	    c -> for_inc_start = i;
	    /* Empty predicate. */
	    if (m -> tokentype == CLOSEPAREN)
	      c -> for_inc_start = c -> for_inc_end = lastmsgptr;
	    break;
	  case inc:
	  case null:
	    break;
	  }
	break;
      case ERROR:
	break;
      default:
	if (m_next && IS_MESSAGE (m_next) &&
	    (m_next -> tokentype == SEMICOLON)) {
	  switch (for_state)
	    {
	    case init:
	      c -> for_init_end = i;
	      break;
	    case term:
	      c -> for_term_end = i;
	      break;
	    case inc:
	    case null:
	      break;
	    }
	}
	if (m_next && IS_MESSAGE (m_next) &&
	    (m_next -> tokentype == CLOSEPAREN) &&
	    (nexttoken == c -> pred_end_ptr)) {
	  c -> for_inc_end = i;
	}
      }

    if ((m -> tokentype == CLOSEPAREN) && (i == c -> pred_end_ptr)) {
      c -> for_inc_end = lastmsgptr;
      if (c -> for_term_end == -1) {
	warning (messages[ptr], "for loop syntax error.");
	c -> for_term_end = lastmsgptr;
      }
      goto done;
    }


    lasttoken = m -> tokentype;
    lastmsgptr = i;
  }

 done:
  return SUCCESS;

}

int for_stmt (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  int i, j, k,
    stack_end,
    p_start,
    /* p_end, *//***/
    lookahead,
    blk_level,
    paren_level,
    array_level,
    last_frame_ptr,
    start_frame_ptr;
  FRAME *this_frame,
    *next_frame;
  MESSAGE *m;
  /* int n_block_levels; *//*** save for now in case we need it again */
  int n_pred_objects;
  OBJECT *pred_objects[MAXARGS];  /* Arrays of object references allow */
                                  /* the code to keep track of objects */
                                  /* without duplicating them.         */

  loop_linemarker (messages, ptr);
  
  if ((c = new_ctrl_blk ()) == NULL)
    error (messages[ptr], "for_stmt: %s.", strerror (errno));

  ctrlblk_push (c);

  c -> stmt_type = stmt_for;
  c -> id = loop_ext++;
  c -> level = CURRENT_PARSER -> level;
  c -> keyword_ptr = ptr;

  stack_end = get_stack_top (messages);

  start_frame_ptr = CURRENT_PARSER -> frame;
  last_frame_ptr = get_frame_pointer ();

  for (i = ptr, p_start = ERROR; i > stack_end; i--) {

    m = messages[i];

    switch (m -> tokentype)
      {
      case LABEL:
	break;
      case OPENPAREN:
	if (p_start == ERROR) {
	  p_start = c -> pred_start_ptr = i;
	  if ((c -> pred_end_ptr = match_paren (messages, i, stack_end))
	      == ERROR) {
	    warning (messages[i], "Mismatched parentheses.");
	    return ERROR;
	  }
	  /* c -> pred_end_ptr = p_end; *//***/
	  (void) for_predicates (c, messages, i);
	}
	/*
	 *  Find the start and end of the block.
	 */

	/*
	 *  Look for the next message in the frame.
	 */
	/*if ((lookahead = nextlangmsg (messages, p_end)) == ERROR) { *//***/
	if ((lookahead = nextlangmsg (messages, c -> pred_end_ptr)) == ERROR) {
	  /*
	   *  If at the end of the frame the block is a single 
	   *  statement and should be contained in the next
	   *  frame.
	   */
	  c -> braces = FALSE;
	  /* c -> blk_start_ptr = p_end - 1; *//***/
	  c -> blk_start_ptr = c -> pred_end_ptr - 1;
	  c -> blk_end_ptr = -1;

 	  for (j = start_frame_ptr - 1; 
 	       ((j >= last_frame_ptr) &&
 		(c -> blk_end_ptr == -1)); 
 	       j--) {
 	    this_frame = frame_at (j);
 	    next_frame = frame_at (j - 1);
 	    for (k = this_frame -> message_frame_top;
 		 ((k > next_frame -> message_frame_top) &&
 		    (c -> blk_end_ptr == -1));
 		 k--) {
 	      if ((messages[k] -> tokentype == NEWLINE) ||
 		  (messages[k] -> tokentype == WHITESPACE))
 		continue;
 	      if (messages[k] -> tokentype == SEMICOLON)
 		c -> blk_end_ptr = k;
 	    }
	  }
	} else {
	  /*
	   *  Block is in the same frame as predicate.
	   * 
	   *  Empty block terminated by a semicolon.
	   */
	  if (messages[lookahead] -> tokentype == SEMICOLON) {
	    c -> blk_start_ptr = c -> blk_end_ptr = lookahead;
	    c -> braces = FALSE;

	    /*
	     *  Check for nested loops without braces.
	     */
	    for (k = ctrlblk_ptr + 2; k <= MAXARGS; k++) {
	      if (ctrlblks[k] -> level < CURRENT_PARSER -> level)
		break;
	      if (ctrlblks[k] -> braces == FALSE) {
		ctrlblks[k] -> blk_end_ptr = c -> blk_end_ptr;
	      }
	    }
	  } else {
	    if (messages[lookahead] -> tokentype == OPENBLOCK) {
	      c -> braces = TRUE;
	      c -> blk_start_ptr = lookahead;
	      for (j = lookahead, blk_level = 0; j > stack_end; j--) {
		if (messages[j] -> tokentype == OPENBLOCK)
		  ++blk_level;
		if (messages[j] -> tokentype == CLOSEBLOCK) {
		  if (--blk_level == 0) {
		    /* int j_1; *//*** save for now... */
		    c -> blk_end_ptr = j;
#if 0 /* *** save for now in case we need it again, plus var decls above. */
      /* might need reworking... */
		    
		    n_block_levels = parsers[current_parser_ptr] -> block_level;
		    for (j_1 = c -> blk_end_ptr - 1;  
			 (j_1 > stack_end) && n_block_levels; j_1--) {
		      switch (M_TOK(messages[j_1]))
			{
			case CLOSEBLOCK:
			  --n_block_levels;
			  break;
			case OPENBLOCK:
			  ++n_block_levels;
			  break;
			}
		    }
		    if (n_block_levels) {
		      warning (messages[c->blk_start_ptr], 
			       "Missing brace after, \"for,\" loop body.");
		    }
#endif		    
		    goto parsed;
		  }
		}
	      }
	    } else {
	      c -> braces = FALSE;
	      c -> blk_start_ptr = lookahead;
	      for (j = lookahead, 
		     blk_level = 0, paren_level = 0, array_level = 0; 
		   j > stack_end; j--) {
		switch (M_TOK(messages[j]))
		  {
		  case OPENBLOCK:
		    ++blk_level;
		    break;
		  case CLOSEBLOCK:
		    --blk_level;
		    /* The case of an expression like:
		       for (...)
		         if (...) {
			   ...
                         }

		       We need to end on the closing brace.

		       We might still enclose the for () block in
                       braces later on, though.
		    */
		    if ((blk_level == 0) && (c -> braces == FALSE)) {
		      c -> blk_end_ptr = j;
		      goto parsed;
		    }
		    break;
		  case OPENPAREN:
		    ++paren_level;
		    break;
		  case CLOSEPAREN:
		    --paren_level;
		    break;
		  case ARRAYOPEN:
		    ++array_level;
		    break;
		  case ARRAYCLOSE:
		    --array_level;
		    break;
		  case SEMICOLON:
		    if (!blk_level && !paren_level && !array_level) {
		      c ->blk_end_ptr = j;
		      goto parsed;
		    }
		    break;
		  }
	      }
	    }
	  }
	}
	goto done;
	break;
      }
  }

 parsed:
  /* 
   *   Find the objects in the predicate.
   */

  for (i = c -> pred_start_ptr, n_pred_objects = 0; 
       i >= c -> pred_end_ptr; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    if (IS_C_OP(M_TOK(messages[i])))
      continue;

    m = messages[i];
    if ((pred_objects[n_pred_objects] = get_object (m -> name, NULL))
	!= NULL)
      ++n_pred_objects;
  }

  /*
   *  Find the objects in the block.  Because methods may independently
   *  change the value of objects, any reference to a predicate object
   *  causes the function to set c -> pred_objs_d.  Only one reference
   *  is needed to set c -> pred_objs_d, so the iteration doesn't 
   *  need to look any further.
   */

  for (i = c -> blk_start_ptr; i >= c -> blk_end_ptr; i--) {
    for (j = 0; j < n_pred_objects; j++) {
      if (str_eq (messages[i] -> name, pred_objects[j] -> __o_name)) {
	c -> pred_args_d = TRUE;
	goto done;
      }
    }
  }

 done:

  return SUCCESS;
}

/*
 *  Try to parse a single block expression like do {...} while (...);
 *  that isn't enclosed by braces.
 */
static int handle_blk_expr_framing_error_a (CTRLBLK *c, MESSAGE_STACK messages) {
  int i, stack_end, n_internal_blocks;
  stack_end = get_stack_top (messages);
  if (c -> blk_start_ptr && !c -> blk_end_ptr) {
    for (i = c -> blk_start_ptr, n_internal_blocks = 0; 
	 (i > stack_end) && (c -> blk_end_ptr == 0); i--) {
      if (M_TOK(messages[i]) == OPENBLOCK)
	++n_internal_blocks;
      if (M_TOK(messages[i]) == CLOSEBLOCK)
	--n_internal_blocks;
      if ((M_TOK(messages[i]) == SEMICOLON) && !n_internal_blocks)
	c -> blk_end_ptr = i;
    }
  }
  return (c -> blk_end_ptr) ? c -> blk_end_ptr : ERROR;
}

static int find_closing_brace_on_block (MESSAGE_STACK messages, 
				  int open_block_idx) {
  int i, stack_top_idx, n_blocks;
  
  stack_top_idx = get_stack_top (messages);

  n_blocks = 0;

  for (i = open_block_idx; i > stack_top_idx; i--) {
    
    if (M_TOK(messages[i]) == OPENBLOCK)
      ++n_blocks;

    if (M_TOK(messages[i]) == CLOSEBLOCK) {
      --n_blocks;
      if (n_blocks == 0)
	return i;
    }
  }

  return ERROR;
}

/*
 *  Check for end of an unframed block. The only special case should
 *  be a block that is a 
 *
 *     do ...; while (); 
 *
 *  statement.
 *
 *  Doesn't use nextlangmsg to scan forward because the scan may need
 *  to cross a frame boundary.
 *
 *  Partially superseded by the parse_<keyword>_body () functions,
 *  below.
 */
static bool is_end_of_unframed_block (CTRLBLK *c,
				     MESSAGE_STACK messages, int idx) {
  MESSAGE *m, *m_next_tok;
  int next_tok_idx, stack_top;
  m = messages[idx];
  if (M_TOK(m) == SEMICOLON) {
    stack_top = get_stack_top (messages);
    for (next_tok_idx = idx - 1; 
	 (next_tok_idx > stack_top) && M_ISSPACE(messages[next_tok_idx]);
	 next_tok_idx--)
      ;
    if (next_tok_idx <= stack_top) {
      c -> blk_end_ptr = stack_top + 1;
      return TRUE;
    }
    m_next_tok = messages[next_tok_idx];
    if (str_eq (M_NAME (m_next_tok), "while")) {
      for (next_tok_idx -= 1; 
	   (next_tok_idx > stack_top) ; next_tok_idx--) {
	m_next_tok = messages[next_tok_idx];
	if (M_TOK(m_next_tok) == SEMICOLON) {
	  c -> blk_end_ptr = next_tok_idx;
	  return TRUE;
	}
      }
    } else {
      c -> blk_end_ptr = idx;
      return TRUE;
    }
  }
  return FALSE;
}

/* 
 *  We don't (yet) need to handle do statements with braces separately:
 *
 *  if (...)
 *   do {
 *        ...
 *      } while (...);
 */
static void parse_do_body (MESSAGE_STACK messages, int i, CTRLBLK *c) {
  int semicolon_1, semicolon_2;
  int next_tok_idx;
  int stack_top;

  if ((next_tok_idx = nextlangmsg (messages, i)) != ERROR) {
    if (M_TOK(messages[next_tok_idx]) != OPENBLOCK) {
      /* Just scan forward to the second semicolon. */
      stack_top = get_stack_top (messages);
  
      if ((semicolon_1 = scanforward (messages, i, stack_top, SEMICOLON))
	  != ERROR) {
	if ((semicolon_2 = scanforward (messages, semicolon_1 - 1, stack_top,
					SEMICOLON)) != ERROR) {
	  c -> blk_end_ptr = semicolon_2;
	}
      }
    }
  }
}

static void parse_for_body (MESSAGE_STACK messages, int i, CTRLBLK *c) {
  int next_tok_idx, eop_idx, semicolon_1, close_brace_idx;
  int stack_top;

  if ((next_tok_idx = nextlangmsg (messages, i)) != ERROR) {
    if (M_TOK(messages[next_tok_idx]) == OPENPAREN) {
      stack_top = get_stack_top (messages);
      if ((eop_idx = match_paren (messages, next_tok_idx,
				  stack_top)) != ERROR) {
	if ((next_tok_idx = nextlangmsgstack (messages, eop_idx)) != ERROR) {

	  if (M_TOK(messages[next_tok_idx]) != OPENBLOCK) {
	    /* Just scan to the semicolon if there are no braces. */

	    if ((semicolon_1 = scanforward (messages, 
					    next_tok_idx, stack_top,
					    SEMICOLON)) != ERROR) {
	      c -> blk_end_ptr = semicolon_1;
	    }

	  } else {
	    if ((close_brace_idx = 
		 find_closing_brace_on_block (messages, next_tok_idx))
		!= ERROR) {
	      c -> blk_end_ptr = close_brace_idx;
	    }
	  }

	}
      }
    }
  }
}

/*
 *  This is (so far) identical to parse_for_body ().
 */
static void parse_if_switch_while_body (MESSAGE_STACK messages, int i, CTRLBLK *c) {
  int next_tok_idx, eop_idx, semicolon_1, close_brace_idx;
  int stack_top;

  if ((next_tok_idx = nextlangmsg (messages, i)) != ERROR) {
    if (M_TOK(messages[next_tok_idx]) == OPENPAREN) {
      stack_top = get_stack_top (messages);
      if ((eop_idx = match_paren (messages, next_tok_idx,
				  stack_top)) != ERROR) {
	if ((next_tok_idx = nextlangmsgstack (messages, eop_idx)) != ERROR) {

	  if (M_TOK(messages[next_tok_idx]) != OPENBLOCK) {
	    /* Again, just scan to the semicolon if there are no braces. */

	    if ((semicolon_1 = scanforward (messages, 
					    next_tok_idx, stack_top,
					    SEMICOLON)) != ERROR) {
	      c -> blk_end_ptr = semicolon_1;
	    }
	  } else {
	    if ((close_brace_idx = 
		 find_closing_brace_on_block (messages, next_tok_idx))
		!= ERROR) {
	      c -> blk_end_ptr = close_brace_idx;
	    }
	  }
	}
      }
    }
  }
}

static void parse_goto_body (MESSAGE_STACK messages, int i, CTRLBLK *c) {
  int semicolon_idx, next_tok_idx;

  if ((next_tok_idx = nextlangmsg (messages, i)) != ERROR) {
    if ((semicolon_idx = scanforward 
	 (messages, 
	  next_tok_idx, get_stack_top (messages),
	  SEMICOLON)) != ERROR) {
      c -> blk_end_ptr = semicolon_idx;
    }
  }
}

/*
 *  This is also (so far) identical to parse_for_body ().
 */
static void parse_keyword_body (MESSAGE_STACK messages, int i, CTRLBLK *c) {
  int next_tok_idx, eop_idx, semicolon_1, close_brace_idx;
  int stack_top;

  if ((next_tok_idx = nextlangmsg (messages, i)) != ERROR) {
    if (M_TOK(messages[next_tok_idx]) == OPENPAREN) {
      stack_top = get_stack_top (messages);
      if ((eop_idx = match_paren (messages, next_tok_idx,
				  stack_top)) != ERROR) {
	if ((next_tok_idx = nextlangmsgstack (messages, eop_idx)) != ERROR) {

	  if (M_TOK(messages[next_tok_idx]) != OPENBLOCK) {
	    /* Again, just scan to the semicolon if there are no braces. */

	    if ((semicolon_1 = scanforward (messages, 
					    next_tok_idx, stack_top,
					    SEMICOLON)) != ERROR) {
	      c -> blk_end_ptr = semicolon_1;
	    }
	  } else {
	    if ((close_brace_idx = 
		 find_closing_brace_on_block (messages, next_tok_idx))
		!= ERROR) {
	      c -> blk_end_ptr = close_brace_idx;
	    }
	  }
	}
      }
    }
  }
}

int if_stmt (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  int i, j, j_prev,
    k = 0,             /* Initializers avoid warnings. */
    stack_end,
    frame_offset,
    start_frame,
    last_frame,
    n_parens,
    r,
    n_blocks = 0;
  FRAME *this_frame, 
    *next_frame;
  enum {
    if_null,
    if_keyword,
    if_pred_start,
    if_pred_end,
    if_blk_start,
    if_blk_end,
    if_else,
    if_else_start,
    if_else_end
  } if_state;

  loop_linemarker (messages, ptr);

  frame_offset = 0;
  if_state = if_null;

  last_frame = get_frame_pointer ();

  stack_end = get_stack_top (messages);

  this_frame = frame_at (CURRENT_PARSER -> frame + frame_offset);
  start_frame = CURRENT_PARSER -> frame;

  if ((c = new_ctrl_blk ()) == NULL)
    error (messages[ptr], "if_stmt: %s.", strerror (errno));

  ctrlblk_push (c);

  c -> stmt_type = stmt_if;
  c -> id = loop_ext++;
  c -> level = CURRENT_PARSER -> level;
  c -> keyword_ptr = ptr;

  if_state = if_keyword;

  for ( ; start_frame + frame_offset >= last_frame; --frame_offset) {

    this_frame = frame_at (CURRENT_PARSER -> frame + frame_offset);
    next_frame = frame_at (CURRENT_PARSER -> frame + frame_offset - 1);

    if (next_frame == NULL)
      return ERROR;

    switch (if_state)
      {
      case if_keyword:
	for (i = ptr, n_parens = 0, n_blocks = 0; 
	     (i > next_frame -> message_frame_top) && 
	       (if_state == if_keyword);
	     i--) {
	  if (messages[i] -> tokentype == OPENPAREN) {
	    c -> pred_start_ptr = i;
	    j_prev = -1;
	    for (j = i; j > stack_end; j--) {
	      if (M_ISSPACE(messages[j]))
		continue;
	      if (messages[j] -> tokentype == OPENPAREN) {
		++n_parens;
		if (n_parens == 1) {  /* Don't count nested parentheses. */
		  /*
		   *  We do a fixup below if a leading unary op
		   *  is overloaded by a method, in which case
		   *  the op gets included in the expression,
		   *  including a unary !.
		   */
		  j = leading_unary_op (messages, j);
		  c -> pred_start_ptr = j;
		  if ((c -> pred_end_ptr = 
		       match_paren (messages, j, stack_end)) == ERROR) {
		    warning (messages[j], "Mismatched parentheses.");
		    delete_control_block (ctrlblk_pop ());
		    return ERROR;
		  }
		}
	      }
	      if (messages[j] -> tokentype == CLOSEPAREN) {
		--n_parens;
		if (n_parens == 0) {
		  c -> pred_end_ptr = j;
		  if_state = if_pred_end;
		  /*
		   *  If the next message is an open block, then
		   *  the control block has braces.  If 
		   *  We get to the end of the frame, then 
		   *  look through the next frames for 
		   *  a label.
		   */
		  for (k = j - 1; 
		       (k > stack_end) && (if_state == if_pred_end); 
		       k--) {
		    switch (M_TOK(messages[k]))
		      {
		      case OPENBLOCK:
			c -> braces = TRUE;
			c -> blk_start_ptr = k;
			if_state = if_blk_start;
			n_blocks = 1;
			goto pred_frame_done;
			break;
		      case WHITESPACE:
		      case CR:
		      case LF:
		      case NEWLINE:
			/* TODO - Find out if there are any other tokens
			   (like PREPROCESS) that we need to skip here. */
			break;
		      default:
			c -> braces = FALSE;
			/* The block starts with a parenthesis,
			   it is part of the  expression, not a
			   delimiter. */
			if (M_TOK(messages[k]) == OPENPAREN) 
			  c -> blk_start_ptr = k + 1;
			else
			  c -> blk_start_ptr = k;
			if_state = if_blk_start;
			n_blocks = 1;
			goto pred_frame_done;
			break;
		      }
		  }
		}
	      }
	      if (M_TOK(messages[j]) == LABEL) {
		MESSAGE *m_lbl;
		m_lbl = messages[j];
		if (M_TOK(messages[j_prev]) == PERIOD ||
		    M_TOK(messages[j_prev]) == DEREF) {
		  if (!is_OBJECT_member (M_NAME(m_lbl)) &&
		      !is_struct_member_tok (messages, j)) {
		    warning (m_lbl, "Undefined label, \"%s.\"",
			     M_NAME(m_lbl));
		  }
		} else {
		  if (!fn_defined_by_header &&
#if defined(__APPLE__) && defined (__x86_64)
		      strncmp (M_NAME(messages[j]), "__builtin_", 10) &&
#endif		      
		      !if_stmt_label_is_defined (messages, j) && 
		      !is_object_or_param (M_NAME(m_lbl), NULL) &&
		      !is_proto_selector (M_NAME(m_lbl)) &&
		      !get_local_var (M_NAME(m_lbl)) &&
		      !get_typedef (M_NAME(m_lbl)) &&
		      !get_function (M_NAME(m_lbl)) &&
		      !is_fn_param (M_NAME(m_lbl)) && /***/
		      !is_enum_member (M_NAME(m_lbl)) &&
		      !is_instance_variable_message (messages, j) &&
		      !is_class_variable_message (messages, j) &&
		      !get_global_var (M_NAME(m_lbl)) &&
		      !get_function (M_NAME(m_lbl)) &&
		      !is_c_data_type (M_NAME(m_lbl)) &&
		      !find_library_include (M_NAME(m_lbl), FALSE) &&
		      !is_collection_initializer (j))
		    warning (m_lbl, "Undefined label, \"%s.\"",
			     M_NAME(m_lbl));
		}
	      }
	      j_prev = j;
	    }
	  }
	}
      pred_frame_done:
	/*
	 *  Find out which frame we ended on.  If the control block
	 *  has braces, go on to the next frame.  If there are no
	 *  braces, move back up one frame so the next loop will
	 *  also be on this frame.
	 */
	while (TRUE) {
	  if ((k <= this_frame -> message_frame_top) &&
	      (k > next_frame -> message_frame_top))
	    break;
	  --frame_offset;
	  this_frame = frame_at (CURRENT_PARSER -> frame + 
				 frame_offset);
	  if ((next_frame = frame_at (CURRENT_PARSER -> frame + 
				      frame_offset - 1)) == NULL) {
	    error (messages[ptr], "if_stmt (): parser error.\n");
	  }
	}
	if (c -> braces == FALSE)
	  ++frame_offset;
	break;
      case if_pred_start:
      case if_pred_end:
	/*
	 *  The predicate start and end occur in the same frame as 
	 *  the keyword, so the function looks for it in the
	 *  case above.
	 */
	break;
      case if_blk_start:
  	for (i = this_frame -> message_frame_top;
  	     (i > next_frame -> message_frame_top) && 
  	       (if_state == if_blk_start);
  	     i--) {
	  if (c -> braces == FALSE) {
	    if (!messages[i] || !IS_MESSAGE(messages[i])) {
	      if_state = if_blk_end;
	      c -> blk_end_ptr = i;
	    } else {
	      /*
	       *  But make sure that there isn't some construct
	       *  like a loop with braces as the if's expression.
	       */
	      /*
	       *  Fills in c -> blk_end_ptr;  Partially
	       *  superseded by the parse_<keyword>_body () functions.
	       */
	      if (is_end_of_unframed_block (c, messages, i)) {
		if_state = if_blk_end;
	      } else if (if_state == if_blk_start &&
			 (i == c -> blk_start_ptr)) {
		if (str_eq (M_NAME(messages[i]), "do")) {
		  parse_do_body (messages, i, c);
		  if_state = if_blk_end;
		} else if (str_eq (M_NAME(messages[i]), "for")) {
		  parse_for_body (messages, i, c);
		  if_state = if_blk_end;
		} else if (str_eq (M_NAME(messages[i]), "while") ||
			   str_eq (M_NAME(messages[i]), "if") ||
			   str_eq (M_NAME(messages[i]), "switch")) {
		  parse_if_switch_while_body (messages, i, c);
		  if_state = if_blk_end;
		} else if (str_eq (M_NAME(messages[i]), "goto")) {
		  parse_goto_body (messages, i, c);
		  if_state = if_blk_end;
		} else if (is_c_keyword (M_NAME(messages[i]))) {
		  parse_keyword_body (messages, i, c);
		  if_state = if_blk_end;
		}
	      }
	    }
	  } else {
	    if (messages[i] -> tokentype == OPENBLOCK)
	      ++n_blocks;
	    if (messages[i] -> tokentype == CLOSEBLOCK) {
	      --n_blocks;
	      if (n_blocks == 0) {
		if_state = if_blk_end;
		c -> blk_end_ptr = i;
	      }
	    }
	  }
	}
	if ((c -> braces == FALSE) && 
	    ((c -> blk_start_ptr == 0) || (c -> blk_end_ptr == 0))) {
	  if ((r = handle_blk_expr_framing_error_a (c, messages)) == ERROR) {
	    warning (messages[c->keyword_ptr], 
		     "if_stmt: Could not parse if block.\n");
	  }
	  if_state = if_blk_end;
	}
	break;
      case if_blk_end:
	/*
	 *  Look for an else clause.  We also have to 
	 *  back the frame offset up one so the next iteration
	 *  stays on this frame.  The opening brace of an
	 *  else clause is in the same frame as the keyword.
	 */
	for (i = this_frame -> message_frame_top; 
	     (i > next_frame -> message_frame_top) && 
	       (if_state == if_blk_end);
	     i--) {
	  if ((messages[i] -> tokentype == WHITESPACE) ||
	      (messages[i] -> tokentype == NEWLINE))
	    continue;
	  if (messages[i] -> tokentype == LABEL) {
	    if (str_eq (messages[i] -> name, "else")) {
	      if_state = if_else;
	      ++frame_offset;
	    } else {
	      if_state = if_null;
	    }
	  } else {
	    /*
	     *  If the preprocessor expands a macro into 
	     *  if () 
	     *    {...};
	     *  else
	     *   ...
	     *  Then elide the semicolon.
	     */
	    if ((messages[i] -> tokentype == SEMICOLON) && 
		(c -> braces)) {
	      messages[i] -> tokentype = WHITESPACE;
	      messages[i] -> name[0] = ' ';
	    }
	    if_state = if_null;
	  }
	}
	break;
      case if_else:
	/*
	 *  The next non-whitespace token after the, "else,"
	 *  keyword is the start of the else block.
	 */
	for (i = this_frame -> message_frame_top; 
	     (i > next_frame -> message_frame_top) && 
	       (if_state == if_else);
	     i--) {
	  if ((messages[i] -> tokentype == WHITESPACE) ||
	      (messages[i] -> tokentype == NEWLINE))
	    continue;
	  if ((messages[i] -> tokentype == LABEL) &&
	      (str_eq (messages[i] -> name, "else"))) {
	    c -> keyword2_ptr = i;
	    continue;
	  }
 	  c -> else_start_ptr = i;
	  if_state = if_else_start;
	  if (messages[i] -> tokentype == OPENBLOCK)
	    ++n_blocks;
	  /*
	   *  If there are no braces, the end of the block
	   *  is in the same frame.  Look through this
	   *  frame again on the next iteration.
	   */
 	  if (c -> braces == FALSE)
 	    ++frame_offset;
	}
	break;
      case if_else_start:

	/*
	 *  If we have construct like 
	 *  if (...) {
	 *  } else if (   ) {
         *  }
	 *  Then move the else start_ptr ahead to the opening brace,
	 *  and adjust the frames, too.
	 */
	if ((M_TOK(messages[c -> else_start_ptr]) == LABEL) &&
	    str_eq (M_NAME(messages[c -> else_start_ptr]), "if")) {
	  int _open_paren_ptr2, _close_paren_ptr2;
	  _open_paren_ptr2 = nextlangmsgstack (messages, c -> else_start_ptr);
	  _close_paren_ptr2 = match_paren (messages, _open_paren_ptr2,
					   get_stack_top (messages));
	  c -> else_start_ptr = nextlangmsgstack (messages, _close_paren_ptr2);
	  if (M_TOK(messages[c -> else_start_ptr]) == OPENBLOCK) {
	    /* Also checks for an else w/o braces independently of
	       the if clause. */
	    while (TRUE) {
	      if ((c -> else_start_ptr <= this_frame -> message_frame_top) &&
		  (c -> else_start_ptr > next_frame -> message_frame_top))
		break;
	      ++frame_offset;
	      this_frame = frame_at (CURRENT_PARSER -> frame + 
				     frame_offset);
	      if ((next_frame = frame_at (CURRENT_PARSER -> frame + 
					  frame_offset - 1)) == NULL) {
		error (messages[ptr], "if_stmt (): parser error.\n");
	      }
	    }
	  }

	}

	/*
	 *  n_blocks should be 1 here if the control structure
	 *  has blocks.
	 */
	for (i = this_frame -> message_frame_top; 
	     (i > next_frame -> message_frame_top) && 
	       (if_state == if_else_start);
	     i--) {
	  if (c -> braces == FALSE) {
	    if (messages[i] -> tokentype == SEMICOLON) {
	      if_state = if_else_end;
	      c -> else_end_ptr = i;
	    }
	  } else {
	    if (messages[i] -> tokentype == OPENBLOCK)
	      ++n_blocks;
	    if (messages[i] -> tokentype == CLOSEBLOCK) {
	      --n_blocks;
	      /*
	       *  Going all the way to the end of nested 
	       *  blocks works okay here.
	       */
 	      if (n_blocks == 0) {
 		if_state = if_else_end;
 		c -> else_end_ptr = i;
 	      }
	    }
	  }
	}
	break;
      case if_else_end:
	if (c -> else_end_ptr != 0)
	  return SUCCESS;
	break;
      case if_null:
	break;
      }
  }

  return SUCCESS;
}

#define LAST_FRAME ((CURRENT_PARSER->frame + frame_offset) == \
  get_frame_pointer())

int while_stmt (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  int i, j, k,
    stack_end;
  int frame_offset,
    start_frame,
    last_frame,
    n_parens;
  enum {
    while_null,
    while_keyword,
    while_pred_start,
    while_pred_end,
    while_blk_start,
    while_blk_end,
  } while_state;

  /*
   *  Make sure the keyword isn't part of a do... while loop.
   */
  if (ctrlblk_ptr < MAXARGS)
    if (C_CTRL_BLK -> stmt_type == stmt_do)
      return SUCCESS;

  loop_linemarker (messages, ptr);

  frame_offset = 0;
  while_state = while_null;
  last_frame = get_frame_pointer ();
  stack_end = get_stack_top (messages);

  start_frame = CURRENT_PARSER -> frame;

  if ((c = new_ctrl_blk ()) == NULL)
    error (messages[ptr], "if_stmt: %s.", strerror (errno));

  ctrlblk_push (c);

  c -> stmt_type = stmt_while;
  c -> id = loop_ext++;
  c -> level = CURRENT_PARSER -> level;
  c -> keyword_ptr = ptr;

  while_state = while_keyword;

  for ( ; (start_frame + frame_offset >= last_frame) &&
	  (while_state != while_blk_end); 
	--frame_offset) {

    switch (while_state)
      {
      case while_keyword:
	for (i = c -> keyword_ptr - 1, n_parens = 0; 
	     (i > stack_end) && (while_state == while_keyword);
	     i--) {
	  if (messages[i] -> tokentype == OPENPAREN) {
	    if (!n_parens) { 
	      i = leading_unary_op (messages, i);
	      c -> pred_start_ptr = i;
	    }
	    /*
	     *  Scan from the keyword again in case there's a leading
	     *  unary op and the actual predicate starts further along.
	     */
	    for (j = c -> keyword_ptr - 1; 
		 (j > stack_end) && M_ISSPACE(messages[j]);
		 j--) 
	      ;
	    for (; (j > stack_end) && (while_state == while_keyword); j--) {
	      if (messages[j] -> tokentype == OPENPAREN) {
		++n_parens;
		/* This condition is here if we run into a
                   strange construct like an attribute. */
		if (!n_parens) {
		  j = leading_unary_op (messages, j);
		  c -> pred_start_ptr = j;
		  if ((c -> pred_end_ptr = 
		       match_paren (messages, j, stack_end)) == ERROR) {
		    warning (messages[j], "Mismatched parentheses.");
		    delete_control_block (ctrlblk_pop ());
		    return ERROR;
		  }
		}
	      }
	      if (messages[j] -> tokentype == CLOSEPAREN) {
		--n_parens;
		if (n_parens == 0) {
		  c -> pred_end_ptr = j;
		  while_state = while_pred_end;
		  /*
		   *  If the next message is an open block, then
		   *  the control block has braces.  If 
		   *  We get to the end of the frame, then 
		   *  look through the next frames for 
		   *  a label.
		   */
		  for (k = c -> pred_end_ptr - 1; 
		       (k > stack_end) && (while_state == while_pred_end); 
		       k--) {
		    if (!M_ISSPACE(messages[k])) {
		      c -> blk_start_ptr = k;
		      while_state = while_blk_start;
		      if (M_TOK(messages[k]) == OPENBLOCK) {
			c -> braces = TRUE;
		      } else {
			c -> braces = FALSE;
			if (M_TOK(messages[k]) == SEMICOLON) {
			  c -> blk_end_ptr = k;
			  while_state = while_blk_end;
			} else {
			  /*
			   *  If the loop block has no braces, there
			   *  should not be a new frame, so stay
			   *  in the current frame when the loop block
			   *  gets parsed in the next iteration.
			   */
			  ++frame_offset;
			}
			/*
			 *  I.e.; Empty loop at the end of a function.
			 */
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}	
	break;
      case while_pred_start:
      case while_pred_end:
	/*
	 *  The predicate start and end, and the block start,
	 *  all occur within the same frame, if the while
	 *  loop has braces, so they are all handled in the
	 *  case above.
	 */
	break;
      case while_blk_start:
	for (k = c -> pred_end_ptr - 1;
	     (k > stack_end) && 
	       (while_state == while_blk_start);
	     k--) {
	  if (c -> braces == FALSE) {
	    if (LAST_FRAME) {
	      /*
	       *  I.e.; Empty loop at the end of a function.
	       */
	      c -> blk_end_ptr = c -> blk_start_ptr;
	      while_state = while_blk_end;
	    } else {
	      if (messages[k] -> tokentype == SEMICOLON) {
		while_state = while_blk_end;
		c -> blk_end_ptr = k;
	      }
	    }
	  } else {
	    if (M_TOK(messages[k]) == OPENBLOCK) {
	      c -> blk_end_ptr = find_closing_brace_on_block
		(messages, k);
	      while_state = while_blk_end;
	    }
	  }
	}
	break;
      case while_blk_end:
      case while_null:
	break;
      }
  }

  return SUCCESS;
}

/*
 *   This function needs to check both forward from a 
 *   semicolon that ends the statement that is the do
 *   block, and also backward from the "while" keyword,
 *   because the parser can set a frame after the semicolon
 *   semicolon, so the to check a the function might begin from the 
 *   start of the next frame - that is, the "while" keyword.
 */
static int do_unframed_stmt_blk_end (CTRLBLK *c, 
				     MESSAGE_STACK messages, int idx) {
  int next_idx, prev_idx, stack_top, stack_start_idx;
  MESSAGE *m;
  m = messages[idx];
  if (M_TOK(m) == SEMICOLON) {
    stack_top = get_stack_top (messages);
    for (next_idx = idx - 1; 
	 next_idx > stack_top && M_ISSPACE(messages[next_idx]); 
	 next_idx--)
      ;
    if (str_eq (M_NAME(messages[next_idx]), "while")) {
      c -> blk_end_ptr = idx;
      return idx;
    } else {
      return FALSE;
    }
  } else {
    if ((M_TOK(m) == LABEL) && str_eq (M_NAME(m), "while")) {
      stack_start_idx = stack_start (messages);
      for (prev_idx = idx + 1; 
	   (prev_idx <= stack_start_idx) && M_ISSPACE(messages[prev_idx]); 
	   prev_idx++)
	;
      if (M_TOK(messages[prev_idx]) == SEMICOLON) {
	c -> blk_end_ptr = prev_idx;
	return prev_idx;
      } else {
	return FALSE;
      }
    } else {
      return FALSE;
    }
  }
  return FALSE;
}

int do_stmt (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  int i, j;
  int frame_offset,
    start_frame,
    last_frame,
    n_parens,
    n_blocks = 0;         /* Avoid a warning. */
  FRAME *this_frame, 
    *next_frame;
  enum {
    do_null,
    do_keyword,
    do_keyword2,
    do_pred_start,
    do_pred_end,
    do_blk_start,
    do_blk_end,
  } do_state;

  /*
   *  Make sure the keyword isn't part of a do... while loop.
   */
  if (ctrlblk_ptr < MAXARGS)
    if (C_CTRL_BLK -> stmt_type == stmt_do)
      return SUCCESS;

  loop_linemarker (messages, ptr);

  frame_offset = 0;
  do_state = do_null;
  last_frame = get_frame_pointer ();

  this_frame = frame_at (CURRENT_PARSER -> frame + frame_offset);
  start_frame = CURRENT_PARSER -> frame;

  if ((c = new_ctrl_blk ()) == NULL)
    error (messages[ptr], "if_stmt: %s.", strerror (errno));

  ctrlblk_push (c);

  c -> stmt_type = stmt_do;
  c -> id = loop_ext++;
  c -> level = CURRENT_PARSER -> level;
  c -> keyword_ptr = ptr;

  do_state = do_keyword;

  for ( ; start_frame + frame_offset >= last_frame; --frame_offset) {

    this_frame = frame_at (CURRENT_PARSER -> frame + frame_offset);
    next_frame = frame_at (CURRENT_PARSER -> frame + frame_offset - 1);

    switch (do_state)
      {
      case do_keyword:
	for (i = c -> keyword_ptr, n_parens = 0, n_blocks = 0; 
	     (i > next_frame -> message_frame_top) && 
	       (do_state == do_keyword);
	     i--) {
	  if (messages[i] -> tokentype == OPENBLOCK) {
	    c -> braces = TRUE;
	    c -> blk_start_ptr = i;
	    do_state = do_blk_start;
	    n_blocks = 1;
	  }
	  if (messages[i] -> tokentype == LABEL) {
	    if (strcmp (messages[i] -> name, "do")) {
	      c -> braces = FALSE;
	      c -> blk_start_ptr = i;
	      do_state = do_blk_start;
	    }
	  }
	}
	break;
      case do_keyword2:
	for (i = this_frame -> message_frame_top, n_parens = 0, n_blocks = 0; 
	     (i > next_frame -> message_frame_top) && 
	       (do_state == do_keyword2);
	     i--) {
	  if (messages[i] -> tokentype == OPENPAREN) {
	    c -> pred_start_ptr = i;
	    for (j = this_frame -> message_frame_top; 
		 j > next_frame -> message_frame_top;
		 j--) {
	      if (messages[j] -> tokentype == OPENPAREN) {
		++n_parens;
		j = leading_unary_op (messages, j);
		if (n_parens == 1)
		  c -> pred_start_ptr = j;
	      }
	      if (messages[j] -> tokentype == CLOSEPAREN) {
		--n_parens;
		if (n_parens == 0) {
		  c -> pred_end_ptr = j;
		  do_state = do_pred_end;
		}
	      }
	    }
	    i = c -> pred_end_ptr;
	  }
	}	
	break;
      case do_pred_start:
      case do_pred_end:
	/*
	 *  The predicate start and end, and the block start,
	 *  all occur within the same frame, if the while
	 *  loop has braces, so they are all handled in the
	 *  case above.
	 */
	break;
      case do_blk_start:
	for (i = this_frame -> message_frame_top; 
	     (i > next_frame -> message_frame_top) && 
	       (do_state == do_blk_start);
	     i--) {
	  if (c -> braces == FALSE) {
	    /*
	     *  Fills in c -> blk_end_ptr.
	     */
	    if (do_unframed_stmt_blk_end (c, messages, i)) {
	      do_state = do_blk_end;
	      /*
	       *  Back up a frame in case we're in the 
	       *  predicate frame.
	       */
	      ++frame_offset;
	    }
	  } else {
	    if (messages[i] -> tokentype == OPENBLOCK)
	      ++n_blocks;
	    if (messages[i] -> tokentype == CLOSEBLOCK) {
	      --n_blocks;
	      if (n_blocks == 0) {
		do_state = do_blk_end;
		c -> blk_end_ptr = i;
	      }
	    }
	  }
	}
	break;
      case do_blk_end:
	for (i = this_frame -> message_frame_top; 
	     (i > next_frame -> message_frame_top) && 
	       (do_state == do_blk_end);
	     i--) {
	  if ((messages[i] -> tokentype == LABEL) &&
	      (str_eq (messages[i] -> name, "while"))) {
	    do_state = do_keyword2;
	    c -> keyword2_ptr = i;
	    /* The predicate should occur in the same frame
	     * as the, "while," keyword, so don't increment
	     * the frame offset in the next iteration.
	     */
	    ++frame_offset;
	  }
	}
	break;
      case do_null:
	break;
      }
  }

  return SUCCESS;
}

/*
 *  All that's needed for a switch statement is to insert a "value"
 *  method call in the argument to the switch keyword.  We should
 *  not need to worry about the block start and end.
 */

int switch_stmt (MESSAGE_STACK messages, int ptr) {

  CTRLBLK *c;
  int i, stack_end, j, n_blocks;

  loop_linemarker (messages, ptr);

  stack_end = get_stack_top (messages);

  if ((c = new_ctrl_blk ()) == NULL)
    error (messages[ptr], "if_stmt: %s.", strerror (errno));

  ctrlblk_push (c);

  c -> stmt_type = stmt_switch;
  c -> id = loop_ext++;
  c -> keyword_ptr = ptr;
  c -> level = CURRENT_PARSER -> level;

  if ((i = nextlangmsg (messages, ptr)) == ERROR) {
    warning (messages[ptr], "Switch statement syntax error.");
    return ERROR;
  }

  if (messages[i] -> tokentype != OPENPAREN) {
    warning (messages[ptr], "Switch statement syntax error.");
    return ERROR;
  } else {
    c -> pred_start_ptr = i;
  }
  
  if ((c -> pred_end_ptr = match_paren (messages, c -> pred_start_ptr, 
					stack_end)) == ERROR) {
    warning (messages[ptr], "Switch statement syntax error.");
    return ERROR;
  }

  if ((i = nextlangmsg (messages, c -> pred_end_ptr)) == ERROR) {
    warning (messages[ptr], "Switch statement syntax error.");
    return ERROR;
  }
  if (M_TOK(messages[i]) != OPENBLOCK) {
    warning (messages[ptr], "Switch statement syntax error.");
    return ERROR;
  }
  c -> blk_start_ptr = i;
  n_blocks = 0;
  for (j = c -> blk_start_ptr; j > stack_end; j--) {
    if (M_TOK(messages[j]) == OPENBLOCK)
      ++n_blocks;
    if (M_TOK(messages[j]) == CLOSEBLOCK)
      --n_blocks;
    if (n_blocks <= 0)
      break;
  }
  c -> blk_end_ptr = j;
  /*
   *  Set blk_end_ptr so that ctrlblk_state deletes the ctrlblk
   *  structure after the predicate when it is no longer needed.
   */

  return SUCCESS;
}

/*
 *  Exit with an error if we can't find a goto target
 *  label within the current argument block.
 */
static void goto_in_argblk_check (MESSAGE_STACK messages,
				  int keyword_idx) {
  int i, label_msg_idx;
  ARGBLK *a = argblks[argblk_ptr+1];
  MESSAGE *label_msg;
  char errbuf[MAXMSG], fn_name[MAXMSG], method_name[MAXMSG];

  label_msg_idx = nextlangmsg (messages, keyword_idx);

  label_msg = messages[label_msg_idx];

  /* check back to start of argblk first. */
  for (i = keyword_idx; i <= a -> start_idx; i++) {
    if (str_eq (M_NAME(messages[i]), M_NAME(label_msg))) {
      return;
    }
  }
  
  for (i = label_msg_idx - 1; i >= a -> end_idx; i--) {
    if (str_eq (M_NAME(messages[i]), M_NAME(label_msg))) {
      return;
    }
  }

  strcatx (errbuf, M_NAME(messages[keyword_idx]), " ",
	   M_NAME(messages[label_msg_idx]), NULL);

  if (interpreter_pass == parsing_pass) {
    strcatx (fn_name, "function '", get_fn_name (), ":'", NULL);
    error (messages[keyword_idx], "error: in %s\n\n\t%s\n\n"
	   "The goto target is not within the current code block.\n",
	   fn_name, errbuf);
  } else { /* method_pass */
    strcatx (method_name, "method '",
	     new_methods[new_method_ptr+1] -> method -> name, ":'",
	     NULL);
    error (messages[keyword_idx], "error: in %s\n\n\t%s\n\n"
	   "The goto target is not within the current code block.\n",
	   method_name, errbuf);
  }

}

/*
 *  Goto statements, like the labels they refer to just
 *  get evaled and output.  We also check for arg block
 *  scope and issue a warning.
 */
int goto_stmt (MESSAGE_STACK messages, int keyword_idx) {

  int label_idx;
  int semicolon_idx;
  int i;
  char errbuf[MAXMSG];
  
  if ((label_idx = nextlangmsg (messages, keyword_idx)) != ERROR) {
    if (M_TOK(messages[label_idx]) != LABEL) {
      return ERROR;
    }
    if ((semicolon_idx = nextlangmsg (messages, label_idx)) != ERROR) {
      if (M_TOK(messages[semicolon_idx]) != SEMICOLON) {
	return ERROR;
      }

      toks2str (messages, keyword_idx, semicolon_idx, errbuf);

      if (argblk) {
	goto_in_argblk_check (messages, keyword_idx);
      }

      /* Only mark as evaled here - file out with the rest of the
	 frame from parse (). */
      for (i = keyword_idx; i >= semicolon_idx; --i) {
	++messages[i] -> evaled;
      }

    }
  }

  return SUCCESS;
}

/*
 *  Called by method_args ().  
 */

int ctrlblk_push_args (OBJECT *rcvr, METHOD *method, OBJECT *arg_object) {

  CTRLBLK *c;
  LIST *l;

  c = C_CTRL_BLK;
  switch (c -> stmt_type)
    {
    case stmt_if:
    case stmt_while:
    case stmt_do:
      /*
       *  If, while, and do statements get evaled at run time.
       */
      break;
    case stmt_for:
      if (ctrlblk_pred) {
	if (for_init) {
	  l = new_list ();
	  l -> data = strdup (fmt_store_arg_call (rcvr, method, arg_object));
	  if (!c -> for_init_args) {
	    c -> for_init_args = l;
	  } else {
	    list_push (&(c -> for_init_args), &l);
	  }
	}
	if (for_inc) {
	  l = new_list ();
	  l -> data = strdup (fmt_store_arg_call (rcvr, method, arg_object));
	  if (!c -> for_inc_args) {
	    c -> for_inc_args = l;
	  } else {
	    list_push (&(c -> for_inc_args), &l);
	  }
	}
      }
      break;
    case stmt_break:
    case stmt_case:
    case stmt_else:
    case stmt_goto:
    case stmt_switch:
    case stmt_default:
      break;
    }
  return SUCCESS;
}

/*
 *  Called by register_c_var () for methods within for
 *  loop predicates.
 *
 *  Stmt_switch, stmt_case, and stmt_if C variables only 
 *  need to be registered before the predicate.
 *
 *  Stmt_while and stmt_do C variables need to be registered
 *  before the predicate, and stored if c -> pred_args_d is
 *  True, so they can be included in a block before each
 *  iteration.
 *
 *  Stmt_for variables are always stored for the init block,
 *  the term block, or the iteration block.
 */

int ctrlblk_register_c_var (char *func_call, int output_idx) {
  CTRLBLK *c;
  LIST *l;

  c = C_CTRL_BLK;

  switch (c -> stmt_type)
    {
    case stmt_switch:
    case stmt_case:
    case stmt_if:
      fileout (func_call, 0, output_idx);
      break;
    case stmt_while:
    case stmt_do:
      l = new_list ();
      l -> data = strdup (func_call);
      if (!c -> pred_args) {
	c -> pred_args = l;
      } else {
	list_push (&(c -> pred_args), &l);
      }
      break;
    case stmt_for:
      if (ctrlblk_pred) {
 	l = new_list ();
	l -> data = strdup (func_call);

	if (for_init) {
 	  if (!c -> for_init_args) {
 	    c -> for_init_args = l;
 	  } else {
 	    list_push (&(c -> for_init_args), &l);
 	  }
	}

	if (for_inc) {
 	  if (!c -> for_inc_args) {
	    c -> for_inc_args = l;
 	  } else {
 	    list_push (&(c -> for_inc_args), &l);
 	  }
	}
      }
      break;
    case stmt_break:
    case stmt_else:
    case stmt_goto:
    case stmt_default:
      break;
    }
  return SUCCESS;
}

/*
 *  Called by method_call ().  Methods within predicates are
 *  so far within C contexts, so they need obj_2_c_wrapper ()
 *  calls.
 *
 *  Register_c_var_method () is used by register_c_var in method.c
 *  if necessary.
 */

extern METHOD *register_c_var_method;

static char *format_ctrlblk_method_call (MESSAGE *m_orig,
					OBJECT *rcvr,
					METHOD *method) {
  char buf_out[MAXMSG];
  return obj_2_c_wrapper (m_orig, rcvr, method,
			  fmt_method_call (rcvr, method -> selector,
					   method -> name, buf_out), TRUE);
}

int ctrlblk_method_call (OBJECT *rcvr, METHOD *method, int method_msg_ptr) {

  CTRLBLK *c;
  LIST *l;
  MESSAGE *m_orig;

  c = C_CTRL_BLK;
  m_orig = message_stack_at (method_msg_ptr);
  switch (c -> stmt_type)
    {
    case stmt_if:
    case stmt_while:
    case stmt_do:
    case stmt_switch:
      /*
       *  The predicate gets evaled at run time.
       */
      register_c_var_method = method;
      (void)ctrlblk_pred_rt_expr (message_stack (), method_msg_ptr);
      register_c_var_method = NULL;
      break;
    case stmt_case:
      break;
    case stmt_for:
      if (ctrlblk_pred) {
	if (for_init) {
	  l = new_list ();
	  l -> data = strdup (format_ctrlblk_method_call 
			      (m_orig, rcvr, method));
 	  if (!c -> for_init_methods) {
 	    c -> for_init_methods = l;
 	  } else {
 	    list_push (&(c -> for_init_methods), &l);
 	  }
	}
	if (for_term) {
	  c ->  for_term_methods = true;
	}
	if (for_inc) {
	  l = new_list ();
	  l -> data = strdup (format_ctrlblk_method_call 
			      (m_orig, rcvr, method));
 	  if (!c -> for_inc_methods) {
	    c -> for_inc_methods = l;
 	  } else {
 	    list_push (&(c -> for_inc_methods), &l);
 	  }
	}
      }
      break;
    case stmt_break:
    case stmt_else:
    case stmt_goto:
    case stmt_default:
      break;
    }
  return SUCCESS;
}

int ctrlblk_cleanup_args (METHOD *method, int ptr) {

  CTRLBLK *c;
  LIST *l;
  int i;

  c = C_CTRL_BLK;
  switch (c -> stmt_type)
    {
    case stmt_if:
    case stmt_while:
    case stmt_do:
      /*
       * If, while, and do statements get evaled at run time.
       */
      break;
    case stmt_for:
      if (ctrlblk_pred) {
	for (i = method -> n_args; i; i--) {
	  l = new_list ();
	  l -> data = strdup ("__ctalk_arg_cleanup((void *)0);\n");
	  if (method -> args[i] && method -> args[i] -> obj) {
	    delete_object (method -> args[i] -> obj);
	    delete_arg (method -> args[i]);
	    method -> args[i] = NULL;
	  }
	  if (for_init) {
	    if (!c -> for_init_cleanup) {
	      c -> for_init_cleanup = l;
	    } else {
	      list_push (&(c -> for_init_cleanup), &l);
	    }
	  }
	  if (for_inc) {
	    if (!c -> for_inc_cleanup) {
	      c -> for_inc_cleanup = l;
	    } else {
	      list_push (&(c -> for_inc_cleanup), &l);
	    }
	  }
	}
      }
      break;
    case stmt_break:
    case stmt_case:
    case stmt_else:
    case stmt_goto:
    case stmt_switch:
    case stmt_default:
      break;
    }

  /*
   *  Then clean up the method arguments.
   * 
   *  This is the same as cleanup_args () in method.c, but we've only
   *  queued the __ctalk_arg_cleanup () calls.
   */

  for (i = method -> n_args - 1; i >= 0; i--) {
    if (method -> args[i] -> obj -> scope == ARG_VAR)
      (void) delete_arg_object (method -> args[i] -> obj);
    delete_arg (method -> args[i]);
    method -> args[i] = NULL;
    method -> n_args = i;
  }
  return SUCCESS;
}

void loop_linemarker (MESSAGE_STACK messages, int ptr) {

  char buf[MAXMSG];

  if (nolinemarker_opt)
    return;

  if (interpreter_pass == parsing_pass) {
    if (!fn_defined_by_header) {
      fmt_loop_line_info (buf, error_line, TRUE);
      (void)fileout (buf, FALSE, ptr);
    }
  }

}

char *handle_rt_prefix_rexpr (MESSAGE_STACK messages, int prefix_idx,
			      int pred_start_idx, int pred_end_idx,
			      char *expr_buf) {
  char expr_buf_tmp[MAXMSG];
  char prefix_buf[MAXMSG], prefix_buf_2[MAXMSG];
  int i;

  *prefix_buf = '\0';
  for (i = prefix_idx; i >= pred_end_idx; i--) {
    strcatx2 (prefix_buf, M_NAME(messages[i]), NULL);
  }
  strcatx (prefix_buf_2, PTR_TRANS_FN_U, " (", 
	   fmt_eval_expr_str (prefix_buf, expr_buf_tmp), ")", NULL);

  *expr_buf = '\0';
  for (i = pred_start_idx; i > prefix_idx; i--) {
    strcatx2 (expr_buf, M_NAME(messages[i]), NULL);
  }
  strcatx2 (expr_buf, prefix_buf_2, NULL);
  return expr_buf;
}

bool major_logical_op_check (MESSAGE_STACK messages, int expr_start,
			     int expr_end) {
  int i;
  int prev_tok, next_tok;

  for (i = expr_start; i >= expr_end; i--) {

    switch (M_TOK(messages[i]))
      {
      case NEWLINE:
      case WHITESPACE:
	continue;
	break;
      case BOOLEAN_AND:
      case BOOLEAN_OR:
	prev_tok = prevlangmsg (messages, i);
	next_tok = nextlangmsg (messages, i);
	if ((M_TOK(messages[prev_tok]) != CLOSEPAREN) &&
	    (M_TOK(messages[next_tok]) != OPENPAREN)) {
	  return true;
	}
	break;
      case METHODMSGLABEL:
	if (str_eq (M_NAME(messages[i]), "&&") || 
	    str_eq (M_NAME(messages[i]), "||")) {
	  prev_tok = prevlangmsg (messages, i);
	  next_tok = nextlangmsg (messages, i);
	  if ((M_TOK(messages[prev_tok]) != CLOSEPAREN) &&
	      (M_TOK(messages[next_tok]) != OPENPAREN)) {
	    return true;
	  }
	}
	break;
      }
  }
  return false;
}

STMT_CLASS get_ctrl_class (char *keyword) {
  if (str_eq (keyword, "break"))
    return stmt_break;
  if (str_eq (keyword, "case"))
    return stmt_case;
  if (str_eq (keyword, "default"))
    return stmt_default;
  if (str_eq (keyword, "do"))
    return stmt_do;
  if (str_eq (keyword, "else"))
    return stmt_else;
  if (str_eq (keyword, "for"))
    return stmt_for;
  if (str_eq (keyword, "goto"))
    return stmt_goto;
  if (str_eq (keyword, "if"))
    return stmt_if;
  if (str_eq (keyword, "switch"))
    return stmt_switch;
  if (str_eq (keyword, "while"))
    return stmt_while;
  return ERROR;
}

/*
 *  Returns the index of the first character of a predicate
 *  if it is a leading unary op, the original index of the
 *  predicate start - usually the opening parenthesis - 
 *  otherwise.
 *
 *  __ctalkEvalExpr () can also handle leading unary ops,
 *  though there are not yet handlers for pre-increment and
 *  pre-decrement operators, because it only handles constant
 *  expressions.
 *
 *  NOTE: These are only the leading unary ops that can appear
 *  before the API expression
 *
 *    <op> __ctalkToCInteger (... )
 *
 *  that the compiler produces for control structures, so this 
 *  fn should *only* be called by the control statement functions 
 *  above.
 */

int leading_unary_op (MESSAGE_STACK messages, int idx) {

  int i, stack_end, tok, n_parens;

  stack_end = get_stack_top (messages);

  for (i = idx, n_parens = 0; i > stack_end; i--) {

    if (M_ISSPACE (messages[i]))
      continue;

    tok = M_TOK (messages[i]);

    if ((tok == AMPERSAND) ||
	(tok == EXCLAM) ||
	(tok == PLUS) ||
	(tok == MINUS) ||
	(tok == BIT_COMP)) {
      return i;
    } else {
      if (tok == OPENPAREN && !n_parens) {
	++n_parens;
	continue;
      } else {
	return idx;
      }
    }
  }

  return idx;
}

int expr_paren_check (MESSAGE_STACK messages, int msg_ptr) {

  int i, n_parens;

  switch (C_CTRL_BLK -> stmt_type)
    {
    case stmt_if:
      for (i = C_CTRL_BLK -> keyword_ptr, n_parens = 0; 
	   i >= C_CTRL_BLK -> blk_start_ptr; i--) {
	if (M_TOK(messages[i]) == OPENPAREN) ++n_parens;
	if (M_TOK(messages[i]) == CLOSEPAREN) --n_parens;
      }
      if (!n_parens) 
	return SUCCESS;
      else 
	return ERROR;
      break;
    default:
      break;
    }
  return SUCCESS;
}

bool is_if_pred_start (MESSAGE_STACK messages, int idx) {
  int prev_tok_idx;
  if (ctrlblk_pred) {
    if (C_CTRL_BLK -> stmt_type == stmt_if) {
      if ((prev_tok_idx = prevlangmsg (messages, idx)) != ERROR) {
	if (prev_tok_idx == C_CTRL_BLK -> pred_start_ptr) {
	  return TRUE;
	} else {
	  /* 
	     A series like
	        if (+ <expr>
	  */
	  if (M_TOK(messages[prev_tok_idx]) == OPENPAREN) {
	    int i_1;
	    for (i_1 = prev_tok_idx; i_1 <= C_CTRL_BLK -> pred_start_ptr;
		 ++i_1) {
	      if (M_TOK(messages[i_1]) != OPENPAREN)
		return FALSE;
	    }
	    return TRUE;
	  }
	}
      }
    }
  }
  return FALSE;
}

bool is_if_subexpr_start (MESSAGE_STACK messages, int idx) {
  int prev_tok_idx;
  if (ctrlblk_pred) {
    if (C_CTRL_BLK -> stmt_type == stmt_if) {
      if ((prev_tok_idx = prevlangmsg (messages, idx)) != ERROR) {
	if (prev_tok_idx == C_CTRL_BLK -> pred_start_ptr) {
	  return TRUE;
	} else {
	  /* 
	     A series like
	        if (... || | && (+ <expr>
	  */
	  if (M_TOK(messages[prev_tok_idx]) == OPENPAREN) {
	    int i_1;
	    for (i_1 = prev_tok_idx; i_1 <= C_CTRL_BLK -> pred_start_ptr;
		 ++i_1) {
	      if (M_ISSPACE(messages[i_1]))
		continue;
	      if ((M_TOK(messages[i_1]) == BOOLEAN_AND) ||
		  (M_TOK(messages[i_1]) == BOOLEAN_OR))
		return TRUE;
	      if (M_TOK(messages[i_1]) != OPENPAREN)
		return FALSE;
	    }
	    return TRUE;
	  }
	}
      }
    }
  }
  return FALSE;
}

bool is_while_pred_start (MESSAGE_STACK messages, int idx) {
  int prev_tok_idx;
  if (ctrlblk_pred) {
    if (C_CTRL_BLK -> stmt_type == stmt_while) {
      if ((prev_tok_idx = prevlangmsg (messages, idx)) != ERROR) {
	if (prev_tok_idx == C_CTRL_BLK -> pred_start_ptr)
	  return TRUE;
      } else {
	/* 
	   Also here, check for a series like
	   if (+ <expr>
	*/
	if (M_TOK(messages[prev_tok_idx]) == OPENPAREN) {
	  int i_1;
	  for (i_1 = prev_tok_idx; i_1 <= C_CTRL_BLK -> pred_start_ptr;
	       ++i_1) {
	    if (M_TOK(messages[i_1]) != OPENPAREN)
	      return FALSE;
	  }
	  return TRUE;
	}
      }
    }
  }
  return FALSE;
}

bool is_while_pred_start_2 (MESSAGE_STACK messages, int idx) {
  if (ctrlblk_pred) {
    if (C_CTRL_BLK -> stmt_type == stmt_while) {
      if (idx == C_CTRL_BLK -> pred_start_ptr)
	return TRUE;
    } else {
      /* 
	 And here, check for a series like
	 if (+ <expr>
      */
      if (M_TOK(messages[idx]) == OPENPAREN) {
	int i_1;
	for (i_1 = idx; i_1 <= C_CTRL_BLK -> pred_start_ptr;
	     ++i_1) {
	  if (M_TOK(messages[i_1]) != OPENPAREN)
	    return FALSE;
	}
	return TRUE;
      }
    }
  }
  return FALSE;
}

#define BLK_BUF_MAX 0x8888

#define FOR_INIT_BLOCK "\n\
{\n\
  %s\n\
  %s\n\
  %s\n\
}\n"

#define FOR_INC_BLOCK FOR_INIT_BLOCK

int handle_blk_continue_stmt (MESSAGE_STACK messages, int idx) {
  int c_ptr;
  CTRLBLK *c;
  char method_buf[MAXMSG];
  char blk_buf[BLK_BUF_MAX];
  char arg_pop_buf[MAXMSG];
  char arg_push_buf[MAXMSG];
  LIST *l;
  int i, next_idx;
  MESSAGE *m;
  ARGBLK *a = NULL;

  for (c_ptr = ctrlblk_ptr + 1; c_ptr <= MAXARGS; ++c_ptr) {
    c = ctrlblks[c_ptr];
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
        snprintf (blk_buf, BLK_BUF_MAX, FOR_INC_BLOCK, arg_push_buf, 
                 method_buf, arg_pop_buf);
        fileout (blk_buf, FALSE, 
		 ((c -> braces) ? idx + 1: idx - 1));
	
	fileout (((c -> braces ) ? "\n\n" : "\n}\n"), FALSE, idx - 1);
      } else {
	/*
	 *  Output a closing brace if necessary even if there are
	 *  no predicates.
	 */
        if (c -> braces == FALSE) {
	  fileout ("\n}\n\n", FALSE, idx - 1);
        }
      }
    } else if (c -> stmt_type == stmt_while) {
      /* Same kludge as below... eechh. */
      if ((a = current_argblk ()) == NULL)
	return SUCCESS;
      for (i = ctrlblk_ptr + 1; i <= MAXARGS; i++) {
	if ((c = ctrlblks[i]) == NULL)
	  return SUCCESS;
	/*
	 *  Find the innermost while, do, or for loop, or switch....
	 */
	if (c -> stmt_type == stmt_while || c -> stmt_type == stmt_for ||
	    c -> stmt_type == stmt_do || c -> stmt_type == stmt_switch)
	  break;
      }
      /*
       *  ... then check we haven't reached the end of the stack before
       *  finding a loop.
       */
      if (c -> stmt_type != stmt_while && c -> stmt_type != stmt_for &&
	  c -> stmt_type != stmt_do && c -> stmt_type != stmt_switch)
	return SUCCESS;
      /*
       *  Make sure that the loop keyword is within the argument block
       *  body on the same parser level.
       */
      if (a -> parser_level != c -> level)
	return SUCCESS;
      if (c -> keyword_ptr >= a -> start_idx)
	return SUCCESS;

      /* then do our mangling. */
      m = messages[idx];
      if ((next_idx = nextlangmsg (messages, idx)) != ERROR) {
	if (M_TOK(messages[next_idx]) == SEMICOLON) {
	  m -> name[0] = ' '; m -> name[1] = 0;
	  m -> tokentype = WHITESPACE;
	  strcpy (M_NAME(messages[next_idx]), "continue;");
	  M_TOK(messages[next_idx]) = LABEL;
	}
      }
    }
  }

  return SUCCESS;
}

/* return true if the break is at least two control structures down
   from the argblk AND the outer structure is not a switch statement. 
   (Note that the calling fn has already determined that the parser
   is on the same recursion level as the argument block.) Any
   control structure that is three nesting levels down from the
   argument block has local breaks, which are retained. */
static bool immediate_innermost_switch (void) {
  ARGBLK *a;
  if ((a = current_argblk ()) == NULL)
    return false;
  if (ctrlblk_ptr < 510) {
    return true;
  } else if (ctrlblk_ptr < 511) {
    if ((ctrlblks[ctrlblk_ptr + 2] -> stmt_type != stmt_switch) &&
	(ctrlblks[ctrlblk_ptr + 2] -> keyword_ptr < a -> start_idx)) {
      return true;
    }
  }
  return false;
}

static void insert_keep_break (MESSAGE *m, MESSAGE *m_next) {
  m -> name[0] = ' '; m -> name[1] = 0;
  m -> tokentype = WHITESPACE;
  strcpy (M_NAME(m_next), "break;");
  M_TOK(m_next) = LABEL;
}

/*
 *  This is seriously kludgy... but it works if called from resolve
 *  with both ctrlblk_blk and argblk set.  Handles a loop within an
 *  argblk.  TODO - Check for the converse, a break within an argblk
 *  within a loop.
 *  We've also added, "break;" to the list in is_c_keyword ().  Eeech.
 */
void handle_blk_break_stmt (MESSAGE_STACK messages, int keyword_ptr) {

  int next_idx, next_idx_2;
  MESSAGE *m = messages[keyword_ptr];
  ARGBLK *a;
  CTRLBLK *c = NULL;  /* Avoid a warning message. */
  int i;
  
  if ((a = current_argblk ()) == NULL)
    return;
  for (i = ctrlblk_ptr + 1; i <= MAXARGS; i++) {
    if ((c = ctrlblks[i]) == NULL)
      return;
    /*
     *  Find the innermost while, do, or for loop, or switch....
     */
    if (c -> stmt_type == stmt_while || c -> stmt_type == stmt_for ||
	c -> stmt_type == stmt_do || c -> stmt_type == stmt_switch)
      break;
  }
  
  /*
   *  ... then check we haven't reached the end of the stack before
   *  finding a loop.
   */
  if (c -> stmt_type != stmt_while && c -> stmt_type != stmt_for &&
      c -> stmt_type != stmt_do && c -> stmt_type != stmt_switch)
    return;

  /*
   *  Make sure that the loop keyword is within the argument block
   *  body on the same parser level.
   */
  if (a -> parser_level != c -> level)
    return;
  if (c -> keyword_ptr >= a -> start_idx)
    return;

  /* then do our mangling - if the control structure is a
     switch statment, then we have to retain a 
     "structural" break; that is - part of the control block. 
     We also have to retain breaks if the control structure
     is at least two levels down (three levels down if the
     outermost control structure within the argument block
     is a switch).

     I.e., don't retain the break if it's not part of the 
     switch structure: in other words, a break in that case
     still needs to break out of the argument block; e.g.,

     arglist map {

       switch (myint)
        {
	case 1:
	  break;             <--- this break gets retained.
        case 2:
          if (something) {
	    break;           <-- this break gets translated into
          }                      a break for the argument block.
        break;               <-- this break gets retained also.
        }

     }
  */
    if ((next_idx = nextlangmsg (messages, keyword_ptr)) != ERROR) {
      if ((next_idx_2 = nextlangmsgstack (messages, next_idx)) != ERROR) {
	if (M_TOK(messages[next_idx]) == SEMICOLON) {
	  if ((M_TOK(messages[next_idx_2]) == LABEL) &&
	      str_eq (M_NAME(messages[next_idx_2]), "case")) {
	    insert_keep_break (m, messages[next_idx]);
	  } else if ((M_TOK(messages[next_idx_2]) == LABEL) &&
		     str_eq (M_NAME(messages[next_idx_2]), "default")) {
	    insert_keep_break (m, messages[next_idx]);
	  } else if (immediate_innermost_switch ()) {
	    /* check for other control structures if the break
	       isn't enclosed by braces. */
	    insert_keep_break (m, messages[next_idx]);
	  } else if (M_TOK(messages[next_idx_2]) == CLOSEBLOCK) {
	    /* check for a switch statement. */
	    if (is_switch_closing_brace (messages, next_idx_2,
					 stack_start (messages))) {
	      insert_keep_break (m, messages[next_idx]);
	    } else {
	      /* if it's in a nested C control structure that
	       is not just inside a top-level switch within an
	       argument block. */
	      if (immediate_innermost_switch ()) {
		insert_keep_break (m, messages[next_idx]);
	      }
	    }
	  }
	}
      }
    }


}

int ctrlblk_pred_start_idx (void) {
  CTRLBLK *c;
  if ((c = C_CTRL_BLK) != NULL) {
    return c -> pred_start_ptr;
  }
  return ERROR;
}
int ctrlblk_pred_end_idx (void) {
  CTRLBLK *c;
  if ((c = C_CTRL_BLK) != NULL) {
    return c -> pred_end_ptr;
  }
  return ERROR;
}
