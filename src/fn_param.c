/* $Id: fn_param.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ -*-c-*-**/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015, 2017 Robert Kiesling, rk3314042@gmail.com.
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
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "cvar.h"
#include "parser.h"

int function_param_states[] = {
#include "paramstate.h"
  -1,   0
};

#define FUNCTION_PARAM_STATE_COLS 2

#define FUNCTION_PARAM_STATE(x) (check_state ((x), messages, \
              function_param_states, FUNCTION_PARAM_STATE_COLS))

static inline PARAMLIST_ENTRY *new_paramlist_entry (void) {
  static PARAMLIST_ENTRY *p;
  if ((p = (PARAMLIST_ENTRY *)__xalloc (sizeof (struct _paramlist_entry)))
      == NULL)
    _error ("new_paramlist_entry (): %s.\n", strerror (errno));
  return p;
}

/*
 *  Call with start_ptr and end_ptr pointing to the opening
 *  and closing parentheses of the argument list.
 *  Note the states for prototype declarations, above.  If
 *  the function encounters a prototype declaration without
 *  the parameter name, it skips the declaration.  This is
 *  consistent with is_c_var_declaration_msg ().
 */

static bool fn_varargs = false;

bool fn_decl_varargs (void) {
  return fn_varargs;
}

int fn_param_declarations (MESSAGE_STACK messages, int start_ptr, int end_ptr,
			       PARAMLIST_ENTRY **args, int *n_args) {
  int j, k;
  int state;
  int last_state = ERROR;               /* Avoid warnings. */
  int n_parens;
  int param_start_ptr = start_ptr;
  MESSAGE *m_tok;

  fn_varargs = false;

  for (j = start_ptr, *n_args = 0, n_parens = 0; j >= end_ptr; j--) {

    if ((state = FUNCTION_PARAM_STATE (j)) == ERROR) {
      /* Checking here for white space simplifies the state 
	 diagram, and check_state skips the whitespace tokens,
	 but the whitespace still ends up in the parameter in 
	 the args array.
	 
	 GNU C attributes are not in the state transition diagram,
	 either.
      */
#ifdef __GNUC__
      if ((j != end_ptr) || 
	  ((j == end_ptr) && (!fn_has_gnu_attribute (messages, j)))) {
 	if (!M_ISSPACE (messages[j])) {
 	  warning (messages[j], "Function argument is not a C expression (in progress).");
	  return ERROR;
	}
      }
#else
      if (!M_ISSPACE (messages[j]))
	error (messages[j], "Parse error.");
#endif
    }

    m_tok = messages[j];

    switch (m_tok -> tokentype)
      {
      case LABEL:
	if (last_state == 0 || last_state == 6) {
	  if (n_parens == 1) {
	    if ((state == 4) && (last_state == 0)) {  /* "void" alone. */
	      if (strcmp (m_tok -> name, PARAM_PREFIX_METHOD_ATTR)) {
		param_start_ptr = ERROR;
	      } else {
		param_start_ptr = j;
	      } 
	    } else {
	      param_start_ptr = j;
	    }
	  }
	}
	break;
      case CTYPE:
	if (state == 843 && last_state == 842) { /* "void" alone */
	  param_start_ptr = ERROR;
	}
	break;
      case CLOSEPAREN:
	--n_parens;
	if ((last_state != 19) && (last_state != 20)) {
	  /*
	   *  Make sure that the paren is the end 
	   *  of the parameter list.
	   */
	  if ((param_start_ptr != ERROR) && (j == end_ptr)) {
	    int empty_arglist_tok_check;
	    args[*n_args] = new_paramlist_entry ();
	    for (k = param_start_ptr; k > j; k--)
	      strcatx2 (args[*n_args] -> buf, messages[k] -> name, NULL);

	    if ((empty_arglist_tok_check = nextlangmsg (messages, start_ptr))
		!= ERROR) {
	      if (empty_arglist_tok_check == j) {
		*n_args = 0;
	      } else {
		args[*n_args] -> start_ptr = param_start_ptr;
		args[*n_args] -> end_ptr = k;
		(*n_args)++;
	      }
	    } else {
	      args[*n_args] -> start_ptr = param_start_ptr;
	      args[*n_args] -> end_ptr = k;
	      (*n_args)++;
	    }
	  }
	}
	break;
      case ARGSEPARATOR:
	if ((last_state != 19) && (last_state != 20)) {
	  if (param_start_ptr != ERROR) {
	    args[*n_args] = new_paramlist_entry ();
	    for (k = param_start_ptr; k > j; k--)
	      strcatx2 (args[*n_args] -> buf, messages[k] -> name, NULL);
	    args[*n_args] -> start_ptr = param_start_ptr;
	    args[*n_args] -> end_ptr = k;
	    (*n_args)++;
	  }
	}
	break;
      case ARRAYOPEN:
	j = find_arg_end (messages, j);
	++j;
	break;
      case ELLIPSIS:
	param_start_ptr = j;
	fn_varargs = true;
	break;
      case OPENPAREN:
	++n_parens;
	if (((state == 0) && (last_state == 96)) ||/* Typecast: , <(>label..*/
	    ((state == 0) && (last_state == 97))){ /* Typecast: (<(>label...*/
	  param_start_ptr = j;
	}
	break;
      case INTEGER:
	if ((last_state == 82) || (last_state == 48)) {
	  param_start_ptr = j;
	}
	break;
      case DOUBLE:  /* Float or double. */
	if ((last_state == 83) || (last_state == 84) || 
	    (last_state == 49) || (last_state == 50)) {
	  param_start_ptr = j;
	}
	break;
      case LITERAL:
	if ((last_state == 79) || (last_state == 16)) {
	  param_start_ptr = j;
	}
	break;
      case LITERAL_CHAR:
	if ((last_state == 80) || (last_state == 100)) {
	  param_start_ptr = j;
	}
	break;
      case LONG:
	if ((last_state == 85) || (last_state == 51)) {
	  param_start_ptr = j;
	}
	break;
      case LONGLONG:
	if ((last_state == 86) || (last_state == 52)) {
	  param_start_ptr = j;
	}
	break;
      case AMPERSAND:
	if ((last_state == 87) || (last_state == 53)) {
	  param_start_ptr = j;
	}
	break;
      case EXCLAM:
	if ((last_state == 88) || (last_state == 54)) {
	  param_start_ptr = j;
	}
	break;
      case INCREMENT:
	if ((last_state == 89) || (last_state == 55)) {
	  param_start_ptr = j;
	}
	break;
      case DECREMENT:
	if ((last_state == 90) || (last_state == 56)) {
	  param_start_ptr = j;
	}
	break;
      case BIT_COMP:
	if ((last_state == 91) || (last_state == 57)) {
	  param_start_ptr = j;
	}
	break;
      case ASTERISK:
	if ((last_state == 92) || (last_state == 58)) {
	  param_start_ptr = j;
	}
	/* TODO - This should be a fixup. State 23 and state 58
	   are the same. */
	if ((last_state == 23) && (param_start_ptr == start_ptr))
	  param_start_ptr = j;
	break;
      default:
	break;
      }

    /* See the comment above. */
    if ((m_tok -> tokentype != WHITESPACE) &&
	(m_tok -> tokentype != NEWLINE))
    last_state = state;
  }

  return SUCCESS;
}

/*
 *  As above, except that it allows a C type alone as a parameter declaration.
 */
int fn_prototype_param_declarations (MESSAGE_STACK messages, int start_ptr, 
				     int end_ptr, 
				    PARAMLIST_ENTRY **args, int *n_args) {
  int j, k;
  int state;
  int last_state = ERROR;               /* Avoid warnings. */
  int n_parens;
  int param_start_ptr = start_ptr;
  MESSAGE *m_tok;

  for (j = start_ptr, *n_args = 0, n_parens = 0; j >= end_ptr; j--) {

    if ((state = FUNCTION_PARAM_STATE (j)) == ERROR) {
      /* Checking here for white space simplifies the state 
	 diagram, and check_state skips the whitespace tokens,
	 but the whitespace still ends up in the parameter in 
	 the args array.
	 
	 GNU C attributes are not in the state transition diagram,
	 either.
      */
#ifdef __GNUC__
      if ((j != end_ptr) || 
	  ((j == end_ptr) && (!fn_has_gnu_attribute (messages, j)))) {
 	if (!M_ISSPACE (messages[j])) {
 	  warning (messages[j], "Function argument is not a C expression (in progress).");
	  return ERROR;
	}
      }
#else
      if (!M_ISSPACE (messages[j]))
	error (messages[j], "Parse error.");
#endif
    }

    m_tok = messages[j];

    switch (m_tok -> tokentype)
      {
      case LABEL:
	if (last_state == 0 || last_state == 6) {
	  if (n_parens == 1) {
	    if ((state == 4) && (last_state == 0)) { /* <label> ')' */
	      param_start_ptr = j;
	    } else if ((state == 3) && (last_state == 0)) { /* <label> <label> */
	      param_start_ptr = j;
	    } else if ((state == 2) && (last_state == 0)) { /* <label> '*' */
	      param_start_ptr = j;
	    } else if ((state == 15) && (last_state == 0)) { /* <label> '(' */
	      param_start_ptr = j;
	    } else if ((state == 5) && (last_state == 0)) { /* <label>  ',' */
	      param_start_ptr = j;
	    }
	  }
	}
	break;
      case CLOSEPAREN:
	--n_parens;
	if ((last_state != 19) && (last_state != 20)) {
	  /*
	   *  Make sure that the paren is the end 
	   *  of the parameter list.
	   */
	  if ((param_start_ptr != ERROR) && (j == end_ptr)) {
	    args[*n_args] = new_paramlist_entry ();
	    for (k = param_start_ptr; k > j; k--)
	      strcatx2 (args[*n_args] -> buf, messages[k] -> name, NULL);
	    args[*n_args] -> start_ptr = param_start_ptr;
	    args[*n_args] -> end_ptr = k;
	    (*n_args)++;
	  }
	}
	break;
      case ARGSEPARATOR:
	if ((last_state != 19) && (last_state != 20)) {
	  if (param_start_ptr != ERROR) {
	    args[*n_args] = new_paramlist_entry ();
	    for (k = param_start_ptr; k > j; k--)
	      strcatx2 (args[*n_args] -> buf, messages[k] -> name, NULL);
	    args[*n_args] -> start_ptr = param_start_ptr;
	    args[*n_args] -> end_ptr = k;
	    (*n_args)++;
	    if ((param_start_ptr = nextlangmsg (messages, k)) == ERROR) {
	      return SUCCESS;
	    }
	  }
	}
	break;
      case ARRAYOPEN:
	j = find_arg_end (messages, j);
	++j;
	break;
      case ELLIPSIS:
	param_start_ptr = j;
	break;
      case OPENPAREN:
	++n_parens;
	if (((state == 0) && (last_state == 96)) ||/* Typecast: , <(>label..*/
	    ((state == 0) && (last_state == 97))){ /* Typecast: (<(>label...*/
	  param_start_ptr = j;
	}
	break;
      case INTEGER:
	if ((last_state == 82) || (last_state == 48)) {
	  param_start_ptr = j;
	}
	break;
      case DOUBLE:  /* Float or double. */
	if ((last_state == 83) || (last_state == 84) || 
	    (last_state == 49) || (last_state == 50)) {
	  param_start_ptr = j;
	}
	break;
      case LITERAL:
	if ((last_state == 79) || (last_state == 16)) {
	  param_start_ptr = j;
	}
	break;
      case LITERAL_CHAR:
	if ((last_state == 80) || (last_state == 100)) {
	  param_start_ptr = j;
	}
	break;
      case LONG:
	if ((last_state == 85) || (last_state == 51)) {
	  param_start_ptr = j;
	}
	break;
      case LONGLONG:
	if ((last_state == 86) || (last_state == 52)) {
	  param_start_ptr = j;
	}
	break;
      case AMPERSAND:
	if ((last_state == 87) || (last_state == 53)) {
	  param_start_ptr = j;
	}
	break;
      case EXCLAM:
	if ((last_state == 88) || (last_state == 54)) {
	  param_start_ptr = j;
	}
	break;
      case INCREMENT:
	if ((last_state == 89) || (last_state == 55)) {
	  param_start_ptr = j;
	}
	break;
      case DECREMENT:
	if ((last_state == 90) || (last_state == 56)) {
	  param_start_ptr = j;
	}
	break;
      case BIT_COMP:
	if ((last_state == 91) || (last_state == 57)) {
	  param_start_ptr = j;
	}
	break;
      case ASTERISK:
	if ((last_state == 92) || (last_state == 58)) {
	  param_start_ptr = j;
	}
	break;
      default:
	break;
      }

    /* See the comment above. */
    if ((m_tok -> tokentype != WHITESPACE) &&
	(m_tok -> tokentype != NEWLINE))
    last_state = state;
  }

  return SUCCESS;
}


