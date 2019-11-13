/* $Id: class.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2018 Robert Kiesling, rk3314042@gmail.com.
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
#include <stdarg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "classlib.h"
#include "symbol.h"

extern OBJECT *classes;       /* Declared in lib/cclasses.c */
extern OBJECT *last_class;

int library_input = FALSE;    /* TRUE if evaluating class libraries.      */

extern int error_line,        /* Declared in lex.c.                       */
  error_column;
extern int ctalkdefs_h_lines;

extern char                   /* Declared in main.c.                      */
input_source_file[FILENAME_MAX];
extern int method_from_proto; /* Declared in mthdref.c.                   */
extern I_PASS interpreter_pass;
extern int nolinemarker_opt; 
extern int verbose_opt;
extern int warn_classlib_opt;
extern char libcachedir[FILENAME_MAX];
extern char ctalkuserhomedir[FILENAME_MAX];

extern CTRLBLK *ctrlblks[MAXARGS + 1];
extern int ctrlblk_ptr;
extern bool ctrlblk_pred,            /* Global states.      */
   ctrlblk_blk;
extern int for_init, 
  for_term,
  for_inc;
extern char ctpp_ofn[FILENAME_MAX];  /* File name of any preprocessor output.*/
  
static CLASSLIB *lib_cache,           /* List of previous library searches.*/
  *cache_ptr;

int add_class_object (OBJECT *o) {

  if (!classes) {
    classes = o;
    last_class = o;
  } else {
    last_class -> next = o;
    o -> prev = last_class;
    last_class = o;
  }

  cache_class_ptr (o);

  return SUCCESS;
}

OBJECT *class_object_search (char *name, int warn) {

  OBJECT *o;

  if (!classes) 
    return NULL;

  for (o = classes; ; o = o -> next) {
    if (!strcmp (o -> __o_name, name))
      return o;
    if (!o || !o -> next)
      break;
  }

  if (!library_search (name, warn))
    for (o = classes; ; o = o -> next) {
      if (!strcmp (o -> __o_name, name))
	return o;
      if (!o || !o -> next)
	break;
    }

  return NULL;
}

CLASSLIB *lib_includes[MAXARGS + 1];
int lib_includes_ptr;

CLASSLIB *input_declarations[MAXARGS + 1];
int input_declarations_ptr;

int get_lib_includes_ptr (void) {
  return lib_includes_ptr;
}

CLASSLIB *lib_include_at (int ptr) {
  return lib_includes[ptr];
}

char *library_pathname (void) {
  if (lib_includes_ptr >= MAXARGS)
    return NULL;
  else
    return lib_includes[lib_includes_ptr + 1] -> path;
}

void push_library_include (char *path, char *name, int src_line) {

  CLASSLIB *c;

  if (lib_includes_ptr == 0)
    _error ("push_library_include: stack overflow.");

  if ((c = (CLASSLIB *)__xalloc (sizeof (CLASSLIB))) == NULL)
    _error ("push_library_include: %s.", strerror (errno));

  c -> sig = CLASSLIB_SIG;
  strcpy (c -> path, path);
  strcpy (c -> name, name);
  strcpy (c -> included_from_filename, __source_filename ());
  if (str_eq (name, OBJECT_CLASSNAME))
    c -> included_from_line = 1;
  else
    c -> included_from_line = src_line;

  lib_includes[lib_includes_ptr--] = c;

  if (warn_classlib_opt) {
    debug ("%d. %s.", lib_includes_ptr + 1, c -> path);
  
    if (lib_includes_ptr + 2 < MAXARGS) {
      int i;
      for (i = lib_includes_ptr + 2; i <= MAXARGS; i++)
	debug ("\tFrom %s, Line %d.", lib_includes[i] -> path, 1);
    }
    debug ("\tFrom %s, Line %d.", input_source_file, error_line);
  }
  
}

CLASSLIB *pop_library_include (void) {

  CLASSLIB *c;
  if (lib_includes_ptr > MAXARGS)
    _error ("pop_library_include: stack underflow.");

  if (warn_classlib_opt) {
    debug ("%d. Exiting %s.", 
	   lib_includes_ptr + 1, 
	   lib_includes[lib_includes_ptr + 1] -> path);
  }

  c = lib_includes[++lib_includes_ptr];
  __ctalkRtSaveSourceFileName (c -> included_from_filename);
  error_line = c -> included_from_line;
  lib_includes[lib_includes_ptr] = NULL;
  return c;
}

/* 
 *  Cache the path of a library search to avoid repeating the
 *  search and redefining objects.
 */

int cache_library_search (CLASSLIB *c) {
  if (!cache_ptr) {
    lib_cache = cache_ptr = c;
  } else {
    cache_ptr -> next = c;
    c -> prev = cache_ptr;
    cache_ptr = c;
  }
  return SUCCESS;
}

/* 
 * Keep searching the library paths for all files of the same name, in
 * case, for example, there is a library module called, "new."
 */

/*
 *  Last line marker info encountered by parser_pass ().
 */

extern struct {
  int line;
  char filename[FILENAME_MAX];
  int flag;
} line_marker;

int __remove_duplicate_vars = FALSE;

int library_search (char *class, int warn) {

  char *path, *inbuf, cache_path[FILENAME_MAX];
  int i, r;
  int prev_library_input;
  int parent_error_line;
  int find_next = FALSE;
  OBJECT *parent_rcvr_class;
  I_PASS last_pass;
  CLASSLIB *l, *libinc;
  struct stat statbuf;
  
  if (!class || ! strlen (class))
    return ERROR;

 next_file:

  if ((path = find_library_include (class, find_next)) == NULL)
    return ERROR;

  /*
   *  Determine if we've already loaded the library _file_.  
   *  This should be factored out if we use it a lot.
   */
  for (l = lib_cache; l; l = l -> next) {
    if (!strcmp (l -> path, path))
      return SUCCESS;
  }
  
  /* 
   *  If this is a library we're currently loading, 
   *  also return.
   *
   *  TO DO - This can be refactored better with the 
   *  library cache so we don't have to look up the
   *  path directly.
   */
  for (i = lib_includes_ptr + 1; i <= MAXARGS; i++) {
    if (!strcmp (lib_includes[i] -> path, path)) {
      if (warn && (interpreter_pass != expr_check)) {
	printf ("warning: Class %s:\n", class);
	if (i < MAXARGS) {
	  printf ("  is already included from class %s.\n",
		  lib_includes[i+1] -> name);
	  printf ("  Consider adding a, \"require %s,\" statement\n", 
		  class);
	  printf ("  to class %s.\n", lib_includes[i+1] -> name);
	
	} else {
	  printf ("  is already included from top level.\n");
	  printf ("  Consider adding a, \"require %s,\" statement\n", 
		  class);
	}
      }
      return SUCCESS;
    }
  }

#ifdef CLASSLIB_TRACE
  debug ("Class lib %s.", path);
#endif

  if (is_dir (path)) {
#ifdef CLASSLIB_TRACE
    _warning ("Class lib %s: is a directory.\n");
#endif
    find_next = TRUE;
    goto next_file;
  }

  if (is_binary_file (path))
    return ERROR;

  if (!has_class_declaration (path, class))
    return ERROR;

  /*
   *  Set the source file to the library file after saving the 
   *  calling source file.
   */
  prev_library_input = library_input;
  library_input = TRUE;
  last_pass = interpreter_pass;
  interpreter_pass = library_pass;
  parent_rcvr_class = get_rcvr_class_obj ();
  push_library_include (path, class, error_line);

  if (ctpp_deps_updated (path)) { 
    /* 
     *  Fills in ctpp_ofn.
     */
    if ((r = preprocess (path, false, true)) == ERROR) 
      goto recover;
    cache_ctpp_output_file (ctpp_ofn);
  } else {
    r = 0;
  }

  strcatx (cache_path, libcachedir, "/", class, ".ctpp", NULL);

  stat (cache_path, &statbuf);
  lib_includes[lib_includes_ptr + 1] -> file_size = statbuf.st_size;
  
  if ((inbuf = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
    _error ("library search %s: %s.", class, strerror (errno));

  (void)read_file (inbuf, cache_path);

  parent_error_line = error_line;
  error_line = 1; error_column = 1;
  (void)parse (inbuf, (long long) statbuf.st_size);
  error_line = parent_error_line;

  __xfree (MEMADDR(inbuf));

 recover:
  library_input = prev_library_input;
  interpreter_pass = last_pass;
  set_rcvr_class_obj (parent_rcvr_class);

  /*
   * Do not delete the lib_include entry because we use it in the
   * cache to prevent a class library from being loaded more than
   * once.
   *
   * There should be an exception handler here, but no errors
   * have yet occurred at this point.
   */

  libinc = pop_library_include ();

  if (r != ERROR)
    cache_library_search (libinc);
  else
    __xfree (MEMADDR(libinc));

  return r;
}

extern MESSAGE *p_messages[P_MESSAGES+1];   /* Declared in preprocess.c. */
extern int p_message_ptr;

extern int parser_output_ptr;
extern int last_fileout_stmt;

int require_class (MESSAGE_STACK messages, int msg_ptr) {

  int operand_ptr,
    term_ptr;
  int old_parser_output_ptr,
    old_last_fileout_stmt;

  if ((interpreter_pass == parsing_pass) ||
      (interpreter_pass == library_pass)) {
    
    if (((operand_ptr = nextlangmsg (messages, msg_ptr)) != ERROR) &&
	((term_ptr = nextlangmsg (messages, operand_ptr)) != ERROR)) {

      if ((messages[operand_ptr] -> tokentype != LABEL) ||
	  (messages[term_ptr] -> tokentype != SEMICOLON)) {
	warning (messages[msg_ptr], "Syntax error.");
	return ERROR;
      }

      ++messages[msg_ptr] -> evaled;
      ++messages[msg_ptr] -> output;
      ++messages[operand_ptr] -> evaled;
      ++messages[operand_ptr] -> output;
      ++messages[term_ptr] -> evaled;
      ++messages[term_ptr] -> output;


      old_parser_output_ptr = parser_output_ptr;
      old_last_fileout_stmt = last_fileout_stmt;

      if (nolinemarker_opt) {
	if (str_eq (messages[operand_ptr] -> name, OBJECT_CLASSNAME)) {
	  error_line = error_column = -ctalkdefs_h_lines + 1;
	}
      }
      if (library_search (messages[operand_ptr] -> name, FALSE) < 0)
	warning (messages[operand_ptr], "require: Class, \"%s.\" not found.",
		 M_NAME(messages[operand_ptr]));

      parser_output_ptr = old_parser_output_ptr;
      last_fileout_stmt = old_last_fileout_stmt;
    }

  }

  return SUCCESS;
}

int is_pending_class (char *s) {
  int i;

  for (i = lib_includes_ptr + 1; i <= MAXARGS; i++) {
    if (!strcmp (lib_includes[i] -> name, s))
      return TRUE;
  }
  return FALSE;
}

int is_cached_class (char *s) {
  CLASSLIB *c;
  for (c = lib_cache; c; c = c -> next) {
    if (!strcmp (c -> name, s))
      return TRUE;
  }
  return FALSE;
}

int add_instance_variables (OBJECT *class, OBJECT *o) {
  OBJECT *var;

  for (var = class -> instancevars; var; var = var -> next) {
    if (!__ctalkGetInstanceVariable (o, var -> __o_name, FALSE)) {
      (void)__ctalkAddInstanceVariable (o, var -> __o_name, var);
    }
  }
  return SUCCESS;
}

int instance_variables_from_class_definition (OBJECT *o) {
  OBJECT *class_object, *superclass_object;

  if ((class_object = o -> __o_class) == NULL) {
    if ((class_object = get_class_object (o -> __o_classname)) == NULL) {
      __ctalkExceptionInternal (message_stack_at (get_messageptr ()),
				undefined_class_x, o -> __o_classname,0);
      return ERROR;
    }
  }
    
  add_instance_variables (class_object, o);

  for (superclass_object = class_object -> __o_superclass;
       superclass_object && *superclass_object -> __o_superclassname;
       superclass_object = superclass_object -> __o_superclass)
    add_instance_variables (superclass_object, o);

  return SUCCESS;
}

static bool _is_subclass_sub (OBJECT *obj, char *superclassname) {
  if (str_eq (obj -> __o_name, superclassname)) {
    return true;
  } else {
    if (IS_OBJECT(obj -> __o_superclass)) {
      return _is_subclass_sub (obj-> __o_superclass, superclassname);
    }
  }
  return false;
}

bool is_subclass_of (char *classname, char *superclassname) {
  OBJECT *class_object;

  if ((class_object = get_class_object (classname)) == NULL) {
    return false;
  }

  if (IS_OBJECT(class_object -> __o_superclass))
    return 
      _is_subclass_sub (class_object  -> __o_superclass, superclassname);
    
  return false;
}

static void delete_methods (METHOD *method_list) {

  METHOD *method_ptr, *method_ptr_prev;
  OBJECT *obj_ptr, *obj_ptr_prev;
  CVAR *cvar_ptr, *cvar_ptr_prev;
  int param_idx;

  for (method_ptr = method_list; 
       method_ptr && method_ptr -> next; 
       method_ptr = method_ptr -> next)
    ;

  while (method_ptr != method_list) {
    method_ptr_prev = method_ptr -> prev;
    if (M_LOCAL_OBJ_LIST(method_ptr)) {
      for (obj_ptr = M_LOCAL_OBJ_LIST(method_ptr);
	   obj_ptr && obj_ptr -> next;
	   obj_ptr = obj_ptr -> next)
	;
      while (obj_ptr != M_LOCAL_OBJ_LIST(method_ptr)) {
	obj_ptr_prev = obj_ptr -> prev;
	delete_object (obj_ptr);
	obj_ptr = obj_ptr_prev;
      }
      if (!IS_OBJECT(M_LOCAL_OBJ_LIST(method_ptr))) {
	_warning ("Delete method: %s (class %s): Invalid local object.\n",
		  method_ptr->name, 
		  ((method_ptr->rcvr_class_obj) ? 
		   method_ptr->rcvr_class_obj->__o_name :
		   OBJECT_CLASSNAME));
      } else {
	delete_object (M_LOCAL_OBJ_LIST(method_ptr));
      }
    }
    if (method_ptr -> local_cvars) {
      for (cvar_ptr = method_ptr -> local_cvars;
	   cvar_ptr && cvar_ptr -> next;
	   cvar_ptr = cvar_ptr -> next)
	;
      while (cvar_ptr != method_ptr -> local_cvars) {
	cvar_ptr_prev = cvar_ptr -> prev;
	_delete_cvar (cvar_ptr);
	cvar_ptr = cvar_ptr_prev;
      }
      _delete_cvar (method_ptr -> local_cvars);
    } 
    if (method_ptr -> n_params) {
      for (param_idx = 0; param_idx < method_ptr -> n_params; param_idx++) {
	__xfree (MEMADDR(method_ptr -> params[param_idx]));
      }
    }
    __xfree (MEMADDR(method_ptr -> src));
    __xfree (MEMADDR(method_ptr));
    method_ptr = method_ptr_prev;
  }

  if (M_LOCAL_OBJ_LIST(method_list)) {
    for (obj_ptr = M_LOCAL_OBJ_LIST(method_list);
	 obj_ptr && obj_ptr -> next;
	 obj_ptr = obj_ptr -> next)
      ;
    while (obj_ptr != M_LOCAL_OBJ_LIST(method_list)) {
      obj_ptr_prev = obj_ptr -> prev;
      delete_object (obj_ptr);
      obj_ptr = obj_ptr_prev;
    }
    delete_object (M_LOCAL_OBJ_LIST(method_list));
  }
  if (method_list -> local_cvars) {
    for (cvar_ptr = method_list -> local_cvars;
	 cvar_ptr && cvar_ptr -> next;
	 cvar_ptr = cvar_ptr -> next)
      ;
    while (cvar_ptr != method_list -> local_cvars) {
      cvar_ptr_prev = cvar_ptr -> prev;
      _delete_cvar (cvar_ptr);
      cvar_ptr = cvar_ptr_prev;
    }
    _delete_cvar (method_list -> local_cvars);
  }
  if (method_list -> n_params) {
    for (param_idx = 0; param_idx < method_list -> n_params; param_idx++) {
      __xfree (MEMADDR(method_list -> params[param_idx]));
    }
  }
  __xfree (MEMADDR(method_list -> src));
  __xfree (MEMADDR(method_list));
}

void delete_class_library (void) {

  OBJECT *class_ptr, *class_ptr_prev;

  class_ptr = last_class;

  if (!class_ptr)
    return;

  while (TRUE) {

    if (class_ptr -> instance_methods) {
      delete_methods (class_ptr -> instance_methods);
      class_ptr -> instance_methods = NULL;
    }

    if (class_ptr -> class_methods) {
      delete_methods (class_ptr -> class_methods);
      class_ptr -> class_methods = NULL;
    }

    if (class_ptr == classes)
      break;
    class_ptr = class_ptr -> prev;
  }

  class_ptr = last_class;
  while (class_ptr != classes) {
    class_ptr_prev = class_ptr -> prev;
    delete_object (class_ptr);
    class_ptr = class_ptr_prev;
  }
  delete_object (classes);
}

void push_input_declaration (char *path, char *name, int src_line) {

  CLASSLIB *c;

  if (lib_includes_ptr == 0)
    _error ("push_input_declaration: stack overflow.");

  if ((c = (CLASSLIB *)__xalloc (sizeof (CLASSLIB))) == NULL)
    _error ("push_input_declaration: %s.", strerror (errno));

  c -> sig = CLASSLIB_SIG;
  strcpy (c -> path, path);
  strcpy (c -> name, name);
  strcpy (c -> included_from_filename, __source_filename ());
  c -> included_from_line = src_line;

  input_declarations[input_declarations_ptr--] = c;

  if (warn_classlib_opt) {
    debug ("%d. %s.", input_declarations_ptr + 1, c -> path);
  
    if (input_declarations_ptr + 2 < MAXARGS) {
      int i;
      for (i = input_declarations_ptr + 2; i <= MAXARGS; i++)
	debug ("\tFrom %s, Line %d.", input_declarations[i] -> path, 1);
    }
    debug ("\tFrom %s, Line %d.", input_source_file, error_line);
  }
  
}

CLASSLIB *pop_input_declaration (void) {

  CLASSLIB *c;
  if (input_declarations_ptr > MAXARGS)
    _error ("pop_input_declaration: stack underflow.");

  if (warn_classlib_opt) {
    debug ("%d. Exiting %s.", 
	   input_declarations_ptr + 1, 
	   input_declarations[input_declarations_ptr + 1] -> path);
  }

  c = input_declarations[++input_declarations_ptr];
  error_line = c -> included_from_line;
  input_declarations[input_declarations_ptr] = NULL;
  return c;
}

void init_library_include_stack (void) {
  lib_includes_ptr = MAXARGS;
  input_declarations_ptr = MAXARGS;
  lib_cache = cache_ptr = NULL;
}

/* 
 *  Object class and primitive methods, "class," "new," "method," 
 *  "instanceVariable," and, "classVariable."
 *
 */
int init_default_classes (void) {
  
  OBJECT *o;                   /* The default Object class object. */
  char buf[MAXMSG];

  o = create_object ("Class", "Object");
#if defined (__GNUC__) && defined (__x86_64) && defined (__amd64__)
  SNPRINTF (buf, MAXMSG, "%#lx", (unsigned long int)o);
#else
  SNPRINTF (buf, MAXMSG, "%#x", (unsigned int)o);
#endif
  __ctalkSetObjectValue (o, buf);
  add_class_object (o);
  generate_global_object_definition ("Object", "Class", NULL, buf, GLOBAL_VAR);
  define_primitive_method ("Object", "class", define_class, 1, FALSE);
  define_primitive_method ("Object", "new", new_object, 1, TRUE);
  define_primitive_method ("Object", "instanceMethod", 
			   new_instance_method, 1, FALSE);
  define_primitive_method ("Object", "classMethod", 
			   new_class_method, 1, FALSE);
  define_primitive_method ("Object", "instanceVariable", 
			   define_instance_variable, 1, TRUE);
  define_primitive_method ("Object", "classVariable", 
			   define_class_variable, 1, TRUE);
  generate_global_method_definition_call ("Object", "class", 
					  "__ctalk_define_class", 1, 0);
  generate_instance_method_param_definition_call ("Object", 
						  "class",
						  "__ctalk_define_class",
						  "__className", 
						  "String",
						  TRUE,
						  FALSE);
  generate_global_method_definition_call ("Object", "new", 
					  "__ctalk_new_object", 1, 0);
  generate_instance_method_param_definition_call ("Object", "new",
						  "__ctalk_new_object",
 						  "__objectName", 
 						  "String",
 						  TRUE,
 						  FALSE);
  generate_global_method_definition_call ("Object", "instanceVariable", 
					  "__ctalkDefineInstanceVariable", 2, 0);
  generate_instance_method_param_definition_call ("Object", 
						  "instanceVariable",
					  "__ctalkDefineInstanceVariable",
 						  "__varName", 
 						  "String",
 						  TRUE,
 						  FALSE);
  generate_instance_method_param_definition_call ("Object", 
						  "instanceVariable",
					  "__ctalkDefineInstanceVariable",
 						  "...", 
 						  "ArgumentList",
 						  FALSE,
 						  FALSE);
  generate_global_method_definition_call ("Object", "classVariable", 
					  "__ctalkDefineClassVariable", 2, 0);
  generate_instance_method_param_definition_call ("Object", 
 						  "classVariable",
						  "__ctalkDefineClassVariable",
 						  "__varName", 
 						  "String",
 						  TRUE,
 						  FALSE);
  generate_instance_method_param_definition_call ("Object", 
 						  "classVariable",
						  "__ctalkDefineClassVariable",
 						  "...", 
 						  "ArgumentList",
 						  FALSE,
 						  FALSE);
  /* Initialize the pseudo-classes CFuncion and Expr. 
   *   NOTE - The classes must be initialized after Object class
   *   and before any template methods. This places the 
   *   class initialization calls directly after the primitive
   *   methods.  See the #defines for TEMPLATE_CLASS_INIT_PRIORITY
   *   and TEMPLATE_INIT_PRIORITY in fm_tmpl.c.
   */
  init_c_fn_class ();
  return SUCCESS;
}

MESSAGE *o_messages[N_MESSAGES + 1];  /* C language message stack and      */
int o_message_ptr = N_MESSAGES;       /* stack pointer.  Also used by      */

int o_message_push (MESSAGE *m) {
  if (o_message_ptr == 0) {
    warning (m, "o_message_push: stack overflow.");
    return ERROR;
  }
  o_messages[o_message_ptr--] = m;
  return o_message_ptr;
}

/*
 *  Find a class declaration sequence:
 *
 *    <superclassname> class <classname>;
 *
 *  Return FALSE if we don't find a class declaration.
 *
 *  Also cache the results so we don't have to search the
 *  class file every time we want to check a class name.
 */

LIST *valid_class_decls = NULL;

void push_decl_check (char *classname) {
  LIST *l = new_list ();
  l -> data = strdup (classname);
  if (valid_class_decls == NULL) {
    valid_class_decls = l;
  } else {
    list_push (&valid_class_decls, &l);
  }
}

#define CLASS_DECL_FN  "classnames"

void restore_decl_list (void) {
  FILE *f;
  LIST *l;
  struct stat statbuf;
  char *nl;
  char decl_path[FILENAME_MAX + 11];  /* + strlen ("/classnames") */
  char inbuf[MAXLABEL];
  sprintf (decl_path, "%s/%s", ctalkuserhomedir, CLASS_DECL_FN);
  if (stat (decl_path, &statbuf) != 0) {
    return;
  }
  if ((f = fopen (decl_path, "r")) == NULL) {
    printf ("ctalk: %s: %s\n", decl_path, strerror (errno));
    exit (EXIT_FAILURE);
  }
  while (fgets (inbuf, MAXLABEL, f)) {
    if (*inbuf == '#')
      continue;
    if ((nl = strchr (inbuf, '\n')) != NULL)
      *nl = '\0';
    l = new_list ();
    l -> data = strdup (inbuf);
    if (valid_class_decls == NULL) {
      valid_class_decls = l;
    } else {
      list_push (&valid_class_decls, &l);
    }
  }
  if (fclose (f)) {
    printf ("ctalk: %s: %s\n", decl_path, strerror (errno));
    exit (EXIT_FAILURE);
  }
}

void save_decl_list (void) {
  FILE *f;
  LIST *l;
  char decl_path[FILENAME_MAX + 11];  /* + strlen ("/classnames") */

  sprintf (decl_path, "%s/%s", ctalkuserhomedir, CLASS_DECL_FN);
  if ((f = fopen (decl_path, "w")) == NULL) {
    printf ("ctalk: %s: %s.\n", decl_path, strerror (errno));
    exit (EXIT_FAILURE);
  }
  fputs ("# This is a machine generated file. Do not edit!\n", f);
  for (l = valid_class_decls; l; l = l -> next) {
    fputs ((char *)l -> data, f), fputc ('\n', f);
  }

  if (fclose (f)) {
    printf ("ctalk: %s: %s.\n", decl_path, strerror (errno));
    exit (EXIT_FAILURE);
  }
}

int has_class_declaration (char *path, char *classname) {
  FILE *f;
  char *buf, *keyword, *sc_start, *sc_end, *followingtok, *cl_start,
    *cl_end;
  char scbuf[MAXLABEL], clbuf[MAXLABEL];
  long long int chars_read;
  LIST *l;
  struct stat statbuf;
  bool have_superclass = false;

  errno = 0;
  if (stat (path, &statbuf)) {
    if (errno && (errno != ENOENT))
      _warning ("ctalk: %s: %s.", path, strerror (errno));
    return FALSE;
  }

  /* Check whether the class is already tested. */
  for (l = valid_class_decls; l; l = l -> next) {
    if (str_eq (l -> data, classname)) {
      return TRUE;
    }
  }

  if (str_eq (classname, OBJECT_CLASSNAME)) {
    push_decl_check (OBJECT_CLASSNAME);
    return TRUE;
  }

  errno = 0;

  if ((f = fopen (path, "r")) == NULL) {
    if (errno && (errno != ENOENT))
      _warning ("ctalk: %s: %s.", path, strerror (errno));
    return FALSE;
  }

  if ((buf = (char *)__xalloc (statbuf.st_size + 1)) == NULL) {
    _error ("has_class_declaration (__xalloc): %s.\n", strerror (errno));
  }
  chars_read = fread ((void *)buf, sizeof(char),
		      (size_t) statbuf.st_size, f);
  if (errno)
    _warning ("has_class_declaration (fread): %s: %s.", 
	      path, strerror (errno));
  if (chars_read != statbuf.st_size) {
    printf ("chars_read:  %lld, st_size: %lld\n", chars_read,
	    (long long int)statbuf.st_size);
  }
  if (fclose (f)) {
    _warning ("has_class_declaration (fclose): %s: %s.", 
	      path, strerror (errno));
    __xfree (MEMADDR(buf));
    return FALSE;
  }
  
  keyword = buf;
  while ((keyword = strstr (keyword, "class")) != NULL) {
    if ((sc_end = keyword - 1) < 0) {
      keyword += 5;
      continue;
    }
    while (isspace ((int)*sc_end))
      --sc_end;
    ++sc_end;
    if (sc_end <= 0) {
      keyword += 5;
      continue;
    }
    sc_start = sc_end - 1;
    while (isalnum ((int)*sc_start) || *sc_start == '_')
      --sc_start;
    ++sc_start;
    memset (scbuf, 0, MAXLABEL);
    strncpy (scbuf, sc_start, sc_end - sc_start);
    have_superclass = false;
    if (!is_class_library_name (scbuf)) {
      /* is_class_library_name checks the classe files in directories
	 that are present when ctalk starts, regardless of whether the
	 class is parsed yet.  valid_class_decls checks classes added
	 - and parsed - when they are in any of the source files while
	 ctalk is running. */
      for (l = valid_class_decls; l; l = l -> next) {
	if (str_eq (l -> data, scbuf)) {
	  have_superclass = true;
	}
      }
    } else {
      have_superclass = true;
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
    } else {

      if (!have_superclass) {
	/* only search for a missing superclass after we find
	   a valid class declaration */
	if (!find_library_include(scbuf, FALSE)) {
	  keyword += 5;
	  continue;
	} else {
	  _warning ("ctalk: \"%s\" not defined.\n\n\t%s class %s\n\n",
		    scbuf, scbuf, clbuf);
	}
      }

      push_decl_check (clbuf);
      __xfree (MEMADDR(buf));
      return TRUE;
    }
    
  }

  __xfree (MEMADDR(buf));
  return FALSE;

}

