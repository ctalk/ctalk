/* $Id: fnbuf.c,v 1.6 2019/11/20 21:08:03 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2015, 2017-2019 Robert Kiesling, rk3314042@gmail.com.
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
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "break.h"

extern int library_input;                    /* Declared in class.c.      */
extern int error_line, error_column;         /* Declared in errorloc.c.   */
extern char input_source_file[FILENAME_MAX]; /* Declared in main.c.       */

extern bool argblk;                          /* Declared in argblk.c.     */
extern ARGBLK *argblks[MAXARGS + 2];
extern int argblk_ptr;

static char *fn_buf = NULL;
static int fn_buf_length = FALSE;
static int need_main_init = FALSE;
static int in_main_args = FALSE;
static int need_main_args = FALSE;
bool unbuffer_fn_call = false;  /* used by is_c_fn_declaration_msg,
				   so we can tell that we don't 
				   need to collect the fn params
				   there */

bool fn_is_builtin = false;   /* set by unbuffer_function_output
					for functions that contain
					"__builtin_", or 
					is_c_fn_declaration_msg for
					gnuc_builtins, cleared by
					fn_return_at_close */

int fn_has_argblk = FALSE;

int buffer_function_output = FALSE;
int fn_defined_by_header = FALSE;

static int fn_start_line; /* Because functions are only buffered for source */
                          /* files, we should only have to record the line */
                          /* of the function start.  TO DO - Add line info  */
                          /* for each function_buffer[] statement.          */

extern int nolinemarker_opt; /* Declared in main.c                          */
extern I_PASS interpreter_pass;

FN_DECLARATOR declarators[10];  /* Declared in cparse.c.             */
extern int n_declarators;

char fn_name[MAXLABEL] = {0,};

LIST *cvar_tab_members = NULL,
  *cvar_tab_members_head = NULL;
LIST *cvar_tab_init = NULL,
  *cvar_tab_init_head = NULL;

#define FN_BUF_BLKSIZE MAXMSG * 2

int get_fn_start_line (void) {
  return fn_start_line;
}

char *get_fn_name (void) {
  return fn_name;
}

void begin_function_buffer (void) {
  char reg_call_buf[MAXLABEL];
  /* 
   *    Don't reset the pointer if already buffering; e.g., from
   *    the main () start message, determined in the first pass.
   */
  if (!buffer_function_output) {
    buffer_function_output = TRUE;
    fn_start_line = error_line;
    fn_buf = __xalloc (FN_BUF_BLKSIZE);
    fn_buf_length = FN_BUF_BLKSIZE;
    need_main_init = in_main_args = fn_has_argblk = FALSE;
    strcpy (fn_name, declarators[n_declarators - 1].name);
#ifdef FUNCTION_BREAK
    if (str_eq (fn_name, FUNCTION_BREAK)) {
      printf (FN_BREAK_MSG, fn_name);
      asm("int3;");
    }
#endif    
    strcatx (reg_call_buf, REGISTER_USER_FN, "(", "\"", fn_name,
	     "\"", ");\n", NULL);
    global_init_statement (reg_call_buf, 3);
#ifdef __APPLE__
    if (input_is_c_header () ||
#ifdef __x86_64
	is_apple_i386_math_builtin (fn_name) || /***/
#else
#  ifdef __ppc__
	is_apple_ppc_math_builtin (fn_name) || /***/
#  endif  
#endif
	strstr (fn_name, "__builtin_") ||
	strstr (fn_name, "__inline_") ||
	!strncmp (fn_name, "__darwin_", 9))
      fn_defined_by_header = TRUE;
#else
    if (input_is_c_header () ||
	strstr (fn_name, "__builtin_") ||
	strstr (fn_name, "__inline_"))
      fn_defined_by_header = TRUE;
#endif
  }
}

void end_function_buffer (void) {
  buffer_function_output = FALSE;
  unbuffer_function_output ();
  delete_local_c_object_references ();
  delete_global_c_object_references ();
  memset (fn_name, 0, MAXLABEL);
}

void buffer_function_statement (char *s, int parser_idx) {
  if (!library_input && (interpreter_pass != c_fn_pass)) {
    if (ARGBLK_TOK(parser_idx)) {
      buffer_argblk_stmt (s);
    } else {
      if ((strlen (fn_buf) + strlen (s)) > fn_buf_length) {
	fn_buf_length *= 2;
	fn_buf = realloc (fn_buf, fn_buf_length);
      }
      if (!strcmp (s, "main")) {
	need_main_init = TRUE;
	need_main_args = TRUE;
	strcatx2 (fn_buf, s, NULL);
      } else {
	if (need_main_args) {
	  if (!strcmp (s, "(")) {
	    need_main_args = FALSE;
	    in_main_args = TRUE;
	    strcatx2 (fn_buf, s, NULL);
	    if ((strlen (fn_buf) + strlen (MAIN_ARGS)) > fn_buf_length) {
	      fn_buf_length *= 2;
	      fn_buf = realloc (fn_buf, fn_buf_length);
	    }
	    strcatx2 (fn_buf, MAIN_ARGS, NULL);
	  } else {
	    strcatx2 (fn_buf, s, NULL);
	  }
	} else {
	  if (in_main_args) {
	    if (!strcmp (s, ")")) {
	      in_main_args = FALSE;
	      strcatx2 (fn_buf, s, NULL);
	    }
	  } else {
	    strcatx2 (fn_buf, s, NULL);
	  }
	}
      }
    }
  }
}

MESSAGE *fn_messages[P_MESSAGES+2]; /* Function message stack.      */
                                           /* P_MESSAGES+2 avoids an       */
                                           /* overrrun issue on some older */
                                           /* gcc's.                       */
int fn_message_ptr = P_MESSAGES;    /* Stack pointer.               */

MESSAGE **fn_message_stack (void) { return fn_messages; }

int get_fn_messageptr (void) { return fn_message_ptr; }

int fn_message_push (MESSAGE *m) {
  if (fn_message_ptr == 0) {
    _warning (_("fn_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef FN_STACK_TRACE
  debug (_("fn_message_push %d. %s."), fn_message_ptr, m -> name);
#endif
  fn_messages[fn_message_ptr--] = m;
  return fn_message_ptr + 1;
}

MESSAGE *fn_message_pop (void) {
  MESSAGE *m;
  if (fn_message_ptr > P_MESSAGES) {
    _warning (_("fn_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("fn_message_pop %d. %s."), fn_message_ptr, 
	 fn_messages[fn_message_ptr+1] -> name);
#endif
  if (((fn_message_ptr + 1) <= P_MESSAGES) &&
      fn_messages[fn_message_ptr + 1] && 
      IS_MESSAGE(fn_messages[fn_message_ptr + 1])) {
    m = fn_messages[fn_message_ptr + 1];
    fn_messages[++fn_message_ptr] = NULL;
    return m;
  } else {
    fn_messages[++fn_message_ptr] = NULL;
    return NULL;
  }
}

/* 
 *  Collect the interpreted function, retokenize, insert the 
 *  init header, and output.
 */

void unbuffer_function_output (void) {

  int i,
    stack_start,
    stack_end;
  int old_error_line,
    old_error_column,
    var_decl_end, 
    lookahead;
#ifndef __DJGPP__
  char linebuf[FILENAME_MAX];
#endif
  bool need_init;
  enum {
    fn_decl,
    fn_var_decl,
    fn_body,
    fn_null
  } fn_state;

  /* 
   *  TO DO - 
   *  Figure out how the line markers work in DJGPP 
   *  before using them.  
   */

#ifndef __DJGPP__
  if (interpreter_pass == parsing_pass && !input_is_c_header ()) {
    if (!nolinemarker_opt && !fn_defined_by_header) {
      fmt_line_info (linebuf, fn_start_line, input_source_file, 1, TRUE);
      __fileout (linebuf);
    }
  }
#endif

  old_error_line = error_line;
  old_error_column = error_column;

  error_line = fn_start_line;
  error_column = 1;

  /*  NOTE -
   *  The function closure may not be buffered, because 
   *  parser_pass uses it to end the function buffering 
   *  and call this function.  
   *  
   *  It may be output separately after the function is
   *  output.
   */

  /* TO DO -
     Make sure that "main" is always found, even if it is not in
     its own buffer. */
  stack_start = P_MESSAGES;

  if (fn_buf == NULL)
    return;

#ifdef __GNUC__
  /* If these are documented, then we can check for them here.
     (Note that the buffer does not include the closing brace.) */
  if (strstr (fn_buf, "__builtin_")) {
    __fileout (fn_buf);
    __xfree (MEMADDR(fn_buf));
    fn_is_builtin = true;
    return;
  }
#endif

  if ((stack_end = tokenize_no_error (fn_message_push, fn_buf)) == ERROR) {
    __xfree (MEMADDR(fn_buf));
    return;
  }
  __xfree (MEMADDR(fn_buf));

  for (i = stack_start, fn_state = fn_null, need_init = True;
       i >= stack_end; i--) {

    switch (fn_messages[i] -> tokentype)
      {
      case LABEL:
      case CTYPE:
	/*
	 *  TO DO - The first clause can sometimes
	 *  be taken after a random identifier.  Try to 
	 *  locate the exceptions, or try fix 
	 *  is_c_fn_declaration_msg (). 
	 *
	 *  Note that is_c_var_declaration_msg () can change a 
	 *  C keyword's token to CTYPE, so handle those here
	 *  also.
	 */
	var_decl_end = ERROR;
	unbuffer_fn_call = true;
	if (is_c_fn_declaration_msg (fn_messages, i, stack_end)) {
 	  for ( ; fn_messages[i] && IS_MESSAGE (fn_messages[i]) &&
			fn_messages[i] -> tokentype != OPENBLOCK; i--) 
 	    __fileout (fn_messages[i] -> name);
	  /* if (fn_messages[i] && IS_MESSAGE (fn_messages[i])) { */
	    /* This should be the opening brace of the function */
	  __fileout (fn_messages[i] -> name);
	    /* } */
	} else {
	  if ((fn_state != fn_body) &&
	      is_c_var_declaration_msg (fn_messages, i, stack_end, FALSE)) {
	    var_decl_end = find_declaration_end (fn_messages, i, stack_end);

	    /*
	     *  After exiting from this loop, i is set incorrectly
	     *  for the next iteration.  It gets reset correctly
	     *  at the break, below.
	     */
 	    for ( ; (i >= var_decl_end) && fn_messages[i]; i--)
	      __fileout (fn_messages[i] -> name);

	    fn_state = fn_var_decl;
	    /*
	     * Look ahead to the next language message.  If the token
	     * does not begin a variable declaration, output the init
	     * block immediately.
	     *
	     * It should be okay to change fn_state to fn_body here,
	     * but this loop's states have been kept lazy, so there 
	     * are several tests whether the init block should be 
	     * output, and generate the block if the loop doesn't 
	     * recognize a construct.
	     */
 	    if ((lookahead = nextlangmsg (fn_messages, i + 1)) != ERROR) {
 	      if (!is_c_var_declaration_msg (fn_messages, lookahead,
					     stack_end, FALSE)) {
  		if (need_init) {
 		  need_init = False;
		  if (!fn_defined_by_header) {
		    generate_init (need_main_init);
		  }
#ifndef __DJGPP__
		  if (interpreter_pass == parsing_pass) {
		    if (!nolinemarker_opt && !fn_defined_by_header) {
		      fmt_line_info (linebuf, 
				     fn_messages[i] -> error_line, 
				     input_source_file, 1, FALSE);
		      __fileout (linebuf);
		    }
		  }
#endif
 		}
 	      }
 	    }
	  } else {
	    if (fn_state != fn_body) {
	      fn_state = fn_body;
	      if (need_init) {
		need_init = False;
		if (!fn_defined_by_header) {
		  generate_init (need_main_init);
		}
#ifndef __DJGPP__
		if (interpreter_pass == parsing_pass) {
		  if (!nolinemarker_opt && !fn_defined_by_header) {
		    fmt_line_info (linebuf, 
				   fn_messages[i] -> error_line, 
				   input_source_file, 1, FALSE);
		    __fileout (linebuf);
		  }
		}
#endif
	      }
	    }
	    /*
	     *  Handle a return block here, to make sure that 
	     *  it is output after the init block, even in 
	     *  an empty function.
	     */
	    if (!strcmp (fn_messages[i] -> name, "return") ||
		!strcmp (fn_messages[i] -> name, "exit")) {
 	      char buf[MAXMSG];
	      if (!fn_defined_by_header) {
		sprintf (buf, "\n{ __ctalk_exitFn (%d); }\n",
			 ((need_main_init) ? TRUE : FALSE));
		__fileout (buf);
	      }
#ifndef __DJGPP__
	      if (interpreter_pass == parsing_pass) {
		if (!nolinemarker_opt && !fn_defined_by_header) {
		  fmt_line_info (linebuf, 
				 fn_messages[i] -> error_line, 
				 input_source_file, 1, FALSE);
		  __fileout (linebuf);
		}
	      }
#endif
	    }
	    __fileout (fn_messages[i] -> name);
	  }
	}
	/*
	 *  If we needed to look past a var declaration above,
	 *  make sure that the stack pointer is set correctly
	 *  for the next iteration.
	 */
	unbuffer_fn_call = false;
 	if (var_decl_end != ERROR)
 	  i = var_decl_end;
	break;
	/*
	 *  A function body can begin with a prefix op.
	 */
      case ASTERISK:
      case AMPERSAND:
      case PLUS:
      case MINUS:
      case EXCLAM:
      case BIT_COMP:
      case SIZEOF:
      case OPENBLOCK:  /* Should be the same action as the prefix ops. */
	if (fn_state != fn_body) {
	  fn_state = fn_body;
	  if (need_init) {
	    need_init = False;
	    generate_init (need_main_init);
#ifndef __DJGPP__
	    if (interpreter_pass == parsing_pass) {
	      if (!nolinemarker_opt && !fn_defined_by_header) {
		fmt_line_info (linebuf, 
			       fn_messages[i] -> error_line, 
			       input_source_file, 1, FALSE);
		__fileout (linebuf);
	      }
	    }
#endif
	  }
	}
	__fileout (M_NAME(fn_messages[i]));
	break;
      case NEWLINE:
	++error_line; error_column = 1;
	__fileout (fn_messages[i] -> name);
	break;
      case PREPROCESS:
	if (fn_messages[i-4]) {
	  if (strstr (M_NAME(fn_messages[i-4]), FIXUPROOT)) {
	    i -= 4;
	    continue;
	  } else {
	    __fileout (fn_messages[i] -> name);
	  }
	  /* Line markers already factor in the newlines they
	     add. */
	  error_line = atoi (M_NAME(fn_messages[i-2]));
	} else {
	  error_line = atoi (M_NAME(fn_messages[i-2]));
	  __fileout (fn_messages[i] -> name);
	}
	break;
      default:
	__fileout (fn_messages[i] -> name);
	break;
      }
  }

  /*
   *  If the function is unbuffered and we still haven't output the
   *  init or exit blocks, output them here, before the frame with the
   *  function closure is output.
   */

  if (need_init) {
    need_init = False;
    generate_init (need_main_init);
  }

  REUSE_MESSAGES(fn_messages,fn_message_ptr,P_MESSAGES)

  error_line = old_error_line;
  error_column = old_error_column;
}

/*
 *  If the function contains a return statement, then
 *  unbuffer_function_output () has output __ctalk_exitFn ()
 *  immediately before it.  
 *
 *  Parser_pass () calls fn_return_at_close () on the frame 
 *  that contains the closing brace of the function, which is 
 *  not included in the buffered function output.
 *
 *  This is a kludge.  There should be a way to keep track
 *  of function info the same way that METHOD types keep track
 *  of methods.  However, if there is a control structure at
 *  the end of a function, and no return, then the parser
 *  would see the last statement within the final clause of the
 *  control structure.  So the parser has to wait until the
 *  final closing brace is output.
 *
 *  Instead, for the moment, the function only looks back through 
 *  two semicolons for a return statement.  If a return statement 
 *  exists, then __ctalk_exitFn () was output when the function
 *  was unbuffered.  If there is no return statement, output
 *  __ctalk_exitFn () here.
 *
 *  FIXME! - Also make sure that there are __ctalk_exitFn () blocks 
 *  for all return statements within the function.
 */
int fn_return_at_close (MESSAGE_STACK messages, int ptr, int main_exit) {

  int i,
    n_semicolons;
  bool have_return;

  if (interpreter_pass == method_pass)
    return SUCCESS;

  if (fn_is_builtin) {
    fn_is_builtin = false;
    return SUCCESS;
  }
  
  for (i = ptr, n_semicolons = 0, have_return = False; 
       (i <= P_MESSAGES) && n_semicolons < 2; i++) {
    if (messages[i] -> tokentype == SEMICOLON) {
      ++n_semicolons;
    } else if (M_TOK(messages[i]) == CLOSEBLOCK) {
      break;
    } else if (!strcmp (messages[i] -> name, "return") ||
	       !strcmp (messages[i] -> name, "exit")) {
      have_return = True;
    }
  }

  if (!have_return)
    generate_fn_return (messages, ptr, ((main_exit) ? TRUE : FALSE));
  return SUCCESS;
}

extern int is_apple_i386_libkern_builtin (const char *);
extern int is_apple_ppc_libkern_builtin (const char *);

int input_is_c_header (void) {
  char *dot;
  if (((dot = rindex (__source_filename (), '.')) != NULL)&&
      (*(dot + 1) == 'h')) {
    return TRUE;
  } else {
#ifdef __APPLE__
# ifdef __ppc__
    if (is_apple_ppc_libkern_builtin (fn_name)) {
      return TRUE;
    }
# else
    if (is_apple_i386_libkern_builtin (fn_name)) {
      return TRUE;
    }
# endif
#endif
  }
  return FALSE;
}

void function_vartab_statement (char *buf) {

  LIST *l;

  if (interpreter_pass == parsing_pass) {

    l = new_list ();
    l -> data = (void *)strdup (buf);

    if (cvar_tab_members == NULL) {
      cvar_tab_members = l;
    } else {
      cvar_tab_members_head -> next = l;
      l -> prev = cvar_tab_members_head;
    }
    cvar_tab_members_head = l;
  }
}

void function_vartab_init_statement (char *buf) {

  LIST *l;

  if (interpreter_pass == parsing_pass) {

    l = new_list ();
    l -> data = (void *)strdupx (buf);

    if (cvar_tab_init == NULL) {
      cvar_tab_init = l;
    } else {
      cvar_tab_init_head -> next = l;
      l -> prev = cvar_tab_init_head;
    }
    cvar_tab_init_head = l;
  }
}

void unbuffer_function_vartab (void) {
  LIST *l;
  generate_vartab (cvar_tab_members);
  while ((l = list_unshift (&cvar_tab_members)) != NULL) {
    __xfree (MEMADDR(l -> data));
    __xfree (MEMADDR(l));
  }
  cvar_tab_members = 
    cvar_tab_members_head = NULL;
}

/* Check for a function with a body that is only a return
   keyword followed by a C expression.  Then mark it as being
   defined in a header file, so we can treat it as not needing
   any Ctalk expressions. */
bool chk_C_return_alone (MESSAGE_STACK messages, int opening_brace_idx) {
  int lookback, return_keyword_idx, i;
  bool have_object = false;
  CFUNC *fn;
  if ((return_keyword_idx = nextlangmsgstack (messages, opening_brace_idx))
      != ERROR) {
    if ((M_TOK(messages[return_keyword_idx]) == LABEL) &&
	!strcmp (M_NAME(messages[return_keyword_idx]), "return")) {
      if ((fn = get_function (fn_name)) != NULL) {
	for (i = return_keyword_idx - 1; M_TOK(messages[i]) != SEMICOLON; --i) {
	  if (M_TOK(messages[i]) == LABEL) {
	    if (!is_c_data_type (M_NAME(messages[i])) &&
		!is_this_fn_param (M_NAME(messages[i])) &&
		!get_global_var (M_NAME(messages[i]))) {
	      /* also check for struct members */
	      if ((lookback = prevlangmsgstack (messages, i)) != ERROR) {
		if ((M_TOK(messages[lookback]) != PERIOD) &&
		    (M_TOK(messages[lookback]) != DEREF)) {
		  have_object = true;
		}
	      }
	    }
	  }
	}
	if (!have_object) {
	  fn_defined_by_header = true;
	  return true;
	}
      }
    }
  }
  return false;
}

