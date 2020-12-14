/* $Id: mthdrep.c,v 1.1.1.1 2020/12/13 14:51:02 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014-2016 Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

static METHOD *mrp_last_method;
static OBJECT *mrp_last_class;

extern I_PASS interpreter_pass;    /* Declared in main.c.               */

METHOD *find_first_instance_method (OBJECT *class_object, char *name, 
				    OBJECT **class_return) { 

  OBJECT *superclass;
  METHOD *m;
  /* char selector[MAXLABEL]; */
  /* int selector_length; */

  if (!IS_OBJECT(class_object)) return NULL;

  for (m = class_object -> instance_methods; m; m = m -> next) {
    if (str_eq (m -> name, name)) {
      mrp_last_method = m;
      mrp_last_class = class_object;
      *class_return = class_object;
      return m;
    }
  }

  for (superclass = class_object -> __o_superclass;
       superclass;
       superclass = superclass -> __o_superclass) {

    for (m = superclass -> instance_methods; m; m = m -> next) {
      if (str_eq (m -> name, name)) {
	mrp_last_method = m;
	mrp_last_class = superclass;
	*class_return = superclass;
	return m;
      }
    }

  }

  return NULL;
}

METHOD *find_next_instance_method (OBJECT *o, char *name, 
				   OBJECT **class_return) { 

  OBJECT *class;
  METHOD *m, *l_last_method;
  char selector[MAXLABEL];
  int selector_length;

  class = mrp_last_class;

  strcatx (selector, class -> __o_name, INSTANCE_SELECTOR, name, NULL);
  selector_length = strlen (selector);

  for (m = mrp_last_method -> next; m; m = m -> next) {

    if (!m)
      return NULL;

    if (!strncmp (m -> selector, selector, selector_length)) {
      mrp_last_method = m;
      mrp_last_class = class;
      *class_return = class;
      return m;
    }
  }

  l_last_method = NULL;

  /* This can report methods repeatedly, or miss some of them,
     but there's nothing yet to test it on. */

  for (class = mrp_last_class -> __o_superclass; class;
       class = class -> __o_superclass) {

    strcatx (selector, class -> __o_name, INSTANCE_SELECTOR, name, NULL);
    selector_length = strlen (selector);

    if (mrp_last_method)
      l_last_method = mrp_last_method;
    else
      l_last_method = class -> instance_methods;

    for (m = l_last_method; m; m = m -> next) {

      if (!m)
	return NULL;

      if (!strncmp (m -> selector, selector, selector_length)) {
	mrp_last_method = m;
	mrp_last_class = class;
	*class_return = class;
	return m;
      }
    }

    mrp_last_method = NULL;
  }

  return NULL;
}

void report_method_argument_mismatch (MESSAGE *m_orig, OBJECT *class_object, 
				      char *name, int n_args_declared) {

  METHOD *m;
  OBJECT *class_return;

  if (interpreter_pass == expr_check)
    return;

  if ((m = find_first_instance_method (class_object, name, 
				       &class_return)) != NULL) {

    if ((n_args_declared != m -> n_params) && !m -> varargs) {

      warning (m_orig, "Method, \"%s,\" (class %s) argument mismatch.",
	       name, class_object -> __o_name);
      warning (m_orig, "Expression declares %d arguments.", n_args_declared);
      warning (m_orig, "Method, \"%s,\" (class %s) uses %d arguments.", 
	       m -> name, class_return -> __o_name, m -> n_params);

    }
  } else {
    return;
  }

  while ((m = find_next_instance_method (class_object, name,
					 &class_return)) != NULL) {
    warning (m_orig, "Method, \"%s,\" (class %s) uses %d arguments.", 
	     m -> name, class_return -> __o_name, m -> n_params);

  }

}

