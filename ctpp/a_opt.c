/* $Id: a_opt.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ctpp.h"

/*
 *  -A command line option(s).
 */

extern int unassert_opt;            /* -A-.  Declared in rtinfo.c.   */

extern DEFINITION *macro_assertions; /* Declared in assert.c.        */
extern DEFINITION *last_assertion;

DEFINITION *macro_symbols;         /* Defined macro list.              */
DEFINITION *last_symbol;           /* List pointer.                    */

int assert_opt (char **args, int opt_idx, int n_args) {

  int lookahead_idx;
  enum {
    a_opt_null,
    a_opt_predicate,
    a_opt_value
  } state;
  char *paren_ptr,
    predicate[MAXLABEL],
    value[MAXLABEL];
  DEFINITION *d;

  for (lookahead_idx = opt_idx, state = a_opt_null; lookahead_idx < n_args;) {

    switch (state)
      {
      case a_opt_null:
	if (!strcmp (args[lookahead_idx], "-A")) {
	  /*
	   *  Form -A <predicate...>
	   */
	  strcpy (predicate, args[++lookahead_idx]);
	} else {
	  /*
	   *  Form -A<predicate...>
	   */
	  strcpy (predicate, &args[lookahead_idx][2]);
	}
	state = a_opt_predicate;
	break;
      case a_opt_predicate:
	/* 
	 *   Form predicate(value)
	 */
	if ((paren_ptr = index (predicate, '(')) != NULL) {
	  if (*(paren_ptr + 1) == '\0') {
	    /*
	     *  Form predicate( value ).
	     */
	    strcpy (value, args[++lookahead_idx]);
	  } else {
	    strcpy (value, paren_ptr + 1);
	  }
	  *paren_ptr = '\0';
	} else {
	  /*
	   *  Form -A-
	   */
	  if (!strcmp (predicate, "-")) {
	    unassert_opt = TRUE;
	    return lookahead_idx - opt_idx;
	  } else {
	    /*
	     *  No value in the predicate string. 
	     *  Look ahead to the next argument.
	     */
	    /*
	     *   Form -A -
	     */
	    if (!strcmp (args[lookahead_idx + 1], "-")) {
	      unassert_opt = TRUE;
	      ++lookahead_idx;
	      return lookahead_idx - opt_idx;
	    } else {
	      /*
	       *   Form predicate (...
	       */
	      if (index (args[lookahead_idx + 1], '(')) {
		/*
		 *  Form predicate ( value...
		 */
		if (!strcmp (args[lookahead_idx + 1], "(")) {
		  lookahead_idx += 2;
		  strcpy (value, args[lookahead_idx]);
		} else {
		  /*
		   *  Form predicate (value...
		   */
		  strcpy (value, &args[++lookahead_idx][1]);
		}
	      } else {
		/*
		 *  No value.  The value defaults to, "1."
		 */
		strcpy (value, "1");
		goto done;
	      }
	    }
	  }
	}
	state = a_opt_value;
	break;
      case a_opt_value:
	/*
	 *  The, "value" buffer contains the value, starting after
	 *  the opening parenthesis.
	 */
	/*
	 *  Form value).
	 */
	if ((paren_ptr = index (value, ')')) != NULL) {
	  *paren_ptr = '\0';
	  goto done;
	} else {
	  /*
	   *  Form value ).  Look ahead for closing parenthesis.
	   */
	  if (!strcmp (args[lookahead_idx + 1], ")")) {
	    ++lookahead_idx;
	    goto done;
	  } else {
	    /*
	     *  Closing parenthesis not found. Issue a 
	     *  warning and continue.
	     */
	    _warning ("-A: Argument syntax error.\n");
	    goto done;
	  }
	}
	break; /* Not reached. */
      }

  }

 done:

  /*
   *  Instead of retokenizing, add the assertion manually.
   */

  if ((d = 
       (DEFINITION *)calloc (1, sizeof (struct _macro_symbol)))
      == NULL)
    _error (_("assert_opt: %s.\n"), strerror (errno));

  d -> sig = MACRODEF_SIG;
  strcpy (d -> name, predicate);
  strcpy (d -> value, value);

  if (!macro_assertions) {
    last_assertion = macro_assertions = d;
  } else {
    last_assertion -> next = d;
    d -> prev = last_assertion;
    last_assertion = d;
  }

  return lookahead_idx - opt_idx;
}

static void free_macro_args (MACRO_ARG **args) {
  int i;
  for (i = 0; args[i]; i++)
    free (args[i]);
}

void handle_unassert (void) {

  DEFINITION *d, *d_prev;

  if ((d = last_assertion) != NULL) {
    while (d -> prev) {
      d_prev = d -> prev;
      free_macro_args (d_prev -> next -> m_args);
      free (d_prev -> next);
      d = d_prev;
    }
    free_macro_args (d -> m_args);
    free (d);
    macro_assertions = last_assertion = NULL;
  }

  if ((d = last_symbol) != NULL) {
    while (d -> prev) {
      d_prev = d -> prev;
      free_macro_args (d_prev -> next -> m_args);
      free (d_prev -> next);
      d = d_prev;
    }
    free_macro_args (d -> m_args);
    free (d);
    macro_symbols = last_symbol = NULL;
  }
}
