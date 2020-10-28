/* $Id: ismethd.c,v 1.2 2020/10/28 10:29:19 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2017, 2019, 2020  Robert Kiesling, rk3314042@gmail.com.
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "typeof.h"

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

HASHTAB instancemethodhash = NULL;
HASHTAB classmethodhash = NULL;

extern OBJECT *__ctalk_receivers[MAXARGS+1];
extern int __ctalk_receiver_ptr;

static inline char *ismethd_hash_key (char *classname, char *methodname,
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

void initialize_ismethd_hashes (void) {
  char h_buf[MAXLABEL*2];
  _new_hash (&instancemethodhash);
  _new_hash (&classmethodhash);
}

#if 0
/* needed? */
bool rt_method_name (char *classname, char *name) {
  char h_key[MAXLABEL * 2];
  ismethd_hash_key ((char *)classname, (char *)name, h_key);
  if (_hash_get (instancemethodhash, h_key) ||
      _hash_get (classmethodhash, h_key))
    return true;
  else
    return false;
}
#endif

void hash_instance_method (char *classname, char *methodname) {
  char h_buf[MAXLABEL * 2];
  ismethd_hash_key (classname, methodname, h_buf);
  _hash_put (instancemethodhash, h_buf, h_buf);
}
void hash_class_method (char *classname, char *methodname) {
  char h_buf[MAXLABEL* 2];
  ismethd_hash_key (classname, methodname, h_buf);
  _hash_put (classmethodhash, h_buf, h_buf);
}

static METHOD *ismethd_get_class_method_by_name (OBJECT *o, const char *name) {

  OBJECT *class, *value_var;
  METHOD *m;

  if (!IS_OBJECT(o))
    return NULL;

  if (o -> __o_class == rt_defclasses -> p_expr_class) {
    value_var = o -> instancevars;
    if (!value_var || !(value_var -> attrs & OBJECT_IS_VALUE_VAR)) {
      return NULL;
    }
    class = value_var -> __o_class;
   } else {
     class = o -> __o_class;
   }

   if (!class) {
     return NULL;
   }

   for (m = class -> class_methods; m; m = m -> next) {
     if (str_eq (m -> name, (char *)name))
       return m;
     if (!m -> next)
       break;
   }
 
   if (class -> __o_superclass)
     return ismethd_get_class_method_by_name (class -> __o_superclass, name);
   else
     return NULL;
}

static METHOD *ismethd_get_instance_method_by_name (OBJECT *o, 
					const char *name) {
  OBJECT *class, *value_var;
  register METHOD *m;

  if (!IS_OBJECT(o)) return NULL;

  /* 
   * Check for the name, "value," in case an object's
   * first instance var is different, as with some 
   * Collections, which can have all Key objects as 
   * instancevars.
   */
  if (o -> instancevars) {
    if (str_eq (o -> instancevars -> __o_name, "value"))
      value_var = o -> instancevars;
    else
      value_var = o;
  } else {

    value_var = o;
  }

  if ((class=
       ((IS_CLASS_OBJECT(value_var)) ? value_var : value_var->__o_class))
      != NULL) {
    for (m = class -> instance_methods; m; m = m -> next) {
      if (str_eq (m -> name, (char *)name))
	return m;
    }

    if (class -> __o_superclass)
      return ismethd_get_instance_method_by_name 
	(class -> __o_superclass, name);
    else
      return NULL;
    

  }

  return NULL;

}

static METHOD *ismethd_find_class_method_by_name (OBJECT **rcvr, 
				      const char *name) {

  int i;
  OBJECT *tmp_rcvr;
  METHOD *m = NULL;

  if ((m = ismethd_get_class_method_by_name (*rcvr, name)) == NULL) {
    for (i = __ctalk_receiver_ptr + 1; !m && (i <= MAXARGS); i++) {
      tmp_rcvr = __ctalk_receivers[i];
      if ((m = 
	   ismethd_get_class_method_by_name (tmp_rcvr, name))
	  != NULL) {
	*rcvr = tmp_rcvr;
      }
    }
  }

  return m;
}

static METHOD *ismethd_find_instance_method_by_name (OBJECT **rcvr, 
					 const char *name) {

  int i;
  OBJECT *tmp_rcvr;
  METHOD *m = NULL;

  if ((m = ismethd_get_instance_method_by_name (*rcvr, name)) == NULL) {
    for (i = __ctalk_receiver_ptr + 1; !m && (i <= MAXARGS); i++) {
      tmp_rcvr = __ctalk_receivers[i];
      if ((m = 
	   ismethd_get_instance_method_by_name(tmp_rcvr, name))
	  != NULL) {
	*rcvr = tmp_rcvr;
      }
    }
  }

  if (IS_OBJECT(*rcvr)) {
    if (!m) {
      if ((!(*rcvr) -> instancevars) ||
	  ((*rcvr) -> instancevars && 
	   !((*rcvr) -> instancevars -> attrs & OBJECT_IS_VALUE_VAR))) {
	OBJECT *collection_value;
	collection_value = *rcvr;
	m = ismethd_get_instance_method_by_name (collection_value, name);
      }
    }
  }

  return m;
}

static METHOD *ismethd_find_method_by_name (OBJECT **rcvr_p, const char *name) {

  METHOD *m = NULL;

  if ((m = ismethd_find_instance_method_by_name (rcvr_p, name)) != NULL)
    return m;

  if ((m = ismethd_find_class_method_by_name (rcvr_p, name)) != NULL)
    return m;

    return NULL;
}

/*
 *  Here are the fns which only check for the presence of a method,
 *  which we use whenever possible - they're much faster.
 */

static bool ismethd_get_instance_method_by_name_p (OBJECT *o, 
						 const char *name) {
  OBJECT *class, *value_var;
  register METHOD *m;
  char h_buf[MAXLABEL * 2];

  if (!IS_OBJECT(o)) return false;

  /* 
   * Check for the name, "value," in case an object's
   * first instance var is different, as with some 
   * Collections, which can have all Key objects as 
   * instancevars.
   */
  if (o -> instancevars) {
    if (str_eq (o -> instancevars -> __o_name, "value"))
      value_var = o -> instancevars;
    else
      value_var = o;
  } else {

    value_var = o;
  }

  if ((class=
       ((IS_CLASS_OBJECT(value_var)) ? value_var : value_var->__o_class))
      != NULL) {

    if (_hash_get (instancemethodhash, ismethd_hash_key (class -> __o_name, 
							(char *)name, h_buf)))
      return true;

    if (class -> __o_superclass)
      return ismethd_get_instance_method_by_name 
	(class -> __o_superclass, name);
    else
      return false;
    

  }

  return false;

}

static bool ismethd_get_class_method_by_name_p (OBJECT *o, const char *name) {

  OBJECT *class, *value_var;
  METHOD *m;
  char h_buf[MAXLABEL * 2];

  if (!IS_OBJECT(o))
    return false;

  if (o -> __o_class == rt_defclasses -> p_expr_class) {
    value_var = o -> instancevars;
    if (!value_var || !(value_var -> attrs & OBJECT_IS_VALUE_VAR)) {
      return false;
    }
    class = value_var -> __o_class;
   } else {
     class = o -> __o_class;
   }

   if (!class) {
     return false;
   }

    if (_hash_get (classmethodhash, ismethd_hash_key (class -> __o_name, 
						     (char *)name, h_buf)))
      return true;

   if (class -> __o_superclass)
     return ismethd_get_class_method_by_name (class -> __o_superclass, name);
   else
     return false;
}

static bool ismethd_find_class_method_by_name_p (OBJECT **rcvr, 
				      const char *name) {

  int i;
  OBJECT *tmp_rcvr;
  METHOD *m = NULL;

  if (ismethd_get_class_method_by_name (*rcvr, name))
    return true;

  return false;
}

static bool ismethd_find_instance_method_by_name_p (OBJECT **rcvr, 
					 const char *name) {

  int i;
  OBJECT *tmp_rcvr;
  METHOD *m = NULL;

  if (ismethd_get_instance_method_by_name_p (*rcvr, name))
    return true;

  for (i = __ctalk_receiver_ptr + 1; !m && (i <= MAXARGS); i++) {
    tmp_rcvr = __ctalk_receivers[i];
    if (ismethd_get_instance_method_by_name_p(tmp_rcvr, name))
      *rcvr = tmp_rcvr;
  }

  if (IS_OBJECT(*rcvr)) {
    if ((!(*rcvr) -> instancevars) ||
	((*rcvr) -> instancevars && 
	 !((*rcvr) -> instancevars -> attrs & OBJECT_IS_VALUE_VAR))) {
      OBJECT *collection_value;
      collection_value = *rcvr;
      if (ismethd_get_instance_method_by_name_p 
	  (collection_value, name))
	return true;
      
    }
  }

  return false;
}

static bool ismethd_find_method_by_name_p (OBJECT **rcvr_p, 
					   const char *name) {

  if (ismethd_find_instance_method_by_name_p (rcvr_p, name))
    return true;

  if (ismethd_find_class_method_by_name (rcvr_p, name))
    return true;

  return false;
}

/* 
   Handle cases like 
   
   (*rcvr) method.... 
	       
   But only check for a reffed object if we have
   a * prefix operator before rcvr if it's a label.

   Also check for constant receivers:

   ("rcvr_str") method ...
   ('rcvr_char') method ...
   (<int>) method ...
   (<long long int>) method ...
   (<float|double>) method ...
*/
static bool find_method_after_close_paren_a (MESSAGE_STACK messages,
					     int method_idx,
					     int prev_tok_ptr,
					     char *method_name,
					     int stack_top) {
  int i_2, i_2_prev, open_paren_idx;
  OBJECT *reffed_obj, *m_prev_tok_obj;
  bool have_prefix_ref = false;
  bool label_is_cvar = false;
  CVAR *cvar_label;
  open_paren_idx = __ctalkMatchParenRev (messages, prev_tok_ptr,
					 stack_top);
  for (i_2 = open_paren_idx - 1; i_2 >= prev_tok_ptr; --i_2) {
    /* If we can't find something definitely here (e.g., a
       label is a struct expression with only a single CVAR
       to represent it), then check more loosely, below. */
    if (i_2 == prev_tok_ptr)
      return false;
    if (M_ISSPACE(messages[i_2]))
      continue;
    if (M_TOK(messages[i_2]) == LABEL) {
      if (messages[i_2] -> attrs & RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS)
	label_is_cvar = true;
      if (have_prefix_ref) {
	if (label_is_cvar) {
	  if ((cvar_label = get_method_arg_cvars (M_NAME(messages[i_2])))
	      != NULL) {
	    if (cvar_label -> val.__type == OBJECT_T ||
		cvar_label -> val.__type == PTR_T) {
	      reffed_obj = cvar_label -> val.__value.__ptr;
	      if (IS_OBJECT (reffed_obj)) {
		if (ismethd_find_method_by_name_p 
		    (&(reffed_obj), method_name)) {
		  return true;
		} else {
		  return false;
		}
	      } else {
		/* If we have a case of the __ptr member pointing to
		   an __o_value member string which contains the address...
		*/
		if ((reffed_obj = obj_ref_str_2
		     ((char *)cvar_label->val.__value.__ptr,
		      /* *Any* object works here. */
		      __ctalkGetClass ("OBJECT"))) != NULL) {
		  if (IS_OBJECT(reffed_obj)) {
		    if (ismethd_find_method_by_name_p 
			(&(reffed_obj), method_name)) {
		      return true;
		    } else {
		      return false;
		    }
		  } else {
		    return false;
		  }
		} else {
		  return false;
		}
	      }
	    } else {
	      return false;
	    }
	  } else {
	    return false;
	  }
	} else { /* if (label_is_cvar) */
	  if (IS_OBJECT(messages[i_2]->obj)) {
	    m_prev_tok_obj = messages[i_2] -> obj;
	    if ((reffed_obj = obj_ref_str_2
		 (m_prev_tok_obj -> __o_value, m_prev_tok_obj))
		!= NULL) {
	      if (ismethd_find_method_by_name_p 
		  (&(reffed_obj), method_name)) {
		return true;
	      } else {
		if (ismethd_find_method_by_name_p 
		    (&(m_prev_tok_obj), method_name)) {
		  return true;
		}
	      }
	    } else if (IS_OBJECT(m_prev_tok_obj -> instancevars)) {
	      if ((reffed_obj = obj_ref_str_2
		   (m_prev_tok_obj -> instancevars -> __o_value,
		    m_prev_tok_obj))
		  != NULL) {
		if (ismethd_find_method_by_name_p 
		    (&(reffed_obj), method_name)) {
		  return true;
		} else {
		  if (ismethd_find_method_by_name_p 
		      (&(m_prev_tok_obj), method_name)) {
		    return true;
		  }
		}
	      }
	    }
	  }
	} /* if (label_is_cvar) */
      } else { /* if (have_prefix_ref) { */
	/* TODO - Handle an OBJECT * CVAR that does not contain
	   an object reference. */
	m_prev_tok_obj = messages[i_2] -> obj;
	if (ismethd_find_method_by_name_p 
	    (&(m_prev_tok_obj), method_name)) {
	  return true;
	}
      }/* if (have_prefix_ref) { */
    } else if (M_TOK(messages[i_2]) == LITERAL) {
      m_prev_tok_obj = rt_defclasses -> p_string_class;
      if (ismethd_find_method_by_name_p 
	  (&(m_prev_tok_obj), method_name)) {
	return true;
      }
    } else if (M_TOK(messages[i_2]) == LITERAL_CHAR) {
      m_prev_tok_obj = rt_defclasses -> p_character_class;
      if (ismethd_find_method_by_name_p 
	  (&(m_prev_tok_obj), method_name)) {
	return true;
      }
    } else if (M_TOK(messages[i_2]) == INTEGER ||
	       M_TOK(messages[i_2]) == LONG)  {
      m_prev_tok_obj = rt_defclasses -> p_integer_class;
      if (ismethd_find_method_by_name_p 
	  (&(m_prev_tok_obj), method_name)) {
	return true;
      }
    } else if (M_TOK(messages[i_2]) == LONGLONG) {
      m_prev_tok_obj = rt_defclasses -> p_longinteger_class;
      if (ismethd_find_method_by_name_p 
	  (&(m_prev_tok_obj), method_name)) {
	return true;
      }
    } else if (M_TOK(messages[i_2]) == FLOAT) {
      m_prev_tok_obj = rt_defclasses -> p_float_class;
      if (ismethd_find_method_by_name_p 
	  (&(m_prev_tok_obj), method_name)) {
	return true;
      }
    } else if ((M_TOK(messages[i_2]) == ASTERISK) &&
	       (messages[i_2] -> attrs & RT_TOK_IS_PREFIX_OPERATOR)) {
      have_prefix_ref = true;
    }
  }
  return false;
}


int __ctalk_isMethod_2 (char *name, MESSAGE_STACK messages, int msg_ptr, 
			int stack_top) {
  int prev_tok_ptr;
  MESSAGE *m_prev_tok;
  METHOD *m;
  OBJECT *m_prev_tok_obj;

  if (M_TOK(messages[msg_ptr]) == METHODMSGLABEL)
    return TRUE;

  if ((prev_tok_ptr = __ctalkPrevLangMsg (messages, msg_ptr, stack_top))
      != ERROR) {
    m_prev_tok = messages[prev_tok_ptr];
    if (M_OBJ_IS_VAR(m_prev_tok)) {

      if (m_prev_tok && m_prev_tok -> obj) {
 	if (IS_OBJECT (m_prev_tok -> obj -> instancevars) &&
	    (m_prev_tok -> obj -> instancevars -> attrs & 
	     OBJECT_IS_VALUE_VAR)) {
	  m_prev_tok_obj = m_prev_tok -> obj -> instancevars;
	  if (ismethd_find_method_by_name (&m_prev_tok_obj, 
					name))
	    return TRUE;
	} else {
	  if (IS_VALUE_INSTANCE_VAR(m_prev_tok->obj)) {
	    m_prev_tok_obj = m_prev_tok -> obj;
	    if (ismethd_find_method_by_name (&m_prev_tok_obj, 
					     name))
	      return TRUE;
	  } else {
	    /*
	     *  Should only be needed for Collection
	     *  subclasses that don't have value instance 
	     *  variables.
	     */
 	    if ((!m_prev_tok -> obj -> instancevars) ||
 		(m_prev_tok -> obj -> instancevars &&
		 !(m_prev_tok -> obj -> instancevars -> attrs & 
		   OBJECT_IS_VALUE_VAR))) {
	      m_prev_tok_obj = m_prev_tok -> obj;
	      if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					    name))
		return TRUE;
	    }
	  }
	}
      } else { /* if (m_prev_tok && m_prev_tok -> obj)  */
	if (m_prev_tok && m_prev_tok -> value_obj) {
 	  if (IS_OBJECT (m_prev_tok -> value_obj -> instancevars) &&
	      (m_prev_tok -> value_obj -> instancevars -> attrs &
	       OBJECT_IS_VALUE_VAR)) {
	    m_prev_tok_obj = m_prev_tok -> value_obj -> instancevars;
	    if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					  name))
	      return TRUE;
	  } else {
	    if (IS_VALUE_INSTANCE_VAR(m_prev_tok->value_obj)) {
	      m_prev_tok_obj = m_prev_tok -> value_obj;
	      if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					    name))
		return TRUE;
	    } else {
	      /*
	       *  Should only be needed for Collection
	       *  subclasses that don't have value instance 
	       *  variables.
	       */
 	      if ((!m_prev_tok -> value_obj -> instancevars) ||
 		  (m_prev_tok -> value_obj -> instancevars && 
		   !(m_prev_tok -> value_obj -> instancevars -> attrs &
		     OBJECT_IS_VALUE_VAR))) {
		m_prev_tok_obj = m_prev_tok -> value_obj;
		if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					      name))
		  return TRUE;
	      }
	    }
	  }
	} else { /* if (m_prev_tok && m_prev_tok -> value_obj)  */
	  if ((M_TOK(m_prev_tok) == METHODMSGLABEL) &&
	      (m_prev_tok -> value_obj &&
	       (m_prev_tok -> value_obj == 
		messages[msg_ptr] -> receiver_obj))) {
	    if ((m_prev_tok_obj = messages[msg_ptr] -> receiver_obj) != NULL){
	      if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					    name)) 
		return TRUE;
	    }
	  }
	}
      }

    } else {
      /*
       *  The previous label is a method - check for a value_obj
       *  result also.
       */
      if (M_TOK(m_prev_tok) == METHODMSGLABEL) {
	if (m_prev_tok && IS_OBJECT (M_VALUE_OBJ(m_prev_tok))) {
	  m_prev_tok_obj = M_VALUE_OBJ(m_prev_tok);
	  if (ismethd_find_method_by_name_p 
	      (&(m_prev_tok_obj), name)) {
	    return TRUE;
	  }  else {
	    /*
	     *  Check for a method from an instance variable value class.
	     */
	    if (m_prev_tok_obj && HAS_INSTANCEVAR_CLASS(m_prev_tok_obj)) {
	      if (IS_OBJECT(m_prev_tok_obj -> instancevars)) {
		if (ismethd_find_method_by_name_p 
		     (&(m_prev_tok_obj->instancevars), name))
		  return TRUE;
	      }
	    }
	  }
	} else {
	  /*
	   *  If the previous method doesn't have a result yet, look
	   *  up the method's return class.
	   */
	  if (IS_OBJECT(m_prev_tok->receiver_obj)){
	    METHOD *m_prev_tok_method;
	    OBJECT *m_prev_tok_rcvr_obj;
	    m_prev_tok_rcvr_obj = m_prev_tok->receiver_obj;
	    if ((m_prev_tok_method = 
		 ismethd_find_method_by_name (&m_prev_tok_rcvr_obj,
					  M_NAME(m_prev_tok)))
		!= NULL) {
	      OBJECT *m_prev_return_class;
	      if (str_eq (m_prev_tok_method -> returnclass, "Any")) {
		m_prev_return_class = 
		  ((m_prev_tok->receiver_obj) ?
		   (m_prev_tok->receiver_obj->__o_class) :
		   (NULL));
	      } else {
		if (*m_prev_tok_method -> returnclass) {
		  m_prev_return_class = 
		    __ctalkGetClass(m_prev_tok_method->returnclass);
		} else {
		  m_prev_return_class = m_prev_tok_rcvr_obj -> __o_class;
		}
	      }
	      /* this still needs the old routine. */
	      if (ismethd_find_method_by_name (&m_prev_return_class, 
					   name)) {
		return TRUE;
	      }
	    }
	  }
	}
      } else {

	/*
	 * This finds up cases like 
	 *   *<rcvr> <method> [<args>...]
	 */
	if (IS_OBJECT (m_prev_tok -> obj) &&
	    m_prev_tok -> obj -> attrs & OBJECT_HAS_PTR_CX) {
	  if ((m_prev_tok_obj = m_prev_tok -> value_obj) != NULL) {
	    if (ismethd_find_method_by_name_p 
		(&(m_prev_tok_obj), name)) {
	      return TRUE;
	    }
	  }
	}

	if (IS_OBJECT (M_VALUE_OBJ(m_prev_tok)) &&
	    IS_CLASS_OBJECT (M_VALUE_OBJ(m_prev_tok))) {
	  OBJECT *o_tmp = M_VALUE_OBJ (m_prev_tok);
	  /* When we have a class object as the receiver, look for
	     a class method first. */
	  if (ismethd_find_class_method_by_name_p (OBJREF(o_tmp), name))
	    return TRUE;
	  if (ismethd_find_instance_method_by_name_p (OBJREF(o_tmp), name))
	    return TRUE;
	}

	if (m_prev_tok -> attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL) {
	  if ((m_prev_tok_obj = m_prev_tok -> value_obj) != NULL) {
	    if (ismethd_find_method_by_name_p 
		(&(m_prev_tok_obj), name)) {
	      return TRUE;
	    }
	  }
	} else if (M_TOK(m_prev_tok) == CLOSEPAREN) {
	  if (find_method_after_close_paren_a (messages, msg_ptr,
					       prev_tok_ptr,
					       name,
					       stack_top)) {
	    return true;
	  } else {
	    goto looser_check;
	  }
	} else {
	looser_check:
	  m_prev_tok_obj = m_prev_tok -> obj;
	  if (m_prev_tok && IS_OBJECT (m_prev_tok -> obj)) {
	    if (ismethd_find_method_by_name_p 
		(&(m_prev_tok_obj), name)) {
	      return TRUE;
	    } else {
	      /*
	       *  Check for a method from an instance variable value class.
	       */
	      if (m_prev_tok_obj && HAS_INSTANCEVAR_CLASS(m_prev_tok_obj)) {
		if (IS_OBJECT(m_prev_tok_obj -> instancevars)) {
		  if ((m = ismethd_find_method_by_name 
		       (&(m_prev_tok_obj->instancevars), name))
		      != NULL)
		    return TRUE;
		}
	      }
	    }
	  } else {
	    m_prev_tok_obj = m_prev_tok -> value_obj;
	    if (m_prev_tok && IS_OBJECT (m_prev_tok -> value_obj)) {
	      if (ismethd_find_method_by_name_p 
		  (&(m_prev_tok_obj), name)) {
		return TRUE;
	      }  else {
		/*
		 *  Check for a method from an instance variable value class.
		 */
		if (m_prev_tok_obj && HAS_INSTANCEVAR_CLASS(m_prev_tok_obj)) {
		  if (IS_OBJECT(m_prev_tok_obj -> instancevars)) {
		    if (ismethd_find_method_by_name_p 
			 (&(m_prev_tok_obj->instancevars), name))
		      return TRUE;
		  }
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

int __ctalk_op_isMethod_2 (char *name, MESSAGE_STACK messages, int msg_ptr, 
	       int stack_top) {
  int prev_tok_ptr;
  MESSAGE *m_prev_tok;
  METHOD *m;
  OBJECT *m_prev_tok_obj;

  if ((prev_tok_ptr = __ctalkPrevLangMsg (messages, msg_ptr, stack_top))
      != ERROR) {
    m_prev_tok = messages[prev_tok_ptr];
    if (M_OBJ_IS_VAR(m_prev_tok)) {

      if (m_prev_tok && m_prev_tok -> obj) {
 	if (IS_OBJECT (m_prev_tok -> obj -> instancevars) &&
	    (m_prev_tok -> obj -> instancevars -> attrs & 
	     OBJECT_IS_VALUE_VAR)) {
	  m_prev_tok_obj = m_prev_tok -> obj -> instancevars;
	  if (ismethd_find_method_by_name (&m_prev_tok_obj, 
					name))
	    return TRUE;
	} else {
	  if (IS_VALUE_INSTANCE_VAR(m_prev_tok->obj)) {
	    m_prev_tok_obj = m_prev_tok -> obj;
	    if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					     name))
	      return TRUE;
	  } else {
	    /*
	     *  Should only be needed for Collection
	     *  subclasses that don't have value instance 
	     *  variables.
	     */
 	    if ((!m_prev_tok -> obj -> instancevars) ||
 		(m_prev_tok -> obj -> instancevars &&
		 !(m_prev_tok -> obj -> instancevars -> attrs &
		   OBJECT_IS_VALUE_VAR))) {
	      m_prev_tok_obj = m_prev_tok -> obj;
	      if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					    name))
		return TRUE;
	    }
	  }
	}
      } else { /* if (m_prev_tok && m_prev_tok -> obj)  */
	if (m_prev_tok && m_prev_tok -> value_obj) {
 	  if (IS_OBJECT (m_prev_tok -> value_obj -> instancevars) &&
	      (m_prev_tok -> value_obj->instancevars->attrs & 
	       OBJECT_IS_VALUE_VAR)) {
	    m_prev_tok_obj = m_prev_tok -> value_obj -> instancevars;
	    if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					  name))
	      return TRUE;
	  } else {
	    if (IS_VALUE_INSTANCE_VAR(m_prev_tok->value_obj)) {
	      m_prev_tok_obj = m_prev_tok -> value_obj;
	      if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					    name))
		return TRUE;
	    } else {
	      /*
	       *  Should only be needed for Collection
	       *  subclasses that don't have value instance 
	       *  variables.
	       */
 	      if ((!m_prev_tok -> value_obj -> instancevars) ||
 		  (m_prev_tok -> value_obj -> instancevars && 
 		   !(m_prev_tok -> value_obj -> instancevars -> attrs &
		     OBJECT_IS_VALUE_VAR))) {
		m_prev_tok_obj = m_prev_tok -> value_obj;
		if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
					      name))
		  return TRUE;
	      }
	    }
	  }
	} else { /* if (m_prev_tok && m_prev_tok -> value_obj)  */
	  if ((M_TOK(m_prev_tok) == METHODMSGLABEL) &&
	      (m_prev_tok -> value_obj &&
	       (m_prev_tok -> value_obj == 
		messages[msg_ptr] -> receiver_obj))) {
	    if ((m_prev_tok_obj = messages[msg_ptr] -> receiver_obj) != NULL){
	      if (ismethd_find_method_by_name_p (&m_prev_tok_obj, 
						 name))
		return TRUE;
	    }
	  }
	}
      }

    } else {
      /*
       *  The previous label is a method - check for a value_obj
       *  result also.
       */
      if (M_TOK(m_prev_tok) == METHODMSGLABEL) {
	if (m_prev_tok && IS_OBJECT (M_VALUE_OBJ(m_prev_tok))) {
	  m_prev_tok_obj = M_VALUE_OBJ(m_prev_tok);
	    if (ismethd_find_method_by_name_p (&(m_prev_tok_obj), name)){
	      return TRUE;
	  }  else {
	    /*
	     *  Check for a method from an instance variable value class.
	     */
	    if (m_prev_tok_obj && HAS_INSTANCEVAR_CLASS(m_prev_tok_obj)) {
	      if (IS_OBJECT(m_prev_tok_obj -> instancevars)) {
		if (ismethd_find_method_by_name_p 
		    (&(m_prev_tok_obj->instancevars), name))
		  return TRUE;
	      }
	    }
	  }
	} else {
	  /*
	   *  If the previous method doesn't have a result yet, look
	   *  up the method's return class.
	   */
	  if (IS_OBJECT(m_prev_tok->receiver_obj)){
	    METHOD *m_prev_tok_method;
	    OBJECT *m_prev_tok_rcvr_obj;
	    m_prev_tok_rcvr_obj = m_prev_tok->receiver_obj;
	    if ((m_prev_tok_method = 
		 ismethd_find_method_by_name (&m_prev_tok_rcvr_obj,
					  M_NAME(m_prev_tok)))
		!= NULL) {
	      OBJECT *m_prev_return_class;
	      if (str_eq (m_prev_tok_method -> returnclass, "Any")) {
		m_prev_return_class = 
		  ((m_prev_tok->receiver_obj) ?
		   (m_prev_tok->receiver_obj->__o_class) :
		   (NULL));
	      } else {
		if (*m_prev_tok_method -> returnclass) {
		  m_prev_return_class = 
		    __ctalkGetClass(m_prev_tok_method->returnclass);
		} else {
		  m_prev_return_class = m_prev_tok_rcvr_obj -> __o_class;
		}
	      }
	      if (ismethd_find_method_by_name_p (&m_prev_return_class, 
					   name)) {
		return TRUE;
	      }
	    }
	  }
	}
      } else {
	if (m_prev_tok -> attrs & RT_TOK_IS_AGGREGATE_MEMBER_LABEL) {
	  if ((m_prev_tok_obj = m_prev_tok -> value_obj) != NULL) {
	      if (ismethd_find_method_by_name_p 
		  (&(m_prev_tok_obj), name)) {
		return TRUE;
	    }
	  }
	} else {
	  m_prev_tok_obj = m_prev_tok -> obj;
	  if (m_prev_tok && IS_OBJECT (m_prev_tok -> obj)) {
	      if (ismethd_find_method_by_name_p 
		  (&(m_prev_tok_obj), name)) {
	      return TRUE;
	    }  else {
	      /*
	       *  Check for a method from an instance variable value class.
	       */
	      if (m_prev_tok_obj && HAS_INSTANCEVAR_CLASS(m_prev_tok_obj)) {
		if (IS_OBJECT(m_prev_tok_obj -> instancevars)) {
		  if (ismethd_find_method_by_name_p 
		       (&(m_prev_tok_obj->instancevars), name))
		    return TRUE;
		}
	      }
	    }
	  } else {
	    m_prev_tok_obj = m_prev_tok -> value_obj;
	    if (m_prev_tok && IS_OBJECT (m_prev_tok -> value_obj)) {
	      if (ismethd_find_method_by_name_p
		  (&(m_prev_tok_obj), name)) {
		return TRUE;
	      }  else {
		/*
		 *  Check for a method from an instance variable value class.
		 */
		if (m_prev_tok_obj && HAS_INSTANCEVAR_CLASS(m_prev_tok_obj)) {
		  if (IS_OBJECT(m_prev_tok_obj -> instancevars)) {
		    if (ismethd_find_method_by_name_p 
			(&(m_prev_tok_obj->instancevars), name))
		      return TRUE;
		  }
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

