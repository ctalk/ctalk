/* $Id: mcct.c,v 1.2 2021/02/21 15:22:36 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2017-2020 Robert Kiesling, rk3314042@gmail.com.
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
 *  Handles mixed C/Ctalk terms separated by math operators, but
 *  without C function calls (the compiler checks for templates
 *  elsewhere when evaluating these expressions).  The compiler
 *  translates the Ctalk terms, and passes the C terms directly to the
 *  output.  That also means that these expressions must evaluate to a
 *  valid C type; e.g., the term cannot evaluate to a string literal
 *  or another type that can't appear on either side of a math
 *  operator.  This is especially important when assigning to a C
 *  variable within a control block expression, and it also saves a
 *  lot of processing whenever C variables appear in a control
 *  structure.  These functions are called from ctrblk_pred_rt_expr*
 *  in ifexpr.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "objtoc.h"

extern CTRLBLK *ctrlblks[MAXARGS + 1];  /* Declared in control.c. */
extern int ctrlblk_ptr;

extern bool argblk;          /* Declared in argblk.c.                   */

extern I_PASS interpreter_pass;    /* Declared in rtinfo.c.               */

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj; /* Declared in primitives.c */

/* possible results for ctblk_handle_mixed_c_ctalk tokens */
typedef enum {
  mcct_null = 0,
  mcct_continue_undef,
  mcct_whitespace,
  mcct_continue,
  mcct_arglist_continue,
  mcct_break,
} MCCT_RESULT;

#define PRE_OR_POSTFIX_INC_OR_DEC(messages, lookahead, lookback)	\
  (M_TOK(messages[lookahead]) == INCREMENT ||				\
   M_TOK(messages[lookahead]) == DECREMENT ||				\
   M_TOK(messages[lookback]) == INCREMENT ||				\
   M_TOK(messages[lookback]) == DECREMENT)


static int mcct_prev_term_idx (MESSAGE_STACK messages,
			       int term_idx, int expr_start_idx) {
  int i;
  for (i = term_idx + 1; i <= expr_start_idx; i++) {

    switch (M_TOK(messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case LABEL: case METHODMSGLABEL: case INTEGER: case LONG:
      case LONGLONG: case FLOAT: case LITERAL_CHAR: case LITERAL:
	return i;
	break;
      }
  }
  return -1;
}
			       
/* A "term method" here is the method LABEL or METHODMSGLABEL following
   a receiver label. Math operators get handled as C operators - we
   don't need to translate them. */

static bool mcct_term_method (MESSAGE_STACK messages,
			      int method_tok_idx,
			      int *arglist_end_out) {
  int lookback;
  METHOD *method;

  /* Check the actual token, in case resolve () has already
     assigned a METHODMSGLABEL token type to an operator. */
  if (IS_C_OP_CHAR(messages[method_tok_idx]->name[0]))
    return false;
  if ((lookback = prevlangmsg (messages, method_tok_idx)) != ERROR) {
    if (((method = get_instance_method (messages[lookback],
					messages[lookback] -> obj,
					M_NAME(messages[method_tok_idx]),
					ANY_ARGS, FALSE)) != NULL) ||
	((method = get_class_method (messages[lookback],
				     messages[lookback] -> obj,
				     M_NAME(messages[lookback]),
				     ANY_ARGS, FALSE)) != NULL)) {
      if (method -> n_params) {
	*arglist_end_out = method_arglist_limit_2
	  (messages,
	   method_tok_idx,
	   nextlangmsg (messages, method_tok_idx),
	   method -> n_params,
	   method -> varargs);
      } else {
	*arglist_end_out = method_tok_idx;
      }
      if (M_TOK(messages[method_tok_idx]) == LABEL)
	messages[method_tok_idx] -> tokentype = METHODMSGLABEL;
      return true;
    } else if (IS_OBJECT(messages[lookback] -> obj) &&
	       is_method_proto (messages[lookback] -> obj -> __o_class,
				M_NAME(messages[method_tok_idx]))) {
      method_from_prototype (M_NAME(messages[method_tok_idx]));
      if (((method = get_instance_method (messages[lookback],
					  messages[lookback] -> obj,
					  M_NAME(messages[method_tok_idx]),
					  ANY_ARGS, FALSE)) != NULL) ||
	  ((method = get_class_method (messages[lookback],
				       messages[lookback] -> obj,
				       M_NAME(messages[lookback]),
				       ANY_ARGS, FALSE)) != NULL)) {
	if (method -> n_params) {
	  *arglist_end_out = method_arglist_limit_2
	    (messages,
	     method_tok_idx,
	     nextlangmsg (messages, method_tok_idx),
	     method -> n_params,
	     method -> varargs);
	} else {
	  *arglist_end_out = method_tok_idx;
	}
	if (M_TOK(messages[method_tok_idx]) == LABEL)
	  messages[method_tok_idx] -> tokentype = METHODMSGLABEL;
	return true;
      }
    }
  }
  return false;
}

static inline bool chmcct_is_param (MESSAGE_STACK messages,
				    METHOD *method, int idx) {
  int i;
  MESSAGE *m;

  m = messages[idx];

  for (i = 0; i < method -> n_params; ++i) {
    if (str_eq (M_NAME(m), method -> params[i] -> name)) {
      /* We may need to be careful with this, if a param class
	 isn't fully parsed yet. */
      get_new_method_param_instance_variable_series
	(get_class_object (method -> params[i] -> class),
	 messages, idx, get_stack_top (messages));
      return true;
    }
  }
  return false;
}

static bool mcct_is_fn_param (char *name) {
  CFUNC *fn;
  CVAR *param;
  char *param_basename, *f;
  if (interpreter_pass != parsing_pass)
    return false;
  if (argblk) {
    if ((param_basename = strchr (name, '_')) != NULL)
      ++param_basename;
    else
      param_basename = name;
  } else {
    param_basename = name;
  }
  f = get_fn_name ();
  if (*f) {
    if ((fn = get_function (f)) != NULL) {
      for (param = fn -> params; param; param = param -> next) {
	if (str_eq (param -> name, param_basename))
	  return true;
      }
    }
  }
  return false;
}

/* term_start_idx will be -1 if we are only checking for trailing
   arguments */
MCCT_RESULT mcct_check_token (MESSAGE_STACK messages, int i,
			      int pred_end_idx, METHOD *method,
			      int *n_subscripts,
			      int *arglist_end, int term_start_idx) {
  MESSAGE *tok = messages[i];
  OBJECT *o, *var;
  CVAR *c;
  int lookahead, lookahead_2, lookback, i_2,
    n_parens;

  if (M_ISSPACE(tok))
      return mcct_whitespace;
    if (IS_OBJECT(tok -> obj)) {
      if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	if ((M_TOK(messages[lookahead]) == LABEL) &&
	    !IS_OBJECT(messages[lookahead] -> obj)) {
	  /* i.e., we haven't already looked for and found an instance
	     variable label series. */
	  if ((messages[lookahead] -> obj =
	       get_instance_variable (M_NAME(messages[lookahead]),
				      (IS_OBJECT(tok -> obj -> instancevars) ?
				       tok -> obj -> instancevars -> __o_classname :
				       tok -> obj -> __o_classname),
				      false)) != NULL) {
	    messages[lookahead] -> receiver_msg = tok;
	    messages[lookahead] -> receiver_obj = tok -> obj;
	    messages[lookahead] -> attrs |= OBJ_IS_INSTANCE_VAR;
	  }
	}
      }
      return mcct_continue;
    } else if ((o = get_object (M_NAME(tok), NULL)) != NULL) {
      tok -> obj = o;
      return mcct_continue;
    } else if (tok -> attrs & OBJ_IS_INSTANCE_VAR) {
      return mcct_continue;
    } else if (method &&
	       chmcct_is_param (messages, method, i)) {
      return mcct_continue;
    } else if (tok -> attrs & TOK_IS_DECLARED_C_VAR) {
      if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	if (M_TOK(messages[lookahead]) == LABEL) {
	  if (!tok -> obj) {
	    if (((c = get_local_var (M_NAME(tok))) != NULL) ||
		(( c = get_global_var (M_NAME(tok))) != NULL) ||
		((c = get_var_from_cvartab_name (M_NAME(tok))) != NULL)) {
	      tok -> obj = create_object 
		(basic_class_from_cvar 
		 (messages[i], c, 0),
		 M_NAME(tok));
	      if (tok -> obj -> __o_class) {
		if (is_method_name(M_NAME(messages[lookahead])) ||
		    is_method_proto (tok->obj->__o_class,
				     messages[lookahead] -> name)) {
		  messages[lookahead] -> receiver_msg = tok;
		  messages[lookahead] -> receiver_obj = tok -> obj;
		  messages[lookahead] -> tokentype = METHODMSGLABEL;
		  return mcct_continue;
		}
	      }
	    }
	  }
	}
      }
      return mcct_continue_undef;
    } else if (tok -> attrs & TOK_SELF) {
      if (!method && !argblk)
	warning (tok, "\"self\" used outside of a method or argument block.");
      if (!tok -> obj) {
	tok -> obj = instantiate_self_object ();
	if ((lookahead_2 = nextlangmsg (messages, i)) != ERROR) {
	mcct_instvar_lookahead:
	  if (M_TOK(messages[lookahead_2]) == LABEL) {
	    if (is_instance_variable_message (messages, lookahead_2)) {
	      if ((var = get_instance_variable_series
		   (tok -> obj,
		    messages[lookahead_2],
		    lookahead_2,
		    get_stack_top (messages))) != NULL) {
		messages[lookahead_2] -> obj = var;
		messages[lookahead_2] -> receiver_obj = tok -> obj;
		messages[lookahead_2] -> receiver_msg = tok;
		messages[lookahead_2] -> attrs |= OBJ_IS_INSTANCE_VAR;
		if ((lookahead_2 = nextlangmsg (messages, lookahead_2))
		    != ERROR) {
		  if ((lookahead_2 >= pred_end_idx) &&
		      (M_TOK(messages[lookahead_2]) == LABEL)) {
		    goto mcct_instvar_lookahead;
		  }
		}
	      }
	    }
	  }
	}
      }
      return mcct_continue;
    } else if (is_object_prefix (messages, i)) {
      return mcct_continue;
    } else if (tok -> attrs & TOK_SUPER) {
      if (!method && !argblk)
	warning (tok, "\"super\" used outside of a method or argument block.");
      /* Check for an instance or class var series. */
      if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	if (M_TOK(messages[lookahead]) == LABEL) {
	  if ((var = get_instance_variable_series
	       (rcvr_class_obj,
		messages[lookahead],
		lookahead,
		get_stack_top (messages))) != NULL) {
	    messages[lookahead] -> obj = var;
	  }
	}
      }
      return mcct_continue;
    } else if (mcct_term_method (messages, i, arglist_end)) {
      /* LABEL or METHODMSGLABEL tokens only. */
      return mcct_continue;
    } else if (IS_C_BINARY_MATH_OP(M_TOK(tok))) {
      if (*n_subscripts == 0) {
	return mcct_break;
      } else {
	return mcct_continue_undef;
      }
    } else if (M_TOK(messages[i]) == CLOSEPAREN) {
      /* we might need a lookahead here, too, for aggregate terms. */
      if ((lookback = prevlangmsg (messages, i)) != ERROR) {
	if (IS_OBJECT(messages[lookback] -> obj)) {
	  return mcct_break;
	} else if (messages[lookback] -> attrs & TOK_SELF ||
		   messages[lookback] -> attrs & TOK_SUPER) {
	  return mcct_break;
	} else if (M_TOK(messages[lookback]) == INTEGER ||
		   M_TOK(messages[lookback]) == FLOAT ||
		   M_TOK(messages[lookback]) == LONG ||
		   M_TOK(messages[lookback]) == LONGLONG ||
		   M_TOK(messages[lookback]) == LITERAL ||
		   M_TOK(messages[lookback]) == LITERAL_CHAR) {
	  /* Check if we have a constant method argument with no paren
	     mismatches from the receiver at term_start_idx.  (This is
	     not used when we are only checking for trailing terms)
	     This also probably needs some more examples so we can do
	     more thorough paren checking. */
	  if (term_start_idx != -1) {
	    n_parens = 0;
	    for (i_2 = term_start_idx; i_2 > i; i_2--) {
	      if (M_TOK(messages[i_2]) == OPENPAREN) {
		++n_parens;
	      } else if (M_TOK(messages[i_2]) == CLOSEPAREN) {
		--n_parens;
	      }
	    }
	    if (n_parens == 0) {
	      return mcct_break;
	    }
	  }
	}
      }
      return mcct_continue_undef;
    } else if (M_TOK(messages[i]) == OPENPAREN) {
      if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	if (M_TOK(messages[lookahead]) == LABEL) {
	  if (IS_OBJECT(messages[lookahead] -> obj)) {
	    return mcct_break;
	  } else if (method && chmcct_is_param (messages, method, lookahead)) {
	    return mcct_break;
	  } else if (messages[lookahead] -> attrs & TOK_SUPER) {
	    return mcct_break;
	  } else if (messages[lookahead] -> attrs & TOK_SELF) {
	    return mcct_break;
	  }
	}
      }
      return mcct_continue_undef;
    } else if (M_TOK(tok) == ARRAYOPEN) {
      (*n_subscripts)++;
      return mcct_continue_undef;
    } else if (M_TOK(tok) == ARRAYCLOSE) {
      (*n_subscripts)--;
      return mcct_continue_undef;
    } else if ((M_TOK(tok) == METHODMSGLABEL) &&
	IS_C_OP_CHAR(tok -> name[0])) {
      return mcct_break;
    } else if ((M_TOK(tok) == INCREMENT) || (M_TOK(tok) == DECREMENT)) {
      if ((lookback = prevlangmsg (messages, i)) != ERROR) {
	if (IS_OBJECT(messages[lookback] -> obj)) {
	  return mcct_continue;
	} else if ((M_TOK(messages[lookback]) == METHODMSGLABEL) ||
		   (M_TOK(messages[lookback]) == M_TOK(tok))) {
	  /* Also checks for doubled postfix ops. */
	  warning (messages[i], "Postfix operator, \"%s,\" "
		   "following method, \"%s,\" might have no effect",
		   M_NAME(messages[i]),
		   M_NAME(messages[lookback]));
	  return mcct_continue;
	} else if (messages[lookback] -> attrs & TOK_SUPER) {
	  return mcct_continue;
	}
      }
      return mcct_break;
    } else if (i == pred_end_idx) {
      return mcct_break;
    } else {
      return mcct_continue_undef;
    }
    return mcct_break;
}

static int mcct_obj_term_end (MESSAGE_STACK messages, int term_start_idx) {
  int i, stack_top_idx, term_tok = term_start_idx;
  stack_top_idx = get_stack_top (messages);
  for (i = term_start_idx; i > stack_top_idx; i--) {
    switch (M_TOK(messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case LABEL:
      case INCREMENT:
      case DECREMENT:
      case DEREF:
	term_tok = i;
	break;
      case METHODMSGLABEL:
      /* these can be morphed into methods already due to 
	 operator precedence. */
	if (str_eq (M_NAME(messages[i]), "||") ||
	    str_eq (M_NAME(messages[i]), "&&")) {
	  return term_tok;
	} else {
	  term_tok = i;
	  continue;
	}
	break;
      default:
	goto obj_term_end_done;
	break;
      }
  }
 obj_term_end_done:
  return term_tok;
}

static bool mcct_object_term (MESSAGE_STACK messages, int term_start_idx,
			   int *term_end_idx) {
  if (is_object_or_param 
      (M_NAME(messages[term_start_idx]), NULL)) {
    *term_end_idx = mcct_obj_term_end (messages, term_start_idx);
    return true;
  } else if (messages[term_start_idx] -> attrs & TOK_SELF) {
    if (interpreter_pass != method_pass && !argblk) {
      warning (messages[term_start_idx], "\"self,\" used outside "
	       "of a method or argument block.");
      return false;
    } else {
      *term_end_idx = mcct_obj_term_end (messages, term_start_idx);
      return true;
    }
  } else if (messages[term_start_idx] -> attrs & TOK_SUPER) {
    *term_end_idx = mcct_obj_term_end (messages, term_start_idx);
    return true;
  } else if ((messages[term_start_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) ||
	     is_unary_minus (messages, term_start_idx)) {
    int lookahead;
    if ((lookahead = nextlangmsg (messages, term_start_idx)) != ERROR) {
      if (is_object_or_param (M_NAME(messages[lookahead]), NULL) ||
	  messages[lookahead] -> attrs & TOK_SELF ||
	  messages[lookahead] -> attrs & TOK_SUPER) {
	*term_end_idx = mcct_obj_term_end (messages, lookahead);
	return true;
      }
    }
  }

  return false;
}

CVAR *ifexpr_is_cvar_not_shadowed (MESSAGE_STACK messages, int label_idx) {
  CVAR *c;
  char *var_or_obj_name = M_NAME(messages[label_idx]);
  if ((((c = get_local_var (var_or_obj_name)) != NULL) ||
       ((c = get_global_var (var_or_obj_name)) != NULL)) &&
      !get_local_object (var_or_obj_name, NULL)) {
    if (get_local_object (var_or_obj_name, NULL) &&
	(c -> scope & GLOBAL_VAR)) {
      error (messages[label_idx], "Redefinition of label, \"%s.\"",
	     var_or_obj_name);
    }
    return c;
  }
  return NULL;
}

static bool mcct_cvar_or_argblk_tab_term (MESSAGE_STACK messages,
					  int label_idx,
					  int *end_idx) {
  int lookahead, lookback;
  MESSAGE *m = messages[label_idx];
  CVAR *c;
  char tmpfmtbuf[MAXLABEL * 2], subscriptbuf[MAXMSG];
  int i, struct_terminal;
  char castbuf[MAXLABEL];

  if (argblk && m -> name[0] == '*') {
    if ((c = get_var_from_cvartab_name (m -> name)) == NULL) {
      return false;
    } else {
      if (((lookahead = nextlangmsg (messages, label_idx)) != ERROR) &&
	  ((lookback = prevlangmsg (messages, label_idx)) != ERROR)) {
	if (M_TOK(messages[lookahead]) == ARRAYOPEN) {
	  *end_idx = last_subscript_tok (messages, label_idx,
					 get_stack_top (messages));
	  if (lookahead - *end_idx > 3) {
	    char err_buf[256];
	    toks2str (messages, label_idx, *end_idx, err_buf);
	    warning (messages[label_idx],
		     "Complex subscripts and expressions are not (yet) "
		     "supported "
		     "in this context.\n\n\t%s\n\n",
		     err_buf);
	  }
	  if (c -> type_attrs & CVAR_TYPE_INT) {
	    /* ignore byte alignment as long as we're on int-compatible
	       machines: i.e. use pointer math: 
	       *my_vartab_var[3] => *my_vartab_var+3
	       */
	    strcpy (messages[lookahead] -> name, "+");
	    messages[lookahead] -> tokentype = PLUS;
	    strcpy (messages[*end_idx] -> name, " ");
	    messages[*end_idx] -> tokentype = WHITESPACE;
	    /* Also add an express typecast to help prevent compiler
	       warnings; e.g., 
	       
	          (int)*my_vartab_var+3

	       For portability, we need to use an intermediate buffer,
	       castbuf.

	       These need a temporary buffer.
	    */
	    if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
	      strcatx (castbuf, "(long long int)",
		       messages[label_idx] -> name, NULL);
	    } else if (c -> type_attrs & CVAR_TYPE_LONG) {
	      strcatx (castbuf, "(long int)",
		       messages[label_idx] -> name, NULL);
	    } else {
	      strcatx (castbuf, "(int)",
		       messages[label_idx] -> name, NULL);
	    }
#ifdef SYMBOL_SIZE_CHECK
	    check_symbol_size (castbuf);
#endif	    
	    strcpy (messages[label_idx] -> name, castbuf);
	  }
	  if (PRE_OR_POSTFIX_INC_OR_DEC (messages, lookahead, *end_idx)) {
	      /* See the comment below.  If there's a subscript, we also
		 need to include that in the parens.  This needs an
		 intermediate buffer, too. */
	      toks2str (messages, lookahead, *end_idx, subscriptbuf);
	      strcatx (tmpfmtbuf, "(", M_NAME(messages[label_idx]),
		       subscriptbuf, ")", NULL);
#ifdef SYMBOL_SIZE_CHECK
	      check_symbol_size (tmpfmtbuf);
#endif	    
	      strcpy (messages[label_idx] -> name, tmpfmtbuf);
	      for (i = lookahead; i >= *end_idx; i--) {
		messages[i] -> name[0] = ' '; messages[i] -> name[1] = '\0';
		M_TOK(messages[i]) = WHITESPACE;
	      }
	  }
	} else if (PRE_OR_POSTFIX_INC_OR_DEC(messages,
					     lookahead, lookback) ||
		   (M_TOK(messages[lookahead]) == PERIOD)) {
	  /* If we have a ++ or -- before or after the label (with a
	     pointer op), or a . after the label, wrap the label in parens. 
	     This also needs a temporary format buffer. */
	  strcatx (tmpfmtbuf, "(", M_NAME(messages[label_idx]), ")", NULL);
#ifdef SYMBOL_SIZE_CHECK
	  check_symbol_size (tmpfmtbuf);
#endif	    
	  strcpy (messages[label_idx] -> name, tmpfmtbuf);
	  *end_idx = label_idx;
	} else {
	  *end_idx = label_idx;
	}
      } else {
	*end_idx = label_idx;
      }
      m -> attrs |= TOK_IS_DECLARED_C_VAR;
      m -> attr_data = *end_idx;
      return true;
    }
  } else if ((bool)ifexpr_is_cvar_not_shadowed (messages, label_idx)) {
    if ((lookahead = nextlangmsg (messages, label_idx)) != ERROR) {
      switch (M_TOK(messages[lookahead]))
	{
	case ARRAYOPEN:
	  *end_idx =
	    last_subscript_tok
	    (messages, label_idx,
	     get_stack_top (messages));
	  break;
	case PERIOD:
	case DEREF:
	  if ((struct_terminal =
	       struct_end (messages,
			   label_idx,
			   get_stack_top (messages))) > 0) {
	    *end_idx = struct_terminal;
	  } else {
	    *end_idx = label_idx;
	  }
	  break;
	default:
	  *end_idx = label_idx;
	  break;
	}
    } else {
      *end_idx = label_idx;
    }
    m -> attrs |= TOK_IS_DECLARED_C_VAR;
    m -> attr_data = *end_idx;
    return true;
  }
  return false;
}

static bool mcct_cvar_assignment (MESSAGE_STACK messages, int idx) {
  int lookahead, end_idx;
  if (M_TOK(messages[idx]) != LABEL)
    return false;
  if (mcct_cvar_or_argblk_tab_term (messages, idx, &end_idx)) {
    if ((lookahead = nextlangmsg (messages, end_idx)) != ERROR) {
      if (IS_C_ASSIGNMENT_OP(M_TOK(messages[lookahead]))) {
	return true;
      }
    }
  }
  return false;
}

bool is_mcct_expr (MESSAGE_STACK messages, int idx,
		   int *end_idx_ret,
		   int *operand_idx_ret) {
  int tok_start_idx;
  int i;
  int lookahead;
  int term_end_idx  = idx;  /* prevent a warning message */
  bool have_c_term, have_object_term;

  if (*operand_idx_ret != -1) {
    /* We already have the stack index of the first token that's an object. */
    *end_idx_ret = idx;
    return true;
  }

  if (messages == message_stack ()) {
    tok_start_idx = C_CTRL_BLK -> pred_start_ptr;
  } else if (messages == method_message_stack ()) {
      tok_start_idx = N_MESSAGES;
  } else {
    char buf[MAXMSG];
    sprintf (buf, "%s", "Error: Unimplemented message stack in "
	     "is_mcct_expr.\n");
    _error (buf);
  }

  /* Anything with a function call or a literal string as a
     term (which isn't valid C) is not in this class of
     expression. */
  for (i = tok_start_idx - 1; i > C_CTRL_BLK -> pred_end_ptr; i--) {

    switch (M_TOK(messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case LABEL:
	if (get_function (M_NAME(messages[i])))
	  return false;
	if (mcct_cvar_assignment (messages, i)) {
	  /* always use a C variable natively when assigning to it in
	     this type of expression (but only after checking for a
	     function). */
	  /* end_idx_ret is set to -1 in ctrlblk_pred_rt_expr_self
	     before calling this function - probably should be set
	     first elsewhere, in case we have some wierd assignments
	     in if predicates */
	  if (*end_idx_ret == -1)
	    *end_idx_ret = i;
	  *operand_idx_ret = i;
	  return true;
	}
	break;
      case LITERAL:
	return false;
	break;
      case CLOSEPAREN:
	/* an expression like
	 *
	 *   (s1 + s2) length
	 *
	 * needs too much state saving for this version of
	 * mcct_check_token.
	 */
	if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	  if (M_TOK(messages[lookahead]) == LABEL ||
	      M_TOK(messages[lookahead]) == METHODMSGLABEL)
	    return false;
	}
	break;
      case DEREF:
	/* same with struct members */
	return false;
	break;
      }
  }
  
  have_c_term = false;
  have_object_term = false;
  *operand_idx_ret = -1;
  for (i = tok_start_idx - 1; i > C_CTRL_BLK -> pred_end_ptr; i--) {
    if ((M_TOK(messages[i]) == LABEL) ||
	(messages[i] -> attrs & TOK_IS_PREFIX_OPERATOR) ||
	is_unary_minus (messages, i)) {
      if (messages[i] -> attrs & TOK_IS_DECLARED_C_VAR &&
	  messages[i] -> attr_data > 0) {
	/* i.e., the term has already been checked by
	   mcct_cvar_or_argblk_tab_term */
	have_c_term = true;
	i = messages[i] -> attr_data;
      } else if (mcct_cvar_or_argblk_tab_term (messages, i, &term_end_idx)) {
	i = term_end_idx;
	have_c_term = true;
      } else if (mcct_object_term (messages, i, &term_end_idx)) {
	i = term_end_idx;
	if (*operand_idx_ret == -1) {
	  *operand_idx_ret = i;
	}
	have_object_term = true;
      }
    } else if (M_TOK(messages[i]) == METHODMSGLABEL) {
      *operand_idx_ret = prevlangmsg (messages, i);
      have_object_term = true;
    }

    if (have_c_term && have_object_term) {
      /* This lets the caller skip over the remainder of
	 the expression in the first pass, not needed
	 otherwise. */
      *end_idx_ret = C_CTRL_BLK -> pred_end_ptr + 1;
      return true;
    }
  }

  /* This fn can be called multiple times, so make sure we don't just 
     return true on the next call (see above). */
  *operand_idx_ret = -1;
  return false;
}

static void mcct_comma_op_buf (char *s) {
  char *p;
  while ((p = index (s, ';')) != NULL) *p = ',';
  while ((p = index (s, '\n')) != NULL) *p = ' ';
}

/*
 *  Checks for an expression like:
 *
 *   <obj> == <cvar>
 *
 *  where <cvar> is an OBJECT *, so we don't need a
 *  further object-to-C translation.
 */
static bool mcct_OBJECT_ptr_context (MESSAGE_STACK messages,
				int expr_last_tok_idx) {
  int next_tok, next_tok_2;
  CVAR *c;
  if ((next_tok = nextlangmsg (messages, expr_last_tok_idx))
      != ERROR) {
    if (IS_C_RELATIONAL_OP(M_TOK(messages[next_tok]))) {
      if ((next_tok_2 = nextlangmsg (messages, next_tok)) != ERROR) {
	if (messages[next_tok_2] -> attrs & TOK_IS_DECLARED_C_VAR) {
	  if (((c = get_local_var (M_NAME(messages[next_tok_2]))) != NULL) ||
	      ((c = get_global_var (M_NAME(messages[next_tok_2]))) != NULL) ||
	      ((c = get_var_from_cvartab_name (M_NAME(messages[next_tok_2])))
	       != NULL)) {
	    if (c -> n_derefs == 1 && str_eq (c -> type, "OBJECT")) {
	      return true;
	    }
	  }
	}
      }
    }
  }
  return false;
}

static bool last_obj_expr (MESSAGE_STACK messages, int term_end_idx) {
  int i, lookahead, lookback;
  CVAR *c;
  OBJECT *o;
  for (i = term_end_idx - 1; i >= C_CTRL_BLK -> pred_end_ptr; --i) {
    if (M_TOK(messages[i]) == LABEL) {
      if (((c = get_local_var (M_NAME(messages[i]))) != NULL) ||
	  ((c = get_global_var (M_NAME(messages[i]))) != NULL) ||
	  ((c = get_var_from_cvartab_name (M_NAME(messages[i]))) != NULL)) {
	  lookahead = nextlangmsg (messages, i);
	  lookback = prevlangmsg (messages, i);
	  if (mcct_is_fn_param (M_NAME(messages[i]))) {
	    return false;
	  } else if (!IS_C_OP(M_TOK(messages[lookahead])) &&
		     !IS_C_OP(M_TOK(messages[lookback])) &&
		     !get_object (M_NAME(messages[i]), NULL) &&
		     !(messages[i] -> attrs & OBJ_IS_INSTANCE_VAR)) {
	    return false;
	  }
      } else if ((o = get_object (M_NAME(messages[i]), NULL)) != NULL) {
	/* We might not need to check for instancevars or method
	   labels here. */
	return false;
      } else {  /* a method label following a CVAR */
	lookback = prevlangmsg (messages, i);
	if ((M_TOK(messages[i]) == LABEL) &&
	    (messages[lookback] -> attrs & TOK_IS_DECLARED_C_VAR)) {
	  return false;
	}
      }
    }
  }
  return true;
}

static bool var_rep (MESSAGE_STACK messages, int idx,
		     int cvar_tok_idx) {
  int i, lookahead, lookback;
  CVAR *c; 
  if (cvar_tok_idx == 0)
    return false;
  for (i = idx - 1; i >= C_CTRL_BLK -> pred_end_ptr; --i) {
    if (M_TOK(messages[i]) == LABEL) {
      if (messages[i] -> attrs & TOK_IS_DECLARED_C_VAR) {
	if (((c = get_local_var (M_NAME(messages[i]))) != NULL) ||
	    ((c = get_global_var (M_NAME(messages[i]))) != NULL) ||
	    ((c = get_var_from_cvartab_name (M_NAME(messages[i]))) != NULL)) {
	  if (str_eq (M_NAME(messages[cvar_tok_idx]), c -> name)) {
	    return true;
	  } else {
	    /* this is the same as in mcct_check_token */
	    if (!IS_OBJECT(messages[i] -> obj)) {
	      lookahead = nextlangmsg (messages, cvar_tok_idx);
	      lookback = prevlangmsg (messages, cvar_tok_idx);
	      if (!IS_C_OP(M_TOK(messages[lookahead])) &&
		  !IS_C_OP(M_TOK(messages[lookback]))) {
		messages[i] -> obj = create_object
		  (basic_class_from_cvar (messages[i], c, 0),
		   M_NAME(messages[i]));
	      }
	      if ((M_TOK(messages[lookahead]) == LABEL) ||
		  (M_TOK(messages[lookahead]) == METHODMSGLABEL)) {
		if (is_method_name (M_NAME(messages[lookahead])) ||
		    is_method_proto (messages[i] -> obj -> __o_class,
				     messages[lookahead] -> name)) {
		  messages[lookahead] -> receiver_msg = messages[i];
		  messages[lookahead] -> receiver_obj =
		    messages[i] -> obj;
		  messages[lookahead] -> tokentype = METHODMSGLABEL;
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

#define DCE_KEEPSTR(i) ((i) ? "1" : "0")
#define DCE_KEEPSTR_DEL_CVARS(i) ((i) ? "3" : "2")
/* 
 *  Same as fmt_default_ctrlblk_expr (rt_expr.c), but it returns the 
 *  register CVAR calls in a buffer of their own.  It also checks
 *  for argblk CVARs.
 */
static char *mcct_fmt_default_ctrlblk_expr (MESSAGE_STACK messages,
					    int start_idx, 
					    int end_idx, int keep,
					    char *expr_buf_out,
					    char *reg_buf_out,
					    bool *vars_occur_again,
					    bool *cvar_reg) {
  char exprbuf[MAXMSG], *c99name,
    *cfn_buf, expr_buf_tmp[MAXMSG],
    cvar_buf[MAXMSG];
  int i, cvar_tok_idx = 0;
  MESSAGE *m;
  CVAR *cvar;
  CFUNC *fn;
  bool have_pattern = false;

  *reg_buf_out = 0;
  *vars_occur_again = *cvar_reg = false;

  for (i = start_idx, *expr_buf_out = 0; i >= end_idx; i--) {

    m = messages[i];

    if (m && M_ISSPACE(m)) continue;

    if (m -> attrs & TOK_IS_DECLARED_C_VAR) {
      if (!(m -> attrs & TOK_CVAR_REGISTRY_IS_OUTPUT)) {
	if (((cvar = get_local_var (M_NAME(m))) != NULL) ||
	    ((cvar = get_global_var_not_shadowed (M_NAME(m)))!= NULL)) {
	  if ((cvar -> attrs & CVAR_ATTR_FN_DECL) ||
	      (cvar -> attrs & CVAR_ATTR_FN_PTR_DECL) ||
	      (cvar -> attrs & CVAR_ATTR_FN_PROTOTYPE)) {
	    /*
	     *  First check if we have a template for the function.
	     *  If so, insert the template in the output, and if 
	     *  necessary, register the template in the initialization
	     *  (though now the function registration by clib_fn_expr ()
	     *  is handled earlier in ctrlblk_pred_rt_expr ()).
	     *  If there's no template, issue a warning.
	     *
	     *  The function should be able to simply do a token 
	     *  replacement here, but later, it might be necessary
	     *  to set the value object, if we need to rescan the
	     *  message.
	     */
	    if ((c99name = c99_name (M_NAME(m), FALSE)) != NULL) {
	      if ((cfn_buf = clib_fn_rt_expr (messages, i)) != NULL) {
		__xfree (MEMADDR(m -> name));
		m -> name = strdup (cfn_buf);
	      }
	    }
	  } else {
	    strcatx2 (reg_buf_out,
		     fmt_register_c_method_arg_call
		     (cvar, cvar -> name, 
		      LOCAL_VAR,
		      cvar_buf), " ", NULL);
	    mcct_comma_op_buf (reg_buf_out);
	    *cvar_reg = true;
	    cvar_tok_idx = i;
	  }
	} else if ((cvar = get_var_from_cvartab_name (M_NAME(m))) != NULL) {
	  strcatx (reg_buf_out, fmt_register_argblk_cvar_from_basic_cvar
		   (messages, i, cvar, reg_buf_out), NULL);
	  mcct_comma_op_buf (reg_buf_out);
	  *cvar_reg = true;
	  cvar_tok_idx = i;
	} else if ((fn = get_function (M_NAME(m))) != NULL) {
	  if ((c99name = c99_name (M_NAME(m), FALSE)) != NULL) {
	    if ((cfn_buf = clib_fn_rt_expr (messages, i)) != NULL) {
	      __xfree (MEMADDR(m -> name));
	      m -> name = strdup (cfn_buf);
	    }
	  }
	}
      } /* if (!(m -> attrs & TOK_CVAR_REGISTRY_IS_OUTPUT)) */
    } else if (M_TOK(messages[i]) == PATTERN) {
      have_pattern = true;
    }
  }

  toks2str (messages, start_idx, end_idx, exprbuf);
  de_newline_buf (exprbuf);
  if ((cvar_tok_idx != 0) && !var_rep (messages, i, cvar_tok_idx)) {
    *vars_occur_again = false;
  } else if (last_obj_expr (messages, end_idx)) {
    *vars_occur_again = false;
  } else {
    *vars_occur_again = true;
  }
  if (have_pattern) {
    if (*cvar_reg) {
      strcatx (expr_buf_out, INT_TRANS_FN, " (",
	       fmt_pattern_eval_expr_str (exprbuf, expr_buf_tmp), 
	       ", ", DCE_KEEPSTR_DEL_CVARS(keep), ")",
	       NULL);
    } else {
      strcatx (expr_buf_out, INT_TRANS_FN, " (",
	       fmt_pattern_eval_expr_str (exprbuf, expr_buf_tmp), 
	       ", ", DCE_KEEPSTR(keep), ")",
	       NULL);
    }
  } else {
    if (mcct_OBJECT_ptr_context (messages, end_idx)) {
      strcatx (expr_buf_out, fmt_eval_expr_str (exprbuf, expr_buf_tmp), 
	       NULL);
    } else {
      if (*vars_occur_again) {
	strcatx (expr_buf_out, INT_TRANS_FN, " (",
		 fmt_eval_expr_str (exprbuf, expr_buf_tmp), 
		 ", ", DCE_KEEPSTR(keep), ")",
		 NULL);
      } else {
	strcatx (expr_buf_out, INT_TRANS_FN, " (",
		 fmt_eval_expr_str (exprbuf, expr_buf_tmp), 
		 ", ", DCE_KEEPSTR_DEL_CVARS(keep), ")",
		 NULL);
      }
    }
  }

  return expr_buf_out;
}

/*
  Note that this uses the actual predicate start and end
  indexes, not the next inner tokens.
*/
void handle_mcct_expr (MESSAGE_STACK messages,
			      int pred_start_idx,
			      int pred_end_idx) {
  int i, term_idx, lookback, obj_expr_end_idx;
  char exprbuf[MAXMSG] , expr_buf_tmp[MAXMSG];
  METHOD *new_method;
  bool term_has_objects;
  char expr_buf_tmp_2[MAXMSG], transbuf[MAXMSG];
  char reg_buf[MAXMSG], reg_buf_tmp[MAXMSG];
  int prev_term_idx;
  int n_subscripts = 0;
  int arglist_end = 0;
  bool vars_occur_again = false;
  bool cvar_reg = false;
  CVAR *c;

  *exprbuf = 0;
  *reg_buf = 0;

  if (interpreter_pass == method_pass) {
    new_method = new_methods[new_method_ptr + 1] -> method;
  } else {
    new_method = NULL;
  }

  term_idx = pred_start_idx;
  lookback = pred_start_idx;

  obj_expr_end_idx = pred_start_idx;
  term_has_objects = false;

 mcct_parse_term:
  for (i = term_idx; i >= pred_end_idx; i--) {

    switch (mcct_check_token (messages, i, pred_end_idx, new_method,
			      &n_subscripts, &arglist_end, term_idx))
      {
      case mcct_whitespace:
	continue;
	break;
      case mcct_continue:
	/* this is in case we get to the end of the predicate expression
	   without finding a trailing expression. */
	obj_expr_end_idx = i;
	lookback = i;
	term_has_objects = true;
	continue;
	break;
      case mcct_arglist_continue:
	obj_expr_end_idx = i;
	lookback = i;
	term_has_objects = true;
	i = arglist_end;
	break;
      case mcct_null:
      case mcct_continue_undef:
	lookback = i;
	continue;
	break;
      case mcct_break:
	obj_expr_end_idx = lookback;
	goto mcct_term_done;
	break;
      }
  }
 mcct_term_done:

  if (term_has_objects) {
    toks2str (messages, term_idx, obj_expr_end_idx, expr_buf_tmp);
    fmt_eval_expr_str (expr_buf_tmp, expr_buf_tmp_2);
    if ((lookback = prevlangmsg (messages, term_idx)) != ERROR) {
      if (IS_C_OP(M_TOK(messages[lookback]))) {
	if ((prev_term_idx = mcct_prev_term_idx (messages,
						 term_idx + 1,
						 C_CTRL_BLK->pred_start_ptr))
	    != ERROR) {
	  switch (M_TOK(messages[prev_term_idx]))
	    {
	    case LABEL:
	      if (((c = get_local_var (M_NAME(messages[prev_term_idx])))
		   != NULL) ||
		  ((c = get_global_var (M_NAME(messages[prev_term_idx])))
		   != NULL)) {
		/* we don't use the output of 
		   mcct_fmt_default_ctrlblk_expr here,
		   only the CVAR registration calls if any
		   and the bools. */
		mcct_fmt_default_ctrlblk_expr 
		  (messages, term_idx, obj_expr_end_idx, TRUE,
		   expr_buf_tmp,
		   reg_buf_tmp, &vars_occur_again, &cvar_reg);
		if (cvar_reg)
		  strcatx2 (reg_buf, reg_buf_tmp, NULL);
		if (vars_occur_again) {
		  fmt_rt_return (expr_buf_tmp_2,
				 basic_class_from_cvar (messages[i],
							c, c -> n_derefs),
				 OBJTOC_OBJECT_DELETE, transbuf);
		} else {
		  fmt_rt_return (expr_buf_tmp_2,
				 basic_class_from_cvar (messages[i],
							c, c -> n_derefs),
				 OBJTOC_OBJECT_DELETE|OBJTOC_DELETE_CVARS,
				 transbuf);
		}
		strcat (exprbuf, transbuf);
		term_has_objects = false;
		goto mcct_expr_formatted;
	      } else if ((c = get_var_from_cvartab_name
			  (M_NAME(messages[prev_term_idx]))) != NULL) {
		if ((c -> n_derefs == 1) && str_eq (c -> type, "OBJECT")) {
		  /* check for an OBJECT * as the first term...
		     then this is all we need */
		  strcatx2 (exprbuf, expr_buf_tmp_2, NULL);
		  term_has_objects = false;
		  goto mcct_expr_formatted;
		}
	      }
	      break;
	    case LITERAL:
	      /* doesn't occur in normal C. */
	      fmt_default_ctrlblk_expr
		(messages, pred_start_idx, pred_end_idx, TRUE, exprbuf);
	      fileout (exprbuf, FALSE, pred_start_idx);
	      for (i = pred_start_idx; i >= pred_end_idx; --i) {
		++messages[i] -> evaled;
		++messages[i] -> output;
	      }
	      return;
	      break;
	    }
	}
      }
    }
    *reg_buf_tmp = 0;
    strcat (exprbuf,
	    mcct_fmt_default_ctrlblk_expr 
	    (messages, term_idx, obj_expr_end_idx, TRUE,
	     expr_buf_tmp, reg_buf_tmp, &vars_occur_again, &cvar_reg));
    if (*reg_buf_tmp) {
      strcat (reg_buf, reg_buf_tmp);
    }
    term_has_objects = false;
  } else {
    strcat (exprbuf, toks2str
	    (messages, term_idx, lookback,
	     expr_buf_tmp));
  }

 mcct_expr_formatted:
  /* Check the trailing expression for objects, and repeat
     if needed. */
  for (i = obj_expr_end_idx - 1; i >= pred_end_idx; i--) {
    if (mcct_check_token (messages, i, pred_end_idx, new_method,
			  &n_subscripts, &arglist_end, -1)
	== mcct_continue) {
      term_idx = i;
      lookback = i;
      goto mcct_parse_term;
    } else {
      strcat (exprbuf, M_NAME(messages[i]));
    }
  }

  if (*reg_buf) {
    /* If we have a CVAR and an object, and the expression starts
       with a unary op, then it is already formatted into a 
       __ctalkEvalExpr call, so we don't have to insert a new
       paren here. We are going to need more examples here. */
    if (M_TOK(messages[pred_start_idx]) == OPENPAREN) {
      /* note that we also add our own paren, and omit the first
	 character of the original ctrblk expression here. */
      strcatx (expr_buf_tmp, "(", reg_buf, &exprbuf[1], NULL);
    } else {
      strcatx (expr_buf_tmp, reg_buf, exprbuf, NULL);
    }
    fileout (expr_buf_tmp, FALSE, pred_start_idx);
  } else {
    fileout (exprbuf, FALSE, pred_start_idx);
  }

  for (i = pred_start_idx; i >= pred_end_idx; --i) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }
}


