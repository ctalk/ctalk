/* $Id: refobj.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2016-2017 Robert Kiesling, rk3314042@gmail.com.
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
 *  Object references only in C functions for now.
 *
 *  Only simple assignment expressions so far; e.g.,
 *
 *   my_c_variable = myObject;   
 *     ==>  my_c_variable = _makeRef ("myObject");
 *
 *  Further references to my_c_variable within its scope get changed
 *  to:
 *
 *    __getRef (my_c_variable)
 *
 *  which returns the actual myObject from its tag.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "ctrlblk.h"

extern I_PASS interpreter_pass;         /* Declared in lib/rtinfo.c.  */

LIST *local_objrefs = NULL;
LIST *global_objrefs = NULL;

bool have_ref (char *c_var_name) {
  char *s;
  LIST *l;
  if (local_objrefs) {
    for (l = local_objrefs; l; l = l -> next) {
      s = strstr ((char *)l -> data, ":::");
      if (!strncmp (c_var_name, (char *)l -> data, 
		    s - (char *)(l -> data))) {
	return true;
      }
    }
  }

  if (global_objrefs) {
    for (l = global_objrefs; l; l = l -> next) {
      s = strstr ((char *)l -> data, ":::");
      if (!strncmp (c_var_name, (char *)l -> data, 
		    s - (char *)(l -> data))) {
	return true;
      }
    }
  }
  return false;
}

/* Check for an OBJREF cast; i.e., 
 *
 *   &my_c_object_alias
 *
 *     or
 *
 *   &(my_c_object_alias)
 */
static int ref_is_objref (MESSAGE_STACK messages, int idx,
			   int stack_start_idx, int *openparen_idx) {
  int i;

  *openparen_idx = -1;
  for (i = idx + 1; i <= stack_start_idx; ++i) {
    switch (M_TOK(messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case OPENPAREN:
	*openparen_idx = i;
	break;
      case AMPERSAND:
	return i;
	break;
      default:
	return -1;
	break;
      }
  }
  return -1;
}

static char *fmt_object_reference (char *objname, char *c_var_name,
				   char *expr_out) {
  strcatx (expr_out, c_var_name, ":::", objname, NULL);
  return expr_out;
}

static bool match_alias_reference (char *tok, char *rec) {
  char *t, *r;
  t = tok;
  r = rec;
  while (*t && *r) {
    if (*t++ != *r++) {
      return false;
    }
  }
  if (*t == 0 && *r == ':') {
    return true;
  } else {
    return false;
  }
}

void delete_local_c_alias_reference (char *s) {
  LIST *l, *l_tmp;
  if (local_objrefs != NULL) {
    for (l = local_objrefs; l; l = l -> next) {
      if (match_alias_reference (s, (char *)l ->  data)) {
	if (l -> next)
	  l -> next -> prev = l -> prev;
	if (l -> prev)
	  l -> prev -> next = l -> next;
	if (l == local_objrefs) {
	  if (l -> next == NULL) {
	    __xfree (MEMADDR(l -> data));
	    __xfree (MEMADDR(l));
	    local_objrefs = NULL;
	  } else {
	    l_tmp = l -> next;
	    __xfree (MEMADDR(l -> data));
	    __xfree (MEMADDR(l));
	    l = l_tmp;
	  }
	  
	} else {
	  __xfree (MEMADDR(l -> data));
	  __xfree (MEMADDR(l));
	}
	return;
      }
    }
  }
}

void delete_global_c_alias_reference (char *s) {
  LIST *l, *l_tmp;
  if (global_objrefs != NULL) {
    for (l = global_objrefs; l; l = l -> next) {
      if (match_alias_reference (s, (char *)l -> data)) {
	if (l -> next)
	  l -> next -> prev = l -> prev;
	if (l -> prev)
	  l -> prev -> next = l -> next;
	if (l == global_objrefs) {
	  if (l -> next == NULL) {
	    __xfree (MEMADDR(l -> data));
	    __xfree (MEMADDR(l));
	    global_objrefs = NULL;
	  } else {
	    l_tmp = l -> next;
	    __xfree (MEMADDR(l -> data));
	    __xfree (MEMADDR(l));
	    l = l_tmp;
	  }
	  
	} else {
	  __xfree (MEMADDR(l -> data));
	  __xfree (MEMADDR(l));
	}
	return;
      }
    }
  }
}

void delete_local_c_object_references (void) {
  LIST *l, *l_prev;
  if (local_objrefs != NULL) {
    for (l = local_objrefs; l && l -> next; l = l -> next)
      ;
    if (l != local_objrefs) {
      while (l != local_objrefs) {
	l_prev = l -> prev;
	__xfree (MEMADDR(l -> data));
	__xfree (MEMADDR(l));
	l = l_prev;
      }
      __xfree (MEMADDR(local_objrefs -> data));
      __xfree (MEMADDR(local_objrefs));
      local_objrefs = NULL;
    } else {
      __xfree (MEMADDR(l -> data));
      __xfree (MEMADDR(l));
      local_objrefs = NULL;
    }
  }
}

void delete_global_c_object_references (void) {
  LIST *l, *l_prev;
  if (global_objrefs != NULL) {
    for (l = global_objrefs; l && l -> next; l = l -> next)
      ;
    if (l != global_objrefs) {
      while (l != global_objrefs) {
	l_prev = l -> prev;
	__xfree (MEMADDR(l -> data));
	__xfree (MEMADDR(l));
	l = l_prev;
      }
      __xfree (MEMADDR(global_objrefs -> data));
      __xfree (MEMADDR(global_objrefs));
      global_objrefs = NULL;
    } else {
      __xfree (MEMADDR(l -> data));
      __xfree (MEMADDR(l));
      global_objrefs = NULL;
    }
  }
}

static void add_local_objref  (char *objname, char *c_var_name) {
  LIST *l;
  char ref_str[MAXMSG];

  fmt_object_reference (objname, c_var_name, ref_str);
  l = new_list ();
  l -> data = (void *)strdup (ref_str);
  if (local_objrefs == NULL) {
    local_objrefs = l;
  } else {
    list_push (&local_objrefs, &l);
  }
}

static void add_global_objref  (char *objname, char *c_var_name) {
  LIST *l;
  char ref_str[MAXMSG];

  fmt_object_reference (objname, c_var_name, ref_str);
  l = new_list ();
  l -> data = (void *)strdup (ref_str);
  if (global_objrefs == NULL) {
    global_objrefs = l;
  } else {
    list_push (&global_objrefs, &l);
  }
}

char *new_object_reference (char *objname, char *c_var_name, 
			    int scope, char *expr_buf_out) {
  switch (scope)
    {
    case LOCAL_VAR:
      add_local_objref (objname, c_var_name);
      break;
    case GLOBAL_VAR:
      add_global_objref (objname, c_var_name);
      break;
    default:
      _warning ("ctalk: Unimplemented scope in new_object_reference: %d.",
	       scope);
      return NULL;
    }

  strcatx (expr_buf_out, "_makeRef (\"", objname, "\")", NULL);

  return expr_buf_out;
}

char *fmt_getRef (char *c_var_name, char *expr_out) {
  strcatx (expr_out, "_getRef (", c_var_name, ")", NULL);
  return expr_out;
}

char *fmt_getRefRef (char *c_var_name, char *expr_out) {
  strcatx (expr_out, "_getRefRef (", c_var_name, ")", NULL);
  return expr_out;
}

bool insert_object_ref (MESSAGE_STACK messages, int cvar_idx) {
  char expr_buf_out[MAXMSG];
  int i, stack_start_idx, stack_top_idx, addr_of_idx,
    open_paren_idx, close_paren_idx, next_tok_idx;

  if ((next_tok_idx = nextlangmsg (messages, cvar_idx)) != ERROR) {
    if (IS_C_ASSIGNMENT_OP (M_TOK(messages[next_tok_idx]))) {
      /* Then we have a re-assignment of the alias to something
	 else - delete the reference record. */
      delete_local_c_alias_reference (M_NAME(messages[cvar_idx]));
      delete_global_c_alias_reference (M_NAME(messages[cvar_idx]));
      return false;
    }
  }

  if (have_ref (M_NAME(messages[cvar_idx]))) {
    stack_start_idx = stack_start (messages);
    if ((addr_of_idx = ref_is_objref (messages, cvar_idx, stack_start_idx,
				      &open_paren_idx)) != -1) {
      /*
       *  An OBJREF normally expands to
       *
       *   &(<cvar_name>)
       *
       *  So we have to do our token munging stuff to
       *  find the close paren at the end of the complete
       *  expression , and insert the _getRefRef call just
       *  at the <cvar_name> token.  This also handles
       *  plain ampersands; i.e., 
       *
       *   &<cvar_name>
       */
      if (open_paren_idx != -1) {
	stack_top_idx = get_stack_top (messages);
	close_paren_idx = match_paren (messages, open_paren_idx, 
				       stack_top_idx);
	for (i = open_paren_idx; i >= close_paren_idx; i--) {
	  ++messages[i] -> evaled;
	  ++messages[i] -> output;
	}
      }
      fmt_getRefRef (M_NAME(messages[cvar_idx]), expr_buf_out);
      fileout (expr_buf_out, 0, cvar_idx);
      ++messages[addr_of_idx] -> evaled;
      ++messages[addr_of_idx] -> output;
      ++messages[cvar_idx] -> evaled;
      ++messages[cvar_idx] -> output;
    } else {
      fmt_getRef (M_NAME(messages[cvar_idx]), expr_buf_out);
      fileout (expr_buf_out, 0, cvar_idx);
      ++messages[cvar_idx] -> evaled;
      ++messages[cvar_idx] -> output;
      return true;
    }
  }
  return false;
}

/* 
   Only for '=' op, not '+=', '-=', etc. 
   Returns the stack index of the lvalue token in lvalue_idx if valid.
*/
bool is_rvalue_of_OBJECT_ptr_lvalue (MESSAGE_STACK messages,
				     int rexpr_start_idx,
				     int *lvalue_idx,
				     int *cvar_scope) {
  int prev_idx, prev_idx_2;
  CVAR *c;
  if ((prev_idx = prevlangmsg (messages, rexpr_start_idx)) != ERROR) {
    if (M_TOK(messages[prev_idx]) == EQ) {
      if ((prev_idx_2 = prevlangmsg (messages, prev_idx)) != ERROR) {
	if (((c = get_local_var (M_NAME(messages[prev_idx_2]))) != NULL) ||
	    ((c = get_global_var (M_NAME(messages[prev_idx_2]))) != NULL)) {
	  if ((c -> type_attrs & CVAR_TYPE_OBJECT) && (c -> n_derefs > 0)) {
	    *lvalue_idx = prev_idx_2;
	    *cvar_scope = c -> scope;
	    return true;
	  }
	}
      }
    }
  }
  *lvalue_idx = *cvar_scope = ERROR;
  return false;
}
