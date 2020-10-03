/* $Id: resolve.c,v 1.4 2020/10/03 14:08:20 rkiesling Exp $ */

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "classlib.h"
#include "symbol.h"

/* 
 *   The function resolve() looks up object and method references.
 */

extern I_PASS interpreter_pass;         /* Declared in lib/rtinfo.c.  */
extern int method_from_proto;           /* Declared in mthdref.c.     */
extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c.*/
extern int new_method_ptr;
extern int verbose_opt;                 /* Declared in main.c.        */
extern CTRLBLK *ctrlblks[MAXARGS + 1];  /* Declared in control.c.     */
extern int ctrlblk_ptr;
extern bool ctrlblk_pred, ctrlblk_blk;
extern bool for_init, for_term, for_inc;
extern bool argblk;                    /* Declared in argblk.c.      */
extern int have_complex_arg_block;     /* Set and reset in complexmethod.c, */
				       /* also reset in method_call ().    */
extern int complex_arg_block_start;
extern int complex_arg_block_rcvr_ptr; /* Set by method_call ()            */
extern OBJECT *rcvr_class_obj;         /* declared in lib/rtnwmthd.c       */
extern int nolinemarker_opt;           /* declared in main.c.              */
extern OBJECT *classes;       /* Class dictionary, declared in cclasses.c */
extern int fn_defined_by_header;  /* declared in fnbuf.c */

extern HASHTAB declared_method_names;       /* Initialized from init_method_stack*/

static inline bool is_method_or_proto_name (OBJECT *class_object,
					    char *name) {
  if (is_method_name (name) ||
      is_method_proto (class_object, name))
    return true;
  else
    return false;
}

static int label_following_deref_expr (MESSAGE_STACK messages,
				       int deref_expr_start) {
  int last_tok_idx, following_tok_idx;

  if ((last_tok_idx = struct_end (message_stack (),
				  deref_expr_start,
				  get_stack_top (message_stack ())))
      != ERROR) {
    if ((following_tok_idx = nextlangmsg (message_stack (),
				 last_tok_idx)) != ERROR) {
      if ((M_TOK(messages[following_tok_idx]) == LABEL) ||
	  (M_TOK(messages[following_tok_idx]) == METHODMSGLABEL)) {
	return following_tok_idx;
      }
    }
  }
  return ERROR;
}

/* this is necessary to determine whether a predicate should
   contain a function template. */
static bool predicate_contains_c_function (void) {
  int start, end, i;
  MESSAGE *m;
  if (((start = ctrlblk_pred_start_idx ()) != ERROR) &&
      ((end = ctrlblk_pred_end_idx ()) != ERROR)) {
    for (i = start; i >= end; i--) {
      m = message_stack_at (i);
      if (M_TOK(m) == LABEL) {
	if (get_function (M_NAME(m))) {
	  return true;
	}
      }
    }
  }
  return false;
}

static bool prev_tok_is_unary_op (MSINFO *ms) {
  int i;

  for  (i = ms -> tok + 1; i <= ms -> stack_start; ++i) {
    if (M_ISSPACE(ms -> messages[i]))
      continue;
    if (IS_C_UNARY_MATH_OP(M_TOK(ms -> messages[i]))) {
      return true;
    } else {
      break;
    }
  }

  return false;
}

/*
 *  Mark a prefix method syntactically _unless_
 *  it is a ! operator and it is the first token
 *  of a contrl block predicate, in which case we 
 *  let the compiler handle the negation in context....
 */
#define PREFIX_PREV_OP_TOKEN_NOEVAL(tok) (IS_C_OP_TOKEN_NOEVAL(tok) || \
			    (tok == OPENBLOCK) || \
			    (tok == CLOSEBLOCK) || \
			       (tok==OPENPAREN) || \
			     (tok==CLOSEPAREN))
int prefix_method_attr (MESSAGE_STACK messages, int message_ptr) {
  int prev_tok_idx, next_tok_idx;
  int r;
  MESSAGE *m_prev_tok;
  MESSAGE *m = messages[message_ptr];
  MESSAGE *m_rcvr, *m_op;

  r = FALSE;

  if ((next_tok_idx = nextlangmsgstack_pfx (messages, message_ptr)) != ERROR) {
    if ((M_TOK(messages[next_tok_idx]) == SEMICOLON) ||
	(M_TOK(messages[next_tok_idx]) == ARGSEPARATOR))
      return r;
  }

  /* Use prevlangmsgstack_pfx () so we can find a token at the beginning of a
     frame without errors. */
  /* If there's an op at the beginning of a stack, then it's
     automatically a prefix op. */
  if ((prev_tok_idx = prevlangmsg (messages, 
				   message_ptr)) == ERROR) {
    if (IS_C_UNARY_MATH_OP(M_TOK(messages[message_ptr])) ||
	(M_TOK(messages[message_ptr]) == MINUS)) {
      messages[message_ptr] -> attrs |= TOK_IS_PREFIX_OPERATOR;
      return TRUE;
    } else {
      return FALSE;
    }
  }

  m_prev_tok = messages[prev_tok_idx];
  if (M_TOK(m) == EXCLAM) {
    if (ctrlblk_pred) {
      /*
       *  If a contrl block predicate begins with a sequence 
       *  like these, then use the C operators.  We can send
       *  them to the output verbatim.
       *
       *     <negation_prefix_tok> : !
       *     <negation_prefix_tok_sequence> :  
       *       <negation_prefix_tok> |
       *       ++ | -- | ~ | & | * <negation_prefix_tok> |
       *       <negation_prefix_tok_sequence> <negation_prefix_tok> |
       *
       *    <if|while> (<prefix_tok_sequence>!<expr>) { ...
       *  
       *  When compiling, though, only sequences of ! and ~ do not generate
       *  errors or warnings.
       *
       *  Sequences of multiple ++|-- operators are not legal C, so we
       *  don't support them in C expressions both here and elsewhere.
       *
       *  NOTE - If we see a leading ! operator, and it is overloaded
       *  by a method, the message gets the TOK_IS_PREFIX_OPERATOR
       *  attribute also, and we fix up the start of the predicate
       *  expression in ctrlblk_pred_rt_expr (), to include the
       *  operator in the run-time expression.
       *
       *  NOTE2 - In actual use this should always be the case,
       *  since we have an Object : ! method.  
       *
       *  TODO - we should simplify this so that ! works the same
       *  everywhere, no matter where it appears in an expression.
       *  See test/expect/condexprs3.c for an example.
       */
      if (!is_leading_ctrlblk_prefix_op_tok
	  (messages, message_ptr, P_MESSAGES)) {
	if (IS_C_UNARY_MATH_OP(M_TOK(m))) {
	  if (PREFIX_PREV_OP_TOKEN_NOEVAL(M_TOK(m_prev_tok))) {
	    m -> attrs |= TOK_IS_PREFIX_OPERATOR;
	    r = TRUE;
	  }
	}
      } else {
	int next_label_idx;
	OBJECT *_pfx_rcvr_obj;
	if ((next_label_idx = nextlangmsg (messages, message_ptr))
	    != ERROR) {
	  m_op = message_stack_at (message_ptr);
	  m_rcvr = message_stack_at (next_label_idx);
	  if (((_pfx_rcvr_obj = get_global_object (M_NAME(m_rcvr), NULL)) 
	       != NULL) || 
	      ((_pfx_rcvr_obj = get_local_object (M_NAME(m_rcvr), NULL)) 
	       != NULL)) {
	    if (get_prefix_instance_method (m, _pfx_rcvr_obj, M_NAME(m_op), 
				     FALSE)) {
	      m -> attrs |= TOK_IS_PREFIX_OPERATOR;
	      r = TRUE;
	    }
	  } else {
	    if (m -> attrs & TOK_SELF) {
	      if (get_prefix_instance_method (m, rcvr_class_obj, 
					      M_NAME(m_op), 
					      FALSE)) {
		m -> attrs |= TOK_IS_PREFIX_OPERATOR;
		r = TRUE;
	      }
	    } else if (m_rcvr -> attrs & TOK_SUPER) {
		if (argblk) {
		  if (get_prefix_instance_method (m, rcvr_class_obj, 
						  M_NAME(m_op), 
						  FALSE)) {
		    m -> attrs |= TOK_IS_PREFIX_OPERATOR;
		    r = TRUE;
		  }
		} else {
		  if (get_prefix_instance_method 
		      (m, 
		       rcvr_class_obj -> __o_superclass, 
		       M_NAME(m_op), 
		       FALSE)) {
		    m -> attrs |= TOK_IS_PREFIX_OPERATOR;
		    r = TRUE;
		  }
		}
	    }
	  }
	}
      }
    } else if (get_function (M_NAME(m_prev_tok))) {
      int i_2, n_fn_parens;
      /*
       *  Distinguish an expression like this:
       *
       *  fn_label (*<arg_expr>)...
       *
       *   from this:
       *
       *   fn_label () * <arg_expr>
       *
       *  ... because prevlangmsgstack_pfx returns the stack index of
       *  the fn_label token.
       */
      n_fn_parens = 0;
      for (i_2 = message_ptr + 1; i_2 <= prev_tok_idx; i_2++) {
	switch (M_TOK(messages[i_2]))
	  {
	  case WHITESPACE:
	  case NEWLINE:
	    continue;
	    break;
	  case OPENPAREN:
	    --n_fn_parens;
	    break;
	  case CLOSEPAREN:
	    ++n_fn_parens;
	    break;
	  }
      }
      if (n_fn_parens < 0) {
	/* then the closing parens of fn_label's argument list
	   occur after our expression's tokens. */
	m -> attrs |= TOK_IS_PREFIX_OPERATOR;
	r = TRUE;
      }
    } else { /* if (ctrlblk_pred) */
      if (IS_C_UNARY_MATH_OP(M_TOK(m))) {
	if (PREFIX_PREV_OP_TOKEN_NOEVAL(M_TOK(m_prev_tok))) {
	  m -> attrs |= TOK_IS_PREFIX_OPERATOR;
	  r = TRUE;
	}
      }
    }
  } else if (IS_C_UNARY_MATH_OP(M_TOK(m)) || /* if (M_TOK(m) == EXCLAM) */
	     is_unary_minus (messages, message_ptr)) {
    if (PREFIX_PREV_OP_TOKEN_NOEVAL(M_TOK(m_prev_tok))) {
      m -> attrs |= TOK_IS_PREFIX_OPERATOR;
      r = TRUE;
    }
  }
  return r;
}

static OBJECT *instantiate_self_from_typecast (MESSAGE_STACK messages,
					       int self_tok_idx) {
  MESSAGE *m;
  
  m = messages[self_tok_idx];

  if (TOK_HAS_CLASS_TYPECAST(m)) {
    if (!IS_OBJECT(m -> obj)) {
      if (!IS_OBJECT(m -> receiver_msg -> obj)) {
	m -> receiver_msg -> obj = class_object_search 
	  (M_NAME(m -> receiver_msg), TRUE);
      }
      m -> obj = 
	instantiate_self_object_from_class
	(m -> receiver_msg -> obj);
    }
    return m -> obj;
  }

  return NULL;
}

/* 
 *  Remove parens in cases like 
 * 
 *    (struct -> mbr) <method>
 *  or
 *    ((struct -> mbr)) <method>
 *
 *   when "struct -> mbr" is registered as the CVAR
 *   receiver.
 *
 *  Note that Ctalk does not yet see an expression like
 *
 *   (struct) -> mbr
 *  
 *  as a struct expression, so an expression like this
 *
 *   ((struct) -> mbr) <method>
 *
 *  doesn't get parsed here.
 */

static void remove_outer_receiver_parens_a (MESSAGE_STACK messages, 
					    int method_message_idx) {
  int stack_begin;
  int close_paren_ptr, open_paren_ptr, last_tok_idx;

  stack_begin = stack_start (messages);

  last_tok_idx = method_message_idx;
  while (1) {

    if ((close_paren_ptr = prevlangmsg (messages, last_tok_idx)) != ERROR) {

      if (M_TOK(messages[close_paren_ptr]) == CLOSEPAREN) {

	if ((open_paren_ptr = match_paren_rev (messages, 
					       close_paren_ptr,
					       stack_begin)) != ERROR) {

	  ++messages[open_paren_ptr] -> evaled;
	  ++messages[open_paren_ptr] -> output;
	  ++messages[close_paren_ptr] -> evaled;
	  ++messages[close_paren_ptr] -> output;

	}
      } else {
	
	return;

      }

      last_tok_idx = close_paren_ptr;
    }

  }

}


/*
 *  Translates a CVAR of the previous token into an object of a basic class
 *  if possible.  Also fills in <rcvr_msg>->obj, <method_msg>->receiver_msg, 
 *  and <method_msg>->receiver_obj.
 */
static int cvar_is_method_receiver (MSINFO *ms, int method_idx) {
  int prev_tok_idx;
  int prev_tok_idx_2;
  int arrayopen_idx, j, n_blocks;
  int struct_start_tok_idx;
  int have_parens;
  MESSAGE *m_prev_tok;
  MESSAGE *m_prev_tok_2;
  CVAR *c, *c_mbr, *c_defn;
  OBJECT *class_object;
  char __s[MAXMSG];

  have_parens = FALSE;

  if ((prev_tok_idx = prevlangmsg (ms -> messages, method_idx)) != ERROR) {
    m_prev_tok = ms -> messages[prev_tok_idx];

    /*
     *  If we see a series of <label> <label> <label>, 
     *  check for a C data type.  This is instead of going
     *  through is_c_var_declaration_msg () again.
     *  
     */
    if (m_prev_tok -> attrs & TOK_IS_DECLARED_C_VAR) {
      if ((prev_tok_idx_2 = prevlangmsg (ms -> messages, prev_tok_idx)) != ERROR) {

	m_prev_tok_2 = ms -> messages[prev_tok_idx_2];

	if ((m_prev_tok_2 -> attrs & TOK_IS_DECLARED_C_VAR) ||
	    is_c_data_type (M_NAME(m_prev_tok_2)))
	    return FALSE;
      }
    }

    /*
     *  Handle a cases where a CVAR is enclosed in parentheses.
     */
    if (M_TOK(m_prev_tok) == CLOSEPAREN)
      have_parens = TRUE;

    while (M_TOK(m_prev_tok) == CLOSEPAREN) {
      if ((prev_tok_idx = prevlangmsg (ms -> messages, prev_tok_idx)) != ERROR) {
	m_prev_tok = ms -> messages[prev_tok_idx];
      }
    }

    if (M_TOK(m_prev_tok) == ARRAYCLOSE) {
      for (n_blocks = 0, j = prev_tok_idx, arrayopen_idx = -1; 
	   (j <= ms -> stack_start) && (arrayopen_idx == -1); j++) {
	if (M_TOK(ms -> messages[j]) == ARRAYCLOSE)
	  --n_blocks;
	if (M_TOK(ms -> messages[j]) == ARRAYOPEN)
	  ++n_blocks;
	if (!n_blocks) {
	  arrayopen_idx = j;
	}
      }
      prev_tok_idx = prevlangmsg (ms -> messages, arrayopen_idx);
      m_prev_tok = ms -> messages[prev_tok_idx];
      if (((c = get_local_var (M_NAME(m_prev_tok))) != NULL) ||
	  (( c = get_global_var (M_NAME(m_prev_tok))) != NULL)) {
	if ((class_object = get_class_object
	     (basic_class_from_cvar (ms -> messages[method_idx], c, 1)))
	    != NULL) {
	    if (is_method_or_proto_name (class_object,
					 M_NAME(ms -> messages[method_idx]))) {
	    m_prev_tok -> obj = create_object_init 
	      (class_object -> __o_name,
	       class_object -> __o_superclassname,
	       c -> name,
	       NULLSTR);
	    ms -> messages[method_idx] -> tokentype = METHODMSGLABEL;
	    ms -> messages[method_idx] -> receiver_msg = m_prev_tok;
	    ms -> messages[method_idx] -> receiver_obj = m_prev_tok -> obj;
	    ms -> messages[method_idx] -> attrs |= RCVR_OBJ_IS_C_ARRAY_ELEMENT;
	    return TRUE;
	  }
	}
      }
    } else { /* if (M_TOK(m_prev_tok) == ARRAYCLOSE) */
      if (m_prev_tok -> attrs & TOK_IS_POSTFIX_OPERATOR) {
	/*
	 *  receiver_msg is set by set_cvar_rcvr_postfix_attrs_a ().
	 */
	if (((c = get_local_var (M_NAME(m_prev_tok->receiver_msg)))!=NULL)||
	    (( c = get_global_var (M_NAME(m_prev_tok->receiver_msg)))!=NULL)){
	  if (c -> attrs & CVAR_ATTR_STRUCT_DECL) {
	    CVAR *__c_mbr, *__c_defn;
	    int __s_term, __s_start;
	    MESSAGE *m_start;
	    ms -> stack_start = stack_start (ms -> messages);
	    for (__s_start = prev_tok_idx; __s_start <= ms -> stack_start; 
		 __s_start++) {
	      if (ms -> messages[__s_start] == m_prev_tok -> receiver_msg)
		break;
	    }
	    m_start = ms -> messages[__s_start];
	    if ((__s_term = struct_end (ms -> messages, 
					      __s_start,
					      get_stack_top (ms -> messages)))
		> 0) {
	      if (((__c_defn = get_local_struct_defn (M_NAME(m_start)))
		   !=NULL) ||
		  ((__c_defn = get_global_var (M_NAME(m_start)))!=NULL)) {
		if ((__c_mbr=struct_member_from_expr_b (ms -> messages, 
						     __s_start,
						     __s_term,
						     __c_defn))!=NULL) {
		  if ((class_object = get_class_object
		       (basic_class_from_cvar (ms -> messages[method_idx],
						 __c_mbr, 0))) != NULL) {
		      if (is_method_or_proto_name
			  (class_object,
			   M_NAME(ms -> messages[method_idx]))) {
		      m_prev_tok -> obj = create_object_init 
			(class_object -> __o_name,
			 class_object -> __o_superclassname,
			 c -> name,
			 NULLSTR);
		      ms -> messages[method_idx] -> tokentype = METHODMSGLABEL;
		      ms -> messages[method_idx] -> attrs |= 
			RCVR_OBJ_IS_C_STRUCT_ELEMENT;
		      ms -> messages[method_idx] -> receiver_msg = 
			m_prev_tok -> receiver_msg;
		      ms -> messages[method_idx] -> receiver_obj = m_prev_tok -> obj;
		      return TRUE;
		    } else { /* if (((m = get_instance_method  ...  */
		      return FALSE;
		    } /* if (((m = get_instance_method ...  */
		  } else { /* if ((class_object = get_class_object... */
		    return FALSE;
		  } /* if ((class_object = get_class_object (classname))...*/
		}
	      }
	    }
	  } else {/* if (c -> attrs & CVAR_ATTR_STRUCT_DECL) */
	    int __struct_expr_end;
	    /* 
	     *  Case where the tag is a not the first tag:
	     *  struct _s {
	     *   ...
	     *  } <tag1>, <tag2>;
	     *
	     */
	    if ((c -> type_attrs & CVAR_ATTR_STRUCT_TAG) ||
		(c -> type_attrs & CVAR_ATTR_STRUCT_PTR_TAG)) {
	      for (j = prev_tok_idx; j <= ms -> stack_start; j++)
		if (ms -> messages[j] == m_prev_tok -> receiver_msg)
		  break;
	      struct_start_tok_idx = j;
	      if ((__struct_expr_end = struct_end
		   (ms -> messages, struct_start_tok_idx,
		    get_stack_top (ms -> messages))) <= 0)
		return FALSE;
	      toks2str (ms -> messages, struct_start_tok_idx,
			__struct_expr_end, __s);
	      if (((c_defn = get_local_struct_defn (c -> type)) 
		   != NULL) ||
		  ((c_defn = get_global_var (c -> type)) != NULL)) {
		c = c_defn;
	      }
	      if ((c_mbr = struct_member_from_expr (__s, c)) != NULL) {
		if ((class_object = get_class_object
		     (basic_class_from_cvar (ms -> messages[method_idx],
					       c_mbr, 0))) != NULL) {
			if (is_method_or_proto_name
			    (class_object,
			     M_NAME(ms -> messages[method_idx]))) {
			m_prev_tok -> obj = create_object_init 
		      (class_object -> __o_name,
		       class_object -> __o_superclassname,
		       c -> name,
		       NULLSTR);
		    ms -> messages[method_idx] -> tokentype = METHODMSGLABEL;
		    ms -> messages[method_idx] -> attrs 
		      |= RCVR_OBJ_IS_C_STRUCT_ELEMENT;
		    ms -> messages[method_idx] -> receiver_msg = 
		      m_prev_tok -> receiver_msg;
		    ms -> messages[method_idx] -> receiver_obj = m_prev_tok -> obj;
		    return TRUE;
		  } else { /* if (((m = get_instance_method  ...  */
		    return FALSE;
		  } /* if (((m = get_instance_method ...  */
		} else { /* if ((class_object = get_class_object (classname))*/
		  return FALSE;
		} /* if ((class_object = get_class_object (classname)) */
	      }
	    } else {/* if ((c -> type_attrs & CVAR_ATTR_STRUCT_TAG)... */
	      if ((c -> attrs & CVAR_ATTR_STRUCT_TAG) ||
		  (c -> attrs & CVAR_ATTR_STRUCT_PTR_TAG) ||
		  ((c -> attrs & CVAR_ATTR_STRUCT_PTR) &&
		   !c -> members)) {
		ms -> stack_start = stack_start (ms -> messages);
		for (j = prev_tok_idx; j <= ms -> stack_start; j++)
		  if (ms -> messages[j] == m_prev_tok -> receiver_msg)
		    break;
		struct_start_tok_idx = j;
		if ((__struct_expr_end = struct_end
		     (ms -> messages, struct_start_tok_idx,
		      get_stack_top (ms -> messages))) <= 0)
		  return FALSE;
		if (((c_defn = get_local_struct_defn (c -> type)) 
		     != NULL) ||
		    ((c_defn = get_global_var (c -> type)) != NULL)) {
		  c = c_defn;
		}
		if ((c_mbr = struct_member_from_expr_b (ms -> messages, 
						      struct_start_tok_idx,
						      __struct_expr_end,
						      c)) != NULL) {
		  if ((class_object = get_class_object
		       (basic_class_from_cvar (ms -> messages[method_idx],
						 c_mbr, 0))) != NULL) {
		      if (is_method_or_proto_name
			  (class_object, M_NAME(ms -> messages[method_idx]))) {
		      m_prev_tok -> obj = create_object_init 
			(class_object -> __o_name,
			 class_object -> __o_superclassname,
			 c -> name,
			 NULLSTR);
		      ms -> messages[method_idx] -> tokentype = METHODMSGLABEL;
		      ms -> messages[method_idx] -> attrs 
			|= RCVR_OBJ_IS_C_STRUCT_ELEMENT;
		      ms -> messages[method_idx] -> receiver_msg = 
			m_prev_tok -> receiver_msg;
		      ms -> messages[method_idx] -> receiver_obj = m_prev_tok -> obj;
		      return TRUE;
		    } else { /* if (((m = get_instance_method  ...  */
		      return FALSE;
		    } /* if (((m = get_instance_method ...  */
		  } else { /* if ((class_object = get_class_object ... */
		    return FALSE;
		  } /* if ((class_object = get_class_object (classname)) */
		}
	      } else { /* if ((c -> attrs & CVAR_ATTR_STRUCT_TAG)... */
	      } /* if ((c -> attrs & CVAR_ATTR_STRUCT_TAG)... */
	    }/* if ((c -> type_attrs & CVAR_ATTR_STRUCT_TAG)... */
	  }/* if (c -> attrs & CVAR_ATTR_STRUCT_DECL) */
	      if ((class_object = get_class_object
	       (basic_class_from_cvar (ms -> messages[method_idx],
					 c, 0))) != NULL) {
	    int __s_close_idx;
	      if (is_method_or_proto_name (class_object,
					   M_NAME(ms -> messages[method_idx]))) {
	      m_prev_tok -> obj = create_object_init 
		(class_object -> __o_name,
		 class_object -> __o_superclassname,
		 c -> name,
		 NULLSTR);
	      ms -> messages[method_idx] -> tokentype = METHODMSGLABEL;
	      ms -> messages[method_idx] -> receiver_msg = m_prev_tok;
	      ms -> messages[method_idx] -> receiver_obj = m_prev_tok -> obj;
	      if ((__s_close_idx = prevlangmsg (message_stack (),
						prev_tok_idx)) != ERROR) {
		if (M_TOK(ms -> messages[__s_close_idx]) == ARRAYCLOSE) {
		  ms -> stack_start = stack_start (ms -> messages);
		  for (n_blocks = 0, j = prev_tok_idx, arrayopen_idx = -1; 
		       (j <= ms -> stack_start) && (arrayopen_idx == -1); j++) {
		    if (M_TOK(ms -> messages[j]) == ARRAYCLOSE)
		      --n_blocks;
		    if (M_TOK(ms -> messages[j]) == ARRAYOPEN)
		      ++n_blocks;
		    if (!n_blocks) {
		      arrayopen_idx = j;
		    }
		  }
		  ms -> messages[method_idx] -> attrs |= 
		    RCVR_OBJ_IS_C_ARRAY_ELEMENT;
		} /* if (M_TOK(ms -> messages[__s_close_idx]) == ARRAYCLOSE) */
	      }
	      return TRUE;
	    }
	  }
	}
      } else { /* if (m_prev_tok -> attrs & TOK_IS_POSTFIX_OPERATOR) */
	if (m_prev_tok -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR) {
	  for (j = prev_tok_idx; j <= ms -> stack_start; j++)
	    if (ms -> messages[j] == m_prev_tok -> receiver_msg)
	      break;
	  struct_start_tok_idx = j;
	  if (((c = get_local_var (M_NAME(ms -> messages[struct_start_tok_idx]))) 
		!= NULL) ||
	      (( c = get_global_var (M_NAME(ms -> messages[struct_start_tok_idx]))) 
	       != NULL)) {
	    /* Means that the fn is looking at the receiver token,
	       not the method token. */
	    if (M_TOK(m_prev_tok) == PERIOD ||
		M_TOK(m_prev_tok) == DEREF)
	      return FALSE;
	    toks2str (ms -> messages, struct_start_tok_idx, prev_tok_idx, __s);
	    if (c -> type_attrs & CVAR_TYPE_STRUCT) {
	      /*
	       *  Case where struct declaration also has tags,
	       *  the struct will be the first tag, e.g.:
	       *  
	       *  struct _s {
	       *   ...
	       *  } s_tag, *s_ptr_tag;
	       *
	       *  and the CVAR contains the struct members.
	       */
	      if (!c -> members) {
		/*
		 *  Case where a struct declaration and tags are 
		 *  separate; e.g.:
		 *  
		 *  struct _s {
		 *   ...
		 *  };
		 *  struct _s s_tag, *s_ptr_tag;
		 *
		 *  Then we need to look up the struct definition.
		 */
		if (((c_defn = get_local_struct_defn (c -> type)) 
		     != NULL) ||
		    ((c_defn = get_global_var (c -> type)) != NULL)) {
		  c = c_defn;
		}
	      }
	    } else {
	      /* 
	       *  Case where the tag is a not the first tag:
	       *  struct _s {
	       *   ...
	       *  } <tag1>, <tag2>;
	       *
	       *  Here also we need to look up the struct definition.
	       *
	       */
	      if ((c -> attrs & CVAR_ATTR_STRUCT_TAG) ||
		  (c -> attrs & CVAR_ATTR_STRUCT_PTR_TAG)) {
		if (((c_defn = get_local_struct_defn (c -> type)) 
		     != NULL) ||
		    ((c_defn = get_global_var (c -> type)) != NULL)) {
		  c = c_defn;
		}
	      }
	    }
	    if ((c_mbr = struct_member_from_expr (__s, c)) != NULL) {
	      if ((class_object = get_class_object
		   (basic_class_from_cvar (ms -> messages[method_idx],
					     c_mbr, 0))) != NULL) {
		  if (is_method_or_proto_name (class_object,
					       M_NAME(ms -> messages[method_idx]))){
		  m_prev_tok -> obj = create_object_init 
		    (class_object -> __o_name,
		     class_object -> __o_superclassname,
		     __s,
		     NULLSTR);
		  ms -> messages[method_idx] -> tokentype = METHODMSGLABEL;
		  ms -> messages[method_idx] -> attrs |= RCVR_OBJ_IS_C_STRUCT_ELEMENT;
		  if (struct_start_tok_idx != prev_tok_idx)
		    ms -> messages[method_idx] -> attrs |= 
		      RCVR_TOK_IS_C_STRUCT_EXPR;
		  ms -> messages[method_idx] -> receiver_msg = 
		    m_prev_tok -> receiver_msg;
		  ms -> messages[method_idx] -> receiver_obj = m_prev_tok -> obj;

		  if (have_parens)
		    remove_outer_receiver_parens_a 
		      (ms -> messages, method_idx);

		  return TRUE;
		} else { /* if (((m = get_instance_method  ...  */
		  return FALSE;
		} /* if (((m = get_instance_method ...  */
	      } else { /* if ((class_object = get_class_object (classname))*/
		return FALSE;
	      } /* if ((class_object = get_class_object (classname)) */
	    }  else { /* if ((c_mbr = struct_member_from_expr (__s, c))... */
	      __xfree (MEMADDR(__s));
	      return FALSE;
	    }  /* if ((c_mbr = struct_member_from_expr (__s, c))... */
	  } else {/*if(((c=get_local_var (M_NAME(ms -> messages[struct_start_tok_idx]...*/
	    return FALSE;
	  }/*if(((c=get_local_var (M_NAME(ms -> messages[struct_start_tok_idx]...*/
	} else { /* if (m_prev_tok -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR) */
	  if (((c = get_local_var (M_NAME(m_prev_tok))) != NULL) ||
	      (( c = get_global_var (M_NAME(m_prev_tok))) != NULL)) {
	    if ((class_object = get_class_object
		 (basic_class_from_cvar (ms -> messages[method_idx],
					   c, 0))) != NULL) {
		if (is_method_or_proto_name (class_object,
					     M_NAME(ms -> messages[method_idx]))) {
		m_prev_tok -> obj = create_object_init 
		  (class_object -> __o_name,
		   class_object -> __o_superclassname,
		   c -> name,
		   NULLSTR);
		ms -> messages[method_idx] -> tokentype = METHODMSGLABEL;
		ms -> messages[method_idx] -> receiver_msg = m_prev_tok;
		ms -> messages[method_idx] -> receiver_obj = m_prev_tok -> obj;
		return TRUE;
	      }
	    }
	  } /* if (((c = get_local_var (M_NAME(m_prev_tok))) != NULL) */
	}  /* if (m_prev_tok -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR) */
      } /* if (m_prev_tok -> attrs & TOK_IS_POSTFIX_OPERATOR) */
    } /* if (M_TOK(m_prev_tok) == ARRAYCLOSE) */
  }
  return FALSE;
}

static int calc_method_visible_line (int message_error_line) {
  return message_error_line + method_start_line () - 
    new_method_param_newline_count ();
}

/* Check for objects and ctalk-specific syntaxes in the entire
   expression. */
static bool cvar_expr_needs_translation (MSINFO *ms,
					 int sender_ptr) {
  int next_idx, prev_idx;
  int i, i_next, frame_start_idx, frame_end_idx;
  int terminal_label_idx;
  FRAME *this_frame, *next_frame;
  PARSER *p;
  MESSAGE *m_next, *m_prev;
  bool have_object = false;
  CVAR *cvar, *c_derived = NULL, *mbr;

  /* If there are no objects anywhere in the expression return false
     and treat the expression as plain C.  But also check for operators
     that need to be handled by Ctalk, in syntaxes like LABEL-LABEL
     and STRING_LITERAL-MATH_OP, etc., etc.  */
  if ((p = parser_at (parser_ptr ())) != NULL) {
    this_frame = frame_at (p -> frame);
    frame_start_idx = this_frame -> message_frame_top;
    if  ((next_frame = frame_at (p -> frame - 1)) != NULL) {
      frame_end_idx = next_frame ->  message_frame_top;
    } else {
      frame_end_idx = ms -> stack_ptr;
    }

    for (i = frame_start_idx; 
	 (i >  frame_end_idx) && (have_object == false); 
	 --i) {
      switch (M_TOK(ms -> messages[i]))
	{
	case LABEL:
	  if (get_local_object (M_NAME(ms -> messages[i]), NULL) ||
	      get_global_object (M_NAME(ms -> messages[i]), NULL)) {
	    have_object = true;
	  } else if ((i_next = nextlangmsg (ms -> messages, i)) !=  ERROR) {
	    if (M_TOK(ms -> messages[i_next]) == LABEL) {
	      if (!is_ctrl_keyword (M_NAME(ms -> messages[i_next]))) {
		/* this is to prevent getting hung up on an
		   if ... else if ... else if ... type clause. */
		have_object = true;
	      }
	    } else if (((cvar = get_local_var (M_NAME(ms -> messages[i])))
			!= NULL) ||
		       ((cvar = get_global_var (M_NAME(ms -> messages[i])))
			!= NULL)||
		       ((cvar = is_this_fn_param_2 (M_NAME(ms -> messages[i])))
			!= NULL)) {
	      /* Check for an aggregate type followed by a math op - 
		 mainly char *'s, but check for structs and typedef'd
		 structs also. */
	      if (cvar -> type_attrs & CVAR_TYPE_TYPEDEF) {
		if ((c_derived = get_typedef (cvar -> type)) != NULL) {
		  if ((c_derived -> type_attrs & CVAR_TYPE_STRUCT) ||
		      (c_derived -> attrs &  CVAR_ATTR_STRUCT)) {
		    i_next = struct_end
		      (ms -> messages, i, ms -> stack_ptr);
		    i_next = nextlangmsg (ms -> messages, i_next);
		  } else {
		    /* Need more examples here. */
		    i_next = nextlangmsg (ms -> messages, i);
		  }
		}
	      } else if ((cvar -> type_attrs & CVAR_TYPE_STRUCT) ||
			 (cvar -> type_attrs & CVAR_ATTR_STRUCT)) {
		i_next = struct_end
		  (ms -> messages, i, ms -> stack_ptr);
		i_next = nextlangmsg (ms -> messages, i_next);
	      } else {
		/* Need more examples here, too. */
		i_next  = nextlangmsg (ms -> messages, i);
	      }
	      if (i_next != ERROR) {
		if (IS_C_BINARY_MATH_OP (M_TOK(ms -> messages[i_next]))) {
		  if ((cvar -> type_attrs & CVAR_TYPE_CHAR) &&
		      (cvar ->  n_derefs > 0)) {
		    have_object = true;
		  } else if (cvar -> type_attrs & CVAR_TYPE_STRUCT) {
		    /* this should be mostly true - might need more examples
		       of adding and subtracting typedefs. */
		    terminal_label_idx = struct_end
		      (ms -> messages, i, ms -> stack_ptr);
		    for (mbr = cvar -> members; mbr;
			 mbr = mbr -> next) {
		      if (str_eq (M_NAME(ms -> messages[terminal_label_idx]),
				  mbr -> name))
			break;
		    }
		    if (mbr) {
		      if (str_eq (mbr -> type, "char") &&
			  mbr -> n_derefs > 0) {
			have_object = true;
		      }
		    }
		  } else if (cvar -> type_attrs & CVAR_TYPE_TYPEDEF) {
		    if (IS_CVAR(c_derived)) {
		      if ((c_derived -> type_attrs & CVAR_TYPE_STRUCT) ||
			  (c_derived -> attrs & CVAR_ATTR_STRUCT)) {
			/* also true here - need more examples. */
			terminal_label_idx = struct_end
			  (ms -> messages, i, ms -> stack_ptr);
			for (mbr = c_derived -> members; mbr;
			     mbr = mbr -> next) {
			  if (str_eq (M_NAME(ms -> messages[terminal_label_idx]),
				      mbr -> name))
			    break;
			}
			if (mbr) {
			  if (str_eq (mbr -> type, "char") &&
			      mbr -> n_derefs > 0) {
			    have_object = true;
			  }
			}
		      }
		    }
		  }
		}
	      } /* if (i_next != ERROR) ... */
	    }
	  }
	break;
	case LITERAL:
	  if ((i_next = nextlangmsg (ms -> messages, i)) !=  ERROR) {
	    if (IS_C_BINARY_MATH_OP(M_TOK(ms -> messages[i_next]))) {
	      have_object = true;
	    } else if (M_TOK(ms -> messages[i_next]) == LABEL) {
	      have_object = true;
	    }
	  }
	  break;
	}
    }
	
    if (have_object)
      return true;

  }

  if ((next_idx = nextlangmsg (ms -> messages, sender_ptr)) == ERROR)
    return false;
  if ((prev_idx = prevlangmsg (ms -> messages, sender_ptr)) == ERROR)
    return false;

  m_next = ms -> messages[next_idx];
  m_prev = ms -> messages[prev_idx];

  /* A relational op between two scalar C terms anywhere is valid C,
     so return false and we won't do anything with it.  */
  if ((M_TOK(m_prev) == CHAR) ||
      (M_TOK(m_prev) == INTEGER) || 
      (M_TOK(m_prev) == FLOAT) ||
      (M_TOK(m_prev) == DOUBLE) ||
      (M_TOK(m_prev) == LONG) ||
      (M_TOK(m_prev) == LITERAL_CHAR) ||
      (M_TOK(m_prev) == LONGLONG)) {
    if (get_local_var (M_NAME(m_next)) || get_global_var (M_NAME(m_next))) {
      if (IS_C_RELATIONAL_OP(M_TOK(ms -> messages[sender_ptr]))) {
	return false;
      }
    }
  }

  return false;
}

#define NON_METHOD_CONTEXT(m) (m -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR || \
			       fn_defined_by_header)

static void undefined_receiver_exception (int message_ptr, 
					  int actual_method_idx) {
  OBJECT_CONTEXT error_context;
  MESSAGE *m, *m_actual_method;
  char e[MAXMSG], *e_ptr;
  int visible_line;
  m = message_stack_at (message_ptr);
  m_actual_method = message_stack_at (actual_method_idx);
  error_context = object_context (message_stack (), 
				  actual_method_idx);
  switch (error_context)
    {
    case receiver_context:
      __ctalkExceptionInternal 
	(m, undefined_receiver_x, M_NAME(m_actual_method),0);
      break;
    case c_argument_context:
      if (is_fn_style_label (message_stack (), actual_method_idx,
			     get_stack_top (message_stack ()))) {
	if (strstr (M_NAME(m), "_builtin_")) {
	  return;
	} else {
	  if (!NON_METHOD_CONTEXT(message_stack_at (actual_method_idx))) {
	    sprintf 
	      (e, "\"%s,\" is either not defined as a method, "
	       "or prototyped as a function", 
	       M_NAME(m_actual_method));
	    __ctalkExceptionInternal 
	      (m, ambiguous_operand_x, e,0);
	  } else {
	    return;
	  }
	}
      } else if (interpreter_pass == method_pass) {
	if (!NON_METHOD_CONTEXT(message_stack_at (actual_method_idx))) {
	  e_ptr = collect_errmsg_expr (message_stack (), message_ptr);
	  sprintf 
	    (e, "In method, \"%s:\"\n"
	     "Undefined receiver or variable argument, \"%s\".\n\n\t %s\n\n",
	     new_methods[new_method_ptr + 1] -> method -> name,
	     M_NAME(m_actual_method), e_ptr);
	  __xfree (MEMADDR(e_ptr));
	} else {
	  return;
	}
      } else {
	if (!NON_METHOD_CONTEXT(message_stack_at (actual_method_idx))) {
	  if (interpreter_pass == parsing_pass) {
	    e_ptr = collect_errmsg_expr (message_stack (), message_ptr);
	    sprintf 
	      (e, "Undefined receiver or variable argument, "
	       "\"%s\".\n\n\t%s\n\n",
	       M_NAME(m_actual_method), e_ptr);
	    __xfree (MEMADDR(e_ptr));
	  } else {
	    sprintf (e, "Undefined receiver or variable argument, \"%s\"",
		     M_NAME(m_actual_method));
	  }
	} else {
	  return;
	}
      }
      __ctalkExceptionInternal (m, invalid_operand_x, e,0);
      break;
    default:
      if (!NON_METHOD_CONTEXT(message_stack_at (actual_method_idx))) {
	if (!is_method_name (M_NAME(m_actual_method)) &&
	    !global_var_is_declared (M_NAME(m_actual_method))) {
	  if (interpreter_pass == method_pass) {
	    if (nolinemarker_opt) {
	      /* Without linemarkers, the start line of a method is
		 the absolute line including the header files, so 
		 just use the line within the method. */
	      sprintf (e, "%s.\"\n\tLine %d in method, \"%s,\" (class %s)", 
		       M_NAME(m),
		       m -> error_line,
		       new_methods[new_method_ptr+1] -> method -> name,
		       rcvr_class_obj -> __o_name);
	      __ctalkExceptionInternal (m, undefined_label_x, e, 0);
	    } else {
	      visible_line = calc_method_visible_line (m -> error_line);
	      __ctalkExceptionInternal 
		(m, undefined_label_x, m_actual_method -> name, visible_line);
	    }
	  } else {
#if defined (__x86_64) /***/
	    /* Secure osx lib replacement check here, too. */
	    if (!strstr (m -> name, "__builtin_"))
	      __ctalkExceptionInternal 
		(m, undefined_label_x, m_actual_method -> name,0);
#else
	      __ctalkExceptionInternal 
		(m, undefined_label_x, m_actual_method -> name,
		 m_actual_method -> error_line);
#endif	  
	  }
	}
      }
      break;
    }
}

/* single-token operands only for now */
static int param_is_unary_prefix_operand (MSINFO *ms) {
  int prev_tok_idx;
  if ((prev_tok_idx = prevlangmsgstack_pfx (ms -> messages,
					    ms -> tok))
      != ERROR) {
    if (ms -> messages[prev_tok_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
      return prev_tok_idx;
    }
  }
  return ERROR;
}

static void undefined_self_exception_in_fn (int message_ptr, 
					    int prev_label_ptr,
					    int actual_method_idx) {
  char e[MAXLABEL * 2];
  char expr[MAXLABEL];
  MESSAGE *m_prev_label;
  MESSAGE *m_actual_method;
  m_prev_label = message_stack_at (prev_label_ptr);
  m_actual_method = message_stack_at (actual_method_idx);
  if (argblk) {
    strcatx (expr, m_prev_label -> name, " ",
	     m_actual_method -> name, NULL);
    if (m_prev_label -> attrs & TOK_SELF) {
      sprintf (e, "In the expression:\n\n"
	       "\t\t\"%s\"\n\n"
	       "\tThe class of, \"self,\" in a C function is undetermined--\n"
	       "\tconsider adding a class cast", expr);
      __ctalkExceptionInternal 
	(m_prev_label, undefined_receiver_x, e, 0);
    }
  } else {
    undefined_receiver_exception (message_ptr, actual_method_idx);
  }
}

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

CVAR *get_global_var_not_shadowed (char *name) {
  static CVAR *c;
  METHOD *m;
  PARSER *p;
  OBJECT *local_var;
  int i;
  if ((c = get_global_var (name)) != NULL) {
    if (interpreter_pass == method_pass) {
      m = new_methods[new_method_ptr + 1] -> method;
      for (i = 0; i < m -> n_params; i++) {
	if (str_eq (name, m -> params[i] -> name))
	  return NULL;
      }
    }

    if ((p = CURRENT_PARSER) != NULL) {
      if (p -> vars) {
	for (local_var = p -> vars; local_var; local_var = local_var -> next) {
	  if (str_eq (local_var -> __o_name, name))
	    return NULL;
	}
      }
    }

  }
  return c;
}

METHOD *resolve_method_tok_method = NULL;

#define CVAR_AGGREGATE_DECL(__c) (((__c) -> attrs & CVAR_ATTR_ARRAY_DECL)|| \
				  ((__c) -> attrs & CVAR_ATTR_STRUCT_DECL))

bool resolve_ctrlblk_cvar_reg = false;

OBJECT *resolve (int message_ptr) {

  OBJECT *result_object = NULL;
  OBJECT *classvar_result = NULL;
  OBJECT *class_result = NULL;
  OBJECT *class_object; 
  OBJECT *instancevar_object,
    *classvar_object;
  MESSAGE *m;      
  int msg_frame_top = 0;       /* Avoid a warning.                     */
  int prev_label_ptr;          /* Stack pointers to messages with      */
  int prev_tok_ptr;
  int prev_tok_ptr_2;
  int sender_idx;
  int method_attrs;
  int fn_idx;
  int t;
  int expr_close_paren,
    expr_open_paren,
    cond_pred_start,
    cond_pred_end;
  char conditional_expr[MAXMSG];
  int rcvr_expr_start,
    rcvr_expr_end;
  CVAR *cvar = NULL;
  MESSAGE *m_sender;
  MESSAGE *m_prev_label=NULL;  /* Messages that refer to previously    */
  int next_label_ptr = message_ptr; /* Forward message references to check */
  MESSAGE *m_next_label;       /*  method arguments, if necessary.     */
  MESSAGE *m_prev_tok;
  METHOD *method = NULL;
  OBJECT_CONTEXT param_context;
  int stack_top;
  int __next_tok_idx;
  int __end_idx;
  MESSAGE *m_next_tok;
  char expr_buf_out[MAXMSG], expr_buf_out_2[MAXMSG];
  char _expr[MAXMSG];
  MSINFO ms;

  ms.messages = message_stack ();
  ms.stack_ptr = stack_top = get_stack_top (message_stack ());
  ms.stack_start = P_MESSAGES;
  ms.tok = message_ptr;

  m = ms.messages[ms.tok];

  if (!m || !IS_MESSAGE(m))
    _error ("Invalid message in resolve.");

  if (m -> evaled) {
    return NULL;
  }

  if (IS_C_UNARY_MATH_OP(M_TOK(m)) || M_TOK(m) == MINUS) {
    if (prefix_method_attr (ms.messages, message_ptr)) {
      prefix_inc_before_method_expr_warning (ms.messages,
					     message_ptr);
      return NULL;
    }
  } else if (M_TOK(m) == CONDITIONAL) {
    if (rt_fn_arg_cond_expr(&ms))
      return NULL;
  } else if (M_TOK(m) == LABEL) {

    if (str_eq (M_NAME(m), "enum")) {
      /* skip in parser pass */
      return NULL;
    }

    if (ctrlblk_blk) {
      if (str_eq (M_NAME(m), "continue")) {
	handle_blk_continue_stmt (ms.messages, message_ptr);
      }
      if (argblk && str_eq (M_NAME(m), "break")) {
	handle_blk_break_stmt (ms.messages, message_ptr);
	return NULL;
      }
    }

    if (str_eq (M_NAME(m), "return")) {
      eval_return_expr (ms.messages, message_ptr);
      return NULL;
    } else if (str_eq (M_NAME(m), "eval")) {
     eval_keyword_expr (ms.messages, message_ptr);
     return NULL;
    }

     if ((result_object = get_object (M_NAME(m), NULL)) == NULL) {
      if (((cvar = get_local_var (M_NAME(m))) != NULL) ||
	  ((cvar = get_global_var_not_shadowed (M_NAME(m))) != NULL)) {
	m -> attrs |= TOK_IS_DECLARED_C_VAR;
	if (handle_cvar_arg_before_terminal_method_a (ms.messages, message_ptr)
	   == ERROR) {
	 if ((next_label_ptr = nextlangmsg (ms.messages, message_ptr))
	     != ERROR) {
	   m_next_label =
	     ms.messages[next_label_ptr];

	   if (m -> attrs & TOK_IS_DECLARED_C_VAR) {
	     /* If we were mistaken for an "int." */
	     if (M_TOK(m_next_label) == SEMICOLON) {
	       if ((prev_tok_ptr =
		    prevlangmsg (ms.messages, message_ptr)) != ERROR) {
		 if (get_class_object
		     (M_NAME(ms.messages[prev_tok_ptr]))) {
		   error (m, "error: Identifier, \"%s,\" follows a "
			  "class name without a constructor.",
			  M_NAME(m));
		 }
	       }
	     }
	   }
	   if (M_TOK(m_next_label) == LABEL) {
	     if (!is_c_keyword (M_NAME(m)) &&
		 !(cvar -> type_attrs & CVAR_TYPE_STRUCT) &&
		 !(cvar -> type_attrs & CVAR_TYPE_UNION) &&
		 /* we don't handle aggregates here (yet) */
		 !function_not_shadowed_by_method (ms.messages, message_ptr) &&
		 !typedef_is_declared (M_NAME (m))) {
	       m -> obj = create_object 
		 (basic_class_from_cvar 
		  (ms.messages[message_ptr], cvar, 0),
		  M_NAME(m));
	       /* NOTE: Don't check for a class typecast here - it's actually
		  slower. */
		 /* sic */
	       if (m -> obj -> __o_class) {
		 if (is_method_name(M_NAME(m_next_label)) ||
		     is_method_proto (m->obj->__o_class, m -> name)) {
		   m_next_label -> receiver_msg = m;
		   m_next_label -> receiver_obj = m -> obj;
		   m_next_label -> tokentype = METHODMSGLABEL;
		   if (ctrlblk_pred) {
		     ctrlblk_pred_rt_expr (ms.messages, message_ptr);
		     return NULL;
		   }
		 }
	       }
	     }
	   } else if (M_TOK(m_next_label) == ARRAYOPEN) {
	     int __a, __i, __n;
	     if ((__a = 
		  is_subscript_expr (ms.messages,
				     message_ptr, stack_top ))  > 0) {
	       /* this doesn't return if true */
	       is_OBJECT_ptr_array_deref (ms.messages,
					  message_ptr);
	       for (__i = next_label_ptr; __i >= __a; __i--) {
		 ms.messages[__i] -> receiver_msg = m;
	       }
	       if ((__n = nextlangmsg (ms.messages, __a)) != ERROR) {
		 MESSAGE *__m_n = ms.messages[__n];
		 if ((M_TOK(__m_n) == INCREMENT) ||
		     (M_TOK(__m_n) == DECREMENT)) {
		   __m_n -> attrs |= TOK_IS_POSTFIX_OPERATOR;
		   __m_n -> receiver_msg = 
		     ms.messages[__a] -> receiver_msg;
		 }
		 if (M_TOK(__m_n) == ARRAYOPEN) {
		   if (IS_OBJECT(result_object)) {
		     warning (__m_n, 
	     "Object, \"%s,\" has more than one subscript.", M_NAME(m));
		     warning (__m_n, 
     "The standard class library does not support multi-dimension arrays.");
		   }
		 }
		 /* Handle a subscripted receiver. 
		    Control structure predicates get handled
		    elsewhere. */
		 if (!ctrlblk_pred && 
		     subscr_rcvr_next_tok_is_method_a 
		     (ms.messages, message_ptr, cvar)) {
		   handle_simple_subscr_rcvr (ms.messages,
					      message_ptr);
		   return NULL;
		 }
	       }
	     }
	   } else if ((M_TOK(m_next_label) == INCREMENT) ||
		      (M_TOK(m_next_label) == DECREMENT)) {
	     m_next_label -> attrs |= TOK_IS_POSTFIX_OPERATOR;
	     m_next_label -> receiver_msg = m;
	   } else if ((M_TOK(m_next_label) == PERIOD) ||
		      (M_TOK(m_next_label) == DEREF)) {
	     if ((t = struct_end 
			 (ms.messages, message_ptr, 
			  stack_top)) > 0) {
	       int __i, __postfix_idx;
	       MESSAGE *__m_postfix;
	       m -> attrs |= RCVR_TOK_IS_C_STRUCT_EXPR;
	       for (__i = next_label_ptr; __i >= t; __i--) {
		 ms.messages[__i] -> receiver_msg = m;
		 ms.messages[__i] -> attrs |= 
		   RCVR_TOK_IS_C_STRUCT_EXPR;
	       }
	       if ((__postfix_idx = is_trailing_postfix_op 
		    (ms.messages, t, stack_top)) > 0) {
		 __m_postfix = ms.messages[__postfix_idx];
		 if ((M_TOK(__m_postfix) == INCREMENT) ||
		     (M_TOK(__m_postfix) == DECREMENT)) {
		   __m_postfix -> attrs |= TOK_IS_POSTFIX_OPERATOR;
		   __m_postfix -> receiver_msg = m;
		 }
	       }
	       if (ctrlblk_pred) {
		 int following_label;
		 if ((following_label =
		      label_following_deref_expr (ms.messages,
						  message_ptr))
		     != ERROR) {
		   ctrlblk_pred_rt_expr (ms.messages, message_ptr);
		   return NULL;
		 }
	       }
	       if  (need_cvar_argblk_translation (cvar)) {
		 /* The CVAR should not be anything but an
		    OBJECT * here, so just translating the
		    struct tag should be okay. */
		 handle_cvar_argblk_translation (ms.messages,
						 message_ptr,
						 next_label_ptr,
						 cvar);
	       } else {
		 insert_object_ref (ms.messages, message_ptr);
	       }
	       return NULL;
	     } /* if ((__t = struct_end (ms.messages...*/
	   } /* else if (M_TOK(m_next_label) == PERIOD  ... */
	 } /* if ((next_label_ptr =  ... */
       } /* if (handle_cvar_arg_before_terminal_method_a ... */

	if (insert_object_ref (ms.messages, message_ptr)) {
	  return NULL;
	}

	if (need_cvar_argblk_translation (cvar))
	  return handle_cvar_argblk_translation (ms.messages,
						 message_ptr,
						 next_label_ptr,
						 cvar);

	if (cvar -> scope & GLOBAL_VAR) {
	  if (m -> output == 0) {
	    /* The CVAR should be output immediately after the
	       frame so setting the attribute here is close enough. */
	    if (!(cvar -> attrs & CVAR_ATTR_TUI_DECL)) {
	      cvar -> attrs |= CVAR_ATTR_TUI_DECL;
	      return NULL;
	    } else {
	      /* "extern" declarations get handled by the parser,
		 so this should not be needed normally. */
	    }
	  }
	}

      } /* if (((cvar = get_local_var (M_NAME(m))) != NULL) || */
     } /* if ((result_object = get_object (M_NAME(m), NULL)) == NULL) */

     /* Check for an object that shadows a C symbol first. */
     if (!IS_OBJECT (result_object)) {
       if (!get_method_param_obj (ms.messages, message_ptr) &&
	   (is_c_keyword (M_NAME(m)) ||
	   cvar ||
	   global_var_is_declared_not_duplicated (ms.messages, message_ptr,
						  M_NAME(m)) ||
	   function_not_shadowed_by_method (ms.messages, message_ptr) ||
	    typedef_is_declared (M_NAME (m)))) {
	 if ((prev_tok_ptr = prevlangmsg (ms.messages, message_ptr))
	     != -1) {
	   if ((m_prev_tok = ms.messages[prev_tok_ptr]) != NULL) {
	     if ((M_TOK(m_prev_tok) == LABEL) && (m_prev_tok -> obj != NULL)) {
	       /* A special case where we can call a template
		  method; e.g.,
		  CFunction <template_name>

		  This could probably be generalized to any
		  class method call, if we have any expressions
		  that require it.
	       */
	       if (IS_CLASS_OBJECT (m_prev_tok -> obj) &&
		   str_eq (m_prev_tok -> obj -> __o_name, 
			   CFUNCTION_CLASSNAME)) {
		 if (template_name (M_NAME(m))) {
		   template_call_from_CFunction_receiver 
		     (ms.messages, message_ptr);
		   return m_prev_tok -> obj;
		 } else {
		   object_does_not_understand_msg_warning_a 
		     (m, m_prev_tok -> obj -> __o_name,
		      m_prev_tok -> obj -> __o_classname,
		      M_NAME(m));
		 }
	       } else {
		 if (get_instance_method (m_prev_tok,
					  m_prev_tok -> obj,
					  M_NAME(m),
					  ANY_ARGS, FALSE) ||
		     get_class_method (m_prev_tok,
				       m_prev_tok -> obj,
				       M_NAME(m),
				       ANY_ARGS, FALSE)) {
		   method_shadows_c_keyword_warning_a (ms.messages,
						       message_ptr,
						       prev_tok_ptr);
		 } else {
		   object_does_not_understand_msg_warning_a 
		     (m, m_prev_tok -> obj -> __o_name,
		      m_prev_tok -> obj -> __o_classname,
		      M_NAME(m));
		 }
	       }
	     }
	   }
	 }

	 if (!function_shadows_class_warning_1 (ms.messages, message_ptr))
	   return NULL;
       } else if ((class_result = class_object_search (m -> name, TRUE)) !=
		  NULL) {
	 m -> obj = class_result;
	 if (!is_class_typecast (&ms, message_ptr))
	   default_method (&ms);
	 return class_result;
       } /* if (!get_method_param_obj (ms.messages, message_ptr) */

     } /* if (!IS_OBJECT(result_object)) */
     
#if 0
     /* Look for a class object. */

     if ((class_result = class_object_search (m -> name, TRUE)) != NULL) {
       m -> obj = class_result;
       if (!is_class_typecast (&ms, message_ptr))
	 default_method (&ms);
       return class_result;
     }
#endif
     
   } else { /* if (M_TOK(m) == LABEL) */
     result_object = NULL;
   } /* if (M_TOK(m) == LABEL) */

  /*
   *   Check for a method parameter reference and create an
   *   object if necessary.
   */
  if ((M_TOK(m) == LABEL) && (interpreter_pass == method_pass) &&
      !(m -> attrs & TOK_SELF)) {
    int i;
    METHOD *method;
    method = new_methods[new_method_ptr + 1] -> method;

    if (method && IS_METHOD (method)) {
      for (i = 0; i < method -> n_params; i++) {

	if (!strcmp (m -> name, method -> params[i] -> name) &&
	    !is_instance_variable_message (ms.messages, message_ptr)) {

	  param_context = object_context_ms (&ms);

  	  if ((param_context == c_context) ||
	      (param_context == c_argument_context)) {
	    if (!is_in_rcvr_subexpr_obj_check 
		(ms.messages, message_ptr, stack_top)) {

	      /* 
	       *  Replace the parameter name with the argument accessor
	       *  function.  In an argument context, the arguments are on 
	       *  the run-time stack in reverse order. 
	       */
	      int __n_th_arg;
	      if (need_rt_eval (ms.messages, message_ptr)) {
		if ((__n_th_arg = 
		     obj_expr_is_fn_arg_ms (&ms, &fn_idx)) != ERROR) {
		  rt_obj_arg (ms.messages, 
			      message_ptr, &__end_idx, fn_idx,
			      __n_th_arg);
		} else if (ctrlblk_pred) {
		  MESSAGE *__m;
		  __m = ms.messages[message_ptr];
		  __m -> attrs |= TOK_IS_METHOD_ARG;
		  ctrlblk_pred_rt_expr (ms.messages, message_ptr);
		  __m -> attrs &= !TOK_IS_METHOD_ARG;
		} else if (is_fmt_arg_2 (&ms)) {
		  int _end_ptr, _j;
		  _end_ptr = find_expression_limit (&ms);

		  toks2str (ms.messages, message_ptr, _end_ptr, _expr);
		  fmt_eval_expr_str (_expr, expr_buf_out);
		  fmt_printf_fmt_arg_ms (&ms, expr_buf_out,
					 expr_buf_out_2);
		  fileout (expr_buf_out_2, 0, message_ptr);
		  for (_j = message_ptr; _j >= _end_ptr; --_j) {
		    ++ms.messages[_j] -> evaled;
		    ++ms.messages[_j] -> output;
		  }
		  return NULL;
		} else if ((prev_tok_ptr =
			    param_is_unary_prefix_operand (&ms))
			   != ERROR) {
		  int _expr_end, _j;
		  ms.tok = prev_tok_ptr;
		  collect_expression_buf (&ms, &_expr_end, expr_buf_out);
		  fmt_eval_expr_str (expr_buf_out, expr_buf_out_2);
		  fileout (expr_buf_out_2, 0, ms.tok);
		  for (_j = prev_tok_ptr; _j >= _expr_end; --_j) {
		    ++ms.messages[_j] -> evaled;
		    ++ms.messages[_j] -> output;
		  }
		  return NULL;
		} else {
		  c_param_expr_arg (&ms);
		}
	      } else { /* if (need_rt_eval ... */
		if (is_single_token_method_param (ms.messages,
						  message_ptr,
						  method)) {
		  if ((__n_th_arg = 
		       obj_expr_is_fn_arg_ms (&ms, &fn_idx)) != ERROR) {
		    param_to_fn_arg (ms.messages, message_ptr);
		    return NULL;
		  } else {
		    if (prev_tok_is_unary_op (&ms)) {
		      int prev_idx;
		      if ((prev_idx = prevlangmsgstack (ms.messages,
						       message_ptr))
			  != ERROR) {
			fileout (fmt_rt_expr (ms.messages, prev_idx,
					      &__end_idx, 
					      expr_buf_out),
				 0, message_ptr);
			for (i = prev_idx; i >= __end_idx; --i) {
			  ++ms.messages[i] -> evaled;
			  ++ms.messages[i] -> output;
			}
		      }
		      return NULL;
		    } else {
		      int max_param = method -> n_params - 1;
		      char param_buf[MAXLABEL],
			param_buf_trans[MAXLABEL];
		      format_method_arg_accessor
			(max_param - i,
			 M_NAME(ms.messages[ms.tok]),
			 method -> varargs, param_buf);
		      if (is_fmt_arg_2 (&ms)) {
			// If it's a printf argument, use the format
			// string character to provide the translation.
			// Otherwise, use the class of the
			// method parameter.
			fmt_printf_fmt_arg_ms  (&ms, param_buf,
						param_buf_trans);
		      } else {
			fmt_rt_return (param_buf,
				       method->params[i]->class,
				       TRUE,
				       param_buf_trans);
		      }
		      fileout (param_buf_trans, 0, ms.tok);
		      ++ms.messages[ms.tok] -> evaled;
		      ++ms.messages[ms.tok] -> output;
		      return NULL;
		    }
		  }
		} else {
		    c_param_expr_arg (&ms);
		}
	      } /* if (need_rt_eval ... */
	    } else { /* if (!is_in_rcvr_subexpr_obj_check (messages, msg_ptr... */
	      m ->  obj = create_object  (method->params[i] -> class,
					  method->params[i] -> name);
	      add_object (m -> obj);
	      return m -> obj;
	      
	    } /* if (!is_in_rcvr_subexpr_obj_check (messages, msg_ptr... */
  	  } else { /* if ((param_context == c_context) ... */

	    /*   If the reference occurs in an argument, replace the
	     *   parameter with the rt arg call, and make sure that
	     *   there is an object associated with it so we can
	     *   perform further rt substitutions if necessary.
	     */
	    /*   Take 2 - There's no way, at the moment, of knowing
	     *   if an argument expression has a valid method.  Unless
	     *   the interpreter adds a second pass, or does something
	     *   really clever, we'll have to leave it as a run time 
	     *   expression.
	     *
	     *   Take 3 - method_param_rt_expr () works okay if
	     *   it's limited to an expression on the right-hand
	     *   side of a OBJECT *<c_var> = <expr>; expression,
	     *   so method_param_rt_expr () is limited to handling
	     *   that; otherwise we fall back to method_arg_rt_expr 
	     *   ().
	     */

	    if (param_context == argument_context) {
	      if (!method_fn_arg_rt_expr (ms.messages,
					  message_ptr)) {
		if (method_param_arg_rt_expr (ms.messages,
					      message_ptr,
					      method) == ERROR) {
		  if (ctrlblk_pred) {
		    ctrlblk_pred_rt_expr (ms.messages, message_ptr);
		  } else {
		    method_arg_rt_expr (ms.messages, message_ptr);
		  }
		}
	      }
	    } else if (param_context == receiver_context) {
	      m -> obj = create_object_init
		(((strlen (method -> params[i] -> class) != 0) ?
		  method -> params[i] -> class : "Object"),
		 "Object",
		 method -> params[i] -> name,
		 NULLSTR);
	      save_method_object (m -> obj);
	      if (is_argblk_expr (ms.messages, message_ptr)) {
		return m -> obj;
	      } else {
		int __t;
		rt_expr (ms.messages, message_ptr, &__t, expr_buf_out);
	      }
	    }
	    if (!IS_OBJECT (m -> obj)) {
	      char *path_ptr;
	      OBJECT *o;
	      if ((path_ptr = find_library_include 
		   (method -> params[i] -> class, FALSE)) != NULL) {
		if (!has_class_declaration (path_ptr,
					    method->params[i]->class)) {
		  resolve_undefined_param_class_error 
		    (m, method -> params[i] -> class, 
		     method -> params[i] -> name);
		}
	      } else {
		/* If the class is defined in our source file. */
		for (o = classes; ; o = o -> next) {
		  if (!strcmp (o -> __o_name, method->params[i]->class))
		    break;
		  if (!o || !o -> next) {
		    resolve_undefined_param_class_error 
		      (m, method -> params[i] -> class, 
		       method -> params[i] -> name);
		  }
		}
	      }

 	      result_object =
 		create_object_init 
		(((strlen (method -> params[i] -> class) != 0) ?
		  method -> params[i] -> class : "Object"),
		 "Object",
		 method -> params[i] -> name,
		 NULLSTR);
	      save_method_object (result_object);
	      m -> obj = result_object;
	      return result_object;
	    }
  	  }
	}
      } /* for (i = 0; i < method -> n_params; i++) */
    } /* if (method && IS_METHOD (method)) */

    /* End of the method parameter evaluation stuff (hopefully). */

  } /* if (interpreter_pass == method_pass) */

  /* 
   *  Look for a global or local object.  
   */
  prev_label_ptr = prevlangmsg (ms.messages, message_ptr);
  if (prev_label_ptr != ERROR)
    m_prev_label = ms.messages[prev_label_ptr];


  /*
   *  Handle simple <object> <var> type expressions. And in
   *  more complex expressions, if we can go on later without 
   *  backtracking to the first receiver label, it simplifies 
   *  the later processing a lot.
   *
   *  Note that finding a previous label depends on the parser
   *  framing if the label is in a receiver context.  Especially
   *  true of class variables.
   *
   *  Case for <classname> <classvarname> expression.
   */
  if (M_TOK(m) == LABEL) {
    if ((prev_label_ptr != ERROR) && 
	((classvar_result = 
	  get_class_variable (M_NAME(m), M_NAME(m_prev_label), FALSE))
	 != NULL)) {
      class_variable_expression (ms.messages, message_ptr);
      return classvar_result;
    } else {
      /*
       *  Case for <classvar> without receiver class.
       */
      if ((classvar_result = find_class_variable_2 
	   (ms.messages, message_ptr)) != NULL) {
	m -> obj = classvar_result;
	m -> attrs |= OBJ_IS_CLASS_VAR;
	if (obj_expr_is_fn_expr_lvalue (ms.messages, message_ptr,
					stack_top)) {
	  format_obj_lval_fn_expr (ms.messages,
				   message_ptr);
	} else {
	  default_method (&ms);
	}
	return classvar_result;
      } else {
	/*
	 *  Case for <instance object> <classvar>.  We can
	 *  eliminate the instance object.
	 *
	 *  The <classvar> should be treated as stateless here,
	 *  not pre-evaled; i.e., 
	 *  the next time through resolve, the interpreter
	 *  starts to interpret the following methods and arguments.
	 */
	if (((next_label_ptr = nextlangmsg (ms.messages, message_ptr))
	     != ERROR) && 
	    IS_OBJECT(result_object) &&
	    ((classvar_result = 
	      find_class_variable_2 (ms.messages,
				     next_label_ptr))
	     != NULL)) {
	  ++m -> evaled;
	  ++m -> output;
	  return result_object;
	} else {
	  /*
	   *  When retrieving an object, also check for instance
	   *  variables with the same name.  The function handles
	   *  expressions with instance variables later on.
	   *
	   *  We still need another call to get_object () here.
	   */

	  if (!((m -> evaled > 0) && (interpreter_pass == method_pass))) { 
	    /* I.e., it's a method parameter token. */
	  if ((result_object = get_object (m -> name, NULL)) != NULL) {
	    if (!is_instance_variable_message (ms.messages, 
					       message_ptr)) {
	      /*
	       *  This is sort of a duplication of find_class_variable
	       *  above.  But get_object looks for class variables, too,
	       *  and we'll need to refactor a lot to get it to catch
	       *  this.  We also can't use a closure here.
	       */
	      if (!is_class_variable_message (ms.messages,
					      message_ptr)) {
		if ((prev_label_ptr != -1) && 
		    (m_prev_label -> attrs & TOK_IS_PREFIX_OPERATOR)) {
		  m -> obj = result_object;
		  prefix_method_expr_a (ms.messages, prev_label_ptr,
					message_ptr);
		  return result_object;
		} else {
		  if (m_prev_label && 
		      m_prev_label -> obj &&
		      !strcmp (m_prev_label -> obj -> __o_classname,
			       "Class")) {
		    /* Do something later. It's probably a 
		       class method with the same name as an 
		       object. */
		  } else {
		    if (m -> receiver_msg &&
			(m -> receiver_msg -> attrs &
			 TOK_IS_CLASS_TYPECAST)) {
		      m -> obj = 
			instantiate_object_from_class 
			(get_class_object (M_NAME(m -> receiver_msg)),
			 M_NAME(m));
		      return m -> obj;
		    }
		     
		    /* Needed for default_method () */
		    m -> obj = result_object;  
		    /*
		     *  These functions need to be grouped together.
		     *  If we need to adjust the evaluation of terminal
		     *  objects, then add to these functions.
		     */
		    if (use_new_c_rval_semantics 
			(ms.messages, message_ptr)) {
		      if (terminal_rexpr (ms.messages, message_ptr)
			  == ERROR)  {
			terminal_printf_arg (ms.messages, message_ptr);
		      }
		    } else {
		      if (m_prev_label && 
			  TOK_HAS_CLASS_TYPECAST (m_prev_label)) {
			if (instantiate_self_from_typecast (ms.messages, 
							    prev_label_ptr)) {
			  goto resolve_methods_and_args;
			}
		      }
		      if (!prefix_method_expr_b (ms.messages,
						 message_ptr)) {
			if (!postfix_method_expr_a (ms.messages,
						    message_ptr)) {
			  if (c_lval_class (ms.messages, message_ptr)) {
			    rval_expr_1 (&ms);
			  } else if (obj_is_fn_expr_lvalue (ms.messages,
							    message_ptr,
							    stack_top)) {
			    format_obj_lval_fn_expr (ms.messages,
						     message_ptr);
			  } else if (obj_expr_is_fn_expr_lvalue 
				     (ms.messages, message_ptr,
				      stack_top)) {
			    format_obj_lval_fn_expr (ms.messages,
						     message_ptr);
			  } else if (rcvr_expr_in_parens
				     (&ms, &expr_open_paren, &__end_idx)) {
			    output_rcvr_expr_in_parens
			      (&ms, expr_open_paren, __end_idx);
			  } else {
			    default_method (&ms);
			  }
			}
		      }
		    }
		    return result_object;
		  }
		}
	      }
	    }
	  }

	  } /* if (!((m -> evaled > 0) && (interpreter_pass == method_pass))) */
	}
      }
    }
  } /* if (M_TOK(m) == LABEL) */

  /* 
   *  Check for, "self," "require," "return," "noMethodInit," 
   *  "eval," and "returnObjectClass." If they don't translate 
   *  into run time code, they get elided here also.
   */
   if (!strcmp (m -> name, "require")) {
     require_class (ms.messages, message_ptr);
     return NULL;
   }

   if (m -> attrs & TOK_SELF) {
     /*
      *  Check for "self" outside of a method and issue
      *  a warning if necessary.
      */
       if ((interpreter_pass == method_pass) &&
	   IS_CONSTRUCTOR(new_methods[new_method_ptr+1]->method))
	 self_within_constructor_warning (m);
       if ((interpreter_pass == library_pass ||
	    interpreter_pass == parsing_pass) &&
	   !argblk) {
	 self_outside_method_error (ms.messages, message_ptr);
       }
       if (need_rt_eval (ms.messages, message_ptr)) {
	 if (ctrlblk_pred) {
	   int __n_th_arg;
	   if ((__n_th_arg = 
		obj_expr_is_fn_arg_ms (&ms, &fn_idx)) != ERROR) {
	     rt_obj_arg (ms.messages, message_ptr, &__end_idx, fn_idx,
			 __n_th_arg);
	     return NULL;
	   } else if (is_fmt_arg_2 (&ms)) {
	     handle_self_conditional_fmt_arg (&ms);
	     return NULL;
	   } else {
	     ctrlblk_pred_rt_expr_self (ms.messages, message_ptr);
	     return NULL;
	   }
	 } else { /* if (ctrlblk_pred) */
	   if (is_argblk_expr (ms.messages, message_ptr)) {
	     m -> obj = self_object (ms.messages, message_ptr);
	     return m -> obj;
	   } else {
	     int _expr_end;
	     if ((interpreter_pass != expr_check) && 
		 !have_complex_arg_block) {

	       if ((__next_tok_idx = nextlangmsg (ms.messages,
						  message_ptr)) != ERROR) {
		 m_next_tok = ms.messages[__next_tok_idx];
		 if (M_TOK(m_next_tok) == LABEL) {
		   if (is_self_instance_variable_message (ms.messages, 
							  __next_tok_idx)) {
		     if (is_self_expr_as_fn_lvalue (m_next_tok,
						    message_ptr,
						    __next_tok_idx,
						    stack_top)) {
		       format_self_lval_fn_expr (ms.messages, 
						 message_ptr);
		       if  (!IS_OBJECT (m -> obj))
			 m -> obj = instantiate_self_object ();
		       return m -> obj;
		     } else {
		       if (is_self_expr_as_C_expr_lvalue (m_next_tok,
							  message_ptr,
							  __next_tok_idx,
							  stack_top)) {
			 format_self_lval_C_expr (ms.messages, 
						 message_ptr);
			 if  (!IS_OBJECT (m -> obj))
			   m -> obj = instantiate_self_object ();
			 return m -> obj;
		       } else {
			 get_self_instance_variable_series (m_next_tok,
							    message_ptr,
							    __next_tok_idx,
							    stack_top);
		       }
		     }
		   }
		 }
	       }
	       (void)rt_self_expr (ms.messages, 
				   message_ptr, &_expr_end,
				   expr_buf_out);
	     } else {
	       if (!IS_OBJECT (m -> obj)) {
		 return instantiate_self_object ();
	       }
	     }
	   }
	 }
       } else if (rcvr_expr_in_parens (&ms, &rcvr_expr_start,
				       &rcvr_expr_end)) {
	 output_rcvr_expr_in_parens (&ms, rcvr_expr_start,
				     rcvr_expr_end);
	 return NULL;
       } else { /* if (need_rt_eval (ms.messages, message_ptr)) */
	 if ((result_object =
	      self_object (ms.messages, message_ptr)) != NULL) {
	   m -> obj = result_object;
	   param_context = object_context_ms (&ms);
	   if (param_context == argument_context) {
	     if (!is_in_rcvr_subexpr_obj_check 
		 (ms.messages, message_ptr, stack_top)) {
	       method_self_arg_rt_expr (ms.messages, message_ptr);
	       /*
		*  self_object () or some other function sets the
		*  output member. Clear it so the expression can
		*  be output.
		*/
	       m -> output = 0;
	       return result_object;
	     }
	   } else {
	     if ((__next_tok_idx = nextlangmsg (ms.messages,
						message_ptr)) != ERROR) {
	       m_next_tok = ms.messages[__next_tok_idx];

	       if (is_self_as_fn_lvalue (m_next_tok,
					 message_ptr,
					 __next_tok_idx,
					 stack_top)) {
		 format_self_lval_fn_expr (ms.messages, 
					   message_ptr);
	       } else {
		 if (!IS_OBJECT (m->obj)) 
		   m -> obj = result_object = 
		     instantiate_self_object ();
		 default_method (&ms);
		 return result_object;
	       }
	     }
	   }
	 } else { /* if ((result_object = ... */
	   if (TOK_HAS_CLASS_TYPECAST(m) && !m -> obj) {
	     m -> obj = instantiate_self_object_from_class
	       (m -> receiver_msg -> obj);
	     return NULL;
	   }
	 }  /* if ((result_object = ... */
       } /* if (need_rt_eval (ms.messages, message_ptr)) */
   } /* if (m -> attrs & TOK_SELF  */

   /* 
    * Except for these cases, wait until we evaluate whatever method
    * follows the super message.
    */
   if (m -> attrs & TOK_SUPER) {
     if (argblk_super_expr (&ms) < 0) {
     /* if (argblk_super_expr (ms.messages, message_ptr)) { */
       if (interpreter_pass == method_pass) {
	 if ((next_label_ptr = nextlangmsg (ms.messages, message_ptr))
	     != ERROR) {
	   m_next_label = ms.messages[next_label_ptr];
	   if (str_eq (new_methods[new_method_ptr+1] -> method -> name,
		       M_NAME(m_next_label))) {
	     char expr_out[MAXLABEL];
	     char *s = collect_tokens (ms.messages,
				       message_ptr, next_label_ptr);
	     fmt_eval_expr_str (s, expr_out);
	     fileout (expr_out, 0, message_ptr);
	     __xfree(MEMADDR(s));
	     for (t = message_ptr; t >= next_label_ptr; t--) {
	       ++ms.messages[t] -> evaled;
	       ++ms.messages[t] -> output;
	     }
	   }
	 }
       }
     } 
     return NULL;
   } /* if (m -> attrs & TOK_SUPER) */

  if (fn_return (ms.messages, message_ptr) == 0)
    return NULL;

  /* 
   *  Now do the keyword stuff specifically for methods.
   */

  if (interpreter_pass == method_pass) {
    int i, next_label_ptr;
    if (!strcmp (m -> name, "returnObjectClass")) {
      if ((next_label_ptr = scanforward (ms.messages, message_ptr - 1,
			 message_frame_top_n (parser_frame_ptr () - 1), 
					 LABEL)) == ERROR) {
	warning (m, "Syntax error");
      }
      strcpy (new_methods[new_method_ptr + 1] -> method -> returnclass,
	      ms.messages[next_label_ptr] -> name);
      if ((next_label_ptr = scanforward (ms.messages, message_ptr + 1,
			 message_frame_top_n (parser_frame_ptr () - 1), 
					 SEMICOLON)) == ERROR) {
	warning (m, "Syntax error");
      }
      for (i = message_ptr; i >= next_label_ptr; i--) {
	++ms.messages[i] -> evaled;
	++ms.messages[i] -> output;
      }
      return NULL;
    }
    if (!strcmp (M_NAME(m), "noMethodInit")) {
      new_methods[new_method_ptr + 1] -> method -> no_init = TRUE;
      ++ms.messages[message_ptr] -> evaled;
      ++ms.messages[message_ptr] -> output;
      if ((i = nextlangmsg (ms.messages, message_ptr)) == ERROR)
	warning (m, "Syntax error");
      if (message_stack_at (i) -> tokentype != SEMICOLON)
	warning (m, "Syntax error");
      ++ms.messages[i] -> evaled;
      ++ms.messages[i] -> output;
      return NULL;
    }
  }

  /* Attempt to resolve methods and arguments. */

  /*
   *  Check for modifiers or predicates to the method.  At the 
   *  moment, the only modifier is, "super."
   *
   *  sender_idx, m_sender, and m_actual_method are sort of misnomers,
   *  because they can point to anything that sends a message
   *  to a receiver.
   */
  
 resolve_methods_and_args:

  if (ctrlblk_pred && m ->evaled) /* ctrlblk_pred_rt_expr or similar has */
    return NULL;                  /* output the expression.              */

  sender_idx = 
    compound_method (ms.messages, message_ptr,
		     &method_attrs);
  m_sender = ms.messages[sender_idx];

  /*
   *  We do, "for," loops separately because they all occur within 
   *  the same frame, so we use the CTRLBLK structure indexes 
   *  instead of the frame top.  
   */
  if (ctrlblk_pred) {
    if (C_CTRL_BLK -> stmt_type == stmt_for) {
      if (for_init)
	msg_frame_top = C_CTRL_BLK -> for_init_start;
      if (for_term)
	msg_frame_top = C_CTRL_BLK -> for_term_start;
      if (for_inc)
	msg_frame_top = C_CTRL_BLK -> for_inc_start;
    } else {
      /*
       *  NOTE - It may be necessary to add conditions for other
       *  special loop cases here.
       */
      msg_frame_top = message_frame_top_n (parser_frame_ptr ());
    }
  } else {
    if (ctrlblk_blk) {
      /*
       *  If we have a code block without braces, then set 
       *  msg_frame_top to blk_start_ptr.  If the control block
       *  is delimited by braces, we can simply use the frame 
       *  top.
       */
      if (C_CTRL_BLK -> braces == 0) {
	msg_frame_top = C_CTRL_BLK -> blk_start_ptr;
      } else {
	msg_frame_top = message_frame_top_n (parser_frame_ptr ());
      }
    } else {
      msg_frame_top = message_frame_top_n (parser_frame_ptr ());
    }
  }

  prev_label_ptr = scanback (ms.messages, sender_idx + 1,
  			     msg_frame_top, LABEL);
  prev_tok_ptr = prevlangmsg (ms.messages, sender_idx);

  /*  If the label isn't resolved as an object above, it could be
   *  interpreted as an argument or method of an object in the
   *  predicate.
   */

  /* 
   *  This might not be strictly necessary, but we can quit 
   *  now when we handle control structure blocks somewhere
   *  else.
   */

  if (ctrlblk_blk) {
    if ((sender_idx <= C_CTRL_BLK -> blk_start_ptr) &&
	(sender_idx >= C_CTRL_BLK -> blk_end_ptr) &&
	(prev_label_ptr <= C_CTRL_BLK -> pred_start_ptr) &&
	(prev_label_ptr >= C_CTRL_BLK -> pred_end_ptr))
      return NULL;
  }

  if (prev_label_ptr == ERROR) {
    if ((prev_label_ptr = scanback (ms.messages, sender_idx + 1,
 				    msg_frame_top, C_KEYWORD)) != -1) {
      return NULL;
    } else {
      if (!input_is_c_header ()) {
	/* 
	 *  Issue a warning only for label tokens for which a receiver object
	 *  has not been defined, or if we can determine that the 
	 *  undefined label token appears in an object context.
	 *
	 *  Also checks for goto : labels, which get sent unchanged
	 *  to the output.
	 */
	if ((m_sender -> tokentype == LABEL) && 
#ifdef __APPLE__
	    /* we have to check osx's builtins, too */
	    !IS_DEFINED_LABEL (m_sender -> name) &&
#else
	    !IS_DEFINED_RECEIVER (m_sender -> name) &&
#endif
	    !result_object) {
#if defined(__APPLE__) && defined (__x86_64)
	  /* And we need to check for secure osx lib replacements. */
	  if (!strstr (m -> name, "__builtin_"))
	    (void)undefined_label_check (ms.messages, sender_idx);
#else	  
	  (void)undefined_label_check (ms.messages, sender_idx);
#endif	  
	  return NULL;
	}
      } /* if (!input_is_c_header ()) */
    } /* else */
  } else { /* if (prev_label_ptr == ERROR) */
    m_prev_label = ms.messages[prev_label_ptr];
  } /* if (prev_label_ptr == ERROR) */

  /*   
   *   This is here mainly as a syntax check for odd variable 
   *   cases, but it might save a lot of work further on, too.
   */

  if (m_prev_label && !m_prev_label -> value_obj) {
    if (is_c_data_type (M_NAME(m_prev_label)) ||
	get_typedef (M_NAME(m_prev_label)) ||
	is_incomplete_type (M_NAME(m_prev_label)))
      return NULL;
    if (ctrlblk_pred) {
      if (prev_label_ptr != ERROR) {
	if ((prev_tok_ptr_2 = prevlangmsg (ms.messages, prev_label_ptr))
	    != ERROR) {
	  if (IS_C_RELATIONAL_OP(M_TOK(ms.messages[prev_tok_ptr_2]))) {
	    if (get_local_var (M_NAME(m_prev_label)) ||
		is_this_fn_param (M_NAME(m_prev_label))) {
	      return NULL;
	    }
	  }
	}
      }
    }
  }
  
  /* 
   *   Check if the the most recent reference is an object, or a 
   *   near reference is a method.
   */

  if (prev_label_ptr != ERROR) {

    if (!m_prev_label -> obj && TOK_HAS_CLASS_TYPECAST (m_prev_label)) {
      m_prev_label -> obj = 
	instantiate_object_from_class 
	(m_prev_label -> receiver_msg -> obj, M_NAME(m_prev_label));
    }

    if (IS_OBJECT (m_prev_label -> obj)) {

      /* Now do the method stuff. */

      if (M_TOK(m) == DEREF) {
	ms.tok = prev_tok_ptr;
	if (objderef_handle_printf_arg (&ms)) {
	  return NULL;
	} else {
	  ms.tok = message_ptr;
	}
      }

      /* The previous reference is a class object. */
      if (IS_CLASS_OBJECT(m_prev_label -> obj)) {
	/*
	 *  This might need a check for the start of 
	 *  the parser frame.
	 */
	  if (!TOK_HAS_CLASS_TYPECAST(m)) {
	    if (((method = get_instance_method (m_prev_label, m_prev_label->obj,
						m_sender->name,
						ERROR, FALSE)) 
		 != NULL) ||
		((method = get_class_method (m_prev_label, m_prev_label->obj,
					     m_sender->name,
					     ERROR, FALSE)) 
		 != NULL) ||
		is_method_proto (m_prev_label->obj->__o_class,
			       m_sender -> name)) {
	      m_sender -> receiver_msg = m_prev_label;
	      m_sender -> receiver_obj = m_prev_label -> obj;
	      m_sender -> tokentype = METHODMSGLABEL;
	    } else if (m_prev_label -> obj &&
		       IS_CLASS_OBJECT (m_prev_label -> obj)) {
	      if (str_eq (m_prev_label -> obj -> __o_name,
			  CFUNCTION_CLASSNAME)) {
		if (template_name (M_NAME(m))) {
		  template_call_from_CFunction_receiver 
		    (ms.messages, message_ptr);
		  return m_prev_label -> obj;
		} else {
		  if (!is_class_typecast (&ms, prev_label_ptr)) {
		    undefined_method_follows_class_object (m_sender);
		  }
		}
	      } else if (prev_tok_ptr == prev_label_ptr) {
		if (!is_class_typecast (&ms, prev_label_ptr)) {
		  /*
		   *  This catches a false warning if there's a token
		   *  or tokens between this label and the class object
		   *  label token.
		   */
		  undefined_method_follows_class_object (m_sender);
		}
		return NULL;
	      } else if (interpreter_pass != expr_check &&
			 interpreter_pass != library_pass) {
		/* the expression isn't being parsed on its own. */
		__ctalkExceptionInternal (m_sender, undefined_method_x,
					  m_sender -> name,0);
		return NULL;
	      }
	    }
	  } /* if (!TOK_HAS_CLASS_TYPECAST(m)) */
      } else {
	
	/* 
	 *   The previous label refers to an object instance. 
	 *   Determine if this label refers to a method or 
	 *   an instance variable.  Give precedence to instance
	 *   variables.
	 */

	if ((class_object = 
	     m_prev_label -> obj -> instancevars ?
	     m_prev_label -> obj -> instancevars -> __o_class :
	     m_prev_label -> obj -> __o_class) == NULL)
 	  error (m_prev_label, "Unknown class \"%s.\"", 
		 m_prev_label ->obj -> __o_classname);
	/*
	 *   We still have to make sure that the method (at sender_idx)
	 *   immediately follows the receiver (at prev_label_ptr).
	 *   If not, it's a complex receiver expression, probably in
	 *   parens, which gets handled below.
	 */
	if (prev_tok_ptr == prev_label_ptr) {
	  m_sender -> receiver_msg = m_prev_label;
 	  m_sender -> receiver_obj = m_prev_label -> obj;
	  instancevar_object = NULL;
 	  if (((instancevar_object = 
 		get_instance_variable_series (m_prev_label -> obj, 
					      m_sender,
					      prev_label_ptr, stack_top)) 
	       != NULL) ||
 	      ((method = get_instance_method (m_sender,
 					      class_object, 
				      m_sender -> name, 
					      ERROR, FALSE)) != NULL) ||
	      ((method = get_class_method (m_sender,
					   class_object,
					   m_sender -> name, 
					   ERROR, FALSE)) != NULL) ||
	      is_method_proto (m_prev_label->obj->__o_class,
			       m_sender -> name)) {
	    if (instancevar_object && ctrlblk_pred) {
	      /* We have to set these manually and also check
		 for a series of following instance vars. */
	      m -> obj = instancevar_object;
	      m -> attrs |= OBJ_IS_INSTANCE_VAR;
	      get_instance_variable_series (m_prev_label -> obj,
					    m, 
					    message_ptr,
					    stack_top);
	      ctrlblk_pred_rt_expr (ms.messages, message_ptr);
	      return NULL;
	    }
	    /* 
	     *  This needs to be lazy right now because of
	     *  possible object mutations -- we simply defer
	     *  everything until run-time evaluation when
	     *  trying to generate code in method_call.
	     */
	    ms.tok = message_ptr;
	    if (rval_ptr_context_translate (&ms, prev_label_ptr))
	      return NULL;
	    if (((M_TOK(m_sender) == BOOLEAN_AND) || 
		(M_TOK(m_sender) == BOOLEAN_OR)) &&
		(interpreter_pass == expr_check)) {
	      /* 
		 This is used if we call a parser in
		 check_constant_expr () from 
		 ctrlblk_pred_rt_expr ().
	      */
	      check_major_boolean_parens (ms.messages, message_ptr);
	    }

	    m_sender -> tokentype = METHODMSGLABEL;
	    m_sender -> obj = instancevar_object;
	    if (instancevar_object)
	      m_sender -> attrs |= OBJ_IS_INSTANCE_VAR;
	  } else {
	    /*
	     *  Class variable alone, possibly (almost certainly if it's
	     *  a complex object) with value class different than var 
	     *  class.
	     */
	    if ((m_prev_label->attrs & OBJ_IS_CLASS_VAR) &&
		m_prev_label->obj->instancevars && 
		((method = 
		  get_instance_method 
		  (m_sender,
		   m_prev_label -> obj -> instancevars -> __o_class,
		   M_NAME(m_sender), 
		   ERROR, FALSE)) != NULL)) {
	      m_sender -> tokentype = METHODMSGLABEL;
	    } else {
	      if (is_method_proto (class_object,
				   M_NAME(m_sender)) &&
		  !method_proto_is_output (M_NAME(m_sender)) &&
		  !method_from_proto) {
		method_from_prototype (M_NAME(m_sender));
	      } else {
		MESSAGE *m_prev_tok;

		m_prev_tok = ms.messages[prev_tok_ptr];

		/*
		 * First look for a case where we have a 
		 * <rcvr> <undefined-label>, and if its
		 * prototype is defined in a superclass that is 
		 * still being evaluated, issue a warning and 
		 * defer the evaluation until run time.
		 * 
		 * We can't simply evaluate the method proto
		 * without getting into circular, recursive 
		 * evaluations, which causes a lot of stress on the
		 * parser and a lot of exceptions to the validity
		 * checking of methods and parameters, so we only 
		 * issue a warning.  
		 *
		 * If the warning appears in one of the basic classes,
		 * it's probably better to add that method to the 
		 * basic class anyway, even if the method is already in 
		 * the superclass.
		 */
		if ((M_TOK (m_sender) == LABEL) && 
		    (M_TOK (m_prev_tok) == LABEL) &&
		    (m_prev_tok -> obj != NULL)) {
		  /* This should probably stay as a separate
		     call to is_superclass_method_proto. */
		  if (is_superclass_method_proto 
		      (m_prev_tok -> obj -> __o_classname, 
		       M_NAME(m_sender))) {
		    warning (m_sender, "Undefined Label.\n"
			     "\tLabel, \"%s,\" is not (yet) defined. "
			     "Deferring evaluation until run time.",
			     M_NAME (m_sender));
		    deferred_method_eval_a (ms.messages,
					    prev_tok_ptr,
					    &sender_idx);
		  } else {
		    if (have_user_prototype 
			(m_prev_tok -> obj -> __o_classname, M_NAME(m))) {
		      deferred_method_eval_a (ms.messages,
					      prev_tok_ptr,
					      &sender_idx);
		    } else {
		      if (have_class_cast_on_unresolved_method 
			  (ms.messages, message_ptr)) {
			int __expr_end;
			rt_expr (ms.messages, prev_tok_ptr, &__expr_end,
				 expr_buf_out);
		      } else {
			undefined_method_exception (m_sender, m_prev_tok);
		      }
		    }
		  }
		} else {		    
		  undefined_method_exception (m_sender, m_prev_tok);
		}
	      }
	      /* Helps prevent a more ugly(er) compiler error if we don't 
		 exit with an error code. */
	      if ((next_label_ptr = nextlangmsg (ms.messages,
						 message_ptr)) != ERROR) {
		MESSAGE *_lookahead = ms.messages[message_ptr];
		MESSAGE *_lookahead_2 = ms.messages[next_label_ptr];
		if (IS_C_OP_TOKEN_NOEVAL(M_TOK(_lookahead)) &&
		    (M_TOK(_lookahead_2) == LABEL)) {
		  /* Do what? */
		  ++m_prev_label -> evaled; 
		  ++m_prev_label -> output;
		  ++_lookahead -> evaled;
		  ++_lookahead -> output;
		  ++_lookahead_2 -> evaled;
		  ++_lookahead_2 -> output;
		} else {
		  ++m_sender -> evaled;
		  ++m_sender -> output;
		}
	      }
	      return NULL;
	    }
	  }
	}
      }
    } else { /*     if (IS_OBJECT (m_prev_label -> obj)) */
      if (m -> attrs & TOK_SELF) {
	OBJECT *__rcvr_class_ptr;

	if (instantiate_self_from_typecast (ms.messages, prev_label_ptr)) {
	  __rcvr_class_ptr = m_prev_label -> receiver_msg -> obj;
	} else {
	  __rcvr_class_ptr = rcvr_class_obj;
	}
	if ((instancevar_object = 
	     __ctalkGetInstanceVariable (__rcvr_class_ptr, 
					 M_NAME(m),
					 FALSE)) != NULL) {
	  /* TO DO - 
	   * factor this out here, and in self_object (),
	   * in object.c, and try to use either 
	   * instance_variables_from_class_definition (),
	   * or instance_vars_from_class_object (), 
	   * everywhere, not both.
	   *
	   * This can also come earlier in the evaluation.
	   * Annd... create a SELF_OBJECT attribute, so
	   * we don't need to strcmp so much.
	   */
	  if (!IS_OBJECT (m_prev_label -> obj))
	    m_prev_label -> obj
	      = instantiate_self_object ();
	  m -> obj = instancevar_object;
	  m -> receiver_obj = __rcvr_class_ptr;
	  m -> receiver_msg = m_prev_label;
	  m -> attrs |= OBJ_IS_INSTANCE_VAR;
	  return instancevar_object;
	} else {
	  if ((classvar_object = 
	       __ctalkGetClassVariable (__rcvr_class_ptr, 
					M_NAME(m),
					FALSE)) != NULL) {
	    if (!IS_OBJECT (m_prev_label -> obj))
	      m_prev_label -> obj
		= instantiate_self_object ();
	    m -> obj = classvar_object;
	    m -> receiver_obj = rcvr_class_obj;
	    m -> receiver_msg = m_prev_label;
	    m -> attrs |= OBJ_IS_CLASS_VAR;
	    return classvar_object;
	    }
	  }
      } /* if (m -> attrs & TOK_SELF)) */
    } /*     if (IS_OBJECT (m_prev_label -> obj)) */

    if ((next_label_ptr = nextlangmsg (ms.messages, sender_idx))
	!= ERROR) {
      m_next_label = ms.messages[next_label_ptr];

      /*
       *  The m_next_label token is set in is_argblk_expr ().
       *  Fixup here now that we know the actual receiver object
       *  and receiver message.
       */
      if ((M_TOK(m_next_label) == METHODMSGLABEL) && argblk) {
	m_next_label->receiver_obj = m_sender->receiver_obj;
	m_next_label->receiver_msg = m_sender->receiver_msg;
      }

      if (m_sender -> tokentype == METHODMSGLABEL) {
  	if ((expr_close_paren = 
	     is_in_rcvr_subexpr_obj_check 
	     (ms.messages, sender_idx, stack_top)) == 0) {
	  if (ctrlblk_pred) {
	    if (for_init || for_term || for_inc) {
	      method_call (sender_idx);
	    } else if (predicate_contains_c_function ()) {
	      method_call (sender_idx);
	    } else {
	      resolve_method_tok_method = method;
	      ctrlblk_pred_rt_expr (ms.messages, prev_label_ptr);
	    }
	    return NULL;
	  } else if ((cond_pred_start =
		      arg_is_question_conditional_predicate (&ms)) > 0) {
	    /* fn args only */
	    rt_expr (ms.messages, cond_pred_start,
		     &cond_pred_end, conditional_expr);
	    return NULL;
	  } else {
	    method_call (sender_idx); 
	  }
  	} else {
	  int i_2;
	  expr_open_paren = match_paren_rev (ms.messages,
					     expr_close_paren,
					     P_MESSAGES);
	  /*
	   *  If there are previous objects, they get resolved later,
	   *  so clear them here.  But check for a C variable as 
	   *  the argument and register it if necessary.
	   */
	  for (i_2 = expr_open_paren; i_2 >= expr_close_paren; i_2--) {
	    MESSAGE *m_2;
	    m_2 = ms.messages[i_2];
	    if ((M_TOK(m_2) == LABEL) && m_2 -> obj)
	      m_2 -> obj = NULL;
	  }
	  register_cvar_arg_expr_a (ms.messages, sender_idx);
	}
      } else {
	if (m->receiver_obj && (M_TOK(m_sender) == LABEL)) {
	  undefined_label_check (ms.messages, sender_idx);
	} else {
	  if (interpreter_pass == parsing_pass) {
	      /*
	       *  Here is where we handle complex receiver
	       *  expressions that occur in C functions.
	       *  Expressions in methods get handled below,
	       *  because we also need evaluate method 
	       *  parameters.
	       */
	    if ((m_sender -> tokentype == LABEL) && 
		!m_sender -> obj &&
		!input_is_c_header ()) {
   	      if (!IS_DEFINED_LABEL (m_sender -> name)) {
#ifdef __APPLE__
		if (strstr (m -> name, "builtin__")) {
		  if (is_apple_inline_chk (&ms)) {
		    return NULL;
		  } else {
		    warning (m, "Built-in function, \"%s,\" in input.",
			     m -> name);
		    return NULL;
		  }
		}
#endif		
		if (is_possible_receiver_subexpr_postfix (&ms, sender_idx)) {
		  handle_subexpr_rcvr (sender_idx);
		  return NULL;
 		} else { /* if (is_possible_receiver_subexpr_postfix ... */
    	          if (cvar_is_method_receiver (&ms, sender_idx)) {
		    method_call (sender_idx);
		    return NULL;
		  } else {

		    /* 
		     *  If there's a series <label> <label> <label>
		     *  that hasn't been resolved yet, look for 
		     *  a C type.  This is instead of going through
		     *  is_c_var_declaration_msg () again.
		     */

		    if (m_prev_label -> attrs & TOK_IS_DECLARED_C_VAR) {
		      int prev_label_ptr_2;
		      MESSAGE *m_prev_label_2;

		      if ((prev_label_ptr_2 = prevlangmsg (ms.messages,
							   prev_label_ptr))
			  != ERROR) {
			m_prev_label_2 = ms.messages[prev_label_ptr_2];
			/* Check if we're in an array subscript -
			   a CVAR should be output directly. */
			if (M_TOK(m_prev_label_2) == ARRAYOPEN) {
			  if ((prev_label_ptr_2 =
			       prevlangmsg (ms.messages,
					    prev_label_ptr_2)) != ERROR) {
			    m_prev_label_2 = ms.messages[prev_label_ptr_2];
			    if (m_prev_label_2 -> attrs &
				TOK_IS_DECLARED_C_VAR) {
			      ++m -> evaled;
			      return NULL;
			    }
			  }
			}

			if (m_prev_label_2 -> attrs & TOK_IS_DECLARED_C_VAR) {
			  if (!is_c_data_type (M_NAME(m_prev_label_2))) {

			    undefined_receiver_exception (message_ptr,
							  sender_idx);
			    return NULL;

			  } else {

			    ++m -> evaled;
			    return NULL;

			  }

			} else {

			  if (!is_c_data_type (M_NAME(m_prev_label_2)))
			    undefined_receiver_exception (message_ptr,
							sender_idx);
			  return NULL;

			}
		      }

		    } else if (!strstr (M_NAME(m), "__builtin_")) { /***/

		      if (!instantiate_self_from_typecast (ms.messages,
							   prev_label_ptr)) {

			undefined_self_exception_in_fn (message_ptr,
							prev_label_ptr,
							sender_idx);
			return NULL;

		      }
		    }
		  }
 		}
 	      } else {
		/* 
		 *  If we got this far, first check for a postfix
		 *  method that shadows a C name before giving up. 
		 *  First check for a C syntax.  We've already
		 *  checked for a control structure predicate, so
		 *  check for the C statements, by syntax, 
		 *  which is quicker.  Handle only the simple cases
		 *  right now - there will probably be more complicated
		 *  occurrences of shadowing later.
		 */
		CVAR *c_1;
		int i_1;
		if (prev_tok_ptr != -1) {

		  m_prev_tok = ms.messages[prev_tok_ptr];

		  if (m_sender -> attrs & TOK_IS_DECLARED_C_VAR) {
		    if ((c_1 = get_global_var(M_NAME(m_sender))) != NULL) {
		      if (IS_C_OP_TOKEN_NOEVAL (M_TOK(m_prev_tok)))
			return M_VALUE_OBJ (m_prev_label);
		    }
		    if ((c_1 = get_local_var(M_NAME(m_sender))) != NULL) {
		      if (IS_C_OP_TOKEN_NOEVAL (M_TOK(m_prev_tok)))
			return M_VALUE_OBJ (m_prev_label);
		    }
		  }
		  if (M_TOK(m_prev_tok) == PERIOD ||
		      M_TOK(m_prev_tok) == DEREF) {
		    if ((i_1 = is_struct_member_tok (ms.messages,
						     sender_idx)) != 0) {
		      return M_VALUE_OBJ(m_prev_label);
		    }
		  }
		}
		/* 
		 *  Okay, it's not a C expression (or it's not going to
		 *  parse anyway).
		 */
		if (is_possible_receiver_subexpr_postfix (&ms, sender_idx)) {
		  handle_subexpr_rcvr (sender_idx);
		  return NULL;
		}
	      }
	    } else {/* if ((m_sender -> tokentype == LABEL) &&  */
	      if (!m_sender -> obj && !m_prev_label -> obj) {
		if (prev_tok_ptr != prev_label_ptr) {
		  m_prev_tok = ms.messages[prev_tok_ptr];
 		  if (IS_CONSTANT_TOK(M_TOK(m_prev_tok))) {
                    if (cvar_expr_needs_translation (&ms, sender_idx)) {
		      register_cvar_arg_expr_a 
			(ms.messages, sender_idx);
		      if (method_call_constant_tok_expr_a (ms.messages,
							   sender_idx)) {
			return NULL;
		      }
  		    }
		  } else {
		    /*
		     *  An <array>[<idx>] <receiver> expression.
		     */
		    if (is_aggregate_term_a (ms.messages, prev_tok_ptr)) {
		      register_cvar_rcvr_expr_b 
			(ms.messages, sender_idx);
		      if (method_call_subexpr_postfix_a (ms.messages,
							 sender_idx)) {
			return NULL;
		      }
		    } else {
		      /*
		       *  A <close paren> <math_op> expression
		       */
		      if (method_call_subexpr_postfix_a (ms.messages,
							 sender_idx)) {
			return NULL;
		      } else {
			/*
			 *  Mark a prefix method syntactically _unless_
			 *  it is a ! operator and it is the first token
			 *  of a contrl block predicate. We can pass the 
			 *  ! token verbatim to the output.
			 */
			prefix_method_attr (ms.messages, message_ptr);
		      }
		    }
		  }
		} else {

		  if (m_prev_label -> attrs & TOK_IS_DECLARED_C_VAR) {
		    /*
		     *   If the previous label is a CVAR, and we're
		     *   in a receiver expression.
		     */
		    CVAR *prev_label_cvar;
		    if (((prev_label_cvar=get_local_var (M_NAME(m_prev_label)))
			 != NULL) ||
			((prev_label_cvar=get_global_var (M_NAME(m_prev_label)))
			 != NULL)) {
		      if (CVAR_AGGREGATE_DECL(prev_label_cvar)) {
			if (method_call_subexpr_postfix_b 
			    (ms.messages,
			     sender_idx) == -2) {
			  constrcvr_unhandled_case_warning (ms.messages,
							    sender_idx);
			}
			return NULL;
		      } else {
			if (register_cvar_rcvr_expr_a (ms.messages,
						       sender_idx,
						       prev_label_cvar,
						       M_NAME(m_prev_label))) {
			  return NULL;
			} else { /* if (register_cvar_rcvr_expr_a .. */
			  if (set_cvar_rcvr_postfix_attrs_a 
			      (ms.messages,
			       sender_idx,
			       prev_label_ptr,
			       prev_label_cvar,
			       M_NAME(m_prev_label))) {
			    return NULL;
			  }
			} /* if (register_cvar_rcvr_expr_a .. */
		      }
		    } /* if (((prev_label_cvar=get_local_var ... */
		  } /* if (m_prev_label -> attrs & TOK_IS_DECLARED_C_VAR) */
		}
	      }
	    }
	  } else {
	    /*
	     *  Resolve handles methods separately than functions
	     *  because it needs to check method parameters also.
	     */
	    if (interpreter_pass == method_pass) {
	      if ((m_sender -> tokentype == LABEL) && 
		  !m_sender -> obj) {
		if (!IS_DEFINED_LABEL (m_sender -> name) &&
		    !is_method_parameter 
		    (ms.messages, sender_idx)) {
		  if (is_possible_receiver_subexpr_postfix (&ms, sender_idx)) {
		    handle_subexpr_rcvr (sender_idx);
		    return NULL;
		  } else if (cvar_is_method_receiver (&ms, sender_idx)) {
		    method_call (sender_idx);
		    return NULL;
		  } else if (is_class_typecast (&ms, message_ptr)) {
		    return NULL;
		  } else if (has_typecast_form (&ms, prev_tok_ptr)) {
		    /* might need to be upgraded to is_typecast_expr,
		       which looks up actual types */
		    return NULL;
		  } else if (_hash_get (declared_method_names, M_NAME(m))) {
		    warning (m, "Method, \"%s\" cannot be resolved.  Waiting "
			     "until run time.", M_NAME(m));
		  } else {
#if defined(__APPLE__) && defined (__x86_64)
		    if (!strstr (m -> name, "__builtin_"))
		      /* parser pass skips these */
		      undefined_receiver_exception (message_ptr, sender_idx);
#else		    
		    undefined_receiver_exception (message_ptr, sender_idx);
#endif		    
		    return NULL;
		  }
		}  else { /* if (!IS_DEFINED_LABEL (m_sender -> name) &&...*/

		  /* 
		   *  Look for a shadowed C variable again, if we've gotten
		   *  this far.  All of these near-duplicate clauses could be
		   *  squirreled away in some file and catalogued, if we
		   *  can factor out the context any further.... 
		   *  But this runs minimally faster, anyway.  And the fn
		   *  really isn't that big, either.
		   *
		   *  Again, for now, the clause looks only for the syntax
		   *  of common C statements - parsing the syntax a little
		   *  is faster.
		   */
		  CVAR *c_1;
		  int i_1;
		  if (prev_tok_ptr != -1) {

		    m_prev_tok = ms.messages[prev_tok_ptr];

		    if (m_sender -> attrs & TOK_IS_DECLARED_C_VAR) {
		      if ((c_1 = get_global_var(M_NAME(m_sender))) != NULL) {
			if (IS_C_OP_TOKEN_NOEVAL (M_TOK(m_prev_tok)))
			  return M_VALUE_OBJ (m_prev_label);
		      }
		      if ((c_1 = get_local_var(M_NAME(m_sender))) != NULL) {
			if (IS_C_OP_TOKEN_NOEVAL (M_TOK(m_prev_tok)))
			  return M_VALUE_OBJ (m_prev_label);
		      }
		    }
		    if (M_TOK(m_prev_tok) == PERIOD ||
			M_TOK(m_prev_tok) == DEREF) {
		      if ((i_1 =
			   is_struct_member_tok (ms.messages,
						 sender_idx)) != 0) {
			return M_VALUE_OBJ (m_prev_label);
		      }
		    }
		  }

		  /* Okay, again, it's a ctalk expression (or it 
		   * wouldn't compile anyway).
		   */

		  if (is_possible_receiver_subexpr_postfix (&ms,sender_idx)) {
		    handle_subexpr_rcvr (sender_idx);
		    return NULL;
		  }

		}
	      } else {/* if ((m_sender -> tokentype == LABEL) ... */
		if (!m_sender -> obj && !m_prev_label -> obj) {
		  if (prev_tok_ptr != prev_label_ptr) {
		    m_prev_tok = ms.messages[prev_tok_ptr];
		    if (IS_CONSTANT_TOK(M_TOK(m_prev_tok))) {
		      if (cvar_expr_needs_translation (&ms, sender_idx)) {
			register_cvar_arg_expr_a 
			  (ms.messages, sender_idx);
			if (method_call_constant_tok_expr_a (ms.messages,
							     sender_idx)) {
			  return NULL;
			}
		      }
		    } else { /* if (CONSTANT_TOK(m_prev_tok))  */

		      /* An <array>[<idx>] <tok> expression */
		      if (is_aggregate_term_a (ms.messages, prev_tok_ptr)) {
			register_cvar_rcvr_expr_b 
			  (ms.messages, sender_idx);
			if (ctrlblk_pred)
			  resolve_ctrlblk_cvar_reg = true;
			else
			  resolve_ctrlblk_cvar_reg = false;
			if (method_call_subexpr_postfix_a (ms.messages,
							   sender_idx)) {
			  return NULL;
			} else {
			  /*
			   *  A <close paren> <math_op> expression
			   */
			  if (method_call_subexpr_postfix_a (ms.messages,
							     sender_idx)) {
			    return NULL;
			  } else {
			    /*
			     *  Mark a prefix method syntactically _unless_
			     *  it is a ! operator and it is the first token
			     *  of a contrl block predicate.  These we
			     *  can send verbatim to the output.
			     */
			    prefix_method_attr (ms.messages, message_ptr);
			  }
			} /* if (method_call_subexpr_postfix_a ... */
		      }  else { /* if (is_aggregate_term_a ... */
			if (method_call_subexpr_postfix_a (ms.messages,
							   sender_idx)) {
			  return NULL;
			} else {
			  /*
			   *  A <close paren> <math_op> expression
			   */
			  if (method_call_subexpr_postfix_a (ms.messages,
							     sender_idx)) {
			    return NULL;
			  } else {
			    /*
			     *  Again, mark a prefix method syntactically 
			     *  _unless_ it is a ! operator and it is the 
			     *  first token of a contrl block predicate.
			     */
			    prefix_method_attr (ms.messages, message_ptr);
			  }
			} /* if (method_call_subexpr_postfix_a ... */
		      } /* if (is_aggregate_term_a ... */
		    }  /* if (CONSTANT_TOK(m_prev_tok))  */
		  } else { /* if (prev_tok_ptr != prev_label_ptr) */
		    /*
		     *   If the previous label is a CVAR, and we're
		     *   in a receiver expression.
		     */
		    CVAR *prev_label_cvar;
		    if (((prev_label_cvar=get_local_var 
			  (M_NAME(m_prev_label)))
			 != NULL) ||
			((prev_label_cvar=get_global_var 
			  (M_NAME(m_prev_label)))
			 != NULL)) {
		      if (CVAR_AGGREGATE_DECL(prev_label_cvar)) {
			if (method_call_subexpr_postfix_b 
			    (ms.messages, sender_idx) == -2) {
			  constrcvr_unhandled_case_warning (ms.messages,
							    sender_idx);
			}
			return NULL;
		      } else {
			if (register_cvar_rcvr_expr_a (ms.messages,
						       sender_idx,
						       prev_label_cvar,
						       M_NAME(m_prev_label))){
			  int term_idx = scanforward (ms.messages,
						      ms.tok,
						      ms.stack_ptr,
						      SEMICOLON);
			  fileout ("\ndelete_method_arg_cvars ();\n",
				   0, term_idx - 1);
			  return NULL;
			} else { /* if (register_cvar_rcvr_expr_a */
			  if (set_cvar_rcvr_postfix_attrs_a 
			      (ms.messages,
			       sender_idx,
			       prev_label_ptr,
			       prev_label_cvar,
			       M_NAME(m_prev_label))){
			    return NULL;
			  }
			} /* if (register_cvar_rcvr_expr_a */
		      }
		    }
		  } /* if (prev_tok_ptr != prev_label_ptr) */
		} /* if (!m_sender -> obj && !m_prev_label -> obj)*/ 
	      } /* if ((m_sender -> tokentype == LABEL) ... */
	    } /* if (interpreter_pass == method_pass) */
	  }
	}
      }
      /* Eeech.... needs to be cleaned up, like the rest of this fn. */
      if (TOK_HAS_CLASS_TYPECAST (m_prev_label)) {
	return NULL;
      } else {
	return resolve_rcvr_is_undefined (m_prev_label, m);
      }
    }

    return m_prev_label -> obj;
  }

  m_sender -> obj = result_object;
  if (m_prev_label && IS_MESSAGE(m_prev_label)) 
    m_sender -> receiver_obj = m_prev_label -> obj;

  return result_object;

}
