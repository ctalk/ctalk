/* $Id: fninit.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2016 Robert Kiesling, rk3314042@gmail.com.
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
#include "object.h"
#include "message.h"
#include "ctalk.h"

extern I_PASS interpreter_pass;

#define MAX_INITS 8192

static int n_global_inits = 0;
static char *global_init[MAX_INITS];
static int n_deferred_global_init1 = 0;   /* Used now for instance variables*/
static char *deferred_global_init1[MAX_INITS];
static int n_deferred_global_init2 = 0;   /* For class variables.           */
static char *deferred_global_init2[MAX_INITS];
static int n_deferred_global_init3 = 0;   /* New global objects.            */
static char *deferred_global_init3[MAX_INITS];

extern char fn_name[MAXLABEL];         /* Declared in fnbuf.c.     */
extern int fn_defined_by_header;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern LIST *cvar_tab_members,
  *cvar_tab_members_head;
extern LIST *cvar_tab_init,
  *cvar_tab_init_head;
/*
 *  Generate an init block with any init statements collected
 *  by the parser.
 *  TO DO - Make sure this is called only when needed, so 
 *  the function doesn't have to rely on p -> init being
 *  NULL.
 */

void generate_init (int main_decl) {

  char buf[MAXMSG];
  PARSER *p;
  LIST *l, *l2;

  if (fn_defined_by_header)
    return;

  p = CURRENT_PARSER;

  fileout ("\n     {\n", 0, 0);

  strcatx (buf, "     ", SRC_FILE_SAVE_FN, 
	   " (\"", __source_filename (),
	   "\");\n", NULL);
  fileout (buf, 0, 0);

  if (main_decl)
    generate_ctalk_init_call ();

  strcatx (buf, "     __ctalk_initFn (\"", fn_name,
	   "\");\n", NULL);
  fileout (buf, 0, 0);

  if (p -> init) {
    for (l = p -> init; l; l = l -> next) {
      fileout ("     ", 0, 0);
      fileout ((char *)l -> data, 0, 0);
    }

    l = p-> init_head;
    do {
      l2 = l -> prev;
      __xfree (MEMADDR(l -> data));
      __xfree (MEMADDR(l));
      if (!l2)
	break;
      l = l2;
    } while (l != p -> init);
    if (IS_LIST(l)) {
      if (*(char *)l -> data) {
	__xfree (MEMADDR(l -> data));
      }
      __xfree (MEMADDR(l));
    }
    p -> init = p -> init_head = NULL;
  }

  if (IS_LIST(cvar_tab_init) && IS_LIST(cvar_tab_init_head)) {
    for (l = cvar_tab_init; l; l = l -> next) {
      fileout ("     ", 0, 0);
      fileout ((char *)l -> data, 0, 0);
    }

    l = cvar_tab_init_head;
    do {
      l2 = l -> prev;
      __xfree (MEMADDR(l -> data));
      __xfree (MEMADDR(l));
      if (!l2)
	break;
      l = l2;
    } while (l != cvar_tab_init);
    if (cvar_tab_init_head != cvar_tab_init) {
      __xfree (MEMADDR(cvar_tab_init -> data));
      __xfree (MEMADDR(cvar_tab_init));
    }
    cvar_tab_init_head = cvar_tab_init = NULL;
  }

  fileout ("     }\n\n", 0, 0);
}

/*
 *   Add statements to a function init.  When parsing class
 *   and superclass libraries, add the init statements to 
 *   the parser that is parsing the source module.  That is
 *   the only way at the moment to ensure that the run-time code 
 *   initializes superclasses before the method is needed.
 */

void fn_init (char *s, int parent_init) {

  PARSER *p;
  LIST *n;

  if (fn_defined_by_header)
    return;

  p = CURRENT_PARSER;

  if (parent_init) {
    int i;
    if ((i = parser_ptr ()) >= MAXARGS) 
      p = CURRENT_PARSER;
    else
      for (i = parser_ptr (); i <= MAXARGS; i++)
	if (parser_at (i) -> pass == parsing_pass)
	  p = parser_at (i);
  }

  n = new_list ();
  n -> data = (void *)strdup (s);

  if (p -> init) {
    p -> init_head -> next = n;
    n -> prev = p -> init_head;
    p -> init_head = n;
  } else {
    p -> init = p -> init_head = n;
  }
}

void global_init_statement (char *buf, int ndefer) {
  switch (ndefer)
    {
    case 3:
      deferred_global_init3[n_deferred_global_init3++] = strdup (buf);
      break;
    case 2:
      deferred_global_init2[n_deferred_global_init2++] = strdup (buf);
      break;
    case 1:
      deferred_global_init1[n_deferred_global_init1++] = strdup (buf);
      break;
    case 0:
      global_init[n_global_inits++] = strdup (buf);
      break;
    }
  if ((n_global_inits > MAX_INITS) ||
      (n_deferred_global_init1 > MAX_INITS) ||
      (n_deferred_global_init2 > MAX_INITS) ||
      (n_deferred_global_init3 > MAX_INITS))
    _error ("global_init_statement: MAX_INITS exceeded.\n");
}

void output_global_init (void) {

  int i;

  __fileout ("\n\n\n");
  __fileout ("void __ctalk_init (char *argvName) {\n");

  __fileout ("__argvName (argvName);\n");
  __fileout ("__rt_init_library_paths ();\n");
  __fileout ("__ctalk_class_initialize ();\n");

  for (i = 0; i < n_global_inits; i++) {
    __fileout (global_init[i]);
    __xfree (MEMADDR(global_init[i]));
  }
  for (i = 0; i < n_deferred_global_init1; i++) {
    __fileout (deferred_global_init1[i]);
    __xfree (MEMADDR(deferred_global_init1[i]));
  }
  for (i = 0; i < n_deferred_global_init2; i++) {
    __fileout (deferred_global_init2[i]);
    __xfree (MEMADDR(deferred_global_init2[i]));
  }
  for (i = 0; i < n_deferred_global_init3; i++) {
    __fileout (deferred_global_init3[i]);
    __xfree (MEMADDR(deferred_global_init3[i]));
  }
  n_global_inits = n_deferred_global_init1 = 
    n_deferred_global_init2 = n_deferred_global_init3 = 0;
  __fileout ("}\n\n");
}

/*
 *  Fn_return () is only needed at the moment for parsing functions.
 *  Methods have the local vars stored in them, and they get 
 *  re-initialized on every method call by __ctalk_initLocalObjects ()
 *  in rt_methd.c.
 */

int fn_return (MESSAGE_STACK messages, int msg_ptr) {

  int i,
    stack_end;
  PARSER *p;
  OBJECT *var;
  CVAR *cvar;
  MESSAGE *m;

  if (strcmp (messages[msg_ptr] -> name, "return"))
    return ERROR;

  stack_end = get_stack_top (messages);

  switch (interpreter_pass)
    {
    case parsing_pass:
      /*
       *  Check whether the return statement of a function contains a
       *  local object or C variable, and issue a warning if it does.
       */
      p = CURRENT_PARSER;
      for (i = msg_ptr; i > stack_end; i--) {

	if ((messages[i] -> tokentype == WHITESPACE) ||
	    (messages[i] -> tokentype == NEWLINE))
	  continue;

	m = messages[i];

	switch (m -> tokentype)
	  {
	  case LABEL:
	    if (p -> vars) {
	      for (var = p -> vars; var; var = var -> next) {
		if (!strcmp (m -> name, var -> __o_name)) 
		  warning (m, "Local object %s in function return.",
			   var -> __o_name);
	      }
	    }
	    if (p -> cvars) {
	      for (cvar = p -> cvars; cvar; cvar = cvar -> next) {
		if (!strcmp (m -> name, cvar -> name) &&
		    (cvar -> scope == LOCAL_VAR))
		  warning (m, "Local variable %s in function return.",
			   cvar -> name);
	      }
	    }
	    break;
	  case SEMICOLON:
	    goto local_check_done;
	    break;
	  default:
	    break;
	  }
      }
      break;
    default:
      break;
    }

 local_check_done:

  return SUCCESS;
}

/*
 *  Called by parser_pass at the closing brace of the 
 *  function, so fileout () immediately before the
 *  closing brace, which should be messages[ptr].
 */

void generate_fn_return (MESSAGE_STACK messages, int ptr, int app_exit) {

  char buf[MAXMSG];
  
  if (fn_defined_by_header) 
    return;

  sprintf (buf, "\n{ __ctalk_exitFn (%d); }\n", app_exit);
  fileout (buf, 0, ptr);

}
