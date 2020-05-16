/* $Id: fn_tmpl.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2020 Robert Kiesling, rk3314042@gmail.com.
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
 *  Functions that handle C functions in method arguments.
 *  If there is an assignment, 
 *  Some cases:
 *
 *  1.  myInt = strlen (myString)    -->
 *      myInt = CFunction cStrlen (myString);
 *  
 *  2.  ((myInt = strlen (myString)) == length) --> 
 *      ((myInt = CFunction cStrlen (myString)) == length)
 *
 *  3.  (strlen (myString) != EOF)   -->
 *      (CFunction cStrlen (myString) != EOF)
 *  
 *  The first two cases, where the libc function is the argument
 *  to a method, are handled by clib_fn_expr (), and get the 
 *  function object from eval_arg () and other functions in 
 *  method.c. 
 *
 *  The third case, where the function appears in a C context, 
 *  is handled by clib_fn_rt_expr (), and it creates its own
 *  expression object for the function template.
 *
 *  TO DO - If other contexts for libc functions arise.
 *
 *  1. Much of the code is duplicated, and should be 
 *     factored out.
 *  2. Add a check for the context that the libc function appears
 *     in, both in conditional predicates and normal statements,
 *     and handle both of these cases with the same functions.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "list.h"
#include "fntmpl.h"

extern I_PASS interpreter_pass;            /* Declared in main.c.            */

extern int print_templates_opt;            /* Declared in main.c.         */

extern char *library_include_paths[MAXUSERDIRS];   /* Declared in rtinfo.c. */

static bool expr_class_defined = False; /* CFunction class and superclass */
static bool cfunction_class_defined = False; /* definitions added to      */
                                           /* __ctalk_init ().               */
static bool template_info_method_initialized = False;

extern int nolinemarker_opt,               /* Declared in main.c.            */
  nolibinc_opt;

extern RT_EXPR_CLASS rt_expr_class;        /* Declared in control.c.         */

MESSAGE *tmpl_messages[N_MESSAGES + 1];
int tmpl_messageptr;

static LIST *pending_templates,
  *templates_ptr;

static LIST *cached_fns,   /* The names of the C functions that have already */
  *cached_fn_ptr;          /* been registered.                               */

static char ctpp_ofn_template[FILENAME_MAX];

static METHOD template_info_method;
static OBJECT *CFunction_class_object = NULL;
static OBJECT *Expression_class_object = NULL;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

#define TEMPLATE_CLASS_INIT_PRIORITY 0
#define TEMPLATE_INIT_PRIORITY 2

bool fn_tmpl_eval_arg = false;

MESSAGE_STACK tmpl_message_stack (void) {
  return tmpl_messages;
}

int tmpl_message_push (MESSAGE *m) {
  if (tmpl_messageptr == 0) {
    _error ("Tmpl_message_push: stack_overflow.");
    return ERROR;
  }

  if (!m)
    _error ("Tmpl_message_push: null pointer, tmpl_messageptr = %d.", 
	    tmpl_messageptr);

  tmpl_messages[tmpl_messageptr] = m;

#ifdef STACK_TRACE
  debug ("Tmpl_message_push %d. %s.", tmpl_messageptr, 
	 (tmpl_messages[tmpl_messageptr] && 
	  IS_MESSAGE (tmpl_messages[tmpl_messageptr])) ?
	 tmpl_messages[tmpl_messageptr] -> name : "(null)"); 
#endif

  --tmpl_messageptr;

  return tmpl_messageptr + 1;
}

MESSAGE *tmpl_message_pop (void) {

  MESSAGE *m;

  if (tmpl_messageptr == N_MESSAGES) {
    return (MESSAGE *)NULL;
  }

  if (tmpl_messageptr > N_MESSAGES)
    _error ("Tmpl_message_pop: Message stack overflow, tmpl_messageptr = %d.", 
		  tmpl_messageptr);

  m = tmpl_messages[++tmpl_messageptr];
  tmpl_messages[tmpl_messageptr] = NULL;

#ifdef STACK_TRACE
  debug ("Tmpl_message_pop %d. %s.", tmpl_messageptr, m -> name);
#endif

  if (!m || !IS_MESSAGE (m))
    _error ("tmpl_message_pop (): Bad message, stack index %d.", 
	    tmpl_messageptr);
  else
    return m;
}

int get_tmpl_messageptr (void) {
  return tmpl_messageptr;
}

MESSAGE *tmpl_message_stack_at (int idx) {
  return tmpl_messages[idx];
}

/*
 *  Create a run-time expression for a function template.  The
 *  idx parameter should point to the libc function we need to
 *  replace.  
 *  
 *  If the function is on the right hand side of a method, the 
 *  CFunction method registration is handled earlier in 
 *  ctrlblk_pred_rt_expr (), with a CFunction object created by
 *  eval_arg ().
 * 
 *  If, however, the function is called individually; e.g., 
 *
 *    while (scanf ("%c", c) != EOF) ... 
 *
 *  then we create a temporary CFunction object on the fly, and 
 *  we don't need to find a receiver class, because the template
 *  method's class is CFunction.
 *
 *  NOTE: At the moment it is only necessary to replace the message 
 *  name.  If the parser needs to make another pass, it will be necessary
 *  to return a value object, or re-tokenize the expression.
 */
char *clib_fn_rt_expr (MESSAGE_STACK messages, int idx) {

  static char buf[MAXMSG];
  char fn_name[MAXLABEL], method_name[MAXLABEL], exprbuf[MAXMSG];
  char *c99name, *cache_entry, *preprocessed_template, *c_template;
  int param_start_idx, param_end_idx;
  FN_TMPL *template;
  OBJECT *fn_object;

  if ((c99name = template_name (messages[idx] -> name)) == NULL)
    return NULL;

  if ((cache_entry = fn_tmpl_is_cached (M_NAME(messages[idx]))) == NULL) {
    if ((template = cache_template (c99name)) == NULL) {
      _warning ("Unregistered function template in clib_fn_rt_expr ().\n");
      return NULL;
    }
    if ((preprocessed_template = preprocess_template (template)) == NULL) {
      error (messages[idx], "Error preprocessing template for %s.",
	     template -> name);
    } else {

      strcpy (template_info_method.name, M_NAME(messages[idx]));

      /*
       *  Fill in the template_info params.  Again, this is 
       *  similar to the code below, except that we create our
       *  own function object - it does not come from eval_arg ().
       */
      template_params (preprocessed_template, template -> tmpl_fn_name);
      if ((param_start_idx = nextlangmsg (messages, idx)) == ERROR)
	error (messages[idx], "Parser error.");
      if (M_TOK(messages[param_start_idx]) != OPENPAREN)
	error (messages[param_start_idx], "Parser error.");
      if ((param_end_idx = match_paren (messages, param_start_idx, 
					get_stack_top (messages))) == ERROR)
	error (messages[param_start_idx], "Parser error.");
      toks2str (messages, idx, param_end_idx, exprbuf);
      fn_object = create_object (CFUNCTION_CLASSNAME, exprbuf);
      strcpy (fn_object -> __o_superclassname, CFUNCTION_SUPERCLASSNAME);
      fn_object -> __o_superclass = 
	fn_object -> __o_class -> __o_superclass;
      __ctalkSetObjectValue (fn_object, exprbuf);
      c_tmpl_fn_args (messages, fn_object, idx);
      c_template = insert_c_methd_return_type (preprocessed_template, 
					       template -> tmpl_fn_name);
      __xfree (MEMADDR(preprocessed_template));

      fmt_template_rt_expr (template, exprbuf);

      generate_fn_template_init (template -> tmpl_fn_name, "Object");
      cache_fn_tmpl_name (template_info_method.name, template -> tmpl_fn_name);
      buffer_fn_template (c_template);

      strcpy (method_name, template -> tmpl_fn_name);

      delete_object (fn_object);
      __xfree (MEMADDR(c_template));
    }
  } else {
    sscanf (cache_entry, TEMPLATE_CACHE_FMT, fn_name, method_name);
  }

  strcatx (buf, " ", CFUNCTION_CLASSNAME, " ", method_name, " ", NULL);

  return buf;
}

/*
 *  Create a ctalk expression for a function template.  
 *  
 */

char *clib_fn_expr (MESSAGE_STACK messages, int idx, int fn_idx, 
		    OBJECT *fn_obj) {
  char *c99name,
    *preprocessed_template,
    *c_template,
    *cache_entry;
  static char exprbuf[MAXMSG];
  char fn_name[MAXLABEL], method_name[MAXLABEL];
  FN_TMPL *template;
  CVAR *c;
  CFUNC *fn;
  int n;

  if ((c99name = template_name (messages[fn_idx] -> name)) == NULL) {
    if (((c = get_global_var (M_NAME(messages[fn_idx]))) == NULL) &&
	((fn = get_function (M_NAME(messages[fn_idx]))) == NULL)) {
      warning (messages[fn_idx], "Undefined function %s.",
		M_NAME(messages[fn_idx]));
      return NULL;
    } else {
      return user_fn_template (messages, fn_idx);
    }
  }
  
  if ((template = get_template (c99name)) == NULL) {
    if ((template = cache_template (c99name)) == NULL) {
      warning (messages[fn_idx], "Template for function %s not found.\n",
	       c99name);
      return NULL;
    }
  }

  if ((cache_entry = fn_tmpl_is_cached (M_NAME(messages[fn_idx]))) == NULL) {
    if ((preprocessed_template = preprocess_template (template)) != NULL) {

    strcpy (template_info_method.name, M_NAME(messages[fn_idx]));

    /*
     *  Fills in template_info parameters.  Does a lot
     *  of other things also which seem to be unnecessary
     *  now, so check if we can pare it down.
     *
     *  Once we have the params, then get the args.
     *  We do all this because we might need to replace
     *  the C args with objects.
     */
    template_params (preprocessed_template, template -> tmpl_fn_name);
    c_tmpl_fn_args (messages, fn_obj, fn_idx);
    c_template = insert_c_methd_return_type (preprocessed_template, 
				template -> tmpl_fn_name);
    fmt_template_rt_expr (template, exprbuf);
    /*
     * "Object" is a placeholder for the actual class from
     * template_return_class ().
     *  Also don't forget add_method_return ()
     */
    generate_fn_template_init (template -> tmpl_fn_name, "Object");
    cache_fn_tmpl_name (template_info_method.name, template -> tmpl_fn_name);
    buffer_fn_template (c_template);

    __xfree (MEMADDR(preprocessed_template));
    __xfree (MEMADDR(c_template));
    } /* if ((preprocessed_template = preprocess_template (template))... */

  } else {
    sscanf (cache_entry, TEMPLATE_CACHE_FMT, fn_name, method_name);
    template_params (template->def, template -> tmpl_fn_name);
    c_tmpl_fn_args (messages, fn_obj, fn_idx);
    fmt_template_rt_expr (template, exprbuf);
  }

  *template_info_method.name = 0;
  for (n = 0; n < template_info_method.n_params; n++) {
    __xfree (MEMADDR(template_info_method.params[n]));
  }
  template_info_method.n_params = 0;
  for (n = 0; n < template_info_method.n_args; n++) {
    if ((!get_object (template_info_method.args[n]->obj->__o_name, 
		      template_info_method.args[n]->obj->__o_classname)) &&
	!saved_method_object (template_info_method.args[n]->obj)) {
      delete_object (template_info_method.args[n] -> obj);
    }
    delete_arg (template_info_method.args[n]);
    template_info_method.args[n] = NULL;
  }
  template_info_method.n_args = 0;

  return exprbuf;
}

/*
 *  Generate the __ctalk_init () calls for Expr and CFunction
 *  classes if necessary.
 */
void init_c_fn_class (void) {
  OBJECT *class_obj;
  static char buf[MAXMSG];
  char addr_buf[MAXMSG];

  if (!expr_class_defined) {
    class_obj = create_object ("Class", EXPR_CLASSNAME);
    strcpy (class_obj -> __o_superclassname, EXPR_SUPERCLASSNAME);
    class_obj -> __o_superclass = get_class_object (EXPR_SUPERCLASSNAME);
#ifdef __x86_64
    htoa (addr_buf, (unsigned long long int)class_obj);
#else    
    htoa (addr_buf, (unsigned int)class_obj);
#endif    
    __ctalkSetObjectValue (class_obj, addr_buf); 
    add_class_object (class_obj);
    Expression_class_object = class_obj;
    strcatx (buf, "__ctalk_arg (\"", EXPR_SUPERCLASSNAME, 
	     "\", \"class\", 1, \"", EXPR_CLASSNAME, "\");\n", NULL);
    global_init_statement (buf, TEMPLATE_CLASS_INIT_PRIORITY);
    strcatx (buf, "__ctalk_primitive_method (\"",
	     EXPR_SUPERCLASSNAME, "\", \"class\", 0);\n", NULL);
    global_init_statement (buf, TEMPLATE_CLASS_INIT_PRIORITY);
    expr_class_defined = True;
  }

  if (!cfunction_class_defined) {
    class_obj = create_object ("Class", CFUNCTION_CLASSNAME);
    strcpy (class_obj -> __o_superclassname, CFUNCTION_SUPERCLASSNAME);
    class_obj -> __o_superclass = get_class_object(CFUNCTION_SUPERCLASSNAME);
#ifdef __x86_64
    htoa (addr_buf, (unsigned long long int)class_obj);
#else    
    htoa (addr_buf, (unsigned int)class_obj);
#endif    
    __ctalkSetObjectValue (class_obj, addr_buf);
    add_class_object (class_obj);
    CFunction_class_object = class_obj;
    strcatx (buf, "__ctalk_arg (\"", CFUNCTION_SUPERCLASSNAME,
	     "\", \"class\", 1, \"", CFUNCTION_CLASSNAME,
	     "\");\n", NULL);
    global_init_statement (buf, TEMPLATE_CLASS_INIT_PRIORITY);
    strcatx (buf, "__ctalk_primitive_method (\"", CFUNCTION_SUPERCLASSNAME,
	     "\", \"class\", 0);\n", NULL);
    global_init_statement (buf, TEMPLATE_CLASS_INIT_PRIORITY);
    cfunction_class_defined = True;
  }
  if (!template_info_method_initialized) {
    template_info_method.rcvr_class_obj = CFunction_class_object;
    strcpy (template_info_method.returnclass, "Any");
    template_info_method_initialized = True;
  }
}

void generate_fn_template_init (char *template_name, char *return_class) {
  static char buf[MAXMSG];
  sprintf 
    (buf, 
     "__ctalkDefineClassMethod (\"CFunction\", \"%s\", (OBJECT *(*)())%s, %d);\n",
     template_name, template_name, template_info_method.n_params);
  global_init_statement (buf, TEMPLATE_INIT_PRIORITY);
  strcatx (buf,
	   "__ctalkClassMethodInitReturnClass (\"CFunction\", \"", 
	   template_name, 
	   "\", \"", (return_class ? return_class : "Any"),
	   "\", -1);\n", NULL);
  global_init_statement (buf, TEMPLATE_INIT_PRIORITY);
  if (interpreter_pass == method_pass) {
    add_template_to_method_init (template_name);
  }
}

/* Called by main (). */
void init_fn_templates (void) {
  pending_templates = templates_ptr = NULL;
  tmpl_messageptr = N_MESSAGES;
  /* The rest of  the method initialization can be done as soon
     as we've created CFunction class, below. ... */
  memset ((void *)&template_info_method, 
	  0, sizeof (struct _method));
}

int buffer_fn_template (char *template_buf) {

  LIST *l;

  l = new_list ();
  l -> data = (void *) strdup (template_buf);

  if (!templates_ptr) {
    pending_templates = templates_ptr = l;
  } else {
    list_add (pending_templates, l);
    templates_ptr = l;
  }

  return SUCCESS;
}

int unbuffer_fn_templates (void) {

  LIST *l;

  if (!templates_ptr) return SUCCESS;

  /* 
     Don't output the template with a cached 
     method.
  */
  if (interpreter_pass == method_pass)
    return SUCCESS;

  while (pending_templates) {
    l = list_unshift (&pending_templates);
    __fileout ((char *) l -> data);
    __fileout ("\n");
    delete_list_element (l);
    templates_ptr = pending_templates;
  }

  return SUCCESS;
}

char *template_return_class (char *template) {
  return NULL;
}

static void __set_arg_message_name_tmpl (MESSAGE *m, char *s) {
  __xfree (MEMADDR(m -> name));
  m -> name = __xstrdup (s);
}

/* this is like method_arg_accessor_fn in method.c, except it does
   not create a temporary object */
static int tmpl_method_arg_accessor_fn (MESSAGE_STACK messages, int idx,
				   int arg_n) {

  char expr_buf[MAXMSG], expr_tmp[MAXMSG];
  if (fmt_arg_type (messages, idx, stack_start (messages)) ==
	fmt_arg_char_ptr) {
    strcatx (expr_buf, STRING_TRANS_FN, "(",
	     format_method_arg_accessor (arg_n, M_NAME(messages[idx]),
					 false, expr_tmp),
	     "1,)", NULL);
    __set_arg_message_name_tmpl (messages[idx], expr_buf);
  } else {
    __set_arg_message_name_tmpl (messages[idx], 
			    format_method_arg_accessor 
				 (arg_n, M_NAME(messages[idx]), false,
				  expr_tmp));
  }

  return SUCCESS;
}

/*
 *  Replacing the template parameters is also easier before
 *  preprocessing.
 */

char *template_params (char *template, char *template_name) {

  int i, j,
    stack_end,
    param_start_ptr,
    param_end_ptr,
    method_name_ptr,
    n_params;
  PARAMLIST_ENTRY *params[MAXARGS];
  static char f_tmpl[MAXMSG];
  MESSAGE *m;
  bool is_param;

  stack_end = tokenize (tmpl_message_push, template);

  for (method_name_ptr = N_MESSAGES; 
       method_name_ptr >= stack_end; 
       method_name_ptr--)
    if (!strcmp (template_name, tmpl_messages[method_name_ptr] -> name))
      break;

  if ((param_start_ptr = scanforward (tmpl_messages, method_name_ptr,
				      stack_end, OPENPAREN)) == ERROR)
    _error ("template_params: parser error.\n");

  if ((param_end_ptr = match_paren (tmpl_messages, param_start_ptr,
				    stack_end)) == ERROR)
    _error ("template_params: parser error.\n");

  fn_param_declarations (tmpl_messages, param_start_ptr, param_end_ptr,
			       params, &n_params);
  template_info_method.n_params = n_params;
  template_info_method.varargs = fn_decl_varargs ();

  for (i = n_params - 1; i >= 0; i--) {
    template_info_method.params[i] = 
      method_param_tok (tmpl_messages, params[i]);
    while(__ctalkPendingException ()) {
      /*
       *  Handle with run-time exception functions, because the
       *  error locations are from a different stack if they
       *  originated in method_param_* () or previous.  Note that
       *  exceptions here, due to C types and typedefs that don't
       *  map directly to classes, are no-ops at the present.
       */
      __ctalkHandleRunTimeException ();
    }
    if (! param_class_exists (template_info_method.params[i] -> class))
      library_search (template_info_method.params[i] -> class, TRUE);
    __xfree (MEMADDR(params[i]));
  }

  for (i = N_MESSAGES, *f_tmpl = 0, is_param = False; i >= stack_end; i--) {
    /*
     *  Elide the C arguments.
     */
    if (i == param_start_ptr) {
      strcatx2 (f_tmpl, METHOD_C_PARAM, NULL);
      i = param_end_ptr;
    } else {
      /*
       *  Replace params with library calls.
       */
      m = tmpl_messages[i];
      if (m -> tokentype == LABEL) {
	for (j = 0; j < template_info_method.n_params; j++) {
	  if (!strcmp (m -> name, template_info_method.params[j] -> name)) {
	    tmpl_method_arg_accessor_fn (tmpl_messages, i, 
				    template_info_method.n_params - 1 - j);
	    strcatx2 (f_tmpl, m -> name, NULL);
	    is_param = True;
	  }
	}
	if (m -> attrs & TOK_SELF) {
	  strcatx2 (f_tmpl, "__ctalk_self_internal ()", NULL);
	} else {
	  if (!is_param)
	    strcatx2 (f_tmpl, m -> name, NULL);
	  else
	    is_param = False;
	}
      } else {  /* if (m -> tokentype == LABEL) */
	strcatx2 (f_tmpl, m -> name, NULL);
      }
    }
  }

  for (i = stack_end; i <= N_MESSAGES; i++)
    reuse_message (tmpl_message_pop ());

  return f_tmpl;
}

ARGSTR tmpl_argstrs[MAXARGS];
int tmpl_argstrptr;

int c_tmpl_fn_args (MESSAGE_STACK messages, OBJECT *arg_object, 
		    int fn_ptr) {

  int arglist_start_ptr,
    arglist_end_ptr,
    arg_start_ptr,
    arg_end_ptr,
    i,
    n_parens;
  char arg_buf[MAXMSG];
  OBJECT *fn_arg_object;
  MESSAGE *m_tok;
  ERROR_LOCATION error_loc;

  if (strncmp (arg_object -> __o_name, messages[fn_ptr] -> name,
	       strlen (messages[fn_ptr] -> name)))
    _error ("c_tmpl_fn_args: Parser error.\n");

  if ((arglist_start_ptr = nextlangmsg (messages, fn_ptr)) 
      == ERROR)
    _error ("c_tmpl_fn_args: Parser error.\n");

  if ((arglist_end_ptr = match_paren (messages, arglist_start_ptr,
				   get_stack_top (messages))) == ERROR)
    _error ("c_tmpl_fn_args: Parser error.\n");

  if ((messages[arglist_start_ptr] -> tokentype != OPENPAREN) ||
      (messages[arglist_end_ptr] -> tokentype != CLOSEPAREN))
    _error ("c_tmpl_fn_args: Parser error.\n");

  for (i = arglist_start_ptr, *arg_buf = 0, n_parens = 0, tmpl_argstrptr = 0,
	 arg_start_ptr = -1, arg_end_ptr = -1; 
       i >= arglist_end_ptr; i--) {

    m_tok = messages[i];

    switch (m_tok -> tokentype)
      {
      case OPENPAREN:
	if (++n_parens > 1) {
	  if (arg_start_ptr == -1) arg_start_ptr = i;
	  strcatx2 (arg_buf, m_tok -> name, NULL);
	}
	break;
      case CLOSEPAREN:
	switch (--n_parens)
	  {
	  case 0:
	    arg_end_ptr = prevlangmsg (messages, i);
	    if (arg_start_ptr != -1) {
	      tmpl_argstrs[tmpl_argstrptr].start_idx = arg_start_ptr;
	      tmpl_argstrs[tmpl_argstrptr].end_idx = arg_end_ptr;
	      tmpl_argstrs[tmpl_argstrptr].arg = strdup (arg_buf);
	      tmpl_argstrs[tmpl_argstrptr].m_s = messages;
	      arg_start_ptr = arg_end_ptr = -1;
	      *arg_buf = 0;
	      ++tmpl_argstrptr;
	    }
	    break;
	  default:
	    strcatx2 (arg_buf, m_tok -> name, NULL);
	    break;
	  }
	break;
      case ARGSEPARATOR:
	arg_end_ptr = prevlangmsg (messages, i);
	tmpl_argstrs[tmpl_argstrptr].start_idx = arg_start_ptr;
	tmpl_argstrs[tmpl_argstrptr].end_idx = arg_end_ptr;
	tmpl_argstrs[tmpl_argstrptr].arg = strdup (arg_buf);
	tmpl_argstrs[tmpl_argstrptr].m_s = messages;
	arg_start_ptr = arg_end_ptr = -1;
	*arg_buf = 0;
	++tmpl_argstrptr;
	break;
      default:
	if (!(M_TOK(m_tok) == WHITESPACE && arg_start_ptr == -1)) {
	  /* don't save leading whitespace */ /***/
	  strcatx2 (arg_buf, m_tok -> name, NULL);
	  if (arg_start_ptr == -1)
	    arg_start_ptr = i;
	}
      }
  }

  template_info_method.n_args = 0; /***/

  if (!template_info_method.varargs &&
      (tmpl_argstrptr != template_info_method.n_params)) {
    warning (messages[i], "Function call, \"%s:\" argument mismatch.\n"
	     "Template prototype defines %d parameter(s).  Expression "
	     "provides %d argument(s).",
	     template_info_method.name,
	     template_info_method.n_params,
	     tmpl_argstrptr);
  }

  arg_error_loc (CURRENT_PARSER -> level,
		 messages[fn_ptr] -> error_line,
		 messages[fn_ptr] -> error_column,
		 &error_loc);

  for (i = 0; i < tmpl_argstrptr; i++) {
    fn_tmpl_eval_arg = true;
    fn_arg_object = eval_arg (&template_info_method, 
			      CFunction_class_object, &tmpl_argstrs[i], 
			      fn_ptr);
    fn_tmpl_eval_arg = false;
    __xfree (MEMADDR(tmpl_argstrs[i].arg));
      template_info_method.args[tmpl_argstrptr - 1 - i] = 
	  create_arg_init (fn_arg_object);
    template_info_method.n_args++;
  }

  return SUCCESS;
}

/*
 *  The template method name is the first label in the template.
 *  This is much easier to find before preprocessing.
 */

char *find_tmpmethodname (char *template) {

  static char name[MAXLABEL];
  int i, stack_end_ptr;
  MESSAGE *m;

  memset ((void *)name, 0, MAXLABEL);

  stack_end_ptr = tokenize (tmpl_message_push, template);

  for (i = N_MESSAGES; i >= stack_end_ptr; i--) {

    m = tmpl_messages[i];

    if (M_ISSPACE (m)) continue;

    switch (m -> tokentype)
      {
      case LABEL:
	strcpy (name, m -> name);
	goto name_found;
	break;
      default:
	break;
      }
  }

 name_found:
  for (i = stack_end_ptr; i <= N_MESSAGES; i++) 
    reuse_message (tmpl_message_pop ());

  return name;
}

int add_method_return (char *template, char *buf) {

  int i, stack_end_ptr;
  bool have_name;
  MESSAGE *m;

  stack_end_ptr = tokenize (tmpl_message_push, template);

  for (i = N_MESSAGES, have_name = False; i >= stack_end_ptr; i--) {

    m = tmpl_messages[i];

    switch (m -> tokentype)
      {
      case LABEL:
	if (have_name == False) {
	  strcatx2 (buf, METHOD_RETURN, NULL);
	  have_name = True;
	}
	break;
      default:
	break;
      }
    strcatx2 (buf, m -> name, NULL);
  }

  for (i = stack_end_ptr; i <= N_MESSAGES; i++) 
    reuse_message (tmpl_message_pop ());

  return SUCCESS;
}

/*
 *  Call ctpp to preprocess the template file.  Do not include
 *  line numbers in ctpp's output.  
 *  
 *  NOTE: In the case of include 
 *  files in the template that are also included in the input
 *  file, there should not be an issue with redefining symbols,
 *  like typedefs.
 */

char *preprocess_template (FN_TMPL *template) {

  char cmd[MAXMSG];
  char ctpp_path[FILENAME_MAX];
  char ctpp_ofn[FILENAME_MAX];
  char argbuf[MAXMSG],
    cachebuf[MAXMSG],
    oldcache[FILENAME_MAX];
  static char *outbuf;
  int retval;
  FILE *P;

  strcpy (ctpp_path, which (CTPP_BIN));
  strcpy (ctpp_ofn, ctpp_name (template -> name, TRUE));

  create_tmp ();
  write_tmp (template -> def);
  close_tmp ();

  if (macro_cache_path ()) {
    strcpy (oldcache, macro_cache_path ());
    if (file_exists (oldcache)) {
      strcatx (cachebuf, "-imacros ", macro_cache_path (), " ", NULL);
      strcpy (argbuf, cachebuf);
    }
  } else {
    *argbuf = 0;
    *oldcache = 0;
  }

  strcatx (cachebuf, "-dF ", new_macro_cache_path (), " ", NULL);
  strcatx2 (argbuf, cachebuf, NULL);

  strcatx (cmd, ctpp_path, " -P ", argbuf, " ", get_tmpname (), " ",
	   ctpp_ofn, NULL);

  if ((P = popen (cmd, "w")) == NULL) {
    printf ("%s: %s.\n", ctpp_ofn, strerror (errno));
    return NULL;
  }

  if ((retval = pclose (P)) == ERROR) {
    cleanup_temp_files (TRUE);
    exit (retval);
  }

  if (*oldcache && file_exists (oldcache)) unlink (oldcache);

  unlink_tmp ();

  if (!file_exists (ctpp_ofn))
    return NULL;

  p_read_file (&outbuf, ctpp_ofn);

  unlink (ctpp_ofn);

  return outbuf;

}

static FN_TMPL *template_cache = NULL;
static FN_TMPL *template_cache_ptr;

/*
 *  This might be able to replace init_fn_templates (), and
 *  related functions.
 */

void init_fn_template_cache (void) {
  template_cache = template_cache_ptr = NULL;
  tmpl_messageptr = N_MESSAGES;
}

FN_TMPL *new_fn_template (void) {

  FN_TMPL *f;

  if ((f = (struct _fn_tmpl *) __xalloc (sizeof (struct _fn_tmpl))) == NULL)
    _error ("new_template (): %s.\n", strerror (errno));

  f -> sig = FN_TMPL_SIG;

  return f;

}

FN_TMPL *get_template (char *c99name) {

  FN_TMPL *f;

  for (f = template_cache; f; f = f -> next) 
    if (!strcmp (c99name, f -> name))
      return f;

  return NULL;
}

/*
 *  After preprocessing, the output looks like set of 
 *  #define statement lines, and this function parses
 *  them into FN_TMPL structures and caches them.
 */

FN_TMPL *cache_template (char *c99name) {

  char *filebuf;
  char *linebuf;
  char namebuf[MAXLABEL+1], methodnamebuf[MAXLABEL];
  char *d_ptr, *n_ptr,
    *s1_ptr, *s2_ptr, *s3_ptr,
    *nl_ptr;
  int r;
  FN_TMPL *fn_tmpl,
    *this_tmpl;
  struct stat statbuf;

  /*
   *  Fills in ctpp_ofn_template.
   */
  if (preprocess_template_file (c99name))
    return NULL;

  this_tmpl = NULL;

  stat (ctpp_ofn_template, &statbuf);

  if ((linebuf = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
    _error ("cache_template (): %s.\n", strerror (errno));

  if ((filebuf = (char  *)__xalloc (statbuf.st_size + 1)) == NULL)
    _error ("cache_template (): %s.\n", strerror (errno));

  if ((r = read_file (filebuf, ctpp_ofn_template)) == 0)
    _error ("cache_template (): %s.\n", strerror (errno));


  d_ptr = n_ptr = filebuf;
  while (1) {

    if ((n_ptr = index (d_ptr, '\n')) != NULL) {
      memset ((void *)linebuf, 0, statbuf.st_size + 1);
      strncpy (linebuf, d_ptr, n_ptr - d_ptr);

      /* Look for a basic function form before continuing.  This
	 lets us continue with template defines only, 
	 not preprocessor macros. */
      if (!strchr (linebuf, '{') && !strchr (linebuf, '}')) {
	/* Same as at the bottom. */
	if ((d_ptr = index (n_ptr, '#')) == NULL)
	  break;
	else
	  continue;
      }

      /*
       *  Line splices in the template definition are replaced
       *  with a space character during preprocessing.
       */
      while ((nl_ptr = strstr (linebuf, "\\n ")) != NULL) {
	*nl_ptr++ = '\n'; 
	if (*nl_ptr) *nl_ptr++ = ' ';
	if (*nl_ptr) *nl_ptr++ = ' ';
      }

      fn_tmpl = new_fn_template ();
      s1_ptr = index (linebuf, ' ');
      s2_ptr = index (s1_ptr + 1, ' ');

      memset ((void *)namebuf, 0, MAXLABEL);
      substrcpy (namebuf, s1_ptr + 1, 0, s2_ptr - (s1_ptr + 1));

      if (!template_name (namebuf)) {
	/* Make sure the #define is an actual template. */
	__xfree (MEMADDR(fn_tmpl));
	if ((d_ptr = index (n_ptr, '#')) == NULL)
	  break;
	else
	  continue;
      }

      for (; isspace((int)*s2_ptr); ++s2_ptr)
	;
      s3_ptr = index (s2_ptr, ' ');
      substrcpy (methodnamebuf, s2_ptr, 0, s3_ptr - s2_ptr);

      strcpy (fn_tmpl -> name, namebuf);
      strcpy (fn_tmpl -> tmpl_fn_name, methodnamebuf);
      fn_tmpl -> def = strdup (s2_ptr);

      if (!strcmp (c99name, fn_tmpl -> name))
	this_tmpl = fn_tmpl;

      if (!template_cache_ptr) {
	template_cache = template_cache_ptr = fn_tmpl;
      } else {
	template_cache_ptr -> next = fn_tmpl;
	fn_tmpl -> prev = template_cache_ptr;
	template_cache_ptr = fn_tmpl;
      }

      if (print_templates_opt) {
	printf ("%s =\n%s\n\n",
		fn_tmpl -> name,
		fn_tmpl -> def);
      }


    }

    if ((d_ptr = index (n_ptr, '#')) == NULL)
      break;
  }


  __xfree (MEMADDR(linebuf));
  __xfree (MEMADDR(filebuf));

  unlink (ctpp_ofn_template);

  return this_tmpl;
}

/*
 *  Use the preprocessor to split the template file into #define
 *  statements.
 */

int preprocess_template_file (char *c99name) {

  char template_fn[FILENAME_MAX];
  char incbuf[FILENAME_MAX];
  char allincbuf[MAXMSG];
  char cmd[MAXMSG];
  char cmdbuf[2];
  int inc_idx;
  int retval;
  FILE *P;
  struct stat statbuf;

  if (get_clib_template_fn (c99name, template_fn) == NULL)
    return ERROR;

  strcpy (ctpp_ofn_template, ctpp_name (template_fn, TRUE));

  *allincbuf = 0;
  for (inc_idx = 0; library_include_paths[inc_idx]; inc_idx++) {
    strcatx (incbuf, " -I ", library_include_paths[inc_idx], NULL);
    strcatx2 (allincbuf, incbuf, NULL);
  }

  strcatx (cmd, which (CTPP_BIN), allincbuf, " -undef -dM ", 
	   template_fn, " > ", ctpp_ofn_template, NULL);

  if ((P = popen (cmd, "r")) != NULL) {
    /*
     *  Needed for Solaris fprintf output.
     */
#if defined (__sparc__) && defined (__GNUC__)
    while (fread (cmdbuf, sizeof (int), 1, P))
      printf ("%s", cmdbuf);
#else
    while (fread (cmdbuf, sizeof (char), 1, P))
      printf ("%s", cmdbuf);
#endif
  } else {
    printf ("%s: %s.\n", template_fn, strerror (errno));
    return ERROR;
  }

  if ((retval = pclose (P)) == ERROR) {
    cleanup_temp_files (TRUE);
    exit (retval);
  }

  /*
   *  If the preprocessor encounters a fatal error, it
   *  will not produce an output file.
   */
  if (!stat (ctpp_ofn_template, &statbuf))
    return SUCCESS;
  else
    return ERROR;
}

/*
 *  Turn the template into a function declaration.
 *  The only thing it does now is insert the 
 *  OBJECT * method return type into the template.
 *
 *  TO DO - There should be a little more analysis
 *  of the template declaration, in case someone
 *  includes the return type in the template.
 */

char *insert_c_methd_return_type (char *buf, char *fn_name) {

  char *fn_name_ptr, *newbuf;
  int defsize;

  if ((fn_name_ptr = strstr (buf, fn_name)) == NULL) 
    return NULL;
  
  defsize = strlen (buf);

  if ((newbuf = (char *)__xalloc (defsize + METHOD_RETURN_LENGTH + 1))
      == NULL)
    _error ("c_tmpl_fn_decl (): %s.\n", strerror (errno));
  
  strncpy (newbuf, buf, fn_name_ptr - buf);
  strcatx2 (newbuf, METHOD_RETURN, fn_name_ptr, NULL);

  return newbuf;
  
}

void cache_fn_tmpl_name (char *fn_name, char *method_name) {
  LIST *l;
  char buf[MAXMSG];
  l = new_list ();
  sprintf (buf, TEMPLATE_CACHE_FMT, fn_name, method_name);
  l -> data = (void *) strdup (buf);
  if (!cached_fn_ptr) {
    cached_fns = cached_fn_ptr = l;
  } else {
    list_add (cached_fns, l);
    cached_fn_ptr = l;
  }
}

char *fn_tmpl_is_cached (char *fn_name) {

  LIST *l;

  for (l = cached_fns; l ; l = l -> next) {
    if (!strncmp (fn_name, (char *)l -> data, strlen (fn_name)))
      return (char *)l -> data;
  }

  return NULL;
}

char *fmt_template_rt_expr (FN_TMPL *template, char *exprbuf) {

  char tmpargbuf[MAXMSG], argbuf[MAXMSG];
  int i;

  if (template_info_method.n_params) {
    strcpy (argbuf, "(");

    for (i = 0; i < template_info_method.n_args; i++) {
      strcpy (tmpargbuf, 
	      template_info_method.args[template_info_method.n_args-i-1] ->
	      obj -> __o_name);
      if (i != (template_info_method.n_args - 1)) {
	strcatx2 (tmpargbuf, ", ", NULL);
      }
      strcatx2 (argbuf, tmpargbuf, NULL);
    }
    strcatx2 (argbuf, ")", NULL);
    strcatx (exprbuf, CFUNCTION_CLASSNAME, " ", template -> tmpl_fn_name,
	     " ", argbuf, NULL);
  } else {/* if (template_info_method.n_params) { */
    strcatx (exprbuf, CFUNCTION_CLASSNAME, " ",
	     template -> tmpl_fn_name, NULL);
  }
  return exprbuf;
}

char *fn_param_prototypes_from_cfunc (CFUNC *fn, char *buf) {

  CVAR *_param = NULL;

  if (fn -> params == NULL) {
    strcpy (buf, "void");
  } else {
    for (_param = fn -> params, *buf = 0; _param; _param = _param -> next) {
      declaration_expr_from_cvar (_param, buf);
      if (_param -> next) {
	strcatx2 (buf, ", ", NULL);
      }
    }
  }
  return buf;
}

int find_fn_arg_parens (MESSAGE_STACK messages, int fn_idx, 
			int *arg_paren_start_idx, 
			int *arg_paren_end_idx) {

  if ((*arg_paren_start_idx = nextlangmsg (messages, fn_idx)) == ERROR)
    error (messages[fn_idx], "find_fn_arg_parens: Parser error.");

  if ((*arg_paren_end_idx = match_paren (messages, *arg_paren_start_idx, 
					get_stack_top (messages))) == ERROR)
    error (messages[fn_idx], "find_fn_arg_parens: Parser error.");

  return SUCCESS;
}

static void __delete_fn_tmpl_elements (FN_TMPL *__f) {
  __xfree (MEMADDR(__f -> def));
}

void cleanup_template_cache (void) {
  FN_TMPL *prev;
  if (!template_cache)
    return;
  while (template_cache_ptr != template_cache) {
    prev = template_cache_ptr -> prev;
    __delete_fn_tmpl_elements (template_cache_ptr);
    template_cache_ptr = prev;
    __xfree (MEMADDR(template_cache_ptr -> next));
  }
  __delete_fn_tmpl_elements (template_cache);
  __xfree (MEMADDR(template_cache));
}

/*
 *  Syntactic sugar.  Handles expressions like:
 *
 *   CFunction <template_name> ([<args>])
 *
 *  Called from resolve ().  We've already checked that
 *  template_msg_idx points to a token that names a template.
 */
void template_call_from_CFunction_receiver (MESSAGE_STACK messages, 
					    int template_msg_idx) {
  FN_TMPL *template;
  MESSAGE *m_template = messages[template_msg_idx];
  char *preprocessed_template, *c_template, exprbuf[MAXMSG], exprbuf2[MAXMSG];
  int param_start_idx, param_end_idx;
  OBJECT *fn_object;
  int i, prev_idx, after_last_idx, n;

  if ((template = get_template (M_NAME(m_template))) == NULL) {
    if ((template = cache_template (M_NAME(m_template))) == NULL) {
      warning (m_template, "Template for function %s not found.\n",
	       M_NAME(m_template));
      return;
    }
    if ((preprocessed_template = preprocess_template (template)) == NULL) {
      error (m_template, "Error preprocessing template for %s.",
	     template -> name);
    } else {

      strcpy (template_info_method.name, M_NAME(m_template));

      /* Fills in template_info_method. */
      template_params (preprocessed_template, template -> tmpl_fn_name);

      if ((param_start_idx = nextlangmsg (messages, template_msg_idx)) == ERROR)
	error (messages[template_msg_idx], "Parser error.");
      if (M_TOK(messages[param_start_idx]) != OPENPAREN)
	error (messages[param_start_idx], "Parser error. "
	       "(Enclose the template parameters in parentheses.)");
      if ((param_end_idx = match_paren (messages, param_start_idx, 
					get_stack_top (messages))) == ERROR)
	error (messages[param_start_idx], "Parser error.");
      toks2str (messages, template_msg_idx, param_end_idx, exprbuf);
      fn_object = create_object (CFUNCTION_CLASSNAME, exprbuf);
      fn_object -> __o_superclass = 
	get_class_object(CFUNCTION_SUPERCLASSNAME);
      __ctalkSetObjectValue (fn_object, exprbuf);
      c_tmpl_fn_args (messages, fn_object, template_msg_idx);
      c_template = insert_c_methd_return_type (preprocessed_template, 
					       template -> tmpl_fn_name);
      __xfree (MEMADDR(preprocessed_template));


      generate_fn_template_init (template -> tmpl_fn_name, "Object");
      cache_fn_tmpl_name (template_info_method.name, template -> tmpl_fn_name);
      buffer_fn_template (c_template);
      fmt_template_rt_expr (template, exprbuf);
      fmt_eval_expr_str (exprbuf, exprbuf2);

      fileout (exprbuf2, 0, template_msg_idx);

      prev_idx = prevlangmsg (messages, template_msg_idx);
      after_last_idx = nextlangmsg (messages, param_end_idx);
      if (M_TOK(messages[after_last_idx]) != SEMICOLON) {
	warning (messages[after_last_idx], 
		 "Extra tokens after method \"%s\" args are not (yet) supported.",
		 M_NAME(messages[template_msg_idx]));
      }
      for (i = prev_idx; i >= param_end_idx; --i) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }

    } /* if ((preprocessed_template ... */

  } else {

    template = cache_template (M_NAME(m_template));

    if ((preprocessed_template = preprocess_template (template)) == NULL) {
      error (m_template, "Error preprocessing template for %s.",
	     template -> name);
    }

    strcpy (template_info_method.name, M_NAME(m_template));

    /* Fills in template_info_method. */
    template_params (preprocessed_template, template -> tmpl_fn_name);

    if ((param_start_idx = nextlangmsg (messages, template_msg_idx)) == ERROR)
      error (messages[template_msg_idx], "Parser error.");
    if (M_TOK(messages[param_start_idx]) != OPENPAREN)
      error (messages[param_start_idx], "Parser error.");
    if ((param_end_idx = match_paren (messages, param_start_idx, 
				      get_stack_top (messages))) == ERROR)
      error (messages[param_start_idx], "Parser error.");
    toks2str (messages, template_msg_idx, param_end_idx, exprbuf);
    fn_object = create_object (CFUNCTION_CLASSNAME, exprbuf);
    c_tmpl_fn_args (messages, fn_object, template_msg_idx);
    fmt_template_rt_expr (template, exprbuf);
    fmt_eval_expr_str (exprbuf, exprbuf2);
    fileout (exprbuf2, 0, template_msg_idx);
    prev_idx = prevlangmsg (messages, template_msg_idx);
    after_last_idx = nextlangmsg (messages, param_end_idx);
    if (M_TOK(messages[after_last_idx]) != SEMICOLON) {
      warning (messages[after_last_idx], 
	       "Extra tokens after method \"%s\" args are not (yet) supported.",
	       M_NAME(messages[template_msg_idx]));
    }
    for (i = prev_idx; i >= param_end_idx; --i) {
      ++messages[i] -> evaled;
      ++messages[i] -> output;
    }

  }

  *template_info_method.name = 0;
  for (n = 0; n < template_info_method.n_params; n++) {
    __xfree (MEMADDR(template_info_method.params[n]));
  }
  template_info_method.n_params = 0;

  for (n = 0; n < template_info_method.n_args; n++) {
    if ((!get_object (template_info_method.args[n]->obj->__o_name, 
		      template_info_method.args[n]->obj->__o_classname)) &&
	!saved_method_object (template_info_method.args[n]->obj)) {
      /* Not saved in template_info_method yet. */
      delete_object (template_info_method.args[n]->obj);
      delete_arg (template_info_method.args[n]);
    }
    template_info_method.args[n] = NULL;
  }
  template_info_method.n_args = 0;

  return;
}

/* Output a template directly if a preloaded method uses it. */
void premethod_output_template (char *template_fn_name) {

  char fn_name[MAXLABEL], method_name[MAXLABEL];
  char *c99name, *cache_entry, *preprocessed_template, *c_template;
  char api_name[MAXLABEL];
  FN_TMPL *template;
  int i;

  /* template_fn_name is the name of the template's C function.  Derive
     the API name from it. */
  strcpy (api_name, &template_fn_name[1]);
  for (i = 0; api_name[i]; ++i)
    api_name[i] |= 32;

  if ((c99name = template_name (api_name)) == NULL)
    return;

  if ((cache_entry = fn_tmpl_is_cached (api_name)) == NULL) {
    if ((template = cache_template (c99name)) == NULL) {
      _warning ("Unregistered function premethod_output_template ().\n");
      return;
    }
    if ((preprocessed_template = preprocess_template (template)) == NULL) {
      _error ("Error preprocessing template for %s.\n", template -> name);
    } else {

      strcpy (template_info_method.name, api_name);

      template_params (preprocessed_template, template -> tmpl_fn_name);

      c_template = insert_c_methd_return_type (preprocessed_template, 
					       template -> tmpl_fn_name);
      __xfree (MEMADDR(preprocessed_template));

      generate_fn_template_init (template -> tmpl_fn_name, "Object");
      cache_fn_tmpl_name (template_info_method.name, template -> tmpl_fn_name);
      buffer_fn_template (c_template);

      strcpy (method_name, template -> tmpl_fn_name);

      __xfree (MEMADDR(c_template));
    }
  } else {
    sscanf (cache_entry, TEMPLATE_CACHE_FMT, fn_name, method_name);
  }
}
