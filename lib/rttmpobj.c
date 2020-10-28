/* $Id: rttmpobj.c,v 1.2 2020/10/28 10:29:20 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2020  Robert Kiesling, rk3314042@gmail.com.
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

int delete_all_objs_expr_frame (EXPR_PARSER *p,
				int idx, int *all_objects_deleted) {
  OBJECT *o;
  int i;
  int have_created_param;

  o = p -> m_s[idx] -> obj;

  have_created_param = 0;

  if (!IS_OBJECT(o))
    return SUCCESS;

  if (o -> nrefs <= 0) {
    for (i = p -> msg_frame_start; i >= p -> msg_frame_top; i--) {

      if (!p -> m_s[i] || !IS_MESSAGE(p -> m_s[i]))
	break;

      if ((p -> m_s[i] -> obj == o) ||
	  is_p_obj (o, p -> m_s[i] -> obj)) {
	if ((p -> m_s[i] -> obj -> nrefs == 0) || 
	    (p -> m_s[i] -> attrs & RT_TOK_OBJ_IS_CREATED_PARAM)) {
	  have_created_param = TRUE;
	  p -> m_s[i] -> attrs &= ~RT_TOK_OBJ_IS_CREATED_PARAM;
	}
	p -> m_s[i] -> obj = NULL;
	if (p -> m_s[i] -> attrs & RT_OBJ_IS_INSTANCE_VAR)
	  p -> m_s[i] -> attrs &= ~RT_OBJ_IS_INSTANCE_VAR;
	if (p -> m_s[i] -> attrs & RT_OBJ_IS_CLASS_VAR)
	  p -> m_s[i] -> attrs &= ~RT_OBJ_IS_CLASS_VAR;
      }

      if ((p -> m_s[i] -> value_obj == o) ||
	  is_p_obj (o, p -> m_s[i] -> value_obj)) {
	p -> m_s[i] -> value_obj = NULL;
	if (p -> m_s[i] -> attrs & RT_VALUE_OBJ_IS_INSTANCE_VAR)
	  p -> m_s[i] -> attrs &= ~RT_VALUE_OBJ_IS_INSTANCE_VAR;
	if (p -> m_s[i] -> attrs & RT_VALUE_OBJ_IS_CLASS_VAR)
	  p -> m_s[i] -> attrs &= ~RT_VALUE_OBJ_IS_CLASS_VAR;
      }

      if ((p -> m_s[i] -> receiver_obj == o) ||
	  is_p_obj (o, p -> m_s[i] -> receiver_obj)) {
	p -> m_s[i] -> receiver_obj = NULL;
      }

    }

    if (!IS_VALUE_INSTANCE_VAR (o) || (have_created_param > 0)) { 
      __ctalkDeleteObject (o);
      *all_objects_deleted = TRUE;
    }

    return -1;

  } else {
    if (have_created_param) {
      __ctalkDeleteObject (o);
      *all_objects_deleted = TRUE;
    } else {
      __ctalkDeleteObjectInternal (o);
      *all_objects_deleted = FALSE;
    }
    return o -> nrefs;
  }
}

int delete_all_value_objs_expr_frame (MESSAGE_STACK messages,
				int idx,
				int start_idx,
				int end_idx,
				int *all_objects_deleted) {
  OBJECT *o;
  int i;
  o = messages[idx] -> value_obj;
  if (!IS_OBJECT(o))
    return SUCCESS;
  if (o -> nrefs <= 0) {
    for (i = start_idx; i >= end_idx; i--) {
      if (!messages[i] || !IS_MESSAGE(messages[i]))
	break;
      if ((messages[i] -> value_obj == o) ||
	  is_p_obj (o, messages[i] -> value_obj)) {
	messages[i] -> value_obj = NULL;
	if (messages[i] -> attrs & RT_VALUE_OBJ_IS_INSTANCE_VAR)
	  messages[i] -> attrs &= ~RT_VALUE_OBJ_IS_INSTANCE_VAR;
	if (messages[i] -> attrs & RT_VALUE_OBJ_IS_CLASS_VAR)
	  messages[i] -> attrs &= ~RT_VALUE_OBJ_IS_CLASS_VAR;
      }
    }
    __ctalkDeleteObjectInternal (o);
    *all_objects_deleted = TRUE;
    return -1;
  } else {
    __ctalkDeleteObjectInternal (o);
    *all_objects_deleted = FALSE;
    return o -> nrefs;
  }
}

int _cleanup_temporary_objects (OBJECT *obj, OBJECT *result_obj, OBJECT *subexpr_obj,
				RT_CLEANUP_MODE cleanup_mode) {

  int nrefs;
  if (!IS_OBJECT(obj)) {
    return ERROR;
  }

  if (object_is_deletable(obj, result_obj, subexpr_obj)) {
    if (obj -> scope & METHOD_USER_OBJECT) {
      if (obj -> nrefs > 0)
	__ctalkDeleteObjectInternal (obj);
      return obj -> nrefs;
    } else {
      __ctalkDeleteObject (obj);
      nrefs = 0;
      return nrefs;
    }
  }
  if (IS_OBJECT(obj) && 
      obj->__o_p_obj && is_receiver(obj->__o_p_obj))
    return ERROR;

  if (IS_OBJECT(obj) && 
      obj->__o_p_obj && is_arg(obj->__o_p_obj))
    return ERROR;

  if (IS_OBJECT(obj) &&
      object_is_decrementable(obj, result_obj, subexpr_obj)) {
    nrefs = obj -> nrefs;
    --nrefs;

    if (obj -> nrefs < 0)
      __objRefCntZero (OBJREF(obj));

    /*
     *  Fixup if we have a saved method resource object 
     *  cross linked with a parameter created by eval_expr ().  
     *  It might be better simply to do the fixup than try to
     *  make an exception in the *SaveCVARResource*, if that's
     *  even possible, or in eval_expr () - These are fairly 
     *  isolated.
     */
    if (obj -> instancevars) {
      if ((obj -> scope & CREATED_PARAM) &&
	  (obj -> instancevars == result_obj) &&
	  (obj -> instancevars -> scope & METHOD_USER_OBJECT))
	obj -> instancevars = NULL;  /* Detach the the result_obj. */
    }

    if ((obj -> scope == ARG_VAR) && (obj -> nrefs <= 1))
      __objRefCntZero (OBJREF(obj));

    __ctalkDeleteObjectInternal (obj);

    if (nrefs < 0)
      return 0;
    if ((obj -> scope & CREATED_PARAM) &&
	(obj -> nrefs == 0)) {
      __ctalkDeleteObjectInternal (obj);
      return 0;
    }
    if ((obj -> scope & SUBEXPR_CREATED_RESULT) &&
	(obj -> nrefs == 0)) {
      __ctalkDeleteObjectInternal (obj);
      return 0;
    }
    if (cleanup_mode == rt_cleanup_obj_ref) {
      if (obj && *(char *)obj == 'O' && 
	  IS_OBJECT(obj) && (obj -> nrefs == 0)) 
	__ctalkDeleteObjectInternal (obj);
    }
    return ((nrefs > 0) ? nrefs : 0);
  }
  if (obj -> scope & VAR_REF_OBJECT && obj -> scope & CVAR_VAR_ALIAS_COPY
      && obj -> nrefs == 0) {
    __ctalkDeleteObjectInternal (obj);
  }
  return ERROR;
}

void _cleanup_temporary_objects_all_instances (OBJECT *obj, 
					       RT_CLEANUP_MODE cleanup_mode,
					       EXPR_PARSER *p,
					       int idx, 
					       int *all_objects_deleted) {
  int nrefs;
  if (!IS_OBJECT(obj)) {
    return;
  }
  if (object_is_deletable(obj, NULL, NULL)) {
    if (obj -> scope & METHOD_USER_OBJECT) {
      if (obj -> nrefs > 0)
	delete_all_objs_expr_frame (p, idx, all_objects_deleted);
      return;
    } else {
      delete_all_objs_expr_frame (p, idx, all_objects_deleted);
      nrefs = 0;
      return;
    }
  }
  if (obj->__o_p_obj && is_receiver(obj->__o_p_obj))
    return;

  if (object_is_decrementable(obj, NULL, NULL)) {
    nrefs = obj -> nrefs;
    --nrefs;
    if (obj -> nrefs < 0)
      __objRefCntZero (OBJREF(obj));
    delete_all_objs_expr_frame (p, idx,	all_objects_deleted);
    if (nrefs < 0)
      return;
    if ((obj -> scope & CREATED_PARAM) &&
	(obj -> nrefs == 0))
      delete_all_objs_expr_frame (p, idx, all_objects_deleted);
    if (cleanup_mode == rt_cleanup_obj_ref) {
      if (obj && *(char *)obj == 'O' && 
	  IS_OBJECT(obj) && (obj -> nrefs == 0)) 
	delete_all_objs_expr_frame (p, idx, all_objects_deleted);
    }
    return;
  }
}

/* 
 * Called from eval_expr () - we've already checked that
 * the message has a valid object.
 */
void clean_up_message_objects (MESSAGE_STACK messages,
			       MESSAGE *m, 
			       OBJECT *target_object, 
			       OBJECT *result_object,
			       OBJECT *subexpr_result,
			       int idx,
			       int stack_start_idx,
			       int check_value_object,
			       RT_CLEANUP_MODE mode) {
  
  int j;

  if (!check_value_object) {
    if ((m -> attrs & RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) ||
	(HAS_CREATED_CVAR_SCOPE(m->obj) && (m -> obj -> nrefs == 0))) {
      if ((m -> obj != result_object) && 
	  !(m -> obj -> scope & VAR_REF_OBJECT)) {
	__ctalkDeleteObject (m -> obj);
	for (j = idx + 1; j <= stack_start_idx; j++) {
	  if (messages[j] -> obj == m -> obj)
	    messages[j] -> obj = NULL;
	}
      }
      m -> obj = NULL;
    } else {
      if (m -> attrs & RT_TOK_OBJ_IS_CREATED_PARAM) {
	if ((m -> obj != result_object) &&
	    (m -> obj -> scope & CREATED_PARAM)) {
	  __ctalkDeleteObject (m -> obj);
	  for (j = idx + 1; j <= stack_start_idx; j++) {
	    if (messages[j] -> obj == m -> obj)
	      messages[j] -> obj = NULL;
	  }
	}
	m -> obj = NULL;
      } else {
	if (!_cleanup_temporary_objects (m -> obj, result_object, 
					 subexpr_result,
				       mode)) {
	  for (j = idx + 1; j <= stack_start_idx; j++) {
	    if (messages[j] -> obj == m -> obj)
	      messages[j] -> obj = NULL;
	  }
	  m -> obj = NULL;
	}
      }
    }
  } else { /* if (!check_value_object) */
    /* op *must* be ==, not & */
    if (m -> attrs == RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS) {
#if 0
      /* Should never be needed for a value object?   ... */
      if (m -> value_obj != result_object) {
	__ctalkDeleteObject (m -> value_obj);
	for (j = idx + 1; j <= stack_start_idx; j++) {
	  if (messages[j] -> obj == m -> value_obj)
	    messages[j] -> value_obj = NULL;
	}
      }
      m -> value_obj = NULL;
#endif
    } else {
      if (m -> attrs & RT_TOK_VALUE_OBJ_IS_NULL_RESULT) {
	if ((m -> value_obj -> attrs & OBJECT_IS_NULL_RESULT_OBJECT) &&
	    (m -> value_obj != result_object) &&
	    (m -> value_obj != subexpr_result) &&
	    !is_arg (m -> value_obj) &&
	    !is_receiver (m -> value_obj) &&
	    (mode == rt_cleanup_exit_delete)) {
	  __ctalkDeleteObject (m -> value_obj);
	  for (j = idx + 1; j <= stack_start_idx; j++) {
	    if (messages[j] -> value_obj == m -> value_obj)
	      messages[j] -> value_obj = NULL;
	  }
	  m -> value_obj = NULL;
	}
      } else {
	if (IS_OBJECT (m -> obj) &&
	    (m -> obj -> scope & TYPECAST_OBJECT) &&
	    /* Find out if a subexpr result is safe to delete here. */
	    /*      (obj -> scope & SUBEXPR_CREATED_RESULT) && */
	    (m -> obj != result_object) &&
	    !is_arg (m -> obj) &&
	    /* TODO - Make sure it's safe to omit this. */
	    /*	    (m -> obj != subexpr_result) && */
	    !is_receiver (m -> obj)) {
	  if (!(m -> obj -> scope & METHOD_USER_OBJECT)) {
	    __ctalkDeleteObject (m -> obj);
	    for (j = idx + 1; j <= stack_start_idx; j++) {
	      if (messages[j] -> obj == m -> obj)
		messages[j] -> obj = NULL;
	    }
	    m -> obj = NULL;
	  }
	} else {
	  if (!_cleanup_temporary_objects (m -> value_obj, 
					   result_object, subexpr_result,
					   mode)) {
	    for (j = idx + 1; j <= stack_start_idx; j++) {
	      if (messages[j] -> value_obj == m -> value_obj)
		messages[j] -> value_obj = NULL;
	    }
	    m -> value_obj = NULL;
	  }
	}
      }
    }
  }
}

int cvar_alias_rcvr_created_here (MESSAGE_STACK messages, int stack_start,
				  int stack_end, OBJECT *rcvr_obj,
				  OBJECT *e_result) {
  int i, j;
  OBJECT *__r;
  char buf[MAXLABEL];

  if (!IS_OBJECT(rcvr_obj))
    return FALSE;

  /* I.e., if it could be the result object, don't delete it yet. */
  if (rcvr_obj == e_result)
    return FALSE;

  /* Also don't delete if rcvr is already saved as an extra object
     by a method. */
  if (rcvr_obj -> scope & METHOD_USER_OBJECT)
    return FALSE;

  /* Or is now a collection member. */
  if (rcvr_obj -> scope & VAR_REF_OBJECT)
    return FALSE;

  /* If the cvar was the operand of an assignment statement,
     then it should have a tag now. */
  if (!IS_EMPTY_VARTAG (rcvr_obj -> __o_vartags))
    return FALSE;

  for (i = stack_start; i >= stack_end; i--) {
    if ((messages[i] -> obj == rcvr_obj) &&
	(messages[i] -> attrs & RT_TOK_OBJ_IS_CREATED_CVAR_ALIAS)) {

      if (IS_OBJECT(rcvr_obj)) {
	/* if the CVAR contains a reference, handle it here, because
	   this condition might not be unique when we're deep in 
	   __ctalkDeleteObject* */
	if (IS_OBJECT(rcvr_obj -> instancevars)) {
	  if (rcvr_obj -> instancevars -> attrs &
	      OBJECT_VALUE_IS_BIN_SYMBOL) {
	    if ((__r = *(OBJECT **)(rcvr_obj -> instancevars -> __o_value))
		!= NULL) {
	      if (obj_ref_str_2 (htoa (buf, (unsigned long)__r),
				 __r)) {
		__ctalkRegisterUserObject (__r);
		rcvr_obj -> instancevars -> __o_value[0] = 0;
		rcvr_obj -> __o_value[0] = 0;
	      }
	    }
	  }
	} else {
	  if ((__r = obj_ref_str_2 (rcvr_obj -> __o_value, rcvr_obj))
	      != NULL) {
	    __ctalkRegisterUserObject (__r);
	    rcvr_obj -> __o_value[0] = 0;
	  }
	}
	__objRefCntZero (OBJREF(rcvr_obj));
	__ctalkDeleteObject (rcvr_obj);
      }
      for (j = stack_start; j >= stack_end; j--) {
	if (messages[j] -> obj == rcvr_obj) {
	  messages[j] -> obj = NULL;
	}
      }
      return TRUE;
    }
  }
  return FALSE;
}

bool cleanup_subexpr_created_arg (EXPR_PARSER *p,
				 OBJECT *arg_object, 
				 OBJECT *e_result) {
  int i, j;

  /* Again, if it could be the result object, don't delete it yet. */
  if (arg_object == e_result)
    return false;

  for (i = p -> msg_frame_start; i >= p -> msg_frame_top; i--) {

    if (IS_OBJECT (p -> m_s[i] -> value_obj) && 
	(p -> m_s[i] -> value_obj == arg_object)) {
      if (p -> m_s[i] -> attrs & RT_TOK_OBJ_IS_CREATED_PARAM) {
	if ((p -> m_s[i] -> value_obj -> scope & SUBEXPR_CREATED_RESULT) &&
	    !(p -> m_s[i] -> value_obj -> scope & VAR_REF_OBJECT)) {
	  __ctalkDeleteObject (arg_object);
	}
	for (j = p -> msg_frame_start; j >= p -> msg_frame_top; j--) {
	  if (p -> m_s[j] -> value_obj == arg_object) {
	    p -> m_s[j] -> value_obj = NULL;
	  }
	}
	return true;
      }
    }
  }
  return false;
}

