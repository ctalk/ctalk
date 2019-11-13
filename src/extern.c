/* $Id: extern.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016 Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"

extern HASHTAB declared_functions;     /* Declared in cvars.c.   */

/* an abbreviated version that doesn't fill 
the fn declaration stuff all over again.... */
static bool extern_is_prototype (MESSAGE_STACK messages, int keyword_idx,
				 int stack_end) {
  int i;

  for (i = keyword_idx; i > stack_end; --i) {
    switch (M_TOK(messages[i]))
      {
      case SEMICOLON:
	return true;
	break;
      case OPENBLOCK:
	return false;
	break;
      case EQ:
	/* A function might have an initializer somewhere.... */
	return false;
	break;
      }
  }
  return false;
}

static int get_qualifier_count (MESSAGE_STACK messages, int start_idx,
				int end_idx) {
  int i;
  int n_qualifiers = 0;
  for (i = start_idx; i >= end_idx; --i) {
    if (M_ISSPACE(messages[i]))
      continue;
    if (is_c_data_type (M_NAME(messages[i])) ||
	is_incomplete_type (M_NAME(messages[i])) ||
	get_typedef (M_NAME(messages[i]))) {
      ++n_qualifiers;
    } else {
      break;
    }
  }
  return n_qualifiers;
}

static int param_n_derefs (MESSAGE_STACK messages, int start_idx,
			   int end_idx) {
  int i;
  int n_derefs = 0;
  for (i = start_idx; i >= end_idx; --i) {
    if (M_TOK(messages[i]) == MULT) {
      ++n_derefs;
    } else if (M_TOK(messages[i]) == ARRAYCLOSE) {
      /* one deref for each set of subscripts. */
      ++n_derefs;
    }
  }
  return n_derefs;
}

static void find_arglist (MESSAGE_STACK messages, int keyword_ptr,
		   int *arg_start, int *arg_end) {
  int open_paren_idx, close_paren_idx;

  if ((open_paren_idx = scanforward (messages, keyword_ptr - 1, 
				get_stack_top (messages),
				       OPENPAREN)) == ERROR)
    _error ("Parse error.\n");

  if ((close_paren_idx = match_paren (messages, open_paren_idx,
				      get_stack_top (messages)))
      == ERROR) 
    _error ("Parse error.\n");

  /* *arg_start = nextlangmsg (messages, open_paren_idx);
   *arg_end = prevlangmsg (messages, close_paren_idx); */
  *arg_start = open_paren_idx;
  *arg_end = close_paren_idx;
}

extern int add_function (MESSAGE *);

int extern_declaration (MESSAGE_STACK messages, int keyword_ptr) {

  int lookahead, arg_start, arg_end,
    stack_ptr;
  CVAR *c, *param_c, *param_head;
  CFUNC *cfn;
  PARAMLIST_ENTRY *params[MAXARGS];
  int i, j, n_params, n_type_labels, nth_label;

  if ((lookahead = scanforward (messages, keyword_ptr - 1, 
			   get_stack_top (messages), LABEL)) == ERROR)
    _error ("Parse error.\n");

  stack_ptr = get_stack_top (messages);
  if (is_c_fn_declaration_msg (messages, lookahead, stack_ptr)) {
    add_function (messages[lookahead]);
    return SUCCESS;
  }
  if (is_c_var_declaration_msg (messages, lookahead, stack_ptr, TRUE)) {
    if ((c = parser_to_cvars ()) != NULL) {
      c -> attrs |= CVAR_ATTR_EXTERN;
      if ((c -> attrs & CVAR_ATTR_FN_DECL) ||
	  (c-> attrs & CVAR_ATTR_FN_PTR_DECL) ||
	  (c -> attrs & CVAR_ATTR_FN_PROTOTYPE)) {
	cfn = cvar_to_cfunc (c);
	if (!cfn -> params) {
	  find_arglist (messages, keyword_ptr, &arg_start, &arg_end);
	  if (arg_start != -1 && arg_end != -1) {
	    if (extern_is_prototype (messages, keyword_ptr, stack_ptr)) {
	      fn_prototype_param_declarations (messages, arg_start, arg_end,
					       params, &n_params);
	      param_head = cfn -> params;
	      for (i = 0; i < n_params; ++i) {
		if (params[i] -> start_ptr == params[i] -> end_ptr) {
		  _new_cvar (CVARREF(param_c));
		  if (cfn -> params == NULL) 
		    cfn -> params = param_c;
		  else
		    param_head = param_c;
		  strcpy (param_c -> type, params[i] -> buf);
		  param_head = param_c -> next;
		} else if ((M_TOK(messages[params[i]->start_ptr])
			    == OPENPAREN) &&
			   (M_TOK(messages[params[i]->end_ptr])
			    == CLOSEPAREN)) {
		  /* Empty set of parens. */
		  continue;
		} else if (!strncmp (params[i] -> buf, "...", 3)) {
		  /* In case there's trailing whitespace at the end. */
		  _new_cvar (CVARREF(param_c));
		  if (cfn -> params == NULL) 
		    cfn -> params = param_c;
		  else
		    param_head = param_c;
		  strcpy (param_c -> type, params[i] -> buf);
		  param_head = param_c -> next;
		} else {
		  n_type_labels = get_qualifier_count
		    (messages, params[i] -> start_ptr, params[i] -> end_ptr);
		  nth_label = 0;
		
		  switch (n_type_labels)
		    {
		    case 1:
		      _new_cvar (CVARREF(param_c));
		      if (cfn -> params == NULL) 
			cfn -> params = param_c;
		      else
			param_head -> next = param_c;
		      for (j = params[i] -> start_ptr;
			   j >= params[i] -> end_ptr; --j) {
			if (M_TOK(messages[j]) == LABEL) {
			  strcpy (param_c -> type, M_NAME(messages[j]));
			  ++nth_label;
			  if (nth_label == n_type_labels)
			    break;
			}
		      }
		      param_head = param_c;
		      break;
		    case 2:
		      _new_cvar (CVARREF(param_c));
		      if (cfn -> params ==  NULL)
			cfn -> params = param_c;
		      else
			param_head -> next = param_c;
		      for (j = params[i] -> start_ptr;
			   j >= params[i] -> end_ptr; --j) {
			if (M_TOK(messages[j]) == LABEL) {
			  if (nth_label == 0) {
			    strcpy (param_c -> qualifier,
				    M_NAME(messages[j]));
			    ++nth_label;
			  } else if (nth_label == 1) {
			    strcpy (param_c -> type,
				    M_NAME(messages[j]));
			    ++nth_label;
			  }
			  if (nth_label == n_type_labels)
			    break;
			}
		      }
		      param_head = param_c;
		      break;
		    case 3:
		      _new_cvar (CVARREF(param_c));
		      if (cfn -> params ==  NULL)
			cfn -> params = param_c;
		      else
			param_head -> next = param_c;
		      for (j = params[i] -> start_ptr;
			   j >= params[i] -> end_ptr; --j) {
			if (M_TOK(messages[j]) == LABEL) {
			  if (nth_label == 0) {
			    strcpy (param_c -> qualifier2,
				    M_NAME(messages[j]));
			    ++nth_label;
			  } else if (nth_label == 1) {
			    strcpy (param_c -> qualifier,
				    M_NAME(messages[j]));
			    ++nth_label;
			  } else if (nth_label == 2) {
			    strcpy (param_c -> type,
				    M_NAME(messages[j]));
			    ++nth_label;
			  } 
			  if (nth_label == n_type_labels)
			    break;
			}
		      }
		      param_head = param_c;
		      break;
		    case 4:
		      _new_cvar (CVARREF(param_c));
		      if (cfn -> params ==  NULL)
			cfn -> params = param_c;
		      else
			param_head -> next = param_c;
		      for (j = params[i] -> start_ptr;
			   j >= params[i] -> end_ptr; --j) {
			if (M_TOK(messages[j]) == LABEL) {
			  if (nth_label == 0) {
			    strcpy (param_c -> qualifier3,
				    M_NAME(messages[j]));
			    ++nth_label;
			  } else if (nth_label == 1) {
			    strcpy (param_c -> qualifier2,
				    M_NAME(messages[j]));
			    ++nth_label;
			  } else if (nth_label == 2) {
			    strcpy (param_c -> qualifier,
				    M_NAME(messages[j]));
			    ++nth_label;
			  } else if (nth_label == 3) {
			    strcpy (param_c -> type,
				    M_NAME(messages[j]));
			    ++nth_label;
			  }
			  if (nth_label == n_type_labels)
			    break;
			}
		      }
		      param_head = param_c;
		      break;
		    default:
		      warning (messages[keyword_ptr],
			       "Unhandled extern param syntax:\n"
			       "Function, \"%s,\" param, \"%s.\"",
			       cfn -> decl, params[i] -> buf);
		    }
		  param_c -> n_derefs =
		    param_n_derefs (messages, params[i] -> start_ptr,
				    params[i] -> end_ptr);
		}
		__xfree (MEMADDR(params[i]));
	      }
	    } else {
	      warning (messages[keyword_ptr],
		       "Unhandled parameter list in extern_declaration");
	    }
	  }
	} /* if (!cfn -> params) */
	_delete_cvar (c);
	unlink_global_cvar (c);
	_hash_put (declared_functions, (void *)cfn, cfn -> decl);
      } else {
	add_variable_from_cvar (c);
      }
    }
  }

  return SUCCESS;
}

