/* $Id: argblk.c,v 1.1.1.1 2021/04/03 11:26:02 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2020 Robert Kiesling, rk3314042@gmail.com.
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
#include <string.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "list.h"

extern bool ctrlblk_pred,            /* From control.c */
  ctrlblk_blk,
  ctrlblk_else_blk;
extern char fn_name[MAXLABEL];       /* from fnbuf.c */

extern char *ascii[8193];             /* from intascii.h */

bool have_complex_arg_block = FALSE;  /* Set and reset in complexmethod.c, also */
				     /* reset in method_call ().              */
int complex_arg_block_start = -1;
int complex_arg_block_rcvr_ptr = -1; /* Set by method_call () and      */ 
                                     /* is_argblk_expr                 */

ARGBLK *argblks[MAXARGS + 2];   /* leave more room for tightly packed arrays.*/
int argblk_ptr;

static int argblk_id;

extern NEWMETHOD *new_methods[MAXARGS+1];  /* Declared in lib/rtnwmthd.c.  */
extern int new_method_ptr;

extern OBJECT *rcvr_class_obj;

extern I_PASS interpreter_pass;

bool argblk;

MESSAGE *argblk_messages[N_MESSAGES + 1];
int argblk_messageptr;

MESSAGE_STACK argblk_message_stack (void) {
  return argblk_messages;
}

int get_argblk_message_ptr (void) {
  return argblk_messageptr;
}

int argblk_message_push (MESSAGE *m) {
  if (argblk_messageptr == 0) {
    _error ("argblk_message_push: stack_overflow.");
    return ERROR;
  }

  if (!m)
    _error ("argblk_message_push: null pointer, argblk_messageptr = %d.", 
	    argblk_messageptr);

  argblk_messages[argblk_messageptr] = m;

#ifdef STACK_TRACE
  debug ("argblk_message_push %d. %s.", argblk_messageptr, 
	 (argblk_messages[argblk_messageptr] && 
	  IS_MESSAGE (argblk_messages[argblk_messageptr])) ?
	 argblk_messages[argblk_messageptr] -> name : "(null)"); 
#endif

  --argblk_messageptr;

  return argblk_messageptr + 1;
}

void init_arg_blocks (void) {
  argblk_ptr = MAXARGS;
  argblk_id = argblk = 0;
  argblk_messageptr = N_MESSAGES;
}

void argblk_push (ARGBLK *a) {
  argblks[argblk_ptr--] = a;
}

ARGBLK *argblk_pop (void) {
  static ARGBLK *a;
  a = argblks[++argblk_ptr];
  argblks[argblk_ptr] = NULL;
  return a;
}

ARGBLK *new_arg_blk (void) {
  static ARGBLK *a;
  if ((a = (ARGBLK *) __xalloc (sizeof (struct _argblk))) == NULL)
    return NULL;
  return a;
}

ARGBLK *current_argblk (void) {
  return  C_ARG_BLK;
}

void buffer_argblk_stmt (char *s) {
  LIST *l;
  l = new_list ();
  l -> data = strdup (s);
  list_push (&C_ARG_BLK_BUFFER, &l);
}

void buffer_argblk_stmt_above (char *s) {
  LIST *l;
  l = new_list ();
  l -> data = strdup (s);
  list_push (&C_ARG_BLK_BUFFER_ABOVE, &l);
}

#define HAVE_COMPLEX_ARG_BLOCK (have_complex_arg_block && \
				(complex_arg_block_rcvr_ptr != -1))

int create_argblk (MESSAGE_STACK messages, int method_idx,
		   int argblk_start_idx) {
  ARGBLK *a;
  int i, n_blocks, rcvr_idx;

  if (M_TOK(messages[method_idx]) != METHODMSGLABEL) {
    if (!HAVE_COMPLEX_ARG_BLOCK) {
      warning (messages[method_idx], "Argument block has no method.");
      return ERROR;
    }
  }

  if (HAVE_COMPLEX_ARG_BLOCK) {
    rcvr_idx = complex_arg_block_rcvr_ptr;
  } else {
    if (messages[method_idx] -> receiver_msg) {
      MESSAGE *__method_msg;
      int __i;
      rcvr_idx = -1;
      __method_msg = messages[method_idx];
      for (__i = method_idx + 1; rcvr_idx == -1; __i++) {
	if (messages[__i] == __method_msg -> receiver_msg)
	  rcvr_idx = __i;
      }
    } else {
      if ((rcvr_idx = prevlangmsg (messages, method_idx)) == ERROR) {
	warning (messages[C_ARG_BLK->start_idx],
		 "Argument block syntax error.");
	return ERROR;
      }
    }
  }

  if (!HAVE_COMPLEX_ARG_BLOCK) {
    if (messages[method_idx]->receiver_obj != 
	messages[rcvr_idx]->obj) {
      warning (messages[C_ARG_BLK->start_idx],
	       "Invalid argument block receiver.");
      return ERROR;
    }
  }

  a = new_arg_blk ();
  a -> id = argblk_id++;

  for (i = argblk_start_idx - 1, n_blocks = 1; ; i--) {

    if (!messages[i]) {
      warning (messages[i+1], "Unterminated argument block.");
      /* so the partial argblk record's boundaries still 
	 contain valid messages. */
      ++i;
      break;
    }      

    switch (M_TOK(messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case OPENBLOCK:
	++n_blocks;
	break;
      case CLOSEBLOCK:
	--n_blocks;
	break;
      case LABEL:
	if (str_eq (M_NAME(messages[i]), "return")) {
	  a -> has_return = true;
	  a -> return_exprs[a -> return_expr_ptr] =
	    strdup (fmt_argblk_return_expr (messages, i)); 
	  a -> return_expr_ptr++;
	}
	break;
      }

    if (!n_blocks)
      break;
  }
  a -> rcvr_idx = rcvr_idx;
  a -> start_idx = argblk_start_idx;
  a -> end_idx = i;
  a -> parser_level = current_parser_level ();
  argblk = TRUE;
  argblk_push (a);
  return SUCCESS;
}

static char *argblk_fn_name (char *s) {
  char srcfile[FILENAME_MAX];
  char *t, *q;
  if (interpreter_pass == method_pass) {
    strcatx (s, new_methods[new_method_ptr+1] -> method -> selector,
	     ARGBLK_LABEL,
	     ascii[C_ARG_BLK->id], NULL);
  } else {
    t = source_filename ();
    q = srcfile;
    while (*t) {
      /* - and . characters don't work in function names, so
	 translate them or stop when we have the input's
	 file basename. */
      if (*t == '-') {
	*q = '_';
      } else if (*t == '.') {
	break;
      } else {
	*q = *t;
      }
      q++, t++;
    }
    *q = '\0';
    strcatx (s, srcfile, ARGBLK_LABEL,
	     ascii[C_ARG_BLK -> id], NULL);
  }
  return s;
}

static char *blk_return_expr (int nth_return, char *buf_out) {
  sprintf (buf_out, "return __ctalkRegisterArgBlkReturn (-2, %s)",
	   C_ARG_BLK -> return_exprs[nth_return]);
  return buf_out;
}

static LIST *find_return_expr_end (LIST *l) {
  do {
    l = l -> next;
  } while (((char *)l -> next -> data)[0] != ';');
  return l;
}

extern int library_input;    /* Declared in class.c. */
extern CLASSLIB *lib_includes[MAXARGS + 1];
extern int lib_includes_ptr;

#define BLK_BUF_SIZE 0x8888
#define BLK_BUF_SIZE_MARGIN 0x400
static int blklength = 0;
static int blklimit = 0;
static char *blk_buf = NULL;

static void argblk_calls (MESSAGE_STACK messages, LIST *argblk_toks) {
  LIST *l;
  int nth_return = 0;
  int n_braces = 0;
  int blk_chars = 0;
  char return_expr_buf[MAXMSG];
  
  for (l = C_ARG_BLK_BUFFER; l; l = l -> next) {
    if (str_eq ((char *)l -> data, "break")) {
      blk_chars =
	strcatx2 (blk_buf,
		  "__ctalkExitArgBlockScope ();\n",
		  "return __ctalkRegisterIntReturn(-1);\n", NULL);
    } else if (str_eq ((char *)l -> data, "continue")) {
      blk_chars =
	strcatx2 (blk_buf, "__ctalkExitArgBlockScope (); return ((void *)0)",
		  NULL);
    } else if (str_eq ((char *)l -> data, "return")) {
      if (!TOP_LEVEL_ARGBLK) {
	warning (messages[C_ARG_BLK->rcvr_idx], 
		 "\"return,\" in this context is not (yet) supported. "
		 "Use, \"break,\" instead.");
	blk_chars = strcatx2 (blk_buf, (char *)l -> data, NULL);
      } else {
	blk_chars =
	  strcatx2 (blk_buf, blk_return_expr (nth_return, return_expr_buf),
		    NULL);
	++nth_return;
	l = find_return_expr_end (l);
      }
    } else if (*(char *)l -> data == '{') {
      ++n_braces;
      blk_chars = strcatx2 (blk_buf, (char *)l -> data, NULL);
      if (n_braces == 1) {
	/* This is argblock_init. */
	strcatx2 (blk_buf, ARGBLK_ENTER_SCOPE_FN, " ();\n", NULL);
      }
    } else if (*(char *)l -> data == '}') {
      --n_braces;
      if (n_braces == 0) {
	/* This is argblock_exit */
	blk_chars = strcatx2 (blk_buf, ARGBLK_EXIT_SCOPE_FN, " ();\n",
			      ARGBLK_RETURN_STMT, ";\n", NULL);
      }
      blk_chars = strcatx2 (blk_buf, (char *)l -> data, NULL);
    } else if (!strstr ((char *)l -> data, FIXUPROOT)) {
      blk_chars = strcatx2 (blk_buf, (char *)l -> data, NULL);
    }
    if (blk_chars > blklimit) {
      blklength *= 2;
      blklimit = blklength - BLK_BUF_SIZE_MARGIN;
      blk_buf = __xrealloc ((void **)&blk_buf, blklength);
    }
  }
  strcat (blk_buf, "\n");
}

/* this is like unbuffer_function_argblk, but it also creates
   a method for the argblk which is written to the preload
   cache with the calling method. */
static void unbuffer_method_argblk (MESSAGE_STACK messages) {
  METHOD *argblk_method;
  char method_fn_buf[MAXMSG];

  if ((argblk_method = (METHOD *)__xalloc (sizeof (METHOD))) == NULL)
    _error ("unbuffer_argblk: %s.", strerror (errno));
  argblk_method -> sig = METHOD_SIG;
  unbuffer_method_vartab ();

  blklength = BLK_BUF_SIZE;
  blklimit = BLK_BUF_SIZE - BLK_BUF_SIZE_MARGIN;

  blk_buf = __xalloc (blklength);
  argblk_calls (messages, C_ARG_BLK_BUFFER);
  argblk_fn_name (argblk_method -> selector);
  strcatx (method_fn_buf, "OBJECT *", argblk_method -> selector,
	   " (void) ", NULL);

  __fileout (method_fn_buf);
  __fileout (blk_buf);

  /* if a method has this attribute, we know the return class is "Object" */
  /* see save_pre_init_info in premethod.c (probably elsewhere, too */
  argblk_method -> attrs = METHOD_ARGBLK_ATTR;
  new_methods[new_method_ptr+1] -> 
    argblks[new_methods[new_method_ptr+1] -> argblk_ptr] = 
    argblk_method;
  --new_methods[new_method_ptr+1] -> argblk_ptr;

  __xfree (MEMADDR(blk_buf));
  delete_list (&C_ARG_BLK_BUFFER);
  C_ARG_BLK_BUFFER = NULL;
  generate_instance_method_definition_call 
    (messages[C_ARG_BLK->rcvr_idx]->obj->__o_classname,
     argblk_method -> selector, argblk_method -> selector, 0, 0);
}

static void unbuffer_function_argblk (MESSAGE_STACK messages) {
  char method_fn_buf[MAXMSG], 
    method_name_buf[MAXLABEL];

  unbuffer_function_vartab ();

  blklength = BLK_BUF_SIZE;
  blklimit = BLK_BUF_SIZE - BLK_BUF_SIZE_MARGIN;

  blk_buf = __xalloc (blklength);
  argblk_calls (messages, C_ARG_BLK_BUFFER);
  argblk_fn_name (method_name_buf);
  strcatx (method_fn_buf, "OBJECT *", method_name_buf,
	   " (void) ", NULL);

  __fileout (method_fn_buf);
  __fileout (blk_buf);

  __xfree (MEMADDR(blk_buf));
  delete_list (&C_ARG_BLK_BUFFER);
  C_ARG_BLK_BUFFER = NULL;
  generate_instance_method_definition_call 
    (messages[C_ARG_BLK->rcvr_idx]->obj->__o_classname,
     method_name_buf, method_name_buf, 0, 0);
}

static char *non_local_return_expr_default_class = "Integer";

static char *non_local_return_expr (MESSAGE_STACK messages, char *buf_out) {
  CFUNC *fn;
  char return_expr2[MAXLABEL], *return_class;
  
  if (interpreter_pass == parsing_pass) {
    if ((fn = get_function (fn_name)) == NULL) {
      _warning ("non_local_return_expr: Can't find function, \"%s.\" "
		"Return type defaulting to int.", fn_name);
      return_class = non_local_return_expr_default_class;
    } else {
      return_class = basic_class_from_cfunc 
	(messages[C_ARG_BLK->start_idx], fn, 0);
    }
    /* We need to leave some room for a __ctalk_exitFn call if
       the return is from main (). */
    (void)fmt_rt_return (" __ctalkArgBlkReturnVal ()",
			 return_class, TRUE, return_expr2);
    strcatx (buf_out, "   return ", return_expr2, ";\n}\n", NULL);
    return buf_out;
  } else if (interpreter_pass == method_pass) {
    strcatx (buf_out, "   return ", " __ctalkArgBlkReturnVal ()", 
	     ";\n}\n", NULL);
    return buf_out;
  }
  return NULL;
}

void replace_argblk (MESSAGE_STACK messages) {
  char rcvr_buf[MAXMSG], expr_buf[MAXMSG], namebuf[MAXLABEL];
  toks2str (messages, C_ARG_BLK -> rcvr_idx, C_ARG_BLK -> start_idx + 1,
	    rcvr_buf);
  if (!TOP_LEVEL_ARGBLK) {
    strcatx (expr_buf, EVAL_EXPR_FN, "(\"", rcvr_buf,
	     argblk_fn_name (namebuf), "\");", NULL);
    buffer_argblk_stmt_above (expr_buf);
  } else {
    if (C_ARG_BLK -> has_return == true) {
      strcatx (expr_buf, EVAL_EXPR_FN, "(\"", rcvr_buf,
	       argblk_fn_name (namebuf), "\");\n",
	       "if (__ctalkNonLocalArgBlkReturn()) {\n", NULL);
      fileout (expr_buf, TRUE, 0);
      fileout (non_local_return_expr (messages, expr_buf), TRUE, 0);
    } else {
      strcatx (expr_buf, EVAL_EXPR_FN, "(\"", rcvr_buf,
	       argblk_fn_name (namebuf), "\");", NULL);
      fileout (expr_buf, TRUE, 0);
    }
  }
}

void delete_argblk (ARGBLK *a) {
  int i;
  for (i = 0; i < a -> return_expr_ptr; ++i)
    __xfree (MEMADDR(a -> return_exprs[i]));
  __xfree (MEMADDR(a));
}

int argblk_end (MESSAGE_STACK messages, int idx) {
  ARGBLK *a;
  if (interpreter_pass == expr_check) 
    return FALSE;
  if (argblk_ptr >= MAXARGS)
    return FALSE;
  /* 
   *  This could be speeded up if we use the global
   *  parser_level instead of calling current_parser_level ()
   */
  if ((idx <= C_ARG_BLK->end_idx) && 
      (C_ARG_BLK -> parser_level == current_parser_level ())) {
    /*
     *  Trailing brace should not be mistaken for a 
     *  function or method closing block.
     */
    buffer_argblk_stmt (M_NAME(messages[idx]));
    ++messages[idx]->evaled;
    ++messages[idx]->output;
    replace_argblk (messages);
    if (interpreter_pass == parsing_pass)
      unbuffer_function_argblk (messages);
    else if (interpreter_pass == method_pass)
      unbuffer_method_argblk (messages);
    if (TOP_LEVEL_ARGBLK)
      argblk = FALSE;
    a = argblk_pop ();
    delete_argblk (a);
    return TRUE;
  }
  return FALSE;
}

int is_argblk_ctrl_struct (MESSAGE_STACK messages, int idx) {
  if (argblk_ptr >= MAXARGS)
    return FALSE;
  if (((idx <= C_ARG_BLK->rcvr_idx) && (idx >= C_ARG_BLK->start_idx)) ||
      (idx == C_ARG_BLK->end_idx))
    return TRUE;
  return FALSE;
}

/*
 *  Look for a construct <label> <label> <openblock>.
 *  The function works by syntax only, so it is probably only 
 *  useful if the first label is a method parameter as
 *  a receiver.  There is a fixup in resolve () when the
 *  parser determines the methods receiver object and 
 *  receiver message.
 *
 *  Here we just assume that the expression is a complex 
 *  arg block predicate, TO DO - which we should do 
 *  everywhere anyways.
 */
int is_argblk_expr (MESSAGE_STACK messages, int rcvr_idx) {
  int i, lookahead, lookahead2, stack_top;
  stack_top = get_stack_top (messages);
  for (i = rcvr_idx; i > stack_top; i--) {
    if (M_ISSPACE(messages[i]))
      continue;
    if ((M_TOK(messages[i]) != LABEL) &&
	(M_TOK(messages[i]) != OPENBLOCK))
      return FALSE;
    if ((lookahead = nextlangmsg (messages, i)) == ERROR)
      return FALSE;
    if (M_TOK(messages[i]) == LABEL) {
      if (M_TOK(messages[lookahead]) == LABEL) {
	if ((lookahead2 = nextlangmsg (messages, lookahead)) != ERROR) {
	  if (M_TOK(messages[lookahead2]) == OPENBLOCK) {
	    int i_2;
	    messages[lookahead] -> tokentype = METHODMSGLABEL;
	    have_complex_arg_block = TRUE;
	    complex_arg_block_rcvr_ptr = rcvr_idx;
	    create_argblk (messages, lookahead, lookahead2);
	    for (i_2 = rcvr_idx; i_2 >= lookahead; i_2--) {
	      /* check that the method label is valid for its receiver
		 class. */
	      if (M_TOK(messages[i_2]) == METHODMSGLABEL) {
		if (messages[i] -> attrs & TOK_SELF) {
		  if (!get_instance_method (messages[i],
					   rcvr_class_obj,
					    M_NAME(messages[lookahead]),
					    ANY_ARGS, FALSE)) {
		    undefined_blk_method_warning
		      (messages[rcvr_idx], messages[i], messages[lookahead]);
		  }
		} else {
		  if ((messages[rcvr_idx] -> attrs & TOK_SELF) &&
		      (rcvr_idx != i)) {
		    OBJECT *instvar;
		    /* check for an instance variable series */
		    instvar = get_self_instance_variable_series
		      (messages[i], rcvr_idx, i, get_stack_top (messages));
		    if (instvar) {
		      if (!get_instance_method
			  (messages[i],
			   instvar -> instancevars -> __o_class,
			   M_NAME(messages[lookahead]),
			   ANY_ARGS, FALSE)) {
			undefined_blk_method_warning
			  (messages[rcvr_idx], messages[i], messages[lookahead]);
		      }
		    }
		  }
		}
	      }
	      ++(messages[i_2]->evaled);
	      ++(messages[i_2]->output);
	    }
	    complex_arg_block_rcvr_ptr = -1;
	    have_complex_arg_block = FALSE;
	    return TRUE;
	  } else {
	    if (M_TOK(messages[lookahead2]) == LABEL) {
	      continue;
	    } else {
	      return FALSE;
	    }
	  }
	} else {
	  return FALSE;
	}
      } else {
	return FALSE;
      }
    }
  }
  return FALSE;
}

/*
 *  Similar to above, but it looks for an already looked-up
 *  method. Called from method_args ().
 *
 *  Using P_MESSAGES as the stack limit works okay because
 *  messages is always the main message stack.
 */

int is_argblk_expr2 (MESSAGE_STACK messages, int rcvr_idx) {
  int i, lookahead, lookahead2, stack_top;
  stack_top = get_stack_top (messages);
  for (i = rcvr_idx; i > stack_top; i--) {
    if (M_ISSPACE(messages[i]))
      continue;
    if ((M_TOK(messages[i]) != LABEL) &&
	(M_TOK(messages[i]) != OPENBLOCK) && 
	(M_TOK(messages[i]) != METHODMSGLABEL))
      return FALSE;
    if ((lookahead = nextlangmsg (messages, i)) == ERROR)
      return FALSE;
    if ((M_TOK(messages[i]) == LABEL) ||
	(M_TOK(messages[i]) == METHODMSGLABEL)) {
      if ((M_TOK(messages[lookahead]) == METHODMSGLABEL) ||
	  (M_TOK(messages[lookahead]) == LABEL)) {
	if ((lookahead2 = nextlangmsg (messages, lookahead)) != ERROR) {
	  if (M_TOK(messages[lookahead2]) == OPENBLOCK) {
	    int i_2;
	    have_complex_arg_block = TRUE;
	    if ((messages[rcvr_idx] -> attrs & OBJ_IS_INSTANCE_VAR) ||
		(messages[rcvr_idx] -> attrs & OBJ_IS_CLASS_VAR)) {
	      int tmp_idx;
	      for (tmp_idx = rcvr_idx; tmp_idx <= P_MESSAGES; tmp_idx++)
		if (messages[tmp_idx] == messages[rcvr_idx] -> receiver_msg) {
		  rcvr_idx = tmp_idx;
		  break;
		}
	    }
	    complex_arg_block_rcvr_ptr = rcvr_idx;
	    create_argblk (messages, lookahead, lookahead2);
	    for (i_2 = rcvr_idx; i_2 >= lookahead; i_2--) {
	      ++(messages[i_2]->evaled);
	      ++(messages[i_2]->output);
	    }
	    complex_arg_block_rcvr_ptr = -1;
	    have_complex_arg_block = FALSE;
	    return TRUE;
	  } else {
	    if ((M_TOK(messages[lookahead2]) == LABEL) ||
		(M_TOK(messages[lookahead2]) == METHODMSGLABEL)) {
	      continue;
	    } else {
	      return FALSE;
	    }
	  }
	} else {
	  return FALSE;
	}
      } else {
	return FALSE;
      }
    }
  }
  return FALSE;
}

OBJECT *argblk_rcvr_object (MESSAGE_STACK messages, int idx) {
  if (argblk_ptr == MAXARGS)
    return NULL;
  return messages[argblks[argblk_ptr+1]->rcvr_idx] -> obj;
}

static void super_c_argument_context_handler (MSINFO *ms) {
  int lookback, prev_tok;
  int expr_end_idx;
  int i;
  static char exprbuf[MAXMSG], expr_buf_out[MAXMSG], arg_expr_buf[MAXMSG];
  char actual_name[MAXLABEL];
  CVAR *typedef_defn, *typedef_defn_2, *mbr;
  bool have_typedef_mbr;
  CVAR *c, *struct_defn = NULL, *struct_decl = NULL; /* Avoid warnings. */

  if (is_fmt_arg_2 (ms)) {
    fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf);
    for (i = ms -> tok; i >= expr_end_idx; i--) {
      ++ms -> messages[i] -> evaled;
      ++ms -> messages[i] -> output;
    }
    fileout (fmt_printf_fmt_arg_ms (ms, exprbuf, expr_buf_out),
	     0, ms -> tok);
  } else {
    fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf);
    /*
     *  This is basically the same as in rt_self_expr ().
     */
    if ((lookback = lval_idx_from_arg_start (ms -> messages, ms -> tok)) 
	!= ERROR) {
      if (((c = get_local_var (M_NAME(ms -> messages[lookback]))) != NULL) ||
	  ((c = get_global_var (M_NAME(ms -> messages[lookback]))) != NULL) ||
	  ((c = get_var_from_cvartab_name (M_NAME(ms -> messages[lookback])))
	   != NULL)) {
	for (i = ms -> tok; i >= expr_end_idx; i--) {
	  ++ms -> messages[i] -> evaled;
	  ++ms -> messages[i] -> output;
	}
	fileout
	  (fmt_rt_return (exprbuf,
			  basic_class_from_cvar (ms -> messages[lookback], c, 0),
			  TRUE, expr_buf_out), 0, ms -> tok);
      } else {
	prev_tok = prevlangmsg (ms -> messages, lookback);
	if (((M_TOK(ms -> messages[prev_tok]) == PERIOD) ||
	     (M_TOK(ms -> messages[prev_tok]) == DEREF)) &&
	    is_struct_member_tok (ms -> messages, lookback)) {
	  int lookback2 = lookback;  /* Avoid a warning. */
	  int last_expr_tok;
	  last_expr_tok = lookback;
	  while ((lookback = prevlangmsg (ms -> messages, lookback)) != ERROR) {
	    if (((lookback2 = prevlangmsg (ms -> messages, lookback)) != ERROR) 
		&& ((M_TOK(ms -> messages[lookback2]) != PERIOD) ||
		    (M_TOK(ms -> messages[lookback2]) != DEREF)))
	      break;
	  }
	  if (!argblk && (((struct_decl = 
			   get_local_var (M_NAME(ms -> messages[lookback2])))
			  == NULL) &&
			  ((struct_decl =
			    get_global_var
			    (M_NAME(ms -> messages[lookback2]))) == NULL))) {
	    warning (ms -> messages[lookback], 
		     "Definition of struct or union, \"%s,\" not found.",
		     M_NAME(ms -> messages[lookback2]));
	    for (i = ms -> tok; i >= expr_end_idx; i--) {
	      ++ms -> messages[i] -> evaled;
	      ++ms -> messages[i] -> output;
	    }
	    fileout (exprbuf, 0, ms -> tok);
	  } else if (ARGBLK_SUBEXPR_WRAPPER(ms -> messages[lookback2]->name)) {
	    /***/
	    /* We've wrapped the struct * in parentheses to avoid a
	       compiler warning.  Extract the actual name. */
	    extract_argblk_name_from_subexpr (ms->messages[lookback2]->name,
					      actual_name);
	    if ((c = get_var_from_cvartab_name
		 (actual_name)) != NULL) {
	      if (c -> type_attrs & CVAR_TYPE_TYPEDEF) {
		if ((typedef_defn = get_typedef (c -> type))
		    != NULL) {
		  if (IS_CVAR (typedef_defn -> members)) {
		    lookback = nextlangmsg (ms -> messages,
					    lookback);
		    
		    for (have_typedef_mbr = false,
			   mbr = typedef_defn -> members; mbr;
			 mbr = mbr -> next) {
		      if (str_eq (mbr -> name,
				  M_NAME(ms -> messages[lookback]))){
			have_typedef_mbr = true;
			break;
		      }
		    }
		    if (have_typedef_mbr) {
		      fmt_rt_argblk_expr (ms -> messages, ms -> tok,
					  &expr_end_idx, arg_expr_buf);
		      fmt_rt_return (exprbuf, 
				     basic_class_from_cvar
				     (ms -> messages[lookback], mbr, 0),
				     TRUE, arg_expr_buf);
		      for (i = ms -> tok; i >= expr_end_idx; i--) {
			++ms -> messages[i] -> evaled;
			++ms -> messages[i] -> output;
		      }
		      fileout (arg_expr_buf, 0, ms -> tok);
		      return;
		    }
		  } else { /* if (IS_CVAR (typedef_defn -> members) */
		    if ((typedef_defn_2 = get_struct_by_type
			 (typedef_defn -> qualifier)) != NULL) {
		      lookback = nextlangmsg (ms -> messages,
					      lookback);
		      for (have_typedef_mbr = false,
			     mbr = typedef_defn_2 -> members; mbr;
			   mbr = mbr -> next) {
			if (str_eq (mbr -> name,
				    M_NAME(ms -> messages[lookback]))){
			  have_typedef_mbr = true;
			  break;
			}
		      }
		      if (have_typedef_mbr) {
			fmt_rt_argblk_expr (ms -> messages,
					    ms -> tok, &expr_end_idx,
					    arg_expr_buf);
			fmt_rt_return (exprbuf, 
				       basic_class_from_cvar
				       (ms -> messages[ms -> tok], mbr, 0),
				       TRUE, arg_expr_buf);
			for (i = ms -> tok; i >= expr_end_idx; i--) {
			  ++ms -> messages[i] -> evaled;
			  ++ms -> messages[i] -> output;
			}
			fileout (arg_expr_buf, 0, ms -> tok);
			return;
		      }
		    }
		  }  /* if (IS_CVAR (typedef_defn -> members) */
		}
	      }
	    }
	  } else {
	    if (struct_decl -> attrs == CVAR_ATTR_STRUCT) {
	      if (((struct_defn = get_global_var (struct_decl->type))
		   == NULL) &&
		  ((struct_defn = get_local_var (struct_decl->type))
		   == NULL)) {
		warning (ms -> messages[lookback], 
			 "Definition of struct or union, \"%s,\" not found.",
			 struct_decl -> type);
		struct_defn = struct_decl;
	      }
	    }
	    c = struct_member_from_expr_b (ms -> messages, 
					   lookback2,
					   last_expr_tok,
					   struct_defn);
	    for (i = ms -> tok; i >= expr_end_idx; i--) {
	      ++ms -> messages[i] -> evaled;
	      ++ms -> messages[i] -> output;
	    }
	    fileout 
	      (fmt_rt_return (exprbuf,
			      basic_class_from_cvar (ms -> messages[lookback],
						     c, 0),
			      TRUE, expr_buf_out), 0, ms -> tok);
	  }
	}
      }
    } else {
      for (i = ms -> tok; i >= expr_end_idx; i--) {
	++ms -> messages[i] -> evaled;
	++ms -> messages[i] -> output;
      }
      fileout (exprbuf, 0, ms -> tok);
    }
  }
}

int argblk_super_expr (MSINFO *ms) {

  int arg_idx, fn_label_idx, expr_end_idx, i,
    prev_tok_idx, prev_tok_idx_2, tmp, expr_start;
  char *expr, *p_cl;
  static char exprbuf[MAXMSG], expr_buf_out[MAXMSG], exprbuf2[MAXMSG];
  OBJECT_CONTEXT super_context;
  OBJECT *blk_rcvr_object;

  super_context = object_context_ms (ms);

  if (interpreter_pass == method_pass) {
    switch (super_context)
      {
      case receiver_context:
	if (ARGBLK_TOK(ms -> tok)) {
	  super_argblk_rcvr_expr 
	    (ms -> messages, ms -> tok,
	     new_methods[new_method_ptr+1]->method->rcvr_class_obj);
	  return SUCCESS;
	}
	break;
      case c_context:
	if ((arg_idx = obj_expr_is_fn_arg_ms (ms, &fn_label_idx)) != -1) {
	  /* rt_obj_arg () does a fileout () of the expression,
	     so we don't have to. */
	  rt_obj_arg (ms -> messages, ms -> tok, &expr_end_idx, fn_label_idx,
		      arg_idx);
	  return SUCCESS;
	} 
	if (ctrlblk_pred) {
	  ctrlblk_pred_rt_expr (ms -> messages, ms -> tok);
	} else {
	  if ((prev_tok_idx = prevlangmsg (ms -> messages, ms -> tok)) != ERROR) {
	    if (ms -> messages[prev_tok_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	      ms -> tok = prev_tok_idx;
	      expr = collect_expression (ms, &expr_end_idx);
	      fmt_eval_expr_str (expr, exprbuf);
	      __xfree (MEMADDR(expr));
	      if (is_fmt_arg_2 (ms)) {
		fileout (fmt_printf_fmt_arg_ms
			     (ms, exprbuf,
			      expr_buf_out), 0, ms -> tok);
	      } else {
		fileout (exprbuf, 0, prev_tok_idx);
	      }
	      for (i = prev_tok_idx; i >= expr_end_idx; --i) {
		++ms -> messages[i] -> evaled;
		++ms -> messages[i] -> output;
	      }
	      return SUCCESS;
	    }
	  }
	}
	break;
      case c_argument_context:
	if (ctrlblk_pred) {
	  ctrlblk_pred_rt_expr (ms -> messages, ms -> tok);
	} else {
	  super_c_argument_context_handler (ms);
	}
	return SUCCESS;
	break;
      case argument_context:
	expr_start = ms -> tok;

	if (ctrlblk_pred) {
	  ctrlblk_pred_rt_expr (ms -> messages, ms -> tok);
	} else {
	  if ((prev_tok_idx = prevlangmsg (ms -> messages, ms -> tok))
	      != ERROR) {
	    if (ms ->  messages[prev_tok_idx] -> attrs & TOK_IS_PREFIX_OPERATOR) {
	      if (((prev_tok_idx_2 = prevlangmsg (ms -> messages, prev_tok_idx))
		  != ERROR) &&
		  (M_TOK(ms -> messages[prev_tok_idx_2]) == OPENPAREN)) {
		if (is_fmt_arg (ms -> messages, prev_tok_idx_2,
				ms -> stack_start, ms -> stack_ptr)) {
		  tmp = ms -> tok;
		  ms -> tok = expr_start = prev_tok_idx_2;
		  fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf2);
		  fmt_printf_fmt_arg (ms -> messages, ms -> tok,
				      ms -> stack_start, exprbuf2,
				      exprbuf);
		  register_argblk_c_vars_1 (ms -> messages, ms -> tok, expr_end_idx);
		  ms -> tok = tmp;
		} else {
		  tmp = ms -> tok;
		  ms -> tok = expr_start = prev_tok_idx_2;
		  fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf);
		  register_argblk_c_vars_1 (ms -> messages, ms -> tok, expr_end_idx);
		  ms -> tok = tmp;
		}
	      } else {
		while (ms -> messages[++prev_tok_idx]
		       -> attrs & TOK_IS_PREFIX_OPERATOR)
		  ;
		--prev_tok_idx;
		if (is_fmt_arg (ms -> messages, prev_tok_idx,
				ms -> stack_start, ms -> stack_ptr)) {
		  tmp = ms -> tok;
		  ms -> tok = expr_start = prev_tok_idx;
		  fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf2);
		  fmt_printf_fmt_arg (ms -> messages, ms -> tok,
				      ms -> stack_start, exprbuf2,
				      exprbuf);
		  register_argblk_c_vars_1 (ms -> messages, ms -> tok, expr_end_idx);
		  ms -> tok = tmp;
		} else {
		  tmp = ms -> tok;
		  ms -> tok = expr_start = prev_tok_idx;
		  fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf2);
		  register_argblk_c_vars_1 (ms -> messages, ms -> tok, expr_end_idx);
		  fileout (exprbuf2, 0, ms -> tok);
		  for (i = expr_start; i >= expr_end_idx; i--) {
		    ++ms -> messages[i] -> evaled;
		    ++ms -> messages[i] -> output;
		  }
		  return SUCCESS;
		}
	      }
	    } else {
	      fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf);
	      register_argblk_c_vars_1 (ms -> messages, ms -> tok, expr_end_idx);
	    }
	  } else {
	    fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf);
	    register_argblk_c_vars_1 (ms -> messages, ms -> tok, expr_end_idx);
	  }
	  if ((p_cl = use_new_c_rval_semantics_b (ms -> messages, ms -> tok)) != NULL) {
	    fileout (fmt_rt_return (exprbuf, p_cl,
					TRUE, expr_buf_out), 0, ms -> tok);
	  } else {
	    fileout (exprbuf, 0, ms -> tok);
	  }
	  for (i = expr_start; i >= expr_end_idx; i--) {
	    ++ms -> messages[i] -> evaled;
	    ++ms -> messages[i] -> output;
	  }
	}
	return SUCCESS;
	break;
      default:
	break;
      }
  } else { /* if (interpreter_pass == method_pass) */
    /* Always print a warning if an argblk super is used in
       a C function. */
    if (interpreter_pass == parsing_pass) {
      if ((blk_rcvr_object = 
	   argblk_rcvr_object (ms -> messages, ms -> tok))
	  != NULL) {
	warning (ms -> messages[ms -> tok], "Keyword, \"super,\" used in a function"
		 " argument block.");
	warning (ms -> messages[ms -> tok], "\tUsing, \"%s,\" as the receiver.",
		 M_NAME(ms -> messages[argblks[argblk_ptr+1]->rcvr_idx]));
      }
    }
    switch (super_context)
      {
      case receiver_context:
	if ((blk_rcvr_object = 
	     argblk_rcvr_object (ms -> messages, ms -> tok))
	    != NULL) {
	  super_argblk_rcvr_expr 
	    (ms -> messages, ms -> tok, blk_rcvr_object);
	  return SUCCESS;
	}
	break;
      case c_context:
	if ((arg_idx = obj_expr_is_fn_arg_ms (ms, &fn_label_idx)) != -1) {
	  /* here also, rt_obj_arg () does a fileout () of the expression.*/
	  rt_obj_arg (ms -> messages, ms -> tok, &expr_end_idx, fn_label_idx,
		      arg_idx);
	  return SUCCESS;
	}
	if (ctrlblk_pred) {
	  ctrlblk_pred_rt_expr (ms -> messages, ms -> tok);
	}
	break;
      case c_argument_context:
	if (ctrlblk_pred) {
	  ctrlblk_pred_rt_expr (ms -> messages, ms -> tok);
	} else {
	  super_c_argument_context_handler (ms);
	}
	return SUCCESS;
	break;
      case argument_context:
	if (interpreter_pass != expr_check) {
	  fmt_rt_expr_ms (ms, &expr_end_idx, exprbuf);
	  register_argblk_c_vars_1 (ms -> messages, ms -> tok, expr_end_idx);
	  if ((p_cl = use_new_c_rval_semantics_b (ms -> messages, ms -> tok)) != NULL) {
	    fileout (fmt_rt_return (exprbuf, p_cl,
					TRUE, expr_buf_out), 0, ms -> tok);
	  } else {
	    fileout (exprbuf, 0, ms -> tok);
	  }
	  for (i = ms -> tok; i >= expr_end_idx; i--) {
	    ++ms -> messages[i] -> evaled;
	    ++ms -> messages[i] -> output;
	  }
	}
	return SUCCESS;
	break;
      default:
	break;
      }
  } /* if (interpreter_pass == method_pass) */

  return ERROR;
}

/*
 *  Use a a cvartab entry for any local CVAR within an argument
 *  block that is a function argument.  We should already have
 *  created a cvartab entry.  This is an odd case - normally we
 *  wouldn't worry about a C variable as a function argument, except
 *  that the code gets relocated because it's in an argument block.
 *  Called by resolve ().
 */
bool argblk_cvar_is_fn_argument (MESSAGE_STACK messages, int cvar_idx,
				CVAR *cvar) {
  int fn_idx;
  int arg_idx, next_idx, name_buf_len;
  MESSAGE *m;
  char name_buf[MAXLABEL];
  if (!(cvar -> scope & LOCAL_VAR))
      return false;

  if ((arg_idx = obj_expr_is_arg (messages, cvar_idx,
				  stack_start (messages),
				  &fn_idx)) == ERROR)
    return false;

  m = messages[cvar_idx];

  next_idx = nextlangmsg (messages, cvar_idx);

  switch (interpreter_pass) 
    {
    case method_pass:
      if (new_methods[new_method_ptr+1] -> method) {
	name_buf_len = method_cvar_alias_basename 
	  (new_methods[new_method_ptr+1] -> method, cvar,
	   name_buf);
	/* Leave room for the '*' character or a cast. */
	if (name_buf_len > (MAXLABEL - 10)) {
	  resize_message (m, strlen (name_buf) + 10);
	}
	if ((cvar -> type_attrs & CVAR_TYPE_CHAR) && (cvar -> n_derefs > 0)) {
	  strcatx (m -> name, "(char *)", name_buf, NULL);
	} else if (next_idx != ERROR &&
		   (M_TOK(messages[next_idx]) == INCREMENT ||
		    M_TOK(messages[next_idx]) == DECREMENT ||
		    M_TOK(messages[next_idx]) == PERIOD ||
		    M_TOK(messages[next_idx]) == DEREF)) {
	  /* if we have a following operator with equal or higher 
	     precedence than unary '*' */
	  sprintf (m -> name, "(*%s)", name_buf);
	  if (M_TOK(messages[next_idx]) == PERIOD) { /***/
	    int i_2, _struct_expr_end = 
	      struct_end (messages, cvar_idx, get_stack_top (messages));
	    m -> attrs |= TOK_HAS_CVARTAB_AGG_WRAPPER;

	    for (i_2 = cvar_idx; i_2 >= _struct_expr_end; --i_2)
	      /* because resolve can't parse the struct basename
		 within the wrapper, this prevents resolve from
		 trying to resolve the member name(s), and finally
		 returning them. */
	      ++(messages[i_2] -> evaled);
	  }
	} else if (next_idx != ERROR &&
		   (M_TOK(messages[next_idx]) == ARRAYOPEN)) {
	  if (!subscripted_int_in_code_block_error (messages,
						    cvar_idx, cvar)) {
	    strcatx (m -> name, "*", name_buf, NULL);
	  }
	} else {
	  strcatx (m -> name, "*", name_buf, NULL);
	}
	++(messages[cvar_idx] ->  evaled);
	return true;
      } else {
	return false;
      }
      break;
    case parsing_pass:
      /* This is untested. */
      name_buf_len = function_cvar_alias_basename (cvar, name_buf);
      /* if (strlen (name_buf) > (MAXLABEL - 10)) { */
      if (name_buf_len > (MAXLABEL - 10)) {
	resize_message (m, strlen (name_buf) + 10);
      }
      if ((cvar -> type_attrs & CVAR_TYPE_CHAR) && (cvar -> n_derefs > 0)) {
	strcatx (m -> name, "(char *)", name_buf, NULL);
      } else if (next_idx != ERROR &&
		 (M_TOK(messages[next_idx]) == INCREMENT ||
		  M_TOK(messages[next_idx]) == DECREMENT ||
		  M_TOK(messages[next_idx]) == PERIOD ||
		  M_TOK(messages[next_idx]) == DEREF)) {
	/* if we have a following operator with equal or higher
	   precedence than unary '*' */
	sprintf (m -> name, "(*%s)", name_buf);
      } else if (next_idx != ERROR &&
		 (M_TOK(messages[next_idx]) == ARRAYOPEN)) {
	if (!subscripted_int_in_code_block_error (messages,
						  cvar_idx, cvar)) {
	  strcatx (m -> name, "*", name_buf, NULL);
	}
      } else {
	strcatx (m -> name, "*", name_buf, NULL);
      }
      ++(messages[cvar_idx] ->  evaled);
      return true;
      break;
    default:
      return false;
      break;
    }

  /* ++(messages[cvar_idx] ->  evaled); */
  return false;
}

/* 
   This needs to be preceeded by a call to handle_cvar_argblk_translation,
   which sets messages[label_idx] -> name to the name of the cvar
   int the translation table, including the prefix deref '*'s
*/
void register_argblk_cvar_from_basic_cvar (MESSAGE_STACK messages,
					   int  label_idx,
					   int output_idx,
					   CVAR *cvar) {
  int old_n_derefs, i_lbl = 0;
  char old_name[MAXLABEL];

  while (messages[label_idx] -> name[i_lbl] == '*')
    ++i_lbl;

  old_n_derefs = cvar -> n_derefs;
  strcpy (old_name, cvar -> name);
  strcpy (cvar -> name, &messages[label_idx] -> name[i_lbl]);
  cvar -> n_derefs = i_lbl + old_n_derefs;
  cvar -> attrs |= CVAR_ATTR_CVARTAB_ENTRY;
  generate_register_c_method_arg_call
    (cvar,
     &messages[label_idx] -> name[i_lbl],
     this_frame () -> scope, output_idx);
  cvar -> n_derefs = old_n_derefs;
  strcpy (cvar -> name, old_name);
  
}

char *fmt_register_argblk_cvar_from_basic_cvar (MESSAGE_STACK messages,
						int  label_idx,
						CVAR *cvar,
						char *buf_out) {
  int stack_top_idx, next_tok, i_deref, i_deref_prev_tok,
    old_n_derefs = cvar -> n_derefs,
    i_lbl = 0,
    term_label_idx = label_idx;
  char old_name[MAXLABEL], struct_members[MAXMSG], old_name_mbr[MAXMSG];
  bool have_deref = false;
  CVAR *mbr, *derived;

  while (messages[label_idx] -> name[i_lbl] == '*')
    ++i_lbl;

  old_n_derefs = cvar -> n_derefs;
  strcpy (old_name, cvar -> name);
  strcpy (cvar -> name, &messages[label_idx] -> name[i_lbl]);
  if ((next_tok = nextlangmsg (messages, label_idx)) != ERROR) {
    if (M_TOK(messages[next_tok]) == DEREF) {
      stack_top_idx = get_stack_top (messages);
      *struct_members = 0;
      i_deref_prev_tok = LABEL;
      for (i_deref = next_tok; i_deref > stack_top_idx; --i_deref) {
	if (i_deref_prev_tok == LABEL) {
	  if ((M_TOK(messages[i_deref]) != DEREF) &&
	      !M_ISSPACE(messages[i_deref]))
	    break;
	} else if (i_deref_prev_tok == DEREF) {
	  if (M_TOK(messages[i_deref]) == LABEL)
	    term_label_idx = i_deref;
	  if ((M_TOK(messages[i_deref]) != LABEL) &&
	      !M_ISSPACE(messages[i_deref]))
	    break;
	} else {
	  if ((M_TOK(messages[i_deref]) != LABEL) &&
	      (M_TOK(messages[i_deref]) != DEREF) &&
	      !M_ISSPACE(messages[i_deref])) {
	    break;
	  }
	}
	strcat (struct_members, M_NAME(messages[i_deref]));
	if (!M_ISSPACE(messages[i_deref]))
	  i_deref_prev_tok = M_TOK(messages[i_deref]);
      }
      if ((M_TOK(messages[next_tok]) == DEREF) &&
	  cvar -> type_attrs & CVAR_TYPE_OBJECT &&
	  (cvar -> n_derefs == 1)) {
	/* char cast_buf[MAXMSG]; */
	/* also in handle_cvar_argblk_translation in cvartab.c --
	   this needs to match the templates in pattypes.c and
	   rt_cvar.c when the program is run. */
	sprintf (messages[label_idx] -> name,
		 "((OBJECT *)*%s)", cvar -> name);
#ifdef SYMBOL_SIZE_CHECK
	check_symbol_size (messages[label_idx] -> name);
#endif	    
	strcatx (cvar -> name, messages[label_idx] -> name,
		 struct_members, NULL);
      } else {
	strcat (cvar -> name, struct_members);
      }
      have_deref = true;
    }
  }
  cvar -> n_derefs += 1;
  cvar -> attrs |= CVAR_ATTR_CVARTAB_ENTRY;
  if (have_deref) {
    /* this all could be simplified */
    mbr = NULL;
    if (cvar -> members) {
      for (mbr = cvar -> members; mbr; mbr = mbr -> next) {
	if (str_eq (mbr -> name, M_NAME(messages[term_label_idx])))
	  break;
      }
    } else {
      if ((((derived = 
	     get_typedef (cvar -> type)) != NULL) ||
	   ((derived = 
	     get_typedef (cvar -> qualifier)) != NULL)) &&
	  derived -> members) {
	for (mbr = derived -> members; mbr; mbr = mbr -> next) {
	  if (str_eq (mbr -> name, M_NAME(messages[term_label_idx])))
	    break;
	}
      }
    }
    if (mbr) {
      strcpy (old_name_mbr, mbr -> name);
      strcpy (mbr -> name, cvar -> name);
      /* Note that this should list an OBJECT ** 
	 (i.e., c -> n_derefs == 2), but it's easier
	 at run time if we give it only one deref,
	 because then we don't need to evaluate a 
	 subexpression.  E.g. (*myStuff) -> __o_value.
	 
	 It also means that for a member like a char *,
	 the actual deref of the member is n_derefs == 1,
	 and we don't need to reconstruct an entire object.
      */
      fmt_register_c_method_arg_call
	(mbr,
	 cvar -> name,
	 this_frame () -> scope, buf_out);
      strcpy (mbr -> name, old_name_mbr);
    }
  } else {
    fmt_register_c_method_arg_call
      (cvar,
       &messages[label_idx] -> name[i_lbl],
       this_frame () -> scope, buf_out);
  }
  cvar -> n_derefs = old_n_derefs;
  strcpy (cvar -> name, old_name);
  cvar -> attrs &= ~CVAR_ATTR_CVARTAB_ENTRY;
  
  return buf_out;
}

char *fmt_register_argblk_cvar_from_basic_cvar_2 (MESSAGE *message,
						CVAR *cvar,
						char *buf_out) {
  int old_n_derefs, i_lbl = 0;
  char old_name[MAXLABEL];

  while (message -> name[i_lbl] == '*')
    ++i_lbl;

  old_n_derefs = cvar -> n_derefs;
  strcpy (old_name, cvar -> name);
  strcpy (cvar -> name, &message -> name[i_lbl]);
  cvar -> n_derefs += 1;
  cvar -> attrs |= CVAR_ATTR_CVARTAB_ENTRY;
  fmt_register_c_method_arg_call
    (cvar,
     &message -> name[i_lbl],
     this_frame () -> scope, buf_out);
  cvar -> n_derefs = old_n_derefs;
  strcpy (cvar -> name, old_name);
  cvar -> attrs &= ~CVAR_ATTR_CVARTAB_ENTRY;
  
  return buf_out;
}

/* should only need the main message stack */
int argblk_fp_containers (MESSAGE_STACK messages, int fp_decl_idx,
			  CVAR *var_list) {
  CVAR *c;
  int stack_top_idx, subscript_start_idx, subscript_end_idx,
    initializer_start_idx = fp_decl_idx, /* avoid warnings */
    initializer_end_idx = fp_decl_idx,   /* and here ... */
    block_body_start = fp_decl_idx,   /* here, too */
    block_body_end = fp_decl_idx,
    i, fp_decl_idx_tmp;
  int tok_err_line, tok_err_col;
  bool have_initializer = false;
  char *decl_buf;
  char *init_buf;
  static char container_buf[MAXMSG];
  char id_buf[MAXMSG] = "";
  PARSER *p;
  FRAME *f;
  
  for (c = var_list; c; c = c -> next) {
    if (c -> attrs & CVAR_ATTR_FP_ARGBLK) {
      /*
       *  First construct the container declaration.
       */
      stack_top_idx = get_stack_top (messages);
      if ((subscript_start_idx = scanforward (messages, fp_decl_idx,
					      stack_top_idx,
					      ARRAYOPEN)) != ERROR) {
	if ((subscript_end_idx = match_array_brace
	     (messages, subscript_start_idx, stack_top_idx)) != ERROR) {
	  if ((initializer_start_idx = nextlangmsg
	       (messages, subscript_end_idx)) != ERROR) {
	    if (M_TOK(messages[initializer_start_idx]) == EQ) {
	      have_initializer = true;
	      if ((initializer_end_idx = scanforward
		   (messages, initializer_start_idx,
		    stack_top_idx, SEMICOLON)) == ERROR)
		return ERROR;
	    } else {
	      have_initializer = false;
	    }
	  } else {
	    return ERROR;
	  }
	  if (str_eq (M_NAME(messages[fp_decl_idx]), "long") &&
	      !(c -> type_attrs & CVAR_TYPE_LONG)) {
	    /* we've shortend a long double array into a double. 
	       use only the "double" keyword from the declaration. */
	    fp_decl_idx_tmp = nextlangmsg (messages, fp_decl_idx);
	  } else {
	    fp_decl_idx_tmp = fp_decl_idx;
	  }
	  if (!have_initializer) {
	    decl_buf =
	      collect_tokens (messages, fp_decl_idx_tmp,
			      initializer_start_idx);
	    strcatx (container_buf, "struct ", c -> type, " {\n\t\t",
		     decl_buf, "\n\t} ", c -> name, ";\n", NULL);
	  } else {
	    decl_buf =
	      collect_tokens (messages, fp_decl_idx_tmp, subscript_end_idx);
	    init_buf = collect_tokens (messages, initializer_start_idx,
				       initializer_end_idx);
	    strcatx (container_buf, "struct ", c -> type, " {\n\t\t",
		     decl_buf, ";\n\t} ", c -> name, "\n\t\t",
		     init_buf, NULL);
	  }
	  fileout (container_buf, 0, fp_decl_idx);
	  if (!have_initializer) {
	    __xfree (MEMADDR(decl_buf));
	    for (i = fp_decl_idx; i >= initializer_start_idx; i--) {
	      ++messages[i] -> evaled;
	      ++messages[i] -> output;
	    }
	  } else {
	    __xfree (MEMADDR(decl_buf));
	    __xfree (MEMADDR(init_buf));
	    for (i = fp_decl_idx; i >= initializer_end_idx; i--) {
	      ++messages[i] -> evaled;
	      ++messages[i] -> output;
	    }
	  }
	}
      }
      /*
       *  Then add the container in the method or function.
       */
      p = pop_parser ();
      push_parser (p);
      if (have_initializer) {
	block_body_start = initializer_end_idx - 1;
      } else {
	block_body_start = initializer_start_idx - 1;
      }
      f = frame_at (p -> last_frame);
      block_body_end = f -> message_frame_top + 1;
      strcatx2 (id_buf, c -> name, ".", c -> members -> name, NULL);
      for (i = block_body_start; i > block_body_end; i--) {
	if (M_TOK(messages[i]) == LABEL) {
	  if (str_eq (M_NAME(messages[i]), c -> members -> name)) {
	    /* shift the tokens down by two to create a space
	       for the struct name and the "." operator. 
	       calling fp_container_adjust twice makes the
	       function easier to diagnose.
	    */
	    tok_err_line = messages[i] -> error_line;
	    tok_err_col = messages[i] -> error_column;
	    fp_container_adjust (i);
	    fp_container_adjust (i - 1);
	    messages[i] = new_message ();
	    strcpy (messages[i] -> name, c -> name);
	    messages[i] -> tokentype = LABEL;
	    messages[i] -> error_line = tok_err_line;
	    messages[i] -> error_column = tok_err_col;
	    messages[i-1] = new_message ();
	    messages[i-1] -> name[0] = '.'; messages[i-1] -> name[1] = '\0';
	    messages[i-1] -> tokentype = PERIOD;
	    messages[i-1] -> error_line = tok_err_line;
	    messages[i-1] -> error_column = tok_err_col;
	    i -= 2;
	  }
	}
      }
    }
  }
  return SUCCESS;
}
