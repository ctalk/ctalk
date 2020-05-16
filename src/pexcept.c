/* $Id: pexcept.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdlib.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"
#include "typeof.h"

EXCEPTION parse_exception = no_x;  /* Global exception descriptor.  */

/* extern DEFINITION *last_symbol; */    /* Last symbol defined by preprocessor. */

/* static DEFINITION *last_symbol_env; *//* Last symbol defined before 
				      preprocessing.                       */
extern bool preprocessing;      /* Declared in preprocess.c.            */
extern bool constant_expr;      /* Declared in cparse.c.                */

int handle_parse_exception (PARSER *p, int frame) {

  int retval = TRUE;

  switch (parse_exception) 
    {
    case false_assertion_x:
      abort ();
      break;
    case cplusplus_header_x:
    case mismatched_paren_x:
    case no_x:
    case undefined_param_class_x:
    case undefined_label_x:
    case undefined_type_x:
    default:
      parse_exception = no_x;
      retval = FALSE;
      break;
    }
  return retval;
}

/* int handle_preprocess_exception (MESSAGE_STACK messages, int start, int end) { */

/*   DEFINITION *t; */
/*   int retval = TRUE; */

/*   switch (parse_exception)  */
/*     { */
/*     case cplusplus_header_x: */
/*       parse_exception = no_x; */
/*       /\* */
/*        * Delete the symbols defined by the system header. */
/*        *\/ */
/*       if (last_symbol) { */
/* 	for (t = last_symbol; ; t = t -> prev) { */
/* 	  if (t == last_symbol_env) */
/* 	    break; */
/* 	  if (t -> prev) { */
/* 	    last_symbol = t -> prev; */
/* 	    t -> prev -> next = NULL; */
/* 	  } else { */
/* 	    last_symbol = NULL; */
/* 	  } */
/* 	  free (t); */
/* 	  if (t == last_symbol_env) */
/* 	    break; */
/* 	} */
/*       } */
/*       break; */
/*     case mismatched_paren_x: */
/*       parse_exception = no_x; */
/*       break; */
/*     case false_assertion_x: */
/*       abort (); */
/*       break; */
/*     case no_x: */
/*     case parse_error_x: */
/*     case undefined_param_class_x: */
/*     case undefined_label_x: */
/*     case undefined_type_x: */
/*     case elif_w_o_if_x: */
/*     default: */
/*       parse_exception = no_x; */
/*       retval = FALSE; */
/*       break; */
/*     } */
/*   return retval; */
/* } */

/* void set_preprocess_env (void) { */
/*   preprocessing = True; */
/*   constant_expr = True; */
/*   last_symbol_env = last_symbol; */
/* } */



