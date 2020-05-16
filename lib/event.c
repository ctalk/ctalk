/* $Id: event.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2016, 2019  Robert Kiesling, rk3314042@gmail.com.
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

/*
 *  Event handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

#define SIGNALEVENT_CLASS "SignalEvent"
#define APPLICATIONEVENT_CLASS "ApplicationEvent"

/* This is the same as the definition in List class. */
#define __LIST_HEAD(__o) ((__o)->instancevars->next)

extern DEFAULTCLASSCACHE *rt_defclasses;

int __ctalkNewSignalEventInternal (int signo, int pid, char *text) {
  OBJECT *class_object, *eventlist_object, *new_event,
    *signo_var, *pid_var, *data_var, *var_ptr, *t;
  OBJECT *key_object;
  char buf[MAXMSG];

  if ((class_object = get_class_by_name ("SignalEvent")) == NULL) {
    __ctalkExceptionInternal (NULL, undefined_class_x, "SignalEvent", 0);
    return ERROR;
  }

  if ((eventlist_object = 
       __ctalkGetClassVariable (class_object, "pendingEvents", TRUE))
      == NULL) {
    return ERROR;
  }

  new_event = __ctalkCreateObjectInit ("__newEvent", 
				       "SignalEvent", "Event",
				       LOCAL_VAR|VAR_REF_OBJECT, NULL);
  var_ptr = new_event -> instancevars;

  signo_var = create_object_init_internal ("sigNo",
					   rt_defclasses -> p_integer_class,
					   LOCAL_VAR|VAR_REF_OBJECT, "");
  SETINTVARS(signo_var, signo);

  var_ptr -> next = signo_var;
  var_ptr = signo_var;

  pid_var = create_object_init_internal ("processID",
					 rt_defclasses -> p_integer_class,
					 LOCAL_VAR|VAR_REF_OBJECT, "");
  SETINTVARS(pid_var, pid);

  var_ptr -> next = pid_var;
  var_ptr = pid_var;

  data_var = create_object_init_internal ("text",
					  rt_defclasses -> p_string_class,
					  LOCAL_VAR|VAR_REF_OBJECT, text);
  var_ptr -> next = data_var;

  __objRefCntSet (&new_event, eventlist_object -> nrefs);

  key_object = create_object_init_internal
    ("__ctalkNewSignalEventInternal_key",
     rt_defclasses -> p_key_class, LOCAL_VAR|VAR_REF_OBJECT, "");
  *(OBJECT **)key_object -> instancevars -> __o_value = new_event;

  __objRefCntSet (&key_object, eventlist_object -> nrefs);
  if (__LIST_HEAD(eventlist_object)) {
    for (t = eventlist_object -> instancevars -> next; t && t -> next; t = t -> next)
      ;
    t -> next = key_object;
    key_object -> prev = t;
  } else {
    __LIST_HEAD(eventlist_object) = key_object;
  }

  return SUCCESS;
}

int __ctalkNewApplicationEventInternal (int pid, char *text) {
  return SUCCESS;
}
