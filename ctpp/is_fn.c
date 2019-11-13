/* $Id: is_fn.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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

/*
 *  Adapted from cparse.c in main ctalk source code.
 */

#include <string.h>
#include "ctpp.h"
#include "typeof.h"

/*
 *  TO DO - Replace with CFUNC struct and CVAR attributes.  See
 *  cvar.h.
 */
int return_is_unsigned;        /* Return value is unsigned or a pointer. */
int return_is_ptr;
int return_is_ptr_ptr;
int decl_is_ptr;
int decl_is_ptr_ptr;
int decl_is_prototype;

#define MAX_DECLARATORS 255

char declarators[MAX_DECLARATORS][MAXLABEL]; /* Stack of declarator labels. */
int n_declarators;

int is_c_function_prototype_declaration (MESSAGE_STACK messages, int msg_ptr) {

  int i,
    lookahead,
    stack_end,
    return_val;
  bool param_start,
    param_end;
  MESSAGE *m;

  stack_end = get_stack_top (messages);

  param_start = param_end = False;

  for (i = msg_ptr, n_declarators = 0, 
	 return_is_ptr = return_is_ptr_ptr = return_val = FALSE; 
       i > stack_end; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    m = messages[i];

    switch (m -> tokentype)
      {
      case LABEL:
      case CTYPE:
	if (param_start != True)
	  strcpy (declarators[n_declarators++], m -> name);
	break;
      case OPENPAREN:
	param_start = True;
	break;
      case CLOSEPAREN:
	param_end = True;
	break;
      case ARRAYOPEN:
      case ARRAYCLOSE:
	return_val = FALSE;
	goto done;
	break;
      case SEMICOLON:
	if (param_end == True)
	  return_val = TRUE;
	goto done;
	break;
      case ASTERISK:
	if (param_start == False) {
	  for (lookahead = i-1; lookahead > stack_end; lookahead--) {
	    if ((messages[lookahead]->tokentype == NEWLINE) ||
		(messages[lookahead]->tokentype == WHITESPACE))
	      continue;
	    else
	      break;
	  }
	  if (messages[lookahead] -> tokentype == ASTERISK) {
	    return_is_ptr_ptr = TRUE;
	    i = lookahead;
	  } else {
	    return_is_ptr = TRUE;
	  }
	}
	break;
      case OPENBLOCK:
	if (param_end == True)
	  return_val = FALSE;
	goto done;
	break;
      default:
	break;
      }
  }

 done:

  return return_val;
}

/*
 *  Check if a declaration is a function.  Also, handle function
 *  function pointers.  The function does not check for C data type 
 *  declarations because a function can return an object, and the 
 *  program hasn't yet defined those.
 *
 *  A function consists of a unique tag before the opening parenthesis of a 
 *  parameter list, and the closing parenthesis of the parameter list followed 
 *  by the opening brace of the function body.  These criteria should
 *  also work for method declarations.
 *
 *  The function, function_param_states (), handles the parameter list.
 *  The return value is a data type, and this function contains
 *  a simplified version of is_c_var_declaration*.
 *
 *  TO DO - Test this more thoroughly.
 *  TO DO - Find out how far into the declaration the calling function 
 *  needs to look ahead after a successful match.  For example, 
 *  "int main (int argc, char **argv) {" is a successful declaration, 
 *  but so is "main (int argc, char **argv) {" so the calling function
 *  needs to look ahead at least past the function tag.  That means 
 *  we couldn't do macro processing or object lookup between the
 *  function tag and the start of the parameter list.
 *
 *  FIXME!  - The function returns False for declarations that have
 *  compiler-specific or non-standard extensions, like GNU C's 
 *  __THROW after the parameter list, but before the opening brace.
 */

int func_declaration_states[] = {
  LABEL,  LABEL,            /* 0. Return val declaration w/o lookahed
				    for a C type. */
  LABEL,  ASTERISK,         /* 1. Return val pointer declaration. */
  LABEL,  OPENPAREN,        /* 2. Start of param list. */
  OPENPAREN, ASTERISK,      /* 3. The declaration is a function ptr. */
  OPENPAREN, LABEL,         /* 4. Another decl type for func ptrs.   */
  LABEL, CLOSEPAREN,        /* 5.  ... both followed by this. */
  CLOSEPAREN, OPENPAREN,    /* 6. Start of param list of ptr decl. */
  CLOSEPAREN, SEMICOLON,    /* 7. End of prototype decl. */ 
  CLOSEPAREN, OPENBLOCK,    /* 8. End of actual decl.  */
  ASTERISK, LABEL,          /* 9. */
  OPENPAREN, CLOSEPAREN,    /* 10. */
  OPENPAREN, ELLIPSIS,      /* 11. */
  ELLIPSIS, CLOSEPAREN,     /* 12. */
  LABEL, ARGSEPARATOR,      /* 13. */
  ARGSEPARATOR, LABEL,      /* 14. */
  ASTERISK, ASTERISK,       /* 15. */
  -1, 0, 0
};

#define FUNCTION_DECL_STATE_COLS 2

#define FUNCTION_DECL_STATE(m,x) (check_state ((x), (m), \
              func_declaration_states, FUNCTION_DECL_STATE_COLS))

int is_c_func_declaration_msg (MESSAGE_STACK messages, int msg_ptr) {

  int i,
    state,
    last_state,
    return_val,
    stack_end,
    n_parens;
  bool param_start,   /* Make sure we have at least seen the start */
    param_end,           /* and end of the param list, so a calling   */
                         /* function can still iterate through        */
                         /* without worrying about a successful       */
                         /* spurious match in the middle of the       */
                         /* declaration.  param_end also implies that */
                         /* the token following is the opening brace  */
                         /* of the function body.                     */
    tag_is_unique;       /* The label preceding the param list is     */
                         /* unique.                                   */
  MESSAGE *m;

  /*
   *  Make sure the message isn't a control structure keyword, 
   *  which are handled in control.c.
   */
  if ((!strcmp (messages[msg_ptr] -> name, "case")) ||
      (!strcmp (messages[msg_ptr] -> name, "do")) ||
      (!strcmp (messages[msg_ptr] -> name, "for")) ||
      (!strcmp (messages[msg_ptr] -> name, "if")) ||
      (!strcmp (messages[msg_ptr] -> name, "switch")) ||
      (!strcmp (messages[msg_ptr] -> name, "while")))
    return FALSE;

  stack_end = get_stack_top (messages);

  param_start = param_end = tag_is_unique = False;

  for (i = msg_ptr, n_declarators = 0, last_state = -1,
	 n_parens = 0,
	 return_val = return_is_unsigned = return_is_ptr = 
	 return_is_ptr_ptr = FALSE; 
       i > stack_end;
       i--) {

    m = messages[i];

    if (M_ISSPACE (m)) continue;

    if ((state = FUNCTION_DECL_STATE (messages, i)) == ERROR)
      return False;

    switch (m -> tokentype) 
      {
      case LABEL:
	if (!param_start) {
	  switch (state)
	    {
	    case 0:
	    case 1:
	    case 2:
	      strcpy (declarators[n_declarators++], m -> name);
	      break;
	    case 5:
	      if ((last_state == 3) || (last_state == 4)) {
		decl_is_ptr = TRUE;
		strcpy (declarators[n_declarators++], m -> name);
	      }
	      break;
	    }
	}
	break;
      case OPENPAREN:
	param_start = True;
	switch (state)
	  {
	  case 3:  /* First arg starts with a pointer. */
	  case 4:  /* First arg is a label.            */
	  case 10: /* No args.                         */
	    /* 
	     * Is the label preceding the openparen a function tag -
	     * Probably we should look at not calling get_symbol, in
	     * case the function tag is supposed to be a macro. 
	     */
	    if (!is_c_data_type (declarators[n_declarators-1]) ||
		!get_symbol (declarators[n_declarators-1], TRUE)) {
	      tag_is_unique = True;
	    }
	    break;
	  }
	++n_parens;
	break;
      case ASTERISK:
	switch (last_state)
	  {
	  case 1:
	    if (state == 15) {
	      return_is_ptr_ptr = TRUE;
	    } else {
	      return_is_ptr = TRUE;
	    }
	    break;
	  case 3:
	    if (state == 15) {
	      decl_is_ptr_ptr = TRUE;
	    } else {
	      decl_is_ptr = TRUE;
	    }
	    break;
	  case 15:
	    break;
	  }
	break;
      case CLOSEPAREN:
	if (state == 8) {
	  param_end = True;
	}
	--n_parens;
	goto done;
	break;
      case ARGSEPARATOR:
	break;
      default:
	break;
      }

    last_state = state;
  }

 done:

  if ((param_start && param_end && tag_is_unique) &&
      (n_parens == 0)) {
    return_val = TRUE;
  }

  return return_val;

}

