/* $Id: object.c,v 1.4 2019/12/12 22:31:48 rkiesling Exp $ */

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
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

extern I_PASS interpreter_pass;

extern CVAR *global_cvars;      /* Declared in rt_cvar.c. */
extern CFUNC *functions;

extern OBJECT *classes;         /* Declared in class.c. */
extern OBJECT *last_class;

static OBJECT *dictionary = NULL;      /* Object dictionary for globals. */

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;
extern bool argblk;                      /* Declared in argblk.c.      */

extern int fn_arg_parser_level; /* Declared in cvars.c. */

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c.   */
extern int new_method_ptr;

extern DEFAULTCLASSCACHE *ct_defclasses; /* Declared in cclasses.c.       */

extern HASHTAB defined_instancevars; /* declared in primitives.c. */

extern int object_size; /* declared in lib/rtnewobj.c */

/* 
 *     Add an object to the dictionary or to the parser's local
 *     variable list.
 */

int add_object (OBJECT *o) {

  OBJECT *p, *q;
  PARSER *r;

  /*
   *  Everything but global variables goes into the parser's
   *  local variable list.
   */

  if (THIS_FRAME -> scope != GLOBAL_VAR) {

    r = CURRENT_PARSER;

    for (q = r -> methodobjects; q; q = q -> next) {
      if (q == o) {
	if (q == r -> methodobjects) {
	  r -> methodobjects = q -> next;
	  if (q -> next) q -> next -> prev = q -> prev;
	} else {
	  if (q -> next) q -> next -> prev = q -> prev;
	  if (q -> prev) q -> prev -> next = q -> next;
	}
	q -> next = q -> prev = NULL;
      }
    }

    if (!r -> vars) {
      r -> vars = o;
      return SUCCESS;
    }

    for (p = r -> vars; p -> next; p = p -> next)
      ;

    p -> next = o;
    o -> prev = p;
    o -> next = NULL;

  } else {
    if (!dictionary) {
      dictionary = o;
      return SUCCESS;
    }

    for (p = dictionary; p -> next; p = p -> next)
      ;

    p -> next = o;
    o -> prev = p;
    o -> next = NULL;

  }

  return SUCCESS;
}

/*
 *   Delete objects that are local to a parser scope.
 */

void delete_local_objects (void) {

  OBJECT *t, *last;
  PARSER *p;

  p = CURRENT_PARSER;    

  if (p -> vars) {
    for (last = p -> vars; last && last -> next; last = last -> next)
      ;

    while (last != p -> vars) {
      t = last -> prev;
      delete_object (last);
      if (!t || t == p -> vars)
	break;
      last = t;
    }
    delete_object (p -> vars);
    p -> vars = NULL;
  }

  if (p -> methodobjects) {
    for (last = p -> methodobjects; last && last -> next; last = last -> next)
      ;
    while (last != p -> methodobjects) {
      t = last -> prev;
      delete_object (last);
      if (!t || t == p -> methodobjects)
	break;
      last = t;
    }
    delete_object (p -> methodobjects);
    p -> methodobjects = NULL;
  }
}

/* 
 *   Delete global objects of a given scope.
 */

void delete_objects (int scope) {

  OBJECT *p;

  for (p = dictionary; p ; p = p -> next) {
    if (p -> scope == scope)
      delete_object (p);
  }
}

bool is_object_or_param (char *name, char *classname) {
  bool have_global = false;
  bool have_local = false;
  bool have_param = false;
  int n_th_param;
  METHOD *n_method;

  if (get_global_object (name, classname))
    have_global = true;

  if (get_local_object (name, classname))
    have_local = true;

  if (interpreter_pass == method_pass) {
    if ((n_method = new_methods[new_method_ptr + 1] -> method) != NULL) {
      for (n_th_param = 0; n_th_param < n_method -> n_params; n_th_param++) {
	if (n_method -> params[n_th_param] != NULL) {
	  if (str_eq (name, 
		      n_method -> params[n_th_param]->name))
	    have_param = true;
	}
      }
    }
  }

  if (have_local && have_param) {
    _warning ("Local object \"%s\" shadows a method parameter.\n", name);
  } else if (have_global && have_param) {
    _warning ("Global object \"%s\" shadows a method parameter.\n", name);
  }

  return have_local || have_global || have_param;

}

/*
 *  Retrieve a global object.  Get_object () calls this 
 *  function and get_local_object ().
 */

OBJECT *get_global_object (char *name, char *classname) {

  OBJECT *o;

  if (!dictionary)
    return NULL;

  for ( o = dictionary; o; o = o -> next) {
    if (classname) {
      if (!strcmp (o -> __o_name, name) &&
	  !strcmp (o -> __o_classname, classname))
	return o;
    } else {
      if (!strcmp (o -> __o_name, name))
	return o;
    }
  }

  return NULL;
}

/* 
 *  Retrieve a local object.  This is essentially the same
 *  as the code in get_object (), so it should be refactored
 *  in a some point.
 */

OBJECT *get_local_object (char *name, char *classname) {

  OBJECT *o;
  int i, local_frame_ptr;

  if (!HAVE_FRAMES)
    return NULL;

  /*
   *  If looking for C fn args, find the parser that 
   *  called c_fn_args ().
   */
  if (interpreter_pass == c_fn_pass) {
    for (i = parser_ptr (); i <= MAXARGS; i++) {
      if (parser_at (i) -> vars) {
	for ( o = parser_at (i) -> vars; o; o = o -> next) {
	  if (classname) {
	    if (!strcmp (o -> __o_name, name) &&
		!strcmp (o -> __o_classname, classname))
	      return o;
	  } else {
	    if (!strcmp (o -> __o_name, name))
	      return o;
	  }
	}
      }
    }
  }

  /*
   *  If checking an expression, use the variables of 
   *  the parent parser.
   */
  if (EXPR_CHECK) {
    i = parser_ptr () + 1;
    if (!parser_at (i) -> vars)
      return NULL;
    for ( o = parser_at (i) -> vars; ; o = o -> next) {
      if (classname) {
	if (!strcmp (o -> __o_name, name) &&
	    !strcmp (o -> __o_classname, classname))
	  return o;
      } else {
	if (!strcmp (o -> __o_name, name))
	  return o;
      }
      if (!o || !o -> next)
	break;
    }
  }

  if (((local_frame_ptr = get_frame_pointer ()) == MAXARGS + 1) || 
      ((local_frame_ptr = get_frame_pointer ()) == ERROR) ||
      (frame_at (local_frame_ptr) == NULL))
    return NULL;

  if (HAVE_FRAME && THIS_FRAME -> scope == LOCAL_VAR) {
    if (CURRENT_PARSER -> vars) {
      for ( o = CURRENT_PARSER -> vars; o; o = o -> next) {
	if (classname) {
	  if (!strcmp (o -> __o_name, name) &&
	      !strcmp (o -> __o_classname, classname))
	    return o;
	} else {
	  if (!strcmp (o -> __o_name, name))
	    return o;
	}
      }
    }
  }

  return NULL;
}

OBJECT *get_instance_variable (char *name, char *classname, int warn) {
  OBJECT *o,
    *class_object;

  if (classname) {
    if ((class_object = get_class_object (classname)) == NULL) {
      if (warn)
	__ctalkExceptionInternal (message_stack_at (get_messageptr ()), 
				  undefined_class_x, classname,0);
      return NULL;
    }

    for (o = class_object -> instancevars; o; o = o -> next) {
      if (!strcmp (o -> __o_name, name))
	return o;
    }

    if (class_object -> __o_superclass && 
	(class_object -> __o_superclass != class_object)) {
      if ((o = get_instance_variable (name, class_object -> __o_superclass -> __o_name, FALSE)) != NULL)
	return o;
    }

  } else {
    for (class_object = classes; class_object; 
	 class_object = class_object -> next) {
      for (o = class_object -> instancevars; o; o = o -> next) {
	if (!strcmp (o -> __o_name, name))
	  return o;
      }
    }
  }

  return NULL;
}

OBJECT *find_class_variable (MESSAGE *m_orig, char *name) {
  OBJECT *class_object, *class_var_object;
  OBJECT *results[MAXARGS] = {0,};
  int results_ptr = 0;

  if (!classes) return NULL;

  for (class_object = classes; ;
       class_object = class_object -> next) {
    if (class_object && class_object -> classvars) {
      for (class_var_object = class_object -> classvars;
	   class_var_object;
	   class_var_object = class_var_object -> next) {
	if (!strcmp (name, class_var_object -> __o_name))
	  results[results_ptr++] = class_var_object;
      }
    }
    if (!class_object || !class_object -> next) break;
  }

  if (results_ptr > 1) {
    warning (m_orig, 
	     "Class variable %s is defined in multiple classes.\n", name);
  }

  return results[0];
}

OBJECT *find_class_variable_2 (MESSAGE_STACK messages, int idx) {
  OBJECT *class_object, *class_var_object;
  OBJECT *results[MAXARGS] = {0,};
  int results_ptr = 0, prev_message_idx;
  MESSAGE *m_prev_msg, *m_orig;

  if (!classes) return NULL;

  m_orig = messages[idx];
  if ((prev_message_idx = prevlangmsg (messages, idx)) != ERROR) {
    m_prev_msg = messages[prev_message_idx];
    if (m_prev_msg -> obj && 
	IS_CLASS_OBJECT(m_prev_msg -> obj)) {
      if (m_prev_msg -> obj -> classvars) {
	for (class_var_object = m_prev_msg -> obj -> classvars;
	     class_var_object; 
	     class_var_object = class_var_object -> next) {
	  if (!strcmp (M_NAME(m_orig),
		       class_var_object -> __o_name)) {
	    results[results_ptr++] = class_var_object;
	  }
	}
      }
      return results[0];
    }
  }

  for (class_object = classes; ;
       class_object = class_object -> next) {
    if (class_object && class_object -> classvars) {
      for (class_var_object = class_object -> classvars;
	   class_var_object;
	   class_var_object = class_var_object -> next) {
	if (!strcmp (M_NAME(m_orig), class_var_object -> __o_name))
	  results[results_ptr++] = class_var_object;
      }
    }
    if (!class_object || !class_object -> next) break;
  }

  if (results_ptr > 1) {
    warning (m_orig, 
	     "Class variable %s is defined in multiple classes.\n", 
	     M_NAME(m_orig));
  }

  return results[0];
}

OBJECT *get_class_variable (char *name, char *classname, int warn) {

  OBJECT *o,
    *class_object;

  if (classname) {
    if ((class_object = get_class_object (classname)) == NULL) {
      if (warn)
	__ctalkExceptionInternal (message_stack_at (get_messageptr ()), 
				  undefined_class_x, classname,0);
      return NULL;
    }

    for (o = class_object -> classvars; o; o = o -> next) {
      if (!strcmp (o -> __o_name, name))
	return o;
    }

  } else {
    for (class_object = classes; class_object; 
	 class_object = class_object -> next) {
      for (o = class_object -> classvars; o; o = o -> next) {
	if (!strcmp (o -> __o_name, name))
	  return o;
      }
    }
  }

  return NULL;
}

/* 
 *  Retrieve any object by name and/or classname. Calls the functions
 *  above for global and local objects.
 */

OBJECT *get_object (char *name, char *classname) {

  OBJECT *o;

  if ((o = get_local_object (name, classname)) != NULL)
    return o;
  if ((o = get_class_variable (name, classname, FALSE)) != NULL)
    return o;
  if ((o = get_global_object (name, classname)) != NULL)
    return o;

  return NULL;
}

/*
 *   Create a new object.  Objects generally change to 
 *   the receiver class in new_object, or are deleted
 *   if they are only used as arguments.
 */

OBJECT *create_object (char *class, char *name) {

  OBJECT *o;
  int frame_ptr;

  if ((o = (OBJECT *)__xalloc (object_size)) == NULL) {
    printf ("create_object: %s\n", strerror (errno));
    exit (EXIT_FAILURE);
  }

  /*
   *  The longest a value should be is the length of a 
   *  literal, which is the same as the name when an
   *  argument object is created.
   */
  if ((o -> __o_value = (char *)__xalloc (NULLSTR_LENGTH)) == NULL) {
    printf ("create_object: %s\n", strerror (errno));
    exit (EXIT_FAILURE);
  }
#ifndef __sparc__
  o -> sig = 0xd3d3d3;
#else
  strcpy (o -> sig, "OBJECT");
#endif


  strcpy (o -> __o_classname, class);
  if (!strcmp (class, "Class")) {
    o -> __o_class = o;
  } else {
    o -> __o_class = get_class_object (class);
  }
  strcpy (o -> __o_name, name);

  frame_ptr = parser_frame_ptr ();
  /* ERROR here means that parsing hasn't started, so
     the call is to define a default (global) object. */
  if (frame_ptr == ERROR)
    o -> scope = GLOBAL_VAR;
  else
    o -> scope = frame_at (frame_ptr) -> scope;

  return o;
}

/*
 *   Wrapper function to create an object with the, "value," instance
 *   variable initialized.
 */
OBJECT *create_object_init (char *class, char *superclass, 
			    char *name, char *value) {

  OBJECT *o, *o_value;

  o = create_object (class, name);
  o_value = create_object (class, "value");
  if (superclass) {
    strcpy (o -> __o_superclassname, superclass);
    strcpy (o_value -> __o_superclassname, superclass);
    o -> __o_superclass = 
      o_value -> __o_superclass = 
      get_class_object (superclass);
  }
  o -> instancevars = o_value;
  __ctalkSetObjectValueVar (o, value);
  
  return o;
}

int push_arg (METHOD *m, OBJECT *arg) {


  if (m -> n_params == m -> n_args)
    return ERROR;

  /*
   *  TO DO - This should be un-kludged 
   *  eventually....
   */
  m -> args[m -> n_args++] = (ARG *)arg;


  return SUCCESS;
}

/*
 *  Delete an object, and its methods if it is a class object.
 */

void delete_object (OBJECT *o) {

  OBJECT *p;
  METHOD *m, *t;              /* Pointers to methods to be deleted. */
  OBJECT *r, *s;
  PARSER *parser;

  if (dictionary) {
    p = dictionary;
    while (p) {
      if (p == o) {
	if (p -> next)
	  p -> next -> prev = p -> prev;
	if (p -> prev)
	  p -> prev -> next = p -> next;
	if (p == dictionary) {
	  if (p -> next) {
	    dictionary = p -> next;
	  } else {
	    dictionary = NULL;
	  }
	}
      }
      p = p -> next;
    }
  }

  if ((parser = CURRENT_PARSER) != NULL) {
    if (o == parser -> methodobjects) {
      parser -> methodobjects = parser -> methodobjects -> next;
      goto local_object_check_done;
    }
    p = parser -> methodobjects;
    while (p) {
      if (p == o) {

	if (p -> next) {
	  p -> next -> prev = p -> prev;
	}
	if (p -> prev) {
	  p -> prev -> next = p -> next;
	}
	goto local_object_check_done;
      }
      p = p -> next;
    }

    if (parser -> vars == o) {
      parser -> vars = parser -> vars -> next;
      goto local_object_check_done;
    }
    p = parser -> vars;
    while (p) {
      if (p == o) {
	if (p -> next) {
	  p -> next -> prev = p -> prev;
	}
	if (p -> prev) {
	  p -> prev -> next = p -> next;
	}
	goto local_object_check_done;
      }
      p = p -> next;
    }
  }

 local_object_check_done:

  /* Delete any methods and their source code. */
  /* This should not occur, because only class
     objects have methods, and they are not 
     deleted.  TO DO - Make sure that methods
     get deleted correctly anyway. */
     
  if (o -> instance_methods) {
    for (t = o -> instance_methods; t -> next; t = t -> next)
      ;

    while (t) {
      m = t -> prev;
      if (t -> src)
	__xfree (MEMADDR(t -> src));
      __xfree (MEMADDR(t));
      t = m;
    }
  }

  if (o -> instancevars) {
    for (r = o -> instancevars; r -> next; r = r -> next)
      ;
    while (r) {
      s = r -> prev;
      delete_object (r);
      r = s;
    }
  }

  if (o -> classvars) {
    for (r = o -> classvars; r -> next; r = r -> next)
      ;
    while (r) {
      s = r -> prev;
      delete_object (r);
      r = s;
    }
  }

  __xfree (MEMADDR(o -> __o_value));
  __xfree (MEMADDR(o));
}

extern OBJECT *rcvr_class_obj;   /* Declared in lib/rtnwmthd.c and */

OBJECT *instantiate_self_object (void) {
  OBJECT *o, *argblk_rcvr, *argblk_instance_var_rcvr;
  ARGBLK *a;
  int next_tok;

  if (interpreter_pass == expr_check)
    return NULL;
  if ((interpreter_pass != method_pass) && !argblk) {
    _warning ("instantiate_self_object () called outside of a method.\n");
    return NULL;
  }

  /* If we have a String object as an argblk receiver, we can
     instantiate self within the argblk as a Character object. 
     Also check for an instance variable expression as the
     receiver. */
  if (argblk) {
    a = argblk_pop ();
    argblk_push (a);
    if ((argblk_rcvr = message_stack_at(a->rcvr_idx)->obj) != NULL) {
      next_tok = nextlangmsg (message_stack (), a -> rcvr_idx);
      if (is_instance_variable_message (message_stack (), next_tok)) {
	if ((argblk_instance_var_rcvr =
	     get_instance_variable (M_NAME(message_stack_at (next_tok)),
				    argblk_rcvr -> __o_classname, FALSE))
	    != NULL) {
	  if (argblk_instance_var_rcvr -> instancevars -> __o_class ==
	      ct_defclasses -> p_string_class) {
	    o = create_object_init
	      (ct_defclasses -> p_character_class -> __o_name, 
	       ct_defclasses -> p_character_class -> __o_superclassname,
	       "self", NULLSTR);
	    save_method_object (o);
	    return o;
	  }
	}
      } else {
	if (argblk_rcvr -> __o_class == ct_defclasses -> p_string_class) {
	  o = create_object_init
	    (ct_defclasses -> p_character_class -> __o_name, 
	     ct_defclasses -> p_character_class -> __o_superclassname,
	     "self", NULLSTR);
	  save_method_object (o);
	  return o;
	}
      }
    }
  }
  
  o = create_object_init (rcvr_class_obj -> __o_name, 
			  rcvr_class_obj -> __o_superclassname,
			  "self", NULLSTR);
  instance_variables_from_class_definition (o);
  save_method_object (o);
  return o;
}

OBJECT *instantiate_self_object_from_class (OBJECT *class_obj) {
  OBJECT *o;

  if (interpreter_pass == expr_check)
    return NULL;
  if ((interpreter_pass != method_pass) && !argblk) {
    _warning ("instantiate_self_object () called outside of a method.\n");
    return NULL;
  }
  o = create_object_init (class_obj -> __o_name, 
			  class_obj -> __o_superclassname,
			  "self", NULLSTR);
  instance_variables_from_class_definition (o);
  save_method_object (o);
  return o;
}

OBJECT *instantiate_object_from_class (OBJECT *class_object, char *name) {
  OBJECT *o;
  if (interpreter_pass == expr_check)
    return NULL;
  o = create_object_init (class_object -> __o_name, 
			  class_object -> __o_superclassname,
			  name, NULLSTR);
  instance_variables_from_class_definition (o);
  save_method_object (o);
  return o;
}

/*
 *   Resolve self.  The function is called every time self appears,
 *   so we can check if it is a receiver or a C keyword.  That way
 *   "self" can be used as a receiver, and also expanded to the
 *   __ctalk_self_internal () function when it is used in 
 *   cases like, "self -> scope."
 */

                                 /* filled in by new_method ().  */

OBJECT *self_object (MESSAGE_STACK messages, int msg_ptr) {

  OBJECT_CONTEXT self_context;
  OBJECT *self_obj = NULL;       /* Avoid a warning. */
  int m_next_ptr, m_prev_ptr;
  int unary_expr_start_ptr, unary_expr_end_ptr, i;
  char unary_expr_buf[MAXMSG];
  char s[MAXMSG], s1[MAXMSG];
  char expr_buf_out[MAXMSG];
  char *lval_class;
  int stack_start_idx, stack_top_idx;
  MESSAGE *m;

  stack_start_idx = stack_start (messages);
  stack_top_idx = get_stack_top (messages);

  if (messages[msg_ptr] -> attrs & TOK_SELF) {

    /*
     *  TO DO - 1. This case should be handled as an exception.
     *          2. It might also be necessary to handle cases
     *             where, "self," appears in a method argument -
     *             myString subString 1, self length - 1;
     *             These cases are handled in method_args () 
     *             at the moment.
     *          3. Handle cases where the receiver is a 
     *             constant - e.g.;
     *             "This is a string." subString 1, self length - 1;
     *             This will require additions in resolve ().
     *
     *  Handle the case where, "self," appears in a function -
     *  that is, there is no receiver class.  Check that we're
     *  in a function.
     */

    if (!rcvr_class_obj) {
      if (interpreter_pass != method_pass) {
	warning (messages[msg_ptr], "Keyword \"self,\" without receiver.");
	if (interpreter_pass == parsing_pass) {
	  char *n_1, errmsg[MAXMSG];

	  if ((n_1 = get_fn_name ()) != NULL) {
	    sprintf (errmsg, "\n  In function %s, declared at line %d.",
		      get_fn_name (), get_fn_start_line ());
	  } else {
	    if (parsers[current_parser_ptr] -> block_level == 0) {
	      sprintf (errmsg, "\n  In global scope.");
	    }
	  }

	  __ctalkExceptionInternal (messages[msg_ptr], 
				    self_without_receiver_x,
				    errmsg, 
				    messages[msg_ptr] -> error_line);
	}
	return NULL;
      } else {
	error (messages[msg_ptr], "Parser error.");
      }
    }

    self_context = object_context (messages, msg_ptr);

    switch (self_context)
      {
      case c_context:
      case c_argument_context:
	if ((m_next_ptr = nextlangmsg (messages, msg_ptr)) != ERROR) {
	  METHOD *m;
	  if ((!METHOD_ARG_TERM_MSG_TYPE (messages[m_next_ptr])) &&
	      ((m = get_instance_method (messages[m_next_ptr],
				 rcvr_class_obj, messages[m_next_ptr] -> name,
					 ERROR, FALSE)) != NULL)) {
	    self_obj = instantiate_self_object ();
	  } else {
	    if (M_TOK(messages[m_next_ptr]) == DEREF) {
	      /*
	       *  Allow expressions like self -> scope.
	       */
	      generate_self_call (msg_ptr);
	    } else {
	      int __arg_idx, __fn_idx, __end_idx;
	      if ((__arg_idx = obj_expr_is_fn_arg (messages, msg_ptr,
						   stack_start_idx,
						   &__fn_idx)) != ERROR) {
		rt_self_expr (messages, msg_ptr, &__end_idx,
			      expr_buf_out);
	      } else {
		if (!is_in_rcvr_subexpr_obj_check (messages, msg_ptr,
						   stack_top_idx)) {
		  if (fmt_arg_type (messages, msg_ptr, stack_start_idx)
		      == fmt_arg_ptr) {
		    /* 
		     * Don't need to cast. 
		     */
		    if ((m_prev_ptr = 
			 prevlangmsg (messages, msg_ptr)) != ERROR) {
		      if (IS_C_UNARY_MATH_OP(M_TOK(messages[m_prev_ptr]))) {
			find_self_unary_expr_limits
			  (messages, msg_ptr, 
			   &unary_expr_start_ptr,
			   &unary_expr_end_ptr,
			   stack_start_idx, stack_top_idx);
			fmt_rt_expr (messages, unary_expr_start_ptr,
				     &unary_expr_end_ptr,
				     unary_expr_buf);

			fileout (unary_expr_buf, 0, msg_ptr);
			
			for (i = unary_expr_start_ptr; i >= unary_expr_end_ptr;
			     i--) {
			  ++messages[i] -> evaled;
			  ++messages[i] -> output;
			}
		      } else {
			strcatx (s1, SELF_ACCESSOR_FN, " ()", NULL);
			fileout (s1, 0, msg_ptr);
			++messages[msg_ptr] -> evaled;
			++messages[msg_ptr] -> output;
		      }
		    } else {
		      strcatx (s1, SELF_ACCESSOR_FN, " ()", NULL);
		      fileout (s1, 0, msg_ptr);
		      ++messages[msg_ptr] -> evaled;
		      ++messages[msg_ptr] -> output;
		    }
		  
		    return NULL;
		  } else if (is_fmt_arg (messages, msg_ptr,
					 stack_start_idx,
					 stack_top_idx)) {
		    strcatx (s1, SELF_ACCESSOR_FN, " ()", NULL);
		    fmt_printf_fmt_arg (messages, msg_ptr, stack_start_idx,
					s1, s);
		    fileout (s, 0, msg_ptr);
		    ++messages[msg_ptr] -> evaled;
		    ++messages[msg_ptr] -> output;
		  
		    return NULL;
		  } else if ((lval_class = use_new_c_rval_semantics_b
			      (messages, msg_ptr)) != NULL) {
		    strcatx (s, SELF_ACCESSOR_FN, " ()", NULL);
		    fmt_rt_return (s, lval_class, TRUE, expr_buf_out);
		    fileout (expr_buf_out, 0, msg_ptr);
		    ++messages[msg_ptr] -> evaled;
		    ++messages[msg_ptr] -> output;
		    return NULL;
		  } else {
		    generate_self_call (msg_ptr);
		  }
		} else {/*if (!is_in_rcvr_subexpr_obj_check ... */
		  self_obj = NULL;
		} 
	      }
	    }
	  }
	}
 	++messages[msg_ptr] -> evaled;
 	++messages[msg_ptr] -> output;
  	return self_obj;
 	break;
      case argument_context:
      case receiver_context:
	if ((interpreter_pass == method_pass) || argblk) {
	  m = messages[msg_ptr];
	  if (TOK_HAS_CLASS_TYPECAST(m)) {
	    m -> obj = instantiate_self_object_from_class
	      (m -> receiver_msg -> obj);
	    return m -> obj;
	  } else {
	    self_obj = instantiate_self_object ();
	    ++messages[msg_ptr] -> evaled;
	    ++messages[msg_ptr] -> output;
	    return self_obj;
	  }
	} else if (interpreter_pass != expr_check) {
	  self_outside_method_error (messages, msg_ptr);
	}
 	break;
      default:
 	break;
      }
  }

  return NULL;
}

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c.*/
extern int new_method_ptr;

int label_is_defined (MESSAGE_STACK messages, int ptr) {

  OBJECT *tmp;
  MESSAGE *m;
  METHOD *method;
  int i, prev_idx;
  CVAR *c;

  m = messages[ptr];

  if (m -> attrs & TOK_SELF || m -> attrs & TOK_SUPER) {
    if (new_method_ptr == MAXARGS && !argblk) {
      warning (m, "Label, \"%s,\" used within a C function.",
	       M_NAME(m));
    }
    return TRUE;
  }

  if (((tmp = get_object (m -> name, NULL)) != NULL) ||
      ((tmp = get_class_object (m -> name)) != NULL))
    return TRUE;

  if (((c = get_local_var (messages[ptr] -> name)) != NULL) ||
      ((c = get_global_var (messages[ptr] -> name)) != NULL))
    return TRUE;

  if (is_struct_element (messages, ptr))
    return TRUE;

  if (is_instance_method (messages, ptr) ||
      is_class_method (messages, ptr))
    return TRUE;

  if (_hash_get (defined_instancevars, M_NAME(messages[ptr])))
    return TRUE;

  if ((interpreter_pass == method_pass) ||
      (interpreter_pass == expr_check)) {
    if (new_method_ptr < MAXARGS) {
      if ((method = new_methods[new_method_ptr + 1] -> method) != NULL) {
	for (i = 0; i < method -> n_params; i++) {
	  if (!strcmp (method -> params[i] -> name, m -> name))
	    return TRUE;
	}
      }
    }
  }

  if (interpreter_pass == method_pass) {
    /* Check for a "self <instancevar>" expression. */
    if ((prev_idx = prevlangmsg (messages, ptr)) != ERROR) {
      if (messages[prev_idx] -> attrs & TOK_SELF) {
	if (IS_OBJECT(rcvr_class_obj)) {
	  OBJECT *var;
	  for (var = rcvr_class_obj -> instancevars; var; var = var -> next) {
	    if (str_eq (var -> __o_name, M_NAME(m))) {
	      return TRUE;
	    }
	  }
	}
      }
    }
  }

  return FALSE;
}

OBJECT *constant_token_object (MESSAGE *m_tok) {

  OBJECT *arg_obj;

  switch (m_tok -> tokentype)
    {
    case LITERAL:
      arg_obj = 
	create_object_init (STRING_CLASSNAME, 
			    STRING_SUPERCLASSNAME,
			    m_tok -> name, m_tok -> name);
      arg_obj -> attrs |= OBJECT_IS_STRING_LITERAL;
      break;
    case LITERAL_CHAR:
      arg_obj = 
	create_object_init (CHARACTER_CLASSNAME, 
			    CHARACTER_SUPERCLASSNAME,
			    m_tok -> name, m_tok -> name);
      arg_obj -> attrs |= OBJECT_IS_LITERAL_CHAR;
      break;
    case INTEGER:
    case LONG:
      arg_obj = 
	create_object_init (INTEGER_CLASSNAME, 
			    INTEGER_SUPERCLASSNAME,
			    m_tok -> name, m_tok -> name);
      break;
    case FLOAT:
      arg_obj = 
	create_object_init (FLOAT_CLASSNAME, 
			    FLOAT_SUPERCLASSNAME,
			    m_tok -> name, m_tok -> name);
      break;
    case LONGLONG:
      arg_obj = 
	create_object_init (LONGINTEGER_CLASSNAME, 
			    LONGINTEGER_SUPERCLASSNAME,
			    m_tok -> name, m_tok -> name);
      break;
    case PATTERN:
      arg_obj = 
	create_object_init (OBJECT_CLASSNAME, 
			    OBJECT_SUPERCLASSNAME,
			    m_tok -> name, m_tok -> name);
      break;
    default:
      _warning ("Unknown token type in constant_token_object.\n");
      arg_obj = NULL;
    }
  return arg_obj;
}

int delete_arg_object (OBJECT *o) {
  PARSER *p;
  if ((p = CURRENT_PARSER) != NULL) {
    if (p -> methodobjects == o) {
      p -> methodobjects = p -> methodobjects -> next;
    }
  }
  if (o -> next) o -> next -> prev = o -> prev;
  if (o -> prev) o -> prev -> next = o -> next;
  delete_object (o);
  return SUCCESS;
}

void cleanup_dictionary (void) {
  OBJECT *o, *o_prev;
  if (dictionary) {
    for (o = dictionary; o && o -> next; o = o -> next) 
      ;
    while (o != dictionary) {
      o_prev = o -> prev;
      delete_object (o);
      o = o_prev;
    }
    delete_object  (dictionary);
  }
}

int constant_rcvr_idx (MESSAGE_STACK messages, int method_msg_idx) {
  int prev_tok_idx;
  if ((prev_tok_idx = prevlangmsg (messages, method_msg_idx)) != ERROR) {
    if ((M_TOK(messages[prev_tok_idx]) == LITERAL) ||
	(M_TOK(messages[prev_tok_idx]) == LITERAL_CHAR) || 
	(M_TOK(messages[prev_tok_idx]) == INTEGER) || 
	(M_TOK(messages[prev_tok_idx]) == FLOAT) ||
	(M_TOK(messages[prev_tok_idx]) == LONG) ||
	(M_TOK(messages[prev_tok_idx]) == LONGLONG))
      return prev_tok_idx;
  }
  return ERROR;
}

/*
 *  Checks for an tokens that need to be evaluated between
 *  parentheses; i.e., anything not a constant, label, parenthesis,
 *  or array or struct notation token indicates that evaluation
 *  occurs within the expression's parentheses.  Also checks that
 *  the "method" label isn't a GNU function attribute.
 */
bool is_possible_receiver_subexpr_postfix (MSINFO *ms, int method_msg_idx) {
  int prev_tok_idx, open_paren_idx, i;
  if ((prev_tok_idx = prevlangmsg (ms -> messages, method_msg_idx)) != ERROR) {
    if (M_TOK(ms -> messages[prev_tok_idx]) == CLOSEPAREN) {
#ifdef __GNUC__
      if (is_gnu_extension_keyword (M_NAME(ms -> messages[method_msg_idx]))) {
	return false;
      }
#endif      
      if ((open_paren_idx = match_paren_rev (ms -> messages, prev_tok_idx,
					     ms -> stack_start))
	  != ERROR) {
	for (i = open_paren_idx; i >= prev_tok_idx; i--) {
	  switch (M_TOK(ms -> messages[i]))
	    {
	    case WHITESPACE: case NEWLINE: case OPENPAREN:
	    case CLOSEPAREN: case LABEL: case LITERAL:
	    case LITERAL_CHAR: case INTEGER: case FLOAT:
	    case LONG: case LONGLONG: case ARRAYOPEN:
	    case ARRAYCLOSE: case PERIOD: case DEREF:
	      continue;
	    }
	  return true;
	}
      }
    }
  }
  return false;
}

static bool is_self_or_super_instance_var_series (MESSAGE_STACK messages,
						  int label_idx) {
  int i, stack_start_idx;
  MESSAGE *m;
  stack_start_idx = stack_start (messages);
  
  for (i = label_idx + 1; i <= stack_start_idx; ++i) {
    if (M_ISSPACE(messages[i]))
      continue;
    if (M_TOK(messages[i]) == LABEL) {

      if (messages[i] -> attrs & TOK_SELF ||
	  messages[i] -> attrs & TOK_SUPER)
	return true;

      m = messages[i];

      if (is_c_keyword (M_NAME(m)))
	return false;

      if (!is_instance_var (M_NAME(m)) &&
	  !is_class_name (M_NAME(m)) &&
	  !is_method_selector (M_NAME(m)) &&
	  !is_proto_selector (M_NAME(m)) &&
	  !is_c_data_type (M_NAME(m)))
	warning (messages[i], "Undefined label, \"%s.\"",
		 M_NAME(messages[i]));

    } else {
      break;
    }
  }
  return false;
}

static bool is_parameter_instance_var_series (MESSAGE_STACK messages,
					      int label_idx,
					      METHOD *new_method) {
  int i, i_2, j, stack_start_idx,
    undef_label_idx = label_idx;
  MESSAGE *m;
  bool possible_undefined_var = false;
  char varclassname[MAXLABEL];
  OBJECT *instancevar_object;
  stack_start_idx = stack_start (messages);
  
  for (i = label_idx; i <= stack_start_idx; ++i) {
    if (M_ISSPACE(messages[i]))
      continue;
    if (M_TOK(messages[i]) == LABEL) {

      for (j = 0; j < new_method -> n_params; ++j) {
	if (str_eq (M_NAME(messages[i]), new_method -> params[j] -> name)) {
	  if (possible_undefined_var) {
	    /* scan forward to the variable name. */
	    strcpy (varclassname, new_method -> params[j] -> class);
	    for (i_2 = i - 1; i_2 >= label_idx; i_2--) {
	      if (M_ISSPACE(messages[i_2])) {
		continue;
	      } else if (M_TOK(messages[i_2]) == LABEL) {
		if ((instancevar_object =
		     get_instance_variable (M_NAME(messages[i_2]),
					    varclassname, FALSE)) == NULL) {
		  warning (messages[undef_label_idx],
			   "Undefined label, \"%s.\"",
			   M_NAME(messages[undef_label_idx]));
		  return true;
		} else {
		  if (IS_OBJECT(instancevar_object) &&
		      IS_OBJECT(instancevar_object -> instancevars) &&
		      IS_OBJECT(instancevar_object -> instancevars ->
				__o_class)) {
		    /* the variable's class native class is the
		       value of it's "value" instancevar, the
		       class of its parent object is the class
		       of the top level object. */
		    strcpy (varclassname,
			    instancevar_object -> instancevars ->
			    __o_class -> __o_name);
		  } else {
		    warning (messages[undef_label_idx],
			     "Invalid instance variable , \"%s.\"",
			     M_NAME(messages[i_2]));
		    return true;
		  }
		}
	      }
	    }
	  }
	  return true;
	}
      }

      m = messages[i];
      if (!is_instance_var (M_NAME(m)) &&
	  !is_class_name (M_NAME(m)) &&
	  !is_method_selector (M_NAME(m)) &&
	  !is_proto_selector (M_NAME(m)) &&
	  !(m -> attrs & TOK_SELF) &&
	  !(m -> attrs & TOK_SUPER) &&
	  !is_c_data_type (M_NAME(m))) {
	possible_undefined_var = true;
	undef_label_idx = i;
      }

    } else {
      break;
    }
  }
  return false;
}

int is_instance_variable_message (MESSAGE_STACK messages, int idx) {
  int prev_tok_idx;
  MESSAGE *m_prev_tok;
  OBJECT *prev_tok_obj;
  METHOD *m;

  if ((prev_tok_idx = prevlangmsgstack (messages, idx)) != ERROR) {
    m_prev_tok = messages[prev_tok_idx];
    if (M_TOK(m_prev_tok) == LABEL) {
      if (is_c_keyword(M_NAME(m_prev_tok))) {
	return FALSE;
      }
      if (m_prev_tok -> obj && IS_OBJECT(m_prev_tok -> obj)) {
	if (get_instance_variable 
	    (M_NAME(messages[idx]), 
	     m_prev_tok -> obj -> __o_class -> __o_name, FALSE)) {
	  return TRUE;
	}
      } else if ((prev_tok_obj = get_local_object (M_NAME(m_prev_tok), NULL))
		 != NULL) {
	if (get_instance_variable 
	    (M_NAME(messages[idx]), 
	     prev_tok_obj -> __o_class -> __o_name, FALSE)) {
	  return TRUE;
	}
      } else if ((prev_tok_obj = get_global_object (M_NAME(m_prev_tok), NULL))
		 != NULL) {
	if (get_instance_variable 
	    (M_NAME(messages[idx]), 
	     prev_tok_obj -> __o_class -> __o_name, FALSE)) {
	  return TRUE;
	}
      } else if (interpreter_pass == method_pass) {
	m = new_methods[new_method_ptr+1] -> method;
	if (m_prev_tok -> attrs & TOK_SELF) {
	  if (get_instance_variable 
	      (M_NAME(messages[idx]), 
	       rcvr_class_obj -> __o_name, FALSE)) {
	    return TRUE;
	  }
	} else if (m_prev_tok -> attrs & TOK_SUPER) {
	  if (argblk) {
	    if (get_instance_variable 
		(M_NAME(messages[idx]), 
		 rcvr_class_obj -> __o_name, FALSE)) {
	      return TRUE;
	    }
	  } else {
	    if (IS_OBJECT(rcvr_class_obj -> __o_class) &&
		IS_OBJECT(rcvr_class_obj -> __o_class -> __o_superclass)) {
	      if (get_instance_variable 
		  (M_NAME(messages[idx]), 
		   rcvr_class_obj -> __o_class -> __o_superclass 
		   -> __o_name, FALSE)) {
		return TRUE;
	      }
	    }
	  }
	} else if (is_parameter_instance_var_series (messages, idx, m)) {
	  return TRUE;
	} else if (is_self_or_super_instance_var_series (messages, idx)) {
	  return TRUE;
	}
      }
    }
  }
  return FALSE;
}

/* Check the label following a "self" message. */
int is_self_instance_variable_message (MESSAGE_STACK messages, int idx) {
  int prev_tok_idx;
  if ((prev_tok_idx = prevlangmsg (messages, idx)) != ERROR) {
    if (messages[prev_tok_idx] -> receiver_msg &&
	messages[prev_tok_idx] ->  receiver_msg -> attrs &
	TOK_IS_CLASS_TYPECAST) {
      if (get_instance_variable 
	  (M_NAME(messages[idx]),
	   messages[prev_tok_idx] -> receiver_msg -> obj -> __o_name,
	   FALSE)) {
	return TRUE;
      }
    }
  }

  if (get_instance_variable 
      (M_NAME(messages[idx]), rcvr_class_obj -> __o_name, FALSE)) {
    return TRUE;
  }
  return FALSE;
}

int is_class_variable_message (MESSAGE_STACK messages, int idx) {
  OBJECT *result;
  if ((result = find_class_variable_2 (messages, idx)) != NULL)
    return TRUE;
  else 
    return FALSE;
}

extern int warn_duplicate_name_opt;

int global_var_is_declared_not_duplicated (MESSAGE_STACK messages, 
					   int idx, 
					   char *s) {
  OBJECT *o;
  if (global_var_is_declared (s)) {
    if (((o = get_object (s, NULL)) == NULL) && 
	!is_method_parameter (messages, idx)) {
      return TRUE;
    } else {
      if (warn_duplicate_name_opt) {
	warning (messages[idx], "Object \"%s\" duplicates a C variable name.",
		 s);
      }
      return FALSE;
    }
  }
  return FALSE;
}

/*
 *  All of the checks of is_in_rcvr_subexpr, plus checks for 
 *  an object immediately after the closing paren.
 *
 *  Returns the closing paren of the expression if true, 0 otherwise.
 */
int is_in_rcvr_subexpr_obj_check (MESSAGE_STACK messages, int idx, 
				  int stack_end) {
  int close_paren_idx, next_tok_idx;

  if ((close_paren_idx = is_in_rcvr_subexpr (messages, idx, 
					     stack_start(messages),
					     stack_end))
      != 0) {
    if ((next_tok_idx = nextlangmsg (messages, close_paren_idx)) != 
	ERROR) {
      if (get_object (M_NAME(messages[next_tok_idx]), NULL)) {
	return FALSE;
      }	else {
	return close_paren_idx;
      }
    }
  }
  return FALSE;
}

/* 
 *  Called by resolve ().  For a sequence of any number of instance
 *  variable messages, sets the object to the instance variable object,
 *  and the message attribute to OBJ_IS_INSTANCE_VAR - EXCEPT for the
 *  first message following the receiver, which resolve (), and then
 *  method_call () check as a method message also in case of a possible
 *  object mutation.
 *
 *  There should also set the receiver message and receiver object
 *  of each instance variable message to the receiver message and
 *  object of the sender message.
 *
 *  Returns the first instance variable in the series, or NULL, which 
 *  is what resolve (), and method_call (), expect.  
 *
 *  This function is very specific - it's probably not useful anywhere
 *  else.
 * 
 *  Also looks up class variables.
 */
OBJECT *get_instance_variable_series (OBJECT *prev_object,
				  MESSAGE *var_message,
				  int var_idx,
				  int stack_end) {
  OBJECT *first_instance_var, *next_instance_var;
  OBJECT *prev_object_l;
  OBJECT *sender_rcvr_obj;
  MESSAGE *next_message;
  MESSAGE *sender_rcvr_msg;
  int next_idx;
  if ((first_instance_var = 
       __ctalkGetInstanceVariable (prev_object, 
				   M_NAME(var_message), FALSE)) != NULL) {

    sender_rcvr_obj = var_message -> receiver_obj;
    sender_rcvr_msg = var_message -> receiver_msg;

    /* DO NOT SET THESE HERE!  This message also gets checked whether
       it's also a method message later on. See the comments in resolve (). */
    /* first_message = message_stack_at (var_idx); */
    /* first_message -> obj = first_instance_var; */

    next_idx = var_idx;
    prev_object_l = first_instance_var;
    if ((next_idx = nextlangmsg (message_stack (), var_idx)) == ERROR)
      return first_instance_var;

    while ((next_idx = nextlangmsg (message_stack (), next_idx)) != ERROR) {

      next_message = message_stack_at (next_idx);

      if (M_ISSPACE(next_message))
	continue;
      if (M_TOK(next_message) != LABEL)
	break;

      if ((next_instance_var = 
	   get_instance_variable 
	   (M_NAME(next_message),
	    (prev_object_l -> instancevars ? 
	     prev_object_l -> instancevars -> __o_classname : 
	     prev_object_l -> __o_classname),
	    FALSE)) != NULL) {

	next_message -> obj = next_instance_var;
	next_message -> attrs |= OBJ_IS_INSTANCE_VAR;
	next_message -> receiver_obj = sender_rcvr_obj;
	next_message -> receiver_msg = sender_rcvr_msg;

	prev_object_l = next_instance_var;

      } else {
	if ((next_instance_var = 
	     get_class_variable 
	     (M_NAME(next_message),
	      (prev_object_l -> instancevars ? 
	       prev_object_l -> instancevars -> __o_classname : 
	       prev_object_l -> __o_classname),
	      FALSE)) != NULL) {

	  next_message -> obj = next_instance_var;
	  next_message -> attrs |= OBJ_IS_CLASS_VAR;
	  next_message -> receiver_obj = prev_object_l;
	  next_message -> receiver_msg = sender_rcvr_msg;
	  
	  prev_object_l = next_instance_var;

	} else {
	
	  return first_instance_var;

	}
      }
    }
  }

  return first_instance_var;
}

/* Called by fmt_instancevar_expr_a (), probably not useful
   anywhere else. */
int get_instance_variable_series_term_idx (OBJECT *prev_object,
					   MESSAGE *m_sender,
					   int message_idx,
					   int stack_end) {
  OBJECT *first_instance_var, *next_instance_var;
  OBJECT *prev_object_l;
  OBJECT *sender_rcvr_obj;
  MESSAGE *next_message;
  MESSAGE *sender_rcvr_msg;
  int next_idx, lookahead;
  if ((first_instance_var = 
       __ctalkGetInstanceVariable (prev_object, 
				   M_NAME(m_sender), FALSE)) != NULL) {

    sender_rcvr_obj = m_sender -> receiver_obj;
    sender_rcvr_msg = m_sender -> receiver_msg;

    /* DO NOT SET THESE HERE!  This message also gets checked whether
       its also a method message later on. See the comments in resolve (). */
    /* first_message = message_stack_at (message_idx); */
    /* first_message -> obj = first_instance_var; */

    next_idx = message_idx;
    prev_object_l = first_instance_var;
    if ((lookahead = nextlangmsg (message_stack (), message_idx)) == ERROR)
      return message_idx;

    while ((lookahead = nextlangmsg (message_stack (), next_idx)) != ERROR) {

      next_message = message_stack_at (lookahead);

      if ((next_instance_var = 
	   get_instance_variable 
	   (M_NAME(next_message),
	    (prev_object_l -> instancevars ? 
	     prev_object_l -> instancevars -> __o_classname : 
	     prev_object_l -> __o_classname),
	    FALSE)) != NULL) {

	next_message -> obj = next_instance_var;
	next_message -> attrs |= OBJ_IS_INSTANCE_VAR;
	next_message -> receiver_obj = sender_rcvr_obj;
	next_message -> receiver_msg = sender_rcvr_msg;

	prev_object_l = next_instance_var;
	next_idx = lookahead;

      } else {
	if ((next_instance_var = 
	     get_class_variable 
	     (M_NAME(next_message),
	      (prev_object_l -> instancevars ? 
	       prev_object_l -> instancevars -> __o_classname : 
	       prev_object_l -> __o_classname),
	      FALSE)) != NULL) {

	  next_message -> obj = next_instance_var;
	  next_message -> attrs |= OBJ_IS_CLASS_VAR;
	  next_message -> receiver_obj = prev_object_l;
	  next_message -> receiver_msg = sender_rcvr_msg;
	  
	  prev_object_l = next_instance_var;
	  next_idx = lookahead;

	} else {
	
	  return next_idx;

	}
      }
    }
  }

  return message_idx;
}

/* Like above, but uses rcvr_class_obj for the "self" message. */
/* Also looks up class variables. */
OBJECT *get_self_instance_variable_series (MESSAGE *m_sender,
					   int self_idx,
					   int message_idx,
					   int stack_end) {
  OBJECT *first_instance_var, *next_instance_var;
  OBJECT *prev_object_l;
  OBJECT *expr_rcvr_class = rcvr_class_obj;
  MESSAGE *m_self;
  MESSAGE *next_message;
  MESSAGE *sender_rcvr_msg;
  int next_idx;

  m_self = message_stack_at (self_idx);

  if (m_self -> receiver_msg &&
      m_self -> receiver_msg -> attrs & TOK_IS_CLASS_TYPECAST)
    expr_rcvr_class = m_self -> receiver_msg -> obj;
    
  if ((first_instance_var = 
       get_instance_variable (M_NAME(m_sender), 
			      expr_rcvr_class -> __o_name, FALSE)) != NULL) {

    sender_rcvr_msg = m_self;

    m_sender -> obj = first_instance_var;
    m_sender -> attrs |= OBJ_IS_INSTANCE_VAR;
    m_sender -> receiver_msg = m_self;
    if (expr_rcvr_class != rcvr_class_obj) {
      m_sender -> receiver_obj =
	instantiate_self_object_from_class (expr_rcvr_class);
    } else {
      m_sender -> receiver_obj = instantiate_self_object ();
    }

    next_idx = message_idx;
    prev_object_l = first_instance_var;

    while ((next_idx = nextlangmsg (message_stack (), next_idx)) != ERROR) {

      next_message = message_stack_at (next_idx);

      if ((next_instance_var = 
	   get_instance_variable 
	   (M_NAME(next_message),
	    (prev_object_l -> instancevars ? 
	     prev_object_l -> instancevars -> __o_classname : 
	     prev_object_l -> __o_classname),
	    FALSE)) != NULL) {

	next_message -> obj = next_instance_var;
	next_message -> attrs |= OBJ_IS_INSTANCE_VAR;
	next_message -> receiver_obj = prev_object_l;
	next_message -> receiver_msg = sender_rcvr_msg;

	prev_object_l = next_instance_var;

      } else {

	if ((next_instance_var = 
	     get_class_variable 
	     (M_NAME(next_message),
	      (prev_object_l -> instancevars ? 
	       prev_object_l -> instancevars -> __o_classname : 
	       prev_object_l -> __o_classname),
	      FALSE)) != NULL) {

	  next_message -> obj = next_instance_var;
	  next_message -> attrs |= OBJ_IS_CLASS_VAR;
	  next_message -> receiver_obj = prev_object_l;
	  next_message -> receiver_msg = sender_rcvr_msg;
	  
	  prev_object_l = next_instance_var;
	} else {
	  
	  /*
	   *  This can be valid in the case where a
	   *  class (like Symbol) can refer to an object
	   *  of any class - so check.
	   */
	  if ((next_instance_var = 
	       get_instance_variable 
	       (M_NAME(next_message),
		prev_object_l -> __o_classname,
		FALSE)) != NULL) {

	    next_message -> obj = next_instance_var;
	    next_message -> attrs |= OBJ_IS_INSTANCE_VAR;
	    next_message -> receiver_obj = prev_object_l;
	    next_message -> receiver_msg = sender_rcvr_msg;
	    
	    prev_object_l = next_instance_var;

	  } else {

	    return first_instance_var;

	  }
	}
      }
    }
  }

  return first_instance_var;
}

/* Called from c_param_expr_arg (). */
char *get_param_instance_variable_series_class (PARAM *param,
						MESSAGE_STACK messages,
						int param_idx,
						int stack_end) {
  OBJECT *param_class_object;
  OBJECT *instancevar_object;
  OBJECT *prev_tok_object;
  static char expr_class[MAXLABEL];
  int i;
  bool have_instance_var;

  memset (expr_class, 0, MAXLABEL);

  /*
   * TODO - Almost certainly this will need some stuff to get definitions
   * for pending and not-yet loaded classes, and superclasses.
   */
  if ((param_class_object = get_class_object (param -> class)) == NULL)
    return NULL;

  prev_tok_object = param_class_object;

  for (i = param_idx - 1; i > stack_end; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    if (M_TOK(messages[i]) == LABEL) {

      for (have_instance_var = false,
	     instancevar_object = prev_tok_object -> instancevars;
	   instancevar_object && !have_instance_var; 
	   instancevar_object = instancevar_object -> next) {

	if (str_eq (instancevar_object -> __o_name, M_NAME(messages[i]))) {
	  if (instancevar_object -> instancevars) {
	    strcpy (expr_class, 
		    instancevar_object -> instancevars -> __o_classname);
	  } else {
	    strcpy (expr_class, 
		    instancevar_object -> __o_classname);
	  }
	  have_instance_var = true;
	  prev_tok_object = instancevar_object;
	}
      }

      if (!have_instance_var)
	break;

    } else {

      break;
    }

  }

  return *expr_class ? expr_class : NULL;
}

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

static inline void save_param_as_methodobject (OBJECT *obj) {

  PARSER *p;
  OBJECT *t;
  /* Save the object here so we can clean it up later. */
  p = parsers[current_parser_ptr];
  if (!p -> methodobjects) {
    p -> methodobjects = obj;
  } else {
    for (t = p -> methodobjects; IS_OBJECT(t) && IS_OBJECT(t -> next);
	 t = t -> next)
      ;
    t -> next = obj;
    obj -> prev = t;
  }
}

/* Called from eval_arg ().  Look for an argument label
   that is a method parameter. */
OBJECT *get_method_param_obj (MESSAGE_STACK messages, int idx) {

  int nth_param;
  METHOD *m_new;
  OBJECT *tmp_object;

  if (interpreter_pass != method_pass)
    return NULL;

  m_new = new_methods[new_method_ptr+1] -> method;
  for (nth_param = 0; nth_param < m_new -> n_params; nth_param++) {
    if (str_eq (m_new -> params[nth_param] -> name, M_NAME(messages[idx]))) {
      tmp_object = create_object (m_new -> params[nth_param] -> class,
				  m_new -> params[nth_param] -> name);
      save_param_as_methodobject (tmp_object);
      return tmp_object;
    }
  }
  return NULL;
}

void get_new_method_param_instance_variable_series (OBJECT *param_class_object,
						    MESSAGE_STACK messages,
						    int param_idx,
						    int stack_end) {
  OBJECT *instancevar_object;
  OBJECT *prev_tok_object;
  int i;
  bool have_instance_var;

  prev_tok_object = create_object (param_class_object -> __o_name,
				   M_NAME(messages[param_idx]));
  instance_variables_from_class_definition (prev_tok_object);

  save_param_as_methodobject (prev_tok_object);

  messages[param_idx] -> obj = prev_tok_object;

  for (i = param_idx - 1; i > stack_end; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    if (M_TOK(messages[i]) == LABEL) {

      for (have_instance_var = false,
	     instancevar_object = prev_tok_object -> instancevars;
	   instancevar_object && !have_instance_var; 
	   instancevar_object = instancevar_object -> next) {

	if (str_eq (instancevar_object -> __o_name, M_NAME(messages[i]))) {
	  have_instance_var = true;
	  prev_tok_object = instancevar_object;
	  messages[i] -> obj = prev_tok_object;
	  messages[i] -> attrs |= OBJ_IS_INSTANCE_VAR;
	  messages[i] -> receiver_msg = messages[param_idx];
	  messages[i] -> receiver_obj = messages[param_idx] -> obj;
	}
      }

      if (!have_instance_var) {
	/***/
	/* TOK_IS_CLASS_TYPECAST set in fn_arg_expression, so far */
	if (str_eq (M_OBJ(messages[param_idx]) -> __o_class -> __o_name,
		    OBJECT_CLASSNAME) &&
	    !(messages[param_idx] -> attrs & TOK_IS_CLASS_TYPECAST)) {
	  warning (messages[param_idx], "Instance variable expression begins "
		   "with parameter \"%s,\" which is declared as an Object.",
		   M_NAME(messages[param_idx]));
	  messages[i] -> obj = prev_tok_object;
	  messages[i] -> attrs |= OBJ_IS_INSTANCE_VAR;
	  messages[i] -> receiver_msg = messages[param_idx];
	  messages[i] -> receiver_obj = messages[param_idx] -> obj;
	}
	break;
      }

    } else {

      break;
    }

  }
}

bool primitive_arg_shadows_method_parameter (char *arg_name) {
  METHOD *m;
  int i;
  if (interpreter_pass == method_pass) {
    if ((m = new_methods[new_method_ptr+1] -> method) != NULL) {
      if (!IS_CONSTRUCTOR_LABEL(m -> name)) {
	/* Unless the current method is also a constructor, then
	   we're simply passing the argument to the superclass
	   constructor.
	*/
	for (i = 0; i < m -> n_params; ++i) {
	  if (str_eq (arg_name, m -> params[i] -> name)) {
	    return true;
	  }
	}
      }
    }
  }
  return false;
}

int primitive_arg_shadows_c_symbol (char *arg_name) {

  if (!get_global_object (arg_name, NULL)) {
    if (get_global_var (arg_name) ||
	get_function (arg_name)) {
      return TRUE;
    }
  }
  return FALSE;
}

/*
 *  Wraps the cvartab reference in parens if the trailing or
 *  leading operator is a unary ++, --, ~, &, ., or ->
 *
 *  e.g., 
 *
 *      *main_x++
 *
 *  becomes
 *
 *      (*main_x)++
 */
static void op_precedence_fmt (MESSAGE_STACK messages,
				     int var_tok_idx,
				     char *buf_in_out) {
  int i;
  char buf_tmp[MAXMSG]; /* use a tmp buf so the label re-formatting works
			   on everything. */
  if ((i = prevlangmsg (messages, var_tok_idx)) != ERROR) {
    if (M_TOK(messages[i]) == INCREMENT ||
	M_TOK(messages[i]) == DECREMENT ||
	M_TOK(messages[i]) == AMPERSAND ||
	M_TOK(messages[i]) == EXCLAM ||
	M_TOK(messages[i]) == MULT) {
      strcpy (buf_tmp, buf_in_out);
      strcatx (buf_in_out, "(", buf_tmp, ")", NULL);
    } else if ((M_TOK(messages[i]) == MINUS) && is_unary_minus (messages, i)) {
      strcpy (buf_tmp, buf_in_out);
      strcatx (buf_in_out, "(", buf_tmp, ")", NULL);
    }
  }
  if ((i = nextlangmsg (messages, var_tok_idx)) != ERROR) {
    if (M_TOK(messages[i]) == INCREMENT ||
	M_TOK(messages[i]) == DECREMENT) {
      strcpy (buf_tmp, buf_in_out);
      strcatx (buf_in_out, "(", buf_tmp, ")", NULL);
    }
  }
}

static OBJECT *EXPR_class_obj = NULL;
static OBJECT *EXPR_superclass_obj = NULL;
extern int fn_arg_expression_call; /* Declared in arg.c. */

OBJECT *create_arg_EXPR_object (ARGSTR *argbuf) {

  OBJECT *o;
  int i;
  MESSAGE *m, *scratch_msg;
  char buf_out[MAXMSG];
  CVAR *cvar;

  if ((o = (OBJECT *)__xalloc (object_size)) == NULL) {
    printf ("create_arg_EXPR_object: %s\n", strerror (errno));
    exit (EXIT_FAILURE);
  }

#ifndef __sparc__
  o -> sig = 0xd3d3d3;
#else
  strcpy (o -> sig, "OBJECT");
#endif

  strcpy (o -> __o_classname, EXPR_CLASSNAME);
  if (!EXPR_class_obj) {
    o -> __o_class = EXPR_class_obj = get_class_object (EXPR_CLASSNAME);
  } else {
    o -> __o_class = EXPR_class_obj;
  }
  strcpy (o -> __o_superclassname, EXPR_SUPERCLASSNAME);
  if (!EXPR_superclass_obj) {
    o -> __o_superclass = EXPR_superclass_obj = 
      get_class_object (EXPR_SUPERCLASSNAME);
  } else {
    o -> __o_superclass = EXPR_superclass_obj;
  }

  o -> scope = ARG_VAR;

  *(o -> __o_name) = 0;
  for (i = argbuf -> start_idx; i >= argbuf -> end_idx; --i) {
    m = argbuf -> m_s[i];

    /* the eval keyword has no effect here, so just remove it. */
    if (i == argbuf -> start_idx) {
      if (str_eq (M_NAME(m), "eval")) {
	--i;
	while (M_ISSPACE(argbuf -> m_s[i]))
	  --i;
      }
      m = argbuf -> m_s[i];
    }

    switch (M_TOK(m))
      {
      case NEWLINE: case CR: case LF:
	strcat (o -> __o_name, " ");
	continue;
	break;
      case LABEL:
	if (have_ref (M_NAME(m)) &&
	    !(m -> attrs & TOK_CVAR_REGISTRY_IS_OUTPUT)) {
	  strcat (o -> __o_name, fmt_getRef (M_NAME(m), buf_out));
	  continue;
	}
	if (argblk) {
	  if ((cvar = get_local_var (M_NAME(m))) != NULL) {
	    scratch_msg = new_message ();
	    if (!(m -> attrs & TOK_CVAR_REGISTRY_IS_OUTPUT)) {
	      translate_argblk_cvar_arg_1 (scratch_msg, cvar);
	      m -> attrs |= TOK_CVAR_REGISTRY_IS_OUTPUT;
	    } else {
	      argblk_CVAR_name_to_msg (scratch_msg, cvar);
	    }
	    op_precedence_fmt (argbuf -> m_s, i, scratch_msg -> name);
	    strcat (o -> __o_name, M_NAME(scratch_msg));
	    __xfree (MEMADDR(scratch_msg));
	  } else {
	    strcat (o -> __o_name, M_NAME(m));
	  }
	} else {
	  strcat (o -> __o_name, M_NAME(m));
	}
	break;
      case PATTERN:
	esc_expr_and_pattern_quotes (M_NAME(m), buf_out);
	strcat (o -> __o_name, buf_out);
	continue;
	break;
      default:
	strcat (o -> __o_name, M_NAME(m));
	break;
      }
  }
  o -> __o_value = strdup (o -> __o_name);

  return o;
}

static OBJECT *CFUNCTION_class_obj = NULL;
static OBJECT *CFUNCTION_superclass_obj = NULL;

OBJECT *create_arg_CFUNCTION_object (char *name_value) {

  OBJECT *o;

  if ((o = (OBJECT *)__xalloc (object_size)) == NULL) {
    printf ("create_arg_CFUNCTION_object: %s\n", strerror (errno));
    exit (EXIT_FAILURE);
  }

  /*
   *  The longest a value should be is the length of a 
   *  literal, which is the same as the name when an
   *  argument object is created.
   */
  if ((o -> __o_value = (char *)__xalloc (strlen (name_value) + 1)) == NULL) {
    printf ("create_arg_CFUNCTION_object: %s\n", strerror (errno));
    exit (EXIT_FAILURE);
  }

#ifndef __sparc__
  o -> sig = 0xd3d3d3;
#else
  strcpy (o -> sig, "OBJECT");
#endif

  strcpy (o -> __o_classname, CFUNCTION_CLASSNAME);
  if (!CFUNCTION_class_obj) {
    o -> __o_class = CFUNCTION_class_obj = get_class_object (CFUNCTION_CLASSNAME);
  } else {
    o -> __o_class = CFUNCTION_class_obj;
  }
  strcpy (o -> __o_superclassname, CFUNCTION_SUPERCLASSNAME);
  if (!CFUNCTION_superclass_obj) {
    o -> __o_superclass = CFUNCTION_superclass_obj = 
      get_class_object (CFUNCTION_SUPERCLASSNAME);
  } else {
    o -> __o_superclass = CFUNCTION_superclass_obj;
  }
  strcpy (o -> __o_name, name_value);

  o -> scope = ARG_VAR;

  strcpy (o -> __o_value, name_value);

  return o;
}
