/* $Id: rtclslib.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ -*-c-*-*/

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
 * Run time class dictionaries and functions.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

OBJECT *__ctalk_classes = NULL;
OBJECT *__ctalk_last_class = NULL;

VARENTRY *__ctalk_dictionary = NULL;
VARENTRY *__ctalk_last_object = NULL;

HASHTAB classvarhash = NULL;

DEFAULTCLASSCACHE *rt_defclasses;

int __cleanup_deletion = FALSE;

void hash_class_var (OBJECT *var) {

  char keybuf[MAXLABEL * 2];

  if (classvarhash == NULL)
    _new_hash (&classvarhash);

  /* Hash twice in case we don't have the classname available
     when looking up the var. */
  strcatx (keybuf, var  -> __o_name, ":", var -> CLASSNAME, NULL);
  _hash_put (classvarhash, (void *)var, keybuf);

  /* sprintf (keybuf, "%s:(null)", var -> __o_name); */
  strcatx (keybuf, var -> __o_name, ":(null)", NULL);
  _hash_put (classvarhash, (void *)var, keybuf);
}

void hash_remove_class_var (OBJECT *var) {
  char keybuf[MAXLABEL * 2];

  if (classvarhash == NULL)
    return;

  strcatx (keybuf, var  -> __o_name, ":", var -> CLASSNAME, NULL);
  /* Do we need to return the var from here? */
  (void)_hash_remove (classvarhash, keybuf);

  strcatx (keybuf, var -> __o_name, ":(null)", NULL);
  (void)_hash_remove (classvarhash, keybuf);
}

void rt_cache_class_ptr (OBJECT *class_obj) {
  if (str_eq (class_obj -> __o_name, OBJECT_CLASSNAME)) {
    rt_defclasses -> p_object_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, COLLECTION_CLASSNAME)) {
    rt_defclasses -> p_collection_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, ARRAY_CLASSNAME)) {
    rt_defclasses -> p_array_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, EXPR_CLASSNAME)) {
    rt_defclasses -> p_expr_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, CFUNCTION_CLASSNAME)) {
    rt_defclasses -> p_cfunction_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, MAGNITUDE_CLASSNAME)) {
    rt_defclasses -> p_magnitude_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, CHARACTER_CLASSNAME)) {
    rt_defclasses -> p_character_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, STRING_CLASSNAME)) {
    rt_defclasses -> p_string_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, FLOAT_CLASSNAME)) {
    rt_defclasses -> p_float_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, INTEGER_CLASSNAME)) {
    rt_defclasses -> p_integer_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, LONGINTEGER_CLASSNAME)) {
    rt_defclasses -> p_longinteger_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, SYMBOL_CLASSNAME)) {
    rt_defclasses -> p_symbol_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, BOOLEAN_CLASSNAME)) {
    rt_defclasses -> p_boolean_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, STREAM_CLASSNAME)) {
    rt_defclasses -> p_stream_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, FILESTREAM_CLASSNAME)) {
    rt_defclasses -> p_filestream_class = class_obj;
  } else if (str_eq (class_obj -> __o_name, KEY_CLASSNAME)) {
    rt_defclasses -> p_key_class = class_obj;
  }
}

void __ctalk_dictionary_add (OBJECT *__o) {

  OBJECT *p;
  VARENTRY *v;

  if (IS_CLASS_OBJECT(__o)) {
    for (p = __ctalk_classes; p; p = p -> next) {
      if (IS_OBJECT(p)) {
	if (!strcmp (p -> __o_name, __o -> __o_name))
	  return;
      }
    }

    if (!__ctalk_classes) {
      __refObj (OBJREF(__ctalk_classes), OBJREF(__o));
      __ctalk_last_class = __o;
    } else {
      __refObj (OBJREF(__ctalk_last_class -> next), OBJREF(__o));
      __o -> prev = __ctalk_last_class;
      __ctalk_last_class = __ctalk_last_class -> next;
    }
    rt_cache_class_ptr (__o);
  } else {
    v = new_varentry (__o);
    __objRefCntSet (OBJREF(__o), 1);
    if (!__ctalk_dictionary) {
      __ctalk_dictionary = __ctalk_last_object = v;
    } else {
      v -> prev = __ctalk_last_object;
      __ctalk_last_object -> next = v;
      __ctalk_last_object = __ctalk_last_object -> next;
    }
  }
}

inline OBJECT *get_class_by_name (const char *s) {
  OBJECT *__o;
  if (!__ctalk_classes)
    return NULL;

  if (*s == 0)
    return NULL;

  for (__o = __ctalk_classes; __o; __o = __o -> next)
    if (str_eq (__o -> __o_name, (char *)s))
      return __o;

  return NULL;
}

typedef struct {
  char name[MAXLABEL],
    path[FILENAME_MAX];
} CLASSNAMEREC;

static CLASSNAMEREC *new_classnamerec (char *classname, char *classpath) {
  CLASSNAMEREC *c;
  c = (CLASSNAMEREC *)__xalloc  (sizeof (CLASSNAMEREC));
  strcpy (c -> name, classname);
  if (classpath)
    strcpy (c -> path, classpath);
  return c;
}

LIST *classnames = NULL;

bool is_class_library_name (char *s) {
  LIST *l;
  for (l = classnames; l; l = l -> next) {
    if (str_eq (((CLASSNAMEREC *)l -> data) ->  name, s)) {
      return true;
    }
  }
  return false;
}

void rt_get_class_names (void) {
  DIR *d;
  struct dirent *d_ent;
  LIST *l;
  char *p, *q, user_path[FILENAME_MAX], expanded_path[FILENAME_MAX],
    class_path[FILENAME_MAX];
  bool have_default_class_dir = false, is_class_name = false;

  /* built-in classes get added automagically. */
  l = new_list ();
  l -> data = new_classnamerec ("Expr", NULL);
  classnames = l;
  l = new_list ();
  l -> data = new_classnamerec ("CFunction", NULL);
  list_push (&classnames, &l);

  if ((p = getenv ("CLASSLIBDIRS")) != NULL) {
    do {
      q = strchr (p, ':');
      if (q) {
	substrcpy (user_path, p, 0, q - p);
	p = q + 1;
      } else {
	strcpy (user_path, p);
      }
      expand_path (user_path, expanded_path);
      if (str_eq (expanded_path,  CLASSLIBDIR))
	have_default_class_dir = true;
      if ((d = opendir (expanded_path)) != NULL) {
	while ((d_ent = readdir (d)) != NULL) {
	  if (*(d_ent -> d_name) == '.')
	    continue;
	  l = new_list ();
	  strcatx (class_path, expanded_path, "/", d_ent -> d_name, NULL);
	  l -> data = new_classnamerec (d_ent -> d_name, class_path);
	  list_push (&classnames, &l);
	}
	closedir (d);
      }
    } while (q != NULL);
  }

  if (!have_default_class_dir) {
    if ((d = opendir (CLASSLIBDIR)) != NULL) {
      while ((d_ent = readdir (d)) != NULL) {
	if (*(d_ent -> d_name) == '.')
	  continue;
	l = new_list ();
	strcatx (class_path, CLASSLIBDIR, "/", d_ent -> d_name, NULL);
	l -> data = new_classnamerec (d_ent -> d_name, class_path);
	list_push (&classnames, &l);
      }
      closedir (d);
    }
  }
}

OBJECT *__ctalkGetClass (const char *s) {

  OBJECT *__o;
  char class_path[FILENAME_MAX];
  char user_path[FILENAME_MAX], expanded_path[FILENAME_MAX];
  char *p, *q;
  DIR *d;
  struct dirent *d_ent;
  LIST *l;
  CLASSNAMEREC *c;

  if (!s || !*s)
    return NULL;

  /* get all our class names if this is the first call. */
  if (classnames == NULL)
    rt_get_class_names ();

  if (!__ctalk_classes)
    return NULL;

  for (__o = __ctalk_classes; __o; __o = __o -> next)
    if (str_eq (__o -> __o_name, (char *)s))
      return __o;

  /* Check this second - if a class file has a class definition
     that's not the file's name, it will be in __ctalk_classes but
     not here. */
  c = NULL;
  for (l = classnames; l; l = l -> next) {
    if (str_eq (((CLASSNAMEREC *)l -> data) ->  name, (char *)s)) {
      c = l -> data;
      break;
    }
  }
  
  /* if (!is_class_name) */
  if (! c)
    return NULL;
  else
    if ((__o = get_class_library_definition (c -> path)) != NULL)
      return __o;
    else
      return NULL;
}

/* 
 *  Class objects don't need a value instance variable, so
 *  if there is one, it gets deleted.  Class objects have 
 *  the __o_class member pointing to the object itself.
 *
 *  The name of the new class is on the argument stack.
 *  The name of the superclass is the receiver on the 
 *  receiver stack.
 */
OBJECT *__ctalk_define_class (ARG **args) {

  OBJECT *rcvr,
    *value_var;
  rcvr = __ctalk_receiver_pop_deref ();
  __ctalk_receiver_push_ref (rcvr);
  /* This is what happens if we don't give the "class" keyword
     any arguments. */
  if (IS_ARG(args[0])) {
    strcpy (ARG_SUPERCLASSNAME(args[0]), rcvr->__o_name);
    strcpy (ARG_CLASSNAME(args[0]), "Class");
    /* If arg is created by create_param (), then these should be reset. */
    ARG_OBJECT(args[0])->__o_class = ARG_OBJECT(args[0]);
    ARG_OBJECT(args[0])->__o_superclass = rcvr;
    
    value_var = ARG_OBJECT(args[0]) -> instancevars;
    if (IS_OBJECT(value_var)) {
      strcpy (value_var -> __o_classname, "Class");
      strcpy (value_var -> __o_superclassname, rcvr -> __o_name);
      value_var -> __o_class = ARG_OBJECT(args[0]);
      value_var -> __o_superclass = rcvr;
    }
    __ctalkSetObjectScope (ARG_OBJECT(args[0]), GLOBAL_VAR);
    if (str_eq (ARG_OBJECT(args[0]) -> __o_name, STRING_CLASSNAME)) {
      __ctalkSetObjectAttr (ARG_OBJECT(args[0]),
			     ZERO_LENGTH_STR_INIT);
    } else if (str_eq (ARG_OBJECT(args[0]) -> __o_name, INTEGER_CLASSNAME)) {
      __ctalkSetObjectAttr (ARG_OBJECT(args[0]), INT_BUF_SIZE_INIT);
    } else if (str_eq (ARG_OBJECT(args[0]) -> __o_name, CHARACTER_CLASSNAME) ||
	       (is_class_or_subclass (ARG_OBJECT(args[0]),
				      rt_defclasses -> p_character_class) &&
		!is_class_or_subclass (ARG_OBJECT(args[0]),
				       rt_defclasses -> p_string_class))) {
      /* String class (as a subclass of Character) overrides the 
	 Character value format.... */
      __ctalkSetObjectAttr (ARG_OBJECT(args[0]), CHAR_BUF_SIZE_INIT);
    } else if (str_eq (ARG_OBJECT(args[0]) -> __o_name, BOOLEAN_CLASSNAME) ||
	       is_class_or_subclass (ARG_OBJECT(args[0]),
				     rt_defclasses -> p_boolean_class)) {
      __ctalkSetObjectAttr (ARG_OBJECT(args[0]), BOOL_BUF_SIZE_INIT);
    } else if (str_eq (ARG_OBJECT(args[0]) -> __o_name,
			LONGINTEGER_CLASSNAME) ||
	       is_class_or_subclass (ARG_OBJECT(args[0]),
				     rt_defclasses -> p_longinteger_class)) {
      __ctalkSetObjectAttr (ARG_OBJECT(args[0]), LONGLONG_BUF_SIZE_INIT);
    } else if (str_eq (ARG_OBJECT(args[0]) -> __o_name,
			SYMBOL_CLASSNAME) ||
	       is_class_or_subclass (ARG_OBJECT(args[0]),
				     rt_defclasses -> p_symbol_class)) {
      __ctalkSetObjectAttr (ARG_OBJECT(args[0]), SYMBOL_BUF_SIZE_INIT);
    } else if (is_class_or_subclass (ARG_OBJECT(args[0]),
				     rt_defclasses -> p_integer_class)) {
      __ctalkSetObjectAttr (ARG_OBJECT(args[0]), INT_BUF_SIZE_INIT);
    }
    /* These class objects have a refcount of 2 after
       they're added to the dictionary. */
    __ctalk_dictionary_add (ARG_OBJECT(args[0]));
    return ARG_OBJECT(args[0]);
  } else {
    return NULL;
  }
}

static MESSAGE *cl_messages[P_MESSAGES+1]; /* Eval message stack.        */
static int cl_message_ptr = P_MESSAGES;    /* Eval stack pointer.        */

int cl_message_push (MESSAGE *m) {
  if (cl_message_ptr == 0) {
    _warning (_("cl_message_push: stack overflow.\n"));
    return ERROR;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("cl_message_push %d. %s."), cl_message_ptr, m -> name);
#endif
  cl_messages[cl_message_ptr--] = m;
  return cl_message_ptr + 1;
}

MESSAGE *cl_message_pop (void) {
  MESSAGE *m;
  if (cl_message_ptr == P_MESSAGES) {
    _warning (_("cl_message_pop: stack underflow.\n"));
    return (MESSAGE *)NULL;
  }
#ifdef MACRO_STACK_TRACE
  debug (_("cl_message_pop %d. %s."), cl_message_ptr, 
	 cl_messages[cl_message_ptr+1] -> name);
#endif
  if (cl_messages[cl_message_ptr + 1] && 
      IS_MESSAGE(cl_messages[cl_message_ptr + 1])) {
    m = cl_messages[cl_message_ptr + 1];
    cl_messages[++cl_message_ptr] = NULL;
    return m;
  } else {
    cl_messages[++cl_message_ptr] = NULL;
    return NULL;
  }
}

OBJECT *get_class_library_definition (char *fn) {

  char *buf, *keyword, *sc_start, *sc_end, *cl_start, *cl_end,
    *followingtok, scbuf[MAXLABEL], clbuf[MAXLABEL];
  int size, bytes_read;
  LIST *l;
  bool have_superclass;
  struct stat statbuf;
  MESSAGE *m;
  OBJECT *o = NULL, *o_value = NULL;

  if (stat (fn, &statbuf))
    return NULL;

  __setClassLibRead (True);

  size = file_size (fn);
  buf = (char *)__xalloc (size * 2);

#ifdef __DJGPP__
  /*
   *  Read_file does line ending translation and error
   *  checking.
   */
  bytes_read = read_file (buf, fn);
#else
  if ((bytes_read = read_file (buf, fn)) != size) {
    _warning ("get_class_library: %s.\n", strerror (errno));
    goto class_def_NULL_return;
  }
#endif

  keyword = buf;
  while ((keyword = strstr (keyword, "class")) != NULL) {
    if ((sc_end = keyword - 1) < 0) {
      goto class_def_NULL_return;
    }
    while (isspace ((int)*sc_end))
      --sc_end;
    ++sc_end;
    if (sc_end <= 0) {
      goto class_def_NULL_return;
    }
    sc_start = sc_end - 1;
    while (isalnum ((int)*sc_start) || *sc_start == '_')
      --sc_start;
    ++sc_start;
    memset (scbuf, 0, MAXLABEL);
    strncpy (scbuf, sc_start, sc_end - sc_start);
    have_superclass = false;
    for (l = classnames; l && !have_superclass; l = l -> next) {
      if (str_eq (((CLASSNAMEREC *)l -> data) -> name, scbuf))
	have_superclass = true;
    }
    if (!have_superclass) {
      keyword += 5;
      continue;
    }

    cl_start = keyword + 5;  /* strlen ("class") */
    while (isspace ((int)*cl_start))
      ++cl_start;
    cl_end = cl_start;
    while (isalnum ((int)*cl_end) || *cl_end == '_')
      ++cl_end;
    memset (clbuf, 0, MAXLABEL);
    strncpy (clbuf, cl_start, cl_end - cl_start);

    followingtok = cl_end;
    while (isspace ((int)*followingtok))
      ++followingtok;
    if (*followingtok != '"' && *followingtok != ';') {
      keyword += 5; /* strlen ("class") */
      continue;
    }

    /* Create the object manually.  If we just use
       __ctalkCreateObjectInit, it can recurse when
       looking up a superclass and clobber this
       object. Note that the class object does not contain the
       (null) default value. */
    o = __xalloc (sizeof (struct _object));
    o_value = __xalloc (sizeof (struct _object));
#ifndef __sparc__
    o -> sig = 0xd3d3d3;
    o_value -> sig = 0xd3d3d3;
#else
    strcpy (o -> sig, "OBJECT");
    strcpy (o_value -> sig, "OBJECT");
#endif
    strcpy (o -> __o_name, clbuf);
    strcpy (o_value -> __o_name, "value");
#ifdef USE_CLASSNAME_STR
    strcpy (o -> __o_classname, "Class");
    strcpy (o_value -> __o_classname, "Class");
#endif
    o -> __o_class = o;
    o_value -> __o_class = o;
    o -> scope = GLOBAL_VAR;
    o_value -> scope = GLOBAL_VAR;
    o -> __o_superclass = 
      __ctalkGetClass (scbuf);
    o_value -> __o_superclass = o -> __o_superclass;
#ifdef USE_SUPERCLASSNAME_STR
    strcpy (o -> __o_superclassname, 
	    o -> __o_superclass -> __o_name);
    strcpy (o_value -> __o_superclassname, 
	    o -> __o_superclass -> __o_name);
#endif
    o -> instancevars = o_value;
    /* A refcount of more than 1 helps insure that
       the class object won't be deleted anywhere. */
    __objRefCntSet (OBJREF(o), 2);
    __ctalk_dictionary_add (o);
    __xfree (MEMADDR (buf));
    __setClassLibRead (False);
    return o;
    
  }

 class_def_NULL_return:
  __xfree (MEMADDR (buf));
  __setClassLibRead (False);

  return NULL;
}

bool is_string_object (OBJECT *o) {
  if (IS_OBJECT (o)) {
    if (is_class_or_subclass (o, rt_defclasses -> p_string_class))
      return true;
  }

  return false;
}

bool is_key_object (OBJECT *o) {
  if (IS_OBJECT (o)) {
    if (is_class_or_subclass (o, rt_defclasses -> p_key_class))
      return true;
  }

  return false;
}

/*
 *  Convenience function.  If the object is a
 *  class object, return itself.
 *
 */
static OBJECT *class_cmp (OBJECT *o, char *name) {
  if (!IS_OBJECT(o))
    return NULL;
  if (str_eq (o -> __o_name, name))
    return o;
  return class_cmp (o -> next, name);
}

OBJECT *__ctalkClassObject (OBJECT *o) {
  if (IS_OBJECT (o -> __o_class))
    return o -> __o_class;
  return class_cmp (__ctalk_classes, o -> CLASSNAME);
}

/* return true if obj is a member of class_obj or one of its subclasses. */
bool is_class_or_subclass (OBJECT *obj, OBJECT *class_obj) {
  OBJECT *superclass_object;

  if (obj -> __o_class == class_obj)
    return true;

  for (superclass_object = obj -> __o_class -> __o_superclass;
       superclass_object;
       superclass_object = superclass_object -> __o_superclass) {
    if (superclass_object == class_obj)
      return true;
    if (superclass_object -> __o_superclass == NULL)
      break;
  }

  return false;
}

bool __ctalkIsSubClassOf (char *classname, char *superclassname) {

  OBJECT *class_object, *superclass_object;

  if ((class_object = __ctalkGetClass (classname)) == NULL) {
    return false;
  }
    
  for (superclass_object = class_object -> __o_superclass;
       superclass_object;
       superclass_object = superclass_object -> __o_superclass) {
    if (!strcmp (superclass_object -> __o_name, superclassname))
      return true;
    if (superclass_object -> __o_superclass == NULL)
      break;
  }

  return false;
}

static void __delete_method_params (METHOD *m) {
  int n_th_param;
  for (n_th_param = 0; n_th_param < m -> n_params; n_th_param++)
    if (m->params[n_th_param])
      __xfree (MEMADDR(m->params[n_th_param]));
}

void cleanup_handler (int signum, siginfo_t *si, void *v) {
  if (signum == SIGSEGV) {
#ifndef WITHOUT_CLEANUP_ERRORS
    printf ("ctalk:  Bad object found during cleanup, addr: %p.  Exiting.\n", 
	    si -> si_addr);
#endif
  } else {
    printf ("ctalk: Received signal %d during cleanup. Exiting.\n", signum);
  }
#ifndef WITHOUT_CLEANUP_ERRORS
  printf ("(To remove these messages, build Ctalk with the ");
  printf ("--without-cleanup-errors\nconfigure option.)\n");
#endif
  _exit (EXIT_FAILURE);
}

static void __delete_method_user_objs (METHOD *m) {

  OBJECT *o;
  LIST *l, *l_prev, *l_next;

  l = m -> user_object_ptr;

  if (l == m -> user_objects) {
    o = (OBJECT *)l -> data;
    if (IS_OBJECT(o)) {
      if (IS_VARTAG(o -> __o_vartags)) {
	if (o -> __o_vartags -> tag)
	  ++o -> __o_vartags -> tag -> del_cnt;
      }
      __ctalkDeleteObject (o);
    }
    l -> data = NULL;
    delete_list_element (l);
    m -> user_objects = m -> user_object_ptr = NULL;
  } else {
    while (l != m -> user_objects) {
      
      l_prev = l -> prev;
      
      o = (OBJECT *)l -> data;
      if (IS_OBJECT(o)) {
	if (IS_VARTAG (o -> __o_vartags)) {
	  if (o -> __o_vartags -> tag) {
	    unlink_varentry (o);
	  }
	  __ctalkDeleteObject (o);
	}
      }
      l -> data = NULL;
      delete_list_element (l);

      if ((l = l_prev) == NULL)
	break;
    }
    o = (OBJECT *)m -> user_objects -> data;
    if (IS_OBJECT(o)) {
      if (o -> __o_vartags && !IS_EMPTY_VARTAG(o -> __o_vartags)) {
	unlink_varentry (o);
      }
      __ctalkDeleteObject (o);
    }
    m -> user_objects -> data = NULL;
    delete_list_element (m -> user_objects);
    m -> user_objects = m -> user_object_ptr = NULL;
  }
}

/* Should be used *only* by __ctalk_exitFn (). */
void __delete_fn_user_objs (RT_FN *r) {

  OBJECT *o, *o_orig;
  LIST *l, *l_prev, *l_next;

  l = r -> user_object_ptr;

  if (l == r -> user_objects) {
    o = (OBJECT *)l -> data;
    if (IS_OBJECT(o)) {
      if (IS_VARTAG(o -> __o_vartags)) {
	if (o -> __o_vartags -> tag)
	  ++o -> __o_vartags -> tag -> del_cnt;
	if (__cleanup_deletion) {
	  if (IS_VARTAG(o -> __o_vartags) &&
	      IS_VARENTRY (o -> __o_vartags -> tag) &&
	      IS_OBJECT(o -> __o_vartags -> tag -> orig_object_rec)) {
	    o_orig = o -> __o_vartags -> tag -> orig_object_rec;
	    __ctalkDeleteObject (o_orig);
	    o -> __o_vartags -> tag -> orig_object_rec = NULL;
	  }
	}
      }
      __ctalkDeleteObject (o);
    }
    __xfree (MEMADDR(l));
    r -> user_objects = r -> user_object_ptr = NULL;
  } else {
    while (l != r -> user_objects) {

      l_prev = l -> prev;

      o = (OBJECT *)l -> data;
      if (IS_OBJECT(o)) {
	if (IS_VARTAG (o -> __o_vartags)) {
	  if (o -> __o_vartags -> tag) {
	    unlink_varentry (o);
	  }
	}
	__ctalkDeleteObject (o);
      }

      __xfree (MEMADDR(l));
      if ((l = l_prev) == NULL)
	break;
    }
    o = (OBJECT *)r -> user_objects -> data;
    if (IS_OBJECT(o)) {
      if (IS_VARTAG (o -> __o_vartags)) {
	if (o -> __o_vartags -> tag) {
	  unlink_varentry (o);
	}
      }
      __ctalkDeleteObject (o);
    }
    __xfree (MEMADDR(l));
    r -> user_objects = r -> user_object_ptr = NULL;
  }
}

void cleanup_unlink_method_user_object (VARENTRY *v) {
  if (!__cleanup_deletion)
    return;
  if (IS_OBJECT(v -> var_object)) {
    if (v -> var_object -> scope & METHOD_USER_OBJECT) {
      /*
	TODO - There hasn't been a case yet where the
	object is the start (or end) of the user_object list,
	so it isn't handled so far.  The user object deletion
	list doesn't use the list head pointer, anyway, so
	we should only need to worry if the start of a method
	user object list appears here.  It probably needs to
	be handled before we get here anyway.
      */
      if (v -> var_object -> prev)
	v -> var_object -> prev -> next = v -> var_object -> next;
      if (v -> var_object -> next)
	v -> var_object -> next -> prev = v -> var_object -> prev;
    }
  }
}

void varentry_cleanup_reset (VARENTRY *v) {
  if (v == NULL)
    return;

  if (v -> var_object == NULL) {
    if (v -> orig_object_rec == NULL) {
      return;
    }  else {
      v -> orig_object_rec = NULL;
      return;
    }
  }

  if (v -> var_object == v -> orig_object_rec)
    v -> var_object = v -> orig_object_rec = NULL;
  else
    v -> var_object = NULL;
}

/*
 *  This helps insure that the cleanup doesn't try to delete an
 *  object twice, in case the object's original VARENTRY is different
 *  than the VARENTRY where the cleanup occurs.
 */
/* #define USE_UNLINK_VARENTRY 1 */
void unlink_varentry (OBJECT *o) {
  if (IS_OBJECT(o)) {
    if (!IS_EMPTY_VARTAG(o -> __o_vartags)) {
      if(o -> __o_vartags -> tag -> orig_object_rec == o) {
	o -> __o_vartags -> tag -> orig_object_rec = NULL;
      }
      if(o -> __o_vartags -> tag -> var_object == o) {
	o -> __o_vartags -> tag -> var_object = NULL;
      }
    }
  }
}

/*
 * Similar to above, but the fn checks that the VARENTRY
 * is the same as the object's tag.  Useful in places
 * like save_local_objects_to_extra (), if we need to 
 * fold the results of a recursive or successive call back
 *  into a caller, and we don't want to lose the original tag.
 */
void unlink_varentry_2 (OBJECT *o, VARENTRY *v) {
  if (o -> __o_vartags) {
    if (o -> __o_vartags -> tag == v) {
      if (o -> __o_vartags -> tag) {
	if(o -> __o_vartags -> tag -> orig_object_rec == o) {
	  o -> __o_vartags -> tag -> orig_object_rec = NULL;
	}
	if(o -> __o_vartags -> tag -> var_object == o) {
	  o -> __o_vartags -> tag -> var_object = NULL;
	}
      }
    }
  }
}

static bool is_collection_member (OBJECT *o) {
  if (IS_OBJECT(o)) {
    return ((o -> attrs & OBJECT_IS_MEMBER_OF_PARENT_COLLECTION) &&
	    IS_OBJECT(o -> __o_p_obj));
  } else {
    return false;
  }
}

static void __delete_var_object (VARENTRY *v) {
  if (v -> var_object == v -> orig_object_rec) {
    v -> orig_object_rec = NULL;
  }
  __ctalkDeleteObject (v -> var_object);
}

static void __delete_method_local_objs (METHOD *m) {
  VARENTRY *v_ptr, *v_ptr_prev;
  int n;

  for (n = m -> nth_local_ptr; n >= 0; n--) {
    for (v_ptr = M_LOCAL_VAR_LIST(m); 
	 IS_VARENTRY(v_ptr) && IS_VARENTRY(v_ptr -> next);
	 v_ptr = v_ptr -> next)
      ;
    if (v_ptr == M_LOCAL_VAR_LIST(m)) {
      if (v_ptr && (v_ptr -> del_cnt == 0)) {
	if (IS_OBJECT(v_ptr -> var_object)) {
	  if (__cleanup_deletion) {
	    cleanup_unlink_method_user_object (v_ptr);
#if USE_UNLINK_VARENTRY
	    unlink_varentry (v_ptr -> var_object);
#endif
	    if (!is_collection_member (v_ptr -> var_object)) {
	      __delete_var_object (v_ptr);
	    } else {
	      v_ptr -> var_object = NULL;
	    }
	    if (IS_OBJECT(v_ptr -> orig_object_rec)) {
	      __ctalkDeleteObject (v_ptr -> orig_object_rec);
	    }
	    varentry_cleanup_reset (v_ptr);
	  } else {
	    __ctalkDeleteObject (v_ptr -> var_object);
	  }
	}
      }

      if (v_ptr && IS_OBJECT(v_ptr -> orig_object_rec) &&
	  (v_ptr -> orig_object_rec != v_ptr -> var_object)) {
#if USE_UNLINK_VARENTRY
	unlink_varentry (v_ptr -> orig_object_rec);
#endif
	__ctalkDeleteObject (v_ptr -> orig_object_rec);
      }

      delete_varentry (v_ptr);
      M_LOCAL_VAR_LIST(m) = NULL;
    } else { /* if (v_ptr == M_LOCAL_VAR_LIST(m)) */
      while (v_ptr != M_LOCAL_VAR_LIST(m)) {
	v_ptr_prev = v_ptr -> prev;

	if (v_ptr) {
	  if (v_ptr -> del_cnt == 0) {
	    if (IS_OBJECT(v_ptr -> var_object)) {
	      /* There should probaly be a check for
		 __cleanup_deletion here, too.  See argblk4.c 
		 for test, and the clause above for how to do it. */
	      cleanup_unlink_method_user_object (v_ptr);
#if USE_UNLINK_VARENTRY
	      unlink_varentry (v_ptr -> var_object);
#endif

	      if (is_collection_member (v_ptr -> var_object)) {
		v_ptr -> var_object = NULL;
	      } else {
		__delete_var_object (v_ptr);
	      }
	    }
	  }
	  if (IS_OBJECT(v_ptr -> orig_object_rec)) {
#if USE_UNLINK_VARENTRY
	    unlink_varentry (v_ptr -> orig_object_rec);
#endif
	    if (is_collection_member (v_ptr -> orig_object_rec)) {
	      v_ptr -> orig_object_rec = NULL;
	    } else {
	      __ctalkDeleteObject (v_ptr -> orig_object_rec);
	    }
	  }
	}

	delete_varentry (v_ptr);
	v_ptr = v_ptr_prev;
      }
      if (v_ptr && (v_ptr -> del_cnt == 0)) {
	if (IS_OBJECT(v_ptr -> var_object)) {
#if USE_UNLINK_VARENTRY
	  unlink_varentry (v_ptr -> var_object);
#endif
	  if (is_collection_member (v_ptr -> var_object)) {
	    v_ptr -> var_object = NULL;
	  } else {
	    __delete_var_object (v_ptr);
	  }
	}
      }
      if (IS_OBJECT(v_ptr -> orig_object_rec)) {
#if USE_UNLINK_VARENTRY
	unlink_varentry (v_ptr -> orig_object_rec);
#endif
	if (is_collection_member (v_ptr -> var_object)) {
	  v_ptr -> orig_object_rec = NULL;
	} else {
	  __ctalkDeleteObject (v_ptr -> orig_object_rec);
	}
      }
      delete_varentry (v_ptr);
      M_LOCAL_VAR_LIST(m) = NULL;
    } /* if (v_ptr == M_LOCAL_VAR_LIST(m)) */
    -- m -> nth_local_ptr;
  } /* for (n = m -> n_th_local_ptr... */
}

/* #define WARN_LEFTOVER_CVARS*/

static void __delete_method_local_cvars (METHOD *m) {
  CVAR *c_ptr, *c_ptr_prev;

  if (IS_CVAR (m -> local_cvars)) {
#ifdef WARN_LEFTOVER_CVARS
    printf ("<<<< Leftover CVARS in method: %s (class %s); selector %s .\n",
	    m -> name, m -> rcvr_class_obj -> __o_name, m -> selector);
#endif    
    for (c_ptr = m -> local_cvars; 
	 IS_CVAR(c_ptr) && 
	   IS_CVAR(c_ptr -> next); 
	 c_ptr = c_ptr -> next)
      ;
    if (c_ptr == m -> local_cvars) {
      _delete_cvar (c_ptr);
      m -> local_cvars = NULL;
    } else {
      while (c_ptr != m -> local_cvars) {
	c_ptr_prev = c_ptr -> prev;
	_delete_cvar (c_ptr);
	c_ptr = c_ptr_prev;
      }
      _delete_cvar (m -> local_cvars);
      m -> local_cvars = NULL;
    }
  } else {
    m -> local_cvars = NULL;
  }
}

static void cleanup_remaining_args (METHOD *m) {
  int __i;
  /*
   *  If the method still has arguments, the exit
   *  is probably due to an error, so simply delete
   *  them.
   */
  if (IS_METHOD (m)) {
    if (m -> n_args) {
      for (__i = 0; __i < m -> n_args; __i++) {
	if (m -> args[__i]) {
	  if (IS_OBJECT(m -> args[__i] -> obj))
	    __ctalkDeleteObject (m->args[__i] -> obj);
	  if (IS_ARG (m -> args[__i]))
	  __ctalkDeleteArgEntry (m->args[__i]);

	}
      }
    }
  }
}

void delete_object_list_internal (OBJECT *obj_list) {
  OBJECT *o, *o_prev;

 find_list_end: for (o = obj_list; 
		     IS_OBJECT(o) && IS_OBJECT(o -> next); 
		     o = o -> next)
    ;
  
  if (o == obj_list) {
    if (IS_VARTAG(o -> __o_vartags)) {
      if (o -> __o_vartags -> tag)
	++o -> __o_vartags -> tag -> del_cnt;
    }
    __ctalkDeleteObject (o);
    obj_list = NULL;
  } else {
    while (o != obj_list) {
      o_prev = o -> prev;

      if (IS_VARTAG (o -> __o_vartags)) {
	if (o -> __o_vartags -> tag) {
	  unlink_varentry (o);
	}
      }
      __ctalkDeleteObject (o);

      /* If one of the list objects got altered somewhere else
	 then it could lose its links. Check through the list 
	 again. */
      if (!o_prev)
	goto find_list_end;

      o = o_prev;
    }
    if (HAS_VARTAGS(obj_list)) {
      if (obj_list -> __o_vartags -> tag) {
	unlink_varentry (obj_list);
      }
    }
    __ctalkDeleteObject (obj_list);
    obj_list = NULL;
  }
}

static void __delete_global_vars (VARENTRY *v) {
  VARENTRY *v_ptr, *v_ptr_prev;

  for (v_ptr = v; v_ptr && v_ptr -> next;
       v_ptr = v_ptr -> next)
    ;
  if (v_ptr == v) {
    if (v_ptr && (v_ptr -> del_cnt == 0)) {
      if (IS_OBJECT(v_ptr -> var_object)) {
	if (__cleanup_deletion) {
	  cleanup_unlink_method_user_object (v_ptr);
#if USE_UNLINK_VARENTRY
	  unlink_varentry (v_ptr -> var_object);
#endif
	  __delete_var_object (v_ptr);
	  varentry_cleanup_reset (v_ptr);
	} else {
	  __ctalkDeleteObject (v_ptr -> var_object);
	}
      }
      if (IS_OBJECT(v_ptr -> orig_object_rec)) {
#if USE_UNLINK_VARENTRY
	unlink_varentry (v_ptr -> orig_object_rec);
#endif
	__ctalkDeleteObject (v_ptr -> orig_object_rec);
      }
    }
    delete_varentry (v_ptr);
    v = NULL;
  } else { /* if (v_ptr == M_LOCAL_VAR_LIST(m)) */
    while (v_ptr != v) {
      v_ptr_prev = v_ptr -> prev;
      if (v_ptr) {
	if (v_ptr -> del_cnt == 0) {
	  if (IS_OBJECT(v_ptr -> var_object)) {
	    /* There should probaly be a check for
	       __cleanup_deletion here, too.  See argblk4.c 
	       for test, and the clause above for how to do it. */
	    cleanup_unlink_method_user_object (v_ptr);
#if USE_UNLINK_VARENTRY
	    unlink_varentry (v_ptr -> var_object);
#endif
	    __delete_var_object (v_ptr);
	  }
	}
	if (IS_OBJECT(v_ptr -> orig_object_rec)) {
#if USE_UNLINK_VARENTRY
	  unlink_varentry (v_ptr -> orig_object_rec);
#endif
	  __ctalkDeleteObject (v_ptr -> orig_object_rec);
	}
      }

      delete_varentry (v_ptr);
      v_ptr = v_ptr_prev;
    }
    if (v_ptr && (v_ptr -> del_cnt == 0)) {
      if (IS_OBJECT(v_ptr -> var_object)) {
#if USE_UNLINK_VARENTRY
	unlink_varentry (v_ptr -> var_object);
#endif
	__delete_var_object (v_ptr);
      }
    }
    if (IS_OBJECT(v_ptr -> orig_object_rec)) {
#if USE_UNLINK_VARENTRY
      unlink_varentry (v_ptr -> orig_object_rec);
#endif
      __ctalkDeleteObject (v_ptr -> orig_object_rec);
    }
    delete_varentry (v_ptr);
    v = NULL;
  } /* if (v_ptr == v) */
}

void __ctalkClassDictionaryCleanup (void) {

  OBJECT *o;
  METHOD *m, *m_ptr;
  struct sigaction sa, old_sa;

  sa.sa_sigaction = cleanup_handler;
  sa.sa_flags = SA_RESETHAND|SA_SIGINFO;
  sigemptyset (&(sa.sa_mask));
  sigaction (SIGSEGV, &sa, &old_sa);

  __cleanup_deletion = TRUE;

  if (__ctalk_dictionary) {
    __delete_global_vars (__ctalk_dictionary);
    __ctalk_dictionary = __ctalk_last_object = NULL;
  }

  if (__ctalk_classes && __ctalk_last_class) {
    for (o = __ctalk_last_class; o; o = o -> prev) {
      if (o -> instance_methods) {
	for (m_ptr = o -> instance_methods; m_ptr && m_ptr->next; 
	     m_ptr = m_ptr -> next)
	  ;
	if (m_ptr == o -> instance_methods) {
	  __delete_method_params (m_ptr);
 	  if (M_LOCAL_VAR_LIST(m_ptr)) 
	    __delete_method_local_objs (m_ptr);
	  if (m_ptr->local_cvars) __delete_method_local_cvars (m_ptr);
	  if (m_ptr->user_objects) __delete_method_user_objs (m_ptr);
	  if (IS_METHOD (o -> instance_methods))
	    __xfree (MEMADDR(o -> instance_methods));
	  o -> instance_methods = NULL;
	} else {
	  while (m_ptr != o -> instance_methods) {
	    m = m_ptr;
	    m_ptr = m_ptr -> prev;
	    __delete_method_params (m);
	    if (M_LOCAL_VAR_LIST(m)) 
	      __delete_method_local_objs (m);
	    if (m->local_cvars) __delete_method_local_cvars (m);
 	    if (m->user_objects) __delete_method_user_objs (m);
	    cleanup_remaining_args (m);
	    if (IS_METHOD (m))
	      __xfree (MEMADDR(m));
	  }
	  __delete_method_params(m_ptr);
	  if (M_LOCAL_VAR_LIST(m_ptr)) __delete_method_local_objs (m_ptr);
	  if (m_ptr->local_cvars) __delete_method_local_cvars (m_ptr);
	  if (m_ptr->user_objects) __delete_method_user_objs (m_ptr);
	  cleanup_remaining_args (m_ptr);
	  if (IS_METHOD(m_ptr))
	    __xfree (MEMADDR(m_ptr));
	  o -> instance_methods = NULL;
	}
      }
      if (o -> class_methods) {
	for (m_ptr = o -> class_methods; m_ptr && m_ptr->next; 
	     m_ptr = m_ptr -> next)
	  ;
	if (m_ptr == o -> class_methods) {
	  __delete_method_params (o->class_methods);
	  if (M_LOCAL_VAR_LIST(o->class_methods)) 
	    __delete_method_local_objs (o->class_methods);
	  if (o->class_methods->local_cvars) 
	    __delete_method_local_cvars (o->class_methods);
	  if (o->class_methods->user_objects) 
	    __delete_method_user_objs (m_ptr);
	  if (IS_METHOD (o -> class_methods))
	    __xfree (MEMADDR(o -> class_methods));
	  o -> class_methods = NULL;
	} else {
	  while (m_ptr != o -> class_methods) {
	    m = m_ptr;
	    m_ptr = m_ptr -> prev;
	    __delete_method_params (m);
	    if (M_LOCAL_VAR_LIST(m)) __delete_method_local_objs (m);
	    if (m->local_cvars) __delete_method_local_cvars (m);
 	    if (m->user_objects) __delete_method_user_objs (m);
	    if (IS_METHOD (m))
	      __xfree (MEMADDR(m));
	  }
	  __delete_method_params (m_ptr);
	  if (M_LOCAL_VAR_LIST(m_ptr)) __delete_method_local_objs (m_ptr);
	  if (m_ptr->local_cvars) __delete_method_local_cvars (m_ptr);
	  if (m_ptr->user_objects) __delete_method_user_objs (m_ptr);
	  if (IS_METHOD (m_ptr))
	    __xfree (MEMADDR(m_ptr));
	  o -> class_methods = NULL;
	}
      }
      if (o -> instancevars) {
	delete_object_list_internal (o -> instancevars);
	o -> instancevars = NULL;
      }
      if (o -> classvars) {
	delete_object_list_internal (o -> classvars);
	o -> classvars = NULL;
      }
    } /* for (o = __ctalk_classes ... */

    delete_object_list_internal (__ctalk_classes);
    __ctalk_classes = __ctalk_last_class = NULL;

  } /* if (__ctalk_classes) */

}

void __ctalkRemoveClass (OBJECT *o) {
  if (!o -> prev) {
    if (o -> next) o -> next -> prev = NULL;
    __ctalk_classes = o -> next;
    if (!__ctalk_classes)
      __ctalk_last_class = __ctalk_classes;
  } else {
    if (!o -> next) {
      if (o -> prev) o -> prev -> next = NULL;
      __ctalk_last_class = o -> prev;
    } else {
      o -> prev -> next = o -> next;
      o -> next -> prev = o -> prev;
    }
  }
}

OBJECT *__ctalk_find_classvar (char *__name, char *__classname) {

  char keybuf[MAXLABEL * 2];
  OBJECT *r;

  if (classvarhash == NULL)
    return NULL;

  if (__classname) {
    strcatx (keybuf, __name, ":", __classname, NULL);
    r = (OBJECT *)_hash_get (classvarhash, keybuf);
    return r;
  } else {
    strcatx (keybuf, __name, ":(null)", NULL);
    r = (OBJECT *)_hash_get (classvarhash, keybuf);
    return r;
  }

}

void init_default_class_cache (void) {
  rt_defclasses = (DEFAULTCLASSCACHE *)__xalloc 
    (sizeof (struct _defaultclasscache));
}

void delete_default_class_cache (void) {
  __xfree (MEMADDR(rt_defclasses));
}

void rt_delete_class_names (void) {
  LIST *l;
  for (l = classnames; l; l = l -> next) {
    __xfree (MEMADDR(l -> data));
    l -> data = NULL;
  }
  delete_list (&l);

}
