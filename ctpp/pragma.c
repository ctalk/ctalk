/* $Id: pragma.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2018 Robert Kiesling, rk3314042@gmail.com.
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
 *  Functions to handle GCC and STDC pragmas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "ctpp.h"

extern EXCEPTION parse_exception; /* Declared in pexcept.c.            */

extern MESSAGE *t_messages[P_MESSAGES +1];  /* Declared in preprocess.c */
extern int t_message_ptr;

/* 
 *    The GCC and cpp info files document the following pragmas:
 *      #pragma interface ...       # C++
 *      #pragma implementation ...
 *
 *    C++ system headers cause an exception and print a warning with
 *    the -v command line switch, and the macro processor breaks off 
 *    parsing.
 *    
 *    The following GNU cpp pragmas are recognized.
 *      #pragma GCC dependency
 *      #pragma GCC poison <identifier>...
 *      
 *    These pragmas are ignored.
 *      #pragma GCC pack()
 *      #pragma GCC system header
 *
 *    The C99 pragmas FP_CONTRACT, FENV_ACCESS, and 
 *      CX_LIMITED_RANGE all apply to floating point 
 *      operations, and ctalk ignores them.
 *
 *    TO DO - 
 *      Determine if we need a parser state (in cparse.c) for
 *      'extern "C" { }' C++ declarations.
 *      Recognize the _Pragma operator.
 */

int handle_pragma_directive (MESSAGE_STACK messages, int op_ptr) {

  int arg_ptr;
  int stack_end;

  stack_end = get_stack_top (messages);

  for (arg_ptr = op_ptr - 1; arg_ptr > stack_end; arg_ptr--)
    if (messages[arg_ptr] -> tokentype == LABEL)
      break;

  if (!strcmp (messages[arg_ptr] -> name, "GCC"))
    handle_gcc_pragma (messages, arg_ptr);
  
  if (!strcmp (messages[arg_ptr] -> name, "STDC"))
    handle_stdc_pragma (messages, arg_ptr);
  
  if ((!strcmp (messages[arg_ptr] -> name, "interface")) ||
      (!strcmp (messages[arg_ptr] -> name, "implementation"))) {
/*     if (verbose_opt) { */
      warning (messages[op_ptr], "Attempt to include a C++ system header.");
      location_trace (messages[op_ptr]);
/*     } */
    parse_exception = cplusplus_header_x;
    return ERROR;
  }

  return SUCCESS;
}

/* Ignore the C99 pragmas, which apply to floating point operations.
   #pragma STDC FP_CONTRACT ON|OFF|DEFAULT
   #pragma STDC FENV_ACCESS ON|OFF|DEFAULT
   #pragma STDC CX_LIMITED_RANGE ON|OFF|DEFAULT
*/

int handle_stdc_pragma (MESSAGE_STACK messages, int msg_ptr) {
  return SUCCESS;
}

/* 
 *  Needs to be called from handle_pragma_directive.
 *  Msg_ptr points to the "GCC" label after the #pragma
 *  directive.
 *
 *  The function performs the following pragmas.  
 *
 *    #pragma GCC dependency
 *    #pragma GCC poison <identifier>...
 */

extern int include_ptr;
extern INCLUDE *includes[MAXARGS + 1];
extern char source_file[FILENAME_MAX];

char poison_identifiers[MAXARGS][MAXLABEL];
int n_poison_identifiers;

int handle_gcc_pragma (MESSAGE_STACK messages, int msg_ptr) {

  int arg_ptr;
  int stack_end;

  stack_end = get_stack_top (messages);

  for (arg_ptr = msg_ptr - 1; arg_ptr > stack_end; arg_ptr--)
    if (messages[arg_ptr] -> tokentype == LABEL)
      break;

  /*
   *  Ignore the, "pack," and, "system header," pragmas.
   *  FIXME!  Check for the "header" token in "system header"
   *   argument.
   */

  if ((!strcmp (messages[arg_ptr] -> name, "pack")) ||
      (!strcmp (messages[arg_ptr] -> name, "system")))
    return SUCCESS;

  if (!strcmp (messages[arg_ptr] -> name, "dependency")) {
    struct stat src_stat_buf, arg_stat_buf;
    char sourcefile[FILENAME_MAX], argfile[FILENAME_MAX];
    int j, r_stat;
    
    for (j = arg_ptr - 1; j > stack_end; j--) {
      if ((messages[j] -> tokentype == LABEL) ||
	  (messages[j] -> tokentype == LITERAL))
	break;
      if (messages[j] -> tokentype == NEWLINE) {
	error (messages[arg_ptr], "Missing argument.");
	parse_exception = parse_error_x;
	return ERROR;
      }
    }

    if (messages[j] -> tokentype == LITERAL)
      (void)substrcpy (argfile, messages[j] -> name, 1, 
		 strlen (messages[j] -> name) - 2);
    else
      strcpy (argfile, messages[j] -> name);
    
    if ((r_stat = stat (argfile, &arg_stat_buf)) == ERROR) 
      error (messages[arg_ptr], "Could not stat %s.", argfile);

    if (include_ptr > MAXARGS)
      strcpy (sourcefile, source_file);
    else 
      strcpy (sourcefile, includes[include_ptr] -> path);

    if ((r_stat = stat (sourcefile, &src_stat_buf)) == ERROR) 
      error (messages[arg_ptr], "Could not stat %s.", sourcefile);

    if (src_stat_buf.st_mtime > arg_stat_buf.st_mtime)
      warning (messages[arg_ptr], "Source file is more recent than %s.",
	       argfile);
  }

  if (!strcmp (messages[arg_ptr] -> name, "poison")) {
    int j;
    int start_ident;

    for (j = arg_ptr - 1, start_ident = n_poison_identifiers; 
	 j > stack_end; j--) {
      if (messages[j] -> tokentype == LABEL) {
	if (messages[j] -> tokentype == LABEL)
	  strcpy (poison_identifiers[n_poison_identifiers++], 
		  messages[j] -> name);
      }

      if ((messages[j] -> tokentype == NEWLINE) &&
	  (start_ident == n_poison_identifiers)) {
	error (messages[arg_ptr], "Missing argument.");
	parse_exception = parse_error_x;
	return ERROR;
      }

      if (messages[j] -> tokentype == NEWLINE)
	break;
    }
  }

  return SUCCESS;
}

/*
 *  Handle a pragma operator of the form:
 *
 *   _Pragma ("<pragma-arguments>")
 *
 *   The contents of the argument can be one of the pragmas described 
 *   above.
 */

int handle_pragma_op (MESSAGE_STACK messages, int op_ptr) {

  int i, arg;
  int stack_end;
  int start_token, end_token;
  char argbuf[MAXMSG], stmtbuf[MAXMSG + 8]; /* + strlen ("#pragma ") */

  stack_end = get_stack_top (messages);

    for (arg = op_ptr - 1; arg > stack_end; arg--) {
      if (messages[arg] -> tokentype == LITERAL)
	break;

      if (messages[arg] -> tokentype == NEWLINE) {
	error (messages[op_ptr], "Missing argument.");
	parse_exception = parse_error_x;
      }
    }

    substrcpy (argbuf, messages[arg]->name, 1, 
	       strlen (messages[arg] -> name) - 2);

    /* A bit kludgy, but we don't have to change handle_pragma_directive. */
    sprintf (stmtbuf, "%s %s", "#pragma", argbuf);

    start_token = P_MESSAGES;
    end_token = tokenize_reuse (t_message_push, stmtbuf);

    handle_pragma_directive (t_messages, start_token);
    
    for (i = end_token; i <= start_token; i++)
      reuse_message (t_message_pop ());

    return SUCCESS;
}

void init_poison_identifiers (void) { n_poison_identifiers = 0; }
