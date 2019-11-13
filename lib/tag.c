/* $Id: tag.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2015-2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#ifndef _VARTAG_H
#include "vartag.h"
#endif

#include "message.h"
#include "object.h"
#include "ctalk.h"

extern RT_INFO *__call_stack[MAXARGS+1];
extern int __call_stack_ptr;

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

VARTAG *new_vartag (VARENTRY *v) {
  VARTAG *vt;

  if ((vt = (VARTAG *) __xalloc (sizeof (struct _vartag))) == NULL) {
    _error ("new_vartag: %s\n",  strerror (errno));
  }

  vt -> sig = VARTAG_SIG;
  vt -> tag = v;
  return vt;
}

void delete_vartag (VARTAG *v, bool delete_tag) {
  if (IS_VARTAG (v)) {

    if (delete_tag && v -> tag && v -> tag -> is_local)
	delete_varentry (v -> tag);
    if (delete_tag && v -> from &&  v -> from -> is_local)
	delete_varentry (v -> from);

    __xfree (MEMADDR(v));
  }
}

void delete_vartag_list (OBJECT *obj) {
  VARTAG *t, *t_prev;
  for (t = obj -> __o_vartags; t && t -> next; t = t -> next)
    ;
  
  while (t != obj -> __o_vartags) {
    t_prev = t -> prev;
    delete_vartag (t, obj -> attrs & OBJECT_HAS_LOCAL_TAG);
    t = t_prev;
  }

  delete_vartag (obj -> __o_vartags, obj -> attrs & OBJECT_HAS_LOCAL_TAG);
  obj -> __o_vartags = NULL;
}

void add_tag (OBJECT *obj, VARENTRY *v) {
  VARTAG *t;
  if (IS_VARTAG(obj -> __o_vartags) &&
      IS_EMPTY_VARTAG(obj -> __o_vartags)) {
    obj -> __o_vartags -> tag = v;
  } else {
    
    for (t = obj -> __o_vartags; t; t = t -> next) {
      if (t -> tag == v)
	break;
      if (t -> next == NULL) {
	t -> next = new_vartag (v);
	t -> next -> prev = t;
	break;
      }
    }

  }

}

void remove_tag (OBJECT *obj, VARENTRY *v) {
  VARTAG *t;
  
  if (HAS_VARTAGS(obj) && obj -> __o_vartags -> next == NULL) {
    /* If the object is declared, not derived, it should always
       have at least one tag. */
    return;
  }

  for (t = obj -> __o_vartags; t && t -> next; t = t -> next) {
    if (t -> tag == v) {
      /* If it's the first tag, only delete if there are
	 additional tags.  The object should always have
	 at least one tag. */
      if (t == obj -> __o_vartags) {
	if (t -> next) {
	  obj -> __o_vartags = t -> next;
	  delete_vartag (t, obj -> attrs & OBJECT_HAS_LOCAL_TAG);
	  return;
	} else {
	  t -> tag = NULL;
	  return;
	}
      } else { /* if (t == obj -> __o_vartags) */

	if (t -> next)
	  t -> next -> prev = t -> prev;
	if (t -> prev)
	  t -> prev -> next = t -> next;
	delete_vartag (t, obj -> attrs & OBJECT_HAS_LOCAL_TAG);
	return;
      
      }  /* if (t == obj -> __o_vartags) */
    }

  } 
  
}

void make_tag_for_varentry_active (OBJECT *obj, VARENTRY *v) {

  VARTAG *vt;

  if (obj == NULL)
    return;

  /* The fn can also be called by the delete routines, so
     check.... */
  if (!IS_VARTAG(obj -> __o_vartags))
    return;

  /* If there's only one tag, or it's already the first, 
     then we don't need to do anything. */
  if (obj -> __o_vartags -> next == NULL)
    return;
  if (obj -> __o_vartags -> tag == v)
    return;

  for (vt = obj -> __o_vartags -> next; vt; vt = vt -> next) {

    if (vt -> tag == v) {

      /* Remove the vartag from the list. */
      if (vt -> next)
	vt -> next -> prev = vt -> prev;
      if (vt -> prev)
	vt -> prev -> next = vt -> next;
      vt -> next = vt -> prev = NULL;

      /* Then re-insert it at the start of the list. */
      vt -> next = obj -> __o_vartags;
      obj -> __o_vartags -> prev = vt;
      obj -> __o_vartags = vt;

      return;
    }

  }
}

void __ctalkIncStringRef (OBJECT *obj, int inc, int op) {
  VARENTRY *v;
  char *c;
  OBJECT *o, *o_parent;
  METHOD *method;

  if (!obj -> __o_vartags) {
    if (obj -> __o_p_obj && obj -> __o_p_obj -> __o_vartags) {
      o_parent = obj -> __o_p_obj;
    } else {
      return;
    }
  } else {
    o_parent = obj;
  }
  if (o_parent -> __o_vartags -> tag == NULL) {
    method = __ctalkRtGetMethod ();
    if (o_parent -> attrs & OBJECT_IS_STRING_LITERAL) {
      _warning ("Warning: Integer operand reference to String constant "
		"\"%s.\"\n", o_parent -> __o_name);
      _warning ("Warning: Pointer math is not supported (yet) for "
		"String constants.\n");
      __warning_trace ();
    }
    if (o_parent -> __o_vartags -> from == NULL)
      return;
  }

  v = o_parent -> __o_vartags -> tag;

  switch (op)
    {
    case TAG_REF_TEMP:
      if (o_parent -> __o_vartags -> from && 
	  o_parent -> attrs & OBJECT_IS_I_RESULT) {
	/* this is a lot simpler than munging the original object. */
	/* see the comments for derived_i (), below. */
	if (o_parent -> __o_vartags -> from -> i_temp == I_UNDEF) {
	  unsigned int offset;
	  offset = derived_i_2 (o_parent -> __o_vartags -> from,
				o_parent);
	  o_parent -> __o_vartags -> from -> i_temp = 
	    o_parent -> __o_vartags -> from -> var_object -> __o_value + 
	    offset + inc;
	}
	__ctalkSetObjectValueVar (o_parent,
				  (char *)o_parent -> __o_vartags -> from
				  -> i_temp);
	o_parent -> __o_vartags -> from -> i_temp = I_UNDEF;
      } else {
	if (v) {
	  if (v -> i == I_UNDEF)
	    v -> i_temp = o_parent -> instancevars -> __o_value;
	  v -> i_temp += inc;
	  if (((char *)v -> i_temp) < o_parent -> instancevars -> __o_value)
	    v -> i_temp = NULL;
	}
      }
      break;
    case TAG_REF_POSTFIX:
      /* if either *i or *i_post is \0, synchronize them
	 and don't go past the end of the buffer. */
      if (v -> i_post != I_UNDEF && *(char *)v -> i_post == '\0') {
	v -> i = v -> i_post;
	return;
      } else if (v -> i != I_UNDEF && v -> i != NULL &&
		 *(char *)v -> i == '\0') {
	v -> i_post = v -> i;
	return;
      }
      /* otherwise, i gets set to i_post after the fetch */
      if (v -> i == I_UNDEF)
	v -> i_post = o_parent -> instancevars -> __o_value + inc;
      else
	v -> i_post = v -> i + inc;
      if (((char *)v -> i_post) < o_parent -> instancevars -> __o_value)
	v -> i_post = NULL;
      register_postfix_fetch_update ();
      break;
    case TAG_REF_PREFIX:
    default:
      if (v -> i == I_UNDEF)
	v -> i = o_parent -> instancevars -> __o_value;
      v -> i += inc;
      if (((char *)v -> i) < o_parent -> instancevars -> __o_value)
	v -> i = NULL;
      break;
    }
}

void __ctalkIncKeyRef (OBJECT *obj, int inc, int op) {
  VARENTRY *v;
  char *c;
  OBJECT *o, *o_parent;
  int j;

  if (!obj -> __o_vartags) {
    if (obj -> __o_p_obj && obj -> __o_p_obj -> __o_vartags) {
      o_parent = obj -> __o_p_obj;
    } else {
      return;
    }
  } else {
    o_parent = obj;
  }
  if (o_parent -> __o_vartags -> tag == NULL) {
    if (o_parent -> __o_vartags -> from == NULL) {
      o_parent -> __o_vartags -> tag = new_varentry (o_parent);
      o_parent -> attrs |= OBJECT_HAS_LOCAL_TAG;
      o_parent -> __o_vartags -> tag -> is_local = true;
      v = o_parent -> __o_vartags -> tag;
    } else {
      v = o_parent -> __o_vartags -> from;
    }
  } else {
    v = o_parent -> __o_vartags -> tag;
  }

  switch (op)
    {
    case TAG_REF_TEMP:
      if (v -> i == I_UNDEF)
	v -> i_temp = (void *)o_parent;
      else
	v -> i_temp = v ->  i;
      if (inc > 0)
	for (j = 0; (j < inc) && v -> i_temp; j++)
	  v -> i_temp = ((OBJECT *)v -> i_temp) -> next;
      if (inc < 0)
	for (j = inc; (j < 0) && v -> i_temp; j++)
	  v -> i_temp = ((OBJECT *)v -> i_temp) -> prev;
      break;
    case TAG_REF_POSTFIX:
      if (v ->  i == NULL) {
	v -> i_post = NULL;
      } else {
	if (v -> i == I_UNDEF)
	  v -> i = (void *)o_parent;
	/* Sufficient for now. */
	if (inc > 0)
	  for (j = 0; (j < inc) && v -> i_post; j++)
	    v -> i_post = ((OBJECT *)v -> i) -> next;
	if (inc < 0)
	  for (j = inc; (j < 0) && v -> i_post; j++)
	    v -> i_post = ((OBJECT *)v -> i) -> prev;
      }
      register_postfix_fetch_update ();
      break;
    case TAG_REF_PREFIX:
    default:
      if (v -> i == I_UNDEF)
	v -> i = (void *)o_parent;

      if (inc > 0)
	for (j = 0; (j < inc) && v -> i; j++)
	  v -> i = ((OBJECT *)v -> i) -> next;

      if (inc < 0)
	for (j = inc; (j < 0) && v -> i; j++)
	  v -> i = ((OBJECT *)v -> i) -> prev;

      break;
    }
}


void *active_i (OBJECT  *o) {

  OBJECT *o_p;
  void *t;

  if (!IS_OBJECT(o))
    return I_UNDEF;

  if (o -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) {
    o_p = o;
  } else {
    if (o -> __o_p_obj && o -> __o_p_obj -> __o_vartags) {
      o_p = o -> __o_p_obj;
    } else {
      if (o -> __o_vartags) {
	o_p = o;
      } else {
	return I_UNDEF;
      }
    }
  }

  if (IS_VARTAG(o_p -> __o_vartags) &&
      !IS_EMPTY_VARTAG (o_p -> __o_vartags) && 
      IS_VARENTRY(o_p -> __o_vartags -> tag)) {
    if (o_p -> __o_vartags -> tag -> i_post != I_UNDEF) {
      t = o_p -> __o_vartags -> tag -> i;
      o_p -> __o_vartags -> tag -> i = o_p -> __o_vartags -> tag -> i_post;
      o_p -> __o_vartags -> tag -> i_post = I_UNDEF;
      return t;
    } else {
      if (o_p -> __o_vartags -> tag -> i_temp != I_UNDEF) {
	t = o_p -> __o_vartags -> tag -> i_temp;
	o_p -> __o_vartags -> tag -> i_temp = I_UNDEF;
	return t;
      } else {
	return o_p -> __o_vartags -> tag -> i;
      }
    }
  } else {
    if (IS_VARTAG(o_p -> __o_vartags) && 
	!IS_EMPTY_VARTAG(o_p -> __o_vartags) &&
	IS_VARENTRY (o_p -> __o_vartags -> tag))
      o_p -> __o_vartags -> tag = NULL;
    return I_UNDEF;
  }

}

static void copy_tag_to_derived_from (OBJECT *orig_object, OBJECT *i_object) {
      if (IS_VARTAG(i_object -> __o_vartags)) {
	if (IS_EMPTY_VARTAG(i_object -> __o_vartags)) {
	  if (IS_VARTAG (orig_object -> __o_vartags)) {
	    if (!IS_EMPTY_VARTAG (orig_object -> __o_vartags)) {
	      i_object -> __o_vartags -> from =
		orig_object -> __o_vartags -> tag;
	    }
	  }
	}
      }
}


/*
 *  Call active_i () first, so that we know that c_i isn't I_UNDEF.
 */
OBJECT *create_param_i (OBJECT *orig_object, void *c_i) {

  /* Any class that doesn't use tag math isn't handled here and
     just gets the original object back. */
  OBJECT *param_object = orig_object;

  if (is_class_or_subclass (orig_object, rt_defclasses -> p_string_class)) {
    param_object =
      __ctalkCreateObjectInit (orig_object -> __o_name,
			       orig_object -> CLASSNAME,
			       _SUPERCLASSNAME(orig_object),
			       CREATED_PARAM,
			       (char *)c_i);
    if (IS_OBJECT (orig_object)) {
      if (orig_object -> __o_vartags) {
	param_object -> __o_vartags -> from =
	  orig_object -> __o_vartags -> tag;
      } else {
	if (orig_object -> __o_p_obj) {
	  if (orig_object -> __o_p_obj -> __o_vartags) {
	    param_object -> __o_vartags -> from =
	      orig_object -> __o_p_obj ->  __o_vartags -> tag;
	  }
	}
      }
    }

    param_object -> attrs |= OBJECT_IS_I_RESULT;
    param_object -> instancevars -> attrs |= OBJECT_IS_I_RESULT;
  } else {

    if (is_class_or_subclass (orig_object, rt_defclasses -> p_key_class)) {
      param_object = (OBJECT *)c_i;
      if (IS_OBJECT(param_object))
	copy_tag_to_derived_from (orig_object, param_object);
    }

  }
  return param_object;
}

/*  Used (so far) in eval_expr (), where an instance or class variable
    message is the terminal token of an expression. */
OBJECT *var_i (OBJECT *var, int scope) {
  void *c_i;
  OBJECT *i_obj;
  if ((c_i = active_i (var)) != I_UNDEF) {

    if ((i_obj = create_param_i (var, c_i)) == NULL) {
      i_obj = __ctalkCreateObjectInit 
	(NULLSTR,
	 var -> CLASSNAME,
	 _SUPERCLASSNAME(var),
	 scope, "0x0");
      __ctalkSetObjectAttr (i_obj, OBJECT_IS_NULL_RESULT_OBJECT);
    }
    return i_obj;
  } else {
    return var;
  }
}

bool is_temporary_i_param (OBJECT *obj) {
  if (IS_OBJECT (obj)) {
    if (obj -> __o_vartags && (obj -> __o_vartags -> tag == NULL)) {
      if (obj -> attrs & OBJECT_IS_I_RESULT)
	return true;
    }
  }
  return false;
}


/*
	This is for cases where we have an expression like:

	  str2 = str1 + int1;

	Then we effectively make it into:

	  str2 = str1;
	  str2 += int1;

	We do this by deriving the i (the value of "int1") from the result 
	of the expression on the right-hand side of the equation, and 
	setting the lvalue's i accordingly.
*/
void derived_i (VARENTRY *lval_varentry, OBJECT *derived_target) {
  unsigned int offset;
  char *str_offset;

  if (is_class_or_subclass (derived_target, rt_defclasses -> p_string_class)){
    if (derived_target -> __o_value[0]) {
      if ((str_offset = strstr (lval_varentry -> var_object -> __o_value,
				derived_target -> __o_value)) != NULL) {
	offset = str_offset - lval_varentry -> var_object -> __o_value;
      }
    } else {
      /* works around a bug (? ... not really, just non-intuitive...
	 see tfmp.) in strstr if the second argument begins with a '\0'. */
      offset = strlen (lval_varentry -> var_object -> __o_value);
    }
    lval_varentry -> i = lval_varentry -> var_object -> __o_value + offset;
  }
}

int derived_i_2 (VARENTRY *lval_varentry, OBJECT *derived_target) {
  unsigned int offset;
  char *str_offset;
  if (is_class_or_subclass (derived_target, rt_defclasses -> p_string_class)) {
    if (derived_target -> __o_value[0]) {
      /* here, too, could be wrong if the second argument to strstr ()
	 begins with a '\0'. */
      if ((str_offset = strstr (lval_varentry -> var_object -> __o_value,
				derived_target -> __o_value)) != NULL) {
	offset = str_offset - lval_varentry -> var_object -> __o_value;
      }
    } else {
      offset = strlen (lval_varentry -> var_object -> __o_value);
    }
  }
  return offset;
}

int n_tags (OBJECT *o) {
  VARTAG *v;
  int i;

  /* Make sure we're at the beginning of the tags list. */
  for (v = o -> __o_vartags; v && v -> prev; v = v -> prev)
    ;

  for (i = 0, v = o -> __o_vartags; v; v = v -> next) {
    if (v -> tag) {
      ++i;
    }
  }

  return i;
}

int make_postfix_current (OBJECT *obj) {
  OBJECT *o_parent;
  OBJECT *i_object;

  if (obj == I_UNDEF)
    return 0;

  if (!IS_OBJECT (obj))
    return 0;

  if (!obj -> __o_vartags) {
    if (obj -> __o_p_obj && obj -> __o_p_obj -> __o_vartags) {
      o_parent = obj -> __o_p_obj;
    } else {
      return ERROR;
    }
  } else {
    o_parent = obj;
  }

  if (!IS_EMPTY_VARTAG(o_parent -> __o_vartags)) {
    if (o_parent -> __o_vartags -> tag -> i_post  != I_UNDEF) {
      o_parent -> __o_vartags -> tag -> i = 
	o_parent -> __o_vartags -> tag -> i_post;
      o_parent -> __o_vartags -> tag -> i_post = I_UNDEF;
      return 0;
    }
    if (o_parent -> __o_vartags -> tag -> i  != I_UNDEF) {
      i_object = (OBJECT *)o_parent -> __o_vartags -> tag -> i;
      if (IS_OBJECT(i_object)) {
	if (IS_EMPTY_VARTAG (i_object -> __o_vartags)) {
	  i_object -> __o_vartags -> tag =
	    o_parent -> __o_vartags -> tag;
	  i_object -> __o_vartags -> tag -> var_object = i_object;
	  i_object -> __o_vartags -> tag -> i = I_UNDEF;
	  o_parent -> __o_vartags -> tag = NULL;
	}
      }
    }
  }
  return ERROR;
}

bool reset_i_if_lval (OBJECT *obj, char *method_name) {

  VARENTRY *v;
  bool retval = false;
  /* this should be sufficient, instead of tokenizing the
     name again. */
  if (strchr (method_name, '=')) {
    if (!IS_EMPTY_VARTAG (obj -> __o_vartags)) {
      v = obj -> __o_vartags -> tag;
      if (v) {
	v -> i = v -> i_post = v -> i_temp = I_UNDEF;
	retval = true;
      }
      v = obj -> __o_vartags -> from;
      if (v) {
	v -> i = v -> i_post = v -> i_temp = I_UNDEF;
	retval = true;
      }
    }
  }
  return retval;
}

static int __basicnew_ext = 0;
static char *__basicnew_label_str = "@_basicNew_%d_";

/* A VARENTRY that has a var_decl with '@' as the first
 * character is not attached to a method (so far) and 
 * should be deleted if it is attached to a VARTAG.
 * Even more reliably, the object has the OBJECT_HAS_LOCAL_TAG
 * attribute set.  We also set is_local to True in the VARENTRY,
 * so the deletion needs to be on a local tag with an object
 * that created it locally.
 * (See delete_vartag (), above.)
 */
void __ctalkAddBasicNewTag (OBJECT *s) {
  VARENTRY *v;
  if (s -> __o_vartags) {
    v = new_varentry (s);
    v -> is_local = true;
    s -> __o_vartags -> tag = v;
    strcpy (v -> var_decl -> class, s -> CLASSNAME);
    sprintf (v ->  var_decl -> name, __basicnew_label_str, __basicnew_ext++);
    s -> attrs |= OBJECT_HAS_LOCAL_TAG;
  }
}

void reset_primary_tag (VARTAG *v) {
  if (v) {
    if (!IS_EMPTY_VARTAG(v)) {
      v -> tag -> i = 
	v -> tag -> i_post =
	v -> tag -> i_temp = I_UNDEF;
    }
  }
}


  /* 
     If we're re-assigning a Key to the start of a collection... 
     check that the rvalue is the same as in the expression.  If so,
     reset the original tag.

     This is the case that handles a sequence like this:

     Key new k

     k = *myCollection;
     ...
     k += 2;
     ...
     k = *myCollection;     by now, k is derived from an i tag,
                            so retrieve the original object from
			    the 'from' tag, reset it, and return
			    the 'from' tag (which is the tag of
			    the from -> var_object.  Also check
                            that the argument matches the text
                            of the argument in the source, so
                            we know for sure to reset the i
                            values.

   */
VARENTRY *reset_if_re_assigning_collection (OBJECT *rcvr, OBJECT *target,
					    OBJECT **rcvr_return) {
  VARENTRY *v = NULL;

  *rcvr_return = NULL;

  if (!IS_OBJECT (rcvr) || !IS_OBJECT (target))
    return NULL;

  if (IS_EMPTY_VARTAG(rcvr -> __o_vartags)) {
    if (IS_VARENTRY (rcvr -> __o_vartags -> from)) {
      if (rcvr -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) {
	if (IS_OBJECT(rcvr -> __o_p_obj) && 
	    IS_OBJECT (rcvr -> __o_vartags -> from -> var_object) &&
	    IS_OBJECT (rcvr -> __o_vartags -> from-> var_object -> __o_p_obj)){
	  if (rcvr -> __o_p_obj ==
	      rcvr -> __o_vartags -> from -> var_object -> __o_p_obj) {
	    /* Checking the argument against the argument text probably
	       needs to be upgraded later. */
	    if (strstr (__call_stack[__call_stack_ptr + 1] -> arg_text,
			target -> __o_name) ||
		strstr (__call_stack[__call_stack_ptr + 1] -> arg_text,
			target -> __o_p_obj -> __o_name)) {
	      *rcvr_return = rcvr -> __o_vartags -> from -> var_object;
	      if (IS_VARENTRY (rcvr -> __o_vartags -> from -> var_object ->
			     __o_vartags -> tag)) {
		v = rcvr -> __o_vartags -> from 
		  -> var_object -> __o_vartags -> tag;
		reset_primary_tag ((*rcvr_return) -> __o_vartags);
	      }
	    }
	  }
	}
      }
    }
  }
  return v;
}
