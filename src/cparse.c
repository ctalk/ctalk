/* $Id: cparse.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2019 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "cvar.h"
#include "parser.h"

extern int error_line, error_column;  /* Declared in parser.c.             */
extern int warn_extension_opt;        /* Declared in main.c.               */
extern int verbose_opt;
extern I_PASS interpreter_pass;
extern bool unbuffer_fn_call;        /* Declared in fnbuf.c */
int main_declaration = FALSE;         /* Used by the parser to determine
					 when to output the initialization
					 code. */
bool constant_expr = False;        /* True if syntax requires a 
					 constant expression.              */
bool typedef_check = False;        /* True if parsing a typedef.        */
extern bool fn_is_builtin;
MESSAGE *c_messages[N_MESSAGES + 1];  /* C language message stack and      */
int c_message_ptr = N_MESSAGES;       /* stack pointer.  Also used by      */
                                      /* method_param () in primitives.c,  */
                                      /* and in control.c.                 */
int preamble = TRUE;                  /* True until the first function 
					 declaration in the module is
					 reached. */
extern EXCEPTION parse_exception;     /* Declared in pexcept.c.            */

extern int format_method_output;      /* Declared in methodbuf.c.          */

DECLARATION_CONTEXT declaration_context = decl_null_context;

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c.      */
extern int new_method_ptr;

extern bool ctrlblk_pred,               /* Declared in control.c.           */
  ctrlblk_blk,
  ctrlblk_else_blk;

extern OBJECT *rcvr_class_obj;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern ARGBLK *argblks[MAXARGS + 2]; /* Declared in argblk.c.             */
extern int argblk_ptr;

int get_c_message_ptr (void) {
  return c_message_ptr;
}

MESSAGE_STACK c_message_stack (void) {
  return c_messages;
}

MESSAGE *c_message_at (int idx) {
  return c_messages[idx];
}

int c_message_push (MESSAGE *m) {
  if (c_message_ptr == 0) {
    warning (m, "c_message_push: stack overflow.");
    return ERROR;
  }
  c_messages[c_message_ptr--] = m;
  return c_message_ptr;
}

/*
 *  Whether the function is inline or not depends on the specific 
 *  compiler; the inline attribute might cause a warning.
 */
static inline int cparse_check_state (int stack_idx,
				      int last_idx,
				      MESSAGE **messages, 
				      int *states_ptr) { 
  int i;
  int next_idx = stack_idx;

  for (i = 0; ; i++) {
  nextrans:
    if (states_ptr[i+i] == ERROR)
      return ERROR;
    if (states_ptr[i+i] == messages[stack_idx] -> tokentype) {
      for (next_idx = stack_idx - 1; next_idx > last_idx; next_idx--) {
	while (M_ISSPACE(messages[next_idx])) {
	  next_idx--;
	  if (!messages[next_idx]) return ERROR;
	}
	if (states_ptr[i+i+1] != messages[next_idx]->tokentype) {
	  ++i;
	  goto nextrans;
	} else {
	  return i;
	}
      }
    }
  }
  return ERROR;
}
      
/*
 *   Return true if a block closure is a function or method closure.
 *   The function checks for a scope change from the frame before
 *   or the frame after the closing brace.
 *
 *   If there are no statements between the end of a function and the 
 *   declaration of the next function, the scope can change from LOCAL_VAR 
 *   to PARAM_VAR.
 */
int fn_closure (MESSAGE_STACK messages, int msg_ptr, int frame) {

  int last_frame;

  if ((messages == message_stack ()) &&
      ARGBLK_TOK(msg_ptr))
    return FALSE;

  /* Make sure that there are frames. */
  if ((interpreter_pass != parsing_pass) &&
      (interpreter_pass != library_pass) && 
      (interpreter_pass != method_pass))
    _error ("fn_closure called without frames.");

  if (messages[msg_ptr] -> tokentype != CLOSEBLOCK)
    _error ("Not a close block character in fn_closure ().\n");

  if ((last_frame = get_frame_pointer ()) >= MAXFRAMES)
    return FALSE;

  if (frame == last_frame) {  
                              /* If the closure is the last frame. */
                              /* We should also have checked for a */
                              /* syntax error.                     */
    if (frame_at (frame + 1) -> scope == LOCAL_VAR)
      return TRUE;
  } else {                    
    if (((frame_at (frame + 1) -> scope == LOCAL_VAR) && 
	 (frame_at (frame)     -> scope != LOCAL_VAR)) ||
	((frame_at (frame)     -> scope == LOCAL_VAR) &&
	 (frame_at (frame - 1) -> scope != LOCAL_VAR)))
      return TRUE;
  }

  return FALSE;
}

/*
 *   Determines whether a declaration is a function prototype.
 */

/* 
 *  Also used by is_c_fn_declaration_msg.
 *  TO DO - The function should also parse the arguments.
 */

int return_is_unsigned;         /* Return value is unsigned or a pointer. */
int return_is_ptr;
int return_is_ptr_ptr;
int decl_is_ptr;
int decl_is_ptr_ptr;
int decl_is_prototype;

FN_DECLARATOR declarators[MAXARGS];
int n_declarators;

static inline int add_fn_declarator (char *s) {
  if (is_ctalk_keyword (s)) return ERROR;
  declarators[n_declarators].attr = is_c_data_type_attr (s);
  declarators[n_declarators].name = s;
  if (n_declarators < MAXARGS) { 
    n_declarators++;
  } else {
    _warning ("add_fn_declarator: %s: too many declarators.\n", s);
    return ERROR;
  }
  return SUCCESS;
}

int is_c_function_prototype_declaration (MESSAGE_STACK messages, int msg_ptr) {

  int i,
    lookahead,
    stack_end,
    return_val;
  bool param_start,
    param_end;
  MESSAGE *m;
  /*
   *  If the scope isn't global, then return False.
   *  If parsing a library between methods, then the
   *  scope is also global.
   */
  if ((interpreter_pass == parsing_pass) ||
      (interpreter_pass == library_pass) ||
      (interpreter_pass == method_pass))
    if ((frame_at (CURRENT_PARSER -> frame)) -> scope != GLOBAL_VAR)
      return FALSE;

  stack_end = get_stack_top (messages);

  param_start = param_end = False;

  for (i = msg_ptr, n_declarators = 0, 
	 return_is_ptr = return_is_ptr_ptr = return_val = FALSE; 
       i > stack_end; i--) {

    m = messages[i];

    if ((m -> tokentype == WHITESPACE) || (m -> tokentype == NEWLINE))
      continue;

    switch (m -> tokentype)
      {
      case LABEL:
      case CTYPE:
	if (param_start != True)
	  if (add_fn_declarator (m -> name) == ERROR) return FALSE;
	break;
      case OPENPAREN:
	if (!param_start) {
	  /*
	   *  A * immediately following the opening parenthesis
	   *  should be enough to identify a function pointer. 
	   */
	  int _lookahead_1;
	  if ((_lookahead_1 = nextlangmsg (messages,  i)) != ERROR) {
	    if (M_TOK(messages[_lookahead_1]) != ASTERISK)
	      param_start = True;
	  }
	}
	break;
      case CLOSEPAREN:
	param_end = True;
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
      case EQ:  /* Break off if we see an initializer. */
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
 *  Called with a message pointer in the function declaration, before
 *  the opening brace.  Returns the parser message stack index of the
 *  closing brace.
 */
int find_function_close (MESSAGE_STACK messages, int msg_ptr,
			 int last_msg_ptr) {

  int ptr;
  int block_level = 0;
  int in_func = FALSE;
  
  for (ptr = msg_ptr; ptr > last_msg_ptr; ptr--) {
    switch (M_TOK(messages[ptr]))
      {
      case OPENBLOCK:
	++block_level;
	if (!in_func)
	  in_func = TRUE;
	break;
      case CLOSEBLOCK:
	--block_level;
	if (in_func && !block_level)
	  return ptr;
	break;
      case SEMICOLON:
	if (!block_level)
	  return ptr;
	break;
      case LABEL:
	if (interpreter_pass == var_pass) {
	  /* parse_vars skips over these tokens, so
	     set the attributes */
	  if (str_eq (M_NAME(messages[ptr]), "self")) {
	    messages[ptr] -> attrs |= TOK_SELF;
	  } else if (str_eq (M_NAME(messages[ptr]), "super")) {
	    messages[ptr] -> attrs |= TOK_SUPER;
	  }
	}
	break;
      }
  }
  return ERROR;
}

/*
 *    A variable declaration ends at the first semicolon outside curly
 *    braces (for global vars like structs).  Returns the stack index
 *    of the semicolon that ends the declaration.
 *
 *    While parsing a C function argument, if the index reaches
 *    the end of the stack, then back up the pointer by one and
 *    return that.
 */

int find_declaration_end (MESSAGE_STACK messages, int msg_ptr, int end) {

  int i;
  int blocklevel,
    n_parens;
  bool have_function;

  for (i = msg_ptr, blocklevel = 0, n_parens = 0, 
	 have_function = False; 
       i > end; i--) {

    if (!messages[i] || !IS_MESSAGE (messages[i])) {
      ++i;
      goto done;
    }

    switch (messages[i] -> tokentype)
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	if (--n_parens == 0) {
	  int m_next_idx;
	  if ((m_next_idx = nextlangmsg (messages, i)) != ERROR)
	    if (M_TOK(messages[m_next_idx]) == OPENBLOCK)
	      have_function = True;
	}
	break;
      case OPENBLOCK:
	++blocklevel;
	break;
      case CLOSEBLOCK:
	--blocklevel;
	if (blocklevel == 0 && have_function)
	  goto done;
	break;
      case SEMICOLON:
	if (!blocklevel)
	  goto done;
	break;
      default:
	break;
      }
  }
  
  if ((i == end) &&
      (interpreter_pass == c_fn_pass)) {
    while (messages[i] == NULL)
      ++i;
  }

 done:
  return i;
}

/*
 *  Called with msg_ptr pointing at the first token of an initializer.
 *
 *  Generates an error if we encounter a Ctalk keyword anywhere before
 *  a semicolon, curly braces, or a function declaration.
 *
 *  Also works for class variables.
 * 
 */
int find_instancevar_declaration_end (MESSAGE_STACK messages, int msg_ptr) {

  int i;
  int n_parens;
  int end;

  end = get_stack_top (messages);

  for (i = msg_ptr, n_parens = 0; i > end; i--) {

    if (!messages[i] || !IS_MESSAGE (messages[i])) {
      ++i;
      goto done;
    }

    switch (messages[i] -> tokentype)
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	if (--n_parens == 0) {
	  int m_next_idx;
	  if ((m_next_idx = nextlangmsg (messages, i)) != ERROR)
	    if (M_TOK(messages[m_next_idx]) == OPENBLOCK)
	      error (messages[msg_ptr], "Instance or class variable "
		     "declaration syntax error.");
	}
	break;
      case SEMICOLON:
	goto done;
	break;
      case OPENBLOCK:
      case CLOSEBLOCK:
	error (messages[msg_ptr], "Instance or class variable declaration "
	       "syntax error.");
	break;
      case LABEL:
	if (is_ctalk_keyword (M_NAME(messages[i]))) {
	  error (messages[msg_ptr], "Instance or class variable declaration "
		 "syntax error.");
	}
	break;
      default:
	break;
      }
  }
  
  if ((i == end) &&
      (interpreter_pass == c_fn_pass)) {
    while (messages[i] == NULL)
      ++i;
  }

 done:
  return i;
}

/*
 *  Like find_declaration_end, but check for the end of a subscripted
 *  declaration.  That means look for commas outside of anything in
 *  a var declaration, or an initializer.  If we try to do this in 
 *  find_declaration_end, we start to get confused with method arguments, 
 *  and other elements.
 */
int find_subscript_declaration_end (MESSAGE_STACK messages, int msg_ptr) {

  int i, 
    end,
    subscriptlevel, 
    block_level;
  MESSAGE *m;

  end = get_stack_top (messages);

  for (i = msg_ptr, subscriptlevel = 0, block_level = 0; i > end; i--) {

    m = messages[i];
    if (M_ISSPACE(m)) continue;

    switch (M_TOK(m))
      {
      case ARRAYOPEN:
	++subscriptlevel;
	break;
      case ARRAYCLOSE:
	--subscriptlevel;
	break;
      case OPENBLOCK:
	++block_level;
	break;
      case CLOSEBLOCK:
	--block_level;
	break;
      case ARGSEPARATOR:
      case SEMICOLON:
	if (!subscriptlevel && !block_level)
	  goto done;
	break;
      }
  }

 done:
  return i;
}

/*
 *  Like find_declaration_end, but scans for the next ARGSEPARATOR
 *  or CLOSEPAREN token.
 */
int find_arg_end (MESSAGE_STACK messages, int i) {
  int comma_l,
    paren_l;
  comma_l = scanforward (messages, i, get_stack_top (messages), ARGSEPARATOR);
  paren_l = scanforward (messages, i, get_stack_top (messages), CLOSEPAREN);

  if (comma_l == ERROR)
    return paren_l;
  if (paren_l == ERROR)
    return comma_l;
  return (comma_l > paren_l) ? comma_l : paren_l;
}

/* Determines if a string contains a C function declaration. 
   Call with the complete function declaration.  The function 
   can also distinguish between function declarations
   and method declarations.

   For input that has already been tokenized, use 
   is_c_function_declaration_msg ().

   Returns TRUE or FALSE.

   For example: 

   int main (int argc, char **argv) {
   }

   FILE *f openfile (char *path) {
   }

   int get_number (void) {
   }

*/

/*
 *  Return true if the token is part of a function 
 *  declaration.
 *
 *  This function is used by parse () to set frames.
 *
 *  NOTE - The function returns True even for functions with
 *  an implied return type, like "main () {}," which means
 *  that it can return True from any label in the function
 *  declaration.  When called from parse (), the declaration 
 *  should only be registered as True at the first token, in 
 *  order to avoid framing errors.
 *
 *  The variable in_ctrlblk_pred allows us to handle cases
 *  of functions within control blocks, which are often 
 *  followed by opening braces.
 *  
 *  Note that this function is only safe when calling with
 *  start_ptr starting on a label.
 *
 *  Aaand... also note that setting a frame to a method declaration
 *  isn't actually needed, because the frames that the front
 *  end actually needs occur at the statements within the method,
 *  and the method declaration (with instanceMethod or classMethod 
 *  keywords) gets replaced by run-time code anyway.
 */

static char fn_decl_buf[MAXMSG * 2];

bool is_c_function_declaration (char *start_ptr) {

  char *open_block_ptr;     /* Start of function body.              */
  char *main_start_ptr, *main_end_ptr;
  char *param_start_ptr, *param_end_ptr;
  char *prev_label_ptr_start, *prev_label_ptr_end;
  char prev_label_buf[MAXLABEL];
  int n_parens;

  /* Sanity check */
  open_block_ptr = start_ptr;
  while (*open_block_ptr) {
    switch (*open_block_ptr)
      {
      case ';':
      case '}':
	return false;
	break;
      case '{':
	goto c_fn_found_block;
	break;
      }
    ++open_block_ptr;
  }
  if (*open_block_ptr == 0)
    return false;

 c_fn_found_block:

  strncpy (fn_decl_buf, start_ptr, 
	   (size_t)(open_block_ptr - start_ptr + 1));
  fn_decl_buf[open_block_ptr - start_ptr + 1] = 0;

  /*
   *  Return if there are characters (not tokens) that don't occur 
   *  in function or method declarations.
   */
  if (strpbrk (fn_decl_buf, "\'\"}=><%&|^;"))
    return false;

  param_end_ptr = open_block_ptr - 1;
  while (param_end_ptr > start_ptr) {
    if (isspace ((int) *param_end_ptr)) {
      --param_end_ptr;
    } else if (*param_end_ptr != ')') {
      return FALSE;
    } else {
      break;
    }
  }
  if (param_end_ptr <= start_ptr)
    return false;
  
  n_parens = 0;
  param_start_ptr = param_end_ptr;
  while (param_start_ptr > start_ptr) {
    switch (*param_start_ptr)
      {
      case ')':
	++n_parens;
	break;
      case '(':
	--n_parens;
	if (n_parens == 0)
	  goto have_start_paren;
	break;
      }
    --param_start_ptr;
  }
  if (param_start_ptr <= start_ptr)
    return false;

 have_start_paren:
  prev_label_ptr_end = param_start_ptr - 1;
  while (isspace((int)*prev_label_ptr_end))
    --prev_label_ptr_end;
  ++prev_label_ptr_end;

  prev_label_ptr_start = prev_label_ptr_end - 1;
  while (!isspace ((int)*prev_label_ptr_start))
    --prev_label_ptr_start;
  ++prev_label_ptr_start;
  
  memset (prev_label_buf, 0, MAXLABEL);
  strncpy (prev_label_buf, prev_label_ptr_start,
	   prev_label_ptr_end - prev_label_ptr_start);

  if (is_ctrl_keyword (prev_label_buf))
    return false;
  
  if ((main_start_ptr = strstr (fn_decl_buf, "main")) != NULL) {
    main_end_ptr = main_start_ptr + 4;
    if (isspace ((int)*(main_start_ptr - 1))) {
      if (isspace ((int) *main_end_ptr)) {
	main_declaration = TRUE;
      }
    }
  }

  return true;

}

/* 
 *     A valid label that is not an object or class reference.
 */
int is_c_identifier (char *s) {

  OBJECT *o;
  int i;

  for (i = 0; i < (int)strlen (s); i++) {
    if (!isalnum ((int)s[i]) && ((int)s[i] != '_'))
      return FALSE;
  }

  if ((o = get_object (s, NULL)) != NULL)
    return FALSE;

  if ((o = get_class_object (s)) != NULL)
    return FALSE;

  return TRUE;

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
#include "fndeclstate.h"
  -1,  0
};

#define FUNCTION_DECL_STATE(m,l,x) (cparse_check_state ((x), (l), (m),	\
               func_declaration_states))

int is_c_fn_declaration_msg (MESSAGE_STACK messages, int msg_ptr,
			     int stack_end) {

  int i,
    state,
    last_state,
    return_val,
    param_start_ptr,
    param_end_ptr,
    n_parens;
  bool param_start,   /* Make sure we have at least seen the start */
    param_end,           /* and end of the param list, so a calling   */
                         /* function can still iterate through        */
                         /* without worrying about a successful       */
                         /* spurious match in the middle of the       */
                         /* declaration.  param_end also implies that */
                         /* the token following is the opening brace  */
                         /* of the function body. If it isn't, and    */
                         /* the scope is global, then the function    */
                         /* changes the scope to PROTOTYPE_VAR, and   */
                         /* a calling function can treat the          */
                         /* declaration as a function prototype.      */
    tag_is_unique;       /* The label preceding the param list is     */
                         /* unique.                                   */
  MESSAGE *m;

  param_start = param_end = tag_is_unique = False;

  for (i = msg_ptr, n_declarators = 0, last_state = -1,
	 param_start_ptr = param_end_ptr = -1, n_parens = 0,
	 return_val = return_is_unsigned = return_is_ptr = 
	 return_is_ptr_ptr = FALSE; 
       i > stack_end;
       i--) {

    m = messages[i];

    if (M_ISSPACE (m)) continue;

    if ((state = FUNCTION_DECL_STATE (messages, stack_end, i)) == ERROR)
      return False;

    switch (m -> tokentype) 
      {
      case LABEL:
#ifdef __GNUC__
 	if (str_eq (M_NAME(m), "__attribute__")) {
 	  int _attr_next_idx;
 	  for (_attr_next_idx = i - 1; _attr_next_idx > stack_end;
 	       _attr_next_idx--)
 	    if (!M_ISSPACE(messages[_attr_next_idx]))
 	      break;
 	  if ((i = match_paren (messages, _attr_next_idx, stack_end))
 	      == ERROR)
 	    _error ("Parser error.\n");
 	  continue;
 	}
#endif
	if (!param_start) {
	  if (is_ctrl_keyword (M_NAME(m)) || IS_PRIMITIVE_METHOD(m))
	    return False;
	  switch (state)
	    {
	    case 2:
	    case 3:
	    case 4:
	      if (add_fn_declarator (m -> name) == ERROR) return FALSE;
	      break;
	    case 7:
	      if ((last_state == 5) || (last_state == 6)) {
		decl_is_ptr = TRUE;
		if (add_fn_declarator (m -> name) == ERROR) return FALSE;
	      }
	      if ((last_state == 11) || (last_state == -1)) {  
		if (!param_start) {  /* Msg_ptr starts in the middle */
		  return FALSE;      /* of the declaration.          */
		} else {
		  decl_is_ptr = TRUE;
		  if (add_fn_declarator (m -> name) == ERROR) return FALSE;
		}
	      }
	      break;
	    }
        } else {
          /* i.e.; a function call as an argument */
          if (state == 4)
            return FALSE;
        }
	break;
      case OPENPAREN:
	if (!n_parens) {
	  if (param_start == False)
	    param_start_ptr = i;
	  param_start = True;
	  switch (state)
	    {
	    case 5:  /* First arg starts with a pointer. */
	    case 6:  /* First arg is a label.            */
	    case 12: /* No args.                         */
	      if (n_declarators == 0)  /* Opening paren of PFI. */
		return FALSE;
	      if (!is_c_data_type (declarators[n_declarators-1].name) ||
		  !get_typedef (declarators[n_declarators-1].name)) {
		if (is_gnuc_builtin_type (declarators[n_declarators-1].name)){
		  fn_is_builtin = true;
		  return FALSE;
		}
		tag_is_unique = True;
	      }
	      break;
	    }
	}
	++n_parens;
	break;
      case ASTERISK:
	switch (last_state)
	  {
	  case 3:
	    if (!param_start) {
	      if (state == 15) {
		return_is_ptr_ptr = TRUE;
	      } else {
		return_is_ptr = TRUE;
	      }
	    }
	    break;
	  case 5:
	    if (!param_start) {
	      if (state == 15) {
		decl_is_ptr_ptr = TRUE;
	      } else {
		decl_is_ptr = TRUE;
	      }
	    }
	    break;
	  case 15:
	    break;
	  }
	break;
      case CLOSEPAREN:
	if (state == 10) {    /* ... ) { ... */
	  param_end = True;
	  param_end_ptr = i;
	  if (--n_parens == 0) {
	    if (param_start && param_end && tag_is_unique) {
	      if (!unbuffer_fn_call) {
		/* TODO - We probably don't need to get the
		   params when called from parser_pass, either,
		   we should be able to use the existing CFUNC. */
		/* ALSO - there are probably a lot of other things
		   in this fn that we can omit when just checking
		   for a fn declaration syntax. */
		fn_params (messages, param_start_ptr, param_end_ptr);
	      }
	      if (preamble) preamble = FALSE;
	      return TRUE;
	    }
	  }
	}
#ifdef __GNUC__
	else if (state == 24 && n_parens == 1 && 
		 interpreter_pass == var_pass) {  /* ... ) <label> ( ... */
	  /* "label" is a GNU attribute. We only do this on
	     var_pass to get the param definitions because adjusting
	     the scope on the later passes gets complex (at least adjusting
	     the scope while in this clause gets complicated). */
	  int lookahead, lookahead_2;
	  if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
	    if ((lookahead_2 = nextlangmsg (messages, lookahead)) != ERROR) {
	      if (M_TOK(messages[lookahead_2]) == OPENPAREN) {
		--n_parens;
		param_end = True;
		param_end_ptr = i;
		if (param_start && param_end && tag_is_unique) {
		  fn_params (messages, param_start_ptr, param_end_ptr);
		  if (preamble) preamble = FALSE;
		  return TRUE;
		}
	      }
	    }
	  }
	}
#endif
	if (state == 9) {    /* ... ) ; ... */
	  param_end = True;
	  param_end_ptr = i;
	  switch (interpreter_pass)
	    {
	      /*
	       *  TO DO - Check for function redefinitions.
	       */
	    case var_pass:
	      if (--n_parens == 0) {
		if (param_start && param_end && tag_is_unique) {
		  if (!get_function (declarators[n_declarators - 1].name)) {
		    fn_params (messages, param_start_ptr, param_end_ptr);
		    return TRUE;
		  } else {
		    return FALSE;
		  }
		}
	      }
	      break;
	    case parsing_pass:
	      if (HAVE_FRAME && (FRAME_SCOPE == GLOBAL_VAR)) {
		FRAME_SCOPE = PROTOTYPE_VAR;
	      }
	      if (--n_parens == 0) {
		if (param_start && param_end && tag_is_unique) {
		  fn_params (messages, param_start_ptr, param_end_ptr);
		  if (HAVE_FRAME && (FRAME_SCOPE == PROTOTYPE_VAR))
		    return TRUE;
		  else
		    return FALSE;
		}
	      }
	      break;
	    case method_pass:
	      return FALSE;
	      break;
	    default:
	      break;
	    }  /* switch (interpreter_pass) */
	} /* if (state == 9) { */    /* ... ) ; ... */
	if ((state == 8) ||  /* ... )( ... */
	    (state == 19) || /* ... ) , ... */
	    (state == 20)) { /* ... ) ) ... */
	  if (--n_parens == 0)
	    return FALSE;
	  else
	    goto fn_loop_done;  /* Don't decrement again at end of case. */
	}
#ifdef __GNUC__
	/*
	 *  Trailing __attribute__.
	 */
 	if (state == 24) { 
 	  if ((--n_parens == 0) && param_start) {  /* ) __attribute__ ... */
	    int _next_tok_idx;
	    for (_next_tok_idx = i - 1; _next_tok_idx > stack_end; _next_tok_idx--)
	      if (!M_ISSPACE(messages[_next_tok_idx]))
		break;
	    if (str_eq (M_NAME(messages[_next_tok_idx]), "__attribute__")) {
	      param_end = TRUE;
	      param_end_ptr = i;
	      if (param_start && param_end && tag_is_unique) {
		if (!get_function (declarators[n_declarators - 1].name)) {
		  fn_params (messages, param_start_ptr, param_end_ptr);
		  for (; _next_tok_idx > stack_end; --_next_tok_idx) {
		    if (M_TOK(messages[_next_tok_idx]) == SEMICOLON) {
		      decl_is_prototype = TRUE;
		      break;
		    } else if (M_TOK(messages[_next_tok_idx]) ==
			       OPENBLOCK) {
		      decl_is_prototype = FALSE;
		      break;
		    }
		  }
		  return TRUE;
		} else {
		  return FALSE;
		}
	      }
	    }
	  }
 	}
#endif
	if (--n_parens == 0)  /* Any other state. */
	  return FALSE;
	break;
      case ARGSEPARATOR:
	break;
      default:
	break;
      }

  fn_loop_done:
    last_state = state;
  }

  return return_val;

}

static PARAMCVAR params[MAXARGS];
static int param_ptr;
static int have_varargs;
static char  *param_types[MAXARGS];

/* These functions are called in cvars.c and elsewhere. */
int get_param_ptr (void) { return param_ptr; }
int get_vararg_status (void) { return have_varargs; }
PARAMCVAR get_param_n (int n) { return params[n]; }
bool param_0_is_void (void) {
  if (params[0].type != NULL) 
    return str_eq (params[0].type, "void");
  else
    return false;
}
char *default_param_type = "int";

static void param_types_to_param_cvar (int n_types) {
  switch (n_types)
    {
    case 0:
      params[param_ptr].type = default_param_type;
      break;
    case 1:
      params[param_ptr].type = param_types[0];
      break;
    case 2:
      params[param_ptr].qualifier = param_types[0];
      params[param_ptr].type = param_types[1];
      break;
    case 3:
      params[param_ptr].qualifier2 = param_types[0];
      params[param_ptr].qualifier = param_types[1];
      params[param_ptr].type = param_types[2];
      break;
    case 4:
      params[param_ptr].qualifier3 = param_types[0];
      params[param_ptr].qualifier2 = param_types[1];
      params[param_ptr].qualifier = param_types[2];
      params[param_ptr].type = param_types[3];
      break;
    case 5:
      params[param_ptr].qualifier4 = param_types[0];
      params[param_ptr].qualifier3 = param_types[1];
      params[param_ptr].qualifier2 = param_types[2];
      params[param_ptr].qualifier = param_types[3];
      params[param_ptr].type = param_types[4];
      break;
    }
}

/*
 *  Attributes and type attributes of the declaration.  See cvars.h. 
 */
int attrs;      /* To be deprecated-now in VARNAME separate for each tag.*/
int type_attrs;

/*
 *  Should be called with param_start_ptr and param_end_ptr pointing
 *  to the opening and closing parentheses of the parameter list.
 */

int fn_params (MESSAGE_STACK messages, int param_start_ptr, int param_end_ptr){

  int i,
    type_level = 0, n_blocks, long_chk, lookahead,
    n_parens = 1;
  bool struct_type_lookahead = false;

  param_ptr = 0;
  have_varargs = FALSE;
  memset (&params[param_ptr], 0, sizeof (PARAMCVAR));
  params[param_ptr].sig = CVAR_SIG;
  params[param_ptr].scope = ARG_VAR;
  type_level = 0;
  n_blocks = 0;
  type_attrs = 0;

  for (i = param_start_ptr - 1; i >= param_end_ptr; i--) {

    switch (messages[i] -> tokentype)
      {
      case LABEL:
	if (is_c_storage_class (messages[i] -> name)) {
	  params[param_ptr].storage_class = messages[i] -> name;
	  params[param_ptr].type_attrs |=
	    is_c_data_type_attr (messages[i] -> name);
	} else {
 	  if (is_c_data_type (messages[i] -> name)) {
	    param_types[type_level] = M_NAME(messages[i]);
	    /* If the type is "long", check for a previous
	       "long" type, so we can make the type "long long" if
	       needed. */
	    long_chk = is_c_data_type_attr (messages[i] -> name);
	    if (long_chk == CVAR_TYPE_LONG) {
	      if (params[param_ptr].type_attrs & CVAR_TYPE_LONG) {
		params[param_ptr].type_attrs &= ~CVAR_TYPE_LONG;
		params[param_ptr].type_attrs |= CVAR_TYPE_LONGLONG;
	      } else {
		params[param_ptr].type_attrs |= long_chk;
	      }
	    } else {
	      params[param_ptr].type_attrs |= long_chk;
	      if (long_chk == CVAR_TYPE_STRUCT) {
		params[param_ptr].attrs |= CVAR_ATTR_STRUCT;
		if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
		  if (M_TOK(messages[lookahead]) == LABEL) {
		    struct_type_lookahead = true;
		  } else {
		    struct_type_lookahead = false;
		  }
		}
	      }
	    }
	    ++type_level;
	  } else {
	    if (get_typedef (M_NAME(messages[i]))) {
	      param_types[type_level] = M_NAME(messages[i]);
	      params[param_ptr].attrs |= CVAR_ATTR_TYPEDEF;
	      ++type_level;
	    } else if (params[param_ptr].type_attrs & CVAR_TYPE_STRUCT) {
	      /* The attr is set by the token in the previous loop(s). */
	      if (struct_type_lookahead == true) {
		param_types[type_level] = M_NAME(messages[i]);
		++type_level;
		struct_type_lookahead = false;
	      } else {
		params[param_ptr].name = messages[i] -> name;
	      }
	    } else {
	      params[param_ptr].name = messages[i] -> name;
	    }
	  }
	}
	break;
      case ASTERISK:
	++params[param_ptr].n_derefs;
	break;
      case ARRAYOPEN:
	++n_blocks;
	break;
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (n_parens == 0) {
	  if (type_level > 0) {
	    param_types_to_param_cvar (type_level);
	    param_ptr++;
	  }
	  return SUCCESS;
	}
	break;
      case ARRAYCLOSE:
	--n_blocks;
	if (n_blocks == 0)
	  ++params[param_ptr].n_derefs;
	break;
      case ARGSEPARATOR:
	param_types_to_param_cvar (type_level);
	param_ptr++;
	memset (&params[param_ptr], 0, sizeof (PARAMCVAR));
	params[param_ptr].sig = CVAR_SIG;
	params[param_ptr].scope = ARG_VAR;
	type_level = 0;
	break;
      case ELLIPSIS:
	param_types[type_level++] = "OBJECT";
	params[param_ptr].name = M_NAME(messages[i]);
	params[param_ptr].n_derefs = 1;
	have_varargs = TRUE;
	break;
      }
    
  }

  return SUCCESS;
}

/* 
 *   Check for a C variable declaration.  Also distinguishes
 *   ctalk declarations.
 *   N.B. - The function returns True on the rightmost token
 *   that comprises a C declaration, so it will not catch 
 *   type qualifiers unless called with the leftmost statement
 *   token.
 *   TO DO - 
 *     1.  Recurse for struct and enum declarations.
 *     2.  Determine if a separate function is necessary for array 
 *         initializers.
 */

int var_declaration_states[] = {
  CTYPE,        LABEL,            /* 0. */
  CTYPE,        CTYPE,            /* 1. */
  CTYPE,        ASTERISK,         /* 2. */
  LABEL,        LABEL,            /* 3. */
  LABEL,        CHAR,             /* 4. */
  LABEL,        SEMICOLON,        /* 5. */
  ASTERISK,     LABEL,            /* 6. */
  EQ,           INTEGER,          /* 7.  E.g., int i = 1;        */
  EQ,           FLOAT,            /* 8.  E.g., double f = 1.0;   */
  EQ,           LITERAL,          /* 9.  E.g., char s[] = "a";   */
  EQ,           LITERAL_CHAR,     /* 10. E.g., char s='c';       */
  EQ,           OPENBLOCK,        /* 11. E.g. char s[] = {...}   */
  ARRAYOPEN,    ARRAYCLOSE,       /* 12. E.g., []                */
  ARRAYCLOSE,   EQ,               /* 13. E.g., [] =              */
  CTYPE,        OPENBLOCK,        /* 14. E.g., struct {...       */
  LABEL,        OPENBLOCK,        /* 15. E.g., struct <x> {...   */
  CLOSEBLOCK,   LABEL,            /* 16. E.g. } <struct_name>    */
  OPENBLOCK,    CTYPE,            /* 17. */
  INTEGER,      SEMICOLON,        /* 18. */
  FLOAT,        SEMICOLON,        /* 19. */
  LITERAL,      SEMICOLON,        /* 20. */
  LITERAL_CHAR, SEMICOLON,        /* 21. */
  OPENBLOCK,    LABEL,            /* 22.  E.g., { int, { enum_type... */
  SEMICOLON,    CLOSEBLOCK,       /* 23.  E.g., int i; } ...          */
  LABEL,        CLOSEBLOCK,       /* 24.  E.g. enum_type }            */
  CHAR,         SEMICOLON,        /* 25.  E.g., ];                    */
  OPENBLOCK,    CLOSEBLOCK,       /* 26.  E.g., struct s{};           */
  CLOSEBLOCK,   SEMICOLON,        /* 27. */
  CLOSEBLOCK,   LABEL,            /* 28. */
  LABEL,        EQ,               /* 29. Initializers. */
  EQ,           LABEL,            /* 30. */
  EQ,           LONG,             /* 31. */
  EQ,           LONGLONG,         /* 32. */
  ARRAYOPEN,    INTEGER,          /* 33. e.g., ary<[n>] */
  INTEGER,      ARRAYCLOSE,       /* 34. e.g., ary[<n]> */
  SEMICOLON,    LABEL,            /* 35. Necessary because stack frames    */
  SEMICOLON,    CTYPE,            /* 36. might not be present.             */
  LABEL,        ASTERISK,         /* 37. In case we missed a derived type. */
  CTYPE,        OPENPAREN,        /* 38. Function pointers -               */
  OPENPAREN,    ASTERISK,         /* 39. */
  LABEL,        CLOSEPAREN,       /* 40. */
  CLOSEPAREN,   OPENPAREN,        /* 41. */
  OPENPAREN,    CTYPE,            /* 42. If token is analyzed.          */
  OPENPAREN,    LABEL,            /* 43. As state 42, if not.           */
  LABEL,        ARGSEPARATOR,     /* 44. */
  ARRAYOPEN,    LABEL,            /* 45. E.g. [<label>]                 */
  CTYPE,        ARRAYCLOSE,       /* 46. E.g. [<ctype>]                 */
  ARGSEPARATOR, ASTERISK,         /* 47. Variable lists - *s1, *s2...   */
  ARGSEPARATOR, LABEL,            /* 48. c, d, e;                       */
  ARRAYCLOSE,   ARGSEPARATOR,     /* 49. c[], d[],...                   */
  ASTERISK,     ASTERISK,         /* 50. **  ........                   */
  OPENPAREN,    OPENPAREN,        /* 51. GNUC Attribute declarations.   */
  CLOSEPAREN,   CLOSEPAREN,       /* 52.  */
  LABEL,        ARRAYOPEN,        /* 53.  e.g., |arrayname[| ...        */
  ARRAYOPEN,    LABEL,            /* 54.  e.g.  arraname|[label|...     */
  LABEL,        COLON,            /* 55.  Bit fields...                 */
  COLON,        INTEGER,          /* 56. */
  INTEGER,      SEMICOLON,        /* 57. */
  CTYPE,        COLON,            /* 58. */
  CTYPE,        CLOSEPAREN,       /* 59. Complements 42, above.        */
  CLOSEPAREN,   SEMICOLON,        /* 60.  ...also used for func protos */
  INTEGER,      ARGSEPARATOR,     /* 61. For initialized enum members. */
  INTEGER,      CLOSEBLOCK,       /* 62. */
  CLOSEBLOCK,   ASTERISK,         /* 63.  ... <} *>... */
  ARRAYCLOSE,   SEMICOLON,        /* 64.  ... <] ;>... */
  SEMICOLON,    PREPROCESS,       /* 65. Break off*/
  ASTERISK,     OPENPAREN,        /* 66. e.g., OBJECT <*(> *cfunc)() */
  OPENPAREN,    CLOSEPAREN,       /* 67. e.g., OBJECT *(*cfunc)<()> */
  LABEL,        OPENPAREN,        /* 68. e.g., int <fn_name (>...) */
  CLOSEPAREN,   OPENBLOCK,        /* 69. (<args>) {<fn_body>...    */
  CLOSEPAREN,   ARGSEPARATOR,     /* 70. ) , ... */
  CTYPE,        ARGSEPARATOR,     /* 71. */
  ASTERISK,     CLOSEPAREN,       /* 72. */
  EQ,           OPENPAREN,        /* 73. E.g. char s = (...   */
  EQ,           EXCLAM,           /* 74. E.g., int i = !... */
  SEMICOLON,    INCREMENT,        /* 75. */
  SEMICOLON,    DECREMENT,        /* 76. */
  SEMICOLON,    OPENPAREN,        /* 77. */
  SEMICOLON,    CLOSEPAREN,       /* 78. */
  SEMICOLON,    ARRAYOPEN,        /* 79. */
  SEMICOLON,    ARRAYCLOSE,       /* 80. */
  SEMICOLON,    DEREF,            /* 81. */
  SEMICOLON,    SIZEOF,           /* 82. */
  SEMICOLON,    EXCLAM,           /* 83. */
  SEMICOLON,    PLUS,
  SEMICOLON,    MINUS,
  SEMICOLON,    ASTERISK,
  SEMICOLON,    AMPERSAND,
  SEMICOLON,    DIVIDE,
  SEMICOLON,    MODULUS,
  SEMICOLON,    ASL,
  SEMICOLON,    ASR,
  SEMICOLON,    LT,
  SEMICOLON,    LE,
  SEMICOLON,    GT,
  SEMICOLON,    GE,
  SEMICOLON,    BOOLEAN_EQ,
  SEMICOLON,    INEQUALITY,
  SEMICOLON,    BIT_AND,
  SEMICOLON,    BIT_OR,
  SEMICOLON,    BIT_XOR,
  SEMICOLON,    BIT_COMP,
  SEMICOLON,    BOOLEAN_AND,
  SEMICOLON,    BOOLEAN_OR,
  SEMICOLON,    CONDITIONAL,
  SEMICOLON,    COLON,
  SEMICOLON,    EQ,
  SEMICOLON,    ASR_ASSIGN,
  SEMICOLON,    ASL_ASSIGN,
  SEMICOLON,    PLUS_ASSIGN,
  SEMICOLON,    PLUS_ASSIGN,
  SEMICOLON,    MINUS_ASSIGN,
  SEMICOLON,    MULT_ASSIGN,
  SEMICOLON,    DIV_ASSIGN,
  SEMICOLON,    BIT_AND_ASSIGN,
  SEMICOLON,    BIT_OR_ASSIGN,
  SEMICOLON,    BIT_XOR_ASSIGN,
  SEMICOLON,    LITERALIZE,
  SEMICOLON,    MACRO_CONCAT,
  CTYPE,        SEMICOLON,
  ARGSEPARATOR, COLON,
  -1,   0
};

#define VAR_DECLARATION_STATE(m,l,x) (cparse_check_state ((x), (l), (m),	\
              var_declaration_states))

VARNAME vars[MAXARGS];
char *storage_class;
bool is_unsigned;
static bool is_struct_decl = False;
CVAR *inner_struct_decl;     /* If there's a tag after an inner */
                             /* struct or union, then we need to */
                             /* locate it to add the tag to the */
                             /* inner struct members.           */


/*
 *  Type declarations.  Storage class is saved by the
 *  storage_class char *, and sign is stored in is_unsigned.
 *
 *  C99 says only that there should be 12 allowed for 
 *  incomplete types. Use MAX_DECLARATORS = 12 for all data 
 *  types anyway.
 */
static char default_type_decl[] = "int";
char *type_decls[MAX_DECLARATORS+1];
int type_decls_ptr;

/*
 *  The global struct_members and struct_members_ptr are only
 *  used when returning from parsing from a struct, until we
 *  call parser_to_cvars () in cvars.c.  Otherwise, the struct
 *  members are stored within a struct_decl[] entry.
 */
CVAR *struct_members,            /* Also change in cvars.c when */
  *struct_members_ptr;           /* changing these definitions. */

STRUCT_DECL struct_decl[MAXARGS];
int struct_decl_ptr = 0;

#define C_STRUCT (struct_decl[struct_decl_ptr - 1])

/*
 *  Saved function declarations when parsing parameter lists.
 */
FN_DECL fn_decl[MAXARGS];
int fn_decl_ptr = 0;

#define C_FN (fn_decl[fn_decl_ptr - 1])

CVAR *fn_param_decls,
  *fn_param_decls_ptr;

int n_vars;

int get_cvar_attrs (void) { return attrs; }
int get_cvar_type_attrs (void) {return type_attrs; }

bool get_struct_decl (void) {return is_struct_decl; }

static void clear_variable (void) {
  memset ((void *)type_decls, 0, (MAX_DECLARATORS * sizeof (char *)));
  memset ((void *)vars, 0, MAXARGS * sizeof (VARNAME));
  is_unsigned = False;

  if (struct_decl_ptr == 0)
    reset_struct_decl ();

  /*
   *  The current struct_members and end pointer get reset
   *  when they are saved before recursing into an inner 
   *  struct, and at the beginning of finding the
   *  members of an outer struct.
   */
  return_is_unsigned = return_is_ptr = return_is_ptr_ptr = 
    decl_is_ptr = decl_is_ptr_ptr = decl_is_prototype = attrs = 
    type_attrs = FALSE;
}

int have_declaration (char *s) {
  CVAR *c;
  OBJECT *o;

  if (is_c_keyword (s))
    return FALSE;

  if (interpreter_pass != var_pass) {
    if (is_method_name (s) || is_method_selector (s))
      return TRUE;
  }
  /*
   *  Note: This only covers instance and class variable messages 
   *  of "self".  At the moment, that is all it should need to
   *  cover, as self expressions are handled by rt_self_expr ().
   */
  if (interpreter_pass == method_pass) {
    if (rcvr_class_obj) {
      OBJECT *__v;
      for (__v = rcvr_class_obj -> instancevars; __v; __v = __v -> next) {
	if (str_eq (__v -> __o_name, s))
	  return TRUE;
      }
      for (__v = rcvr_class_obj -> classvars; __v; __v = __v -> next) {
	if (str_eq (__v -> __o_name, s))
	  return TRUE;
      }
    }
  }

  if (str_eq (s, "self"))
    return TRUE;
  if (((c = get_instance_method_local_cvar (s)) != NULL) ||
      ((o = get_instance_method_local_object (s)) != NULL))
    return TRUE;

  if (((c = get_local_var (s)) != NULL) || global_var_is_declared (s)) {
  /*
   *  get_typedef insures that we don't run into a case where a 
   *  struct tag is the same as a typedef tag, in which case, 
   *  we don't have an already declared struct, just the typedef.
   */
    if (get_typedef (s)) {
      return FALSE;
    } else {
      return TRUE;
    } 
  } else if (get_function (s)) {
    return TRUE;
  }

  return FALSE;
}

/*
 *  If the label is not a type prefix, check back until
 *  we either find a data type label or an operator.
 */

int have_type_prefix (MESSAGE_STACK messages, int ptr) {

  int i,
    start_ptr;
  MESSAGE *m;

  if ((messages == message_stack ()) && HAVE_FRAMES) {
    start_ptr = THIS_FRAME -> message_frame_top;
  } else {
    start_ptr = stack_start (messages);
  }

  if (ptr == start_ptr) {
    m = messages[ptr];
    if ((m -> tokentype == SEMICOLON) ||
	(m -> tokentype == OPENPAREN) ||
	(m -> tokentype == ARRAYOPEN) ||
	(m -> tokentype == OPENBLOCK) ||
	(m -> tokentype == ARGSEPARATOR) ||
	(m -> tokentype == DEREF) ||
	(m -> tokentype == PERIOD))
      return FALSE;
    if (m -> tokentype == LABEL) {
      if (is_c_data_type (m -> name) ||
	  is_c_derived_type (m -> name))
	return TRUE;
    }
  }

  for (i = ptr + 1; i <= start_ptr; i++) {
    m = messages[i];
    if ((m -> tokentype == SEMICOLON) ||
	(m -> tokentype == OPENPAREN) ||
	(m -> tokentype == ARRAYOPEN) ||
	(m -> tokentype == OPENBLOCK) ||
	(m -> tokentype == ARGSEPARATOR) ||
	(m -> tokentype == DEREF) ||
	(m -> tokentype == PERIOD))
      return FALSE;
    if (m -> tokentype == LABEL) {
      if (is_c_data_type (m -> name) ||
	  is_c_derived_type (m -> name))
	return TRUE;
    }
  }

  return FALSE;
}

int have_ref_prefix (MESSAGE_STACK messages, int ptr) {
  int lookback;

  if (messages[ptr] -> tokentype != LABEL)
    return FALSE;

  /*
   *  Not having a previous token is not an 
   *  error when a token starts the frame.
   */
  if ((lookback = prevlangmsg (messages, ptr)) == ERROR)
    return FALSE;

  if (messages[lookback] -> tokentype == ASTERISK) {
    if ((lookback = prevlangmsg (messages, lookback)) == ERROR) {
#if 0
      /* This doesn't need to have a warning any more - but 
	 return false anyway because it's (should be) handled elsewhere. */
      warning (messages[ptr], "Unsupported (for now) expression syntax.");
#endif
      return FALSE;
    }
    if (messages[lookback] -> tokentype == LABEL) {
      if (!is_c_data_type (messages[lookback] -> name) &&
	  !get_typedef (messages[lookback] -> name)) {
	return TRUE;
      } else {
	return FALSE;
      }
    } else {
      return TRUE;
    }
  }

  /*
   *  An ampersand can also be an operator, but it is
   *  never used in variable declarations, so 
   *  the function does not need to check its
   *  context, and it can be treated simply
   *  as an address of operator here.
   */
  if ((messages[lookback] -> tokentype == AMPERSAND) ||
      (messages[lookback] -> tokentype == DEREF) ||
      (messages[lookback] -> tokentype == PERIOD))
    return TRUE;

  return FALSE;
}

/*
 * NOTE - The function allows for some laziness in data typing, so
 * that a derived type, if qualified, need not be defined - it is 
 * treated as a label and parsed by syntax.  This is so the interpreter 
 * can accomodate derived types that may not be defined on certain 
 * systems, for example:
 *
 *   typedef struct _pthread_descr_struct *_pthread_descr;
 * 
 * Where _pthread_descr_struct is not defined on some systems.
 */

bool is_c_var_declaration_msg (MESSAGE_STACK messages, int msg_ptr,
			       int end_ptr,
			       int ignore_oop_names) {
  int i, j;
  int state,
    last_state,
    last_state2;
  int r;
  int const_expr_end;          /* Pointer for constant expr substitution. */
  int n_subscripts;
  bool return_val = True;
  bool aggregate_type,      /* True if aggregate type.                 */
    aggregate_decl;            /* True if aggregate initializers.         */
  bool func_ptr_decl,       /* True if decl is a function pointer.     */
    fn_arg_list,             /* True if within function decl arg list.  */
    is_fn_decl;
  bool comment = False;
  bool have_data_type;
  bool pointer;
  int block_level,
    n_parens;
  MESSAGE *m;
  OBJECT *tmpobj;
  VAL macro_val;
  METHOD *_n_method;
  int _n_th_param;
  bool have_possible_global_var_method_dup_label = False;
  bool have_possible_ctalk_keyword = false;
  int global_var_dup_label_idx = -1;
  int struct_decl_attrs;
  int aggregate_class;
  CVAR *c;

  /*
   *  Check whether a label has already raised an exception.
   */
  if (__ctalkTrapExceptionInternal (messages[msg_ptr]))
    return False;

  if (messages[msg_ptr] -> evaled && messages[msg_ptr] -> output)
    return False;

  clear_variable ();
  struct_decl_attrs = 0;

  /*
   *  If the variable is already declared, make sure that its
   *  use is not also taken as a declaration.  A redeclaration
   *  needs an explicit type preceding the variable tag. So
   *  if there is a declaration like,
   *  
   *    OBJECT *val_obj;
   *
   *  then a statment like
   *
   *    val_obj = ...
   *
   *  is not mistaken for a declaration of type, "int."
   *
   *  The function issues the redeclaration warning below.
   */
  if (have_declaration (messages[msg_ptr] -> name)) {
    if (!have_type_prefix (messages, msg_ptr)) {
      return False;
    }
  }

  /*
   * Also exclude cases where the label is 
   * preceded by a ref operator, except 
   * where an asterisk prefix is preceded by
   * a type label.
   */
  if (have_ref_prefix (messages, msg_ptr))
    return False;

  if (is_instance_method_or_variable (messages, msg_ptr)) 
    return False;

  /*
   *  Declarations also don't start with pointer or 
   *  address of operators, or aggregate operators,
   *  or immediately follow them.  So take care of those 
   *  cases here.
   */
  if ((messages[msg_ptr] -> tokentype == ASTERISK) ||
      (messages[msg_ptr] -> tokentype == AMPERSAND) ||
      (messages[msg_ptr] -> tokentype == DEREF) ||
      (messages[msg_ptr] -> tokentype == PERIOD))
    return False;

  block_level = n_parens = 0;
  type_decls_ptr = n_vars = 0;
  aggregate_type = aggregate_decl = is_unsigned = pointer = 
    func_ptr_decl = is_fn_decl = fn_arg_list = have_data_type = False;
  storage_class = NULL;

  state = last_state = last_state2 = ERROR;
  
  for (i = msg_ptr; i >= end_ptr; i--) {

    m = messages[i];

    /* It should only be necessary to evaluate a variable
       declaration once. */

    if (!m || !IS_MESSAGE (m))
      break;

    if (M_ISSPACE (m)) continue;

    /* These are needed until the parser is able to remove 
       all comment tokens. */
    if (m -> tokentype == OPENCCOMMENT) {
      comment = True;
      continue;
    }
    if (comment && (m -> tokentype == CLOSECCOMMENT)) {
      comment = False;
      continue;
    }
    
    if (!ignore_oop_names && !typedef_check) {
      /*
       *  See if we can gather these under one 
       *  (M_TOK(m) == LABEL) test.
       */
      if (M_TOK(m) == LABEL) {
	if ((tmpobj = get_object (M_NAME(m), NULL)) != NULL)
	  return False;
	if ((tmpobj = get_class_object (M_NAME(m))) != NULL)
	  return False;
	if (is_pending_class (M_NAME(m)))
	  return False;
	if (str_eq (m -> name, "returnObjectClass") ||
	    str_eq (m -> name, "noMethodInit"))
	  return False;
	if (is_ctalk_keyword (m -> name)) {
	  have_possible_ctalk_keyword = true;
	  global_var_dup_label_idx = i;
	}

	if (is_method_name (M_NAME(m))) {
	  if (interpreter_pass == var_pass) {
	    /* this is possible... sometimes */
	    have_possible_global_var_method_dup_label = TRUE;
	    global_var_dup_label_idx = i;
	  }
	}
      }

      if (interpreter_pass == method_pass && M_TOK(m) == LABEL) {
	_n_method = new_methods[new_method_ptr+1] -> method;
	for (_n_th_param = 0; _n_th_param < _n_method -> n_params; 
	     _n_th_param++) {
	  if (str_eq (M_NAME(m), _n_method->params[_n_th_param]->name)) {
	    if (i < msg_ptr) {
	      /* An implicit declaration (i.e., the first token only) 
		 wouldn't occur here, so there's no need to issue
		 a warning - just return. */
	      warning (m, "C variable \"%s\" shadows a method parameter.",
		       M_NAME(m));
	    }
	    return False;
	  }
	}
      }
    }
    if (M_TOK(m) == LABEL) {
#ifdef __GNUC__
      if ((r = is_c_data_type_attr (m -> name)) != FALSE) {
	if ((r == CVAR_TYPE_LONG) && (type_attrs & CVAR_TYPE_LONG)) {
	  type_attrs |= CVAR_TYPE_LONGLONG;
	} else {
	  type_attrs |= r;
	}
	m -> tokentype = CTYPE;
      } else {
	if ((r = is_gnuc_builtin_type (m -> name)) == TRUE) {
	  m -> tokentype = CTYPE;
	} else if ((r = is_c_derived_type (m -> name)) == TRUE) {
	  m -> tokentype = CTYPE;
	  type_attrs |= CVAR_TYPE_TYPEDEF;
	} else {
	  if ((r = is_gnu_extension_keyword (m -> name)) == TRUE) {
	    if (warn_extension_opt) {
	      warning (m, "Compiler extension %s.", m -> name);
	    }
	    continue;
	  }
	}
      }
#else
      if ((r = is_c_data_type_attr (m -> name)) != FALSE) {
	if (r == CVAR_TYPE_LONG) {
	  if (type_attrs & CVAR_TYPE_INT) {
	    type_attrs &= ~CVAR_TYPE_INT;
	    type_attrs |= CVAR_TYPE_LONG;
	  } else if (type_attrs & CVAR_TYPE_LONG) {
	    type_attrs &= ~CVAR_TYPE_LONG;
	    type_attrs |= CVAR_TYPE_LONGLONG;
	  } else if (type_attrs & CVAR_TYPE_DOUBLE) {
	    type_attrs &= ~CVAR_TYPE_DOUBLE;
	    type_attrs |= CVAR_TYPE_LONGDOUBLE;
	  } else if (type_attrs & CVAR_TYPE_FLOAT) {
	    type_attrs &= ~CVAR_TYPE_FLOAT;
	    type_attrs |= CVAR_TYPE_DOUBLE;
	  } else { 
	    type_attrs |= r;
	  }
	} else {
	  type_attrs |= r;
	}
	m -> tokentype = CTYPE;
      } else {
	if ((r = is_c_derived_type (m -> name)) == TRUE) {
	  m -> tokentype = CTYPE;
	  type_attrs |= CVAR_TYPE_TYPEDEF;
	}
      }
#endif
    } /* if (M_TOK(m) == LABEL) */
    /* TO DO - Determine if tracking through the branches
       here, even if they are not legal statements, 
       can be recovered for later. */
    if ((state = VAR_DECLARATION_STATE (messages, end_ptr, i)) == ERROR) {
      /*
       *  Return an error if not at the end of the stack; i.e.,
       *  when not parsing parameter declarations.
       */
      if (format_method_output)
	goto done;
      if (nextlangmsgstack (messages, i) != ERROR) return False;
    }

    /*
     *  Skip initializers, but check for objects, and print 
     *  a warning message.
     */
    if (aggregate_decl == False) {
      int __i_end, i_2, i_prev;
      switch (state)
	{
	case 7:	case 8:	case 9:	case 10: case 11: case 30:
	case 32: case 73: case 74:
	  /* First check for a function block tmp expression, where
	     objects are okay. */
	  i_prev = prevlangmsg (messages, i);
	  if (!strncmp (M_NAME(messages[i_prev]), TMP_FN_BLK_PFX,
			TMP_FN_BLK_PFX_LENGTH))
	    return false;
	  __i_end = initializer_end (messages, i);
	  if (!ignore_oop_names) {
	    for (i_2 = msg_ptr; messages[i_2] && (i_2 >= __i_end); --i_2) {
	      if (M_TOK(messages[i_2]) == LABEL) {
		if (is_ctalk_keyword (M_NAME(messages[i_2]))) {
		  if (!(messages[i_2] -> attrs & TOK_SELF) &&
		      !(messages[i_2] -> attrs & TOK_SUPER)) {
		    return False;
		  }
		}
		if (messages[i_2] -> attrs & TOK_SELF ||
		    is_method_parameter (messages, i_2)) {
		  if (interpreter_pass == method_pass) {
		    object_in_initializer_warning 
		      (messages, i_2,
		       new_methods[new_method_ptr+1] -> method,
		       rcvr_class_obj);
		  } else {
		    object_in_initializer_warning (messages, i_2, NULL, NULL);
		  }
		  return False;
		}
	      }
	    }
	  } /* if (!ignore_oop_names) */
	  i = __i_end;
	  continue;
	} /* switch (state) */
    } /* if (aggregate_decl == False) */

    /* Don't analyze keywords like, "return," etc. */
    if (M_TOK(m) == LABEL) {
      if (is_c_c_keyword (M_NAME(m)))
	return False;
    }

    switch (m -> tokentype)
      {
      case CTYPE:
	switch (state)
	  {
	  case 0:  /* 
		    *  C types except for sign labels and storage
		    *  classes go in the type_decls[] array.
		    */
	    /* signed|unsigned -- see IS_SIGN_LABEL2 in cvar.h. */
	    if (IS_SIGN_LABEL2 (m -> name) == 2) {
	      is_unsigned = True;
	      type_attrs |= CVAR_TYPE_UNSIGNED;
	    } else if ((storage_class = 
			is_c_storage_class (m -> name)) == NULL) {
	      /* const|extern|inline|register|static|volatile */
	      if ((aggregate_class = IS_AGGREGATE_TYPE (m -> name)) > 0) {
	      /* enum|struct|union -- see the #define of  IS_AGGREGATE_TYPE
		 in cvar.h. */
		aggregate_type = True;
		/* Enums get handled separately. */
		if (aggregate_class == 1 || aggregate_class == 3) {
		  attrs |= CVAR_ATTR_STRUCT;
		  struct_decl_attrs |= CVAR_ATTR_STRUCT;
		}
	      }
	    }
	    type_decls[type_decls_ptr++] = m ->name;

	    break;
	  case 2:   /* <type> * */
	    type_decls[type_decls_ptr++] = m ->name;
	    if ((c = get_typedef (M_NAME(m))) != NULL) {
	      if (c -> attrs & CVAR_ATTR_TYPEDEF && 
		  str_eq (c -> type, "struct")) {
		aggregate_type = True;
 		attrs |= CVAR_ATTR_STRUCT_PTR;
	      }
	    }
	    break;
	  case 14:      /* <struct|enum|union {> */
	    type_decls[type_decls_ptr++] = m ->name;
	    aggregate_type = aggregate_decl = True;
	    break;
	  case 38:      /* <type> (... */
	    type_decls[type_decls_ptr++] = m ->name;
	    break;
	  case 59:
	    if (n_parens <= 0)
	      return False;
	    break;
	  case 119:  /* If we have common type, like FILE.... */
	    if (last_state == 16) {  /* } <label> ; */
	      vars[n_vars].name = m -> name;
	      vars[n_vars].attrs = struct_decl_attrs;
	      struct_decl_attrs = 0;
	      ++n_vars;
	    }
	    break;
	  default:
	    type_decls[type_decls_ptr++] = m ->name;
	    break;
	  } /* switch (state) */
	break;
      case OPENBLOCK:
	++block_level;
	if (aggregate_type && block_level == 1) {
	  int t_block_level;
	  aggregate_decl = True;
	  attrs |= CVAR_ATTR_STRUCT_DECL;
	  struct_decl_attrs |= CVAR_ATTR_STRUCT_DECL; 
	  save_struct_declaration (messages, i);
	  find_struct_members (messages, i);
	  for (j = i-1, t_block_level = block_level; 
	       (j >= end_ptr) && t_block_level; j--) {
	    switch (messages[j] -> tokentype)
	      {
	      case OPENBLOCK:
		++t_block_level;
		break;
	      case CLOSEBLOCK:
		--t_block_level;
		break;
	      }
	  }
	  /*
	   *  Back up to the token before the closing block.
	   *  On the following iterations, the state changes
	   *  from the closing block forward should be sufficient
	   *  to determine the context of a token, so we
	   *  don't need to worry about last_state* for 
	   *  the closing block.
	   */
	  i = j + 2;
	}
	break;
      case CLOSEBLOCK:
	--block_level;
	if (aggregate_decl && (block_level == 0)) {
	  aggregate_decl = False;
	  restore_struct_declaration ();  /* Fall through. */
	}
	if (state == 27) {  /* } ; */
	  /*
	   *  Copy the last type specifier to the variable tag.
	   */
	  if (type_decls_ptr > 1) {
	    if (!str_eq (type_decls[type_decls_ptr - 1], "struct") &&
		!str_eq (type_decls[type_decls_ptr - 1], "union") &&
		!str_eq (type_decls[type_decls_ptr - 1], "enum")) {
	      vars[n_vars].name = type_decls[--type_decls_ptr];
	      vars[n_vars].attrs = struct_decl_attrs;
	      struct_decl_attrs = 0;
	      ++n_vars;
	    }	    
	  }
	}
	break;
      case CLOSEPAREN:
	if ((state == 41) && pointer)
	  func_ptr_decl = True;
	/*
	 *  ") { ..."  A internal function or control structure.
	 */
	if ((state == 69) && (interpreter_pass == var_pass)) 
	  return False;
	--n_parens;
	break;
      case OPENPAREN:
	++n_parens;
	switch (state)
	  {
	  case 42:
	    switch (last_state)
	      {
	      case 41:
		fn_arg_list = True;
		break;
	      }
	    break;
	  case 43:  /* ( label */
	    switch (last_state)
	      {
		/*
		 *  Function param list.
		 */
	      case 41:  /* ) (  (fn_ptr_decl) */
	      case 68:  /* label ( */
		if (fn_is_declaration (messages, i)) {
		  int i_1;
		  fn_param_cvars (messages, i, FALSE);
		  i_1 = match_paren (messages, i, end_ptr);
		  if ((state = VAR_DECLARATION_STATE (messages, end_ptr, i_1)) 
		      == ERROR) {
#ifdef __GNUC__
		    if (fn_has_gnu_attribute (messages, i_1)) {
		      if (have_possible_global_var_method_dup_label == True){
			global_var_dup_label_idx = i;
			warning (messages[msg_ptr],
			 "Label, \"%s,\" duplicates the name of a method.",
				 M_NAME(m));
		      }
		      return True;
		    } else {
		      _warning ("Parse error.\n");
		      return False;
		    }
#else
		    _warning ("Parse error.\n");
		    return False;
#endif
		  }
		  /*
		   *  Strange but true.
		   */
		  if (state == 60) {  /* <)> ; */
		    if (interpreter_pass == var_pass) {
		      attrs |= CVAR_ATTR_FN_PROTOTYPE;
		      vars[n_vars-1].attrs |= CVAR_ATTR_FN_PROTOTYPE;
		      if (!fn_param_decls)
			fn_param_cvars (messages, i, TRUE);
		    } else {
		      if (FRAME_SCOPE == GLOBAL_VAR) {
			attrs |= CVAR_ATTR_FN_PROTOTYPE;
			/* Note - Does not have a test case. */
			vars[n_vars-1].attrs |= CVAR_ATTR_FN_PROTOTYPE;
		      }
		      if (!fn_param_decls)
			fn_param_cvars (messages, i, TRUE);
		    }
		    i = i_1;
		  }
		  if (state == 70) {  /* <)> , */
		    i = i_1;
		  }
		} /* if (fn_is_declaration (messages, i)) */
	      }
	    break;
#ifdef __GNUC__
 	  case 51:  /* The start of a GNUC attribute: <(>( */
                    /* The state of the previous label before */
                    /* __attribute__ may have cause it to be */
                    /* interpreted as a type instead of a tag, */
                    /* so remove it from the types, and add to */
                    /* the var name. */
	    if ((i = match_paren (messages, i, end_ptr)) != ERROR) {
	      if (type_decls_ptr > 0) {
		if (!is_c_data_type (type_decls[type_decls_ptr -1]) &&
		    !typedef_is_declared (type_decls[type_decls_ptr - 1])) {
		  /*
		   *  We've reached the typedef tag.
		   */
		  vars[n_vars++].name = type_decls[--type_decls_ptr];
		  type_decls[type_decls_ptr+1] = NULL;
		  goto done;
		  break;
		}
	      }
	    }
	    break;
#endif
	  case 67:  /* <(> )  empty parameter list */
	    /*   fn_param_decls = fn_param_decls_ptr = NULL; */
	    attrs |= CVAR_ATTR_FN_NO_PARAMS;
	    vars[n_vars-1].attrs |= CVAR_ATTR_FN_NO_PARAMS;
 	    break;
	  }  /* switch (state) ... */
	break;
      case LABEL:
	switch (state)
	  {
	  case 3:   /* <this_label> <label> */
	    switch (last_state)
	      {
	      case 0:  /* <ctype> <this_label> */
		if (aggregate_type && typedef_check && !is_struct_decl) {
		                   /* Incomplete struct/enum typedef: e.g., */
		  return False;    /* typedef struct <struct_tag> <type_tag>;*/
		} else {
		  type_decls[type_decls_ptr++] = m -> name;
		}
		break;
	      case 3:  /* <label> <this_label> */
		type_decls[type_decls_ptr++] = m -> name;
		break;
	      case 16: /* } <this_label> <next_label>*/
		/* FIX? aggregate_decl should be set here,
		   as aggregate_type already is. */
		vars[n_vars].name = m -> name;
		vars[n_vars].attrs = struct_decl_attrs;
		struct_decl_attrs = 0;
		++n_vars;
		break;
	      } /* switch (last_state) */
	    break;

	    /*
	     *  This should be sufficient for the case of
	     *  a label occurring on its own, because the
	     *  function checks for a default type below.
	     */
	  case 5: /* <this_label> ; */
	    switch (last_state) 
	      {
	      case 16: /* } <this_label> */
		/* FIX? Check whether both aggregate_type
		   and aggregate_decl need to be set here. */
		vars[n_vars].name = m -> name;
		vars[n_vars].attrs = struct_decl_attrs;
		struct_decl_attrs = 0;
		++n_vars;
		break;
	      case 6:   /* * <this_label> ; */
		switch (last_state2)
		  {
		  case 63:  /* } * <this_label> ; */
		    vars[n_vars].name = m -> name;
		    vars[n_vars].attrs = struct_decl_attrs;
		    struct_decl_attrs = 0;
		    ++n_vars;
		    break;
		  default:
		    vars[n_vars].name = m -> name;
		    if ((attrs & CVAR_ATTR_STRUCT) ||
		(attrs == (CVAR_ATTR_STRUCT | CVAR_ATTR_STRUCT_INCOMPLETE)))
		    {
 		      if (have_struct (type_decls[type_decls_ptr-1])) {
 			if (vars[n_vars].n_derefs) {
 			  attrs = CVAR_ATTR_STRUCT_PTR;
			  vars[n_vars].attrs = CVAR_ATTR_STRUCT_PTR;
			  struct_decl_attrs = 0;
 			} else {
 			  attrs = CVAR_ATTR_STRUCT;
			  vars[n_vars].attrs = CVAR_ATTR_STRUCT;
			  struct_decl_attrs = 0;
 			}
 		      } else {
 			if ((last_state == 47) || (last_state2 == 47)) {
			  /* ... , <label> or ... , *<label> */
			  if (vars[n_vars].n_derefs) {
			    attrs = CVAR_ATTR_STRUCT_PTR_TAG;
			    vars[n_vars].attrs = CVAR_ATTR_STRUCT_PTR_TAG;
			    struct_decl_attrs = 0;
			  } else {
			    attrs = CVAR_ATTR_STRUCT_TAG;
			    vars[n_vars].attrs = CVAR_ATTR_STRUCT_TAG;
			    struct_decl_attrs = 0;
			  }
			} else {
			  attrs |= CVAR_ATTR_STRUCT_INCOMPLETE;
			  vars[n_vars].attrs |= CVAR_ATTR_STRUCT_INCOMPLETE;
			  struct_decl_attrs = 0;
			}
 		      }
		    }
		    ++n_vars;
		    break;
		  }
		break;
	      default:
		vars[n_vars].name = m -> name;
		if ((attrs == CVAR_ATTR_STRUCT) ||
	    (attrs == (CVAR_ATTR_STRUCT | CVAR_ATTR_STRUCT_INCOMPLETE)))
		  {
		    if (have_struct (type_decls[type_decls_ptr-1])) {
		      if (vars[n_vars].n_derefs) {
			attrs = CVAR_ATTR_STRUCT_PTR;
			vars[n_vars].attrs = CVAR_ATTR_STRUCT_PTR;
			struct_decl_attrs = 0;
		      } else {
			attrs = CVAR_ATTR_STRUCT;
			vars[n_vars].attrs = CVAR_ATTR_STRUCT;
			struct_decl_attrs = 0;
		      }
		    } else {
		      attrs |= CVAR_ATTR_STRUCT_INCOMPLETE;
		      vars[n_vars].attrs |= CVAR_ATTR_STRUCT_INCOMPLETE;
		      struct_decl_attrs = 0;
		    }
		  }
		++n_vars;
		break;
	      }
	    break;
	  case 15:  /* <this_label> { */
	  case 37:  /* <this_label> * */
	    /* If we have an incomplete type forward 
	       declared and used as a type tag, 
	       find it here. */
	    if (is_incomplete_type (m -> name))
	      m -> tokentype = CTYPE;
	    type_decls[type_decls_ptr++] = m -> name;
	    /*
	     *  Increment n_derefs only on an asterisk itself.
	     */
	    break;
	  case 40:  /* <this_label> ) */
	    if (!n_parens)
	      return False;
	    switch (last_state)
	      {
	      case 6:  /* * <this_label> ) */
		vars[n_vars].name = m -> name;
		attrs |= CVAR_ATTR_FN_PTR_DECL;
		vars[n_vars].attrs |= CVAR_ATTR_FN_PTR_DECL;
		++n_vars;
		break;
	      default:
		vars[n_vars++].name = m -> name;
		break;
	      }
	    break;
	  case 44:  /* <this_label> , */
	    switch (last_state) /* ... } <label> , ... */
	      {
	      case 16:
		/* attrs should be CVAR_ATTR_STRUCT | CVAR_ATTR_STRUCT_DECL */
		vars[n_vars].attrs = attrs;
		break;
 	      case 6: /* ... * <label>, ... */
		vars[n_vars].attrs = attrs;
		break;
	      }
	    vars[n_vars++].name = m -> name;
	    break;
	  case 53:  /* <this_label> [ */
	    switch (last_state)
	      {
	      case 43:  /* ( <this_label> [ (it's a fn argument) */
		break;
	      case 16:  /* } <this_label> [ */
		attrs |= CVAR_ATTR_STRUCT_TAG;
		/* fall through */
	      default:  /* <this_label> [ */
		aggregate_decl = True;
		attrs |= CVAR_ATTR_ARRAY_DECL;
		vars[n_vars].attrs |= attrs;
		vars[n_vars].name = m -> name;
		find_n_subscripts (messages, i, &n_subscripts);
		if (!format_method_output) {
		  if ((vars[n_vars].array_initializer_size = 
		       find_subscript_initializer_size (messages, i))
		      < 0) {
		    return FALSE;
		  }
		}
		vars[n_vars].n_derefs += n_subscripts;
		++n_vars;
		switch (declaration_context)
		  {
		  case decl_param_list_context:
		    /*
		     *  ERROR here could mean we're at the end of the stack.
		     */
		    if ((i = find_arg_end (messages, i)) != ERROR) {
		      ++i;
		      if ((state = VAR_DECLARATION_STATE (messages, end_ptr,
							  i)) == ERROR)
			return False;
		    } else {
		      i = end_ptr;
		      ++n_vars;
		    }
		    break;
 		  case decl_null_context:
 		  case decl_var_context:
		  default:
		    i = find_subscript_declaration_end (messages, i);
		    /*
		     *  Back up the stack pointer so it points to 
		     *  the expression separator or terminator on the
		     *  next iteration, and set the state for the 
		     *  closing bracket.
		     */
		    if ((i = prevlangmsg (messages, i)) != ERROR) {
		      if ((state = VAR_DECLARATION_STATE (messages, end_ptr,
							  i)) == ERROR)
			return False;
		    }
		    break;
		  } /* switch (declaration_context) */
		break;
	      }  /* switch (last_state) */
	    break;
	  case 55: /* <tag> : */
	    vars[n_vars].name = m -> name;
	    vars[n_vars].attrs = 0;
	    ++n_vars;
	    break;
	  case 68:   /* <this_label> ( */
	    switch (last_state)
	      {
	      case 0:  /* ctype <this_label> */
		vars[n_vars].name = m -> name;
		if (!n_parens) {
		  is_fn_decl = True;
		  attrs |= CVAR_ATTR_FN_DECL;
		  vars[n_vars].attrs |= CVAR_ATTR_FN_DECL;
		}
		++n_vars;
		break;
	      case 6:  /* ctype * <this_label> */
		vars[n_vars].name = m -> name;
		if (!n_parens) {
		  is_fn_decl = True;
		  attrs |= CVAR_ATTR_FN_PTR_DECL;
		  vars[n_vars].attrs |= CVAR_ATTR_FN_PTR_DECL;
		}
		++n_vars;
		break;
	      }
	    break;
	  default:
	    /*
	     *  For cases like in a sub parser and at the end of the stack,
	     *  when the state is -1.
	     */
	    vars[n_vars++].name = m -> name;
	    break;
	  } /* switch (state) */
	break;  /* case LABEL: */
      case ASTERISK:
	switch (state)
	  {
	  case 6:   /* <*> label */
	    switch (last_state)
	      {
	      case 2:   /* ctype <*> label */
 	      case 63:  /* End of struct: } <*> label */
	      case 37:   /* label <*> label */
	      case 39:   /* ( <*> label (fn ptr) */
	      case 47:  /* , *<label> */
	      case 50:  /* * <*> label */
		++vars[n_vars].n_derefs;
		break;
	      case 66:   /* RETURN_TYPE <*(> *pfi)() */
		break;
	      }
	    break;
	  case 50:  /* <*>* */
	    ++vars[n_vars].n_derefs;
	    break;
	  }
	break;
      case CHAR:
	break;
      case ARRAYOPEN:
	/* 
	 * Evaluating the constant expression makes the state
	 * transitions here much simpler.  We should not need
	 * to provide states for all the constant expression
	 * operators, just evaluate the expression and then 
	 * skip past it.
	 */
	if (!aggregate_decl)
	  pointer = True;

	for (const_expr_end = i; ; const_expr_end--) {
	  if (messages[const_expr_end] -> tokentype == ARRAYCLOSE)
	    break;
	}
	++const_expr_end;
	memset ((void *)&macro_val, 0, sizeof (VAL));
	macro_val.__type = INTEGER_T;
	eval_constant_expr (messages, i - 1, &const_expr_end, 
			    &macro_val);
	i = const_expr_end - 1;
	break;
      case ARGSEPARATOR:
	pointer = False;
	struct_decl_attrs = 0;
	break;
      case SEMICOLON:
	goto done;
      }
    last_state2 = last_state;
    last_state = state;

  }

 done:

/*
 *  Workaround for Solaris 2.10 /usr/include/sys/siginfo.h.
 *  Need to check if this is the definition for an incomplete
 *  type.
 */
#if defined (__sun__) && defined (__svr4__)
if (str_eq (type_decls[0], "struct") &&
    str_eq (type_decls[1], "siginfo") &&
    str_eq (type_decls[2], "siginfo_t")) {
  vars[0].name = type_decls[2];
  n_vars = 1;
  type_decls_ptr--;
}
#endif

  /*
   *  Make sure there is at least one basic C type or 
   *  typedef in the declaration.
   */
  for (j = 0, have_data_type = False; 
       (j < type_decls_ptr) && !have_data_type; j++) {
    if ((is_c_data_type (type_decls[j]) &&
	 (!is_c_storage_class (type_decls[j]) &&
	  !IS_SIGN_LABEL (type_decls[j]) &&
	  !IS_TYPE_LENGTH (type_decls[j]))) ||
#ifdef __GNUC__
	is_gnuc_builtin_type (type_decls[j]) ||
#endif
	get_typedef (type_decls[j]) ||
	/* Include stuff like FILE, __FILE, pthread... ... */
	is_incomplete_type (type_decls[j]))
      have_data_type = True;
  }
  if (!have_data_type) {
    for (j = 0; j < type_decls_ptr; j++) {
      if (is_ctalk_keyword(type_decls[j]) ||
	  IS_CONSTRUCTOR_LABEL(type_decls[j]))
	return False;
    }
  }

  if (!have_data_type)
    type_decls[type_decls_ptr++] =  default_type_decl;

  if (vars[0].name) {
    return_val = True;
  } else {
    return_val = False;
  }

  if ((return_val == True) && 
      (have_possible_global_var_method_dup_label == True) && 
      (global_var_dup_label_idx != -1) &&
      (is_struct_decl == False)) {
    warning (messages[global_var_dup_label_idx],
	     "Label, \"%s,\" duplicates the name of a method.", 
	     M_NAME(messages[global_var_dup_label_idx]));
  }

  if ((return_val == True) && 
      (have_possible_ctalk_keyword == True) &&
      (is_struct_decl == false) &&
      (interpreter_pass == library_pass)) {
    /* we only want globals here. */
    warning (messages[global_var_dup_label_idx],
	     "Keyword, \"%s,\" used in C declaration.", 
	     M_NAME(messages[global_var_dup_label_idx]));
  }

  return return_val;
}

/* 
   There are enough differences when parsing C method parameters that
   this can be a different function than is_c_var_declaration_msg () ...
   we should be able to remove a lot of things that don't occur in 
   parameters, like struct definitions, initializers, etc. etc. 
*/
bool is_c_param_declaration_msg (MESSAGE_STACK messages, int msg_ptr,
				  int ignore_oop_names) {
  int i, j;
  int end_ptr;
  int state,
    last_state,
    last_state2;
  int r;
  int const_expr_end;          /* Pointer for constant expr substitution. */
  int n_subscripts;
  bool return_val = True;
  bool aggregate_type,      /* True if aggregate type.                 */
    aggregate_decl;            /* True if aggregate initializers.         */
  bool func_ptr_decl,       /* True if decl is a function pointer.     */
    fn_arg_list,             /* True if within function decl arg list.  */
    is_fn_decl;
  bool comment = False;
  bool have_data_type;
  bool pointer;
  int block_level,
    n_parens;
  MESSAGE *m;
  OBJECT *tmpobj;
  VAL macro_val;
  bool have_possible_global_var_method_dup_label = False;
  int global_var_dup_label_idx = -1;
  int struct_decl_attrs;
  int aggregate_class;
  CVAR *c;
  METHOD *_n_method;
  int _n_th_param;
	

  /*
   *  Check whether a label has already raised an exception.
   */
  if (__ctalkTrapExceptionInternal (messages[msg_ptr]))
    return False;

  if (messages[msg_ptr] -> evaled && messages[msg_ptr] -> output)
    return False;

  clear_variable ();
  struct_decl_attrs = 0;

  /*
   *  If the variable is already declared, make sure that its
   *  use is not also taken as a declaration.  A redeclaration
   *  needs an explicit type preceding the variable tag. So
   *  if there is a declaration like,
   *  
   *    OBJECT *val_obj;
   *
   *  then a statment like
   *
   *    val_obj = ...
   *
   *  is not mistaken for a declaration of type, "int."
   *
   *  The function issues the redeclaration warning below.
   */
  if (have_declaration (messages[msg_ptr] -> name)) {
    if (!have_type_prefix (messages, msg_ptr)) {
      return False;
    }
  }

  /*
   * Also exclude cases where the label is 
   * preceded by a ref operator, except 
   * where an asterisk prefix is preceded by
   * a type label.
   */
  if (have_ref_prefix (messages, msg_ptr))
    return False;

  if (is_instance_method_or_variable (messages, msg_ptr)) 
    return False;

  /*
   *  Declarations also don't start with pointer or 
   *  address of operators, or aggregate operators,
   *  or immediately follow them.  So take care of those 
   *  cases here.
   */
  if ((messages[msg_ptr] -> tokentype == ASTERISK) ||
      (messages[msg_ptr] -> tokentype == AMPERSAND) ||
      (messages[msg_ptr] -> tokentype == DEREF) ||
      (messages[msg_ptr] -> tokentype == PERIOD))
    return False;

  end_ptr = get_stack_top (messages);
  block_level = n_parens = 0;
  type_decls_ptr = n_vars = 0;
  aggregate_type = aggregate_decl = is_unsigned = pointer = 
    func_ptr_decl = is_fn_decl = fn_arg_list = have_data_type = False;
  storage_class = NULL;

  state = last_state = last_state2 = ERROR;
  
  /* Initialize in case the macro expression doesn't set a value. */

  for (i = msg_ptr; i >= end_ptr; i--) {

    m = messages[i];

    /* It should only be necessary to evaluate a variable
       declaration once. */

    if (!m || !IS_MESSAGE (m))
      break;

    if (M_ISSPACE (m)) continue;

    /* These are needed until the parser is able to remove 
       all comment tokens. */
    if (m -> tokentype == OPENCCOMMENT) {
      comment = True;
      continue;
    }
    if (comment && (m -> tokentype == CLOSECCOMMENT)) {
      comment = False;
      continue;
    }
    
    if (!ignore_oop_names) {
      /*
       *  See if we can gather these under one 
       *  (M_TOK(m) == LABEL) test.
       */
      if (M_TOK(m) == LABEL) {
	if ((tmpobj = get_object (M_NAME(m), NULL)) != NULL)
	  return False;
	if ((tmpobj = get_class_object (M_NAME(m))) != NULL)
	  return False;
	if (is_pending_class (M_NAME(m)))
	  return False;
	if (str_eq (m -> name, "returnObjectClass") ||
	    str_eq (m -> name, "noMethodInit"))
	  return False;
      }
      if ((M_TOK(m) == LABEL) &&
	  is_method_name (M_NAME(m))) {
	if (interpreter_pass == var_pass) {
	  have_possible_global_var_method_dup_label = TRUE;
	  global_var_dup_label_idx = i;
	}
      }

      if (interpreter_pass == method_pass) {
	_n_method = new_methods[new_method_ptr+1] -> method;
	for (_n_th_param = 0; _n_th_param < _n_method -> n_params; 
	     _n_th_param++) {
	  if (str_eq (M_NAME(m), _n_method->params[_n_th_param]->name)) {
	    warning (m, "Parameter \"%s\" shadows a method parameter.",
		     M_NAME(m));
	    
	    return False;
	  }
	}

      }

    }
#ifdef __GNUC__
    if ((r = is_c_data_type_attr (m -> name)) != FALSE) {
      if ((r == CVAR_TYPE_LONG) && (type_attrs & CVAR_TYPE_LONG)) {
	type_attrs |= CVAR_TYPE_LONGLONG;
      } else {
	type_attrs |= r;
      }
      m -> tokentype = CTYPE;
    } else {
      if (((r = is_gnuc_builtin_type (m -> name)) == TRUE) ||
	  ((r = is_c_derived_type (m -> name)) == TRUE)) {
	m -> tokentype = CTYPE;
      } else {
	if ((r = is_gnu_extension_keyword (m -> name)) == TRUE) {
	  if (warn_extension_opt) {
	    warning (m, "Compiler extension %s.", m -> name);
	  }
	  continue;
	}
      }
    }
#else
    if ((r = is_c_data_type_attr (m -> name)) != FALSE) {
      if ((r == CVAR_TYPE_LONG) && (type_attrs & CVAR_TYPE_LONG)) {
	type_attrs |= CVAR_TYPE_LONGLONG;
      } else {
	type_attrs |= r;
      }
      m -> tokentype = CTYPE;
    } else {
      if ((r = is_c_derived_type (m -> name)) == TRUE) {
	m -> tokentype = CTYPE;
      }
    }
#endif

    /* TO DO - Determine if tracking through the branches
       here, even if they are not legal statements, 
       can be recovered for later. */
    if ((state = VAR_DECLARATION_STATE (messages, end_ptr, i)) == ERROR) {
      /*
       *  Return an error if not at the end of the stack; i.e.,
       *  when not parsing parameter declarations.
       */
      if (format_method_output)
	goto done;
      if (nextlangmsgstack (messages, i) != ERROR) return False;
    }

    /* Don't analyze keywords like, "return," etc. */
    if (M_TOK(m) == LABEL) {
      if (is_c_c_keyword (M_NAME(m)))
	return False;
    }

    switch (m -> tokentype)
      {
      case CTYPE:
	switch (state)
	  {
	  case 0:  /* 
		    *  C types except for sign labels and storage
		    *  classes go in the type_decls[] array.
		    */
	    /* signed|unsigned -- see IS_SIGN_LABEL2 in cvar.h. */
	    if (IS_SIGN_LABEL2 (m -> name) == 2) {
	      is_unsigned = True;
	    } else if ((storage_class = 
			is_c_storage_class (m -> name)) == NULL) {
	    /* const|extern|inline|register|static|volatile */
	      if ((aggregate_class = IS_AGGREGATE_TYPE (m -> name) > 0)) {
		/* enum|struct|union -- see the #define of
		   IS_AGGREGATE_TYPE in cvar.h. */
		aggregate_type = True;
		/* Enums get handled separately. */
		if (aggregate_class == 1 || aggregate_class == 3) {
		  attrs |= CVAR_ATTR_STRUCT;
		  struct_decl_attrs |= CVAR_ATTR_STRUCT;
		}
	      }
	    }
	    type_decls[type_decls_ptr++] = m ->name;

	    break;
	  case 2:   /* <type> * */
	    type_decls[type_decls_ptr++] = m -> name;
	    if ((c = get_typedef (M_NAME(m))) != NULL) {
	      if (c -> attrs & CVAR_ATTR_TYPEDEF && 
		  str_eq (c -> type, "struct")) {
		aggregate_type = True;
 		attrs |= CVAR_ATTR_STRUCT_PTR;
	      }
	    }
	    break;
	  case 14:      /* <struct|enum|union {> */
	    type_decls[type_decls_ptr++] = m -> name;
	    aggregate_type = aggregate_decl = True;
	    break;
	  case 38:      /* <type> (... */
	    type_decls[type_decls_ptr++] = m -> name;
	    break;
	  case 59:
	    if (n_parens <= 0)
	      return False;
	    break;
	  default:
	    type_decls[type_decls_ptr++] = m -> name;
	    break;
	  } /* switch (state) */
	break;
      case OPENBLOCK:
	++block_level;
	if (aggregate_type && block_level == 1) {
	  int t_block_level;
	  aggregate_decl = True;
	  attrs |= CVAR_ATTR_STRUCT_DECL;
	  struct_decl_attrs |= CVAR_ATTR_STRUCT_DECL; 
	  save_struct_declaration (messages, i);
	  find_struct_members (messages, i);
	  for (j = i-1, t_block_level = block_level; 
	       (j >= end_ptr) && t_block_level; j--) {
	    switch (messages[j] -> tokentype)
	      {
	      case OPENBLOCK:
		++t_block_level;
		break;
	      case CLOSEBLOCK:
		--t_block_level;
		break;
	      }
	  }
	  /*
	   *  Back up to the token before the closing block.
	   *  On the following iterations, the state changes
	   *  from the closing block forward should be sufficient
	   *  to determine the context of a token, so we
	   *  don't need to worry about last_state* for 
	   *  the closing block.
	   */
	  i = j + 2;
	}
	break;
      case CLOSEBLOCK:
	--block_level;
	if (aggregate_decl && (block_level == 0)) {
	  aggregate_decl = False;
	  restore_struct_declaration ();  /* Fall through. */
	}
	if (state == 27) {  /* } ; */
	  /*
	   *  Copy the last type specifier to the variable tag.
	   */
	  if (type_decls_ptr > 1) {
	    if (!str_eq (type_decls[type_decls_ptr - 1], "struct") &&
		!str_eq (type_decls[type_decls_ptr - 1], "union") &&
		!str_eq (type_decls[type_decls_ptr - 1], "enum")) {
	      vars[n_vars].name = type_decls[--type_decls_ptr];
	      vars[n_vars].attrs = struct_decl_attrs;
	      struct_decl_attrs = 0;
	      ++n_vars;
	    }	    
	  }
	}
	break;
      case CLOSEPAREN:
	if ((state == 41) && pointer)
	  func_ptr_decl = True;
	/*
	 *  ") { ..."  A internal function or control structure.
	 */
	if ((state == 69) && (interpreter_pass == var_pass)) 
	  return False;
	--n_parens;
	if (n_parens == 0) {
	  if (state == 52 || state == 70) {
	    goto done;
	  }
	}
	break;
      case OPENPAREN:
	++n_parens;
	switch (state)
	  {
	  case 42:
	    switch (last_state)
	      {
	      case 41:
		fn_arg_list = True;
		break;
	      }
	    break;
	  case 43:  /* ( label */
	    switch (last_state)
	      {
		/*
		 *  Function param list.
		 */
	      case 41:  /* ) (  (fn_ptr_decl) */
	      case 68:  /* label ( */
		if (fn_is_declaration (messages, i)) {
		  int i_1;
		  fn_param_cvars (messages, i, FALSE);
		  i_1 = match_paren (messages, i, end_ptr);
		  if ((state = VAR_DECLARATION_STATE (messages, end_ptr,
						      i_1)) == ERROR) {
#ifdef __GNUC__
		    if (fn_has_gnu_attribute (messages, i_1)) {
		      if (have_possible_global_var_method_dup_label == True){
			global_var_dup_label_idx = i;
			warning (messages[msg_ptr],
			 "Label, \"%s,\" duplicates the name of a method.",
				 M_NAME(m));
		      }
		      return True;
		    } else {
		      _warning ("Parse error.\n");
		      return False;
		    }
#else
		    _warning ("Parse error.\n");
		    return False;
#endif
		  }
		  /*
		   *  Strange but true.
		   */
		  if (state == 60) {  /* <)> ; */
		    if (interpreter_pass == var_pass) {
		      attrs |= CVAR_ATTR_FN_PROTOTYPE;
		      vars[n_vars-1].attrs |= CVAR_ATTR_FN_PROTOTYPE;
		      if (!fn_param_decls)
			fn_param_cvars (messages, i, TRUE);
		    } else {
		      if (FRAME_SCOPE == GLOBAL_VAR) {
			attrs |= CVAR_ATTR_FN_PROTOTYPE;
			/* Note - Does not have a test case. */
			vars[n_vars-1].attrs |= CVAR_ATTR_FN_PROTOTYPE;
		      }
		      if (!fn_param_decls)
			fn_param_cvars (messages, i, TRUE);
		    }
		    i = i_1;
		  }
		  if (state == 70) {  /* <)> , */
		    i = i_1;
		  }
		} /* if (fn_is_declaration (messages, i)) */
	      }
	    break;
#ifdef __GNUC__
 	  case 51:  /* The start of a GNUC attribute: <(>( */
                    /* The state of the previous label before */
                    /* __attribute__ may have cause it to be */
                    /* interpreted as a type instead of a tag, */
                    /* so remove it from the types, and add to */
                    /* the var name. */
	    if ((i = match_paren (messages, i, end_ptr)) != ERROR) {
	      if (!is_c_data_type (type_decls[type_decls_ptr -1]) &&
		  !typedef_is_declared (type_decls[type_decls_ptr - 1])) {
		/*
		 *  We've reached the typedef tag.
		 */
		vars[n_vars++].name = type_decls[--type_decls_ptr];
		type_decls[type_decls_ptr + 1] = NULL;
		goto done;
		break;
	      }
	    }
	    break;
#endif
	  case 67:  /* <(> )  empty parameter list */
	    /*   fn_param_decls = fn_param_decls_ptr = NULL; */
	    attrs |= CVAR_ATTR_FN_NO_PARAMS;
	    vars[n_vars-1].attrs |= CVAR_ATTR_FN_NO_PARAMS;
 	    break;
	  }  /* switch (state) ... */
	break;
      case LABEL:
	switch (state)
	  {
	  case 3:   /* <this_label> <label> */
	    switch (last_state)
	      {
	      case 0:  /* <ctype> <this_label> */
		if (aggregate_type && typedef_check && !is_struct_decl) {
		                   /* Incomplete struct/enum typedef: e.g., */
		  return False;    /* typedef struct <struct_tag> <type_tag>;*/
		} else {
		  type_decls[type_decls_ptr++] = m -> name;
		}
		break;
	      case 3:  /* <label> <this_label> */
		type_decls[type_decls_ptr++] = m -> name;
		break;
	      case 16: /* } <this_label> <next_label>*/
		/* FIX? aggregate_decl should be set here,
		   as aggregate_type already is. */
		vars[n_vars].name = m -> name;
		vars[n_vars].attrs = struct_decl_attrs;
		struct_decl_attrs = 0;
		++n_vars;
		break;
	      } /* switch (last_state) */
	    break;

	    /*
	     *  This should be sufficient for the case of
	     *  a label occurring on its own, because the
	     *  function checks for a default type below.
	     */
	  case 5: /* <this_label> ; */
	    switch (last_state) 
	      {
	      case 16: /* } <this_label> */
		/* FIX? Check whether both aggregate_type
		   and aggregate_decl need to be set here. */
		vars[n_vars].name = m -> name;
		vars[n_vars].attrs = struct_decl_attrs;
		struct_decl_attrs = 0;
		++n_vars;
		break;
	      case 6:   /* * <this_label> ; */
		switch (last_state2)
		  {
		  case 63:  /* } * <this_label> ; */
		    vars[n_vars].name = m -> name;
		    vars[n_vars].attrs = struct_decl_attrs;
		    struct_decl_attrs = 0;
		    ++n_vars;
		    break;
		  default:
		    vars[n_vars].name = m -> name;
		    if ((attrs & CVAR_ATTR_STRUCT) ||
		(attrs == (CVAR_ATTR_STRUCT | CVAR_ATTR_STRUCT_INCOMPLETE)))
		    {
 		      if (have_struct (type_decls[type_decls_ptr-1])) {
 			if (vars[n_vars].n_derefs) {
 			  attrs = CVAR_ATTR_STRUCT_PTR;
			  vars[n_vars].attrs = CVAR_ATTR_STRUCT_PTR;
			  struct_decl_attrs = 0;
 			} else {
 			  attrs = CVAR_ATTR_STRUCT;
			  vars[n_vars].attrs = CVAR_ATTR_STRUCT;
			  struct_decl_attrs = 0;
 			}
 		      } else {
 			if ((last_state == 47) || (last_state2 == 47)) {
			  /* ... , <label> or ... , *<label> */
			  if (vars[n_vars].n_derefs) {
			    attrs = CVAR_ATTR_STRUCT_PTR_TAG;
			    vars[n_vars].attrs = CVAR_ATTR_STRUCT_PTR_TAG;
			    struct_decl_attrs = 0;
			  } else {
			    attrs = CVAR_ATTR_STRUCT_TAG;
			    vars[n_vars].attrs = CVAR_ATTR_STRUCT_TAG;
			    struct_decl_attrs = 0;
			  }
			} else {
			  attrs |= CVAR_ATTR_STRUCT_INCOMPLETE;
			  vars[n_vars].attrs |= CVAR_ATTR_STRUCT_INCOMPLETE;
			  struct_decl_attrs = 0;
			}
 		      }
		    }
		    ++n_vars;
		    break;
		  }
		break;
	      default:
		vars[n_vars].name = m -> name;
		if ((attrs == CVAR_ATTR_STRUCT) ||
	    (attrs == (CVAR_ATTR_STRUCT | CVAR_ATTR_STRUCT_INCOMPLETE)))
		  {
		    if (have_struct (type_decls[type_decls_ptr-1])) {
		      if (vars[n_vars].n_derefs) {
			attrs = CVAR_ATTR_STRUCT_PTR;
			vars[n_vars].attrs = CVAR_ATTR_STRUCT_PTR;
			struct_decl_attrs = 0;
		      } else {
			attrs = CVAR_ATTR_STRUCT;
			vars[n_vars].attrs = CVAR_ATTR_STRUCT;
			struct_decl_attrs = 0;
		      }
		    } else {
		      attrs |= CVAR_ATTR_STRUCT_INCOMPLETE;
		      vars[n_vars].attrs |= CVAR_ATTR_STRUCT_INCOMPLETE;
		      struct_decl_attrs = 0;
		    }
		  }
		++n_vars;
		break;
	      }
	    break;
	  case 15:  /* <this_label> { */
	  case 37:  /* <this_label> * */
	    /* If we have an incomplete type forward 
	       declared and used as a type tag, 
	       find it here. */
	    if (is_incomplete_type (m -> name))
	      m -> tokentype = CTYPE;
	    type_decls[type_decls_ptr++] = m -> name;
	    /*
	     *  Increment n_derefs only on an asterisk itself.
	     */
	    break;
	  case 40:  /* <this_label> ) */
	    if (!n_parens) {
	      vars[n_vars++].name = m -> name;
	      goto done;
	    }
	    switch (last_state)
	      {
	      case 6:  /* * <this_label> ) */
		vars[n_vars].name = m -> name;
		attrs |= CVAR_ATTR_FN_PTR_DECL;
		vars[n_vars].attrs |= CVAR_ATTR_FN_PTR_DECL;
		++n_vars;
		break;
	      default:
		vars[n_vars++].name = m -> name;
		break;
	      }
	    break;
	  case 44:  /* <this_label> , */
	    switch (last_state) /* ... } <label> , ... */
	      {
	      case 16:
		/* attrs should be CVAR_ATTR_STRUCT | CVAR_ATTR_STRUCT_DECL */
		vars[n_vars].attrs = attrs;
		break;
 	      case 6: /* ... * <label>, ... */
		vars[n_vars].attrs = attrs;
		break;
	      }
	    vars[n_vars++].name = m ->name;
	    goto done;
	    break;
	  case 53:  /* <this_label> [ */
	    switch (last_state)
	      {
	      case 43:  /* ( <this_label> [ (it's a fn argument) */
		break;
	      case 16:  /* } <this_label> [ */
	      default:  /* <this_label> [ */
		aggregate_decl = True;
		attrs |= CVAR_ATTR_ARRAY_DECL;
		vars[n_vars].attrs |= CVAR_ATTR_ARRAY_DECL;
		vars[n_vars].name = m -> name;
		find_n_subscripts (messages, i, &n_subscripts);
		if (!format_method_output) {
		    vars[n_vars].array_initializer_size = 
		      find_subscript_initializer_size (messages, i);
		}
		vars[n_vars].n_derefs += n_subscripts;
		++n_vars;
		switch (declaration_context)
		  {
		  case decl_param_list_context:
		    /*
		     *  ERROR here could mean we're at the end of the stack.
		     */
		    if ((i = find_arg_end (messages, i)) != ERROR) {
		      ++i;
		      if ((state = VAR_DECLARATION_STATE (messages, end_ptr,
							  i)) == ERROR)
			return False;
		    } else {
		      i = end_ptr;
		      ++n_vars;
		    }
		    break;
 		  case decl_null_context:
 		  case decl_var_context:
		  default:
		    i = find_subscript_declaration_end (messages, i);
		    /*
		     *  Back up the stack pointer so it points to 
		     *  the expression separator or terminator on the
		     *  next iteration, and set the state for the 
		     *  closing bracket.
		     */
		    if ((i = prevlangmsg (messages, i)) != ERROR) {
		      if ((state = VAR_DECLARATION_STATE (messages,
							  end_ptr, i)) 
			  == ERROR)
			return False;
		    }
		    break;
		  } /* switch (declaration_context) */
		break;
	      }  /* switch (last_state) */
	    break;
	  case 68:   /* <this_label> ( */
	    switch (last_state)
	      {
	      case 0:  /* ctype <this_label> */
		vars[n_vars].name = m -> name;
		if (!n_parens) {
		  is_fn_decl = True;
		  attrs |= CVAR_ATTR_FN_DECL;
		  vars[n_vars].attrs |= CVAR_ATTR_FN_DECL;
		}
		++n_vars;
		break;
	      case 6:  /* ctype * <this_label> */
		vars[n_vars].name = m -> name;
		if (!n_parens) {
		  is_fn_decl = True;
		  attrs |= CVAR_ATTR_FN_PTR_DECL;
		  vars[n_vars].attrs |= CVAR_ATTR_FN_PTR_DECL;
		}
		++n_vars;
		break;
	      }
	    break;
	  default:
	    /*
	     *  For cases like in a sub parser and at the end of the stack,
	     *  when the state is -1.
	     */
	    vars[n_vars++].name = m -> name;
	    break;
	  } /* switch (state) */
	break;  /* case LABEL: */
      case ASTERISK:
	switch (state)
	  {
	  case 6:   /* <*> label */
	    switch (last_state)
	      {
	      case 2:   /* ctype <*> label */
 	      case 63:  /* End of struct: } <*> label */
	      case 37:   /* label <*> label */
	      case 39:   /* ( <*> label (fn ptr) */
	      case 47:  /* , *<label> */
	      case 50:  /* * <*> label */
		++vars[n_vars].n_derefs;
		break;
	      case 66:   /* RETURN_TYPE <*(> *pfi)() */
		break;
	      }
	    break;
	  case 50:  /* <*>* */
	    ++vars[n_vars].n_derefs;
	    break;
	  }
	break;
      case CHAR:
	break;
      case ARRAYOPEN:
	/* 
	 * Evaluating the constant expression makes the state
	 * transitions here much simpler.  We should not need
	 * to provide states for all the constant expression
	 * operators, just evaluate the expression and then 
	 * skip past it.
	 */
	if (!aggregate_decl)
	  pointer = True;

	for (const_expr_end = i; ; const_expr_end--) {
	  if (messages[const_expr_end] -> tokentype == ARRAYCLOSE)
	    break;
	}
	++const_expr_end;
	memset ((void *)&macro_val, 0, sizeof (VAL));
	macro_val.__type = INTEGER_T;
	eval_constant_expr (messages, i - 1, &const_expr_end, 
			    &macro_val);
	i = const_expr_end - 1;
	break;
      case ARGSEPARATOR:
	pointer = False;
	struct_decl_attrs = 0;
	break;
      case SEMICOLON:
	goto done;
      }
    last_state2 = last_state;
    last_state = state;

  }

 done:

/*
 *  Workaround for Solaris 2.10 /usr/include/sys/siginfo.h.
 *  Need to check if this is the definition for an incomplete
 *  type.
 */
#if defined (__sun__) && defined (__svr4__)
if (str_eq (type_decls[0], "struct") &&
    str_eq (type_decls[1], "siginfo") &&
    str_eq (type_decls[2], "siginfo_t")) {
  vars[0].name = type_decls[2];
  n_vars = 1;
  type_decls_ptr--;
}
#endif

  /*
   *  Make sure there is at least one basic C type or 
   *  typedef in the declaration.
   */
  for (j = 0, have_data_type = False; 
       (j < type_decls_ptr) && !have_data_type; j++) {
    if ((is_c_data_type (type_decls[j]) &&
	 (!is_c_storage_class (type_decls[j]) &&
	  !IS_SIGN_LABEL (type_decls[j]) &&
	  !IS_TYPE_LENGTH (type_decls[j]))) ||
#ifdef __GNUC__
	is_gnuc_builtin_type (type_decls[j]) ||
#endif
	get_typedef (type_decls[j]) ||
	/* Include stuff like FILE, __FILE, pthread... ... */
	is_incomplete_type (type_decls[j]))
      have_data_type = True;
  }
  if (!have_data_type) {
    for (j = 0; j < type_decls_ptr; j++) {
      if (is_ctalk_keyword(type_decls[j]) ||
	  IS_CONSTRUCTOR_LABEL(type_decls[j]))
	return False;
    }
  }

  if (!have_data_type)
    type_decls[type_decls_ptr++] = default_type_decl;

  if (*vars[0].name) {
    return_val = True;
  } else {
    return_val = False;
  }

  if ((return_val == True) && 
      (have_possible_global_var_method_dup_label == True) && 
      (global_var_dup_label_idx != -1) &&
      (is_struct_decl == False)) {
    warning (messages[global_var_dup_label_idx],
	     "Label, \"%s,\" duplicates the name of a method.", 
	     M_NAME(messages[global_var_dup_label_idx]));
  }

  return return_val;
}

int initializer_end (MESSAGE_STACK messages, int idx) {

  int i,
    n_parens,
    n_blocks,
    n_braces,
    prev_token = -1;

  n_parens = n_blocks = n_braces = FALSE;
  for (i = idx - 1; messages[i]; i--) {
    switch (M_TOK(messages[i]))
      {
      case OPENPAREN:
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	break;
      case ARRAYOPEN:
	++n_blocks;
	break;
      case ARRAYCLOSE:
	--n_blocks;
	break;
      case OPENBLOCK:
	++n_braces;
	break;
      case CLOSEBLOCK:
	--n_braces;
	break;
      case COLON: /* bitfield */
	return prev_token;
	break;
      case ARGSEPARATOR:
      case SEMICOLON:
	if (!n_parens && !n_blocks && !n_braces) {
	  if (prev_token == -1) {
	    return i;
	  } else {
	    return prev_token;
	  }
	}
	break;
      }
    prev_token = i;
  }

  return ERROR;
}

void save_fn_declaration (MESSAGE_STACK messages, int ptr) {

  memset ((void *)&fn_decl[fn_decl_ptr], 0, sizeof (struct _fn_decl));

  if (storage_class) {
    strcpy (fn_decl[fn_decl_ptr].storage_class, storage_class); 
  }

  memcpy ((void *)&(fn_decl[fn_decl_ptr].type_decls),
	  (void *)type_decls, (MAX_DECLARATORS * sizeof (char *)));

  fn_decl[fn_decl_ptr].type_decls_ptr = type_decls_ptr;

  memcpy ((void *)&(fn_decl[fn_decl_ptr].vars), (void *)&(vars),
	  (sizeof (VARNAME) * MAXARGS));
  fn_decl[fn_decl_ptr].n_vars = n_vars;

  fn_decl[fn_decl_ptr].attrs = attrs;

  fn_decl[fn_decl_ptr].params = fn_param_decls;

  if (n_vars > -1) {
    if (vars[n_vars].name) {
      strcpy (fn_decl[fn_decl_ptr].name, vars[n_vars].name);
    } else {
      fn_decl[fn_decl_ptr].name[0] = '\0';
    }
  }

  ++fn_decl_ptr;
}

/*
 *   This should be called from find_struct_members () with 
 *   ptr pointing to the opening block of the struct.
 */
void save_struct_declaration (MESSAGE_STACK messages, int ptr) {

  int i, 
    n_braces,
    stack_begin,
    stack_end;

  stack_begin = stack_start (messages);
  stack_end = get_stack_top (messages);

  memset ((void *)&struct_decl[struct_decl_ptr], 
	  0, sizeof (struct _struct_decl));

  if (messages[ptr] -> tokentype != OPENBLOCK) {
    if ((struct_decl[struct_decl_ptr].block_start = 
	 scanback (messages, ptr, stack_begin, OPENBLOCK)) == ERROR)
      _error ("Parse error.\n");
  } else {
    struct_decl[struct_decl_ptr].block_start = ptr;
  }

  for (i = struct_decl[struct_decl_ptr].block_start, n_braces = 0; 
       i > stack_end; i--) {
    if (messages[i] -> tokentype == OPENBLOCK)
      ++n_braces;
    if (messages[i] -> tokentype == CLOSEBLOCK)
      --n_braces;
    if (!n_braces)
      break;
  }
  struct_decl[struct_decl_ptr].block_end = i;
  
  if (storage_class) {
    strcpy (struct_decl[struct_decl_ptr].storage_class, storage_class); 
  }

  memcpy ((void *)&(struct_decl[struct_decl_ptr].type_decls),
	  (void *)type_decls, (MAX_DECLARATORS * sizeof (char *)));
  struct_decl[struct_decl_ptr].type_decls_ptr = type_decls_ptr;

  memcpy ((void *)&(struct_decl[struct_decl_ptr].vars),
	  (void *)&(vars),
	  (sizeof (VARNAME) * MAXARGS));
  struct_decl[struct_decl_ptr].n_vars = n_vars;

  struct_decl[struct_decl_ptr].attrs = attrs;

  if (n_vars > -1) {
    if (vars[n_vars].name) {
      strcpy (struct_decl[struct_decl_ptr].name, vars[n_vars].name);
    } else {
      struct_decl[struct_decl_ptr].name[0] = '\0';
    }
  }

  /*
   *  If struct_members and struct_decl_ptr == 0, then we're starting 
   *  to find the members of the outermost struct, and struct members
   *  should be NULL.  On inner structs, save the members
   *  we've already parsed in the outer struct.
   */
  if (struct_members) {
    struct_decl[struct_decl_ptr-1].members = struct_members;
    struct_decl[struct_decl_ptr-1].members_ptr = struct_members_ptr;
    struct_members = struct_members_ptr = NULL;
  }

  /*
   *  Struct_members should be NULL on entering the outermost 
   *  struct.
   */

  ++struct_decl_ptr;

}

void reset_struct_decl (void) {
  is_struct_decl = False; 
  struct_decl_ptr = 0;
  struct_members = struct_members_ptr = NULL;
}

void restore_fn_declaration (void) {

  int c_fn_ptr;

  c_fn_ptr = fn_decl_ptr - 1;

  clear_variable ();

  if (*fn_decl[c_fn_ptr - 1].storage_class) {
    /* point to the version of the label in static memory. */
    storage_class =  is_c_storage_class  (fn_decl[c_fn_ptr].storage_class);
  }

  vars[0].name = fn_decl[c_fn_ptr].name;

  memcpy ((void *)type_decls, 
	  (void *)&(fn_decl[c_fn_ptr].type_decls),
	  (MAX_DECLARATORS * sizeof (char *)));

  type_decls_ptr = fn_decl[c_fn_ptr].type_decls_ptr;
  
  memcpy ((void *)&vars, (void *)&(fn_decl[c_fn_ptr].vars),
	  (MAX_DECLARATORS * MAXLABEL));

  n_vars = fn_decl[c_fn_ptr].n_vars;
  attrs = fn_decl[c_fn_ptr].attrs;

  memset ((void *)&fn_decl[c_fn_ptr], 0, sizeof (STRUCT_DECL));

  --fn_decl_ptr;

}

void restore_struct_declaration (void) {

  int c_struct_ptr;

  c_struct_ptr = struct_decl_ptr - 1;

  /*
   *  All member variables should be in the struct_members
   *  list from find_struct_members ().
   */
  clear_variable ();

  if (*struct_decl[c_struct_ptr - 1].storage_class) {
    /* point to  the storage class to the label in static memory. */
    storage_class = 
      is_c_storage_class (struct_decl[c_struct_ptr].storage_class);
  }

  vars[0].name = struct_decl[c_struct_ptr].name;

  memcpy ((void *)type_decls, 
	  (void *)&(struct_decl[c_struct_ptr].type_decls),
	  (MAX_DECLARATORS * sizeof (char *)));

  type_decls_ptr = struct_decl[c_struct_ptr].type_decls_ptr;
  
  memcpy ((void *)&vars, 
	  (void *)&(struct_decl[c_struct_ptr].vars),
	  (MAX_DECLARATORS * MAXLABEL));

  n_vars = struct_decl[c_struct_ptr].n_vars;
  attrs = struct_decl[c_struct_ptr].attrs;

  struct_members = struct_decl[c_struct_ptr].members;
  struct_members_ptr = struct_decl[c_struct_ptr].members_ptr;

  memset ((void *)&struct_decl[c_struct_ptr], 0, sizeof (STRUCT_DECL));

  --struct_decl_ptr;

}

/*
 *  Called with the stack index at the opening brace of a 
 *  struct definition.
 *
 *  When finished, restores the struct declaration to the variable
 *  qualifiers, at the outermost level of a struct.
 *
 *  The inner structs get added as members to the next outer 
 *  struct declaration.
 */

int find_struct_members (MESSAGE_STACK messages, int ptr) {

  int i, j, end_ptr, n_blocks;
  MESSAGE *m;
  CVAR *mbr = NULL;  /* Avoid a warning. */
  CVAR *mbr_1 = NULL;
  bool in_inner_struct;

  /*
   *  Enum_decl () handles enums.
   */
  for (i = 0; i < type_decls_ptr; i++) {
    if (str_eq (type_decls[i], "enum"))
      return ERROR;
  }

  is_struct_decl = True;
  clear_variable ();

  end_ptr = get_stack_top (messages);

  for (i = ptr, n_blocks = 0, struct_members = struct_members_ptr = NULL; 
       i >= end_ptr; 
       i--) {

    m = messages[i];

    if (M_ISSPACE(m)) continue;

    switch (m -> tokentype)
      {
      case OPENBLOCK:
	++n_blocks;
	break;
      case CLOSEBLOCK:
	--n_blocks;
	if (!n_blocks)
	  goto done;
	break;
      case LABEL:
	mbr = NULL;
 	if (is_c_var_declaration_msg (messages, i, end_ptr, FALSE)) {
	  if ((mbr = parser_to_cvars ()) != NULL) {
	    for (mbr_1 = mbr; mbr_1; mbr_1 = mbr_1 -> next)
	      mbr_1 -> attrs |= CVAR_ATTR_STRUCT_MBR;
	    /*
	     *  If the result of a variable parse is a 
	     *  struct or a union, it is a nested struct
	     *  or union.  Add its struct members
	     *  to the CVAR, and add the CVAR to the list of
	     *  members of the next outer struct, and then
	     *  put back the outermost struct member list.
	     */
	    for (j = struct_decl_ptr - 1, in_inner_struct = False; 
		 (j >= 0) && !in_inner_struct; j--) {
	      if ((i <= struct_decl[j].block_start) &&
		  (i >= struct_decl[j].block_end)) {
		in_inner_struct = True;
		if (!struct_decl[j].members) {
		  if (mbr) {
		    struct_decl[j].members = mbr;
		    for (struct_decl[j].members_ptr = mbr; 
			 struct_decl[j].members_ptr -> next;
			 struct_decl[j].members_ptr = 
			   struct_decl[j].members_ptr -> next)
		      ;
		  }
		} else {
		  if (mbr) {
		    struct_decl[j].members_ptr -> next = mbr;
		    mbr -> prev = struct_decl[j].members_ptr;
		    for (struct_decl[j].members_ptr = mbr; 
			 struct_decl[j].members_ptr -> next; 
			 struct_decl[j].members_ptr = 
			   struct_decl[j].members_ptr -> next)
		      ;
		  }
		}
	      }
	    }
	  }
        } else {
          /* Handle a member that is an anonymous struct. */
          if (attrs & CVAR_ATTR_STRUCT_DECL) {
            int _n_type_decls;
            _new_cvar (CVARREF(mbr));
            mbr -> attrs = attrs;
            mbr -> members = struct_members;
            struct_members = NULL;
            for (_n_type_decls = type_decls_ptr - 1; _n_type_decls >= 0; 
                 _n_type_decls--) {
                 switch (_n_type_decls) 
                   {
                  case 4:
  		    strcpy (mbr -> qualifier4, type_decls[_n_type_decls]);
                    break;
                  case 3:
  		    strcpy (mbr -> qualifier3, type_decls[_n_type_decls]);
                    break;
                  case 2:
  		    strcpy (mbr -> qualifier2, type_decls[_n_type_decls]);
                    break;
                  case 1:
  		    strcpy (mbr -> qualifier, type_decls[_n_type_decls]);
                    break;
                  case 0:
  		    strcpy (mbr -> type, type_decls[_n_type_decls]);
                    break;
                   }

              }
           }
	    for (j = struct_decl_ptr - 1, in_inner_struct = False; 
		 (j >= 0) && !in_inner_struct; j--) {
	      if ((i <= struct_decl[j].block_start) &&
		  (i >= struct_decl[j].block_end)) {
		in_inner_struct = True;
		if (!struct_decl[j].members) {
		  if (mbr) {
		    struct_decl[j].members = mbr;
		    for (struct_decl[j].members_ptr = mbr; 
			 struct_decl[j].members_ptr -> next;
			 struct_decl[j].members_ptr = 
			   struct_decl[j].members_ptr -> next)
		      ;
		  }
		} else {
		  if (mbr) {
		    struct_decl[j].members_ptr -> next = mbr;
		    mbr -> prev = struct_decl[j].members_ptr;
		    for (struct_decl[j].members_ptr = mbr; 
			 struct_decl[j].members_ptr -> next; 
			 struct_decl[j].members_ptr = 
			   struct_decl[j].members_ptr -> next)
		      ;
		  }
		}
             }
           }
        }
	i = find_declaration_end (messages, i, end_ptr);
	break;
      default:
	break;
      }
  }

 done:

  return SUCCESS;
}

#define PARAM_IS_C_STORAGE_CLASS(__s) (*(__s) && is_c_storage_class(__s))

/*
 *  Idx should point to the opening parenthesis of the parameter list.
 */
int fn_param_cvars (MESSAGE_STACK messages, int idx, int is_prototype_declaration) {

  int i, j, k,
    param_start,
    param_end, 
    n_params,
    end_ptr,
    n_tags,
    have_name_tag,
    n_derefs,
    n_parens,
    is_fn_ptr,
    prev_tok,
    ellipsis;
  PARAMLIST_ENTRY *param_ptrs[MAXARGS];
  char *param_tags[MAXARGS] = {NULL,};
  CVAR *param;
  DECLARATION_CONTEXT old_context;

  end_ptr = get_stack_top (messages);

  param_start = idx;

  if ((param_end = match_paren (messages, param_start, end_ptr)) == ERROR)
    _error ("parse_error.\n");

  if (is_prototype_declaration) {
    if (fn_prototype_param_declarations (messages, param_start, param_end,
					 param_ptrs, &n_params) == ERROR) {
      check_constant_expr (messages, param_start, param_end);
      return ERROR;
    }
  } else {
    if (fn_param_declarations (messages, param_start, param_end,
			       param_ptrs, &n_params) == ERROR) {
      check_constant_expr (messages, param_start, param_end);
      return ERROR;
    }
  }
  save_fn_declaration (messages, idx);

  for (i = 0; i < n_params; i++) {

    old_context = declaration_context;
    declaration_context = decl_param_list_context;
    for (j = param_ptrs[i] -> start_ptr, n_tags = 0, n_derefs = 0,
	   n_parens = 0, have_name_tag = FALSE, is_fn_ptr = FALSE,
	   prev_tok = -1, ellipsis = FALSE;
	 j >= param_ptrs[i] -> end_ptr; j--) {
      switch (M_TOK(messages[j]))
 	{
 	case LABEL:
	  if (!n_parens) {
	    param_tags[n_tags++] = M_NAME(messages[j]);
	    if (!is_c_data_type (M_NAME(messages[j])) &&
		!get_typedef (M_NAME(messages[j])))
	      have_name_tag = TRUE;
	  } else {
	    if (is_fn_ptr) {
	      if (!is_c_data_type (M_NAME(messages[j])) &&
		  !get_typedef (M_NAME(messages[j]))) {
		if (!have_name_tag) {
		  param_tags[n_tags++] = M_NAME(messages[j]);
		  have_name_tag = TRUE;
		}
	      }
	    }
	  }
 	  break;
 	case ASTERISK:
	  if (prev_tok == OPENPAREN)
	    is_fn_ptr = TRUE;
 	  ++n_derefs;
 	  break;
	case ELLIPSIS:
	  ellipsis = TRUE;
	  break;
 	case ARRAYOPEN:
 	  ++n_derefs;
 	  break;
	case OPENPAREN:
	  ++n_parens;
	  break;
	case CLOSEPAREN:
	  --n_parens;
	  break;
 	}
      prev_tok = M_TOK(messages[j]);
    }

    _new_cvar (CVARREF(param));
    param -> attrs |= CVAR_ATTR_FN_PARAM;
    if (is_fn_ptr) param -> attrs |= CVAR_ATTR_FN_PTR_DECL;
    if (ellipsis) {
      param -> attrs |= CVAR_ATTR_ELLIPSIS;
      strcpy (param -> name, "...");
    }
    param -> n_derefs = n_derefs;

    k = n_tags - 1;
    if (have_name_tag)
      strcpy (param -> name, param_tags[k--]);
    switch (k)
      {
      case -1:
	strcpy (param -> type, "int");   /* Default. */
	break;
      case 0:
	if (PARAM_IS_C_STORAGE_CLASS(param_tags[k])) {
	  strcpy (param -> storage_class, param_tags[k--]);
	  strcpy (param -> type, "int");
	} else {
	  strcpy (param -> type, param_tags[k--]);
	}
	break;
      case 1:
	strcpy (param -> type, param_tags[k--]);
	if (PARAM_IS_C_STORAGE_CLASS (param_tags[k])) {
	  strcpy (param -> storage_class, param_tags[k--]);
	} else {
	  strcpy (param -> qualifier, param_tags[k--]);
	}
	break;
      case 2:
	strcpy (param -> type, param_tags[k--]);
	strcpy (param -> qualifier, param_tags[k--]);
	if (PARAM_IS_C_STORAGE_CLASS (param_tags[k])) {
	  strcpy (param -> storage_class, param_tags[k--]);
	} else {
	  strcpy (param -> qualifier2, param_tags[k--]);
	}
	break;
      case 3:
	strcpy (param -> type, param_tags[k--]);
	strcpy (param -> qualifier, param_tags[k--]);
	strcpy (param -> qualifier2, param_tags[k--]);
	if (PARAM_IS_C_STORAGE_CLASS (param_tags[k])) {
	  strcpy (param -> storage_class, param_tags[k--]);
	} else {
	  strcpy (param -> qualifier3, param_tags[k--]);
	}
	break;
      case 4:
	strcpy (param -> type, param_tags[k--]);
	strcpy (param -> qualifier, param_tags[k--]);
	strcpy (param -> qualifier2, param_tags[k--]);
	strcpy (param -> qualifier3, param_tags[k--]);
	if (PARAM_IS_C_STORAGE_CLASS (param_tags[k])) {
	  strcpy (param -> storage_class, param_tags[k--]);
	} else {
	  strcpy (param -> qualifier4, param_tags[k--]);
	}
	break;
      }

    if (!fn_param_decls) {
      fn_param_decls = fn_param_decls_ptr = param;
    } else {
      fn_param_decls_ptr -> next = param;
      param -> prev = fn_param_decls_ptr;
      fn_param_decls_ptr = param;
    }

    declaration_context = old_context;
    __xfree (MEMADDR(param_ptrs[i]));
  }
  restore_fn_declaration ();

  return SUCCESS;
}

/*
 *    Return the stack index of the line end, including
 *    escaped newlines.
 */

int escaped_line_end (MESSAGE **message_stack, int this_message, 
		      int last_message) {

  int i;

  for (i = this_message; i > last_message; i--) {
    if (message_stack[i] -> tokentype == NEWLINE && 
	!LINE_SPLICE (message_stack, i))
      return i;
  }

  return ERROR;
}

int typedef_var (MESSAGE_STACK messages, int keyword_ptr, int end) {

  int i;
  bool r;
  int type_idx;
  int blocklevel;
  int func_identifier = 0;      /* Avoid a warning. */
  MESSAGE *m;
  enum {
    typedef_keyword,
    typedef_type,
    typedef_tag,
  } typedef_state;

  for (i = keyword_ptr - 1, type_idx = -1, typedef_state = typedef_keyword, 
	 blocklevel = 0;
       i >= end; i--) {

    m = messages[i];

    if (M_ISSPACE (m)) continue;

    switch (m -> tokentype)
      {
      case OPENBLOCK:
	++blocklevel;
	break;
      case CLOSEBLOCK:
	--blocklevel;
	break;
      case SEMICOLON:
	if (!blocklevel) goto not_typed;
	break;
      case LABEL:
	/*
	 *  Structs and unions have separate namespaces for their
	 *  members.
	 */
	if (blocklevel) continue;

	switch (typedef_state)
	  {
	  case typedef_keyword:
	    type_idx = i;
	    typedef_state = typedef_type;
	    break;
	  case typedef_type:
	    if (!get_typedef (m -> name) &&
		!is_c_data_type (m -> name)) {
	      typedef_state = typedef_tag; 
	    }
	    break;
	  case typedef_tag:
#ifdef __GNUC__
	    if (is_gnu_extension_keyword (M_NAME(m))) {
	      int i2, attr_n_parens;
	      for (i2 = i, attr_n_parens = 0; i2 > end; i2--) {
		switch (messages[i2]->tokentype)
		  {
		  case OPENPAREN:
		    ++attr_n_parens;
		    break;
		  case CLOSEPAREN:
		    if (--attr_n_parens == 0) {
		      i = i2;
		      if (!blocklevel) goto not_typed;
		    }
		    break;
		  }
	      }
	    }
#endif
	    break;
	  }
      }
  }

 not_typed:

  for (i = keyword_ptr - 1; i > end; i--) {

    if ((messages[i] -> tokentype == WHITESPACE) ||
	(messages[i] -> tokentype == NEWLINE))
      continue;

    switch (messages[i] -> tokentype)
      {
      case LABEL:
	if (str_eq (messages[i] -> name, "enum")) {
	  add_typedef_from_cvar (enum_decl (messages, i));
	} else {
	  typedef_check = True;
	  if ((r = is_c_var_declaration_msg (messages, i, end, FALSE))
	      == True) {
	    add_typedef_from_parser ();
	  } else {
	    int __r;
	    /*
	     *  Note: The fill-in below should be qualified
	     *  by a preprocessor cache state if used to
	     *  delete duplicate typedefs.
	     */
	    for (__r = type_decls_ptr - 1; __r >= 0; __r--) {
	      if (type_decls[__r] != NULL) {
		if (is_gnuc_builtin_type (type_decls[__r]) || 
		    is_c_derived_type (type_decls[__r])) {
		  warning (messages[keyword_ptr], 
			   "Redefinition of type %s.", type_decls[__r]); 
		  return ERROR;
		}
	      }
	    }
	    if (type_idx != -1) {
	      /*
	       *  Handle types that are not complete by the end of the 
	       *  declaration.
	       */
	      add_incomplete_type (messages, keyword_ptr, type_idx);
	    } else {
	      warning (messages[keyword_ptr], "error in typedef statement.");
	    }
	  } /* else */
	  typedef_check = False;
	} /* else */
	goto done;
	break;
      case OPENPAREN:
	if (!func_identifier)
	  func_identifier = TRUE;
	break;
      case CLOSEPAREN:
	if (func_identifier)
	  goto done;
	break;
      case SEMICOLON:
	goto done;
	break;
      }
  }

 done:
  return SUCCESS;
}

/* 
 *    Don't check the state because the statement is 
 *    terminated by a newline.
 */
int parse_include_directive (MESSAGE_STACK messages, int start, int end, 
			     INCLUDE *inc) {

  int i;
  int lasttoken = start;    /* Avoid a warning. */

  for (i = start, *inc -> name = 0; i >= end; i--) {
    
    if (messages[i] -> tokentype == WHITESPACE)
      continue;
    if (messages[i] -> tokentype == NEWLINE ||
	messages[i] -> tokentype == OPENCCOMMENT)
      break;

    switch (messages[i] -> tokentype)
      {
      case LITERAL:
	strcpy (inc -> name, messages[i] -> name);
	(void)substrcpy (inc -> name, inc -> name, 1, 
			 (int)(strlen (inc -> name) - 2));
	inc -> path_type = abs_path;
	break;
      case LT:
	inc -> path_type = inc_path;
	break;
      case FWDSLASH:
      case PERIOD:
      case MINUS:
      case PLUS:
	strcatx2 (inc->name, messages[i] -> name, NULL);
	break;
      case LABEL:
	if (lasttoken == FWDSLASH || lasttoken == LT || lasttoken == PERIOD ||
	    lasttoken == MINUS || lasttoken == PLUS)
	  strcatx2 (inc -> name, messages[i] -> name, NULL);
	break;
      }
    lasttoken = messages[i] -> tokentype;
  }

  return SUCCESS;
}

int find_n_subscripts (MESSAGE_STACK messages, int ptr, int *n_subscripts) {

  int i,
    end_ptr,
    n_array_lvl;
  bool in_subscript;
  MESSAGE *m;

  end_ptr = get_stack_top (messages);

  for (i = ptr, n_array_lvl = 0, in_subscript = False, *n_subscripts = 0; 
       i > end_ptr; i--) {

    m = messages[i];

    switch (m -> tokentype)
      {
      case SEMICOLON:
      case ARGSEPARATOR:
	if (!n_array_lvl) return SUCCESS;
	break;
      case ARRAYOPEN:
	if (!in_subscript) {
	  in_subscript = True;
	  ++*n_subscripts;
	}
	++n_array_lvl;
	break;
      case ARRAYCLOSE:
	--n_array_lvl;
	if (!n_array_lvl) in_subscript = False;
	break;
      }
  }

  return SUCCESS;
}

int find_subscript_initializer_size (MESSAGE_STACK messages,
				     int array_label_idx) {
  int i, n_brackets, stack_end, subscript_start_idx, 
    subscript_end_idx;
  VAL v;
  v.__type = v.__value.__i = 0;
  stack_end = get_stack_top (messages);
  n_brackets = 0;

  subscript_start_idx = subscript_end_idx = -1;
  for (i = array_label_idx; 
       (i > stack_end) && (subscript_end_idx == -1); i--) {
    switch (M_TOK(messages[i])) 
      {
      case ARRAYOPEN:
	if (n_brackets == 0) {
	  subscript_start_idx = nextlangmsgstack (messages, i);
	  /* Empty subscript */
	  if (M_TOK(messages[subscript_start_idx]) == ARRAYCLOSE)
	    return 0;
	}
	++n_brackets;
	break;
      case ARRAYCLOSE:
	--n_brackets;
	if (n_brackets == 0) {
	  subscript_end_idx = prevlangmsgstack (messages, i);
	}
	break;
      }
  }
  if (eval_constant_expr (messages, subscript_start_idx, &subscript_end_idx,
			  &v) < 0) {
    return -1;
  } else {
    return v.__value.__i;
  }
}

int math (MESSAGE *op_msg, MATH_OP op, VAL * op1, VAL *op2, VAL *result) {

  switch (op)
    {
    case add:
      switch (op1 -> __type)
	{
	case INTEGER_T:
	  result -> __type = INTEGER_T;
	  result -> __value.__i = op1 -> __value.__i + op2 -> __value.__i;
	  break;
	case LONG_T:
	  result -> __type = LONG_T;
	  result -> __value.__l = op1 -> __value.__l + op2 -> __value.__l;
	  break;
	case LONGLONG_T:
	  result -> __type = LONGLONG_T;
	  result -> __value.__l = (long int)(op1 -> __value.__ll + op2 -> __value.__ll);
	  break;
	case FLOAT_T:
	case DOUBLE_T:
	  result -> __type = DOUBLE_T;
	  result -> __value.__d = op1 -> __value.__d + op2 -> __value.__d;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __type = LONGDOUBLE_T;
	  result -> __value.__ld = op1 -> __value.__ld + op2 -> __value.__ld;
#endif
	  break;
	default:
	  break;
	}
      break;
    case subtract:
      switch (op1 -> __type)
	{
	case INTEGER_T:
	  result -> __type = INTEGER_T;
	  result -> __value.__i = op1 -> __value.__i - op2 -> __value.__i;
	  break;
	case LONG_T:
	  result -> __type = LONG_T;
	  result -> __value.__l = op1 -> __value.__l - op2 -> __value.__l;
	  break;
	case LONGLONG_T:
	  result -> __type = LONGLONG_T;
	  result -> __value.__l = (long int)(op1 -> __value.__ll - op2 -> __value.__ll);
	  break;
	case FLOAT_T:
	case DOUBLE_T:
	  result -> __type = DOUBLE_T;
	  result -> __value.__d = op1 -> __value.__d - op2 -> __value.__d;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __type = LONGDOUBLE_T;
	  result -> __value.__ld = op1 -> __value.__ld - op2 -> __value.__ld;
#endif
	  break;
	default:
	  break;
	}
      break;
    case mult:
      switch (op1 -> __type)
	{
	case INTEGER_T:
	  result -> __type = INTEGER_T;
	  result -> __value.__i = op1 -> __value.__i * op2 -> __value.__i;
	  break;
	case LONG_T:
	  result -> __type = LONG_T;
	  result -> __value.__l = op1 -> __value.__l * op2 -> __value.__l;
	  break;
	case LONGLONG_T:
	  result -> __type = LONGLONG_T;
	  result -> __value.__l = (long int)(op1 -> __value.__ll * op2 -> __value.__ll);
	  break;
	case FLOAT_T:
	case DOUBLE_T:
	  result -> __type = DOUBLE_T;
	  result -> __value.__d = op1 -> __value.__d * op2 -> __value.__d;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __type = LONGDOUBLE_T;
	  result -> __value.__ld = op1 -> __value.__ld * op2 -> __value.__ld;
#endif
	  break;
	default:
	  break;
	}
      break;
    case divide:
      switch (op1 -> __type)
	{
	case INTEGER_T:
	  result -> __type = INTEGER_T;
	  if (!op2 -> __value.__i)
	    warning (op_msg, "Division by 0 error in math ().");
	  else
	    result -> __value.__i = op1 -> __value.__i / op2 -> __value.__i;
	  break;
	case LONG_T:
	  result -> __type = LONG_T;
	  if (!op2 -> __value.__l)
	    warning (op_msg, "Division by 0 error in math ().");
	  else
	    result -> __value.__l = op1 -> __value.__l / op2 -> __value.__l;
	  break;
	case LONGLONG_T:
	  result -> __type = LONGLONG_T;
	  if (!op2 -> __value.__ll)
	    warning (op_msg, "Division by 0 error in math ().");
	  else
	    result -> __value.__ll = (long int)(op1 -> __value.__ll / op2 -> __value.__ll);
	  break;
	case FLOAT_T:
	case DOUBLE_T:
	  result -> __type = DOUBLE_T;
	  if (!op2 -> __value.__d)
	    warning (op_msg, "Division by 0 error in math ().");
	  else
	    result -> __value.__d = op1 -> __value.__d / op2 -> __value.__d;
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  result -> __type = LONGDOUBLE_T;
	  if (!op2 -> __value.__ld)
	    warning (op_msg, "Division by 0 error in math ().");
	  else
	  result -> __value.__ld = op1 -> __value.__ld / op2 -> __value.__ld;
	  break;
#endif
	default:
	  break;
	}
      break;
      /* 
       *  TO DO - Determine if additional argument checking is
       *  necessary, and with asr, below, and whether the 
       *  arguments can be of different integer sizes.
       */
    case asl:
      switch (op1 -> __type)
	{
	case INTEGER_T:
	  result -> __type = INTEGER_T;
	  result -> __value.__i = op1 -> __value.__i << op2 -> __value.__i;
	  break;
	case LONG_T:
	  result -> __type = LONG_T;
	  result -> __value.__l = op1 -> __value.__l << op2 -> __value.__l;
	  break;
	case LONGLONG_T:
	  result -> __type = LONGLONG_T;
	  result -> __value.__l = (long int)(op1 -> __value.__ll << op2 -> __value.__ll);
	  break;
	default:
	  break;
	}
      break;
    case asr:
      switch (op1 -> __type)
	{
	case INTEGER_T:
	  result -> __type = INTEGER_T;
	  result -> __value.__i = op1 -> __value.__i >> op2 -> __value.__i;
	  break;
	case LONG_T:
	  result -> __type = LONG_T;
	  result -> __value.__l = op1 -> __value.__l >> op2 -> __value.__l;
	  break;
	case LONGLONG_T:
	  result -> __type = LONGLONG_T;
	  result -> __value.__l = (long int)(op1 -> __value.__ll >> op2 -> __value.__ll);
	  break;
	default:
	  break;
	}
      break;
    default:
      break;
    }

  return result -> __type;
}

int eval_constant_expr (MESSAGE_STACK messages, int start, int *end, 
			VAL *result) {
  int i;
  MESSAGE *m;
  CVAR *c;
  int close_paren;
  int precedence = 0;
  int conditional_end_ptr,
    sizeof_end_ptr;
  int bin_exp, idx;

  /* Resolve the symbols and numeric values in the expression. */
  for (i = start; i >= *end; i--) {

    if (parse_exception != no_x) {
      result -> __type = PTR_T;
      result -> __value.__ptr = NULL;
      return FALSE;
    }

    m = messages[i];
    switch (m -> tokentype) 
      {
      case CHAR:
	result -> __value.__i = m -> name[0] - '0';
	result -> __type = INTEGER_T;
	m_print_val (m, result);
	break;
      case LITERAL_CHAR:
	result -> __value.__i = m -> name[1] - '0';
	result -> __type = INTEGER_T;
	m_print_val (m, result);
	break;
      case LITERAL:
	result -> __value.__ptr = m -> name;
	result -> __type = PTR_T;
	m_print_val (m, result);
	++m -> evaled;
	break;
      case INTEGER:
	switch (radix_of (m -> name))
	  {
	  case decimal:
	    result -> __value.__i = atoi (m -> name);
	    break;
	  case octal:
	    result -> __value.__i = strtol (m -> name, NULL, 8);
	    break;
	  case hexadecimal:
	    result -> __value.__i = strtol (m -> name, NULL, 16);
	    break;
	  case binary:
	    for (idx = strlen(m -> name) - 1, bin_exp = 1, i = 0; 
		 idx >= 0; idx--) {
	      if ((m -> name[idx] == 'b') || (m -> name[idx] == 'B'))
		continue;
	      i += (m -> name[idx] == '1') ? bin_exp: 0;
	      bin_exp *= 2;
	    }
	    result -> __value.__i = i;
	    break;
	  }
	result -> __type = INTEGER_T;
	m_print_val (m, result);
	++m -> evaled;
	break;
      case LONG:
	result -> __value.__l = atol (m -> name);
	result -> __type = LONG_T;
	m_print_val (m, result);
	++m -> evaled;
	break;
      case LONGLONG:
	result -> __value.__ll = atoll (m -> name);
	result -> __type = LONGLONG_T;
	m_print_val (m, result);
	++m -> evaled;
	break;
      case SIZEOF:
	handle_sizeof_op (messages, i, &sizeof_end_ptr, result);
	i = sizeof_end_ptr;
	break;
      case FLOAT:
	result -> __type = DOUBLE_T;
	result -> __value.__d = strtod(M_NAME(m), (char **)NULL);
	m_print_val (m, result);
	++m -> evaled;
	break;
      case LABEL:
	if (is_c_keyword ( m -> name)) {
	  /* FIXME !
	     Check for stray keywords that are not arguments
	     to sizeof (). */
	} else {
	  if ((c = get_local_var (m -> name)) == NULL) {
	    if ((c = get_typedef (m -> name)) == NULL) {
	      warning (m, "Undefined symbol, \"%s.\"", m -> name);
	      parse_exception = parse_error_x;
	    } /* if (c = get_typedef... */
	  } /* if (c = get_local_var... */
	} /* if (is_c_keyword) */
	break;
      default:
	break;
      }
  }

  re_eval: for (i = start; i >= *end; i--) {

    if (parse_exception != no_x)
      goto done;

    m = messages[i];

      switch (m -> tokentype) 
	{
	case SEMICOLON:
	  goto done;
	  break;
	case NEWLINE:
	  break;
	case CHAR:
	case LITERAL_CHAR:
	case LITERAL:
	case INTEGER:
	case LONG:
	case LONGLONG:
	case DOUBLE:
	case WHITESPACE:
	case PREPROCESS_EVALED:
	case RESULT:
	case LABEL:
	  break;
	default:
	  if (op_precedence (precedence, messages, i)) { 
	    switch (m -> tokentype)
	      {
	      case OPENPAREN:
		if ((close_paren = match_paren (messages, i,
						  get_stack_top (messages)))
		    == ERROR) {
		  parse_exception = mismatched_paren_x;
		  warning (messages[i], "Mismatched parentheses.");
		  goto done;
		}
		(void)constant_subexpr (messages, i, &close_paren, result);
		i = close_paren;
		break;
	      case CLOSEPAREN:  /* Matching parens are tagged as evaled by the 
				   parser on recursion. */
		parse_exception = mismatched_paren_x;
		warning (messages[i], "Mismatched parentheses.");
		goto done;
		break;
		/* Handle math operators */
	      case PLUS:
	      case MINUS:
	      case MULT:
	      case DIVIDE:
	      case ASL:
	      case ASR:
	      case BIT_AND:
	      case BIT_COMP:
	      case BIT_OR:
	      case BIT_XOR:
	      case MODULUS:
		eval_math (messages, i, result);
		++m -> evaled;
		break;
		/* Handle operators that return boolean values. */
	      case BOOLEAN_EQ:
	      case GT:
	      case LT:
	      case GE:
	      case LE:
	      case LOG_NEG:
	      case INEQUALITY:
	      case BOOLEAN_AND:
	      case BOOLEAN_OR:
		(void)eval_bool (messages, i, result);
		++m -> evaled;
		break;
	      case CONDITIONAL:
		/* This is the only ternary op that we need
		   to evaluate at the moment, so evaluate on 
		   its own. */
		(void)question_conditional_eval (messages, i, 
					   &conditional_end_ptr, result);
		i = conditional_end_ptr;
		++m -> evaled;
		break;
	      case SIZEOF:
		handle_sizeof_op (messages, i, &sizeof_end_ptr, result);
		i = sizeof_end_ptr;
		break;
		/* Catch errors of operators that assign values. */
	      case INCREMENT:
	      case DECREMENT:
	      case EQ:
	      case ASR_ASSIGN:
	      case ASL_ASSIGN:
	      case PLUS_ASSIGN:
	      case MINUS_ASSIGN:
	      case MULT_ASSIGN:
	      case DIV_ASSIGN:
	      case BIT_AND_ASSIGN:
	      case BIT_OR_ASSIGN:
	      case BIT_XOR_ASSIGN:
		parse_exception = parse_error_x;
		assignment_in_constant_expr_warning (messages, start, *end);
		goto done;
		break;
	      default:
		break;
	      }
	  }
	  break;
	}  /* switch (m -> tokentype) */
  }

  /* TO DO -
   * Progressing to the next precedence relies too much on 
   * setting subexpressions to evaled token types.  Use the
   * eval message member, or find a way to step through 
   * precedence levels without changing the token type.
   */
  for (; precedence < 13; ++precedence) {
    for (i = start; i >= *end; i--) {
      if (op_precedence (precedence, messages, i)  &&
	  (messages[i] -> evaled == 0))
	goto re_eval;
    }
  }

 done:
  if (parse_exception == parse_error_x) {
    return -1;
  } else {
    return is_val_true (result);
  }
}

int fn_has_gnu_attribute (MESSAGE_STACK messages, int close_paren_idx) {
#ifdef __GNUC__
  int next_token_idx;

  if ((next_token_idx = nextlangmsg (messages, close_paren_idx)) != ERROR) {
    if (M_TOK(messages[next_token_idx]) == LABEL)
	return is_gnu_extension_keyword (M_NAME(messages[next_token_idx]));
  }
#endif
  return FALSE;
}

int fn_is_declaration (MESSAGE_STACK messages, int arglist_start_idx) {

  int arglist_end_idx;
  int next_tok_idx;
  int next_tok_type;

  if ((arglist_end_idx = match_paren (messages, arglist_start_idx,
				      get_stack_top (messages))) != ERROR) {
    if ((next_tok_idx = nextlangmsg (messages, arglist_end_idx)) != ERROR) {
      next_tok_type = M_TOK(messages[next_tok_idx]);
#ifdef __GNUC__
      if ((next_tok_type == OPENBLOCK) ||
	  (next_tok_type == SEMICOLON) ||
	  ((next_tok_type == LABEL) && 
	   fn_has_gnu_attribute (messages, arglist_end_idx)))
	return TRUE;
#else
      if ((next_tok_type == OPENBLOCK) ||
	  (next_tok_type == SEMICOLON))
	return TRUE;
#endif
    }
  }
  return FALSE;
}

/*
 *  If calling on another token than OPENPAREN, then this needs to
 *  be modified suitably.
 */
int param_is_fnptr (MESSAGE_STACK messages, int idx, int n_parens) {

  int prev_tok_idx, next_tok_idx;
  int next_close_paren_idx;

  if ((prev_tok_idx = prevlangmsg (messages, idx)) == ERROR)
    return FALSE;

  /*
   *  If the previous token is a closing paren, then its a (*fn)().
   */
  if (M_TOK(messages[prev_tok_idx]) == CLOSEPAREN)
    return TRUE;

  /*
   *  Otherwise, check for the token after the matching close
   *  paren.
   */
  if ((next_close_paren_idx = match_paren (messages, idx, 
					  get_stack_top (messages))) 
      == ERROR)
    return FALSE;
  
  if ((next_tok_idx = nextlangmsg (messages, next_close_paren_idx)) == ERROR)
    return FALSE;

  if (M_TOK(messages[next_tok_idx]) == OPENPAREN)
    return TRUE;

  return FALSE;
}

int is_c_prefix_op (MESSAGE_STACK messages, int idx) {

  int next_tok;
  CVAR *c;

  if (!IS_C_UNARY_MATH_OP(M_TOK(messages[idx])))
    return FALSE;

  if ((next_tok = nextlangmsg (messages, idx)) == ERROR)
    return FALSE;

  if (((c = get_local_var (M_NAME(messages[next_tok]))) != NULL) ||
      global_var_is_declared (M_NAME(messages[next_tok])))
    return TRUE;
  
  return FALSE;
}

