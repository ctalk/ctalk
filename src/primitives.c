/* $Id: primitives.c,v 1.3 2020/02/14 17:54:36 rkiesling Exp $ */

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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "break.h"

extern int library_input;   /* Declared in class.c.  TRUE if evaluating */
                            /* class libraries.                         */
extern I_PASS interpreter_pass;    /* Declared in rtinfo.c.                    */

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;
extern OBJECT *rcvr_class_obj;

extern HASHTAB declared_method_names;    /* Declared in method.c.       */
extern HASHTAB declared_method_selectors;
extern HASHTAB declared_global_variables; /* Declared in cvars.c.       */

extern JMP_ENV jmp_env;                   /* Declared in except.c.      */
extern int exception_return;

extern int error_line,            /* Declared in errorloc.c.            */
  error_column;
extern int last_method_line;      /* Declared in parser.c.  See the       
				     comments there.                    */

extern int nopreload_opt;

extern OBJECT *classes;       /* Class dictionary and list head pointer.  */

extern char *ascii[8193];             /* from intascii.h */

extern int do_predicate;           /* Declared in loop.c.               */

extern DEFAULTCLASSCACHE *ct_defclasses; /* Declared in cclasses.c.       */

HASHTAB defined_instancevars;

void init_new_method_stack (void) {
  new_method_ptr = MAXARGS;
  _new_hash (&defined_instancevars);
}

int method_start_line (void) {
  return new_methods[new_method_ptr+1]->method->error_line;
}

/* Return the error line of the class name token before the 
   "instanceMethod" or "classMethod" token. */
static int find_method_start_line (MESSAGE_STACK messages, int idx) {
  int stack_start_idx, class_tok_idx;
  stack_start_idx = stack_start (messages);
  for ( ; idx <= stack_start_idx; idx++) {
    if (str_eq (M_NAME(messages[idx]), "instanceMethod") ||
	str_eq (M_NAME(messages[idx]), "classMethod")) {
      if ((class_tok_idx = prevlangmsg (messages, idx)) != ERROR)
	return messages[class_tok_idx] -> error_line;
      else
	return ERROR;
    }
  }
  return ERROR;
}

static int param_newline_count (int param_start_ptr, int param_end_ptr) {
  MESSAGE *m;
  int i;
  int n_newlines = 0;

  for (i = param_start_ptr; i >= param_end_ptr; i--) {
    m = message_stack_at (i);
    if (M_TOK(m) == NEWLINE)
      n_newlines += strlen (M_NAME(m));
  }
  return n_newlines;
}

int new_method_param_newline_count (void) {
  return new_methods[new_method_ptr + 1] -> n_param_newlines;
}

char *make_selector (OBJECT *rcvr_class_obj_arg, 
			    METHOD *m, char *funcname, I_OR_C i_or_c) {
  static char buf[MAXMSG];

  switch (i_or_c)
    {
    case i_or_c_instance:
      if (m -> varargs || m -> prefix || m -> no_init) {
	strcatx (buf, rcvr_class_obj_arg -> __o_name,
		 INSTANCE_SELECTOR, funcname, "_",
		 ((m -> varargs) ? "v" :
		  (m -> prefix) ? "p" :
		  (m -> no_init ? "n" : "u")), NULL);
      } else {
	strcatx (buf, rcvr_class_obj_arg -> __o_name,
		 INSTANCE_SELECTOR, funcname, "_", 
		 ascii[m -> n_params], NULL);
      }
      break;
    case i_or_c_class:
      if (m -> varargs || m -> prefix || m -> no_init) {
	strcatx (buf, rcvr_class_obj_arg -> __o_name,
		 CLASS_SELECTOR, funcname, "_",
		 ((m -> varargs) ? "v" :
		  (m -> prefix) ? "p" :
		  (m -> no_init ? "n" : "u")), NULL);
      } else {
	strcatx (buf, rcvr_class_obj_arg -> __o_name,
		 CLASS_SELECTOR, funcname, "_", 
		 ascii[m -> n_params], NULL);
      }
      break;
    }

  return buf;
}

void add_template_to_method_init (char *template_name) {
  NEWMETHOD *nm = new_methods[new_method_ptr+1];
  LIST *l;
  if (nm -> templates == NULL) {
    nm -> templates = new_list ();
    nm -> templates -> data = strdup (template_name);
  } else {
    l = new_list ();
    l -> data = strdup (template_name);
    list_push (&(nm -> templates), &l);
  }
}

#ifdef __x86_64
#define PHTOA(buf,r) {(void)htoa((buf),(unsigned long long int)(r));}
#else
#define PHTOA(buf,r) {(void)htoa((buf),(unsigned int)(r));}
#endif

/*
  Set the name to the name of the class, the classname to 
  "Class," and the superclass to the name of the receiver
  object, and add the object to the class dictionary.
*/
OBJECT *define_class (int method_msg_ptr) {

  OBJECT *arg_object;        /* The new class object.          */
  OBJECT *tmp_object;        /* Existing class object, if any. */
  MESSAGE *m;                /* Message of the method call.    */
  MESSAGE *m_receiver;       /* Message of the receiver.       */
  METHOD *method;
  int i;
  int msg_frame_top_ptr;     /* Frame pointers for statement  */
  int next_frame_top_ptr;    /* code generation.              */
  CVAR *cvar_dup;
  char _addr_buf[MAXMSG];

  m = message_stack_at (method_msg_ptr);
  m_receiver = m -> receiver_msg;

  method = get_instance_method (m, m -> receiver_obj, m -> name, 
				ERROR, TRUE);

  class_primitive_method_args (method, message_stack (), method_msg_ptr);

  if (method -> n_args != method -> n_params)
    error (m, "Method %s: Wrong number of arguments.", m -> name);

  arg_object = method -> args[0] -> obj;

  if ((tmp_object = get_class_object (arg_object -> __o_name)) != NULL) {
    warning (m, "Redefinition of class, \"%s.\"",
	     arg_object -> __o_name);
    return NULL;
  }

  if (!class_deps_updated (arg_object -> __o_name) &&
      file_exists (class_cache_path_name (arg_object -> __o_name)) && 
      !nopreload_opt && 
      !is_pending_class (arg_object -> __o_name)) {

    load_cached_class (arg_object -> __o_name);

    arg_object -> scope = GLOBAL_VAR;

    PHTOA(_addr_buf, arg_object);
    __ctalkSetObjectValueVar (arg_object, _addr_buf);

    add_class_object (arg_object);

    if ((cvar_dup = _hash_remove (declared_global_variables,
				  arg_object -> __o_name)) != NULL) {
      unlink_global_cvar (cvar_dup);
      _delete_cvar (cvar_dup);
    }

  } else {

    arg_object -> scope = GLOBAL_VAR;

    PHTOA(_addr_buf,arg_object);
    __ctalkSetObjectValueVar (arg_object, _addr_buf);

    add_class_object (arg_object);

    if ((cvar_dup = _hash_remove (declared_global_variables,
				  arg_object -> __o_name)) != NULL) {
      unlink_global_cvar (cvar_dup);
      _delete_cvar (cvar_dup);
    }

    /* Generate C code and mark the ctalk statement as
       evaluated and output. */

    generate_primitive_class_definition_call (m_receiver -> obj, method);

    save_class_init_info (arg_object -> __o_name, 
			  arg_object -> __o_superclassname);
  } /* if (file_exists (cache_fn)) */

  next_frame_top_ptr = message_frame_top_n (parser_frame_ptr () - 1);
  msg_frame_top_ptr = message_frame_top_n (parser_frame_ptr ());

  for (i = msg_frame_top_ptr; i > next_frame_top_ptr; i--) {
    m = message_stack_at (i);
    ++m -> evaled;
    ++m -> output;
  }

  return NULL;
}

static METHOD *get_actual_constructor (int method_msg_ptr,
				       METHOD **method,
				       METHOD **method_orig,
				       int *attrs,
				       bool *replaces_primitive) {
  int prev_tok_idx;
  MESSAGE *m;
  /*
   *  Note that compound_method () does not work here, because
   *  method_msg_ptr points to, "new," and not, "super," if it
   *  is present.
   */
  if ((prev_tok_idx = prevlangmsg (message_stack (), method_msg_ptr))
      == ERROR)
    _error ("Parser error.\n");
  if (attrs != NULL) {
    if (message_stack_at (prev_tok_idx) -> attrs & TOK_SUPER)
      *attrs |= METHOD_SUPER_ATTR;
  }
  m = message_stack_at (method_msg_ptr);
  *method_orig = get_instance_method (m, m -> receiver_obj, m -> name, 
				     ERROR, TRUE);
  *method = method_replaces_primitive (*method_orig, m -> receiver_obj);
  if (replaces_primitive != NULL) {
    *replaces_primitive = (*method != *method_orig) ? True : False;
  }
  return *method;
}

static void copy_arg_class (OBJECT *arg_obj_src,  OBJECT *arg_obj_dest) {
  /* method_args creates the first object correctly -
     we have to make sure that the following objects
     are the same class, too. (Note that these are also
     the objects that are attached to the argument tokens. */
  if ( arg_obj_src -> __o_class != arg_obj_dest -> __o_class) {
    arg_obj_dest -> __o_class = arg_obj_src ->  __o_class;
    arg_obj_dest -> __o_superclass = arg_obj_src ->  __o_superclass;
    strcpy (arg_obj_dest -> __o_classname, arg_obj_src ->  __o_classname);
    strcpy (arg_obj_dest -> __o_superclassname,
	    arg_obj_src ->  __o_superclassname);
  }
}

/* 
 *  Define a new object.  Make sure the new object has the same 
 *  class as the receiver, and add it to the dictionary.  
 */
static OBJECT *__new_object_internal (int method_msg_ptr, int arg_ptr,
				      int nth_arg) {

  MESSAGE *m;
  MESSAGE *m_receiver;
  OBJECT *arg_object;      /* Object  from the method arguments, and   */
  OBJECT *tmp_object;      /* possibly existing in the dictionary.     */
  MESSAGE *m_arg;          /* Message of argument.                     */
  METHOD *method = NULL,   /* Actual method used to generate the object*/
    *method_orig = NULL;   /* and the original method.                 */
  char buf[MAXLABEL];
  int i;
  int attrs;
  int msg_frame_top_ptr;     /* Frame pointers for statement  */
  int next_frame_top_ptr;    /* code generation.              */
  bool arg_is_param;
  bool orig_method_replaces_primitive;

  m = message_stack_at (method_msg_ptr);
  m_receiver = m -> receiver_msg;
  arg_is_param = False;

  m_arg = message_stack_at (arg_ptr);

  attrs = 0;

  get_actual_constructor (method_msg_ptr, &method, &method_orig,
			  &attrs, &orig_method_replaces_primitive);

  if (!(attrs & METHOD_SUPER_ATTR) && !orig_method_replaces_primitive) {
    if (is_method_proto (m -> receiver_obj, m -> name)) {
      char selector[MAXMSG];
      strcatx (selector, m -> receiver_obj -> __o_name, "_instance_",
	       m -> name, "_1", NULL);
      if (!method_proto_is_output (selector)) {
	still_prototyped_constructor_warning 
	  (m, m -> receiver_obj -> __o_name, m -> name);
      }
    }
  }


  if ((!m_arg || !IS_MESSAGE (m_arg)) ||
      (!IS_OBJECT (m_arg -> obj)) ||
      (strcmp (m_arg -> obj -> __o_name, 
	       method -> args[nth_arg] -> obj -> __o_name)) ||
      /* this is supposed to be arg[0], so it's another 
	 argument class check. */
      (m_arg -> obj -> __o_class != method -> args[0] -> obj -> __o_class))
    warning (m_arg, "new_object (): Argument mismatch.");

  /*
   *  This method has a variable number of arguments,
   *  but the function should check here that the 
   *  method is okay.
   */

  if (!method -> varargs || (method -> n_args < method -> n_params))
    error (m, "Method \"%s:\" (Receiver %s) Wrong number of arguments.\n", 
	   m -> name, m -> receiver_obj -> __o_name);

  if (interpreter_pass == method_pass) {
    METHOD *n_method;
    n_method = new_methods[new_method_ptr + 1] -> method;
    for (i = 0; i < n_method -> n_params; i++) {
      if (!strcmp (M_NAME(m_arg), n_method -> params[i] -> name)) {
	/*
	 *  If called from method_call (), method_args () should already have
	 *  provided the argument.
	 */
	if (method -> args[i] && IS_ARG(method -> args[i])) {
	  if (method -> args[i] -> obj && 
	      IS_OBJECT(method -> args[i] -> obj)) {
	    delete_object (method -> args[i] -> obj);
	    delete_arg (method -> args[i]);
	  }
	}
	method -> args[i] = create_arg_init (method_arg_object (m_arg));
 	arg_is_param = True;
      }
    }
  }

  arg_object = method -> args[nth_arg] -> obj;

  if (!IS_OBJECT (arg_object)) {
    error (m_arg, "Method \"%s\": invalid argument.", method -> name);
  } else {
    if (!arg_is_param)
      strcpy (arg_object -> __o_classname, m_receiver -> obj -> __o_name);
    arg_object -> scope = parser_frame_scope ();
    /* Make sure an object of this class does not already exist. */
    if ((tmp_object = get_object (arg_object -> __o_name, 
				  m_receiver -> obj -> __o_name)) != NULL) {
      if ((tmp_object != arg_object) &&
	  !IS_CONSTRUCTOR(method)) {
	error (m_arg, "Redefinition of object, \"%s.\"", 
	       arg_object -> __o_name);
      } else {
	goto new_object_cleanup;
      }
    }
    /*
     *  This is the same as __ctalkInstanceVariableFromClassObject.
     */
    if (!arg_is_param) {
      if (arg_object -> instancevars == NULL) {
	arg_object -> instancevars = 
	  create_object (arg_object -> __o_classname, "value");
      }
      instance_variables_from_class_definition (arg_object);
    }

    add_object (arg_object);
    m_receiver -> value_obj = arg_object;
  }
  
  /* method can == method_orig if one of the methods is still a
     prototype. */
  if (orig_method_replaces_primitive && (method != method_orig)) {
    for (i = 0; i < method -> n_args; i++) {
      /* DON'T remove the args from method -> args ... later routines
	 use this as a default constructor, when we needn't or can't
	 determine the class at run time.  */
      method_orig -> args[i] = method -> args[i];
      method_orig -> n_args++;
    }
  }

  if (orig_method_replaces_primitive) {
    /*
     *  Super method calls.  This should handle the cases where the 
     *  superclass method is either an actual method or a primitive.
     */
    OBJECT *method_receiver;
    METHOD *m_t;
    bool have_match = False;

    for (m_t = m_receiver->obj -> instance_methods; m_t; m_t = m_t -> next){
      if (method_orig == m_t) {
	have_match = True;
	generate_method_call_from_primitive (m_receiver->obj, 
					     method_orig, buf, attrs,
					     nth_arg);
      }
    }
    if (!have_match) {
      for (method_receiver = m_receiver-> obj -> __o_superclass;
	   method_receiver && method_receiver -> __o_superclassname 
	     && !have_match;
	   method_receiver = method_receiver -> __o_superclass) {
	for (m_t = method_receiver -> instance_methods; m_t && !have_match;
	     m_t = m_t -> next) 
	  if (method_orig == m_t) {
	    if (attrs & METHOD_SUPER_ATTR) {
	      /* we select the receiver class of the superclass-derived 
		 method so the args get stored in the right method at
		 run time without (hopefully) any fixups. */
	      generate_method_call_from_primitive
		(method_orig -> rcvr_class_obj -> __o_class,
		 method_orig, buf, attrs, nth_arg);
	    } else {
	      generate_method_call_from_primitive (m_receiver->obj->__o_class,
						   method_orig, buf, attrs,
						   nth_arg);
	    }
	    have_match = True;
	  }
      }
    }
  } else {
    generate_primitive_method_call_2 (m_receiver->obj, method, buf, attrs,
				      nth_arg);
  }
 
  if (is_global_frame ())
    generate_set_global_variable_call (arg_object -> __o_name, 
				       arg_object -> __o_classname);
  else
    generate_set_local_variable_call (arg_object -> __o_name, 
				      m_receiver -> obj -> __o_name);

 new_object_cleanup:
  next_frame_top_ptr = message_frame_top_n (parser_frame_ptr () - 1);
  msg_frame_top_ptr = message_frame_top_n (parser_frame_ptr ());

  for (i = msg_frame_top_ptr; i > next_frame_top_ptr; i--) {
    m = message_stack_at (i);
    if (m -> tokentype != NEWLINE) {
      ++m -> evaled;
      ++m -> output;
    }
  }

  /*
   *  Should be factored into cleanup_args () at some time.
   */
  if (orig_method_replaces_primitive) {
    for (i = 0; i < method -> n_args; i++) {
      method_orig -> args[i] = NULL;
    }
    method_orig -> n_args = 0;
  }
  return NULL;
}

OBJECT *new_object (int method_msg_ptr) {
  int i, stack_top, nth_arg;
  MESSAGE *m;
  METHOD *method, *method_orig;

  stack_top = get_stack_top (message_stack ());

  get_actual_constructor (method_msg_ptr, &method, &method_orig, NULL, NULL);
  nth_arg = 0;
  for (i = method_msg_ptr - 1; i > stack_top; --i) {
    m = message_stack_at (i);
    if (M_ISSPACE(m))
      continue;
    switch (M_TOK(m))
      {
      case LABEL:
	if (nth_arg > 0) {
	  copy_arg_class (method -> args[0] -> obj,
			  method -> args[nth_arg] -> obj);
	}
	__new_object_internal (method_msg_ptr, i, nth_arg);
	++nth_arg;
	break;
      case  SEMICOLON:
	goto new_object_done;
	break;
      }
  }

 new_object_done:

  return NULL;
}

/*
 *  Define a new method from source file or class library.
 *
 *    Method declarations have either one or two 
 *    arguments after the method. 
 *
 *      <Class> method <function> (<args>) {<body>}
 *
 *    or 
 *
 *      <Class> method <alias> <function> (<args>) {<body>}
 *
 *    The <function> element must be a valid C identifier.
 * 
 *    Methods when imported and translated into C have the
 *    following prototype in the generated code:
 * 
 *      OBJECT *<function> (<args>) {<body>}
 *   
 */

/*
 *   Values that need to be available when parsing the
 *   method.  We need to stack the new methods when 
 *   parsing methods recursively.
 */
/*
 *  Used in library_search () to set and restore receiver classes when
 *  recursing into another class library.  get_rcvr_class_obj () must
 *  be called before parse (), and set_rcvr_class_obj () for the previous
 *  level after output_imported_methods ().
 */

OBJECT *get_rcvr_class_obj (void) {
  return rcvr_class_obj;
}

void set_rcvr_class_obj (OBJECT *o) {
  rcvr_class_obj = o;
}

/*
 *  Prevent a "sizeof," operator if overloaded, from being interpreted 
 *  as  an operator if it occurs in a method declaration - the parser
 *  should treat sizeof as a normal label if it occurs as a method alias.
 */
static void un_sizeof_declaration (MESSAGE_STACK messages, 
				    int method_msg_ptr, 
				    int param_start_ptr) {
  int i;
  for (i = method_msg_ptr; i >= param_start_ptr; i--) 
    if (M_TOK(messages[i]) == SIZEOF)
      messages[i] -> tokentype = LABEL;
}

static void method_params_from_src (MESSAGE_STACK messages,
				    int param_start_idx,
				    int param_end_idx,
				    METHOD *n_method,
				    int *have_c_param,
				    char *c_param) {
  PARAMLIST_ENTRY *params[MAXARGS];
  int i, n_params;

  fn_param_declarations (messages, param_start_idx, param_end_idx,
			       params, &n_params);

  n_method -> n_params = n_params;
  for (i = n_params - 1; i >= 0; i--) {
    /* TO DO - Possibly add error checking in method_param (). 
       See the comment below. */
    /* TO DO - Make sure that method_param has the correct
       error line, column, and source file name. */
    n_method -> params[i] = method_param_tok (messages, params[i]);

#if 0
    /*
     *  NOTE - This should be handled as an exception in
     *  a future release.
     */
    if (! param_class_exists (n_method -> params[i] -> class)) {
      /* Not used right now. */
      /* param_class_exception = i; */
    }
#endif

    if ((i == 0) && 
	(n_method -> params[i] -> attrs & PARAM_C_PARAM)) {
      char *s;
      s = strchr (params[i] -> buf, ' ');
      *have_c_param = TRUE;
      strcpy (c_param, s);
    }
    if ((i == 0) &&
 	(n_method -> params[i] -> attrs & PARAM_PREFIX_METHOD_ATTRIBUTE)) {
      n_method -> prefix = TRUE;
    }
    if (n_method -> params[i] -> attrs & PARAM_VARARGS_PARAM)
      n_method -> varargs = TRUE;
    __xfree (MEMADDR(params[i]));
  }
}

OBJECT *new_instance_method (int method_msg_ptr) {

  int rcvr_ptr,           /* Index of the class message, which is the
			     same class as the method message's receiver 
			     object.                                       */
    param_start_ptr,      /* Index of parameter start.                     */
    param_end_ptr,        /* Index of parameter end.                       */
    method_start_ptr,     /* Index of function body open block.            */
    alias_ptr,      /* Index of the method alias, if any.           */
    funcname_ptr,   /* Index of the function name.                  */
    method_end_ptr, /* End of the method body.                      */
    i,
    buflength,
    prev_error_line;
  char *tmpbuf,     /* Buffer for collected method body tokens.       */
    *srcbuf,      /* Buffer for method with translated declaration. */
    funcname[MAXLABEL],
    alias[MAXLABEL],
    c_param[MAXMSG],
    cache_fn[FILENAME_MAX];
  MESSAGE *m_method, /* Message of the, "method," keyword. */
    *m_rcvr;         /* Message of the class object receiver. */
  I_PASS prev_pass;
  METHOD  *n_method,
    *tm,
    *rcvr_method;
  int have_c_param = 0; 
  CFUNC *__shadowed_c_fn;
  CVAR *__cvar_dup;

  char *tmpname;
  int x_chars_read;
  struct stat statbuf;

  m_method = message_stack_at (method_msg_ptr);

  if (!IS_OBJECT (m_method -> receiver_obj))
    error (m_method, "new_instance_method: Undefined receiver object.");

  if ((rcvr_ptr = prevlangmsg (message_stack (), method_msg_ptr)) == ERROR)
    error (m_method, "new_instance_method: can't find class object declaration.");

  m_rcvr = message_stack_at (rcvr_ptr);

  if ((strcmp (m_rcvr -> name, m_method -> receiver_obj -> __o_name)) ||
      (m_rcvr -> obj != m_method -> receiver_obj))
    error (m_method, "new_instance_method: receiver mismatch error.");

  set_rcvr_class_obj (m_rcvr -> obj);

  if (!IS_CLASS_OBJECT(rcvr_class_obj))
    error (m_method, "new_instance_method: Receiver %s is not a class object.",
	   rcvr_class_obj -> __o_name);

  /* 
   *  Find the number of args required by the method declaration.
   *  TO DO - Determine if we can use method_declaration_info ()
   *  here also - we only need n_params here.
   */

  if ((method_end_ptr = find_function_close (message_stack (),
					     method_msg_ptr,
					     get_messageptr ()))
      == ERROR) {
    int e_prev_idx, e_next_idx;
    MESSAGE *m_error_rcvr, *m_error_name;
    if ((e_prev_idx = prevlangmsg (message_stack(), 
				   method_msg_ptr)) != ERROR) {
      m_error_rcvr = message_stack_at (e_prev_idx);
    } else {
      m_error_rcvr = NULL;
    }
    if ((e_next_idx = nextlangmsg (message_stack(), 
				   method_msg_ptr)) != ERROR) {
      m_error_name = message_stack_at (e_next_idx);
    } else {
      m_error_name = NULL;
    }
    error (m_method, "%s method %s : Can't find end of method.",
	   (m_error_rcvr ? M_NAME(m_error_rcvr) : ""), 
	   (m_error_name ? M_NAME(m_error_name) : ""));
  }

  if ((param_start_ptr = 
       scanforward (message_stack (), method_msg_ptr,
		    method_end_ptr, OPENPAREN)) == ERROR)
    error (m_method, "Missing parameters in method declaration.");

  if ((param_end_ptr = match_paren (message_stack (), param_start_ptr,
				    method_end_ptr))  == ERROR)
    error (m_method, "Missing parameters in method declaration.");

  if (M_TOK(message_stack_at (method_end_ptr)) == SEMICOLON) {
    add_user_prototype (message_stack (), rcvr_ptr, param_start_ptr,
			param_end_ptr, method_end_ptr);
    return SUCCESS;
  } else {
    if ((method_start_ptr = scanforward (message_stack (), param_start_ptr,
					 method_end_ptr, OPENBLOCK)) == ERROR)
      error (m_method, "Missing start of method body.");
  }


  if ((n_method = (METHOD *)__xalloc (sizeof (METHOD))) == NULL)
    _error ("new_instance_method: %s.", strerror (errno));
  n_method -> sig = METHOD_SIG;

  new_methods[new_method_ptr--] = create_newmethod_init (n_method);
  new_methods[new_method_ptr+1] -> n_param_newlines = 
    param_newline_count (param_start_ptr, param_end_ptr);

  n_method -> rcvr_class_obj = rcvr_class_obj;

  if (method_contains_argblk (message_stack (), method_start_ptr,
			      method_end_ptr)) {
    
    n_method -> attrs |= METHOD_CONTAINS_ARGBLK_ATTR;
  }

  un_sizeof_declaration (message_stack (), method_msg_ptr, param_start_ptr);

  method_declaration_msg (message_stack (), method_msg_ptr, param_start_ptr, 
			  funcname, &funcname_ptr, 
			  alias, &alias_ptr);

  method_params_from_src (message_stack (), param_start_ptr, param_end_ptr,
			  n_method, &have_c_param, c_param);

  strcpy (n_method -> selector, 
	  make_selector (rcvr_class_obj, n_method, funcname, 
			 i_or_c_instance));

  if (method_proto_is_output (n_method -> selector)) {
    for (i = rcvr_ptr; i >= method_end_ptr; i--) {
      MESSAGE *m;
      m = message_stack_at (i);
      ++m -> evaled;
      ++m -> output;
    }
    for (i = 0; i < n_method -> n_params; i++) {
      __xfree (MEMADDR(n_method -> params[i]));
    }
    __xfree (MEMADDR(n_method));

    delete_newmethod (new_methods[new_method_ptr+1]);
    new_methods[++new_method_ptr] = NULL;
    return NULL;
  }

#ifdef DEBUG_UNDEFINED_PARAMETER_CLASSES
  /* Not used right now */
  /* if (param_class_exception != ERROR) */
  /*   warning (message_stack_at (method_msg_ptr), */
  /* 	     "Undefined parameter class %s. (Method %s, Class %s).", */
  /* 	     n_method -> params[param_class_exception] -> class, */
  /* 	     ((*alias) ? alias : funcname), */
  /* 	     rcvr_class_obj -> __o_name); */
#endif

  strcpy (n_method -> name, ((*alias) ? alias : funcname));

  n_method -> n_args = 0;
  n_method -> nth_local_ptr = 0;
  n_method -> error_line = find_method_start_line (message_stack (), 
						    method_msg_ptr);
  new_methods[new_method_ptr+1] -> source_line = n_method -> error_line;
  n_method -> error_column = m_method -> error_column;
  /* This is a kludge - all methods are marked as imported,
     until we can get the method buffering straightened out. */
  n_method -> imported = False;
  n_method -> queued = False;
  
  strcpy (n_method -> returnclass, rcvr_class_obj -> __o_name);

  for (rcvr_method = rcvr_class_obj -> instance_methods; rcvr_method;
       rcvr_method = rcvr_method -> next) {

    if (str_eq (rcvr_method -> name, n_method -> name) && 
	(rcvr_method -> n_params == n_method -> n_params) &&
	(rcvr_method -> prefix == n_method -> prefix) &&
	(rcvr_method -> varargs == n_method -> varargs) &&
	(rcvr_method -> no_init == n_method -> no_init)) {
    
      warning (m_method, "Redefinition of method, \"%s.\" (Class %s).",
	     n_method -> name, rcvr_class_obj -> __o_name);
      delete_newmethod (new_methods[new_method_ptr+1]);
      new_methods[new_method_ptr++] = NULL;
      __xfree (MEMADDR(n_method));
      return NULL;
    }
  }

  if ((__cvar_dup = _hash_remove (declared_global_variables, 
				  n_method -> name)) != NULL) {
    warning (message_stack_at(method_msg_ptr),
	     "Method, \"%s,\" shadows a global variable.", n_method -> name);
    /* We don't need it, so far. */
    unlink_global_cvar (__cvar_dup);
    _delete_cvar (__cvar_dup);
  }

  if ((__shadowed_c_fn = get_function (n_method -> name)) != NULL) {
      warning (message_stack_at(method_msg_ptr), 
	       "Method, \"%s,\" shadows a C function.", n_method -> name);
  }

  prev_pass = interpreter_pass;
  interpreter_pass = method_pass;

  strcatx (cache_fn, home_libcache_path (), "/", 
	   n_method -> selector, NULL);
	   	   
#ifdef METHOD_BREAK
  if (str_eq (n_method -> selector,
	      METHOD_BREAK)) {
    printf (METHOD_BREAK_MSG, METHOD_BREAK);
    asm("int3;");
  }
#endif  

  if (!method_deps_updated (n_method -> selector) && 
      !stat(cache_fn, &statbuf) && !nopreload_opt) {

    if ((n_method -> src = (char *) __xalloc (statbuf.st_size + 2)) 
	== NULL)
      error (m_method, "new_instance_method %s (class %s): %s.",
	     n_method -> name, rcvr_class_obj -> __o_name,
	     strerror (errno));

#ifdef __DJGPP__
    errno = 0;
    x_chars_read = read_file (n_method -> src, cache_fn);
    if (errno)
      error (m_method, "new_instance_method %s (class %s): %s.",
	     n_method -> name, rcvr_class_obj -> __o_name,
	     strerror (errno));
#else
    if ((x_chars_read = read_file (n_method -> src, cache_fn)) != statbuf.st_size)
      error (m_method, "new_instance_method %s (class %s): %s.",
	     n_method -> name, rcvr_class_obj -> __o_name,
	     strerror (errno));
#endif

    n_method -> imported = True;
    strcpy (n_method -> returnclass, 
	    get_init_return_class (n_method -> selector));

    output_pre_method (n_method -> selector, n_method -> src);

  } else { /* if (!file_exists (cache_fn)) */
  
    if (!pre_method_is_output (n_method -> selector)) {

      tmpbuf = 
	collect_tokens (message_stack (), method_start_ptr, method_end_ptr);

      if (have_c_param) {
	buflength = METHOD_RETURN_LENGTH + 2 + /* 2 spaces */
	  strlen (c_param) + 5 +               /*  strlen (" ( ) ") */
	  strlen (n_method -> selector) + strlen (tmpbuf);
      } else {
	buflength = METHOD_RETURN_LENGTH + 3 + /* 3 spaces */
	  METHOD_C_PARAM_LENGTH + 
	  strlen (n_method -> selector) + strlen (tmpbuf);
      }

      if ((srcbuf = (char *)__xalloc (buflength + 1)) == NULL)
	_error ("new_instance_method: %s.", strerror (errno));

      if (n_method -> params[0] &&
	  (n_method -> params[0] -> attrs & PARAM_C_PARAM)) {
	strcatx (srcbuf, METHOD_RETURN, " ", 
		 n_method -> selector, " (", 
		 c_param, ") ", tmpbuf, NULL);
      } else {
	strcatx (srcbuf, METHOD_RETURN, " ", 
		 n_method -> selector, " ",
		 METHOD_C_PARAM, " ", tmpbuf, NULL);
      }

      generate_instance_method_definition_call 
	(rcvr_class_obj -> __o_name,
	 ((*alias) ? alias : funcname),
	 n_method -> selector, n_method -> n_params, 
	 method_definition_attributes (n_method));

      for (i = 0; i < n_method -> n_params; i++) {
	if (!(n_method -> params[i] -> attrs & PARAM_C_PARAM))
	  generate_instance_method_param_definition_call 
	    (rcvr_class_obj -> __o_name,
	     ((*alias) ? alias : funcname),
	     n_method -> selector,
	     n_method -> params[i] -> name,
	     n_method -> params[i] -> class,
	     n_method -> params[i] -> is_ptr,
	     n_method -> params[i] -> is_ptrptr);
      }

      create_tmp ();
      begin_method_buffer (n_method);
      prev_error_line = error_line;
      error_line = 1;
      parse (srcbuf, (long long)buflength);
      error_line = prev_error_line + last_method_line;
      end_method_buffer ();
      unbuffer_method_statements ();

      close_tmp ();

      generate_instance_method_return_class_call 
	(rcvr_class_obj -> __o_name,
	 n_method -> name, 
	 n_method -> selector,
	 n_method -> returnclass,
	 n_method -> n_params);
      tmpname = get_tmpname ();
      stat (tmpname, &statbuf);
      if ((n_method -> src = (char *) 
	   __xalloc (statbuf.st_size + 1)) == NULL)
	error (m_method, "new_instance_method %s (class %s): %s.",
	       n_method -> name, rcvr_class_obj -> __o_name,
	       strerror (errno));
#ifdef __DJGPP__
      errno = 0;
      x_chars_read = read_file (n_method -> src, tmpname);
      if (errno)
	error (m_method, "new_instance_method %s (class %s): %s.",
	       n_method -> name, rcvr_class_obj -> __o_name,
	       strerror (errno));
#else
      if ((x_chars_read = read_file (n_method -> src, tmpname)) != statbuf.st_size)
	error (m_method, "new_instance_method %s (class %s): %s.",
	       n_method -> name, rcvr_class_obj -> __o_name,
	       strerror (errno));
#endif
      __xfree (MEMADDR(srcbuf));
      __xfree (MEMADDR(tmpbuf));

      /* Save class libraries only for pre-loading for now. */
      if (library_input && !nopreload_opt) {
	save_pre_init_info (cache_fn, n_method, rcvr_class_obj);
	rename (tmpname, cache_fn);
      }
      remove_tmpfile_entry ();

      /* This is also used for methods in the main source input
	 so the methods can also use output_imported_methods ()
	 for their buffering. 
      */
      n_method -> imported = True;

      queue_method_for_output (rcvr_class_obj, n_method);

    } /* if (!pre_method_is_output (n_method -> selector)) */

  } /* if (!file_exists (cache_fn)) */

  interpreter_pass = prev_pass;

  if (!library_input) {

    /* Replace the unevaled method's tokens with whitespace so
       the parent parser doesn't evaluate the method body again. */
    /* TO DO - 
       Inform the parent parser simply to skip the frames of the 
       source method, if possible, without adding another state to 
       the parser. */
    for (i = rcvr_ptr; i >= method_end_ptr; i--) {
      MESSAGE *m_tmp;
       m_tmp = message_stack_at (i);
       if ((m_tmp -> tokentype != WHITESPACE) && 
	   (m_tmp -> tokentype != NEWLINE)) {
	 m_tmp -> tokentype = WHITESPACE;
	 strcpy (m_tmp -> name, " ");
       }
    }
  }

  if (rcvr_class_obj -> instance_methods == NULL) {
    rcvr_class_obj -> instance_methods = n_method;
  } else {
    for (tm = rcvr_class_obj -> instance_methods; tm -> next; tm = tm -> next)
      ;
    tm -> next = n_method;
    n_method -> prev = tm;
  }
  _hash_put (declared_method_names, n_method -> name, n_method -> name);
  _hash_put (declared_method_selectors, n_method -> selector, n_method -> selector);

  new_methods[++new_method_ptr] = NULL;
  
  for (i = rcvr_ptr; i >= method_end_ptr; i--) {
    MESSAGE *m;
    m = message_stack_at (i);
    ++m -> evaled;
    ++m -> output;
  }

  return NULL;
}

OBJECT *new_class_method (int method_msg_ptr) {

  int rcvr_ptr,           /* Index of the class message, which is the
			     same class as the method message's receiver 
			     object.                                       */
    param_start_ptr,      /* Index of parameter start.                     */
    param_end_ptr,        /* Index of parameter end.                       */
    method_start_ptr,     /* Index of function body open block.            */
    alias_ptr,      /* Index of the method alias, if any.           */
    funcname_ptr,   /* Index of the function name.                  */
    method_end_ptr, /* End of the method body.                      */
    i,
    buflength,
    prev_error_line;
  char *tmpbuf,     /* Buffer for collected method body tokens.       */
    *srcbuf,      /* Buffer for method with translated declaration. */
    funcname[MAXLABEL],
    alias[MAXLABEL],
    cache_fn[FILENAME_MAX];
  MESSAGE *m_method, /* Message of the, "method," keyword. */
    *m_rcvr;         /* Message of the class object receiver. */
  I_PASS prev_pass;
  METHOD  *n_method,
    *tm,
    *rcvr_method;
  CFUNC *__c_fn;
  CVAR *__cvar_dup;
  int have_c_param;
  char c_param[MAXLABEL];

  char  *tmpname;
  int x_chars_read;
  struct stat statbuf;
  m_method = message_stack_at (method_msg_ptr);

  if (!IS_OBJECT (m_method -> receiver_obj))
    error (m_method, "new_class_method: Undefined receiver object.");

  if ((rcvr_ptr = prevlangmsg (message_stack (), method_msg_ptr)) == ERROR)
    error (m_method, "new_class_method: can't find class object declaration.");

  m_rcvr = message_stack_at (rcvr_ptr);

  if ((strcmp (m_rcvr -> name, m_method -> receiver_obj -> __o_name)) ||
      (m_rcvr -> obj != m_method -> receiver_obj))
    error (m_method, "new_class_method: receiver mismatch error.");

  set_rcvr_class_obj (m_rcvr -> obj);

  if (!IS_CLASS_OBJECT(rcvr_class_obj))
    error (m_method, "new_class_method: Receiver %s is not a class object.",
	   rcvr_class_obj -> __o_name);

  /* 
   *  Find the number of args required by the method declaration.
   *  TO DO - Determine if we can use method_declaration_info ()
   *  here also - we only need n_params here.
   */

  if ((method_end_ptr = find_function_close (message_stack (), method_msg_ptr,
					     get_messageptr ()))
      == ERROR)
    error (m_method, "new_class_method: can't find function close.");

  if ((param_start_ptr = 
       scanforward (message_stack (), method_msg_ptr,
		    method_end_ptr, OPENPAREN)) == ERROR)
    error (m_method, "Missing parameters in method declaration.");

  if ((param_end_ptr = match_paren (message_stack (), param_start_ptr,
				    method_end_ptr))  == ERROR)
    error (m_method, "Missing parameters in method declaration.");

  if (M_TOK(message_stack_at (method_end_ptr)) == SEMICOLON) {
    add_user_prototype (message_stack (), rcvr_ptr, param_start_ptr,
			param_end_ptr, method_end_ptr);
    return SUCCESS;
  } else {
    if ((method_start_ptr = scanforward (message_stack (), param_start_ptr,
					 method_end_ptr, OPENBLOCK)) == ERROR)
      error (m_method, "Missing start of method body.");
  }


  if ((n_method = (METHOD *)__xalloc (sizeof (METHOD))) == NULL)
    _error ("new_class_method: %s.", strerror (errno));
  n_method -> sig = METHOD_SIG;

  new_methods[new_method_ptr--] = create_newmethod_init (n_method);
  new_methods[new_method_ptr+1] -> n_param_newlines = 
    param_newline_count (param_start_ptr, param_end_ptr);

  n_method -> rcvr_class_obj = rcvr_class_obj;

  if (method_contains_argblk (message_stack (), method_start_ptr,
			      method_end_ptr)) {
    
    n_method -> attrs |= METHOD_CONTAINS_ARGBLK_ATTR;
  }

  un_sizeof_declaration (message_stack (), method_msg_ptr, param_start_ptr);

  method_declaration_msg (message_stack (), method_msg_ptr, param_start_ptr, 
			  funcname, &funcname_ptr, 
			  alias, &alias_ptr);

#ifdef DEBUG_UNDEFINED_PARAMETER_CLASSES
  /* Not used right now. */
  /* if (param_class_exception != ERROR) */
  /*   warning (message_stack_at (method_msg_ptr), */
  /* 	     "Undefined parameter class %s. (Method %s, Class %s).", */
  /* 	     n_method -> params[param_class_exception] -> class, */
  /* 	     ((*alias) ? alias : funcname), */
  /* 	     rcvr_class_obj -> __o_name); */
#endif

  strcpy (n_method -> name, ((*alias) ? alias : funcname));
  n_method -> n_args = 0;
  n_method -> nth_local_ptr = 0;
  n_method -> error_line = find_method_start_line (message_stack (), 
						   method_msg_ptr);
  new_methods[new_method_ptr+1] -> source_line = n_method -> error_line;
  n_method -> error_line = m_method -> error_line;
  n_method -> error_column = m_method -> error_column;
  n_method -> imported = False;
  n_method -> queued = False;
  
  method_params_from_src (message_stack (), param_start_ptr, param_end_ptr,
			  n_method, &have_c_param, c_param);

  strcpy (n_method -> selector, 
	  make_selector (rcvr_class_obj, n_method, funcname, i_or_c_class));

  if (method_proto_is_output (n_method -> selector)) {
    for (i = rcvr_ptr; i >= method_end_ptr; i--) {
      MESSAGE *m;
      m = message_stack_at (i);
      ++m -> evaled;
      ++m -> output;
    }
    for (i = 0; i < n_method -> n_params; i++) {
      __xfree (MEMADDR(n_method -> params[i]));
    }
    __xfree (MEMADDR(n_method));

    delete_newmethod (new_methods[new_method_ptr+1]);
    new_methods[++new_method_ptr] = NULL;
    return NULL;
  }

  /* TODO!  */
  /* make this work like instance methods. */
  strcpy (n_method -> returnclass, rcvr_class_obj -> __o_name);

  for (rcvr_method = rcvr_class_obj -> instance_methods; rcvr_method;
       rcvr_method = rcvr_method -> next) {

    if (str_eq (rcvr_method -> name, n_method -> name) && 
	(rcvr_method -> n_params == n_method -> n_params) &&
	(rcvr_method -> prefix == n_method -> prefix) &&
	(rcvr_method -> varargs == n_method -> varargs) &&
	(rcvr_method -> no_init == n_method -> no_init)) {
    
      error (m_method, "Redefinition of method, \"%s.\" (Class %s).",
	     n_method -> name, rcvr_class_obj -> __o_name);
    }
  }

  /* Could try to move this down some, with the other stuff that
     needs to check for a cached method. */
  strcatx (cache_fn, home_libcache_path (), "/", 
	   n_method -> selector, NULL);

  if ((__cvar_dup = _hash_remove (declared_global_variables,
				  n_method -> name)) != NULL) {
    warning (message_stack_at(method_msg_ptr),
	     "Method %s shadows a global variable.", n_method -> name);
    unlink_global_cvar (__cvar_dup);
    _delete_cvar (__cvar_dup);
  }

  if ((__c_fn = get_function (n_method -> name)) != NULL) {
      warning (message_stack_at(method_msg_ptr), 
	       "Method %s shadows a function.", n_method -> name);
  }

  /* Collect tokens from the end of the ctalk declaration to the end of
     the method.
  */

    tmpbuf = 
      collect_tokens (message_stack (), method_start_ptr, method_end_ptr);

     buflength = METHOD_RETURN_LENGTH + 3 + /* 3 spaces */
       METHOD_C_PARAM_LENGTH + 
       strlen (n_method -> selector) + strlen (tmpbuf);

     if ((srcbuf = (char *)__xalloc (buflength + 1)) == NULL)
      _error ("new_class_method: %s.", strerror (errno));
    strcatx (srcbuf, METHOD_RETURN, " ",
	     n_method -> selector, " ",
	     METHOD_C_PARAM, " ",
	     tmpbuf, NULL);

    prev_pass = interpreter_pass;
    interpreter_pass = method_pass;
    /*
      TO DO -
      1. Make sure that methods can be resolved as externs - and also
      class declarations, which might be initialized in a method's
      init block.
    */

    if (!method_deps_updated (n_method -> selector) &&
	!stat (cache_fn, &statbuf) && !nopreload_opt) {

      if ((n_method -> src = (char *) malloc (statbuf.st_size + 2)) == NULL)
      error (m_method, "new_class_method %s (class %s): %s.",
	     n_method -> name, rcvr_class_obj -> __o_name,
	     strerror (errno));

#ifdef __DJGPP__
    errno = 0;
    x_chars_read = read_file (n_method -> src, cache_fn);
    if (errno)
      error (m_method, "new_instance_method %s (class %s): %s.",
	     n_method -> name, rcvr_class_obj -> __o_name,
	     strerror (errno));
#else
    if ((x_chars_read = read_file (n_method -> src, cache_fn)) != statbuf.st_size)
      error (m_method, "new_instance_method %s (class %s): %s.",
	     n_method -> name, rcvr_class_obj -> __o_name,
	     strerror (errno));
#endif

    n_method -> src[x_chars_read] = 0;

    n_method -> imported = True;

    output_pre_method (n_method -> selector, n_method -> src);

  } else { /* if (!file_exists (cache_fn)) */
  
    create_tmp ();
    begin_method_buffer (n_method);
    prev_error_line = error_line;
    error_line = 1;
    parse (srcbuf, (long long)buflength);
    error_line = prev_error_line + last_method_line;
    end_method_buffer ();
    unbuffer_method_statements ();
    close_tmp ();
    generate_class_method_definition_call 
      (rcvr_class_obj -> __o_name,
       ((*alias) ? alias : funcname),
       n_method -> selector,
       n_method -> n_params);
    generate_class_method_return_class_call (rcvr_class_obj -> __o_name,
					     n_method -> name, 
					     n_method -> returnclass,
					     n_method -> n_params);
    for (i = 0; i < n_method -> n_params; i++) 
      generate_class_method_param_definition_call 
	(rcvr_class_obj -> __o_name,
	 ((*alias) ? alias : funcname),
	 n_method -> selector,
	 n_method -> params[i] -> name,
	 n_method -> params[i] -> class,
	 n_method -> params[i] -> is_ptr,
	 n_method -> params[i] -> is_ptrptr
	 );
    tmpname = get_tmpname ();
    stat (tmpname, &statbuf);
    if ((n_method ->  src = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
      error (m_method, "new_class_method %s (class %s): %s.",
	     n_method -> name, rcvr_class_obj -> __o_name,
	     strerror (errno));
#ifdef __DJGPP__
    /*
     *  Let read_file handle the error checking.
     */ 
    x_chars_read = read_file (n_method -> src, tmpname);
#else
    if ((x_chars_read = read_file (n_method ->  src, tmpname)) != statbuf.st_size)
      error (m_method, "new_class_method %s (class %s): %s.",
	     n_method -> name, rcvr_class_obj -> __o_name,
	     strerror (errno));
#endif

    __xfree (MEMADDR(srcbuf));
    __xfree (MEMADDR(tmpbuf));
    interpreter_pass = prev_pass;
    /* Save class libraries only for pre-loading for now. */
    if (library_input && !nopreload_opt) {
      save_pre_init_info (cache_fn, n_method, rcvr_class_obj);
      rename (tmpname, cache_fn);

    }
    remove_tmpfile_entry ();

    n_method -> imported = True;

    queue_method_for_output (rcvr_class_obj, n_method);

  }  

  interpreter_pass = prev_pass;

  if (!library_input) {

    /* Replace the unevaled method's tokens with whitespace so
       the parent parser doesn't evaluate the method body again. */
    /* TO DO - 
       Inform the parent parser simply to skip the frames of the 
       source method, if possible, without adding another state to 
       the parser. */
    for (i = rcvr_ptr; i >= method_end_ptr; i--) {
      MESSAGE *m_tmp;
      m_tmp = message_stack_at (i);
      if ((m_tmp -> tokentype != WHITESPACE) && 
	  (m_tmp -> tokentype != NEWLINE)) {
	m_tmp -> tokentype = WHITESPACE;
	strcpy (m_tmp -> name, " ");
      }
    }
  }

  if (rcvr_class_obj -> class_methods == NULL) {
    rcvr_class_obj -> class_methods = n_method;
  } else {
    for (tm = rcvr_class_obj -> class_methods; tm -> next; tm = tm -> next)
      ;
    tm -> next = n_method;
    n_method -> prev = tm;
  }
  _hash_put (declared_method_names, n_method -> name, n_method -> name);
  _hash_put (declared_method_selectors, n_method -> selector, n_method -> selector);

  new_methods[++new_method_ptr] = NULL;
  
  for (i = rcvr_ptr; i >= method_end_ptr; i--) {
    MESSAGE *m;
    m = message_stack_at (i);
    ++m -> evaled;
    ++m -> output;
  }

  return NULL;
}

/*
 *  Create a method parameter from the method declaration.
 *  Automagically translates C param types into the 
 *  equivalent objects.
 *
 *  These functions are also used in fn_tmpl.c.
 *
 *  TO DO - this function might have to return NULL on error.
 */

/*
 *  The preprocesor stack is unused in these passes, so use
 *  it for temporary tokenizatnion.
 */
extern MESSAGE *p_messages[P_MESSAGES+1]; /* Preprocessor message stack. */

PARAM *method_param_str (char *paramstr) {

  int end_ptr,
    i, i_2,
    lookahead;
  OBJECT *class_obj;
  MESSAGE *m;
  CVAR *cvar;
  CVAR *__cvar_global_obj;
  PARAM *p;
  char parambuf[MAXMSG];

  /*
   *  End parameter with a semicolon so is_c_var_declaration_msg ()
   *  doesn't return False over an unknown state at the last token.
   */
  strcatx (parambuf, paramstr, ";", NULL);
  end_ptr = tokenize_no_error (p_message_push, parambuf);

  p = new_param ();

  for (i = P_MESSAGES; i > end_ptr; i--) {

    m = p_messages[i];

    switch (m -> tokentype) 
      {
      case LABEL:
	if (!strcmp (M_NAME(m), PARAM_C_ARG_ATTR)) {
	  p -> attrs |= PARAM_C_PARAM;
	  continue;
	}
	if (!strcmp (M_NAME(m), PARAM_PREFIX_METHOD_ATTR)) {
	  p -> attrs |= PARAM_PREFIX_METHOD_ATTRIBUTE;
	}
	if (is_c_data_type (M_NAME(m)) ||
	    is_c_derived_type (M_NAME(m)) ||
	    is_incomplete_type (M_NAME(m))) { 
	  /* Map C data types to classes. */
	  /* Check incomplete types for stuff like FILE, __FILE, etc... */
	  if (is_c_var_declaration_msg (p_messages, i, end_ptr - 1, TRUE)) {
	    cvar = __cvar_global_obj = NULL;
 	    cvar = parser_to_cvars ();
 	    strcpy (p -> name, cvar -> name);
 	    p -> n_derefs = cvar -> n_derefs;
	    strcpy (p -> class, basic_class_from_cvar
		    (NULL, cvar, 0));
	    _delete_cvar (cvar);

	    for (i_2 = end_ptr; i_2 <= P_MESSAGES; i_2++)
	      reuse_message (p_message_pop ());
	    return p;
	  }
	} else { /* if (is_c_data_type (m -> name)) */
	  /*
	   * We should be able to use the highest level routine
	   * possible here, so we can search for library classes
	   * and local classes in the source text.
	   */
	  if ((class_obj = class_object_search (m -> name, FALSE)) != NULL) {
	    strcpy (p -> class, m -> name);
	  } else {
	    if (!strcmp (m -> name, "OBJECT")) {
	      strcpy (p -> class, OBJECT_CLASSNAME);
	    } else {
	      if (!(*p -> class) && 
		  !(p -> attrs & PARAM_PREFIX_METHOD_ATTRIBUTE)) {
		warning (m, "Undefined parameter class %s, defaulting to Object.\n", m -> name);
		strcpy (p -> class, OBJECT_CLASSNAME);
	      } else {
		strcpy (p -> name, m -> name);
	      }
	    }
	  }
	}
	break;
      case ELLIPSIS:
	strcpy (p -> class, "ArgumentList");
	strcpy (p -> name, m -> name);
	p -> attrs |= PARAM_VARARGS_PARAM;
	break;
      case ASTERISK:
	for (lookahead = i - 1, p -> is_ptrptr = -1; 
	     lookahead > end_ptr; lookahead--) {
	  if ((p_messages[lookahead] -> tokentype == NEWLINE) ||
	      (p_messages[lookahead] -> tokentype == WHITESPACE))
	    continue;
	  if (p_messages[lookahead] -> tokentype != ASTERISK)
	    break;
	  p -> is_ptrptr = TRUE;
	  /*
	   *  Decrement the stack pointer to look past the
	   *  second asterisk on the next iteration.
	   */
	  --i;
	}
	if (p -> is_ptrptr == -1) {
	  p -> is_ptr = TRUE;
	  p -> is_ptrptr = FALSE;
	}
	break;
      default:
	break;
      }
  }

  /* This calls p_message_pop () enought times to remove all of the
     tokens from the stack. */
  REUSE_MESSAGES(p_messages, end_ptr, P_MESSAGES);

  return p;
}

PARAM *method_param_tok (MESSAGE_STACK messages, PARAMLIST_ENTRY *param) {

  int i,
    lookahead;
  OBJECT *class_obj;
  MESSAGE *m;
  CVAR *cvar;
  CVAR *__cvar_global_obj;
  PARAM *p;

  /*
   *  End parameter with a semicolon so is_c_var_declaration_msg ()
   *  doesn't return False over an unknown state at the last token.
   */

  p = new_param ();

  for (i = param -> start_ptr; i >= param -> end_ptr; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    m = messages[i];

    switch (m -> tokentype) 
      {
      case LABEL:
	if (!strcmp (M_NAME(m), PARAM_PREFIX_METHOD_ATTR)) {
	  strcpy (p -> name, m -> name);
	  strcpy (p -> class, NULLSTR);
	  p -> attrs |= PARAM_PREFIX_METHOD_ATTRIBUTE;
	  continue;
	} else if (!strcmp (M_NAME(m), PARAM_C_ARG_ATTR)) {
	  strcpy (p -> name, m -> name);
	  strcpy (p -> class, NULLSTR);
	  p -> attrs |= PARAM_C_PARAM;
	  continue;
	}
	if (is_c_data_type (M_NAME(m)) ||
	    is_c_derived_type (M_NAME(m)) ||
	    is_incomplete_type (M_NAME(m))) { 
	  /* Map C data types to classes. */
	  /* Check incomplete types for stuff like FILE, __FILE, etc... */
	  if (is_c_param_declaration_msg (messages, i, TRUE)) {
	    cvar = __cvar_global_obj = NULL;
 	    cvar = parser_to_cvars ();
 	    strcpy (p -> name, cvar -> name);
 	    p -> n_derefs = cvar -> n_derefs;
	    strcpy (p -> class, basic_class_from_cvar
		    (NULL, cvar, 0));
	    if (cvar -> attrs & CVAR_ATTR_FN_PTR_DECL)
	      p -> attrs |= PARAM_PFO;
	    if (have_unknown_c_type ()) {
	      warning (m, "Parameter, \"%s,\" (type %s) is not translatable to a class.",
		       p -> name,
		       cvar -> type);
	    }
	    _delete_cvar (cvar);
	    return p;
	  }
	} else { /* if (is_c_data_type (m -> name)) */
	  /*
	   * We should be able to use the highest level routine
	   * possible here, so we can search for library classes
	   * and local classes in the source text.
	   */
	  if (((class_obj = class_object_search (m -> name, FALSE)) != NULL) ||
	      is_pending_class (m -> name)) {
	    strcpy (p -> class, m -> name);
	  } else {
	    if (!strcmp (m -> name, "OBJECT")) {
	      strcpy (p -> class, OBJECT_CLASSNAME);
	    } else {
	      if (!(*p -> class) && 
		  !(p -> attrs & PARAM_PREFIX_METHOD_ATTRIBUTE)) {
		warning (m, "Undefined parameter class %s, defaulting to Object.\n", m -> name);
		strcpy (p -> class, OBJECT_CLASSNAME);
	      } else {
		strcpy (p -> name, m -> name);
		if (param_label_shadows_method (p -> class, m -> name)) {
		  warning (m, "Parameter label, \"%s,\", shadows a method.",
			   m -> name);
		}
	      }
	    }
	  }
	}
	break;
      case ELLIPSIS:
	strcpy (p -> class, "ArgumentList");
	strcpy (p -> name, m -> name);
	p -> attrs |= PARAM_VARARGS_PARAM;
	break;
      case ASTERISK:
	for (lookahead = i - 1, p -> is_ptrptr = -1; 
	     lookahead >= param -> end_ptr; lookahead--) {
	  if (M_ISSPACE(messages[lookahead]))
	    continue;
	  if (messages[lookahead] -> tokentype != ASTERISK)
	    break;
	  p -> is_ptrptr = TRUE;
	  /*
	   *  Decrement the stack pointer to look past the
	   *  second asterisk on the next iteration.
	   */
	  --i;
	}
	if (p -> is_ptrptr == -1) {
	  p -> is_ptr = TRUE;
	  p -> is_ptrptr = FALSE;
	}
	break;
      default:
	break;
      }
  }

  return p;
}

int param_class_exists (char *class) {
  if (is_cached_class (class) ||
      is_pending_class (class))
    return TRUE;
  else
    return FALSE;
}

static struct  _var_definition_args {
  char classobjname[MAXLABEL];
  char varname[MAXLABEL];
  char varvalueclassname[MAXLABEL];
  char initial_value[MAXLABEL];
} var_definition_args;

static inline void constructor_add_instancevars (OBJECT *class, OBJECT *o) {
  OBJECT *var;

  for (var = class -> instancevars -> next; var; var = var -> next) {
    /* if (!__ctalkGetInstanceVariable (o, var -> __o_name, FALSE)) { */
      __ctalkAddInstanceVariable (o, var -> __o_name, var);
      /* } */
  }
  /*  return SUCCESS;*/
}

/*
 *  Used for initializing class member variables.  It uses 
 *  the, "value," instance variable, and has no problems with
 *  extracting instance variable definitions at the same time
 *  the class is being defined.
 */
static inline int 
  instance_vars_from_class_object (OBJECT *o) {

  OBJECT *class_object, *superclass_object;

  if ((class_object = ((o -> instancevars) ? 
		       o -> instancevars -> __o_class :
		       o -> __o_class)) == NULL) {
    __ctalkExceptionInternal (NULL, undefined_class_x, o -> __o_classname,0);
    return ERROR;
  }

  constructor_add_instancevars (class_object, o);

  for (superclass_object = 
	 __ctalk_get_object (class_object -> __o_superclassname, "Class");
       superclass_object && *superclass_object -> __o_superclassname;
       superclass_object = 
	 __ctalk_get_object (superclass_object -> __o_superclassname, "Class"))
    constructor_add_instancevars (superclass_object, o);
  return SUCCESS;
}

/*
 *  TO DO - It's less confusing to have a stack of these 
 *  than local copies when recursing.
 */
static struct _var_definition_idx {
  int class_obj_name_idx,
    var_name_idx,
    var_value_class_name_idx,
    initial_value_start_idx,
    initial_value_end_idx;
} var_definition_idx;

static void undefined_var_class_exception (MESSAGE_STACK messages, 
					   int var_class_idx,
					   char *var_class_name) {
  if (var_class_name[0] == 0) {
    if (IS_MESSAGE (messages[var_class_idx])) {
      __ctalkExceptionInternal (messages[var_class_idx],
				undefined_class_x,
				M_NAME(messages[var_class_idx]),
				0);
    } else {
      __ctalkExceptionInternal (messages[var_class_idx],
				undefined_class_x,
				NULLSTR, 0);
    }
  } else {
      __ctalkExceptionInternal (messages[var_class_idx],
				undefined_class_x,
				var_class_name,	0);
  }
}

/* On entry, expr_end_ptr should be the stack index of the
   semicolon terminator. */
static int find_initializer_end (MESSAGE_STACK messages,
				 int initializer_start,
				 int expr_end_ptr) {
  int p_tok;
  p_tok = prevlangmsg (messages, expr_end_ptr);
  /* There should not be any cases where there's just a set of tokens
     followed by a literal then the semicolon. */
  if ((M_TOK(messages[p_tok]) == LITERAL) && (p_tok < initializer_start)) {
    return prevlangmsg (messages, p_tok);
  } else {
    return p_tok;
  }
}

static char var_value_buf[MAXMSG];
static OBJECT *__var_typecast_class;
static OBJECT *var_definition (MESSAGE_STACK messages, int idx) {

  int class_ptr, arg_ptr, value_lex_type = 0;  /* Avoid a warning. */
  int typecast_expr_end;
  OBJECT *var_class_object, *v;
  struct _tmp_var_definition_idx {
  int class_obj_name_idx,
    var_name_idx,
    var_value_class_name_idx,
    initial_value_start_idx,
    initial_value_end_idx;
  } tmp_var_definition_idx;

  var_definition_idx.class_obj_name_idx = 
    var_definition_idx.var_name_idx =
    var_definition_idx.var_value_class_name_idx =
    var_definition_idx.initial_value_start_idx =
    var_definition_idx.initial_value_end_idx = ERROR;

  if ((class_ptr = prevlangmsg (messages, idx)) == ERROR) {
    _warning ("Undefined class in var_definition ().\n");
    return NULL;
  }
  var_definition_idx.class_obj_name_idx = class_ptr;

  if (((arg_ptr = nextlangmsg (messages, idx)) == ERROR) ||
      (M_TOK(messages[arg_ptr]) == SEMICOLON)) {
    _warning ("Undefined arg in var_definition ().\n");
    return NULL;
  }

  do {
    if (M_TOK(messages[arg_ptr]) == SEMICOLON) break;

    if (M_TOK(messages[arg_ptr]) ==  LITERAL) {
      int prev_tok;
      prev_tok = prevlangmsg (messages, arg_ptr);

      switch (M_TOK(messages[prev_tok]))
	{
	case LABEL:
	  if (var_definition_idx.var_value_class_name_idx == -1) {
	    /* An abbreviated definition. */
	    continue;
	  }
	  break;
	case LITERAL:
	  if (var_definition_idx.var_value_class_name_idx != -1) {
	    if ((var_definition_idx.initial_value_start_idx != -1) &&
		(var_definition_idx.initial_value_end_idx != -1)) {
	      /* We already have an initializer. */
	      continue;
	    }
	  }
	default:
	  /* Anything else was an initializer... */
	  continue;
	  break;
	}
    }

    if (var_definition_idx.var_name_idx == ERROR) {
      if (!lextype_is_PTR_T (M_NAME(messages[arg_ptr]))) {
	warning (messages[idx], "instance or class variable argument syntax error.");
	break;
      }
      var_definition_idx.var_name_idx = arg_ptr;
      continue;
    }

    if (var_definition_idx.var_value_class_name_idx == ERROR) {
      if (!is_typecast_expr (messages, arg_ptr, &typecast_expr_end)) {
	if (!lextype_is_PTR_T (M_NAME(messages[arg_ptr]))) {
	  warning (messages[idx], "instance or class variable argument syntax error.");
	  break;
	}
	var_definition_idx.var_value_class_name_idx = arg_ptr;
      } else {
	var_definition_idx.var_value_class_name_idx = arg_ptr;
	arg_ptr = typecast_expr_end;
      }
      value_lex_type = lextype_of_class (M_NAME(messages[arg_ptr]));
      continue;
    }
    
    if (var_definition_idx.initial_value_start_idx == ERROR) {
      VAL result;
      int expr_end_ptr;
      int value_class_name_idx;

      memset (&result, 0, sizeof (struct _val));
      if (M_TOK(messages[arg_ptr]) != LITERAL)  {
	/* Literal is always a single token. */
	expr_end_ptr = find_instancevar_declaration_end (messages, arg_ptr);
	expr_end_ptr = find_initializer_end (messages, arg_ptr, expr_end_ptr);
      } else {
	expr_end_ptr = arg_ptr;
      }
      eval_constant_expr (messages, arg_ptr, &expr_end_ptr, &result);
      var_definition_idx.initial_value_start_idx = arg_ptr;
      var_definition_idx.initial_value_end_idx = expr_end_ptr;

      switch (value_lex_type)
	{
	case INTEGER_T:
	  __ctalkDecimalIntegerToASCII (result.__value.__i, var_value_buf);
	  break;
	case LONGLONG_T:
	  sprintf (var_value_buf, "%lldll", result.__value.__ll);
	  break;
	case FLOAT_T:
	  sprintf (var_value_buf, "%f", 
		   ((result.__value.__d != 0) ? result.__value.__d : 0.0));
	  break;
	case PTR_T:
	  if (var_definition_idx.var_value_class_name_idx != ERROR) {
	    value_class_name_idx = 
	      var_definition_idx.var_value_class_name_idx;
	  } else {
	    value_class_name_idx =
	      var_definition_idx.class_obj_name_idx;
	  }
	  if (!strcmp (M_NAME(messages[value_class_name_idx]), 
		       STRING_CLASSNAME)) {
	    if (result.__value.__ptr == 0) {
	      strcpy (var_value_buf, NULLSTR);
	    } else {
	      strcpy (var_value_buf, (char *)result.__value.__ptr);
	    }
	    while (*var_value_buf == '\"')
	      TRIM_LITERAL (var_value_buf);
	  } else {
	    PHTOA(var_value_buf,result.__value.__ptr);
	  }
	  break;
	default:
	  __ctalkDecimalIntegerToASCII (result.__value.__i, var_value_buf);
	  break;
	}
      continue;
    }

  } while ((arg_ptr = nextlangmsg (messages, arg_ptr)) != ERROR);

  if (var_definition_idx.class_obj_name_idx == -1 ||
      var_definition_idx.var_name_idx == -1) {
    invalid_class_variable_declaration_error 
      (messages,  var_definition_idx.class_obj_name_idx);
    /* Doesn't return. */
  }

  tmp_var_definition_idx.class_obj_name_idx = 
    var_definition_idx.class_obj_name_idx;
  tmp_var_definition_idx.var_name_idx = 
    var_definition_idx.var_name_idx;
  tmp_var_definition_idx.var_value_class_name_idx = 
    var_definition_idx.var_value_class_name_idx;
  tmp_var_definition_idx.initial_value_start_idx = 
    var_definition_idx.initial_value_start_idx;
  tmp_var_definition_idx.initial_value_end_idx = 
    var_definition_idx.initial_value_end_idx;

  if ((var_class_object = 
       class_object_search 
       (M_NAME(messages[var_definition_idx.class_obj_name_idx]),
	FALSE)) == NULL) {
    __ctalkExceptionInternal
      (messages[idx], undefined_class_x, 
       M_NAME(messages[var_definition_idx.class_obj_name_idx]),0);
    return NULL;
  }

  var_definition_idx.class_obj_name_idx = 
    tmp_var_definition_idx.class_obj_name_idx;
  var_definition_idx.var_name_idx = 
    tmp_var_definition_idx.var_name_idx;
  var_definition_idx.var_value_class_name_idx = 
    tmp_var_definition_idx.var_value_class_name_idx;
  var_definition_idx.initial_value_start_idx = 
    tmp_var_definition_idx.initial_value_start_idx;
  var_definition_idx.initial_value_end_idx = 
    tmp_var_definition_idx.initial_value_end_idx;

  if ((v = 
       create_object_init (var_class_object -> __o_name,
			   var_class_object -> __o_superclassname,
			   M_NAME(messages[var_definition_idx.var_name_idx]),
			   var_value_buf)) == NULL)
    _warning ("var_definition (): Error.\n");

  if (var_definition_idx.var_value_class_name_idx != ERROR) {
    OBJECT *var_value_class_object;

    tmp_var_definition_idx.class_obj_name_idx = 
      var_definition_idx.class_obj_name_idx;
    tmp_var_definition_idx.var_name_idx = 
      var_definition_idx.var_name_idx;
    tmp_var_definition_idx.var_value_class_name_idx = 
      var_definition_idx.var_value_class_name_idx;
    tmp_var_definition_idx.initial_value_start_idx = 
      var_definition_idx.initial_value_start_idx;
    tmp_var_definition_idx.initial_value_end_idx = 
      var_definition_idx.initial_value_end_idx;

      /*
       *  NOTE: This basically allows you to use a typecast in
       *  place of the variable's native class. See 
       *  basic_class_from_cvar () in objtoc.c for the C to classlib 
       *  translations that Ctalk knows about now.  Anything that is a 
       *  cast to a pointer type gets "Symbol" as the class of 
       *  the instance or class variable anyway.  
       *
       *  It isn't possible, at the moment, to use a C variable as
       *  the initializer of a class or instance variable.  Instance
       *  variables aren't actually instantiated until the program
       *  calls some implementation of the "new" method or other.
       *  Might be possible to do by extending __ctalk_new_object () or 
       *  __ctalkInstanceVarsFromClassObject or something similar, and 
       *  you need to register the C variable each time that the 
       *  constructor is called.
       */
    if (is_typecast_expr (messages, 
			  var_definition_idx.var_value_class_name_idx,
			  &typecast_expr_end)) {
      var_value_class_object =
	class_object_search 
	(basic_class_from_typecast 
	 (messages,
	  var_definition_idx.var_value_class_name_idx,
	  typecast_expr_end), FALSE);
      if (typecast_is_pointer (messages, 
			       var_definition_idx.var_value_class_name_idx,
			       typecast_expr_end)) {
	if (DEFAULTCLASS_CMP(var_value_class_object,
			     ct_defclasses -> p_integer_class,
			     INTEGER_CLASSNAME)) {
	  if ((var_value_class_object = 
	       class_object_search (SYMBOL_CLASSNAME, FALSE)) == NULL) {
	    undefined_var_class_exception 
	      (messages, 
	       var_definition_idx.var_value_class_name_idx,
	       var_definition_args.varvalueclassname);
	    return NULL;
	  }
	}
      }
      __var_typecast_class = var_value_class_object;
    } else {
      if ((var_value_class_object = 
	   class_object_search 
	   (M_NAME(messages[var_definition_idx.var_value_class_name_idx]),
	    FALSE))== NULL) {
	undefined_var_class_exception 
	  (messages, 
	   var_definition_idx.var_value_class_name_idx,
	   var_definition_args.varvalueclassname);
	return NULL;
      }
    }

    var_definition_idx.class_obj_name_idx = 
      tmp_var_definition_idx.class_obj_name_idx;
    var_definition_idx.var_name_idx = 
      tmp_var_definition_idx.var_name_idx;
    var_definition_idx.var_value_class_name_idx = 
      tmp_var_definition_idx.var_value_class_name_idx;
    var_definition_idx.initial_value_start_idx = 
      tmp_var_definition_idx.initial_value_start_idx;
    var_definition_idx.initial_value_end_idx = 
      tmp_var_definition_idx.initial_value_end_idx;

    __ctalkSetObjectValueClass (v, var_value_class_object);
  }
  __ctalkSetObjectValue (v, var_value_buf);
  /*
   *  Instance_vars_from_class_object uses the class
   *  of the value instance variable, if available,
   *  when setting class definitions.  This avoids
   *  recursion problems in defining instance variables
   *  while the class is still being defined.
   */
  instance_vars_from_class_object (v);

  return v;
}

static int set_var_definition_evaled (MESSAGE_STACK messages,int keyword_ptr) {

  int i, stack_top, rcvr_ptr;
  MESSAGE *m;

  if ((rcvr_ptr = prevlangmsg (messages, keyword_ptr)) == ERROR) {
    _warning ("Parser error.\n");
    return ERROR;
  }

  stack_top = get_stack_top (messages);
  for (i = rcvr_ptr; i > stack_top; i--) {
    m = messages[i];
    ++m -> evaled;
    ++m -> output;
    if (M_ISSPACE (m)) continue;

    if (M_TOK(m) == SEMICOLON) 
      break;
  }

  return SUCCESS;
}

bool is_instance_var (char *s) {
  if (_hash_get (defined_instancevars, s))
    return true;
  else
    return false;
}

OBJECT *define_instance_variable (int keyword_ptr) {
  OBJECT *v, *v_ptr, *rcvr_obj = NULL;  /* Avoid a warning. */
  int rcvr_ptr;
  __var_typecast_class = NULL;
  if ((v = var_definition (message_stack (), keyword_ptr)) != NULL) {

    _hash_put (defined_instancevars, v -> __o_name, v -> __o_name);

    if ((rcvr_ptr = prevlangmsg (message_stack (), keyword_ptr)) == ERROR) {
      _warning ("Undefined class in define_instance_variable ().\n");
      return NULL;
    }
    
    if ((rcvr_obj = 
	 class_object_search (M_NAME(message_stack_at (rcvr_ptr)),
			      FALSE)) == NULL) {
      __ctalkExceptionInternal (message_stack_at (rcvr_ptr), 
				undefined_class_x, 
				M_NAME(message_stack_at (rcvr_ptr)),0);
      return NULL;
    }

    if (rcvr_obj -> instancevars == NULL) {
      rcvr_obj -> instancevars = v;
    } else {
      for (v_ptr = rcvr_obj->instancevars; v_ptr -> next; v_ptr=v_ptr -> next)
	;
      v_ptr -> next = v;
      v -> prev = v_ptr;
    }

  }

  if ((var_definition_idx.class_obj_name_idx == -1) ||
      (var_definition_idx.var_name_idx == -1)) {
    warning (message_stack_at (keyword_ptr), 
	     "Incomplete instance variable definition.");
  }

  generate_define_instance_variable_call 
    (M_NAME(message_stack_at(var_definition_idx.class_obj_name_idx)),
     M_NAME(message_stack_at(var_definition_idx.var_name_idx)),
     ((var_definition_idx.var_value_class_name_idx == -1) ? "" :
      ((__var_typecast_class) ? __var_typecast_class -> __o_name: 
       M_NAME(message_stack_at(var_definition_idx.var_value_class_name_idx)))),
     ((var_definition_idx.initial_value_start_idx == -1) ? "" :
      var_value_buf));

  save_instance_var_init_info 
    (M_NAME(message_stack_at(var_definition_idx.class_obj_name_idx)),
     M_NAME(message_stack_at(var_definition_idx.var_name_idx)),
     ((var_definition_idx.var_value_class_name_idx == -1) ? "" :
      ((__var_typecast_class) ? __var_typecast_class -> __o_name: 
       M_NAME(message_stack_at(var_definition_idx.var_value_class_name_idx)))),
     ((var_definition_idx.initial_value_start_idx == -1) ? "" :
      var_value_buf));
     

  set_var_definition_evaled (message_stack (), keyword_ptr);

  return rcvr_obj;
}

OBJECT *define_class_variable (int keyword_ptr) {
  OBJECT *v, *v_ptr, *rcvr_obj;
  int rcvr_ptr;
  if ((v = var_definition (message_stack (), keyword_ptr)) != NULL) {

    if ((rcvr_ptr = prevlangmsg (message_stack (), keyword_ptr)) == ERROR) {
      _warning ("Undefined class in define_class_variabel ().\n");
      return NULL;
    }
    
    if ((rcvr_obj = 
	 class_object_search (M_NAME(message_stack_at (rcvr_ptr)),
			      FALSE)) == NULL) {
      __ctalkExceptionInternal (message_stack_at (rcvr_ptr), 
				undefined_class_x, 
				M_NAME(message_stack_at (rcvr_ptr)),0);
      return NULL;
    }

    if (rcvr_obj -> classvars == NULL) {
      rcvr_obj -> classvars = v;
    } else {
      for (v_ptr = rcvr_obj -> classvars; v_ptr -> next; v_ptr=v_ptr -> next)
	;
      v_ptr -> next = v;
      v -> prev = v_ptr;
    }

  }

  if ((var_definition_idx.class_obj_name_idx == -1) ||
      (var_definition_idx.var_name_idx == -1)) {
    warning (message_stack_at (keyword_ptr), 
	     "Incomplete class variable definition.");
  }

  generate_define_class_variable_call 
    (M_NAME(message_stack_at(var_definition_idx.class_obj_name_idx)),
     M_NAME(message_stack_at(var_definition_idx.var_name_idx)),
     ((var_definition_idx.var_value_class_name_idx == -1) ? NULL :
      M_NAME(message_stack_at(var_definition_idx.var_value_class_name_idx))),
     ((var_definition_idx.initial_value_start_idx == -1) ? NULL :
      var_value_buf));

  save_class_var_init_info 
    (M_NAME(message_stack_at(var_definition_idx.class_obj_name_idx)),
     M_NAME(message_stack_at(var_definition_idx.var_name_idx)),
     ((var_definition_idx.var_value_class_name_idx == -1) ? "" :
      ((__var_typecast_class) ? __var_typecast_class -> __o_name: 
       M_NAME(message_stack_at(var_definition_idx.var_value_class_name_idx)))),
     ((var_definition_idx.initial_value_start_idx == -1) ? "" :
      var_value_buf));
     

  set_var_definition_evaled (message_stack (), keyword_ptr);

  return v;
}

int class_primitive_method_args (METHOD *method, MESSAGE_STACK messages, 
				 int method_idx) {

  int stack_top;
  int i;
  MESSAGE *m;
  enum {class_arg_null,
	class_arg_method,
	class_arg_arg,
	class_arg_docstring,
	class_arg_end
  } class_arg_state;

  stack_top = get_stack_top (messages);

  for (i = method_idx, class_arg_state = class_arg_null; 
       (i > stack_top) && (class_arg_state != class_arg_end); i--) {
    
    m = messages[i];
    if (M_ISSPACE(m)) continue;
    switch (class_arg_state)
      {
      case class_arg_null:
	if (strcmp (M_NAME(m), method -> name)) 
	  _error ("class_primitive_method_args: parser error.\n");
	class_arg_state = class_arg_method;
	break;
      case class_arg_method:
	if (M_TOK(m) == LABEL) {
	    /*
	     *  Create the object manually.
	     */
	  method->args[method->n_args] = 
	    create_arg_init (create_object ("Class", M_NAME(m)));
	  method->args[method->n_args] -> obj -> instancevars =  
	    create_object (M_NAME(m), "value");
	  strcpy (method->args[method->n_args] -> obj -> __o_superclassname,
		  messages[method_idx]->receiver_obj -> __o_name);
	  strcpy (method->args[method->n_args] -> obj
		  -> instancevars -> __o_superclassname,
		  messages[method_idx]->receiver_obj -> __o_name);

	  method->args[method->n_args] -> obj -> __o_class =
	  method->args[method->n_args] -> obj -> instancevars -> __o_class =
	    method->args[method->n_args] -> obj;

	  method->args[method->n_args]->obj->__o_superclass =
	  method->args[method->n_args]->obj->instancevars->__o_superclass =
	    messages[method_idx]->receiver_obj;

	  ++method -> n_args;
	  class_arg_state = class_arg_arg;
	} else {
	  __ctalkExceptionInternal (m, parse_error_x, M_NAME(m),0);
	}
	break;
      case class_arg_arg:
	if (M_TOK(m) != SEMICOLON) {
	  class_arg_state = class_arg_end;
	} else if (M_TOK(m) != LITERAL) {
	  class_arg_state = class_arg_docstring;
	} else {
	  __ctalkExceptionInternal (m, parse_error_x, M_NAME(m),0);
	}
	break;
      case class_arg_docstring:
	if (M_TOK(m) != SEMICOLON) {
	  class_arg_state = class_arg_end;
	} else {
	  __ctalkExceptionInternal (m, parse_error_x, M_NAME(m),0);
	}
	break;
      case class_arg_end:
	break;
      }
  }

  return SUCCESS;
}


/* This is not very fast, but we only need to call it once, after
   the input has been processed. 
*/
void check_return_classes (void) {
  OBJECT *c;
  METHOD *m;
  char classname[MAXLABEL];

  for (c = classes; c; c = c -> next) {

    strcpy (classname, c -> __o_name);

    for (m = c -> instance_methods; m; m = m -> next) {

      if (str_eq (m -> returnclass, "Any"))
	continue;

      /* 
	 A class that is defined in a source input file 
	 is still defined.  This prevents a warning and
	 attempted load for non-library classes.
      */
      if (str_eq (classname, m -> returnclass))
	continue;

      if (!is_cached_class (m -> returnclass)) {

	/* A class declared in the source input won't be
	   in the library cache, so look for an actual class
	   object also. */
	if ((library_search (m -> returnclass, FALSE) != SUCCESS) &&
	    !get_class_object (m -> returnclass)) {
	  printf ("warning: undefined return class.\n"
		"\tMethod, \"%s,\" (class %s) has undefined\n"
		"\treturn class, \"%s.\"\n",
		m -> name, classname, m -> returnclass);
	}

      }
    }

    for (m = c -> class_methods; m; m = m -> next) {

      if (str_eq (m -> returnclass, "Any"))
	continue;

      /* See the comment above. */
      if (str_eq (classname, m -> returnclass))
	continue;

      if (!is_cached_class (m -> returnclass)) {

	/* See the comment above. */
	if ((library_search (m -> returnclass, FALSE) != SUCCESS) &&
	    !get_class_object (m -> returnclass)) {
	  printf ("warning: undefined return class.\n"
		  "\tMethod, \"%s,\" (class %s) has undefined\n"
		  "\treturn class, \"%s.\"\n",
		  m -> name, classname, m -> returnclass);
	}

      }
    }

  }
}

