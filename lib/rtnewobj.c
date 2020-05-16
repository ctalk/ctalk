/* $Id: rtnewobj.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

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

/* 
 * Run time object constructors, destructors, and basic 
 * garbage collection.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

extern I_PASS interpreter_pass; /* Declared in rtinfo.c. */

#define NULLSTR_BUF_LENGTH 7 /* "(null)" + 1 */

void __call_delete_object_internal (OBJECT *);

extern OBJECT *__ctalk_classes;

extern DEFAULTCLASSCACHE *rt_defclasses; /* Declared in rtclslib.c. */

int object_size = 0;

void init_object_size (void) {
  object_size = sizeof (struct _object);
}


OBJECT *create_object_init_internal (char *name, OBJECT *class,
				     int scope, char *value) {
  OBJECT *o, *o_value;
  char *o_value_ptr, *o_value_value_ptr;
  int bool_value;

  o = (OBJECT *)__xalloc (object_size);
  o_value = (OBJECT *)__xalloc (object_size);

  o -> instancevars = o_value;
  o_value -> __o_p_obj = o;

#ifndef __sparc
  o -> sig = 0xd3d3d3;
#else
  strcpy (o -> sig, "OBJECT");
#endif
  if (name) {
    memset (o -> __o_name, 0, MAXLABEL);
    strncpy (o -> __o_name, name, MAXLABEL - 1);
  } else {
    strcpy (o -> __o_name, NULLSTR);
  }

  if (str_eq (class -> __o_classname, "Class")) {
    o -> __o_class = class;
    o -> __o_superclass = class -> __o_superclass;
  } else {
    if (interpreter_pass == run_time_pass) {

#ifdef USE_CLASSNAME_STR
      strcpy (o -> __o_classname, class -> __o_classname);
      strcpy (o_value -> __o_classname, class -> __o_value);
#endif

#ifdef USE_SUPERCLASSNAME_STR
      if (superclass) {
	strcpy (o -> __o_superclassname, class -> __o_superclass -> __o_name);
      }
#endif

      o -> __o_class = class;
      o -> __o_superclass = o -> __o_class -> __o_superclass;

    } else {

      strcpy (o -> __o_classname, class -> __o_name);
      strcpy (o_value -> __o_classname, class -> __o_value);
      o -> __o_class = class;
      if (IS_OBJECT(o -> __o_class))
	o -> __o_superclass = o -> __o_class -> __o_superclass;

    }
  }

  o -> scope = scope;

#ifndef __sparc__
  o_value -> sig = 0xd3d3d3;
#else
  strcpy (o_value -> sig, "OBJECT");
#endif

  strcpy (o_value -> __o_name, "value");
  o_value -> attrs = OBJECT_IS_VALUE_VAR;
  o_value -> __o_class = o -> __o_class;
  o_value -> __o_superclass = o -> __o_superclass;
  o_value -> scope = scope;

  if (value) {
    if (str_eq (value, NULLSTR)) {
      if (o -> __o_class -> attrs & ZERO_LENGTH_STR_INIT) {
	o -> __o_value = strdup ("");
	o -> instancevars -> __o_value = strdup ("");
      } else if (o -> __o_class -> attrs & INT_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (INTBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
	o -> attrs |= OBJECT_VALUE_IS_BIN_INT;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
      } else if (o -> __o_class -> attrs & LONGLONG_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (LLBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (LLBUFSIZE);
	o -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
      } else if (o -> __o_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (PTRBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (PTRBUFSIZE);
	o -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
      } else {
	o -> __o_value = strdup (NULLSTR);
	o -> instancevars -> __o_value = strdup (NULLSTR);
      }
    } else {
      if (o -> __o_class -> attrs & INT_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (INTBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
	errno = 0;
	if (*value != '\0') {
	  *(int *)o -> __o_value = 
	    *(int *)o -> instancevars -> __o_value =
	    strtol (value, NULL, 0);
	}
	if (errno) {
	  if (*value) {
	    if (strlen (value) == 1) {
	      if (isascii((int)value[0])) {
		int intval = (int)*value;
		*(int *)o -> __o_value = 
		  *(int *)o -> instancevars -> __o_value = intval;
	      }
	    } else {
	      fprintf (stderr, "ctalk: can't translate %s to an int.\n",
		       value);
	    }
	  }
	}
	o -> attrs |= OBJECT_VALUE_IS_BIN_INT;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
      } else if (o -> __o_class -> attrs & CHAR_BUF_SIZE_INIT) {
	/* E.g., Character class - This needs to be initialized
	 like a binary int value buffer, so we can treat them
	 the same when needed. */
	o -> __o_value = __xalloc (INTBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
	if (value) {
	  strcpy (o -> __o_value, value);
	  strcpy (o -> instancevars -> __o_value, value);
	}
      } else if (o -> __o_class -> attrs & BOOL_BUF_SIZE_INIT) {
	/* We'll just use an int-size buf here too, so we can
	   just use ints */
	o -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
	o -> __o_value = __xalloc (INTBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
	errno = 0;
	bool_value = strtol (value, NULL, 0);
	if (!errno) {
	  *o -> __o_value = 
	    *o -> instancevars -> __o_value = (bool_value) ? 1 : 0;
	} else if (*value == '\0') {
	  *o -> __o_value = 
	    *o -> instancevars -> __o_value = 0;
	} else {
	  *o -> __o_value = 
	    *o -> instancevars -> __o_value = 1;
	}
      } else if (o -> __o_class -> attrs & LONGLONG_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (LLBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (LLBUFSIZE);
	errno = 0;
	if (*value != '\0') {
	  *(long long int *)o -> __o_value = 
	    *(long long int *)o -> instancevars -> __o_value =
	    strtoll (value, NULL, 0);
	}
	if ((scope != CREATED_PARAM) && errno) {
	  /* It's normal for temporary objects to have the same value
	     as the name, regardless of what it is, so if the name
	     is non-numeric, its okay for the value to be also. 
	     But try the conversion anyway for now. */
	  /* if (errno) { */
	  fprintf (stderr, "ctalk: can't translate %s to a long long int.\n",
		   *value ? value : NULLSTR);
	}
	o -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
      } else if (o -> __o_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (PTRBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (PTRBUFSIZE);
	if (*value != '\0') {
	  /* The "value" param is a char *, so this should work
	     anywhere. */
	  errno = 0;
	  /* use strtoll and cast so we don't have any ERANGE issues */
	  *(long int *)o -> __o_value = 
	    *(long int *)o -> instancevars -> __o_value =
	    (long)strtoll (value, NULL, 0);
	  if ((scope != CREATED_PARAM) && errno) {
	    /* see the comment above */
	    /* Unless it's a Vector object, which we handle 
	       in a new or basicNew method. */
	    if (!str_eq (class -> __o_name, "Vector"))
	      fprintf (stderr, "ctalk: can't translate %s to a pointer.\n",
		       *value ? value : NULLSTR);
	  }
	}
	o -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
      } else {
	o -> __o_value = strdup (value);
	o -> instancevars -> __o_value = strdup (value);
      }
    }
  } else { /* if (value) */
    if (o -> __o_class -> attrs & ZERO_LENGTH_STR_INIT) {
      o -> __o_value = strdup ("");
      o -> instancevars -> __o_value = strdup ("");
    } else if (o -> __o_class -> attrs & INT_BUF_SIZE_INIT) {
      o -> __o_value = __xalloc (INTBUFSIZE);
      o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
      o -> attrs |= OBJECT_VALUE_IS_BIN_INT;
      o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
    } else if (o -> __o_class -> attrs & BOOL_BUF_SIZE_INIT) {
      /* We'll just use an int-size buf here too, so we can
	 just use ints */
      o -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
      o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
      o -> __o_value = __xalloc (INTBUFSIZE);
      o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
    } else if (o -> __o_class -> attrs & LONGLONG_BUF_SIZE_INIT) {
      o -> __o_value = __xalloc (LLBUFSIZE);
      o -> instancevars -> __o_value = __xalloc (LLBUFSIZE);
      o -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
      o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
    } else if (o -> __o_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
      o -> __o_value = __xalloc (PTRBUFSIZE);
      o -> instancevars -> __o_value = __xalloc (PTRBUFSIZE);
      o -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
      o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
    } else {
      o -> __o_value = strdup (NULLSTR);
      o -> instancevars -> __o_value = strdup (NULLSTR);
    }
  }

  o -> __o_vartags = new_vartag (NULL);

  return o;
}

/* We shouldn't be needing all the value initialization stuff here. */
OBJECT *create_class_object_init (char *name, char *class, 
				  char *superclass, int scope,
				  char *value) {
  OBJECT *o, *o_value;
  char *o_value_ptr, *o_value_value_ptr;
  int bool_value;

  o = (OBJECT *)__xalloc (object_size);
  o_value = (OBJECT *)__xalloc (object_size);

  o -> instancevars = o_value;
  o_value -> __o_p_obj = o;

#ifndef __sparc
  o -> sig = 0xd3d3d3;
#else
  strcpy (o -> sig, "OBJECT");
#endif
  if (name) {
    memset (o -> __o_name, 0, MAXLABEL);
    strncpy (o -> __o_name, name, MAXLABEL - 1);
  } else {
    strcpy (o -> __o_name, NULLSTR);
  }

  if (str_eq (class, "Class")) {
    o -> __o_class = o;
    o -> __o_superclass = __ctalkGetClass (superclass);
  } else {
    if (interpreter_pass == run_time_pass) {

#ifdef USE_CLASSNAME_STR
      strcpy (o -> __o_classname, class);
      strcpy (o_value -> __o_classname, class);
#endif

#ifdef USE_SUPERCLASSNAME_STR
      if (superclass) {
	strcpy (o -> __o_superclassname, superclass);
      }
#endif

      o -> __o_class = __ctalkGetClass (class);
      o -> __o_superclass = o -> __o_class -> __o_superclass;

    } else {

      strcpy (o -> __o_classname, class);
      strcpy (o_value -> __o_classname, class);
      o -> __o_class = get_class_object (class);
      if (IS_OBJECT(o -> __o_class))
	o -> __o_superclass = o -> __o_class -> __o_superclass;

    }
  }

  o -> scope = scope;

#ifndef __sparc__
  o_value -> sig = 0xd3d3d3;
#else
  strcpy (o_value -> sig, "OBJECT");
#endif

  strcpy (o_value -> __o_name, "value");
  o_value -> attrs = OBJECT_IS_VALUE_VAR;
  o_value -> __o_class = o -> __o_class;
  o_value -> __o_superclass = o -> __o_superclass;
  o_value -> scope = scope;

  if (value) {
    if (str_eq (value, NULLSTR)) {
      if (o -> __o_class -> attrs & ZERO_LENGTH_STR_INIT) {
	o -> __o_value = strdup ("");
	o -> instancevars -> __o_value = strdup ("");
      } else if (o -> __o_class -> attrs & INT_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (INTBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
	o -> attrs |= OBJECT_VALUE_IS_BIN_INT;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
      } else if (o -> __o_class -> attrs & LONGLONG_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (LLBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (LLBUFSIZE);
	o -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
      } else if (o -> __o_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (PTRBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (PTRBUFSIZE);
	o -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
      } else {
	o -> __o_value = strdup (NULLSTR);
	o -> instancevars -> __o_value = strdup (NULLSTR);
      }
    } else {
      if (o -> __o_class -> attrs & INT_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (INTBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
	errno = 0;
	if (*value != '\0') {
	  *(int *)o -> __o_value = strtol (value, NULL, 0);
	  *(int *)o -> instancevars -> __o_value = strtol (value, NULL, 0);
	}
	if (errno) {
	  fprintf (stderr, "ctalk: can't translate %s to an int.\n",
		   *value ? value : NULLSTR);
	}
	o -> attrs |= OBJECT_VALUE_IS_BIN_INT;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
      } else if (o -> __o_class -> attrs & CHAR_BUF_SIZE_INIT) {
	/* E.g., Character class - This needs to be initialized
	 like a binary int value buffer, so we can treat them
	 the same when needed. */
	o -> __o_value = __xalloc (INTBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
	if (value) {
	  strcpy (o -> __o_value, value);
	  strcpy (o -> instancevars -> __o_value, value);
	}
      } else if (o -> __o_class -> attrs & BOOL_BUF_SIZE_INIT) {
	/* We'll just use an int-size buf here too, so we can
	   just use ints */
	o -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
	o -> __o_value = __xalloc (INTBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
	errno = 0;
	bool_value = strtol (value, NULL, 0);
	if (!errno) {
	  *o -> __o_value = 
	    *o -> instancevars -> __o_value = (bool_value) ? 1 : 0;
	} else if (*value == '\0') {
	  *o -> __o_value = 
	    *o -> instancevars -> __o_value = 0;
	} else {
	  *o -> __o_value = 
	    *o -> instancevars -> __o_value = 1;
	}
      } else if (o -> __o_class -> attrs & LONGLONG_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (LLBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (LLBUFSIZE);
	errno = 0;
	if (*value != '\0') {
	  *(long long int *)o -> __o_value = 
	    *(long long int *)o -> instancevars -> __o_value =
	    strtoll (value, NULL, 0);
	}
	if (errno) {
	  fprintf (stderr, "ctalk: can't translate %s to a long long int.\n",
		   *value ? value : NULLSTR);
	}
	o -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
      } else if (o -> __o_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
	o -> __o_value = __xalloc (PTRBUFSIZE);
	o -> instancevars -> __o_value = __xalloc (PTRBUFSIZE);
	if (*value != '\0') {
	  *(long int *)o -> __o_value = 
	    *(long int *)o -> instancevars -> __o_value =
	    strtol (value, NULL, 0);
	}
	o -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
	o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
      } else {
	o -> __o_value = strdup (value);
	o -> instancevars -> __o_value = strdup (value);
      }
    }
  } else { /* if (value) */
    if (o -> __o_class -> attrs & ZERO_LENGTH_STR_INIT) {
      o -> __o_value = strdup ("");
      o -> instancevars -> __o_value = strdup ("");
    } else if (o -> __o_class -> attrs & INT_BUF_SIZE_INIT) {
      o -> __o_value = __xalloc (INTBUFSIZE);
      o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
      o -> attrs |= OBJECT_VALUE_IS_BIN_INT;
      o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_INT;
    } else if (o -> __o_class -> attrs & BOOL_BUF_SIZE_INIT) {
      /* We'll just use an int-size buf here too, so we can
	 just use ints */
      o -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
      o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
      o -> __o_value = __xalloc (INTBUFSIZE);
      o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
    } else if (o -> __o_class -> attrs & LONGLONG_BUF_SIZE_INIT) {
      o -> __o_value = __xalloc (LLBUFSIZE);
      o -> instancevars -> __o_value = __xalloc (LLBUFSIZE);
      o -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
      o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_LONGLONG;
    } else if (o -> __o_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
      o -> __o_value = __xalloc (PTRBUFSIZE);
      o -> instancevars -> __o_value = __xalloc (PTRBUFSIZE);
      o -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
      o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_SYMBOL;
    } else {
      o -> __o_value = strdup (NULLSTR);
      o -> instancevars -> __o_value = strdup (NULLSTR);
    }
  }

  o -> __o_vartags = new_vartag (NULL);

  return o;
}

OBJECT *__ctalkCreateObjectInit (char *name, char *class, 
				    char *superclass, int scope,
				    char *value) {
  OBJECT *classobj = __ctalkGetClass (class);
  
  /* That means we're defining a class object */
  if (classobj == NULL) {
    return create_class_object_init (name, class, superclass, scope, value);
  } else {
    return create_object_init_internal (name, classobj, scope, value);
  }
}
 
 
OBJECT *__ctalkCreateObject (char *name, char *class, char *superclass,
			       int scope) {
  OBJECT *o;
  char *o_value_ptr;

  o = (OBJECT *)__xalloc (object_size);
  o_value_ptr = (char *)__xalloc (NULLSTR_BUF_LENGTH);

  o -> __o_value = o_value_ptr;

#ifndef __sparc__
  o -> sig = 0xd3d3d3;
#else
  strcpy (o -> sig, "OBJECT");
#endif
  memset (o -> __o_name, 0, MAXLABEL);
  strncpy (o -> __o_name, name, MAXLABEL - 1);
  if (str_eq (class, "Class")) {
    o -> __o_class = o;
  } else {
    if (interpreter_pass == run_time_pass) {
#ifdef USE_CLASSNAME_STR
      strcpy (o -> __o_classname, class);
#endif
#ifdef USE_SUPERCLASSNAME_STR
      if (superclass) {
	strcpy (o -> __o_superclassname, superclass);
      }
#endif
      o -> __o_class = __ctalkGetClass (class);
    } else {
      strcpy (o -> __o_classname, class);
      o -> __o_class = get_class_object (class);
    }
  }
  if (IS_OBJECT(o -> __o_class))
    o -> __o_superclass = o -> __o_class -> __o_superclass;

  o -> scope = scope;
  if (o -> __o_class -> attrs & ZERO_LENGTH_STR_INIT) {
    *(o -> __o_value) = 0;
  } else {
    strcpy (o -> __o_value, NULLSTR);
  }

  o -> __o_vartags = new_vartag (NULL);

  return o;
  
}

/* specifically for use with copy_object_internal, this saves us
   an extra call to __ctalkSetObjectValue for each object. */
OBJECT *__create_object_internal (char *name, char *class, char *superclass,
				  char *value, int scope, int attrs) {
  OBJECT *o;
  char *o_value_ptr;

  o = (OBJECT *)__xalloc (sizeof (struct _object));

#ifndef __sparc__
  o -> sig = 0xd3d3d3;
#else
  strcpy (o -> sig, "OBJECT");
#endif
  memset (o -> __o_name, 0, MAXLABEL);
  strncpy (o -> __o_name, name, MAXLABEL - 1);
  if (str_eq (class, "Class")) {
    o -> __o_class = o;
  } else {
    if (interpreter_pass == run_time_pass) {
#ifdef USE_CLASSNAME_STR
      strcpy (o -> __o_classname, class);
#endif
#ifdef USE_SUPERCLASSNAME_STR
      if (superclass) {
	strcpy (o -> __o_superclassname, superclass);
      }
#endif
      o -> __o_class = __ctalkGetClass (class);
    } else {
      strcpy (o -> __o_classname, class);
      o -> __o_class = get_class_object (class);
    }
  }
  if (IS_OBJECT(o -> __o_class))
    o -> __o_superclass = o -> __o_class -> __o_superclass;

  o -> scope = scope;
  o -> attrs = attrs;
  if (attrs & OBJECT_VALUE_IS_BIN_INT) {
    o -> __o_value = __xalloc (INTBUFSIZE);
    if (value != NULL)
      memcpy ((void *)o -> __o_value, (void *)value, sizeof (int));
  } else if (attrs & OBJECT_VALUE_IS_BIN_BOOL) {
    o -> __o_value = __xalloc (BOOLBUFSIZE);
    if (value != NULL)
      memcpy ((void *)o -> __o_value, (void *)value, BOOLBUFSIZE);
  } else if (o -> __o_class -> attrs & INT_BUF_SIZE_INIT) {
    o -> __o_value = __xalloc (INTBUFSIZE);
    if (value != NULL) 
      memcpy ((void *)o -> __o_value, (void *)value, sizeof (int));
    o -> attrs |= OBJECT_VALUE_IS_BIN_INT;
  } else if (o -> __o_class -> attrs & CHAR_BUF_SIZE_INIT) {
    /* Here also, Character class needs to be initialized
       like a binary int value buffer, so we can directly use
       values as a *(int *) when needed. */
    o -> __o_value = __xalloc (INTBUFSIZE);
    if (value)
      strcpy (o -> __o_value, value);
  } else if (o -> __o_class -> attrs & LONGLONG_BUF_SIZE_INIT) {
    o -> __o_value = __xalloc (LLBUFSIZE);
    if (value != NULL)
      memcpy ((void *)o -> __o_value, (void *)value, sizeof (long long int));
  } else if (o -> __o_class -> attrs & SYMBOL_BUF_SIZE_INIT) {
    o -> __o_value = __xalloc (PTRBUFSIZE);
    if (value != NULL)
      memcpy ((void *)o -> __o_value, (void *)value, sizeof (uintptr_t));
  } else {
    o -> __o_value = strdup (value ? value : "");
  }
  o -> __o_vartags = new_vartag (NULL);

  return o;
  
}

/* From ctalklib - 
   The name of the new object is on the argument stack.
   The new object's class is the class of the receiver
   object. 
*/
extern int primitive_method_call_attrs; /* Declared in rt_methd.c */
OBJECT *__ctalk_new_object (ARG **args) {

  OBJECT *rcvr;

  rcvr = __ctalk_receiver_pop ();
  if (IS_OBJECT (rcvr)) {
    (void)__ctalk_receiver_push (rcvr);
  } else {
    _warning ("__ctalk_new_object: Invalid receiver object.\n");
    return NULL;
  }

#if 0
  if (!(primitive_method_call_attrs & METHOD_SUPER_ATTR)) {
#ifdef USE_CLASSNAME_STR
    strcpy (ARG_CLASSNAME(args[0]), rcvr -> __o_name);
#endif
#ifdef USE_SUPERCLASSNAME_STR
    strcpy (ARG_SUPERCLASSNAME(args[0]), rcvr -> __o_superclassname);
#endif
  }
#endif
  (void)__ctalkInstanceVarsFromClassObject (ARG_OBJECT(args[0]));
  __objRefCntZero (OBJREF(args[0]->obj));
  __ctalk_dictionary_add (ARG_OBJECT(args[0]));
  
  return ARG_OBJECT(args[0]);
}

/*
 *   Set an object's value, making sure that the value member's
 *   buffer is large enough for the value.
 */

int __ctalkSetObjectValue (OBJECT *__o, char *__value) {

  return __ctalkSetObjectValueVar (__o, __value);
}

/*
 *  This should replace the function above.
 */

static char *new_value_ptr;
static char *new_value_value_ptr;

int __ctalkSetObjectValueVar (OBJECT *__o, char *__value) {

  OBJECT *__o_value_obj;
  int new_length = 0;
  char nullbuf[16];
  register char *s, *t;

  if (!IS_OBJECT(__o))
    return ERROR;

  if (__value) {

    new_length = strlen (__value);

    if (__o -> __o_value) {
      if (new_length > strlen (__o -> __o_value)) {
	new_value_ptr = __o->__o_value;
	new_value_ptr = realloc (new_value_ptr, new_length + 1);
	__o -> __o_value = new_value_ptr;
      }
      s = __o -> __o_value, t = __value;
      while (*s++ = *t++)
	;
    } else {
      __o -> __o_value = strdup (__value);
    }

    if ((__o_value_obj = __o -> instancevars) != NULL) {
      if (new_length > strlen (__o_value_obj -> __o_value)) {
	new_value_value_ptr = __o_value_obj->__o_value;
	new_value_value_ptr = realloc (new_value_value_ptr, new_length + 1);
	__o_value_obj -> __o_value = new_value_value_ptr;
      }
      s = __o_value_obj -> __o_value, t = __value;
      while (*s++ = *t++)
	;
    }
  } else { /* if (__value) { */

    if (__o -> __o_value) {
      if (new_length > strlen (__o -> __o_value)) {
	new_value_ptr = __o->__o_value;
	new_value_ptr = realloc (new_value_ptr, NULLSTR_BUF_LENGTH);
	__o -> __o_value = new_value_ptr;
      }
      strcpy (__o -> __o_value, NULLSTR);
    } else {
      __o -> __o_value = strdup (NULLSTR);
    }

    if ((__o_value_obj = __o -> instancevars) != NULL) {
      if (new_length > strlen (__o_value_obj -> __o_value)) {
	new_value_value_ptr = __o_value_obj->__o_value;
	new_value_value_ptr = realloc (new_value_value_ptr, new_length + 1);
	__o_value_obj -> __o_value = new_value_value_ptr;
      }
      strcpy (__o_value_obj -> __o_value, NULLSTR);
    }

  }  /* if (__value) { */

  return SUCCESS;
}


static char __val_copy[MAXMSG];
/*
 *  Similar to above, but with the use of a static buffer in
 *  case the original and new values overlap.
 */
int __ctalkSetObjectValueVarB (OBJECT *__o, char *__value) {

  OBJECT *__o_value_obj;
  int new_length;
  char nullbuf[16];

  if (!IS_OBJECT(__o))
    return ERROR;

  strcpy (__val_copy, __value);

  if (*__val_copy) {

    new_length = strlen (__val_copy);

    if (__o -> __o_value) {
      if (new_length > strlen (__o -> __o_value)) {
	new_value_ptr = __o->__o_value;
	new_value_ptr = realloc (new_value_ptr, new_length + 1);
	__o -> __o_value = new_value_ptr;
      }
      strcpy (__o -> __o_value, __val_copy);
    } else {
      __o -> __o_value = strdup (__val_copy);
    }

    if ((__o_value_obj = __o -> instancevars) != NULL) {
      if (new_length > strlen (__o_value_obj -> __o_value)) {
	new_value_value_ptr = __o_value_obj->__o_value;
	new_value_value_ptr = realloc (new_value_value_ptr, new_length + 1);
	__o_value_obj -> __o_value = new_value_value_ptr;
      }
      /* So does this. */
      strcpy (__o_value_obj -> __o_value, __val_copy);
    }
  } else { /* if (__val_copy) { */

    if (__o -> __o_value) {
      if (new_length > strlen (__o -> __o_value)) {
	new_value_ptr = __o->__o_value;
	new_value_ptr = realloc (new_value_ptr, NULLSTR_BUF_LENGTH);
	__o -> __o_value = new_value_ptr;
      }
      strcpy (__o -> __o_value, NULLSTR);
    } else {
      __o -> __o_value = strdup (NULLSTR);
    }

    if ((__o_value_obj = __o -> instancevars) != NULL) {
      if (new_length > strlen (__o_value_obj -> __o_value)) {
	new_value_value_ptr = __o_value_obj->__o_value;
	new_value_value_ptr = realloc (new_value_value_ptr, NULLSTR_BUF_LENGTH);
	__o_value_obj -> __o_value = new_value_value_ptr;
      }
      strcpy (__o_value_obj -> __o_value, NULLSTR);
    }

  }  /* if (__val_copy) { */

  return SUCCESS;
}

/*
 *  Add an empty buffer to the object's value variable.
 */
int __ctalkSetObjectValueBuf (OBJECT *__o, void *__buf) {

  OBJECT *__o_value_obj;

  if ((__o_value_obj = __o -> instancevars) != NULL) {
    __xfree (MEMADDR(__o_value_obj -> __o_value));
    __o_value_obj -> __o_value = __buf;
  } else {
    _warning 
      ("__ctalkSetObjectValueBuf: Instance variable, \"value,\" not found.\n");
    return ERROR;
  }

  return SUCCESS;
}

/*
 *  Add a pointer to a memory vector to the object, which should be a
 *  member of Vector class or one of its subclasses.  Also fill in the
 *  object's length instance variable, set the object's
 *  OBJECT_VALUE_IS_MEMORY_VECTOR attribute, and register the vector
 *  address.
 */
int __ctalkSetObjectValueAddr (OBJECT *__o, void *__buf, int length) {

  OBJECT *__o_parent, *__o_value_obj, *__o_length_instance_var; 
  bool have_vector_object;
  char int_buf[0xff];

  if (IS_VALUE_INSTANCE_VAR(__o))
    __o_parent = __o -> __o_p_obj;
  else
    __o_parent = __o;

  if (!is_class_or_subclass (__o_parent, __ctalkGetClass ("Vector"))) {
    _warning ("__ctalkSetObjectValueAddr: Object %s is not a "
	      "member of Vector class or its subclasses.\n", 
	      __o_parent -> __o_name);
    have_vector_object = false;
  } else {
    have_vector_object = true;
  }

  if (have_vector_object) {
    if ((__o_length_instance_var = 
	 __ctalkGetInstanceVariable (__o_parent, "length", FALSE)) != NULL) {
      *(int *)__o_length_instance_var -> __o_value = length;
      if (IS_OBJECT(__o_length_instance_var -> instancevars))
	*(int *)__o_length_instance_var -> instancevars
	  -> __o_value = length;
    }
  }

  if ((__o_value_obj = __o_parent -> instancevars) != NULL) {
    __xfree (MEMADDR(__o_value_obj -> __o_value));
    __o_value_obj -> __o_value = __buf;
  } else {
    _warning 
      ("__ctalkSetObjectValueAddr: Instance variable, \"value,\" not found.\n");
    return ERROR;
  }

  __ctalkSetObjectAttr (__o_value_obj, __o_value_obj -> attrs | 
			OBJECT_VALUE_IS_MEMORY_VECTOR);

  register_mem_vec (__buf);

  return SUCCESS;
}

/*
 *  This function is only used by __ctalkDefineInstanceVariable and
 *  __ctalkDefineClassVariable, so there should be no need to print
 *  a warning, only return an ERROR.  Some classes don't have value
 *  variables, which the functions don't check for.
 */
int __ctalkSetObjectValueClass (OBJECT *__o, OBJECT *value_class_obj) {
  OBJECT *value_var;
  if (IS_OBJECT(__o -> instancevars) &&
      __o -> instancevars -> attrs & OBJECT_IS_VALUE_VAR) {
    value_var = __o -> instancevars;
  } else {
    if ((value_var = __o -> instancevars) == NULL)
      return ERROR;
  }

  if (interpreter_pass == run_time_pass) {

#ifdef USE_CLASSNAME_STR
    strcpy (value_var -> __o_classname, value_class_obj -> __o_name);
#endif
#ifdef USE_SUPERCLASSNAME_STR
    strcpy (value_var -> __o_superclassname, 
	    _SUPERCLASSNAME(value_class_obj));
#endif

  } else {
    strcpy (value_var -> __o_classname, value_class_obj -> __o_name);
    strcpy (value_var -> __o_superclassname, 
	    value_class_obj -> __o_superclassname);
  }
  value_var -> __o_class = value_class_obj;
  value_var -> __o_superclass = value_class_obj -> __o_superclass;
  if (value_class_obj -> attrs & INT_BUF_SIZE_INIT) {
    __xfree (MEMADDR(__o -> __o_value));
    __o -> __o_value = __xalloc (INTBUFSIZE);
    __o -> attrs |= OBJECT_VALUE_IS_BIN_INT;
    __xfree (MEMADDR(value_var -> __o_value));
    value_var -> __o_value = __xalloc (INTBUFSIZE);
    value_var -> attrs |= OBJECT_VALUE_IS_BIN_INT;
  } else if (value_class_obj -> attrs & BOOL_BUF_SIZE_INIT) { /***/
    __o -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
    __o -> instancevars -> attrs |= OBJECT_VALUE_IS_BIN_BOOL;
    __xfree (MEMADDR(__o -> __o_value));
    __xfree (MEMADDR(__o -> instancevars -> __o_value));
    __o -> __o_value = __xalloc (INTBUFSIZE);
    __o -> instancevars -> __o_value = __xalloc (INTBUFSIZE);
  }
  return SUCCESS;
}

int __ctalkSetObjectName (OBJECT *__o, char *__name) {
  if (!IS_OBJECT(__o))
    return ERROR;
  if (__o -> __o_name  != __name) {
    memset (__o -> __o_name, 0, MAXLABEL);
    strncpy (__o -> __o_name, __name, MAXLABEL - 1);
  }
  return SUCCESS;
}

/*
 *  If a constructor has made a copy of a class object, then 
 *  change to a local variable and set its reference count to
 *  0.
 */
static int __fixup_class_object_copy (OBJECT *o) {
  OBJECT *o_class;
  if (IS_CLASS_OBJECT(o)) {
    if ((o_class = __ctalkGetClass (o -> __o_name)) != NULL) {
      if (o_class != o) {
	__ctalkSetObjectScope (o, LOCAL_VAR);
	__objRefCntZero (OBJREF(o));
      }
    }
  }
  return SUCCESS;
}

extern RT_INFO *__call_stack[MAXARGS+1];   /* Declared in rtinfo.c */
extern int __call_stack_ptr;

static void xlinked_object_warning (OBJECT *obj, OBJECT *obj_next) {
  int idx;
#ifdef NO_CIRCULAR_OBJECT_WARNINGS
  return;
#endif
  if ((obj -> scope & METHOD_USER_OBJECT) || 
      (obj_next -> scope & METHOD_USER_OBJECT))
    return;
  fprintf (stderr, 
	   "Warning: __ctalkDeleteObjectInternal: Possibly cross linked object.\nWarning: Addr: %p, Name: \"%s,\" Class: \"%s.\"\n", 
	   obj_next, obj_next -> __o_name, 
	   obj_next -> CLASSNAME);
  _warning ("(To disable these messages, build Ctalk with the option,\n");
  _warning ("--without-circular-object-reference-warnings.)\n");
  
  for (idx = __call_stack_ptr + 1; idx <= MAXARGS; idx++) {
    if (__call_stack[idx] -> method) {
      fprintf (stderr, "\tFrom %s : %s\n", 
	       __call_stack[idx]->method->rcvr_class_obj->__o_name,
	       __call_stack[idx]->method->name);
    } else {
      fprintf (stderr, "\tFrom %s\n", "<cfunc>");
    }
  }
}

/*
 *  Delete an object after making sure that it is not in
 *  a dictionary or class library.  
 *
 *  NOTE - This is iffy, in case we want to delete a class 
 *  or a global object, in which case the application should
 *  call __ctalkDeleteObject, so it isn't checked here.
 *
 *  The function __ctalk_arg_cleanup checks that an object
 *  is not global before trying to delete it.
 *
 *  If the object is an instance variable, make sure that its
 *  reference count is the same as the dictionary object's 
 *  reference count.
 *
 *  Check for the recursion level during a cleanup, in
 *  case called randomly from within a signal handler.
 */

extern int __cleanup_deletion;
static int cleanup_lvl = 0;
extern int __app_exit;
 
static int is_class_variable (OBJECT *__o) {
  OBJECT *p;
  for (p = __o -> __o_p_obj; p && p -> __o_p_obj; p = p -> __o_p_obj)
    ;
  if (p && IS_CLASS_OBJECT(p))
    return TRUE;
  return FALSE;
}

extern VARENTRY *__ctalk_dictionary;
static int is_declared_outside_method (OBJECT *tgt) {
  VARENTRY *v, *v_t;
  int i;
  /*
   *  Start looking in the scope before the current method 
   *  == __call_stack_ptr + 2.
   */
  for (i = __call_stack_ptr + 2; i <= MAXARGS; i++) {
    if (__call_stack[i] -> method) {
      v = __call_stack[i] -> method -> local_objects[0].vars;
      for (v_t = v; v_t; v_t = v_t -> next)
	if (v_t -> var_object == tgt)
	  return TRUE;
    } else {
      v = __call_stack[i] -> _rt_fn -> local_objects.vars;
      for (v_t = v; v_t; v_t = v_t -> next)
	if (v_t -> var_object == tgt)
	  return TRUE;
    }
  }
  for (v_t = __ctalk_dictionary; v_t; v_t = v_t -> next)
    if (v_t -> var_object == tgt)
      return TRUE;
  return FALSE;
}

/* True if the object that refers to the reffed object is
   the member of a collection, not some other object that
   refers to the object. */
static bool collection_member_reference (OBJECT *o) {
  if (IS_OBJECT(o -> __o_p_obj))
    return o -> __o_p_obj -> attrs & 
      OBJECT_IS_MEMBER_OF_PARENT_COLLECTION;
  else
    return false;
}

/*
  WARNING: 
  Be careful here if we get a segfault -
  we may need to check the scope of the 
  reffed objects more carefully.
*/
void cleanup_reffed_object (OBJECT *__o, OBJECT *__r) {
  int has_super_scope;

  has_super_scope = is_declared_outside_method (__r);
  if (IS_OBJECT (__r) && !is_class_variable (__r) && !is_arg (__r) && 
      !is_receiver (__r) && 
      (!(__r->scope & METHOD_USER_OBJECT) || __cleanup_deletion)) {
    __fixup_class_object_copy (__r);
    if (!__cleanup_deletion) {
      if (has_super_scope) {
	if (__r -> nrefs > 1) {
	  (void)__objRefCntDec(OBJREF(__r));
	}
      } else {
	(void)__objRefCntDec(OBJREF(__r));
      }
      if (!parent_ref_is_circular (__o, __r)) {
	if (!has_super_scope)
	  _cleanup_temporary_objects (__r, NULL, NULL, 
				      rt_cleanup_obj_ref);
	/*
	 *  If the object still exists with a reference count of 0....
	 *  This case can occur with intermediate objects
	 *  created by Collection : atPut.
	 */
	if (IS_OBJECT(__r)) {
	  if (!IS_CLASS_OBJECT (__r)) {
	    if (__r -> nrefs == 0) {
	      if ((__r -> __o_p_obj) && 
		  (__r -> __o_p_obj -> nrefs > 0)) {
		/*  With a fixup kludge if necessary, should an instance
		 *  variable get dereffed independently of its parent 
		 *  object.
		 */
		__r -> nrefs = __r -> __o_p_obj -> nrefs;
	      } else {
		if (__o -> __o_p_obj) {
		  if (__o -> __o_p_obj -> nrefs == 0) {
		    if (__cleanup_deletion)
		      varentry_cleanup_reset (__r -> __o_vartags -> tag);
		    if (collection_member_reference (__o)) {
		      __ctalkDeleteObject (__r);
		    } else {
		      __objRefCntDec (OBJREF(__r));
		    }
		  }
		} else {
		  if (__o -> nrefs == 0) {
		    if (__cleanup_deletion)
		      varentry_cleanup_reset (__r -> __o_vartags -> tag);
		    if (collection_member_reference (__o)) {
		      __ctalkDeleteObject (__r);
		    } else {
		      __objRefCntDec (OBJREF(__r));
		    }
		  }
		}
	      }
	    }
	  }
	}
      } else {
	__o -> __o_value[0] = '\0';
      }
      /*
       *  NOTE: Kludge for references that have
       *  ARG_VAR and/or CREATED_PARAM, etc.
       */
    } else {
      if (!parent_ref_is_circular (__o, __r)) {
	if (!(__r -> scope & GLOBAL_VAR) ||
	    ((__r -> scope & GLOBAL_VAR) &&
	     (!IS_CLASS_OBJECT(__r)))) {
	  if (__r -> scope & LOCAL_VAR) {
	    if (!__r -> __o_p_obj &&
		!((__r -> __o_vartags) ?
		  __r -> __o_vartags -> tag : NULL)) {
	      if (collection_member_reference (__o)) {
		__objRefCntZero (OBJREF(__r));
		__call_delete_object_internal (__r);
	      } else if (__cleanup_deletion &&
			 (__r -> scope & CVAR_VAR_ALIAS_COPY)) {
		__objRefCntZero (OBJREF(__r));
		__call_delete_object_internal (__r);
	      } else {
		__objRefCntDec (OBJREF(__r));
	      }
	    } else {
	      if (__r -> scope & VAR_REF_OBJECT) {
		if (IS_VARTAG(__r -> __o_vartags)) {
		  if (!IS_EMPTY_VARTAG(__r -> __o_vartags)) {
		    if (__r -> __o_vartags -> tag -> is_local == true &&
			__cleanup_deletion) {
		      if (collection_member_reference (__o)) {
			/* i.e., check that the object that references
			   __r is a collection member instead of a
			   separate object that's pointing to a collection
			   member. */
			__objRefCntZero (OBJREF(__r));
			__call_delete_object_internal (__r);
		      } else {
			__objRefCntDec (OBJREF(__r));
		      }
		    }
		  }
		}
	      }
	    }
	  } else {
	    if (__r -> scope & CREATED_PARAM) {
	      if (!(__r -> scope & METHOD_USER_OBJECT)) {
		if (__r -> scope & VAR_REF_OBJECT) {
		  if (IS_OBJECT(__o -> __o_p_obj)) {
		    if (collection_member_reference (__o) || __cleanup_deletion) {
		      __objRefCntZero (OBJREF(__r));
		      __call_delete_object_internal (__r);
		    } else {
		      __objRefCntDec (OBJREF(__r));
		    }
		  }
		} else {
		  __objRefCntZero (OBJREF(__r));
		  __call_delete_object_internal (__r);
		}
	      }
	    } else {
	      if (__r -> scope & ARG_VAR) {
		if ((__r -> __o_vartags && 
		     __r -> __o_vartags -> tag) ||
		    (__r -> __o_p_obj &&
		      __r -> __o_p_obj -> __o_vartags &&
		     __r -> __o_p_obj -> __o_vartags -> tag)) {
		  (void)__objRefCntDec (OBJREF (__r));
		} else {
		  if (collection_member_reference (__o)) {
		    __objRefCntZero (OBJREF(__r));
		    __call_delete_object_internal (__r);
		  } else {
		    __objRefCntDec (OBJREF(__r));
		  }
		}
	      } else {
		if (__r -> nrefs > 1) {
		  (void)__objRefCntDec (OBJREF(__r));
		}
	      }
	    }
	  }
	}
      }
      __o -> __o_value[0] = '\0';
    }
  }
}

void __ctalkDeleteObjectInternal (OBJECT *__o) {
  OBJECT *obj, *__r;
  LIST *vec_entry;
  char buf[64];

  if (__cleanup_deletion) {
    if (++cleanup_lvl > MAX_CLEANUP_LEVELS) {
      printf ("__ctalkDeleteObjectInternal (): Maximum levels of object "
	      "cleanup exceeded.\n");
      exit (1);
    }
  }
  if (!IS_OBJECT (__o)) {
#ifdef DEBUG_OBJECT_DESTRUCTION
    _warning ("__ctalkDeleteObjectInternal: Not an object.\n");
#endif
    if (__cleanup_deletion)
      --cleanup_lvl;
    return;
  }

  if (__o -> nrefs <= 0) {
    if (IS_VARTAG(__o -> __o_vartags)) {
      delete_vartag_list (__o);
    }
    if (IS_OBJECT(__o -> instancevars)) {
      OBJECT *o_ptr, *o_ptr_prev;
      for (o_ptr = __o -> instancevars; 
	   IS_OBJECT(o_ptr) && o_ptr -> next; 
	   o_ptr = o_ptr -> next) {
	if (IS_OBJECT(o_ptr -> next) &&
	    (o_ptr -> scope != o_ptr -> next -> scope) &&
	    (o_ptr -> __o_p_obj != o_ptr -> next -> __o_p_obj)) {
	  xlinked_object_warning (o_ptr, o_ptr -> next);
	  goto xlinked_jmp;
	}
      }
    xlinked_jmp:
      if (o_ptr == __o -> instancevars) {
	if (o_ptr) __call_delete_object_internal (o_ptr);
	__o -> instancevars = NULL;
      } else {
	while (o_ptr && (o_ptr != __o -> instancevars)) {
	  o_ptr_prev = (o_ptr) ? o_ptr -> prev : NULL;
	  if (IS_OBJECT (o_ptr)) {
	      __call_delete_object_internal (o_ptr);
	  } else {
	    while (o_ptr_prev && 
		   !IS_OBJECT (o_ptr_prev) && 
		   (o_ptr_prev != __o -> instancevars)) {
	      if ((o_ptr_prev = o_ptr_prev -> prev) == NULL)
		break;
	    }
	  }
	  o_ptr = o_ptr_prev;
	  if (o_ptr) o_ptr -> next = NULL;
	}
	__call_delete_object_internal(__o -> instancevars);
	__o -> instancevars = NULL;
      }
    }
    if (!__cleanup_deletion) {
      if ((obj = 
	   __ctalk_get_object 
	   (__o -> __o_name, __o -> CLASSNAME)) != NULL) {
	/*
	 *  TODO - The call above depends too much on 
	 *  __ctalk_get_object's search for objects, and
	 *  the searches should be factored out.
	 */
	if (__o == obj)
	  __ctalk_remove_object (obj);
      }
    }
    if (IS_OBJECT (__o)) {
     if (__o -> __o_value)  {
       if (IS_OBJECT(__o -> __o_class)) {
	 if ((__o -> __o_class != rt_defclasses -> p_string_class) &&
	     ((__r = obj_ref_str (__o -> __o_value)) != NULL)) {
	   cleanup_reffed_object (__o, __r);
	   __xfree (MEMADDR(__o -> __o_value));
	   __o ->__o_value = NULL;
	 } else if (__o -> attrs & OBJECT_VALUE_IS_MEMORY_VECTOR) {
	   if ((vec_entry = vec_is_registered (__o -> __o_value)) != NULL) {
	     remove_mem_vec (vec_entry);
	     __xfree (MEMADDR(__o -> __o_value));
	     __o -> __o_value = NULL;
	   }
	   __o -> __o_value = NULL;
	 } else if (__app_exit &&
		    (__o -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL)) {
	   if ((__r = *(OBJECT **)__o -> __o_value) != NULL) {
	     sprintf (buf, "%p", (void *)__r);
	     if (obj_ref_str (buf)) {
	       if (!parent_ref_is_circular (__o, __r)) {
		 if (__r -> next) __r -> next -> prev = __r -> prev;
		 if (__r -> prev) __r -> prev -> next = __r -> next;
		 if ((__r -> attrs & OBJECT_IS_VALUE_VAR) &&
		     (IS_OBJECT(__r -> __o_p_obj))) {
		   __r -> __o_p_obj -> instancevars = __r -> next;
		 }
		 __ctalkDeleteObject (__r);
	       }
	     }
	   }
	   __xfree (MEMADDR(__o -> __o_value));
	   __o ->__o_value = NULL;
	 } else if (__cleanup_deletion &&
		    (__o -> attrs & OBJECT_VALUE_IS_BIN_SYMBOL)) {
	   if ((__r = *(OBJECT **)__o -> __o_value) != NULL) {
	     sprintf (buf, "%p", (void *)__r);
	     if (obj_ref_str (buf)) {
	       if (!(__r -> scope & GLOBAL_VAR)&& !is_receiver (__r)) {
		 if (!parent_ref_is_circular (__o, __r)) {
		   if (__r -> next) __r -> next -> prev = __r -> prev;
		   if (__r -> prev) __r -> prev -> next = __r -> next;
		   if ((__r -> attrs & OBJECT_IS_VALUE_VAR) &&
		       (IS_OBJECT(__r -> __o_p_obj))) {
		     __r -> __o_p_obj -> instancevars = __r -> next;
		   }
		   __ctalkRegisterUserObject (__r);
		 }
	       }
	     }
	   }
	   __xfree (MEMADDR(__o -> __o_value));
	   __o ->__o_value = NULL;
	 } else if (IS_OBJECT(__o)) {
	   __xfree (MEMADDR(__o -> __o_value));
	   __o ->__o_value = NULL;
	 }
       }
     }
     if (IS_OBJECT(__o)) {
#ifndef __sparc
       __o -> sig = 0;
#else
       __o -> sig[0] = '\0';
#endif
       __xfree (MEMADDR(__o -> __o_value));
       __xfree (MEMADDR(__o));
     }
     __o = NULL;
    }
  } else {
    (void)__objRefCntDec (OBJREF (__o));
  }

  if (__cleanup_deletion)
    --cleanup_lvl;
}

/*
 *   Set an object's reference count to 0.  Also set the reference
 *   count of the object's instance variables to 0, unless they 
 *   are in the dictionary or the class dictionary.
 */

void __call_delete_object_internal (OBJECT *__o) {
  OBJECT *var;
  if (IS_OBJECT(__o)) {
    __ctalkDeleteObjectInternal (__o);
  }
}

static void unlink_varentry_replace (OBJECT *__o) {
  VARTAG *v;
  /* 
   * This is like unlink_varentry (), except that
   * it replaces the var_object with the orig_object_rec,
   * for the next time we re-use the tag.
   */
  if (IS_VARTAG(__o -> __o_vartags)) {
    for (v = __o -> __o_vartags; IS_VARTAG(v); v = v ->  next) {
      if (v -> tag) {
	if (v -> tag -> orig_object_rec == __o) {
	  v -> tag -> orig_object_rec = NULL;
	  v -> tag -> i = v -> tag -> i_post = v -> tag -> i_temp = I_UNDEF;
	}
	if (v -> tag -> var_object == __o) {
	  v -> tag -> var_object = v -> tag -> orig_object_rec;
	  v -> tag -> orig_object_rec = NULL;
	  v -> tag -> i = v -> tag -> i_post = v -> tag -> i_temp = I_UNDEF;
	}
      }
    }
  }
}

void __ctalkDeleteObject (OBJECT *__o) {
  if (IS_OBJECT(__o)) {
    if (__cleanup_deletion) {
      cleanup_lvl = 0;
      __o -> attrs |= OBJECT_IS_BEING_CLEANED_UP;
    } else {
      unlink_varentry_replace (__o);
    }
    __objRefCntZero (OBJREF(__o));
    __call_delete_object_internal (__o);
  }
}

