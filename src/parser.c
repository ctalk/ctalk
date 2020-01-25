/* $Id: parser.c,v 1.5 2020/01/25 23:05:15 rkiesling Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "ctrlblk.h"
#include "break.h"

extern char input_source_file[FILENAME_MAX]; /* Declared in main.c.       */
extern char *source_files[MAXARGS]; /* Declared in infiles.c */
extern int n_input_files, input_idx;
extern I_PASS interpreter_pass;
extern int main_declaration;      /* Determined in the first pass.        */
static int main_start_frame;
extern int error_line,            /* Declared in errorloc.c.              */
  error_column;
extern int library_input;         /* Declared in class.c.  TRUE if        */
                                  /* evaluating class libraries.          */
extern int show_progress_opt;     /* From main.c                          */


                                  /* The main parser message stack.       */
MESSAGE *messages[P_MESSAGES+1];
int messageptr;            /* Message stack pointer.               */
static int main_start = 0;        /* Stack index of main() declaration.   */

extern CTRLBLK *ctrlblks[MAXARGS + 1]; /* Declared in control.c.          */
extern int ctrlblk_ptr;
extern bool ctrlblk_pred,               /* Global states.                  */
  ctrlblk_blk,
  ctrlblk_else_blk;
int last_method_line;            /* Last line number of method sub-parser,
                                    used by new_*_method to reset line
                                    after parsing a method.  Should ONLY
				    be used by new*method on return from
                                    parsing because it does not account 
                                    for sub-parsers during method parsing
                                    passes. */

extern EXCEPTION parse_exception;  /* Declared in pexcept.c. */

extern int fn_defined_by_header;

extern FRAME *frames[MAXFRAMES+1]; /* Declared in frame.c. */
extern int frame_pointer;

extern int fn_has_argblk;         /* Declared in fnbuf.c */

#define _MPUTCHAR(__m,__c) (__m)->name[0] = (__c); (__m)->name[1] = '\0';

/*
 *   Functions to manage the message stack.
 */

int init_message_stack (void) {
  messageptr = P_MESSAGES;
  return SUCCESS;
}

int get_messageptr (void) {
  return messageptr;
}

void message_push (MESSAGE *m) {
#ifdef MINIMUM_MESSAGE_HANDLING
  messages[messageptr] = m;
  --messageptr;
#else
  if (messageptr == 0)
    _error ("Message_push: stack_overflow.");

  if (!m)
    _error ("Message_push: null pointer, messageptr = %d.", messageptr);

  messages[messageptr] = m;

#ifdef STACK_TRACE
  debug ("Message_push %d. %s.", messageptr, 
	 (messages[messageptr] && IS_MESSAGE (messages[messageptr])) ?
	  messages[messageptr] -> name : "(null)"); 
#endif

  --messageptr;

  /*  return messageptr + 1; */
#endif
}

MESSAGE *message_pop (void) {
#ifdef MINIMUM_MESSAGE_HANDLING
  MESSAGE *m;
  m = messages[++messageptr];
  messages[messageptr] = NULL;
  return m;
#else  
  
  MESSAGE *m;

  if (messageptr == P_MESSAGES) {
    return (MESSAGE *)NULL;
  }

  if (messageptr > P_MESSAGES)
    _error ("Message_pop: Message stack overflow, messageptr = %d.", 
		  messageptr);

  m = messages[++messageptr];
  messages[messageptr] = NULL;

#ifdef STACK_TRACE
  debug ("Message_pop %d. %s.", messageptr, m -> name);
#endif

  if (!m || !IS_MESSAGE (m))
    _error ("message_pop (): Bad message, stack index %d.", messageptr);
  else
    return m;
#endif
}

MESSAGE *message_stack_at (int n) {
  if (n > P_MESSAGES) return NULL;
  return messages[n];
}

MESSAGE **message_stack (void) {
  return messages;
}

int reset_messageptr (int new_ptr) {
  messageptr = new_ptr;
  return messageptr;
}

void clear_frame_messages  (int this_frame_ptr) {

  MESSAGE *m;

  while (frames[this_frame_ptr] -> message_frame_top > messageptr) {
    m = messages[++messageptr];
    messages[messageptr] = NULL;

    /* if (m && IS_MESSAGE (m)) { */
      reuse_message (m);
      /* } */
  }
}

void clear_all_messages (void) {

  MESSAGE *m = NULL;
  int i;

  while ((i = get_messageptr ()) <= MAXARGS) {
    m = message_pop ();
    if (m) reuse_message (m);
  }
}

/* 
 *  Return FALSE if all of the statements in a frame have
 *  been evaluated.  Return the stack index of the first
 *  unevaluated message otherwise.
 */

int unevaled (int this_frame_ptr) {

  int j,  msg_frame_top, next_msg_frame_top;
  MESSAGE *m;

  if ((msg_frame_top = message_frame_top_n (this_frame_ptr)) == -1)
    msg_frame_top = P_MESSAGES;

  if ((next_msg_frame_top = message_frame_top_n (this_frame_ptr - 1)) == -1)
    next_msg_frame_top = get_messageptr ();

  for ( j = msg_frame_top; j > next_msg_frame_top; j--) {
    m = message_stack_at (j);
    if (m && IS_MESSAGE (m) && !m -> evaled)
      return j;
  }

  return FALSE;
}

static int set_declaration_evaled (MESSAGE_STACK messages, int ptr) {
  int i, end_ptr;

  end_ptr = find_declaration_end (messages, ptr, messageptr);

  for ( i = ptr; i >= end_ptr; i--) {
    if (!messages[i] || !IS_MESSAGE (messages[i]))
      break;
    ++messages[i] -> evaled;
  }

  if (messages[i] && IS_MESSAGE (messages[i]))
    ++messages[i] -> evaled;
  return i;
}

/*
 *  Note - When calling parser_pass () separately, these must be 
 *  saved and restored.  TO DO - Use CURRENT_PARSER -> frame
 *  instead.
 */

static int frame_level = 0;
static int frame_ptr = 0;

/*
 *   Functions that handle the parser stack.
 */

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

void init_parsers (void) {
  current_parser_ptr = MAXARGS + 1;
}

static int end_of_input (void) {
  return ((current_parser_ptr >= MAXARGS) &&
	  (input_idx == (n_input_files - 1)));
}

int parser_ptr (void) {
  return current_parser_ptr;
}


void push_parser (PARSER *p) {
  if (current_parser_ptr == 0)
    _error ("push_parser: stack overflow.");
  parsers[--current_parser_ptr] = p;
}

static char *__ctrl_keywords[] = {
  "break",
  "case",
  "default",
  "do",
  "else",
  "for",
  "goto",
  "if",
  "return",
  "switch",
  "while"
};
#define __N_CTRL_KEYWORDS 11

static inline int tok_can_begin_var_declaration (MESSAGE *m) {
  int i;
  if (M_TOK(m) == LABEL) {
    for (i = 0; i < __N_CTRL_KEYWORDS; i++) {
      if (str_eq (M_NAME(m), __ctrl_keywords[i]))
	return FALSE;
    }
    if (get_function (M_NAME(m)))
      return FALSE;
    return TRUE;
  }
  return FALSE;
}

PARSER *pop_parser (void) {
  PARSER *p;
  if (current_parser_ptr > MAXARGS)
    _error ("pop_parser: stack underflow.");
  p = parsers[current_parser_ptr] ;
  parsers[current_parser_ptr++] = NULL;
  return p;
}

PARSER *parser_at (int i) {
  return parsers[i];
}

int current_parser_level (void) {
  return parsers[current_parser_ptr] -> level;
}

/* 
 *  TO DO - This needs to be a stack, or added in the
 *  parserinfo struct.
 */

struct {
  int line;
  char filename[FILENAME_MAX];
  int flag;
} line_marker;


#define GCC_EXTENSION_W_EXPR(s) ((*s == '_') && \
			 (str_eq (s, "__attribute__") || str_eq (s, "__asm__")))

/* 
 *  Parse the statements within the stack frame given by
 *  the frame stack index given in the argument.
 */

int parser_pass (int this_frame_ptr, PARSER *p) {

  int start_ptr = P_MESSAGES,
    end_ptr = messageptr,
    j, k, l, n,
    typecast_end_ptr;
  CVAR *local_vars;
  MESSAGE *m;
  bool have_printf_fmt;
  I_EXCEPTION *ex;
  char expr_out[MAXMSG];

  p -> frame = frame_ptr = this_frame_ptr;

  have_printf_fmt = False;

  if (frames[this_frame_ptr])
    start_ptr = frames[this_frame_ptr] -> message_frame_top;

  if (frames[this_frame_ptr - 1])
    end_ptr = frames[this_frame_ptr - 1] -> message_frame_top;

  for ( j = start_ptr; j > end_ptr; j--) {

    m = messages[j];

    error_line = m -> error_line;
    error_column = m -> error_column;

    if (have_printf_fmt) 
      m -> attrs |= TOK_IS_PRINTF_ARG;

    switch (ctrl_block_state (j))
      {
      case PRED_END:   /*  Write a loop beginning and predicate 
			   block for loops that have a single 
			   statement and no braces enclosing the
			   block.  The entire loop is contained
			   in this frame.
		       */
	loop_pred_end (message_stack (), j);
	break;
      case BLK_START:  /* Start outputting the loop predicate block
			  for while and for loops.  The frame ends at
			  the opening brace, so predicate blocks at 
			  the beginning of the loop can be output 
			  immediately after the frame, in addition
			  to the generic loop start.  

			  If the control block does not have braces,
			  resolve () only scans back to the first
			  label of the block.  The predicate and body 
			  are within one frame.

			  Also check for a completely empty block - 
			  a loop with only a semicolon - and handle
			  the block end here also if necessary.

			  If the code block does not have braces,
			  insert an opening brace in the output.
		       */
	loop_block_start (message_stack (), j);
	if (C_CTRL_BLK -> blk_start_ptr == C_CTRL_BLK -> blk_end_ptr)
	  loop_block_end (message_stack (), j);
	break;
      case BLK_END:    /* Output the loop predicate block for do...while
			  loops.  A closing brace should occur in a frame
			  by itself - so we can output predicate
			  blocks for do... while loops immediately before
			  the frame.

			  Insert a closing brace in the output if the
			  code block is not enclosed by braces.
		       */
	loop_block_end (message_stack (), j);
	break;
      case ELSE_END:
	if_else_end (message_stack (), j);
	break;
      }
    argblk_end (message_stack (), j);

    switch (m -> tokentype) 
      {
      case SEMICOLON:
	if (have_printf_fmt)
	  have_printf_fmt = False;
	++m -> evaled;
	break;
      case PREPROCESS:
	if (IS_MESSAGE(messages[j-2])) {
	  if (messages[j - 2] -> tokentype == INTEGER) {
	    set_line_info (message_stack (), j, end_ptr);
	  }
	}
	++m -> evaled;
	break;
      case CTYPE:
      case LABEL:
	/*
	 *  Check GNU C attributes and fall through.
	 */
#ifdef __GNUC__
	/*
	 *  This should be moved down if possible.
	 */
	if (GCC_EXTENSION_W_EXPR(M_NAME(m))) {
	  int l, lookahead;
	  if ((lookahead = nextlangmsg (message_stack (), j)) == ERROR)
	    _error ("Parser error.\n");
	  if (message_stack_at (lookahead) -> tokentype != OPENPAREN)  {
	    /*
	     *  Looking ahead for only one extension, e.g., 
	     *    __asm__ __volatile__
	     *  is not that great, but there aren't many actual code 
	     *  examples.
	     */
	    if (is_gnu_extension_keyword (message_stack_at (lookahead) -> name)) {
	      lookahead = nextlangmsg (message_stack (), lookahead);
	      if (message_stack_at (lookahead) -> tokentype != OPENPAREN) {
		_error ("Parser error.\n");
	      }
	    } else {
	      _error ("Parser error.\n");
	    }
	  }
	  if ((lookahead = match_paren (message_stack (), lookahead, end_ptr))
	      != ERROR) {
	    if (message_stack_at (lookahead) -> tokentype != CLOSEPAREN)
	      _error ("Parser error.\n");
	    for (l = j; l >= lookahead; l--)
	      ++message_stack_at (l) -> evaled;
	    j = lookahead;
	  } else {
	    int l, lookahead, attr_start = j;
	    if ((lookahead = scanforward (message_stack (), attr_start, 
					  end_ptr, SEMICOLON)) != ERROR) {
	      for (l = j; l >= lookahead; l--)
		++message_stack_at (l) -> evaled;
	      j = lookahead;
	    } else {
	      int l2, lookahead2, attr_start2 = j;
	      if ((lookahead2 = scanforward (message_stack (), attr_start2, 
					     end_ptr, OPENBLOCK)) != ERROR) {
		for (l2 = j; l2 >= lookahead2; l2--)
		  ++message_stack_at (l2) -> evaled;
		j = lookahead2;
	      } else {
		int l2, lookahead2, attr_start2 = j;
		if ((lookahead2 = scanforward (message_stack (), attr_start2, 
					       end_ptr, OPENBLOCK)) != ERROR){
		  for (l2 = j; l2 >= lookahead2; l2--)
		    ++message_stack_at (l2) -> evaled;
		  j = lookahead2;
		}
	      }
	    }
	  }
	  continue;
	}
#endif
	if (m -> attrs & TOK_IS_FN_START) {
	  if (is_c_fn_declaration_msg (messages, j, messageptr)) {
	    int k;
 	    if (!library_input && (FRAME_SCOPE != PROTOTYPE_VAR)) {
	      /*
	       *  Look out for methods in main source file.
	       *  Method buffering takes place in new_instance_method ()
	       *  and new_class_method ().
	       */
	      if (interpreter_pass != method_pass) {
		get_params ();
		begin_function_buffer ();
		if(function_contains_argblk (messages, j)) {
		  CFUNC *fn;
		  if ((fn = get_function (get_fn_name ())) != NULL) {
		    if (IS_CVAR(fn -> params))
		      function_param_cvar_tab_entry (messages, j,
						   fn -> params);
		  }
		}
	      }
	    }
 	    p -> need_init = FALSE;
 	    /*  Look ahead to the end of the declaration
 	     *  and increment the block level if the
	     *  declaration is not a prototype.
 	     */
 	    for (k = j;
 		 messages[k] && messages[k] -> tokentype != 
		   ((FRAME_SCOPE == PROTOTYPE_VAR) ? SEMICOLON : OPENBLOCK);
 		 k--)
 	      ++messages[k] -> evaled;
 	    if (messages[k])
 	      ++messages[k] -> evaled;
 	    j = k;
	    if (FRAME_SCOPE != PROTOTYPE_VAR) {
	      ++p -> block_level;
	      if (!library_input) {
		/* messages[k] points to opening bracket */
		chk_C_return_alone (messages, k);
	      }
	    }
	  }
	} /* if (m -> attrs & TOK_IS_FN_START) { */
	/* fall through */
      case METHODMSGLABEL:
      case EQ:          /* Cases for overloaded operators, except for */
      case BOOLEAN_EQ:  /* opening and closing parentheses.           */
      case INEQUALITY:
      case GT:
      case GE:
      case ASR:
      case ASR_ASSIGN:
      case LT:
      case LE:
      case ASL:
      case ASL_ASSIGN:
      case PLUS:
      case PLUS_ASSIGN:
      case INCREMENT:
      case MINUS:
      case MINUS_ASSIGN:
      case DECREMENT:
      case DEREF:
      case MULT:
      case MULT_ASSIGN:
      case DIVIDE:
      case DIV_ASSIGN:
      case BIT_AND:
      case BOOLEAN_AND:
      case BIT_AND_ASSIGN:
      case BIT_COMP:
      case LOG_NEG:
      case BIT_OR:
      case BOOLEAN_OR:
      case BIT_OR_ASSIGN:
      case BIT_XOR:
      case BIT_XOR_ASSIGN:
      case PERIOD:
      case ELLIPSIS:
      case LITERALIZE:
      case MACRO_CONCAT:
      case CONDITIONAL:
      case COLON:
      case MODULUS:
      case MODULUS_ASSIGN:
      case MATCH:
      case NOMATCH:
	/* TO DO - 
	   Determine if the result should be m -> value_obj,
	   in the case of a method message, for example, and 
	   whether the value of m should be set in resolve ()
	   after determining if m's token is a receiver,
	   method, argument, or something else. */
	/* 
	 *  If the token does not resolve to an object...
	 *  also check whether resolve handled the expression but
	 *  didn't create an object.
	 */
	if (((m -> obj = resolve (j)) == NULL) && (m -> evaled == 0)) {
	  if (str_eq (M_NAME(m), "enum")) {
	    /* This helps streamline resolve a lot. */
	    k = find_declaration_end (messages, j, messageptr);
	    for ( ; j >= k; j--) {
	      ++messages[j] -> evaled;
	    }
	  } else { /* Check for a local variable declaration. */
	    if ((frame_at (this_frame_ptr) -> scope == LOCAL_VAR) && 
		!(messages[j] -> attrs & TOK_IS_DECLARED_C_VAR)) {

 	      if (tok_can_begin_var_declaration (messages[j]) &&
 		  is_c_var_declaration_msg (messages, j, messageptr, FALSE)) {

		if ((local_vars = parser_to_cvars ()) != NULL) {
		  add_variable_from_cvar (local_vars);

		   if (interpreter_pass == method_pass) {
		     /*
		      *  This should only be temporary.  For now,
		      *  the C var needs to be declared before the
		      *  init block.
		      */
		     if (p -> block_level == 1) {
		       if (new_method_has_contain_argblk_attr ()) {
			 argblk_fp_containers (messages, j, local_vars);
			 method_cvar_tab_entry (messages, j, local_vars);
		       }
		     }
		   } else if (interpreter_pass == parsing_pass) {
		     if (p -> block_level == 1) {
		       if (fn_has_argblk) {
			 argblk_fp_containers (messages, j, local_vars);
			 function_cvar_tab_entry (messages, j,
						      local_vars);
		       }
		     }
		   }

		}

 		if (interpreter_pass != c_fn_pass) {
		  int j_1;
		  j_1 = set_declaration_evaled (messages, j);
		  if (j_1 < j) {
		    int j_2;
		    for (j_2 = j; j_2 >= j_1; j_2--)
		      /* Same as above.  See the comments there. */
		      switch (ctrl_block_state (j_2))
			{
			case PRED_END:
			  loop_pred_end (message_stack (), j_2);
			  break;
			case BLK_START:  
			  loop_block_start (message_stack (), j_2);
			  if (C_CTRL_BLK -> blk_start_ptr == 
			      C_CTRL_BLK -> blk_end_ptr)
			    loop_block_end (message_stack (), j_2);
			  break;
			case BLK_END:    
			  loop_block_end (message_stack (), j_2);
			  break;
			case ELSE_END:
			  if_else_end (message_stack (), j_2);
			  break;
			}
		  }
		  j = j_1;
		}

	      } else {
		if (str_eq (m -> name, "returnObjectClass")) {
		  j = set_declaration_evaled (messages, j);
		} else {
		  control_structure (messages, j);
		}
	      }
	    } else {/* if (frame_at (this_frame_ptr) -> scope == LOCAL_VAR) */
	      if (str_eq (M_NAME(m), "main") && (j == start_ptr)) {
		warning (m, "Function, \"main,\" should be declared:\n\n\t"
			 "\"int main ...\"\n");
	      }
 	      if (str_eq (M_NAME(m), "extern")) {
#ifdef __GNUC__
 		gnuc_fix_empty_extern (messages, j);
#endif
		j = set_declaration_evaled (messages, j);
 	      }
 	    }
	  } /* 	if (is_c_fn_declaration_msg (j)) */
	}
	++m -> evaled;
	break;
      case LITERAL:
	if (is_printf_fmt_msg (messages, j, 
			       P_MESSAGES,
			       end_ptr)) {
	  have_printf_fmt = True;
	}
	/* Fall through */
      case LITERAL_CHAR:
      case INTEGER:
      case LONG:
      case LONGLONG:
      case FLOAT:
	if (!(m -> attrs & TOK_IS_RT_EXPR)) {
	    c_constant_object (message_stack (), j);
	    ++m -> evaled;
	}
	break;
      case SIZEOF:
	k = scanforward (message_stack (), j, end_ptr, OPENPAREN);
	n = match_paren (message_stack (), k, end_ptr);
	l = nextlangmsg (message_stack (), k);
	if (sizeof_arg_needs_rt_eval (message_stack(), l)) {
	  if (is_fmt_arg (message_stack (), j,
			  P_MESSAGES,
			  get_stack_top (message_stack ()))) {
	    char _s[MAXMSG];
	    fmt_rt_expr (message_stack (), j, &n, _s);
	    fileout (fmt_printf_fmt_arg 
		     (message_stack (), j, 
		      P_MESSAGES,
		      _s, expr_out),
		     0, j);
	  }
	  for (; j >= n; --j) {
	    ++message_stack_at (j) -> evaled;
	    ++message_stack_at (j) -> output;
	  }
 	  ++message_stack_at (j) -> evaled;
	} else {
	  for (; j >= n; --j)
	    ++message_stack_at (j) -> evaled;
	  ++message_stack_at (j) -> evaled;
	}
	break;
      case OPENPAREN:
	if (is_typecast_expr (messages, j, &typecast_end_ptr)) {
	  int j_1;
	  for (j_1 = j; j_1 >= typecast_end_ptr; j_1--) {
	    messages[j_1] -> attrs |= TOK_IS_TYPECAST_EXPR;
	    ++messages[j_1] -> evaled;
	  }
	} else {
	  ++messages[j] -> evaled;
	}
	break;
       case OPENBLOCK:
	 /*
	  *  The block level also is incremented after 
	  *  is_c_fn_declaration_msg (), above.
	  */
   	++p -> block_level;
 	++m -> evaled;
 	break;
      case CLOSEBLOCK:
   	--p -> block_level;
	/*
	 * End buffering a function or method.  The final closing 
	 * brace should appear in its own frame and is output in
	 * parse ().
	 *
	 * Output __ctalk_init () at the end of a function if 
	 * it has not already been output.
	 */
	if ((p -> block_level == 0) && 
	    fn_closure (messages, j, this_frame_ptr)) {
	  if (!library_input) {
 	    if (interpreter_pass != method_pass) {
	      end_function_buffer ();
	      /*
	       *  If there is no return statement at the 
	       *  end of the function, output a 
	       *  __ctalk_exitFn () call here.  The closing
	       *  brace itself gets output in parse (), 
	       *  below.
	       */
	      if (!fn_defined_by_header) {
		fn_return_at_close (message_stack (), j, p -> need_main_exit);
	      } else {
		fn_defined_by_header = FALSE;
	      }
	      if (p -> need_main_exit) p -> need_main_exit = FALSE;
 	    }
	  }
	  /*
	   *  FIXME! - We have to wait until the 
	   *  sub-parser exits before we output a method.
	   *  Otherwise, we output only a partial method
	   *  when parsing another method from within a
	   *  method.  That means the method's local 
	   *  variables are lost when formatting the method
	   *  after returning from the parser.  The function
	   *  is_c_var_declaration_msg2 () might be able 
	   *  to do without the local variable definitions
	   *  because the data types are global and still exist.  
	   *
	   *  TO DO - 
	   *  1. Fix the method buffering so that 
	   *     complete methods get output when 
	   *     called from here.
	   *
	   *     The save_method_local_objects () and 
	   *     save_method_local_cvars () functions 
	   *     should go away if the method buffering 
	   *     can be fixed.
	   */
	  if (interpreter_pass == method_pass) {
	    save_method_local_objects ();
	    save_method_local_cvars ();
	  }
	  /*
	   *  Note that the frame that contains the closing
	   *  block itself is not buffered and is output 
	   *  from parse (), below.
	   */
	  delete_objects (LOCAL_VAR);
	  delete_local_objects ();
	  delete_local_c_vars ();
   	}
	/* Fall through. */
      default:
	++m->evaled;
	break;
      } /* switch  (m -> tokentype) */

    if ((ex = __ctalkTrapExceptionInternal (m)) != NULL) {
      if (ex -> _exception != method_used_before_define_x) {
	char *buf;
	if (frames[this_frame_ptr-1]) {
	  buf = collect_tokens
	    (messages,
	     frames[this_frame_ptr] -> message_frame_top,
	     frames[this_frame_ptr-1] -> message_frame_top);
	} else {
	  buf = collect_tokens
	    (messages,
	     frames[this_frame_ptr] -> message_frame_top,
	     messageptr + 1);
	}
	/* have to think of something better here when
	   we have an example .. vsprintf can choke on 
	   some text that contains a '%' with no argument
	   list */
	if (!strchr (buf, '%'))
	  warning (m, buf);
	__xfree (MEMADDR(buf));
	__ctalkExceptionNotifyInternal (ex);
	exit (EXIT_FAILURE);
      } else {
	(void)__ctalkGetRunTimeException ();
      }
    }

  } /* for ( j = start_ptr; j > end_ptr; j--) */

  return SUCCESS;
}

/*
  IMPORTANT - Parser_frame_ptr () should only be called from 
  parser_pass (), or from functions that parser_pass () calls.
*/

/*
 *   Return the frame index that parser_pass is evaluating.
 *   TO DO - These should return CURRENT_PARSER -> frame,
 *   instead of frame_ptr, declared above, so we don't have
 *   to save and restore these individually.
 */
int parser_frame_ptr (void) {
  return frame_ptr;
}

void set_parser_frame_ptr (int ptr) {
  frame_ptr = ptr;
}

/* 
 *  Return the scope of the current parser frame.
 */

int parser_frame_scope (void) {
  FRAME *f;
  if (parser_frame_ptr () != ERROR) {
    f = frame_at (parser_frame_ptr ());
    return f -> scope;
  }
  return ERROR;
}

void init_parser_frame_ptr (void) {
  frame_ptr = ERROR;
}

/* 
 *   Functions to add frames and increment the frame level.
 */

int add_frame (int scope, int level) {
  new_frame (scope, level);
  ++frame_level;
  return frame_level;
}

int remove_frame (void) {
  delete_frame ();
  --frame_level;
  return frame_level;
}

typedef enum {
  before, 
  after,
  notneeded
} NEED_FRAME;

typedef enum {
  ctrl_null,
  ctrl_keyword1,
  ctrl_keyword2,
  ctrl_pred_start,
  ctrl_pred_end,
  ctrl_blk_start,
  ctrl_blk_end,
  ctrl_else,
  ctrl_else_start,
  ctrl_else_end
} CTRL_STATE;

/* 
 *   The first parser pass, which tokenizes the input and
 *   adds it to the message stack, and adds stack frames.
 */

int parser_level = 0;

char *parse (char *inbuf, long long bufsize) {

  long long int i = 0ll;
  int token_start;
  int j;
  int prev_frame_ptr;        /* Save and restore the old frame.            */
  int preprocess = FALSE;
  int need_line_no = FALSE;
  NEED_FRAME need_frame = notneeded;
  bool in_fn_decl;        /* This simplifies is_c_function_declaration. */
  int scope;
  int n_block_levels = 0;
  int n_parens = 0;
  int linemarker_line;
  int n_dots;
  int docstr_cx_idx, docstr_cx_idx_2;
  MESSAGE *m;
  PARSER *p;
  char *str;
  CTRL_STATE ctrl_state;
  STMT_CLASS ctrl_class = (STMT_CLASS)-1;
  bool ctrl_has_braces = False;   /* Avoid a warning. */
  bool require_stmt;
  int message_start_ptr;

  ++parser_level;
  prev_frame_ptr = frame_ptr;

  p = new_parserinfo ();
  parsers[--current_parser_ptr] = p;
  p -> level = parser_level;

  /*
   *    If this is the top level parser, the scope begins as
   *    GLOBAL_VAR.  Otherwise, further parsers inherit the scope
   *    from the upper level parser's frame; i.e., the last frame 
   *    before parse () was called again.  Because we already pushed
   *    this parser, above, the calling parser is at
   *    parsers[current_parser_ptr + 1].
   */
  if (parser_level == 1)
    scope = GLOBAL_VAR;
  else if (interpreter_pass == library_pass)
    /* should only be true if we're called by library_search */
    scope = GLOBAL_VAR;
  else
    scope = frames[parsers[current_parser_ptr + 1] -> frame] -> scope;

  in_fn_decl = FALSE;
  ctrl_state = ctrl_null;
  require_stmt = False;
  n_dots = 0;

  add_frame (scope, parser_level);

  p -> top_frame = frame_pointer;

  message_start_ptr = messageptr;

  while (i < bufsize) {

    if ((m = get_reused_message()) == NULL)
      error (messages[messageptr + 1], "Parse: Invalid message.");

    /* 
     *   Is_c_func_declaration () looks ahead on untokenized
     *   input.  Use lexical () instead of tokenize () and
     *   record the inbuf index of each token start.
     */
    token_start = i;

    lexical (inbuf, &i, m);

  m -> error_column = error_column;
  m -> error_line = error_line;

  if (m -> tokentype == NEWLINE) {
    error_column = 1;
    str = m -> name;
    while (*str++)
      ++error_line;
  } else if (m -> tokentype != LITERAL) {
    /* lexical updates the error line and column for literals
       so we don't have to do it here. */
    str = m -> name;
    while (*str++)
      ++error_column;
  }
  if (show_progress_opt) {
    ++n_dots;
    if ((n_dots % 100) == 0) {
      fprintf (stdout, ".");
      fflush (stdout);
    }
  }

  switch (m -> tokentype) 
    {
    case LABEL:
      if (is_c_function_declaration (&inbuf[token_start])) {
	FRAME *f;
	/* 
	   Recording the in_fn_decl state here simplifies 
	   is_c_function_declaration, and allows us
	   to set a frame regardless of the statements 
	   preceding the function declaration, at 
	   the cost of possibly setting an empty
	   frame immediately before the declaration.
	   TO DO - Try to omit empty frames that 
	   immediately precede the frame we set here. 
	*/
	if (!in_fn_decl) {
	  in_fn_decl = True;
	  need_frame = before;
	}
	if (main_declaration && !main_start) {
	  main_declaration = FALSE;
	  main_start_frame = get_frame_pointer ();
	}
	/* The function parameters get the scope ARG_VAR,
	   although they should be treated as local 
	   variables when they are registered. */
	f = frame_at (get_frame_pointer ());
	f -> scope = scope = ARG_VAR;
	m -> attrs |= TOK_IS_FN_START;
      } else if (str_eq (M_NAME(m), "self")) {
	m -> attrs |= TOK_SELF;
      } else if (str_eq (M_NAME(m), "super")) {
	m -> attrs |= TOK_SUPER;
      } else {
	  /*
	   *  The parser does not handle framing for constructs 
	   *  like this here.
	   *  if (...)
	   *    do {...} while (...);
	   *
	   *  There are separate tests in if_stmt () that look
	   *  past the frames that the parser places before and 
	   *  after the do statement's block.
	   */
	  if (is_ctrl_keyword (m -> name)) {
	    ctrl_class = get_ctrl_class (m -> name);
	    if (ctrl_class == stmt_else) {
	      ctrl_state = ctrl_else;
	      if (ctrl_has_braces == False) need_frame = after;
	    } else {
	      ctrl_state = ctrl_keyword1;
	    }
	  } else {
	    /*
	     *  Require statements get their own frames
	     *  so we can position line markers more 
	     *  accurately.
	     */
	    if (str_eq (m -> name, "require")) {
	      require_stmt = True;
	      need_frame = before;
	    } 
	  }
	}
	/*
	 *  If the parser encounters a label and 
	 *  the ctrl_state is ctrl_pred_end, it
	 *  means that the input has a control structure
	 *  without control blocks.  Pretend there's 
	 *  an opening brace and set a frame.
	 *
	 *  Unless it's a break, which, if a normal label follows it,
	 *  should always need to be a break like this:
	 *
	 *  while (<something>) {
	 *     ...
	 *     if (<something_else>)
	 *       break;
         *  }                   -- We should already have set a frame after
	 *                         the brace.
	 *    ...  <our label>  -- There can be anything preceding the label,
	 *                         (probably a prefix operator), which 
	 *                         would end up in a frame of its own.
	 * 
	 *  TODO - reset the ctrl_state after a frameless break control
	 *  block statement.
	 */
	if (ctrl_state == ctrl_pred_end) {
	  if (ctrl_class != stmt_break) {
	    ctrl_state = ctrl_blk_start;
	    need_frame = before;
	    ctrl_has_braces = False;
	  }
	}
	break;
	/*
	 *  Needed for line_info.  
	 *  After the tokens are collected, if we are parsing the
	 *  source file, elide the line marker, because we will
	 *  place a new line marker in the output when the function
	 *  is unbuffered.
	 *  
	 *  A line marker placed before a function has the same name
	 *  as the source file.  Elide all source line markers except
	 *  the marker at line 1, which is placed automatically.
	 *
	 *  Note that this depends on the format of a line marker.
	 */
      case NEWLINE:
	if (preprocess) {
	  --error_line;  /* The line number was incremented in 
			    set_error_location (), but the
			    line number in the line marker refers
			    to the line _following_ the line marker,
			    so undo this newline's increment. */
	  preprocess = FALSE;
	  need_frame = after;
	}
	break;
      case OPENBLOCK:
	need_frame = after;
	++n_block_levels;

	if (n_block_levels == 1) {
	  if (frame_at (get_frame_pointer ()) -> scope == ARG_VAR) {
	    scope = LOCAL_VAR;
	    in_fn_decl = False;
	  }
	  if (interpreter_pass == method_pass) {
	    error_line += new_method_param_newline_count ();
	  }
	}
	if (ctrl_state == ctrl_pred_start)
	  ctrl_state = ctrl_blk_start;
	if (ctrl_state == ctrl_pred_end)
	  ctrl_state = ctrl_blk_start;
	if (ctrl_state == ctrl_else)
	  ctrl_state = ctrl_else_start;
	if ((ctrl_state == ctrl_pred_start) || (ctrl_state == ctrl_else_start))
	  ctrl_has_braces = True;
	break;
      case CLOSEBLOCK:
	--n_block_levels;
	if (!n_block_levels)
	  scope = GLOBAL_VAR;
	/*
	 *  A state change to ctrl_null should work as long as 
	 *  the parser doesn't need to check for matching 
	 *  if... else clauses.
	 */
	if ((ctrl_state == ctrl_blk_start) || (ctrl_state == ctrl_else_start))
	  ctrl_state = ctrl_null;
	need_frame = after;
	break;
      case OPENPAREN:
	++n_parens;
	if ((n_parens == 1) && (ctrl_state == ctrl_keyword1))
	  ctrl_state = ctrl_pred_start;
	/* If we have a control block without braces that
	   starts with a cast expression.... */
	if ((n_parens == 1) && (ctrl_state == ctrl_pred_end)) {
	  if (ctrl_class != stmt_break) {
	    ctrl_state = ctrl_blk_start;
	    need_frame = before;
	    ctrl_has_braces = False;
	  }
	}
	break;
      case CLOSEPAREN:
	--n_parens;
	if ((n_parens == 0) && (ctrl_state == ctrl_pred_start))
	  ctrl_state = ctrl_pred_end;
	break;
      case CPPCOMMENT:
	break;
      case PREPROCESS:
   	preprocess = TRUE;
	need_line_no = TRUE;
	break;
      case INTEGER:
	if (need_line_no) {
	  error_line = linemarker_line = atoi (M_NAME(m));
	  need_line_no = FALSE;
	}
	break;
      case LITERAL:
	if (!preprocess) {
	  /* Check for a document string - a literal after a <close paren>
	     <opening block> sequence (presumably at the start of a method body,
	     but it could be a function start, too).  Skip any intervening
	     whitespace. */
	  for (docstr_cx_idx = messageptr + 1; 
	       docstr_cx_idx <= P_MESSAGES; docstr_cx_idx++) {
	    if (M_ISSPACE(messages[docstr_cx_idx]))
	      continue;
	    if (M_TOK(messages[docstr_cx_idx]) == OPENBLOCK) {
	      for (docstr_cx_idx_2 = docstr_cx_idx + 1; 
		   docstr_cx_idx_2 <= P_MESSAGES; docstr_cx_idx_2++) {
		if (M_ISSPACE(messages[docstr_cx_idx_2]))
		  continue;
		if (M_TOK(messages[docstr_cx_idx_2]) == CLOSEPAREN) {
		  /* set_error_location_above (), also adds newlines
		     to error_line, so restart at the starting line of the
		     docstring. */
		  error_line = m -> error_line + docstring_to_line_count (m);
		  message_push (m);
		  goto docstr_done;
		} else {
		  goto docstr_done;
		}
	      }
	    } else {
	      goto docstr_done;
	    }
	  }
	docstr_done:
	  /* Continue with the outer loop if the message is now whitespace. */
	  if (M_TOK(m) == WHITESPACE)
	    continue;
	} /* if (!preprocess) { */
	break;
      case SEMICOLON:
	/*
	 *  Don't put frames in the middle of for loop 
	 *  statements.
	 */
	if (!n_parens)
	  need_frame = after;

	if (require_stmt == True) {
	  require_stmt = False;
	  need_frame = after;
	}

	if (ctrl_state == ctrl_null)
	  need_frame = after;
	if ((ctrl_state == ctrl_blk_start) && (ctrl_has_braces == False)) {
	  need_frame = after;
	  ctrl_state = ctrl_null;
	} 
	break;
      case COLON:
	if (ctrl_class == stmt_default) {
	  need_frame = after;
	  ctrl_class = (STMT_CLASS)-1;
	  ctrl_state = ctrl_null;
	}
	if (ctrl_class == stmt_case) {
	  need_frame = after;
	  ctrl_class = (STMT_CLASS)-1;
	  ctrl_state = ctrl_null;
	}
	break;
      case INCREMENT:
      case DECREMENT:
      case BIT_COMP:
      case LOG_NEG:
      case AMPERSAND:
      case ASTERISK:
      case SIZEOF:
	/* Note when we're at the start of a control block that
	   doesn't have curly braces. */
	if (ctrl_state == ctrl_pred_end) {
	  if (ctrl_class == stmt_for || ctrl_class == stmt_if ||
	      ctrl_class == stmt_while || ctrl_class == stmt_else ||
	      ctrl_class == stmt_do) {
	    ctrl_state = ctrl_blk_start;
	    need_frame = before;
	  }
	}
	break;
      default:
	break;
      }

    switch (need_frame)
      {
      case notneeded:
	message_push (m);
	break;
      case before:
	need_frame = notneeded;
	add_frame (scope, parser_level);
	p -> frame = get_frame_pointer ();
	message_push (m);
	break;
      case after:
	message_push (m);
	need_frame = notneeded;
	add_frame (scope, parser_level);
	p -> frame = get_frame_pointer ();
	break;
      }

  } /* while (i < bufsize) */

  if (interpreter_pass == library_pass) {
    interpreter_pass = var_pass;
    parse_vars_and_prototypes (messages, message_start_ptr,
			       messageptr);
    resolve_incomplete_types ();
    interpreter_pass = library_pass;
    __fileout ("\n");
  }
  
  p -> last_frame = get_frame_pointer ();
  
  for (j = p -> top_frame; j >= p -> last_frame; j--) {

    if (j == main_start_frame) {
      p -> need_main_init = TRUE;
      p -> need_main_exit = TRUE;
#ifdef MAIN_BREAK
      printf (FN_BREAK_MSG, "main");
      asm("int3;");
#endif      
    }

    while ((unevaled (j)) != 0)
      parser_pass (j, p);

    output_imported_methods ();

    output_frame (messages, j,
		  (frames[j] ?
		   frames[j]->message_frame_top : P_MESSAGES),
		  (frames[j-1] ?
		   frames[j-1]->message_frame_top : messageptr),
		  (frames[j-2] ?
		   frames[j-2]->message_frame_top : messageptr));

    /*
     *  If there is a change of frame scope from LOCAL_VAR to
     *  GLOBAL_VAR, it means we are at the end of a function
     *  or method.  Output templates.
     */
    if ((j < MAXFRAMES) &&
	((frame_at (j + 1) -> scope == LOCAL_VAR) &&
	 (frame_at (j) -> scope == GLOBAL_VAR)))
      unbuffer_fn_templates ();
  }
	
  if (interpreter_pass == method_pass) {
    last_method_line = 
      messages[frame_at(p->last_frame)->message_frame_top+1]->error_line;
  }
  for (j = p -> last_frame; j <= p -> top_frame; j++) {
    clear_frame_messages (j);
    remove_frame ();
  }

  --parser_level;
  frame_ptr = prev_frame_ptr;

  if (interpreter_pass == expr_check) {
    /* Normally occurs at the end of a method. */
    delete_local_objects ();
    delete_local_c_vars ();
  }

  if (end_of_input ()) {  
    warn_unresolved_labels ();
    /*
     *  Call fileout () while there is still a parser.
     */
    check_return_classes ();
    output_global_init ();
  }

  delete_parserinfo (pop_parser ());

  return  inbuf;
}

/*
 *  Check for valid state transitions between tokens,
 *  excluding newline and whitespace tokens.
 */

int check_state (int stack_idx, MESSAGE **messages, int *states, 
		 int cols) {
  register int i;
  int token;
  int next_idx = stack_idx;
  int last_idx;
  register int *states_ptr;

  states_ptr = states;
  token = messages[stack_idx] -> tokentype;

  last_idx = get_stack_top (messages);

  for (i = 0; ; i++) {
  nextrans:
    if (states_ptr[i+i] == ERROR)
      return ERROR;
    if (states_ptr[i+i] == token) {
      for (next_idx = stack_idx - 1; next_idx > last_idx; next_idx--) {
	while (M_ISSPACE(messages[next_idx])) {
	  next_idx--;
	  if (!messages[next_idx]) return ERROR;
	}
	if (states_ptr[i+i+1] != messages[next_idx]->tokentype) {
	  ++i;
	  goto nextrans;
	} else {
	  return i;
	}
      }
    }
  }
  return ERROR;
}
      
PARSER *new_parserinfo (void) {

  PARSER *p;

  if ((p = (PARSER *)__xalloc (sizeof (PARSER))) == NULL)
    _error ("new_parserinfo: %s.", strerror (errno));

  p -> sig = PARSER_SIG;
  p -> block_level = 0;
  p -> need_init = TRUE;
  p -> need_main_init = FALSE;
  p -> _p_exception = no_x;
  p -> pass = interpreter_pass;
  p -> pred_state = ctrlblk_pred;
  p -> blk_state = ctrlblk_blk;
  p -> else_state = ctrlblk_else_blk;
  return p;

}

void delete_parserinfo (PARSER *p) {
  int c_idx;

  ctrlblk_pred = p -> pred_state;
  ctrlblk_blk = p -> blk_state;
  ctrlblk_else_blk = p -> else_state;
  
  /*
   *  In case there is an empty block after the end of the last loop
   *  at a parser level.  Probably needed for other various
   *  occurrences also.
   */
  for (c_idx = ctrlblk_ptr + 1; c_idx <= MAXARGS; c_idx++) {
    if (ctrlblks[c_idx]->level == p -> level) {
      delete_control_block (ctrlblk_pop ());
    }
  }

  if (p -> cvars != NULL) {
    CVAR *c, *c_prev;
    for (c = p -> cvars; c -> next; c = c -> next)
      ;
  
    while (c != p -> cvars) {
      c_prev = c -> prev;
      _delete_cvar (c);
      c = c_prev;
    }
    _delete_cvar (c);
    p -> cvars = NULL;
  }
  __xfree (MEMADDR(p));
}

/*
 *   Find the end of a preprocessor statement - the index of the
 *   token immediately before the newline.
 */
int end_of_p_stmt (MESSAGE_STACK messages, int msg_ptr) {

  int i = msg_ptr;
  while (1) {
    /*
     *  Check for input that is not terminated by a newline.
     */
    if (!messages[i - 1] || !IS_MESSAGE (messages[i - 1]))
      break;
    if (messages[i - 1] -> tokentype == NEWLINE 
	&& !LINE_SPLICE (messages, i - 1))
      break;
    i--;
  }
  return i;
}

/* see the comments in argblk.c for argblk_fp_container */
void fp_container_adjust (int tok_idx) {
  int i, f_i, last_frame;
  FRAME *f;
  for (i = messageptr + 1; i <= tok_idx; ++i) {
    messages[i-1] = messages[i];
  }
  messages[tok_idx] = NULL;
  last_frame = get_frame_pointer ();
  for (f_i = parsers[current_parser_ptr]->frame; f_i >= last_frame;
       f_i--) {
    f = frame_at (f_i);
    if (f -> message_frame_top < tok_idx) {
      --f -> message_frame_top;
    }
  }
  --messageptr;
}
