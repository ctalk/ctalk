/* $Id: subscr.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2014-2019 Robert Kiesling, rk3314042@gmail.com.
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
 *   C aggregate variable expressions, both structs and arrays.
 *
 *   For arguments that have objects within subscripts, create a 
 *   template for the run time expression, for compilers that
 *   don't allow function calls (ie, to __ctalkEvalExpr ()) 
 *   within subscripts.
 *
 *   And other miscellaneous stuff, to register aggregate variable
 *   expressions.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"

extern I_PASS interpreter_pass;
extern bool argblk;

static int subscript_ext = 0;

int subscript_object_expr = FALSE;

SUBSCRIPT subscripts[MAXARGS] = {{0,0,0,0, NULL},};
int subscript_ptr = 0;

extern CTRLBLK *ctrlblks[MAXARGS + 1];
extern int ctrlblk_ptr;

#define SUBSCRIPTFMTMAX  (MAXMSG * 5)

static char tmpvarname[MAXMSG];
static char tmpvarnames[MAXMSG];
static char exprbuf[MAXMSG * 3];
static char exprs[MAXMSG];
static char regcvarbuf[MAXMSG];
static char argbuf[MAXMSG];

#define SUBSCRIPT_CVAR_TEMPLATE "\n    { \n\
      int %s; \n\
      %s \n\
      %s \n\
      %s \n\
  }\n\
"

/* 
 * Construct a list of tmp var declarations.  The final
 * semicolon is part of the template. 
 *
 * NOTE - Make sure that tmpvarnames is initialized to zero
 * before calling this function.
 */
static void collect_tmp_var_decls (char *new_tmp_var) {
  if (*tmpvarnames) {
    strcatx2 (tmpvarnames, ", ", tmpvarname, NULL);
  } else {
    strcpy (tmpvarnames, tmpvarname);
  }
}

/*
 *  Here, also, make sure that exprs is initialized to
 *  zero before calling the function.
 */
static void collect_tmp_var_exprs (char *new_expr) {
  if (*exprs) {
    /* strcat (exprs, ";\n\t"); strcat (exprs, new_expr); */
    strcatx2 (exprs, ";\n\t", new_expr, NULL);
  } else {
    strcatx (exprs, "\n\t", new_expr, NULL);
  }
}

static int find_eff_n_derefs (CVAR *v, int scope) {
  int eff_n_derefs = 0;
  if (!(scope & SUBSCRIPT_OBJECT_ALIAS)) {
    if (str_eq (v -> type, "char") && (v -> n_derefs == 2)) {
      eff_n_derefs = v -> n_derefs - 1;
    } else {
      eff_n_derefs = v -> n_derefs;
    }
  } else {
    eff_n_derefs = v -> n_derefs;
  }
  return eff_n_derefs;
}

char *subscript_expr (MESSAGE_STACK messages, int start, int end,
		      OBJECT *rcvr_object, METHOD *method,
		      char *array_basename, CVAR *v) {
  char orig_subscr_expr[MAXMSG];
  static char buf[SUBSCRIPTFMTMAX];
  char array_expr[MAXLABEL];
  char subscript_expr[MAXMSG];
  int i, subscript_n;
  int attrs_out = 0;
  int scope_out = LOCAL_VAR;
  int eff_n_derefs;
  bool with_ampersand = True;

  memset (tmpvarnames, 0, MAXMSG);
  memset (exprs, 0, MAXMSG);

  strcpy (array_expr, array_basename);

  for (subscript_n = 0; subscript_n < subscript_ptr; subscript_n++) {

    memset (orig_subscr_expr, 0, MAXMSG);

    for (i = subscripts[subscript_n].start_idx; 
	 i >= subscripts[subscript_n].end_idx; i--)
      strcatx2 (orig_subscr_expr, messages[i] -> name, NULL);

    sprintf (tmpvarname, SUBSCRIPT_VAR_FMT, subscript_ext++);

    collect_tmp_var_decls (tmpvarname);
    
    strcatx (subscript_expr, "[", tmpvarname, "]", NULL);
    strcatx2 (array_expr, subscript_expr, NULL);

    sprintf (exprbuf, TMP_SUBSCRIPT_FMT, tmpvarname, orig_subscr_expr);

    collect_tmp_var_exprs (exprbuf);
  }

  /*  Here, again, helps avoid a namespace collision. */
  if (v -> attrs & CVAR_ATTR_ARRAY_DECL) {
    scope_out |= SUBSCRIPT_OBJECT_ALIAS;
  }
  attrs_out = basic_attrs_of (v, basic_type_of (v));
  
  eff_n_derefs = find_eff_n_derefs (v, scope_out);

  if (str_eq (v -> type, "char") && (eff_n_derefs == 1)) {
    if (strstr (array_expr, "[")) {
      with_ampersand = False;
    }
  }

  /* These register calls nearly always have a '&' before the var,
     which is why we can't call __fmt_c_method_arg_call () instead. 
     We still have to adjust for subscript semantics for char *'s,
     though, which is why we checked for a subscript in the expression,
     just above. */
  sprintf (regcvarbuf,
	   "%s (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d, %d, %d, (void *)%s%s);\n",
	   REGISTER_C_METHOD_ARG,
	   ((v -> decl) ? v -> decl : NULLSTR),
	   ((v -> type) ? v -> type : NULLSTR),
	   ((v -> qualifier) ? v -> qualifier : NULLSTR),
	   ((v -> qualifier2) ? v -> qualifier2 : NULLSTR),
	   ((v -> storage_class) ? v -> storage_class : NULLSTR),
	   array_expr,
	   v -> type_attrs,
	   eff_n_derefs,
	   v -> initializer_size, 
	   scope_out,
	   attrs_out,
	   ((with_ampersand) ? "&" : ""),
	   array_expr);

  sprintf (argbuf, "__ctalk_arg (\"%s\", \"%s\", %d, (void *)\"%s\");", 
	   rcvr_object -> __o_name, method -> name, 
	   method -> n_params, array_expr);

  sprintf (buf, SUBSCRIPT_CVAR_TEMPLATE, tmpvarnames, exprs, regcvarbuf,
	   argbuf);

  return buf;
}

char *register_template_subscript_arg (MESSAGE_STACK messages, 
				       int start, int end) {
  char orig_subscr_expr[MAXMSG];
  char array_expr[MAXLABEL];
  char subscript_expr[MAXMSG];
  int i, subscript_n;
  int attrs_out = 0;
  int scope_out = LOCAL_VAR;
  int eff_n_derefs;
  bool with_ampersand = True;
  CVAR *v = NULL;   /* Avoid a warning message. */
  bool have_array = false;
  int subscript_start, n_subscripts;

  memset (tmpvarnames, 0, MAXMSG);
  memset (exprs, 0, MAXMSG);

  n_subscripts = 0;
  for (i = start; i >= end; i--) {
    if (M_TOK(messages[i]) == LABEL) {
      if ((subscript_start = nextlangmsg (messages, i)) != ERROR) {
	if (M_TOK(messages[subscript_start]) == ARRAYOPEN) {
	  strcpy (array_expr, M_NAME(messages[i]));
	  have_array = true;
	  if (((v = get_local_var (M_NAME(messages[i]))) == NULL) &&
	      ((v = get_global_var (M_NAME(messages[i]))) == NULL)) {
	    _error ("register_template_subscript_arg: variable, \"%s.\""
		    " not found.\n", M_NAME(messages[i]));
	      }
	}
      }
    } else if (have_array) {
      if (M_TOK(messages[i]) == ARRAYOPEN) {
	++n_subscripts;
	strcatx2 (array_expr, M_NAME(messages[i]), NULL);
      } else if (M_TOK(messages[i]) == ARRAYCLOSE) {
	--n_subscripts;
	if (n_subscripts == 0)
	  have_array = false;
	strcatx2 (array_expr, M_NAME(messages[i]), NULL);
      } else {
	strcatx2 (array_expr, M_NAME(messages[i]), NULL);
      }
    }
  }

  for (subscript_n = 0; subscript_n < subscript_ptr; subscript_n++) {

    memset (orig_subscr_expr, 0, MAXMSG);

    for (i = subscripts[subscript_n].start_idx; 
	 i >= subscripts[subscript_n].end_idx; i--)
      strcatx2 (orig_subscr_expr, messages[i] -> name, NULL);

    sprintf (tmpvarname, SUBSCRIPT_VAR_FMT, subscript_ext++);

    collect_tmp_var_decls (tmpvarname);
    
    strcatx (subscript_expr, "[", tmpvarname, "]", NULL);
    strcatx2 (array_expr, subscript_expr, NULL);

    sprintf (exprbuf, TMP_SUBSCRIPT_FMT, tmpvarname, orig_subscr_expr);

    collect_tmp_var_exprs (exprbuf);
  }

  /*  Here, again, helps avoid a namespace collision. */
  if (v -> attrs & CVAR_ATTR_ARRAY_DECL) {
    scope_out |= SUBSCRIPT_OBJECT_ALIAS;
  }
  attrs_out = basic_attrs_of (v, basic_type_of (v));
  
  eff_n_derefs = find_eff_n_derefs (v, scope_out);

  if (str_eq (v -> type, "char") && (eff_n_derefs == 1)) {
    if (strstr (array_expr, "[")) {
      with_ampersand = False;
    }
  }

  /* These register calls nearly always have a '&' before the var,
     which is why we can't call __fmt_c_method_arg_call () instead. 
     We still have to adjust for subscript semantics for char *'s,
     though, which is why we checked for a subscript in the expression,
     just above. */
  sprintf (regcvarbuf,
	   "%s (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d, %d, %d, (void *)%s%s);\n",
	   REGISTER_C_METHOD_ARG,
	   ((v -> decl) ? v -> decl : NULLSTR),
	   ((v -> type) ? v -> type : NULLSTR),
	   ((v -> qualifier) ? v -> qualifier : NULLSTR),
	   ((v -> qualifier2) ? v -> qualifier2 : NULLSTR),
	   ((v -> storage_class) ? v -> storage_class : NULLSTR),
	   array_expr,
	   v -> type_attrs,
	   eff_n_derefs,
	   v -> initializer_size, 
	   scope_out,
	   attrs_out,
	   ((with_ampersand) ? "&" : ""),
	   array_expr);

  return regcvarbuf;
}

char *subscript_expr_struct (MESSAGE_STACK messages, 
			     int subscript_start, int subscript_end,
			     int expr_start, int expr_end,
			     OBJECT *rcvr_object, METHOD *method,
			     char *array_basename, CVAR *v) {
  static char buf[SUBSCRIPTFMTMAX];
  char array_expr[MAXLABEL];
  char orig_subscr_expr[MAXMSG];
  char subscript_expr[MAXMSG];
  int i, subscript_n;
  int attrs_out = 0;
  int scope_out = LOCAL_VAR;
  int eff_n_derefs = 0;

  memset (tmpvarnames, 0, MAXMSG);
  memset (exprs, 0, MAXMSG);
  memset (array_expr, 0, MAXLABEL);

  for (i = expr_start; i > subscripts[0].block_start_idx; i--)
    strcatx2 (array_expr, M_NAME(messages[i]), NULL);

  for (subscript_n = 0; subscript_n < subscript_ptr; subscript_n++) {

    memset (orig_subscr_expr, 0, MAXMSG);

    for (i = subscripts[subscript_n].start_idx; 
	 i >= subscripts[subscript_n].end_idx; i--)
      strcatx2 (orig_subscr_expr, messages[i] -> name, NULL);

    sprintf (tmpvarname, SUBSCRIPT_VAR_FMT, subscript_ext++);

    collect_tmp_var_decls (tmpvarname);
    
    strcatx (subscript_expr, "[", tmpvarname, "]", NULL);
    strcatx2 (array_expr, subscript_expr, NULL);

    sprintf (exprbuf, TMP_SUBSCRIPT_FMT, tmpvarname, orig_subscr_expr);

    collect_tmp_var_exprs (exprbuf);

    if (subscript_n < (subscript_ptr - 1)) {
      for (i = subscripts[subscript_n].block_end_idx - 1;
	   i > subscripts[subscript_n + 1].block_start_idx;
	   i--) {
	strcatx2 (array_expr, M_NAME(messages[i]), NULL);
      }
    }
  }

  /* Find the last closing bracket and append the struct members
     to the expression. */
  for (i = subscripts[subscript_ptr-1].end_idx; ; i--) {
    if (M_TOK(messages[i]) == ARRAYCLOSE)
      break;
  }
  --i;
  for (; i >= expr_end; i--)
    strcatx2 (array_expr, M_NAME(messages[i]), NULL);

  /*  Here, again, helps avoid a namespace collision. */
  if (v -> attrs & CVAR_ATTR_ARRAY_DECL) {
    scope_out |= SUBSCRIPT_OBJECT_ALIAS;
  }
  attrs_out = basic_attrs_of (v, basic_type_of (v));
  
  eff_n_derefs = find_eff_n_derefs (v, scope_out);

  /* Again, these register calls are always have a '&' before the var,
     so the fn can't use __fmt_c_method_arg_call () directly. */
  sprintf (regcvarbuf,
	   "%s (\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %d, %d, %d, %d, %d, (void *)&%s);\n",
	   REGISTER_C_METHOD_ARG,
	   ((v -> decl) ? v -> decl : NULLSTR),
	   ((v -> type) ? v -> type : NULLSTR),
	   ((v -> qualifier) ? v -> qualifier : NULLSTR),
	   ((v -> qualifier2) ? v -> qualifier2 : NULLSTR),
	   ((v -> storage_class) ? v -> storage_class : NULLSTR),
	   array_expr,
	   v -> type_attrs,
	   eff_n_derefs,
	   v -> initializer_size, 
	   scope_out,
	   attrs_out,
	   array_expr);

  sprintf (argbuf, "__ctalk_arg (\"%s\", \"%s\", %d, (void *)\"%s\");", 
	   rcvr_object -> __o_name, method -> name, 
	   method -> n_params, array_expr);

  sprintf (buf, SUBSCRIPT_CVAR_TEMPLATE, tmpvarnames, exprs, regcvarbuf,
	   argbuf);

  return buf;
}

void register_struct_terminal (MESSAGE_STACK messages, 
			       MESSAGE *m_err, int var_start_idx,
			       int expr_end, 
			       int subscript_begin,
			       int subscript_end,
			       int struct_deref_end,
			       CVAR *v_struct, 
			       char *basename,
			       char *varname,
			       OBJECT *subscript_obj,
			       OBJECT *register_c_var_receiver,
			       METHOD *register_c_var_method) {
  CVAR *mbr, *_c_mbr = NULL;
  CVAR *struct_defn;
  char s[MAXMSG], *mbr_name_start_ptr,
    *mbr_name_end_ptr;
  if ((mbr_name_start_ptr = strchr (varname, '.')) != NULL) {
  next_elem:
    do {
      ++mbr_name_start_ptr;
    } while (isspace ((int)*mbr_name_start_ptr));
    mbr_name_end_ptr = mbr_name_start_ptr;
    /* find the actual end of the label, in case the label refers
       to an array with a subscript following the label token, e.g.,

          mystruct.mbrlabel[a];
    */
    while (isalnum((int)*mbr_name_end_ptr) ||
	   (*mbr_name_end_ptr == '_')) {
      ++mbr_name_end_ptr;
    }
    for (mbr = v_struct -> members; 
	 (mbr && !_c_mbr); mbr = mbr -> next) {
      if (!strncmp (mbr_name_start_ptr, mbr -> name,
		    mbr_name_end_ptr - mbr_name_start_ptr)) {
	_c_mbr = mbr;
      }
    }
    if (!_c_mbr) {
      if ((mbr_name_start_ptr = strchr (mbr_name_end_ptr, '.'))
	  != NULL) {
	/* 
	 *    In case we have nested structs in the members, like
	 *   
	 *      tab[0].elements[0].int_mbr
	 *
	 *   Then check for the next nested struct's element.
	 *   See test/expect/margexprs36.c for an example.
	 */
	goto next_elem;
      }
    }
  } else {
    for (mbr = v_struct -> members; 
	 (mbr && !_c_mbr); mbr = mbr -> next) {
      if (strstr (varname, mbr -> name)) {
	_c_mbr = mbr;
      }
    }
  }
  if (_c_mbr) {
    if (IS_STRUCT_OR_UNION (_c_mbr)) {
      if ((struct_defn = get_struct_defn (_c_mbr -> type)) != NULL) {
	return register_struct_terminal 
	  (messages, m_err, 
	   var_start_idx,
	   expr_end,
	   subscript_begin,
	   subscript_end,
	   struct_deref_end,
	   struct_defn,
	   basename, varname,
	   subscript_obj,
	   register_c_var_receiver,
	   register_c_var_method);
      } else {
	return warning (m_err, "Undefined struct, \"%s.\"", 
			_c_mbr -> type);
      }
    } else {
      cvar_register_output (messages, 
			    var_start_idx, expr_end,
			    subscript_begin,
			    subscript_end,
			    register_c_var_receiver,
			    register_c_var_method,
			    _c_mbr, 
			    basename, varname, subscript_obj);
    }
  } else {
    if (_c_mbr && (struct_deref_end != -1)) {
      warning (m_err, "Could not find struct member %s.\n",
	       M_NAME(messages[struct_deref_end]));
    } else {
      toks2str (messages, var_start_idx, expr_end, s);
      warning (m_err, "Could not find struct member in %s.\n", s);
    }
  }
}

char *register_struct_terminal_buf (MESSAGE_STACK messages, 
				    MESSAGE *m_err, int var_start_idx,
				    int expr_end, 
				    int subscript_begin,
				    int subscript_end,
				    int struct_deref_end,
				    CVAR *v_struct, 
				    char *basename,
				    char *varname,
				    OBJECT *subscript_obj,
				    OBJECT *register_c_var_receiver,
				    METHOD *register_c_var_method,
				    char *buf_out) {
  CVAR *mbr, *_c_mbr = NULL;
  CVAR *struct_defn;
  char s[MAXMSG];
  for (mbr = v_struct -> members; 
       (mbr && !_c_mbr); mbr = mbr -> next) {
    if (strstr (varname, mbr -> name)) {
      _c_mbr = mbr;
    }
  }
  if (_c_mbr) {
    if (IS_STRUCT_OR_UNION (_c_mbr)) {
      if ((struct_defn = get_struct_defn (_c_mbr -> type)) != NULL) {
	return register_struct_terminal_buf 
	  (messages, m_err, 
	   var_start_idx,
	   expr_end,
	   subscript_begin,
	   subscript_end,
	   struct_deref_end,
	   struct_defn,
	   basename, varname,
	   subscript_obj,
	   register_c_var_receiver,
	   register_c_var_method,
	   buf_out);
      } else {
	warning (m_err, "Undefined struct, \"%s.\"", 
		 _c_mbr -> type);
	return NULL;
      }
    } else {
      cvar_register_output_buf
	(messages, 
	 var_start_idx, expr_end,
	 subscript_begin,
	 subscript_end,
	 register_c_var_receiver,
	 register_c_var_method,
	 _c_mbr, 
	 basename, varname, subscript_obj,
	 buf_out);
    }
  } else {
    if (_c_mbr && (struct_deref_end != -1)) {
      warning (m_err, "Could not find struct member %s.\n",
	       M_NAME(messages[struct_deref_end]));
    } else {
      toks2str (messages, var_start_idx, expr_end, s);
      warning (m_err, "Could not find struct member in %s.\n", s);
    }
  }
  return buf_out;
}

extern MESSAGE *m_messages[N_MESSAGES+1];  /* Declared in method.c */
extern int m_message_ptr;

extern PARSER *parsers[MAXARGS+1];           /* Declared in rtinfo.c. */
extern int current_parser_ptr;

extern bool ctrlblk_pred;              /* Declared in control.c */

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c.   */
extern int new_method_ptr;

void cvar_register_output (MESSAGE_STACK messages, 
				  int expr_start,
				  int expr_end,
				  int subscript_begin,
				  int subscript_end,
				  OBJECT *rcvr_object,
				  METHOD *rcvr_method,
				  CVAR *c, 
				  char *basename, char *varname, 
				  OBJECT *subscript_obj) {
  int i;
  char cvar_buf[MAXMSG];
  /* 
     If in an argument block, the registration should be done by
     fmt_register_argblk_c_vars_* () in complexmethd.c.
  */
  if (argblk)
    return;

  if (ctrlblk_pred) {
    ctrlblk_register_c_var 
      (fmt_register_c_method_arg_call (c, varname, FRAME_SCOPE, cvar_buf),
      C_CTRL_BLK -> keyword_ptr + 1);
  } else {
    if (IS_OBJECT(subscript_obj)) {
      subscript_object_expr = TRUE;

      fileout (subscript_expr_struct 
	       (m_messages, subscript_begin,
		subscript_end, 
		expr_start, expr_end,
		rcvr_object,
		rcvr_method, basename, c), 0, FRAME_START_IDX);
      
      for (i = expr_start; i >= expr_end; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }

    } else {
      generate_register_c_method_arg_call (c, varname, FRAME_SCOPE, 
					   FRAME_START_IDX);
    }
  }
}

char *cvar_register_output_buf (MESSAGE_STACK messages, 
				int expr_start,
				int expr_end,
				int subscript_begin,
				int subscript_end,
				OBJECT *rcvr_object,
				METHOD *rcvr_method,
				CVAR *c, 
				char *basename, char *varname, 
				OBJECT *subscript_obj,
				char *buf_out) {
  int i;

  /* 
     If in an argument block, the registration should be done by
     fmt_register_argblk_c_vars_* () in complexmethd.c.
  */
  if (argblk)
    return NULL;

  if (ctrlblk_pred) {
    fmt_register_c_method_arg_call (c, varname, FRAME_SCOPE, buf_out);
  } else {
    if (IS_OBJECT(subscript_obj)) {
      subscript_object_expr = TRUE;
      strcatx (buf_out,
	       subscript_expr_struct 
	       (m_messages, subscript_begin,
		subscript_end, 
		expr_start, expr_end,
		rcvr_object,
		rcvr_method, basename, c), NULL);
      
      for (i = expr_start; i >= expr_end; i--) {
	++messages[i] -> evaled;
	++messages[i] -> output;
      }

    } else {
      fmt_register_c_method_arg_call (c, varname, FRAME_SCOPE, buf_out);
    }
  }
  return buf_out;
}

static char *subscr_rcvr_class (MESSAGE_STACK messages,
				int org_idx, CVAR *c) {
  return basic_class_from_cvar (messages[org_idx], c,
				find_eff_n_derefs (c, LOCAL_VAR));
}

/* 
 * Return true for an expression like the following.
 *
 *  <array>[<subscr>] <method>
 *
 *  Receiver context only, so far (ie, no obj-to-c translation). 
 *  And no assignment operators as methods (we check for plain C 
 *  lvalues and rvalues elsewhere).
 */
bool subscr_rcvr_next_tok_is_method_a (MESSAGE_STACK messages, int basename_idx,
				       CVAR *cvar) {
  char *rcvr_class;
  OBJECT *rcvr_class_obj, *superclass;
  int subscript_end_idx, method_idx;
  int struct_terminal_idx;
  bool subscript_terminal = false,
    struct_terminal = false;
  MESSAGE *m_method;
  METHOD *method;

  if (object_context (messages, basename_idx) != receiver_context)
    return false;

  /* A struct tag looks like a receiver context when it's declared.
     Check that it's not appearing for the first time at the end of
     the struct declaration. */
  if (cvar -> attrs & CVAR_ATTR_STRUCT_TAG) {
    int prev;
    if ((prev = prevlangmsgstack (messages, basename_idx)) != ERROR) {
      int o_b, p1, p2;

      switch (M_TOK(messages[prev])) 
	{
	case EQ:
	case ASR_ASSIGN:
	case ASL_ASSIGN:
	case PLUS_ASSIGN:
	case MINUS_ASSIGN:
	case MULT_ASSIGN:
	case DIV_ASSIGN:
	case BIT_AND_ASSIGN:
	case BIT_OR_ASSIGN:
	case BIT_XOR_ASSIGN:
	  /* Check for these here anyway - they get handled elsewhere. */
	  return false;
	  break;
	case CLOSEBLOCK:
	  o_b = match_block_open_brace 
	    (messages, prev, stack_start (messages));
	  if (o_b != ERROR) {
	    if ((p1 = prevlangmsgstack (messages, o_b)) != ERROR) {
	      if (str_eq (M_NAME(messages[p1]), "struct") ||
		  str_eq (M_NAME(messages[p1]), "union"))
		return false;
	    }
	    if (p1 != ERROR) {
	      if ((p2 = prevlangmsgstack (messages, p1)) != ERROR) {
		if (str_eq (M_NAME(messages[p2]), "struct") ||
		    str_eq (M_NAME(messages[p2]), "union"))
		  return false;
	      }
	    }
	  }
	}
    }
  }/* if (M_TOK(messages[i]) == CLOSEBLOCK) */

  if ((subscript_end_idx = 
       is_subscript_expr (messages, basename_idx, get_stack_top (messages)))
      <= 0) {
    return false;
  }

  rcvr_class = NULL;

  if ((struct_terminal_idx = struct_end (messages, basename_idx,
 					 get_stack_top (messages)))
      > 0) {
    if (cvar -> members) {
      CVAR *mbr;
      if ((mbr = struct_member_from_expr_b (messages, basename_idx,
					    struct_terminal_idx,
					    cvar)) != NULL) {
	rcvr_class =
	  basic_class_from_cvar (messages[struct_terminal_idx],
				   mbr, mbr -> n_derefs);
	struct_terminal = true;
      }
    } else if (cvar -> type_attrs & CVAR_TYPE_TYPEDEF) {
      CVAR *derived_type;
      CVAR *mbr;
      if ((derived_type = get_typedef (cvar -> type)) != NULL) {
	if ((mbr = mbr_from_struct_or_union_cvar 
	     (derived_type, M_NAME(messages[struct_terminal_idx])))
	    != NULL) {
	  rcvr_class =
	    basic_class_from_cvar (messages[struct_terminal_idx], mbr,
				   mbr -> n_derefs);
	  struct_terminal = true;
	}
      }
    } else {
      CVAR *defn, *mbr;
      if ((defn = struct_defn_from_struct_decl (cvar -> type))
	  != NULL) {
	if ((mbr = mbr_from_struct_or_union_cvar 
	     (defn, M_NAME(messages[struct_terminal_idx])))
	    != NULL) {
	  rcvr_class =
	    basic_class_from_cvar (messages[struct_terminal_idx], mbr,
				     mbr -> n_derefs);
	  struct_terminal = true;
	}
      }
    }
  } else {
    if (subscript_end_idx > 0) {
      if ((method_idx = nextlangmsg (messages, subscript_end_idx)) == ERROR)
	return false;
      if (!IS_C_BINARY_MATH_OP(M_TOK(messages[method_idx]))) {
	rcvr_class = subscr_rcvr_class (messages, method_idx, cvar);
      } else if (argblk) {
	/* we'll probably need a cvartab translation... */
	rcvr_class = subscr_rcvr_class (messages, method_idx, cvar);
      } else {
	return false;
      }
    } else {
      /*
       * An simple expression like:
       *
       *  argv[i] <method>
       *
       */ 
      if ((method_idx = nextlangmsg (messages, subscript_end_idx)) == ERROR)
	return false;
      rcvr_class = subscr_rcvr_class (messages, method_idx, cvar);
    }
    subscript_terminal = true;
  }

  if (!rcvr_class) {
    warning (messages[basename_idx], "receiver class for, \"%s,\" not found, "
	     "defaulting to Integer", cvar -> name);
    rcvr_class = INTEGER_CLASSNAME;
  }
  
  if ((rcvr_class_obj = get_class_object (rcvr_class)) == NULL)
    return false;

  if (struct_terminal == true) {
    if ((method_idx = nextlangmsg (messages, struct_terminal_idx)) == ERROR)
      return false;
  } else if (subscript_terminal == true) {
      if ((method_idx = nextlangmsg (messages, subscript_end_idx)) == ERROR)
	return false;
  } else {
    warning (messages[basename_idx], "Unhandled case in next_tok_method_a.");
    return false;
  }

  m_method = messages[method_idx];

  if (IS_C_BINARY_MATH_OP(M_TOK(m_method)))
    return false;

  /* Do this ourselves so we don't try to load a method here. */
  for (method = rcvr_class_obj -> instance_methods; method;
       method = method -> next) {
    if (str_eq (method -> name, M_NAME(m_method)))
      return true;
  }
  for (method = rcvr_class_obj -> class_methods; method;
       method = method -> next) {
    if (str_eq (method -> name, M_NAME(m_method)))
      return true;
  }
  for (superclass = rcvr_class_obj -> __o_superclass;
       superclass; superclass = superclass -> __o_superclass) {
    for (method = superclass -> instance_methods; method;
	 method = method -> next) {
      if (str_eq (method -> name, M_NAME(m_method)))
	return true;
    }
    for (method = superclass -> class_methods; method;
	 method = method -> next) {
      if (str_eq (method -> name, M_NAME(m_method)))
	return true;
    }
  }

  if (is_method_proto (rcvr_class_obj, m_method -> name)) {
    return true;
  }

  return false;
}

void handle_simple_subscr_rcvr (MESSAGE_STACK messages, 
				int cvar_basename_idx) {
  int i_tmp, agg_var_end_idx;
  char expr_buf_out[MAXMSG];
  CVAR *c;
  MSINFO ms;
  ms.messages = messages;
  ms.stack_start = P_MESSAGES;
  ms.stack_ptr = get_stack_top (messages);
  ms.tok = cvar_basename_idx;
  int end_idx_tmp = find_expression_limit (&ms);

  register_c_var (messages[cvar_basename_idx], messages, cvar_basename_idx,
		  &agg_var_end_idx);
  /* Check for any other CVARs in the expression. */
  for (i_tmp = agg_var_end_idx - 1; i_tmp >= end_idx_tmp; --i_tmp) {
    if (M_TOK(messages[i_tmp]) == LABEL) {
      if (((c = get_local_var (M_NAME(messages[i_tmp]))) != NULL) ||
	  ((c = get_global_var (M_NAME(messages[i_tmp]))) != NULL)) {
	register_c_var (messages[i_tmp], messages, i_tmp, &agg_var_end_idx);
      }
    }
  }
  fileout (fmt_rt_expr 
	   (messages, cvar_basename_idx, &end_idx_tmp, expr_buf_out),
	   FALSE, cvar_basename_idx);
  for (i_tmp = cvar_basename_idx; i_tmp >= end_idx_tmp; --i_tmp) {
    ++message_stack_at (i_tmp) -> evaled;
    ++message_stack_at (i_tmp) -> output;
  }
}
