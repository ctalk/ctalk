/* $Id: method.c,v 1.3 2020/09/27 11:53:49 rkiesling Exp $ */

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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "bufmthd.h"

extern int error_line,      /* Declared in errorloc.c.                  */
  error_column;

extern I_PASS interpreter_pass;    /* Declared in main.c.               */
extern int nolinemarker_opt; 
extern int verbose_opt;
extern int warn_duplicate_name_opt;

extern bool argblk;          /* Declared in argblk.c.                    */
extern bool have_complex_arg_block;
extern int complex_arg_block_start;
extern int complex_arg_block_rcvr_ptr;  /* Set by method call, below.   */

extern int parser_output_ptr;
extern int last_fileout_stmt;

extern CTRLBLK              /* Declared in control.c                    */
*ctrlblks[MAXARGS + 1];
extern int ctrlblk_ptr;
extern bool ctrlblk_pred, ctrlblk_blk, ctrlblk_else_blk;
extern int for_init, for_term, for_inc;

extern char *ascii[8193];             /* from intascii.h */

extern int do_predicate;           /* Declared in loop.c.               */

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c.   */
extern int new_method_ptr;
extern bool formatted_fn_arg_trans;      /* declared in output.c   */

extern HASHTAB defined_instancevars; /* declared in primitives.c */

static inline OBJECT *class_from_object (OBJECT *) __attribute__((always_inline));

MESSAGE *m_messages[N_MESSAGES+1];
int m_message_ptr;

extern OBJECT *rcvr_class_obj;        /* Declared in lib/rtnwmthd.c and    */
                                      /* filled in by                      */
                                      /* new_instance|class_method ().     */

extern int subscript_object_expr;

static char errbuf[MAXMSG];

extern int 
n_th_arg_CVAR_has_unary_inc_prefix;   /* Declared in prefixop.c.           */
extern int n_th_arg_CVAR_has_unary_dec_prefix;
extern int n_th_arg_CVAR_has_unary_inc_postfix;
extern int n_th_arg_CVAR_has_unary_dec_postfix;

extern METHOD *register_c_var_method; /* Declared in reg_cvar.c */
extern OBJECT *register_c_var_receiver;

extern ARG_TERM arg_c_fn_terms[MAXARGS];
extern int arg_c_fn_term_ptr;

extern bool eval_arg_cvar_reg;

typedef struct {
  OBJECT *classobj;
  char *source_file;
  METHOD *method;
} IMPORTED_METHOD;

static IMPORTED_METHOD *imported_methods[MAXARGS+1]; /* Methods that are forward */
static int imported_method_ptr = MAXARGS;   /* declared if imported within a     */
                                     /* function.                         */

HASHTAB declared_method_names;       /* Initialized from init_method_stack*/
HASHTAB declared_method_selectors;   /* for now.                          */

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern DEFAULTCLASSCACHE *ct_defclasses; /* Declared in cclasses.c.       */

#define PARSER_BLOCK_LEVEL (parsers[current_parser_ptr]->block_level)

MESSAGE_STACK method_message_stack (void) { return m_messages; }

#define _MPUTCHAR(__m,__c) (__m)->name[0] = (__c); (__m)->name[1] = '\0';

int get_method_message_ptr (void) {return m_message_ptr; }

int ambiguous_arg_start_idx = -1,
  ambiguous_arg_end_idx = -1;

int ambiguous_operand_throw (MESSAGE_STACK messages, 
			     int idx, METHOD *method) {
  char s[MAXLABEL * 3];
  if ((idx == stack_start (messages)) &&
      !label_is_defined (messages, idx) &&
      (method -> n_params == 0) &&
      !strcmp (method -> returnclass, "Any")) {
    sprintf (s, 
"Warning: Message, \"%s,\" follows method, \"%s,\" which returns \"Any\" class object",
	     M_NAME(messages[idx]), method->name);
    __ctalkExceptionInternal (messages[idx], ambiguous_operand_x, 
			      s, 0);
    return TRUE;
  } else if ((idx == stack_start (messages)) &&
	     /* an instancevar following the method needs to be 
		checked expressly. */
	     _hash_get (defined_instancevars, M_NAME(messages[idx])) &&
	     (method -> n_params == 0) &&
	     str_eq (method -> returnclass, "Any")) {
    sprintf (s, 
"Warning: Message, \"%s,\" follows method, \"%s,\" which returns \"Any\" class object",
	     M_NAME(messages[idx]), method->name);
    __ctalkExceptionInternal (messages[idx], ambiguous_operand_x, 
			      s, 0);
    return TRUE;
  } else {
    ambiguous_arg_start_idx = ambiguous_arg_end_idx = -1;
  }
  return FALSE;
}

/*
 *  Called by eval_arg ().  This fn can fill in a CVAR in an argblk
 *  that has been initialized by argblk_cvar_is_fn_argument ()....
 */
void translate_argblk_cvar_arg_1 (MESSAGE *m_tok, CVAR *cvar) {
  char namebuf[MAXLABEL];
  int namebuf_len;
  char c[MAXLABEL];

  /* Don't cause an undefined label exception later on. */
  m_tok -> attrs |= TOK_IS_RT_EXPR;

  switch (interpreter_pass)
    {
    case method_pass:
      argblk_CVAR_name_to_msg (m_tok, cvar);
      fmt_register_argblk_c_vars_2 (m_tok, cvar, c);
      if (*c)
	fileout (c, 0, FRAME_START_IDX);
      break;
    case parsing_pass:
      namebuf_len = function_cvar_alias_basename (cvar, namebuf);
      if (namebuf_len > (MAXLABEL - 10)) {
	resize_message (m_tok, strlen (namebuf) + 10);
      }

      /* this is the same as in argblk_cvar_is_fn_argument ().  If it
	 needs to be any more complicated, it should have a fn of its
	 own. */
      if ((cvar -> type_attrs & CVAR_TYPE_CHAR) && (cvar -> n_derefs > 0)) {
	strcpy (m_tok -> name, namebuf);
      } else {
	strcatx (m_tok -> name, "*", namebuf, NULL);
      }

      fmt_register_argblk_c_vars_2 (m_tok, cvar, c);
      if (*c)
	fileout (c, 0, FRAME_START_IDX);
      break;
    default:
      break;
    }
}

static int ambiguous_operand_catch (MESSAGE_STACK messages, int idx,
				    METHOD *method) {
  I_EXCEPTION *i;
  if ((i = __ctalkTrapExceptionInternal (messages[idx])) != NULL) {
    if ((i -> _exception == ambiguous_operand_x) &&
	(method -> n_params == 0) &&
	!strcmp (method -> returnclass, "Any")) {
      __ctalkHandleInterpreterExceptionInternal (messages[idx]);
      return TRUE;
    }
  }
  return FALSE;
}

bool have_class_cast_on_unresolved_method (MESSAGE_STACK messages,
					   int method_message_ptr) {
  MESSAGE *m;
  m = messages[method_message_ptr];

  if (m -> receiver_msg && m -> receiver_msg -> receiver_msg) {
    if (m -> receiver_msg -> receiver_msg -> attrs & 
	TOK_IS_CLASS_TYPECAST)
      return true;
  }
  return false;
}

int have_class_cast_on_unresolved_expr (MESSAGE_STACK messages,
					       int method_message_ptr) {
  MESSAGE *m;
  m = messages[method_message_ptr];

  if ((m -> attrs & OBJ_IS_INSTANCE_VAR) ||
      (m -> attrs & VALUE_OBJ_IS_INSTANCE_VAR) ||
      (m -> attrs & OBJ_IS_CLASS_VAR) ||
      (m -> attrs & VALUE_OBJ_IS_CLASS_VAR)) {
    if (m -> receiver_msg && m -> receiver_msg -> receiver_msg) {
      if (m -> receiver_msg -> receiver_msg -> attrs & 
	  TOK_IS_CLASS_TYPECAST)
	return TRUE;
    }
  }
  return FALSE;
}

static char *ambiguous_operand_format (MESSAGE_STACK messages,
				       int start_idx, int end_idx,
				       char *output_buf) {
  char token_buf[MAXMSG], esc_buf_out[MAXMSG];
  int i;

  toks2str (messages, start_idx, end_idx, token_buf);
  de_newline_buf (token_buf);
  for (i = start_idx; i >= end_idx; i--) {
    messages[i] -> attrs |= TOK_IS_RT_EXPR;
    ++(messages[i]) -> evaled;
    ++(messages[i]) -> output;
  }
  strcatx (output_buf, EVAL_EXPR_FN, "(\"", 
	   escape_str_quotes (token_buf, esc_buf_out), 
	   "\")", NULL);
  return output_buf;
}

static void ambiguous_operand_reset (void) {
  /*
   *  __ctalkHandleInterpreterExceptionInternal, called above,
   *  clears the exception.
   */
  ambiguous_arg_start_idx = ambiguous_arg_end_idx = -1;
}

int is_struct_or_union (CVAR *c) {
  if (!IS_CVAR(c))
    return FALSE;
  if (!strcmp (c -> type, "struct") ||
      !strcmp (c -> type, "union") ||
      !strcmp (c -> qualifier, "struct") ||
      !strcmp (c -> qualifier, "union"))
    return TRUE;
  else
    return FALSE;
}

CVAR *get_struct_defn (char *type) {
  static CVAR *struct_defn;
  if (((struct_defn = get_local_struct_defn (type)) == NULL) &&
      ((struct_defn = get_global_struct_defn (type)) == NULL) && 
      ((struct_defn = have_struct (type)) == NULL)) 
    return NULL;
  else
    return struct_defn;
}

/*
 *  In case we have a struct member as a receiver that is 
 *  enclosed in parentheses, and a previous routine hasn't 
 *  elided them yet, elide them here.  It makes the rt expression
 *  formatting functions much simpler.
 */
static void struct_elide_parentheses_kludge (MESSAGE_STACK messages,
				      int rcvr_ptr,
				      int expr_end_idx) {
  int i, next_idx = -1, 
    next_idx_2, struct_end_idx, 
    n_parens = 0,
    paren_start_idx;
  if ((struct_end_idx = struct_end (messages,
					  rcvr_ptr,
					  get_stack_top (messages)))
      > 0) {
    n_parens = 0;
    if ((next_idx = nextlangmsg (messages, struct_end_idx)) != ERROR) {
      while (M_TOK(messages[next_idx]) == CLOSEPAREN) {
	if ((next_idx_2 = nextlangmsg (messages, next_idx)) == ERROR)
	  break;
	next_idx = next_idx_2;
	++n_parens;
      }
    }
  }
  paren_start_idx = rcvr_ptr;
  if (n_parens > 0) {
    while (n_parens--) {
      paren_start_idx = prevlangmsg (messages, paren_start_idx);
    }
  }
  
  if (next_idx != -1) {
    for (i = paren_start_idx; i > next_idx; i--) {
      if ((M_TOK(messages[i]) == OPENPAREN) ||
	  (M_TOK(messages[i]) == CLOSEPAREN)) {
	_MPUTCHAR(messages[i], ' ');
	messages[i] -> tokentype = WHITESPACE;
      }
    }
  }
}

CVAR *get_local_local_cvar (char *s) {
  PARSER *p;
  CVAR *c;
  if ((p = parser_at (parser_ptr ())) != NULL) {
    for (c = p -> cvars; c; c = c -> next) {
      if (!strcmp (c -> name, s)) {
	return c;
      }
    }
  }
  return NULL;
}

ARG_CLASS arg_class;

extern int fn_arg_expression_call;
int m_message_stack = FALSE;
OBJECT *eval_arg_obj = NULL;

/*
 *  Like resolve (), but only has the code for method arguments.
 */

OBJECT *resolve_arg (METHOD *rcvr_method, MESSAGE_STACK messages, 
		     int message_ptr, OBJECT *rcvr_class_obj_arg) {

  OBJECT *result_object = NULL;
  OBJECT *class_object; 
  OBJECT *instancevar_object;
  MESSAGE *m;      
  int msg_frame_top;
  int prev_label_ptr = ERROR;  /* Stack pointers to messages with      */
  int prev_method_ptr = ERROR; /*  resolved objects.                   */
  int prev_tok_ptr = ERROR;
  MESSAGE *m_prev_label = NULL;    /* Messages that refer to previously  */
  MESSAGE *m_prev_method = NULL;   /* resolved objects. Initialized here */
				   /* to avoid warnings.                 */
  int next_label_ptr;          /* Forward message references to check  */
  METHOD *method;
  MSINFO ms;
  ms.messages = messages;
  ms.stack_start = stack_start (messages);
  ms.stack_ptr = get_stack_top (messages);
  ms.tok = message_ptr;

  m = messages[message_ptr];

  if (!m || !IS_MESSAGE(m))
    _error ("Invalid message in resolve_arg.\n");

  if (m -> evaled)
    return NULL;

  /* Look for a class object. */

  msg_frame_top = message_frame_top_n (parser_frame_ptr ());

  if ((result_object = class_object_search (m -> name, FALSE)) != NULL) {
    m -> obj = result_object;
    /*
     *  Add a value method only if necessary.
     */
    if (!ctrlblk_pred)
      default_method (&ms);
    return result_object;
  }

  /*
   *   Check for a method parameter reference and create an
   *   object if necessary.
   */
  if (interpreter_pass == method_pass) {
    int i;
    METHOD *method;
    OBJECT_CONTEXT param_context;

    method = new_methods[new_method_ptr + 1] -> method;

    if (rcvr_class_obj_arg && IS_CONSTRUCTOR(method)) {
      if (method -> params[0]) {
	if (strcmp (M_NAME(messages[message_ptr]),
		    method->params[0]->name) &&
	    strstr (rcvr_class_obj_arg->__o_name, method->selector))
	  warning (messages[message_ptr], 
		   "Argument, \"%s,\" does not match constructor parameter.",
		   M_NAME(messages[message_ptr]));
      } else {
	warning (messages[message_ptr], 
		 "Argument, \"%s,\" does not match constructor parameter.",
		 M_NAME(messages[message_ptr]));
      }
    }

    if (method && IS_METHOD (method)) {
      for (i = 0; i < method -> n_params; i++) {
	if (!strcmp (m -> name, method -> params[i] -> name) &&
	    !(method->params[i]->attrs & PARAM_C_PARAM)) {

	  param_context = object_context (messages, message_ptr);

	  switch (param_context) 
	    {
	    case c_context:
	    case c_argument_context:
	      if (need_rt_eval (messages, message_ptr)) {
		ctrlblk_pred_rt_expr (messages, message_ptr);
	      } else {
		method_arg_accessor_fn (messages, message_ptr, 
					method -> n_params - i - 1,
					param_context,
					method -> varargs);
	      }
	      break;
	    case argument_context:
	      method_arg_accessor_fn (messages, message_ptr,
				      method -> n_params - i - 1,
				      param_context,
				      method -> varargs);
	      break;
	    default:
	      if (!IS_OBJECT (m -> obj)) {
		if ((class_object =
		     class_object_search (method -> params[i] -> class,
					  FALSE)) 
		    == NULL) 
		  /* An Object works equally well to define a parameter
		     class at this point. */
		  strcpy (method -> params[i] -> class,
			  OBJECT_CLASSNAME);
		result_object =
		  create_object_init (method -> params[i] -> class,
				      "Object",
				      method -> params[i] -> name,
				      NULLSTR);
		m -> obj = result_object;
		save_method_object (result_object);
		return result_object;
	      }
	      break;
	    }
	}
      }
    }
  }

  /* 
   *  Look for a global or local object.  
   */

  if ((result_object = get_object (m -> name, NULL)) != NULL) {
    if (IS_CONSTRUCTOR(rcvr_method) &&
	(result_object -> scope == GLOBAL_VAR) &&
	(frame_at (CURRENT_PARSER -> frame) -> scope == LOCAL_VAR)) {
      warning (m, "Object, \"%s,\" shadows a global object.", M_NAME(m));
      return NULL;
    } else {
      m -> obj = result_object;
      /*
       *  The default method should be handled by resolve (),
       *  if there is no method message, so we shouldn't need
       *  to check for it here.
       */
      return result_object;
    }
  }

  /* 
   *  Check for, "self," "super," "return," and 
   *  "returnObjectClass." If they don't translate 
   *  into run time code, they get elided here also.
   */

  if (m -> attrs & TOK_SELF &&
      need_rt_eval (messages, message_ptr)) {
    if ((new_method_ptr == MAXARGS) && !argblk) {
      warning (m, "Receiver, \"self,\" used within a C function.");
    }
    /* I.e., called from eval_arg, so only a subexpression is on 
       the stack. */
    if (messages != m_messages)
      ctrlblk_pred_rt_expr (messages, message_ptr);
  } else {
    if ((result_object = 
	 self_object (messages, message_ptr)) != NULL) {
      m -> obj = result_object;
      default_method (&ms);
      return result_object;
    }
  }

  if (fn_return (messages, message_ptr) == 0)
    return NULL;

  if (interpreter_pass == method_pass) {
    int i, next_label_ptr;
    if (!strcmp (m -> name, "returnObjectClass")) {
      if ((next_label_ptr = scanforward (messages, message_ptr - 1,
			 message_frame_top_n (parser_frame_ptr () - 1), 
					 LABEL)) == ERROR) {
	warning (m, "Syntax error");
      }

      strcpy (new_methods[new_method_ptr + 1] -> method -> returnclass,
	      message_stack_at(next_label_ptr) -> name);
      if ((next_label_ptr = scanforward (messages, message_ptr + 1,
			 message_frame_top_n (parser_frame_ptr () - 1), 
					 SEMICOLON)) == ERROR) {
	warning (m, "Syntax error");
      }
      for (i = message_ptr; i >= next_label_ptr; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }
      return NULL;
    }

    if (!strcmp (M_NAME(m), "noMethodInit")) {

      new_methods[new_method_ptr + 1] -> method -> no_init = TRUE;
      ++messages[message_ptr] -> evaled;
      ++messages[message_ptr] -> output;
      if ((i = nextlangmsg (message_stack (), message_ptr)) == ERROR)
	warning (m, "Syntax error");

      if (messages[i] -> tokentype != SEMICOLON)
	warning (m, "Syntax error");
      ++messages[i] -> evaled;
      ++messages[i] -> output;
      return NULL;
    }
  }

  /* Attempt to resolve methods and arguments. */

  /*
   *  Here we have to account for, "for," loop predicates.  Because
   *  they all occur within the same frame, we use the CTRLBLK structure
   *  indexes instead of the frame top.
   */
#define STACK_MSG_FRAME(__idx_exp) ((messages == m_messages) ? \
      N_MESSAGES : \
     ((messages == tmpl_message_stack ()) ? \
      N_MESSAGES : \
     ((messages == fn_message_stack ()) ? \
      P_MESSAGES : \
     ((messages == method_buf_message_stack ()) ? \
      P_MESSAGES : \
     ((messages == c_message_stack ()) ? \
      N_MESSAGES : \
     ((messages == var_message_stack ()) ? \
      N_MESSAGES : \
     (__idx_exp)))))))

  if (ctrlblk_pred) {
    if (C_CTRL_BLK -> stmt_type == stmt_for) {
      if (for_init)
	msg_frame_top = STACK_MSG_FRAME(C_CTRL_BLK -> for_init_start);
      if (for_term)
	msg_frame_top = STACK_MSG_FRAME(C_CTRL_BLK -> for_term_start);
      if (for_inc)
	msg_frame_top = STACK_MSG_FRAME(C_CTRL_BLK -> for_inc_start);
    } else {
      /*
       *  NOTE - It may be necessary to add conditions for other
       *  special loop cases here.
       */
      msg_frame_top = 
	STACK_MSG_FRAME(message_frame_top_n (parser_frame_ptr ()));
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
	msg_frame_top = STACK_MSG_FRAME(C_CTRL_BLK -> blk_start_ptr);
      } else {
	msg_frame_top = 
	  STACK_MSG_FRAME(message_frame_top_n (parser_frame_ptr ()));
      }
    } else {
      msg_frame_top = 
	STACK_MSG_FRAME(message_frame_top_n (parser_frame_ptr ()));
    }
  }

  /*
   *  The prev_*_ptr's are initialized at the start
   *  of the function.  Unless the function is using
   *  message_stack () (which it isn't currently), 
   *  these aren't needed.
   */
  if (message_ptr < N_MESSAGES) {
    prev_label_ptr = scanback (messages, message_ptr + 1,
			       msg_frame_top, LABEL);
    prev_method_ptr = scanback (messages, message_ptr + 1,
				msg_frame_top, METHODMSGLABEL);
    prev_tok_ptr = prevlangmsg (messages, message_ptr);
  }

  /*  If the label isn't resolved as an object above, it could be
   *  interpreted as an argument or method of an object in the
   *  predicate.
   */

  if (ctrlblk_blk) {
    if ((message_ptr <= C_CTRL_BLK -> blk_start_ptr) &&
	(message_ptr >= C_CTRL_BLK -> blk_end_ptr) &&
	(prev_label_ptr <= C_CTRL_BLK -> pred_start_ptr) &&
	(prev_label_ptr >= C_CTRL_BLK -> pred_end_ptr))
      return NULL;
  }

  if (prev_label_ptr != ERROR)
    m_prev_label = messages[prev_label_ptr];
  if (prev_method_ptr != ERROR)
    m_prev_method = messages[prev_method_ptr];

  if (prev_label_ptr == ERROR) {
    if ((message_ptr > N_MESSAGES) && 
	((prev_label_ptr = scanback (messages, message_ptr + 1,
 				    msg_frame_top, C_KEYWORD)) != -1)) {
      return NULL;
    } else {
      OBJECT_CONTEXT c;
      /* 
       *  Issue a warning only if we can determine that the 
       *  token appears in an object context.
       */
      if (is_c_identifier (m -> name) || is_c_keyword (m -> name))
	return NULL;
      c = object_context (messages, message_ptr);
      if ((c == argument_context) || (c == receiver_context))
	warning (m, "\"%s\" has unknown receiver.\n", m -> name);
      return NULL;
    }
  }

  /* The most recent reference is an object. */

  if (prev_label_ptr != ERROR && 
      ((prev_method_ptr == ERROR) || 
       (prev_label_ptr < prev_method_ptr))) {
    if (IS_OBJECT (m_prev_label -> obj)) {

      /* The previous reference is a class object. */
      if (IS_CLASS_OBJECT(m_prev_label->obj)) {
	if (((method = get_instance_method 
	      (m_prev_label, m_prev_label -> obj, m -> name, 
	       ERROR, FALSE)) != NULL) ||
	    ((method = get_class_method
	      (m_prev_label, m_prev_label -> obj, m -> name, 
	       ERROR, FALSE)) != NULL)) {
	  m -> receiver_msg = m_prev_label;
	  m -> receiver_obj = m_prev_label -> obj;
	  m -> tokentype = METHODMSGLABEL;
	} else {
	  sprintf (errbuf, METHOD_ERR_FMT, M_NAME(m), 
		   m_prev_label -> obj -> __o_classname);
	  __ctalkExceptionInternal (m, undefined_method_x, errbuf,0);
	  return NULL;
	}
      } else {
	
	/* 
	 *   The previous label refers to an object instance. 
	 *   Determine if this label refers to a method or 
	 *   an instance variable.  Give precedence to instance
	 *   variables.
	 */

	if ((class_object = m_prev_label -> obj -> __o_class) == NULL) {
	  if ((class_object = 
	       get_class_object (m_prev_label -> obj -> __o_classname))
	      == NULL)
	    error (m_prev_label, "Unknown class \"%s.\"", 
		   m_prev_label ->obj -> __o_classname);
	}
	/*
	 *   We still have to make sure that the method (at message_ptr)
	 *   immediately follows the receiver (at prev_label_ptr).
	 */
	if (prev_tok_ptr == prev_label_ptr) {
	  if (((instancevar_object = 
		__ctalkGetInstanceVariable (m_prev_label -> obj, m -> name,
					     FALSE)) != NULL) ||
	      ((method = get_instance_method (m, 
					      class_object, 
					      m -> name, 
					      ERROR, TRUE)) != NULL)) {
	    m -> receiver_msg = m_prev_label;
	    m -> receiver_obj = m_prev_label -> obj;
	    m -> tokentype = METHODMSGLABEL;
	  } else {
	    sprintf (errbuf, METHOD_ERR_FMT, M_NAME(m),
		     m_prev_label -> obj -> __o_classname);
	    __ctalkExceptionInternal (m, undefined_method_x, errbuf,0);
	    return NULL;
	  }
	}
      }
    }

    /* If there are no arguments, the method call code must be output
	immediately, because it also marks the ctalk statement not to
	be output.  Otherwise, the parser waits for the next pass, and
	the ctalk statement is output, which is non-parsable by GCC.
    */
    if ((next_label_ptr = nextlangmsg (messages, message_ptr))
	!= ERROR) {
      if (m -> tokentype == METHODMSGLABEL)
	method_call (message_ptr);
      return M_VALUE_OBJ (m_prev_label);
    }

    return m_prev_label -> obj;
  }
  
  /* The most recent reference is a method.  If not evaled, set the
     method args and call the method.
  */

  m -> obj = result_object;
  m -> receiver_obj = m_prev_label -> obj;

  if (prev_method_ptr < prev_label_ptr) {
    if ((method = get_instance_method 
	 (m_prev_label, m_prev_label -> obj, 
	  m_prev_method -> name, ERROR, TRUE)) != NULL) {

      if (!m -> evaled && !m_prev_label -> evaled) 
	method_args (method, message_ptr);
    }    
  }  

  /* ERROR here means that the statement has already been evaluated. */

  if (!m_prev_method -> evaled) {
    if (method_call (prev_method_ptr) == ERROR)
      return NULL;
  }

  return result_object;

}

/* 
 * Eval_receiver_token is like eval_arg (), but it also checks for
 * parameters and constants that are receivers.
 */

OBJECT *eval_receiver_token (MESSAGE_STACK messages, int idx, OBJECT *rcvr_class) {

  OBJECT *token_object = NULL;  /* Avoid a warning. */
  char buf[MAXLABEL];
  int r;
  int agg_var_end_idx;
  MESSAGE *m;

  m = messages[idx];
  eval_arg_cvar_reg = false;

  switch (m -> tokentype)
    {
    case LABEL:  /* Determine if the argument refers to an object,
		    a parameter, or a C variable.  If an object doesn't 
		    exist, create an object of the appropriate class. */
      if ((r = register_c_var (m, messages, idx,
			       &agg_var_end_idx)) == SUCCESS) {
	eval_arg_cvar_reg = true;
 	if ((token_object = get_object (m -> name,
 					rcvr_class -> __o_name)) == NULL) {
 	  token_object = create_object (rcvr_class -> __o_name, m -> name);
 	  strcpy (token_object -> __o_superclassname,
 		  rcvr_class -> __o_superclassname);
 	  token_object -> scope = ARG_VAR;
 	  strcpy (token_object -> __o_value, token_object -> __o_name);
 	}
      } else {  /* The message refers to an object. */
	/*
	 *  The order of precedence for resolving objects should be:
	 *  1. Local objects.
	 *  2. Method parameters, if parsing a method. Note that 
	 *     the new method is at the top of the new_methods[]
         *     stack in primitives.c, and not the method that 
	 *     gets passed to method_args (), which should be 
	 *     a superclass method.
	 *  3. Global objects.
	 *
	 *  TO DO - We should also check for parameter shadowing 
	 *  here.
	 */
 	if ((token_object =
 	     get_local_object (m -> name, rcvr_class -> __o_name))
	    == NULL) {
	  
 	  if (interpreter_pass == method_pass)
	    token_object = method_arg_object (m);

	  /*
	   *  If the object isn't global either, then create a new
	   *  object.
	   */
 	  if (!token_object) {
 	    if ((token_object =
 		 get_global_object (m -> name, rcvr_class -> __o_name))
 		== NULL) {
 	      token_object = 
 		create_object_init (rcvr_class -> __o_name,
 				    rcvr_class -> __o_superclassname,
 				    m -> name, m -> name);
 	      __ctalkSetObjectValue (token_object, m -> name);
 	      token_object -> scope = ARG_VAR;
	      save_method_object (token_object);
 	    }
 	  }
 	}
      }
      break;
      /* 
       *  Translate a C constant into an object of the appropriate
       *  type.  We shouldn't need to load a library for them.
       */
    case LITERAL:
    case INTEGER:
    case LONG:
    case DOUBLE:
    case LONGLONG:
      token_object = constant_token_object (m);
      __ctalkSetObjectValue (token_object, m -> name);
      token_object -> scope = ARG_VAR;
      break;
    case LITERAL_CHAR:
      token_object = constant_token_object (m);
      if (*m -> name == '\'') {
  	strcpy (buf, m -> name);
      } else {
	strcatx (buf, "\'", m -> name, "\'", NULL);
      }
      __ctalkSetObjectValue (token_object, m -> name);
      token_object -> scope = ARG_VAR;
      break;
    }
  return token_object;
}

int arg_error_loc (int level, int line, int column, 
		   ERROR_LOCATION *error_loc) {
   error_loc -> error_line = line;
   error_loc -> error_column = column;
   error_loc -> parser_lvl = level;
   error_loc -> fn[0] = '\0';
   return SUCCESS;
}

/* 
   Should only be called when we have a ',' or ';' token. 
*/
static void ma_add_arg (MESSAGE_STACK messages, int tok_idx,
			ARGSTR argstrs[],
			int *argstrptr,
			char *argbuf) {
  argstrs[*argstrptr].m_s = messages;
  argstrs[*argstrptr].arg = strdup (argbuf);
  argstrs[*argstrptr].end_idx = prevlangmsg (messages, tok_idx);
  ++*argstrptr;
  /*
   *  Set the start index of the next argument here.
   */
  argstrs[*argstrptr].start_idx = nextlangmsg (messages, tok_idx) ;
  *argbuf = 0;
  ++messages[tok_idx] -> evaled;
  ++messages[tok_idx] -> output;
}

static void make_basic_arglist (MESSAGE_STACK messages,
				int arglist_start, int arglist_end,
				ARGSTR *argstrs, int *argstrptr) {
  int i;
  for (i = 0; i < *argstrptr; i++)
    __xfree (MEMADDR(argstrs[i].arg));
  argstrs[0].arg = collect_tokens (message_stack (),
				   arglist_start, arglist_end);
  argstrs[0].start_idx = arglist_start;
  argstrs[0].end_idx = arglist_end;
  argstrs[0].m_s = message_stack ();
  *argstrptr = 1;
}

/*
 *  Split the argument list in separate arguments, then 
 *  evaluate each argument.
 *  
 *  To split the argument lists correctly, check first
 *  if the method requires any arguments.  If not, 
 *  simply return.  
 *
 *  If the method requires one argument, then simply
 *  evaluate the tokens until the next METHOD_ARG_TERM_MSG_TYPE;
 *  
 *  If the method requires more than one argument, 
 *  then split the argument by commas, and evaluate each
 *  separately.  NOTE - If there are subexpressions within 
 *  the argument, make sure that only the outer expression
 *  gets split.
 *
 *  Technically, an argument expression may be one of the following:
 *
 *    LABEL
 *    LITERAL
 *    LITERAL_CHAR
 *    INTEGER
 *    FLOAT
 *    LONG
 *    LONGLONG
 *    (<expr>)
 *
 *  NOTE = However, if we encounter a complex argument, or a sequence of
 *  methods that requires backtracking, we simply record all
 *  the tokens following the method, register the variables, and 
 *  evaluate the entire sequence of tokens at run time, when we
 *  know the values of the objects.
 *
 *  FIXME! - If we encounter an expression like:
 *    myArray size == a
 *  it is evaluated as a method with one argument, "== a,"
 *  instead of a method with no arguments followed by a method
 *  that requires one argument.
 */

int method_args (METHOD *method, int method_msg_ptr) {

  int arglist_start,
    arglist_end,
    i,
    j,
    r,
    label_term_ptr,
    prev_idx, prev_end_ptr;
  int have_external_parens,
    arglist_start_ext,
    arglist_end_ext;
  int n_parens;
  char argbuf[MAXMSG];
  OBJECT *rcvr_class_obj,
    *arg_obj = NULL;           /* Avoid a warning. */
  MESSAGE *m_arg, 
    *m_method;
  ERROR_LOCATION error_loc;
  ARGSTR argstrs[MAXARGS] = {{NULL, NULL, 0, 0, false, NULL, 0, 0},};
  int argstrptr;

  char _e[MAXMSG];
  static MESSAGE *_m;
  static OBJECT *t_arg_obj;

  if ((arglist_start = nextlangmsg (message_stack(), method_msg_ptr)) == ERROR)
    error (message_stack_at (method_msg_ptr), "Parser error.");

  /*
   * If argblk's receiver is a method param, then the argblk
   * is already created from resolve ().
   */
  if (!argblk) {
    if (M_TOK(message_stack_at (arglist_start)) == OPENBLOCK) {
      create_argblk (message_stack (), method_msg_ptr, arglist_start);
      return SUCCESS;
    } 
  }
  
  if ((arglist_end = method_arglist_limit_2 (message_stack (), 
					     method_msg_ptr, 
					     arglist_start, 
					     method -> n_params,
					     method -> varargs)) == ERROR) {
    if ((prev_idx=prevlangmsg (message_stack(),method_msg_ptr))!=ERROR) {
      if (is_argblk_expr2 (message_stack (), prev_idx)) {
	return SUCCESS;
      } else {

	/*
	 *  Don't generate exceptions when an expr check hits the
	 *  end of a temporary stack -- arglist_end == -1 in that
	 *  case also.
	 */
	if (interpreter_pass != expr_check) {
	  int __next_idx;

	  arglist_syntax_error_msg (message_stack (), arglist_start);

	  if ((__next_idx=nextlangmsg (message_stack(),method_msg_ptr))==ERROR){
	    __ctalkExceptionInternal (message_stack_at (method_msg_ptr),
				      parse_error_x, NULL,0);
	    return ERROR;
	  }
	  /*
	   *  resolve () checks if the next message is also a method.
	   *  Should only occur for a compound method preceding an
	   *  argument block.  If resolve can't determine this case,
	   *  then we may need to add an exception, the same as for
	   *  a <rcvr> <instancevar> <method> {... } expression.
	   */
	  if (M_TOK(message_stack_at (__next_idx)) == METHODMSGLABEL) {
	    MESSAGE *m_next_method, *m_this_method;
	    m_this_method = message_stack_at (method_msg_ptr);
	    m_next_method = message_stack_at (__next_idx);
	    m_next_method -> receiver_obj = m_this_method -> receiver_obj;
	    m_next_method -> receiver_msg = m_this_method -> receiver_msg;
	    return ERROR;
	  } else {
	    __ctalkExceptionInternal (message_stack_at (method_msg_ptr),
				    parse_error_x, NULL,0);
	    return ERROR;
	  }
	}
      }
    } else {
      __ctalkExceptionInternal (message_stack_at (method_msg_ptr),
				parse_error_x, NULL,0);
      return ERROR;
    }
  }

  /* if (!global_constructor_arg (method, method_msg_ptr))
     return SUCCESS;*/

  /*
   *  Handle empty argument lists directly.
   */
  if ((arglist_start == arglist_end) && 
      _is_empty_arglist (message_stack (), arglist_start,
			 P_MESSAGES)) {
    if (method -> n_args) {
      warning (message_stack_at (arglist_start), 
	       "Method %s: Wrong number of arguments\n.", method -> name);
    }
    return SUCCESS; 
  }

  /*
   *  Adjust the start and end indexes to exclude parentheses.
   * 
   *  However, we also have to check that the parentheses
   *  enclose the entire expression, so we don't also 
   *  exclude parentheses in expressions like (1 + 1) - (2 + 2).
   */

  if ((message_stack_at (arglist_start) -> tokentype == OPENPAREN) &&
      (message_stack_at (arglist_end) -> tokentype == CLOSEPAREN) &&
      _external_parens (message_stack (), arglist_start, arglist_end)) {
    have_external_parens = TRUE;
    arglist_start_ext = arglist_start;
    arglist_end_ext = arglist_end;
    ++message_stack_at (arglist_start) -> evaled;
    ++message_stack_at (arglist_start) -> output;
    ++message_stack_at (arglist_end) -> evaled;
    ++message_stack_at (arglist_end) -> output;
    arglist_start = nextlangmsg (message_stack (), arglist_start);
    arglist_end = prevlangmsg (message_stack (), arglist_end);
  } else {
    have_external_parens = FALSE;
    arglist_start_ext = -1;
    arglist_end_ext = -1;
  }

  argstrs[0].start_idx = arglist_start;

  /*
   *  If the method requires only one argument, and does not
   *  accept a variable number of arguments, then collect
   *  the tokens of the first thing that looks like an argument.
   */

  if (!method -> varargs && (method -> n_params == 1)) {

    m_method = message_stack_at (method_msg_ptr);
    if ((rcvr_class_obj = m_method -> receiver_obj -> __o_class) == NULL) {
      rcvr_class_obj = get_class_object 
	(IS_CLASS_OBJECT(m_method -> receiver_obj) ?
	 m_method -> receiver_obj -> __o_name :
	 m_method -> receiver_obj -> CLASSNAME);
    }

    /*
     *  If we're looking for only one argument and we encounter
     *  a parenthesis, then the argument is the entire expression
     *  in the parenthesis.
     */

    argstrs[0].arg = collect_tokens (message_stack (), arglist_start,
				     ((arglist_end == -1) ? 
				      get_messageptr () + 1 : arglist_end));
    argstrs[0].end_idx = arglist_end;
    argstrs[0].m_s = message_stack ();

    arg_error_loc (CURRENT_PARSER -> level,
		   message_stack_at (argstrs[0].start_idx) -> error_line,
		   message_stack_at (argstrs[0].start_idx) -> error_column,
		   &error_loc);

    if ((arg_obj = eval_arg (method, rcvr_class_obj, &argstrs[0], 
			     method_msg_ptr))
	!= NULL) {
      if (arg_class == arg_c_sizeof_expr) {
	fileout (arg_obj -> __o_name, 0, method_msg_ptr);
	for (i = arglist_start; i >= arglist_end; i--) {
	  message_stack_at (i) -> obj = arg_obj;
	  ++message_stack_at (i) -> evaled;
	  ++message_stack_at (i) -> output;
	}
	__xfree (MEMADDR(argstrs[0].arg));
	argstrs[0].start_idx = argstrs[0].end_idx = 0;
	return SUCCESS;
      }
      if (IS_COLLECTION_SUBCLASS_OBJ(rcvr_class_obj) &&
	  !IS_STREAM_SUBCLASS_OBJ(rcvr_class_obj)) {
	/* This is probably not necessary now */
	/* Collection argument processing should go here. */
	/* The expression should be formatted as a run time expression,
	   from method_call. */
	if (subscript_object_expr)
	  subscript_object_expr = FALSE;
      } else {
	/*
	 *  TO DO - eval_arg () can set arg_obj to a function that is
	 *  itself an argument, so a temporary fix here is to output
	 *  the entire argument.  This needs to be fixed in eval_arg ()
	 *  if the parser is to interpret mixed function/object
	 *  argument lists correctly.
	 */
	if (DEFAULTCLASS_CMP(arg_obj, ct_defclasses->p_cfunction_class,
			     CFUNCTION_CLASSNAME)) {
  	  if ((r = fn_output_context (message_stack (), method_msg_ptr,
		      arg_obj, method, arglist_start, arglist_end))
  	      == FN_IS_ARG_RETURN_VAL) {
  	    if (arg_class == arg_c_writable_fn_expr) {
	      toks2str (message_stack (), arglist_start, arglist_end, _e);
	      
    	      t_arg_obj = create_object_init (CFUNCTION_CLASSNAME,
  					      CFUNCTION_SUPERCLASSNAME,
  					      _e, _e);
  	      _m = message_stack_at (arglist_start);
  	      _m -> obj = t_arg_obj;
  	      writable_arg_rt_arg_expr 
 		      (message_stack (), arglist_start,
  		       arglist_end, arg_obj -> __o_name);
    	      delete_object (t_arg_obj);
   	      _m -> obj = NULL;
	      /*
	       *  The parser doesn't always set frames at 
	       *  label terminators (':'), so check here just in
	       *  case.
	       */
	      if ((label_term_ptr = 
		   have_label_terminator (message_stack (), 
					  method_msg_ptr,
					  FRAME_START_IDX)) != ERROR) {
		generate_c_expr_store_arg_call (m_method -> receiver_obj,
						method, arg_obj,
				M_NAME(message_stack_at (arglist_start)),
						label_term_ptr - 1,
						&argstrs[0]);
	      } else {
		generate_c_expr_store_arg_call (m_method -> receiver_obj,
						method, arg_obj,
				M_NAME(message_stack_at (arglist_start)),
						FRAME_START_IDX,
						&argstrs[0]);
	      }
	    } else if (arg_class == arg_c_fn_expr) {
	      if (argstrs[0].start_idx ==
		  arg_c_fn_terms[arg_c_fn_term_ptr-1].start &&
		  argstrs[0].end_idx ==
		  arg_c_fn_terms[arg_c_fn_term_ptr-1].end) {
	      } else {
		output_mixed_c_to_obj_arg_block
		  (message_stack (), method_msg_ptr, &argstrs[0],
		   method);
	      }
  	    } else if ((label_term_ptr = 
			have_label_terminator (message_stack (), 
					       method_msg_ptr,
					       FRAME_START_IDX)) != ERROR) {
	      generate_c_expr_store_arg_call
		(m_method -> receiver_obj,
		 method, arg_obj,
		 M_NAME(message_stack_at (arglist_start)),
		 label_term_ptr - 1,
		 &argstrs[0]);
	    } else if (message_stack_at (argstrs[0].start_idx) -> attrs &
		       OBJ_IS_SINGLE_TOK_ARG_ACCESSOR) {
	      /* This attribute is set in resolve_single_token_arg in
		 eval_arg.c when outputting __ctalk_arg_internal () to
		 retrieve a method argument. */
	      generate_c_expr_store_arg_call
		(m_method -> receiver_obj,
		 method, arg_obj, METHOD_ARG_ACCESSOR_FN,
		 FRAME_START_IDX, &argstrs[0]);
	    } else {
	      generate_c_expr_store_arg_call
		(m_method -> receiver_obj,
		 method, arg_obj, 
		 M_NAME(message_stack_at (argstrs[0].start_idx)),
		 FRAME_START_IDX,
		 &argstrs[0]);
	    }
  	  }
	} else {
	  if (ctrlblk_pred) {
	    ctrlblk_push_args (m_method -> receiver_obj, method, arg_obj);
	  } else {
	    if (!method -> cfunc) {
	      if (subscript_object_expr) {
		/* 
		 *  The template with the argument call is generated 
		 *  if necessary from register_c_var (), so don't try 
		 *  to generate another store arg call.  That means skip
		 *  all of the following clauses.
		 */
		subscript_object_expr = FALSE;
	      } else {
 		if (!collection_needs_rt_eval
 		    (m_method -> receiver_obj -> __o_class,
 		     method) && 
		    !method_expr_is_c_fmt_arg 
		    (message_stack (), method_msg_ptr,
		     P_MESSAGES,
		     get_stack_top (message_stack ()))) {
		  if (interpreter_pass != expr_check)
		    generate_store_arg_call (m_method -> receiver_obj,
					     method, arg_obj, FRAME_START_IDX);
		  if ((arg_obj -> scope & CVAR_VAR_ALIAS_COPY) ||
		      (eval_arg_cvar_reg == true)) {
		    /* set in eval_arg */
		    output_delete_cvars_call
		      (message_stack (), method_msg_ptr,
		       get_stack_top (message_stack ()));
		  }
		} else {
		  if (method_expr_is_c_fmt_arg 
		      (message_stack (), method_msg_ptr,
		       P_MESSAGES,
		       get_stack_top (message_stack ())) &&
		      (arg_class != arg_rt_expr)) {
		    if (arg_class == arg_obj_tok || arg_class == arg_const_tok){
		      /*
		       *  We'll do this simply for now....
		       *  We might need something more involved
		       *  for arg_obj_expr class arguments.
		       */
		      char _t[MAXMSG];
		      MESSAGE *_m, *_m_start;
		      int _r_idx;
		      _r_idx = prevlangmsg (message_stack (), method_msg_ptr);

		      /* Try to fix up what _were_ external parentheses 
			 around the args. */
		      if (have_external_parens) {
			_m_start = message_stack_at (_r_idx);
			if ((M_TOK(_m_start) == OPENPAREN) ||
			    (_r_idx == arglist_start_ext)) {
			  /* Parens still enclose the entire expression,
			     don't include in the actual expression that  
			     we need to evaluate. */
			  toks2str (message_stack (), _r_idx,
				    arglist_start, _t);
			} else {
			  /* parens enclose the arg list only, so include
			     the last paren. */
			  toks2str (message_stack (), _r_idx,
				    arglist_end_ext, _t);
			}
		      } else {
			toks2str (message_stack (), _r_idx, arglist_start,
				  _t);
		      }
		      t_arg_obj = create_object_init (EXPR_CLASSNAME,
						      EXPR_SUPERCLASSNAME,
						      _t, _t);
		      save_method_object (t_arg_obj);
		      _m = message_stack_at (_r_idx);
		      _m -> obj = t_arg_obj;
		      arg_class = arg_rt_expr;
		    } else {
		      generate_store_arg_call (m_method -> receiver_obj,
					       method, arg_obj, FRAME_START_IDX);
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
      if (method_expr_is_c_fmt_arg 
	  (message_stack (), method_msg_ptr,
	   P_MESSAGES,
	   get_stack_top (message_stack ()))) {
	if (arg_class != arg_rt_expr) {
	/*
	 *  rt expression generated above.
	 */
	  store_arg_object (method, arg_obj);
	}
      } else {
	store_arg_object (method, arg_obj);
      }
    }
    if (interpreter_pass != expr_check) {
      if (DEFAULTCLASS_CMP(arg_obj, ct_defclasses -> p_expr_class,
			   EXPR_CLASSNAME)) {
	for (i = arglist_start; i >= arglist_end; i--) {
	  message_stack_at (i) -> attrs |= TOK_IS_RT_EXPR;
	}
      }
      for (i = arglist_start; i >= arglist_end; i--) {
	message_stack_at (i) -> obj = arg_obj;
	++message_stack_at (i) -> evaled;
	++message_stack_at (i) -> output;
      }
    }
    if (argstrs[0].arg != NULL) {
      /* During an expr_check this might not be filled in. */
      __xfree (MEMADDR(argstrs[0].arg));
      argstrs[0].start_idx = argstrs[0].end_idx = 0;
      argstrs[0].m_s = NULL;
    }
    n_th_arg_CVAR_has_unary_inc_prefix = 
      n_th_arg_CVAR_has_unary_dec_prefix = -1;
    n_th_arg_CVAR_has_unary_inc_postfix = 
      n_th_arg_CVAR_has_unary_dec_postfix = -1;
    return SUCCESS;
  } else if (method -> n_params == 0) {
    m_arg = message_stack_at (arglist_start);
    if (M_TOK(m_arg) == INCREMENT ||
	M_TOK(m_arg) == DECREMENT) {
      if (rcvr_cx_postfix_warning (message_stack (), arglist_start))
	return ERROR;
    }
  }

  for (i = arglist_start, argstrptr = 0, *argbuf = 0, n_parens = 0;
       i >= arglist_end; i--) {

    m_arg = message_stack_at (i);

    switch (m_arg -> tokentype)
      {
      case ARGSEPARATOR:
	/* check if the comma is between parentheses before ending
	   the arg */
	if (n_parens == 0)
	  ma_add_arg (message_stack (), i, argstrs, &argstrptr, argbuf);
	else
	  strcat (argbuf, m_arg -> name);
	break;
      case SEMICOLON:
	ma_add_arg (message_stack (), i, argstrs, &argstrptr, argbuf);
	break;
      case WHITESPACE:
      case NEWLINE:
	/* no leading whitespace */
	if (*argbuf)
	  strcat (argbuf, m_arg -> name);
	break;
      case OPENPAREN:
	++n_parens;
	strcat (argbuf, m_arg -> name);
	break;
      case CLOSEPAREN:
	--n_parens;
	strcat (argbuf, m_arg -> name);
	break;
      default:
	strcat (argbuf, m_arg -> name);
	break;
      }

    if (i == arglist_end) {
      argstrs[argstrptr].arg = strdup (argbuf);
      argstrs[argstrptr].m_s = message_stack ();
      argstrs[argstrptr++].end_idx = arglist_end;
    }
  }

  /*
   *  Check here for a complex method expression.
   *  Do fixups as much as possible.  Might need to
   *  do an ambiguous operand throw here also, and
   *  just go on to method_call ().
   */
  if (argstrptr != method -> n_params) {
    if (method -> n_params == 0) {
      if (!str_eq (method -> returnclass, "Any")) {
	OBJECT *return_class_object;
	METHOD *arg_method;
	MESSAGE *m_arg_method;
	/* Check if the following message is a method with the
	   receiver of the main method's return class. */
	m_method = message_stack_at (method_msg_ptr);
	return_class_object = get_class_object (method -> returnclass);
	m_arg_method = message_stack_at (arglist_start);
	if (((arg_method = get_instance_method 
	      (m_method, return_class_object, M_NAME(m_arg_method), 
	       ERROR, FALSE)) 
	     != NULL) ||
	    ((arg_method = get_class_method
	      (m_method, return_class_object, M_NAME(m_arg_method), 
	       ERROR, FALSE)) 
	     != NULL)) {
	  make_basic_arglist (message_stack (), arglist_start, arglist_end,
			      argstrs, &argstrptr);
	} else if (M_TOK(m_arg_method) == LABEL &&
		   M_TOK(m_method) == METHODMSGLABEL) {
	  if (is_instance_variable_message (message_stack (), arglist_start)) {
	    make_basic_arglist (message_stack (), arglist_start, arglist_end,
				argstrs, &argstrptr);
	  }
	} else {
	  method_args_wrong_number_of_arguments_1 
	    (message_stack (),
	     method_msg_ptr,
	     ((interpreter_pass == method_pass) ?
	      new_methods[new_method_ptr+1] -> method : NULL),
	     method,
	     get_fn_name (), interpreter_pass);
	}
      } else { /* if (!str_eq (method -> returnclass, "Any")) */
	/* 
	 *  TODO? 
	 *  Try an ambiguous operand throw here, or signal to 
	 *  method_call (). 
	 *  For now though, just printing a warning message is 
	 *  enough. See prefix12.c and prefix13.c for how this 
	 *  works. 
	 */
	method_args_ambiguous_argument_1 
	  (message_stack (),
	   method_msg_ptr,
	   ((interpreter_pass == method_pass) ?
	    new_methods[new_method_ptr+1] -> method : NULL),
	   method,
	   arglist_start,
	   get_fn_name (), interpreter_pass);
      } /* if (!str_eq (method -> returnclass, "Any")) */
    }
  }

  m_method = message_stack_at (method_msg_ptr);
  if (IS_CLASS_OBJECT(m_method->receiver_obj)) {
    /* Then the method is probably a constructor method
       (new, basicNew, class, instanceVariable, classVariable....) */
    if (primitive_arg_shadows_c_symbol (argstrs[argstrptr-1].arg)) {
      arg_shadows_a_c_variable_warning (m_method, argstrs[argstrptr-1].arg);
    } else if (primitive_arg_shadows_method_parameter
	       (argstrs[argstrptr-1].arg)) {
      primitive_arg_shadows_a_parameter_warning 
	(message_stack (), argstrs[argstrptr - 1].start_idx);
    }
    /* Won't always register if we're in the method pass and compiling
       a method before a source module, but should occasionally be 
       useful anyway. */
    if (get_global_object (argstrs[argstrptr-1].arg, NULL)) {
      warning (m_method,
	       "Argument, \"%s,\" shadows a global object.",
	       argstrs[argstrptr-1].arg);
    }
    if (IS_C_OP_CHAR (argstrs[argstrptr-1].arg[0]) &&
	(argstrs[argstrptr-1].start_idx == argstrs[argstrptr-1].end_idx)) {
      /* i.e., a single-token identifier or constant. */
      warning (m_method,
	       "Argument, \"%s,\" is not a valid identifier.",
	       argstrs[argstrptr-1].arg);
    }
    if ((rcvr_class_obj = m_method -> receiver_obj -> __o_class) == NULL) {
      rcvr_class_obj = get_class_object (m_method->receiver_obj->__o_name);
    }
  } else {
    if ((rcvr_class_obj = m_method -> receiver_obj -> __o_class) == NULL) {
      rcvr_class_obj = get_class_object (m_method->receiver_obj->__o_classname);
    }
  }

  for (i = 0; i < argstrptr; i++) {

    if (ctrlblk_pred && 
	(message_stack_at (argstrs[i].start_idx) -> evaled > 0))
      continue;

    arg_error_loc (CURRENT_PARSER -> level, 
		   message_stack_at (argstrs[i].start_idx) -> error_line,
 		   message_stack_at (argstrs[i].start_idx) -> error_column,
 		   &error_loc);
    /* Need to have the argstrs done right here. */
    if ((arg_obj = eval_arg (method, rcvr_class_obj, &argstrs[i], 
			     method_msg_ptr)) != NULL) {
      if (DEFAULTCLASS_CMP(arg_obj, ct_defclasses -> p_cfunction_class,
			   CFUNCTION_CLASSNAME)) {
	int r_fn;
	if ((r_fn = fn_output_context (message_stack (), method_msg_ptr, 
			   arg_obj, method, 
			   argstrs[i].start_idx, 
				       argstrs[i].end_idx)) 
	    == FN_IS_ARG_RETURN_VAL) {
	  if (!(message_stack_at (argstrs[i].start_idx) -> attrs &
		OBJ_IS_SINGLE_TOK_ARG_ACCESSOR)) {
	    /*
	     *  OBJ_IS_SINGLE_TOK_ARG_ACCESSOR is set in
	     *  resolve_single_tok_arg in eval_arg.c, and output
	     *  above (so far).
	     *
	     *  Otherwise look for a template and generate a store arg call.  
	     *  Print a warning if the function doesn't have a template.
	     */
	    char *r_expr;
	    OBJECT *r_expr_object;
	    if ((r_expr = clib_fn_rt_expr (message_stack(), 
					   argstrs[i].start_idx)) != NULL) {
	      r_expr_object = create_object (EXPR_CLASSNAME, r_expr);
	      r_expr_object -> __o_superclass = 
		get_class_object (EXPR_SUPERCLASSNAME); 
	      generate_store_arg_call
		(m_method -> receiver_obj,
		 method, 
		 r_expr_object,
		 frame_at (CURRENT_PARSER -> frame) -> message_frame_top);
	    } else if (arg_class == arg_c_fn_expr) {
	      char translatebuf[MAXMSG];
	      fmt_c_to_obj_call (message_stack (),
				 argstrs[i].start_idx,
				 rcvr_class_obj, method,
				 arg_obj, translatebuf,
				 &argstrs[i]);
	      __xfree (MEMADDR(argstrs[i].arg));
	      argstrs[i].arg = strdup (translatebuf);
	      generate_c_expr_store_arg_call
		(m_method -> receiver_obj,
		 method, arg_obj,
		 M_NAME(message_stack_at (argstrs[i].start_idx)),
		 FRAME_START_IDX, &argstrs[i]);
	    } else {
	      warning (message_stack_at(argstrs[i].start_idx),
		       "Function %s used as method argument without template.",
		       M_NAME(message_stack_at(argstrs[i].start_idx)));
	    }
	  } else { /* ... attrs & OBJ_IS_SINGLE_TOK_ARG_ACCESSOR ... */
	    generate_c_expr_store_arg_call
	      (m_method -> receiver_obj,
	       method, arg_obj, METHOD_ARG_ACCESSOR_FN,
	       FRAME_START_IDX, &argstrs[i]);
	  } /* ... attrs & OBJ_IS_SINGLE_TOK_ARG_ACCESSOR ... */
	} else {
	  char __objrtrnbf[MAXMSG], __buf[MAXMSG];
	  if (arg_class == arg_c_fn_expr) {
	    if (argstrs[i].start_idx ==
		arg_c_fn_terms[arg_c_fn_term_ptr-1].start &&
		argstrs[i].end_idx ==
		arg_c_fn_terms[arg_c_fn_term_ptr-1].end) {
	      /* The function and args is the entire argument - use
		 the older function. */
	      goto arg_is_fn_term_only;
	    } else {
	      output_mixed_c_to_obj_arg_block
		(message_stack (), method_msg_ptr, &argstrs[i],
		 method);
	    }
	  } else {
	  arg_is_fn_term_only:
	    fmt_c_to_obj_call (message_stack (), argstrs[i].start_idx,
			       m_method -> receiver_obj, method,
			       arg_obj,
			       __objrtrnbf, &argstrs[i]);
	    strcatx (__buf, "__ctalk_arg (\"",
		     m_method -> receiver_obj -> __o_name, "\", \"",
		     method -> name, "\", ", ascii[method -> n_params],
		     ", (void *)", __objrtrnbf, ");\n", NULL);
	    fileout (__buf, 0,
		     frame_at (CURRENT_PARSER -> frame) -> message_frame_top);
	  }
	} 
      } else {
	if (ctrlblk_pred) {
	  ctrlblk_push_args (m_method -> receiver_obj, method, arg_obj);
	} else {
	  /*
	   *  If the interpreter method has a cfunc, it is a primitive
	   *  method and is called by method_call (), below.
	   */
	  if (!method -> cfunc) {
	    if (subscript_object_expr) {
	      /* arg call template generated when calling register_c_var (),
		 don't generate another __ctalk_arg () call. */
	      subscript_object_expr = FALSE;
	    } else {
	      if ((arg_class != arg_rt_expr) && !collection_needs_rt_eval 
		  (m_method->receiver_obj -> __o_class, method)) {
		prev_idx = prevlangmsg (message_stack (), method_msg_ptr);
		if (message_stack_at (prev_idx) -> attrs &
		    TOK_IS_DECLARED_C_VAR) {
		  /* We can't use just one register call here, because
		     __ctalk_arg deletes the CVAR immediately after 
		     it pushes the arg onto the stack */
		  register_c_var (message_stack_at (prev_idx),
				  message_stack (),
				  prev_idx, &prev_end_ptr);
		}
		generate_store_arg_call (m_method -> receiver_obj,
					 method, arg_obj,
					 frame_at (CURRENT_PARSER -> frame)
					 -> message_frame_top);
	      }
	    }
	  }
	}
      }
      if (arg_class != arg_rt_expr)
	store_arg_object (method, arg_obj);
      for (j = argstrs[i].start_idx; j >= argstrs[i].end_idx; j--) {
	message_stack_at (j) -> obj = arg_obj;
	++message_stack_at (j) -> evaled;
	++message_stack_at (j) -> output;
      }
    }
    n_th_arg_CVAR_has_unary_inc_prefix = 
      n_th_arg_CVAR_has_unary_dec_prefix = -1;
    n_th_arg_CVAR_has_unary_inc_postfix = 
      n_th_arg_CVAR_has_unary_dec_postfix = -1;
  }

  if (interpreter_pass != expr_check) {
    /* expr_check pass doesn't (always) fill in a method's arguments,
     so come up with something else here if we need yet another argument
     mismatch check. */
    if ((method -> n_args != method -> n_params) &&
	!method -> varargs && 
	arg_obj &&
	!DEFAULTCLASS_CMP(arg_obj, ct_defclasses->p_expr_class,
			  EXPR_CLASSNAME)) {
      char errbuf[MAXMSG];
      int i;
      memset (errbuf, 0, MAXMSG);
      for (i = 0; i < argstrptr; i++) {
	strcatx2 (errbuf, argstrs[i].arg, NULL);
	if (i < (argstrptr - 1))
	  strcatx2 (errbuf, ",", NULL);
      }
      method_args_wrong_number_of_arguments_2 
	(message_stack (),
	 method_msg_ptr,
	 ((interpreter_pass == method_pass) ?
	  new_methods[new_method_ptr+1] -> method : NULL),
	 method,
	 get_fn_name (),        
	 argstrptr,
	 errbuf,
	 interpreter_pass);
    }
  } /* if (interpreter_pass != expr_check) */
  
  while (--argstrptr >= 0) {
    __xfree (MEMADDR(argstrs[argstrptr].arg));
    if (argstrs[argstrptr].typecast_expr != NULL) {
      __xfree (MEMADDR(argstrs[argstrptr].typecast_expr));
    }
  }

  /*
   *  Catch this here in case an arg didn't reset it above -
   *  shouldn't reset the var too much before this.
   */
  if (subscript_object_expr)
    subscript_object_expr = FALSE;

  return SUCCESS;
}

static int super_method_lookup = FALSE;
/* Used for warning messages, so we can print the name of the
   original receiver's class. */
static OBJECT *super_orig_class;
static OBJECT *super_orig_rcvr;

/* this is only called by compound method, below. */
METHOD *get_super_instance_method (MESSAGE_STACK messages, int tok_idx,
				   OBJECT *o, char *name, int warn) {

  METHOD *m = NULL;
  if (argblk) {
    m = get_instance_method (messages[tok_idx], 
			     o -> __o_class, name, ERROR, warn);
  } else {
    super_method_lookup = TRUE;

    if (o -> __o_superclass) {
    super_orig_class = o -> __o_superclass;
      super_orig_rcvr = o;
      m = get_instance_method (messages[tok_idx], 
			     o -> __o_superclass, name, ERROR, warn);
    }
    super_method_lookup = FALSE;
  }
  return m;
}

/* 
 *   Retrieve an instance method for an object by its class object or 
 *   superclass objects.  If not found, search the class libraries
 *   for the method.
 */

extern int method_from_proto; /* Declared in mthdref.c */

/* 
 * If n_params_wanted is -1, it means we don't know, don't care, 
 * or haven't implemented the arg counting yet.  Further on, this 
 * function is going to have attribute parameters like prefix, varargs, 
 * and no_init.
 *
 * If n_params_wanted is < 0, it's either a primitive declaration like
 * instanceMethod () or classMethod () or instanceVariable () or 
 * classVariable (), or there's an error.  See the comments for
 * method_arglist_n_args () in arg.c.
 */
static inline int match_method (METHOD *m, char *name, int n_params_wanted) {

#define GET_METHOD_WITH_PARAM_COUNT

#ifdef GET_METHOD_WITH_PARAM_COUNT
  if (n_params_wanted < 0) {

    if (str_eq (m -> name, name) && !m -> prefix)
      return TRUE;

  } else {

    if (m -> varargs) {

      if (str_eq (m -> name, name))
   	return TRUE;

    } else {

      if (str_eq (m -> name, name) && 
   	  (m -> n_params == n_params_wanted) && 
   	  !m -> prefix)
   	return TRUE;

    }
  }

#else

    if (str_eq (m -> name, name) && !m -> prefix)
      return TRUE;

#endif /* GET_METHOD_WITH_PARAM_COUNT */

  return FALSE;
}

static inline OBJECT *class_from_object (OBJECT *o) {
  if (IS_OBJECT(o -> __o_class))
    return o -> __o_class;
  else
    return class_object_search (o ->  __o_classname, FALSE);
}

METHOD *get_instance_method (MESSAGE *m_org, OBJECT *o, char *name, 
			     int n_params_wanted, int warn) {

  OBJECT *class;
  METHOD *m, *requested_method = NULL;

  if (!IS_OBJECT(o)) return NULL;
  if (!IS_CLASS_OBJECT (o)) 
    class = class_from_object (o);
  else 
    class = o;

  if (!IS_OBJECT(class)) return NULL;

  for (m = class -> instance_methods; m; m = m -> next) {
    if (match_method (m, name, n_params_wanted)) {
      requested_method = m;
      queue_method_for_output (class, m);
      break;
    }

    if (!m -> next)
      break;
  }
  
  if (requested_method)
    return requested_method;

  if (o->__o_superclass) {
    if ((requested_method = 
	 get_instance_method (((m_org) ? m_org : NULL), 
			      o->__o_superclass, name, 
			      n_params_wanted, warn)) != NULL) 
      /*
       *  Here the function must check for a method of the 
       *  same class that is forward declared, but also has
       *  a method of the same name in a superclass.  
       *  Note that this case should not apply to methods
       *  that are part of a superclass lookup, and should
       *  not apply to recursive calls, which should be 
       *  handled above.
       */
      if (is_method_proto (class, name) &&
	  !method_proto_is_output (name) &&
	  !method_from_proto &&
	  !super_method_lookup && 
	  (strcmp (name, new_methods[new_method_ptr+1]->method->name))) {

	/* Create the method from its prototype, then check again. */
	method_from_prototype_2 (class, name);

	for (m = class -> instance_methods; m; m = m -> next) {
	  if (match_method (m, name, n_params_wanted)) {
	    requested_method = m;
	    queue_method_for_output (class, m);
	    break;
	  }

	  if (!m -> next)
	    break;
	}

      }
    return requested_method;
  } else {
    OBJECT *__t;
    if (!IS_OBJECT(class -> __o_superclass)) {
      __t = class_object_search (class -> __o_superclassname, FALSE);
    } else {
      __t = class -> __o_superclass;
    }
    if (IS_OBJECT(__t)) {
      if ((requested_method = 
	   get_instance_method (((m_org) ? m_org : NULL), 
				__t, name, n_params_wanted, warn)) != NULL) 
	return requested_method;
    }
  }

  /* If the method still isn't found, try to find a prototype, then
     look in the class library once more. */
  if (requested_method) {
    return requested_method;
  } else {
    library_search (class -> __o_name, FALSE);
    for (m = class -> instance_methods; m; m = m -> next) {
      if (match_method (m, name, n_params_wanted)) {
	requested_method = m;
	queue_method_for_output (class, m);
	break;
      }

      if (!m -> next)
	break;
    }
  }

  if (requested_method) {
    return requested_method;
  } else {
    if (warn) {
      if (super_method_lookup) {
	warning (m_org, "Label %s (Receiver %s, Class %s) not found.\n", 
		 name, 
		 (IS_OBJECT(super_orig_rcvr) ? 
		  super_orig_rcvr -> __o_name : NULLSTR),
		 (IS_OBJECT(super_orig_rcvr) ? 
		  super_orig_rcvr -> __o_classname: class -> __o_name));
      } else {
	warning (m_org, "Label %s (Class %s) not found.\n", 
		 name, class -> __o_name);
      }
    }
  }

  return NULL;

}

METHOD *get_prefix_instance_method (MESSAGE *m_org, OBJECT *o, char *name, 
				    int warn) {

  OBJECT *class;
  METHOD *m, *requested_method = NULL;

  if (!IS_OBJECT(o)) return NULL;
  if (!IS_CLASS_OBJECT (o)) 
    class = class_from_object (o);
  else 
    class = o;

  if (!IS_OBJECT(class)) return NULL;

  for (m = class -> instance_methods; m; m = m -> next) {
    if (str_eq (m -> name, name) && m -> prefix) {
      requested_method = m;
      queue_method_for_output (class, m);
    }
    if (!m -> next)
      break;
  }
  
  if (requested_method)
    return requested_method;

  if (o->__o_superclass) {
    if ((requested_method = 
	 get_prefix_instance_method (((m_org) ? m_org : NULL), 
				     o->__o_superclass, name, warn)) != NULL) 
      /*
       *  Here the function must check for a method of the 
       *  same class that is forward declared, but also has
       *  a method of the same name in a superclass.  
       *  Note that this case should not apply to methods
       *  that are part of a superclass lookup, and should
       *  not apply to recursive calls, which should be 
       *  handled above.
       */
      if (is_method_proto (class, name) &&
	  !method_proto_is_output (name) &&
	  !method_from_proto &&
	  !super_method_lookup && 
	  (strcmp (name, new_methods[new_method_ptr+1]->method->name))) {
	method_from_prototype (name);
	for (m = class -> instance_methods; m; m = m -> next) {
	  if (str_eq (m -> name, name)) {
	    requested_method = m;
	    queue_method_for_output (class, m);
	  }
	  if (!m -> next)
	    break;
	}
      }
    return requested_method;
  } else {
    OBJECT *__t;
    if (!IS_OBJECT(class -> __o_superclass)) {
      __t = class_object_search (class -> __o_superclassname, FALSE);
    } else {
      __t = class -> __o_superclass;
    }
    if (IS_OBJECT(__t)) {
      if ((requested_method = 
	   get_prefix_instance_method (((m_org) ? m_org : NULL), 
				       __t, name, warn)) != NULL) 
	return requested_method;
    }
  }

  /* If the method still isn't found, try to find a prototype, then
     look in the the class library once more. */
  if (requested_method) {
    return requested_method;
  } else {
    library_search (class -> __o_name, FALSE);
    for (m = class -> instance_methods; m; m = m -> next) {
      if (str_eq (m -> name, name) && m -> prefix) {
	requested_method = m;
	queue_method_for_output (class, m);
      }
      if (!m -> next)
	break;
    }
  }

  if (requested_method) {
    return requested_method;
  } else {
    if (warn)
      _warning ("Method %s (Class %s) not found.\n", name, class -> __o_name);
  }

  return NULL;

}

/* 
 *   Retrieve a class method for an object by its class object or 
 *   superclass objects.  If not found, search the class libraries
 *   for the method.
 */

METHOD *get_class_method (MESSAGE *m_org, OBJECT *o, char *name, 
			  int n_args_wanted, int warn) {

  OBJECT *class;
  METHOD *m, *requested_method = NULL;

  if (!IS_OBJECT(o)) return NULL;

  if (!IS_CLASS_OBJECT(o))
    class = class_from_object (o);
  else 
    class = o;

  for (m = class -> class_methods; m; m = m -> next) {
    if (match_method (m, name, n_args_wanted)) {
      requested_method = m;
      queue_method_for_output (class, m);
      break;
    }

    if (!m -> next)
      break;
  }
  
  if (requested_method)
    return requested_method;

  if (o->__o_superclass) {
    if ((requested_method = 
	 get_class_method (m_org, o -> __o_superclass, name, 
			   n_args_wanted, warn)) != NULL) 
      return requested_method;
  } else {
    if ((o->__o_superclass = 
	 class_object_search (class -> __o_superclassname,
			      FALSE)) != NULL) {
      if ((requested_method = 
	   get_class_method (m_org, o -> __o_superclass, 
			     name, n_args_wanted, warn)) != NULL) 
	return requested_method;
    }
  }

  /* If the method still isn't found, try to find it in the class
     library once more. */
  if (requested_method) {
    return requested_method;
  } else {
    library_search (class -> __o_name, FALSE);
    for (m = class -> class_methods; m; m = m -> next) {
      if (match_method (m, name, n_args_wanted)) {
	requested_method = m;
	queue_method_for_output (class, m);
	break;
      }
      if (!m -> next)
	break;
    }
  }

  if (requested_method) {
    return requested_method;
  } else {
    if (warn)
      warning (m_org, "Method %s (Class %s) not found.\n", name, 
	       class -> __o_name);
  }

  return NULL;

}

/*
 *    Define a primitive method (one that calls a C language function
 *    internally).  Because this is part of the initialization code,
 *    we'll construct a dummy method with error line 0 for the 
 *    error messages.
 */

int define_primitive_method (char *classname, char *name, 
			     OBJECT *(*cfunc)(int), int n_params, 
			     int varargs) {
  METHOD *m, *tm;
  MESSAGE *orig;
  OBJECT *class;
  
  orig = new_message ();

  if ((class = get_class_object (classname)) == NULL)
    error (orig, "Undefined class %s", classname);

  if ((m = (METHOD *)__xalloc (sizeof (struct _method))) == NULL)
    _error ("define_primitive_method: %s.", strerror (errno));

  m -> sig = METHOD_SIG;
  strcpy (m -> name, name);
  m -> n_params = n_params;
  m -> varargs = varargs;
  m -> n_args = 0;
  m -> cfunc = cfunc;
  m -> rcvr_class_obj = class;

  strcpy (m -> returnclass, OBJECT_CLASSNAME);
  if (str_eq (name, "class") || 
      str_eq (name, "new")) {
    strcatx (m -> selector, OBJECT_CLASSNAME, INSTANCE_SELECTOR,
	     name, "_1", NULL);
  } else {
    strcatx (m -> selector, OBJECT_CLASSNAME, INSTANCE_SELECTOR,
	     name, "_v", NULL);
    m -> n_params = 0;
    m -> varargs = TRUE;
  }

  if (class -> instance_methods == NULL) {
    class -> instance_methods = m;
  } else {
    for (tm = class -> instance_methods; tm -> next; tm = tm -> next)
      ;
    tm -> next = m;
    m -> prev = tm;
  }
  delete_message (orig);
  return SUCCESS;
}

/*
 *  New_method (), and other functions, call method_declaration_msg ()
 *  when the input has been tokenized and semantically analyzed by
 *  resolve, so it also uses the METHODMSGLABEL token, unlike 
 *  method_declaration_info (), evaluates the C translation
 *  of the method declaration.
 *
 *  Unlike method_declaration_info, this function does not parse
 *  the parameter declarations.
 *
 *  This function should be able to parse from the start of 
 *  a stack frame, to the beginning of the parameter declarations,
 *  and also parse a method declaration that ends without a
 *  parameter declaration and function body.
 *  Handle a method declaration that has either one or two 
 *  arguments after the method. 
 *
 *  A method declaration can have the form of:
 *
 *       <Class> method <function> (<param_declarations>) {<body>}
 *
 *  or 
 *       <Class> method <alias> <function> (<param_declarations>) {<body>}
 *
 *  New_method translates the method declaration into standard C, and 
 *  stores the declaration with the method body in the class object's
 *  method dictionary.
 *
 *  <param_declarations> in an untranslated method get parsed by 
 *  function_param_declarations ().
 *  The <function> element must be a valid C function name, although
 *  it gets changed into a class method selector.
 */

static int method_decl_msg_states[] = {
#include "method_decl_states.h"
  -1, 0 
};

#define METHOD_MSG_STATE_COLS 2

#define METHOD_DECL_MSG_STATE(x) (check_state ((x), messages, \
              method_decl_msg_states, METHOD_MSG_STATE_COLS))

int method_declaration_msg (MESSAGE_STACK messages,
			    int start_ptr, int end_ptr, char *name, 
			    int *name_ptr, char *alias, 
			    int *alias_ptr) {
  int j;
  MESSAGE *m_tok;
  char buf[MAXMSG];
  int state;

  for (j = start_ptr, *name = 0, *alias = 0,
	 *name_ptr = ERROR, *alias_ptr = ERROR; 
       j > end_ptr; j--) {

    m_tok = messages[j];
    if ((m_tok -> tokentype == NEWLINE) ||
	(m_tok -> tokentype == WHITESPACE))
      continue;

    if ((state = METHOD_DECL_MSG_STATE (j)) == ERROR) {
      toks2str (messages, start_ptr, nextlangmsg (messages, j), buf);
      error (m_tok, "Parse error: %s ... .", buf);
    }

    switch (m_tok -> tokentype)
      {
      case LABEL:
	if (state == 2) {
	  strcpy (alias, m_tok -> name);
	  *alias_ptr = j;
	}
	if (state == 1 || state == 3) {
	  strcpy (name, m_tok -> name);
	  *name_ptr = j;
	}
	break;
      case CHAR:
      case EQ:
      case BOOLEAN_EQ:
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
      case BOOLEAN_OR:
      case BIT_AND_ASSIGN:
      case BIT_COMP:
      case LOG_NEG:
      case INEQUALITY:
      case BIT_OR:
      case BIT_OR_ASSIGN:
      case BIT_XOR:
      case BIT_XOR_ASSIGN:
      case MODULUS:
      case MODULUS_ASSIGN:
      case PERIOD:
      case ELLIPSIS:
      case LITERALIZE:
      case MACRO_CONCAT:
      case CONDITIONAL:
      case MATCH:
      case NOMATCH:
      case COLON:
	if (state >= 50 && state <= 92) {
	  strcpy (alias, m_tok -> name);
	  *alias_ptr = j;
	}
      default:
	break;
      }

  }

  return SUCCESS;

}

static bool register_c_receiver_var (MESSAGE_STACK messages,
				     int rcvr_ptr, 
				     int method_message_ptr) {
  int i;
  int rcvr_expr_end_ptr;
  CVAR *c_rcvr, *c_typedef, *c_mbr;
  MESSAGE *__m, *m_method;
  char output_buf[MAXMSG];
  char expr_buf[MAXMSG];
  bool have_reg = false;

  m_method = messages[method_message_ptr];

  if (m_method -> attrs & RCVR_TOK_IS_C_STRUCT_EXPR) {

    rcvr_expr_end_ptr = 
      struct_end (messages, rcvr_ptr, get_stack_top (messages));

    toks2str (messages, rcvr_ptr, rcvr_expr_end_ptr, expr_buf);

    __m = messages[rcvr_ptr];
    if (((c_rcvr = get_local_var (M_NAME(__m))) != NULL) ||
	((c_rcvr = get_global_var (M_NAME(__m))) != NULL)) {

      if (!c_rcvr -> members) {
	if ((c_typedef = get_typedef (c_rcvr -> type)) != NULL) {

	  if (c_typedef -> members) {

	    __m = messages[rcvr_expr_end_ptr];
	    for (c_mbr = c_typedef -> members; c_mbr; c_mbr = c_mbr -> next)
	      if (!strcmp (c_mbr -> name, M_NAME(__m)))
		break;

	    fileout (fmt_register_c_method_arg_call
		     (c_mbr, expr_buf, (is_global_frame () ? 
					GLOBAL_VAR: LOCAL_VAR),
		      output_buf), 
		     0, FRAME_START_IDX);
	    have_reg = true;
	    
	  }
	}
      } else {

	/* This is untested. */
	fileout (fmt_register_c_method_arg_call
		 (c_rcvr, expr_buf,
		  (is_global_frame () ? GLOBAL_VAR : LOCAL_VAR),
		  output_buf),
		 0, FRAME_START_IDX);
	have_reg = true;
      }

    }

  } else {

    for (i = rcvr_ptr; i >= method_message_ptr; i--) {
      __m = message_stack_at (i);
      if (M_TOK(__m) == LABEL) {
	if (((c_rcvr = get_local_var (M_NAME(__m))) != NULL) ||
	    ((c_rcvr = get_global_var_not_shadowed (M_NAME(__m))) != NULL)) {
	  fileout (fmt_register_c_method_arg_call 
		   (c_rcvr, M_NAME(__m),
		    (is_global_frame () ? GLOBAL_VAR : LOCAL_VAR),
		    output_buf),
		   0, FRAME_START_IDX);
	  have_reg = true;
	}
      }
    }

  }
  return have_reg;
}

static void output_CVAR_subscript_registration (MESSAGE_STACK messages,
						int expr_start_idx,
						int expr_end_idx, CVAR *c_rcvr) {
  char buf[MAXMSG], output_buf[MAXMSG];
  toks2str (messages, expr_start_idx, expr_end_idx, buf);

  fileout (fmt_register_c_method_arg_call
	   (c_rcvr, buf,
	    (is_global_frame () ? GLOBAL_VAR :
	     LOCAL_VAR), output_buf),
	   0, FRAME_START_IDX);
}

static int first_open_paren (MESSAGE_STACK messages, int innermost_paren_idx) {
  /* tries to handle spaces between parens, too. */
  int i, i_prev, i_next, stack_start_idx;
  stack_start_idx = stack_start (messages);
  for (i = innermost_paren_idx, i_next = innermost_paren_idx;
       i <= stack_start_idx; i++) {
    if (M_TOK(messages[i]) == OPENPAREN) {
      i_next = i;
      continue;
    } else if (M_ISSPACE(messages[i])) {
      if ((i_prev = prevlangmsg (messages, i)) != ERROR) {
	if (M_TOK(messages[i_prev]) == OPENPAREN) {
	  i_next = i_prev;
	  continue;
	} else {
	  return i_next;
	}
      }
    } else {
      return i_next;
    }
  }
  return ERROR;
}

static void mc_cvar_cleanup (MESSAGE_STACK messages, int tok_idx) {
  int term_idx;
  term_idx = scanforward (message_stack (), tok_idx,
			  get_stack_top (message_stack ()),
			  SEMICOLON);
  fileout ("\ndelete_method_arg_cvars ();\n", 0, term_idx - 1);
}

/*
 *    Given a method token's message stack index, output a
 *    method call. Resolve () should already have identified its receiver.
 */

static char default_ctrlblk_class[] = "Integer";

int method_call (int method_message_ptr) {

  OBJECT *receiver = NULL;   /* Avoid a warning.  If receiver is not 
				set further down, then there needs
				to be another way to determine the
				receiver. */
  OBJECT *class_object;
  OBJECT *value_object;
  OBJECT *instancevar_object;
  MESSAGE *m,
    *m_super;
  MESSAGE *rcvr_msg = NULL;
  METHOD *method;
  int i,
    rcvr_ptr = 0,             /* Avoid a warning */
    stmt_end_ptr,
    m_super_idx,
    expr_end_idx = -1,
    lookahead;
  int i_2, _expr_end;
  int n_args_declared;
  char *_expr_class_buf, _expr_buf[MAXMSG];
  OBJECT *(*cfunc)();         /* C function that performs method.      */
  int __pfx_idx, __postfix_idx;
  static char expr_buf_out[MAXMSG];  /* for use by rt_expr's fileout */
  int fmt_arg_prev_idx;
  MESSAGE *fmt_arg_m_prev;
  bool rcvr_cvar_registration = false;

  m = message_stack_at (method_message_ptr);

  value_object = NULL;
  method = NULL;
  m_super_idx = -1;
  m_super = NULL;

  if (IS_METHOD_MSG (m) ||
      (m -> attrs & RCVR_OBJ_IS_CONSTANT)) {

    /* Get the receiver's class object. */
    if ((receiver = m -> receiver_obj) == NULL)
      return ERROR;

    if (IS_CLASS_OBJECT(receiver)) {
      class_object = receiver;
    } else {
      class_object = class_object_search 
	(receiver -> instancevars ? 
	 receiver -> instancevars -> __o_classname :
	 receiver -> __o_classname, FALSE);
    }
	
    if ((rcvr_ptr =
	 constant_rcvr_idx (message_stack (), method_message_ptr)) 
	== ERROR) {
      if (m -> attrs & RCVR_OBJ_IS_SUBEXPR) {
	rcvr_ptr = ERROR;
	for (i_2 = method_message_ptr + 1; i_2 <= P_MESSAGES; i_2++)
	  if (message_stack_at (i_2) == m -> receiver_msg)
	    break;
	rcvr_ptr = i_2;
	if (rcvr_ptr == ERROR)
	  error (m, "method_call: unknown receiver.");
      } else {
	if (m -> attrs & RCVR_OBJ_IS_C_ARRAY_ELEMENT) {
	  MESSAGE *__m_actual_rcvr;
	  __m_actual_rcvr = m -> receiver_msg;
	  /*
	   *  Up to here, an array element with a postfix (++|--) op
	   *  is parsed by resolve () as a complete receiver
	   *  expression.  But it potentially has two methods - the
	   *  postfix op and the method pointed to by method_msg_ptr.
	   *  The receiver gets deconstructed into actual C further
	   *  down.  Here the function looks for the initial label -
	   *  the name of the array.
	   */
	  if (__m_actual_rcvr -> attrs & TOK_IS_POSTFIX_OPERATOR)
	    __m_actual_rcvr = __m_actual_rcvr -> receiver_msg;
	  for (i_2 = method_message_ptr; i_2 <= P_MESSAGES; i_2++) {
	    if (message_stack_at (i_2) == __m_actual_rcvr)
	      break;
	  }
	  rcvr_ptr = i_2;
	} else { /* if (m -> attrs & RCVR_OBJ_IS_C_ARRAY_ELEMENT) */
	  /* 
	   * Check the structs here.
	   */
	  if (m -> attrs & RCVR_OBJ_IS_C_STRUCT_ELEMENT) {
	    for (i_2 = method_message_ptr + 1; i_2 <= P_MESSAGES; i_2++)
	      if (message_stack_at (i_2) == m -> receiver_msg)
		break;
	    rcvr_ptr = i_2;
	  } else { /* if (m -> attrs & RCVR_OBJ_IS_C_STRUCT_ELEMENT) */
	    if ((rcvr_ptr = scanback (message_stack (), method_message_ptr,
				      P_MESSAGES,
				      LABEL)) 
		== ERROR)
	      error (m, "method_call: unknown receiver.");
	  } /* if (m -> attrs & RCVR_OBJ_IS_C_STRUCT_ELEMENT) */
	} /* if (m -> attrs & RCVR_OBJ_IS_C_ARRAY_ELEMENT) */
      }
    }

    /* 
     *  If the message refers to an instance variable, then simply
     *  output the run time library call to retrieve the instance
     *  variable.
     *
     *  Note that the lvalue must be an OBJECT *, though in some
     *  future version the parser will do a translation to C, 
     *  as with the functions in lib/objtoc.c.
     *
     *  Generate_rt_instancevar_call () does not output a semicolon,
     *  so we only need to mark the instance variable label as evaled
     *  and output.
     *
     *  The message, m, is the message of the method or variable 
     *  name, which has the stack index, message_ptr.  rcvr_ptr 
     *  is set below to the stack index of the receiver.
     *
     *  Eval_receiver_token () determines here if the receiver is a 
     *  parameter name.
     */

    if ((value_object = 
	 __ctalkGetInstanceVariable (receiver, m -> name, FALSE))
	!= NULL) {
      OBJECT *rcvr_class, *token_object;
      OBJECT_CONTEXT rcvr_context;
      char output_buf[MAXMSG];

      rcvr_msg = message_stack_at (rcvr_ptr);

      rcvr_class = receiver -> __o_class;
      if (!IS_OBJECT(rcvr_class))
	rcvr_class = get_class_object (receiver -> __o_classname);

      rcvr_context = object_context (message_stack (), rcvr_ptr);

      token_object = eval_receiver_token (message_stack (), rcvr_ptr, rcvr_class);

      if (complex_method_statement 
	  (token_object, message_stack (), method_message_ptr)) {
	switch (rcvr_context)
	  {
	  case c_context:
	  case c_argument_context:
	    if (is_if_pred_start (message_stack (), rcvr_ptr) ||
		is_if_subexpr_start (message_stack (), rcvr_ptr) ||
		is_while_pred_start (message_stack (), rcvr_ptr)) {
	      _expr_class_buf = default_ctrlblk_class;
	    } else {
	      _expr_class_buf =
		complex_expr_class (message_stack (), rcvr_ptr);
	    }

	    if (rcvr_msg -> attrs & TOK_IS_PRINTF_ARG) {
	      if (m -> attrs & OBJ_IS_INSTANCE_VAR) {
		strcpy (_expr_buf,
			fmt_instancevar_expr_a (message_stack (),
						rcvr_ptr,
						method_message_ptr,
						&_expr_end));
	      } else {
		fmt_rt_expr (message_stack (), rcvr_ptr, &_expr_end,
			     _expr_buf);
	      }
	      fileout (fmt_rt_return_2 
		      (message_stack (), rcvr_ptr,
		       _expr_end, _expr_buf, _expr_class_buf),
		       0, rcvr_ptr);
	    } else {
	      fileout (fmt_rt_return_chk_fn_arg
		       (fmt_rt_expr (message_stack (), 
				     rcvr_ptr,
				     &_expr_end, expr_buf_out),
			_expr_class_buf, 
			((ctrlblk_pred) ? FALSE : TRUE),
			message_stack (), rcvr_ptr), 0, rcvr_ptr);
	    }
	    for (i_2 = rcvr_ptr; i_2 >= _expr_end; i_2--) {
	      ++(message_stack_at (i_2) -> evaled);
	      ++(message_stack_at (i_2) -> output);
	    }
	    break;
	  default:
	    if (have_complex_arg_block) {
	      complex_arg_block_rcvr_ptr = rcvr_ptr;
	      create_argblk (message_stack (), method_message_ptr,
				     complex_arg_block_start);
	      for (i_2 = complex_arg_block_rcvr_ptr; 
		   i_2 > complex_arg_block_start; i_2--) {
		++(message_stack_at (i_2) -> evaled);
		++(message_stack_at (i_2) -> output);
	      }
	      have_complex_arg_block = FALSE;
	      complex_arg_block_start = complex_arg_block_rcvr_ptr = -1;
	    } else {
	      rt_expr (message_stack (), rcvr_ptr, &_expr_end, expr_buf_out);
	      if (message_stack_at (_expr_end)) {
		/* We need to check there's a message at the end while
		   checking constant control structure predicates. */
		if (message_stack_at(_expr_end)->receiver_obj && 
		    __ctalkIsInstanceVariableOf (message_stack_at(_expr_end)->receiver_obj,
						 M_NAME(message_stack_at(_expr_end)))) {
		  if (verbose_opt &&
		      (interpreter_pass == method_pass) &&
		      rcvr_is_start_of_expr (message_stack (), _expr_end,
					     P_MESSAGES)) {
		    warning (message_stack_at(_expr_end), 
			     "In method %s (Class %s):",
			     new_methods[new_method_ptr+1]->method->name,
			     rcvr_class_obj -> __o_name);
		    warning (message_stack_at(_expr_end), 
			     "Expression ending with Instance variable message, \"%s\" has no lvalue.",
			     M_NAME(message_stack_at (_expr_end)));
		  }
		}
	      }
	    }
	    break;
	  }
	return SUCCESS;
      }

      switch (rcvr_context) 
	{
	case c_context:
	case c_argument_context:
	  if (ctrlblk_pred) {
	    CTRLBLK *__c;
	    __c = ctrlblk_pop ();
	    ctrlblk_push (__c);
	    if (__c -> stmt_type == stmt_do) 
	      __c -> pred_expr_evaled = TRUE;
	    if (__c -> stmt_type == stmt_while) {
	    /* The call to fmt_default_ctrlblk_expr in loop_block_start
	       (loop.c) handles everything that we don't
	       catch otherwise. */
	      return SUCCESS;
	    }
	    fmt_rt_instancevar_call (token_object, m -> name, output_buf);
	  } else {
	    fmt_rt_instancevar_call_2 (message_stack (), rcvr_ptr,
				       method_message_ptr,
				       token_object, m -> name,
				       output_buf);
	  }
	  undefined_label_after_instance_variable_warning 
	    (message_stack (), method_message_ptr);
	  if (((instancevar_object = 
	       __ctalkGetInstanceVariable (receiver, M_NAME(m), FALSE)) 
	       == NULL) &&
	      ((method = 
		get_instance_method (m, class_object, m->name, 
				     ERROR, FALSE)) == NULL) &&
	      ((method = 
		get_class_method (m, class_object, m -> name, 
				  ERROR, FALSE)) == NULL)) {
	    sprintf (errbuf, METHOD_ERR_FMT, M_NAME(m), 
		     class_object->__o_classname);
	    if (is_method_proto (receiver, M_NAME(m))) {
	      __ctalkExceptionInternal (m, method_used_before_define_x, errbuf,0);
	    } else {
	      __ctalkExceptionInternal (m, undefined_method_x, errbuf,0);
	    }
	    return ERROR;
	  }
	  if (is_global_frame ()) {
	    fn_init (obj_2_c_wrapper 
		     (m, token_object, method, output_buf, TRUE), FALSE);
	  } else {
	    /* Can be set by fmt_rt_instancevar_call_2, above. */
	    if (!formatted_fn_arg_trans) { 
		fileout 
		  (obj_2_c_wrapper_trans (message_stack (), rcvr_ptr, 
					  m, token_object, method, 
					  output_buf, TRUE), 
		   0, method_message_ptr);
	    } else {
	      fileout (output_buf, 0, method_message_ptr);
	      formatted_fn_arg_trans = false;
	    }
	  }
	  break;
	default:
	  if (!complex_method_statement 
	      (token_object, message_stack (), method_message_ptr)) {
	    int m_next_ptr, expr_end;
	    MESSAGE *m_next;
	    if ((m_next_ptr = nextlangmsg (message_stack (),
					   method_message_ptr)) != -1) {
	      m_next = message_stack_at (m_next_ptr);
	      if ((M_TOK (m_next) == LABEL) || 
		  (M_TOK(m_next) == METHODMSGLABEL)) {
		if (m -> obj) {
		  if (!have_class_cast_on_unresolved_expr 
		      (message_stack (), method_message_ptr)) {
		    unresolved_eval_delay_warning_2
		      (m, (interpreter_pass == method_pass ?
			   new_methods[new_method_ptr+1] -> method -> name : NULL),
		       m -> obj -> __o_name,
		       ((interpreter_pass == method_pass) ?
			rcvr_class_obj -> __o_name :
			m -> obj -> instancevars -> __o_classname),
		       m -> obj -> instancevars -> __o_classname,
		       M_NAME(m_next));
		  }
		  rt_expr (message_stack (), rcvr_ptr, &expr_end, expr_buf_out);
		  if (eval_arg_cvar_reg)
		    mc_cvar_cleanup (message_stack (), expr_end);
		}
	      } else {
		generate_rt_instancevar_call (message_stack (), rcvr_ptr,
					      method_message_ptr,
					      token_object,
					      m -> name, method_message_ptr);
	      }
	    }
	  }
	  break;
	}

      ++m -> evaled; 
      ++m -> output;

      if (IS_OBJECT (rcvr_msg -> obj)) {
 	if (strcmp (rcvr_msg -> obj -> __o_name, receiver -> __o_name))
 	  error (m, "Method_call: receiver mismatch.");
      } else {
 	if (strcmp (rcvr_msg -> name, receiver -> __o_name))
 	  error (m, "Method_call: receiver mismatch.");
      }

      ++rcvr_msg -> evaled;
      ++rcvr_msg -> output;

      return SUCCESS;
    }

    /*
     *  If the message didn't refer to an instance variable, 
     *  above, and we're still here, then look for a method.
     *  Check for, "super," here.
     */

    n_args_declared = 
      method_arglist_n_args (message_stack (), method_message_ptr);

    if (m -> attrs & TOK_SUPER) {

      m_super_idx = nextlangmsg (message_stack (), method_message_ptr);
      m_super = message_stack_at (m_super_idx);

      n_args_declared = 
	method_arglist_n_args (message_stack (), m_super_idx);

      if (((method = get_instance_method (m_super, receiver -> __o_superclass, 
					  M_NAME(m_super), 
					  n_args_declared, FALSE))==NULL) ||
	  ((method = get_class_method (m_super, receiver -> __o_superclass, 
				       M_NAME(m_super), 
				       ERROR, FALSE))==NULL)) {
	sprintf (errbuf, METHOD_ERR_FMT, M_NAME(m_super),
		 receiver -> __o_superclass -> __o_name);
	__ctalkExceptionInternal (m_super, undefined_method_x, errbuf,0);

	report_method_argument_mismatch (m, class_object, 
					 M_NAME(m), n_args_declared);
	return ERROR;
      }
    } else {
      /*
       *  Receivers that are class variables, both with and 
       *  without receiver value objects.
       */
      if (m->receiver_msg && (m->receiver_msg->attrs & OBJ_IS_CLASS_VAR)) {
	if (m->receiver_msg->obj->instancevars) {
	  if (((method = 
		get_instance_method (m, 
		     m->receiver_msg -> obj -> instancevars -> __o_class,
		     m -> name, 
		    n_args_declared, FALSE))==NULL) &&
	    ((method = 
	      get_class_method (m, 
				m->receiver_msg->obj->instancevars->__o_class,
				m -> name, 
				ERROR, FALSE))==NULL)) {
	    sprintf (errbuf, 
		     METHOD_ERR_FMT, 
		     M_NAME(m), 
		     class_object->__o_name);
	    report_method_argument_mismatch (m, class_object, 
					     M_NAME(m), n_args_declared);
	    __ctalkExceptionInternal (m, undefined_method_x, errbuf, 0);
	    return ERROR;
	  }
	} else {
	  if (((method = 
		get_instance_method (m, 
		     m->receiver_msg->obj->__o_class,
				     m -> name, 
				     n_args_declared, FALSE))==NULL) &&
	    ((method = 
	      get_class_method (m, 
				m -> receiver_msg -> obj -> __o_class,
				m -> name, ERROR, FALSE))==NULL)) {
	    sprintf (errbuf, 
		     METHOD_ERR_FMT, 
		     M_NAME(m), 
		     class_object->__o_name);
	    report_method_argument_mismatch (m, class_object, 
					     M_NAME(m), n_args_declared);
	    __ctalkExceptionInternal (m, undefined_method_x, errbuf, 0);
	    return ERROR;
	  }
	}
      } else {
	if (((method = 
	      get_instance_method (m, class_object, m -> name, 
				   n_args_declared, FALSE))==NULL) &&
	    ((method = 
	      get_class_method (m, class_object, m -> name, 
				ERROR, FALSE))==NULL)) {
	    if (is_method_proto (class_object, m -> name)) {
 	    if (!method_proto_is_output (M_NAME(m)) && 
		!this_method_from_proto (class_object->CLASSNAME, M_NAME(m))) {
	      method_from_prototype (M_NAME(m));
	      if (((method = 
		    get_instance_method (m, class_object, 
					 m -> name, 
					 n_args_declared, 
					 FALSE))==NULL) &&
		  ((method = 
		    get_class_method (m, class_object, m -> name, 
				      ERROR, FALSE))==NULL)) {
		if (interpreter_pass != expr_check) {
		  sprintf (errbuf, 
			   METHOD_ERR_FMT, 
			   M_NAME(m), 
			   class_object->__o_name);
		  report_method_argument_mismatch (m, class_object, 
						   M_NAME(m), n_args_declared);
		  __ctalkExceptionInternal (m, method_used_before_define_x, 
					    errbuf,0);
		}
		return ERROR;
	      }
	    } else {
	      report_method_argument_mismatch (m, class_object, 
					       M_NAME(m), n_args_declared);
	      sprintf (errbuf, 
		       METHOD_ERR_FMT, 
		       M_NAME(m), 
		       class_object->__o_name);
	      __ctalkExceptionInternal (m, method_used_before_define_x, 
					errbuf,0);
	      return ERROR;
	    }
	  } else {
	    if (get_function (M_NAME(m))) {
	      int _prev_idx;
	      if ((_prev_idx = prevlangmsgstack (message_stack (),
						 method_message_ptr))
		  != ERROR) {
		if (M_TOK(message_stack_at (_prev_idx)) != LABEL) {
		  missing_separator_error (message_stack (), 
					   method_message_ptr);
		}
	      }
	    } else {
	      sprintf (errbuf, METHOD_ERR_FMT, M_NAME(m), 
		       class_object->__o_name);
	      __ctalkExceptionInternal (m, undefined_method_x, 
					errbuf, 0);

	      report_method_argument_mismatch (m, class_object, 
					       M_NAME(m), n_args_declared);
	      return ERROR;
	    }
	  }
	}
      }
    }

/* #define DEBUG_ARGUMENT_PROTOTYPING */
#ifdef DEBUG_ARGUMENT_PROTOTYPING
    if ((n_args_declared >= 0) && 
	(method -> varargs == 0)) {
      if (n_args_declared != method -> n_params) {
	warning (m, 
		 "Argument mismatch method %s (class %s): expr %d, " 
		 "method %d\n", method -> name, rcvr_class_obj -> __o_name,
		 n_args_declared, method -> n_params);
      }
    }
#endif

    /*
     *  If the method replaces a primitive method in the
     *  interpreter, use the primitive method instead.
     */
    method = method_replaces_primitive (method, class_object);

    /*
     *  The define_instance_method () and define_class_method () 
     *  primitives evaluate an entire method declaration and body 
     *  separately in a separate parser pass, so we shouldn't 
     *  duplicate the process here.
     *
     *  Register_c_var_method and register_c_var_receiver are here 
     *  in case register_c_var () above, needs to call resolve_arg (),
     *  or format an array subscript expression template.
     */
    /*
     *  NOTE: IS_PRIMITIVE_METHOD does not match, "new," which
     *  uses method_args () for its argument.
     */
    if (!IS_PRIMITIVE_METHOD(method)) {
      if (!(m -> attrs & RCVR_OBJ_IS_SUBEXPR)) {
	register_c_var_method = method;
	register_c_var_receiver = receiver;

	if (method_args (method, 
			 (m_super_idx != -1) ? m_super_idx : method_message_ptr)
	    != 0) {
	  return ERROR;
	}
	register_c_var_method = NULL;
      }
    }
  }  /* if (IS_METHOD_MSG(m)) */

  /*
   *  If not recognized as a method by IS_METHOD_MSG, 
   *  just print a warning and return.
   */
  if (!method) {
    if (rcvr_msg && IS_MESSAGE(rcvr_msg)) {
      warning (m, "\"%s\" is not recognized as a method. (Receiver \"%s\")",
	       M_NAME(m), M_NAME(rcvr_msg));
    } else {
      warning (m, "\"%s\" is not recognized as a method", M_NAME(m));
    }
    return ERROR;
  }

  /* Primitive methods mark the messages as evaluated and
     output. 
  */
  if (method -> cfunc) {
    /* Check_args () gets called by the primitive functions,
       or method_args () in the case of define_method ().
    */
    if (interpreter_pass != expr_check) {
      cfunc = method -> cfunc;
      (void)(cfunc) (method_message_ptr);
      if (frame_at (CURRENT_PARSER -> frame - 1)) {
	cleanup_args (method, message_stack_at(method_message_ptr)->receiver_obj,
		      (frame_at (CURRENT_PARSER -> frame - 1)) ->
		      message_frame_top + 1);
      } else {
	cleanup_args (method,
		      message_stack_at(method_message_ptr)->receiver_obj,
		      get_messageptr ());
      }
    }
  } else {
    char output_buf[MAXMSG];
    OBJECT_CONTEXT context;

    /*
     * TO DO -
     *  Here the function may need a better scan back for 
     *  rcvr_ptr - until 
     *
     *  messages[method_message_ptr] -> receiver_obj == 
     *    messages[method_message_ptr+_n_] -> obj.
     *  
     *  For now, the scan to the start of the frame is 
     *  sufficient.
     */

    context = object_context (message_stack (), rcvr_ptr);

    /*
     *  If evaluating an argument didn't require finding a
     *  receiver class object, then find one now.  Note that
     *  rcvr_class_obj is also filled in by 
     *  new_instance|class_method.
     */
    if (!IS_OBJECT(rcvr_class_obj))
      rcvr_class_obj = get_class_object (receiver -> __o_classname);

    /*
     *  The functions in argblk.c take care of eliding the argument block
     *  *after it has been processed independently of the enclosing
     *   function or method*.
     */

    if (is_argblk_ctrl_struct (message_stack (), method_message_ptr)) {
      goto cleanup_args;
    }

    /*
     *  Collection class methods without arguments don't
     *  need rt_expressions, and collection expressions
     *  in a control block get their own __ctalkEvalExpr
     *  call.
     */
    if (!ctrlblk_pred && collection_needs_rt_eval 
	(m -> receiver_obj -> __o_class, method)) {
      collection_rt_expr (method, message_stack (), rcvr_ptr,
			  method_message_ptr, context);
    } else {
      switch (context)
	{
	case c_context:
	case c_argument_context:
	  if (ctrlblk_pred) {
	    ctrlblk_method_call (receiver, method, method_message_ptr);
	  } else {
 	    if (method_expr_is_c_fmt_arg (message_stack (), method_message_ptr,
					  P_MESSAGES,
					  get_stack_top (message_stack ())) &&
		(ambiguous_arg_start_idx == -1)) {
	      if (arg_class == arg_rt_expr) {
		/*
		 *  Eval_arg stuffed the expression object into 
		 *  the receiver's value_obj slot.
		 */
		char tmp[MAXMSG];
		strcatx (tmp, EVAL_EXPR_FN, "(\"", 
			 M_VALUE_OBJ(message_stack_at (rcvr_ptr))->__o_name,
			 "\")", NULL);
		fileout
		  (obj_2_c_wrapper_trans 
		   (message_stack(), rcvr_ptr,
		    m, receiver, method, tmp, FALSE),
		   0, method_message_ptr);
		arg_class = arg_null;
	      } else {
		fileout
		  (obj_2_c_wrapper_trans 
		   (message_stack(), rcvr_ptr,
		    m, receiver, method, 
		    stdarg_fmt_arg_expr (message_stack (), 
					 method_message_ptr, method,
					 output_buf),
		    FALSE),
		   0, method_message_ptr);
	      }
 	    } else {
	      if (ambiguous_operand_catch (message_stack (), 
					   method_message_ptr,
					   method)) {
		int __i, fn_arg_term_idx;
		ambiguous_operand_format 
		  (message_stack (), rcvr_ptr, 
		   ambiguous_arg_end_idx, output_buf);
		for (fn_arg_term_idx = method_message_ptr;
		     M_TOK(message_stack_at (fn_arg_term_idx)) != 
		       SEMICOLON; fn_arg_term_idx--)
		  ;
		for (__i = method -> n_args - 1; __i >= 0; __i--) {
		  generate_method_pop_arg_call (fn_arg_term_idx-1);
		  delete_arg_object (method -> args[__i] -> obj);
		}
		ambiguous_operand_reset ();
		method -> n_args = 0;
	      } else {
		if (arg_class == arg_rt_expr) {

		  output_arg_rt_expr (message_stack (), 
				      rcvr_ptr, method_message_ptr,
				      method);
		  arg_class = arg_null;
		  for (i = rcvr_ptr; i >= method_message_ptr; i--) {
		    ++(message_stack_at (i) -> evaled);
		    ++(message_stack_at (i) -> output);
		  }
		  return SUCCESS;
		} else {
		  if (m -> attrs & RCVR_OBJ_IS_SUBEXPR) {
		    /*
		     *  For parenthesized receiver expressions,
		     *  rcvr_ptr is from the method message's
		     *  receiver_msg, which points to the opening
		     *  parenthesis of the receiver expression,
		     *  set in have_subexpr_rcvr_class* ().
		     */
		    expr_end_idx = method_message_ptr;
		    fmt_rt_expr (message_stack (), rcvr_ptr, &expr_end_idx,
				 output_buf);
		    rcvr_cvar_registration = true;
 		    method_expr_is_c_fmt_arg 
 		      (message_stack (),
 		       expr_end_idx,
		       P_MESSAGES,
 		       get_stack_top (message_stack ()));
		  } else { /* if (m -> attrs & RCVR_OBJ_IS_SUBEXPR) */
		    if (m -> attrs & RCVR_OBJ_IS_C_ARRAY_ELEMENT) {
		      int __subscript_end_idx;
		      CVAR *c_rcvr;
		      MESSAGE *__m;
		      /*
		       *  Same as above, but register the receiver CVAR 
		       *  here, making sure that we get the subscript
		       *  expression and class right.
		       */
		      __m = message_stack_at (rcvr_ptr);
		      if (((c_rcvr = get_local_var (M_NAME(__m))) != NULL) ||
			  ((c_rcvr = get_global_var (M_NAME(__m))) != NULL)) {
			__subscript_end_idx = 
			     is_subscript_expr 
			  (message_stack (), rcvr_ptr, 
			   get_stack_top (message_stack ()));
			if ((__pfx_idx = 
			     is_leading_prefix_op (message_stack (),
						   rcvr_ptr,
						   P_MESSAGES))
			    != ERROR) {
			  if ((M_TOK(message_stack_at(__pfx_idx)) == INCREMENT) ||
			      (M_TOK(message_stack_at(__pfx_idx)) == DECREMENT)) {
			    prefix_rcvr_cvar_expr_registration 
			      (message_stack (), rcvr_ptr);
			  } else {
			    if ((__postfix_idx = 
				 is_trailing_postfix_op (message_stack (),
						 __subscript_end_idx,
					 get_stack_top (message_stack ()))) 
				!= ERROR) {
			      if ((M_TOK(message_stack_at(__postfix_idx)) 
				   == INCREMENT) ||
			      (M_TOK(message_stack_at(__postfix_idx)) 
			       == DECREMENT)) {
				postfix_rcvr_cvar_expr_registration 
				  (message_stack (), rcvr_ptr);
				rcvr_cvar_registration = true;
			      } else {/*if((M_TOK(message_stack_at(__postfix_idx*/ 
				output_CVAR_subscript_registration
				  (message_stack (), rcvr_ptr,
				   __subscript_end_idx, c_rcvr);
			      } /*if((M_TOK(message_stack_at(__postfix_idx*/ 
			    } else { /* if ((__postfix_idx =  ... */
			      output_CVAR_subscript_registration
				(message_stack (), rcvr_ptr,
				 __subscript_end_idx, c_rcvr);
			    } /* if ((__postfix_idx =  ... */
			  }
			} else {
			  output_CVAR_subscript_registration
			    (message_stack (), rcvr_ptr,
			     __subscript_end_idx, c_rcvr);
			}
		      }
		      expr_end_idx = method_message_ptr;
		      fmt_rt_expr (message_stack (), rcvr_ptr, &expr_end_idx,
				   output_buf);
		      if (rcvr_cvar_registration)
			mc_cvar_cleanup (message_stack (),
					 method_message_ptr);
		      method_expr_is_c_fmt_arg 
			(message_stack (),
			 expr_end_idx,
			 P_MESSAGES,
			 get_stack_top (message_stack ()));
		    } else { /* if (m -> attrs & RCVR_OBJ_IS_C_ARRAY_ELEMENT) */
		      if (m -> attrs & RCVR_OBJ_IS_C_STRUCT_ELEMENT) {
			int __st, __s_term;
			MESSAGE *__m;
			char __s[MAXMSG];
			CVAR *c_struct, *c_member;
			__st = get_stack_top (message_stack ());
			__m = message_stack_at (rcvr_ptr);
			if (((c_struct=get_local_var(M_NAME(__m)))!= NULL)||
			    ((c_struct=get_global_var(M_NAME(__m)))!=NULL)) {
			  if ((__s_term = struct_end 
			       (message_stack (), rcvr_ptr, __st)) > 0) {
			    toks2str (message_stack (), rcvr_ptr, __s_term, __s);
			    if ((c_struct->attrs & CVAR_ATTR_STRUCT_TAG) ||
				(c_struct->attrs & CVAR_ATTR_STRUCT_PTR_TAG)||
				((c_struct->attrs & CVAR_ATTR_STRUCT_PTR) &&
				 !c_struct -> members )){
			      CVAR *c_defn;
			      if (((c_defn = get_local_struct_defn (c_struct->type))
				  != NULL) ||
				  ((c_defn = get_global_var (c_struct->type))
				   != NULL))
				c_struct = c_defn;
			    }
			    if ((__pfx_idx = 
				 is_leading_prefix_op 
				 (message_stack (),
				  rcvr_ptr, 
				  P_MESSAGES))
				!= ERROR) {
			      if ((M_TOK(message_stack_at(__pfx_idx)) 
				   == INCREMENT) ||
				  (M_TOK(message_stack_at(__pfx_idx)) 
				   == DECREMENT)) {
				prefix_rcvr_cvar_expr_registration 
				  (message_stack (), rcvr_ptr);
			      } else { /* if ((__pfx_idx =  ... */
 				__s_term =
 				  struct_end
 				  (message_stack (),
 				   rcvr_ptr,
 				   get_stack_top (message_stack ()));
  				if ((__postfix_idx =
  				     is_trailing_postfix_op (message_stack (),
 							     __s_term,
  					 get_stack_top (message_stack ())))
 				    != ERROR) {
				  if ((M_TOK(message_stack_at(__postfix_idx)) 
				       == INCREMENT) ||
				      (M_TOK(message_stack_at(__postfix_idx)) 
				       == DECREMENT)) {
				    postfix_rcvr_cvar_expr_registration
				      (message_stack (), rcvr_ptr);
				  } else {/* if ((M_TOK(message_stack_at...*/
				    if((c_member = struct_member_from_expr_b
					(message_stack (), 
					 rcvr_ptr, 
					 __s_term,
					 c_struct)) != NULL) {
				      fileout (fmt_register_c_method_arg_call
					       (c_member, __s,
						(is_global_frame () ?
						 GLOBAL_VAR : LOCAL_VAR),
						output_buf),
					       0, FRAME_START_IDX);
				    }
				  }/* if ((M_TOK(message_stack_at...*/
				} else { /* if ((__postfix_idx = ... */
				  if((c_member = struct_member_from_expr_b 
				      (message_stack (), 
				       rcvr_ptr,
				       __s_term,
				       c_struct)) != NULL) {
				    fileout (fmt_register_c_method_arg_call
					     (c_member, __s, 
					      (is_global_frame () ?
					       GLOBAL_VAR : LOCAL_VAR),
					      output_buf),
					     0, FRAME_START_IDX);
				  }
 				}/* if ((__postfix_idx = ... */
			      } /* if ((__pfx_idx =  ... */
			      expr_end_idx = method_message_ptr;
			      struct_elide_parentheses_kludge 
				(message_stack (), rcvr_ptr, 
				 expr_end_idx);
			      fmt_rt_expr (message_stack (), rcvr_ptr,
					   &expr_end_idx, output_buf);
			      method_expr_is_c_fmt_arg 
				(message_stack (),
				 expr_end_idx,
				 P_MESSAGES,
				 get_stack_top (message_stack ()));
			    }
			  } /* if ((__s_term = struct_end  */
			} /* if (((c_struct= ... */
		      } else {/*if (m -> attrs & RCVR_OBJ_IS_C_STRUCT_ELEMENT)*/
			if (prefix_rcvr_cvar_expr_registration 
			    (message_stack (), rcvr_ptr)
			    == ERROR) {
			  if (postfix_rcvr_cvar_expr_registration
			      (message_stack (), rcvr_ptr) == ERROR) {
			    (void)register_c_receiver_var
			      (message_stack (), 
			       rcvr_ptr, 
			       method_message_ptr);
			  }
			}
			if (message_stack_at (rcvr_ptr) -> attrs &
			    TOK_IS_PRINTF_ARG) {
			  /* if there are  opening parentheses in front of
			     the receiver, include them in the expression,
			     and output it here. */
			  if ((fmt_arg_prev_idx =
			       prevlangmsg (message_stack (), rcvr_ptr))
			      != ERROR) {
			    fmt_arg_m_prev = message_stack_at (fmt_arg_prev_idx);
			    if (M_TOK(fmt_arg_m_prev) == OPENPAREN) {
			      int expr_end;
			      char expr_buf_tmp_2[MAXMSG];
			      MSINFO ms;
			      fmt_arg_prev_idx =
				first_open_paren( message_stack (), fmt_arg_prev_idx);
			      fmt_arg_m_prev = message_stack_at (fmt_arg_prev_idx);
			      ms.messages = message_stack ();
			      ms.stack_start = P_MESSAGES;
			      ms.stack_ptr = get_stack_top (message_stack ());
			      ms.tok = fmt_arg_prev_idx;
			      char *expr_buf_tmp  =
				collect_expression (&ms, &expr_end);
			      fmt_eval_expr_str (expr_buf_tmp, expr_buf_tmp_2);
			      __xfree (MEMADDR(expr_buf_tmp));
			      fmt_printf_fmt_arg (message_stack (),
						  fmt_arg_prev_idx,
						  P_MESSAGES,
						  expr_buf_tmp_2, output_buf);
			      for (i = fmt_arg_prev_idx; i >= expr_end; --i) {
				++message_stack_at (i) -> evaled;
				++message_stack_at (i) -> output;
			      }
			      fileout (output_buf, FALSE, fmt_arg_prev_idx);
			      return SUCCESS;
			    } else {
			      rt_library_method_call (receiver, method,
						      message_stack (),
						      method_message_ptr, output_buf);
			    }
			  } else {
			    rt_library_method_call (receiver, method, message_stack (),
						    method_message_ptr, output_buf);
			  }
			} else { /* if (context == c_argument_context) */
			  rt_library_method_call (receiver, method, message_stack (),
						  method_message_ptr, output_buf);
			} /* if (context == c_argument_context) */
		      } /*if (m -> attrs & RCVR_OBJ_IS_C_STRUCT_ELEMENT)*/
		    } /* if (m -> attrs & RCVR_OBJ_IS_C_ARRAY_ELEMENT) */
		  } /* if (m -> attrs & RCVR_OBJ_IS_SUBEXPR) */
		}
	      }
	      if (is_global_frame ())
		fn_init 
		  (obj_2_c_wrapper (m, receiver, method, output_buf, TRUE), 0);
	      else 
		/*
		 *  Other object-to-c translations do not at this
		 *  point require us to discard an extra object.
		 */
		fileout 
		  (obj_2_c_wrapper_trans (message_stack (), rcvr_ptr,
			  m, receiver, method, output_buf,
			  ((context == c_context) || 
			   (context == c_argument_context) ? FALSE : TRUE)), 0,
		   method_message_ptr);
	      if (rcvr_cvar_registration)
		mc_cvar_cleanup (message_stack (), method_message_ptr);
 	    }
	  }
	  break;
	default:
	  /*
	   *  Don't depend on context if evaluating a do predicate in
	   *  its own frame with a sub-parser.  The same with for_term
	   *  and for_inc methods.  The latter two have receiver
	   *  context due to the preceding semicolon, but they are 
	   *  actually in C context.
	   *
	   *  TO DO - These cases should probably be handled in 
	   *  object_context (), and there probably will be 
	   *  unforseen cases where one of these would receive c_context
	   *  or c_argument_context; e.g. while (c = getchar ()). {...
	   *
	   *  Class-specific constructors get added to the function
	   *  initialization.
	   */
	  if (ctrlblk_pred && (do_predicate || for_init || for_term || for_inc)) {
	    ctrlblk_method_call (receiver, method, method_message_ptr);
	  } else {
	    if (!strcmp (method -> name, "new")) {
	      char mcbuf[MAXMSG];
	      char fmt_buf[MAXLABEL];
	      strcatx (fmt_buf, fmt_method_call (receiver,
						 method -> selector,
						 method -> name, mcbuf), 
		       ";\n",
		       NULL);
		       
	      queue_init_statement (fmt_buf, FALSE, 0);
	      /*
	       *  This is kludgy, but it will have to work until
	       *  there is a way to duplicate a non-primitive, "new,"
	       *  method.
	       */
	      if (is_global_frame ())
		generate_set_global_variable_call 
		  (method -> args[0] -> obj -> __o_name,
		   receiver -> __o_name);
	      else
		generate_set_local_variable_call (method -> args[0] -> obj -> __o_name,
						  receiver -> __o_name);
	    } else {
	      if (ambiguous_operand_catch (message_stack (), 
					 method_message_ptr,
					   method)) {
		switch (context)
		  {
		  case receiver_context:
		    fileout 
		      (ambiguous_operand_format
		       (message_stack (), rcvr_ptr, ambiguous_arg_end_idx,
			output_buf),
		       rcvr_ptr, 0);
		    ambiguous_operand_reset ();
		    break;
		  default:
		    warning 
		      (m,"Ambiguous method %s message not in receiver context.",
		       M_NAME(m));
		    fileout 
		      (ambiguous_operand_format
		       (message_stack (), rcvr_ptr, ambiguous_arg_end_idx,
			output_buf),
		       rcvr_ptr, 0);
		    ambiguous_operand_reset ();
		    break;
		  }
	      } else {
		if (arg_class == arg_rt_expr) {
		  output_arg_rt_expr (message_stack (), 
				      rcvr_ptr, method_message_ptr,
				      method);
		  arg_class = arg_null;
		  for (i = rcvr_ptr; i >= method_message_ptr; i--) {
		    ++(message_stack_at (i) -> evaled);
		    ++(message_stack_at (i) -> output);
		  }
		  goto cleanup_args;
		} else {
		  /*
		   *  TODO -
		   *  Not needed if there's no CVAR in the expression.
		   *  Check before calling these? ... 
		   *
		   *   
		   */
		  if (prefix_rcvr_cvar_expr_registration (message_stack(), rcvr_ptr)
		      == ERROR) {
		    /* ... otherwise, just register the CVAR - 
		       any previous __ctalk_arg call will remove
		       the CVAR immediately after pushing the
		       arg onto the stack */
		    if (register_c_receiver_var
			(message_stack (), rcvr_ptr, 
			 method_message_ptr)) 
		      rcvr_cvar_registration = true;
		  }
		  if ((lookahead = nextlangmsg (message_stack (),
						method_message_ptr)) != ERROR) {
		    if ((M_TOK(message_stack_at (lookahead)) == CLOSEPAREN) ||
			(M_TOK(message_stack_at (lookahead)) == ARGSEPARATOR)) {
		      /* This is like generate_method_call, but we don't
			 add the semicolonr if the method call is a
			 function argument or otherwise enclosed in
			 parens */
		      char _buf[MAXMSG];
		      if (is_global_frame ())
			fn_init (fmt_method_call (receiver,
						  method -> selector,
						  method -> name, _buf), FALSE);
		      else
			fileout (fmt_method_call (receiver,
						  method -> selector,
						  method -> name, _buf),
				 0, method_message_ptr);
		    } else {
		      generate_method_call (receiver, method -> selector,
					    method -> name,
					    method_message_ptr);
		    }
		  }
		  if (rcvr_cvar_registration) {
		    if (method -> n_params == 0) {
		      output_delete_cvars_call
			(message_stack (), method_message_ptr,
			 get_stack_top (message_stack ()));
		    }
		  }
		}
	      }
	    }
	  }
	  break;
	} /* switch */
    }

  cleanup_args:
    if (ctrlblk_pred) {
      stmt_end_ptr = C_CTRL_BLK -> blk_end_ptr - 1;
      ctrlblk_cleanup_args (method, stmt_end_ptr);
    } else {
      /*
       *  Expr_check can use only one frame, so check
       *  here before before trying to find the next
       *  frame.
       */
      if (!((arg_class == arg_rt_expr) || 
	    (interpreter_pass == expr_check))) {
	cleanup_args (method, m -> receiver_obj,
		      (frame_at (CURRENT_PARSER -> frame - 1) ->
		       message_frame_top) + 1);
      }
    }

    if (expr_end_idx != -1) {
      for (i = rcvr_ptr; i >= expr_end_idx; i--) {
	++(message_stack_at (i) -> evaled);
	++(message_stack_at (i) -> output);
      }
    } else {
      for (i = rcvr_ptr; i >= method_message_ptr; i--) {
	++(message_stack_at (i) -> evaled);
	++(message_stack_at (i) -> output);
      }
    }

  }

  return SUCCESS;
}

/* 
 *
 *   Functions to manage the method parser stack.
 *
 */

int init_method_stack (void) {
  m_message_ptr = N_MESSAGES;
  _new_hash (&declared_method_names);
  _new_hash (&declared_method_selectors);
  return SUCCESS;
}

int method_message_push (MESSAGE *m) {
  if (m_message_ptr == 0) {
    warning (m, "Method_message_push: stack_overflow.");
    return ERROR;
  }

  if (!m) {
    _warning ("Method_message_push: null pointer, messageptr = %d.", 
	     m_message_ptr);
    return ERROR;
  }

  m_messages[m_message_ptr] = m;

#ifdef STACK_TRACE
  warning (m_messages[m_message_ptr], "Method_message_push %d. %s.", 
	   m_message_ptr, m_messages[m_message_ptr] -> name); 
#endif

  --m_message_ptr;

  return m_message_ptr;
}

MESSAGE *method_message_pop (void) {

  if (m_message_ptr == N_MESSAGES)
    return NULL;

  if (m_message_ptr > N_MESSAGES) {
    _warning ("Method_message_pop: Stack overflow, m_message_ptr = %d.", 
	    m_message_ptr);
    return NULL;
  }

#ifdef STACK_TRACE
  warning (m_messages[m_message_ptr+1],
	   "Method_message_pop %d. %s.", m_message_ptr, m -> name); 
#endif

  return m_messages[++m_message_ptr];
}

int store_arg_object (METHOD *m, OBJECT *arg) {

  if (interpreter_pass == expr_check) {
    if (arg -> scope == ARG_VAR) 
      delete_object (arg);
    return SUCCESS;
  }
  m -> args[m -> n_args++] = create_arg_init (arg);
  return SUCCESS;
}

/*
 *  Clear the method arguments and generate a __ctalk_arg_cleanup () 
 *  call in the output.  Also delete temporary arg objects, and
 *  remove the message pointer from the argument stack.
 *
 *  If we're in a loop, ctrlblk_cleanup_args () in control.c performs
 *  this task after it queues the __ctalk_arg_cleanup () calls for the
 *  block predicates.
 *
 *  Whether to use stmt_end_ptr - 1 is questionable, because in
 *  a single-line control statement block, the semicolon also
 *  anchors the closing brace of an, "if," clause without an,
 *  "else," or the closing brace of an, "else," clause.
 */

void cleanup_args (METHOD *m, OBJECT *receiver_object, int stmt_end_ptr) {
  int i;
  int collection_rt_eval = FALSE;

  if (interpreter_pass == expr_check) return;

  if (IS_CLASS_OBJECT(receiver_object)) {
    if (collection_needs_rt_eval (receiver_object, m))
      collection_rt_eval = TRUE;
  } else {
    if (collection_needs_rt_eval 
	/* (get_class_object(receiver_object->__o_classname)*/
	(receiver_object -> __o_class, m))
      collection_rt_eval = TRUE;
  }

  for (i = m -> n_args - 1; i >= 0; i--) {
    if (!collection_rt_eval) {
      if (!m -> cfunc)
	generate_method_pop_arg_call (stmt_end_ptr - 1);
    }
    if (m -> args[i] -> obj -> scope == ARG_VAR) 
      delete_arg_object (m -> args[i] -> obj);
    delete_arg (m -> args[i]);
    m -> args[i] = NULL;
    m -> n_args = i;
  }
}

void init_imported_method_queue (void) {
  imported_method_ptr = 0;
}

void queue_method_for_output (OBJECT *class, METHOD *m) {

  IMPORTED_METHOD *i;
  int includes_ptr;

  if (m -> queued == False) {

    if ((i = (IMPORTED_METHOD *)__xalloc (sizeof (IMPORTED_METHOD))) == NULL)
      _error ("queue_method_for_output: %s\n", strerror (errno));

    i -> classobj = class;
    if ((includes_ptr = get_lib_includes_ptr ()) < MAXARGS) {
      i -> source_file = lib_include_at (includes_ptr + 1) -> path;
    } else {
      i -> source_file = __source_filename ();
    }
    i -> method = m;

    imported_methods[imported_method_ptr++] = i;
  }

}

extern MESSAGE *p_messages[P_MESSAGES+1]; /* Declared in preprocess.c. */

/* 
 *  Output imported methods.  The methods are preprocessed when
 *  retrieved by a library search, and added to the interpreter
 *  object's dictionary.  
 */

void output_imported_methods (void) {

  int i;
  METHOD *m;
  char line_info_buf[MAXMSG];
  char *p, *r;

  for (i = 0; i < imported_method_ptr; i++) {
    if (imported_methods[i] -> method -> imported == True) {

      imported_methods[i] -> method -> imported = False;
      imported_methods[i] -> method -> queued = False;

      m = NULL;
      for (m = imported_methods[i] -> classobj -> instance_methods;
	   m; m = m -> next)
	if (!strcmp (m -> name, imported_methods[i] -> method -> name))
	  break;
      if (!m)
	for (m = imported_methods[i] -> classobj -> class_methods;
	     m; m = m -> next)
	  if (!strcmp (m -> name, imported_methods[i] -> method -> name))
	    break;

      if (pre_method_is_output (imported_methods[i] -> method -> selector)) {
	__xfree (MEMADDR(imported_methods[i]));
	continue;
      }

      if (!m)
	_error ("output_imported_methods: Undefined %s method %s.\n",
		imported_methods[i] -> classobj -> __o_classname, 
		imported_methods[i] -> method -> name);

      /* The method initialization code is inserted in the 
	 source by new_instance|class_method ()'s call to parse (). */
      if (!nolinemarker_opt) {
	fmt_line_info (line_info_buf,
		       imported_methods[i] -> method -> error_line,
		       imported_methods[i] -> source_file,
		       0,
		       TRUE);
	__fileout (line_info_buf);
      }

      while ((r = strstr (imported_methods[i] -> method -> src, 
			  FIXUPROOT)) != NULL) {

	p = r;     /* Find start of fixup marker. */
	while (*(--p) != '#')
	  ;

	do {
	  *p = ' ';
	  ++p;
	} while (*p != '\n');

      }

      __fileout_import (imported_methods[i] -> method -> src);
      
      pre_method_register_selector_as_output 
	(imported_methods[i] -> method ->selector);
    }

    __xfree (MEMADDR(imported_methods[i]));

  }

  imported_method_ptr = 0;

}

int is_method_name (const char *name) {
  if (declared_method_names) {
    if (_hash_get (declared_method_names, name)) {
      return TRUE;
    }
  }
  return FALSE;
}

int is_method_selector (const char *selector) {
  if (_hash_get (declared_method_selectors, selector)) 
    return TRUE;
  else
    return FALSE;
}

/*
 * save_method_local_objects () and save_method_local_cvars ()
 * are sort of a kludge, because at the moment it is not possible
 * to output buffered methods from within the parser, where 
 * the variables are stored.  So we save them in the method
 * for use by format_method (), after the sub-parser returns.
 * 
 * If the method buffering gets fixed in a future release, 
 * then these functions, as well as the local_object and 
 * local_cvar members of the method structs, should go away.
 */

void save_method_local_objects (void) {
  PARSER *p;
  p = CURRENT_PARSER;
  M_LOCAL_OBJ_LIST(new_methods[new_method_ptr+1]->method) = p -> vars;
  p -> vars = NULL;
}

void save_method_local_cvars (void) {
  PARSER *p;
  METHOD *new_method;

  p = CURRENT_PARSER;

  new_method = new_methods[new_method_ptr + 1] -> method;
  new_method -> local_cvars = p -> cvars;
  p -> cvars = NULL;
}

OBJECT *get_instance_method_local_object (char *s) {
  METHOD *m;
  OBJECT *o;

  if (new_method_ptr >= MAXARGS)
    return NULL;

  m = new_methods[new_method_ptr + 1] -> method;

  for (o = M_LOCAL_OBJ_LIST(m); o; o = o -> next) {
    if (!strcmp (o -> __o_name, s))
      return o;
  }
  return NULL;
}

CVAR *get_instance_method_local_cvar (char *s) {
  METHOD *m;
  CVAR *c;

  if (new_method_ptr >= MAXARGS)
    return NULL;

  m = new_methods[new_method_ptr + 1] -> method;

  for (c = m -> local_cvars; c; c = c -> next) {
    if (!strcmp (c -> name, s))
      return c;
  }
  return NULL;
}

/*
 *  Determine if a label is an instance method.  
 *  TO DO - Only works for a limited number of 
 *  cases right now.
 */

bool is_instance_method (MESSAGE_STACK messages, int msg_ptr) {

  METHOD *method;
  OBJECT *possible_rcvr;
  int prev_label_ptr;

  if (messages[msg_ptr] -> tokentype == METHODMSGLABEL)
    return True;

  switch (interpreter_pass)
    {
    case method_pass:
      /*
       *  TO DO - This case is too limited.
       */
      if ((method = get_instance_method (messages[msg_ptr],
					 rcvr_class_obj, 
					 messages[msg_ptr] -> name, 
					 ERROR, FALSE)) != NULL) {
	return True;
      }
      break;
    case parsing_pass:
    case expr_check:
      if (is_c_constant_instance_method (messages, msg_ptr))
	return True;
      if (!rcvr_class_obj) {
	if ((prev_label_ptr = prevlangmsg (messages, msg_ptr)) != ERROR) {
	  if ((possible_rcvr = 
	       get_object (messages[prev_label_ptr] -> name, NULL)) != NULL) {
	    if ((method = get_instance_method (messages[msg_ptr], 
					       possible_rcvr, 
					       messages[msg_ptr] -> name, 
					       ERROR, FALSE)) 
		!= NULL) {
	      return True;
	    }
	  }
	}
      }
      break;
    default:
      break;
    }
  
  return False;
}

bool is_instance_method_or_variable (MESSAGE_STACK messages, int msg_ptr) {

  METHOD *method;
  OBJECT *possible_rcvr;
  int prev_label_ptr;

  if (messages[msg_ptr] -> tokentype == METHODMSGLABEL)
    return True;

  if (messages[msg_ptr] -> attrs & TOK_IS_RT_EXPR)
    return True;

  switch (interpreter_pass)
    {
    case method_pass:
      /*
       *  TO DO - This case is too limited.
       */
      if (!method_from_proto) {
	if ((method = get_instance_method (messages[msg_ptr],
					   rcvr_class_obj, 
					   messages[msg_ptr] -> name, 
					   ERROR, FALSE)) != NULL) {
	  return True;
	}
      }
      break;
    case parsing_pass:
    case expr_check:
      if (is_c_constant_instance_method (messages, msg_ptr))
	return True;
      if (!rcvr_class_obj) {
	if ((prev_label_ptr = prevlangmsg (messages, msg_ptr)) != ERROR) {
	  if ((possible_rcvr = 
	       get_object (messages[prev_label_ptr] -> name, NULL)) != NULL) {
	    if ((method = get_instance_method (messages[msg_ptr], 
					       possible_rcvr, 
					       messages[msg_ptr] -> name, 
					       ERROR, FALSE)) 
		!= NULL) {
	      return True;
	    } else {
	      OBJECT *__v;
	      for (__v = possible_rcvr -> instancevars; __v; 
		   __v=__v->next) {
		if (!strcmp (__v -> __o_name, M_NAME(messages[msg_ptr])))
		  return TRUE;
	      }
	    }
	  }
	}
      } else { /*       if (!rcvr_class_obj) { */
	if ((prev_label_ptr = prevlangmsg (messages, msg_ptr)) != ERROR) {
	  if ((possible_rcvr = 
	       get_object (messages[prev_label_ptr] -> name, NULL)) != NULL) {
	    OBJECT *__v;
	    for (__v = possible_rcvr -> instancevars; __v; 
		 __v=__v->next) {
	      if (!strcmp (__v -> __o_name, M_NAME(messages[msg_ptr])))
		return TRUE;
	    }
	  }
	}
      }
      break;
    default:
      break;
    }
  
  return False;
}

bool is_class_method (MESSAGE_STACK messages, int msg_ptr) {

  METHOD *method;
  OBJECT *possible_rcvr;
  int prev_label_ptr;

  if (messages[msg_ptr] -> tokentype == METHODMSGLABEL)
    return True;

  switch (interpreter_pass)
    {
    case method_pass:
      /*
       *  TO DO - This case is too limited.
       */
      if ((method = get_class_method 
	   (messages[msg_ptr],
	    rcvr_class_obj, messages[msg_ptr] -> name, 
	    ERROR, FALSE)) != NULL)
	return True;
      break;
    case parsing_pass:
    case expr_check:
      /*
       *  Should not be needed for class objects.
       */
      if (!rcvr_class_obj) {
	if ((prev_label_ptr = prevlangmsg (messages, msg_ptr)) != ERROR) {
	  if ((possible_rcvr = 
	       get_object (messages[prev_label_ptr] -> name, NULL)) != NULL) {
	    if ((method = get_class_method 
		 (messages[msg_ptr], possible_rcvr, 
		  messages[msg_ptr] -> name, ERROR, FALSE)) 
		!= NULL) {
	      return True;
	    }
	  }
	}
      }
      break;
    default:
      break;
    }
  
  return False;
}

/*
 *  Look back to the start of a frame, and find the previous
 *  identifier.  If the receiver is a C constant expression, 
 *  look for a method in the corresponding class.  Look back further 
 *  if the receiver is a C struct or array member.
 *
 *  NOTE - This function also returns False if a class library is
 *  not loaded yet.
 */
bool is_c_constant_instance_method (MESSAGE_STACK messages, int msg_ptr) {

  int lookback,
    stack_begin,
    n_blocks;
  bool is_array_member,
    is_struct_mbr;
  OBJECT *rcvr_obj;
  MESSAGE *m;
  METHOD *method;
  CVAR *c;

  if ((interpreter_pass != parsing_pass) &&
      (interpreter_pass != method_pass))
    return False;

  stack_begin = stack_start (messages);
  is_array_member = is_struct_mbr = False;
  rcvr_obj = NULL;

  for (lookback = msg_ptr + 1; lookback <= stack_begin; lookback++) {

    m = messages[lookback];

    if ((m -> tokentype == NEWLINE) || (m -> tokentype == WHITESPACE))
      continue;
	
    switch (m -> tokentype)
      {
      case ARRAYCLOSE:
	n_blocks = 1;
	for (++lookback; (lookback <= stack_begin) && n_blocks; lookback++) {
	  switch (messages[lookback] -> tokentype)
	    {
	    case ARRAYCLOSE:
	      ++n_blocks;
	      break;
	    case ARRAYOPEN:
	      --n_blocks;
	      break;
	    }
	}
	/*
	 *  GNUC leaves lookback at one ahead of the last block.
	 *  Decrement lookback so it is set correctly for the 
	 *  next iteration.
	 */
	--lookback;
	is_array_member = True;
	break;
      case LITERAL:
	rcvr_obj = get_class_object (STRING_CLASSNAME);
	if (IS_OBJECT (rcvr_obj)) {
	  if ((method = 
	       get_instance_method (messages[msg_ptr],
				    rcvr_obj, messages[msg_ptr] -> name, 
				    ERROR, FALSE))
	      != NULL)
	    return True;
	  else
	    return False;
	} else {
	  return False;
	}
	break;
      case LITERAL_CHAR:
	rcvr_obj = get_class_object (CHARACTER_CLASSNAME);
	if (IS_OBJECT (rcvr_obj)) {
	  if ((method = 
	       get_instance_method (messages[msg_ptr],
				    rcvr_obj, messages[msg_ptr] -> name, 
				    ERROR, FALSE))
	      != NULL)
	    return True;
	  else
	    return False;
	} else {
	  return False;
	}
	break;
      case INTEGER:
      case LONG:
	rcvr_obj = get_class_object (INTEGER_CLASSNAME);
	if (IS_OBJECT (rcvr_obj)) {
	  if ((method = 
	       get_instance_method (messages[msg_ptr],
				    rcvr_obj, messages[msg_ptr] -> name, 
				    ERROR, FALSE))
	      != NULL)
	    return True;
	  else
	    return False;
	} else {
	  return False;
	}
	break;
      case LONGLONG:
	rcvr_obj = get_class_object (LONGINTEGER_CLASSNAME);
	if (IS_OBJECT (rcvr_obj)) {
	  if ((method = 
	       get_instance_method (messages[msg_ptr],
				    rcvr_obj, messages[msg_ptr] -> name, 
				    ERROR, FALSE))
	      != NULL)
	    return True;
	  else
	    return False;
	} else {
	  return False;
	}
	break;
      case DOUBLE:
	rcvr_obj = get_class_object (FLOAT_CLASSNAME);
	if (IS_OBJECT (rcvr_obj)) {
	  if ((method = 
	       get_instance_method (messages[msg_ptr],
				    rcvr_obj, messages[msg_ptr] -> name, 
				    ERROR, FALSE))
	      != NULL)
	    return True;
	  else
	    return False;
	} else {
	  return False;
	}
	break;
      case LABEL:
	if (is_array_member) {
	  if (((c = get_local_var (m -> name)) != NULL) ||
	      ((c = get_global_var (m -> name)) != NULL)) {
	    switch (c -> n_derefs - 1)
	      {
	      case 0:
		if (!strcmp (c -> type, "char"))
		  rcvr_obj = get_class_object (CHARACTER_CLASSNAME);
		if (!strcmp (c -> type, "int")) {
		  if (!strcmp (c -> qualifier2, "long") &&
		      !strcmp (c -> qualifier, "long")) {
		    rcvr_obj = get_class_object (LONGINTEGER_CLASSNAME);
		  } else {
		    rcvr_obj = get_class_object (INTEGER_CLASSNAME);
		  }
		}
		if (!strcmp (c -> type, "double") ||
		    !strcmp (c -> type, "float"))
		  rcvr_obj = get_class_object (FLOAT_CLASSNAME);
		break;
	      case 1:
		if (!strcmp (c -> type, "char"))
		  rcvr_obj = get_class_object (STRING_CLASSNAME);
		if (!strcmp (c -> type, "int") ||
		    !strcmp (c -> type, "double") ||
		    !strcmp (c -> type, "float"))
		  rcvr_obj = get_class_object (ARRAY_CLASSNAME);
		break;
	      case 2:
		rcvr_obj = get_class_object (ARRAY_CLASSNAME);
		break;
	      case -1:
		break;
	      } /* switch (c -> n_derefs) */
	    if (IS_OBJECT (rcvr_obj)) {
	      if ((method = 
		   get_instance_method (messages[msg_ptr],
					rcvr_obj, messages[msg_ptr] -> name, 
					ERROR, FALSE))
		  != NULL)
		return True;
	      else
		return False;
	    } else {
	      /*
	       *  Note that this case can mean that a class is not loaded
	       *  yet.
	       */
	      return False;
	    }
	  } /* 	  if (((c = get_local_var (m -> name)) != NULL) ||
		      ((c = get_global_var (m -> name)) != NULL)) { */
	} /* 	if (is_array_member) */

	break;
      }
  }
  return False;
}

static inline void __set_arg_message_name (MESSAGE *m, char *s) {
  strcpy (m -> name, s);
}

/*
 *  Replace a method parameter with its argument accessor function.
 *  This function is only used in a limited number of cases so far,
 *  so it won't be updated without more test cases.
 */

int method_arg_accessor_fn (MESSAGE_STACK messages, int idx, int arg_n,
			    OBJECT_CONTEXT context, bool varargs) {

  char expr_tmp[MAXMSG];
  switch (context)
    {
    case c_context:
    case c_argument_context:
      if (fmt_arg_type (messages, idx, stack_start (messages)) ==
	  fmt_arg_char_ptr) {
	strcatx (messages[idx] -> name, STRING_TRANS_FN, "(",
		 format_method_arg_accessor
		 (arg_n, M_NAME(messages[idx]), false,expr_tmp),
		 "1,)", NULL);
      } else {
	__set_arg_message_name (messages[idx], 
				format_method_arg_accessor 
				(arg_n, M_NAME(messages[idx]),
				 varargs, expr_tmp));
      }
      break;
    default:
      __set_arg_message_name (messages[idx], 
			      format_method_arg_accessor 
			      (arg_n, M_NAME(messages[idx]),
			       varargs, expr_tmp));
      break;
    }

  messages[idx] -> obj = 
    create_object_init (CFUNCTION_CLASSNAME, 
			CFUNCTION_SUPERCLASSNAME,
			messages[idx] -> name,
			messages[idx] -> name);
  save_method_object (messages[idx]->obj);
  
  return SUCCESS;
}

/*
 *  Should only be called within a C context.
 */
int method_arg_accessor_fn_c (MESSAGE_STACK messages, int idx, 
			      int method_n_th_param) {

  int fn_label_idx, fn_arg_idx, next_tok_idx; 
  CFUNC *cfn;
  METHOD *method;
  char expr_buf[MAXMSG];
  char arg_expr_buf[MAXMSG];

  if (fmt_arg_type (messages, idx, stack_start (messages)) ==
      fmt_arg_char_ptr) {
    strcatx (messages[idx]->name, STRING_TRANS_FN, " (",
	     format_method_arg_accessor (method_n_th_param,
					 M_NAME(messages[idx]),
					 false, expr_buf), ", 1)",
	     NULL);
  } else {
    if ((fn_arg_idx = obj_expr_is_arg (messages, idx, 
				  stack_start (messages),
				       &fn_label_idx)) == ERROR) {
      /*
       *  Not within a function.
       */
      __set_arg_message_name (messages[idx],
			      format_method_arg_accessor 
			      (method_n_th_param, M_NAME(messages[idx]),
			       false, expr_buf));
    } else {
      if (!strcmp (M_NAME(messages[fn_label_idx]), CHAR_TRANS_FN) ||
	  !strcmp (M_NAME(messages[fn_label_idx]), FLOAT_TRANS_FN) ||
	  !strcmp (M_NAME(messages[fn_label_idx]), INT_TRANS_FN) ||
	  !strcmp (M_NAME(messages[fn_label_idx]), LONGLONGINT_TRANS_FN) ||
	  !strcmp (M_NAME(messages[fn_label_idx]), STRING_TRANS_FN) ||
	  !strcmp (M_NAME(messages[fn_label_idx]), STRING_TRANS_FN_OLD) ||
	  !strcmp (M_NAME(messages[fn_label_idx]), PTR_TRANS_FN) ||
	  !strcmp (M_NAME(messages[fn_label_idx]), ARRAY_TRANS_FN) ||
	  !strcmp (M_NAME(messages[fn_label_idx]), ARRAY_INT_TRANS_FN)) {
	/*
	 *  User has added a translation function manually.
	 */
	__set_arg_message_name (messages[idx],
				format_method_arg_accessor
				(method_n_th_param, M_NAME(messages[idx]),
				 false, expr_buf));
      } else {
	if ((cfn = get_function (M_NAME(messages[fn_label_idx]))) == NULL) {
	  _warning ("method_arg_accesor_fn_c: function %s not found.\n",
		    M_NAME(messages[fn_label_idx]));
	  return ERROR;
	} else {
	  OBJECT *__param_class_object, *__param_superclass_object;
	  method = new_methods[new_method_ptr+1] -> method;
	  if ((__param_class_object = 
	       get_class_object  (method->params[method_n_th_param]->class))
	      == NULL) {
	    __param_class_object = 
	      get_class_object (OBJECT_CLASSNAME);
	  }
	  __param_superclass_object = __param_class_object -> __o_superclass;
	  /*
	   *  Lazy.  If the receiver is a method argument, and with 
	   *  this front end, the evaluation must wait until run time.  
	   *  Fake our way through based on the expression's syntax.
	   */
	  if (((next_tok_idx = nextlangmsg (messages, idx)) != ERROR) &&
	      (is_method_name (M_NAME(messages[next_tok_idx])) ||
	       (M_TOK(messages[next_tok_idx]) == LABEL))) {
	    method_arg_rt_expr (messages, idx);
	  } else {
	    format_method_arg_accessor (method_n_th_param,
					M_NAME(messages[idx]),
					method -> varargs,
					arg_expr_buf);
						
	    messages[idx] -> obj =
	      create_object_init (__param_class_object -> __o_name,
				  ((__param_superclass_object) ?
				   __param_superclass_object -> __o_name :
				   NULL),
				  M_NAME(messages[idx]),
				  M_NAME(messages[idx]));
	    strcpy (messages[idx] -> name,
		    obj_arg_to_c_expr 
		    (messages, fn_label_idx,
		     /* Compatiblize */
		     messages[idx] -> obj, fn_arg_idx + 1)); 
#ifdef SYMBOL_SIZE_CHECK
	    check_symbol_size (messages[idx] -> name);
#endif	    
	    delete_object (messages[idx] -> obj);
	  }
	}
      }
    }
  }

  messages[idx] -> obj = 
    create_object_init (CFUNCTION_CLASSNAME, 
			CFUNCTION_SUPERCLASSNAME,
			messages[idx] -> name,
			messages[idx] -> name);
  save_method_object (messages[idx]->obj);
  
  return SUCCESS;
}

static void format_plain_param_expr (MESSAGE_STACK messages,
				     int expr_start,
				     int expr_end) {
  char tokenbuf[MAXMSG];
  int i;
  toks2str (messages, expr_start, expr_end, tokenbuf);
  fmt_eval_expr_str (tokenbuf, messages[expr_start] -> name);
  messages[expr_start] -> tokentype = RESULT;
    
  for (i = expr_start - 1; i >= expr_end; i--) {
    _MPUTCHAR(messages[i], ' ');
    messages[i] -> tokentype = RESULT;
  }
}

static void format_plain_param_subscr_expr (MESSAGE_STACK messages,
					    int expr_start,
					    int expr_end) {
  char tokenbuf[MAXMSG], tokenbuf2[MAXMSG];
  int i;
  toks2str (messages, expr_start, expr_end, tokenbuf);
  fmt_eval_expr_str (tokenbuf, tokenbuf2);
  strcatx (messages[expr_start] -> name, "__ctalkToCInteger (",
	   tokenbuf2, ",1)", NULL);
  messages[expr_start] -> tokentype = RESULT;
    
  for (i = expr_start - 1; i >= expr_end; i--) {
    _MPUTCHAR(messages[i], ' ');
    messages[i] -> tokentype = RESULT;
  }
}

/*
 *  Called normally when we have a method call for
 *  that we might not yet have a class for; e.g., 
 *  method parameter receivers in the argument 
 *  of another method or C expression..
 *
 *  TODO - Can also be called for an argument to an 
 *  unprototyped C function - needs a complicated backtracking
 *  check to determine whether the message is an argument 
 *  to a method or a C function, and should issue a warning
 *  in that case - see c_param_expr_arg ().
 *
 */

int method_arg_rt_expr (MESSAGE_STACK messages, int idx) {

  int i, expr_end, stack_end;
  int lookback, prev_label_idx;
  int n_subs, n_parens;
  char tokenbuf[MAXMSG];
  char tokenbuf2[MAXMSG];
  CVAR *c;


  stack_end = get_stack_top (messages);

  for (i = idx, n_subs = 0, n_parens = 0, expr_end = idx; 
       i >= stack_end; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    if (METHOD_ARG_TERM_MSG_TYPE (messages[i]) && !n_subs && !n_parens) {
      expr_end = i + 1;
      goto e_e;
    }
    switch (messages[i] -> tokentype)
      {
      case ARRAYOPEN:
	++n_subs;
	break;
      case ARRAYCLOSE:
	--n_subs;
	break;
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	break;
      }
    if (M_TOK(messages[i]) != LABEL &&
	M_TOK(messages[i]) != METHODMSGLABEL &&
	!n_subs && !n_parens) {
      expr_end = i + 1;
      goto e_e;
    }

  }

 e_e:

  /* This handles little used cases where a parameter needs
     a C translation. */
  if ((lookback = prevlangmsg (messages, idx)) != ERROR) {
    if (M_TOK(messages[lookback]) == ARRAYOPEN) {
      toks2str (messages, idx, expr_end, tokenbuf);
      format_plain_param_subscr_expr (messages, idx, expr_end);
      return SUCCESS;
    } else if (IS_C_OP(M_TOK(messages[lookback]))) {
      toks2str (messages, idx, expr_end, tokenbuf);
      if ((prev_label_idx = scanback (messages, idx + 1,
				      stack_start (messages),
				      LABEL)) != ERROR) {
	if (((c = get_local_var (M_NAME(messages[prev_label_idx])))
	     != NULL) ||
	    ((c = get_global_var (M_NAME(messages[prev_label_idx])))
	     != NULL)) {
	  fmt_eval_expr_str (tokenbuf, tokenbuf2);
	  fmt_rt_return (tokenbuf2,
			 basic_class_from_cvar (messages[i],
						c, c -> n_derefs),
			 FALSE, messages[idx] -> name);
#ifdef SYMBOL_SIZE_CHECK
	  check_symbol_size (messages[idx] -> name);
#endif	    
	  for (i = idx - 1; i >= expr_end; i--) {
	    _MPUTCHAR(messages[i], ' ');
	    messages[i] -> tokentype = RESULT;
	  }
	  return SUCCESS;
	}
      }
    }
  }
  format_plain_param_expr (messages, idx, expr_end);

  return SUCCESS;
}

int method_fn_arg_rt_expr (MESSAGE_STACK messages, int idx) {

  int fn_nth_arg, fn_idx, expr_end, i;
  CFUNC *cfn;
  MESSAGE *m, *m_tok;
  char *expr, expr_buf_out[MAXMSG];
  MSINFO ms;

  if ((fn_nth_arg = obj_expr_is_arg (messages,
				     idx,
				     stack_start (messages),
				     &fn_idx)) >= 0) {
    if ((cfn = get_function (M_NAME(messages[fn_idx]))) != NULL) {
      ms.messages = messages;
      ms.stack_start = stack_start (messages);
      ms.stack_ptr = get_stack_top (messages);
      ms.tok = idx;
      expr = collect_expression (&ms, &expr_end);
      fmt_eval_expr_str (expr, expr_buf_out);
      __xfree (MEMADDR(expr));
      m = messages[idx];
      strcpy (m -> name,
	      fn_param_return_trans
	      (message_stack_at (idx),
	       cfn, expr_buf_out, fn_nth_arg));
      m -> tokentype = RESULT;
      for (i = idx - 1; i >= expr_end; i--) {
	m_tok = message_stack_at (i);
	m_tok -> name[0] = ' '; m_tok -> name[1] = 0;
	m_tok -> tokentype = RESULT;
      }
    } else {
      warning (messages[fn_idx],
	       "Could not find parameter prototype for function, "
	       "\"%s.\"",
	       M_NAME(messages[fn_idx]));
      method_arg_rt_expr (messages, idx);
    }
    return TRUE;
  }
  return FALSE;
}

OBJECT *method_arg_object ( MESSAGE *m) {

  int j;
  METHOD *new_method;
  char buf[MAXMSG];
  OBJECT *arg_expr_object = NULL;
	    
  new_method = new_methods[new_method_ptr + 1] -> method;

  if (!new_method || !IS_METHOD (new_method))
    error (m, "Invalid new method in method_arg_object ().");
	    
  for (j = 0; j < new_method -> n_params; j++) {
    if (!strcmp (m -> name, new_method -> params[j] -> name)) {
      strcatx (buf, "__ctalk_arg_internal (",
	       ascii[new_method -> n_params - j - 1], ")", NULL);
      arg_expr_object = create_object_init (EXPR_CLASSNAME, 
					 EXPR_SUPERCLASSNAME,
					 buf, buf);
      __ctalkSetObjectValue (arg_expr_object, buf);
      arg_expr_object -> scope = ARG_VAR;
    }
  }
  return arg_expr_object;
}

/*
 *  The only method modifier so far is, "super."  
 *
 *  There are two semantics for, "super," covered here.
 *    1. "super" modifies a method, but it is preceded by
 *        a receiver.  
 *    2. "super" represents the receiver.  Then the function
 *       creates an object for super of the method's receiver
 *       superclass.
 */

int compound_method (MESSAGE_STACK messages, int method_idx, int *attrs) {

  int idx_next_tok,
    idx_prev_tok;
  MESSAGE *m_prev,
    *m_next,
    *m_orig;
  OBJECT *rcvr_obj, *var_obj;
  METHOD *method;

  m_orig = messages[method_idx];

  if (m_orig -> attrs & TOK_SUPER) {


    /* Probably should be filled in if we have an example. */
    if (interpreter_pass == expr_check)
      return method_idx;

    if (((idx_next_tok = nextlangmsg (messages, method_idx)) == ERROR) ||
	((idx_prev_tok = prevlangmsg (messages, method_idx)) == ERROR)) {
      __ctalkExceptionInternal (m_orig, parse_error_x, M_NAME(m_orig),0);
      return method_idx;
    }

    m_prev = messages[idx_prev_tok];
    m_next = messages[idx_next_tok];

    if (!m_prev -> obj) {
      /*
       *  Super keyword is acting as a receiver.
       */
      if (interpreter_pass == method_pass) {
	idx_prev_tok = method_idx;
	m_prev = messages[method_idx];
	if (argblk) {
	  m_prev -> obj = 
	    create_object_init 
	    (rcvr_class_obj -> __o_class -> __o_name,
	     rcvr_class_obj -> __o_class -> __o_superclassname,
	     "result", NULLSTR);
	} else {
	  m_prev -> obj = 
	    create_object_init 
	    (rcvr_class_obj -> __o_superclass -> __o_classname,
	     rcvr_class_obj -> __o_superclass -> __o_superclassname,
	     "result", NULLSTR);
	}
      } else {
	if (interpreter_pass == expr_check) {
	  return method_idx;
	} else {

	  return method_idx;
	}
      }
    }
    rcvr_obj = M_VALUE_OBJ (m_prev);

    /* Just return for right now. */
    if (((var_obj = get_instance_variable (M_NAME(m_next), 
					   rcvr_obj -> __o_class -> __o_name, 
					   FALSE))
	!= NULL) ||
	((var_obj = get_class_variable (M_NAME(m_next), 
					rcvr_obj -> __o_class -> __o_name,
					FALSE))
	 != NULL)) {
      return method_idx;
    }

    if (!METHOD_ARG_TERM_MSG_TYPE (m_next)) {
      if ((method = get_super_instance_method (messages, method_idx,
					       rcvr_obj, M_NAME(m_next), TRUE))
	  == NULL) {
	sprintf (errbuf, METHOD_ERR_FMT, M_NAME(m_next), rcvr_obj -> __o_classname);
	__ctalkExceptionInternal (m_next, undefined_method_x, errbuf,0);
	return method_idx;
      }
    }

    if (idx_prev_tok > method_idx) {
      /*
       *  Super is preceded by a receiver object.
       */
      m_orig ->  tokentype = 
	m_next -> tokentype = 
	METHODMSGLABEL;
      m_orig -> receiver_obj = 
	m_next -> receiver_obj =
	rcvr_obj;
      m_orig -> receiver_msg = 
	m_next -> receiver_msg =
	m_prev;

      ++m_orig -> evaled;
      ++m_orig -> output;

    } else {
      /*
       *  Super represents the receiver.
       */
      m_next -> receiver_obj = rcvr_obj;
      m_next -> receiver_msg = m_prev;
    }

    *attrs |= METHOD_SUPER_ATTR;

    return idx_next_tok;
  }

  return method_idx;
}

char *rt_library_method_call (OBJECT *rcvr, METHOD *method, 
			      MESSAGE_STACK messages, int method_idx,
			      char *buf_out) {
  return fmt_method_call (rcvr, method -> selector, method -> name, buf_out);
}

/*
 *  If the method replaces a primitive method in the interpreter
 *  use the primitive method instead.
 *  
 *  Note that method_replaces_primitive () must also be called in 
 *  a primitive that looks up the method; e.g., in new_object ().
 */

METHOD *method_replaces_primitive (METHOD *method, OBJECT *class_object) {

  OBJECT *superclass_object;
  METHOD *super_method;

  if (method -> cfunc)
    return method;

  for (superclass_object = class_object -> __o_superclass,
	 super_method = method;
       superclass_object && *superclass_object -> __o_superclassname;
       superclass_object = superclass_object -> __o_superclass) {
    if (((super_method = get_instance_method (NULL, superclass_object, 
					      method -> name, 
					      ERROR, FALSE)) != NULL) &&
	super_method -> cfunc) {
      return super_method;
    }
  }
    
  if (class_object -> __o_superclass) {
    if (!strcmp (class_object -> __o_superclass -> __o_name, 
		 OBJECT_CLASSNAME)){
      if (((super_method = get_instance_method (NULL, superclass_object, 
					      method -> name, 
						ERROR, FALSE)) != NULL) &&
	  super_method -> cfunc) {
	return super_method;
      }
    }
  }

  return method;
}

OBJECT *new_method_parameter_class_object (MESSAGE_STACK messages, int idx) {
  METHOD *n_method;
  int n_th_param;
  OBJECT *param_class_object;

  n_method = new_methods[new_method_ptr + 1] -> method;

  for (n_th_param = 0; n_th_param < n_method -> n_params; n_th_param++) {
    if (!strcmp (M_NAME(messages[idx]), n_method -> params[n_th_param]->name)){
      if ((param_class_object = 
	   get_class_object(n_method -> params[n_th_param] -> class)) 
	  != NULL) {
	return param_class_object;
      } else {
	return NULL;
      }
    }
  }
  return NULL;
}

int is_method_parameter (MESSAGE_STACK messages, int idx) {

  METHOD *n_method;
  int n_th_param;

  if (!messages[idx])
    return FALSE;

  if (new_method_ptr >= MAXARGS)
    return FALSE;

  if ((n_method = new_methods[new_method_ptr + 1] -> method) != NULL) {
    for (n_th_param = 0; n_th_param < n_method -> n_params; n_th_param++) {
      /* This can be called while parsing method parameters... 
	 if we're still parsing a new method. */
      if (n_method -> params[n_th_param] != NULL) {
	if (str_eq (M_NAME(messages[idx]), 
		    n_method -> params[n_th_param]->name))
	  return TRUE;
      }
    }
  }

  return FALSE;
}

bool is_method_parameter_s (char *paramname) {

  METHOD *n_method;
  int n_th_param;

  if (new_method_ptr >= MAXARGS || interpreter_pass != method_pass)
    return false;

  if ((n_method = new_methods[new_method_ptr + 1] -> method) != NULL) {
    for (n_th_param = 0; n_th_param < n_method -> n_params; n_th_param++) {
      if (str_eq (paramname, n_method -> params[n_th_param]->name))
	return true;
    }
  }

  return false;
}

/*
 *  Save miscellaneous method objects so they can be found
 *  quickly later.
 */
void save_method_object (OBJECT *o) {

  OBJECT *p_obj;
  PARSER *p;

  if (!IS_OBJECT(o))
    return;

  p = CURRENT_PARSER;

  if (p -> methodobjects == NULL) {
    p -> methodobjects = o;
    return;
  }

  for ( p_obj = p -> methodobjects ;p_obj && p_obj -> next; p_obj = p_obj -> next)
    ;
  p_obj -> next = o;
  o -> prev = p_obj;
  o -> next = NULL;
}

int saved_method_object (OBJECT *o) {

  OBJECT *p;

  for (p = CURRENT_PARSER -> methodobjects; p; p = p -> next)
    if (p == o)
      return TRUE;

  return FALSE;
}

bool method_has_param_pfo_def (METHOD *method) {
  int i;
  for (i = 0; i < method -> n_params; ++i) {
    if (IS_PARAM(method -> params[i])) {
      if (method -> params[i] -> attrs & PARAM_PFO) {
	return true;
      }
    }
  }
  return false;
}

/* Is the label a parameter of the new method that we're
   parsing. */
bool is_new_method_parameter (char *param_name)  {
  int i;
  METHOD *method;
  if (interpreter_pass != method_pass && interpreter_pass != expr_check)
    return false;

  if (new_method_ptr < MAXARGS) {
    if ((method = new_methods[new_method_ptr+1] -> method) != NULL) {

      for (i = 0; i < method -> n_params; ++i) {
	if (str_eq (method -> params[i] -> name, param_name)) {
	  return true;
	}
      }
    }
  }
  return false;
}


bool param_label_shadows_method (char *param_class_name, char *label) {
  OBJECT *class;
  METHOD *m;
  if ((class = class_object_search (param_class_name, FALSE)) != NULL) {

    for (m = class -> instance_methods; m; m = m -> next) {
      if (match_method (m, label, ANY_ARGS)) {
	return true;
      }
      if (!m -> next)
      break;
    }
  
    if (class->__o_superclass) {
      if ((m =
	   get_instance_method (NULL,
				class->__o_superclass, label, 
				ANY_ARGS, FALSE)) != NULL) {
	return true;
      }
      if (is_method_proto (class, label) &&
	  !method_proto_is_output (label) &&
	  !method_from_proto &&
	  !super_method_lookup && 
	  (strcmp (label, new_methods[new_method_ptr+1]->method->name))) {
	
	for (m = class -> instance_methods; m; m = m -> next) {
	  if (match_method (m, label, ANY_ARGS)) {
	    return true;
	  }
	}
      }
    }
  }
  return false;
}

  
