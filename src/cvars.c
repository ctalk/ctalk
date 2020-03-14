/* $Id: cvars.c,v 1.2 2019/12/11 10:53:54 rkiesling Exp $ */

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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "cvar.h"

/*
 *  Prototype here so we don't have to include stdio.h in everything.
 */
FILE *fopen_tmp (char *, char *);
int fclose_tmp (FILE *);

extern int warn_extension_opt;        /* Declared in main.c.               */
extern I_PASS interpreter_pass;

extern char input_source_file[FILENAME_MAX];

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */
extern int new_method_ptr;

extern CVAR *global_cvars;            /* Declared in rt_cvar.c.            */
extern CVAR *local_cvars;
extern CVAR *typedefs;
extern CVAR *typedefs_ptr;
extern CVAR *externs;
extern CVAR *externs_ptr;

extern int fn_has_argblk;   /* Declared in fnbuf.c. */

HASHTAB declared_global_variables;
HASHTAB declared_typedefs;
HASHTAB declared_functions;
HASHTAB function_params;
static HASHTAB global_struct_member_names;
static HASHTAB enum_hash;
static HASHTAB enum_members;

extern CVAR *struct_members,            /* Declared in cparse.c. */
  *struct_members_ptr;

extern STRUCT_DECL struct_decl[MAXARGS];
extern int struct_decl_ptr;

extern CVAR *fn_param_decls,
  *fn_param_decls_ptr;

static CVAR *incomplete_typedefs,
  *incomplete_typedefs_ptr;

MESSAGE *var_messages[N_VAR_MESSAGES + 1];
int var_messageptr;


void init_cvars (void) {
  var_messageptr = N_VAR_MESSAGES;
  global_cvars = local_cvars = typedefs = typedefs_ptr = 
    externs = externs_ptr = NULL;
  incomplete_typedefs = incomplete_typedefs_ptr = NULL;
  _new_hash (&declared_global_variables);
  _new_hash (&declared_typedefs);
  _new_hash (&declared_functions);
  _new_hash (&enum_hash);
  _new_hash (&function_params);
  _new_hash (&global_struct_member_names);
  _new_hash (&enum_members);
}

int var_message_push (MESSAGE *m) {
  if (var_messageptr == 0) {
    _error ("Var_message_push: stack_overflow.");
    return ERROR;
  }

  if (!m)
    _error ("Var_message_push: null pointer, var_messageptr = %d.", 
	    var_messageptr);

  var_messages[var_messageptr] = m;

#ifdef STACK_TRACE
  debug ("Var_message_push %d. %s.", var_messageptr, 
	 (var_messages[var_messageptr] && 
	  IS_MESSAGE (var_messages[var_messageptr])) ?
	 var_messages[var_messageptr] -> name : "(null)"); 
#endif

  --var_messageptr;

  return var_messageptr + 1;
}

int get_var_messageptr (void) {
  return var_messageptr;
}

MESSAGE_STACK var_message_stack (void) {
  return var_messages;
}

MESSAGE *var_message_stack_at (int idx) {
  return var_messages[idx];
}

int get_params (void) {

  PARSER *p;
  PARAMCVAR param;
  CVAR *c, *t;
  int param_ptr,
    i, n_eff_params;

  param_ptr = get_param_ptr ();

  p = CURRENT_PARSER;

  if (param_0_is_void ()) {
    n_eff_params = param_ptr;
  } else {
    n_eff_params = param_ptr - 1;
  }

  for (i = 0; i <= n_eff_params; i++) {

    _new_cvar (CVARREF(c));

    param = get_param_n (i);

    if (param.decl)
      strcpy (c -> decl, param.decl);
    if (param.type) {
      strcpy (c -> type, param.type);
      /* See the comment in unref_vartab_var (rt_cvar.c). */
      c -> type_attrs |= is_c_data_type_attr (c -> type);
    }
    if (param.qualifier)
      strcpy (c -> qualifier, param.qualifier);
    if (param.qualifier2)
      strcpy (c -> qualifier2, param.qualifier2);
    if (param.storage_class)
      strcpy (c -> storage_class, param.storage_class);
    if (param.name)
      strcpy (c -> name, param.name);
    c -> n_derefs = param.n_derefs;
    c -> is_unsigned = param.is_unsigned;
    c -> scope = param.scope;

    if (!p -> cvars) {
      p -> cvars = c;
    } else {
      for (t = p -> cvars; ; t = t -> next) {
	if (!t -> next) break;
      }
      t -> next = c;
      c -> prev = t;
    }
  }

  return SUCCESS;
}

extern int return_is_unsigned,         /* Declared in cparse.c. */
  return_is_ptr,
  return_is_ptr_ptr,
  decl_is_ptr,
  decl_is_ptr_ptr,
  decl_is_prototype;
extern FN_DECLARATOR declarators[10]; 
extern int n_declarators;
static char default_type_decl[] = "int";

#define DECLARATOR_IS_STORAGE_CLASS(attr) \
  (((attr) & CVAR_TYPE_CONST) || \
   ((attr) & CVAR_TYPE_EXTERN) || \
   ((attr) & CVAR_TYPE_INLINE) || \
   ((attr) & CVAR_TYPE_REGISTER) || \
   ((attr) & CVAR_TYPE_STATIC) || \
   ((attr) & CVAR_TYPE_VOLATILE))

int add_function (MESSAGE *orig) {

  CFUNC *c;
  int i, n_params, param_n;
  PARAMCVAR tmp_cvar;
  CVAR *param_cvar, *param_head = NULL; /* Avoid a warning. */

  if (orig == NULL)
    return ERROR;

  if ((c = (CFUNC *)__xalloc (sizeof (CFUNC))) == NULL)
    _error ("add_function (): %s\n.", strerror (errno));
  
  /*
   *  It should be okay simply to disambiguate a function declaration 
   *  here when the declaration is "long <fn>" or "short <fn>" to 
   *  "long int <fn>" or "short int <fn>".
   */
  if (n_declarators >= 2) {
    if (str_eq (declarators[n_declarators - 2].name, "long") ||
	str_eq (declarators[n_declarators - 2].name, "short")) {
      declarators[n_declarators].name  = 
	declarators[n_declarators - 1].name;
      declarators [n_declarators - 1].name = default_type_decl; /*i.e., "int"*/
      declarators[n_declarators].attr = 
	declarators[n_declarators-1].attr;
      declarators[n_declarators-1].attr = 0;
      ++n_declarators;
    }
  }
  i = n_declarators;
  --i;
  /* Perform some type checking here. 
     TO DO - This should be factored out for vars as well.
   */
  if (!decl_is_prototype &&
      (is_c_data_type (declarators[i].name) ||
       get_typedef (declarators[i].name) ||
       is_c_storage_class (declarators[i].name)))
    warning (orig, "\"%s\" is not a valid function name.", declarators[i].name);
  strcpy (c -> decl, declarators[i].name);

  --i;
  if (i >= 0) {
    if (declarators[i].attr & CVAR_TYPE_UNSIGNED) {
      c -> is_unsigned = TRUE;
      c -> return_type_attrs |= CVAR_TYPE_UNSIGNED;
    } else {
#if 0
      /* Keep this here in case it's needed later - this can
	 probably be checked faster (e.g., only the tag before the
	 argument list must be be unique). */
      if (!IS_DEFINED_LABEL (declarators[i].name) && 
	  !is_incomplete_type (declarators[i].name)) {
	if (is_ctalk_keyword(declarators[i].name)) {
	  warning 
	    (orig, 
	     "Invalid keyword, \"%s,\" in function or method declaration.", 
	     declarators[i].name);
	} else {
	  if (IS_CONSTRUCTOR_LABEL(declarators[i].name)) {
	    warning 
	      (orig, 
	       "Invalid constructor, \"%s,\" in function or method declaration.", 
	       declarators[i].name);
	  } else {
	    warning 
	      (orig, 
	       "Undefined label, \"%s,\" in function or method declaration.", 
	       declarators[i].name);
	  }
	}
      }
#endif      
      strcpy (c -> return_type, declarators[i].name);
      c -> return_type_attrs |= declarators[i].attr;
    }
  }
  
  --i;
  if (i >= 0) {
    if (declarators[i].attr & CVAR_TYPE_UNSIGNED) {
      c -> is_unsigned = TRUE;
      c -> return_type_attrs |= CVAR_TYPE_UNSIGNED;
    } else {
      if (DECLARATOR_IS_STORAGE_CLASS(declarators[i].attr)) {
	strcpy (c -> storage_class, declarators[i].name);
      } else {
	strcpy (c -> qualifier_type, declarators[i].name);
      }
      c -> return_type_attrs |= declarators[i].attr;
    }
  }

  --i;
  if (i >= 0) {
    if (declarators[i].attr & CVAR_TYPE_UNSIGNED) {
      c -> is_unsigned = TRUE;
      c -> return_type_attrs |= CVAR_TYPE_UNSIGNED;
    } else {
      if (DECLARATOR_IS_STORAGE_CLASS(declarators[i].attr)) {
	strcpy (c -> storage_class, declarators[i].name);
      } else {
	strcpy (c -> qualifier2_type, declarators[i].name);
      }
      c -> return_type_attrs |= declarators[i].attr;
    }
  }

  --i;
  if (i >= 0) {
    if (declarators[i].attr & CVAR_TYPE_UNSIGNED) {
      c -> is_unsigned = TRUE;
      c -> return_type_attrs |= CVAR_TYPE_UNSIGNED;
    } else {
      if (!DECLARATOR_IS_STORAGE_CLASS(declarators[i].attr))
	warning (orig, "Unknown storage class, \"%s.\"", declarators[i].name);
      strcpy (c -> storage_class, declarators[i].name);
      c -> return_type_attrs |= declarators[i].attr;
    }
  }

  if ((c -> return_type_attrs & CVAR_TYPE_LONG) &&
      *(c -> qualifier2_type)) {
    /* both "long" declarators each get the CVAR_TYPE_LONG
       attribute.  If we've filled in the second qualifier,
       the declaration is a long long int, so add the
       attibute here. */
    c -> return_type_attrs  |= CVAR_TYPE_LONGLONG;
    c -> return_type_attrs &= ~CVAR_TYPE_INT;
    c -> return_type_attrs &= ~CVAR_TYPE_LONG;
  }

  c -> is_ptr_decl = decl_is_ptr;
  c -> is_prototype_decl = decl_is_prototype;
  c -> return_derefs = ((return_is_ptr) ? 1 : ((return_is_ptr_ptr) ? 2 : 0));

  n_params = get_param_ptr ();
  for (param_n = 0; param_n < n_params; param_n++) {
    _new_cvar (CVARREF(param_cvar));
    tmp_cvar = get_param_n (param_n);
    if (tmp_cvar.type)
      strcpy (param_cvar -> type, tmp_cvar.type);
    if (tmp_cvar.qualifier)
      strcpy (param_cvar -> qualifier, tmp_cvar.qualifier);
    if (tmp_cvar.qualifier2)
      strcpy (param_cvar -> qualifier2, tmp_cvar.qualifier2);
    if (tmp_cvar.qualifier3)
      strcpy (param_cvar -> qualifier3, tmp_cvar.qualifier3);
    if (tmp_cvar.qualifier4)
      strcpy (param_cvar -> qualifier4, tmp_cvar.qualifier4);
    if (tmp_cvar.storage_class)
      strcpy (param_cvar -> storage_class, tmp_cvar.storage_class);
    if (tmp_cvar.name)
      strcpy (param_cvar -> name, tmp_cvar.name);
    param_cvar -> n_derefs = tmp_cvar.n_derefs;
    param_cvar -> initializer_size = tmp_cvar.initializer_size;
    param_cvar -> attrs = tmp_cvar.attrs;
    if ((param_cvar -> type_attrs = tmp_cvar.type_attrs) == 0) {
      if (tmp_cvar.type) {
	param_cvar -> type_attrs = is_c_data_type_attr (tmp_cvar.type);
      }
      if (param_cvar -> type_attrs == CVAR_TYPE_INT) {
	/* Check for long and longlong qualifiers. */
	if (tmp_cvar.qualifier2) {
	  param_cvar -> type_attrs = CVAR_TYPE_LONGLONG;
	} else if (tmp_cvar.qualifier) {
	  param_cvar -> type_attrs = CVAR_TYPE_LONG;
	}
      }
    }
    param_cvar -> is_unsigned = tmp_cvar.is_unsigned;
    param_cvar -> scope = tmp_cvar.scope;

    _hash_put (function_params, param_cvar, param_cvar -> name);
    if (c -> params == NULL) {
      c -> params = param_head = param_cvar;
    } else {
      param_head -> next = param_cvar;
      param_cvar -> prev = param_head;
      param_head = param_cvar;
    }
  }

  _hash_put (declared_functions, (void *)c, c -> decl);

  return SUCCESS;

}

int var_cmp2 (CVAR *c1, CVAR *c2) {

  if (!IS_CVAR(c1) || !IS_CVAR(c2))
    return ERROR;

  if (!memcmp ((void *)c1, (void *)c2, offsetof (CVAR, members)))
    return SUCCESS;

  /* 
   *  If the names match, make sure that the redefinition isn't
   *  actually a type tag before issuing a warning, and we
   *  also aren't issuing warnings for aggregate type members. 
   *
   *  C99 says that tags of structs, unions, and enums, have
   *  a different name space than other identifiers.  If only
   *  one of the attributes specifies a tag, then add the
   *  identifier with the duplicate name.
   */
  if (str_eq (c1 -> name, c2 -> name)) {

    if ((c1 -> attrs & CVAR_ATTR_STRUCT_DECL) &&
	(c2 -> attrs & CVAR_ATTR_STRUCT_DECL))
      return SUCCESS;
    else
      return ERROR;
  }

  return ERROR;
}

void delete_local_c_vars (void) {

  PARSER *p;
  CVAR *c, *c_prev;

  p = CURRENT_PARSER;

  if (!p -> cvars)
    return;

  for (c = p -> cvars; c -> next; c = c -> next)
    ;
  
  while (c != local_cvars) {
    c_prev = c -> prev;
    _delete_cvar (c);
    c = c_prev;
  }
  _delete_cvar (c);
  p -> cvars = NULL;
  
}

int add_variable_from_cvar (CVAR *c) {

  int global_frame;
  PARSER *p;
  CVAR *t, *c1, *d, *d1;

  if (c -> attrs & CVAR_ATTR_ENUM) {
    for (t = c -> members; t; t = t -> next) {
      _hash_put (enum_members, t, t -> name);
    }
  }

  if (((global_frame = is_global_frame ()) == FALSE) && /* Local frame */
      (interpreter_pass != var_pass)) { /* lib and scope isn't global.*/
#if 0 /***/
    if (((global_frame = is_global_frame ()) == FALSE) && /* Local frame */
      ((interpreter_pass != var_pass) &&      /* If recursing into class */
       (interpreter_pass != library_pass))) { /* lib and scope isn't global.*/
#endif      
    /*
     *  Add the variable to the parser's variable list.
     */
    p = CURRENT_PARSER;

    if (!p -> cvars) {
      p -> cvars = c;
    } else {
      for (t = p -> cvars; ; t = t -> next) {
 	if (!var_cmp2 (t, c)) {
 	  _delete_cvar (c);
 	  return ERROR;
 	}
	if (!t -> next) break;
      }
      t -> next = c;
      c -> prev = t;
    }

  } else {  /* Global frame (or no frame). */


    if (!global_cvars) {
      global_cvars = c;
      for (c1 = c; c1; c1 = c1 -> next) {
	if (c1 -> attrs & CVAR_ATTR_ENUM) {
	  _hash_put (enum_hash, (void *)c1, c1 -> name);
	} else {
	  _hash_put (declared_global_variables, (void *)c1, c1 -> name);
	}
      }
    } else {
      /* do the hash first because it doesn't add links to
	 previously added cvars. */
      for (c1 = c; c1; c1 = c1 -> next) {
	if (c1 -> attrs & CVAR_ATTR_ENUM) {
	  _hash_put (enum_hash, (void *)c1, c1 -> name);
	} else {
	  _hash_put (declared_global_variables, (void *)c1, c1 -> name);
	}
      }
      /* add the new cvars to the front of the global var list. */
      d = c;
      for (d1 = c; IS_CVAR(d1 -> next); d1 = d1 -> next)
	;
      global_cvars -> prev = d1;
      d1 -> next = global_cvars;
      global_cvars = d;
    }
  }
  return SUCCESS;
}

/*
 * Any typedef that isn't completed by the end of the statement
 * goes here, then is resolved at the end of the variable pass.
 */
int add_incomplete_type (MESSAGE_STACK messages,int keyword_ptr,int type_ptr) {

  CVAR *c;
  int i, stack_end;
  MESSAGE *m;
  char *type_ptr_1 = "";
  char tag_buf[12][MAXLABEL] = {{""},};
  int tag_buf_idx;
  enum {
    incomplete_typedef_null,
    incomplete_typedef_keyword,
    incomplete_typedef_type,
    incomplete_typedef_tag
  } incomplete_typedef_state;

  stack_end = get_stack_top (messages);

  /* *type_buf = 0; */
  tag_buf_idx = 0;
  for (i = keyword_ptr, incomplete_typedef_state = incomplete_typedef_null; 
       i > stack_end; --i) {

    m = messages[i];

    if (M_ISSPACE(m)) continue;

    switch (M_TOK(m))
      {
      case LABEL:
      case CTYPE:
	switch (incomplete_typedef_state)
	  {
	  case incomplete_typedef_null:
	    if (!str_eq (M_NAME(m), "typedef")) {
	      warning (m, "Error in typedef statement.");
	      return ERROR;
	    }
	    incomplete_typedef_state = incomplete_typedef_keyword;
	    break;
	  case incomplete_typedef_keyword:
	    type_ptr_1 = M_NAME(m);
	    incomplete_typedef_state = incomplete_typedef_type;
	    break;
	  case incomplete_typedef_type:
	    strcpy (tag_buf[tag_buf_idx++], M_NAME(m));
	    incomplete_typedef_state = incomplete_typedef_tag;
	    break;
	  case incomplete_typedef_tag:
#ifdef __GNUC__
	    if (is_gnu_extension_keyword (M_NAME(m))) {
	      int i2, attr_n_parens;
	      for (i2 = i, attr_n_parens = 0; i2 > stack_end; i2--) {
		switch (messages[i2]->tokentype)
		  {
		  case OPENPAREN:
		    ++attr_n_parens;
		    break;
		  case CLOSEPAREN:
		    if (--attr_n_parens == 0) {
		      i = i2;
		      continue;
		    }
		    break;
		  }
	      }
	    } else {
#endif
	      strcpy (tag_buf[tag_buf_idx++], M_NAME(m));
#ifdef __GNUC__
	    }
#endif
	  }
	break;
      case SEMICOLON:
	if (incomplete_typedef_state != incomplete_typedef_tag) {
	  warning (m, "Error in typedef statement.");
	  return ERROR;
	}
	_new_cvar (CVARREF(c));
	strcpy (c -> storage_class, "typedef");
	c -> attrs = CVAR_ATTR_TYPEDEF_INCOMPLETE;
	strcpy (c -> type, type_ptr_1);
	strcpy (c -> name, tag_buf[--tag_buf_idx]);
	while (tag_buf_idx > 0) {
	  switch (--tag_buf_idx)
	    {
	    case 0:
	      strcpy (c -> qualifier, tag_buf[tag_buf_idx]);
	      break;
	    case 1:
	      strcpy (c -> qualifier2, tag_buf[tag_buf_idx]);
	      break;
	    case 2:
	      strcpy (c -> qualifier3, tag_buf[tag_buf_idx]);
	      break;
	    case 3:
	      strcpy (c -> qualifier4, tag_buf[tag_buf_idx]);
	      break;
	    default:
	      _warning ("add_incomplete_type: Too many qualifiers in typedef %s\n", c -> name);
	      break;
	    }
	}
	if (!incomplete_typedefs_ptr) {
	  incomplete_typedefs = incomplete_typedefs_ptr = c;
	} else {
	  incomplete_typedefs_ptr -> next = c;
	  c -> prev = incomplete_typedefs_ptr;
	  incomplete_typedefs_ptr = c;
	}
	return SUCCESS;
	break;
      }
  }

  return SUCCESS;
}

int is_incomplete_type (char *s) {
  CVAR *t;
  for (t = incomplete_typedefs; t; t = t -> next) {
    if (str_eq (s, t -> name))
      return TRUE;
  }
  return FALSE;
}

static void __delete_incomplete_typedef (CVAR *t) {
  CVAR *tmp;
  if (t == incomplete_typedefs) {
    incomplete_typedefs = incomplete_typedefs->next;
    if (incomplete_typedefs) incomplete_typedefs -> prev = NULL;
  } 
  if (t == incomplete_typedefs_ptr) {
    incomplete_typedefs_ptr = incomplete_typedefs_ptr -> prev;
    if (incomplete_typedefs_ptr) { 
      incomplete_typedefs_ptr -> next = NULL;
    }
  }
  tmp = t;
  if (t -> next) t -> next -> prev = t -> prev;
  if (t -> prev) t -> prev -> next = t -> next;
  _delete_cvar (tmp);
}

/*
 *  Called at the end of the var pass. 
 *  TO DO - Does not have an incomplete typedef from
 *  another typedef.
 */
int resolve_incomplete_types (void) {

  CVAR *t, *n, *p, *var, *mbr, *mbr_ptr = NULL;

  for (t = incomplete_typedefs; t;) {
    /*
     *  SunOS says that typedef <type> <type> is valid, 
     *  even if incomplete.
     */
    if (str_eq (t -> name, t -> type) ||
	str_eq (t -> name, t -> qualifier)) {
      CVAR *tmp;
      t -> attrs |= CVAR_ATTR_TYPEDEF;
      if (t == incomplete_typedefs) 
	incomplete_typedefs = incomplete_typedefs->next;
      tmp = t;
      if (t -> next) t -> next -> prev = t -> prev;
      if (t -> prev) t -> prev -> next = t -> next;
      t = t -> next;
      tmp -> next = tmp -> prev = NULL;
      add_typedef_from_cvar (tmp);
    } else {
      if (((var = get_global_var (t -> name)) != NULL) ||
	  ((var = get_global_var (t -> qualifier)) != NULL)) {
	n= __ctalkCopyCVariable (t);
	for (p = var -> members; p; p = p -> next) {
	  mbr = __ctalkCopyCVariable (p);
	  _hash_put (global_struct_member_names, mbr, mbr -> name); 
	  if (! n -> members) {
	    n -> members = mbr_ptr = mbr;
	  } else {
	    mbr_ptr -> next = mbr;
	    mbr -> prev = mbr_ptr;
	    mbr_ptr = mbr_ptr -> next;
	  }
	}
	n -> attrs |= CVAR_ATTR_TYPEDEF;
	add_typedef_from_cvar (n);
	if (t -> next) {
	  t = t -> next;
	  __delete_incomplete_typedef (t->prev);
	} else {
	  __delete_incomplete_typedef (t);
	  goto resolve_incomplete_typedefs_done;
	}
      } else {
	t = t -> next;
      } 
    }
  }

 resolve_incomplete_typedefs_done:

  /*
   *  Add any still-unresolved typedefs to the declared global
   *  variables hash so we can look them up quickly later
   *  on.
   */
  for (t = incomplete_typedefs; t; t = t -> next) {
    _hash_put (declared_global_variables, (void *)t, t -> name);
  }

  return SUCCESS;
}

int add_typedef_from_cvar (CVAR *c) {

  CVAR *t;

  for (t = c; t; t = t -> next) {
    t -> attrs |= CVAR_ATTR_TYPEDEF;
    generate_register_typedef_call (t);
    _hash_put (declared_typedefs, (void *)t, t -> name);
    if (t -> attrs & CVAR_ATTR_ENUM)
      _hash_put (enum_hash, (void *)t, t -> name);
    /*
     *  Enums with different types 
     *  (c -> type != enum ? c -> qualifier == enum) and 
     *  tag (c -> name) can be registered with both the type 
     *  and tag so far.
     */
    if ((t -> attrs & CVAR_ATTR_ENUM) &&
 	!str_eq (c -> type, "enum")) {
      _hash_put (declared_typedefs, (void *)t, t -> type);
      _hash_put (enum_hash, (void *)t, t -> type);
    }
  }

  if (!typedefs) {
    typedefs = typedefs_ptr = c;
  } else {
    typedefs_ptr -> next = c;
    c -> prev = typedefs_ptr;
    typedefs_ptr = c;
  }

  /*
   *  If there's a list of cvars, set the list pointer
   *  to the last node.
   */
  while (typedefs_ptr -> next)
    typedefs_ptr = typedefs_ptr -> next;
  
  struct_def_from_typedef (c);

  return SUCCESS;
}

extern VARNAME vars[MAXARGS];  /* declared in cparse.c. */
extern int n_vars;

int add_typedef_from_parser (void) {

  CVAR *c, *t;

  if (get_typedef (vars[n_vars-1].name)) {
    _warning ("Redefinition of typedef \"%s.\"\n",
	      vars[n_vars-1].name);
    return ERROR;
  }

  if ((c = parser_to_cvars ()) != NULL) {

    for (t = c; t; t = t -> next) {
      t -> attrs |= CVAR_ATTR_TYPEDEF;
      generate_register_typedef_call (t);
      _hash_put (declared_typedefs, (void *)t, t -> name);
      if (t -> attrs & CVAR_ATTR_ENUM)
	_hash_put (enum_hash, (void *)t, t -> name);
    }

    if (!typedefs) {
       typedefs = typedefs_ptr = c;
     } else {
       typedefs_ptr -> next = c;
       c -> prev = typedefs_ptr;
       typedefs_ptr = c;
     }
   while (typedefs_ptr -> next)
     typedefs_ptr = typedefs_ptr -> next;

   struct_def_from_typedef (c);

  }
  return SUCCESS;
}

/*
 *  If we encounter a typedef like the following:
 * 
 *  typedef struct _s_type {
 *    ...
 *  } s_tag;
 *
 *  Then add a global struct definition as:
 *
 *  struct {
 *    ...
 *  } _s_type;
 *
 *  NOTE - C99 says that typedefs share the same namespace
 *  as variables.  So in the future if we add attributes
 *  to the CVAR type that specifies a typedef, we can
 *  simply check for a struct definition using the type
 *  or qualifier of the typedef instead of adding the
 *  struct definition separately using this function.
 *
 *  NOTE2 - This function only toggles the TYPEDEF
 *  attribute, so it can only be called with a 
 *  typedef declaration for the attributes of the
 *  definition to be set correctly
 */
int struct_def_from_typedef (CVAR *c) {
  CVAR *n, *n_mbr, *dup_member, *n_mbr_ptr = NULL;   /* Avoid a warning. */

  if ((str_eq (c -> qualifier, "struct") ||
       str_eq (c -> qualifier, "union")) &&
      (*c -> type != 0) &&
      c -> members) {
    _new_cvar (CVARREF(n));
    strcpy (n -> name, c -> type);
    strcpy (n -> type, c -> qualifier);
    /* In case of nested structs. */
    _hash_put (global_struct_member_names, n, n -> name);
    for (n_mbr = c -> members; n_mbr; n_mbr = n_mbr -> next) {
      dup_member = __ctalkCopyCVariable (n_mbr);
      _hash_put (global_struct_member_names, dup_member, dup_member -> name);
      if (!n -> members) {
	n -> members = n_mbr_ptr = dup_member;
      } else {
	n_mbr_ptr -> next = dup_member;
	dup_member -> prev = n_mbr_ptr;
	n_mbr_ptr = dup_member;
      }
    }
    n -> attrs = c -> attrs ^ CVAR_ATTR_TYPEDEF;
    add_variable_from_cvar (n);
  }
  return SUCCESS;
}

static CVAR *copy_anonymous_struct_members (CVAR *mbrs) {
  /* Avoid warnings. */
  CVAR *c, *c_2 = NULL, *mbrs_copy = NULL;
  for (c = mbrs; c; c = c -> next) {
    if (!mbrs_copy) {
      _new_cvar (&mbrs_copy);
      c_2 = mbrs_copy;
    } else {
      _new_cvar (&(c_2 -> next));
      c_2 = c_2 -> next;
    }
    strcpy (c_2 -> name, c -> name);
    if (*c -> type) strcpy (c_2 -> type, c -> type);
    if (*c -> qualifier) strcpy (c_2 -> qualifier, c -> qualifier);
    if (*c -> qualifier2) strcpy (c_2 -> qualifier2, c -> qualifier2);
    if (*c -> qualifier3) strcpy (c_2 -> qualifier3, c -> qualifier3);
    if (*c -> qualifier4) strcpy (c_2 -> qualifier4, c -> qualifier4);
    if (*c -> storage_class) 
      strcpy (c_2 -> storage_class, c -> storage_class);
    if (c -> members)
      c_2 -> members = copy_anonymous_struct_members (c -> members);
    c_2 -> n_derefs = c -> n_derefs;
    c_2 -> initializer_size = c -> initializer_size;
    c_2 -> attrs = c -> attrs;
    c_2 -> type_attrs = c -> type_attrs;
    c_2 -> is_unsigned = c -> is_unsigned;
    c_2 -> scope = c -> scope;
    _hash_put (global_struct_member_names, c_2, c_2 -> name);
  }
  return mbrs_copy;
}

extern bool is_unsigned;    /* declared in cparse.c. */
extern char *storage_class;
extern int type_attrs;
extern char *type_decls[MAX_DECLARATORS+1];
extern int type_decls_ptr;

static int fp_container_id = 0;

/*
 *  Return a list of one or more cvars after calling
 *  is_c_var_declaration_msg ().
 */

/* the maximum length of a the method selector component of a container
   variable - the fn truncates the selector name if necessary */
#define C_CONT_SELECT_LENGTH (MAXLABEL - 10)

CVAR *parser_to_cvars (void) {

  CVAR *c, *l_ptr, *mbr_ptr, *c_cont;
  static CVAR *l;
  char c_cont_select_buf[C_CONT_SELECT_LENGTH];
  int i, j,
    n_type_decls;
  int initial_tag_attrs = 0;

  n_type_decls = type_decls_ptr;

  for (i = 0, l = l_ptr = NULL; i < n_vars; i++) {

    _new_cvar (CVARREF(c));

    strcpy (c -> name, vars[i].name);
    c -> attrs = vars[i].attrs;

    strcpy (c -> type, type_decls[n_type_decls - 1]);
    for (j = 0; j <= n_type_decls - 2; j++) {
      if (j > 4)
	_warning ("Too many type qualifiers.\n");
      switch ((n_type_decls - 2) - j)
	{
 	case 0:
	  strcpy (c -> qualifier, type_decls[j]);
 	  break;
 	case 1:
	  strcpy (c -> qualifier2, type_decls[j]);
 	  break;
 	case 2:
	  strcpy (c -> qualifier3, type_decls[j]);
 	  break;
 	case 3:
	  strcpy (c -> qualifier3, type_decls[j]);
 	  break;
	}
    }

    if (interpreter_pass != var_pass)
      c -> scope = frame_at (parsers[current_parser_ptr] -> frame) -> scope;
    c -> is_unsigned = is_unsigned;
    if (storage_class)
      strcpy (c -> storage_class, storage_class);
    c -> type_attrs = type_attrs;

    c -> n_derefs = vars[i].n_derefs;
    if (c -> attrs & CVAR_ATTR_ARRAY_DECL)
      c -> initializer_size = vars[i].array_initializer_size;
    /*
     *  We have to be careful here, because the C variable 
     *  parser doesn't know the difference between a 
     *  declaration like "struct _s {... }s;" and 
     *  "struct _s s;"  In cparse.c, we only store struct
     *  members in the struct_decl[] stack, and then
     *  set the global struct_members to the member cvars
     *  when returning from parsing an inner struct.
     *  This also makes it difficult, because every time
     *  we call is_c_var_declaration_msg (), we have to
     *  check additionally for a nested struct when 
     *  retrieving the cvar.
     */
    if (VAR_IS_STRUCT (c)) {
      c -> members = struct_members;
      for (mbr_ptr = c -> members; mbr_ptr; mbr_ptr = mbr_ptr -> next) {
	mbr_ptr -> scope = c -> scope;
	_hash_put (global_struct_member_names, mbr_ptr, mbr_ptr -> name);
      }
      /* In case the struct is nested, add its tag. */
      _hash_put (global_struct_member_names, c, c -> name);
      if ((n_type_decls == 1) && (n_vars > 1)) {
	/*
	 *  If we encounter an anonymous struct declaration with 
	 *  multiple tags, like this:
	 *
	 *  struct {
	 *    ....
	 *  } s_struct, *s_ptr;
	 *
	 *  Then each tag's CVAR simply inherits the attributes of the
	 *  first tag and its own copy of the struct members.  
	 */
	if (i == 0) {
	  initial_tag_attrs = c -> attrs;
	} else {
	  c -> attrs = initial_tag_attrs;
	}
	if (i < (n_vars - 1)) {
	  struct_members = copy_anonymous_struct_members (c -> members);
	  /* In case we have a nested struct tag. */
	  _hash_put (global_struct_member_names, c, c -> name);
	} else {
	  struct_members = NULL;
	}
      } else {
	struct_members = NULL;
      }
    } else if ((c -> attrs & CVAR_ATTR_FN_DECL) ||
	       (c -> attrs & CVAR_ATTR_FN_PTR_DECL) ||
	       (c -> attrs & CVAR_ATTR_FN_PROTOTYPE)) {
      if (!(c -> attrs & CVAR_ATTR_FN_NO_PARAMS)) {
	c -> params = fn_param_decls;
	fn_param_decls = fn_param_decls_ptr = NULL;
      }
    } else if (c -> scope == LOCAL_VAR &&
	       ((c -> type_attrs & CVAR_TYPE_FLOAT) ||
		(c -> type_attrs & CVAR_TYPE_DOUBLE) ||
		(c -> type_attrs & CVAR_TYPE_LONGDOUBLE)) &&
	       c -> n_derefs > 0 &&
	       c -> initializer_size > 0) {
      /* container struct for a float or double array... */
      _new_cvar (CVARREF(c_cont));
      /* ... which doesn't work with long doubles. */
      if (c -> type_attrs & CVAR_TYPE_LONG) {
	warning (message_stack_at
		 (frame_at (parsers[current_parser_ptr] -> frame)
		  -> message_frame_top),
		 "Compiler does not support long double aliasing for "
		 "argument blocks.  Changing type of array, \"%s,\" "
		 "to, \"double.\"",
		 c -> name);
	c -> type_attrs &= ~CVAR_TYPE_LONG;
	c -> qualifier[0] = '\0';
      }
      if (interpreter_pass == method_pass) {
	if (new_methods[new_method_ptr+1] -> method -> attrs &
	    METHOD_CONTAINS_ARGBLK_ATTR) {
	  memset (c_cont_select_buf, 0, C_CONT_SELECT_LENGTH);
	  strncpy (c_cont_select_buf,
		   new_methods[new_method_ptr+1] -> method -> selector,
		   C_CONT_SELECT_LENGTH);
	  sprintf (c_cont -> type, "_%s_%d_l",
		   c_cont_select_buf,
		   fp_container_id++);
	  /* type is the name with leading underscore */
	  strcpy (c_cont -> name, &c_cont -> type[1]);
	  strcpy (c_cont -> qualifier, "struct");
	  c_cont -> members = c;
	  c = c_cont;
	  c -> scope = c -> members -> scope;
	  c -> attrs = CVAR_ATTR_STRUCT|CVAR_ATTR_STRUCT_DECL|
	    CVAR_ATTR_FP_ARGBLK;
	}
      } else if (interpreter_pass == parsing_pass) {
	if (fn_has_argblk) {
	  sprintf (c_cont -> type, "_%s_%d_l",
		   get_fn_name (), fp_container_id++);
	  strcpy (c_cont -> name, &c_cont -> type[1]);
	  strcpy (c_cont -> qualifier, "struct");
	  c_cont -> members = c;
	  c = c_cont;
	  c -> scope = c -> members -> scope;
	  c -> attrs = CVAR_ATTR_STRUCT|CVAR_ATTR_STRUCT_DECL|
	    CVAR_ATTR_FP_ARGBLK;
	}
      }
    }

    if (!l) {
      l = l_ptr = c;
    } else {
      l_ptr -> next = c;
      c -> prev = l_ptr;
      l_ptr = c;
    }

  }   /* for (i = 0, l = l_ptr = NULL; i < n_vars; i++) */

  return l;
}

/* 
 *  TO DO - Long longs, doubles, and long doubles do not
 *  align with ints and longs.  Test for promoting them.
 *
 *  Handle doubles and long doubles.
 */

int match_type (VAL *val1, VAL *val2) {

  int retval = ERROR;

  if (val1 -> __type == val2 -> __type) {
    retval = SUCCESS;
  } else {

    switch (val1 -> __type) 
      {
      case INTEGER_T:
	switch (val2 -> __type)
	  {
	  case LONG_T:
	    val1 -> __type = LONG_T;
	    retval = SUCCESS;
	    break;
	  case LONGLONG_T:
	    val1 -> __type = LONGLONG_T;
	    val1 -> __value.__ll = (long long) val1 -> __value.__i;
	    retval = SUCCESS;
	    break;
	  }
	break;
      case LONG_T:
	switch (val2 -> __type)
	  {
	  case INTEGER_T:
	    val2 -> __type = LONG_T;
	    retval = SUCCESS;
	    break;
	  case LONGLONG_T:
	    val1 -> __type = LONGLONG_T;
	    val1 -> __value.__ll = (long long) val1 -> __value.__l;
	    retval = SUCCESS;
	    break;
	  }
	break;
      case LONGLONG_T:
	switch (val2 -> __type)
	  {
	  case INTEGER_T:
	    val2 -> __type = LONGLONG_T;
	    val2 -> __value.__ll = (long long) val2 -> __value.__i;
	    retval = SUCCESS;
	    break;
	  case LONG_T:
	    val2 -> __type = LONGLONG_T;
	    val2 -> __value.__ll = (long long) val2 -> __value.__l;
	    retval = SUCCESS;
	    break;
	  }
	break;
      default:
	_warning ("Unimplemented type %d in match_type.", val1 -> __type);
	break;
      }
  }
  

  return retval;
}

/*
 *  Note that structs that are declared with a type and a tag
 *  have the attributes CVAR_ATTR_STRUCT|CVAR_ATTR_STRUCT_DECL.
 *  If that changes in the future, the function may need to 
 *  check each attribute separately.
 */

#define CVAR_STRUCT_TYPE_TAG_ATTR (CVAR_ATTR_STRUCT|CVAR_ATTR_STRUCT_DECL)

CVAR *get_local_var (char *s) {

  PARSER *p;
  CVAR *c;

  if ((interpreter_pass == preprocessing_pass) ||
      (interpreter_pass == var_pass))
    return (CVAR *)NULL;

  if ((interpreter_pass == c_fn_pass) ||
      (interpreter_pass == expr_check)) {
    int parser_lvl;
    for (parser_lvl = parser_ptr () + 1; parser_lvl <= 512; parser_lvl++) {
      p = parser_at (parser_lvl);
      for (c = p -> cvars; c; c = c -> next) {
	if (str_eq (s, c -> name)) {
	  if (c -> scope == 0)
	    c -> scope = LOCAL_VAR;
	  return c;
	}
      }
    }
    return NULL;
  } else {
    p = CURRENT_PARSER;
  }

  if (!p -> cvars)
    return NULL;

  for (c = p -> cvars; c; c = c -> next) {
    if (str_eq (s, c -> name)) {
      if (c -> scope == 0)
	c -> scope = LOCAL_VAR;
      return c;
    }
  }

  return NULL;
}

CVAR *get_local_struct_defn (char *s) {

  PARSER *p;
  CVAR *c;

  if ((interpreter_pass == preprocessing_pass) ||
      (interpreter_pass == var_pass))
    return (CVAR *)NULL;

  if ((interpreter_pass == c_fn_pass) ||
      (interpreter_pass == expr_check)) {
    int parser_lvl;
    for (parser_lvl = parser_ptr () + 1; parser_lvl <= 512; parser_lvl++) {
      p = parser_at (parser_lvl);
      for (c = p -> cvars; c; c = c -> next) {
	if (c -> attrs & CVAR_ATTR_STRUCT_DECL) {
	  if ((str_eq (s, c -> type) || !strcmp (s, c -> name)) &&
	      c -> name)
	    return c;
	}
      }
    }
    return NULL;
  } else {
    p = CURRENT_PARSER;
  }

  if (!p -> cvars)
    return NULL;

  for (c = p -> cvars; c; c = c -> next) {
    if (c -> attrs & CVAR_ATTR_STRUCT_DECL) {
      if ((str_eq (s, c -> type) || str_eq (s, c -> name)) &&
	  c -> members)
	return c;
    }
  }

  return NULL;
}

/*
 *  TO DO - Check for members of declarations without tags; e.g., 
 *  enums.
 */

CVAR *get_global_var (char *s) {
  CVAR *c;
  if (!s || *s == '\0')
    return NULL;

  if ((c = (CVAR *)_hash_get (declared_global_variables, s)) != NULL){
    if (c -> scope == 0)
      c -> scope = GLOBAL_VAR;
    return c;
  }

  return NULL;
}

CVAR *get_global_struct_defn (char *s) {
  CVAR *c;

  for (c = global_cvars; c; c = c -> next) {
    if (c -> attrs & CVAR_ATTR_STRUCT_DECL) {
      if ((str_eq (s, c -> name) ||
	   str_eq (s, c -> type)) &&
	  c -> members)
 	return c;
    }
  }

  return NULL;
}

int global_var_is_declared (char *name) {

  if (_hash_get (declared_global_variables, name))
    return TRUE;
  else
    return FALSE;

}

CFUNC *get_function (char *s) {
  CFUNC *cfn;
  if ((cfn = (CFUNC *)_hash_get (declared_functions, s)) != NULL)
    return cfn;
  return NULL;
}

/*
 *   This shouldn't be called until after the var pass
 *   because it doesn't handle forward declarations.
 */
static int get_derived_type_attrs (CVAR *c) {
  int i;
  CVAR *c_1, *c_basic;

  c_basic = c;
  for (i = 0; i < 10; i++) {

    if (get_type_conv_name (c_basic -> type))
      return c_basic -> type_attrs;

    for (c_1 = typedefs; c_1; c_1 = c_1 -> next) {
      if (str_eq (c_1 -> name, c_basic -> type)) {
	if (c_basic -> type_attrs) {
	  return c_basic -> type_attrs;
	} else {
	c_basic = c_1;
	}
      }
    }
  }

  return 0;
}

CVAR *get_typedef (char *s) {
  CVAR *c;
  if ((c = (CVAR *)_hash_get (declared_typedefs, s)) != NULL) {
    if (c -> type_attrs == 0) {
      c -> type_attrs = get_derived_type_attrs (c);
    }
    return c;
  } else {
    for (c = incomplete_typedefs; c; c = c -> next) {
      if (str_eq (s, c -> name)) {
	if (c -> type_attrs == 0) {
	  c -> type_attrs = get_derived_type_attrs (c);
	}
	return c;
      }
    }
  }
  return NULL;
}

int typedef_is_declared (char *name) {
  return ((get_typedef (name)) ? TRUE : FALSE);
}

int is_c_derived_type (char *s) {

  if (_hash_get (declared_typedefs, s))
    return TRUE;
  else
    return FALSE;
}

void dump_typedefs (MESSAGE *orig) {
  CVAR *c;
  char buf[MAXLABEL + 8], /* strlen ("typedef ") */
    buf1[MAXMSG], buf2[MAXMSG];
  int i;

  _hash_all_initialize ();
  while ((c = (CVAR *)_hash_all (declared_typedefs)) != NULL) {
    strcpy (buf1, "typedef ");
    if (c -> is_unsigned)
      strcatx (buf, buf1, " ", "unsigned", NULL);
    if (*c -> qualifier2)
      strcatx (buf, buf1, " ", c -> qualifier2, NULL);
     if (*c -> qualifier)
       strcatx (buf, buf1, " ", c -> qualifier, NULL);
     if (*c -> type)
       strcatx (buf, buf1, " ", c -> type, NULL);
     for (i = 0; i < c -> n_derefs; ++i) {
       /* strcatx can't handle overlapping buffers yet. */
       sprintf (buf2, "%s %s", buf, "*");
     }
     if (*c -> name)
       strcatx (buf, buf1, " ", c -> name, NULL);
     warning (orig, "%s.", buf);
  }
}

static int old_input_size = 0;
static char *input = NULL;

int parse_vars (char *tmpname) {

  int i, end_ptr;
  MESSAGE *m;
  char tmp_src_file[FILENAME_MAX];
  CVAR *c;
  struct stat statbuf;

  strcpy (tmp_src_file, input_source_file);

  if (stat (tmpname, &statbuf))
      _error ("%s: %s.", tmpname, strerror (errno));

  if (statbuf.st_size > old_input_size) {
    old_input_size = statbuf.st_size + 1;
    if ((input = (char *)realloc (input, statbuf.st_size + 1)) == NULL)
      _error ("parse_vars: %s.", strerror (errno));
  }
  memset ((void *)input, 0, old_input_size);

  read_file (input, tmpname);

  end_ptr = tokenize_no_error (var_message_push, input);

  for (i = N_VAR_MESSAGES; i >= end_ptr; i--) {

    if (M_TOK(var_messages[i]) != LABEL)
      continue;
      

    m = var_messages[i];

    /* 
       This should probably stay for a while.
    */
    /* #define DEBUG_VAR_LABEL
       #define VAR_LABEL "__ctalkX11UseFontBasic" */

#ifdef DEBUG_VAR_LABEL
    if ((var_messages[i] && 
	 str_eq (M_NAME(var_messages[i]), VAR_LABEL)) ||
	(var_messages[i-1] && 
	 str_eq (M_NAME(var_messages[i-1]), VAR_LABEL)) ||	
	(var_messages[i-2] && 
	 str_eq (M_NAME(var_messages[i-2]), VAR_LABEL)) ||	
	(var_messages[i-3] && 
	 str_eq (M_NAME(var_messages[i-3]), VAR_LABEL)) ||
	(var_messages[i-4] && 
	 str_eq (M_NAME(var_messages[i-4]), VAR_LABEL)) ||
	(var_messages[i-5] && 
	 str_eq (M_NAME(var_messages[i-5]), VAR_LABEL)) ||
	(var_messages[i-6] && 
	 str_eq (M_NAME(var_messages[i-6]), VAR_LABEL)) ||
	(var_messages[i-7] && 
	 str_eq (M_NAME(var_messages[i-7]), VAR_LABEL)) ||
	(var_messages[i-8] && 
	 str_eq (M_NAME(var_messages[i-8]), VAR_LABEL)) ||
	(var_messages[i-9] && 
	 str_eq (M_NAME(var_messages[i-9]), VAR_LABEL)) ||
	(var_messages[i-10] && 
	 str_eq (M_NAME(var_messages[i-10]), VAR_LABEL)) ||
	(var_messages[i-11] && 
	 str_eq (M_NAME(var_messages[i-11]), VAR_LABEL)) ||
	(var_messages[i-12] && 
	 str_eq (M_NAME(var_messages[i-12]), VAR_LABEL)) ||
	(var_messages[i-13] && 
	 str_eq (M_NAME(var_messages[i-13]), VAR_LABEL)) ||
	(var_messages[i-14] && 
	 str_eq (M_NAME(var_messages[i-14]), VAR_LABEL)) ||
	(var_messages[i-15] && 
	 str_eq (M_NAME(var_messages[i-15]), VAR_LABEL)) ||
	(var_messages[i-16] && 
	 str_eq (M_NAME(var_messages[i-16]), VAR_LABEL)) ||
	(var_messages[i-17] && 
	 str_eq (M_NAME(var_messages[i-17]), VAR_LABEL)) ||
	(var_messages[i-18] && 
	 str_eq (M_NAME(var_messages[i-18]), VAR_LABEL)) ||
	(var_messages[i-19] && 
	 str_eq (M_NAME(var_messages[i-19]), VAR_LABEL)) ||
	(var_messages[i-20] && 
	 str_eq (M_NAME(var_messages[i-20]), VAR_LABEL)) ||
	(var_messages[i-21] && 
	 str_eq (M_NAME(var_messages[i-21]), VAR_LABEL)) ||
	(var_messages[i-22] && 
	 str_eq (M_NAME(var_messages[i-22]), VAR_LABEL)) ||
	(var_messages[i-23] && 
	 str_eq (M_NAME(var_messages[i-23]), VAR_LABEL)) ||
	(var_messages[i-24] && 
	 str_eq (M_NAME(var_messages[i-24]), VAR_LABEL)))
      asm volatile ("int3;");
#endif


    if (is_gnu_extension_keyword (m -> name)) {
      if (warn_extension_opt) {
	warning (m, "Compiler extension %s.", m -> name);
      }
      if (str_eq (M_NAME(m), "__attribute__"))
	gnu_attributes (var_messages, i);
      continue;
    }
    
    if (is_ctalk_keyword (m -> name)) {
      i = find_declaration_end (var_messages, i, end_ptr); 
      continue;
    }

    if (str_eq (m -> name, "enum")) {
      CVAR *c;
      if ((c = enum_decl (var_messages, i)) != NULL)
	add_variable_from_cvar (c);
      i = find_declaration_end (var_messages, i, end_ptr);
    } else if (str_eq (m -> name, "typedef")) {
      typedef_var (var_messages, i, end_ptr);
      i = find_declaration_end (var_messages, i, end_ptr);
    } else if (str_eq (m -> name, "extern")) {
      extern_declaration (var_messages, i);
      i = find_declaration_end (var_messages, i, end_ptr);
    } else {
      if (is_c_fn_declaration_msg (var_messages, i, end_ptr)) {
	i = find_function_close (var_messages, i, end_ptr);
	add_function (m);
      } else {
	if (is_c_function_prototype_declaration (var_messages, i)) {
	  decl_is_prototype = TRUE;
	  i = find_declaration_end (var_messages, i, end_ptr);
	  add_function (m);
	} else {
	  if (is_c_var_declaration_msg (var_messages, i,
					end_ptr, FALSE)) {
	    if ((c = parser_to_cvars ()) != NULL) {
	      validate_type_declarations (c);
	      add_variable_from_cvar (c);
	    }
	  }
	  i = find_declaration_end (var_messages, i, end_ptr);
	}
      }
    } /* (!strcmp (m -> name, "enum")) */
  } /* for (i = N_VAR_MESSAGES... */


#ifdef DEBUG_TYPEDEFS
  dump_typedefs (var_message_stack_at (N_VAR_MESSAGES));
  exit (EXIT_FAILURE);
#endif

  REUSE_MESSAGES(var_messages,var_messageptr,N_VAR_MESSAGES)

  strcpy (input_source_file, tmp_src_file);

  return SUCCESS;
}

extern CLASSLIB *lib_includes[];
extern int lib_includes_ptr;

int parse_vars_and_prototypes (MESSAGE_STACK messages, int start_ptr,
			       int end_ptr) {

  int i;
  MESSAGE *m;
  CVAR *c;

  for (i = start_ptr; i > end_ptr; i--) {

    m = messages[i];

    if (M_ISSPACE (m)) continue;

    switch (m -> tokentype)
      {
      case PREPROCESS:
	if (IS_MESSAGE(messages[i - 2]))
	  if (messages[i-2] -> tokentype == INTEGER)
	    set_line_info (messages, i, end_ptr);
	break;
      case LABEL:
	if (is_gnu_extension_keyword (m -> name)) {
	  if (warn_extension_opt) {
	    warning (m, "Compiler extension %s.", m -> name);
	  }
	  if (str_eq (M_NAME(m), "__attribute__"))
	    gnu_attributes (messages, i);
	  continue;
	}

	if (str_eq (M_NAME(m), "self")) {
	  m -> attrs |= TOK_SELF;
	  i = find_declaration_end (messages, i, end_ptr); 
	  continue;
	} else if (str_eq (M_NAME(m), "super")) {
	  m -> attrs |= TOK_SUPER;
	  i = find_declaration_end (messages, i, end_ptr); 
	  continue;
	} else if (is_ctalk_keyword (m -> name)) {
	  i = find_declaration_end (messages, i, end_ptr); 
	  continue;
	}

	if (str_eq (m -> name, "enum")) {
	  CVAR *c;
	  if ((c = enum_decl (messages, i)) != NULL)
	    add_variable_from_cvar (c);
	  i = find_declaration_end (messages, i, end_ptr);
	} else {
	  if (str_eq (m -> name, "typedef")) {
	    typedef_var (messages, i, end_ptr);
	    i = find_declaration_end (messages, i, end_ptr);
	  } else {
	    if (str_eq (m -> name, "extern")) {
	      extern_declaration (messages, i);
	      i = find_declaration_end (messages, i, end_ptr);
	    } else {
	      if (is_c_fn_declaration_msg (messages, i, end_ptr)) {
		i = find_function_close (messages, i, end_ptr);
		add_function (m);
	      } else {
		if (is_c_function_prototype_declaration (messages, i)) {
		  decl_is_prototype = TRUE;
		  i = find_declaration_end (messages, i, end_ptr);
		  add_function (m);
		} else {
		  if (is_c_var_declaration_msg (messages, i,
						end_ptr, FALSE)) {
		    if ((c = parser_to_cvars ()) != NULL) {
		      validate_type_declarations (c);
		      add_variable_from_cvar (c);
		    }
		  }
		  i = find_declaration_end (messages, i, end_ptr);
		}
	      }
	    }
	  } /* (!strcmp (m -> name, "typedef")) */
	} /* (!strcmp (m -> name, "enum")) */
	break;
      default:
	break;
      } /* switch */
  } /* for (i = start_ptr ... */

  method_prototypes_tok (messages, start_ptr, end_ptr,
			 C_LIB_INCLUDE);

  return SUCCESS;
}

/*
 *  If we have a constant expression, then check if the next message
 *  is a label. 
 */

OBJECT *c_constant_object (MESSAGE_STACK messages, int msg_ptr) {

  OBJECT *o = NULL;       /* Avoid a warning. */
  MESSAGE *m, *m_next;
  int m_next_ptr;
  
  if ((m_next_ptr = nextlangmsg (messages, msg_ptr)) == ERROR)
    return NULL;

  if (M_TOK(messages[m_next_ptr]) != LABEL)
    return NULL;

  if (get_object (M_NAME(messages[m_next_ptr]), NULL)) {
    object_follows_a_constant_warning (messages, msg_ptr, m_next_ptr);
    return NULL;
  }

  m = messages[msg_ptr];
  m_next = messages[m_next_ptr];

  switch (m -> tokentype)
    {
    case LITERAL:
      o = create_object (STRING_CLASSNAME, m -> name);
      strcpy (o -> __o_superclassname, STRING_SUPERCLASSNAME);
      break;
    case LITERAL_CHAR:
      o = create_object (CHARACTER_CLASSNAME, m -> name);
      strcpy (o -> __o_superclassname, CHARACTER_SUPERCLASSNAME);
      break;
    case INTEGER:
    case LONG:
      o = create_object (INTEGER_CLASSNAME, m -> name);
      strcpy (o -> __o_superclassname, INTEGER_SUPERCLASSNAME);
      break;
    case LONGLONG:
      o = create_object (LONGINTEGER_CLASSNAME, m -> name);
      strcpy (o -> __o_superclassname, LONGINTEGER_SUPERCLASSNAME);
      break;
    case FLOAT:
      o = create_object (FLOAT_CLASSNAME, m -> name);
      strcpy (o -> __o_superclassname, FLOAT_SUPERCLASSNAME);
      break;
    }

  o -> scope = RECEIVER_VAR;
  m -> obj = o;
  m_next -> receiver_obj = o;
  m_next -> tokentype = METHODMSGLABEL;
  m_next -> receiver_msg = m;
  return o;

}

static PENDING_FN_ARG *fn_arg_list,
  *fn_arg_ptr;

void buffer_fn_args (PENDING *p, char *tok) {

  /*
   *  Duplicate the pending statement so fileout () et al. can
   *  delete the original pending statement.
   */
  PENDING_FN_ARG *p_dup;

  if ((p_dup = (PENDING_FN_ARG *) calloc (1, sizeof (struct _pending_fn_arg)))
      == NULL)
    _error ("buffer_fn_args: %s.\n", strerror (errno));

  strcpy (p_dup -> stmt, p -> stmt);
  strcpy (p_dup -> tok, tok);
  p_dup -> n = p -> n;

  if (!fn_arg_list) {
    fn_arg_list = fn_arg_ptr = p_dup;
  } else {
    fn_arg_ptr -> next = p_dup;
    p_dup -> prev = fn_arg_ptr;
    fn_arg_ptr = p_dup;
  }
}

/*
 *  Call with any label in the struct.  On exit the start
 *  and end stack indexes are in struct_element_start and
 *  struct_element_end.  To retrieve the struct CVAR, call
 *  get_struct_from_member () next.
 */

static int struct_element_start;
static int struct_element_end;
static char struct_labels[MAXARGS][MAXLABEL];
static int struct_label_ptr;

int is_struct_element (MESSAGE_STACK messages, int ptr) {

  int lookahead,
    lookback,
    stack_begin,
    stack_end,
    last_token = ptr,       /* Avoid warnings. */
    prev_token = ptr,
    non_struct_token,
    i,
    n_subscripts;
  MESSAGE *m;

  stack_begin = stack_start (messages);
  stack_end = get_stack_top (messages);

  for (lookahead = ptr - 1, non_struct_token = FALSE, 
	 struct_element_end = ptr; 
       (lookahead >= stack_end) && (non_struct_token == FALSE); 
       lookahead--) {

    m = messages[lookahead];

    if (!m || !IS_MESSAGE (m))
      break;

    if (m -> tokentype == NEWLINE || m -> tokentype == WHITESPACE)
      continue;

    switch (m -> tokentype)
      {
      case LABEL:
	if (prev_token != DEREF && prev_token != PERIOD)
	  non_struct_token = TRUE;
	else
	  struct_element_end = lookahead;
	break;
      case DEREF:
      case PERIOD:
      case WHITESPACE:
      case NEWLINE:
	break;
      default:
	non_struct_token = TRUE;
	break;
      }
    prev_token = m -> tokentype;
  }

  for (lookback = ptr + 1, non_struct_token = FALSE, 
	 struct_element_start = ptr; 
       (lookback <= stack_begin) && (non_struct_token == FALSE); 
       lookback++) {

    m = messages[lookback];

    if (!m || !IS_MESSAGE (m))
      break;

    if (m -> tokentype == NEWLINE || m -> tokentype == WHITESPACE)
      continue;

    switch (m -> tokentype)
      {
      case LABEL:
	if (last_token != DEREF && last_token != PERIOD &&
	    last_token != ARRAYOPEN)
	  non_struct_token = TRUE;
	else
	  struct_element_start = lookback;
	break;
      case DEREF:
      case PERIOD:
      case WHITESPACE:
      case NEWLINE:
	break;
      case ARRAYCLOSE:
	lookback = match_array_brace_rev
	  (messages, lookback,
	   ((messages == message_stack ()) ?
	    P_MESSAGES : N_MESSAGES),
	   &n_subscripts);
	m = messages[lookback];
	break;
      default:
	non_struct_token = TRUE;
	break;
      }
    last_token = m -> tokentype;
  }

  n_subscripts = 0;
  for (i = struct_element_start, struct_label_ptr = 0; 
       i >= struct_element_end; i--) {
    m = messages[i];
    if (m -> tokentype == LABEL) {
      if (n_subscripts == 0) {
	strcpy (struct_labels[struct_label_ptr++], m -> name);
      }
    } else if (m -> tokentype == ARRAYOPEN) {
      ++n_subscripts;
    } else if (m -> tokentype == ARRAYCLOSE) {
      --n_subscripts;
    }
  }

  if (struct_label_ptr < 1)
    return FALSE;

  if (get_struct_from_member ())
    return TRUE;

  return FALSE;
}

/* this is like get_local_var, except that we keep going
   until we find a CVAR with struct members.  That's because
   if we have struct declarations like this:

    struct _my_struct {
      int mbr;
    } myStructArray[20] = { 0, };

    struct _my_struct *myStructPtrs[20];

  Then the CVARs look the same, except only one of them has
  the c -> members element filled in.
*/

static CVAR *get_local_struct_defn_from_type (char *struct_decl) {
  PARSER *p;
  CVAR *c, *c_decl, *c_defn;

  if ((interpreter_pass == preprocessing_pass) ||
      (interpreter_pass == var_pass))
    return (CVAR *)NULL;

  c_decl = NULL;
  if ((p = parser_at (parser_ptr ())) != NULL) {
    for (c = p -> cvars; c; c = c -> next) {
      if (str_eq (struct_decl, c -> name)) {
	c_decl = c;
	if (c_decl -> members)
	  return c_decl;
      }
    }
  } else {
    return (CVAR *)NULL;
  }
  if (!c_decl)
    return (CVAR *)NULL;
  for (c = p -> cvars; c; c = c -> next) {
    if (str_eq (c_decl -> type, c -> type)) {
      c_defn = c;
      if (c_defn -> members)
	return c_defn;
    }
  }
  for (c = global_cvars; c; c = c -> next) {
    if (c -> attrs & CVAR_ATTR_STRUCT_DECL) {
      if (str_eq (c -> type, c_decl -> type) && c -> members)
 	return c;
    }
  }

  return (CVAR *)NULL;
}

CVAR *get_struct_from_member (void) {

  CVAR *c, *c_mbr;
  int i;

  if (((c = get_local_struct_defn_from_type (struct_labels[0])) != NULL) ||
      ((c = get_global_var (struct_labels[0])) != NULL)) {
    for (i = 1; i < struct_label_ptr; i++) {
      for (c_mbr = c -> members; c_mbr; c_mbr = c_mbr -> next) {
	if (str_eq (struct_labels[i], c_mbr -> name))
	  break;
      }
      if (c_mbr == NULL)
	return NULL;
    }
    return c;
  }
  return NULL;
}

bool is_enum_member (char *s) {
  return (bool)_hash_get (enum_members, s);
}

bool is_struct_member (char *s) {
  return (bool)_hash_get (global_struct_member_names, s);
}

/* subscripted structs or fn derefs get checked further. */
#define STRUCT_SEPARATOR(m) (M_TOK(m) == ARRAYCLOSE || M_TOK(m) == CLOSEPAREN)

/* perform a syntax check before the big search. */
int is_struct_member_tok (MESSAGE_STACK messages, int mbr_idx) {

  int stack_start_idx, i, last_label_tok;
  bool have_deref_op;
  CVAR *struct_var;

  if (M_TOK(messages[mbr_idx]) != LABEL)
    return FALSE;
  have_deref_op = false;
  last_label_tok = -1;
  stack_start_idx = stack_start (messages);
  for (i = mbr_idx + 1; i <= stack_start_idx; ++i) {
    if (M_ISSPACE(messages[i]))
      continue;
    if ((M_TOK(messages[i]) == PERIOD) || (M_TOK(messages[i]) == DEREF)) {
      have_deref_op = true;
      continue;
    } else if (M_TOK(messages[i]) != LABEL) {
      if (last_label_tok == -1) {
	/* subscripted structs and fn derefs need to be checked further. */
	if (STRUCT_SEPARATOR(messages[i])) {
	  break;
	} else {
	  return FALSE;
	}
      } else {
	break;
      }
    } else if (have_deref_op) {
      if (M_TOK(messages[i]) == LABEL) {
	last_label_tok = i;
	continue;
      } else if (STRUCT_SEPARATOR(messages[i])) {
	break;
      } else {
	break;
      }
    } else if (STRUCT_SEPARATOR(messages[i])) {
      break;
    } else {
      return FALSE;
    }
  }

  if (last_label_tok > 0) {
    if ((struct_var =
	 _hash_get (declared_global_variables,
		    M_NAME(messages[last_label_tok]))) != NULL) {
      if (struct_var -> attrs & CVAR_ATTR_STRUCT_DECL) {
	return TRUE;
      }
    } else if ((struct_var =
		_hash_get (declared_typedefs,
			   M_NAME(messages[last_label_tok]))) != NULL) {
      if (struct_var -> attrs & CVAR_ATTR_STRUCT_DECL) {
	return TRUE;
      }
    } else {
      for (struct_var = CURRENT_PARSER -> cvars;
	   struct_var; struct_var = struct_var -> next) {
	if (str_eq (struct_var -> name, M_NAME(messages[last_label_tok]))) {
	  if (struct_var -> attrs & CVAR_ATTR_STRUCT_DECL) {
	    return TRUE;
	  }
	}
      }
    }
  }

  return is_struct_member (M_NAME(messages[mbr_idx]));
}

bool is_this_fn_param (char *s) {
  CFUNC *fn;
  CVAR *param;
  char *fn_name;
  if (interpreter_pass != parsing_pass)
    return false;
  /* keep this stuff here in order to keep _hash_get as
     short as possible. */
  if ((fn_name = get_fn_name ()) != NULL) {
    if (*fn_name != 0) {
      if ((fn = _hash_get (declared_functions, fn_name)) != NULL) {
	for (param = fn -> params; param; param = param -> next) {
	  if (str_eq (param -> name, s))
	    return true;
	}
      }
    }
  }
  return false;
}

CVAR *is_this_fn_param_2 (char *s) {
  CFUNC *fn;
  CVAR *param;
  char *fn_name;
  if (interpreter_pass != parsing_pass)
    return false;
  /* keep this stuff here in order to keep _hash_get as
     short as possible. */
  if ((fn_name = get_fn_name ()) != NULL) {
    if (*fn_name != 0) {
      if ((fn = _hash_get (declared_functions, fn_name)) != NULL) {
	for (param = fn -> params; param; param = param -> next) {
	  if (str_eq (param -> name, s))
	    return param;
	}
      }
    }
  }
  return NULL;
}

bool is_fn_param (char *s) {
  return (bool)_hash_get (function_params, s);
}

/*
 *  Here because this check is only needed when registering
 *  variables.
 */

char *declaration_expr_from_cvar (CVAR *c, char *buf) {
  int n_derefs;
  if (*(c -> qualifier4)) {strcatx2 (buf, c -> qualifier4, NULL);
                           strcatx2 (buf, " ", NULL);}
  if (*(c -> qualifier3)) {strcatx2 (buf, c -> qualifier3, NULL);
                           strcatx2 (buf, " ", NULL);}
  if (*(c -> qualifier2)) {strcatx2 (buf, c -> qualifier2, NULL);
                           strcatx2 (buf, " ", NULL);}
  if (*(c -> qualifier)) {strcatx2 (buf, c -> qualifier, NULL);
                          strcatx2 (buf, " ", NULL);}
  if (*(c -> type)) {strcatx2 (buf, c -> type, NULL);
                     strcatx2 (buf, " ", NULL);} 
  else {strcatx2 (buf, "int", NULL); strcatx2 (buf, " ", NULL);}
  for (n_derefs = 0; n_derefs < c -> n_derefs; n_derefs++)
    strcatx2 (buf, "*", NULL);
  if (*(c -> name)) {strcatx2 (buf, c -> name, NULL);}
  return buf;
}

CVAR *have_struct (char *struct_type) {
  CVAR *c;
  PARSER *p;
  int global_frame;
  p = CURRENT_PARSER;

  for (c = global_cvars; c; c = c -> next) {
    if (str_eq (c -> name, struct_type) ||
	str_eq (c -> type, struct_type))
      return c;
  }

  if ((global_frame = is_global_frame ()) == FALSE) { /* Local frame */
    /*
     *  Add the variable to the parser's variable list.
     */
    for (c = p -> cvars; c; c = c -> next) {
      if (str_eq (c -> name, struct_type) ||
	  str_eq (c -> type, struct_type))
	return c;
    }
  }

  if ((c = get_typedef (struct_type)) != NULL)
    return c;

  return NULL;
}

CFUNC *cvar_to_cfunc (CVAR *c) {
  CFUNC *cfn;
  if ((cfn = (CFUNC *)calloc (1, sizeof (CFUNC))) == NULL) 
    _error ("add_function (): %s\n.", strerror (errno));
  if (*c -> decl) strcpy (cfn -> decl, c -> decl);
  if (*c -> type) strcpy (cfn -> return_type, c -> type);
  if (*c -> qualifier) strcpy (cfn -> qualifier_type, c -> qualifier);
  if (*c -> qualifier2) strcpy (cfn -> qualifier2_type, c -> qualifier2);
  if (*c -> storage_class) strcpy (cfn -> storage_class, c -> storage_class);
  if (*c -> name) strcpy (cfn -> decl, c -> name);
  cfn -> is_unsigned = c -> is_unsigned;
  cfn -> return_derefs = c -> n_derefs;
  cfn -> params = c -> params;
  cfn -> return_type_attrs = c -> type_attrs;
  return cfn;
}

void unlink_global_cvar (CVAR *c) {

  CVAR *var;
  int is_linked = FALSE;

  if (c == global_cvars) {
    global_cvars = c -> next;
    is_linked = TRUE;
  }

  for (var = global_cvars; var && !is_linked; var = var -> next) {
    if (var == c) is_linked = TRUE;
  }
  
  
  if (is_linked) {
    if (c -> next) c -> next -> prev = c -> prev;
    if (c -> prev) c -> prev -> next = c -> next;
  }
}

int is_c_type (char *s) {
#ifdef __GNUC__
  return is_c_data_type (s) ||
    get_typedef (s) ||
    get_type_conv_name (s) || 
    is_gnu_extension_keyword(s);
#else
  return is_c_data_type (s) ||
    get_type_conv_name (s) || 
    get_typedef (s);
#endif
}

int validate_type_declarations (CVAR *c) {
  int have_undefined_type = FALSE;
  if (c -> type[0] && !is_c_type (c -> type))
    have_undefined_type = TRUE;
  if (c -> qualifier[0] && !is_c_type (c -> qualifier))
    have_undefined_type = TRUE;
  if (c -> qualifier2[0] && !is_c_type (c -> qualifier2))
    have_undefined_type = TRUE;
  if (c -> qualifier3[0] && !is_c_type (c -> qualifier3))
    have_undefined_type = TRUE;
  if (c -> qualifier4[0] && !is_c_type (c -> qualifier4))
    have_undefined_type = TRUE;
  if (c -> storage_class[0] && !is_c_type (c -> storage_class))
    have_undefined_type = TRUE;

  if (c -> type_attrs == 0) {
    c -> type_attrs = is_c_data_type_attr (c -> type);
  }

  /*
   *  Declaration rec should be correct enough by now, 
   *  but check for a few cases.
   */

  /*
   *  Array of structs:
   *
   *  struct _tag {
   *   ... 
   *  } decl [] ...
   *
   *  or
   *
   *  struct _tag {
   *   ... 
   *  } decl [] [] ...
   *
   *  TODO - We could probably add 
   *  CVAR_ATTR_STRUCT|CVAR_ATTR_STRUCT_DECL to 
   *  the CVAR's attrs - the attrs are only 
   *  CVAR_ATTR_ARRAY_DECL right now.
   */
  if ((c -> type[0] && c -> qualifier[0] &&
      str_eq (c -> qualifier, "struct")) &&
      (c -> attrs & CVAR_ATTR_ARRAY_DECL) && 
      (c -> n_derefs > 0)) 
    return TRUE;

  if ((c -> attrs & CVAR_ATTR_STRUCT_DECL) ||
      (c -> attrs & CVAR_ATTR_STRUCT_PTR) ||
      (c -> attrs & CVAR_ATTR_STRUCT))
    return TRUE;

  if (have_undefined_type)
    return FALSE;
  else
    return TRUE;
}

CVAR *struct_member_from_expr (char *expr, CVAR *struct_defn) {
  int end_ptr, idx, n_th_label;
  CVAR *defn_mbr, *defn_mbr_mbr, 
    *defn_mbr_typedef;

  end_ptr = tokenize_no_error (var_message_push, expr);
  defn_mbr = struct_defn;
  for (idx = N_VAR_MESSAGES, n_th_label = 0; idx >= end_ptr; idx--) {
    if (M_TOK(var_messages[idx]) == LABEL) {
      switch (++n_th_label)
	{
	case 0:
	case 1:
	  break;
	default:
 	  if (!(defn_mbr -> attrs & CVAR_ATTR_TYPEDEF) &&
	      /* Exclude actual C type check for simple declarations.
		 Vars with anyother attributes need to have the 
		 type member compared. */
	      !(defn_mbr -> attrs == CVAR_ATTR_STRUCT_DECL)) {
	    if (!is_c_data_type (defn_mbr -> type)) {
	      if ((defn_mbr_typedef = get_typedef (defn_mbr -> type)) 
		  != NULL) {
		defn_mbr = defn_mbr_typedef;
	      }
	    }
 	  }
	  for (defn_mbr_mbr = defn_mbr -> members; defn_mbr_mbr; 
	       defn_mbr_mbr = defn_mbr_mbr -> next) {
	    if (str_eq (defn_mbr_mbr -> name, M_NAME(var_messages[idx]))) {
	      if (((defn_mbr_mbr -> type_attrs & CVAR_TYPE_STRUCT) ||
		  (defn_mbr_mbr -> type_attrs & CVAR_TYPE_UNION)) &&
		  (defn_mbr_mbr -> members == NULL)) {
		if (idx == end_ptr) {
		  defn_mbr = defn_mbr_mbr;
		} else {
		  if (((defn_mbr = 
			get_local_struct_defn (defn_mbr_mbr->type))==NULL)&&
		      ((defn_mbr = 
			get_global_struct_defn (defn_mbr_mbr->type))==NULL)&&
		      ((defn_mbr = 
			have_struct (defn_mbr_mbr->type))==NULL)){
		    _warning ("warning: struct_member_from_expr: could not find definition of struct expression \"%s\".\n", expr);
		    defn_mbr = defn_mbr_mbr;
		  }
		}
	      } else {
		defn_mbr = defn_mbr_mbr;
	      }
	    }
	  }
	  break;
	}
    }
  }
  REUSE_MESSAGES(var_messages,var_messageptr,N_VAR_MESSAGES)
  return defn_mbr;
}

/* Like the fn above, but this one doesn't need a re-untokenization
   and then a re-tokenization. */
CVAR *struct_member_from_expr_b (MESSAGE_STACK messages, 
				 int start_idx, int end_idx,
				 CVAR *struct_defn) {
  int idx, n_th_label;
  CVAR *defn_mbr, *defn_mbr_mbr, 
    *defn_mbr_typedef;
  char expr[MAXMSG];

  defn_mbr = struct_defn;
  for (idx = start_idx, n_th_label = 0; idx >= end_idx; idx--) {
    if (M_TOK(messages[idx]) == LABEL) {
      switch (++n_th_label)
	{
	case 0:
	case 1:
	  break;
	default:
 	  if (!(defn_mbr -> attrs & CVAR_ATTR_TYPEDEF) &&
	      /* Exclude actual C type check for simple declarations.
		 Vars with anyother attributes need to have the 
		 type member compared. */
	      !(defn_mbr -> attrs == CVAR_ATTR_STRUCT_DECL)) {
	    if (!is_c_data_type (defn_mbr -> type)) {
	      if ((defn_mbr_typedef = get_typedef (defn_mbr -> type)) 
		  != NULL) {
		defn_mbr = defn_mbr_typedef;
	      }
	    }
 	  }
	  for (defn_mbr_mbr = defn_mbr -> members; defn_mbr_mbr; 
	       defn_mbr_mbr = defn_mbr_mbr -> next) {
	    if (str_eq (defn_mbr_mbr -> name, M_NAME(messages[idx]))) {
	      if (((defn_mbr_mbr -> type_attrs & CVAR_TYPE_STRUCT) ||
		  (defn_mbr_mbr -> type_attrs & CVAR_TYPE_UNION)) &&
		  (defn_mbr_mbr -> members == NULL)) {
		if (idx == end_idx) {
		  defn_mbr = defn_mbr_mbr;
		} else {
		  if (((defn_mbr = 
			get_local_struct_defn (defn_mbr_mbr->type))==NULL)&&
		      ((defn_mbr = 
			get_global_struct_defn (defn_mbr_mbr->type))==NULL)&&
		      ((defn_mbr = 
			have_struct (defn_mbr_mbr->type))==NULL)){
		    toks2str (messages, start_idx, end_idx, expr);
		    _warning ("warning: struct_member_from_expr_b: could not find definition of struct expression \"%s\".\n", expr);
		    defn_mbr = defn_mbr_mbr;
		  }
		}
	      } else {
		defn_mbr = defn_mbr_mbr;
	      }
	    }
	  }
	  break;
	}
    }
  }
  if ((defn_mbr -> type_attrs & CVAR_TYPE_STRUCT) && 
      (defn_mbr -> members == NULL)) {
    CVAR *defn_sub_mbr, *defn_mbr_2;
    if ((defn_sub_mbr = struct_defn_from_struct_decl 
	 (defn_mbr -> type)) != NULL) {
      if ((defn_mbr_2 = mbr_from_struct_or_union_cvar 
	   (defn_sub_mbr, M_NAME(messages[end_idx]))) != NULL) {
	defn_mbr = defn_mbr_2;
      }
    }
  }
  return defn_mbr;
}

int function_not_shadowed_by_method (MESSAGE_STACK messages, int idx) {
  int prev_tok_idx;
  METHOD *method;
  MESSAGE *m_prev_tok;
  if (get_function (M_NAME(messages[idx]))) {
    if ((prev_tok_idx = prevlangmsg (messages, idx)) != ERROR) {
      m_prev_tok = messages[prev_tok_idx];
      if (IS_OBJECT(m_prev_tok -> obj)) {
	if (((method = get_instance_method (messages[idx],
					   m_prev_tok -> obj,
					   M_NAME(messages[idx]),
					    ERROR,
					   FALSE)) != NULL) ||
	    ((method = get_class_method (messages[idx],
					 m_prev_tok -> obj,
					 M_NAME(messages[idx]),
					 ERROR, FALSE)) != NULL)) {
	  return FALSE;
	} else {
	  return TRUE;
	}
      } else {
	return TRUE;
      }
    } else {
      return TRUE;
    }
  }
  return FALSE;
}

/* If this doesn't seem to work, try get_struct_defn (),
   (method.c), although it doesn't check for typedefs. */
CVAR *struct_defn_from_struct_decl (char *struct_type) {
  CVAR *c;
  if ((c = get_global_struct_defn (struct_type)) != NULL)
    return c;
  if ((c = get_local_struct_defn (struct_type)) != NULL)
    return c;
  if ((c = get_typedef (struct_type)) != NULL)
    return c;
  return NULL;
}

CVAR *mbr_from_struct_or_union_cvar (CVAR *struct_cvar, char *name) {
  CVAR *mbr = NULL;

  if (struct_cvar -> members) {
    for (mbr = struct_cvar -> members; mbr; mbr = mbr -> next) {
      if (mbr -> members) {
	if ((mbr = mbr_from_struct_or_union_cvar (mbr, name)) != NULL) {
	  return mbr;
	}
      } else {
	if (str_eq (mbr -> name, name))
	  return mbr;
      }
    }
  }

  return mbr;
}

char *struct_expr_basename (char *expr, char *basename_out) {
  int i;
  for (i = 0; expr[i]; ++i) {
    if (isalnum (expr[i]) || expr[i] == '_') {
      basename_out[i] = expr[i];
      basename_out[i+1] = 0;
    } else {
      basename_out[i] = 0;
      break;
    }
  }
  return basename_out;
}

