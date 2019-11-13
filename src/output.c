/* $Id: output.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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

/*
 *   Functions that generate Ctalk -> C code.  
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

extern char input_source_file[],    /* Declared in main.c.              */
  output_file[];
extern int include_dir_opt;   /* Include directories given as     */  
extern char *include_dirs[];  /* command line options.            */
extern int n_include_dirs;
extern int outputfile_opt;

extern int 
n_th_arg_CVAR_has_unary_inc_prefix;   /* Declared in prefixop.c.           */
extern int n_th_arg_CVAR_has_unary_dec_prefix;

extern I_PASS interpreter_pass;

extern int buffer_function_output;   /* Declared in fnbuf.c.      */

int ctalk_init = FALSE;              /* __ctalk_init and its function     */ 
int ctalk_init_call = FALSE;         /* call when main is reached.        */

extern int preamble;                 /* Preamble and main declaration     */
extern int main_declaration;         /* relative locations, declared in   */
                                     /* cparse.c.                         */

extern char *ascii[8193];             /* from intascii.h */

extern int do_block;                 /* Declared in control.c.            */


static PENDING *pending_statements = NULL; /* List of pending statements. */
static PENDING *pending_head_ptr = NULL;

static int tmp_var_ext;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern DEFAULTCLASSCACHE *ct_defclasses; /* Declared in cclasses.c.       */

/* 
 *  The stack index of the last message output.
 */

bool formatted_fn_arg_trans = false;

int parser_output_ptr = 0;

int get_parser_output_ptr (void) {
  return parser_output_ptr;
}

static FILE *f_output;

void init_output_vars (void) {
  tmp_var_ext = 0;

  if (outputfile_opt) {
    if ((f_output = fopen (output_file, FILE_WRITE_MODE)) == NULL)
      _error ("init_output_vars: %s: %s.", output_file, strerror(errno));
  } else {
    f_output = stdout;
  }
}

void cleanup_output_vars (void) {
  int r;
  if (outputfile_opt && f_output) {
    if ((r = fclose (f_output)) != 0) {
      printf ("cleanup_output_vars: %s: %s.\n", output_file, 
	      strerror (errno));
    }
  }
}

/* 
 * Output the call to __ctalk_init() after the local variable
 * declarations in main().
 */

/*
 *  Generic output function interface.
 */

static PENDING *pending_stmts[0xffff];
static int pending_ptr = 0;

static PENDING *__new_pending_stmt (char *str) {
  static PENDING *p;
  if (pending_ptr > 0) {
    p = pending_stmts[--pending_ptr];
    XSTRCPY(p->stmt,str);
    return p;
  } else {
    p = (PENDING *) __xalloc (sizeof (struct _pending));
    p -> sig = PENDING_SIG;
    XSTRCPY(p->stmt,str);
    return p;
  }
}

static void __delete_pending_stmt (PENDING *p) {
  if (pending_ptr < 0xffff) {
    p -> n = 0;
    p -> parser_lvl = 0;
    p -> next = p -> prev = NULL;
    pending_stmts[pending_ptr++] = p;
  } else {
    if (p && IS_PENDING(p))
      __xfree (MEMADDR(p));
  }
}

int output_buffer (char *buf, int when) {
  if (is_global_frame ()) {
    fn_init (buf, FALSE);
  } else {
    fileout (buf, 0, when);
  }
  return SUCCESS;
}

static char ctalk_init_fn_call[] = "     __ctalk_init (argv[0]);\n";

void generate_ctalk_init_call (void) {
  if (buffer_function_output)
    buffer_function_statement (ctalk_init_fn_call, 0);
  else
    fileout (ctalk_init_fn_call, 0, 0);
}

/*
 *  Format and output a __ctalk_self_internal () call.
 */

int generate_self_call (int when) {
  char buf[MAXMSG];

  strcatx (buf, SELF_ACCESSOR_FN, " ()", NULL);
  fileout (buf, 0, when);
  return SUCCESS;
}

/*
 *  Format a __ctalk_method call for output.
 */

char *fmt_method_call (OBJECT *rcvr, char *method_selector,
		       char *method_name, char *buf_out) {

  char namebuf[MAXLABEL], namebuf_out[MAXLABEL];

  strcpy (namebuf, rcvr -> __o_name);

  strcatx (buf_out, "__ctalk_method (\"", 
	   (lextype_is_LITERAL_T (namebuf) ?
	    escape_str (namebuf, namebuf_out) : namebuf), "\", ", 
	   method_selector, ", ", "\"", method_name, "\")", NULL);

  return buf_out;
}

/* 
 *  Output a __ctalk_method() function call at the
 *  source token index given by the parameter, "when."
 */

int generate_method_call (OBJECT *rcvr, char *method, 
			  char *method_name, int when) {

  char buf[MAXMSG];
  if (is_global_frame ())
    fn_init (fmt_method_call (rcvr, method, method_name, buf), FALSE);
  else
    fileout (fmt_method_call (rcvr, method, method_name, buf), 0, when);

  return SUCCESS;
}

/* 
 *  expr_idx can be anywhere within an expression that we want
 *  to add a delete_method_arg_cvars_call after.
 */
void output_delete_cvars_call (MESSAGE_STACK messages, int expr_idx,
			       int stack_end_idx) {
  int term_idx;

  if ((term_idx = scanforward (messages, expr_idx, stack_end_idx,
			       SEMICOLON)) != ERROR) {
    fileout ("\ndelete_method_arg_cvars ();\n", 0, term_idx - 1);
  }
}

/*
 *  Generate either a __ctalkGetInstanceVariable () or 
 *  __ctalkGetInstanceVariableByName () call, depending on 
 *  the class of the receiver.
 *
 *  The call should probably not generate a semicolon, because
 *  it may be used in place in an expression.
 */

char *fmt_rt_instancevar_call (OBJECT *receiver, char *varname, char *buf_out) {

  if (DEFAULTCLASS_CMP(receiver, ct_defclasses -> p_expr_class,
		       EXPR_CLASSNAME) ||
      DEFAULTCLASS_CMP(receiver, ct_defclasses -> p_cfunction_class,
		       CFUNCTION_CLASSNAME))
    strcatx (buf_out, "__ctalkGetInstanceVariable (", 
	     receiver -> __o_name, ", \"", varname, "\", 1)", NULL);
  else
    strcatx (buf_out, "__ctalkGetInstanceVariableByName (\"", 
	     receiver -> __o_name, "\", \"", varname, "\", 1)", NULL);

  return buf_out;
}

char *fmt_rt_instancevar_call_2 (MESSAGE_STACK messages, int rcvr_idx,
				 int instancevar_idx,
				 OBJECT *receiver, char *varname,
				 char *buf_out) {
  char buf[MAXMSG], expr[MAXMSG],  expr_out[MAXMSG];
  int fn_arg_index, fn_label_idx;
  CFUNC *cfn;

  /* this is read by method_call, in case we already added
     an object-to-c translation for a C function call argument. */
  formatted_fn_arg_trans = false;
  if ((fn_arg_index = obj_expr_is_arg (messages, rcvr_idx,
				       stack_start (messages),
				       &fn_label_idx)) >= 0) {
    if ((cfn = get_function (M_NAME(messages[fn_label_idx]))) != NULL) {
      toks2str (messages, rcvr_idx, instancevar_idx, expr);
      fmt_eval_expr_str (expr, expr_out);
      strcpy (buf_out, fn_param_return_trans
	      (messages[rcvr_idx], cfn, expr_out, fn_arg_index));
      formatted_fn_arg_trans = true;
    } else {
      warning (messages[fn_label_idx],
	       "Could not find parameter prototype for function, \"%s.\"",
	       M_NAME(messages[fn_label_idx]));
      fmt_rt_instancevar_call (receiver, varname, buf);
    }
  } else {
    
    if (DEFAULTCLASS_CMP(receiver, ct_defclasses -> p_expr_class,
			 EXPR_CLASSNAME) ||
	DEFAULTCLASS_CMP(receiver, ct_defclasses -> p_cfunction_class,
			 CFUNCTION_CLASSNAME)) {
      strcatx (buf_out, "__ctalkGetInstanceVariable (", 
	       receiver -> __o_name, ", \"", varname, "\", 1)", NULL);
    } else {
      strcatx (buf_out, "__ctalkGetInstanceVariableByName (\"", 
	       receiver -> __o_name, "\", \"", varname, "\", 1)", NULL);
    }
  }
  return buf_out;
}

int generate_rt_instancevar_call (MESSAGE_STACK messages, int rcvr_idx,
				  int var_idx,
				  OBJECT *receiver, char *varname, int when) {

  static char buf[MAXMSG], buf2[MAXMSG], expr_out[MAXMSG];
  char expr[MAXMSG];
  int fn_arg_index, fn_label_idx;
  CFUNC *cfn;

  if (messages[rcvr_idx] -> attrs & TOK_IS_PRINTF_ARG) {
    fmt_printf_fmt_arg 
      (messages,
       rcvr_idx,
       stack_start (messages),
       fmt_rt_instancevar_call (receiver, varname, buf2), buf);
  } else {
    if ((fn_arg_index = obj_expr_is_arg (messages, rcvr_idx,
					 stack_start (messages),
					 &fn_label_idx)) >= 0) {
      if ((cfn = get_function (M_NAME(messages[fn_label_idx]))) != NULL) {
	toks2str (messages, rcvr_idx, var_idx, expr);
	fmt_eval_expr_str (expr, expr_out);
	strcpy (buf, fn_param_return_trans
		(messages[rcvr_idx], cfn, expr_out, fn_arg_index));
      } else {
	warning (messages[fn_label_idx],
		 "Could not find parameter prototype for function, \"%s.\"",
		 M_NAME(messages[fn_label_idx]));
	fmt_rt_instancevar_call (receiver, varname, buf);
      }
    } else {
      fmt_rt_instancevar_call (receiver, varname, buf);
    }
  }

  if (is_global_frame ())
    fn_init (buf, FALSE);
  else
    fileout (buf, 0, when);

  return SUCCESS;
}

int generate_global_object_definition (char *name, char *class, 
				       char *superclass,
				       char *buf, int scope) {
  if (!superclass)
    strcatx (buf, "__ctalk_dictionary_add (__ctalkCreateObjectInit (\"",
	     name, "\", \"", class, "\", ", STR_VAL(NULL), ", ", 
	     ascii[scope], ", ", STR_VAL(NULL), "));\n", NULL);
  else
    strcatx (buf, "__ctalk_dictionary_add(__ctalkCreateObjectInit (\"",
	     name, "\", \"", class, "\", \"", superclass, ", ",
	     ascii[scope], ", ", STR_VAL(NULL), "));\n", NULL);
    
  if (is_global_frame ())
    global_init_statement (buf, FALSE);
  else 
    fileout (buf, 0, 0);
  return SUCCESS;
}

/* 
 *  Generate the C language declaration for a method function.
 */

int generate_method_c_declaration (char *buf) {
  fileout ("char *", 0, 0);
  return SUCCESS;
}

int generate_c_expr_store_arg_call (OBJECT *rcvr, METHOD *method, OBJECT *arg,
				    char *fn_name, int when,
				    ARGSTR *argstr) {
  if (is_global_frame () || IS_CONSTRUCTOR(method))
    fn_init (fmt_c_expr_store_arg_call (rcvr, method, arg, fn_name, argstr), FALSE);

  else
    fileout (fmt_c_expr_store_arg_call (rcvr, method, arg, fn_name, argstr), 0, when);


   return SUCCESS;
}

char arglistbuf[MAXMSG];

/* 
 *    Format a __ctalk_store_arg_object() function call for a complex
 *    C expression.  
 */

extern bool eval_arg_leading_typecast; /* declared and set in eval_arg.c. */
extern ARGSTR arg_leading_typecast;

char *fmt_c_expr_store_arg_call (OBJECT *rcvr, METHOD *method, OBJECT *arg,
				 char *fn_name, ARGSTR *argstr) {
  static char buf[MAXMSG];
  char t_object_buf[MAXLABEL], arg_to_obj_buf[MAXMSG];
  CVAR *c_var;
  CFUNC *c_fn;
  OBJECT *t_arg;

  if (((c_var = get_global_var (fn_name)) == NULL) &&
      ((c_var = get_local_var (fn_name)) == NULL) &&
      ((c_fn = get_function (fn_name)) == NULL)) {
    _warning ("fmt_c_expr_store_arg_call: Undefined or unprototyped C function %s.  Return class defaulting to Integer.\n",
	     fn_name);
    t_arg = create_object_init (INTEGER_CLASSNAME,
				INTEGER_SUPERCLASSNAME,
				arg -> __o_name,
				arg -> __o_name);
    if (argstr -> leading_typecast) {
      strcatx (t_object_buf, argstr -> typecast_expr, t_arg -> __o_name,
	       NULL);
      strcpy (t_arg -> __o_name, t_object_buf);
      __xfree (MEMADDR(t_arg -> __o_value));
      t_arg -> __o_value = strdup (t_arg -> __o_name);
    }
  } else {
    t_arg = arg;
  }

  if (str_eq (t_arg -> __o_name, fn_name)) {
    fmt_c_to_symbol_obj_call (rcvr, method, t_arg, arg_to_obj_buf);
  } else {
    /* this is normally used with the main message stack. */
    fmt_c_to_obj_call (message_stack (), argstr -> start_idx,
		       rcvr, method, t_arg, arg_to_obj_buf, argstr);
  }

  strcatx (buf, "__ctalk_arg (\"", rcvr -> __o_name, "\", \"",
	   method -> name, "\", ",
	   ascii[method -> n_params], ", ", 
	   arg_to_obj_buf, ");\n", NULL);

  return buf;

}

int generate_store_arg_call (OBJECT *rcvr, METHOD *method, OBJECT *arg,
			     int when) {
  if (is_global_frame () || IS_CONSTRUCTOR(method))
    fn_init (fmt_store_arg_call (rcvr, method, arg), FALSE);
  else
    fileout (fmt_store_arg_call (rcvr, method, arg), 0, when);

   return SUCCESS;
}

/* 
 *    Format a __ctalk_store_arg_object() function call.
 */

char *fmt_store_arg_call (OBJECT *rcvr, METHOD *method, OBJECT *arg) {
  static char buf[MAXMSG * 4];
  char argnamebuf[MAXMSG],
    rcvrnamebuf[MAXMSG],
    leading_newline,
    *argnamep;
  leading_newline = ' ';

  if ((arg -> scope & CVAR_VAR_ALIAS_COPY) ||
      (arg -> attrs & OBJECT_IS_FN_ARG_OBJECT)) {

    strcpy (argnamebuf, arg -> __o_value);
    while (*argnamebuf == '\"')
      TRIM_LITERAL (argnamebuf);
    argnamep = argnamebuf;
  } else {
    if ((arg -> __o_class == ct_defclasses -> p_string_class) &&
	(arg -> attrs & OBJECT_IS_STRING_LITERAL)) {
      escape_str_quotes (arg -> __o_value, argnamebuf);
      argnamep = argnamebuf;
    } else {
      if ((arg -> __o_class == ct_defclasses -> p_character_class) &&
	  (arg -> attrs & OBJECT_IS_LITERAL_CHAR)) {
	escape_str_quotes (arg -> __o_value, argnamebuf);
	argnamep = argnamebuf;
      } else {
      if ((arg -> __o_class == ct_defclasses -> p_expr_class) ||
	  (arg -> __o_class == ct_defclasses -> p_cfunction_class)) {
	/* 
	   There are many cases where new args are referred to only
	   by name, so use __o_name when referring to a possibly
	   new argument object. 

	   We can't escape quotes any earlier, because lexical
	   we have to wait until lexical is done with all of its
	   tokenizations.
	*/
	  if (strchr (arg -> __o_name, '"')) {
	    esc_expr_quotes (arg -> __o_name, argnamebuf);
	    argnamep = argnamebuf;
	  } else {
	    argnamep = arg -> __o_name;
	  }
	} else {
	  argnamep = arg -> __o_name;
	}
      }
    }
  }

  if (n_th_arg_CVAR_has_unary_inc_prefix || 
      n_th_arg_CVAR_has_unary_dec_prefix)
    leading_newline = '\n';    

  if ((rcvr -> __o_class == ct_defclasses -> p_expr_class) ||
      (rcvr -> __o_class == ct_defclasses -> p_cfunction_class)) {
    sprintf (buf, "%c__ctalk_arg (%s, \"%s\", %d, (void *)\"%s\");\n",
  	     leading_newline, rcvr->__o_name, method->name, 
	     method -> n_params, argnamep);
  } else {
    if ((rcvr -> __o_class == ct_defclasses -> p_string_class) &&
	lextype_is_LITERAL_T (rcvr -> __o_name)) {
      escape_str_quotes (rcvr -> __o_name, rcvrnamebuf);
      sprintf (buf, "%c__ctalk_arg (\"%s\", \"%s\", %d, (void *)\"%s\");\n",
  	       leading_newline, rcvrnamebuf, method->name, 
	       method -> n_params, argnamep);
    } else {
      sprintf (buf, "%c__ctalk_arg (\"%s\", \"%s\", %d, (void *)\"%s\");\n",
  	       leading_newline, rcvr->__o_name, method->name, 
	       method -> n_params, argnamep);
    }
  }
  return buf;

}

/* 
 *    Generate a __ctalk_arg_pop () function call.
 */

static char arg_cleanup_call [] = "__ctalk_arg_cleanup((void *)0);\n";

int generate_method_pop_arg_call (int when) {
  if (is_global_frame ())
    fn_init (arg_cleanup_call, FALSE);
  else
    fileout (arg_cleanup_call, 0, when);

  return SUCCESS;
}

/*
 *  Queue an init statement for output as a global init statement,
 *  a statement in a function init block, or a method init block.
 *  If, "promote," is true, then allow the statement to be promoted
 *  to the calling function's init block.  See the comments in 
 *  fninit.c for the levels of global_defer.
 */

int queue_init_statement (char *s, int promote, int global_defer) {
  if (is_global_frame ()) {
      global_init_statement (s, global_defer);
  } else {
    switch (interpreter_pass)
      {
      case method_pass:
	method_init_statement (s);
	break;
      case parsing_pass:
      case library_pass:
	fn_init (s, promote);
	break;
      default:
	break;
      }
  }
  return SUCCESS;
}

/* 
 *  Generate the ctalk lib function calls to save a method's 
 *  arguments and call the method.  If given in the preamble,
 *  include the statements in __ctalk_init().  
 */

static char constructor_arg_fn [] = CONSTRUCTOR_ARG_FN;
static char create_arg_fn [] = "__ctalkCreateArg";
static char arg_fn_basic [] = "__ctalk_arg";

int generate_primitive_method_call (OBJECT *o, METHOD *m,char *buf,int attrs) {
  int i;
  char *arg_fn;
  switch (interpreter_pass)
    {
    case method_pass:
      if (!strcmp (m -> name, "new")) {
	arg_fn = constructor_arg_fn;
      } else {
	arg_fn = create_arg_fn;
      }
      break;
    default:
      if (!strcmp (m -> name, "new")) {
	arg_fn = constructor_arg_fn;
      } else {
	arg_fn = arg_fn_basic;
      }
      break;
    }

  for (i = 0; i < m -> n_args; i++) {
    if (!strncmp (m -> args[i] -> obj -> __o_name, METHOD_ARG_ACCESSOR_FN, 
		  METHOD_ARG_ACCESSOR_FN_LENGTH)) {
      strcatx (buf, arg_fn, " (\"", o -> __o_name, "\", \"",
	       m -> name, "\", ", m -> args[i] -> obj -> __o_name, 
	       ");\n", NULL);
    } else {
      strcatx (buf, arg_fn, " (\"", o -> __o_name, "\", \"",
	       m -> name, "\", \"", 
	       m -> args[i] -> obj -> __o_name, "\");\n", NULL);
    }
    switch (interpreter_pass)
      {
      case method_pass:
	method_init_statement (buf);
	break;
      default:
	queue_init_statement (buf, FALSE, 3);
	break;
      }
  }
  strcatx (buf, "__ctalk_primitive_method (\"", o -> __o_name, "\", \"", 
	   m -> name, "\", ", ascii[attrs], ");\n", NULL);
    switch (interpreter_pass)
      {
      case method_pass:
	method_init_statement (buf);
	break;
      default:
	queue_init_statement (buf, FALSE, 3);
	break;
      }
  return SUCCESS;
}

/* like generate_primitive_method_call, above, but this fn
   uses only the nth arg to create a set of constructor
   statements. */
int generate_primitive_method_call_2 (OBJECT *o, METHOD *m,char *buf,
				      int attrs,
				      int nth_arg) {
  char *arg_fn;

  /* Note that the only method name here should be, "new" */
#if 0
  switch (interpreter_pass)
    {
    case method_pass:
      if (!strcmp (m -> name, "new")) {
	arg_fn = constructor_arg_fn;
      } else {
	arg_fn = create_arg_fn;
      }
      break;
    default:
      if (!strcmp (m -> name, "new")) {
	arg_fn = constructor_arg_fn;
      } else {
	arg_fn = arg_fn_basic;
      }
      break;
    }
#endif

  arg_fn = constructor_arg_fn;
  
  if (!strncmp (m -> args[nth_arg] -> obj -> __o_name,
		METHOD_ARG_ACCESSOR_FN, 
		METHOD_ARG_ACCESSOR_FN_LENGTH)) {
    strcatx (buf, arg_fn, " (\"", o -> __o_name, "\", \"",
	     m -> name, "\", ", m -> args[nth_arg] -> obj -> __o_name, 
	     ");\n", NULL);
  } else {
    strcatx (buf, arg_fn, " (\"", o -> __o_name, "\", \"",
	     m -> name, "\", \"", 
	     m -> args[nth_arg] -> obj -> __o_name, "\");\n", NULL);
  }
  switch (interpreter_pass)
    {
    case method_pass:
      method_init_statement (buf);
      break;
    default:
      queue_init_statement (buf, FALSE, 3);
      break;
    }
  strcatx (buf, "__ctalk_primitive_method (\"", o -> __o_name, "\", \"", 
	   m -> name, "\", ",
	   ascii[attrs], ");\n", NULL);
    switch (interpreter_pass)
      {
      case method_pass:
	method_init_statement (buf);
	break;
      default:
	queue_init_statement (buf, FALSE, 3);
	break;
      }
  return SUCCESS;
}

/*
 *  As above, but generates a standard method call from a primitive
 *  in cases where a subclass method replaces a primitive method.
 */
static char arg_pop_deref_call [] = "__ctalk_arg_pop_deref ();\n";

int generate_method_call_from_primitive (OBJECT *o, METHOD *m, char *buf, 
					 int attrs, int nth_arg) {

  if (!strncmp (m -> args[nth_arg] -> obj -> __o_name,
		METHOD_ARG_ACCESSOR_FN, 
		METHOD_ARG_ACCESSOR_FN_LENGTH)) {
      strcatx (buf, "__ctalk_arg (\"", o -> __o_name, "\", \"",
	       m -> name, "\", ", ascii[m -> n_params], 
	       ", ", m -> args[nth_arg] -> obj -> __o_name, ");\n", NULL);
    } else {
      strcatx (buf, "__ctalk_arg (\"", o -> __o_name, "\", \"", 
	       m -> name, "\", ", ascii[m -> n_params], 
	       ", \"", m -> args[nth_arg] -> obj -> __o_name, "\");\n", NULL);
    }
    queue_init_statement (buf, FALSE, 3);
  
  strcatx (buf, "__ctalk_method (\"", o -> __o_name,
	   "\", ", m -> selector, ", \"", m -> name, "\");\n", NULL);
  queue_init_statement (buf, FALSE, 3);
  queue_init_statement (arg_pop_deref_call, FALSE, 3);
  return SUCCESS;
}

/*
 *  As above, but generated when the constructor that replaces
 *  a primitive method is still a prototype.
 */
int generate_method_call_for_prototype (OBJECT *o, 
					char *method_selector,
					char *method_name, 
					char *arg) {

  char buf[MAXMSG];

  /*  Constructors take only one arg so far. */
  strcatx (buf, "__ctalk_arg (\"", o -> __o_name, "\", \"", 
	   method_name, "\", 1, \"", arg, "\");\n", NULL);
  queue_init_statement (buf, FALSE, 3);
  strcatx (buf, "__ctalk_method (\"", o -> __o_name, "\", ", 
	   method_selector, ", \"", method_name, "\");\n", 
	   NULL);
  queue_init_statement (buf, FALSE, 3);
  queue_init_statement (arg_pop_deref_call, FALSE, 3);
  return SUCCESS;
}

/*
 * DO NOT CHANGE THIS FORMAT! 
 *
 * Without also changing output_pre_method (), 
 * which depends on it.
 */
int fmt_fixup_info (char *buf, int line, char *selector, int newline_before) {
  char intbuf[0xff];
  if (newline_before)
    strcatx (buf, "\n# ", ctitoa (line, intbuf), " \"/",
	     FIXUPROOT, "/", selector, "\"\n", NULL);
  else
    strcatx (buf, "# ", ctitoa (line, intbuf), " \"/",
	     FIXUPROOT, "/", selector, "\"\n", NULL);

  return SUCCESS;
}


/*
 *  Once again, there seems to be a problem with line markers
 *  in DJGPP, which generates warning messages when returning
 *  to a file from more than one included files.  For now,
 *  simply omitting the flag in DJGPP line markers should
 *  avoid the warning messages.
 */

#define FLI_FLAGSTR(f) ((f == 0) ? "0" : \
			((f == 1) ? "1" : "2"))

void fmt_line_info (char *buf, int line, char *file, int flag, 
		   int newline_before) {
#ifdef __DJGPP__
    if (newline_before)
      sprintf (buf, "\n# %d \"%s\"\n", line, file);
    else
      sprintf (buf, "# %d \"%s\"\n", line, file);
#else
    char intbuf1[0xff]/*, intbuf2[0xff]*/;

    if (!file || *file == '\0') {
      if (flag) {
	if (newline_before)
	  strcatx (buf, "\n# ", ctitoa (line, intbuf1), " \"\" ",
		   FLI_FLAGSTR(flag), "\n", NULL);
	else
	  strcatx (buf, "# ", ctitoa (line, intbuf1), " \"\" ",
		   FLI_FLAGSTR(flag), "\n", NULL);
      } else {
	if (newline_before)
	  strcatx (buf, "\n# ", ctitoa (line, intbuf1), "\n", NULL);
	else
	  strcatx (buf, "# ", ctitoa (line, intbuf1), "\n", NULL);
      }
    } else {
      if (flag) {
	if (newline_before)
	  strcatx (buf, "\n# ", ctitoa (line, intbuf1), " \"", file, 
		   "\" ", FLI_FLAGSTR(flag), "\n", NULL);
	else
	  strcatx (buf, "# ", ctitoa (line, intbuf1), " \"", file, 
		   "\" ", FLI_FLAGSTR (flag), "\n", NULL);
      } else {
	if (newline_before)
	  strcatx (buf, "\n# ", ctitoa (line, intbuf1), " \"", file,
		   "\"\n", NULL);
	else
	  strcatx (buf, "# ", ctitoa (line, intbuf1), " \"", file,
		   "\"\n", NULL);
      }
    }
#endif
}

void fmt_loop_line_info (char *buf, int line, int newline_before) {
  char intbuf[0xff];
  if (newline_before)
    strcatx (buf, "\n# ", ctitoa (line, intbuf), "\n", NULL);
  else
    strcatx (buf, "# ", ctitoa (line, intbuf), "\n", NULL);
}

int generate_set_global_variable_call (char *name, char *classname) {

  static char buf[MAXLABEL];

  strcatx (buf, "__ctalk_set_global (\"", name, "\", \"", classname,
	   "\");\n", NULL);
  queue_init_statement (buf, FALSE, 3);
  return SUCCESS;
}

int generate_set_local_variable_call (char *name, char *classname) {

  char buf[MAXLABEL];

  if (!strncmp (name, METHOD_ARG_ACCESSOR_FN, 
		METHOD_ARG_ACCESSOR_FN_LENGTH)) {
    strcatx (buf, "__ctalk_set_local (", name, ");\n", NULL);
  } else {
    strcatx (buf, "__ctalk_set_local_by_name (\"", name, "\");\n", NULL);
    }

  queue_init_statement (buf, FALSE, 0);

  return SUCCESS;
}

int basic_attrs_of (CVAR *c, CVAR *c_basic) {
  int attrs_out = 0;
   /*  Helps avoid a namespace collision between
       a var and a struct member of the same name. */
  if (c -> attrs & CVAR_ATTR_STRUCT_MBR)
    attrs_out |= CVAR_ATTR_STRUCT_MBR;
  if (c -> attrs & CVAR_ATTR_CVARTAB_ENTRY)
    attrs_out |= CVAR_ATTR_CVARTAB_ENTRY;
  if (c -> attrs & CVAR_ATTR_ARRAY_DECL)
    attrs_out |= CVAR_ATTR_ARRAY_DECL;

  if (c -> type_attrs & CVAR_TYPE_TYPEDEF) {
    c -> type_attrs |= c_basic -> type_attrs;
  }
  return attrs_out;
}

/*
 *   The prototype is: 
 *   REGISTER_C_METHOD_ARG (char *decl, char *type,
 *                                  char *qualifier, char *qualifier2,
 *                                  char *storage_class, 
 *                                  char *name,
 *                                  int type_attrs,
 *                                  int n_derefs,
 *                                  int scope,
 *                                  int attrs,
 *                                  (void *)var);
 *
 *   __ctalk_method () deletes the cvars after the method call.
 */

CVAR *basic_type_of (CVAR *__c) {
  CVAR *__c_basic, *__c_basic2 = NULL;

  if (get_type_conv_name  (__c -> type))
    return __c;

  if ((__c_basic = get_typedef (__c -> type)) != NULL) {

    while (TRUE) {

      if (get_type_conv_name (__c_basic -> type))
	return __c_basic;

      if ((__c_basic2 = get_typedef (__c_basic -> type)) == NULL) {
	return __c_basic;
      } else {
	if (__c_basic2 == __c_basic) {
	  return __c_basic;
	} else {
	  __c_basic = __c_basic2;
	}
      }
    }
  }
  return __c;
}

char *__fmt_c_method_arg_call (char *namebuf, char *labelbuf,
			       int scope, CVAR *c, 
			       CVAR *c_basic, char *buf_out) {
  int have_subscript_expr;
  int eff_n_derefs;
  int attrs_out = 0;
  CVAR *c_tg;

  if (strstr (labelbuf, "[") && 
      !strstr (namebuf, "[")) {
    have_subscript_expr = TRUE;
  } else {
    have_subscript_expr = FALSE;
  }

  /* 
   *  E.g., another char **alias to a char array[][],
   *  which uses a subscript like alias[n] in an 
   *  expression, which we simply treat as a char *. 
   *  There aren't enough examples of multi-dimensional
   *  arrays to do anything further yet.
   * 
   *  TODO - We should check the number of subscripts,
   *  in case there's an expression like alias[m][n].
   */
  if (!(scope & SUBSCRIPT_OBJECT_ALIAS)) {
    if (str_eq (c -> type, "char") && (c -> n_derefs == 2) && 
	have_subscript_expr) {
      have_subscript_expr = FALSE;
      eff_n_derefs = c -> n_derefs - 1;
    } else {
      eff_n_derefs = c -> n_derefs;
    }
  } else {
    eff_n_derefs = c -> n_derefs;
  }

  attrs_out = basic_attrs_of (c, c_basic);

  /*
   *  Do a fixup in case of  a derived type.
   */
  if ((c -> type_attrs == 0) && (c_basic -> type_attrs != 0))
    c -> type_attrs = c_basic -> type_attrs;

  if (c == c_basic)
    c_tg = c;
  else
    c_tg = c_basic;  /* tg is short for "type guide" */
  if (*c_tg -> qualifier2 == 0) {
    if (*c_tg ->  qualifier == 0) {
      if (*c_tg -> decl == 0) {
	sprintf (buf_out, 
		 "%s (\"%s\", \"%s\", \"%s\", %d, %d, %d, %d, %d, (void *)%s%s);\n",
		 REGISTER_C_METHOD_ARG_D,
		 ((*c_basic->type) ? c_basic -> type : NULLSTR),
		 ((*c_basic->storage_class) ? c_basic -> storage_class : NULLSTR),
		 namebuf,
		 c -> type_attrs,
		 eff_n_derefs,
		 ((have_subscript_expr) ? 0 : c -> initializer_size),
		 scope,
		 attrs_out,
		 ((c -> n_derefs && !have_subscript_expr &&
		   !struct_expr_terminal_array_member (labelbuf))? "" : "&"),
		 labelbuf);
      } else {
	sprintf (buf_out, 
		 "%s (\"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d, %d, %d, (void *)%s%s);\n",
		 REGISTER_C_METHOD_ARG_C,
		 ((*c_basic->decl) ? c_basic -> decl : NULLSTR),
		 ((*c_basic->type) ? c_basic -> type : NULLSTR),
		 ((*c_basic->storage_class) ? c_basic -> storage_class : NULLSTR),
		 namebuf,
		 c -> type_attrs,
		 eff_n_derefs,
		 ((have_subscript_expr) ? 0 : c -> initializer_size),
		 scope,
		 attrs_out,
		 ((c -> n_derefs && !have_subscript_expr &&
		   !struct_expr_terminal_array_member (labelbuf))? "" : "&"),
		 labelbuf);
      }
    } else {
      sprintf (buf_out, 
	       "%s (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d, %d, %d, (void *)%s%s);\n",
	       REGISTER_C_METHOD_ARG_B,
	       ((*c_basic->decl) ? c_basic -> decl : NULLSTR),
	       ((*c_basic->type) ? c_basic -> type : NULLSTR),
	       ((*c_basic->qualifier) ? c_basic -> qualifier : NULLSTR),
	       ((*c_basic->storage_class) ? c_basic -> storage_class : NULLSTR),
	       namebuf,
	       c -> type_attrs,
	       eff_n_derefs,
	       ((have_subscript_expr) ? 0 : c -> initializer_size),
	       scope,
	       attrs_out,
	       ((c -> n_derefs && !have_subscript_expr &&
		 !struct_expr_terminal_array_member (labelbuf))? "" : "&"),
	       labelbuf);
    }
  } else {

    /* leave this for when we don't need four separate int buffers. */
    sprintf (buf_out, 
	     "%s (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d, %d, %d, (void *)%s%s);\n",
	   REGISTER_C_METHOD_ARG,
	     ((*c_basic->decl) ? c_basic -> decl : NULLSTR),
	     ((*c_basic->type) ? c_basic -> type : NULLSTR),
	     ((*c_basic->qualifier) ? c_basic -> qualifier : NULLSTR),
	     ((*c_basic->qualifier2) ? c_basic -> qualifier2 : NULLSTR),
	     ((*c_basic->storage_class) ? c_basic -> storage_class : NULLSTR),
	     namebuf,
	   c -> type_attrs,
	   eff_n_derefs,
	   ((have_subscript_expr) ? 0 : c -> initializer_size),
	   scope,
	   attrs_out,
	   ((c -> n_derefs && !have_subscript_expr &&
	     !struct_expr_terminal_array_member (labelbuf))? "" : "&"),
	   labelbuf);
  }

  return buf_out;
}

static AGGREGATE_EXPR_TYPE aggregate_var_type (char *expr) {
  char *s = expr;

  while (*s) {
    if (*s == '.' || (*s == '-' && *(s + 1) == '>'))
      return aggregate_expr_type_struct;
    else if (*s == '[')
      return aggregate_expr_type_array;
    ++s;
  }
  return aggregate_expr_type_null;
}

char *fmt_register_c_method_arg_call (CVAR *c, char *label, int scope,
					char *buf_out) {

  char namebuf[MAXLABEL];
  char labelbuf[MAXLABEL];
  CVAR *c_w = c;
  AGGREGATE_EXPR_TYPE aet;

  trimstr (label, labelbuf);

  /*
   *  If a terminal struct member, then print immediately.
   */
  if ((c -> attrs & CVAR_ATTR_STRUCT_MBR) &&
     !IS_STRUCT_OR_UNION(c)) {
    return __fmt_c_method_arg_call (labelbuf, labelbuf, scope, 
				    c, basic_type_of(c), buf_out);
  } else if (((aet = aggregate_var_type (labelbuf)) ==
	      aggregate_expr_type_struct) &&
	     (c -> type_attrs == CVAR_TYPE_TYPEDEF)) {
    /* This is a failsafe if we have a CVAR derived from something
       like a "typedef union..." that doesn't contain any information
       about the aggregate declaration itself.... */
    if ((c_w = get_typedef (c -> type)) == NULL) {
      /* ... but still use the CVAR that we have if we don't find the
         aggregate declaration */
      c_w = c;
    }
  }
  
  /*
   *  If we have an aggregate type, then derive the full aggregate
   *  name.
   */
  if ((c_w -> type_attrs & CVAR_TYPE_STRUCT) ||
      (c_w -> type_attrs & CVAR_TYPE_UNION) ||
      (c_w -> attrs & CVAR_ATTR_STRUCT_PTR) ||
      (c_w -> attrs & CVAR_ATTR_STRUCT_DECL)) {
    /*
     *  Struct member.
     */
    if (aet == aggregate_expr_type_struct) {
      CVAR *struct_var, *struct_defn, *mbr_var;
      char *s;
      strcpy (namebuf, labelbuf);
      for (s = namebuf; *s; s++)
	if (!isalnum ((int) *s) && *s != '_')
	  break;
      *s = 0;
      if (((struct_var = get_local_var (namebuf)) != NULL) ||
	  ((struct_var = get_global_var (namebuf)) != NULL)) {

	/*
	 *  If we have a pointer to a struct, then 
	 *  find the definition.
	 */

	if ((struct_var -> n_derefs) && (struct_var -> members == NULL)) {
	  if (((struct_defn = get_local_struct_defn (struct_var -> type)) != NULL) ||
	      ((struct_defn = get_global_struct_defn (struct_var -> type)) != NULL) ||
	      ((struct_defn = have_struct (struct_var -> type)) != NULL)) {
	    if ((mbr_var = struct_member_from_expr (labelbuf, struct_defn))
		!= NULL) {
	      return __fmt_c_method_arg_call (labelbuf, labelbuf, scope,
					      mbr_var,
					      basic_type_of(mbr_var),
					      buf_out);
	    }
	  }
	} else {
	  /*
	   *  Struct declaration, find the definition.
	   */
	  if (struct_var -> members == NULL) {
	    if (((struct_defn = get_local_struct_defn (struct_var -> type)) != NULL) ||
		((struct_defn = get_global_struct_defn (struct_var -> type)) != NULL)||
		((struct_defn = have_struct (struct_var -> type)) != NULL)){
	      if ((mbr_var = struct_member_from_expr (labelbuf, struct_defn))
		  != NULL) {
		return __fmt_c_method_arg_call (labelbuf, labelbuf, scope,
						mbr_var,
						basic_type_of(mbr_var),
						buf_out);
	      }
	    }
	  } else {
	    /*
	     *  The variable is the struct declaration.
	     */
	    if ((mbr_var = struct_member_from_expr (labelbuf, struct_var))
		!= NULL) {
	      return __fmt_c_method_arg_call (labelbuf, labelbuf, scope,
					      mbr_var,
					      basic_type_of(mbr_var),
					      buf_out);
	    }
	  }
	}
	if (*namebuf == 0) {
	  _warning 
	    ("warning: fmt_register_c_method_arg_call: %s contains undefined member of struct or union \"%s.\"\n",
	     labelbuf,
	     c_w -> name);
	  strcpy (namebuf, labelbuf);
	}
      } else {
	_warning ("fmt_register_c_method_arg_call: struct not found.\n");
	strcpy (namebuf, labelbuf);
      }
    } else {
      return __fmt_c_method_arg_call (labelbuf, labelbuf, scope,
				      c_w, 
				      basic_type_of(c_w),
				      buf_out);

    }
    /*
     *  Array member.  Not used at present, should be removed.
     */
    if (aet == aggregate_expr_type_array) {
      /*
       *  The name and the label should be the same, unless 
       *  the variable has an expression; for example, 
       *  argv[__ctalk_to_c_int (__ctalkEvalExpr (myInt))].
       *  For now, if the label contains "__ctalkEvalExpr", use
       *  the CVAR name as the name.
       *  Register_c_variable () takes care that the CVAR 
       *  has the correct array member and data type.
       *  TO DO - We could probably move the aggregate ->
       *  aggregate member translations to separate functions
       *  for structs and arrays.
       */
      if (strstr (labelbuf, EVAL_EXPR_FN)) {
	strcpy (namebuf, c_w -> name);
      } else {
	strcpy (namebuf, labelbuf);
      }
    }
  } else {
    strcpy (namebuf, c_w -> name);
  }
  if ((c_w -> attrs & CVAR_ATTR_ARRAY_DECL) &&
      (aet == aggregate_expr_type_array))
    scope |= SUBSCRIPT_OBJECT_ALIAS;
  return __fmt_c_method_arg_call (namebuf, labelbuf, scope, 
				  c_w, 
				  basic_type_of(c_w),
				  buf_out);
}

int generate_register_c_method_arg_call (CVAR *c, char *label, int scope,
					 int when) {
  char buf[MAXMSG];
  fileout (fmt_register_c_method_arg_call (c, label, scope, buf), 0, when);
  return SUCCESS;
}

/*
 *  Write the generated output to the output file.
 */

void __fileout (char *s) {
  if (interpreter_pass == method_pass) {
    write_tmp (s);
  } else {
    errno = 0;
    if (fputs (s, f_output) < 0) {
      if (errno != SUCCESS) {
	_error ("ctalk: write error %s.", strerror(errno));
      }
    }
  }
}

/* file out the source argument + 2 newlines. */
void __fileout_cache (char *s) {
  errno = 0;
  if ((fputs (s, f_output) < 0) ||
      (fputs ("\n\n", f_output)) < 0) {
    if (errno != SUCCESS) {
      _error ("ctalk: write error %s.", strerror(errno));
    }
  }
}

void __fileout_import (char *s) {
  __fileout_cache (s);
}

int last_fileout_stmt;

/*
 *  Output the generated ctalk code, delaying the output 
 *  if necessary for insertion elsewhere in the output.
 */

void fileout (char *s, int now, int when) {

  PENDING *p = NULL;
  PENDING *_tmp_ptr;
  int parser_lvl;

  if (interpreter_pass == expr_check) return;

  parser_lvl = CURRENT_PARSER -> level;

 if (when) {

   if ((p = __new_pending_stmt (s)) == NULL)
      _error ("fileout: %s.\n", strerror (errno));
      

    p -> n = when;
    p -> parser_lvl = parser_lvl;

    if (!pending_statements) {
      pending_statements = pending_head_ptr = p;
    } else {
      pending_head_ptr -> next = p;
      p -> prev = pending_head_ptr;
      pending_head_ptr = p;
    }
  }

  for (p = pending_statements; p; p = p -> next) {
  fileout_pending_loop:
    if (last_fileout_stmt) {
      if ((p -> parser_lvl == parser_lvl) &&
 	  ((p -> n < last_fileout_stmt) &&
 	   (p -> n >= parser_output_ptr))) {

	if (interpreter_pass == method_pass) {
	  buffer_method_statement (p -> stmt, p -> n);
	} else {
	  if (interpreter_pass == c_fn_pass) {
	    buffer_fn_args (p, message_stack_at (p -> n) -> name);
	  } else {
	    if (buffer_function_output) {
	      buffer_function_statement (p -> stmt, p -> n);
	    } else {
	      __fileout (p -> stmt);
	    }
	  }
	}

	if (p == pending_head_ptr)
	  pending_head_ptr = p -> prev;
	if (p -> next)
	  p -> next -> prev = p -> prev;
	if (p -> prev)
	  p -> prev -> next = p -> next;
	if (p == pending_statements) {
	  if (p -> next && IS_PENDING(p -> next)) {
	    pending_statements = p -> next;
	  } else {
	    pending_statements = pending_head_ptr = NULL;
	  }
	}
	_tmp_ptr = p -> next;
	__delete_pending_stmt (p);
	if ((p = _tmp_ptr) == NULL)
	  goto fileout_now;
	else
	  goto fileout_pending_loop;
      }
    }
    if (!pending_head_ptr)
      break;
    if (p == p -> next)
      break;
  }

 fileout_now:

  if (now)
    last_fileout_stmt = now;

  if (!when) {                 /* Output in statement above. */
    if (interpreter_pass == library_pass) {
      __fileout (s);
    } else {
      if (interpreter_pass == method_pass) {
	buffer_method_statement (s, now);
      } else {
	if (buffer_function_output) {
	  buffer_function_statement (s, now);
	} else {
	  __fileout (s);
	}
      }
    }
  }

}

/*
 *  NOTE  If a control block function has inserted a 
 *  closing brace past the end of the frame, output the
 *  pending statement below by looking ahead for pending
 *  statements in the next frame.
 *  If there  are insertions other than closing 
 *  braces,  then the functions that do the insertions 
 *  will have to use stack splicing.
 */

int output_frame (MESSAGE_STACK messages,
		  int frame, int start, int end, int end_next) {

  int j;
  int parser_lvl;
  MESSAGE *m;
  PENDING *p;

  if (interpreter_pass == expr_check) 
    return SUCCESS;

  parser_lvl = CURRENT_PARSER -> level;

  for (parser_output_ptr = start;
       parser_output_ptr > end; parser_output_ptr--) {
    m = messages[parser_output_ptr];
    if (!m -> output) {
      fileout (m -> name, parser_output_ptr, 0);
      ++m -> output;
    }
  }

  if ((p = pending_statements) == NULL)
    return SUCCESS;

  for (j = end; j > end_next; j--) {
  frame_loop_outer:
    while (p) {
    frame_loop_inner:
      if ((p -> parser_lvl == parser_lvl) &&
	  (p -> n == j)) {
	if (interpreter_pass == method_pass) {
	  buffer_method_statement (p -> stmt, p -> n);
	} else {
	  if (interpreter_pass == c_fn_pass) {
	    buffer_fn_args (p, message_stack_at (p -> n) -> name);
	  } else {
	    if (buffer_function_output) {
	      buffer_function_statement (p -> stmt, p -> n);
	    } else {
	      __fileout (p -> stmt);
	    }
	  }
	}

	if (p == pending_head_ptr) {
	  pending_head_ptr = p -> prev;
	  if (p == pending_statements) {
	    __delete_pending_stmt (p);
	    pending_head_ptr = pending_statements = NULL;
	    return SUCCESS;
	  } else {
	    __delete_pending_stmt (p);
	    --j;
	    goto frame_loop_outer;
	  }
	} /* if (p == pending_head_ptr) */
	
	if (p -> next)
	  p -> next -> prev = p -> prev;
	if (p -> prev)
	  p -> prev -> next = p -> next;
	if (p == pending_statements) {
	  if (p -> next) {
	    pending_statements = p -> next;
	    __delete_pending_stmt (p);
	    p = pending_statements;
	    goto frame_loop_inner;
	  } else {
	    pending_statements = pending_head_ptr = NULL;
	    __delete_pending_stmt (p);
	    return SUCCESS;
	  }
	} /* if (p == pending_statements) */
	p = p -> next;
	if (p -> prev) __delete_pending_stmt (p -> prev);
	goto frame_loop_inner;
      }
      p = p -> next;
    }
  }

  return SUCCESS;
}

/*
 *
 *  Note that method definitions are always in the global 
 *  __ctalk_init ();
 *
 */

int generate_instance_method_param_definition_call (char *classname, 
						    char *name,
						    char *selector,
					   char *paramName, char *paramClass,
					   int param_is_pointer,
					   int param_is_pointerpointer) {
  char buf[MAXMSG];
  /* this uses sprintf because some of the arguments could be 
     NULL, esp. paramName, paramClass for a prefix method. */
  sprintf (buf, "__ctalkInstanceMethodParam (\"%s\", \"%s\", %s, \"%s\", \"%s\", %d);\n",
      classname, name, selector, paramClass, paramName, param_is_pointer);

    global_init_statement (buf, FALSE);

  return SUCCESS;
}

int generate_class_method_param_definition_call (char *classname, 
						    char *name,
						    char *selector,
					   char *paramName, char *paramClass,
					   int param_is_pointer,
					   int param_is_pointerpointer) {
  char buf[MAXMSG];
  /* before changing this make sure that none of the arguments are NULL,
     esp. paramName, paramClass for a prefix method. */
  /* Also see the comment about strcatx in
     generate_instance_method_param_definition_call. */
  sprintf (buf, "__ctalkClassMethodParam (\"%s\", \"%s\", %s, \"%s\", \"%s\", %d);\n",
      classname, name, selector, paramClass, paramName, param_is_pointer);
    global_init_statement (buf, FALSE);

  return SUCCESS;
}

/****
 ****    
 ****    METHOD DEFINITION CALLS
 ****
 ****    Method definition calls should be output in
 ****    __ctalk_init ().
 ****    
 ****/

int method_definition_attributes (METHOD *m) {
  int i = 0;
  if (m -> varargs) i |= METHOD_VARARGS_ATTR;
  if (m -> prefix) 
    i |= METHOD_PREFIX_ATTR;
  if (m -> no_init) i |= METHOD_NOINIT_ATTR;
  return i;
}

/*
 *    Generate a method definition call to be included in 
 *    the global init.
 */

int generate_instance_method_definition_call (char *classname, char *name,
			      char *funcname, int required_args, int attrs) {
  char buf[MAXMSG];

  strcatx (buf, "__ctalkDefineInstanceMethod (\"", classname, "\", \"",
	   name, "\", ", funcname, ", ", 
	   ascii[required_args], ", ", ascii[attrs], ");\n", NULL);

  global_init_statement (buf, FALSE);

  return SUCCESS;
}

int generate_class_method_definition_call (char *classname, char *name,
				     char *funcname, int required_args) {
  char buf[MAXMSG];

  strcatx (buf, "__ctalkDefineClassMethod (\"", classname, "\", \"",
	   name, "\", ", funcname, ", ",
	   ascii[required_args], ");\n", NULL);

  global_init_statement (buf, FALSE);

  return SUCCESS;
}

/*
 *  Generate an instance method return class call in __ctalk_init ().
 */

int generate_instance_method_return_class_call (char *class, 
						char *methodName, 
						char *selector,
						char *returnClass, 
						int nParams) {
  char buf[MAXMSG];
  strcatx (buf, "__ctalkInstanceMethodInitReturnClass (\"", class, "\", \"",
	   methodName, "\", ", selector, ", \"", returnClass, "\", ",
	   ascii[nParams], ");\n", NULL);


    global_init_statement (buf, FALSE);

  return SUCCESS;
}

int generate_class_method_return_class_call (char *class, char *methodName, 
					     char *returnClass,
					     int n_params) {
  char buf[MAXMSG];
  strcatx (buf, "__ctalkClassMethodInitReturnClass (\"", class, "\", \"",
	   methodName, "\", \"", returnClass, "\", ",
	   ascii[n_params], ");\n", NULL);

    global_init_statement (buf, FALSE);

  return SUCCESS;
}

/*
 *  Generate a global method definition call in __ctalk_init ().
 *  These calls are used define primitive methods.  At the 
 *  moment, they are the same as a normal method definition
 *  call. 
 */

int generate_global_method_definition_call (char *classname, char *name,
			    char *funcname, int required_args, int attrs) {
  char buf[MAXMSG];

  strcatx (buf, "__ctalkDefineInstanceMethod (\"", classname, "\", \"",
	   name, "\", ", funcname, ", ", ascii[required_args], 
	   ", ", ascii[attrs], ");\n", NULL);
  global_init_statement (buf, FALSE);
  return SUCCESS;
}

/****
 ****    
 ****    CLASS DEFINITION CALLS
 ****
 ****    Class definition calls should also be output in
 ****    __ctalk_init ().
 ****    
 ****/

/*
 *   Generate an imported class definition call in the function or 
 *   global initialization.  
 *
 *   Class initialization goes in the initialization of the parent
 *   parser.  If there are superclass initializations at this
 *   level (from sub-parsers), transfer those to the parent level
 *   also.
 */

int generate_primitive_class_definition_call (OBJECT *o, METHOD *m) {
  int i;
  char buf[MAXMSG];
  PARSER *p;

  p = CURRENT_PARSER;

  if (p -> init) {  
    /* 
     *   Add superclass initializations from sub-parsers
     *   to the parent level.                            
     */
    LIST *l, *l_arg, *l_init, *l_method;
    char *s, pattern[MAXMSG];

    /*
     *  Make sure the patterns matches the __ctalk_arg ()
     *  call, below, and __ctalkDefineInstanceMethod ().
     *  This pattern can be used for recursive 
     *  primitive calls, but the routine promotes only 
     *  class initializations specifically.  That means
     *  we also have to promote the method definitions.
     */
    for (l = p -> init; l; l = l -> next) {
      /*
       *  The length of the loop varies, so make sure we 
       *  have a valid list element.
       */
      if (!l || !IS_LIST (l) || !l -> data) break;

      strcatx (pattern, ", \"", m -> name, "\", ", NULL);

      if ((s = strstr ((char *)l -> data, pattern)) != NULL) {

 	l_arg = list_unshift (&(p->init));
 	l_init = list_unshift (&(p->init));

	global_init_statement ((char *)l_arg -> data, FALSE);
	global_init_statement ((char *)l_init -> data, FALSE);

	l = l_init -> next;
	delete_list_element (l_arg);
	delete_list_element (l_init);
      }

      if ((s = strstr ((char *)l -> data, "__ctalkDefineInstanceMethod ("))
	  != NULL) {
	l_method = list_unshift (&(p->init));
	global_init_statement ((char *)l_method -> data, FALSE);
	delete_list_element (l_method);
      }
    }
  }

  for (i = 0; i < m -> n_args; i++) {
    strcatx (buf, "__ctalk_arg (\"", o -> __o_name, "\", \"", 
	     m -> name, "\", ",
	     ascii[m -> n_params], ", \"",
	     m -> args[i] -> obj -> __o_name, "\");\n", NULL);
	     global_init_statement (buf, FALSE);
  }
  strcatx (buf, "__ctalk_primitive_method (\"", o -> __o_name, "\", \"",
	   m -> name, "\", 0);\n", NULL);
  global_init_statement (buf, FALSE);

  return SUCCESS;
}

void generate_primitive_class_definition_call_from_init (char *classname,

							 char *superclassname) {
  char buf[MAXMSG];

  strcatx (buf, "__ctalk_arg (\"", superclassname, "\", \"class\", 1, \"",
	   classname, "\");\n", NULL);

  global_init_statement (buf, FALSE);

  strcatx (buf, "__ctalk_primitive_method (\"", superclassname, 
     "\", \"class\", 0);\n", NULL);
	   
  global_init_statement (buf, FALSE);

}


/*
 *  The prototype of the library call is:
 *
 *  __ctalkRegisterCTypedef (char *type, 
 *                           char *qualifier,
 *                           char *qualifier2,
 *                           char *qualifier3,
 *                           char *qualifier4,
 *                           char *storage_class,
 *                           char *name,
 *                           int n_derefs,
 *                           int attrs,
 *                           int is_unsigned,
 *                           int scope);
 */

int generate_register_typedef_call (CVAR *c) {
  char buf[MAXMSG];

  sprintf (buf, 
	   "__ctalkRegisterCTypedef (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d, %d);\n",
	   ((*(c -> type)) ? c -> type : ""),
	   ((*(c -> qualifier)) ? c -> qualifier : ""),
	   ((*(c -> qualifier2)) ? c -> qualifier2 : ""),
	   ((*(c -> qualifier3)) ? c -> qualifier3 : ""),
	   ((*(c -> qualifier4)) ? c -> qualifier4 : ""),
	   ((*(c -> storage_class)) ? c -> storage_class : ""),
	   ((*(c -> name)) ? c -> name : ""),
	   c -> n_derefs,
	   c -> attrs,
	   c -> is_unsigned,
	   c -> scope);

  global_init_statement (buf, FALSE);
  return SUCCESS;
}

int generate_define_class_variable_call (char *class_rcvr, char *name, 
					 char *class, char *init) {
  static char buf[MAXMSG];

  /* Check if we've already written the init from the cache. */
  if (class_or_instance_var_init_is_output_2 
      (class_rcvr, "class", name))
    return SUCCESS;

  set_class_and_instance_var_init_output_2 (class_rcvr, "class", name);

  /* Before replacing this with strcatx, make sure that none of the
     arguments are NULL. */
  sprintf(buf,"__ctalkDefineClassVariable (\"%s\", \"%s\", \"%s\", \"%s\");\n",
	  ((class_rcvr) ? class_rcvr : ""),
	  ((name) ? name : ""),
	  /*
	   *  Must have a class for the variable value.
	   */
	  ((class) ? class : class_rcvr),
	  ((init) ? init : ""));

  global_init_statement (buf, 1);

  return SUCCESS;
}

int generate_define_instance_variable_call (char *class_rcvr, char *name, 
					 char *class, char *init) {
  static char buf[MAXMSG];

  /* Again, check if we've already written the init from the cache. */
  if (class_or_instance_var_init_is_output_2 
      (class_rcvr, "instance", name))
    return SUCCESS;

  set_class_and_instance_var_init_output_2 (class_rcvr, "instance", name);

  /* Also here - before replacing this with strcatx, make sure that none of the
     arguments are NULL. */
  sprintf(buf,"__ctalkDefineInstanceVariable (\"%s\", \"%s\", \"%s\", \"%s\");\n",
	  ((class_rcvr) ? class_rcvr : ""),
	  ((name) ? name : ""),
	  ((class) ? class : ""),
	  ((init) ? init : ""));
  global_init_statement (buf, 1);

  return SUCCESS;
}

/*
 *  Should call is_struct_or_union_expr () first.
 *
 *  Also checks for <fn> () -> and <fn> () . expressions.
 */
char *struct_or_union_expr (MESSAGE_STACK messages, 
			    int start_label_idx,
			    int stack_end_idx, int *expr_end_idx) {
  static char exprbuf[MAXMSG];
  int j, next_tok_idx, close_paren_idx, j_1;

  strcpy (exprbuf, M_NAME(messages[start_label_idx]));
  j = start_label_idx - 1;
  if ((next_tok_idx = nextlangmsg (messages, start_label_idx)) != ERROR) {
    if (M_TOK(messages[next_tok_idx]) == OPENPAREN) {
      if ((close_paren_idx = match_paren (messages, next_tok_idx,
					  stack_end_idx)) != ERROR) {
	if ((next_tok_idx = nextlangmsg (messages, close_paren_idx)) 
	    != ERROR) {
	  if ((M_TOK(messages[next_tok_idx]) == PERIOD) ||
	      (M_TOK(messages[next_tok_idx]) == DEREF)) {
	    for (j_1 = start_label_idx - 1; j_1 > next_tok_idx; j_1--) {
	      if (M_ISSPACE(messages[j_1]))
		continue;
	      strcat (exprbuf, M_NAME(messages[j_1]));
	      j = next_tok_idx;
	    }
	  }
	}
      }
    }
  }
  for (; j > stack_end_idx; j--) {
    if (M_ISSPACE(messages[j]))
      continue;
    if ((M_TOK(messages[j]) == LABEL) ||
	(M_TOK(messages[j]) == PERIOD) ||
	(M_TOK(messages[j]) == DEREF))
      strcat (exprbuf, M_NAME(messages[j]));
    else
      break;
  }
  *expr_end_idx = j;
  return exprbuf;
}

char *subscript_cvar_registration_expr (MESSAGE_STACK messages, 
					int start_label_idx,
					int stack_end_idx, 
					int *expr_end_idx) {
  static char exprbuf[MAXMSG];
  int j, n_blocks;

  strcpy (exprbuf, M_NAME(messages[start_label_idx]));
  for (j = start_label_idx - 1, n_blocks = 0; j > stack_end_idx; j--) {
    if (M_ISSPACE(messages[j]))
      continue;
    strcat (exprbuf, M_NAME(messages[j]));
    switch (M_TOK(messages[j]))
      {
      case ARRAYOPEN:
	++n_blocks;
	break;
      case ARRAYCLOSE:
	if (--n_blocks == 0)
	  goto handle_subscript_end;
	break;
      }
  }
 handle_subscript_end:
  *expr_end_idx = j;
  return exprbuf;
}


