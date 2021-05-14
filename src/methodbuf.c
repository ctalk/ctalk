/* $Id: methodbuf.c,v 1.1.1.1 2021/04/03 11:26:02 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2016, 2020 Robert Kiesling, rk3314042@gmail.com.
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

/*
 *  Essentially the same as fnbuf.c but called when reparsing in 
 *  output_imported_methods.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "list.h"
#include "bufmthd.h"

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c. */

extern int library_input;            /* Declared in class.c.              */
extern I_PASS interpreter_pass;      /* Declared in main.c.               */

extern ARGBLK *argblks[MAXARGS + 2]; /* Declared in argblk.c.             */
extern int argblk_ptr;

BUFFERED_METHOD method_buffer[MAXARGS+1] = {{NULL,},};
int method_buffer_ptr = -1;

int buffer_method_output = FALSE;

int format_method_output = FALSE;

#define MB_LIST_CACHE_MAX 0xffff


static MBLIST *mb_list_cache[MB_LIST_CACHE_MAX];
static int mb_list_cache_ptr = 0;

static MBLIST *create_mb_list (void) {
  MBLIST *l;
  l = (MBLIST *)__xalloc (MAXMSG);
  l -> sig = LIST_SIG;
  return l;
}

static MBLIST *new_mb_list () {
  MBLIST *l;
  if (mb_list_cache_ptr == 0) {
    return create_mb_list ();
  } else {
    l = mb_list_cache[--mb_list_cache_ptr];
    /* mb_list_cache[mb_list_cache_ptr - 1] = NULL; */
    return l;
  }
  return NULL;
}

static void reuse_mb_list (MBLIST *l) {
  if (mb_list_cache_ptr == MB_LIST_CACHE_MAX) {
    __xfree (MEMADDR(l));
  } else {
    l -> data[0] = '\0';
    l -> next = NULL;
    mb_list_cache[mb_list_cache_ptr++] = l;
  }
}

MBLIST *mb_list_unshift (MBLIST **l) {
  MBLIST *t;

  if (!*l) return (MBLIST *)NULL;
  t = *l;
  *l = t -> next;
  return t;
}

void method_init_statement (char *buf) {

  MBLIST *l;

  if ((interpreter_pass == method_pass) && 
      (method_buffer_ptr >= 0)) {

    l = new_mb_list ();
    strcpy (l -> data, buf);

    if (method_buffer[method_buffer_ptr].init == NULL) {
      method_buffer[method_buffer_ptr].init = l;
    } else {
      method_buffer[method_buffer_ptr].init_head -> next = l;
    }
    method_buffer[method_buffer_ptr].init_head = l;
  }
}

void method_vartab_statement (char *buf) {

  MBLIST *l;

  if ((interpreter_pass == method_pass) && 
      (method_buffer_ptr >= 0)) {

    l = new_mb_list ();
    strcpy (l -> data, buf);

    if (method_buffer[method_buffer_ptr].cvar_tab_members == NULL) {
      method_buffer[method_buffer_ptr].cvar_tab_members = l;
    } else {
      method_buffer[method_buffer_ptr].cvar_tab_members_head -> next = l;
    }
    method_buffer[method_buffer_ptr].cvar_tab_members_head = l;
  }
}

void method_vartab_init_statement (char *buf) {

  MBLIST *l;

  if ((interpreter_pass == method_pass) && 
      (method_buffer_ptr >= 0)) {

    l = new_mb_list ();
    strcpy (l -> data, buf);

    if (method_buffer[method_buffer_ptr].cvar_tab_init == NULL) {
      method_buffer[method_buffer_ptr].cvar_tab_init = l;
    } else {
      method_buffer[method_buffer_ptr].cvar_tab_init_head -> next = l;
    }
    method_buffer[method_buffer_ptr].cvar_tab_init_head = l;
  }
}

/*
 *  Buffer method statements only if called during method_pass;
 *  i.e., from new_method ().
 */

void begin_method_buffer (METHOD *method) {
  if (interpreter_pass == method_pass) {
    ++method_buffer_ptr;
    method_buffer[method_buffer_ptr].method = method;
  }
}

void end_method_buffer (void) {
   if (interpreter_pass == method_pass)
     --method_buffer_ptr;
   delete_local_c_object_references ();
   delete_global_c_object_references ();
}

void buffer_method_statement (char *s, int parser_idx) {

  MBLIST *l;

  if (ARGBLK_TOK(parser_idx)) {
    buffer_argblk_stmt (s);
  } else {
    l = new_mb_list ();
    strcpy (l -> data, s);

    if (method_buffer[method_buffer_ptr].src == NULL) {
      method_buffer[method_buffer_ptr].src = l;
    } else {
      method_buffer[method_buffer_ptr].src_head -> next = l;
    }
    method_buffer[method_buffer_ptr].src_head = l;
  }
}

MESSAGE *method_buf_messages[P_MESSAGES+1]; /* Method message stack.  */
int method_buf_message_ptr = P_MESSAGES;    /* Stack pointer.         */

MESSAGE **method_buf_message_stack (void) { return method_buf_messages; }

int get_method_buf_message_ptr (void) { return method_buf_message_ptr; }

int method_buf_message_push (MESSAGE *m) {
  if (method_buf_message_ptr == 0) {
    warning (m, "method_buf_message_push: stack overflow.");
    return ERROR;
  }
  method_buf_messages[method_buf_message_ptr--] = m;
  return method_buf_message_ptr;
}

/*
 *   Call format method on any method buffers after
 *   methodbuffer[method_buffer_ptr], which is the buffer of the
 *   current method.  Any buffers after are methods that the library
 *   input has returned from.
 */

void unbuffer_method_statements (void) {

  int i,
    r;
  MBLIST *l;

  for (i = method_buffer_ptr + 1; method_buffer[i].src; i++) {
    if (method_buffer[i].src) {

      if ((r = format_method (method_buffer[i])) == SUCCESS) {
	while ((l = mb_list_unshift (&method_buffer[i].src)) != NULL) {
	  reuse_mb_list (l);
	}
	while ((l = mb_list_unshift (&method_buffer[i].init)) != NULL) {
	  reuse_mb_list (l);
	}
	while ((l = mb_list_unshift (&method_buffer[i].cvar_tab_members)) 
	       != NULL) {
	  reuse_mb_list (l);
	}
	while ((l = mb_list_unshift (&method_buffer[i].cvar_tab_init)) 
	       != NULL) {
	  reuse_mb_list (l);
	}
	method_buffer[i].src = NULL;
	method_buffer[i].method = NULL;
	method_buffer[i].init = NULL;
	method_buffer[i].cvar_tab_members = NULL;
	method_buffer[i].cvar_tab_init = NULL;
      }
    }
  }
}

int format_method (BUFFERED_METHOD b) {

  int i,
    stack_start,
    stack_end,
    block_level,
    return_expr_end,
    tok_after_return,
    need_return_block;
  bool need_init, 
    have_return_at_end;
  enum {
    fn_decl,
    fn_var_decl,
    fn_body,
    fn_null
  } fn_state;
  MBLIST *t;
  MESSAGE *m; 
  bool have_function_decl = false;


  format_method_output = TRUE;
  have_return_at_end = False;

  stack_start = stack_end = P_MESSAGES;

  for (t = b.src; t; t = t -> next)
    stack_end = tokenize_no_error (method_buf_message_push, (char *)t -> data);

  for (i = stack_start, fn_state = fn_null, block_level = 0, need_init = True; 
       i > stack_end; i--) {

    m = method_buf_messages[i];

    if (!m || !IS_MESSAGE (m)) break;

    switch (m -> tokentype)
      {
      case LABEL:
  	if (!have_function_decl &&
	    ((is_c_fn_declaration_msg (method_buf_messages, i, stack_end)) &&
	     !is_c_function_prototype_declaration (method_buf_messages, i))) {
 	  ++block_level;
  	  for ( ; method_buf_messages[i] -> tokentype != OPENBLOCK; i--) 
  	    __fileout (method_buf_messages[i] -> name);
 	  __fileout (method_buf_messages[i] -> name);
	  have_function_decl = true;
 	} else {
	  if (is_c_var_declaration_msg (method_buf_messages, i,
					stack_end, FALSE)) {
	    int k;
	    k = find_declaration_end (method_buf_messages, i, stack_end);
	    for ( ; i >= k; i--) {
	      __fileout (method_buf_messages[i] -> name);
	    }
	    ++i;
	    fn_state = fn_var_decl;
	  } else {
	    if (fn_state != fn_body) {
	      fn_state = fn_body;
	      if (need_init == True)
		generate_method_init (b);
	      need_init = False;
	    } 
	    /*
	     *  need_return_block and return_expr_end work
	     *  independently.  return_expr_end always returns the
	     *  index of the token before the semicolon of the return
	     *  expression.  return_block () returns ERROR if it is
	     *  necessary for us to insert a return block.  
	     */
	    if (!strcmp (M_NAME(method_buf_messages[i]), "return")) {
	      if ((need_return_block = 
		   return_block (method_buf_messages, i, 
				 &return_expr_end)) != ERROR) {
	      } else {
		__fileout (method_buf_messages[i] -> name);
	      }
	      if ((tok_after_return = 
		   tok_after_return_idx (method_buf_messages,i))
		  != ERROR) {
		if ((M_TOK(method_buf_messages[tok_after_return]) 
		    == CLOSEBLOCK) &&
		    (block_level == 1)) {
		  have_return_at_end = True;
		}
	      }
	    } else {
	      if (str_eq (M_NAME(method_buf_messages[i]), "exit")) {
		__fileout ("\n{ __ctalk_exitFn (1); }\n");
		__fileout (method_buf_messages[i] -> name);
	      } else {
		__fileout (method_buf_messages[i] -> name);
	      }
	    }
	  }
 	}
	break;
      case OPENBLOCK:
	++block_level;
	if ((block_level > 1) && need_init) {
	  generate_method_init (b);
	  need_init = False;
	}
	__fileout (method_buf_messages[i] -> name);
	break;
      case CLOSEBLOCK:
	--block_level;
	if (!block_level) {
	  if (fn_closure (method_buf_messages, i, get_frame_pointer ())) {
	    if (need_init) {
	      generate_method_init (b);
	    }
	    if (have_return_at_end == False) {
	      __fileout ("return ((void *)0);\n");
	    }
	  }
	}
	__fileout (method_buf_messages[i] -> name);
	if (!method_buf_messages[i-1])
	  goto format_method_done;
	break;
	/*
	 *  A method body can begin with a prefix op.
	 */
      case ASTERISK:
      case AMPERSAND:
      case PLUS:
      case MINUS:
      case EXCLAM:
      case BIT_COMP:
      case SIZEOF:
      case INCREMENT:
      case DECREMENT:
	if (fn_state == fn_null) {
	  fn_state = fn_body;
	  if (need_init == True)
	    generate_method_init (b);
	  need_init = False;
	} else if (fn_state != fn_body) {
	  fn_state = fn_body;
	  if (need_init == True)
	    generate_method_init (b);
	  need_init = False;
	}
	__fileout (method_buf_messages[i] -> name);
	break;
      default:
	__fileout (method_buf_messages[i] -> name);
	break;
      }
  }

 format_method_done:
  REUSE_MESSAGES(method_buf_messages,method_buf_message_ptr,P_MESSAGES)

    format_method_output = FALSE;

  return SUCCESS;
}

/* 
 *  Generate a method init.  __fileout () here instead of re-buffering,
 *  because generate_init () can use a temp file.  TO DO - Determine
 *  if we can use generate_init () instead.
 */

void generate_method_init (BUFFERED_METHOD b) {

  char buf[MAXMSG];
  MBLIST *l;

  if (b.method -> no_init) return;

  __fileout ("\n     {\n");

  strcatx (buf, "     ", SRC_FILE_SAVE_FN, " (\"", 
	   source_filename (), "\");\n", NULL);
  __fileout (buf);

  __fileout ("     __ctalk_initLocalObjects ();\n");

  for (l = b.init; l; l = l -> next) {
    __fileout ("     ");
    __fileout ((char *) l -> data);
  }

  for (l = b.cvar_tab_init; l; l = l -> next) {
    __fileout ("     ");
    __fileout ((char *) l -> data);
  }

  __fileout ("     }\n\n");

}

static void generate_vartab_b (MBLIST *l_vartab);

void generate_vartab_b (MBLIST *l_vartab) {

  MBLIST *l;

  __fileout ("\n");

  for (l = l_vartab; l; l = l -> next) {
    __fileout ((char *) l -> data);
  }
  __fileout ("\n");


}

/* 
   This will need a check if the vartab gets cached with the wrong method.
*/
void unbuffer_method_vartab (void) {
  MBLIST  *l;
  generate_vartab_b (method_buffer[method_buffer_ptr].cvar_tab_members);

  while ((l = mb_list_unshift 
	  (&method_buffer[method_buffer_ptr].cvar_tab_members)) 
	 != NULL) {
    reuse_mb_list (l);
  }
  method_buffer[method_buffer_ptr].cvar_tab_members = NULL;
}

void generate_vartab (LIST *l_vartab) {

  LIST *l;

  __fileout ("\n");

  for (l = l_vartab; l; l = l -> next) {
    __fileout ((char *) l -> data);
  }
  __fileout ("\n");


}
