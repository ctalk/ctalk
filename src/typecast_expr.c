/* $Id: typecast_expr.c,v 1.3 2020/07/09 20:21:43 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016, 2018, 2019 Robert Kiesling, rk3314042@gmail.com.
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
#include "typeof.h"

static int typecast_states[] = {
  #include "../include/typecaststate.h"
  -1, 0, 0
};

#define TYPECAST_STATE_COLS 3

#define A_IDX_(a,b,ncols) (((a)*(ncols))+(b))

static inline int __c_check_state (int stack_idx, MESSAGE_STACK messages, 
				 int *states, 
				 int cols) {
  int i, j;
  int token;
  int next_idx = stack_idx;

  token = messages[stack_idx] -> tokentype;

  for (i = 0; ; i++) {

  nextrans:
     if (states[A_IDX_(i,0,cols)] == ERROR)
       return ERROR;

     if (states[A_IDX_(i,0,cols)] == token) {

       for (j = 1, next_idx = stack_idx - 1; ; j++, next_idx--) {
	
	 while (states[A_IDX_(i,j,cols)]) {
	   if (!messages[next_idx] || !IS_MESSAGE (messages[next_idx]))
	     return ERROR;
           while (M_ISSPACE(messages[next_idx])) {
	     next_idx--;
	     /* TO DO - 
		Find a way to notify the calling function whether 
		the message is the end of the stack, or determine
		the end of the stack by the calling function.
	     */
	     if (!messages[next_idx] || !IS_MESSAGE (messages[next_idx]))
	       return ERROR;
	   }
	   if (states[A_IDX_(i,j,cols)] != messages[next_idx]->tokentype) {
	     ++i;
	     goto nextrans;
	   }
	   else
	     ++j;
	 }
	 return i;
       }
     }
  }
}

#define TYPECAST_STATE(x) (__c_check_state ((x), messages, \
              typecast_states, TYPECAST_STATE_COLS))

int is_typecast_expr (MESSAGE_STACK messages, int start, int *end) {
  int i,
    state,
    n_parens,
    lookahead,
    stack_end,
    paren_delim;
  MESSAGE *m_tok;

  /* Empty set of parentheses. */
  if (M_TOK(messages[start]) == CLOSEPAREN)
    return FALSE;

  stack_end = get_stack_top (messages);

  if (M_TOK(messages[start]) == OPENPAREN) {
    paren_delim = TRUE;
  } else {
    paren_delim = FALSE;
  }

  for (i = start, n_parens = 0; i >= stack_end; i--) {

    m_tok = messages[i];

    if (M_ISSPACE (m_tok)) continue;

    /*
     *  This is to handle sizeof () arguments.
     */
    if ((M_TOK(messages[i]) == CLOSEPAREN) &&
	(n_parens == 0) &&
	(paren_delim == FALSE)) {
      int i_1;
      for (i_1 = start; i_1 > i; i_1--) {
	if (M_ISSPACE(messages[i_1])) 
	  continue;
	switch (M_TOK(messages[i_1]))
	  {
	  case LABEL:
	    if (!is_c_data_type (M_NAME(messages[i_1])) &&
		!get_typedef (M_NAME(messages[i_1])))
	      return FALSE;
	    break;
	  case MULT:
	  case ARRAYOPEN:
	  case ARRAYCLOSE:
	    return TRUE;
	    break;
	  case CTYPE:
	    return TRUE;
	    break;
	  default:
	    return FALSE;
	    break;
	  }
      }
      return TRUE;
    }

    if ((state = TYPECAST_STATE (i)) == ERROR)
      return FALSE;

    switch (m_tok -> tokentype)
      {
      case LABEL:
      case CTYPE:
	if (!is_c_data_type (M_NAME(m_tok)) &&
	    !get_typedef (M_NAME(m_tok)) &&
	    !get_global_var (M_NAME(m_tok))) {
	  /*
	   *  NOTE: The var pass can also identify this 
	   *  expression as an incomplete type.
	   *
	   *  TO DO: If it becomes an issue, make note 
	   *  of the unknown type, and only issue a warning
	   *  on exit if the expression syntactically
	   *  would be a type cast otherwise.
	   *
	   *  First, do a quick lookahead for anything not 
	   *  a valid typecast token.
	   */
	  if (n_parens) {
	    int lookahead = i - 1;

	    for (;lookahead >= stack_end; lookahead--) {
	      if ((M_TOK(messages[lookahead]) != OPENPAREN) ||
		  (M_TOK(messages[lookahead]) != CLOSEPAREN) ||
		  !M_ISSPACE(messages[lookahead]) ||
		  (M_TOK(messages[lookahead]) != ASTERISK) ||
		  (M_TOK(messages[lookahead]) != LABEL))
		return FALSE;
	    }

	    if ((M_TOK(messages[lookahead]) == LABEL) ||
		(M_TOK(messages[lookahead]) == CTYPE)) {
	      warning (m_tok, "Unknown type, \"%s,\" in typecast.", 
		       M_NAME(m_tok));
	      break;
	    }
	  }
	}
	break;
      case OPENPAREN:
	++n_parens;
	if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	  if ((M_TOK(messages[lookahead]) == CTYPE) ||
	      (M_TOK(messages[lookahead]) == LABEL)) {
	    if (!is_c_data_type (M_NAME(messages[lookahead])) &&
		!get_typedef (M_NAME(messages[lookahead])))
	      return FALSE;
	  }
	}
	break;
      case CLOSEPAREN:
	if (n_parens == 1) {
	  *end = i;
	  return TRUE;
	} else {
	  if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	    if (M_TOK(messages[lookahead]) != INTEGER &&
		M_TOK(messages[lookahead]) != LONG &&
		M_TOK(messages[lookahead]) != LONGLONG &&
		M_TOK(messages[lookahead]) != FLOAT &&
		M_TOK(messages[lookahead]) != OPENPAREN &&
		M_TOK(messages[lookahead]) != CLOSEPAREN &&
		M_TOK(messages[lookahead]) != CHAR &&
		M_TOK(messages[lookahead]) != LITERAL_CHAR &&
		M_TOK(messages[lookahead]) != LITERAL) {
	      return FALSE;
	    }
	  }
	}
	--n_parens;
	break;
      default:
	break;
      }
  }

  return FALSE;

}

/*
 *  Should be called with param_start_ptr and param_end_ptr pointing
 *  to the opening and closing parentheses of the parameter list.
 */

static int typecast_ptr = 0;
static char  *typecast_types[MAXARGS];
static PARAMCVAR typecasts[MAXARGS];
char *default_typecast_type = "int";

static void typecast_types_to_typecast_cvar (int n_types) {
  switch (n_types)
    {
    case 0:
      typecasts[typecast_ptr].type = default_typecast_type;
      break;
    case 1:
      typecasts[typecast_ptr].type = typecast_types[0];
      break;
    case 2:
      typecasts[typecast_ptr].qualifier = typecast_types[0];
      typecasts[typecast_ptr].type = typecast_types[1];
      break;
    case 3:
      typecasts[typecast_ptr].qualifier2 = typecast_types[0];
      typecasts[typecast_ptr].qualifier = typecast_types[1];
      typecasts[typecast_ptr].type = typecast_types[2];
      break;
    case 4:
      typecasts[typecast_ptr].qualifier3 = typecast_types[0];
      typecasts[typecast_ptr].qualifier2 = typecast_types[1];
      typecasts[typecast_ptr].qualifier = typecast_types[2];
      typecasts[typecast_ptr].type = typecast_types[3];
      break;
    case 5:
      typecasts[typecast_ptr].qualifier4 = typecast_types[0];
      typecasts[typecast_ptr].qualifier3 = typecast_types[1];
      typecasts[typecast_ptr].qualifier2 = typecast_types[2];
      typecasts[typecast_ptr].qualifier = typecast_types[3];
      typecasts[typecast_ptr].type = typecast_types[4];
      break;
    }
}

static int typecast_type (MESSAGE_STACK messages, int param_start_ptr,
			  int param_end_ptr){

  int i,
    type_level = 0, n_blocks;

  typecast_ptr = 0;
  memset (&typecasts[typecast_ptr], 0, sizeof (PARAMCVAR));
  typecasts[typecast_ptr].sig = CVAR_SIG;
  typecasts[typecast_ptr].scope = ARG_VAR;
  type_level = 0;
  n_blocks = 0;

  for (i = param_start_ptr - 1; i > param_end_ptr; i--) {

    switch (messages[i] -> tokentype)
      {
      case LABEL:
	if (is_c_storage_class (messages[i] -> name)) {
	  typecasts[typecast_ptr].storage_class = messages[i] -> name;
	} else {
 	  if (is_c_data_type (messages[i] -> name)) {
	    typecast_types[type_level] = M_NAME(messages[i]);
	    ++type_level;
	  } else {
	    if (get_typedef (M_NAME(messages[i]))) {
	      typecast_types[type_level] = M_NAME(messages[i]);
	      ++type_level;
	    } else {
	      typecasts[typecast_ptr].name = messages[i] -> name;
	    }
	  }
	}
	break;
      case ASTERISK:
	++typecasts[typecast_ptr].n_derefs;
	break;
      case ARRAYOPEN:
	++n_blocks;
	break;
      case ARRAYCLOSE:
	--n_blocks;
	if (n_blocks == 0)
	  ++typecasts[typecast_ptr].n_derefs;
	break;
      case ARGSEPARATOR:
	typecast_types_to_typecast_cvar (type_level);
	typecast_ptr++;
	memset (&typecasts[typecast_ptr], 0, sizeof (PARAMCVAR));
	typecasts[typecast_ptr].sig = CVAR_SIG;
	typecasts[typecast_ptr].scope = ARG_VAR;
	type_level = 0;
	break;
      case ELLIPSIS:
	typecast_types[type_level++] = "OBJECT";
	typecasts[typecast_ptr].name = M_NAME(messages[i]);
	typecasts[typecast_ptr].n_derefs = 1;
	/*	have_varargs = TRUE; */
	break;
      }
    
  }

  typecast_types_to_typecast_cvar (type_level);

  return SUCCESS;
}

extern CVAR *fn_param_decls,
  *fn_param_decls_ptr;

char *basic_class_from_typecast (MESSAGE_STACK messages, int start,
				int end) {
  PARAMCVAR c;

  typecast_type (messages, start, end);
  if (typecasts[0].type == NULL) {
    return INTEGER_CLASSNAME;
  } else {
    c = typecasts[0];
    return basic_class_from_paramcvar (messages[start],
				       &c, 0);
  }
}

/* #define CLASS_CAST_WARNINGS */

int class_cast_receiver_scan (MESSAGE_STACK messages,
			      int stack_top_idx,
			      int cast_end_idx,
			      int *receiver_label_idx,
			      int *deref_prefix_op_idx) {
  int i;

  for (i = cast_end_idx - 1, *receiver_label_idx = -1;
       (i > stack_top_idx) && (*receiver_label_idx == -1);
       --i) {
    if (M_ISSPACE(messages[i]))
      continue;
    switch (M_TOK(messages[i]))
      {
      case OPENPAREN:
	break;
      case ASTERISK:
	*deref_prefix_op_idx = i;
	break;
      case LABEL:
	*receiver_label_idx = i;
	return SUCCESS;
	break;
      default:
	return ERROR;
	break;
      }
  }
  return ERROR;
}

extern OBJECT *classes;         /* Declared in class.c. */
extern OBJECT *last_class;

/*
 *  Sets the self token's receiver_msg member to the class label, which
 *  already has a class object as the obj member, so we can use the 
 *  TOK_HAS_CLASS_TYPECAST macro when evaluating "self" tokens.
 *
 *  Checks for, "super," too.
 */
bool is_class_typecast (MSINFO *msi, int class_obj_tok) {
  int i;
  int open_paren_idx = class_obj_tok,  /* Avoid warning messages. */
    close_paren_idx = class_obj_tok;
  int rcvr_tok_idx = class_obj_tok;
  int deref_idx;
  OBJECT *c;
  bool have_class = false;
  
  for (c = classes; c; c = c -> next) {
    if (str_eq (c -> __o_name, M_NAME(msi -> messages[class_obj_tok]))) {
      have_class = true;
      break;
    }
  }
  if (!have_class)
    return false;

  for (i = class_obj_tok + 1; i <= msi -> stack_start; i++) {
    switch (M_TOK(msi -> messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case OPENPAREN:
	open_paren_idx = i;
	goto cast_open_paren_done;
	break;
      default:
	return false;
	break;
      }
  }
 cast_open_paren_done:
  for (i = class_obj_tok - 1; i >= msi -> stack_ptr; i--) {
    switch (M_TOK(msi -> messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
      case ASTERISK:
	continue;
	break;
      case CLOSEPAREN:
	close_paren_idx = i;
	goto cast_close_paren_done;
	break;
      default:
	return false;
	break;
      }
  }

 cast_close_paren_done:
  if (class_cast_receiver_scan (msi -> messages,
				msi -> stack_ptr,
				close_paren_idx,
				&rcvr_tok_idx, &deref_idx) == ERROR) {
    return false;
  }

  
  /* The self and super check needs to be a string comparison
     because this function looks ahead of eval_arg setting the
     TOK_SELF and TOK_SUPER attributes on the m_message stack. */
  if (!str_eq (M_NAME(msi -> messages[rcvr_tok_idx]), "self") &&
      !str_eq (M_NAME(msi -> messages[rcvr_tok_idx]), "super") &&
      !get_local_var (M_NAME(msi -> messages[rcvr_tok_idx])) &&
      !get_global_var (M_NAME(msi -> messages[rcvr_tok_idx])) &&
      !get_object (M_NAME(msi -> messages[rcvr_tok_idx]), NULL) &&
      !is_method_parameter (msi -> messages, rcvr_tok_idx)) { /***/
#ifdef CLASS_CAST_WARNINGS
    warning (msi -> messages[close_paren_idx],
	     "Undefined class cast receiver, \"%s.\"",
	     M_NAME(msi -> messages[rcvr_tok_idx]));
#endif
    return false;
  }

  msi -> messages[class_obj_tok] -> attrs |= TOK_IS_CLASS_TYPECAST;
  msi -> messages[rcvr_tok_idx] -> receiver_msg =
    msi -> messages[class_obj_tok];

  for (i = open_paren_idx; i >= close_paren_idx; i--) {
    ++msi -> messages[i] -> evaled;
    ++msi -> messages[i] -> output;
  }

  return true;

}

/* As above, but scans from the opening paren. Used by eval_arg. */
bool is_class_typecast_2 (MSINFO *msi, int open_paren_tok) {
  int i;
  int class_obj_idx,
    close_paren_idx;
  int rcvr_tok_idx;
  int deref_idx;
  OBJECT *c;
  bool have_class = false;
  
  if ((close_paren_idx = match_paren (msi -> messages, open_paren_tok,
				      msi -> stack_ptr)) == ERROR) {
    return false;
  }

  if ((class_obj_idx = nextlangmsg (msi -> messages, open_paren_tok))
      ==ERROR) {
    return false;
  }

  for (c = classes; c; c = c -> next) {
    if (str_eq (c -> __o_name, M_NAME(msi -> messages[class_obj_idx]))) {
      have_class = true;
      break;
    }
  }
  if (!have_class)
    return false;

#if 0
  for (i = class_obj_idx + 1; i <= msi -> stack_start; i++) {
    switch (M_TOK(msi -> messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case OPENPAREN:
	open_paren_idx = i;
	goto cast_open_paren_done;
	break;
      default:
	return false;
	break;
      }
  }
 cast_open_paren_done:
#endif  

  for (i = class_obj_idx - 1; i >= msi -> stack_ptr; i--) {
    switch (M_TOK(msi -> messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
      case ASTERISK:
	continue;
	break;
      case CLOSEPAREN:
	close_paren_idx = i;
	goto cast_close_paren_done;
	break;
      default:
	return false;
	break;
      }
  }

 cast_close_paren_done:
  if (class_cast_receiver_scan (msi -> messages,
				msi -> stack_ptr,
				close_paren_idx,
				&rcvr_tok_idx, &deref_idx) == ERROR) {
    return false;
  }

  
  /* The self and super check needs to be a string comparison
     because this function looks ahead of eval_arg setting the
     TOK_SELF and TOK_SUPER attributes on the m_message stack. */
  if (!str_eq (M_NAME(msi -> messages[rcvr_tok_idx]), "self") &&
      !str_eq (M_NAME(msi -> messages[rcvr_tok_idx]), "super") &&
      !get_local_var (M_NAME(msi -> messages[rcvr_tok_idx])) &&
      !get_global_var (M_NAME(msi -> messages[rcvr_tok_idx])) &&
      !get_object (M_NAME(msi -> messages[rcvr_tok_idx]), NULL) &&
      !is_method_parameter (msi -> messages, rcvr_tok_idx)) { /***/
#ifdef CLASS_CAST_WARNINGS
    warning (msi -> messages[close_paren_idx],
	     "Undefined class cast receiver, \"%s.\"",
	     M_NAME(msi -> messages[rcvr_tok_idx]));
#endif
    return false;
  }

  msi -> messages[class_obj_idx] -> attrs |= TOK_IS_CLASS_TYPECAST;
  msi -> messages[rcvr_tok_idx] -> receiver_msg =
    msi -> messages[class_obj_idx];

  for (i = open_paren_tok; i >= close_paren_idx; i--) {
    ++msi -> messages[i] -> evaled;
    ++msi -> messages[i] -> output;
  }

  return true;

}

bool has_typecast_form (MSINFO *ms, int open_paren_idx) {
  int i, n_parens = 0, n_labels = 0;
  for (i = open_paren_idx; i > ms -> stack_ptr; i--) {
    switch (M_TOK(ms -> messages[i]))
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (n_parens == 0) {
	  if (n_labels == 1) {
	    return true;
	  } else {
	    return false;
	  }
	}
	break;
      case LABEL:
      case CTYPE:
	++n_labels;
	break;
      case ASTERISK:
      case WHITESPACE:
      case NEWLINE:
	break;
      default:
	return false;
      }
  }
  return false;
}
