/* $Id: enum.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2014, 2017 Robert Kiesling, rk3314042@gmail.com.
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
 *  Parse an enum declaration and return a CVAR.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"
#include "cvar.h"
#include "parser.h"

static int enum_states[] = {
  LABEL, LABEL,       /* 0. */
  LABEL, OPENBLOCK,   /* 1. */
  OPENBLOCK, LABEL,   /* 2. */
  LABEL, EQ,          /* 3.  If we see an initializer, scan forward to the
			   ARGSEPARATOR or to the CLOSING BLOCK. */
  ARGSEPARATOR, LABEL,  /* 4. */
  CLOSEBLOCK, SEMICOLON,  /* 5. */
  CLOSEBLOCK, LABEL,      /* 6. */
  LABEL, SEMICOLON,       /* 7. */
  LABEL, ARGSEPARATOR,    /* 8. */
  LABEL, CLOSEBLOCK,      /* 9. */
  ARGSEPARATOR, ASTERISK,  /* 10 */
  ASTERISK, LABEL,        /* 11. */
  ARGSEPARATOR, CLOSEBLOCK, /* 12. */
  /* Here in case there's a preprocessor include line marker
     in the enum body. */
  OPENBLOCK, PREPROCESS,   /* 13. */
  PREPROCESS,  INTEGER,    /* 14. */
  INTEGER, LITERAL,    /* 15. */
  LITERAL, INTEGER,    /* 16. */
  INTEGER,  LABEL,   /* 17. */
  ARGSEPARATOR, PREPROCESS,  /* 18 */
  -1,                  0
};

#define ENUM_STATE_COLS 2

#define ENUM_STATE(x) (check_state ((x), messages, \
              enum_states, ENUM_STATE_COLS))

static char default_enum_tag[] = "enum";
static VARNAME enum_tags[MAXARGS];
static int n_enum_tags;

static char enum_types[MAXLABEL][MAXARGS];
static int n_enum_types;

static CVAR *enum_members,
  *enum_members_ptr;

extern int __remove_duplicate_vars; /* Declared in class.c and set when
				       parsing cached libraries.        */
extern bool typedef_check;       /* Declared in cparse.c.            */

CVAR *enum_decl (MESSAGE_STACK messages, int start) {

  CVAR *c, *enums, *enums_ptr;
  CVAR *c_mbr = NULL;   /* Avoid a warning. */
  int i, j,
    state = 0,          /* Avoid a warning here, too. */
    n_braces, 
    stack_end,
    initializer_end;
  bool members_list; 
  MESSAGE *m;
  char init_val[MAXMSG];

  n_enum_tags = n_enum_types = 0;

  enum_members = enum_members_ptr = NULL;

  members_list = False;

  stack_end = get_stack_top (messages);

  for (i = start, n_braces = 0; i > stack_end; i--) {
    
    m = messages[i];

    if ((m -> tokentype == WHITESPACE) ||
	(m -> tokentype == NEWLINE))
      continue;

    if (state == 5 || state == 7) {
      if (m -> tokentype == SEMICOLON)
	goto done;
    }

    /*
     *  Don't check the state of the initializer, because it
     *  is simply stored as the enum member's value.
     */
    if ((state = ENUM_STATE (i)) == ERROR) {
      if (m -> tokentype != EQ)
	warning (m, "Syntax error.");
    }

    switch (m -> tokentype)
      {
      case LABEL:
	if (is_enum_member (M_NAME(m))) {
	  if (__remove_duplicate_vars) {
	    if (typedef_check) {
	    } else {
	      int __end, __idx;
	      __end = find_declaration_end (messages, start, stack_end);
	      for (__idx = start; __idx >= __end; __idx--) {
		if ((M_TOK(messages[__idx]) != WHITESPACE) &&
		    (M_TOK(messages[__idx]) != NEWLINE)) {
		  strcpy (messages[__idx]->name, " ");
		  messages[__idx] -> tokentype = WHITESPACE;
		}
	      }
	      goto done;
	    }
	  } else {
	    warning (m, "Redefinition of %s.", M_NAME(m));
	  }
	}
	if (!strcmp (m -> name, "enum")) {
	  strcpy (enum_types[n_enum_types++], m -> name);
	} else {
	  if (members_list == True) {
	  switch (state)
	    {
	    case 3:                  /* Initializer follows.          */
	    case 8:                  /* No initializer.               */
	    case 9:                  /* Final member, no initializer. */
	      _new_cvar (CVARREF(c_mbr));
	      strcpy (c_mbr -> name, m -> name);
	      if (!enum_members_ptr) {
		enum_members = enum_members_ptr = c_mbr;
	      } else {
		enum_members_ptr -> next = c_mbr;
		c_mbr -> prev = enum_members_ptr;
		enum_members_ptr = c_mbr;
	      }
	      break;
	    }
	  } else {
	    switch (state)
	      {
	      case 1:  /* <enum_tag> { */
		strcpy (enum_types[n_enum_types++], m -> name);
		break;
	      case 7:  /* <enum_tag> ; */
	      case 8:  /* <enum_tag> , */
		enum_tags[n_enum_tags++].name = m -> name;
		break;
	      }
	  } /* if (members_list == False) */
	}
	break;
      case EQ:
	initializer_end = i;
	if (enum_initializer (messages, i, &initializer_end, init_val)) {
	  c_mbr -> val.__type = PTR_T;
	  c_mbr -> val.__value.__ptr = strdup (init_val);
	}
	i = initializer_end + 1;
	break;
      case ASTERISK:
	if (members_list == False)
	  ++enum_tags[n_enum_tags].n_derefs;
	break;
      case OPENBLOCK:
	++n_braces;
	if (n_braces == 1)
	  members_list = True;
	break;
      case CLOSEBLOCK:
	--n_braces;
	if (state == 5) {  /* } ; */
	  /*
	   *  If we have a type preceding the members list,
	   *  copy it to the tag list.
	   *  If there are no tags, copy, "enum," to the
	   *  tag list.
	   */
	  if (n_enum_types == 1) {
	    enum_tags[n_enum_tags++].name = default_enum_tag;
	  } else {
	    enum_tags[n_enum_tags++].name = enum_types[--n_enum_types];
	  }
	}
	if (n_braces == 0)
	  members_list = False;
	/* Fall through. */
      case ARGSEPARATOR:
	break;
      case SEMICOLON:
	if (!n_braces)
	  goto done;
	break;
      }
  }

 done:

  enums = enums_ptr = NULL;

  for (i = 0; i < n_enum_tags; i++) {
    _new_cvar (CVARREF(c));

    strcpy (c -> name, enum_tags[i].name);
    c -> n_derefs = enum_tags[i].n_derefs;
    c -> attrs |= CVAR_ATTR_ENUM;
    if (n_enum_types > 5) {
      _warning ("enum_decl2: too many qualifiers.\n");
      strcpy (c -> type, "enum");
      continue;
    }

    for (j = 0; j < n_enum_types; j++) {

      switch ((n_enum_types - 1) - j)
	{
 	case 0:
 	  strcpy (c -> type, enum_types[j]);
 	  break;
 	case 1:
 	  strcpy (c -> qualifier, enum_types[j]);
 	  break;
 	case 2:
 	  strcpy (c -> qualifier2, enum_types[j]);
 	  break;
 	case 3:
 	  strcpy (c -> qualifier3, enum_types[j]);
 	  break;
 	case 4:
 	  strcpy (c -> qualifier4, enum_types[j]);
 	  break;
	}
    }

    /*
     *  NOTE: 
     *  Each enum points to the same member list, so
     *  if we ever have to delete enums, we will need
     *  to copy member list for each enum declaration.
     */
    c -> members = enum_members;

    if (!enums) {
      enums = enums_ptr = c;
    } else {
      enums_ptr -> next = c;
      c -> prev = enums_ptr;
      enums_ptr = c;
    }
  }

  return enums;
}

/*
 *  This should be called with, "start," pointing to 
 *  the variable tag or the, "=."
 */

char *enum_initializer (MESSAGE_STACK messages, int start, int *end,
			char *i_buf) {
  int i, j,
    stack_end,
    init_start;
  MESSAGE *m;

  stack_end = get_stack_top (messages);

  for (i = start, init_start = ERROR, *i_buf = 0; i > stack_end; i--) {

    m = messages[i];

    switch (messages[i] -> tokentype)
      {
      case EQ:
	for (j = i; (init_start == ERROR) && (j > stack_end); j--) {
	  if ((messages[j] -> tokentype == SEMICOLON) ||
	      (messages[j] -> tokentype == ARGSEPARATOR) ||
	      (messages[j] -> tokentype == CLOSEBLOCK)) {
	    warning (m, "Initializer syntax error.");
	    return NULL;
	  }
	  if ((messages[j] -> tokentype != WHITESPACE) &&
	      (messages[j] -> tokentype != EQ) &&
	      (messages[j] -> tokentype != NEWLINE)) {
	    init_start = j;
	  }
	}
	break;
      case SEMICOLON:
      case ARGSEPARATOR:
      case CLOSEBLOCK:
	*end = i;
	goto done;
      default:
	if (init_start != ERROR)
	  strcatx2 (i_buf, m -> name, NULL);
	break;
      }
  }

 done:

  return i_buf;
}
