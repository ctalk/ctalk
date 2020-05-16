/* $Id: preprocess.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2011, 2014, 2016-2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include "ctpp.h"
#include "typeof.h"

#define _error_out _warning

#define SIMPLE_MACRO 1          /* Used by define_symbol */

extern char source_file[];        /* Defined in main.c.                 */
extern int nolinemarker_opt;      /* Declared in lib/rtinfo.c.          */
extern int nolinemarkerflags_opt;
extern int keep_pragma_opt;
extern int keepcomments_opt;
extern int definestooutput_opt;
extern int definenamestooutput_opt;
extern int definesonly_opt;
extern int definestofile_opt;
extern int definenamesonly_opt;
extern int warn_all_opt;
extern int warnunused_opt;
extern int makerule_opts;
extern int print_headers_opt;
extern int no_include_opt;
extern int no_simple_macros_opt;
extern char *systeminc_dirs[];
extern int n_system_inc_dirs;
extern int move_includes_opt;

extern int constant_expr_eval_lvl;  /* Declared in cexpr.c and set by
				       eval_constant_expr ().            */

extern int error_line;              /* Declared in lex.c                 */

extern int rescanning;              /* Declared in lex.c.                */

extern EXCEPTION parse_exception;   /* Declared in pexcept.c.            */

extern int preprocess_stack_start;  /* p_message_ptr when starting to    */
                                    /* tokenize -include <files>.        */
                                    /* Declared in i_opt.c.              */
extern int sol10_gcc_sfw_mod;       /* Declared in ccompat.c.            */

extern int n_configure_include_dirs; /* From ccompat.c */

bool sub_parsing = False;
bool message_parsing = False;

MESSAGE *p_messages[P_MESSAGES+1]; /* Preprocess message stack.        */
int p_message_ptr = P_MESSAGES;    /* Preprocess stack pointer.        */

MESSAGE *t_messages[P_MESSAGES +1];/* Temporary stack for include and  */
int t_message_ptr = P_MESSAGES;    /* macro tokenization.              */

MESSAGE *a_messages[P_MESSAGES +1];/* Temporary stack for assertion    */
int a_message_ptr = P_MESSAGES;    /* expressions.                     */

MESSAGE *i_messages[P_MESSAGES +1];  /* Temporary stack for includes.  */
int i_message_ptr = P_MESSAGES;

MESSAGE *ni_messages[P_MESSAGES +1];  /* Temporary stack for no_include parsing.  */
int ni_message_ptr = P_MESSAGES;

MESSAGE *v_messages[P_MESSAGES +1];  /* Temporary stack for varargs and */
int v_message_ptr = P_MESSAGES;      /* (expr) ? (true) : (false)       */
                                     /* conditionals.                   */

extern DEFINITION *macro_assertions;/* Declared in assert.c.           */
extern DEFINITION *last_assertion;

/*@null@*/
DEFINITION *self_definition = NULL;   /* The macro being replaced.     */

extern HASHTAB macrodefs;             /* Declared in hash.c.           */
extern HASHTAB ansisymbols;

extern int line_info_line;            /* Declared in lineinfo.c.       */

extern int warndollar_opt;

#ifndef HAVE_OFF_T
extern int input_size;              /* Declared in lib/read.c.       */
#else
extern off_t input_size;
#endif

int stack_start (MESSAGE_STACK messages) {
  if ((messages == p_messages) || (messages == t_messages) ||
      (messages == a_messages) || (messages == i_messages) ||
      (messages == v_messages) || (messages == ni_messages))
    return P_MESSAGES;
  _error (_("stack_start: unimplemented message stack."));
  /* Avoid warning message. */
  return ERROR;
}

/*
 *  Return the index of the next stack entry.
 */
int get_stack_top (MESSAGE_STACK messages) {
  if (messages == p_messages)
    return p_message_ptr;
  if (messages == t_messages)
    return t_message_ptr;
  if (messages == a_messages)
    return a_message_ptr;
  if (messages == i_messages)
    return i_message_ptr;
  if (messages == ni_messages)
    return ni_message_ptr;
  if (messages == v_messages)
    return v_message_ptr;
  _error (_("get_stack_top: unimplemented message stack."));
  /* Avoid warning message. */
  return ERROR;
}

/*
 *  Return the index of the next stack entry.
 */
int get_stack_ptr (MESSAGE_STACK messages, int **stack_ptr_ptr) {
  if (messages == p_messages) {
    *stack_ptr_ptr = &p_message_ptr;
    return p_message_ptr;
  }
  if (messages == t_messages) {
    *stack_ptr_ptr = &t_message_ptr;
    return t_message_ptr;
  }
  if (messages == a_messages) {
    *stack_ptr_ptr = &a_message_ptr;
    return a_message_ptr;
  }
  if (messages == i_messages) {
    *stack_ptr_ptr = &i_message_ptr;
    return i_message_ptr;
  }
  if (messages == ni_messages) {
    *stack_ptr_ptr = &ni_message_ptr;
    return ni_message_ptr;
  }
  if (messages == v_messages) {
    *stack_ptr_ptr = &v_message_ptr;
    return v_message_ptr;
  }
  _error (_("get_stack_top: unimplemented message stack."));
  /* Avoid warning message. */
  return ERROR;
}

int p_message_push (MESSAGE *m) {
  if (p_message_ptr == 0) {
    _warning (_("p_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("p_message_push %d. %s."), p_message_ptr, m -> name);
#endif
  p_messages[p_message_ptr--] = m;
  return p_message_ptr + 1;
}

MESSAGE *p_message_pop (void) {
  MESSAGE *m;
  if (p_message_ptr == P_MESSAGES) {
    _warning (_("p_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("p_message_pop %d. %s."), p_message_ptr, 
	 p_messages[p_message_ptr+1] -> name);
#endif
  if (p_messages[p_message_ptr + 1] && 
      IS_MESSAGE(p_messages[p_message_ptr + 1])) {
    m = p_messages[p_message_ptr + 1];
    p_messages[++p_message_ptr] = NULL;
    return m;
  } else {
    p_messages[++p_message_ptr] = NULL;
    return NULL;
  }
}

int t_message_push (MESSAGE *m) {
  if (t_message_ptr == 0) {
    _warning (_("t_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("t_message_push %d. %s."), t_message_ptr, m -> name);
#endif
  t_messages[t_message_ptr--] = m;
  return t_message_ptr + 1;
}

MESSAGE *t_message_pop (void) {
  MESSAGE *m;
  if (t_message_ptr == P_MESSAGES) {
    _warning (_("t_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("t_message_pop %d. %s."), t_message_ptr, 
	 t_messages[t_message_ptr+1] -> name);
#endif
  if (t_messages[t_message_ptr+1] && 
      IS_MESSAGE(t_messages[t_message_ptr+1])) {
    m = t_messages[t_message_ptr+1];
    t_messages[++t_message_ptr] = NULL;
    return m;
  } else {
    t_messages[++t_message_ptr] = NULL;
    return NULL;
  }
}

int a_message_push (MESSAGE *m) {
  if (a_message_ptr == 0) {
    _warning (_("a_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("a_message_push %d. %s."), a_message_ptr, m -> name);
#endif
  a_messages[a_message_ptr--] = m;
  return a_message_ptr + 1;
}

MESSAGE *a_message_pop (void) {
  MESSAGE *m;
  if (a_message_ptr == P_MESSAGES) {
    _warning (_("a_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("a_message_pop %d. %s."), a_message_ptr, 
	 a_messages[a_message_ptr+1] -> name);
#endif
  if (a_messages[a_message_ptr+1] && 
      IS_MESSAGE(a_messages[a_message_ptr+1])) {
    m = a_messages[a_message_ptr+1];
    a_messages[++a_message_ptr] = NULL;
    return m;
  } else {
    a_messages[++a_message_ptr] = NULL;
    return NULL;
  }
}

int i_message_push (MESSAGE *m) {
  if (i_message_ptr == 0) {
    _warning (_("i_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("i_message_push %d. %s."), i_message_ptr, m -> name);
#endif
  i_messages[i_message_ptr--] = m;
  return i_message_ptr + 1;
}

MESSAGE *i_message_pop (void) {
  MESSAGE *m;
  if (i_message_ptr == P_MESSAGES) {
    _warning (_("i_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("i_message_pop %d. %s."), i_message_ptr, 
	 i_messages[i_message_ptr+1] -> name);
#endif
  if (i_messages[i_message_ptr+1] && 
      IS_MESSAGE(i_messages[i_message_ptr+1])) {
    m = i_messages[i_message_ptr+1];
    i_messages[++i_message_ptr] = NULL;
    return m;
  } else {
    i_messages[++i_message_ptr] = NULL;
    return NULL;
  }
}

int ni_message_push (MESSAGE *m) {
  if (ni_message_ptr == 0) {
    _warning (_("ni_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("ni_message_push %d. %s."), ni_message_ptr, m -> name);
#endif
  ni_messages[ni_message_ptr--] = m;
  return ni_message_ptr + 1;
}

MESSAGE *ni_message_pop (void) {
  MESSAGE *m;
  if (ni_message_ptr == P_MESSAGES) {
    _warning (_("ni_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("ni_message_pop %d. %s."), ni_message_ptr, 
	 ni_messages[ni_message_ptr+1] -> name);
#endif
  if (ni_messages[ni_message_ptr+1] && 
      IS_MESSAGE(ni_messages[ni_message_ptr+1])) {
    m = ni_messages[ni_message_ptr+1];
    ni_messages[++ni_message_ptr] = NULL;
    return m;
  } else {
    ni_messages[++ni_message_ptr] = NULL;
    return NULL;
  }
}

int v_message_push (MESSAGE *m) {
  if (v_message_ptr == 0) {
    _warning (_("v_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("v_message_push %d. %s."), v_message_ptr, m -> name);
#endif
  v_messages[v_message_ptr--] = m;
  return v_message_ptr + 1;
}

MESSAGE *v_message_pop (void) {
  MESSAGE *m;
  if (v_message_ptr == P_MESSAGES) {
    _warning (_("v_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("v_message_pop %d. %s."), v_message_ptr, 
	 v_messages[v_message_ptr+1] -> name);
#endif
  if (v_messages[v_message_ptr+1] && 
      IS_MESSAGE(v_messages[v_message_ptr+1])) {
    m = v_messages[v_message_ptr+1];
    v_messages[++v_message_ptr] = NULL;
    return m;
  } else {
    v_messages[++v_message_ptr] = NULL;
    return NULL;
  }
}

static void init_args (char **args, int n) {

    int i;
    for (i = 0; i < n; i++)
      args[i] = NULL;
}

static void init_macro_args (MACRO_ARG **args, int n) {

  int i;
  for (i = 0; i < n; i++)
    args[i] = NULL;
}

static void free_args (char **args) {
   int i;
   for (i = 0; args[i]; i++) {
     if (args[i]) free (args[i]);
     args[i] = NULL;
   }
}

static void free_macro_args (MACRO_ARG **args) {
  int i;
  for (i = 0; args[i]; i++)
    free (args[i]);
}

MACRO_ARG *new_arg (void) {
  MACRO_ARG *m;
  if ((m = (MACRO_ARG *)calloc (1, sizeof (struct _macro_arg))) == NULL)
    _error ("new_arg: %s.\n", strerror (errno));
  return m;
}

static inline int preproc_stmt (MESSAGE_STACK messages, int idx) {
  return messages[prevlangmsg (messages, idx)] -> tokentype == PREPROCESS;
}

/*
 *  Take care that d is allocated by calloc (),
 *  so that arg pointers will be NULL even 
 *  if not explicitly initialized.
 */
int n_definition_params (DEFINITION *d) {
  int i;
  
  for (i = 0; d -> m_args[i]; i++)
    ;
  return i;
}

/*
 *  Return the argument index of the ellipsis if the macro takes a 
 *  variable number of arguments, or error.
 */
int macro_is_variadic (DEFINITION *d) {

  int i;

  for (i = 0; d -> m_args[i]; i++)
    if (str_eq (d -> m_args[i] -> name, "..."))
      return i;

  return ERROR;
}


/* 
 * Add a #define'd symbol to the list of defined macro symbols.
 */
DEFINITION *add_symbol (char *name, char *value, MACRO_ARG **m_args) {

  DEFINITION *macro_symbol;
  int arg = 0;

  if ((macro_symbol = 
       (DEFINITION *)calloc (1, sizeof (struct _macro_symbol)))
      == NULL)
    _error (_("add_symbol: %s."), strerror (errno));

  macro_symbol -> sig = MACRODEF_SIG;

  strcpy (macro_symbol -> name, name);

  if (*value)
    strcpy (macro_symbol -> value, value);
  
  while (m_args[arg]) {
    macro_symbol -> m_args[arg] = new_arg ();
    strcpy (macro_symbol -> m_args[arg] -> name, m_args[arg] -> name);
    macro_symbol -> m_args[arg] -> paste_mode = m_args[arg] -> paste_mode;
    arg++;
  }

  _hash_put (macrodefs, macro_symbol, mbasename(macro_symbol -> name));

  return macro_symbol;
}

INCLUDE *includes[MAXARGS + 1] = {NULL,};
					  
extern int include_ptr;

/* 
 *    Nested if levels and indexes into if_vals stack, below.
 *    Declared before first use.
 */
static int if_level = 0;    /* Level of nested #ifs.                     */
static int result_ptr = 0;

static bool conditional_line = False;
static bool pragma_line = False;
static bool preprocess_line = False;

static COND expr;

#define TOP_LEVEL_INCLUDE (include_ptr == (MAXARGS + 1))
#define INCLUDE_LEVEL ((MAXARGS + 1) - include_ptr)

static COND_EXPR if_vals[MAXARGS+1];

void adj_include_stack (int n) {

  int i;

  for (i = include_ptr; i <= MAXARGS; i++)
    includes[i] -> s_end += n;
}

int include_return (int msg_ptr) {

  INCLUDE *include;
  int line_info_length,
    include_line,
    adj;

  if (include_ptr > MAXARGS) return 0;
  if (constant_expr_eval_lvl) return 0;

  if (msg_ptr < includes[include_ptr] -> s_end) {

#ifdef TRACE_PREPROCESS_INCLUDES
    debug (_("RETURN %s. Message %d line %d."),
	   includes[include_ptr] -> name,
	   msg_ptr, p_messages[msg_ptr] -> error_line);
    location_trace (p_messages[msg_ptr]);
#endif

    include = includes[include_ptr];
    includes[include_ptr++] = NULL;

    if (include_ptr <= MAXARGS) 
      __source_file (includes[include_ptr] -> path);
    else
      __source_file (source_file);

    if (!nolinemarker_opt) {
      /*
       *  GCC, at least version 3.3, thinks that 
       *  the return line marker should be inserted 
       *  on the line after the line where the 
       *  include statement occured in the text.  This 
       *  seems to work okay, without inserting any 
       *  extra newlines.  See line_info (), below.
       *
       *  GCC, the later versions, also close up
       *  a lot of white space and insert line 
       *  markers to increment the line number
       *  accordingly, which we ignore.  The
       *  scheme here, where we try to keep the
       *  same number of newlines in the output,
       *  seems to work well with all versions
       *  of GNU C.
       *
       *  If we place the return line marker at the 
       *  last newline of the include file, some
       *  versions of GCC issue a, "line 0," after
       *  lots of error messages.  
       */

#ifdef __GNUC__
      adj = -1;
      if (msg_ptr + adj <= p_message_ptr) {
        /*
         *  If the #include is the last directive in
         *  the input, don't go past the end of the 
         *  stack.  TO DO - Decide if we should 
         *  make a line number past the end of
         *  the input, depending on what 
         *  circumstances it would matter for.
         */
        include_line = p_messages[msg_ptr] -> error_line;
      } else {
        include_line = p_messages[msg_ptr + adj] -> error_line;
      }
#else
      adj = 0;
      include_line = include -> error_line;
#endif

      free (include->buf);
      free (include);

      if ((M_TOK(p_messages[msg_ptr]) == PREPROCESS) &&
	  (M_TOK(p_messages[msg_ptr-2]) == INTEGER)) { /***/
	/* Don't put a line marker in the middle of another line
	   marker. This needs more testing */
	for (adj = 1; (msg_ptr - adj) > p_message_ptr; adj++) {
	  if (M_TOK(p_messages[msg_ptr - adj]) == NEWLINE) {
	    break;
	  } 
	}
      }

      line_info_length = line_info (msg_ptr + adj, __source_filename (), 
				      include_line, 2);
      adj_include_stack (-line_info_length);
      return line_info_length;
    } else {

      free (include);
      return 0;

    }
  }

  return 0;
}

/* static int idx = 0; *//***/ /* for the debugging below */

int line_info (int p, char *file, int line, int flag) {

  int i,
    first,
    last,
    new_line;
  char buf[MAXMSG];

  if (nolinemarker_opt)
    return 0;

#if 0 /***/
  /* might be needed yet */
  if (idx == 699)
    asm("int3;");

  printf ("%d %s\n", idx++, file);
#endif

  /* 
   *  Note that the macro parser depends on this exact 
   *  spacing when checking for a line number directive; 
   *  i.e., messages['#' - 2] is the line number, and 
   *  messages['#' - 4] is the file name.
   *
   */

  /*
   *  The line number is incremented at the new line.  If 
   *  we need to place a line marker at the end of a non
   *  blank line, we have to insert a newline in front
   *  of the line marker so it begins at column 1.
   *
   *  The case with GCC is that it seems to place the line
   *  markers on the line after the point of the original
   *  #include directive.  There should be cases where it
   *  would be necessary to add a newline after the line
   *  marker, but none have occurred so far.
   */
  if (p < P_MESSAGES)
    new_line = (p_messages[p + 1] -> tokentype != NEWLINE) ? TRUE : FALSE;
  else
    new_line = FALSE;

  first = t_message_ptr = P_MESSAGES;
  line_info_line = TRUE;

  if (nolinemarkerflags_opt) {
    last = tokenize_reuse (t_message_push,
		     fmt_line_info (line, file, 0, new_line, buf));
  } else {
    last = tokenize_reuse (t_message_push,
		     fmt_line_info (line, file, flag, new_line, buf));
  }

  line_info_line = FALSE;

  insert_stack2 (p_messages, p, t_messages, P_MESSAGES, last, &p_message_ptr);

  for (i = last; i <= first; i++) {
    reuse_message (t_message_pop ());
    t_messages[i] = NULL;
  }

  return first - last + 1;
}

/*
 *  Do the actual work of including the file.  Line number lines
 *  are included in the length of the inclusion.
 */

void include_to_stack (MESSAGE_STACK messages, INCLUDE *include, char *path,
		      int start, int end) {

  int i;
  int r_stat;
  size_t include_chars_read,
    prev_input_size;
  int include_size,
    end_msg;
  int line_info_length;
  struct stat statbuf;

  if ((r_stat = stat (path, &statbuf)) != 0)
    error (messages[start], _("Include file %s: %s."), path, strerror (errno));

  strcpy (include -> path, path);

  include_chars_read = read_file (&(include->buf), path);

  /*
   *  Tokenize () needs this, in case it needs to allocate buffers
   *  for messages that pass comments through to the output, using
   *  the -C option.  In those cases, tokenize () needs to know the
   *  size of the include file, not the source file.
   */
  prev_input_size = input_size;
  input_size = include_chars_read;

  /*
   * If no_include_opt is set, we don't want to modify the original message
   * stack so we replace it with ni_messages.  We don't need line directives.
   */
  if (no_include_opt && messages == p_messages) {

    VAL result;

    memset ((void *)&result, 0, sizeof (VAL));

    end_msg = tokenize_reuse (ni_message_push, include -> buf);

    input_size = prev_input_size;

    include -> s_start = P_MESSAGES; /* Top of ni_messages */

    include -> s_end = end_msg;

#ifdef DEBUG_CODE
    if (include_ptr != MAXARGS)
      debug (_("no_include_opt should only be triggered at top-level."));
#endif

    (void)macro_parse (ni_messages, P_MESSAGES, &end_msg, &result);

    for (i = end_msg; i <= P_MESSAGES; i++) {
      reuse_message (ni_message_pop ());
      ni_messages[i] = NULL;
    }

    include->s_end = P_MESSAGES;
    return;

  } else { /* no_include_opt */

    /* Skip the stack splicing when there's no include directive 
       to replace; e.g., when including the standard library 
       headers, because it adjusts the stack pointer. */
    if (start != end)
      splice_stack (messages, start, end - 1);

    line_info_length = line_info (start, include -> path, 1, 1);

    t_message_ptr = P_MESSAGES;

    end_msg = tokenize_reuse (t_message_push, include -> buf);

    input_size = prev_input_size;

    include -> s_start = start;

    include_size = P_MESSAGES - end_msg;

    /* Insert the file's tokens after the line info. */
    insert_stack (messages, include -> s_start - line_info_length, 
                  t_messages, P_MESSAGES, end_msg);

    for (i = end_msg; i <= P_MESSAGES; i++) {
      reuse_message (t_message_pop ());
      t_messages[i] = NULL;
    }

    include -> s_end = include -> s_start - include_size - line_info_length;

    for (i = include_ptr + 1; i <= MAXARGS; i++) {
      includes[i] -> s_end -= include_size;
      includes[i] -> s_end -= line_info_length;
      /* Don't count the replaced #include directive. */
      includes[i] -> s_end += start - end;
    }
  }
}

static void print_include (char *path) {

  int i;

  for (i = include_ptr + 1; i <= MAXARGS; i++)
    _error_out ("\t");
  _error_out (path);
  _error_out ("\n");

}

int include_file (MESSAGE_STACK messages, int start, int end) {

  INCLUDE *include,
    *prev_include;
  struct stat statbuf;
  char path[FILENAME_MAX * 2],
    prev_path[FILENAME_MAX];
  char *path_ptr;
  int r_stat;
  
  /*
   *   Save the calling *.h file's states if necessary.
   */
  if (!TOP_LEVEL_INCLUDE) {
    prev_include = includes[include_ptr];
    includes[include_ptr] -> expr = expr;
    includes[include_ptr] -> result_ptr = result_ptr;
    includes[include_ptr] -> message_ptr = p_message_ptr;

    /* 
     *  Reset the if states here because conditionals in
     *  the input file need to maintain the states between
     *  calls to macro_parse ().
     */
  } else {
    prev_include = NULL;
  }

  if ((include = (INCLUDE *)calloc (1, sizeof (INCLUDE))) == NULL)
    _error (_("include_file: %s."), strerror (errno));

  include -> error_line = messages[start]->error_line;

  parse_include_directive (messages, start, end, include);

  if (include -> path_type == inc_path) {

    if ((path_ptr = find_include (include -> name, FALSE)) == NULL) {
      warning (messages[start], 
	       _("Include file %s: File not found."), include -> name);
      location_trace (messages[start]);
#ifdef DEBUG_SYMBOLS
      dump_symbols ();
#endif
      exit (1);
    } else {
      strcpy (path, path_ptr);
    }
  } else {
    /* 
     * If a quoted filename, first check for the file in the 
     * directory of the previous include, or if there is no
     * previous include, in the path as given, then the include 
     * paths.  However, if the filename specifies a path, then
     * use that.
     */
    if (prev_include) {
      char *path_end;
      path_end = rindex (prev_include -> path, '/');
      ++path_end;
      (void)substrcpy (prev_path, prev_include -> path, 
		 0, path_end - prev_include -> path);
      if (*(include -> name) == '/') { /* Absolute path. */
	strcpy (path, include -> name);
      } else {                         /* Relative path. */
	SNPRINTF (path, FILENAME_MAX * 2, "%s%s", prev_path, include -> name);
      }
    } else {
      SNPRINTF (path, FILENAME_MAX + 3, "./%s", include -> name);
    }
    /* If the include isn't in ., check the include path. */
    if ((r_stat = stat (path, &statbuf)) != 0) {
      if ((makerule_opts & MAKERULE) &&
	  (makerule_opts & MAKERULEGENHEADER)) {
	if ((path_ptr = find_include (include -> name, FALSE)) == NULL) {
	  include_dependency (path);
	  free (include);
	  return SUCCESS;
	} else {
	  strcpy (path, path_ptr);
	}
      } else {
	if ((path_ptr = find_include (include -> name, FALSE)) 
	    == NULL) {
	  warning (messages[start], 
		   _("Include file %s: File not found."), include -> name);
	  location_trace (messages[start]);
#ifdef DEBUG_SYMBOLS
	  dump_symbols ();
#endif
	  exit (1);
	} else {
	  strcpy (path, path_ptr);
	}
      } /* if ((r_stat = stat (path, &statbuf)) != 0) */
    }
  }

  if (print_headers_opt) print_include (path);

  includes[--include_ptr] = include;

  include_to_stack (messages, include, path, start, end);

#ifdef TRACE_PREPROCESS_INCLUDES
  debug (_("INCLUDE %s"), include -> name);
  location_trace (messages[start]);
#endif

  if (makerule_opts & MAKERULE) {
    switch (include -> path_type)
      {
      case abs_path:
	include_dependency (include -> path);
	break;
      case inc_path:
	if (!(makerule_opts & MAKERULELOCALHEADER))
	  include_dependency (include -> path);
	break;
      }
  }

  if (include_ptr <= MAXARGS)
    ansi__FILE__ (includes[include_ptr] -> name);
  else
    ansi__FILE__ (source_file);

  return SUCCESS;
}

/*
 * This is a no-op for compilers other than GNU C, where
 * it is unavoidable when including system headers.
 */

extern char *searchdirs[];         /* Defined in ccompat.c */

int include_next_file (MESSAGE_STACK messages, int start, int end) {

#ifdef __GNUC__

  INCLUDE *include,
    *prev_include;
  struct stat statbuf;
  char path[FILENAME_MAX * 2 + 1];
  char prev_path[FILENAME_MAX];
  char *path_end;
  int r_stat;
  int i;

  if (TOP_LEVEL_INCLUDE) {
    warning (messages[start], 
	     _("include_next without previous include."));
    return include_file (messages, start, end);
  }

  if (warn_all_opt)
    warning (messages[start], 
       "The, \"#include_next,\" directive is an extension to C99.");

  /*
   *   Save the calling *.h file's states if necessary.
   */
  if (!TOP_LEVEL_INCLUDE) {
    prev_include = includes[include_ptr];

    includes[include_ptr] -> expr = expr;
    includes[include_ptr] -> result_ptr = result_ptr;
    includes[include_ptr] -> message_ptr = p_message_ptr;

    /* 
     *  Reset the if states here because conditionals in
     *  the input file need to maintain the states between
     *  calls to macro_parse ().
     */
  } else {
    prev_include = NULL;
  }

  if ((include = (INCLUDE *)calloc (1, sizeof (INCLUDE))) == NULL)
    _error (_("include_file: %s."), strerror (errno));

  include -> error_line = messages[start]->error_line;
  include -> s_start = p_message_ptr;

  parse_include_directive (messages, start, end, include);

  includes[--include_ptr] = include;

  ansi__FILE__ (include -> name);

  /* 
   * #include_next uses the same search for 
   * <filename> and "filename" arguments.
   */

  path_end = rindex (prev_include -> path, '/');
  substrcpy (prev_path, prev_include -> path, 0, 
	     path_end - prev_include -> path);
  if (str_eq (prev_path, ".")) { /* Use the current directory. */
    sprintf (path, "%s/%s", prev_path, include -> name);

    if ((r_stat = stat (include -> name, &statbuf)) != 0) { 

      for (i = 0, *path = 0, r_stat = ERROR; i < n_configure_include_dirs; 
	   i++) {
	sprintf (path, "%s/%s", searchdirs[i], include -> name);
	if ((r_stat = stat (include -> name, &statbuf)) == 0)
	  break;
      }
      
    }
  } else {
    for (i = 0; i < n_configure_include_dirs; i++) {
      if (str_eq (searchdirs[i], prev_path))
	break;
    }
    ++i;  /* Next search directory. */

    for ( *path = 0; i < n_configure_include_dirs; i++) {
      sprintf (path, "%s/%s", searchdirs[i], include -> name);
      if ((r_stat = stat (path, &statbuf)) == 0)
	break;
    }
  }

  if (! *path) {
    ++include_ptr;
    free (include);
    for (i = start; i >= end; i--) {
      strcpy (messages[i]->name, " ");
      messages[i] -> tokentype = WHITESPACE;
    }
    return ERROR;
  }

  if (print_headers_opt) print_include (path);

  include_to_stack (messages, include, path, start, end);

#endif /* __GNUC__ */

  return TRUE;

}

int macro_sub_parse (MESSAGE_STACK messages, int start, int *end, 
		     VAL *result) {

  bool p_preprocess_line;
  bool sub_parser_val;

  sub_parser_val = sub_parsing;
  sub_parsing = True;
  p_preprocess_line = preprocess_line;

  (void)macro_parse (messages, start, end, result);
  preprocess_line = p_preprocess_line;
  sub_parsing = sub_parser_val;
  return result -> __type;
}

/* TO DO - 
 *  Move COND_BRANCH into parser.h when the preprocessor is finalized.
 */

#define COND_BRANCH \
       (conditional_line || \
        sub_parsing || \
        (if_level == 0) || \
        ((expr == cond_expr || expr == cond_expr_n || \
          expr == cond_expr_ifdef || expr == cond_expr_ifndef || \
          expr == cond_expr_elif || expr == cond_expr_else) && \
	 (if_vals[if_level].val == if_val_true)))


int eval_conditional (MESSAGE *orig, int val) {

  int result = val;

  /* 
   *     The handler for #ifndef has already inverted the logical
   *     value of the argument, so #if, #ifdef, and #ifndef can 
   *     be treated the same here.
   */
  if (expr == cond_expr || expr == cond_expr_n || expr == cond_expr_ifdef ||
      expr == cond_expr_ifndef) {

    if_vals[if_level].expr = expr;
    /* The preceding conditional must be true and 
       this conditional must be true. */
    if ((if_vals[if_level - 1].val == if_val_true) || (if_level == 1)) {
      if_vals[if_level].val = (result == FALSE) ? if_val_false : if_val_true;
    } else {
      if_vals[if_level].val = if_val_false;
    }
  }

  if (expr == cond_expr_elif) {

    switch (if_vals[if_level].expr)
      {
      case cond_expr_else:
      case cond_expr_endif:
	new_exception (elif_w_o_if_x);
	break;
      case cond_expr_ifdef:
      case cond_expr_ifndef:
	/*
	 *  Solaris 2.10 headers have this construct, so maybe ctpp
	 *  shouldn't use this exception.
	 */
	/* new_exception (elif_after_ifdef_x); */
	break;
      case cond_expr_elif:
      case cond_expr:
      case cond_expr_n:
      case null_expr:
	break;
      }

    /* 
     *  val must be true, if must have failed, and, if the 
     *  condition is nested within another condition, the 
     *  outer conditional must be true. 
     */

    if_vals[if_level].expr = cond_expr_elif;

      if ((if_vals[if_level - 1].val == if_val_true) ||
	  if_level == 1) {

	switch (if_vals[if_level].val)
	  {
	  case if_val_true:
	    if_vals[if_level].val = if_val_already_true;
	    break;
	  case if_val_false:
	    if_vals[if_level].val = (val) ? if_val_true : if_val_false;
	    break;
	  case if_val_already_true:
	  case if_val_undef:
	    break;
	  }
      }
  }  /*  if (expr == cond_expr_elif) */

  if (expr == cond_expr_else) {
    if ((if_vals[if_level].expr == cond_expr_endif) ||
	(if_vals[if_level].expr == cond_expr_else)) {
      new_exception (else_w_o_if_x);
    }

    /* If statement must have failed and the preceding conditional
       must be true. */
    /* If there is a preceding if expression. */
    if (if_vals[if_level - 1].expr != null_expr) {
      if_vals[if_level].expr = cond_expr_else;
      if (if_vals[if_level - 1].val == if_val_true) {

	if_vals[if_level].val = 
	  ((if_vals[if_level].val == if_val_true) ||
	   (if_vals[if_level].val == if_val_already_true)) ? 
	  if_val_false : if_val_true;

      } else {
	if_vals[if_level].val = if_val_false;
      }
    } else { /* First level of conditional nesting. */
      if_vals[if_level].expr = cond_expr_else;

	if_vals[if_level].val = 
	  ((if_vals[if_level].val == if_val_true) ||
	   (if_vals[if_level].val == if_val_already_true)) ? 
	  if_val_false : if_val_true;
    }
  }

  if (expr == cond_expr_endif) {

    if_vals[if_level].expr = cond_expr_endif;

    if (--if_level < 0) {
      new_exception (endif_w_o_if_x);
      if_level = 0;
    }

    expr = if_vals[if_level].expr;
  }

  return result;
}

/*
 *   Find the end of a preprocessor statement - the index of the
 *   token immediately before the newline.
 */
int end_of_p_stmt (MESSAGE_STACK messages, int msg_ptr) {

  int i = msg_ptr;
  while (1) {
    /*
     *  Check for input that is not terminated by a newline.
     */
    if (!messages[i - 1] || !IS_MESSAGE (messages[i - 1]))
      break;
    if (messages[i - 1] -> tokentype == NEWLINE 
	&& !LINE_SPLICE (messages, i - 1))
      break;
    i--;
  }
  return i;
}

static int sub_fn_tag (MESSAGE_STACK messages, int idx, int *end, 
		       VAL *result) {
  DEFINITION *d;
  int expr_end_orig, expr_end_new, r, i;

  i = idx;
  if ((d = get_symbol (M_NAME(messages[idx]),
		       FALSE)) != NULL) {
    if (message_parsing && warn_all_opt) 
      warning (messages[i], 
	       _("Macro substitution in message text is an extension to standard C."));
    result -> __type = INTEGER_T; result -> __value.__i = TRUE;
    if (d -> m_args[0]) {
      expr_end_orig = expr_end_new = 
	match_paren (messages, i, get_stack_top (messages));
      replace_args (d, messages, i, &expr_end_new);
    } else {
      expr_end_orig = expr_end_new = i;
      if (!no_simple_macros_opt) {
	if ((r = replace_macro (d, messages, i, &expr_end_new))
	    == 1) {
	  ++i;
	  *end -= expr_end_orig - expr_end_new;
	  return i;
	}
      }
    }
    if (parse_exception == success_x) {
      i = expr_end_new;
      *end -= expr_end_orig - expr_end_new;
    }
    if (parse_exception == argument_parse_error_x) {
      i = expr_end_new;
      if (expr_end_orig > expr_end_new) 
	*end -= expr_end_orig - expr_end_new;
    }
    return i;
  }
  return i;
}


/* 
 *  Preprocess the tokenized input file - include header files,
 *  process and remove preprocessor directives, and include
 *  declarations from the header files.
 */

int macro_parse (MESSAGE_STACK messages, int start, int *end, VAL *result) {

  int i,
    label_ptr,
    preprocess_ptr = -1;
  int lastlangtoken = -1;
  int macro_result;
  int return_adj;
  int keyword;
  int lookahead, j;
  bool output_define_directive;  /* Used with definetooutput_opt. */
  MESSAGE *m; 
  DEFINITION *d;

  for (i = start, 
	 preprocess_line = False,
	 output_define_directive = False; i >= *end; i--) {

#ifdef DEBUG_CODE
    if ((*end != p_message_ptr + 1) && !sub_parsing)
      debug (_("stack_end_mismatch."));
#endif

    if (messages[i] -> tokentype == WHITESPACE)
      continue;

    i -= exception_adj (messages, i);

    m = messages[i];

    ansi__LINE__ (m -> error_line);

    /* Adjust for line info when returning to a previous include. */
    if ((return_adj = include_return (i)) != 0) {
      *end -= return_adj;
      /* Line map fix 1. */
      if (include_ptr > MAXARGS) {
	/* It's messages[i] -> error_line + 1, because the
	   line marker line refers to the line following the
	   line marker. We just insert the line marker here
	   without any stack adjustment. */
	*end -= line_info (i - return_adj, 
			   source_file, messages[i] -> error_line + 1,
			   1);
      }
      i += 1;
      continue;
    }

    switch (m -> tokentype)
      {
      case LABEL:

	if (warndollar_opt)
	  if (index (m -> name, '$')) warning (m, "'$' in identifier.");

	/* 
	 *   Check macro keywords first. Check also for a 
	 *   PREPROCESS token prefix and that the keyword
	 *   is a preprocess statement.
	 */

	if (((keyword = is_macro_keyword (m -> name)) != 0) && 
	    (lastlangtoken == PREPROCESS)) {

	  if (keyword == KEYWORD_IF) {
 	    int expr_start, expr_end_orig, expr_end_new, line_change;
	    ++if_level;
	    expr = cond_expr;
	    conditional_line = True;
	    /* The next token is whitespace, so allow the 
	       expression parser to elide it and deal with whatever
	       follows in the expression. */
	    expr_start = i - 1;
	    expr_end_orig = expr_end_new = i = end_of_p_stmt (messages, i);
	    (void)eval_constant_expr (messages, expr_start, &expr_end_new, result);
	    i =  expr_end_new;
	    line_change = expr_end_orig - expr_end_new;
	    *end -= line_change;
	    continue;
	  }

	  if (keyword == KEYWORD_IFDEF) {
 	    ++if_level;
	    expr = cond_expr_ifdef;
	    conditional_line = True;
 	    macro_result = handle_ifdef (messages, i, result);
	    i = end_of_p_stmt (messages, i);
	    continue;
	  }
	  
	  if (keyword == KEYWORD_IFNDEF) {
 	    ++if_level;
	    expr = cond_expr_ifndef;
	    conditional_line = True;
 	    macro_result = handle_ifndef (messages, i, result);
	    i = end_of_p_stmt (messages, i);
	    continue;
	  }

	  if (KEYWORD_CMP(m -> name, "elif")) {
 	    int start, line_end;
 	    if (lastlangtoken == PREPROCESS) {
 	      expr = cond_expr_elif;
 	      conditional_line = True;
 	    }
 	    start = nextlangmsg (messages, i);
	    i = line_end = end_of_p_stmt (messages, i);
 	    (void)eval_constant_expr (messages, start, &line_end, result);
	    if (line_end != i) {
	      *end -= i - line_end;
	      i = line_end;
	    }
	    continue;
	  }

	  if (keyword == KEYWORD_ELSE) {
	    /* if (lastlangtoken == PREPROCESS) { */
	      expr = cond_expr_else;
	      conditional_line = True;
	    /* } */
	  }

	  if (keyword == KEYWORD_ENDIF) {
	    expr = cond_expr_endif;
	    conditional_line = True;
	  }

 	  if (COND_BRANCH) {

	    if (keyword == KEYWORD_DEFINE) {
 	      macro_result = define_symbol (messages, i);
	      if (definestooutput_opt ||
                  (no_simple_macros_opt && macro_result == SIMPLE_MACRO))
		output_define_directive = True;
	      result_ptr = i = end_of_p_stmt (messages, i);
	      continue;
	    }

	    if (keyword == KEYWORD_UNDEF) {
 	      undefine_symbol (messages, i);
	      macro_result = TRUE;
	      result_ptr = i = end_of_p_stmt (messages, i);
	      continue;
	    }

	    if ((keyword == KEYWORD_INCLUDE) ||
		(keyword == KEYWORD_INCLUDE_HERE)) {
	      /* if (lastlangtoken == PREPROCESS) { */
		int j, dir_start, dir_end;
		for (j = i, dir_start = i; j <= start; j++) {
		  dir_start = j;
		  if (messages[j] -> tokentype == PREPROCESS)
		    break;
		}
		dir_end = end_of_p_stmt (messages, i);
		macro_result = include_file (messages, dir_start, dir_end);
		if (includes[include_ptr] && (!no_include_opt || include_ptr < MAXARGS)) {
		  /* 
		   *  Subtract the replaced #include directive, then
		   *  add the size in message tokens of the included file. 
		   */
		  *end += dir_start - dir_end;
		  *end -= includes[include_ptr] -> s_start - 
		    includes[include_ptr] -> s_end;
		  /* 
		   *  Move the stack index back before the point of inclusion
		   *  and continue because we need to reset m immediately.
		   */
		  i = preprocess_ptr + 1;
		} else {
		  /*
		   *  We are using the -MG option, and have
		   *  encountered a nonexistent (supposedly
		   *  generated) header file.
		   */
		  i = end_of_p_stmt (messages, i);
		}
                if (no_include_opt && include_ptr == MAXARGS) {
                  preprocess_line = False; /* so the #include line will not be removed. */
                }
		continue;
	    }

	    if (keyword == KEYWORD_INCLUDE_NEXT) {
	      if (lastlangtoken == PREPROCESS) {
		int j, dir_start, dir_end;
		for (j = i, dir_start = i; j <= start; j++) {
		  dir_start = j;
		  if (messages[j] -> tokentype == PREPROCESS)
		    break;
		}
		dir_end = end_of_p_stmt (messages, i);
		if ((macro_result = 
		     include_next_file (messages, dir_start, dir_end)) 
		    != ERROR) {
		  if (no_include_opt && include_ptr == MAXARGS) {
		    preprocess_line = False; /* so the #include line will not be removed. */
		    i = end_of_p_stmt (messages, i);
		  } else {
		    *end += dir_start - dir_end;
		    *end -= includes[include_ptr] -> s_start - 
		      includes[include_ptr] -> s_end;
		    i = preprocess_ptr + 1;
		  }
		}
                continue;
	      }

	    }

	    if (keyword == KEYWORD_ERROR)
	      preprocess_error (messages, i);  /* No return. */

	    if (keyword == KEYWORD_WARNING) {
	      *end -= preprocess_warning (messages, i);
	      i = end_of_p_stmt (messages, i);
	      continue;
	    }

	    if (keyword == KEYWORD_PRAGMA) {
	      /* TO DO - 
		 Make sure that other pragmas get passed through to 
		 the output if necessary.   Not much implemented
		 in GNUC, needs to be tested with other compilers.
	      */
	      pragma_line = True;
	      handle_pragma_directive (messages, i);
	      i = end_of_p_stmt (messages, i);
	      continue;
	    }

	    /*
	     *  FIXME!  Make sure the #line directive works!
	     */
	    if (keyword == KEYWORD_LINE) {
	      /* if (lastlangtoken == PREPROCESS) { */
		*end -= line_directive (messages, i);
		i = end_of_p_stmt (messages, i);
		continue;
	      /* } */
	    }

	    if (keyword == KEYWORD_ASSERT) {
	      define_assertion (messages, i);
	    }

#ifdef __GNUC__
	    /* 
	     *  Not defined in C99, but included here for legacy
	     *  and GNU compatibility, as no-ops.
	     */
	    if (keyword == KEYWORD_IDENT || 
		keyword == KEYWORD_SCCS ||
		keyword == KEYWORD_UNASSERT) {
	      new_exception (deprecated_keyword_x);
	      continue;
	    }
#endif

	  }  /* COND_BRANCH */

	} else { /* is_macro_keyword (m -> name) */
 	  if (COND_BRANCH) {
	    if ((d = get_symbol (m -> name, FALSE)) != NULL) {
	      int expr_end_orig, expr_end_new;
	      if (message_parsing && warn_all_opt) 
		warning (messages[i], 
		 _("Macro substitution in message text is an extension to standard C."));
	      /* 
	       * If the symbol is defined, the result is at least
	       * true, even if the replacement value is empty.
	       */
	      result -> __type = INTEGER_T; result -> __value.__i = TRUE;
	      if (d -> m_args[0]) {
 		expr_end_orig = expr_end_new = 
		  match_paren (messages, i, get_stack_top (messages));
		replace_args (d, messages, i, &expr_end_new);
	      } else {
		expr_end_orig = expr_end_new = i;
                if (!no_simple_macros_opt) {
		  int r;
                  if ((r = replace_macro (d, messages, i, &expr_end_new))
		      == 1) {
		    ++i;
		    *end -= expr_end_orig - expr_end_new;
		    continue;
		  }
		}
	      }
 	      if (parse_exception == success_x) {
		i = expr_end_new;
		*end -= expr_end_orig - expr_end_new;
 	      }
	      if (parse_exception == argument_parse_error_x) {
		i = expr_end_new;
		if (expr_end_orig > expr_end_new) 
		  *end -= expr_end_orig - expr_end_new;
	      }
	      continue;
	    } else { /* d= get_symbol != NULL */


	      if (KEYWORD_CMP(m -> name, "_Pragma") ||
		  keyword == KEYWORD_PRAGMA) {
		pragma_line = True;
		handle_pragma_op (messages, i);
		i = end_of_p_stmt (messages, i);
		continue;
	      } else {
		/* 
		 *   Insert line info at function declarations in the 
		 *   source file(s), and record the function name.  
		 *
		 *   Don't put line info on functions declared in
		 *   the include files.
		 *
		 *   is_c_func_declaration_msg () also returns false
		 *   for fn prototypes.
		 */

  		if ((is_c_func_declaration_msg (messages, i) || 
  		     is_instance_method_declaration_start (messages, i, *end))){
		  if (include_ptr >= MAXARGS) {
		    while (1) {
		      if ((lookahead = nextlangmsg (messages, i)) != ERROR) {
			i = lookahead;
			if (messages[lookahead] -> tokentype == LABEL) {
			  /*
			   *  Sub_fn_tag (hopefully) can handle 
			   *  parameter and multiple-token replacements 
			   *  in function tags.  
			   */
			  lookahead = 
			    sub_fn_tag (messages, lookahead, end, result);
			  j = nextlangmsg (messages, lookahead);
			  if (messages[j] -> tokentype == OPENPAREN) {
			    ansi__func__ (messages[lookahead] -> name);
			  }
			}
		      }
 		      if ((lookahead == ERROR) ||
 			  (messages[lookahead] -> tokentype == OPENPAREN))
 			break;
		    }
		  }
		} else {
		  result -> __type = INTEGER_T;
		  result -> __value.__i = FALSE;
		}
	      }
	    }
	  }
	}
	break;
      case MACRO_CONCAT:
	/* It's not possible to do arg checking here, so 
	   just assume we want to concatenate whatever is 
	   on each side of the operator. */
 	concatenate_tokens (messages, &i, concat_both);
	break;
      case LITERALIZE:
	literalize_token (messages, &i);
	break;
      case PREPROCESS:
	/* 
	 *  Ensure that the following tokens are a valid macro 
	 *  keyword or line info. 
	 */
	if ((label_ptr = next_label (messages, i)) == ERROR) {
	  /* 
	   *  Line number info.  The format and token order is
	   *  set in line_info ().
	   */
	  if((M_TOK(messages[i-2])!=INTEGER)||(M_TOK(messages[i-4])!=LITERAL)){
	    new_exception (undefined_keyword_x);
	  } else {
	    i = end_of_p_stmt (messages, i);
	    continue;
	  }
	} else {
	  preprocess_ptr = i;
	  preprocess_line = True;
	  if (!is_macro_keyword (messages[label_ptr] -> name)) {
	    new_exception (undefined_keyword_x);
	    handle_preprocess_exception (messages, label_ptr, start, *end);
	  }
	}
	break;
      case INTEGER:
	{
	  int int_val = atoi (M_NAME(messages[i]));
	  result -> __type = INTEGER_T;
	  result -> __value.__i = int_val;
	}
	break;
      case NEWLINE:
	if (conditional_line) {
	  conditional_line = False;
	  macro_result = eval_conditional (m, is_val_true (result));
	}
	/* Don't output preprocessor directives... */
	if ((preprocess_line && !pragma_line) || 
	    (pragma_line                     /* Unless it's a pragma that */
	     && !keep_pragma_opt)) {         /* we want to keep.          */
	  int j;
	  preprocess_line = pragma_line = False;
	  /* If not a line marker. */
	  if (messages[preprocess_ptr - 2] && 
	      (messages[preprocess_ptr - 2] -> tokentype != INTEGER)) {
 	    for (j = preprocess_ptr; j > i; j--) {
	      if (messages[j] -> tokentype == NEWLINE)
		continue;
	      if (!output_define_directive) {
		strcpy (messages[j] -> name, " ");
		messages[j] -> tokentype = RESULT;
	      }
 	    }
	    if (output_define_directive)
	      output_define_directive = False;
	  }
	} else {
	  if (!COND_BRANCH) { /* Mark  any other statements within
				 false conditional branches to prevent 
				 outputting them. */
	    int j;
	    for (j = i; messages[j+1] -> tokentype != NEWLINE; ++j)
	      ;
	    for ( ; j > i; j--) {
	      strcpy (messages[j] -> name, " ");
	      messages[j] -> tokentype = RESULT;
	    }
	  }
	}
	break;
      default:
	break;
	}

    lastlangtoken = m -> tokentype;

  }

  if (parse_exception == success_x)
    return is_val_true (result);
  else
    return ERROR;
}

void init_preprocess_if_vals (void) {
  int i;
  for (i = 0; i <= MAXARGS; i++) {
    if_vals[i].val = if_val_undef;
    if_vals[i].expr = null_expr;
  }
}

/* 
 *  This function checks both ifdef and ifndef arguments.  Handle_ifndef ()
 *  calls this function and then performs a logical inversion of the result.
 */

int handle_ifdef (MESSAGE_STACK messages, int keyword_ptr, VAL *result) {

  DEFINITION *d;
  int i;

  for (i = keyword_ptr; ; i--) {
    if ((messages[i] -> tokentype == NEWLINE) && 
	!LINE_SPLICE (messages, i))
      break;
    if (str_eq (messages[i] -> name, "ifdef") ||
	str_eq (messages[i] -> name, "ifndef"))
      continue;
    if (messages[i] -> tokentype == LABEL) {
      if (warndollar_opt)
	if (index (messages[i] -> name, '$')) 
	  warning (messages[i], "'$' in identifier.");

      if ((d = get_symbol (messages[i] -> name, FALSE)) != NULL) {
	result -> __type = INTEGER_T;
	result -> __value.__i = TRUE;
 	sprintf (messages[i] -> value, "%d", TRUE);
      } else {
	result -> __type = INTEGER_T;
	result -> __value.__i = FALSE;
 	sprintf (messages[i] -> value, "%d", FALSE);
      }
    }
  }

  ++messages[keyword_ptr] -> evaled;
  return is_val_true (result);
}

int handle_ifndef (MESSAGE_STACK messages, int keyword_ptr, VAL *result) {

  (void)handle_ifdef (messages, keyword_ptr, result);

  if (is_val_true (result)) {
    result -> __value.__i = FALSE;
  } else {
    result -> __value.__i = TRUE;
  }
  return is_val_true (result);
}

static int defined_op_states[] = {
  LABEL,             LABEL,        0,      /* 0. */
  LABEL,             NEWLINE,      0,      /* 1. */
  LABEL,             BOOLEAN_AND,  0,      /* 2. */
  LABEL,             BOOLEAN_OR,   0,      /* 3. */
  LABEL,             OPENCCOMMENT, 0,      /* 4. */
  LABEL,             OPENPAREN,    0,      /* 5. */
  OPENPAREN,         LABEL,        0,      /* 6. */
  LABEL,             CLOSEPAREN,   0,      /* 7. */
  PREPROCESS_EVALED, NEWLINE,      0,      /* 8. */
  PREPROCESS_EVALED, BOOLEAN_AND,  0,      /* 9. */
  PREPROCESS_EVALED, BOOLEAN_OR,   0,      /* 10. */
  -1, 0, 0
};

#define DEFINED_OP_STATE_COLS 3

#define DEFINED_OP_STATE(x) (check_preprocess_state ((x), \
                                    messages, \
                                    defined_op_states, \
                                    DEFINED_OP_STATE_COLS))

int handle_defined_op (MESSAGE_STACK messages, int keyword_ptr, 
		       int *operand_end, VAL *result) {

  int i;
  int close_paren_ptr;

  int state;
  int last_state;
  int last_message_ptr;
  VAL *subexpr_val;

  subexpr_val = (VAL *)calloc (1, sizeof (VAL));

  last_message_ptr = get_stack_top (messages);

  for (i = keyword_ptr, last_state = state = ERROR; 
       i > last_message_ptr; i--) {

    if (messages[i] -> tokentype == WHITESPACE)
      continue;

    state = DEFINED_OP_STATE (i);

    switch (messages[i] -> tokentype)
      {
      case LABEL:
	if (last_state == 0) {
	  if (get_symbol (messages[i] -> name, TRUE) == NULL) {
	    result -> __type = INTEGER_T;
	    result -> __value.__i = FALSE;
	  } else {
	    result -> __type = INTEGER_T;
	    result -> __value.__i = TRUE;
	  }
	  *operand_end = i;
	  goto done;
	}
	break;
      case OPENPAREN:
	close_paren_ptr = p_match_paren (messages, i, 
					 get_stack_top (messages));
	macro_subexpr (messages, i, &close_paren_ptr, subexpr_val);
	/* 
	 *  When checking if a macro is defined, we do not need
	 *  to include the argument list in the defined() statement.
	 */
	if (parse_exception == missing_arg_list_x) {
	  if (warn_all_opt) {
	    char *buf;
	    buf = collect_tokens (messages, i, close_paren_ptr);
	    warning (messages[i],
		     "Macro expression, \"%s,\" used without argument list.",
		     buf);
	    free (buf);
	  }
	  parse_exception = success_x;
	}
	result -> __type = INTEGER_T;
	result -> __value.__i = is_val_true (subexpr_val);
	*operand_end = close_paren_ptr;
	goto done;
	break;
      case PREPROCESS_EVALED:
	numeric_value (messages[i] -> value, subexpr_val);
	result -> __type = INTEGER_T;
	result -> __value.__i = (is_val_true (subexpr_val)) ? TRUE : FALSE;
 	while (messages[--i] -> tokentype == PREPROCESS_EVALED)
	  *operand_end = i;
	goto done;
  	break;
      default:
 	goto done;
      }	    
    last_state = state;
  }

 done:

  for (i = keyword_ptr; i >= *operand_end; i--) {
    messages[i] -> tokentype = PREPROCESS_EVALED;
    sprintf (messages[i] -> value, "%d", result -> __value.__i);
  }

  free (subexpr_val);

  return result -> __type;

}

static void format_message (char *buf, char *msg, int line) {
  sprintf (buf, "%s:%d: %s", 
	   TOP_LEVEL_INCLUDE ? source_file : includes[include_ptr] -> name,
	   line, msg);
}

void preprocess_error (MESSAGE_STACK messages, int msg_ptr) {
  message_parsing = True;
  preprocess_message (messages, msg_ptr);
  message_parsing = False;
#ifdef DEBUG_SYMBOLS
  dump_symbols ();
#endif
  exit (1);
}

int preprocess_warning (MESSAGE_STACK messages, int msg_ptr) {
  int stack_adj;
  message_parsing = True;
  stack_adj = preprocess_message (messages, msg_ptr);
  message_parsing = False;
  return stack_adj;
}

/*
 *  The return value of preprocess message is the stack index
 *  adjustment if there are macro substitutions in the argument.
 */

int preprocess_message (MESSAGE_STACK messages, int keyword_idx) {

  int start_idx, end_idx, end_idx_orig;
  VAL result;
  char msg[MAXMSG], buf[MAXMSG * 2];

  if (!is_macro_keyword (M_NAME (messages[keyword_idx])))
    _error ("Bad keyword %s in preprocess_message ()\n", 
	    M_NAME (messages[keyword_idx]));

  start_idx = nextlangmsg (messages, keyword_idx);
  end_idx = end_idx_orig = end_of_p_stmt (messages, keyword_idx);

  /*
   *  If the argument is a literal, trim and output it.  Otherwise,
   *  parse and replace macros if necessary, printing warnings
   *  if -Wall is used.
   */
  if ((start_idx == end_idx) && (M_TOK (messages[start_idx]) == LITERAL)) {
    strcpy (msg, M_NAME (messages[start_idx]));
    TRIM_LITERAL (msg);
  } else {
    char *__t;
    memset ((void *)&result, 0, sizeof (VAL));
    result.__type = 1;
    macro_sub_parse (messages, start_idx, &end_idx, &result);
    __t = collect_tokens (messages, start_idx, end_idx);
    strcpy (msg, __t);
    free (__t);
  }

  format_message (buf, msg, messages[start_idx] -> error_line);
  /* We assume that the newline that terminates the statement 
   * is part of the message text.
   */
  strcat (buf, "\n");
  _warning (buf);

  return end_idx_orig - end_idx;

}

/*
 *  Process an #undef preprocessor directive.  Undefine macros
 *  with the basename of the #undef argument, whether or not the
 *  macro takes parameters.  Undefine_symbol deletes all
 *  symbols with the same basename if they happen to exist.
 */

int undefine_symbol (MESSAGE_STACK messages, int keyword_ptr) {

  int label_ptr;
  int retval = ERROR;
  MESSAGE *m_label;
  DEFINITION *t;

  label_ptr = next_label (messages, keyword_ptr);

  if (label_ptr == ERROR)
    error (messages[keyword_ptr], _("Parse error."));

  m_label = messages[label_ptr];

  if ((t = 
       (DEFINITION *)_hash_remove (macrodefs, 
		   mbasename(M_NAME (m_label)))) != NULL) {
    if (t -> m_args[0]) free_macro_args (t -> m_args);
    free (t);
  }

  return retval;
}

/*
 *  Process a #define preprocessor directive.
 *  The parser does not track all possible states for macro
 *  parameters, only the states that signify the start and 
 *  end of parameter lists.
 *
 *  Note - If the symbol is defined without an argument, then
 *  the result after retrieval by get_symbol () or get_symbol ()
 *  must be set to true.  Replace_macro () replaces null values
 *  with space characters in the output.
 */

int define_symbol (MESSAGE_STACK message_stack, int keyword_ptr) {

  int i, label_ptr, line_end_ptr, end_ptr;
  int next_tok;
  int param_list_start = -1, param_list_end = -1;
  int leading_whitespace;
  bool param_list = False;     /* True if within a param list. */
  bool comment = False;
  int n_parens;
  DEFINITION *macro_symbol,
    *previous_value, *previous_value_sub;
  char name[MAXMSG], val[MAXMSG]; 
  MACRO_ARG *m_args[MAXARGS];
  int n_args;

  init_macro_args (m_args, MAXARGS);
  n_args = -1;

  label_ptr = next_label (message_stack, keyword_ptr);

  if (label_ptr == ERROR)
    error (message_stack[keyword_ptr], _("Parse error."));

  end_ptr = get_stack_top (message_stack);

  /* Sanity check, in case something (including 
     a GNUC __attribute__ label) elides. */
  for (i = keyword_ptr - 1, next_tok = ERROR; 
       (i > end_ptr) && (next_tok == ERROR); i--) {
    if (message_stack[i] -> tokentype != WHITESPACE)
      next_tok = i;
  }

  if (next_tok != label_ptr)
    return ERROR;

  /* 
   *    Collect the complete label in the case of parameterized 
   *    macros.  When finished, it should point to the space after
   *    the macro name.
   *
   */
  for (i = label_ptr, *name = 0, n_args = 0, n_parens = 0; 
       i > end_ptr; i--) {

    if ((comment == True) && (message_stack[i] -> tokentype != CLOSECCOMMENT))
      continue;

    switch (message_stack[i] -> tokentype)
      {
      case LABEL:
	if (param_list) {
	  strcat (m_args[n_args] -> name, message_stack[i] -> name);
	}
	strcat (name, message_stack[i] -> name);
	if (warnunused_opt) p_hash_loc (message_stack[i]);
	break;
      case OPENPAREN:
	if (!param_list && (param_list_end > -1))
	  goto label_done;
	if (n_parens == 0) {
	  if (param_list_start == -1) {
	    m_args[n_args] = new_arg ();
	  }
	}
	++n_parens;
	if (param_list) {
	  strcat (m_args[n_args] -> name, message_stack[i] -> name);
	}
	strcat (name, message_stack[i] -> name);
	if (param_list_start == -1) {
	  param_list_start = i;
	  param_list = True;
	}
	break;
      case CLOSEPAREN:
	--n_parens;
	if (n_parens == 0) {
	  if (param_list_end == -1) {
	    param_list = False;
	    param_list_end = i;
	  }
	} else {
	  strcat (m_args[n_args] -> name, message_stack[i] -> name);
	}
	strcat (name, message_stack[i] -> name);
	break;
      case ARGSEPARATOR:
	++n_args;
	m_args[n_args] = new_arg ();
	strcat (name, message_stack[i] -> name);
	break;
      case WHITESPACE:
	if (!param_list)
	  goto label_done;
	else
	  strcat (name, message_stack[i] -> name);
	break;
      case NEWLINE:
	if (!LINE_SPLICE (message_stack, i))
	    goto label_done;
	break;
      default:
	strcat (name, message_stack[i] -> name);
	if (param_list) {
	  strcat (m_args[n_args] -> name, message_stack[i] -> name);
	}
	break;
      }
  }


 label_done:

  if (warndollar_opt)
    if (index (message_stack[label_ptr] -> name, '$')) 
      warning (message_stack[label_ptr], "'$' in identifier.");

  if ((line_end_ptr = 
       escaped_line_end (message_stack, i, end_ptr)) == ERROR)
    line_end_ptr = end_ptr;

  /* The newline is not included in the symbol __value. */
  for (*val = 0, leading_whitespace = TRUE; i > line_end_ptr; i--) { 
    if (leading_whitespace && 
	message_stack[i] -> tokentype == OPENCCOMMENT)
      break;
    if (leading_whitespace && 
	message_stack[i] -> tokentype != WHITESPACE)
      leading_whitespace = FALSE;
    if (leading_whitespace && 
	message_stack[i] -> tokentype == WHITESPACE)
      continue;

    if (message_stack[i] -> tokentype == OPENCCOMMENT) {
      comment = True;
      continue;
    }
    if (comment && (message_stack[i] -> tokentype != CLOSECCOMMENT))
      continue;

    if (message_stack[i] -> tokentype == CLOSECCOMMENT) {
      comment = False;
      continue;
    }

    replacement_concat_modes (message_stack, i, m_args);

    strcat (val, message_stack[i] -> name);

  }

  /* Trim the trailing whitespace from the definition. */
  for (i = strlen (val) - 1; ((i >= 0) && isspace (val[i])); i--)
    val[i] = 0;

   if ((previous_value = 
	(DEFINITION *)_hash_get (macrodefs, mbasename(name))) != NULL) {
#ifdef GCC_STDINT_WRAPPER
# ifdef __APPLE__  /* Silently ignore for now. */
     if (wrapped_gcc_stdints_dup_osx (name)) {
       free_macro_args (m_args);
       return SUCCESS;
     }
# endif
#endif /* GCC_STDINT_WRAPPER */

     previous_value_sub = sub_symbol (previous_value);
     /* 
	Only create an exception if the value is different... 

	.... aand check that it's not really the same definition
	that has been rescanned....
     */
     /* this needs to use strcmp. */
     if (strcmp (val, previous_value_sub -> value) && 
	strcmp (val, previous_value -> value)) {
       undefine_symbol (message_stack, keyword_ptr);
       new_exception (redefined_symbol_x);
       handle_preprocess_exception (message_stack, label_ptr,label_ptr,end_ptr);
     } else {
       /*
	 ... duplicates get silently ignored.
       */
       undefine_symbol (message_stack, keyword_ptr);
     }
   }

#ifdef GCC_STDINT_WRAPPER
# ifdef __linux__
   if (!strncmp (name, "__intN_t", 8) ||
       !strncmp (name, "__u_intN_t", 10)) {
     fixup_wrapped_gcc_stdints_linux ();
   }
# else
#  ifdef __APPLE__
   /*
    *  Similar to the function call above, but we have it here also
    *  anyway.
    */
   if (wrapped_gcc_stdints_dup_osx (name)) {
     free_macro_args (m_args);
     return SUCCESS;
   }
#  endif /* __APPLE__ */
# endif /* __linux__ */
#endif /* GCC_STDINT_WRAPPER */

   /* 
    *  Add the macro as defined.  
    *  Substitution should occur only when the macro 
    *  is replaced.
    */
   macro_symbol = add_symbol (name, val, m_args);

  free_macro_args (m_args);

  if (macro_symbol->m_args[0])
    return SUCCESS;
  else
    return SIMPLE_MACRO;
}

/*
 *  Idx points to the token in the macro *replacement*.  When
 *  replacing the macro, token pasting occurs during the first
 *  scan, and it's easiest to determine in define_symbol (),
 *  when the entire macro is first tokenized.
 */
int replacement_concat_modes (MESSAGE_STACK messages, int idx, 
			      MACRO_ARG **args) {
  int i, next_tok_idx, prev_tok_idx;

  for (i = 0; args[i]; i++) {
    if (str_eq (messages[idx] -> name, args[i] -> name)) {
      if ((prev_tok_idx = prevlangmsg (messages, idx)) != ERROR) {
	if (messages[prev_tok_idx] -> tokentype == MACRO_CONCAT)
	  args[i] -> paste_mode = concat_op_1;
	if (messages[prev_tok_idx] -> tokentype == LITERALIZE)
	  args[i] -> paste_mode = concat_literal;
      }
      if ((next_tok_idx = nextlangmsg (messages, idx)) != ERROR) {
	if (messages[next_tok_idx] -> tokentype == MACRO_CONCAT)
	  args[i] -> paste_mode = concat_op_2;
      }
      if (((prev_tok_idx != ERROR) && (next_tok_idx != ERROR)) &&
	  ((messages[prev_tok_idx] ->  tokentype == MACRO_CONCAT) &&
	   (messages[next_tok_idx] -> tokentype == MACRO_CONCAT)))
	args[i] -> paste_mode = concat_both;
    }
  }
  return SUCCESS;
}

/*
 *   Return the symbol referred to by d.  The function
 *   returns its argument if the value does not resolve
 *   to another symbol.
 */

DEFINITION *sub_symbol (DEFINITION *d) {

  DEFINITION *d_sub;

  if ( !*d -> value || str_eq (d -> value, d -> name))
    return d;

  if ((d_sub = get_symbol (d -> value, TRUE)) != NULL) {
#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
    if (!sol10_attribute_macro_sub (d))
      return d;
#endif
    return sub_symbol (d_sub); 
  } else {
    return d;
  }
}

/*
 * Resolve the symbol referred to by the argument into 
 * its C __value.
 */

int resolve_symbol (char *s, VAL *val) {

  int i, type;
  DEFINITION *d;
  int start_msg,
    end_msg;
  VAL symbol_val;

  if ((d = get_symbol (s, TRUE)) != NULL) {

    if (! *d -> value) {
      val -> __type = INTEGER_T;
      val -> __value.__i = TRUE;
      return val -> __type;
    }
      
    if ((type = p_type_of (d -> value)) == PTR_T) {
      /*
       *  If the value is another defined symbol,
       *  recurse.
       */
      if (get_symbol (d -> value, TRUE)) {
	type = resolve_symbol (d -> value, val);
      } else {
	val -> __type = PTR_T;
	val -> __value.__ptr = d -> value;
      }
    } else {
      /* Evaluate as a constant expression. */
      start_msg = p_message_ptr;
      end_msg = tokenize_reuse (p_message_push, d -> value);
      eval_constant_expr (p_messages, start_msg, &end_msg, &symbol_val);
      for (i = start_msg; i >= end_msg; i--)
	reuse_message (p_message_pop ());
      copy_val (&symbol_val, val);
      type = symbol_val.__type;
    }
  } else {
    /* Check here for numbers also. */
    if ((type = _lextype (s)) != ERROR) {
      val -> __type = type;
      switch (type)
	{
	case INTEGER_T:
	  val -> __value.__i = atoi (s);
	  break;
	case UINTEGER_T:
	  val -> __value.__u = atoi (s);
	  break;
	case LONG_T:
	  val -> __value.__l = atol (s);
	  break;
	case ULONG_T:
	  val -> __value.__ul = atol (s);
	  break;
	case LONGLONG_T:
#if defined(__DJGPP__) || defined(__APPLE__)  
	  sscanf (s, "%lld", &val -> __value.__ll);
#else
	  val -> __value.__ll = atoll (s);
#endif
	  break;
	case DOUBLE_T:
	  sscanf (s, "%lf", &(val -> __value.__d));
	  break;
#ifndef __APPLE__
	case LONGDOUBLE_T:
	  sscanf (s, "%Lf", &(val -> __value.__ld));
	  break;
#endif
	case PTR_T:
	  val -> __value.__ptr = NULL;
	  break;
	}
    }
  }

  return type;
}

/* 
 *  Return a pointer to the symbol definition, or null.
 *  The values of some of the ANSI symbols, like __LINE__, 
 *  __func__, and __FILE__, change as preprocessing proceeds,
 *  and they are handled in ansisymbols.c.  If we're doing
 *  a rescanning of a macro replacement, then self_definition
 *  should contain the name of the macro being replaced, and
 *  the function should return NULL on a match.
 */

DEFINITION *get_symbol (char *basename, int exact) {

  DEFINITION *m;

  if ((m = (DEFINITION *)_hash_get (ansisymbols, mbasename(basename))) != NULL)
    return m;

  if (self_definition) {
    if (!basename_cmp (basename, self_definition -> name))
      return NULL;
  }

  if ((m = (DEFINITION *)_hash_get (macrodefs, mbasename(basename))) != NULL)
    return m;

  return NULL;
}

/*
 *  Rescan an argument.  This does not do any adjustment
 *  of the stack.
 */

int rescan_arg (DEFINITION *d, MESSAGE_STACK messages, int start,
		int *end) {
  
  char *argvals[MAXARGS];
  int i, j,
    end_ptr,
    n_parens,
    args,
    lastlangmsg,
    first_macro_token,
    last_macro_token;
  
  bool param_list;
  DEFINITION *d_arg;
  char argbuf[MAXMSG];

  init_args (argvals, MAXARGS);

  end_ptr = get_stack_top (messages);

  for (i = start, n_parens = 0, param_list = False, args = -1, *argbuf = 0,
	 lastlangmsg = -1; 
       i > end_ptr; i--) {

    switch (messages[i] -> tokentype)
      {
      case OPENPAREN:
	if (lastlangmsg != start && !n_parens) {
	  new_exception (missing_arg_list_x);
 	  free_args (argvals);
	}
	if (!n_parens) {
	  param_list = True;
	  ++args;
	} else {
	  strcat (argbuf, messages[i] -> name);
	}
	++n_parens;
	break;
      case CLOSEPAREN:
	--n_parens;
	if (!n_parens) {
	  param_list = False;
	  argvals[args] = strdup (argbuf);
	  *argbuf = 0;
	  goto actual_args_done;
	} else {
	  strcat (argbuf, messages[i] -> name);
	}
	break;
      case WHITESPACE:
	if (n_parens >= 1)
	  strcat (argbuf, messages[i] -> name);
	break;
      case ARGSEPARATOR:
	if (n_parens == 1) {  /* If an arg in parens contains other args. */ 
	  argvals[args] = strdup (argbuf);
	  ++args;
	  *argbuf = 0;
	} else {
	  strcat (argbuf, messages[i] -> name);
	}
	break;
      default:
	if (param_list) {
	  if ((d_arg = get_symbol (messages[i] -> name, TRUE)) != NULL) {
	    if (! *(d_arg -> value)) {
	      /*
	       *  TO DO - if the symbol's value is simply undefined
	       *  figure out what it should evaluate to when
	       *  rescanning.
	       */
	      warning (messages[start], 
		       _("Symbol, \"%s,\" value undefined."), d_arg -> name);
	      strcat (argbuf, " ");
	    } else {
	      if (!d_arg -> m_args[0]) {
		strcat (argbuf, d_arg -> value);
	      }
	    }
	  } else {
	    strcat (argbuf, messages[i] -> name);
	  }
	} else {
	  if (lastlangmsg == start) {
	    new_exception (missing_arg_list_x);
	    free_args (argvals);
	  }
	}
	break;
      }
    if (messages[i] -> tokentype != WHITESPACE)
      lastlangmsg = i;
  }

 actual_args_done:

  if (parse_exception != success_x)
    return ERROR;

  if (!d || ! *d -> value || str_eq (d -> value, " "))
    return FALSE;

  rescanning = TRUE;
  first_macro_token = P_MESSAGES;
  last_macro_token = tokenize_reuse (t_message_push, d -> value);
  rescanning = FALSE;

#ifdef DEBUG_CODE
  if ((first_macro_token - last_macro_token) > 
      (start - *end))
    debug (_("You must insert stack space in rescan_arg."));
#else
  if ((first_macro_token - last_macro_token) >
      (start - *end)) {
    splice_stack (messages, start, *end - 1);
    insert_stack (messages, start, t_messages, first_macro_token, 
		  last_macro_token);
    *end -= (first_macro_token - last_macro_token) - (start - *end);
  }
#endif

  for (i = first_macro_token; i >= last_macro_token; i--) {

    for (j = 0; j <= args; j++) {
      if (!t_messages[i] || ! d -> m_args[j]) {
 	warning (messages[start],
 		 _("rescan_args: arg error for message %d."), i);
      } else {
 	if (str_eq (t_messages[i] -> name, d -> m_args[j] -> name)) {
 	  if (!argvals[j]) {
 	    warning (messages[start],
 		     _("rescan_args: replacement arg error for message %d."), 
		     i);
 	  } else {
	      strcpy (t_messages[i] -> name, argvals[j]);
 	  }
 	}
 	if (t_messages[i] -> tokentype == MACRO_CONCAT) {
  	  concatenate_args (t_messages, d, first_macro_token, i,
  			      last_macro_token, argvals);
 	}
 	if (t_messages[i] -> tokentype == LITERALIZE) {
 	  last_macro_token += literalize_arg (t_messages, 
					      d, first_macro_token, i,
					      last_macro_token, 
					      start, argvals);
 	}
       }
     }
  }

  free_args(argvals);

  /* Replace with whitespace if the replacement is shorter than
     the macro. */
  for (i = start; i >= *end; i--) {
    messages[i] -> name[0] = ' '; messages[i] -> name[1] = '\0';
    messages[i] -> tokentype = WHITESPACE;
  }
  for (i = 0; i <= first_macro_token - last_macro_token; i++) {
    strcpy (messages[start - i]-> name, 
	    t_messages[first_macro_token - i] -> name);
    messages[start - i] -> tokentype = 
      t_messages[first_macro_token - i] -> tokentype;
  }

  for (i = last_macro_token; i <= first_macro_token; i++)
    reuse_message (t_message_pop ());

  return SUCCESS;
}

static int replacement_has_args_with_no_arglist  (DEFINITION *d, 
				     MESSAGE_STACK messages, int d_idx) {
  int next_idx;
  if (d -> m_args[0]) {
    if ((next_idx = nextlangmsg (messages, d_idx)) != ERROR) {
      if (messages[next_idx] -> tokentype != OPENPAREN) {
	return TRUE;
      } else {
	return FALSE;
      } 
    } else {
      return FALSE;
    }
  } 
  return FALSE;
}

/*
 *  Replace a macro with parameter substitution.
 */

int replace_args (DEFINITION *d, MESSAGE_STACK messages, int msg_ptr, 
		  int *end) {

  char *argvals[MAXARGS];
  int i, j,
    end_ptr,
    n_parens,
    args,
    va_ptr,
    variadic_macro,
    lastlangmsg = -1,
    first_macro_token,
    last_macro_token,
    macro_arg_end = -1,
    macro_size,                    /* Number of tokens in actual macro.     */
    replacement_size;              /* Number of tokens in replacement text. */
  bool param_list;
  DEFINITION *d_arg;
  char argbuf[MAXMSG];
  char replacement_buf[MAXMSG];
  VAL sub_val;

  init_args (argvals, MAXARGS);

  end_ptr = get_stack_top (messages);

  if ((va_ptr = macro_is_variadic (d)) != ERROR)
    variadic_macro = TRUE;
  else
    variadic_macro = FALSE;

  for (i = msg_ptr, n_parens = 0, param_list = False, args = -1, *argbuf = 0; 
       i > end_ptr; i--) {

    switch (messages[i] -> tokentype)
      {
      case OPENPAREN:
	if (lastlangmsg != msg_ptr && !n_parens) {
	  new_exception (missing_arg_list_x);
	  free_args (argvals);
	}
	if (!n_parens) {
	  param_list = True;
	  ++args;
	} else {
	  strcat (argbuf, messages[i] -> name);
	}
	++n_parens;
	break;
      case CLOSEPAREN:
	/* Check if there isn't an arglist.... */
	if (!n_parens) {
	  new_exception (missing_arg_list_x);
	  free_args (argvals);
	  return ERROR;
	}
	--n_parens;
	if (!n_parens) {
	  param_list = False;
	  macro_arg_end = i;
	  argvals[args] = strdup (argbuf);
	  *argbuf = 0;
	  goto actual_args_done;
	} else {
	  strcat (argbuf, messages[i] -> name);
	}
	break;
      case WHITESPACE:
	if (n_parens >= 1)
	  strcat (argbuf, messages[i] -> name);
	break;
      case ARGSEPARATOR:
	if (n_parens == 1) {  /* If an arg in parens contains other args. */ 
	  argvals[args] = strdup (argbuf);
	  ++args;
	  *argbuf = 0;
	} else {
	  strcat (argbuf, messages[i] -> name);
	}
	break;
      default:
	if (param_list) {
 	  if ((d_arg = replace_macro_arg_scan1 (d, messages, i)) 
	      != NULL) {
	    if (! *(d_arg -> value)) {
	      strcat (argbuf, " ");
	    } else {
	      if (!d_arg -> m_args[0]) {
		strcat (argbuf, d_arg -> value);
	      } else {
		if (replacement_has_args_with_no_arglist (d_arg,
							  messages, 
							  i)) {
		  char *p_ptr, *p_ptr_2;
		  if ((p_ptr = strchr (d_arg -> name, '(')) != NULL) {
		    char t_buf[MAXLABEL];
		    strcpy (t_buf, d_arg -> name);
		    p_ptr_2 = strchr (t_buf, '(');
		    *p_ptr_2 = '\0';
		    strcat (argbuf, t_buf); 
		  } else {
		    strcat (argbuf, d_arg -> name);
		  }
		} else {
		  int expr_end_orig, expr_end_new, j;
		  /*
		   *  Rescan the argument.
		   */
		  for (j = i; j > *end; j--)
		    if (messages[j] -> tokentype == OPENPAREN)
		      break;
		  expr_end_orig = expr_end_new = 
		    p_match_paren (messages, j, get_stack_top (messages));
		  rescan_arg (d_arg, messages, i, &expr_end_new);
		  if (expr_end_orig > expr_end_new) {
		    *end -= expr_end_orig - expr_end_new;
		    end_ptr -= expr_end_orig - expr_end_new;
		  }
		  i++;
		}
	      }
	    }
	  } else {
	    strcat (argbuf, messages[i] -> name);
	  }
	} else {
	  int j;
	  /* Make sure the next non-whitespace token starts the 
	     argument list. */
	  for (j = i - 1; 
	       (j >= *end) && (messages[j] -> tokentype == WHITESPACE); 
	       --j) 
	    ;
	  if (messages[j] -> tokentype != OPENPAREN) {
	    new_exception (missing_arg_list_x);
	    free_args (argvals);
	  }
	}
	break;
      }
    if (messages[i] -> tokentype != WHITESPACE)
      lastlangmsg = i;
  }

 actual_args_done:

  if (parse_exception != success_x) {
    *end = msg_ptr;
    free_args (argvals);
    return ERROR;
  }

  /*
   *  Extra check if the number of arguments in
   *  a list doesn't match the macro specification.
   */
  if (!variadic_macro) {
    if (args != (n_definition_params (d) - 1)) {
      new_exception (missing_arg_list_x);
      free_args (argvals);
      return ERROR;
    }
  }
  if (argvals[args] == NULL) {
    new_exception (argument_parse_error_x);
    free_args(argvals);
    return ERROR;
  }

  /* msg_ptr points to the first token, so add 1. */
  macro_size = msg_ptr - i + 1;

  if (!d || ! *d -> value || str_eq (d -> value, " ")) {
    free_args (argvals);
    splice_stack (messages, msg_ptr, msg_ptr - macro_size);
    adj_include_stack (macro_size);
    *end += macro_size;
    return macro_size;
  }

  rescanning = TRUE;
  first_macro_token = t_message_ptr;
  last_macro_token = tokenize_reuse (t_message_push, d -> value);
  rescanning = FALSE;

  for (i = first_macro_token; i >= last_macro_token; i--) {
    for (j = 0; 
	 ((!variadic_macro && (j <= args)) ||
	  (variadic_macro && (j <= va_ptr))) ;
	 j++) {
      if (!t_messages[i] || (!d -> m_args[j] && !variadic_macro)) {
	warning (t_messages[first_macro_token], 
		 _("replace_args: arg error for message %d."), i);
      } else {
	if (str_eq (t_messages[i] -> name, d -> m_args[j]->name)) {
	  if (!argvals[j]) {
	    warning (t_messages[i],
		     _("replace_args: replacement arg error for message %d."), 
		     i);
	  } else {
	    strcpy (t_messages[i] -> name, (*argvals[j] ? argvals[j] : " "));
	  }
	}
	if (t_messages[i] -> tokentype == LITERAL) {
	  /* If we have a "<symbol>" macro definition instead of
	   * #<symbol>.  The tokenizer handles the definition as one
	   * token, so check with a string comparison.
	   */
	  if (!strncmp (&(t_messages[i] -> name[1]), d -> m_args[j] -> name,
			strlen (d -> m_args[j] -> name))) {
	    sprintf (t_messages[i] -> name, "\"%s\"", 
		     (*argvals[j] ? argvals[j] : " "));
	  }
	} else if (t_messages[i] -> tokentype == MACRO_CONCAT) {
	  /* TO DO - 
	     More than one ## in an expression should be handled
	     during rescanning, but it needs to be verified. */
   	  concatenate_args (t_messages, d, first_macro_token, i,
			    last_macro_token, argvals);
	} else if (t_messages[i] -> tokentype == LITERALIZE) {
	  last_macro_token += literalize_arg (t_messages, 
					      d, first_macro_token, i,
					      last_macro_token, 
					      msg_ptr, argvals);
	} else if (str_eq (t_messages[i] -> name, VA_ARG_LABEL)) {
	  last_macro_token += replace_va_args (t_messages, d,
					       first_macro_token, i,
					       last_macro_token, argvals);
	  /***/
#if 0
	  while (t_messages[last_macro_token] == NULL) {
	    ++last_macro_token;
	  }
#endif	  
	} else if (str_eq (t_messages[i] -> name, N_ARG_LABEL)) {
	  int __n, __n_args;
	  for (__n = 0, __n_args = 0; argvals[__n]; __n++)
	    if (argvals[__n]) 
	      __n_args = __n + 1;
	  /* TODO  - tighten this up. */
	  /* free (t_messages[i] -> name); */
	  sprintf (t_messages[i] -> name, "%d", __n_args);
	  /* t_messages[i] -> name = strdup (buf); */
	  t_messages[i] -> tokentype = INTEGER;
	}
      }
    }
  }

  /* Now retokenize in case an actual parameter consists of more than
     one token. */
  for (i = first_macro_token, *replacement_buf = 0; 
       i >= last_macro_token; i--)
    strcat (replacement_buf, t_messages[i] -> name);
    
  /* Clear the messages of the last tokenization. */
  for (i = last_macro_token; i <= first_macro_token; i++) {
    reuse_message (t_message_pop ());
    t_messages[i] = NULL;
  }

  first_macro_token = t_message_ptr;
  last_macro_token = tokenize_reuse (t_message_push, replacement_buf);

  replacement_size = first_macro_token - last_macro_token + 1;

  splice_stack (messages, msg_ptr, macro_arg_end - 1);

  insert_stack (messages, msg_ptr, 
		t_messages, first_macro_token, last_macro_token);

  for (i = last_macro_token; i <= first_macro_token; i++) {
    reuse_message (t_message_pop ());
    t_messages[i] = NULL;
  }

   *end += (macro_size - replacement_size);
   adj_include_stack (macro_size - replacement_size);

   free_args (argvals);

  /* Now rescan the replaced macro, without substituting the 
     original macro again. The value sub_val is required by
     macro_parse - we should not need to worry about
     the return value here. 

     TO DO - 
     1. Allow macro_parse to take a null fourth argument.
  */

  self_definition = d;

  memset ((void *)&sub_val, 0, sizeof (VAL));
  sub_val.__type = 1;
  macro_sub_parse (messages, msg_ptr, end, &sub_val);

  self_definition = NULL;

  return SUCCESS;
}

/*
 *  Return the definition of an argument macro during the first 
 *  scan of the argument list.  Returns the DEFINITION if defined,
 *  and not preceded or followed by token paste or literalization
 *  operators, *in the replacement list*, in which case the pasted 
 *  token gets replaced during rescanning.
 */
DEFINITION *replace_macro_arg_scan1 (DEFINITION *d, MESSAGE_STACK messages, 
				     int idx) {
  DEFINITION *d_arg;
  int arg = 0;

  while (d -> m_args[arg]) {
    if (str_eq (d -> m_args[arg] -> name, messages[idx] -> name)) {
      if (d -> m_args[arg] -> paste_mode != concat_null)
	return NULL;
    } else {
      if ((d_arg = get_symbol (messages[idx] -> name, TRUE)) != NULL) {
	if (d -> m_args[arg] -> paste_mode != concat_null) {
	  return NULL;
	} else {
	  return d_arg;
	}
      }
    }
    ++arg;
  }

  if ((d_arg = get_symbol (messages[idx] -> name, TRUE)) != NULL)
    return d_arg;

  return NULL;
}

/*
 *  Replaces macro text in the output.  If the macro's defined
 *  value is null, replaces the macro text with a space.  If
 *  the macro value refers to another macro, calls sub_symbol ()
 *  until the macro is resolved.
 */

int replace_macro (DEFINITION *d, MESSAGE_STACK messages, int msg_ptr, 
		  int *end) {

  int i;
  int first, last;
  DEFINITION *d_sub;
  VAL sub_val;

  if ((d_sub = sub_symbol (d)) == NULL)
    d_sub = d;

  if (*d_sub -> value == 0) {
    strcpy (messages[msg_ptr] -> name, " ");
    messages[msg_ptr] -> tokentype = WHITESPACE;
    return SUCCESS;
  }

  /*
   *  If the replacement has an argument.
   */
  if (!strstr (d -> name, "(") && strstr (d -> value, "(") && (d != d_sub)) {
    int insert_size;
    first = t_message_ptr;
    last = tokenize_reuse (t_message_push, d -> value);
    /*                               Single token macro. */
    splice_stack (messages, msg_ptr, msg_ptr - 1);
    insert_stack (messages, msg_ptr, t_messages, first, last);
    for (i = last; i <= first; i++) {
      reuse_message (t_message_pop ());
      t_messages[i] = NULL;
    }
    insert_size = first - last;
    *end -= insert_size;
    adj_include_stack (last - first + 1);
    return 1;
  }

  rescanning = TRUE;

  /* first = t_message_ptr = P_MESSAGES; */ /***/
  first = t_message_ptr;
  last = tokenize_reuse (t_message_push, d_sub -> value);

  splice_stack (messages, msg_ptr, msg_ptr - 1);

  insert_stack (messages, msg_ptr, t_messages, P_MESSAGES, last);

  /* No adjustment here for the token removed by splice_stack. */
  *end -= first - last;

  adj_include_stack (last - first);

  for (i = last; i <= first; i++) {
    reuse_message (t_message_pop ());
    t_messages[i] = NULL;
  }

  /* Rescan. */ 

  self_definition = d;

  memset ((void *)&sub_val, 0, sizeof (VAL));
  sub_val.__type = 1;
  macro_sub_parse (messages, msg_ptr, end, &sub_val);

  self_definition = NULL;

  rescanning = FALSE;

  return SUCCESS;
  
}

void splice_stack (MESSAGE_STACK messages, int p1, int p2) {

  int i;
  register int ptr_d = p2, ptr_s = p1;
  int stack_end, *stack_ptr_ptr = NULL;

  stack_end = get_stack_ptr (messages, &stack_ptr_ptr);

  for (i = p1; i > p2; i--) {
    delete_message (messages[i]);
    messages[i] = NULL;
  }

  /* ptr_s = p1; ptr_d = p2; */
  while (ptr_d > stack_end)
    messages[ptr_s--] = messages[ptr_d--];

  for (i = stack_end + (p1 - p2); i > stack_end; i--) 
    messages[i] = NULL;

  *stack_ptr_ptr += (p1 - p2);

}

/*
 *  Faster version of splice_stack where we know which stack
 *  is being spliced.  Currently only called from move_includes.
 */
void splice_stack2 (MESSAGE_STACK messages, int p1, int p2, int *messages_ptr){

  register int i;
  int stack_end, ptr_d, ptr_s;

  stack_end = get_stack_top (messages);
  for (i = p1; i > p2; i--) {
    delete_message (messages[i]);
    messages[i] = NULL;
  }
  ptr_s = p1; ptr_d = p2;
  while (ptr_d > stack_end)
    messages[ptr_s--] = messages[ptr_d--];
  for (i = stack_end + (p1 - p2); i > stack_end; i--) 
    messages[i] = NULL;
  *messages_ptr += (p1 - p2);
}

/*
 *  Insert tokens from s_messages into d_messages.  
 */

void insert_stack (MESSAGE_STACK d_messages, int p, 
		   MESSAGE_STACK s_messages, int s_start, int s_end) {

  /*  int i, j; */
  int ins_length;       /* Length of insertion, in tokens. */
  int stack_end;
  register int ptr_s, ptr_d;
  int *stack_ptr_ptr = NULL;
  
  /* 
   *  The function does not yet handle copying messages over 
   *  themselves.  Return if that is the case.
   */

  if ((p == s_start) && (s_messages == d_messages))
    return;

  ins_length = s_start - s_end;
  stack_end = get_stack_ptr (d_messages, &stack_ptr_ptr);

  ptr_s = stack_end; ptr_d = stack_end - ins_length - 1;
  while (ptr_s <= p)
    d_messages[ptr_d++] = d_messages[ptr_s++];

  ptr_s = p - ins_length; ptr_d = s_start - ins_length;
  while (ptr_s <= p)
    d_messages[ptr_s++] = dup_message (s_messages[ptr_d++]);

  *stack_ptr_ptr -= ins_length + 1;

}

/*
 *  A slightly faster version of above, where we know 
 *  which stack the insertion is being performed on.
 */
void insert_stack2 (MESSAGE_STACK d_messages, int p, 
		   MESSAGE_STACK s_messages, int s_start, int s_end,
		    int *d_messages_ptr) {

  register int i;
  int ins_length;       /* Length of insertion, in tokens. */
  int stack_end;
  int ptr_s, ptr_d;
  
  /* 
   *  The function does not yet handle copying messages over 
   *  themselves.  Return if that is the case.
   */

  if ((p == s_start) && (s_messages == d_messages))
    return;

  ins_length = s_start - s_end;
  stack_end = get_stack_top (d_messages);

  ptr_s = stack_end; ptr_d = stack_end - ins_length - 1;
  while (ptr_s <= p)
    d_messages[ptr_d++] = d_messages[ptr_s++];

  for (i = 0; i <= ins_length; i++) {
#ifdef DEBUG_CODE
    if (!s_messages[s_start - i] || 
	!IS_MESSAGE (s_messages[s_start - i])) {
      debug (_("bad message in insert_stack."));
    } else {
      d_messages[p - i] = dup_message (s_messages[s_start - i]);
    }
#else
    d_messages[p - i] = dup_message (s_messages[s_start - i]);
#endif
  }
  *d_messages_ptr -= ins_length + 1;
}

int literalize_token (MESSAGE_STACK messages, int *op_ptr) {
  
  DEFINITION *d;
  int stack_end, arg_ptr;

  stack_end = get_stack_top (messages);

  for (arg_ptr = *op_ptr - 1; arg_ptr > stack_end; arg_ptr--)
    if (messages[arg_ptr] -> tokentype != WHITESPACE)
      break;

  if ((d = get_symbol (messages[arg_ptr] -> name, TRUE)) != NULL) {
    d = sub_symbol (d);
    sprintf (messages[*op_ptr] -> name, "\"%s\"", d -> value);
    messages[*op_ptr] -> tokentype = LITERAL;
    strcpy (messages[arg_ptr] -> name, " ");
    messages[arg_ptr] -> tokentype = WHITESPACE;
  } else {
    char buf[MAXLABEL];
    sprintf (buf, "\"%s\"", messages[arg_ptr] -> name);
    strcpy (messages[*op_ptr] -> name, buf);
    messages[*op_ptr] -> tokentype = LITERAL;
    strcpy (messages[arg_ptr] -> name, " ");
    messages[arg_ptr] -> tokentype = WHITESPACE;
  }
  return SUCCESS;
}

int paste_va_arg (MESSAGE_STACK messages, DEFINITION *d, 
		    int first, int op_ptr, 
		    int last, int macro_ptr, char **argvals) {
  int next_tok_idx,
    p_stack_end,
    v_stack_begin,
    v_stack_end,
    i,
    n_parens;
  char scanned_args[MAXMSG];

  if ((next_tok_idx = nextlangmsg (messages, op_ptr)) != ERROR) {
    if (str_eq (M_NAME(messages[next_tok_idx]), "__VA_ARGS__")) {
      splice_stack (messages, op_ptr, op_ptr - 2);
      *scanned_args = '\0';
      p_stack_end = get_stack_top (p_messages);
      for (i = macro_ptr, n_parens = 0; i > p_stack_end; i--) {
	if (M_TOK(p_messages[i]) == OPENPAREN) {
	  switch (n_parens)
	    {
	    case 0:
	      strcat (scanned_args, "\"");
	      ++n_parens;
	      break;
	    default:
	      strcat (scanned_args, M_NAME(p_messages[i]));
	      ++n_parens;
	      break;
	    }
	  continue;
	}
	if (M_TOK(p_messages[i]) == CLOSEPAREN) {
	  switch (n_parens)
	    {
	    case 1:
	      --n_parens;
	      strcat (scanned_args, "\"");
	      if (!n_parens)
		goto actual_va_params_scanned;
	      break;
	    default:
	      --n_parens;
	      strcat (scanned_args, M_NAME(p_messages[i]));
	      break;
	    }
	  continue;
	}
	if (n_parens)
	  strcat (scanned_args, M_NAME(p_messages[i]));
      }
    actual_va_params_scanned:
      v_stack_begin = stack_start (v_messages);
      v_stack_end = tokenize_reuse (v_message_push, scanned_args);
      insert_stack (messages, op_ptr, v_messages, v_stack_begin, v_stack_end);
      for (i = v_stack_end;  i <= v_stack_begin; i++) {
	reuse_message (v_messages[i]);
	v_messages[i] = NULL;
      }
      v_message_ptr = P_MESSAGES;
      return 1;
    }
  }
  return 0;
}

/*
 *   Replace a #<param_name> sequence with the parameter 
 *   in quotes.  The parameter isn't expanded.
 *
 *   The return value here also is the adjustment to
 *   the end of the stack.
 */

int literalize_arg (MESSAGE_STACK messages, DEFINITION *d, 
		    int first, int op_ptr, 
		    int last, int macro_ptr, char **argvals) {

  int i;
  int repl_ptr;
  int next_tok_idx;


  repl_ptr = op_ptr - 1;  /* The param name must immediately follow
			     the # operator. */
  
  if ((next_tok_idx = nextlangmsg (messages, op_ptr)) != ERROR) {
    if (str_eq (M_NAME(messages[next_tok_idx]), "__VA_ARGS__")) {
      return paste_va_arg (messages, d, first, op_ptr,
			   last, macro_ptr, argvals);
    }
  }

  for (i = 0; d -> m_args[i]; i++) {
    if (str_eq (messages[repl_ptr] -> name, d -> m_args[i] -> name)) {
      char buf[MAXLABEL];
      strcpy (buf, argvals[i]);
      trim_leading_whitespace(buf);
      trim_trailing_whitespace(buf);
      sprintf (messages[repl_ptr] -> name, "\"%s\"", buf);
    }
  }

  splice_stack (messages, op_ptr, repl_ptr);

  return op_ptr - repl_ptr;
}

/*
 *   Concatenate tokens with the ## operator.  op_ptr
 *   is returned pointing to the new token, so we can
 *   rescan it.
 */

int concatenate_tokens (MESSAGE_STACK messages, int *op_ptr, 
			CONCAT_MODE mode) {

  int r, op1_ptr, op2_ptr;

  if ((r = operands (messages, *op_ptr, &op1_ptr, &op2_ptr))
      == ERROR)
    error (messages[*op_ptr], _("Bad operands in concatenate_tokens."));

  switch (mode)
    {
    case concat_op_1:
      strcpy (messages[*op_ptr] -> name, " ");
      messages[*op_ptr] -> tokentype = WHITESPACE;
      *op_ptr = op1_ptr;
      break;
    case concat_op_2:
      strcpy (messages[*op_ptr] -> name, " ");
      messages[*op_ptr] -> tokentype = WHITESPACE;
      *op_ptr = op2_ptr;
      break;
    case concat_both:
      trim_trailing_whitespace (messages[op1_ptr] -> name);
      trim_leading_whitespace  (messages[op2_ptr] -> name);
      snprintf (messages[op1_ptr] -> name, MAXLABEL, "%s%s", 
	       messages[op1_ptr] -> name, messages[op2_ptr] -> name);
      strcpy (messages[*op_ptr] -> name, " ");
      messages[*op_ptr] -> tokentype = WHITESPACE;
      strcpy (messages[op2_ptr] -> name, " ");
      messages[op2_ptr] -> tokentype = WHITESPACE;
      *op_ptr = op1_ptr;
      break;
    default:
      break;
    }

  return *op_ptr;
}


/*
 *  Handle the ## operator within macro arguments.
 */

int concatenate_args (MESSAGE_STACK messages, DEFINITION *d, 
			int first, int op_ptr, 
			int last, char **argvals) {

  int i, op1_ptr, op2_ptr;
  int self1_ptr = -1, 
    self2_ptr = -1;

  if (op_ptr == first || op_ptr == last) {
    warning (messages[op_ptr], _("Concatenation without argument."));
    location_trace (messages[op_ptr]);
  }

  for (i = op_ptr + 1; i <= first; i++) {
    if ((messages[i] -> tokentype == WHITESPACE) || 
	(messages[i] -> tokentype == OPENCCOMMENT) ||
	(messages[i] -> tokentype == CLOSECCOMMENT) ||
	(messages[i] -> tokentype == NEWLINE && LINE_SPLICE (messages, i)))
      continue;
    else
      break;
  }
  op1_ptr = i;

  for (i = op_ptr - 1; i >= last; i--) {
    if ((messages[i] -> tokentype == WHITESPACE) || 
	(messages[i] -> tokentype == OPENCCOMMENT) ||
	(messages[i] -> tokentype == CLOSECCOMMENT) ||
	(messages[i] -> tokentype == NEWLINE && LINE_SPLICE (messages, i)))
      continue;
    else
      break;
  }
  op2_ptr = i;

  /* First check if one of the tokens corresponds to an argument. */
  /* TO DO - Make sure this works if both operands get replaced. */
  for (i = 0; d -> m_args[i]; i++) {
    if (str_eq (d -> m_args[i] -> name, messages[op1_ptr] -> name)) {
      trim_trailing_whitespace (argvals[i]);
      trim_leading_whitespace (messages[op2_ptr] -> name);
      sprintf (messages[op1_ptr] -> name, 
	       "%s%s", argvals[i], messages[op2_ptr]->name);
      messages[op_ptr] -> name[0] = ' '; messages[op_ptr] -> name[1] = '\0';
      messages[op_ptr] -> tokentype = WHITESPACE;
      messages[op2_ptr] -> name[0] = ' '; messages[op2_ptr] -> name[1] = '\0';
      messages[op2_ptr] -> tokentype = WHITESPACE;
      return SUCCESS;
    }
    if (str_eq (d -> m_args[i] -> name, messages[op2_ptr] -> name)) {
      trim_leading_whitespace (argvals[i]);
      trim_trailing_whitespace (messages[op1_ptr] -> name);
      sprintf (messages[op1_ptr] -> name, "%s%s", 
	       messages[op1_ptr] -> name, argvals[i]);
      messages[op_ptr] -> name[0] = ' '; messages[op_ptr] -> name[1] = '\0';
      messages[op_ptr] -> tokentype = WHITESPACE;
      messages[op2_ptr] -> name[0] = ' '; messages[op2_ptr] -> name[1] = '\0';
      messages[op2_ptr] -> tokentype = WHITESPACE; 
      return SUCCESS;
    }
  }

  /* Then check if one or both of the tokens corresponds to another token. */
  for (i = first; i >= last; i--) {
    if (str_eq (messages[op1_ptr] -> name, messages[i] -> name))
      self1_ptr = i;
    if (str_eq (messages[op2_ptr] -> name, messages[i] -> name))
      self2_ptr = i;
  }

  if (op1_ptr != self1_ptr)
    strcpy (messages[op1_ptr] -> name, messages[self1_ptr] -> name);
  if (op2_ptr != self2_ptr)
    strcpy (messages[op2_ptr] -> name, messages[self2_ptr] -> name);

  trim_trailing_whitespace (messages[op1_ptr] -> name);
  trim_leading_whitespace  (messages[op2_ptr] -> name);

  sprintf (messages[op1_ptr] -> name, "%s%s", messages[op1_ptr] -> name, 
	   messages[op2_ptr] -> name);
  messages[op_ptr] -> name[0] = ' '; messages[op_ptr] -> name[1] = '\0';
  messages[op_ptr] -> tokentype = WHITESPACE;
  messages[op2_ptr] -> name[0] = ' '; messages[op2_ptr] -> name[1] = '\0';
  messages[op2_ptr] -> tokentype = WHITESPACE;
	   
  return SUCCESS;
}

int replace_va_args (MESSAGE_STACK messages, DEFINITION *d, 
		    int first, int va_ptr, 
		    int last, char **argvals) {

  int i, j, k, v_param_start,
    n_scanned_args,
    stack_begin,
    stack_end,
    arg_ptr;
  char *scanned_arg_vals[MAXARGS + 1];
  VAL result;

  memset ((void *)&result, 0, sizeof (VAL));
  memset ((void *)scanned_arg_vals, 0, sizeof (char *) * MAXARGS);

  /*
   *  Remove the __VA_ARGS__ parameter from the macro definition.
   */
  splice_stack (messages, va_ptr, va_ptr - 1);

  for (i = 0, n_scanned_args = 0, v_param_start = ERROR; argvals[i]; i++) {

    /*
     *  Find the actual variable arg parameter.
     */
    if (d -> m_args[i] && 
	str_eq (d -> m_args[i] -> name, VA_ARG_PARAM)) v_param_start = i;

    /*
     *  Evaluate the arguments.  Even though we set rescanning
     *  to True here, it is only so that lexical () doesn't 
     *  interpret token pastes as preprocessor directives.
     *  The actual rescanning occurs in replace_args ().
     */
    if (v_param_start != ERROR) {

      rescanning = TRUE;

      stack_begin = stack_start (v_messages);
      stack_end = tokenize_reuse (v_message_push, argvals[i]);

      macro_sub_parse (v_messages, stack_begin, &stack_end, &result);

      eval_constant_expr (v_messages, stack_begin, &stack_end, &result);

      rescanning = FALSE;

      scanned_arg_vals[n_scanned_args++] =
	collect_tokens (v_messages, stack_begin, stack_end);

      for (j = stack_end; j <= stack_begin; j++) {
	reuse_message (v_messages[j]);
	v_messages[j] = NULL;
      }
      v_message_ptr = P_MESSAGES;

    }


  }

  /*
   *  Retokenize the scanned args and insert into the output.
   *  Also tokenize argument separators.
   */

  for (j = 0, arg_ptr = va_ptr; j < n_scanned_args; j++) {

    stack_begin = stack_start (v_messages);
    stack_end = tokenize_reuse (v_message_push, scanned_arg_vals[j]);

    insert_stack (messages, arg_ptr, v_messages, stack_begin, stack_end);

    for (k = stack_end; k <= stack_begin; k++) {
      reuse_message (v_messages[k]);
      v_messages[k] = NULL;
    }
    v_message_ptr = P_MESSAGES;

    arg_ptr -= (stack_begin - stack_end);

    /*
     *  Insert argument separators if necessary, and only
     *  make adjustments to the arg_ptr insertion pointer
     *  if inserting commas - otherwise, the macro 
     *  substitution can overrun the stack.
     */
    if ((j + 1) < n_scanned_args) {
      stack_begin = stack_start (v_messages);
      stack_end = tokenize_reuse (v_message_push, ",");
      arg_ptr -= 1;
      insert_stack (messages, arg_ptr, v_messages, stack_begin, stack_end);
      arg_ptr -= 1;
      for (k = stack_end; k <= stack_begin; k++) {
	reuse_message (v_messages[k]);
	v_messages[k] = NULL;
      }
      v_message_ptr = P_MESSAGES;
    }

    free (scanned_arg_vals[j]);
  }
  
  return arg_ptr - va_ptr;
}

/*
 *  Tokenize_define () is only safe for use with
 *  the predefined macro definitions in ccompat.c,
 *  where the #define statement is the only 
 *  preprocessor line on the stack.
 */

void tokenize_define (char *line) {
  int i, last_ptr;
  VAL result;

  memset ((void *)&result, 0, sizeof (VAL));
  result.__type = 1;

  last_ptr = tokenize_reuse (p_message_push, line);
  macro_parse (p_messages, P_MESSAGES, &last_ptr, &result);
  for (i = last_ptr; i <= P_MESSAGES; i++) {
    reuse_message (p_messages[i]);
    p_messages[i] = NULL;
  }
  p_message_ptr = P_MESSAGES;
}

int next_label (MESSAGE_STACK messages, int this_message_ptr) {

  int i, label_ptr, 
    end_ptr = 0;
  MESSAGE *m;

  end_ptr = get_stack_top (messages);

  for (label_ptr = ERROR, i = this_message_ptr - 1; 
       (i > end_ptr) && (label_ptr == ERROR); i--) {
    m = messages[i];
    if (m -> tokentype == LABEL)
      label_ptr = i;
    if (m -> tokentype == PREPROCESS_EVALED) {
      int tmp_tok;
      long long tmp_idx = 0;
      MESSAGE *m_tmp;
      m_tmp = new_message ();
      if ((tmp_tok = lexical (m -> name, &tmp_idx, m_tmp)) == LABEL)
	label_ptr = i;
      delete_message (m_tmp);
    }
    if ((m -> tokentype == NEWLINE) && 
	!LINE_SPLICE (messages, i))
      break;
  }

  return label_ptr;

}

void dump_symbols (void) {
#ifdef DEBUG_CODE
  DEFINITION *m;
  HLIST *h;

  if ((h = _hash_first (&macrodefs)) != NULL) {
    /* Duplicate cpp's -dM output format so we can get
       more meaningful diffs when testing. */
    m = (DEFINITION *) h -> data;
    debug (_("#define %s %s "), m -> name, m -> value);
    while ((h = _hash_next (&macrodefs)) != NULL) {
      m = (DEFINITION *) h -> data;
      debug (_("#define %s %s "), m -> name, m -> value);
    }
  }

#endif
}

/* 
 *    Like check_state (), but doesn't ignore newlines.
 */

#define A_IDX_(a,b,ncols) (((a)*(ncols))+(b))

int check_preprocess_state (int stack_idx, MESSAGE **messages, int *states, 
		 int cols) {
  int i, j;
  int token;
  int next_idx = stack_idx;

  token = messages[stack_idx] -> tokentype;

  for (i = 0; ; i++) {

  nextrans:
     if (states[A_IDX_(i,0,cols)] == ERROR)
       return ERROR;

     if (states[A_IDX_(i,0,cols)] == token) {

       for (j = 1, next_idx = stack_idx - 1; ; j++, next_idx--) {
	
	 while (states[A_IDX_(i,j,cols)]) {

	   if (!messages[next_idx])
	     return -1;

	   while (messages[next_idx]->tokentype == WHITESPACE)
	     next_idx--;
	   if (states[A_IDX_(i,j,cols)] != messages[next_idx]->tokentype) {
	     ++i;
	     goto nextrans;
	   }
	   else
	     ++j;
	 }
	 return i;
       }
     }
  }
}

/*
 *  Preprocess_stack_start will point to the start of stack if
 *  -include <file> options are prensent on the command line.
 *  Otherwise, start, declared locally below, points to the
 *  top of the p_messages stack.
 */
#define START ((preprocess_stack_start > -1) ? preprocess_stack_start : start)

void preprocess (char *input) {

  int i, r;
  int start, end;
  int line_marker_length;
  VAL result;
  int input_start_ptr;

  memset ((void *)&result, 0, sizeof (VAL));
  result.__type = 1;

  start = get_stack_top (p_messages);


  /*
   *  Maybe this works better - if we always put a 
   *  beginning line marker at the start of the input
   *  file - record the message ptr here *after* processing
   *  -include directives from the command line, but 
   *  *before* trying to make any adjustments from moving
   *  the include files.
   */
  input_start_ptr = p_message_ptr;

  if ((end = tokenize_reuse (p_message_push, input)) == ERROR)
    return;

  line_marker_length = line_info (input_start_ptr, source_file, 1, 1);
  adj_include_stack (-line_marker_length);
  end -= line_marker_length;

  if (move_includes_opt)
    move_includes (p_messages, START, &end);

#ifdef __CYGWIN__
  if (START == P_MESSAGES) {
    line_marker_length = line_info (START, source_file, 1, 0);
    adj_include_stack (-line_marker_length);
    end -= line_marker_length;
  }
  line_marker_length = line_info (start - line_marker_length, 
				  source_file, 1, 1);
  adj_include_stack (-line_marker_length);
  end -= line_marker_length;
#else
  if (sol10_gcc_sfw_mod)
    line_marker_length = line_info (START, source_file, 1, 0);
  else
    line_marker_length = line_info (START, source_file, 1, 1);
  adj_include_stack (-line_marker_length);
  end -= line_marker_length;
#endif
  
  if ((r = macro_parse (p_messages, START, &end, &result)) == ERROR) {
    /* Check exception here? .... */
  }

#ifndef __DJGPP__
#ifdef DEBUG_CODE
  for (i = START; i >= end; i--) { 
    if (!p_messages[i] || !IS_MESSAGE (p_messages[i])) {
      debug (_("not a message in preprocess ()."));
    } else {
      if (p_messages[i] -> tokentype == RESULT)
	continue;
      write_tmp (p_messages[i] -> name);
    }
  }
#else
  for (i = START; i >= end; i--) {
    if (p_messages[i] -> tokentype == RESULT)
      continue;
    write_tmp (p_messages[i] -> name);
  }
#endif

  close_tmp ();

  for (i = START; i >= end; i--) {
    MESSAGE *m;
    m = p_message_pop ();
    if (m && IS_MESSAGE (m))
      reuse_message (m);
  }

  if (definesonly_opt) {
    unlink_tmp ();
    output_symbols ();
  } else {
    if (definenamesonly_opt) {
      unlink_tmp ();
      output_symbol_names ();
    } else {
      if (definestofile_opt) {
	write_defines ();
	tmp_to_output ();
	unlink_tmp ();
      } else {
	if (makerule_opts & MAKERULE) {
	  unlink_tmp ();
	  output_make_rule ();
	} else {
	  tmp_to_output ();
	  unlink_tmp ();
	}
      }
    }
  }
  if (warnunused_opt)
    output_unused_symbols ();

  cleanup_reused_messages ();

#else  /* #ifndef __DJGPP__ */

  if (definesonly_opt) {
    output_symbols ();
  } else {
    if (definenamesonly_opt) {
      output_symbol_names ();
    } else {
      if (definestofile_opt) {
	write_defines ();
	output_djgpp (p_messages, START, end);
	unlink_tmp ();
      } else {
	if (makerule_opts & MAKERULE) {
	  unlink_tmp ();
	  output_make_rule ();
	} else {
	  output_djgpp (p_messages, START, end);
	  unlink_tmp ();
	}
      }
    }
  }
  if (warnunused_opt)
    output_unused_symbols ();

  for (i = START; i >= end; i--) {
    MESSAGE *m;
    m = p_message_pop ();
    if (m && IS_MESSAGE (m))
      reuse_message (m);
  }

  cleanup_reused_messages ();
#endif

}

/*
 *  The return value is the stack pointer adjustment to 
 *  make when handling an exception.
 */
int exception_adj (MESSAGE_STACK messages, int idx_org) {

  int stack_begin, stack_end, idx_new;

  stack_begin = stack_start (messages);
  stack_end = get_stack_top (messages);
  idx_new = idx_org;

  switch (handle_preprocess_exception (messages, idx_org, 
				       stack_begin, stack_end))
    {
    case TRUE:
      idx_new = stack_end;
      break;
    case SKIP:
      /*
       *  Point past newline to leave line unaffected, and
       *  reset states.
       */
      preprocess_line = pragma_line = False;
      parse_exception = success_x;
      idx_new = end_of_p_stmt (messages, idx_org);
      --idx_new;
      parse_exception = success_x;
      break;
    case FALSE:  /* E.G. SUCCESS. */
      parse_exception = success_x;
      break;
    }

  return idx_org - idx_new;
}

void p_hash_loc (MESSAGE *orig) {

  _hash_symbol_line (orig -> error_line);
  _hash_symbol_column (orig -> error_column);
  if (include_ptr <= MAXARGS)
    _hash_symbol_source_file (includes[include_ptr]->path);
  else
    _hash_symbol_source_file (source_file);
}

int move_includes (MESSAGE_STACK messages, int start, int *end) {

  int i, j, p_dir_start, p_dir_end, p, t_end_idx;
  MESSAGE *m;
  bool have_p_token;
  char *d_buf;

  for (i = start, p_dir_start = start, p = start, have_p_token = False; 
       i > *end; i--) {
    
    m = messages[i];

    if (M_ISSPACE(m)) 
      continue;

    if ((M_TOK(m) == PREPROCESS) && (m -> error_column == 1)) {
      have_p_token = True;
      p_dir_start = i;
      continue;
    }      

    if (have_p_token) {
      if (M_TOK(m) == LABEL) {
	if (str_eq (M_NAME(m), "include") ||
	    str_eq (M_NAME(m), "include_next")) {

	  for (p_dir_end = p_dir_start; p_dir_end > *end; p_dir_end--) 
	    if (M_TOK(messages[p_dir_end]) == NEWLINE)
	      break;

	  d_buf = collect_tokens (messages, p_dir_start, p_dir_end);
	  t_end_idx = tokenize_reuse (t_message_push, d_buf);
	  free (d_buf);

	  splice_stack (messages, p_dir_start, p_dir_end);
	  *end += (p_dir_start - p_dir_end);
	  insert_stack (messages, p, t_messages, P_MESSAGES, t_end_idx);
	  *end -= (P_MESSAGES - t_end_idx + 1);

	  i = p_dir_start;
	  p -= p_dir_start - p_dir_end + 1;
	  have_p_token = False;
	  
	  for (j = t_end_idx; j <= P_MESSAGES; j++) {
	    reuse_message (t_message_pop ());
	    t_messages[j] = NULL;
	  }
	}
      }
    }
    have_p_token = False;

  }

  return p;
}

