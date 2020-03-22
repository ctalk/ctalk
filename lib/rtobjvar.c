/* $Id: rtobjvar.c,v 1.6 2020/03/22 18:15:17 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2019  Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

extern int __cleanup_deletion;

extern HASHTAB instancemethodhash;

extern EXPR_PARSER *expr_parsers[MAXARGS+1];
extern int expr_parser_ptr;

extern VARENTRY *__ctalk_dictionary;
extern VARENTRY *__ctalk_last_object;

extern OBJECT *__ctalk_classes;
extern OBJECT *__ctalk_last_class;

static inline char *method_hash_key (char *classname, char *methodname,
			      char *key_out) {
  char *s = classname;
  char *t = key_out;
  while (*t++ = *s++) ;
  --t, *t++ = ':';
  s =  methodname;
  while (*t++ = *s++) ;
  --t, *t = '\0';
  return key_out;
}

/*
 *  Add or replace an instance variable.  If the variable
 *  we're replacing is, "value," then we only use the,
 * "value" instance variable of, "var."
 *
 *  NOTE - The instance variable might need to get the same 
 *  reference count as the receiver if it is not in a 
 *  dictionary.
 *
 *  We must be careful here because the, "value," instance
 *  variable does not itself contain instance variables, and
 *  in the cases where this function receives a, "value,"
 *  instance variable as the third argument, var and the new 
 *  value_var need to be the same, and the function needs to 
 *  check for it.
 *
 *  NOTE  (2) - This function assumes that the, "value," instance 
 *  variable exists, so it should not be called by a 
 *  constructor.  Instead, the constructors simply point
 *  object -> instancevars to the, "value," instance variable 
 *  object.  
 */
OBJECT *__ctalkAddInstanceVariable (OBJECT *receiver, char *name, OBJECT *var) {

  OBJECT *t, *old_var, *value_var, *new_var, *var_copy;
  OBJECT *__r1, *__r2;

  if (!IS_OBJECT(receiver)) {
    __ctalkExceptionInternal (NULL, invalid_receiver_x, "NULL receiver in __ctalkAddInstanceVariable", 0);
    __ctalkPrintExceptionTrace ();
    __ctalkHandleRunTimeException ();
    return NULL;
  }

  if (!IS_OBJECT(var)) {
    __ctalkExceptionInternal (NULL, invalid_operand_x, "NULL argument in __ctalkAddInstanceVariable", 0);
    __ctalkPrintExceptionTrace ();
    __ctalkHandleRunTimeException ();
    return NULL;
  }

  old_var = NULL;

  for (t = receiver -> instancevars; 
       IS_OBJECT(t) && (old_var == NULL); t = t -> next) {
    if (!strcmp (t -> __o_name, name))
      old_var = t;
  }

  /*
   *  If we're returning from a method, then var might be 
   *  the value var itself, so first check whether the value_var
   *  is an instance variable of var, and then if var is the value
   *  variable itself.
   */
  if (!strcmp (name, "value")) {
    value_var =
      var -> instancevars ? var ->  instancevars : var;
    __ctalkCopyObject (OBJREF(value_var), OBJREF(new_var));
    __objRefCntSet (OBJREF(new_var), receiver -> nrefs);
    __ctalkSetObjectScope(new_var, receiver -> scope);
    /* This alias is okay (really). */
    value_var = new_var;
    new_var -> attrs = OBJECT_IS_VALUE_VAR;
  } else {
    value_var = NULL;
  }

  /*
   *  Replace a previous object, making sure that it is not
   *  in a dictionary.
   */
  if (IS_OBJECT (old_var)) {

    if (!strcmp (old_var -> __o_name, "value")) {
      
      if (receiver -> instancevars == old_var)
	receiver -> instancevars = value_var;
      value_var -> next = old_var -> next; 
      if (value_var -> next) value_var -> next -> prev = value_var;
      value_var -> prev = old_var -> prev;
      if (value_var -> prev) value_var -> prev -> next = value_var;
      __objRefCntSet (OBJREF (value_var), receiver -> nrefs);
      value_var -> __o_p_obj = receiver;
      value_var -> attrs = OBJECT_IS_VALUE_VAR;
      /*
       *  The old variable _should_ be unique, so delete it.
       */
      __ctalkDeleteObject (old_var);

    } else {
      OBJECT *v, *v_next;
      for (v = receiver -> instancevars; IS_OBJECT(v);) {
	v_next = v -> next;
	var_copy = NULL;
	if (!strcmp (v -> __o_name, name)) {
	  __ctalkCopyObject (OBJREF(var), OBJREF(var_copy));
	  var_copy -> __o_p_obj = receiver;
	  var_copy -> prev = v -> prev;
	  var_copy -> next = v -> next;
	  /* Just in case we get a constant as an argument. */
	  memset (var_copy -> __o_name, 0, MAXLABEL);
	  strncpy (var_copy -> __o_name, name, MAXLABEL - 1);
	  if (v -> prev && v -> prev -> next) v -> prev -> next = var_copy;
	  if (v -> next && v -> next -> prev) v -> next -> prev = var_copy;
	  /*
	   *  If the new instance variable also contains a new
	   *  unique object reference, then we need to delete the
	   *  old object reference if it has a CVAR_VAR_ALIAS_COPY scope.
	   */
	  if (v -> __o_p_obj && var_copy -> __o_p_obj) {
	    if (v -> __o_p_obj == var_copy -> __o_p_obj) {
	      __r1 = obj_ref_str 
		((v -> instancevars) ? v -> instancevars -> __o_value :
		 v -> __o_value);
	      if (IS_OBJECT (__r1) && __r1 -> scope & CVAR_VAR_ALIAS_COPY) {
		__ctalkDeleteObject (__r1);
	      }
	      if (IS_OBJECT(__r1) && __r1 -> attrs & OBJECT_IS_GLOBAL_COPY) {
		__ctalkDeleteObject (__r1);
	      }
	    } 
	    __ctalkDeleteObject (v);
	  } else {
	    __ctalkDeleteObject (v);
	  }

	}
	if ((v = v_next) == NULL)
	  break;
      }
    }

    __ctalkDeleteObject (old_var);
    return var_copy;

  } else {

    /*
     *  As a special case, and hopefully faster and more robust if
     *  there are references to the value object, if both old and new
     *  variables are, "value," just copy the value over.
     */
    if (!strcmp (receiver -> __o_name, "value") &&
	!strcmp (var -> __o_name, "value")) {
      __ctalkSetObjectValue (receiver, var -> __o_value);
      if (IS_OBJECT(new_var))
	__ctalkDeleteObject (new_var);
      return receiver;
    }

    /*
     *   Add the variable object.   Create a copy of the variable 
     *   object first.
     */

    __ctalkCopyObject (OBJREF(var), OBJREF(new_var));
    if (new_var) {
      __objRefCntSet (OBJREF (new_var), receiver -> nrefs);
      __ctalkSetObjectName (new_var, name);
      new_var -> __o_p_obj = receiver;
      new_var -> attrs = var -> attrs;
      if (!receiver -> instancevars) {
	receiver -> instancevars = new_var;
      } else {
	for (t = receiver -> instancevars; 
	     IS_OBJECT(t) && t -> next; t = t -> next)
	  ;
	t -> next = new_var;
	new_var -> prev = t;
	new_var -> next = NULL;
      }
      return var;
    } else {
      return NULL;
    }
  }
}

OBJECT *__ctalkAddClassVariable (OBJECT *class_obj, char *name, OBJECT *var) {

  OBJECT *t, *old_var;

  old_var = NULL;

  for (t = class_obj -> classvars; 
       t && (old_var == NULL); t = t -> next) {
    if (!strcmp (t -> __o_name, name))
      old_var = t;
  }

  /*
   *  Replace a previous object, making sure that it is not
   *  in a dictionary.
   */
  if (IS_OBJECT (old_var)) {

    hash_remove_class_var (old_var);

    __ctalkSetObjectName (var, name);
    if (class_obj -> classvars == old_var)
      class_obj -> classvars = var;
    var -> next = old_var -> next;
    var -> prev = old_var -> prev;
    if (old_var -> prev) old_var -> prev -> next = var;
    if (old_var -> next) old_var -> next -> prev = var;
    __objRefCntSet (OBJREF(var), class_obj -> nrefs);
    var -> __o_p_obj = class_obj;
    __ctalkDeleteObject (old_var);

  } else {
    __objRefCntSet (OBJREF (var), class_obj -> nrefs);
    __ctalkSetObjectName (var, name);

    if (!class_obj -> classvars) {
      class_obj -> classvars = var;
    } else {
      for (t = class_obj -> classvars; t && t -> next; t = t -> next)
	;
      t -> next = var;
      var -> prev = t;
    }

  }
  
  hash_class_var (var);

  return var;

}

OBJECT *__ctalkGetInstanceVariableByName (char *rcvr_name, char *var_name,
					   int error) {
  OBJECT *rcvr_obj;

  if ((rcvr_obj = __ctalk_get_arg_tok (rcvr_name)) == NULL)
    _error ("__ctalkGetInstanceVariableByName: Undefined object (%s).\n",
	    rcvr_name);

  /*
   *  In case the receiver is already an instance variable value.
   */
  if (!strcmp (rcvr_obj->__o_name, var_name) && 
      rcvr_obj->instancevars == NULL) 
    return rcvr_obj;

  return __ctalkGetInstanceVariable (rcvr_obj, var_name, error);

}

OBJECT *__ctalkGetInstanceVariable (OBJECT *receiver, char *name, 
				 int error) {
  OBJECT *instance_var;
  if (!IS_OBJECT(receiver)) {
    if (error) {
      __ctalkExceptionInternal (NULL, invalid_receiver_x, "NULL receiver in __ctalkGetInstanceVariable", 0);
      __ctalkPrintExceptionTrace ();
      __ctalkHandleRunTimeException ();
    }
    return NULL;
  }
  if (IS_OBJECT (receiver -> instancevars)) {
    for (instance_var = receiver -> instancevars; 
	 IS_OBJECT(instance_var); 
	 instance_var = instance_var -> next) {
      if (!strcmp (instance_var -> __o_name, name))
 	return instance_var;
    }
  }

  if (str_eq (receiver -> __o_name, name) &&
      (!receiver -> instancevars))
    return receiver;


  if (error) {
    if (__ctalkGetExceptionTrace ()) {
      restore_stdin_stdout_termios ();
      __warning_trace ();
      _error ("__ctalkGetInstanceVariable: %s (Class %s) variable, \"%s,\" undefined.\n", receiver -> __o_name, receiver -> CLASSNAME, name);
    }
  }

  return NULL;
}

OBJECT *__ctalkGetInstanceVariableComplex (OBJECT *receiver, char *name, 
				 int error) {
  OBJECT *instance_var;
  if (receiver && IS_OBJECT (receiver -> instancevars)) {
    for (instance_var = receiver -> instancevars; 
	 IS_OBJECT(instance_var); 
	 instance_var = instance_var -> next) {
      if (!strcmp (instance_var -> __o_name, name))
 	return instance_var;
    }
  }

  if (!strcmp (receiver -> __o_name, name) &&
      (!receiver -> instancevars) &&
      (receiver -> __o_p_obj))
    return receiver;

  if (receiver && IS_OBJECT (receiver -> instancevars)) {
    for (instance_var = receiver -> instancevars; 
	 IS_OBJECT(instance_var); 
	 instance_var = instance_var -> next) {
      OBJECT *i_var_1;
      if ((i_var_1 = 
	   __ctalkGetInstanceVariableComplex (instance_var,
					      name,
					      error)) != NULL)
	return i_var_1;
    }
  }

  if (error)
    _error ("__ctalkGetInstanceVariable: %s (Class %s) variable, \"%s,\" undefined.\n", receiver -> __o_name, receiver -> CLASSNAME, name);

  return NULL;
}

OBJECT *__ctalkGetClassVariable (OBJECT *receiver, char *name, 
				 int error) {
  OBJECT *class_var, *receiver_class_object,
    *receiver_superclass_object;

  if (!IS_OBJECT(receiver)) return NULL;

  if (((receiver_class_object = receiver -> __o_class) == NULL) ||
      (receiver -> CLASSNAME[0] == 0))
    return NULL;

  if (IS_OBJECT(receiver_class_object) && 
      IS_OBJECT (receiver_class_object -> classvars)) {
    for (class_var = receiver_class_object -> classvars; 
	 class_var; class_var = class_var -> next) {
      if (!strcmp (class_var -> __o_name, name))
 	return class_var;
    }
  }

  if (IS_OBJECT(receiver_class_object -> __o_superclass)) {
    for (receiver_class_object = receiver_class_object -> __o_superclass;
	 IS_OBJECT(receiver_class_object);
	 receiver_class_object = receiver_class_object -> __o_superclass) {
      if (IS_OBJECT (receiver_class_object -> classvars)) {
	for (class_var = receiver_class_object -> classvars; 
	     class_var; class_var = class_var -> next) {
	  if (!strcmp (class_var -> __o_name, name))
	    return class_var;
	}
      }
    }
  }

  if (error)
    _error ("__ctalkGetClassVariable: %s (Class %s) variable, \"%s,\" undefined.\n", receiver -> __o_name, receiver -> CLASSNAME, name);

  return NULL;
}

OBJECT *__ctalkFindClassVariable (char *name, int error) {

  OBJECT *var;

  if ((var = __ctalk_find_classvar (name, NULL)) == NULL) {
    if (error)
      _error ("__ctalkFindClassVariable: \"%s,\" undefined.\n", name);
  }

  return var;

}

/*
 *  Check the class object for an instance variable of the 
 *  name given as the argument.
 */
int __ctalkIsClassVariableOf (OBJECT *class, const char *varname) {
  OBJECT *o;
  for (o = class -> classvars; o; o = o -> next)
    if (!strcmp (o -> __o_name, (char *)varname))
      return TRUE;

  return FALSE;
}

/*
 *  Check the class object for an instance variable of the 
 *  name given as the argument.
 */
int __ctalkIsInstanceVariableOf (OBJECT *class, const char *varname) {
  OBJECT *o;
  for (o = class -> instancevars; o; o = o -> next)
    if (!strcmp (o -> __o_name, (char *)varname))
      return TRUE;

  if (class -> __o_superclass)
    return __ctalkIsInstanceVariableOf (class -> __o_superclass, varname);
  else
    return FALSE;
}

/* like the fn above, except it skips the "value" instance var in the
   search */
bool is_instance_variable_of_no_value (OBJECT *class, const char *varname) {
  OBJECT *o /* , *o_superclass_tmp */;
  if (!class -> instancevars -> next)
    return false;
  for (o = class -> instancevars; o; o = o -> next)
    if (!strcmp (o -> __o_name, (char *)varname))
      return TRUE;

  if (class -> __o_superclass)
    return __ctalkIsInstanceVariableOf (class -> __o_superclass, varname);
  else
    return FALSE;
}

void __remove_var_from_dictionary (VARENTRY *var) {

  if (var == __ctalk_last_object) {
    if (IS_OBJECT(var -> var_object)) {
      var -> var_object -> __o_vartags -> tag = NULL;
    }
    if (__ctalk_last_object == __ctalk_dictionary) {
      delete_varentry (__ctalk_dictionary);
      __ctalk_dictionary = __ctalk_last_object = NULL;
    } else {
      if (__ctalk_last_object -> prev)
	__ctalk_last_object -> prev -> next = __ctalk_last_object -> next;
      if (__ctalk_last_object -> next)
	__ctalk_last_object -> next -> prev = __ctalk_last_object -> prev;
      __ctalk_last_object -> next = NULL;
      delete_varentry (var);
    }
  } else {
   _warning ("__remove_var_from_dictionary: var is not most recent object.\n");
  }
}

/*
 *  this should no longer be needed.
 */
OBJECT *__var_primitive_constructor (char *varname, 
				     METHOD *constructor_method,
				     OBJECT *class_object, 
				     OBJECT *constructor_class_object) {
  VARENTRY *var;
  OBJECT *var_object;

  __add_arg_object_entry_frame 
    (constructor_method,
     __ctalkCreateObjectInit (varname, 
			      class_object -> __o_name,
			      _SUPERCLASSNAME(class_object),
			      LOCAL_VAR, NULL));
  __ctalk_receiver_push (class_object);
  __ctalk_arg (constructor_class_object -> __o_name, 
	       constructor_method -> name, 
	       constructor_method -> n_params, varname);
  __ctalk_primitive_method (constructor_class_object -> __o_name,
			    constructor_method -> name, 0);
  __ctalk_receiver_pop ();
  constructor_method -> args[0] = NULL;
  var = __ctalk_last_object;
  var_object = var -> var_object;
  __remove_var_from_dictionary (var);
  delete_varentry (var);
  return var_object;
}

static OBJECT *__var_constructor (char *varname, METHOD *constructor_method,
				  OBJECT *class_object, 
				  OBJECT *constructor_class_object,
				  char *init_expr) {
  OBJECT *var_result;
  OBJECT *arg_name_object;

  arg_name_object = 
    __ctalkCreateObjectInit (varname, 
			     constructor_class_object -> __o_name,
			     _SUPERCLASSNAME(constructor_class_object),
			     LOCAL_VAR, init_expr);

  __ctalk_arg (constructor_class_object -> __o_name, "new", 1, varname);
  var_result = 
    __ctalk_method (constructor_class_object -> __o_name, 
		    constructor_method -> cfunc,
		    constructor_method -> name);
  __ctalk_arg_pop_deref ();
  if (arg_name_object != var_result) {
    /*
     *  var_result should also be removed from the dictionary 
     *  if it has the same name and class name as the argument 
     *  object.  Check if the result is still in the dictionary.
     *  If the argument and the result are the same object, don't
     *  delete.
     */
    __ctalkDeleteObjectInternal (arg_name_object);
    if (var_result -> __o_vartags -> tag == __ctalk_last_object) {
      if (var_result -> __o_vartags -> tag) {
	__remove_var_from_dictionary (var_result -> __o_vartags -> tag);
      }
    }
  }
  if (var_result -> instancevars)
    var_result -> instancevars -> __o_p_obj = var_result;
  return var_result;
}

OBJECT *__ctalkDefineInstanceVariable (char *classname, char *varname, 
				   char *varclass, char *init_expr) {
  OBJECT *class_object,
    *var_class_object,
    *var, *var_ptr, *o_t, *constructor_class_object = NULL;
  METHOD *m_t, *constructor_method = NULL;
  char hash_key[MAXLABEL];

  if ((class_object = __ctalkGetClass (classname)) == NULL) {
    _warning ("__ctalkDefineInstanceVariable: Undefined class %s.\n", 
	      classname);
    return NULL;
  }

  if ((var_class_object = __ctalkGetClass (varclass)) == NULL) {
    _warning ("__ctalkDefineInstanceVariable: Undefined variable class %s.\n", 
	      classname);
    return NULL;
  }

  if (!IS_OBJECT(var_class_object -> __o_superclass)) {
    constructor_class_object = var_class_object;
  } else {
    for (o_t = var_class_object;
	 o_t && !constructor_method; o_t = o_t -> __o_superclass) {
      method_hash_key (o_t -> __o_name, "new", hash_key);
      if (_hash_get (instancemethodhash, hash_key)) {
	for (m_t = o_t -> instance_methods; m_t && !constructor_method; 
	     m_t = m_t -> next)
	  if (IS_CONSTRUCTOR(m_t)) {
	    constructor_method = m_t;
	    constructor_class_object = o_t;
	  }
      }
    }
  }

  /*
   *  Check if the constructor is Object : new, which is a primitive.
   */
  if (!IS_OBJECT(constructor_class_object -> __o_superclass)) {
    var = __ctalkCreateObjectInit (varname, 
				   class_object -> __o_name,
				   _SUPERCLASSNAME(class_object),
				   LOCAL_VAR, init_expr);
    __ctalkSetObjectValueClass (var, var_class_object);
    __ctalkInstanceVarsFromClassObject (var);
    if (var_class_object -> attrs & INT_BUF_SIZE_INIT) {
      int intval;
      __xfree (MEMADDR(var -> __o_value));
      __xfree (MEMADDR(var -> instancevars -> __o_value));
      var -> __o_value = __xalloc (INTBUFSIZE);
      var -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
      intval = atoi (init_expr);
      memcpy (var -> __o_value, &intval, sizeof (int));
      memcpy (var -> instancevars -> __o_value,
	      var -> __o_value, sizeof (int));
      var -> attrs |= OBJECT_VALUE_IS_BIN_INT;
      var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
    } else if (var_class_object -> attrs & BOOL_BUF_SIZE_INIT) { /***/
      int intval;
      __xfree (MEMADDR(var -> __o_value));
      __xfree (MEMADDR(var -> instancevars -> __o_value));
      var -> __o_value = __xalloc (INTBUFSIZE);
      var -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
      intval = atoi (init_expr);
      intval = (intval) ? 1 : 0;
      memcpy (var -> __o_value, &intval, sizeof (int));
      memcpy (var -> instancevars -> __o_value,
	      var -> __o_value, sizeof (int));
      var -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
      var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
    } else if (var_class_object -> attrs & LONGLONG_BUF_SIZE_INIT) {
      long long int llval;
      __xfree (MEMADDR(var -> __o_value));
      __xfree (MEMADDR(var -> instancevars -> __o_value));
      var -> __o_value = __xalloc (LLBUFSIZE);
      var -> instancevars -> __o_value = __xalloc (LLBUFSIZE);
#ifndef _ppc_
      llval = atoll (init_expr);
#else
      llval = atol (init_expr);
#endif      
      memcpy (var -> __o_value, &llval, sizeof (long long int));
      memcpy (var -> instancevars -> __o_value,
	      var -> __o_value, sizeof (long long int));
      var -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
      var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
    } else if (var_class_object -> attrs & SYMBOL_BUF_SIZE_INIT) {
      unsigned long int val;
      __xfree (MEMADDR(var -> __o_value));
      __xfree (MEMADDR(var -> instancevars -> __o_value));
      var -> __o_value = __xalloc (PTRBUFSIZE);
      var -> instancevars -> __o_value = __xalloc (PTRBUFSIZE);
      val = strtoul (init_expr, NULL, 0);
      memcpy (var -> __o_value, &val, sizeof (unsigned long int));
      memcpy (var -> instancevars -> __o_value,
	      var -> __o_value, sizeof (unsigned long int));
      var -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
      var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
    }
  } else {
    var = __var_constructor (varname, constructor_method, class_object,
			     constructor_class_object, init_expr);
    __ctalkSetObjectValueClass (var, var_class_object);
    if (constructor_class_object != class_object) { /***/
      /* Make sure we get the class' instance variables defined.
	 First we set the class of the var to its actual class
	 so we can use __ctalkInstanceVarsFromClassObject ... */
      var -> __o_class = var_class_object;
      var -> __o_superclass = var_class_object -> __o_superclass;
      __ctalkInstanceVarsFromClassObject (var);
      /* ... then we set it to the member class before finishing. */
      var -> __o_class = class_object;
      var -> __o_superclass = class_object -> __o_superclass;
    }
  }

  __objRefCntSet (OBJREF(var), class_object -> nrefs);

#ifdef USE_CLASSNAME_STR
  strcpy (var -> __o_classname, class_object -> __o_name);
#endif
#ifdef USE_SUPERCLASSNAME_STR
  strcpy (var -> __o_superclassname, _SUPERCLASSNAME(class_object));
#endif

  for (var_ptr = class_object -> instancevars; var_ptr -> next; 
       var_ptr = var_ptr -> next)
    ;
  var_ptr -> next = var;
  var -> prev = var_ptr;

  return NULL;
}

OBJECT *var_check;

OBJECT *__ctalkDefineClassVariable (char *classname, char *varname, 
				   char *varclass, char *init_expr) {
  OBJECT *class_object, *constructor_class_object = NULL,
    *var_class_object, *o_t, *var, *var_ptr;
  METHOD *m_t, *constructor_method = NULL;
  int bool_value;
  unsigned long symbol_value;

  if ((class_object = __ctalkGetClass (classname)) == NULL) {
    _warning ("__ctalkDefineClassVariable: Undefined class %s.\n", 
	      classname);
    return NULL;
  }

  if (strcmp (classname, varclass)) {
    if ((var_class_object = __ctalkGetClass (varclass)) == NULL) {
      _warning ("__ctalkDefineClassVariable: Undefined variable class %s.\n", 
		classname);
      return NULL;
    }
  } else {
    var_class_object = class_object;
  }

  /*
   *  If the variable class is the same as the class we are 
   *  defining, then we have to start looking for a constructor
   *  in the superclass.  In this case, we will add the instance
   *  variables manually below.
   */
  for (o_t = ((class_object == var_class_object) ? 
	 var_class_object -> __o_superclass : var_class_object -> __o_class);
       o_t && !constructor_method; o_t = o_t -> __o_superclass) {
    
    for (m_t = o_t -> instance_methods; m_t; m_t = m_t -> next)
      if (IS_CONSTRUCTOR(m_t)) {
	constructor_method = m_t;
	constructor_class_object = o_t;
      }
  }


  /*
   *  If in the unlikely event the class of the class variable is
   *  Object (or something even more strange), try to use that
   *  constructor.
   */
  if (!constructor_method) {
    o_t = class_object;
    for (m_t = o_t -> instance_methods; m_t; m_t = m_t -> next)
      if (IS_CONSTRUCTOR(m_t)) {
	constructor_method = m_t;
	constructor_class_object = o_t;
      }
  }

  /*
   *  Check if the constructor is Object : new, which is a primitive.
   */
  if (!IS_OBJECT (constructor_class_object -> __o_superclass)) {
    var = __ctalkCreateObjectInit (varname, 
			     class_object -> __o_name,
				   _SUPERCLASSNAME(class_object),
			     LOCAL_VAR, init_expr);
  } else {
    var = __var_constructor (varname, constructor_method, class_object,
			     constructor_class_object, init_expr);
  }

  __objRefCntSet (OBJREF(var), class_object -> nrefs);

#ifdef USE_CLASSNAME_STR
  strcpy (var -> __o_classname, var_class_object -> __o_name);
#endif
#ifdef USE_SUPERCLASSNAME_STR
  strcpy (var -> __o_superclassname, _SUPERCLASSNAME(var_class_object));
#endif
  var -> __o_class = var_class_object -> __o_class;
  var -> __o_superclass = var_class_object -> __o_superclass;
  if (var_class_object -> attrs & INT_BUF_SIZE_INIT) {
    __xfree (MEMADDR(var -> __o_value));
    __xfree (MEMADDR(var -> instancevars -> __o_value));
    var -> __o_value = __xalloc (INTBUFSIZE);
    var -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
    if (init_expr && *init_expr) {
      errno = 0;
      if (*init_expr != '\0') {
	INTVAL(var -> __o_value) = 
	  INTVAL(var -> instancevars -> __o_value) =
	  strtol (init_expr, NULL, 0);
      }
      if (errno) {
	fprintf (stderr, "ctalk: can't translate %s to an int.\n",
		 *init_expr ? init_expr : NULLSTR);
      }
    }
    var -> attrs |= OBJECT_VALUE_IS_BIN_INT;
    var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
    __ctalkSetObjectValueClass (var, var_class_object);
  } else if (var_class_object -> attrs & LONGLONG_BUF_SIZE_INIT) {
    __xfree (MEMADDR(var -> __o_value));
    __xfree (MEMADDR(var -> instancevars -> __o_value));
    var -> __o_value = __xalloc (LLBUFSIZE);
    var -> instancevars -> __o_value = __xalloc (LLBUFSIZE);
    errno = 0;
    if (*init_expr != '\0') {
      *(long long int *)var -> __o_value = 
	*(long long int *)var -> instancevars -> __o_value =
	strtoll (init_expr, NULL, 0);
    }
    if (errno) {
      fprintf (stderr, "ctalk: can't translate %s to a long long int.\n",
	       *init_expr ? init_expr : NULLSTR);
    }
    var -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
    var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
    __ctalkSetObjectValueClass (var, var_class_object);
  } else if (var -> __o_class -> attrs & CHAR_BUF_SIZE_INIT) {
    /* E.g., Character class - This needs to be initialized
       like a binary int value buffer, so we can treat them
       the same when needed. */
    __xfree (MEMADDR(var -> __o_value));
    __xfree (MEMADDR(var -> instancevars -> __o_value));
    var -> __o_value = __xalloc (INTBUFSIZE);
    var -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
    if (init_expr) {
      strcpy (var -> __o_value, init_expr);
      strcpy (var -> instancevars -> __o_value, init_expr);
    }
    __ctalkSetObjectValueClass (var, var_class_object);
  } else if (var -> __o_class -> attrs & BOOL_BUF_SIZE_INIT) {
    /* We'll just use an int-size buf here too, so we can
       just use ints */
    __xfree (MEMADDR(var -> __o_value));
    __xfree (MEMADDR(var -> instancevars -> __o_value));
    var -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
    var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
    var -> __o_value = __xalloc (INTBUFSIZE);
    var -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
    if (init_expr && *init_expr) {
      errno = 0;
      bool_value = strtol (init_expr, NULL, 0);
      if (!errno) {
	*var -> __o_value = 
	  *var -> instancevars -> __o_value = (bool_value) ? 1 : 0;
      } else if (*init_expr == '\0') {
      *var -> __o_value = 
	*var -> instancevars -> __o_value = 0;
      } else {
	*var -> __o_value = 
	  *var -> instancevars -> __o_value = 1;
      }
    }
    /* NOTE: We do not need to call  __ctalkSetObjectValueClass here. */
  } else if (var -> __o_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
    __xfree (MEMADDR(var -> __o_value));
    __xfree (MEMADDR(var -> instancevars -> __o_value));
    var -> __o_value = __xalloc (PTRBUFSIZE);
    var -> instancevars -> __o_value = __xalloc (PTRBUFSIZE);
    var -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
    var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
    if (*init_expr) {
      errno = 0;
      symbol_value = strtoul (init_expr, NULL, 0);
      if (errno) {
	fprintf (stderr, "ctalk: can't translate %s to a pointer value.\n",
		 *init_expr ? init_expr : NULLSTR);
      }
      *(unsigned long *)var -> instancevars -> __o_value = symbol_value;
      *(unsigned long *)var -> __o_value = symbol_value;
    }
    __ctalkSetObjectValueClass (var, var_class_object);
  } else {
    __ctalkSetObjectValueClass (var, var_class_object);
  }
  __ctalkSetObjectScope (var, class_object -> scope);

  /*
   *  Add the instance variables from this class, except for
   *  value.
   */
  if (class_object == var_class_object) {
    __ctalkInstanceVarsFromClassObject (var);
  }

  if (class_object -> classvars == NULL) {
    class_object -> classvars = var;
  } else {
    for (var_ptr = class_object -> classvars; var_ptr -> next; 
	 var_ptr = var_ptr -> next)
      ;
    var_ptr -> next = var;
    var -> prev = var_ptr;
  }

  hash_class_var (var);

  return NULL;
}

OBJECT *__ctalkFindMemberClass (OBJECT *o) {
  OBJECT *r, *t;
  for (r = __ctalk_classes; r; r = r -> next) {
    for (t = r -> classvars; t; t = t -> next) {
      if (t == o) return r;
    }
  }
  return NULL;
}

static void copy_instance_vars_from_class (OBJECT *class, OBJECT *o) {
  OBJECT *t, *var, *new_var;

  if (IS_OBJECT (class -> instancevars -> next)) {
    /* If the object has the first of the classes' instance variables, 
       then it **should** already have all of them */
    if (__ctalkGetInstanceVariable
	(o, class -> instancevars -> next -> __o_name,
	 FALSE))
      return;
  } else {
    /* or, the class has no instance variables besides, "value." */
    return;
  }

  for (t = o -> instancevars; 
       IS_OBJECT(t) && t -> next; t = t -> next)
    ;

  for (var = class -> instancevars -> next; var; var = var -> next) {
    __ctalkCopyObject (OBJREF(var), OBJREF(new_var));
    if (new_var) {
      __objRefCntSet (OBJREF (new_var), o -> nrefs);
      new_var -> __o_p_obj = o;
      new_var -> attrs = var -> attrs;
      if (var -> instancevars -> __o_class -> attrs & INT_BUF_SIZE_INIT) {
	var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
	new_var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
      } else if (var -> instancevars -> __o_class -> attrs & BOOL_BUF_SIZE_INIT) {
	var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
	new_var -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
	__xfree (MEMADDR(new_var -> __o_value));
	new_var->__o_value = __xalloc (BOOLBUFSIZE);
	memcpy ((void *)new_var -> __o_value,
		(void *)var -> instancevars -> __o_value, BOOLBUFSIZE);
	__xfree (MEMADDR(new_var -> instancevars -> __o_value));
	new_var->instancevars->__o_value = __xalloc (BOOLBUFSIZE);
	memcpy ((void *)new_var -> instancevars -> __o_value,
		(void *)var -> instancevars -> __o_value, BOOLBUFSIZE);
      }

      t -> next = new_var;
      new_var -> prev = t;
      t = new_var;
    }
  }
}

/*
 *  Add instance variables for normal objects by finding
 *  the argument's class object.  If initializing a class
 *  variable, also search for the variable's parent class
 *  object and initialize the object from the actual 
 *  parent class.
 */
int __ctalkInstanceVarsFromClassObject (OBJECT *o) {

  OBJECT *class_object, *superclass_object;
  OBJECT *r;

  if ((class_object = ((o -> instancevars) ? 
		       o -> instancevars -> __o_class :
		       o -> __o_class)) == NULL) {
    __ctalkExceptionInternal (NULL, undefined_class_x, o -> CLASSNAME,0);
    return ERROR;
  }

  copy_instance_vars_from_class (class_object, o);
  for (superclass_object = class_object -> __o_superclass;
       superclass_object;
       superclass_object = superclass_object -> __o_superclass)
    copy_instance_vars_from_class (superclass_object, o);
  if ((r = __ctalkFindMemberClass (o)) != NULL) {
    copy_instance_vars_from_class (class_object, o);
    for (superclass_object = r -> __o_superclass;
	 superclass_object;
	 superclass_object = superclass_object -> __o_superclass)
      copy_instance_vars_from_class (superclass_object, o);
  }

  return SUCCESS;
}

int __ctalkDeleteObjectList (OBJECT *list) {
  if (!list)
    return SUCCESS;
  if (list -> __o_p_obj) {
    if (list -> __o_p_obj -> instancevars == list)
      list -> __o_p_obj -> instancevars = NULL;
    if (list -> __o_p_obj -> classvars == list)
      list -> __o_p_obj -> classvars = NULL;
  }
  /* See the comments in object.h */
  /* DELETE_OBJECT_LIST(list) */
  delete_object_list_internal (list);
  return SUCCESS;
}

int __is_instvar_of_method_return (EXPR_PARSER *p, int msg_idx) {
  int prev_msg_idx;

  if ((prev_msg_idx = 
       __ctalkPrevLangMsg (p -> m_s, msg_idx, p -> msg_frame_start))
      != ERROR) {
    /*
     *  Message that is an instance variable of a receiver returned
     *  by a method.
     */
    if (M_TOK(p -> m_s[prev_msg_idx]) == METHODMSGLABEL) {
      if (IS_OBJECT(p -> m_s[prev_msg_idx] -> receiver_obj)) {
	METHOD *method;
	OBJECT *rcvr_p;
	rcvr_p = p -> m_s[prev_msg_idx] -> receiver_obj;
	if (((method = __ctalkFindInstanceMethodByName (&rcvr_p,
					   M_NAME(p -> m_s[prev_msg_idx]),
							FALSE, ANY_ARGS)) 
	     != NULL) ||
	    ((method = __ctalkFindClassMethodByName (&rcvr_p,
					   M_NAME(p -> m_s[prev_msg_idx]),
						     FALSE, ANY_ARGS)) 
	     != NULL)) {
	  if (*(method -> returnclass)) {
	    OBJECT *__return_class_object;
	    if ((__return_class_object = __ctalkGetClass(method->returnclass))
		!= NULL) {
	      if (__ctalkIsInstanceVariableOf (__return_class_object,
					       M_NAME(p -> m_s[msg_idx])))
		return TRUE;
	    } else {
	      if (!strcmp (method->returnclass, "Any")) {
		if (IS_OBJECT (p -> m_s[prev_msg_idx] -> value_obj)) {
		  if (__ctalkIsInstanceVariableOf 
		      (p -> m_s[prev_msg_idx] -> value_obj,
		       M_NAME(p -> m_s[msg_idx])))
		    return TRUE;
		}
	      }
	    }
	  }
	}
      }
    }
  }
  return FALSE;
}

static bool object_is_not_derived (OBJECT *obj) {
  return 
    ((!(obj -> attrs & OBJECT_IS_I_RESULT)) &&
     /* This prevents us from duplicating some argument reference
	calculations, since the argument is already a reference... */
     !((obj -> scope & ARG_VAR) && IS_EMPTY_VARTAG(obj -> __o_vartags)));
  /* ... or it's a constant argument, and we're going to assign it to a
     tag originally anyway. */
}

/* If a previous method is declared to return something other than
   a String, then we _should_ assign it by reference. */
static bool string_return_needs_mutation (void) {
  EXPR_PARSER *expr_parser;
  METHOD *method, *prev_method;
  if ((expr_parser = C_EXPR_PARSER) != NULL) {
    if (expr_parser -> call_stack_level == __current_call_stack_idx () + 1) {
      if ((method = __ctalkRtGetMethod ()) != NULL) {
	if (method == expr_parser -> e_methods[expr_parser -> e_method_ptr-1]) {
	  /* We should just be able to check the previous method's return
	     class. First check that there is a previous method. */
	  if (expr_parser -> e_method_ptr >= 2) {
	    prev_method = 
	      expr_parser -> e_methods [expr_parser -> e_method_ptr - 2];
	    if (!str_eq (prev_method -> returnclass, STRING_CLASSNAME))
	      return true;
	  }
	}
      }
    }
  }
  return false;
}

/* The object contains a temporary tag created by one of the
   basicNew methods. */
static bool object_has_local_tag (OBJECT *o) {
  return o -> attrs & OBJECT_HAS_LOCAL_TAG;
}

void __ctalkStringifyName (OBJECT *src, OBJECT *dest) {
  if (!IS_OBJECT (src) || !IS_OBJECT (dest))
    return;

  /*  We should assume that the object came from anywhere, 
      so make sure to cast it as a string class object, so we don't
      get the wrong class in further expressions. (We often get
      null results at the end of a set of iterations.)  */
  if (dest -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) {
    dest -> __o_class = __ctalkGetClass (STRING_CLASSNAME);
    dest -> __o_superclass = dest -> __o_class -> __o_superclass;
    dest -> __o_classname[0] = 0;
    dest -> __o_superclassname[0] = 0;
    if (IS_OBJECT(dest ->  instancevars)) {
      dest -> instancevars -> __o_class = dest -> __o_class;
      dest -> instancevars -> __o_superclass = dest -> __o_superclass;
      dest -> instancevars -> __o_classname[0] = 0;
      dest -> instancevars -> __o_superclassname[0] = 0;
    }
  }

  if (dest -> scope & ARG_VAR) {
    /* We probably need more examples here....  */
    if (dest -> attrs & OBJECT_IS_VALUE_VAR) {
      (void)__ctalkSetObjectName ((dest -> __o_p_obj ? 
				   dest -> __o_p_obj : dest), src -> __o_name);
    } else {
      (void)__ctalkSetObjectName (dest, src -> __o_name);
    }
  }

  /* If dest (which is the target of the assignment) has
     a wider scope than the receiver (src), then the String
     should be assigned by value. Unless it's already a 
     reference.... */
  if (object_is_not_derived (dest) && !string_return_needs_mutation () &&
      !object_has_local_tag (dest)) {
    if (is_receiver (dest) || is_arg (dest)) {
      register_string_assign_by_value ();
    }
    if (dest -> __o_p_obj) {
      if (is_receiver (dest -> __o_p_obj) || is_arg (dest -> __o_p_obj)) {
	register_string_assign_by_value ();
      }
    }
    if ((dest -> scope & GLOBAL_VAR) && !(src -> scope & GLOBAL_VAR)) {
      register_string_assign_by_value ();
    }
  }
  
  /*  This is only really necessary when both the receiver
      and the argument have i's, in which case the
      receiver's actual value was modified in __ctalk_method (),
      then we need to know if we should restore if after the
      method call. */
  if (src -> __o_vartags &&
      src -> __o_vartags -> tag && 
      (src -> __o_vartags -> tag -> i != I_UNDEF)) {
    if (dest -> attrs & OBJECT_IS_I_RESULT) {
      register_str_rcvr_mod (true);
    }
  }
}

