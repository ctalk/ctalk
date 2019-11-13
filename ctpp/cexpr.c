/* $Id: cexpr.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2014, 2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include "ctpp.h"
#include "typeof.h"

bool constant_expr = False;        /* True if syntax requires a 
					 constant expression.              */

int constant_expr_eval_lvl = 0;   /* > 0 if evaluating constant
				     expressions.                       */
extern EXCEPTION parse_exception;    /* Declared in ppexcept.c.            */

extern MESSAGE *i_messages[P_MESSAGES +1];  /* Declared in preprocess.c    */
extern int i_message_ptr;

/* 
 *    The function expands and evaluates the argument, and
 *    tokenizes the argument again in case an evaluation produced
 *    derived tokens.  
 *
 *    The function must take care that it doesn't expand
 *    arguments that are already valid forms: e.g., enclosed
 *    in "<>" delimiters or "\"" quotes.
 *
 *    Don't check the state because the statement is 
 *    terminated by a newline.
 */
void parse_include_directive (MESSAGE_STACK messages, int start, int end,
 			     INCLUDE *inc) {

  int i, j;
  int dir_idx,
    arg_idx,
    stack_begin,
    stack_end;
  int lasttoken = -1;
  char *t_argbuf,
    argbuf[MAXMSG];
  VAL result;

  memset ((void *) &result, 0, sizeof (VAL));

  if ((dir_idx = nextlangmsg (messages, start)) == ERROR)
    _error ("Internal error in parse_include_directive ().\n");
  if ((arg_idx = nextlangmsg (messages, dir_idx)) == ERROR)
    _error ("Internal error in parse_include_directive ().\n");

  t_argbuf = collect_tokens (messages, arg_idx, end);
  sprintf (argbuf, "%s\n", t_argbuf);
  free (t_argbuf);

  stack_begin = stack_start (i_messages);
  stack_end = tokenize_reuse (i_message_push, argbuf);

  /*
   *  Check for a valid filename argument before evaluating.
   */
  if ((i_messages[stack_begin] -> tokentype != LT) &&
      (i_messages[stack_begin] -> tokentype != LITERAL)) {
    (void)macro_sub_parse (i_messages, stack_begin, &stack_end, &result);
    (void)eval_constant_expr (i_messages, stack_begin, &stack_end, &result);
  }

  t_argbuf = collect_tokens (i_messages, stack_begin, stack_end);
  strcpy (argbuf, t_argbuf);
  free (t_argbuf);

  for (i = stack_begin; i >= stack_end; i--) {
    MESSAGE *m;
    m = i_message_pop ();
    if (m && IS_MESSAGE (m))
      reuse_message (m);
  }

  stack_begin = stack_start (i_messages);
  stack_end = tokenize_reuse (i_message_push, argbuf);

  for (i = stack_begin, *inc -> name = 0; i >= stack_end; i--) {
    
    if (i_messages[i] -> tokentype == WHITESPACE)
      continue;
    if (i_messages[i] -> tokentype == NEWLINE ||
 	i_messages[i] -> tokentype == OPENCCOMMENT)
      break;

    switch (i_messages[i] -> tokentype)
      {
      case LITERAL:
 	strcpy (inc -> name, i_messages[i] -> name);
	for (j = 1; inc -> name[j] && index (INC_PATH_CHARS, inc -> name[j]);
	     j++)
	  ;
  	(void)substrcpy (inc -> name, inc -> name, 1, j-1);
 	inc -> path_type = abs_path;
 	break;
      case LT:
 	inc -> path_type = inc_path;
 	break;
      case LABEL:
 	if (lasttoken == FWDSLASH || lasttoken == LT || lasttoken == PERIOD ||
 	    lasttoken == MINUS || lasttoken == PLUS)
 	  strcat (inc -> name, i_messages[i] -> name);
 	break;
      default:
	for (j = 0; 
	     i_messages[i] -> name[j] && 
	       index (INC_PATH_CHARS, i_messages[i] -> name[j]);
	     j++)
	  strncat (inc -> name, &(i_messages[i] -> name[j]), 1);
	break;
      }
    lasttoken = i_messages[i] -> tokentype;
  }

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
  sol10_use_gnuc_stdarg_h (inc);
#endif
  for (i = stack_begin; i >= stack_end; i--) {
    MESSAGE *m;
    m = i_message_pop ();
    if (m && IS_MESSAGE (m))
      reuse_message (m);
  }

}

static char *collect_has_inc_name (int open_delimiter_tok,
				   MESSAGE_STACK messages,
				   int name_start_idx,
				   int end, char *name_buf) {
  int i, close_delimiter_tok;

  if (open_delimiter_tok == LT)
    close_delimiter_tok = GT;
  else if (open_delimiter_tok == EQ)
    close_delimiter_tok = EQ;

  *name_buf = 0;
  for (i = name_start_idx; i >= end; i--) {
    if (M_TOK(messages[i]) == close_delimiter_tok)
      break;
    if (M_ISSPACE(messages[i]))
      continue;
    strcat (name_buf, M_NAME(messages[i]));
  }
  return name_buf;
}

static int parse_has_include (MESSAGE_STACK messages, int keyword_idx,
			      int *end_idx_out, int end_expr, 
			      char *inc_name) {
  int open_paren_idx, close_paren_idx, delimiter_idx, name_start_idx;
  if ((open_paren_idx = nextlangmsg (messages, keyword_idx)) == ERROR)
    return ERROR;
  if ((delimiter_idx = nextlangmsg (messages, open_paren_idx)) == ERROR)
    return ERROR;
  if ((name_start_idx = nextlangmsg (messages, delimiter_idx)) == ERROR)
    return ERROR;
  if ((close_paren_idx = p_match_paren (messages, open_paren_idx,
					get_stack_top (messages)))
      == ERROR)
    return ERROR;
  else
    *end_idx_out = close_paren_idx;

  collect_has_inc_name (M_TOK(messages[delimiter_idx]),
			messages, name_start_idx, end_expr,
			inc_name);
  return SUCCESS;
}

/*
  Evaluate a constant expression.  This is used by the preprocessor.
 */

int eval_constant_expr (MESSAGE_STACK messages, int start, int *end, 
			VAL *result) {

  int i;
  MESSAGE *m;
  DEFINITION *d;
  int tclose_paren, close_paren;
  int precedence = 0;
  int conditional_end_ptr,
    sizeof_end_ptr,
     operand_end;
  int __next_tok_idx, __operand_end_2_orig, __operand_end_2, j;
  char inc_name[MAXLABEL * 3];

  ++constant_expr_eval_lvl;

  /* Resolve the symbols and numeric values in the expression. */
  for (i = start; i >= *end; i--) {

    if (parse_exception != success_x) {
      --constant_expr_eval_lvl;
      return FALSE;
    }

    m = messages[i];

    switch (m -> tokentype) 
      {
      case CHAR:
	result -> __value.__i = m -> name[0] - '0';
	result -> __type = INTEGER_T;
 	sprintf (m -> value, "%d", (int) m -> name[0]);
	break;
      case LITERAL_CHAR:
	result -> __value.__i = m -> name[1] - '0';
	result -> __type = INTEGER_T;
	sprintf (m -> value, "%d", (int) m -> name[1]);
	break;
      case LITERAL:
	result -> __value.__ptr = m -> name;
	result -> __type = PTR_T;
	m_print_val (m, result);
	++m -> evaled;
	break;
      case INTEGER:
      case UINTEGER:
      case LONG:
      case ULONG:
      case LONGLONG:
	(void)numeric_value (m -> name, result);
	m_print_val (m, result);
	++m -> evaled;
	break;
      case DOUBLE:
	(void)sscanf (m -> name, "%lf", &result -> __value.__d);
	result -> __type = DOUBLE_T;
	m_print_val (m, result);
	++m -> evaled;
	break;
      case LABEL:
	if (!is_c_keyword (m -> name)) {
	  if ((d = get_symbol (m -> name, FALSE)) != NULL) {
	    d = sub_symbol (d);
	    if (d -> m_args[0]) {
	      int expr_end_orig, expr_end_new, expr_end_change;
	      if (!strncmp (d -> name, "__has_include", 13)) {
		if (parse_has_include (messages, i, &__operand_end_2_orig,
				       *end, inc_name) < 0) {
		  new_exception (parse_error_x);
		  continue;
		}
		result -> __value.__b = has_include (inc_name, false);
		result -> __type = BOOLEAN_T;
		for (j = i; j >= __operand_end_2_orig; j--) {
		  messages[j] -> tokentype = PREPROCESS_EVALED;
		  m_print_val (messages[j], result);
		  ++(messages[j] -> evaled);
		}
		j = __operand_end_2_orig;
		continue;
	      } else if (!strncmp (d -> name, "__has_include_next", 18)) {
		if (parse_has_include (messages, i, &__operand_end_2_orig,
				       *end, inc_name) < 0) {
		  new_exception (parse_error_x);
		  continue;
		}
		result -> __value.__b = has_include (inc_name, true);
		result -> __type = BOOLEAN_T;
		for (j = i; j >= __operand_end_2_orig; j--) {
		  messages[j] -> tokentype = PREPROCESS_EVALED;
		  m_print_val (messages[j], result);
		  ++(messages[j] -> evaled);
		}
		j = __operand_end_2_orig;
		continue;
	      } else {
		expr_end_orig = expr_end_new = 
		  p_match_paren (messages, i, get_stack_top (messages));
		(void)replace_args (d, messages, i, &expr_end_new);
		expr_end_change = expr_end_orig - expr_end_new;
		*end -= expr_end_change;
		i += 1;
	      }
	    } else {
	      memset ((void *)result, 0, sizeof (VAL));
 	      (void)resolve_symbol (d -> name, result);
	      switch (result -> __type)
		{
		case INTEGER_T:
		case LONG_T:
		case LONGLONG_T:
		case DOUBLE_T:
		case FLOAT_T:
		case LONGDOUBLE_T:
		case LITERAL_T:
		case LITERAL_CHAR_T:
		  m_print_val (m, result);
		  break;
		case PTR_T:
		  /*
		   *  TO DO - This test should be in m_print_val
		   *  in val.c.
		   */
		  if ((result -> __type == 10) &&
		      (result -> __value.__ptr != NULL))
		    m_print_val (m, result);
		  break;
		}
	    }
	  } else {
	    /*
	     *  Undefined symbol.
	     */
	    if (!strcmp (m -> name, "defined")) {
	      if ((__next_tok_idx = nextlangmsg (messages, i))== ERROR){
		new_exception (parse_error_x);
	      }
	      if (M_TOK(messages[__next_tok_idx]) == OPENPAREN) {
		__operand_end_2_orig = 
		  p_match_paren (messages, __next_tok_idx, 
				 get_stack_top(messages));
		if (__operand_end_2_orig == ERROR) {
		  _warning ("Close paren not found.\n");
		  new_exception (parse_error_x);
		}
	      } else {
		__operand_end_2_orig = __next_tok_idx;
	      }
	      (void)handle_defined_op (messages, i, &__operand_end_2, result);
	      *end -= (__operand_end_2_orig - __operand_end_2);
	      for (j = i; j >= __operand_end_2; j--) {
		messages[j] -> tokentype = PREPROCESS_EVALED;
		m_print_val (messages[j], result);
		++(messages[j] -> evaled);
	      }
#if 0 
	      /*
	       *  We don't check this here (yet), because as yet LLVM
	       *  only uses __has_include if it's also defined as as a
	       *  macro, which means there's a DEFINITION for it, so
	       *  we've matched the macro definition for it in the
	       *  clause above.
	       */
	    } else if (!strcmp (m -> name, "__has_include")) { /***/
	      if (parse_has_include (messages, i, &__operand_end_2_orig,
				     *end, inc_name) < 0) {
		new_exception (parse_error_x);
		continue;
	      }
	      result -> __value.__b = has_include (inc_name, false);
	      result -> __type = BOOLEAN_T;
	      for (j = i; j >= __operand_end_2; j--) {
		messages[j] -> tokentype = PREPROCESS_EVALED;
		m_print_val (messages[j], result);
		++(messages[j] -> evaled);
	      }
#endif
	    } else {  
	      new_exception (undefined_symbol_x);
	      result -> __type = INTEGER_T;
	      result -> __value.__i = 0;
	      m_print_val (m, result);
	    }
 	  }
	}	
	break;
      case LITERALIZE:
      case PREPROCESS:
	/*
	 *  Check for an assertion.  Check for literalize here also,
	 *  because that is how the tokenizer interprets a pound that 
	 *  doesn't occur in column 1.
	 */
	if (is_assertion (messages[nextlangmsg (messages, i)] -> name)) {
	  if (!check_assertion (messages, i, &operand_end, result)) {
	    new_exception (false_assertion_x);
	  } else {
	    int j;
	    for (j = i; j >= operand_end; j--) {
	      messages[j] -> tokentype = PREPROCESS_EVALED;
	      m_print_val (messages[j], result);
	      ++(messages[j] -> evaled);
	    }
	  }
	}
	break;
      default:
	break;
      }

    if (handle_preprocess_exception (messages, i, start, *end))
      goto done;
  }

  re_eval: for (i = start; i >= *end; i--) {

    if (parse_exception != success_x)
      goto done;

    m = messages[i];

      switch (m -> tokentype) 
	{
	case SEMICOLON:
	  goto done;
	  break;
	case NEWLINE:
	  if (!LINE_SPLICE (messages, i))
	    goto done;
	  break;
	case CHAR:
	case LITERAL_CHAR:
	case LITERAL:
	case INTEGER:
	case UINTEGER:
	case LONG:
	case ULONG:
	case LONGLONG:
	case DOUBLE:
	case WHITESPACE:
	case PREPROCESS_EVALED:
	case RESULT:
	case LABEL:
	  ++m -> evaled; /***/
	  break;
	default:
	  if (op_precedence (precedence, messages, i)) { 
	    switch (m -> tokentype)
	      {
	      case OPENPAREN:
		if ((tclose_paren = close_paren = match_paren (messages, i,
					     get_stack_top (messages)))
		    == ERROR) {
		  new_exception (mismatched_paren_x);
		} else {
		  (void)constant_subexpr (messages, i, &close_paren, result);
		  i = close_paren;
		  *end += (close_paren - tclose_paren);
		}
		break;
	      case CLOSEPAREN:  /* Matching parens are tagged as evaled by the 
				   parser on recursion. */
		new_exception (mismatched_paren_x);
		warning (messages[i], "Mismatched parentheses.");
		goto done;
		break;
		/* Handle math operators */
	      case PLUS:
	      case MINUS:
	      case MULT:
	      case DIVIDE:
	      case ASL:
	      case ASR:
	      case BIT_AND:
	      case BIT_COMP:
	      case BIT_OR:
	      case BIT_XOR:
		eval_math (messages, i, result);
		if (parse_exception == invalid_operand_x) {
		  /*
		   *  Try to re-interpret the expression.
		   *  Note: "end" is already an int *.
		   */
		  if (!expr_reparse (messages, i, end, result)) {
		    parse_exception = success_x;
		  } else {
		    /*
		     *  Silently return false.
		     */
		    /*
		     *  TO DO - Here, also add optional
		     *  warning for undefined symbols.
		     */
		    int j;
		    for (j = start; j >= *end; j--)
		      ++messages[j] -> evaled;
		    result -> __type = INTEGER_T;
		    result -> __value.__i = False;
		    parse_exception = success_x;
		    --constant_expr_eval_lvl;
		    return is_val_true (result);
		  }
		}
		++m -> evaled;
		break;
		/* Handle operators that return boolean values. */
	      case BOOLEAN_EQ:
	      case GT:
	      case LT:
	      case GE:
	      case LE:
	      case LOG_NEG:
	      case INEQUALITY:
	      case BOOLEAN_AND:
	      case BOOLEAN_OR:
		/*
		 *  TO DO - 
		 *  If we have a warning for undefined operands
		 *  test for it here.  GNU cpp silently evaluates
		 *  them to False.
		 */
		eval_bool (messages, i, result);
		++m -> evaled;
		break;
	      case CONDITIONAL:
		/* This is the only ternary op that we need
		   to evaluate at the moment, so evaluate on 
		   its own. */
		question_conditional_eval (messages, i, 
					   &conditional_end_ptr, result);
		i = conditional_end_ptr;
		++m -> evaled;
		break;
	      case SIZEOF:
		handle_sizeof_op (messages, i, &sizeof_end_ptr, result);
		i = sizeof_end_ptr;
		break;
		/* Catch errors of operators that assign values. */
	      case INCREMENT:
	      case DECREMENT:
	      case EQ:
	      case ASR_ASSIGN:
	      case ASL_ASSIGN:
	      case PLUS_ASSIGN:
	      case MINUS_ASSIGN:
	      case MULT_ASSIGN:
	      case DIV_ASSIGN:
	      case BIT_AND_ASSIGN:
	      case BIT_OR_ASSIGN:
	      case BIT_XOR_ASSIGN:
		new_exception (assignment_in_constant_expr_x);
		parse_exception = parse_error_x;
		warning (messages[i], "Assignment in constant expression.");
		goto done;
		break;
	      default:
		++m -> evaled; /***/
		break;
	      }
	  }
	  break;
	}  /* switch (m -> tokentype) */

      if (handle_preprocess_exception (messages, i, start, *end))
	goto done;

  }

  /* TO DO -
   * Progressing to the next precedence relies too much on 
   * setting subexpressions to evaled token types.  Use the
   * eval message member, or find a way to step through 
   * precedence levels without changing the token type.
   */
  for (; precedence < 13; ++precedence) {
    for (i = start; i >= *end; i--) {
      if (messages[i] -> evaled == 0) {
	if (op_precedence (precedence, messages, i)) {
	  goto re_eval;
	}
      }
#if 0
      /***/
      if (IS_C_OP(M_TOK(messages[i])))
	if (op_precedence (precedence, messages, i)  &&
	    (messages[i] -> evaled == 0))
	  goto re_eval;
#endif      
    }
  }

 done:
  --constant_expr_eval_lvl;
  return is_val_true (result);
}

/*
 *  If we have an invalid math expression, try to fit it:
 *  1. To a label that contains non-standard characters,
 *     and expand the macro if necessary.
 */

extern DEFINITION *macro_symbols;  /* Defined macro list.            */
extern DEFINITION *last_symbol;    /* List pointer.                  */

int expr_reparse (MESSAGE_STACK messages, int op_ptr, int *end, VAL *result) {

  int i, op_res, op1_ptr, op2_ptr;
  char tsymbol[MAXLABEL];
  DEFINITION *d;

  /*
   *  FIXME!  Memset () should not be needed.  Glibc strcmp (),
   *  below, seems to be sensitive to the byte alignment of 
   *  tsymbol[], and tsymbol's alignment varies depending
   *  on the size of the input buffer(s) and stack message
   *  buffers.  In short, strcmp () otherwise doesn't always
   *  work correctly.
   */
  memset ((void *)tsymbol, 0, MAXLABEL);

  if ((op_res = operands (messages, op_ptr, &op1_ptr, &op2_ptr)) == ERROR)
    return ERROR;

  /*
   *  Collect the symbols and try to find a greedy match.
   */
  strcpy (tsymbol, messages[op1_ptr] -> name);

  for (i = op1_ptr - 1; i >= op2_ptr; i--) {
    strcat (tsymbol, messages[i] -> name);

    for (d = macro_symbols; ; d = d -> next) {
      if (!strcmp (d -> name, tsymbol)) {
	/*
	 *  This has yet to occur.
	 */
      } else {
	if (!strcmp (d -> value, tsymbol)) {
	  /*
	   *  TO DO - Also check here for valid expressions
	   *  and macro expansions.  The only exception
	   *  handled here so far is invalid_operand_x,
	   *  we can be sure for the moment that we're
	   *  here because we couldn't evaluate the 
	   *  expression, so we can simply concatenate
	   *  into a label.
	   */
	  splice_stack (messages, op1_ptr, i);
	  *end += (op1_ptr - i);
	  free (messages[op1_ptr] -> name);
	  messages[op1_ptr] -> name = strdup (tsymbol);
	  free (messages[op1_ptr] -> value);
	  messages[op1_ptr] -> value = strdup (tsymbol);
	  messages[op1_ptr] -> tokentype = LABEL;
	  messages[op1_ptr] -> evaled = messages[op1_ptr] -> output = 0;
	  return SUCCESS;
	}
      }
      if (d == last_symbol) break;
    }
  }

  parse_exception = parse_error_x;
  return ERROR;
  
}

