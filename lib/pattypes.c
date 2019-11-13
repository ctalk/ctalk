 /* $Id: pattypes.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

 /*
   This file is part of Ctalk.
   Copyright © 2005-2019  Robert Kiesling, rk3314042@gmail.com.
   Permission is granted to copy this software provided that this copyright
   notice is included in all source code modules.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, 
   Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 #include <errno.h>
 #include "lex.h"
 #include "object.h"
 #include "message.h"
 #include "ctalk.h"
 #include "typeof.h"
 #include "rtinfo.h"

/*
  *  Complete expression is within arrayopen.. arrayclose pair.
  */
 int is_simple_subscript (MESSAGE_STACK messages, int expr_start_idx,
			  int stack_start_idx, int stack_end_idx) {
   int prev_tok_idx, next_tok_idx, expr_end_idx;
   char *exprbuf;
   MSINFO ms;
   

   if ((prev_tok_idx = __ctalkPrevLangMsg (messages, expr_start_idx, 
				       stack_start_idx)) == ERROR)
     return FALSE;

   if (M_TOK(messages[prev_tok_idx]) != ARRAYOPEN) return FALSE;

   ms.messages = messages;
   ms.stack_start = stack_start_idx;
   ms.stack_ptr = stack_end_idx;
   ms.tok = expr_start_idx;
   exprbuf = collect_expression (&ms, &expr_end_idx);
   __xfree (MEMADDR(exprbuf));

   if ((next_tok_idx = __ctalkNextLangMsg (messages, expr_end_idx, 
				       stack_end_idx)) == ERROR)
     return FALSE;

   if (M_TOK(messages[next_tok_idx]) != ARRAYCLOSE) return FALSE;

   return TRUE;
 }

/* this could be replaced by is_fmt_arg_2 sometime */

bool is_fmt_arg (MESSAGE_STACK messages, int expr_start_idx,
		 int stack_start_idx, int stack_end_idx) {
   int i, n_th_arg;
   int paren_lvl, block_lvl, array_lvl, start_paren_lvl, start_array_lvl;
   MESSAGE *m;
   int fmt_str_length;
   char *fmt_char_ptr;
   enum {
     ptr_fmt_arg_null,
     ptr_fmt_arg_expr,
     ptr_fmt_arg_separator,
     ptr_fmt_arg_literal
   } state;

   paren_lvl = block_lvl = array_lvl = n_th_arg = 0;
   start_paren_lvl = start_array_lvl = -1;
   for (i = expr_start_idx, state = ptr_fmt_arg_null; i <= stack_start_idx; i++) {

     m = messages[i];
     if (M_ISSPACE(m)) continue;

     switch (M_TOK(m))
       {
       case LABEL:
	 if ((state == ptr_fmt_arg_null) || (state == ptr_fmt_arg_separator))
	   state = ptr_fmt_arg_expr;
	 break; 
       case ARGSEPARATOR:
	 if (start_paren_lvl != -1) {
	   if (!block_lvl && !array_lvl) {
	     state = ptr_fmt_arg_separator;
	     --n_th_arg;
	   }
	 } else {
	   if (!paren_lvl && !block_lvl && !array_lvl) {
	     state = ptr_fmt_arg_separator;
	     --n_th_arg;
	   } else if (paren_lvl && state == ptr_fmt_arg_expr) {
	     state = ptr_fmt_arg_separator;
	     --n_th_arg;
	   }
	 }
	 break;
       case LITERAL:
	 if (state == ptr_fmt_arg_separator) {
	   if ((fmt_str_length = is_printf_fmt (M_NAME(m))) != 0) {
	     return TRUE;
	   }
	 } else {
	   state = ptr_fmt_arg_expr;
	 }
	 break;
       case OPENPAREN:
	 ++paren_lvl;
	 if (state == ptr_fmt_arg_null) {
	   start_paren_lvl = paren_lvl;
	   state = ptr_fmt_arg_expr;
	 } else {
	   int __prev_idx;
	   if ((__prev_idx = __ctalkPrevLangMsg (messages, i, stack_start_idx)) 
	       != ERROR) {
	     /*
	      *  Function call syntax.
	      */
	     if (M_TOK(messages[__prev_idx]) == LABEL)
	       return FALSE;
	   }
	 }
	 break;
       case CLOSEPAREN:
	 --paren_lvl;
	 if (state == ptr_fmt_arg_separator)
	   state = ptr_fmt_arg_expr;
	 break;
       case OPENBLOCK:
	 ++block_lvl;
	 break;
       case CLOSEBLOCK:
	 --block_lvl;
	 break;
       case ARRAYOPEN:
	 ++array_lvl;
	 if (state == ptr_fmt_arg_null)
	   start_array_lvl = array_lvl;
	 break;
       case ARRAYCLOSE:
	 --array_lvl;
	 break;
       case SEMICOLON:
	 return FALSE;
	 break;
       default:
	 break;
       } /* switch (M_TOK(m)) */
   }
   return TRUE;
 }

bool is_fmt_arg_2 (MSINFO *ms) {
   int i, n_th_arg;
   int paren_lvl, block_lvl, array_lvl, start_paren_lvl, start_array_lvl;
   MESSAGE *m;
   int fmt_str_length;
   char *fmt_char_ptr;
   enum {
     ptr_fmt_arg_null,
     ptr_fmt_arg_expr,
     ptr_fmt_arg_separator,
     ptr_fmt_arg_literal
   } state;

   paren_lvl = block_lvl = array_lvl = n_th_arg = 0;
   start_paren_lvl = start_array_lvl = -1;
   for (i = ms -> tok, state = ptr_fmt_arg_null; i <= ms -> stack_start; i++) {

     m = ms -> messages[i];
     if (M_ISSPACE(m)) continue;

     switch (M_TOK(m))
       {
       case LABEL:
	 if ((state == ptr_fmt_arg_null) || (state == ptr_fmt_arg_separator))
	   state = ptr_fmt_arg_expr;
	 break; 
       case ARGSEPARATOR:
	 if (start_paren_lvl != -1) {
	   if (!block_lvl && !array_lvl) {
	     state = ptr_fmt_arg_separator;
	     --n_th_arg;
	   }
	 } else {
	   if (!paren_lvl && !block_lvl && !array_lvl) {
	     state = ptr_fmt_arg_separator;
	     --n_th_arg;
	   } else if (paren_lvl && state == ptr_fmt_arg_expr) {
	     state = ptr_fmt_arg_separator;
	     --n_th_arg;
	   }
	 }
	 break;
       case LITERAL:
	 if (state == ptr_fmt_arg_separator) {
	   if ((fmt_str_length = is_printf_fmt (M_NAME(m))) != 0) {
	     return TRUE;
	   }
	 } else {
	   state = ptr_fmt_arg_expr;
	 }
	 break;
       case OPENPAREN:
	 ++paren_lvl;
	 if (state == ptr_fmt_arg_null) {
	   start_paren_lvl = paren_lvl;
	   state = ptr_fmt_arg_expr;
	 } else {
	   int __prev_idx;
	   if ((__prev_idx = __ctalkPrevLangMsg (ms -> messages, i, ms -> stack_start)) 
	       != ERROR) {
	     /*
	      *  Function call syntax.
	      */
	     if (M_TOK(ms -> messages[__prev_idx]) == LABEL)
	       return FALSE;
	   }
	 }
	 break;
       case CLOSEPAREN:
	 --paren_lvl;
	 if (state == ptr_fmt_arg_separator)
	   state = ptr_fmt_arg_expr;
	 break;
       case OPENBLOCK:
	 ++block_lvl;
	 break;
       case CLOSEBLOCK:
	 --block_lvl;
	 break;
       case ARRAYOPEN:
	 ++array_lvl;
	 if (state == ptr_fmt_arg_null)
	   start_array_lvl = array_lvl;
	 break;
       case ARRAYCLOSE:
	 --array_lvl;
	 break;
       case SEMICOLON:
	 return FALSE;
	 break;
       default:
	 break;
       } /* switch (M_TOK(m)) */
   }
   return TRUE;
 }

int is_printf_fmt_msg (MESSAGE_STACK messages, int fmt_tok_idx,
		       int stack_start_idx, int stack_end_idx) {
  return is_printf_fmt (M_NAME(messages[fmt_tok_idx]));
}

int is_function_call (MESSAGE_STACK messages, int fn_label_idx,
		      int stack_start_idx, int stack_end_idx) {
  int open_paren_idx, close_paren_idx;
  if ((open_paren_idx = __ctalkNextLangMsg (messages, fn_label_idx,
					    stack_end_idx)) == ERROR)
    return FALSE;
  if (M_TOK(messages[open_paren_idx]) != OPENPAREN)
    return FALSE;
  if ((close_paren_idx = __ctalkMatchParen (messages, 
					    open_paren_idx,
					    stack_end_idx)) == ERROR)
    return FALSE;
  return TRUE;
}

int is_struct_or_union_expr (MESSAGE_STACK messages, 
			     int expr_start_label_idx,
			     int stack_start_idx,
			     int stack_end_idx) {
  int next_tok_idx, close_paren_idx;
  if ((next_tok_idx = __ctalkNextLangMsg(messages,
					 expr_start_label_idx,
					 stack_end_idx)) != ERROR) {
    if ((M_TOK(messages[next_tok_idx]) == PERIOD) ||
	(M_TOK(messages[next_tok_idx]) == DEREF)) {
      return TRUE;
    }
    /*
     *  Also match <fn> () -> or <fn> () .
     */
    if (M_TOK(messages[next_tok_idx]) == OPENPAREN) {
      if ((close_paren_idx = __ctalkMatchParen (messages,
						next_tok_idx,
						stack_end_idx)) != ERROR) {
	if ((next_tok_idx = __ctalkNextLangMsg(messages,
					       close_paren_idx,
					       stack_end_idx)) != ERROR) {
	  if ((M_TOK(messages[next_tok_idx]) == PERIOD) ||
	      (M_TOK(messages[next_tok_idx]) == DEREF)) {
	    return TRUE;
	  }
	}
      }
    }
  }
  return FALSE;
}

/*
 *  Returns stack index of closing bracket, 0 if no subscript,
 *  -1 if a bracket mismatch.
 */
int is_subscript_expr (MESSAGE_STACK messages, 
		       int expr_start_label_idx,
		       int stack_end_idx) {
  int next_tok_idx, i, n_brackets;
  if ((next_tok_idx = __ctalkNextLangMsg(messages,
					 expr_start_label_idx,
					 stack_end_idx)) != ERROR) {
    if (M_TOK(messages[next_tok_idx]) == ARRAYOPEN) {
      n_brackets = 1;
      for (i = next_tok_idx; i > stack_end_idx; i--) {
	if (M_TOK(messages[i]) == ARRAYCLOSE) {
	  if (--n_brackets == 0) {
	    return i;
	  }
	}
      }
      return ERROR;
    }
  }
  return FALSE;
}

int match_array_brace (MESSAGE_STACK messages, int open_brace_idx,
		       int stack_top_idx) {
  int i;
  int n_levels;

  n_levels = 0;
  for (i = open_brace_idx; i > stack_top_idx; i--) {
    if (M_TOK (messages[i]) == ARRAYOPEN)
      ++n_levels;
    if (M_TOK (messages[i]) ==  ARRAYCLOSE)
      --n_levels;
    if (n_levels == 0)
      return i;
  }
  return -1;
}

int match_array_brace_rev (MESSAGE_STACK messages, int close_brace_idx,
			   int stack_start_idx, int *n_subscripts_out) {
  int i;
  int n_levels;
  int prev_tok_idx;

  n_levels = 0;
  *n_subscripts_out = 0;
  for (i = close_brace_idx; i <= stack_start_idx; ++i) {
    if (M_TOK(messages[i]) == ARRAYCLOSE)
      ++n_levels;
    if (M_TOK(messages[i]) == ARRAYOPEN)
      --n_levels;
    if (n_levels == 0) {
      ++(*n_subscripts_out);
      if ((prev_tok_idx = __ctalkPrevLangMsg (messages, i,
					      stack_start_idx))
	  !=  ERROR) {
	if (M_TOK(messages[prev_tok_idx]) != ARRAYCLOSE) {
	  return i;
	}
      }
    }
  }
  return -1;
}

/*
 *  Returns stack index of terminal token, 0 if no struct or deref 
 *  operator, -1 on error.  Also checks for subscripted members.
 */
int struct_end (MESSAGE_STACK messages, 
		       int expr_start_label_idx,
		       int stack_end_idx) {
  int next_tok_idx, next_tok_idx_2, next_tok_idx_3,
    i, n_derefs;
  n_derefs = 0;
  i = expr_start_label_idx;
  while ((next_tok_idx = __ctalkNextLangMsg(messages,
					    i,
					    stack_end_idx)) != ERROR) {
    if ((M_TOK(messages[next_tok_idx]) == PERIOD) ||
	(M_TOK(messages[next_tok_idx]) == DEREF)) {
      ++n_derefs;
      i = next_tok_idx;
      continue;
    }

    if (M_TOK(messages[next_tok_idx]) == ARRAYOPEN) {
      i = match_array_brace (messages, next_tok_idx, stack_end_idx);
      continue;
    }

    if (M_TOK(messages[next_tok_idx]) == LABEL) {
      if ((next_tok_idx_2 = 
	   __ctalkNextLangMsg(messages,
			      next_tok_idx,
			      stack_end_idx)) != ERROR) {

	/* If the terminal member is an array element, return
	   its label. */
	if (M_TOK(messages[next_tok_idx_2]) == ARRAYOPEN) {
	  i = match_array_brace (messages, next_tok_idx_2, stack_end_idx);
	  if ((next_tok_idx_3 = __ctalkNextLangMsg (messages,
						    i, stack_end_idx))
	      != ERROR) {
	    
	    if ((M_TOK(messages[next_tok_idx_3])  == PERIOD) ||
		(M_TOK(messages[next_tok_idx_3]) == DEREF)) {
	      continue; /* i is already where we need it. */
	    }
	  }
	}

	if ((M_TOK(messages[next_tok_idx_2]) == PERIOD) ||
	    (M_TOK(messages[next_tok_idx_2]) == DEREF)) {
	  i = next_tok_idx;
	  continue;
	} else {
	  if (n_derefs) {
	    return next_tok_idx;
	  } else {
	    return 0;
	  }
	}
      } else {
	if (n_derefs) {
	  return next_tok_idx;
	} else {
	  return 0;
	}
      }
    }
    if (M_TOK(messages[next_tok_idx]) ==  ARRAYOPEN) {
      i = match_array_brace (messages, next_tok_idx, stack_end_idx);
      continue;
    }
    /* Anything else. */
    if (n_derefs > 0)
      return next_tok_idx;
    else
      return FALSE;
  }
  return FALSE;
}

/*
 *  Returns the index of the argument, or -1;
 *
 *  Args except for printf format args.
 */
int obj_expr_is_arg (MESSAGE_STACK messages, int expr_start_idx,
		     int stack_start_idx, int *fn_label_idx) {
  int arg_idx = -1;
  int i;
  int prev_tok;
  int n_parens = 0;

  for (i = expr_start_idx; i <= stack_start_idx; i++) {
    switch (M_TOK(messages[i]))
      {
      case ARGSEPARATOR:
	++arg_idx;
	break;
      case OPENPAREN:
	--n_parens;
	if (n_parens < 0) {
	  ++arg_idx;
	  if ((*fn_label_idx = 
 	       __ctalkPrevLangMsg (messages, i, stack_start_idx)) != ERROR) {
	    if ((M_TOK(messages[*fn_label_idx]) == LABEL) &&
		!is_ctrl_keyword (M_NAME(messages[*fn_label_idx])))
	      return arg_idx;

	    /* Just open parens starting an argument, not the arglist -
	       reset. */
	    if ((M_TOK(messages[*fn_label_idx]) == ARGSEPARATOR) ||
		(M_TOK(messages[*fn_label_idx]) == OPENPAREN)) {
	      --arg_idx;
	      n_parens = 0;
	    } else if ((prev_tok = __ctalkPrevLangMsg
		      (messages, i, stack_start_idx)) != ERROR) {
	      /* Handle a series of opening parens and/or whitespace
		 between the first argument and the function label. */
	      switch (M_TOK(messages[prev_tok]))
		{
		case OPENPAREN:
		  /* series of open parens and/or spaces 
		     after the function label. */
		  while ((M_TOK(messages[prev_tok]) == OPENPAREN) ||
			 M_ISSPACE(messages[prev_tok])) {
		    ++prev_tok;
		  }
		  if ((M_TOK(messages[prev_tok]) == LABEL) &&
		      !is_ctrl_keyword (M_NAME(messages[prev_tok]))) {
		    *fn_label_idx = prev_tok;
		    return arg_idx;
		  } else if (M_TOK(messages[prev_tok]) == ARGSEPARATOR) {
		    ++arg_idx;
		    i = prev_tok;
		    n_parens = 0;
		  }
		  break;
		case CLOSEPAREN:
		  /* A typecast after the argument list's
		     beginning. */
		  prev_tok = __ctalkMatchParenRev
		    (messages, prev_tok, stack_start_idx);
		  ++prev_tok;
		  while ((M_TOK(messages[prev_tok]) == OPENPAREN) ||
			 M_ISSPACE(messages[prev_tok])) {
		    ++prev_tok;
		  }
		  if ((M_TOK(messages[prev_tok]) == LABEL) &&
		      !is_ctrl_keyword (M_NAME(messages[prev_tok]))) {
		    *fn_label_idx = prev_tok;
		    return arg_idx;
		  } else if (M_TOK(messages[prev_tok]) == ARGSEPARATOR) {
		    ++arg_idx;
		    i = prev_tok;
		    n_parens = 0;
		  }
		default:
		  /* something else... just reset */
		  --arg_idx;
		  n_parens = 0;
		  break;
		}
	    }
	  }
	}
	break;
      case CLOSEPAREN:
	++n_parens;
	break;
      case SEMICOLON:
	return ERROR;
	break;
      case OPENBLOCK:
	return ERROR;
	break;
      case LITERAL:
	if (is_printf_fmt (M_NAME(messages[i])))
	  return ERROR;
	break;
      }
  }
  return ERROR;
}

/*
 *  Returns the index of the argument, or -1;
 *
 *  Args except for printf format args.
 *
 *  ms -> tok points to the expr_start_idx.
 */
int obj_expr_is_arg_ms (MSINFO *ms, int *fn_label_idx) {
  int arg_idx = -1;
  int i;
  int prev_tok;
  int n_parens = 0;

  for (i = ms -> tok; i <= ms -> stack_start; i++) {
    switch (M_TOK(ms -> messages[i]))
      {
      case ARGSEPARATOR:
	++arg_idx;
	break;
      case OPENPAREN:
	--n_parens;
	if (n_parens < 0) {
	  ++arg_idx;
	  if ((*fn_label_idx = 
 	       __ctalkPrevLangMsg (ms -> messages, i, ms -> stack_start)) != ERROR) {
	    if ((M_TOK(ms -> messages[*fn_label_idx]) == LABEL) &&
		!is_ctrl_keyword (M_NAME(ms -> messages[*fn_label_idx])))
	      return arg_idx;

	    /* Just open parens starting an argument, not the arglist -
	       reset. */
	    if ((M_TOK(ms -> messages[*fn_label_idx]) == ARGSEPARATOR) ||
		(M_TOK(ms -> messages[*fn_label_idx]) == OPENPAREN)) {
	      --arg_idx;
	      n_parens = 0;
	    } else if ((prev_tok = __ctalkPrevLangMsg
		      (ms -> messages, i, ms -> stack_start)) != ERROR) {
	      /* Handle a series of opening parens and/or whitespace
		 between the first argument and the function label. */
	      switch (M_TOK(ms -> messages[prev_tok]))
		{
		case OPENPAREN:
		  /* series of open parens and/or spaces 
		     after the function label. */
		  while ((M_TOK(ms -> messages[prev_tok]) == OPENPAREN) ||
			 M_ISSPACE(ms -> messages[prev_tok])) {
		    ++prev_tok;
		  }
		  if ((M_TOK(ms -> messages[prev_tok]) == LABEL) &&
		      !is_ctrl_keyword (M_NAME(ms -> messages[prev_tok]))) {
		    *fn_label_idx = prev_tok;
		    return arg_idx;
		  } else if (M_TOK(ms -> messages[prev_tok]) == ARGSEPARATOR) {
		    ++arg_idx;
		    i = prev_tok;
		    n_parens = 0;
		  }
		  break;
		case CLOSEPAREN:
		  /* A typecast after the argument list's
		     beginning. */
		  prev_tok = __ctalkMatchParenRev
		    (ms -> messages, prev_tok, ms -> stack_start);
		  ++prev_tok;
		  while ((M_TOK(ms -> messages[prev_tok]) == OPENPAREN) ||
			 M_ISSPACE(ms -> messages[prev_tok])) {
		    ++prev_tok;
		  }
		  if ((M_TOK(ms -> messages[prev_tok]) == LABEL) &&
		      !is_ctrl_keyword (M_NAME(ms -> messages[prev_tok]))) {
		    *fn_label_idx = prev_tok;
		    return arg_idx;
		  } else if (M_TOK(ms -> messages[prev_tok]) == ARGSEPARATOR) {
		    ++arg_idx;
		    i = prev_tok;
		    n_parens = 0;
		  }
		default:
		  /* something else... just reset */
		  --arg_idx;
		  n_parens = 0;
		  break;
		}
	    }
	  }
	}
	break;
      case CLOSEPAREN:
	++n_parens;
	break;
      case SEMICOLON:
	return ERROR;
	break;
      case OPENBLOCK:
	return ERROR;
	break;
      case LITERAL:
	if (is_printf_fmt (M_NAME(ms -> messages[i])))
	  return ERROR;
	break;
      }
  }
  return ERROR;
}


int rcvr_is_start_of_expr (MESSAGE_STACK messages, int idx, int stack_start) {
  int i;
  for (i = idx; i <= stack_start; i++) {
    if (M_ISSPACE(messages[i]))
      continue;
    if ((M_TOK(messages[i]) == SEMICOLON) ||
	(M_TOK(messages[i]) == COLON) ||
	(M_TOK(messages[i]) == OPENBLOCK) ||
	(M_TOK(messages[i]) == ARGSEPARATOR) ||
	(M_TOK(messages[i]) == ARRAYOPEN))
      return TRUE;
    if ((M_TOK(messages[i]) != LABEL) &&
	(M_TOK(messages[i]) != METHODMSGLABEL))
      return FALSE;
  }
  return TRUE;
}

int have_label_terminator (MESSAGE_STACK messages, int start, int end) {
  int i;
  for (i = start; i <= end; i++) {
    if (M_TOK(messages[i]) == COLON)
      return i;
  }
  return -1;
}


/*
 *  Only recognzes a "(...) <label>" construct from within the parentheses,
 *  checks for C keywords also; i.e., control structure keywords like 
 *  "break," "return," and, "goto," "if," "while," and so on and on, are 
 *  valid also.
 *
 *  Also check for an opening parenthesis preceded by a control 
 *  keyword, which is a C construct; e.g., 
 *
 *    <ctrl keyword> (...) <label>
 *
 *  Returns the stack index of the closing paren on the subexpression if
 *  true, 0 if false.
 *
 */
int is_in_rcvr_subexpr (MESSAGE_STACK messages, int idx, 
			int stack_start, int stack_end) {
  int i, lasttok = -1, lasttok_idx = -1, /* Avoid warnings */
    n_parens;
  MESSAGE *m;
  n_parens = 0;

  if (messages[idx] -> attrs & RT_TOK_IS_TYPECAST_EXPR)
    return FALSE;

  for (i = idx; i > stack_end; i--) {
    m = messages[i];
    if (M_ISSPACE(m))
      continue;
    switch (M_TOK(m))
      {
      case OPENPAREN:
	if ((lasttok == CLOSEPAREN) && 
	    (n_parens < 0)) {
	  return FALSE;
	} else {
	  n_parens += 1;
	}
	break;
      case CLOSEPAREN:
	n_parens -= 1;
	break;
      case SEMICOLON:
	/* TODO - SEMICOLON here should always return False -- 
	   needs testing, though. */
	if ((lasttok == CLOSEPAREN) && 
	    (n_parens < 0)) {
	  return FALSE;
	}
	if (IS_CONSTANT_TOK(lasttok))
	  return FALSE;
	if ((lasttok == LABEL) &&
	    (n_parens == 0))
	  return FALSE;
	if ((lasttok == ARRAYCLOSE) && (n_parens == 0))
	  return FALSE;
	break;
      case CLOSEBLOCK:
      case OPENBLOCK:
      case ARGSEPARATOR:
      case ARRAYOPEN:
      case ARRAYCLOSE:
	if ((lasttok == CLOSEPAREN) && 
	    (n_parens < 0)) {
	  return FALSE;
	}
	break;
      case INCREMENT:
      case DECREMENT:
      case LABEL:
	if ((lasttok == CLOSEPAREN)  &&
	    (n_parens < 0)) {
	  if (!is_c_keyword (M_NAME(m))) {
	    /*
	     *  Check if the opening paren is preceded by
	     *  a control keyword, which is a C construct; e.g.
	     *
	     *   <ctrl_keyword> (....) <message m>
	     */
	    int i_2, n_parens_2;
	    n_parens_2 = 0;
	    for (i_2 = lasttok_idx; i_2 <= stack_start; i_2++) {
	      if (M_ISSPACE(messages[i_2]))
		continue;
	      if (M_TOK(messages[i_2]) == CLOSEPAREN)
		--n_parens_2;
	      if (M_TOK(messages[i_2]) == OPENPAREN)
		++n_parens_2;
	      /*
	       *  Sanity check so we don't need to 
	       *  look at the entire input.
	       */
	      if ((M_TOK(messages[i_2]) == OPENBLOCK) ||
		  (M_TOK(messages[i_2]) == CLOSEBLOCK))
		  return FALSE;
	      if (n_parens_2 == 0) {
		int prev_tok_idx_2;
		if ((prev_tok_idx_2 = __ctalkPrevLangMsg 
		     (messages, i_2, stack_start)) != ERROR) {
		  if (is_ctrl_keyword (M_NAME(messages[prev_tok_idx_2]))) {
		    return FALSE;
		  } else {
		    MESSAGE *m_2;
		    int i_3;
		    /*
		     *  Check for a struct expression alone within the
		     *  parentheses.
		     */
		    for (i_3 = i_2; i_3 >= lasttok_idx; i_3--) {
		      m_2 = messages[i_3];
		      if (!M_ISSPACE(m_2)) {
			if (/*(m_2 -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR) ||*/
			    (M_TOK(m_2) == INCREMENT) ||
			    (M_TOK(m_2) == DECREMENT) ||
			    (M_TOK(m_2) == OPENPAREN) ||
			    (M_TOK(m_2) == CLOSEPAREN)) {
			  continue;
			} else {
			  return lasttok_idx;
			}
		      }
		    }
		    return FALSE;
		  }
		}
	      }
	    }
	    if (lasttok == SEMICOLON) {
	      if (is_c_keyword (M_NAME(m))) {
		return FALSE;
	      }
	    }
	  } else {
	    return FALSE;
	  }
	}
	break;
      }
    lasttok = M_TOK(m);
    lasttok_idx = i;
  }
  return FALSE;
}

int is_fn_style_label (MESSAGE_STACK messages, int idx, int stack_top) {
  int next_tok;
  if (M_TOK(messages[idx]) == LABEL) {
    if ((next_tok = __ctalkNextLangMsg (messages, idx, stack_top)) 
	!= ERROR) {
      if (M_TOK(messages[next_tok]) == OPENPAREN) {
	return TRUE;
      }
    }
  }
  return FALSE;
}

int is_first_ctrlblk_pred_tok (MESSAGE_STACK messages, int idx, int stack_start) {
  int i;
  for (i = idx+1; i <= stack_start; i++) {
    if (M_ISSPACE(messages[i]))
      continue;
    if (M_TOK(messages[i]) != OPENPAREN)
      break;
  }
  if (is_ctrl_keyword (M_NAME(messages[i])))
    return TRUE;
  return FALSE;
}

#define IS_LEADING_CTRLBLK_UNARY_MATH_OP(tok) \
   ((tok == INCREMENT) || \
    (tok == DECREMENT) || \
    (tok == BIT_COMP) ||  \
    (tok == LOG_NEG) ||	  \
    (tok == AMPERSAND) || \
    (tok == ASTERISK))


/*
 *  Recognizes <tok><label> or <tok><(+><label>
 *  Skips leading OPENPAREN tokens.  Used in method_call *only*.
 *  This is basically more convenient than scanback_other ().
 *  Returns the stack index of the non-OPENPAREN token before
 *  the label, -1 on error.
 */
int is_leading_prefix_op (MESSAGE_STACK messages, int label_idx, 
			  int stack_start) {
  int j;
  for (j = label_idx + 1; j <= stack_start; j++) {
    if (M_ISSPACE(messages[j]))
	continue;
    if (M_TOK(messages[j]) == OPENPAREN) {
      continue;
    } else {
      return j;
    }
  }
  return -1;
}

/*
 *  Recognizes <var_end_tok><postfix_tok> or 
 *  <var_end_tok><)+><postfix_tok>
 *  Skips trailing CLOSEPAREN tokens.  Used in method_call *only*.
 *  This is basically more convenient than scanforward_other ().
 *  Returns the stack index of the non-CLOSEPAREN token after the var
 *  end, -1 on error.
 */
int is_trailing_postfix_op (MESSAGE_STACK messages, int last_var_tok_idx, 
			    int stack_end) {
  int j;
  for (j = last_var_tok_idx - 1; j >= stack_end; j--) {
    if (!IS_MESSAGE(messages[j]))
      break;
    if (M_ISSPACE(messages[j]))
	continue;
    if (M_TOK(messages[j]) == CLOSEPAREN) {
      continue;
    } else {
      if (M_TOK(messages[j]) == INCREMENT ||
	  M_TOK(messages[j]) == DECREMENT) {
	return j;
      } else {
	return -1;
      }
    }
  }
  return -1;
}

int is_leading_ctrlblk_prefix_op_tok (MESSAGE_STACK messages, int idx, 
				      int stack_start) {
  int i;
  for (i = idx+1; i <= stack_start; i++) {
    if (M_ISSPACE(messages[i]))
      continue;
    if ((M_TOK(messages[i]) != OPENPAREN) && 
	(!IS_LEADING_CTRLBLK_UNARY_MATH_OP(M_TOK(messages[i]))))
      break;
  }
  if (is_ctrl_keyword (M_NAME(messages[i])))
    return TRUE;
  return FALSE;
}

int prefix_op_is_sizeof (MESSAGE_STACK messages, int idx, int stack_start_idx) {
  int i;
  int have_prefix_attr;
  MESSAGE *m;
  enum {
    prefix_op_is_sizeof_null,
    prefix_op_is_sizeof_openparen/*  , */
  } state;

  state = prefix_op_is_sizeof_null;
  have_prefix_attr = FALSE;
  for (i = idx; i <= stack_start_idx; i++) {
    if (M_ISSPACE(messages[i]))
      continue;
    m = messages[i];
    if ((M_TOK(m) == SEMICOLON) ||
	(M_TOK(m) == OPENBLOCK))
      break;
    if (m -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
      if (!have_prefix_attr)
	have_prefix_attr = TRUE;
      switch (state)
	{
	case prefix_op_is_sizeof_null:
	  if (M_TOK(m) == OPENPAREN)
	    state = prefix_op_is_sizeof_openparen;
	  break;
	case prefix_op_is_sizeof_openparen:
	  if ((M_TOK(m) == LABEL) || 
	      (M_TOK(m) == SIZEOF)) {
	    if (str_eq (M_NAME(m), "sizeof"))
	      return i;
	  }
	  break;
	}
    } else {
      if (have_prefix_attr)
	return 0;
    }
  }
  return 0;
}

int method_contains_argblk (MESSAGE_STACK messages, int method_open_block,
			    int method_close_block) {
  int i, lookahead;
  int prev_idx = -1, prev_idx_2 = -1, prev_idx_3 = -1;

  for (i = method_open_block; i >= method_close_block; i--) {

      if (M_ISSPACE(messages[i]))
	continue;

    for (lookahead = i - 1; lookahead >= method_close_block; lookahead--) {

      if (M_ISSPACE(messages[lookahead]))
	continue;

      if ((M_TOK(messages[i]) == LABEL) && 
	  (M_TOK(messages[lookahead]) == OPENBLOCK)) {

	/* expressions like, "else {" */
	if (is_c_keyword (messages[i] -> name))
	  break;
	/* expressions like "struct {" or "struct <name> {" */
	if (prev_idx != -1) {
	  if (is_c_keyword (messages[prev_idx] -> name))
	    break;
	}
	if (prev_idx_3 != -1) {
	  if (is_c_keyword (messages[prev_idx_3] -> name))
	    break;
	}
	if (prev_idx_2 != -1) {
	  if (is_c_keyword (messages[prev_idx_2] -> name))
	    break;
	}
	if (prev_idx != -1) {
	  if (is_c_keyword (messages[prev_idx] -> name))
	    break;
	}

	return TRUE;
      } else {
	break;
      }

    }

    prev_idx_3 = prev_idx_2;
    prev_idx_2 = prev_idx;
    prev_idx = i;
  }

  return FALSE;
}

/*
 *  Returns True for the following cases so far:
 *
 *   <prefix_op_token> <op_token> <any_token>
 *   <prefix_op_token> <same_prefix_op_token> <any_token>
 *   <op_token><same_op_token><non_method_label>
 *   <method_label><op_token><any_token>
 *
 *  Returns False for the following cases so far:
 *   <non_method_label><op_token>
 *   <non_method_label><op_token><op_token>
 *
 *  If we have an operator between two labels, then it's a prefix op
 *  if the preceding label is a method message, which should normally
 *  already be evaluated, so it's easy to check.
 */
bool op_is_prefix_op (EXPR_PARSER *p, int op_ptr) {

  int prev_msg_ptr, next_msg_ptr;
  int next_msg_ptr_2, prev_msg_ptr_2;

  if ((prev_msg_ptr = __ctalkPrevLangMsg (p -> m_s, op_ptr,
					  p -> msg_frame_start)) != ERROR) {
    if (IS_C_OP_TOKEN_NOEVAL (M_TOK(p -> m_s[prev_msg_ptr]))) {
      if (M_TOK(p -> m_s[prev_msg_ptr]) == M_TOK(p -> m_s[op_ptr])) {
	if (!(p -> m_s[prev_msg_ptr] -> attrs & RT_TOK_IS_PREFIX_OPERATOR)) {
	  return false;
	}
      }
      return true;
    } else {

      if ((next_msg_ptr = __ctalkNextLangMsg (p -> m_s, op_ptr,
					      p -> msg_frame_top))
	  != ERROR) {
	if ((next_msg_ptr_2 = __ctalkNextLangMsg (p -> m_s, 
						  next_msg_ptr,
						  p -> msg_frame_top))
	    != ERROR) {
	  if ((IS_C_OP_TOKEN_NOEVAL 
	       (M_TOK(p -> m_s[next_msg_ptr])))  &&
	      (M_TOK(p -> m_s[op_ptr]) == 
	       M_TOK(p -> m_s[next_msg_ptr]))) {


	    /*
	     *  If the previous label is a method message, then
	     *  the operator attaches to the label following
	     *  operator or to a following op token/label sequence.
	     *
	     *  Check first for a preceding METHODMSGLABEL token, then
	     *  look for the method if the preceding label is
	     *  unevaluated.
	     */
	    if (M_TOK(p -> m_s[prev_msg_ptr]) == METHODMSGLABEL) {
	      if (M_TOK(p -> m_s[next_msg_ptr]) == LABEL)
		return true;
	      if (M_TOK(p -> m_s[next_msg_ptr_2]) == LABEL)
		return true;
	      return false;
	    }

	    if (M_TOK(p -> m_s[prev_msg_ptr]) == LABEL) {
	      if (__ctalk_isMethod_2 (M_NAME(p -> m_s[prev_msg_ptr]),
				    p -> m_s, prev_msg_ptr,
				    p -> msg_frame_start)) {
		if (M_TOK(p -> m_s[next_msg_ptr]) == LABEL)
		  return true;
		if (M_TOK(p -> m_s[next_msg_ptr_2]) == LABEL)
		  return true;
		return false;
	      }
	    }

	    if ((prev_msg_ptr_2 = __ctalkPrevLangMsg (p -> m_s,
						      prev_msg_ptr,
						      p -> msg_frame_start))
		!= ERROR) {
	      if (M_TOK(p -> m_s[prev_msg_ptr_2]) == LABEL) {
		if (!__ctalk_isMethod_2 (M_NAME(p -> m_s[prev_msg_ptr_2]),
				       p -> m_s, prev_msg_ptr_2,
				       p -> msg_frame_start)) {
		  return false;
		}
	      }
	    } else {
	      if ((M_TOK(p -> m_s[prev_msg_ptr]) == LABEL) &&
		  !__ctalk_isMethod_2 (M_NAME(p -> m_s[prev_msg_ptr]),
				     p -> m_s, prev_msg_ptr,
				     p -> msg_frame_start))
		return false;
	    }
	  }
	}
      }
    }
  } else if (op_ptr == p -> msg_frame_start) {
    /* if it's the start of the stack... */
    return true;
  } else {
    int i_2;
    /* ... or there's only whitespace before our token - */
    /* e.g., we've elided an 'eval' keyword or something. */
    for (i_2 = op_ptr + 1; i_2 <= p -> msg_frame_start; ++i_2) {
      if (!M_ISSPACE(p -> m_s[i_2]))
	return false;
    }
    return true;
  }
  return false;
}

/* Called by self_object () for printf ptr fmt args _only_. */
void find_self_unary_expr_limits (MESSAGE_STACK messages, int self_ptr,
				  int *expr_start, int *expr_end,
				  int stack_start, int stack_end) {
  int i, lookback, lookahead;
  *expr_start = -1;
  for (i = self_ptr; (i <= stack_start) && (*expr_start == -1); i++) {
    lookback = __ctalkPrevLangMsg (messages, i, stack_start);
    if (M_TOK(messages[lookback]) == ARGSEPARATOR) {
      *expr_start = i;
    }      
  }

  *expr_end = -1;
  for (i = self_ptr; (i >= stack_end) && (*expr_end == -1); i--) {
    lookahead = __ctalkNextLangMsg (messages, i, stack_end);
    if (METHOD_ARG_TERM_MSG_TYPE(messages[lookahead]))
      *expr_end = i;
  }
}

/* Called by eval_expr () _only_. */
void find_self_unary_expr_limits_2 (MESSAGE_STACK messages, int self_ptr,
				  int *expr_end, int stack_end) {
  int i, lookback, lookahead;

  *expr_end = -1;
  for (i = self_ptr; (i >= stack_end) && (*expr_end == -1); i--) {
    if ((lookahead = __ctalkNextLangMsg (messages, i, stack_end)) == -1) {
      *expr_end = i;
    } else {
      /* If we have any label after an op... this can certainly be extended.*/
      if ((M_TOK(messages[self_ptr]) != LABEL) &&
	  (M_TOK(messages[lookahead]) == LABEL) &&
	  !str_eq (M_NAME(messages[lookahead]), "self")) {
	*expr_end = lookahead;
	continue;
      }
      if (!(messages[lookahead] -> attrs & RT_OBJ_IS_INSTANCE_VAR) &&
	  !(messages[lookahead] -> attrs & RT_OBJ_IS_CLASS_VAR)) {
	*expr_end = i;
      }
    }
  }
}

int open_paren_is_arglist_start (MESSAGE_STACK messages, int paren_idx,
				 int stack_start) {
  int lookback;
  if ((lookback = __ctalkPrevLangMsg (messages, paren_idx, stack_start))
      != ERROR) {
    if (M_TOK(messages[lookback]) == LABEL)
      return TRUE;
  }
  return FALSE;
}

int find_leading_tok_idx (MESSAGE_STACK messages, int start_idx,
			  int stack_start_idx, int stack_top_idx) {
  int i, lookback, lookback2;
  
  for (i  = start_idx + 1; i <= stack_start_idx; i++) {
    if (M_ISSPACE (messages[i]))
      continue;
    lookback = __ctalkPrevLangMsg (messages, i, stack_start_idx);
    switch (M_TOK (messages[i]))
      {
      case INCREMENT:
      case DECREMENT:
      case BIT_COMP:
      case LOG_NEG:
      case AMPERSAND:
      case ASTERISK:
      case SIZEOF:
	if (messages[i] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
	  if (lookback == ERROR)
	    return i;
	  if (M_TOK(messages[lookback]) != M_TOK(messages[i])) {
	    if (M_TOK(messages[lookback]) == OPENPAREN) {
	      if (open_paren_is_arglist_start (messages, lookback, 
					       stack_start_idx)) {
		return i;
	      } else {
		lookback2 = __ctalkPrevLangMsg (messages, lookback, 
						stack_start_idx);
		/* E.g., the open paren follows a type cast. */
		if (M_TOK(messages[lookback2]) == CLOSEPAREN) {
		  /* If there's more to the term after the closing
		     paren, then the opening paren (which lookback
		     is pointing to) is included.  */
		  int match_open_paren, tok_after_paren;
		  if ((match_open_paren =
		       __ctalkMatchParen (messages,
					  lookback,
					  stack_top_idx)) != ERROR) {
		    if ((tok_after_paren =
			 __ctalkNextLangMsg (messages, match_open_paren,
					     stack_top_idx)) != ERROR) {
		      if (M_TOK(messages[tok_after_paren]) ==
			  LABEL) {
			return lookback;
		      }
		    }
		  }
		  return i;
		}
	      }
	    } else {
	      return i;
	    }
	  }
	} else {
	  return start_idx;
	} 
	break;
      case OPENPAREN:
	if (lookback == ERROR)
	  return i;
	if (M_TOK(messages[lookback]) == ARGSEPARATOR ||
	    M_TOK(messages[lookback]) == SEMICOLON ||
	    M_TOK(messages[lookback]) == CLOSEBLOCK ||
	    M_TOK(messages[lookback]) == OPENBLOCK ||
	    M_TOK(messages[lookback]) == ARRAYOPEN ||
	    M_TOK(messages[lookback]) == ARRAYCLOSE)
	  return i;
	if (M_TOK(messages[lookback]) == OPENPAREN) {
	  if (open_paren_is_arglist_start (messages, lookback, 
					   stack_start_idx)) {
	    return i;
	  }
	}
	break;
      default:
	return start_idx;
      }
  }
  return start_idx;
}

/*
 *  Returns true for a simple case like self -> <object_member> ...
 *  or (self) -> <object_member> ...
 *  we can simply use __ctalk_self_internal () later on.
 *
 *  Returns false self has a unary prefix op, however, because
 *  then we can't simply substitute __ctalk_self_internal () for the
 *  self token.
 *
 *  Called from need_rt_eval ().
 */
int self_used_in_simple_object_deref (MESSAGE_STACK messages, 
				      int self_tok_idx, 
				      int stack_start_idx,
				      int stack_end_idx) {

  int lookahead;
  int last_close_paren_idx, first_open_paren_idx;
  int prefix_tok_idx;


  if ((lookahead = __ctalkNextLangMsg (messages, self_tok_idx, stack_end_idx))
      != ERROR) {
    if ((M_TOK(messages[lookahead]) == PERIOD) ||
	(M_TOK(messages[lookahead]) == DEREF)) {
      return TRUE;
    }
  } else {
    return FALSE;
  }

  if (M_TOK(messages[lookahead]) == CLOSEPAREN) {
    last_close_paren_idx = lookahead;

    while ((lookahead = __ctalkNextLangMsg (messages, lookahead, stack_end_idx))
	   != ERROR) {
      if (M_TOK(messages[lookahead]) == CLOSEPAREN) {
	last_close_paren_idx = lookahead;
	continue;
      }
      if ((M_TOK(messages[lookahead]) == PERIOD) ||
	  (M_TOK(messages[lookahead]) == DEREF)) {

	/* Check for a leading unary, which makes the expression need
	   a call to __ctalkEvalExpr ().
	*/

	if ((first_open_paren_idx = 
	     __ctalkMatchParenRev (messages, 
				   last_close_paren_idx,
				   stack_start_idx))
	    != ERROR) {
	  if ((prefix_tok_idx = __ctalkPrevLangMsg 
	       (messages, first_open_paren_idx, stack_start_idx))
	      != ERROR) {
	    if (IS_C_UNARY_MATH_OP(M_TOK(messages[prefix_tok_idx]))) {
	      return FALSE;
	    } else {
	      return TRUE;
	    }
	  } else {
	    return TRUE;
	  }
	} else {
	  return FALSE;
	}
      } else {
	return FALSE;
      }
    }
  }  
  return FALSE;
}

bool tok_precedes_assignment_op (EXPR_PARSER *p, int tok_idx) {
  int i;
  MESSAGE *m;

  for (i = tok_idx - 1; i > p -> msg_frame_top; i--) {
    if (M_ISSPACE(p -> m_s[i]))
      continue;
    m = p -> m_s[i];

    if ((M_TOK(m) == OPENPAREN) ||
	(M_TOK(m) == CLOSEPAREN) ||
	(M_TOK(m) == OPENBLOCK) || 
	(M_TOK(m) == CLOSEBLOCK) ||
	(M_TOK(m) == ARRAYOPEN) ||
	(M_TOK(m) == ARRAYCLOSE) ||

	(M_TOK(m) == PERIOD) ||
	(M_TOK(m) == DEREF))
      continue;

    if (IS_C_ASSIGNMENT_OP(M_TOK(m)))
      return true;
  }
  return false;
}

/* Occurs when we want to keep the value of a prefix expression when
   its used as a term; e.g., 

     str = terma + *termb + termc;
   
   and the result of *termb needs to be the receiver of the second
   +, instead of being cleared after being the argument of the
   first +.

   Only needs to be return the operand's index if there's another
   operator following the term.
   
*/

int prefix_value_obj_is_term_result (EXPR_PARSER *p, int prefix_op_idx) {
  int next_idx, next_idx_2;

  if (p -> m_s[prefix_op_idx] -> attrs & RT_TOK_IS_PREFIX_OPERATOR) {
    if ((next_idx = __ctalkNextLangMsg (p -> m_s, prefix_op_idx,
					p -> msg_frame_top))
	!= ERROR) {
      if ((next_idx_2 = __ctalkNextLangMsg (p -> m_s, next_idx,
					    p -> msg_frame_top))
	  != ERROR) {
	if (METHOD_ARG_TERM_MSG_TYPE(p -> m_s[next_idx_2])) {
	  return 0;
	} else {
	  return next_idx;
	}
      }
    }
  }

  return 0;
}

/* Checks for an assignment operator even if the message's
   token type is METHODMSGLABEL. */
bool is_c_assignment_op_label (MESSAGE *m) {
  if (IS_C_ASSIGNMENT_OP (M_TOK(m)))
    return true;
  if (m -> name[0] == '=' || m -> name[1] == '=' || m -> name[2] == '=')
    return true;
  return false;
}

/* Finds the opening brace for a block's closing brace.  Returns -1 if
   the matching open block isn't found. 
   This isn't used right now .. and it's slow. 
*/
int match_block_open_brace (MESSAGE_STACK messages, int close_block_idx,
			   int stack_start_idx) {
  int i;
  int n_blocks;

  for (i = close_block_idx, n_blocks = 0; i <= stack_start_idx;
       ++i) {
    if (M_TOK(messages[i]) == CLOSEBLOCK) {
      --n_blocks;
    } else if (M_TOK(messages[i]) == OPENBLOCK) {
      ++n_blocks;
    }
    if (n_blocks == 0) {
      return i;
    }
  }

  return -1;

}

/* Returns the level of paren nesting for the token at tok_idx. */
int tok_paren_level (MESSAGE_STACK messages, int first_paren_idx,
		     int tok_idx) {
  int n_parens = 0;
  int i;
  for (i = first_paren_idx; i >= tok_idx; --i) {
    if (M_TOK(messages[i]) == OPENPAREN)
      ++n_parens;
    if (M_TOK(messages[i]) == CLOSEPAREN)
      --n_parens;
  }
  return n_parens;
}

/* returns the stack index of the final ARRAYCLOSE token. */
int last_subscript_tok (MESSAGE_STACK messages, int array_label_idx,
			int stack_end) {
  int i, next_tok, lvl;

  lvl = 0;
  i = __ctalkNextLangMsg (messages, array_label_idx, stack_end);

  for (; i > stack_end; --i) {

    if (M_ISSPACE(messages[i]))
      continue;

    if (M_TOK(messages[i]) == ARRAYOPEN) {
      ++lvl;
    }
    if (M_TOK(messages[i]) == ARRAYCLOSE) {
      --lvl;
    }

    if (lvl == 0) {
      if ((next_tok = __ctalkNextLangMsg (messages, i, stack_end)) != ERROR) {
	if (M_TOK(messages[next_tok]) != ARRAYOPEN) {
	  return i;
	}
      } else {
	return ERROR;
      }
    }

  }
  return ERROR;
}

/*  return true for a struct expression with a terminal
    member that is an array element; e.g.,
    
    mystruct.mymbr[x]
    
    Don't add this  to any CVAR registration fns without
    an example expression.
*/
bool struct_expr_terminal_array_member (char *expr) {
  if (strchr (expr, '.') &&
      expr[strlen(expr) - 1] == ']')
    return true;
  else
    return false;
}


static inline int next_msg (MESSAGE_STACK messages, 
			    int this_msg, int stack_end) { 

  int i = this_msg - 1;

  while ( i >= stack_end ) {
    /* This is necessary when stack_end != e_message_ptr. */
    if (!messages[i] || !IS_MESSAGE (messages[i]))
      return ERROR;
    if (!M_ISSPACE(messages[i]))
      return i;
    --i;
  }
  return ERROR;
}

/* 
   Check for an already evaluated typecast within
   a set of parentheses. E.g.,
         ((void *)0)
   Note that -----^ 
   must be a single token constant.

*/
bool is_single_token_cast (EXPR_PARSER *p, int idx,
			   int *close_paren_out) {
  int next_idx, matching_paren_idx, 
    matching_inner_paren_idx, i_2, inner_next_idx,
    inner_next_idx_2;

  if ((next_idx = next_msg (p -> m_s, idx, p -> msg_frame_top))
      != ERROR) {
    if ((M_TOK(p -> m_s[next_idx]) == OPENPAREN) &&
	(p -> m_s[next_idx] -> attrs & 
	 RT_TOK_IS_TYPECAST_EXPR)) {
      if ((matching_paren_idx = __ctalkMatchParen
	   (p -> m_s, idx, p -> msg_frame_top)) != ERROR) {
	if ((matching_inner_paren_idx = __ctalkMatchParen
	     (p -> m_s, next_idx, p -> msg_frame_top)) 
	    != ERROR) {
	  if ((inner_next_idx = next_msg 
	       (p -> m_s, matching_inner_paren_idx,
		p -> msg_frame_top))
	      != ERROR) {
	    if ((M_TOK(p -> m_s[inner_next_idx]) == INTEGER) ||
		(M_TOK(p -> m_s[inner_next_idx]) == LONG) ||
		(M_TOK(p -> m_s[inner_next_idx]) == LONGLONG) ||
		(M_TOK(p -> m_s[inner_next_idx]) == FLOAT)) {
	      if ((inner_next_idx_2 = next_msg
		   (p -> m_s, inner_next_idx,
		    p -> msg_frame_top))
		  != ERROR) {
		if (inner_next_idx_2 ==
		    matching_paren_idx) {
		  p -> m_s[idx] -> obj = 
		    p -> m_s[matching_paren_idx] -> obj = 
		    p -> m_s[next_idx] -> obj;
		  for (i_2 = idx; i_2 >= matching_paren_idx; --i_2)
		    ++p -> m_s[i_2] -> evaled;
		  *close_paren_out = matching_paren_idx;
		  return true;
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return false;
}

/* Returns a pointer to the last -> or . in an expression. */
char *terminal_struct_op (char *expr) {
  int n_matches, last_arrow = -1, last_dot = -1;
  long long int dot_offsets[MAXARGS], arrow_offsets[MAXARGS];
  if ((n_matches = __ctalkSearchBuffer ("->", expr, arrow_offsets))
      != 0) {
    last_arrow = arrow_offsets[n_matches - 1];
  }
  if ((n_matches = __ctalkSearchBuffer (".", expr, dot_offsets))
      != 0) {
    last_dot = dot_offsets[n_matches - 1];
  }
  if (last_arrow > last_dot) {
    return &expr[last_arrow];
  } else if (last_dot > last_arrow) {
    return &expr[last_dot];
  } else {
    return NULL;
  }
}

/* 
   Most SUBSCRIPT stacks should be MAXARGS in size, which
   should be enough anywhere.
   Doesn't handle nested subscripts yet.    
*/
int parse_subscript_expr (MESSAGE_STACK messages,
			  int start_idx, int end_idx,
			  SUBSCRIPT subs[],
			  int *n_subscripts_out) {
  int i;
  *n_subscripts_out = 0;
  for (i = start_idx; i >= end_idx; i--) {
    switch (M_TOK(messages[i])) {
    case ARRAYOPEN:
      subs[*n_subscripts_out].block_start_idx = i;
      subs[*n_subscripts_out].start_idx = 
	__ctalkNextLangMsg (messages, i, end_idx);
      break;
    case ARRAYCLOSE:
      subs[*n_subscripts_out].block_end_idx = i;
      subs[*n_subscripts_out].end_idx = 
	__ctalkPrevLangMsg (messages, i, start_idx);
      ++(*n_subscripts_out);
      break;
    }
  }
  return *n_subscripts_out;
}

/* Note: Make sure that we are within an argument block first,
   as when called by handle_blk_break_stmt (control.c). */
bool is_switch_closing_brace (MESSAGE_STACK messages,
			      int close_brace_idx,
			      int stack_start) {
  int i;
  int n_braces, n_parens;
  n_braces = 0;
  for (i = close_brace_idx; i <= stack_start; ++i) {
    if (M_TOK(messages[i]) == CLOSEBLOCK) {
      ++n_braces;
    } else if  (M_TOK(messages[i]) == OPENBLOCK) {
      --n_braces;
    }
    if (n_braces == 0)
      break;
  }

  if (i >= stack_start) {
    return false;
  }

  if ((i = __ctalkPrevLangMsg (messages, i, stack_start)) != ERROR) {
    if (M_TOK(messages[i]) == CLOSEPAREN) {
      if ((i = __ctalkMatchParenRev (messages, i, stack_start)) != ERROR) {
	if ((i = __ctalkPrevLangMsg (messages, i, stack_start)) != ERROR) {
	  if (M_TOK(messages[i]) == LABEL) {
	    if (str_eq (M_NAME(messages[i]), "switch")) {
	      return true;
	    }
	  }
	}
      }
    }
  }
  return false;
}

char *elide_cvartab_struct_alias (char *s, char *buf_out) {
  char *t, *p, *open_paren, *close_paren_1, *close_paren_2;
  int n_parens;
  /* In case we have an argblk alias expression like this:

       ((OBJECT *)*mystuff_s) -> __o_value

     We can elide the typecast (and second paren), since
     it's only there to prevent compiler warnings.
  */

  *buf_out = 0;
  if (strstr (s, "((OBJECT *)") &&
      ((close_paren_2 = strstr (s, ") -> ")) != NULL)) {
    memset (buf_out, 0, MAXMSG);
    t = s;
    n_parens = 0;
    while (*t) {
      if (*t == '(') {
	if (n_parens == 0)
	  open_paren = t;
	++n_parens;
      } else if (*t == ')') {
	--n_parens;
	if (n_parens == 1) {
	  close_paren_1 = t;
	  break;
	}
      }
      ++t;
    }
    p = buf_out;
    t = s;
    for (t = close_paren_1 + 1; *t;) {
      if (t != close_paren_2) {
	*p = *t;
	++p; ++t;
      } else {
	++t;
      }
      
    }
  }
  return buf_out;
}

/* the same as above, but used after the whitespace is removed
   from the expression (i.e., in get_method_arg_cvars_fold). */
char *elide_cvartab_struct_alias_2 (char *s, char *buf_out) {
  char *t, *p, *open_paren, *close_paren_1, *close_paren_2;
  int n_parens;

  *buf_out = 0;
  if ((open_paren = strstr (s, "((OBJECT*)")) != NULL) {
    if ((close_paren_2 = strstr (s, ")->")) != NULL) {
      memset (buf_out, 0, MAXMSG);
      t = s;
      n_parens = 0;
      while (*t) {
	if (*t == '(') {
	  if (n_parens == 0)
	    open_paren = t;
	  ++n_parens;
	} else if (*t == ')') {
	  --n_parens;
	  if (n_parens == 1) {
	    close_paren_1 = t;
	    break;
	  }
	}
	++t;
      }
      p = buf_out;
      t = s;
      for (t = close_paren_1 + 1; *t;) {
	if (t != close_paren_2) {
	  *p = *t;
	  ++p; ++t;
	} else {
	  ++t;
	}
      
      }
    }
  }
  return buf_out;
}

bool rcvr_has_ptr_cx (EXPR_PARSER *p, int tok_idx, int c_rcvr_idx) {
  int prev_tok, prev_tok_2, i;
  if ((prev_tok = __ctalkPrevLangMsg (p -> m_s, tok_idx, p -> msg_frame_start))
      != ERROR) {
    if ((prev_tok_2 = __ctalkPrevLangMsg
	 (p -> m_s, prev_tok, p -> msg_frame_start)) != ERROR) {
      if (M_TOK(p -> m_s[prev_tok_2]) == MULT) {
	return true;
      } else if ((c_rcvr_idx != -1) &&
		 ((prev_tok_2 = __ctalkPrevLangMsg
		   (p -> m_s, c_rcvr_idx, p -> msg_frame_start)) != ERROR)) {
	if (M_TOK(p -> m_s[prev_tok_2]) == MULT) {
	  return true;
	}
      } else if ((c_rcvr_idx == p -> msg_frame_start) &&
		 (c_rcvr_idx < P_MESSAGES)) {
	for (i = c_rcvr_idx; i <= P_MESSAGES; ++i) {
	  if (M_ISSPACE(p -> m_s[i]))
	    continue;
	  else if (M_TOK(p -> m_s[i]) == OPENPAREN)
	    return true;
	  else if (M_TOK(p -> m_s[i]) == CLOSEPAREN)
	    return true;
	}
      } else if (tok_idx < P_MESSAGES) {
	for (i = tok_idx; i <= P_MESSAGES; ++i) {
	  if (M_ISSPACE(p -> m_s[i]))
	    continue;
	  else if (M_TOK(p -> m_s[i]) == OPENPAREN)
	    return true;
	  else if (M_TOK(p -> m_s[i]) == CLOSEPAREN)
	    return true;
	  else if (M_TOK(p -> m_s[i]) == MULT)
	    return true;
	}
      }
    }
  }
  return false;
}

bool prev_tok_is_symbol (MESSAGE_STACK messages, int tok_idx) {
  int prev_idx;
  if ((prev_idx = __ctalkPrevLangMsg (messages, tok_idx, P_MESSAGES))
      != ERROR) {
    if (IS_OBJECT(messages[prev_idx] -> obj)) {
      if (str_eq (messages[prev_idx] -> obj -> __o_class -> __o_name,
		  SYMBOL_CLASSNAME)) {
	return true;
      } else if (__ctalkIsSubClassOf
		 (messages[prev_idx] -> obj -> __o_class
		  -> __o_name, SYMBOL_CLASSNAME)) {
	return true;
      }
    }
  }
  return false;
}

/*
  Checks for an inline function in the headers in the form:

    static inline char *__inline_<fn_name> (param_list) {
      return <our_builtin> (<arglist);
    }

    These occur a lot in MacOS headers.
 */
#ifdef __APPLE__
 bool is_apple_inline_chk (MSINFO *ms) {
  int prev_tok, prev_tok_2, prev_tok_3, i;
  prev_tok = __ctalkPrevLangMsg (ms -> messages, ms -> tok,
				 ms -> stack_start);
  if (prev_tok != ERROR) {
    prev_tok_2 = __ctalkPrevLangMsg (ms -> messages, prev_tok,
				     ms -> stack_start);
  } else {
    return false;
  }
  if (prev_tok_2 != ERROR) {
    prev_tok_3 = __ctalkPrevLangMsg (ms -> messages, prev_tok_2,
				     ms -> stack_start);
  } else {
    return false;
  }
  
  if ((M_TOK(ms -> messages[prev_tok]) == LABEL) &&
      (M_TOK(ms -> messages[prev_tok_2]) == OPENBLOCK) &&
      (M_TOK(ms -> messages[prev_tok_3]) == CLOSEPAREN)) {
    return true;
  } else {
    return false;
  }
}
#endif
