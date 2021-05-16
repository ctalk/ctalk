/* $Id: mthdrf.c,v 1.1.1.1 2021/04/03 11:26:02 rkiesling Exp $ */

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
#include "uproto.h"

extern CLASSLIB *lib_includes[MAXARGS + 1];  /* Declared in class.c.  */
extern int lib_includes_ptr;
extern CLASSLIB *input_declarations[MAXARGS+1];
extern int input_declarations_ptr;

extern char *ascii[8193];             /* from intascii.h */

MESSAGE *r_messages[N_R_MESSAGES + 1];
int r_messageptr;

int method_from_proto;

HASHTAB proto_selectors;

extern HASHTAB declared_method_names;    /* Declared in method.c. */
extern HASHTAB declared_method_selectors;
extern HASHTAB declared_global_variables; /* Declared in cvars.c. */
extern I_PASS interpreter_pass;    /* Declared in rtinfo.c.             */

extern int library_input;   /* Declared in class.c.  TRUE if evaluating */
                            /* class libraries.                         */

extern LIST *input_methods; /* Declared in infiles.c.                   */

static void make_proto_tag (char *, char *, char *, char *);
static void make_preload_proto_tag (char *, char *, char *,
				    bool, bool, int, char *);

LIST *output_method_prototypes = NULL;

/* Uncomment to fill in parameter information for the prototype.
   Normally not needed (so far), unless we check overloaded
   methods in methodgt.c. */
/* #define FULL_PROTO */

void init_method_proto (void) {
  _new_hash (&proto_selectors);
}

MESSAGE_STACK r_message_stack (void) {
  return r_messages;
}

int r_message_push (MESSAGE *m) {
  if (r_messageptr == 0) {
    _error ("r_message_push: stack_overflow.");
    return ERROR;
  }

  if (!m)
    _error ("r_message_push: null pointer, r_messageptr = %d.", 
	    r_messageptr);

  r_messages[r_messageptr] = m;

#ifdef STACK_TRACE
  debug ("r_message_push %d. %s.", r_messageptr, 
	 (r_messages[r_messageptr] && 
	  IS_MESSAGE (r_messages[r_messageptr])) ?
	 r_messages[r_messageptr] -> name : "(null)"); 
#endif

  --r_messageptr;

  return r_messageptr + 1;
}

void init_r_messages (void) {
  r_messageptr = N_R_MESSAGES;
}

int get_r_message_ptr (void) {
  return r_messageptr;
}

static int tag_format_length = 0;

int method_prototypes (char *p_lib_fn, CLASSLIB *c) {

  int idx, end_ptr;
  MESSAGE *m;
  char tag[MAXMSG], *s_buf, *bufp;
  char d_buf[MAXMSG];
  METHOD_PROTO *mp;
  struct stat statbuf;
  int _class_idx, _selector_idx, _alias_idx;
  int _open_arg_idx, _close_arg_idx;
#ifdef USE_PROTO_PARAMS
  int n_arg_separators, j;
#endif  
  int _close_block_idx;
  int stack_top, stack_start_idx;

  if (!stat (p_lib_fn, &statbuf)) {
    if ((bufp = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
      _error ("method_prototypes (__xalloc): %s.\n", 
	      strerror (errno));
    read_file (bufp, p_lib_fn);
  } else {
    _error ("method_prototypes: %s: %s.", p_lib_fn, strerror (errno));
  }

  stack_start_idx = stack_start (r_messages);
  end_ptr = tokenize_reuse (r_message_push, bufp);

  __xfree (MEMADDR(bufp));

  stack_top = get_stack_top (r_messages);
  _close_block_idx = -1; /* Avoid a warning. */

  for (idx = stack_start_idx; idx >= end_ptr; idx--) {

    if (M_ISSPACE(r_messages[idx])) continue;

    if (M_TOK(r_messages[idx]) != LABEL) continue;
      
    m = r_messages[idx];
    if (str_eq (M_NAME(m), "instanceMethod"))
      m -> attrs |= TOK_IS_INSTANCE_METHOD_KEYWORD;
    else if (str_eq (M_NAME(m), "classMethod"))
      m -> attrs |= TOK_IS_CLASS_METHOD_KEYWORD;
    else
      continue;

    if (((_class_idx = prevlangmsg (r_messages, idx)) == ERROR) ||
	((_selector_idx = nextlangmsg (r_messages, idx)) == ERROR) ||
	((_alias_idx = nextlangmsg (r_messages, _selector_idx)) == ERROR))
      _error ("Parse error.\n");

    if (M_TOK(r_messages[_alias_idx]) == OPENPAREN) {
      _open_arg_idx = _alias_idx;
    } else {
      if ((_open_arg_idx = nextlangmsg (r_messages, _alias_idx)) == ERROR)
	_error ("Parse error.\n");
    }

    if ((_close_arg_idx = 
	 match_paren (r_messages, 
		      _open_arg_idx, stack_top)) == ERROR)
      _error ("Parse error.\n");
    if ((mp = (METHOD_PROTO *)__xalloc (sizeof (METHOD_PROTO))) == NULL)
      _error ("method_prototypes (__xalloc): %s.", strerror (errno));
    mp -> sig = METHOD_PROTO_SIG;
#ifdef USE_PROTO_PARAMS
    if (M_TOK(r_messages[_alias_idx]) != OPENPAREN)
      mp -> alias = strdup (M_NAME(r_messages[_alias_idx]));
    n_arg_separators = 0;
    for (j = _open_arg_idx; j >= _close_arg_idx; j--) {
      switch (M_TOK(r_messages[j]))
	{
	case LABEL:
	  if (str_eq (r_messages[j] -> name, "__prefix__")) {
	    mp -> prefix = true;
	    goto arg_separator_check_done;
	  }
	  break;
	case ARGSEPARATOR:
	  ++n_arg_separators;
	  break;
	case ELLIPSIS:
	  mp -> varargs = true;
	  break;
	}
    }
  arg_separator_check_done:
    if (!mp ->  varargs && !mp -> prefix && n_arg_separators == 0) {
      int void_idx, void_next_idx;
      if ((void_idx = nextlangmsg (r_messages, _open_arg_idx))
	  != ERROR) {
	if (M_TOK(r_messages[void_idx]) == LABEL) {
	  if (str_eq (r_messages[void_idx] -> name, "void")) {
	    if ((void_next_idx = nextlangmsg (r_messages, void_idx))
		!= ERROR) {
	      if (M_TOK(r_messages[void_next_idx]) == CLOSEPAREN) {
		n_arg_separators = -1;
	      }
	    }
	  } 
	}
      }
    }
    mp -> n_params = n_arg_separators + 1;
#else
    mp -> n_params = ANY_ARGS;
#endif /* USE_PROTO_PARAMS */

    if ((_close_block_idx = 
	 find_function_close (r_messages, _close_arg_idx, end_ptr)) == ERROR)
      _error ("%s:%d:Could not find method closing brace.\n",
	      c -> name, r_messages[_close_arg_idx] -> error_line);

    toks2str (r_messages, _class_idx, _close_block_idx, d_buf);
    make_proto_tag 
      (M_NAME(r_messages[_class_idx]),
       ((m -> attrs & TOK_IS_INSTANCE_METHOD_KEYWORD) ?
	"instance" : "class"),
       M_NAME(r_messages[_selector_idx]), tag);

    s_buf = (char *)malloc (strlen (d_buf) + tag_format_length + 1);
    strcatx (s_buf, tag, d_buf, NULL);
    mp -> src = s_buf;

    _hash_put (proto_selectors, M_NAME(r_messages[_selector_idx]),
	       M_NAME(r_messages[_selector_idx]));
    if (!c -> proto) {
      c -> proto = c -> proto_head = mp;
    } else {
      c -> proto_head -> next = mp;
      mp -> prev = c -> proto_head;
      c -> proto_head = mp;
    }
    idx = _close_block_idx;
  }
  REUSE_MESSAGES(r_messages,r_messageptr,stack_start_idx)

  return SUCCESS;
}


/* this fn only stores the prototype tag, not the  source
   code, but it also makes a tag with the information about
   the number of params formatted as cache file name. */
int preload_method_prototypes (char *p_lib_fn, CLASSLIB *c) {

  int idx, end_ptr;
  MESSAGE *m;
  char tag[MAXMSG], *bufp;
  METHOD_PROTO *mp;
  struct stat statbuf;
  bool prefix = false, varargs = false;
  int _class_idx, _selector_idx, _alias_idx;
  int _open_arg_idx, _close_arg_idx, n_arg_separators, j;
  int _close_block_idx;
  int stack_top, stack_start_idx;

  if (!stat (p_lib_fn, &statbuf)) {
    if ((bufp = (char *)__xalloc (statbuf.st_size + 1)) == NULL)
      _error ("method_prototypes (__xalloc): %s.\n", 
	      strerror (errno));
    read_file (bufp, p_lib_fn);
  } else {
    _error ("method_prototypes: %s: %s.", p_lib_fn, strerror (errno));
  }

  stack_start_idx = stack_start (r_messages);
  end_ptr = tokenize_reuse (r_message_push, bufp);

  __xfree (MEMADDR(bufp));

  stack_top = get_stack_top (r_messages);
  _close_block_idx = -1; /* Avoid a warning. */

  for (idx = stack_start_idx; idx >= end_ptr; idx--) {

    if (M_ISSPACE(r_messages[idx])) continue;

    if (M_TOK(r_messages[idx]) != LABEL) continue;
      
    m = r_messages[idx];
    if (str_eq (M_NAME(m), "instanceMethod"))
      m -> attrs |= TOK_IS_INSTANCE_METHOD_KEYWORD;
    else if (str_eq (M_NAME(m), "classMethod"))
      m -> attrs |= TOK_IS_CLASS_METHOD_KEYWORD;
    else
      continue;

    if (((_class_idx = prevlangmsg (r_messages, idx)) == ERROR) ||
	((_selector_idx = nextlangmsg (r_messages, idx)) == ERROR) ||
	((_alias_idx = nextlangmsg (r_messages, _selector_idx)) == ERROR))
      _error ("Parse error.\n");

    if (M_TOK(r_messages[_alias_idx]) == OPENPAREN) {
      _open_arg_idx = _alias_idx;
    } else {
      if ((_open_arg_idx = nextlangmsg (r_messages, _alias_idx)) == ERROR)
	_error ("Parse error.\n");
    }

    if ((_close_arg_idx = 
	 match_paren (r_messages, 
		      _open_arg_idx, stack_top)) == ERROR)
      _error ("Parse error.\n");
    if ((mp = (METHOD_PROTO *)__xalloc (sizeof (METHOD_PROTO))) == NULL)
      _error ("method_prototypes (__xalloc): %s.", strerror (errno));
    mp -> sig = METHOD_PROTO_SIG;
    n_arg_separators = 0;
    for (j = _open_arg_idx; j >= _close_arg_idx; j--) {
      switch (M_TOK(r_messages[j]))
	{
	case LABEL:
	  if (str_eq (r_messages[j] -> name, "__prefix__")) {
	    mp -> prefix = true;
	    goto arg_separator_check_done;
	  }
	  break;
	case ARGSEPARATOR:
	  ++n_arg_separators;
	  break;
	case ELLIPSIS:
	  mp -> varargs = true;
	  break;
	}
    }
  arg_separator_check_done:
    if (!varargs && !prefix && n_arg_separators == 0) {
      int void_idx, void_next_idx;
      if ((void_idx = nextlangmsg (r_messages, _open_arg_idx))
	  != ERROR) {
	if (M_TOK(r_messages[void_idx]) == LABEL) {
	  if (str_eq (r_messages[void_idx] -> name, "void")) {
	    if ((void_next_idx = nextlangmsg (r_messages, void_idx))
		!= ERROR) {
	      if (M_TOK(r_messages[void_next_idx]) == CLOSEPAREN) {
		n_arg_separators = -1;
	      }
	    }
	  } 
	}
      }
    }

    if ((_close_block_idx = 
	 find_function_close (r_messages, _close_arg_idx, end_ptr)) == ERROR)
      _error ("%s:%d:Could not find method closing brace.\n",
	      c -> name, r_messages[_close_arg_idx] -> error_line);

    make_preload_proto_tag 
      (M_NAME(r_messages[_class_idx]),
       ((m -> attrs & TOK_IS_INSTANCE_METHOD_KEYWORD) ?
	"instance" : "class"),
       (M_TOK(r_messages[_alias_idx]) == LABEL ?
	M_NAME(r_messages[_alias_idx]) :
	M_NAME(r_messages[_selector_idx])),
       varargs, prefix,
       n_arg_separators + 1,
       tag);

    mp -> src = strdup (tag);

    _hash_put (proto_selectors, M_NAME(r_messages[_selector_idx]),
       M_NAME(r_messages[_selector_idx]));
    if (!c -> proto) {
      c -> proto = c -> proto_head = mp;
    } else {
      c -> proto_head -> next = mp;
      mp -> prev = c -> proto_head;
      c -> proto_head = mp;
    }
    idx = _close_block_idx;
  }
  REUSE_MESSAGES(r_messages,r_messageptr,stack_start_idx)

  return SUCCESS;
}

int method_prototypes_tok (MESSAGE_STACK messages,
			   int start_idx, int end_idx,
			   CLASSLIB *c) {

  MESSAGE *m;
  char tag[MAXMSG], *s_buf;
  char d_buf[MAXMSG];
  METHOD_PROTO *mp;
  int idx, _class_idx, _selector_idx, _alias_idx;
  int _open_arg_idx, _close_arg_idx;
#ifdef USE_PROTO_PARAMS
  int n_arg_separators, j;
#endif  
  int _close_block_idx;

  _close_block_idx = -1; /* Avoid a warning. */

  for (idx = start_idx; idx > end_idx; idx--) {

    if (M_ISSPACE(messages[idx])) continue;
    if (M_TOK(messages[idx]) != LABEL) continue;

    m = messages[idx];

    if (str_eq (M_NAME(m), "instanceMethod"))
      m -> attrs |= TOK_IS_INSTANCE_METHOD_KEYWORD;
    else if (str_eq (M_NAME(m), "classMethod"))
      m -> attrs |= TOK_IS_CLASS_METHOD_KEYWORD;
    else
      continue;

    if (((_class_idx = prevlangmsg (messages, idx)) == ERROR) ||
	((_selector_idx = nextlangmsg (messages, idx)) == ERROR) ||
	((_alias_idx = nextlangmsg (messages, _selector_idx)) == ERROR))
      _error ("Parse error.\n");
      
    if (M_TOK(messages[_alias_idx]) == OPENPAREN) {
      _open_arg_idx = _alias_idx;
    } else {
      if ((_open_arg_idx = nextlangmsg (messages, _alias_idx)) == ERROR)
	_error ("Parse error.\n");
    }
      
    if ((_close_arg_idx = 
	 match_paren (messages, 
		      _open_arg_idx, end_idx)) == ERROR)
      _error ("Parse error.\n");
    if ((mp = (METHOD_PROTO *)__xalloc (sizeof (METHOD_PROTO))) == NULL)
      _error ("method_prototypes (__xalloc): %s.", strerror (errno));
    mp -> sig = METHOD_PROTO_SIG;
#ifdef USE_PROTO_PARAMS
    if (M_TOK(messages[_alias_idx]) != OPENPAREN)
      mp -> alias = strdup (M_NAME(messages[_alias_idx]));
    n_arg_separators = 0;
    for (j = _open_arg_idx; j >= _close_arg_idx; j--) {
      switch (M_TOK(messages[j]))
	{
	case LABEL:
	  if (str_eq (messages[j] -> name, "__prefix__")) {
	    mp -> prefix = true;
	    goto arg_separator_check_done;
	  }
	  break;
	case ARGSEPARATOR:
	  ++n_arg_separators;
	  break;
	case ELLIPSIS:
	  mp -> varargs = true;
	  break;
	}
    }
    if (!mp -> varargs && !mp -> prefix && n_arg_separators == 0) {
      int void_idx, void_next_idx;
      if ((void_idx = nextlangmsg (messages, _open_arg_idx))
	  != ERROR) {
	if (M_TOK(messages[void_idx]) == LABEL) {
	  if (str_eq (messages[void_idx] -> name, "void")) {
	    if ((void_next_idx = nextlangmsg (messages, void_idx))
		!= ERROR) {
	      if (M_TOK(messages[void_next_idx]) == CLOSEPAREN) {
		n_arg_separators = -1;
	      }
	    }
	  } 
	}
      }
    }
    mp -> n_params = n_arg_separators + 1;
#else
    mp -> n_params = ANY_ARGS;
#endif /* USE_PROTO_PARAMS */

    if ((_close_block_idx = 
	 find_function_close (messages, _close_arg_idx, end_idx)) == ERROR)
      _error ("%s:%d:Could not find method closing brace.\n",
	      c -> name, messages[_close_arg_idx] -> error_line);

    toks2str (messages, _class_idx, _close_block_idx, d_buf);

    make_proto_tag 
      (M_NAME(messages[_class_idx]),
       ((m -> attrs & TOK_IS_INSTANCE_METHOD_KEYWORD) ?
	"instance" : "class"),
       M_NAME(messages[_selector_idx]), tag);
    s_buf = (char *)malloc (strlen (d_buf) + tag_format_length + 1);
    strcatx (s_buf, tag, d_buf, NULL);
    mp -> src = s_buf;

    _hash_put (proto_selectors, M_NAME(messages[_selector_idx]),
	       M_NAME(messages[_selector_idx]));
    if (!c -> proto) {
      c -> proto = c -> proto_head = mp;
    } else {
      c -> proto_head -> next = mp;
      mp -> prev = c -> proto_head;
      c -> proto_head = mp;
    }
    idx = _close_block_idx;
  }

  return SUCCESS;
}

bool is_proto_selector (char *selector) {
  return (_hash_get (proto_selectors, selector) ? true : false);
}

#define INPUT_DECLARATION input_declarations[input_declarations_ptr+1]

int is_method_proto (OBJECT* class_object, char *selector) {

  OBJECT *o;
  char *class_name;
  int lib_ptr;
  METHOD_PROTO *m;
  int i;
  char instancemethod_tag[MAXMSG], classmethod_tag[MAXMSG];
  int instancemethod_tag_length, classmethod_tag_length;

  if (!IS_OBJECT(class_object))
    return FALSE;

  class_name = IS_CLASS_OBJECT(class_object) ? class_object -> __o_name :
    class_object -> __o_class -> __o_name;

  make_proto_tag (class_name, "instance", selector, instancemethod_tag);
  instancemethod_tag_length = tag_format_length;

  make_proto_tag (class_name, "class", selector, classmethod_tag);
  classmethod_tag_length = tag_format_length;
  
  for (i = lib_includes_ptr + 1; i <= MAXARGS; i++) {
    if (str_eq (lib_includes[i]->name, class_name)) {
      for (m = lib_includes[i]->proto; m; m = m -> next) {

	if (!strncmp (m -> src, instancemethod_tag, instancemethod_tag_length))
	  return TRUE;
	if (!strncmp (m -> src, classmethod_tag, classmethod_tag_length))
	  return TRUE;

      }
    }
  }

  if (input_declarations_ptr < MAXARGS) {
    if (str_eq (INPUT_DECLARATION -> name, class_name)) {
      for (m = INPUT_DECLARATION->proto; m; m = m -> next) {

	if (!strncmp (m -> src, instancemethod_tag,
		      instancemethod_tag_length))
	  return TRUE;
	if (!strncmp (m -> src, classmethod_tag,
		      classmethod_tag_length))
	  return TRUE;


      }
    }
  }

  /* If we reach here, check the superclasses. */
  for (o = class_object -> __o_superclass; o;
       o = o -> __o_superclass) {

    for (lib_ptr = lib_includes_ptr + 1; 
	 lib_ptr <= MAXARGS; lib_ptr++) {
      
      if (str_eq (lib_includes[lib_ptr] -> name, 
		  o -> __o_name)) {
	
	make_proto_tag (o -> __o_name, "instance", selector,
			instancemethod_tag);
	instancemethod_tag_length = tag_format_length;
	make_proto_tag (o -> __o_name, "class", selector,
			classmethod_tag);
	classmethod_tag_length = tag_format_length;
	
	for (m = lib_includes[lib_ptr] ->proto; m; m = m -> next) {

	  if (!strncmp (m -> src, instancemethod_tag,
			instancemethod_tag_length))
	    return TRUE;
	  if (!strncmp (m -> src, classmethod_tag,
			classmethod_tag_length))
	    return TRUE;
	}
      }
    }
  }

  return FALSE;
}

/* This is still needed as an independent fn, even though
   is_method_proto, above, also checks the superclasses. */
int is_superclass_method_proto (char *classname, char *name) {
  METHOD_PROTO *m;

  OBJECT *class_object, *o;
  int lib_ptr;
  char instancemethod_tag[MAXMSG], classmethod_tag[MAXMSG];
  int instancemethod_tag_length, classmethod_tag_length;

  if ((class_object = get_class_object (classname)) != NULL) {

    for (o = class_object -> __o_superclass; o;
	 o = o -> __o_superclass) {

      for (lib_ptr = lib_includes_ptr + 1; 
	   lib_ptr <= MAXARGS; lib_ptr++) {

	if (str_eq (lib_includes[lib_ptr] -> name, 
		    o -> __o_name)) {

	  make_proto_tag (o -> __o_name, "instance", name,
			  instancemethod_tag);
	  instancemethod_tag_length = tag_format_length;
	  make_proto_tag (o -> __o_name, "class", name,
			  classmethod_tag);
	  classmethod_tag_length = tag_format_length;

	  for (m = lib_includes[lib_ptr] ->proto; m; m = m -> next) {

	    if (!strncmp (m -> src, instancemethod_tag,
			  instancemethod_tag_length))
	      return TRUE;
	    if (!strncmp (m -> src, classmethod_tag,
			  classmethod_tag_length))
	      return TRUE;
	  }
	}
      }
    }
  }

  return FALSE;
}

int method_proto_is_output (char *selector) {
  LIST *l;
  for (l = output_method_prototypes; l; l = l -> next) {
    if (str_eq ((char *)l -> data, selector))
	return TRUE;
  }
  return FALSE;
}

char *method_proto (char *classname, char *selector) {
  METHOD_PROTO *m;
  int i;
  char instancemethod_tag[MAXMSG], classmethod_tag[MAXMSG];
  int instancemethod_tag_length, classmethod_tag_length;

  make_proto_tag (classname, "instance", selector,
		  instancemethod_tag);
  instancemethod_tag_length = tag_format_length;
  make_proto_tag (classname, "class", selector,
		  classmethod_tag);
  classmethod_tag_length = tag_format_length;

  for (i = lib_includes_ptr + 1; i <= MAXARGS; i++) {
    if (str_eq (lib_includes[i]->name, classname)) {

      for (m = lib_includes[i]->proto; m; m = m -> next) {

	if (!strncmp (m -> src, instancemethod_tag,
		      instancemethod_tag_length))
	  return m -> src;
	if (!strncmp (m -> src, classmethod_tag,
		      classmethod_tag_length))
	  return m -> src;
      }
    }
  }

  for (m = INPUT_DECLARATION->proto; m; m = m -> next) {
    if (str_eq (INPUT_DECLARATION -> name, classname)) {

	if (!strncmp (m -> src, instancemethod_tag,
		      instancemethod_tag_length))
	  return m -> src;
	if (!strncmp (m -> src, classmethod_tag,
		      classmethod_tag_length))
	  return m -> src;

    }

  }

  return NULL;
}

void input_prototypes (char *fn) {
  method_prototypes (fn, input_declarations[input_declarations_ptr+1]);
}

extern OBJECT *rcvr_class_obj;         /* Declared in lib/rtnwmthd.c */
extern NEWMETHOD *new_methods[MAXARGS+1];  
extern int new_method_ptr;

OBJECT *method_from_prototype (char *methodname) {

  int stack_start;
  char *proto_buf;
  char tmpbuf[MAXMSG],    /* Buffer for collected method body tokens.      */
    *srcbuf;              /* Buffer for method with translated declaration.*/
    int method_msg_ptr,
    param_start_ptr,     /* Index of parameter start.                     */
    param_end_ptr,       /* Index of parameter end.                       */
    method_start_ptr,    /* Index of function body open block.            */
    method_end_ptr,      /* End of the method body.                       */
    alias_ptr,           /* Index of the method alias, if any.            */
    funcname_ptr,        /* Index of the function name.                   */
    n_params,            /* Number of parameters in declaration.          */
    i,
    buflength;
  int have_c_param = 0;
  int x_chars_read;
  MESSAGE *m_method;  /* Message of the, "method," keyword. */

  METHOD  *n_method,
    *tm,
    *rcvr_method;

  PARAMLIST_ENTRY *params[MAXARGS];
  char c_param[MAXMSG],
    funcname[MAXLABEL],
    alias[MAXLABEL],
    tmpname[FILENAME_MAX],  /* For method buffering. */
    *x_srcbuf, *src_start;

  CVAR *__cvar_dup;
  CFUNC *__shadowed_c_fn;
  
  I_PASS prev_pass;
  struct stat statbuf;

  method_from_proto = TRUE;
  stack_start = r_messageptr; /* Before lexing, points to the
				 start of the stack. */

  if ((proto_buf = method_proto (rcvr_class_obj -> __o_name, methodname))
      == NULL) 
    return NULL;

  src_start = strstr (proto_buf, ":::");
  src_start += 3;

  (void) tokenize_reuse (r_message_push, src_start);

  if ((method_msg_ptr = nextlangmsg (r_messages, stack_start)) == ERROR) {
    _error ("Parser error.\n");
  } else {
    m_method = r_messages[method_msg_ptr];
    m_method -> receiver_obj = r_messages[stack_start] -> obj;
    m_method -> tokentype = METHODMSGLABEL;
  }

  /* 
   *  Find the number of args required by the method declaration.
   *  TO DO - Determine if we can use method_declaration_info ()
   *  here also - we only need n_params here.
   */

   if ((method_end_ptr = 
	find_function_close (r_messages, method_msg_ptr, r_messageptr))
       == ERROR) {
     int e_prev_idx, e_next_idx;
     MESSAGE *m_error_rcvr, *m_error_name;
     if ((e_prev_idx = prevlangmsg (r_messages,
				    method_msg_ptr)) != ERROR) {
       m_error_rcvr = r_messages[e_prev_idx];
     } else {
       m_error_rcvr = NULL;
     }
     if ((e_next_idx = nextlangmsg (r_messages,
				    method_msg_ptr)) != ERROR) {
       m_error_name = r_messages[e_next_idx];
     } else {
       m_error_name = NULL;
     }
     error (m_method, "%s method %s : Can't find method close.",
 	   (m_error_rcvr ? M_NAME(m_error_rcvr) : ""),
 	   (m_error_name ? M_NAME(m_error_name) : ""));
   }

   if ((param_start_ptr = 
        scanforward (r_messages, method_msg_ptr,
 		    method_end_ptr, OPENPAREN)) == ERROR)
     error (m_method, "Missing parameters in method declaration.");

   if ((param_end_ptr = scanforward (r_messages, param_start_ptr,
 				    method_end_ptr, CLOSEPAREN)) == ERROR)
     error (m_method, "Missing parameters in method declaration.");

   if ((method_start_ptr = scanforward (r_messages, param_start_ptr,
					method_end_ptr, OPENBLOCK)) == ERROR)
     error (m_method, "Missing start of method body.");


   if ((n_method = (METHOD *)__xalloc (sizeof (METHOD))) == NULL)
     _error ("method_from_prototype: %s.", strerror (errno));
   n_method -> sig = METHOD_SIG;

   new_methods[new_method_ptr--] = create_newmethod_init (n_method);

   fn_param_declarations (r_messages, param_start_ptr, param_end_ptr,
			  params, &n_params);

  /*
   *   Reversing the order of the parameters here helps to 
   *   ensure that the run-time code pushes the arguments 
   *   in the correct order for nested method calls.
   */
   n_method -> n_params = n_params;

   for (i = n_params - 1; i >= 0; i--) {
     /* TO DO - Possibly add error checking in method_param (). 
	See the comment below. */
     /* TO DO - Make sure that method_param has the correct
	error line, column, and source file name. */
     n_method -> params[i] = method_param_tok (r_messages, params[i]);

     if (! param_class_exists (n_method -> params[i] -> class)) {
      /*
       *  NOTE - This should be handled as an exception in
       *  a future release.  (Remember to declare this again
       *  at the start of the function.)
       */
       /* param_class_exception = i; */
     }
     if ((i == 0) && 
	 (n_method -> params[i] -> attrs & PARAM_C_PARAM)) {
       char *s;
       s = strchr (params[i] -> buf, ' ');
       have_c_param = TRUE;
       strcpy (c_param, s);
     }
     if (n_method -> params[i] -> attrs & PARAM_VARARGS_PARAM)
       n_method -> varargs = TRUE;
     __xfree (MEMADDR(params[i]));
   }

   method_declaration_msg (r_messages, method_msg_ptr, param_start_ptr,
			   funcname, &funcname_ptr, alias, &alias_ptr);

#ifdef DEBUG_UNDEFINED_PARAMETER_CLASSES
   if (param_class_exception != ERROR)
     warning (r_messagesmethod_msg_ptr],
       "Undefined parameter class %s. (Method %s, Class %s).",
       n_method -> params[param_class_exception] -> class,
       ((*alias) ? alias : funcname),
       rcvr_class_obj -> __o_name);
#endif

     strcpy (n_method -> name, ((*alias) ? alias : funcname));
     n_method -> n_args = 0;
     n_method -> error_line = m_method -> error_line;
     n_method -> error_column = m_method -> error_column;
  /* This is a kludge - all methods are marked as imported,
     until we can get the method buffering straightened out. */
     n_method -> imported = True;
     n_method -> queued = False;
  
     strcpy (n_method -> selector, 
	make_selector (rcvr_class_obj, n_method, funcname, i_or_c_instance));

     strcpy (n_method -> returnclass, rcvr_class_obj -> __o_name);

     for (rcvr_method = rcvr_class_obj -> instance_methods; rcvr_method;
	  rcvr_method = rcvr_method -> next) {
    if (str_eq (rcvr_method -> name, n_method -> name) && 
	(rcvr_method -> n_params == n_method -> n_params) &&
	(rcvr_method -> prefix == n_method -> prefix) &&
	(rcvr_method -> varargs == n_method -> varargs) &&
	(rcvr_method -> no_init == n_method -> no_init)) {
    
	 warning (m_method, "Redefinition of method, \"%s.\" (Class %s).",
		  n_method -> name, rcvr_class_obj -> __o_name);

	 delete_newmethod (new_methods[new_method_ptr+1]);
	 new_methods[new_method_ptr++] = NULL;
	 __xfree (MEMADDR(n_method));
	 return NULL;

       }
     }

     generate_instance_method_definition_call (rcvr_class_obj -> __o_name,
					       ((*alias) ? alias : funcname),
					       n_method -> selector, n_params, 
				       method_definition_attributes (n_method));

   for (i = 0; i < n_params; i++) {
     if (!(n_method -> params[i] -> attrs & PARAM_C_PARAM))
       generate_instance_method_param_definition_call (rcvr_class_obj -> __o_name,
 					   ((*alias) ? alias : funcname),
				       n_method -> selector,
 					   n_method -> params[i] -> name,
 					   n_method -> params[i] -> class,
 					   n_method -> params[i] -> is_ptr,
				       n_method -> params[i] -> is_ptrptr
						       );
   }

   if ((__cvar_dup = _hash_remove (declared_global_variables,
 				  n_method -> name)) != NULL) {
     warning (message_stack_at(method_msg_ptr),
	      "Method %s shadows a global variable.", n_method -> name);
     unlink_global_cvar (__cvar_dup);
     _delete_cvar (__cvar_dup);
   }

   if ((__shadowed_c_fn = get_function (n_method -> name)) != NULL) {
       warning (message_stack_at(method_msg_ptr),
 	       "Method %s shadows a function.", n_method -> name);
   }

  /* Collect tokens from the end of the ctalk declaration to the end of
     the method.
  */

   toks2str (r_messages, method_start_ptr, method_end_ptr, tmpbuf);

   if (have_c_param) {
     buflength = METHOD_RETURN_LENGTH + 2 + /* 2 spaces */
       strlen (c_param) + 5 +               /*  strlen (" ( ) ") */
       strlen (n_method -> selector) + strlen (tmpbuf);
   } else {
     buflength = METHOD_RETURN_LENGTH + 3 + /* 3 spaces */
       METHOD_C_PARAM_LENGTH + 
       strlen (n_method -> selector) + strlen (tmpbuf);
   }

if ((srcbuf = (char *)__xalloc (buflength + 1)) == NULL)
     _error ("new_instance_method: %s.", strerror (errno));

   if (n_method -> params[0] &&
       (n_method -> params[0] -> attrs & PARAM_C_PARAM)) {
     strcatx (srcbuf, METHOD_RETURN, " ",
	      n_method -> selector, " (", 
	      c_param, ") ", tmpbuf, NULL);
    } else {
     strcatx (srcbuf, METHOD_RETURN, " ",
	      n_method -> selector, " ",
	      METHOD_C_PARAM, " ",
	      tmpbuf, NULL);
	      
    }

   prev_pass = interpreter_pass;
   interpreter_pass = method_pass;

  /*
    TO DO -
    1. Make sure that methods can be resolved as externs - and also
       class declarations, which might be initialized in a method's
       init block.
  */

   create_tmp ();
   begin_method_buffer (n_method);
   parse (srcbuf, (long long) strlen (srcbuf));
   end_method_buffer ();
   unbuffer_method_statements ();

   close_tmp ();
   generate_instance_method_return_class_call (rcvr_class_obj -> __o_name,
 				     n_method -> name,
					       n_method -> selector,
					       n_method -> returnclass,
					       n_method -> n_params);
   strcpy (tmpname, get_tmpname ());
   stat (tmpname, &statbuf);
if ((x_srcbuf = (char *) __xalloc (statbuf.st_size + 1)) == NULL)
     error (m_method, "new_instance_method %s (class %s): %s.",
 	   n_method -> name, rcvr_class_obj -> __o_name,
 	   strerror (errno));
#ifdef __DJGPP__
     errno = 0;
     x_chars_read = read_file (x_srcbuf, tmpname);
     if (errno)
     error (m_method, "new_instance_method %s (class %s): %s.",
 	   n_method -> name, rcvr_class_obj -> __o_name,
	    strerror (errno));
#else
if ((x_chars_read = read_file (x_srcbuf, tmpname)) != statbuf.st_size)
     error (m_method, "new_instance_method %s (class %s): %s.",
 	   n_method -> name, rcvr_class_obj -> __o_name,
 	   strerror (errno));
#endif
   n_method -> src = strdup (x_srcbuf);
   __xfree (MEMADDR(x_srcbuf));

   queue_method_for_output (rcvr_class_obj, n_method);

   if (!library_input) {

    /* TO DO - 
       Inform the parent parser simply to skip the frames of the 
       source method, if possible, without adding another state to 
       the parser. */
     for (i = stack_start; i >= method_end_ptr; i--) {
       MESSAGE *m_tmp;
        m_tmp = r_messages[i];
        if ((m_tmp -> tokentype != WHITESPACE) &&
 	   (m_tmp -> tokentype != NEWLINE)) {
 	 m_tmp -> tokentype = WHITESPACE;
 	 strcpy (m_tmp -> name, " ");
        }
     }
   }

   if (rcvr_class_obj -> instance_methods == NULL) {
     rcvr_class_obj -> instance_methods = n_method;
   } else {
     for (tm = rcvr_class_obj -> instance_methods; tm -> next; tm = tm -> next)
       ;
     tm -> next = n_method;
     n_method -> prev = tm;
   }

   _hash_put (declared_method_names, n_method -> name, n_method -> name);
   _hash_put (declared_method_selectors, n_method -> selector, n_method -> selector);

   if (output_method_prototypes == NULL) {
     output_method_prototypes = new_list ();
     output_method_prototypes -> data = strdup (n_method -> selector);
   } else {
     LIST *l;
     l = new_list ();
     l -> data = strdup (n_method -> selector);
     list_push (&output_method_prototypes, &l);
   }

   unlink_tmp ();

   interpreter_pass = prev_pass;

   new_methods[++new_method_ptr] = NULL;
  
   REUSE_MESSAGES(r_messages,r_messageptr,N_R_MESSAGES)

   __xfree (MEMADDR(srcbuf));

   method_from_proto = FALSE;
   return NULL;
}

/* this should eventually replace each call to method_from_prototype (), 
   when we have an example of receiver class objects' name collisions. */
OBJECT *method_from_prototype_2 (OBJECT *class_obj, char *methodname) {

  int stack_start;
  char *proto_buf;
  char tmpbuf[MAXMSG],    /* Buffer for collected method body tokens.      */
    *srcbuf;              /* Buffer for method with translated declaration.*/
    int method_msg_ptr,
    param_start_ptr,     /* Index of parameter start.                     */
    param_end_ptr,       /* Index of parameter end.                       */
    method_start_ptr,    /* Index of function body open block.            */
    method_end_ptr,      /* End of the method body.                       */
    alias_ptr,           /* Index of the method alias, if any.            */
    funcname_ptr,        /* Index of the function name.                   */
    n_params,            /* Number of parameters in declaration.          */
    i,
    buflength;
  int have_c_param = 0;
  int x_chars_read;
  struct stat statbuf;
  MESSAGE *m_method;  /* Message of the, "method," keyword. */

  METHOD  *n_method,
    *tm,
    *rcvr_method;

  PARAMLIST_ENTRY *params[MAXARGS];
  char c_param[MAXMSG],
    funcname[MAXLABEL],
    alias[MAXLABEL],
    tmpname[FILENAME_MAX],  /* For method buffering. */
    *x_srcbuf, *src_start;

  CVAR *__cvar_dup;
  CFUNC *__shadowed_c_fn;
  
  I_PASS prev_pass;

  method_from_proto = TRUE;
  stack_start = r_messageptr; /* before lexing, points to the start of
				the stack */

  if ((proto_buf = method_proto (class_obj -> __o_name, methodname))
      == NULL) 
    return NULL;

  src_start = strstr (proto_buf, ":::");
  src_start += 3;

  (void) tokenize_reuse (r_message_push, src_start);

  if ((method_msg_ptr = nextlangmsg (r_messages, stack_start)) == ERROR) {
    _error ("Parser error.\n");
  } else {
    m_method = r_messages[method_msg_ptr];
    m_method -> receiver_obj = r_messages[stack_start] -> obj;
    m_method -> tokentype = METHODMSGLABEL;
  }

  /* 
   *  Find the number of args required by the method declaration.
   *  TO DO - Determine if we can use method_declaration_info ()
   *  here also - we only need n_params here.
   */

   if ((method_end_ptr = 
	find_function_close (r_messages, method_msg_ptr, r_messageptr))
       == ERROR) {
     int e_prev_idx, e_next_idx;
     MESSAGE *m_error_rcvr, *m_error_name;
     if ((e_prev_idx = prevlangmsg (r_messages,
				    method_msg_ptr)) != ERROR) {
       m_error_rcvr = r_messages[e_prev_idx];
     } else {
       m_error_rcvr = NULL;
     }
     if ((e_next_idx = nextlangmsg (r_messages,
				    method_msg_ptr)) != ERROR) {
       m_error_name = r_messages[e_next_idx];
     } else {
       m_error_name = NULL;
     }
     error (m_method, "%s method %s : Can't find method close.",
 	   (m_error_rcvr ? M_NAME(m_error_rcvr) : ""),
 	   (m_error_name ? M_NAME(m_error_name) : ""));
   }

   if ((param_start_ptr = 
        scanforward (r_messages, method_msg_ptr,
 		    method_end_ptr, OPENPAREN)) == ERROR)
     error (m_method, "Missing parameters in method declaration.");

   if ((param_end_ptr = scanforward (r_messages, param_start_ptr,
 				    method_end_ptr, CLOSEPAREN)) == ERROR)
     error (m_method, "Missing parameters in method declaration.");

   if ((method_start_ptr = scanforward (r_messages, param_start_ptr,
					method_end_ptr, OPENBLOCK)) == ERROR)
     error (m_method, "Missing start of method body.");


   if ((n_method = (METHOD *)__xalloc (sizeof (METHOD))) == NULL)
     _error ("method_from_prototype_2: %s.", strerror (errno));
   n_method -> sig = METHOD_SIG;

   new_methods[new_method_ptr--] = create_newmethod_init (n_method);

   fn_param_declarations (r_messages, param_start_ptr, param_end_ptr,
			  params, &n_params);

  /*
   *   Reversing the order of the parameters here helps to 
   *   ensure that the run-time code pushes the arguments 
   *   in the correct order for nested method calls.
   */
   n_method -> n_params = n_params;

   for (i = n_params - 1; i >= 0; i--) {
     /* TO DO - Possibly add error checking in method_param (). 
	See the comment below. */
     /* TO DO - Make sure that method_param has the correct
	error line, column, and source file name. */
     n_method -> params[i] = method_param_tok (r_messages, params[i]);

     if (! param_class_exists (n_method -> params[i] -> class)) {
      /*
       *  NOTE - This should be handled as an exception in
       *  a future release.  (Remember to declare this again
       *  at the start of the function.)
       */
       /* param_class_exception = i; */
     }
     if ((i == 0) && 
	 (n_method -> params[i] -> attrs & PARAM_C_PARAM)) {
       char *s;
       s = strchr (params[i] -> buf, ' ');
       have_c_param = TRUE;
       strcpy (c_param, s);
     }
     if (n_method -> params[i] -> attrs & PARAM_VARARGS_PARAM)
       n_method -> varargs = TRUE;
     __xfree (MEMADDR(params[i]));
   }

   method_declaration_msg (r_messages, method_msg_ptr, param_start_ptr,
			   funcname, &funcname_ptr, alias, &alias_ptr);

#ifdef DEBUG_UNDEFINED_PARAMETER_CLASSES
   if (param_class_exception != ERROR)
     warning (r_messagesmethod_msg_ptr],
       "Undefined parameter class %s. (Method %s, Class %s).",
       n_method -> params[param_class_exception] -> class,
       ((*alias) ? alias : funcname),
       class_obj -> __o_name);
#endif

     strcpy (n_method -> name, ((*alias) ? alias : funcname));
     n_method -> n_args = 0;
     n_method -> error_line = m_method -> error_line;
     n_method -> error_column = m_method -> error_column;
  /* This is a kludge - all methods are marked as imported,
     until we can get the method buffering straightened out. */
     n_method -> imported = True;
     n_method -> queued = False;
  
     strcpy (n_method -> selector, 
	make_selector (class_obj, n_method, funcname, i_or_c_instance));

     strcpy (n_method -> returnclass, class_obj -> __o_name);

     for (rcvr_method = class_obj -> instance_methods; rcvr_method;
	  rcvr_method = rcvr_method -> next) {
    if (str_eq (rcvr_method -> name, n_method -> name) && 
	(rcvr_method -> n_params == n_method -> n_params) &&
	(rcvr_method -> prefix == n_method -> prefix) &&
	(rcvr_method -> varargs == n_method -> varargs) &&
	(rcvr_method -> no_init == n_method -> no_init)) {
    
	 warning (m_method, "Redefinition of method, \"%s.\" (Class %s).",
		  n_method -> name, class_obj -> __o_name);

	 delete_newmethod (new_methods[new_method_ptr+1]);
	 new_methods[new_method_ptr++] = NULL;
	 __xfree (MEMADDR(n_method));
	 return NULL;

       }
     }

     generate_instance_method_definition_call (class_obj -> __o_name,
					       ((*alias) ? alias : funcname),
					       n_method -> selector, n_params, 
				       method_definition_attributes (n_method));

   for (i = 0; i < n_params; i++) {
     if (!(n_method -> params[i] -> attrs & PARAM_C_PARAM))
       generate_instance_method_param_definition_call (class_obj -> __o_name,
 					   ((*alias) ? alias : funcname),
				       n_method -> selector,
 					   n_method -> params[i] -> name,
 					   n_method -> params[i] -> class,
 					   n_method -> params[i] -> is_ptr,
				       n_method -> params[i] -> is_ptrptr
						       );
   }

   if ((__cvar_dup = _hash_remove (declared_global_variables,
 				  n_method -> name)) != NULL) {
     warning (message_stack_at(method_msg_ptr),
	      "Method %s shadows a global variable.", n_method -> name);
     unlink_global_cvar (__cvar_dup);
     _delete_cvar (__cvar_dup);
   }

   if ((__shadowed_c_fn = get_function (n_method -> name)) != NULL) {
       warning (message_stack_at(method_msg_ptr),
 	       "Method %s shadows a function.", n_method -> name);
   }

  /* Collect tokens from the end of the ctalk declaration to the end of
     the method.
  */

toks2str (r_messages, method_start_ptr, method_end_ptr, tmpbuf);

   if (have_c_param) {
     buflength = METHOD_RETURN_LENGTH + 2 + /* 2 spaces */
       strlen (c_param) + 5 +               /*  strlen (" ( ) ") */
       strlen (n_method -> selector) + strlen (tmpbuf);
   } else {
     buflength = METHOD_RETURN_LENGTH + 3 + /* 3 spaces */
       METHOD_C_PARAM_LENGTH + 
       strlen (n_method -> selector) + strlen (tmpbuf);
   }

if ((srcbuf = (char *)__xalloc (buflength + 1)) == NULL)
     _error ("new_instance_method: %s.", strerror (errno));

   if (n_method -> params[0] &&
       (n_method -> params[0] -> attrs & PARAM_C_PARAM)) {
     strcatx (srcbuf, METHOD_RETURN, " ",
	      n_method -> selector, " (", 
	      c_param, ") ", tmpbuf, NULL);
    } else {
     strcatx (srcbuf, METHOD_RETURN, " ",
	      n_method -> selector, " ",
	      METHOD_C_PARAM, " ",
	      tmpbuf, NULL);
    }

   prev_pass = interpreter_pass;
   interpreter_pass = method_pass;

  /*
    TO DO -
    1. Make sure that methods can be resolved as externs - and also
       class declarations, which might be initialized in a method's
       init block.
  */

   create_tmp ();
   begin_method_buffer (n_method);
   parse (srcbuf, (long long) strlen (srcbuf));
   end_method_buffer ();
   unbuffer_method_statements ();

   close_tmp ();
   

   generate_instance_method_return_class_call (class_obj -> __o_name,
 				     n_method -> name,
					       n_method -> selector,
					       n_method -> returnclass,
					       n_method -> n_params);
   strcpy (tmpname, get_tmpname ());
stat (tmpname, &statbuf);
if ((x_srcbuf = (char *) __xalloc (statbuf.st_size + 1)) == NULL)
     error (m_method, "new_instance_method %s (class %s): %s.",
 	   n_method -> name, class_obj -> __o_name,
 	   strerror (errno));
#ifdef __DJGPP__
     errno = 0;
     x_chars_read = read_file (x_srcbuf, tmpname);
     if (errno)
     error (m_method, "new_instance_method %s (class %s): %s.",
 	   n_method -> name, class_obj -> __o_name,
	    strerror (errno));
#else
   if ((x_chars_read = read_file (x_srcbuf, tmpname)) != statbuf.st_size)
     error (m_method, "new_instance_method %s (class %s): %s.",
 	   n_method -> name, class_obj -> __o_name,
 	   strerror (errno));
#endif
   n_method -> src = strdup (x_srcbuf);
   __xfree (MEMADDR(x_srcbuf));

   queue_method_for_output (class_obj, n_method);

   if (!library_input) {

    /* TO DO - 
       Inform the parent parser simply to skip the frames of the 
       source method, if possible, without adding another state to 
       the parser. */
     for (i = stack_start; i >= method_end_ptr; i--) {
       MESSAGE *m_tmp;
        m_tmp = r_messages[i];
        if ((m_tmp -> tokentype != WHITESPACE) &&
 	   (m_tmp -> tokentype != NEWLINE)) {
 	 m_tmp -> tokentype = WHITESPACE;
 	 strcpy (m_tmp -> name, " ");
        }
     }
   }

   if (class_obj -> instance_methods == NULL) {
     class_obj -> instance_methods = n_method;
   } else {
     for (tm = class_obj -> instance_methods; tm -> next; tm = tm -> next)
       ;
     tm -> next = n_method;
     n_method -> prev = tm;
   }

   _hash_put (declared_method_names, n_method -> name, n_method -> name);
   _hash_put (declared_method_selectors, n_method -> selector, n_method -> selector);

   if (output_method_prototypes == NULL) {
     output_method_prototypes = new_list ();
     output_method_prototypes -> data = strdup (n_method -> selector);
   } else {
     LIST *l;
     l = new_list ();
     l -> data = strdup (n_method -> selector);
     list_push (&output_method_prototypes, &l);
   }

   unlink_tmp ();

   interpreter_pass = prev_pass;

   new_methods[++new_method_ptr] = NULL;
  
   REUSE_MESSAGES(r_messages,r_messageptr,N_R_MESSAGES)

   __xfree (MEMADDR(srcbuf));

   method_from_proto = FALSE;
   return NULL;
}

int this_method_from_proto (char *classname, char *name) {
  if (new_method_ptr < MAXARGS) {
    if (!strcmp (classname, rcvr_class_obj->__o_name) &&
	!strcmp (name, new_methods[new_method_ptr+1]->method->name))
      return TRUE;
  }  
  return FALSE;
}

LIST *user_prototypes = NULL;

bool have_user_prototype (char *rcvr_classname, char *name)  {
  LIST *l;
  UPROTO *proto;

  if (user_prototypes) {
    for (l = user_prototypes; l; l = l -> next) {
      proto = (UPROTO *)l -> data;
      if (str_eq (proto -> classname, rcvr_classname)) {
	if (proto -> alias[0]) {
	  if (str_eq (proto -> alias, name)) {
	    return true;
	  }
	} else {
	  if (str_eq (proto -> name, name)) {
	    return true;
	  }
	}
      }
    }
  }
  return false;
}

void add_user_prototype (MESSAGE_STACK messages, 
			 int method_decl_start_idx, 
			 int param_start_ptr,
			 int param_end_ptr,
			 int end_idx) {
  UPROTO *proto;
  int i, name_idx, keyword_idx, alias_idx, returnclass_idx;
  LIST *l, *l_param;
  PARAMLIST_ENTRY *params[MAXARGS];
  int n_params;
  PARAM *p;

  if ((proto = (UPROTO *) __xalloc (sizeof (struct _uproto)))
      == NULL) 
    _error ("add_user_prototype: %s\n", strerror (errno));

  strcpy (proto -> classname, M_NAME(messages[method_decl_start_idx]));

  if ((keyword_idx = nextlangmsg (messages, method_decl_start_idx))
      == ERROR)
    _error ("add_user_prototype: missing keyword.\n");

  if ((name_idx = nextlangmsg (messages, keyword_idx)) == ERROR)
    _error ("add_user_prototype: missing method name.\n");
  strcpy (proto -> name, M_NAME(messages[name_idx]));

  if ((alias_idx = nextlangmsg (messages, name_idx)) == ERROR)
    _error ("add_user_prototype: missing argument.\n");
  if (M_TOK(messages[alias_idx]) != OPENPAREN) {
    strcpy (proto -> alias, M_NAME(messages[alias_idx]));
  }

  if ((returnclass_idx = prevlangmsg (messages, end_idx)) != ERROR) {
    if (M_TOK(messages[returnclass_idx]) == LABEL)  {
      strcpy (proto -> returnclass, M_NAME(messages[returnclass_idx]));
    }
  }

  fn_param_declarations (messages, param_start_ptr, param_end_ptr,
			 params, &n_params);

  for (i = n_params - 1; i >= 0; i--) {
    /* TO DO - Possibly add error checking in method_param (). 
       See the comment below. */
    /* TO DO - Make sure that method_param has the correct
       error line, column, and source file name. */
    p = method_param_tok (messages, params[i]);
    l_param = new_list ();
    l_param -> data = (void *)p;
    if (proto -> params == NULL) {
      proto -> params = l_param;
    } else {
      list_push (&(proto -> params), &l_param);
    }
    __xfree (MEMADDR(params[i]));
  }


  l = new_list ();

  l -> data = (void *)proto;

  if (user_prototypes == NULL) {
    user_prototypes = l;
  } else {
    list_push (&user_prototypes, &l);
  }

  for (i = method_decl_start_idx; i >= end_idx; i--) {
    ++messages[i] -> evaled;
    ++messages[i] -> output;
  }

}

static void delete_prototype (UPROTO *p) {
  if (p -> params) {
    LIST *t1, *t2;

    for (t1 = p -> params; t1 -> next; t1 = t1 -> next)
      ;

    if (t1 == p -> params) {
      delete_list_element (t1);
      p -> params = NULL;
      __xfree (MEMADDR(p));
      return;
    }

    while (t1 != p -> params) {
      t2 = t1 -> prev;
      delete_list_element (t1);
      if (t2 == p -> params)
	break;
      t1 = t2;
    }
    delete_list_element (p -> params);
    p -> params = NULL;
  }
  __xfree (MEMADDR(p));
}

void cleanup_user_prototypes (void) {
  LIST *t1, *t2;

  if (!user_prototypes)
    return;

  for (t1 = user_prototypes; t1 -> next; t1 = t1 -> next)
    ;

  if (t1 == user_prototypes) {
    delete_prototype ((UPROTO *)t1 -> data);
    t1 -> data = NULL;
    delete_list_element (t1);
    return;
  }

  while (t1 != user_prototypes) {
     t2 = t1 -> prev;
     delete_prototype ((UPROTO *)t1 -> data);
     t1 -> data = NULL;
     delete_list_element (t1);
     if (t2 == user_prototypes)
       break;
     t1 = t2;
   }
   delete_prototype ((UPROTO *)user_prototypes -> data);
   user_prototypes -> data = NULL;
   delete_list_element (user_prototypes);
}

/* Makes a prototype tag with the format: 

    "%s_%s_%s:::" <classname>, "_instance_" | "_class_", <selector>
*/

static void make_proto_tag (char *class, char *i_or_c_keywd, char *name,
			     char *buf_out) {
  int i = 0, j = 0;
  while (class[j])
    buf_out[i++] = class[j++];
  buf_out[i++] = '_';
  j = 0;
  while (i_or_c_keywd[j])
    buf_out[i++] = i_or_c_keywd[j++];
  buf_out[i++] = '_';
  j = 0;
  while (name[j])
    buf_out[i++] = name[j++];
  buf_out[i++] = ':'; buf_out[i++] = ':'; buf_out[i++] = ':';
  tag_format_length = i;
  buf_out[i] = '\0';
}

/* as above, but always uses the label identifier in the tag - 
   omits the separator, too, and adds the "_v", "_p", or "_<n>"
   parameter specifier. */
static void make_preload_proto_tag (char *class, char *i_or_c_keywd,
				    char *name, bool prefix,
				    bool vararg, int n_params,
				    char *buf_out) {
  int i = 0, j = 0;
  while (class[j])
    buf_out[i++] = class[j++];
  buf_out[i++] = '_';
  j = 0;
  while (i_or_c_keywd[j])
    buf_out[i++] = i_or_c_keywd[j++];
  buf_out[i++] = '_';
  j = 0;
  while (name[j])
    buf_out[i++] = name[j++];
  if (vararg) {
    buf_out[i++] = '_'; buf_out[i++] = 'v'; buf_out[i] = '\0';
  } else if (prefix) {
    buf_out[i++] = '_'; buf_out[i++] = 'p'; buf_out[i] = '\0';
  } else {
    buf_out[i++] = '_';
    buf_out[i] = '\0';
    strcat (buf_out, ascii[n_params]);
  }
}

bool is_method_proto_name (char *name) {
  int i;

  for (i = lib_includes_ptr + 1; i <= MAXARGS; ++i) {
    if (is_method_proto (get_class_object (lib_includes[i] -> name), name))
      return true;
  }
  return false;
}
