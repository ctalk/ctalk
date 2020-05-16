/* $Id: d_opt.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

#include <stdio.h>
#include <string.h>
#include "ctpp.h"

/*
 *  Handle the -D command line option.
 */

int define_opt (char **args, int opt_idx, int n_args) {

  int lookahead_idx; 
  enum {
    d_opt_null,
    d_opt_symbol,
    d_opt_assign,
    d_opt_value
  } state;
  char *v_ptr,
    symbol[MAXLABEL],
    value[MAXLABEL],
    exprbuf[MAXLABEL * 2 + 9]; /* + strlen ("#define ") */

  for (lookahead_idx = opt_idx, state = d_opt_null; lookahead_idx < n_args;) {

    switch (state)
      {
      case d_opt_null:
	if (!strcmp (args[lookahead_idx], "-D")) {
	  /*
	   *  Form -D <symbol>
	   */
	  strcpy (symbol, args[++lookahead_idx]);
	} else {
	  /*
	   *  Form -D<symbol>
	   */
	  strcpy (symbol, &args[lookahead_idx][2]);
	}
	state = d_opt_symbol;
	break;
      case d_opt_symbol:
	/*
	 *  Look for an assignment either in the same argument
	 *  as, "symbol," or in the next argument.  If an 
	 *  assignment doesn't exist, set the value to, "1,"
	 *  and finish parsing.
	 */
	if ((v_ptr = index (symbol, '=')) != NULL) {
	  if (!*(v_ptr + 1)) {
	    /*
	     *  Check for a case where we have:
	     *  -D symbol= value
	     */
	    *v_ptr = 0;
	    ++lookahead_idx;
	    /*
	     *  We can skip the d_opt_assign state here.
	     */
	    state = d_opt_value;
	  } else {
	    /*
	     *  -D symbol=value 
	     *    or
	     *  -Dsymbol=value
	     */
	    strcpy (value, v_ptr + 1);
	    *v_ptr = 0;
	    goto done;
	  }
	} else {
	  if (index (args[lookahead_idx+1], '=')) {
	    ++lookahead_idx;
	    state = d_opt_assign;
	  } else {
	    /*
	     *  No assignment.
	     */
	    strcpy (value, "1");
	    goto done;
	  }
	}
	break;
      case d_opt_assign:
	if (strcmp (args[lookahead_idx], "=")) {
	  /*
	   *  Handle cases like:
	   *   -D sym =val
	   *     or
	   *   -Dsym =val
	   */
	  v_ptr = index (args[lookahead_idx], '=');
	  strcpy (value, v_ptr + 1);
	  goto done;
	} else {
	  /*
	   *   -D sym = val
	   *     or
	   *   -Dsym = val
	   */
	  ++lookahead_idx;
	}
	state = d_opt_value;
	break;
      case d_opt_value:
	strcpy (value, args[lookahead_idx]);
	goto done;
	break;
      }

  }

 done:
  sprintf (exprbuf, "#define %s %s\n", symbol, value);
  tokenize_define (exprbuf);
  return lookahead_idx - opt_idx;
}

char defines_fname[FILENAME_MAX];

int set_defines_file_name (char **a, int idx, int cnt) {
  if (*a[idx + 1] == '-') help ();

  strcpy (defines_fname, a[++idx]);

  return 1;
}

