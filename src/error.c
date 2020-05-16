/* $Id: error.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

extern char appname[];                 /* Declared in main.c.         */
extern char input_source_file[];
extern I_PASS interpreter_pass;
extern int error_line, error_column;   /* Declared in errorloc.c.     */

extern int library_input;              /* Declared in class.c.        */

extern int nolinemarker_opt;           /* Declared in main.c.         */

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;

/* Library prototypes. */
extern void _error (char *,...);
extern void _error_out (char *);
extern void _warning (char *,...);

LIST *unresolved_labels = NULL;

/*
 *  Return the name of the source file depending on which
 *  pass the interpreter is in.
 */

char *source_filename (void) {

  switch (interpreter_pass)
    {
    case preprocessing_pass:
      return __source_filename ();
      break;
    case var_pass:
    case parsing_pass:
    case c_fn_pass:
    case expr_check:
      return __source_filename ();
      break;
    case library_pass:
    case method_pass:
      if (library_input)
	return library_pathname ();
      else
	return __source_filename ();
      break;
    case run_time_pass:
      break;
    default:
      break;
    }

  return NULL;
}

static int pass_error_line (int orig_line) {
  /*
   *  Try subtracting 1 from the method start line, because
   *  the parser already numbers the method's first line
   *  as 1.
   */
  return ((interpreter_pass == method_pass) ?
	  (orig_line + (method_start_line () - 1)) : orig_line);
}

/*
 *  Exit with an error.  The _error () function doesn't return, so
 *  perform file cleanup first.
 */

void error (MESSAGE *orig, char *fmt,...) {
  va_list ap;
  char fmtbuf[MAXMSG];

  cleanup_temp_files (TRUE);

  if ((interpreter_pass == method_pass)) {
    if (nolinemarker_opt)
      _warning ("Error: in method, %s : %s:\n",
		rcvr_class_obj -> __o_name,
		new_methods[new_method_ptr + 1] -> method -> name);
    else
      _warning ("%s:%d: Error: in method, %s : %s:\n",
		source_filename (), pass_error_line (orig -> error_line),
		rcvr_class_obj -> __o_name,
		new_methods[new_method_ptr + 1] -> method -> name);

  }
  va_start (ap, fmt);
  vsprintf (fmtbuf, fmt, ap);
  _error ("%s:%d. %s\n", source_filename (), 
	  pass_error_line (orig -> error_line), fmtbuf);

}

void warning (MESSAGE *orig, char *fmt,...) {
  va_list ap;
  char fmtbuf[MAXMSG];
  char *name_ptr, filename_buf[FILENAME_MAX];
  if ((name_ptr = strrchr (source_filename (), '/')) != NULL) {
    ++name_ptr;
    strcpy (filename_buf, name_ptr);
  } else {
    strcpy (filename_buf, source_filename ());
  }

  if ((interpreter_pass == method_pass)) {
    if (nolinemarker_opt)
      _warning ("Warning: in method, %s : %s:\n",
		rcvr_class_obj -> __o_name,
		new_methods[new_method_ptr + 1] -> method -> name);
    else
      _warning ("%s:%d: in method, %s : %s:\n",
		filename_buf, pass_error_line (orig -> error_line),
		rcvr_class_obj -> __o_name,
		new_methods[new_method_ptr + 1] -> method -> name);

  }
  va_start (ap, fmt);
  vsprintf (fmtbuf, fmt, ap);
  _warning ("%s:%d: Warning: %s\n", filename_buf,
	    ((orig) ? pass_error_line (orig->error_line): 0), fmtbuf);

}

void warning_text (char *buf_out, MESSAGE *orig, char *fmt,...) {
  va_list ap;
  char *name_ptr, filename_buf[FILENAME_MAX], fmtbuf[MAXMSG];
  if ((name_ptr = strrchr (source_filename (), '/')) != NULL) {
    ++name_ptr;
    strcpy (filename_buf, name_ptr);
  } else {
    strcpy (filename_buf, source_filename ());
  }

  if ((interpreter_pass == method_pass)) {
    if (nolinemarker_opt)
    sprintf (buf_out, "Warning: in method, %s : %s:\n",
	     rcvr_class_obj -> __o_name,
	     new_methods[new_method_ptr + 1] -> method -> name);
    else
    sprintf (buf_out, "Warning: in method, %s : %s:\n",
	     rcvr_class_obj -> __o_name,
	     new_methods[new_method_ptr + 1] -> method -> name);
  }
  va_start (ap, fmt);
  vsprintf (fmtbuf, fmt, ap);
  sprintf (buf_out, "%s:%d: Warning: %s\n", filename_buf,
	    ((orig) ? pass_error_line (orig->error_line): 0), 
	   fmtbuf);

}

#ifdef DEBUG_CODE
void debug (char *fmt,...) {
  va_list ap;
  char fmtbuf[MAXMSG];
  va_start (ap, fmt);
  vsprintf (fmtbuf, fmt, ap);
  _warning ("%s\n", fmtbuf);
}
#endif

static bool is_fn_expr_typecast_a (MESSAGE_STACK messages,
				   int closing_paren_idx) {
  int open_paren_idx, expr_start_idx,
    typecast_end_idx;

  if (M_TOK(messages[closing_paren_idx]) != CLOSEPAREN)
    return false;

  if ((open_paren_idx = match_paren_rev 
       (messages, closing_paren_idx, stack_start (messages))) == ERROR)
    return false;

  if ((expr_start_idx = nextlangmsg (messages, open_paren_idx)) == ERROR)
    return false;

  return (bool) is_typecast_expr (messages, expr_start_idx, 
				  &typecast_end_idx);
}

void fn_check_expr (MESSAGE_STACK messages, int fn_idx, int stack_start,
		    int main_stack_idx) {
  int prev_tok_idx, prev_tok;
  MESSAGE *m_orig;
  if ((prev_tok_idx = prevlangmsg (messages, fn_idx)) != ERROR) {
    if (prev_tok_idx >= stack_start)
      return;
    prev_tok = M_TOK(messages[prev_tok_idx]);
    m_orig = message_stack_at (main_stack_idx);
    if (!IS_C_BINARY_MATH_OP(prev_tok) &&
	!is_fn_expr_typecast_a (messages, prev_tok_idx) &&
	(prev_tok != OPENPAREN) &&
	(prev_tok != ARGSEPARATOR) &&
	(prev_tok != SEMICOLON)) {
      if (m_orig) {
	warning (message_stack_at (main_stack_idx), 
		 "Function, \"%s,\" in expression is preceded by an invalid token.", 
		 M_NAME(messages[fn_idx]));
      } else {
	_warning 
	  (
   "fn_check_expr: Function, \"%s,\" in expression is preceded by an invalid token.\n", 
		 M_NAME(messages[fn_idx]));
      }
    }
  }
}

void unresolved_eval_delay_warning_1 
  (MESSAGE *orig,
   MESSAGE_STACK messages,
   int symbol_idx,
   char *method_name, 
   char *class_name,
   char *symbol,
   char *expr) {
  int prev_idx;
  char *p;
  int terminal_width, source_filename_length;
  OBJECT *prev_idx_obj = NULL;
  char msg_buf[MAXMSG], line_buf[MAXMSG];
  char *lbreak, break_before[MAXMSG], break_after[MAXMSG];
  char hang_indent[MAXMSG];
  int i, hang_indent_length;
  
  terminal_width = __ctalkTerminalWidth ();
  strcpy (msg_buf, source_filename ());
  if ((p = strrchr (msg_buf, '/')) != NULL) {
    ++p;
    source_filename_length = strlen (p);
  } else {
    source_filename_length = strlen (msg_buf);
  }
  source_filename_length += 5; /* plus, ":ddd: " line number. */
  hang_indent_length = source_filename_length;
  *msg_buf = 0;
  if (method_name) 
  if ((prev_idx = prevlangmsg (messages, symbol_idx)) != ERROR) {
    if (IS_OBJECT(messages[prev_idx]->obj)) {
      if (TOK_IS_MEMBER_VAR(messages[prev_idx])) {
	if (IS_OBJECT(messages[prev_idx] -> obj -> instancevars)) {
	  prev_idx_obj = messages[prev_idx] -> obj -> instancevars;
	} else {
	  prev_idx_obj = messages[prev_idx] -> obj;
	}
      } else {
	prev_idx_obj = messages[prev_idx] -> obj;
      }
    }
  }
  if (IS_OBJECT(prev_idx_obj)) {
    if (terminal_width > 0) {
      /* if we know the width of the terminal, wrap the line nicely */
      sprintf (line_buf,
	       "Undefined label, \"%s,\" "
	       "(receiver, \"%s,\" receiver class %s).\n",
	       symbol, M_NAME(messages[prev_idx]), 
	       prev_idx_obj -> __o_classname);
      if ((strlen (line_buf) + source_filename_length) > terminal_width) {
	lbreak = &line_buf[terminal_width - source_filename_length];
	/* scan back two words. */
	while (!isspace ((int)*lbreak)) --lbreak;
	while (isspace ((int)*lbreak)) --lbreak;
	while (!isspace ((int)*lbreak)) --lbreak;

	*hang_indent = 0;
	for (i = 0; i < hang_indent_length; ++i) {
	  strcatx2 (hang_indent, " ", NULL);
	}
	memset (break_before, 0, MAXMSG);
	strncpy (break_before, line_buf, lbreak - line_buf);
	strcpy (break_after, lbreak);
	strcatx (line_buf, break_before, "\n",
		 hang_indent, break_after, NULL);
      }
    } else {
      sprintf (line_buf,
	       "Undefined label, \"%s,\"\n\t"
	       "(receiver, \"%s,\" receiver class %s).\n",
	       symbol, M_NAME(messages[prev_idx]), 
	       prev_idx_obj -> __o_classname);
    }
    strcatx2 (msg_buf, line_buf, NULL);
  } else {
    sprintf (line_buf, "Undefined label, \"%s\".\n", symbol);
    strcatx2 (msg_buf, line_buf, NULL);
  }
  sprintf (line_buf, "\n\t\"%s\"\n\n", expr);
  strcatx2 (msg_buf, line_buf, NULL);
  sprintf (line_buf, "Waiting until run time to evaluate the expression.\n");
  strcatx2 (msg_buf, line_buf, NULL);
  warning (orig, msg_buf);
}

void unresolved_eval_delay_warning_2 (MESSAGE *orig, 
				      char *method_name,
				      char *name, 
				      char *method_class_name,
				      char *actual_rcvr_class_name,
				      char *next_name) {
  if (method_name) 
    warning (orig, "In method, \"%s,\" (Class %s):",
	     method_name, method_class_name);
  warning (orig, 
	   "  Object, \"%s,\" (declared as a, \"%s,\" object) does not understand message, \"%s.\"",
	   name, actual_rcvr_class_name, next_name);
  warning (orig, 
	   "  Waiting until run time to evaluate the expression.");
}

void warn_unknown_c_type (MESSAGE *m_orig, 
			 char *type_name) {
  METHOD *method;
  if (interpreter_pass == method_pass) {
    method = new_methods[new_method_ptr + 1] -> method;
    if (m_orig) {
      warning (m_orig, "In method, \"%s,\" (Class %s):",
	       method -> name, rcvr_class_obj -> __o_name);
      warning (m_orig, "Unimplemented C type, \"%s,\" in conversion, class defaulting to, \"Integer.\"\n", 
	       type_name);
    } else {
      _warning ("In method, \"%s,\" (Class %s):",
	       method -> name, rcvr_class_obj -> __o_name);
      _warning ("Unimplemented C type, \"%s,\" in conversion, class defaulting to, \"Integer.\"\n", 
		type_name);
    }
  } else {
    if (m_orig) {
      warning (m_orig, "Unimplemented C type, \"%s,\" in conversion, class defaulting to, \"Integer.\"\n", 
	       type_name);
    } else {
      _warning ("Unimplemented C type, \"%s,\" in conversion, class defaulting to, \"Integer.\"\n", 
		type_name);
    }
  }
}

void method_shadows_instance_variable_warning_1 
  (MESSAGE *orig, 
   char *method_name,
   char *method_class_name,
   char *shadow_method_name, 
   char *shadow_method_class_name) {
  if (method_name) 
    warning (orig, "In method, \"%s,\" (Class %s):",
	     method_name, method_class_name);
  warning (orig, 
	   "  Method, \"%s,\" (Class %s) shadows an instance variable.",
	   shadow_method_name, shadow_method_class_name);
}

void undefined_method_exception (MESSAGE *m_sender, MESSAGE *m_rcvr) {
  char errtext[MAXLABEL];
  
  if ((m_sender -> receiver_msg == m_rcvr) &&
      (m_rcvr -> attrs & OBJ_IS_INSTANCE_VAR)) {
    /* the expression will be evaluated at run time. */
    return;
  }

  if (m_rcvr -> obj) {
    strcatx (errtext, "Undefined method, \"", M_NAME(m_sender),
	     ",\" (receiver, \"", M_NAME(m_rcvr),
	     ",\" class, \"", m_rcvr->obj->CLASSNAME, NULL);
  } else {
    strcatx (errtext, "Undefined method, \"", M_NAME(m_sender),
	     ",\" (receiver, \"", M_NAME(m_rcvr),
	     "\")",  NULL);
  }
	   
  __ctalkExceptionInternal (m_sender, undefined_method_x,
			    errtext ,0);
}

/*
 *  This message gets printed at the point we encounter the class name...
 *  it's a lot faster than checking every function against the class
 *  library.  (Actually that could be "class_shadows_function... "  :\ ).
 */
bool function_shadows_class_warning_1 (MESSAGE_STACK messages, int msg_ptr) {
  if (get_function (M_NAME(messages[msg_ptr]))) {
    if (is_class_name (M_NAME(messages[msg_ptr]))) {
      warning (messages[msg_ptr], "Class \"%s,\" shadows a C function.",
	       M_NAME(messages[msg_ptr]));
      return true;
    }      
  }
  return false;
}

void missing_separator_error (MESSAGE_STACK messages, int method_idx) {
  error (messages[method_idx], "Missing separator before label, \"%s.\"", 
	 M_NAME(messages[method_idx]));
}

/* called after parsing - undefined_first_label_warning (errmsgs.c)
   and other functions call this. So far, only forwardly declared
   globals should end up here. */
void warn_unresolved_labels (void) {
  LIST *l;
  char object_name[MAXLABEL], *msg_p;
  if (unresolved_labels) {
    for (l = unresolved_labels; l; l = l -> next) {
      if ((msg_p = strstr ((char *)l -> data, ":::")) != NULL) {
	memset (object_name, 0, MAXLABEL);
	strncpy (object_name, (char *)l -> data,
		 msg_p - (char *)(l -> data));
	if (get_global_object (object_name, NULL))
	  continue;
	msg_p += 3;
	printf ("%s\n", msg_p);
      }
    }
    delete_list (&unresolved_labels);
  }
}

void ssroe_class_mismatch_warning (MESSAGE *m_org,
				   char *rcvr_classname,
				   char *operand_classname) {
  if (!str_eq (rcvr_classname, operand_classname)) {
    warning (m_org, "Operand class, \"%s,\" is different than receiver class,"
	     " \"%s.\"", operand_classname, rcvr_classname);
    warning (m_org, "The method should perform the necessary class " 
	     "conversion(s) to the receiver's class.");
  }
}

#ifdef CHECK_SYMBOL_SIZE
void check_symbol_size (char *s) {
  if (strlen (s) > MAXLABEL) {
    printf (">>>> symbol too long: %s, %d.\n"
	    s, strlen (s));
    exit (1);
  }
}
#endif
