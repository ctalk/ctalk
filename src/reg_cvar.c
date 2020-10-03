/* $Id: reg_cvar.c,v 1.2 2020/10/03 02:14:08 rkiesling Exp $ */

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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"

extern bool argblk;          /* Declared in argblk.c.                    */

extern bool ctrlblk_pred;    /* From control.c. */
extern CTRLBLK *ctrlblks[MAXARGS + 1];
extern int ctrlblk_ptr;

extern SUBSCRIPT subscripts[MAXARGS];       /* Declared in subscr.c. */
extern int subscript_ptr;

extern int subscript_object_expr;

extern MESSAGE *m_messages[];    /* Declared in method.c */
extern int m_message_ptr;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern int parser_output_ptr;
extern int last_fileout_stmt;

extern I_PASS interpreter_pass;

static CVAR *__arg_is_struct_member (CVAR *struct_defn, MESSAGE_STACK messages,
				 int idx) {
  CVAR *mbr, *r_mbr;
  for (mbr = struct_defn -> members; mbr; mbr = mbr -> next) {
    if (mbr -> members) {
      if ((r_mbr = __arg_is_struct_member (mbr, messages, idx)) != NULL)
	return r_mbr;
    }
    if (!strcmp (mbr -> name, M_NAME(messages[idx])))
      return mbr;
  }
  return NULL;
}

/* For an example, see the local struct declaration in cvarrcvr22.c */
static CVAR *local_struct_from_ptr_tag (CVAR *v_tag) {
  PARSER *p;
  CVAR *c;
  if (v_tag -> attrs & CVAR_ATTR_STRUCT_PTR_TAG) {
    p = pop_parser ();
    push_parser (p);
    for (c = p -> cvars; c; c = c -> next) {
      if (str_eq (v_tag -> type, c -> type) &&
	  (c -> attrs & CVAR_ATTR_STRUCT_DECL)) {
	return c;
      }
    }
  }
  return NULL;
}

/*
 *   Register a C variable for the run time lib, and make sure 
 *   that the appropriate class library gets loaded.
 *
 *   If the variable is a member of a struct or array, then
 *   collect the tokens to get the name of the variable member.
 *
 *   Constant expressions get the same check in eval_arg ().
 *
 *   Register_c_var_method and register_c_var_receiver are here in 
 *   case the are needed by subscript_expr () in subscr.c, in the
 *   cases where a subscript contains an object and needs to be 
 *   evaluated.  In that case, the template that evaluates the 
 *   subscript is output here, and sets subscript_object_expr to
 *   TRUE, outputs the formatted template, and returns so that a 
 *   normal register_c_var call isn't output further down in the 
 *   function, and a __ctalk_arg () call isn't output after returning 
 *   to method_args ().
 */

METHOD *register_c_var_method;
OBJECT *register_c_var_receiver;

/*
 *  If the CVAR is a derived type, try to find the basic
 *  type and return a type_attr value.  Otherwise, return 0.
 */
static int derived_type_fixup (CVAR *v, CVAR *v_derived) {
  CVAR *c_basic_type;
  if (!IS_CVAR(v) || !IS_CVAR(v_derived))
    return 0;
  
  if ((v_derived -> attrs & CVAR_ATTR_TYPEDEF) &&
      (v_derived -> attrs & CVAR_ATTR_STRUCT_DECL)) {
    return CVAR_TYPE_STRUCT;
  }

  if ((c_basic_type = basic_type_of (v_derived)) != NULL) {
    return c_basic_type -> type_attrs;
  }

  return 0;
}

/* Map a local stack index (like an index for on m_messages or
   tmpl_messages) to the token on the main stack so we can set
   attributes on the main stack when we're done with retokenizing. */
static void tag_main_stack_token_output (char *basename,
					 char *varname, 
					 int idx_start,
					 int idx_end) {
  PARSER *p;
  FRAME *f, *f_next;
  MESSAGE *m;
  int i, j, n_toks;

  p = parsers[current_parser_ptr];
  f = frame_at (p -> frame);
  f_next = frame_at (p -> frame - 1);
  n_toks = idx_start - idx_end;

  for (i = f -> message_frame_top; i > f_next -> message_frame_top; --i) {
    m = message_stack_at (i);
    if (str_eq (M_NAME(m), basename)) {
      m -> attrs |= TOK_CVAR_REGISTRY_IS_OUTPUT;
      for (j = i - 1; j > (i - n_toks); --j) {
	m = message_stack_at (j);
	m -> attrs |= TOK_CVAR_REGISTRY_IS_OUTPUT;
      }
      return;
    }
  }

}

static char *collect_cvar_expr (MESSAGE_STACK messages, int start_tok_idx,
				int *var_expr_end_idx, int *struct_deref_end,
				int *expr_end, char *expr_buf_out,
				AGGREGATE_EXPR_TYPE *aet) {
  int stack_top, i, n_derefs, n_labels, n_blocks, n_parens,
    prev_tok = 0;
  stack_top = get_stack_top (messages);
  MESSAGE *m;
  char ref_buf_out[MAXMSG];

  *aet = aggregate_expr_type_null;

  for (i = start_tok_idx, *expr_buf_out = 0, n_derefs = 0, 
	 n_labels = 0, n_blocks = 0, n_parens = 0, *struct_deref_end = -1;
       i > stack_top; i--) {
    m = messages[i];
    switch (m -> tokentype)
      {
      case SEMICOLON:
      case ARGSEPARATOR:
	goto done;
	break;
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	if (!n_parens)
	  goto done;
	--n_parens;
	break;
      case ARRAYOPEN:
	/*
	 *  It should be okay to have whitespace here.
	 */
	if (!n_blocks) {

	  /* subscript_begin/end are now redundant, and can
	     probably be removed. */
	  subscripts[subscript_ptr].block_start_idx = i;
	  subscripts[subscript_ptr].start_idx = nextlangmsg (messages, i);
	}
	++n_blocks;
	*aet = aggregate_expr_type_array;
	break;
	case ARRAYCLOSE:
	  if (n_blocks == 0)
	    goto done;
	  if (--n_blocks == 0) {
	    int _subscr_lookahead;
	    subscripts[subscript_ptr].block_end_idx = i;
	    subscripts[subscript_ptr++].end_idx = prevlangmsg (messages, i);
	    /*
	     *  Look ahead for another subscript; e.g., a[]<[>],
	     *  or a struct operator; e.g. a[n]<.>mbr, a[n]< -> > mbr.
	     */
	    if ((_subscr_lookahead = nextlangmsg (messages, i)) == ERROR) {
	      strcatx2 (expr_buf_out, messages[i] -> name, NULL);
	      *var_expr_end_idx = i;
	      goto done;
	    }
	    if ((M_TOK(messages[_subscr_lookahead]) != ARRAYOPEN) &&
		(M_TOK(messages[_subscr_lookahead]) != PERIOD) &&
		(M_TOK(messages[_subscr_lookahead]) != DEREF)) {
	      strcatx2 (expr_buf_out, messages[i] -> name, NULL);
	      *var_expr_end_idx = i;
	      goto done;
	    }
	  }
	  break;
      case LABEL:
	/*
	 *  Check for the case where a method label follows a 
	 *  variable label.  That is, we already have a label, 
	 *  or a sequence of dereferenced labels, and another 
	 *  label follows it.  The loop should not need to 
	 *  check for math operators.
	 */

	if (!n_blocks) /* Not labels within subscripts, though. */
	  ++n_labels;
	if ((prev_tok == DEREF) || (prev_tok == PERIOD))
	  *struct_deref_end = i;
	if ((n_labels > (n_derefs + 1)) && !n_blocks && !n_parens)
	  goto done;
	/*
	 *  If within an array subscript, check whether the label refers
	 *  to an object, and record the subscript expression.
	 *
	 *  If register_c_var_method is NULL, the function is 
	 *  being called while parsing a run-time expression,
	 *  and we won't need a separate block later (from 
	 *  subscr_expr ()) to extract an array element as a method 
	 *  argument.
	 */
	if (n_blocks && register_c_var_method) {
	  subscripts[subscript_ptr].obj =
	    resolve_arg (register_c_var_method, messages, i, NULL);
	} else {
	  if (n_blocks) {
	    if (subscripts[subscript_ptr].start_idx > 0)
	      if (get_object 
		  (M_NAME(messages[subscripts[subscript_ptr].start_idx]),
		   NULL)) {
		error (messages[subscripts[subscript_ptr].start_idx],
		       "Object, \"%s,\" in a C array subscript is not (yet) "
		       "supported in this context.",
			 M_NAME(messages[subscripts[subscript_ptr].start_idx]));
	      }
	  }
	}
	break;
      case DEREF:
      case PERIOD:
	++n_derefs;
	*aet = aggregate_expr_type_struct;
	break;
      case WHITESPACE:
	break;
      case AMPERSAND:
	break;
      default:
	if (!n_parens && !n_blocks)
	  goto done;
	break;
      }
    if (M_TOK(messages[i]) == LABEL) {
      if (have_ref (messages[i] -> name)) {
	strcatx2 (expr_buf_out,
		  fmt_getRef (messages[i] -> name, ref_buf_out), NULL);
      } else {
	strcatx2 (expr_buf_out, messages[i] -> name, NULL);
      }
      *expr_end = i;
    } else {
      if (!M_ISSPACE(m)) {
	/* don't include trailing space */
	strcatx2 (expr_buf_out, messages[i] -> name, NULL);
	*expr_end = i;
      }
    }
    if (!M_ISSPACE(m))
      prev_tok = m -> tokentype;
  }
 done:
  /*
   *  By the time we get here, points to the end of the expression
   *  if it's the end of a subscript.  If the end of the expression
   *  is a struct member, then struct_deref_end is correct.  Might
   *  need cleaning up.
   */
  if (*struct_deref_end != -1)
    *expr_end = *struct_deref_end;
  return expr_buf_out;
}

int basename_idx;

/* 
 *  When making changes here, also make the same changes to
 *  register_c_var_buf, below.
 */
int register_c_var (MESSAGE *m_err, MESSAGE_STACK messages, int idx,
		    int *var_end_idx) {

  CVAR *v = NULL;
  CVAR *v_derived = NULL;
  int i,
    struct_deref_end,
    prev_tok,
    next_tok,
    expr_end = -1;
  int old_parser_output_ptr,
    old_last_fileout_stmt;
  char *basename, varname[MAXLABEL], *subscriptbuf;
  MSINFO ms;
  AGGREGATE_EXPR_TYPE varname_aet;

  if (interpreter_pass == expr_check)
    return SUCCESS;
  /*
   *  If in an argument block, the CVAR registration should be handled
   *  by fmt_register_argblk_c_vars_* () (complexmethd.c).
   *  
   *  Unliess it's a global C variable and we're writing a
   *  complete argument expression for evaluation at run time -
   *  see eval_arg.c.
   */
  if (argblk) {
    ms.messages = messages;
    ms.tok = idx;
    ms.stack_ptr = get_stack_top (messages);
    ms.stack_start = stack_start (messages);
    *var_end_idx = find_expression_limit (&ms);
    return SUCCESS;
  }

  /* we only reset these just before we need to save new
     subscript data */
  for (i = subscript_ptr; i >= 0; i--) {
    subscripts[i].obj = NULL;
    subscripts[i].start_idx = subscripts[i].end_idx =
      subscripts[i].block_start_idx = subscripts[i].block_end_idx = 0;
  }
  subscript_ptr = 0;

  if (IS_C_UNARY_MATH_OP(M_TOK(messages[idx]))) {
    int _basename_lookahead;
    _basename_lookahead = nextlangmsg (messages, idx);
    basename = M_NAME(messages[_basename_lookahead]);
  } else {
    basename = M_NAME(messages[idx]);
  }

  prev_tok = 0;

  collect_cvar_expr (messages, idx, var_end_idx, &struct_deref_end,
		     &expr_end, varname, &varname_aet);
  if (((v = get_local_var (basename)) != NULL) ||
      ((v = get_global_var_not_shadowed (basename)) != NULL)) {
    if (!validate_type_declarations (v))
      return ERROR;

    /*
     *  Struct or union members.
     */
    if (IS_STRUCT_OR_UNION(v)) {
      /*
       *  If the cvar is a struct pointer, look for the
       *  struct definition.
       */
      if (v -> n_derefs && !v -> members) {
	CVAR *struct_defn;
	CVAR *mbr;
	if ((struct_defn = get_struct_defn (v -> type)) == NULL) {
	  __ctalkExceptionInternal (messages[idx],
				    undefined_type_x, v -> type,0);
	  return ERROR;
	}

	/*
	 *  Find the struct member.
	 */
	if (varname_aet == aggregate_expr_type_struct) {
	  for (mbr = struct_defn -> members; mbr; mbr = mbr -> next) {
	    if (strstr (varname, mbr -> name))
	      cvar_register_output (messages, 
				    idx, expr_end,
				    subscripts[subscript_ptr - 1].start_idx,
				    subscripts[subscript_ptr - 1].end_idx,
				    register_c_var_receiver,
				    register_c_var_method,
				    mbr, basename,
				    varname, subscripts[subscript_ptr-1].obj);
	  }
	} else {
	  /*
	   *  The complete struct.
	   */
	  cvar_register_output (messages, 
				idx, expr_end,
				subscripts[subscript_ptr - 1].start_idx,
				subscripts[subscript_ptr - 1].end_idx,
				register_c_var_receiver,
				register_c_var_method,
				v, basename,
				varname, subscripts[subscript_ptr-1].obj);

	}
      } else {
	/*
	 *  The cvar is a struct declaration, but not the definition.
	 */
	if (v -> members == NULL) {
	  CVAR *struct_defn;
	  CVAR *mbr;
	  if ((struct_defn = get_struct_defn (v -> type)) == NULL) {
	    __ctalkExceptionInternal (messages[idx],
				      undefined_type_x, v -> type,0);
	    return ERROR;
	  }

	  /*
	   *  Struct member.
	   */
	  if (strstr (v -> name, ".") ||
	      strstr (v -> name, "->")) {
	    for (mbr = struct_defn -> members; mbr; mbr = mbr -> next) {
	      if (strstr (varname, mbr -> name))
		cvar_register_output (messages, 
				      idx, expr_end,
				      subscripts[subscript_ptr - 1].start_idx,
				      subscripts[subscript_ptr - 1].end_idx,
				      register_c_var_receiver,
				      register_c_var_method,
				      mbr, basename,
				      varname, subscripts[subscript_ptr-1].obj);
	    }
	  } else {
	    if (v -> attrs & CVAR_ATTR_STRUCT) {
	      /*
	       *  Variable derived from a declaration independent of 
	       *  the definition.
	       *  The function found struct_defn above.
	       */
	      if (varname_aet == aggregate_expr_type_struct) {
		for (mbr = struct_defn -> members; mbr; mbr = mbr -> next) {
		  if (strstr (varname, mbr -> name))
		    cvar_register_output (messages, 
					  idx,
					  expr_end,
					  subscripts[subscript_ptr - 1].start_idx,
					  subscripts[subscript_ptr - 1].end_idx,
					  register_c_var_receiver,
					  register_c_var_method,
					  mbr, basename,
					  varname,
					  subscripts[subscript_ptr-1].obj);
		}
	      }
	    } else {
	      /*
	       *  Complete struct.
	       */
	      cvar_register_output (messages, idx,
				    expr_end,
				    subscripts[subscript_ptr - 1].start_idx,
				    subscripts[subscript_ptr - 1].end_idx,
				    register_c_var_receiver,
				    register_c_var_method,
				    v, basename, 
				    varname, subscripts[subscript_ptr-1].obj);
	    }
	  }
	} else {
	  register_struct_terminal (messages, m_err, 
				    idx, expr_end, 
				    subscripts[subscript_ptr - 1].start_idx,
				    subscripts[subscript_ptr - 1].end_idx,
				    struct_deref_end, v,
				    basename, varname,
				    subscripts[subscript_ptr-1].obj,
				    register_c_var_receiver,
				    register_c_var_method);
	}
      }
    } else {
      /*
       *  Derived struct type.
       *
       *  TODO - Make sure that typedef struct definitions
       *  also have the CVAR_ATTR_STRUCT attribute set.
       */
      if ((((v_derived = 
	     get_typedef (v -> type)) != NULL) ||
	   ((v_derived = 
	     get_typedef (v -> qualifier)) != NULL) ||
	   /* this is a CVAR_ATTR_STRUCT_PTR_TAG - see cvarrcvr22.c */
	   ((v_derived = /***/
	     local_struct_from_ptr_tag (v)) != NULL)) &&
	  v_derived -> members &&
	  struct_deref_end != ERROR) {
	CVAR *derived_mbr;
	if ((derived_mbr = 
	     __arg_is_struct_member (v_derived, messages, struct_deref_end))
	    != NULL) {
	  cvar_register_output (messages, 
				idx, expr_end,
				subscripts[subscript_ptr - 1].start_idx,
				subscripts[subscript_ptr - 1].end_idx,
				register_c_var_receiver,
				register_c_var_method,
				derived_mbr, 
				basename,
				varname, subscripts[subscript_ptr-1].obj);
	} else {
	  warning (m_err, "Could not find struct member %s.\n",
		   M_NAME(messages[struct_deref_end]));
	  cvar_register_output (messages, 
				idx, expr_end,
				subscripts[subscript_ptr - 1].start_idx,
				subscripts[subscript_ptr - 1].end_idx,
				register_c_var_receiver,
				register_c_var_method,
				v_derived, 
				basename,
				varname, subscripts[subscript_ptr-1].obj);
	}
	return SUCCESS;
      } else {
	/*
	 *  If varname refers to an array member, create a new 
	 *  cvar for the member.  If the subscript is an object
	 *  expression, generate a wrapper for it and paste
	 *  into the varname buffer.  It would be much better
	 *  to add another state somewhere so the interpreter can 
	 *  treat arguments subscripts like in-line subscripts, but 
	 *  it should work.
	 */

	if (IS_OBJECT(subscripts[subscript_ptr - 1].obj)) {
	  subscript_object_expr = TRUE;
	  fileout (subscript_expr (m_messages, 
				   subscripts[subscript_ptr - 1].start_idx,
				   subscripts[subscript_ptr - 1].end_idx,
				   register_c_var_receiver,
				   register_c_var_method, basename, v),
		   0, FRAME_START_IDX);

	  for (i = idx; i >= expr_end; i--) {
	    ++messages[i] -> evaled;
	    ++messages[i] -> output;
	  }

	  return SUCCESS;
	} else {
	  subscriptbuf = varname;
	}
      }

      if ((v -> n_derefs > 0) &&
	  (varname_aet == aggregate_expr_type_array)) {
	CVAR *v_mbr;
	_new_cvar (CVARREF(v_mbr));
	strcpy (v_mbr -> name, varname);
	if (*v -> decl) strcpy (v_mbr -> decl, v -> decl);
	if (*v -> type) strcpy (v_mbr -> type, v -> type);
	if (*v -> qualifier) strcpy (v_mbr -> qualifier, v -> qualifier);
	if (*v -> qualifier2) strcpy (v_mbr -> qualifier2, v -> qualifier2);
	if (*v -> storage_class) 
	  strcpy (v_mbr -> storage_class, v -> storage_class);
	/*
	 *  For array elements, use one less level of dereferencing.
	 */
	if (v -> n_derefs) v_mbr -> n_derefs = v -> n_derefs - 1;
	v_mbr -> is_unsigned = v -> is_unsigned;
	v_mbr -> scope = v -> scope;
	v_mbr -> type_attrs = v -> type_attrs;

	if (!subscript_object_expr) /* Complete template output above. */
	  /* But still use a NULL subscript_obj as the argument. */
	  cvar_register_output (messages, 
				idx, expr_end,
				subscripts[subscript_ptr - 1].start_idx,
				subscripts[subscript_ptr - 1].end_idx,
				register_c_var_receiver,
				register_c_var_method,
				v_mbr, basename,
				subscriptbuf,
				subscripts[subscript_ptr-1].obj);
	__xfree (MEMADDR(v_mbr));
      } else {
	prev_tok = prevlangmsg (messages, idx);
	next_tok = nextlangmsg (messages, idx);
	  /*
	   *  These are the only prefix ops that we need to construct
	   *  an expression for, because they write a modified value
	   *  to the CVAR.  Also sets arg_CVAR_has_unary_inc_or_dec 
	   *  declared in prefixop.c, to the number of the argument.
	   */
	if ((prev_tok != ERROR) && 
	    ((M_TOK(messages[prev_tok]) == INCREMENT) ||
	     (M_TOK(messages[prev_tok]) == DECREMENT))) {
	  prefix_arg_cvar_expr_registration 
	    (messages, idx, v, M_TOK(messages[prev_tok]), prev_tok);
	  goto registered_prefix_or_postfix_op;
	}
	if ((next_tok != ERROR) && 
	    ((M_TOK(messages[next_tok]) == INCREMENT) ||
	     (M_TOK(messages[next_tok]) == DECREMENT))) {
	  postfix_arg_cvar_expr_registration 
	    (messages, idx, v, M_TOK(messages[next_tok]), next_tok);
	  goto registered_prefix_or_postfix_op;
	}
	if (v -> type_attrs == 0) {
	  v -> type_attrs = derived_type_fixup (v, v_derived);
	} else if (v -> type_attrs == CVAR_TYPE_TYPEDEF) {
	  v -> type_attrs |= derived_type_fixup (v, v_derived);
	}
	if (ctrlblk_pred) {
	  /* While loops and do-while loops need the CVAR registration
	     inside the while () construct, per fmt_default_ctrlblk_expr */
	  cvar_register_output (messages, 
				idx, expr_end,
				subscripts[subscript_ptr - 1].start_idx,
				subscripts[subscript_ptr - 1].end_idx,
				register_c_var_receiver,
				register_c_var_method,
				v, 
				basename, varname,
				subscripts[subscript_ptr-1].obj);
	  tag_main_stack_token_output (basename, varname, idx, expr_end);
	  /* Checked at least by create_arg_EXPR_object, so we don'
	     add a _getRef statement in an expression when the
	     CVAR (with a _getRef call) is registered in the 
	     statment _immediately before_ the expression. */
	  messages[idx] -> attrs |= TOK_CVAR_REGISTRY_IS_OUTPUT;
	} else {
	  cvar_register_output (messages, 
				idx, expr_end,
				subscripts[subscript_ptr - 1].start_idx,
				subscripts[subscript_ptr - 1].end_idx,
				register_c_var_receiver,
				register_c_var_method,
				v, 
				basename, varname,
				subscripts[subscript_ptr-1].obj);
	  tag_main_stack_token_output (basename, varname, idx, expr_end);
	  messages[idx] -> attrs |= TOK_CVAR_REGISTRY_IS_OUTPUT;
	}
      }
    }
  registered_prefix_or_postfix_op:
    /*
     *  If we load, then parse and save with the output the libraries
     *  that correspond to C types here, we don't need to load and
     *  parse them at run time.
     */

    /*
     *  If the class search and parsing ever recurses, the function
     *  might need something better than saving and restoring the 
     *  output pointers.
     */
    old_parser_output_ptr = parser_output_ptr;
    old_last_fileout_stmt = last_fileout_stmt;


    if (v -> type_attrs & CVAR_TYPE_INT) {
      if ((v -> type_attrs & CVAR_TYPE_LONG) &&
	  (v -> type_attrs & CVAR_TYPE_LONGLONG)) {
	class_object_search ("LongInteger", FALSE);
      } else {
	class_object_search ("Integer", FALSE);
      }
    }
    if (v -> type_attrs & CVAR_TYPE_DOUBLE)
      class_object_search ("Float", FALSE);

    parser_output_ptr = old_parser_output_ptr;
    last_fileout_stmt = old_last_fileout_stmt;

    return SUCCESS;
  }
  return ERROR;
}

char *register_c_var_buf (MESSAGE *m_err, MESSAGE_STACK messages, int idx,
			  int *var_end_idx, char *buf_out) {

  CVAR *v = NULL;
  CVAR *v_derived = NULL;
  int i,
    struct_deref_end,
    prev_tok,
    next_tok,
    expr_end = -1;
  int old_parser_output_ptr,
    old_last_fileout_stmt;
  char basename[MAXLABEL], 
    varname[MAXLABEL],
    subscriptbuf[MAXMSG];
  AGGREGATE_EXPR_TYPE varname_aet;

  if (interpreter_pass == expr_check)
    return NULL;
  /*
   *  If in an argument block, the CVAR registration should be handled
   *  by fmt_register_argblk_c_vars_* () (complexmethd.c).
   */
  if (argblk)
    return NULL;

  /* we only reset these just before we need to save new
     subscript data */
  for (i = subscript_ptr; i >= 0; i--) {
    subscripts[i].obj = NULL;
    subscripts[i].start_idx = subscripts[i].end_idx =
      subscripts[i].block_start_idx = subscripts[i].block_end_idx = 0;
  }
  subscript_ptr = 0;

  if (IS_C_UNARY_MATH_OP(M_TOK(messages[idx]))) {
    int _basename_lookahead;
    _basename_lookahead = nextlangmsg (messages, idx);
    strcpy (basename, M_NAME(messages[_basename_lookahead]));
  } else {
    strcpy (basename, M_NAME(messages[idx]));
  }

  prev_tok = 0;

  collect_cvar_expr (messages, idx, var_end_idx, &struct_deref_end,
		     &expr_end, varname, &varname_aet);
  *var_end_idx = expr_end;

  if (((v = get_local_var (basename)) != NULL) ||
      ((v = get_global_var_not_shadowed (basename)) != NULL)) {
    if (!validate_type_declarations (v))
      return NULL;

    /*
     *  Struct or union members.
     */
    if (v -> type_attrs & CVAR_TYPE_STRUCT ||
	v -> type_attrs & CVAR_TYPE_UNION ||
	v -> attrs & CVAR_ATTR_STRUCT) {
      /*
       *  If the cvar is a struct pointer, look for the
       *  struct definition.
       */
      if (v -> n_derefs && !v -> members) {
	CVAR *struct_defn;
	CVAR *mbr;
	if ((struct_defn = get_struct_defn (v -> type)) == NULL) {
	  __ctalkExceptionInternal (messages[idx],
				    undefined_type_x, v -> type,0);
	  return NULL;
	}

	/*
	 *  Find the struct member.
	 */
	if (varname_aet == aggregate_expr_type_struct) {
	  for (mbr = struct_defn -> members; mbr; mbr = mbr -> next) {
	    if (strstr (varname, mbr -> name))
	      cvar_register_output_buf
		(messages, 
		 idx, expr_end,
		 subscripts[subscript_ptr - 1].start_idx,
		 subscripts[subscript_ptr - 1].end_idx,
		 register_c_var_receiver,
		 register_c_var_method,
		 mbr, basename,
		 varname, subscripts[subscript_ptr-1].obj,
		 buf_out);
	  }
	} else {
	  /*
	   *  The complete struct.
	   */
	  cvar_register_output_buf
	    (messages, 
	     idx, expr_end,
	     subscripts[subscript_ptr - 1].start_idx,
	     subscripts[subscript_ptr - 1].end_idx,
	     register_c_var_receiver,
	     register_c_var_method,
	     v, basename,
	     varname, subscripts[subscript_ptr-1].obj,
	     buf_out);

	}
      } else {
	/*
	 *  The cvar is a struct declaration, but not the definition.
	 */
	if (v -> members == NULL) {
	  CVAR *struct_defn;
	  CVAR *mbr;
	  if ((struct_defn = get_struct_defn (v -> type)) == NULL) {
	    __ctalkExceptionInternal (messages[idx],
				      undefined_type_x, v -> type,0);
	    return NULL;
	  }

	  /*
	   *  Struct member.
	   */
	  if (strstr (v -> name, ".") ||
	      strstr (v -> name, "->")) {
	    for (mbr = struct_defn -> members; mbr; mbr = mbr -> next) {
	      if (strstr (varname, mbr -> name))
		cvar_register_output_buf
		  (messages, 
		   idx, expr_end,
		   subscripts[subscript_ptr - 1].start_idx,
		   subscripts[subscript_ptr - 1].end_idx,
		   register_c_var_receiver,
		   register_c_var_method,
		   mbr, basename,
		   varname, subscripts[subscript_ptr-1].obj,
		   buf_out);
	    }
	  } else {
	    if (v -> attrs & CVAR_ATTR_STRUCT) {
	      /*
	       *  Variable derived from a declaration independent of 
	       *  the definition.
	       *  The function found struct_defn above.
	       */
	      if (varname_aet == aggregate_expr_type_struct) {
		for (mbr = struct_defn -> members; mbr; mbr = mbr -> next) {
		  if (strstr (varname, mbr -> name))
		    cvar_register_output_buf
		      (messages, 
		       idx,
		       expr_end,
		       subscripts[subscript_ptr - 1].start_idx,
		       subscripts[subscript_ptr - 1].end_idx,
		       register_c_var_receiver,
		       register_c_var_method,
		       mbr, basename,
		       varname,
		       subscripts[subscript_ptr-1].obj,
		       buf_out);
		}
	      }
	    } else {
	      /*
	       *  Complete struct.
	       */
	      cvar_register_output_buf
		(messages, idx,
		 expr_end,
		 subscripts[subscript_ptr - 1].start_idx,
		 subscripts[subscript_ptr - 1].end_idx,
		 register_c_var_receiver,
		 register_c_var_method,
		 v, basename, 
		 varname, subscripts[subscript_ptr-1].obj,
		 buf_out);
	    }
	  }
	} else {
	  register_struct_terminal_buf
	    (messages, m_err, 
	     idx, expr_end, 
	     subscripts[subscript_ptr - 1].start_idx,
	     subscripts[subscript_ptr - 1].end_idx,
	     struct_deref_end, v,
	     basename, varname,
	     subscripts[subscript_ptr-1].obj,
	     register_c_var_receiver,
	     register_c_var_method, buf_out);
	}
      }
    } else {
      /*
       *  Derived struct type.
       *
       *  TODO - Make sure that typedef struct definitions
       *  also have the CVAR_ATTR_STRUCT attribute set.
       */
      if ((((v_derived = 
	    get_typedef (v -> type)) != NULL) ||
	  ((v_derived = 
	    get_typedef (v -> qualifier)) != NULL)) &&
	  v_derived -> members &&
	  struct_deref_end != ERROR) {
	CVAR *derived_mbr;
	if ((derived_mbr = 
	     __arg_is_struct_member (v_derived, messages, struct_deref_end))
	    != NULL) {
	  cvar_register_output_buf
	    (messages, 
	     idx, expr_end,
	     subscripts[subscript_ptr - 1].start_idx,
	     subscripts[subscript_ptr - 1].end_idx,
	     register_c_var_receiver,
	     register_c_var_method,
	     derived_mbr, 
	     basename,
	     varname, subscripts[subscript_ptr-1].obj,
	     buf_out);
	} else {
	  warning (m_err, "Could not find struct member %s.\n",
		   M_NAME(messages[struct_deref_end]));
	  cvar_register_output_buf
	    (messages, 
	     idx, expr_end,
	     subscripts[subscript_ptr - 1].start_idx,
	     subscripts[subscript_ptr - 1].end_idx,
	     register_c_var_receiver,
	     register_c_var_method,
	     v_derived, 
	     basename,
	     varname, subscripts[subscript_ptr-1].obj,
	     buf_out);
	}
	return buf_out;
      } else {
	/*
	 *  If varname refers to an array member, create a new 
	 *  cvar for the member.  If the subscript is an object
	 *  expression, generate a wrapper for it and paste
	 *  into the varname buffer.  It would be much better
	 *  to add another state somewhere so the interpreter can 
	 *  treat arguments subscripts like in-line subscripts, but 
	 *  it should work.
	 */

	if (IS_OBJECT(subscripts[subscript_ptr - 1].obj)) {
	  subscript_object_expr = TRUE;
	  strcatx (buf_out, subscript_expr
		   (m_messages,
		    subscripts[subscript_ptr - 1].start_idx,
		    subscripts[subscript_ptr - 1].end_idx,
		    register_c_var_receiver,
		    register_c_var_method,
		    basename, v), NULL);
	  
	  for (i = idx; i >= expr_end; i--) {
	    ++messages[i] -> evaled;
	    ++messages[i] -> output;
	  }

	  return buf_out;
	} else {
	  strcpy (subscriptbuf, varname);
	}
      }

      if ((v -> n_derefs > 0) && (varname_aet == aggregate_expr_type_array)) {
	CVAR *v_mbr;
	_new_cvar (CVARREF(v_mbr));
	strcpy (v_mbr -> name, varname);
	if (*v -> decl) strcpy (v_mbr -> decl, v -> decl);
	if (*v -> type) strcpy (v_mbr -> type, v -> type);
	if (*v -> qualifier) strcpy (v_mbr -> qualifier, v -> qualifier);
	if (*v -> qualifier2) strcpy (v_mbr -> qualifier2, v -> qualifier2);
	if (*v -> storage_class) 
	  strcpy (v_mbr -> storage_class, v -> storage_class);
	/*
	 *  For array elements, use one less level of dereferencing.
	 */
	if (v -> n_derefs) v_mbr -> n_derefs = v -> n_derefs - 1;
	v_mbr -> is_unsigned = v -> is_unsigned;
	v_mbr -> scope = v -> scope;
	v_mbr -> type_attrs = v -> type_attrs;

	if (!subscript_object_expr) /* Complete template output above. */
	  /* But still use a NULL subscript_obj as the argument. */
	  cvar_register_output_buf
	    (messages, 
	     idx, expr_end,
	     subscripts[subscript_ptr - 1].start_idx,
	     subscripts[subscript_ptr - 1].end_idx,
	     register_c_var_receiver,
	     register_c_var_method,
	     v_mbr, basename,
	     subscriptbuf,
	     subscripts[subscript_ptr-1].obj,
	     buf_out);
	__xfree (MEMADDR(v_mbr));
      } else {
	prev_tok = prevlangmsg (messages, idx);
	next_tok = nextlangmsg (messages, idx);
	  /*
	   *  These are the only prefix ops that we need to construct
	   *  an expression for, because they write a modified value
	   *  to the CVAR.  Also sets arg_CVAR_has_unary_inc_or_dec 
	   *  declared in prefixop.c, to the number of the argument.
	   */
	if ((prev_tok != ERROR) && 
	    ((M_TOK(messages[prev_tok]) == INCREMENT) ||
	     (M_TOK(messages[prev_tok]) == DECREMENT))) {
	  prefix_arg_cvar_expr_registration 
	    (messages, idx, v, M_TOK(messages[prev_tok]), prev_tok);
	  goto registered_prefix_or_postfix_op;
	}
	if ((next_tok != ERROR) && 
	    ((M_TOK(messages[next_tok]) == INCREMENT) ||
	     (M_TOK(messages[next_tok]) == DECREMENT))) {
	  postfix_arg_cvar_expr_registration 
	    (messages, idx, v, M_TOK(messages[next_tok]), next_tok);
	  goto registered_prefix_or_postfix_op;
	}
	if (v -> type_attrs == 0) {
	  v -> type_attrs = derived_type_fixup (v, v_derived);
	} else if (v -> type_attrs == CVAR_TYPE_TYPEDEF) {
	  v -> type_attrs |= derived_type_fixup (v, v_derived);
	}
	if (ctrlblk_pred) {
	  /* While loops and do-while loops need the CVAR registration
	     inside the while () construct, per fmt_default_ctrlblk_expr */
	  cvar_register_output_buf
	    (messages, 
	     idx, expr_end,
	     subscripts[subscript_ptr - 1].start_idx,
	     subscripts[subscript_ptr - 1].end_idx,
	     register_c_var_receiver,
	     register_c_var_method,
	     v, 
	     basename, varname,
	     subscripts[subscript_ptr-1].obj,
	     buf_out);
	  tag_main_stack_token_output (basename, varname, idx, expr_end);
	  /* Checked at least by create_arg_EXPR_object, so we don'
	     add a _getRef statement in an expression when the
	     CVAR (with a _getRef call) is registered in the 
	     statment _immediately before_ the expression. */
	  messages[idx] -> attrs |= TOK_CVAR_REGISTRY_IS_OUTPUT;
	} else {
	  cvar_register_output_buf
	    (messages, 
	     idx, expr_end,
	     subscripts[subscript_ptr - 1].start_idx,
	     subscripts[subscript_ptr - 1].end_idx,
	     register_c_var_receiver,
	     register_c_var_method,
	     v, 
	     basename, varname,
	     subscripts[subscript_ptr-1].obj,
	     buf_out);
	  tag_main_stack_token_output (basename, varname, idx, expr_end);
	  /* See  the comment above. */
	  messages[idx] -> attrs |= TOK_CVAR_REGISTRY_IS_OUTPUT;
	}
      }
    }
  registered_prefix_or_postfix_op:
    /*
     *  If we load, then parse and save with the output the libraries
     *  that correspond to C types here, we don't need to load and
     *  parse them at run time.
     */

    /*
     *  If the class search and parsing ever recurses, the function
     *  might need something better than saving and restoring the 
     *  output pointers.
     */
    old_parser_output_ptr = parser_output_ptr;
    old_last_fileout_stmt = last_fileout_stmt;


    if (v -> type_attrs & CVAR_TYPE_INT) {
      if ((v -> type_attrs & CVAR_TYPE_LONG) &&
	  (v -> type_attrs & CVAR_TYPE_LONGLONG)) {
	class_object_search ("LongInteger", FALSE);
      } else {
	class_object_search ("Integer", FALSE);
      }
    }
    if (v -> type_attrs & CVAR_TYPE_DOUBLE)
      class_object_search ("Float", FALSE);

    parser_output_ptr = old_parser_output_ptr;
    last_fileout_stmt = old_last_fileout_stmt;

    return buf_out;
  }
  return NULL;
}
