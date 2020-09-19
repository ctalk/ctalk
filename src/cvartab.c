/* $Id: cvartab.c,v 1.2 2020/09/19 01:08:27 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014-2020 Robert Kiesling, rk3314042@gmail.com.
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
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

extern NEWMETHOD *new_methods[MAXARGS+1]; /* Declared in lib/rtnwmthd.c.   */
extern int new_method_ptr;

extern char fn_name[MAXLABEL];

extern bool argblk;                    /* Declared in argblk.c.      */

extern char *ascii[8193];             /* from intascii.h */

extern bool ctrlblk_pred,            /* Global states.                    */
  ctrlblk_blk,
  ctrlblk_else_blk;

extern I_PASS interpreter_pass;    /* Declared in main.c.               */

static bool struct_has_tag (MESSAGE_STACK messages,
			    int struct_keyword_idx) {
  int open_bracket_idx, close_bracket_idx,
    lookahead, stack_top_idx, n_brackets, i;
  stack_top_idx = get_stack_top (messages);
  if ((open_bracket_idx = scanforward (messages, struct_keyword_idx,
				       stack_top_idx,
				       OPENBLOCK)) != ERROR) {
    n_brackets = 1; /* count the opening bracket automatically */
    close_bracket_idx = -1;
    for (i = open_bracket_idx - 1; i > stack_top_idx; i--) {
      if (M_ISSPACE(messages[i]))
	continue;
      if (M_TOK(messages[i]) == OPENBLOCK) {
	++n_brackets;
      } else if (M_TOK(messages[i]) == CLOSEBLOCK) {
	--n_brackets;
	if (n_brackets == 0) {
	  close_bracket_idx = i;
	  break;
	}
      }
    }

    if (n_brackets || close_bracket_idx == -1)
      return false;

    if ((lookahead = nextlangmsgstack (messages, close_bracket_idx)) == ERROR)
      return false;

    if (M_TOK(messages[lookahead]) == SEMICOLON)
      return false;
    else if (M_TOK(messages[lookahead]) == LABEL)
      return true;

  }
  return false;
}

static char *struct_type_tag (CVAR *, char *, char *)
  __attribute__ ((noinline));

static char *struct_type_tag (CVAR *struct_cvar,
			      char *type_tag, char *buf_out) {
  CVAR *struct_def;
  if (interpreter_pass == method_pass) {
    strcatx (buf_out, new_methods[new_method_ptr+1] -> method -> selector,
	     "_", type_tag, NULL);
    return buf_out;
  } else if (interpreter_pass == parsing_pass) {
    if (struct_cvar -> scope  == LOCAL_VAR &&
	struct_cvar -> members == NULL) {
      /* Look for a global definition of the struct, in which
	 case we can use the type tag as it is. */
      if ((struct_def = get_global_struct_defn (type_tag)) != NULL) {
	if (struct_def -> scope == GLOBAL_VAR) {
	  strcpy (buf_out, type_tag);
	  return buf_out;
	} else {
	  strcatx (buf_out, fn_name, "_", type_tag, NULL);
	  return buf_out;
	}
      } else {
	strcatx (buf_out, fn_name, "_", type_tag, NULL);
	return buf_out;
      }
    } else {
      strcatx (buf_out, fn_name, "_", type_tag, NULL);
      return buf_out;
    }
  }
  return NULL;
}

static int sdecl_id = 0;

/* this tag is used only by the declaration in the cvar tab, mainly
   so we can cast the struct it aliases to it and avoid compiler
   warnings. Note that they're serialized in case of multiple
   structs in a method or function. */
static char *struct_decl_tag (char *buf_out) {
  if (interpreter_pass == method_pass) {
    strcatx (buf_out, new_methods[new_method_ptr+1] -> method -> selector,
	     "_sdecl_", ascii[sdecl_id++], NULL);
    return buf_out;
  } else if (interpreter_pass == parsing_pass) {
    strcatx (buf_out, fn_name, "_sdecl_", ascii[sdecl_id++], NULL);
    return buf_out;
  }
  return NULL;
}

/* See the comments in method_cvar_tab_init_stmt, below. 
   We also need to check if the CVAR is an argblk fp
   wrapper, which does not have the actual struct tokens
   in the input. */
static void vartab_struct_entry_w_type (MESSAGE_STACK messages,
					int struct_keyword_idx,
					CVAR *struct_cvar,
					char *type_buf_out) {
  char decl_buf[MAXMSG], *name_tmp,
    subscript_size_buf[64];
  int i, type_tag_idx, stack_top, struct_body_end, n_braces;

  if (!(struct_cvar -> attrs & CVAR_ATTR_FP_ARGBLK)) {
    stack_top = get_stack_top (messages);
    if ((type_tag_idx = nextlangmsg (messages, struct_keyword_idx)) != ERROR) {
      struct_type_tag (struct_cvar, M_NAME(messages[type_tag_idx]),
		       type_buf_out);
    }
    n_braces = 0;
    struct_body_end = -1;
    name_tmp = messages[type_tag_idx] -> name;
    for (i = struct_keyword_idx; i > stack_top; i--) {
      switch (M_TOK(messages[i]))
	{
	case WHITESPACE:
	case NEWLINE:
	  continue;
	  break;
	case OPENBLOCK:
	  ++n_braces;
	  break;
	case CLOSEBLOCK:
	  --n_braces;
	  if (n_braces == 0) {
	    struct_body_end = i;
	    goto vsewt_found_closebrace;
	  }
	}
      if (i == type_tag_idx)
	messages[type_tag_idx] -> name = strdup (type_buf_out);
    }
  vsewt_found_closebrace:
    if (struct_body_end != -1) {
      toks2str (messages, struct_keyword_idx, struct_body_end, decl_buf);
      strcat (decl_buf, ";\n");
      if (interpreter_pass == parsing_pass)
	function_vartab_statement (decl_buf);
      else if (interpreter_pass == method_pass)
	method_vartab_statement (decl_buf);
    }
    __xfree (MEMADDR(messages[type_tag_idx] -> name));
    messages[type_tag_idx] -> name = name_tmp;
  } else {
    struct_type_tag (struct_cvar, struct_cvar -> type, type_buf_out);
    sprintf (subscript_size_buf, "%d", struct_cvar -> members ->
	     initializer_size);
    strcatx (decl_buf, "struct ", type_buf_out, " {\n\t",
	     struct_cvar -> members -> type, " ",
	     struct_cvar -> members -> name, "[", subscript_size_buf,
	     "];\n",
	     "};\n", NULL);
    if (interpreter_pass == parsing_pass)
      function_vartab_statement (decl_buf);
    else if (interpreter_pass == method_pass)
      method_vartab_statement (decl_buf);
  }
}

/* Here, also, see the comments in method_cvar_tab_entry, below. */
static void vartab_struct_entry (MESSAGE_STACK messages,
					int struct_keyword_idx,
					char *type_buf_out) {
  char type_buf[MAXMSG], decl_buf[MAXMSG], *decl_tag_tmp;
  int i, stack_top, struct_body_end, n_braces;

  stack_top = get_stack_top (messages);

  /* paste our internal type tag here.  We'll replace it with the
     original (it's just the "struct" keyword) below. */
  struct_decl_tag (type_buf_out);
  decl_tag_tmp = messages[struct_keyword_idx] -> name;
  sprintf (type_buf, "struct %s", type_buf_out);
  messages[struct_keyword_idx] -> name = strdup (type_buf);

  n_braces = 0;
  struct_body_end = -1;
  /* name_tmp = messages[type_tag_idx] -> name; */
  for (i = struct_keyword_idx; i > stack_top; i--) {
    switch (M_TOK(messages[i]))
      {
      case WHITESPACE:
      case NEWLINE:
	continue;
	break;
      case OPENBLOCK:
	++n_braces;
	break;
      case CLOSEBLOCK:
	--n_braces;
	if (n_braces == 0) {
	  struct_body_end = i;
	  goto vse_found_closeblock;
	}
      }
  }
 vse_found_closeblock:
  if (struct_body_end != -1) {
    toks2str (messages, struct_keyword_idx, struct_body_end, decl_buf);
    strcat (decl_buf, ";\n");
    if (interpreter_pass == method_pass) 
      method_vartab_statement (decl_buf);
    else if (interpreter_pass == parsing_pass)
      function_vartab_statement (decl_buf);
  }
  __xfree (MEMADDR(messages[struct_keyword_idx] -> name));
  messages[struct_keyword_idx] -> name = decl_tag_tmp;
}

static char *cv_tab_pfx_0 = " *";
static char *cv_tab_pfx_1 = " **";
static char *cv_tab_pfx_2 = " ***";
static char *cv_tab_pfx_misc = " ";

/* the message stack and index are here mainly to collect 
   struct declarations. In that case, idx should point to 
   the, "struct" keyword.
*/

void method_cvar_tab_entry (MESSAGE_STACK messages, int idx,
				CVAR *var_list) {

  CVAR *c;
  char vartab_entry[MAXLABEL];
  char vartab_init_entry[MAXMSG];
  char struct_type_buf[MAXLABEL];
  bool struct_has_tag_b = false;
  char *tab_pfx = "";
  int struct_type_idx;

  for (c = var_list; c; c = c -> next) {

    if (c -> scope != LOCAL_VAR)
      continue;

    switch (c -> n_derefs)
      {
      case 0:
	tab_pfx = cv_tab_pfx_0;
	break;
      case 1:
	tab_pfx = cv_tab_pfx_1;
	break;
      case 2:
	tab_pfx = cv_tab_pfx_2;
	break;
      default:
	warning (messages[idx], "C variable %s has an unsupported "
		 "(so far) number of dereferences (%d).", c -> name,
		 c -> n_derefs);
	tab_pfx = cv_tab_pfx_misc;
	break;
      }

    if (c -> attrs == (CVAR_ATTR_FN_PTR_DECL|CVAR_ATTR_FN_NO_PARAMS)) {
      strcatx (vartab_entry, c -> type, " *(**",
	       new_methods[new_method_ptr+1] -> method -> selector,
	       "_", c -> name, ")();\n", NULL);
      method_vartab_statement (vartab_entry);
    } else if ((c -> attrs & CVAR_ATTR_STRUCT) &&
	       (c -> attrs & CVAR_ATTR_STRUCT_DECL)) {
      /* 
       *   Local struct declarations with the struct type; e.g.;
       *
       *     struct _structtype {
       *       int mbr;
       *     } mystruct;
       *
       *   The vartab_struct_entry_w_type function, above, outputs
       *   this similiar to this example, i.e., without a tag, 
       *   and the clause below formats the tag separately.   
       *
       *   or this:
       *
       *     struct _structtype {
       *       int mbr;
       *     };
       *
       *  So we need to have struct_has_tag to check whether
       *  we need to format the tag here.
       *
       */

      vartab_struct_entry_w_type (messages, idx, c, struct_type_buf);

      if (((struct_has_tag_b = struct_has_tag (messages, idx)) == true) ||
	  (c -> attrs & CVAR_ATTR_FP_ARGBLK)) {
	strcatx (vartab_entry, c -> qualifier, " ",
		 struct_type_buf, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
	method_vartab_statement (vartab_entry);
      }
    } else if (c -> attrs == CVAR_ATTR_STRUCT_DECL) {
      /* 
       *   A local struct declaration like this:
       *
       *     struct {
       *       int mbr;
       *     } mystruct;
       *
       *  In this case, we add our own type tag to the cvartab
       *  declaration.
       *
       */
      vartab_struct_entry (messages, idx, struct_type_buf);
      strcatx (vartab_entry, c -> type, " ", struct_type_buf, tab_pfx,
	       new_methods[new_method_ptr+1] -> method -> selector,
	       "_", c -> name, ";\n", NULL);
      method_vartab_statement (vartab_entry);
    } else if (c -> attrs == CVAR_ATTR_STRUCT) {
      /*
       *  A struct declaration without the definition; e.g.;
       *
       *    struct mystruct mystructtag;
       *
       *  We've already output the struct definition, above.
       */
      struct_type_idx = nextlangmsg (messages, idx);
      struct_type_tag (c, M_NAME(messages[struct_type_idx]), struct_type_buf);
      strcatx (vartab_entry, c -> qualifier, " ", struct_type_buf, tab_pfx,
	       new_methods[new_method_ptr+1] -> method -> selector,
	       "_", c -> name, ";\n", NULL);
      method_vartab_statement (vartab_entry);
    } else if (c -> attrs == CVAR_ATTR_STRUCT_PTR) {
      struct_type_idx = nextlangmsg (messages, idx);
      /* Skip over asterisks and any whitespace until we get to the 
	 tag itself. */
      while (M_TOK(messages[struct_type_idx]) == MULT)
	++struct_type_idx;
      if (M_ISSPACE(messages[struct_type_idx]))
	struct_type_idx = prevlangmsg (messages, struct_type_idx);
      if ((c -> type_attrs & CVAR_TYPE_OBJECT) && (c -> n_derefs == 1)) {
	strcpy (struct_type_buf, c -> type);
      } else {
	struct_type_tag (c, M_NAME(messages[struct_type_idx]), struct_type_buf);
      }
      strcatx (vartab_entry, c -> qualifier, " ", struct_type_buf, tab_pfx,
	       new_methods[new_method_ptr+1] -> method -> selector,
	       "_", c -> name, ";\n", NULL);
      method_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier3, " ",
		 c -> qualifier2, " ", c -> qualifier,
		 " ", c -> type, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> qualifier2, " ", c -> qualifier,
		 " ", c -> type, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
      }
      method_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONG) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier2, " ",
		 c -> qualifier, " ", c -> type, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
      }
      method_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_SHORT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier2, " ", c -> qualifier, " ",
		 c -> type, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
      }
      method_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_INT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> type, tab_pfx,
		 new_methods[new_method_ptr+1] -> method -> selector,
		 "_", c -> name, ";\n", NULL);
      }
      method_vartab_statement (vartab_entry);
    } else {
      strcatx (vartab_entry, c -> type, tab_pfx,
	       new_methods[new_method_ptr+1] -> method -> selector,
	       "_", c -> name, ";\n", NULL);
      method_vartab_statement (vartab_entry);
    }

    if (c -> type_attrs & CVAR_TYPE_CHAR) {
      /* If the cvar was declared with a subscript, then this 
	 helps avoid a warning with a lot of compilers. */
      strcatx (vartab_init_entry, 
	       new_methods[new_method_ptr+1] -> method -> selector, "_",
	       c -> name, " = ",
	       (((c -> n_derefs == 2) ? "(char ***)" : 
		 ((c -> n_derefs == 1) ? "(char **)" : 
		  ((c -> n_derefs == 0) ? "(char *)" : "")))), "&",
	       c -> name, ";\n", NULL);
      method_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry, 
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned long long int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned long long int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned long long int *)" :
		     "")))), "&",
		 c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(long long int ***)" : 
		   ((c -> n_derefs == 1) ? "(long long int **)" : 
		    ((c -> n_derefs == 0) ? "(long long int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      method_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONG) {
      if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
	strcatx (vartab_init_entry, 
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(long double ***)" : 
		   ((c -> n_derefs == 1) ? "(long double **)" : 
		    ((c -> n_derefs == 0) ? "(long double *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      } else {
	if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	  strcatx (vartab_init_entry, 
		   new_methods[new_method_ptr+1] -> method -> selector, "_",
		   c -> name, " = ",
		   (((c -> n_derefs == 2) ? "(unsigned long int ***)" : 
		     ((c -> n_derefs == 1) ? "(unsigned long int **)" : 
		      ((c -> n_derefs == 0) ? "(unsigned long int *)" :
		       "")))), "&",
		   c -> name, ";\n", NULL);
	} else {
	  strcatx (vartab_init_entry, 
		   new_methods[new_method_ptr+1] -> method -> selector, "_",
		   c -> name, " = ",
		   (((c -> n_derefs == 2) ? "(long int ***)" : 
		     ((c -> n_derefs == 1) ? "(long int **)" : 
		      ((c -> n_derefs == 0) ? "(long int *)" : "")))), "&",
		   c -> name, ";\n", NULL);
	}
      }
      method_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_SHORT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry, 
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned short int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned short int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned short int *)" : "")))),
		 "&", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(short int ***)" : 
		   ((c -> n_derefs == 1) ? "(short int **)" : 
		    ((c -> n_derefs == 0) ? "(short int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      method_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_INT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry, 
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(int ***)" : 
		   ((c -> n_derefs == 1) ? "(int **)" : 
		    ((c -> n_derefs == 0) ? "(int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      method_vartab_init_statement (vartab_init_entry);
    } else if (c -> attrs == (CVAR_ATTR_STRUCT|CVAR_ATTR_STRUCT_DECL)) {
      /* See the comments above. */
      if (struct_has_tag_b) {
	strcatx (vartab_init_entry,
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name, " = (struct ", struct_type_buf," *)&",
		 c -> name, ";\n", NULL);
	method_vartab_init_statement (vartab_init_entry);
      }
    } else if (((c -> attrs == CVAR_ATTR_STRUCT) ||
		(c -> attrs == CVAR_ATTR_STRUCT_DECL)) ||
	       (c -> attrs & CVAR_ATTR_FP_ARGBLK)) {
      strcatx (vartab_init_entry,
	       new_methods[new_method_ptr+1] -> method -> selector, "_",
	       c -> name, " = (struct ", struct_type_buf," *)&",
	       c -> name, ";\n", NULL);
      method_vartab_init_statement (vartab_init_entry);
    } else if (c -> attrs == CVAR_ATTR_STRUCT_PTR) {
      /* This should be expanded to handle any typedef, besides
	 OBJECT when we get examples in source code. */
      if ((c -> type_attrs & CVAR_TYPE_OBJECT) && (c -> n_derefs == 1)) {
	strcatx (vartab_init_entry,
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name,
		 " = (", struct_type_buf, " **)&",
		 c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry,
		 new_methods[new_method_ptr+1] -> method -> selector, "_",
		 c -> name,
		 " = (struct ", struct_type_buf, " **)&",
		 c -> name, ";\n", NULL);
      }
      method_vartab_init_statement (vartab_init_entry);
    } else {
      strcatx (vartab_init_entry,
	       new_methods[new_method_ptr+1] -> method -> selector, "_",
	       c -> name, " = &", c -> name, ";\n", NULL);
      method_vartab_init_statement (vartab_init_entry);
    }
  }
}

void function_cvar_tab_entry (MESSAGE_STACK messages, int idx,
				  CVAR *var_list) {

  CVAR *c;
  char vartab_entry[MAXLABEL];
  char vartab_init_entry[MAXMSG];
  char struct_type_buf[MAXLABEL];
  bool struct_has_tag_b = false;
  int struct_type_idx;
  char *tab_pfx = "";

  /* see the comments in method_cvar_tab_entry */
  for (c = var_list; c; c = c -> next) {

    if (c -> scope != LOCAL_VAR)
      continue;

    switch (c -> n_derefs)
      {
      case 0: tab_pfx = cv_tab_pfx_0; break;
      case 1: tab_pfx = cv_tab_pfx_1; break;
      case 2: tab_pfx = cv_tab_pfx_2; break;
      default:
	warning (messages[idx], "C variable %s has an unsupported "
		 "(so far) number of dereferences (%d).", c -> name,
		 c -> n_derefs);
	tab_pfx = cv_tab_pfx_misc;
	break;
      }

    if (((c -> attrs & CVAR_ATTR_STRUCT) &&
	(c -> attrs & CVAR_ATTR_STRUCT_DECL)) ||
	(c -> attrs & CVAR_ATTR_FP_ARGBLK)) {
      vartab_struct_entry_w_type (messages, idx, c, struct_type_buf);

      if (((struct_has_tag_b = struct_has_tag (messages, idx)) == true) ||
	  (c -> attrs & CVAR_ATTR_FP_ARGBLK)) {
	strcatx (vartab_entry, c -> qualifier, " ", struct_type_buf,
		 tab_pfx, fn_name, "_", c -> name, ";\n", NULL);
	function_vartab_statement (vartab_entry);
      }
    } else if (c -> attrs == CVAR_ATTR_STRUCT_DECL) {
      vartab_struct_entry (messages, idx, struct_type_buf);
      strcatx (vartab_entry, c -> type, " ", struct_type_buf,
	       tab_pfx, fn_name, "_", c -> name, ";\n", NULL);
      function_vartab_statement (vartab_entry);
    } else if (c -> attrs == CVAR_ATTR_STRUCT) {
      struct_type_idx = nextlangmsg (messages, idx);
      struct_type_tag (c, M_NAME(messages[struct_type_idx]), struct_type_buf);
      strcatx (vartab_entry, c -> qualifier, " ", struct_type_buf,
	       tab_pfx, fn_name, "_", c -> name, ";\n", NULL);
      function_vartab_statement (vartab_entry);
    } else if (c -> attrs == CVAR_ATTR_STRUCT_PTR) {
      struct_type_idx = nextlangmsg (messages, idx);
      struct_type_tag (c, M_NAME(messages[struct_type_idx]), struct_type_buf);
      strcatx (vartab_entry, c -> qualifier, " ", struct_type_buf,
	       tab_pfx, fn_name, "_", c -> name, ";\n", NULL);
      function_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_CHAR) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, tab_pfx,
		 fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> type, tab_pfx,
		 fn_name, "_", c -> name, ";\n", NULL);
      }
      function_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
      /* this is the vartab entry that we need the
	 -Wno-pointer-to-int-cast compiler flag for. */
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier3, " ", c -> qualifier2, " ",
		 c -> qualifier, " ", c -> type, tab_pfx,
		 fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> qualifier2, " ", c -> qualifier,
		 " ", c -> type, tab_pfx,
		 fn_name, "_", c -> name, ";\n", NULL);
      }
      function_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONG) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier2, " ",
		 c -> qualifier, " ", c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      }
      function_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_SHORT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier2, " ", c -> qualifier,
		 " ", c -> type, tab_pfx,
		 fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, tab_pfx,
		 fn_name, "_", c -> name, ";\n", NULL);
      }
      function_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_INT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type,
		 tab_pfx, fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> type, tab_pfx,
		 fn_name, "_", c -> name, ";\n", NULL);
      }
      function_vartab_statement (vartab_entry);
    } else {
      strcatx (vartab_entry, c -> type, tab_pfx,
	       fn_name, "_", c -> name, ";\n", NULL);
      function_vartab_statement (vartab_entry);
    }

    if (c -> type_attrs & CVAR_TYPE_CHAR) {
      /* If the cvar was declared with a subscript, then this 
	 helps avoid a warning with a lot of compilers. */
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry,
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned char ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned char **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned char *)" :
		     "")))), "&", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry,
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(char ***)" : 
		   ((c -> n_derefs == 1) ? "(char **)" : 
		    ((c -> n_derefs == 0) ? "(char *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned long long int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned long long int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned long long int *)" :
		     "")))), "&", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(long long int ***)" : 
		   ((c -> n_derefs == 1) ? "(long long int **)" : 
		    ((c -> n_derefs == 0) ? "(long long int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONG) {
      if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
	strcatx (vartab_init_entry, 
		fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(long double ***)" : 
		   ((c -> n_derefs == 1) ? "(long double **)" : 
		    ((c -> n_derefs == 0) ? "(long double *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      } else {
	if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	  strcatx (vartab_init_entry, 
		   fn_name, "_", c -> name, " = ",
		   (((c -> n_derefs == 2) ? "(unsigned long int ***)" : 
		     ((c -> n_derefs == 1) ? "(unsigned long int **)" : 
		      ((c -> n_derefs == 0) ? "(unsigned long int *)" : "")))),
		   "&", c -> name, ";\n", NULL);
	} else {
	  strcatx (vartab_init_entry, 
		   fn_name, "_", c -> name, " = ",
		   (((c -> n_derefs == 2) ? "(long int ***)" : 
		     ((c -> n_derefs == 1) ? "(long int **)" : 
		      ((c -> n_derefs == 0) ? "(long int *)" : "")))), "&",
		   c -> name, ";\n", NULL);
	}
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_SHORT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned short int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned short int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned short int *)" : "")))),
		 "&", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(short int ***)" : 
		   ((c -> n_derefs == 1) ? "(short int **)" : 
		    ((c -> n_derefs == 0) ? "(short int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_INT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(int ***)" : 
		   ((c -> n_derefs == 1) ? "(int **)" : 
		    ((c -> n_derefs == 0) ? "(int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> attrs == (CVAR_ATTR_STRUCT|CVAR_ATTR_STRUCT_DECL)) {
       /* see the comments in method_cvar_tab_entry, here too */
      if (struct_has_tag_b) {
	strcatx (vartab_init_entry,
		 fn_name, "_",
		 c -> name, " = (struct ", struct_type_buf," *)&",
		 c -> name, ";\n", NULL);
	function_vartab_init_statement (vartab_init_entry);
      }
    } else if (((c -> attrs == CVAR_ATTR_STRUCT) ||
		(c -> attrs == CVAR_ATTR_STRUCT_DECL)) ||
	       (c -> attrs & CVAR_ATTR_FP_ARGBLK)) {
      strcatx (vartab_init_entry,
	       fn_name, "_",
	       c -> name, " = (struct ", struct_type_buf," *)&",
	       c -> name, ";\n", NULL);
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> attrs == CVAR_ATTR_STRUCT_PTR) {
      strcatx (vartab_init_entry, fn_name, "_", c -> name,
	       " = (struct ", struct_type_buf, " **)&",
	       c -> name, ";\n", NULL);
      function_vartab_init_statement (vartab_init_entry);
    } else {
      strcatx (vartab_init_entry, fn_name, "_", c -> name,
	       " = &", c -> name, ";\n", NULL);
      function_vartab_init_statement (vartab_init_entry);
    }
  }
}

void function_param_cvar_tab_entry (MESSAGE_STACK messages, int idx,
					CVAR *var_list) {

  CVAR *c;
  char vartab_entry[MAXLABEL];
  char vartab_init_entry[MAXMSG];
  char struct_type_buf[MAXLABEL];
  bool struct_has_tag_b = false;

  /* see the comments in method_cvar_tab_entry */
  for (c = var_list; c; c = c -> next) {

    /* ARG_VAR is the scope of function parameters. */
    if (c -> scope != ARG_VAR)
      continue;

    /* this is the only parameter if the function has a "void" keyword
       for the parameter list. */
    if ((c -> type_attrs & CVAR_TYPE_VOID) && (*c -> name == 0))
      break;

    if (((c -> attrs & CVAR_ATTR_STRUCT) &&
	(c -> attrs & CVAR_ATTR_STRUCT_DECL)) ||
	(c -> attrs & CVAR_ATTR_FP_ARGBLK)) {
      vartab_struct_entry_w_type (messages, idx, c, struct_type_buf);
      vartab_struct_entry_w_type (messages, idx, c, struct_type_buf);

      if (((struct_has_tag_b = struct_has_tag (messages, idx)) == true) ||
	  (c -> attrs & CVAR_ATTR_FP_ARGBLK)) {
	strcatx (vartab_entry, c -> qualifier, " ", struct_type_buf, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))),
		 fn_name, "_", c -> name, ";\n", NULL);
	function_vartab_statement (vartab_entry);
      }
    } else if (c -> attrs == CVAR_ATTR_STRUCT_DECL) {
      vartab_struct_entry (messages, idx, struct_type_buf);
      strcatx (vartab_entry, c -> type, " ", struct_type_buf, " ",
	       (((c -> n_derefs == 2) ? "***" : 
		 ((c -> n_derefs == 1) ? "**" : 
		  ((c -> n_derefs == 0) ? "*" : "")))),
	       fn_name, "_", c -> name, ";\n", NULL);
      function_vartab_statement (vartab_entry);
    } else if (c -> attrs == CVAR_ATTR_STRUCT) {
      strcatx (vartab_entry, c -> qualifier, " ", c -> type, " ",
	       (((c -> n_derefs == 2) ? "***" : 
		 ((c -> n_derefs == 1) ? "**" : 
		  ((c -> n_derefs == 0) ? "*" : "")))),
	       fn_name, "_", c -> name, ";\n", NULL);
      function_vartab_statement (vartab_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
      /* this is the vartab entry that we need the
	 -Wno-pointer-to-int-cast compiler flag for. */
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier3, " ", c -> qualifier2,
		 " ", c -> qualifier, " ", c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> qualifier2, " ", c -> qualifier, " ",
		 c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      }
      function_vartab_statement (vartab_entry);
      if (c -> n_derefs > 2)
      _warning 
     	("Warning: Variable %s has an unsupported number of dereferences.\n",
	 c -> name);
    } else if (c -> type_attrs & CVAR_TYPE_LONG) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier2, " ",
		 c -> qualifier, " ", c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      }

      function_vartab_statement (vartab_entry);
      if (c -> n_derefs > 2)
      _warning 
     	("Warning: Variable %s has an unsupported number of dereferences.\n",
	 c -> name);
    } else if (c -> type_attrs & CVAR_TYPE_INT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      }

      function_vartab_statement (vartab_entry);
      if (c -> n_derefs > 2)
      _warning 
     	("Warning: Variable %s has an unsupported number of dereferences.\n",
	 c -> name);
    } else if (c -> type_attrs & CVAR_TYPE_SHORT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_entry, c -> qualifier, " ", c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_entry, c -> type, " ",
		 (((c -> n_derefs == 2) ? "***" : 
		   ((c -> n_derefs == 1) ? "**" : 
		    ((c -> n_derefs == 0) ? "*" : "")))), 
		 fn_name, "_", c -> name, ";\n", NULL);
      }
      function_vartab_statement (vartab_entry);
      if (c -> n_derefs > 2)
      _warning 
     	("Warning: Variable %s has an unsupported number of dereferences.\n",
	 c -> name);
    } else if (c -> type_attrs & CVAR_TYPE_FLOAT) {
      strcatx (vartab_entry, c -> type, " ",
	       (((c -> n_derefs == 2) ? "***" : 
		 ((c -> n_derefs == 1) ? "**" : 
		  ((c -> n_derefs == 0) ? "*" : "")))), 
	       fn_name, "_", c -> name, ";\n", NULL);

      function_vartab_statement (vartab_entry);
      if (c -> n_derefs > 2)
      _warning 
     	("Warning: Variable %s has an unsupported number of dereferences.\n",
	 c -> name);
    } else {
      strcatx (vartab_entry, c -> type, " ",
	       (((c -> n_derefs == 2) ? "***" : 
		 ((c -> n_derefs == 1) ? "**" : 
		  ((c -> n_derefs == 0) ? "*" : "")))), 
	       fn_name, "_", c -> name, ";\n", NULL);

      function_vartab_statement (vartab_entry);
      if (c -> n_derefs > 2)
      _warning 
     	("Warning: Variable %s has an unsupported number of dereferences.\n",
	 c -> name);
    }

    if (c -> type_attrs & CVAR_TYPE_CHAR) {
      /* If the cvar was declared with a subscript, then this 
	 helps avoid a warning with a lot of compilers. */
      strcatx (vartab_init_entry,
	       fn_name, "_", c -> name, " = ",
	       (((c -> n_derefs == 2) ? "(char ***)" : 
		 ((c -> n_derefs == 1) ? "(char **)" : 
		  ((c -> n_derefs == 0) ? "(char *)" : "")))), "&",
	       c -> name, ";\n", NULL);
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONGLONG) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned long long int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned long long int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned long long int *)" :
		     "")))), "&", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(long long int ***)" : 
		   ((c -> n_derefs == 1) ? "(long long int **)" : 
		    ((c -> n_derefs == 0) ? "(long long int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_LONG) {
      if (c -> type_attrs & CVAR_TYPE_DOUBLE) {
	strcatx (vartab_init_entry, 
		fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(long double ***)" : 
		   ((c -> n_derefs == 1) ? "(long double **)" : 
		    ((c -> n_derefs == 0) ? "(long double *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      } else {
	if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	  strcatx (vartab_init_entry, 
		   fn_name, "_", c -> name, " = ",
		   (((c -> n_derefs == 2) ? "(unsigned long int ***)" : 
		     ((c -> n_derefs == 1) ? "(unsigned long int **)" : 
		      ((c -> n_derefs == 0) ? "(unsigned long int *)" :
		       "")))), "&", c -> name, ";\n", NULL);
	} else {
	  strcatx (vartab_init_entry, 
		   fn_name, "_", c -> name, " = ",
		   (((c -> n_derefs == 2) ? "(long int ***)" : 
		     ((c -> n_derefs == 1) ? "(long int **)" : 
		      ((c -> n_derefs == 0) ? "(long int *)" : "")))), "&",
		   c -> name, ";\n", NULL);
	}
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_SHORT) {
      if (c -> is_unsigned || (c -> type_attrs & CVAR_TYPE_UNSIGNED)) {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned short int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned short int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned short int *)" : "")))),
		 "&", c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(short int ***)" : 
		   ((c -> n_derefs == 1) ? "(short int **)" : 
		    ((c -> n_derefs == 0) ? "(short int *)" : "")))),
		 "&", c -> name, ";\n", NULL);
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_INT) {
      if (c -> type_attrs & CVAR_TYPE_UNSIGNED) {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(unsigned int ***)" : 
		   ((c -> n_derefs == 1) ? "(unsigned int **)" : 
		    ((c -> n_derefs == 0) ? "(unsigned int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      } else {
	strcatx (vartab_init_entry, 
		 fn_name, "_", c -> name, " = ",
		 (((c -> n_derefs == 2) ? "(int ***)" : 
		   ((c -> n_derefs == 1) ? "(int **)" : 
		    ((c -> n_derefs == 0) ? "(int *)" : "")))), "&",
		 c -> name, ";\n", NULL);
      }
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> attrs == (CVAR_ATTR_STRUCT|CVAR_ATTR_STRUCT_DECL)) {
       /* see the comments in method_cvar_tab_entry, here too */
      if (struct_has_tag_b) {
	strcatx (vartab_init_entry,
		 fn_name, "_",
		 c -> name, " = (struct ", struct_type_buf," *)&",
		 c -> name, ";\n", NULL);
	function_vartab_init_statement (vartab_init_entry);
      }
    } else if (((c -> attrs == CVAR_ATTR_STRUCT) ||
		(c -> attrs == CVAR_ATTR_STRUCT_DECL)) ||
	       (c -> attrs & CVAR_ATTR_FP_ARGBLK)) {
      strcatx (struct_type_buf, "(struct ", c -> type,
	       (((c -> n_derefs == 2) ? "***)" :
		 ((c -> n_derefs == 1) ? "**)" :
		  ((c -> n_derefs == 0) ? "*)" : "")))), NULL);
					 
      strcatx (vartab_init_entry, fn_name, "_",
	       c -> name, " = ", struct_type_buf,
		"&", c -> name, ";\n", NULL);
      function_vartab_init_statement (vartab_init_entry);
    } else if (c -> type_attrs & CVAR_TYPE_FLOAT) {
      strcatx (vartab_init_entry, 
	       fn_name, "_", c -> name, " = ",
	       (((c -> n_derefs == 2) ? "(float ***)" : 
		 ((c -> n_derefs == 1) ? "(float **)" : 
		  ((c -> n_derefs == 0) ? "(float *)" : "")))), "&",
	       c -> name, ";\n", NULL);
      function_vartab_init_statement (vartab_init_entry);
    } else {
      strcatx (vartab_init_entry, fn_name, "_", c -> name,
	       " = &", c -> name, ";\n", NULL);
      function_vartab_init_statement (vartab_init_entry);
    }
  }
}

int new_method_has_contain_argblk_attr (void) {

  if (new_method_ptr < MAXARGS) {
    if (new_methods[new_method_ptr+1] -> method -> attrs & 
	METHOD_CONTAINS_ARGBLK_ATTR)
      return TRUE;
  }
  return FALSE;
}

int method_cvar_alias_basename (METHOD *m, CVAR *c, char *s) {
  return strcatx (s, m -> selector, "_", c -> name, NULL);
}

int function_cvar_alias_basename (CVAR *c, char *s) {
  return strcatx (s, fn_name, "_", c -> name, NULL);
}

CVAR *get_var_from_cvartab_name (char *name) {
  CVAR *c;
  char *selector_start = name, *var_start;
  int len;

  if (!argblk)
    return NULL;
  
  while (*selector_start == '*')
    ++selector_start;

  switch (interpreter_pass)
    {
    case method_pass:
      len = strlen (new_methods[new_method_ptr+1] -> method -> selector) + 1;
      break;
    default:
      len = strlen (fn_name) + 1;
      break;
    }

  var_start = selector_start + len;

  if (((c = get_local_var (var_start)) != NULL) ||
      ((c = get_global_var (var_start)) != NULL)) {
    return c;
  }
  return NULL;
}

/* 
   method_contains_argblk () is in pattypes.c.  This fn is here
   because find_function_close () is in cparse.c, and this fn uses
   it.
*/
extern int fn_has_argblk;

int function_contains_argblk (MESSAGE_STACK messages, int fn_decl_idx) {

  int i, lookahead, fn_start_idx, fn_end_idx;
  int prev_idx = -1, prev_idx_2 = -1, prev_idx_3 = -1;
  int stack_top;

  stack_top = get_stack_top (messages);

  if ((fn_start_idx = scanforward (messages, fn_decl_idx,
				   stack_top, OPENBLOCK)) == ERROR) {
    warning (messages[fn_decl_idx],
	     "Warning: Can't find function opening block.");
    fn_has_argblk = FALSE;
    return FALSE;
  }

  if ((fn_end_idx = find_function_close (messages, fn_decl_idx,
					 stack_top)) == ERROR) {
    error (messages[fn_decl_idx], "Can't find function close.");
    /* Not reached. ... */
    fn_has_argblk = FALSE;
    return FALSE;
  }

  for (i = fn_start_idx; i >= fn_end_idx; i--) {

    if (M_ISSPACE(messages[i]))
      continue;

    for (lookahead = i - 1; lookahead >= fn_end_idx; lookahead--) {

      if (M_ISSPACE(messages[lookahead]))
       	continue;

      if ((M_TOK(messages[i]) == LABEL) && 
	  (M_TOK(messages[lookahead]) == OPENBLOCK)) {

	/*	expressions like, "else {" */
	if (is_c_keyword (messages[i] -> name))
	  break;
	/* expressions like "struct {" or "struct <name> {" */
	if (prev_idx != -1) {
	  if (is_c_keyword (messages[prev_idx] -> name))
	    break;
	}
	if (prev_idx_3 != -1) {
	  if (is_c_keyword (messages[prev_idx_3] -> name))
	    break;
	}
	if (prev_idx_2 != -1) {
	  if (is_c_keyword (messages[prev_idx_2] -> name))
	    break;
	}
	if (prev_idx != -1) {
	  if (is_c_keyword (messages[prev_idx] -> name))
	    break;
	}

	fn_has_argblk = TRUE;
	return TRUE;
      } else {
	break;
      }

    }

    prev_idx_3 = prev_idx_2;
    prev_idx_2 = prev_idx;
    prev_idx = i;
  }

  fn_has_argblk = FALSE;
  return FALSE;
}

/* Called by resolve. */
bool need_cvar_argblk_translation (CVAR *c) {
  return (IS_CVAR (c) &&
	  ((c -> scope == LOCAL_VAR) || (c -> scope == ARG_VAR)) &&
	  argblk);
}

static char *fmt_cvar_tab_ref (CVAR *c, char *buf_out) {
  char buf[MAXMSG];
  if (interpreter_pass == method_pass) {
    method_cvar_alias_basename
      (new_methods[new_method_ptr+1] -> method, c, buf);
    strcatx (buf_out, "*", buf, NULL);
  } else if (interpreter_pass == parsing_pass) {
    function_cvar_alias_basename (c, buf); 
    strcatx (buf_out, "*", buf, NULL);
  }
  return buf_out;
}

/* #define STRCPY_ARGBLK_SYMBOLS */
/* #define CHECK_SYMBOL_SIZE */
#define NEED_TAB_ENTRY(c) (c -> scope & LOCAL_VAR || c -> scope & ARG_VAR)

OBJECT *handle_cvar_argblk_translation (MESSAGE_STACK messages,
					int message_ptr,
					int next_label_ptr,
					CVAR *cvar) {
  char cvar_block_alias[MAXLABEL];
  MESSAGE *m;

  if (argblk_cvar_is_fn_argument (messages, message_ptr, cvar)) {
    return NULL;
  } 

  m = messages[message_ptr];

  if (next_label_ptr != ERROR) {

    switch (M_TOK(messages[next_label_ptr]))
      {
      case DEREF:
	if (str_eq (cvar -> type, "OBJECT") &&
	    (cvar -> n_derefs == 1)) {
	  if (NEED_TAB_ENTRY(cvar)) {
	    strcatx (m -> name, "((OBJECT *)",
		     fmt_cvar_tab_ref (cvar, cvar_block_alias), ")", NULL);
	  } else {
	    strcatx (m -> name, "((OBJECT *)", cvar -> name, ")", NULL);
	  }
#ifdef SYMBOL_SIZE_CHECK
	  check_symbol_size (m -> name);
#endif	    
	} else {
	  if (NEED_TAB_ENTRY(cvar)) {
	    strcatx (m -> name, "(",
		     fmt_cvar_tab_ref (cvar, cvar_block_alias), ")", NULL);
	  } else {
	    strcatx (m -> name, "(", cvar -> name, ")", NULL);
	  }
	}
	break;
      case PERIOD:
      case INCREMENT:
      case DECREMENT:
	if (NEED_TAB_ENTRY(cvar)) {
	  strcatx (m -> name, "(",
		   fmt_cvar_tab_ref (cvar, cvar_block_alias), ")", NULL);
	} else {
	  strcatx (m -> name, "(", cvar -> name, ")", NULL);
	}
#ifdef SYMBOL_SIZE_CHECK
	check_symbol_size (m -> name);
#endif	    
	break;
      case ARRAYOPEN:
	if (!subscripted_int_in_code_block_error (messages, message_ptr,
						  cvar)) {
	  if (cvar -> type_attrs & CVAR_TYPE_CHAR &&
	      cvar -> n_derefs > 0) {
	    int lookahead2;
	    char *s, errexpr[64];
	    lookahead2 = scanforward (messages, next_label_ptr,
				      get_stack_top (messages),
				      ARRAYCLOSE);
	    s = collect_tokens (messages, message_ptr, lookahead2);
	    strcpy (errexpr, s);
	    __xfree (MEMADDR(s));
	    error (messages[message_ptr],
		   "Subscripted char array expressions like:\n\n\t%s\n\n"
		   "are not allowed in argument blocks.  You should "
		   "consider using a String object.", errexpr);
	  }
	  fmt_cvar_tab_ref (cvar, m -> name);
#ifdef SYMBOL_SIZE_CHECK
	  check_symbol_size (m -> name);
#endif	    
	}
	break;
      default:
	fmt_cvar_tab_ref (cvar, m -> name);
#ifdef SYMBOL_SIZE_CHECK
	check_symbol_size (cvar_block_alias);
#endif	    
	break;
      }
  } else {
#ifdef SYMBOL_SIZE_CHECK
    check_symbol_size (cvar_block_alias);
#endif	    
    strcpy (m -> name, cvar_block_alias);
  }
  ++m -> evaled;
       
  return NULL;
}

OBJECT *argblk_CVAR_name_to_msg (MESSAGE *m,
				 CVAR *cvar) {
  fmt_cvar_tab_ref (cvar, m -> name);

  return NULL;
}
