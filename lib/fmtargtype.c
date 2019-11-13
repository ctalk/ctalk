 /* $Id: fmtargtype.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

 /*
   This file is part of Ctalk.
   Copyright © 2018  Robert Kiesling, rk3314042@gmail.com.
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

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 #include <errno.h>
 #include "lex.h"
 #include "object.h"
 #include "message.h"
 #include "ctalk.h"
 #include "typeof.h"
 #include "rtinfo.h"

/* True if the format is "%#x", false if it's "%p". */
bool ptr_fmt_is_alt_int_fmt = false;

static bool is_long_long_int_fmt (char *s) {
  int i;
  enum {
    llint_fmt_null,
    llint_fmt_percent,
    llint_fmt_precision,
    llint_fmt_length_mod,
    llint_fmt_fmt_char
  } state;

  for (i = 0, state = llint_fmt_null; s[i]; i++) {

    switch (s[i])
      {
      case '%':
	switch (state)
	  {
	  case llint_fmt_null:
	    state = llint_fmt_percent;
	    break;
	  case llint_fmt_percent:
	    state = llint_fmt_null;
	    break;
	    /* Any other state means we've reached a following '%' format. */
	  default:
	    return false;
	    break;
	  }
	break;
      case '.':
	if (state == llint_fmt_percent)
	  state = llint_fmt_precision;
	break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	if (state == llint_fmt_percent)
	  state = llint_fmt_precision;
	break;
      case 'L':
	if ((state == llint_fmt_percent) ||
	    (state == llint_fmt_precision))
	  state = llint_fmt_length_mod;
	break;
      case 'l':
	if ((state == llint_fmt_percent) ||
	    (state == llint_fmt_precision)) {
	  if (s[i+1] == 'l') {
	    ++i;
	    state = llint_fmt_length_mod;
	  }
	}
	break;
      case 'i':
      case 'd':
      case 'o':
      case 'u':
      case 'x':
      case 'X':
	if (state == llint_fmt_length_mod)
	  return true;
	break;
      case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
      case 'a': case 'A': case 'c': case 's': case 'P': case 'n':
      case 'm':
 	if ((state == llint_fmt_percent) ||
	    (state == llint_fmt_precision) ||
	    (state == llint_fmt_length_mod))
	  return false;
	break;
      }
  }
  return false;
}

static bool is_long_int_fmt (char *s) {
   int i;
   enum {
     lint_fmt_null,
     lint_fmt_percent,
     lint_fmt_precision,
     lint_fmt_arg_length_mod,
     lint_fmt_fmt_char
   } state;

   for (i = 0, state = lint_fmt_null; s[i]; i++) {

     switch (s[i])
       {
       case '%':
	 switch (state)
	   {
	   case lint_fmt_null:
	     state = lint_fmt_percent;
	     break;
	   case lint_fmt_percent:
	     state = lint_fmt_null;
	     break;
	     /* Any other state means we've reached a following '%' format. */
	   default:
	     return false;
	     break;
	   }
	 break;
       case '.':
	 if (state == lint_fmt_percent)
	   state = lint_fmt_precision;
	 break;
       case '0':
       case '1':
       case '2':
       case '3':
       case '4':
       case '5':
       case '6':
       case '7':
       case '8':
       case '9':
	 if (state == lint_fmt_percent)
	   state = lint_fmt_precision;
	 break;
       case 'l':
	 if (s[i+1] == 'l') {
	   return false;
	 } else if ((state == lint_fmt_percent) ||
		    (state == lint_fmt_precision)) {
	   state = lint_fmt_arg_length_mod;
	 }
	break;
       case 'L':
	 if ((state == lint_fmt_percent) ||
	     (state == lint_fmt_precision)) {
	   return false;
	 }
	 break;
       case 'i':
       case 'd':
       case 'o':
       case 'u':
	if ((state == lint_fmt_percent) ||
	    (state == lint_fmt_arg_length_mod) ||
	    (state == lint_fmt_precision))
	  return true;
	break;
       case 'x':
       case 'X':
	/* is_ptr_fmt_arg handles "%#x" formats. */
	if (s[i-1] == '#')
	  return false;
	else
	  return true;
	break;
       case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
       case 'a': case 'A': case 'c': case 's': case 'P': case 'n':
       case 'm':
 	if ((state == lint_fmt_percent) ||
	    (state == lint_fmt_arg_length_mod) ||
	    (state == lint_fmt_precision))
	  return false;
	break;
     }
  }
  return false;
}

static bool is_char_fmt (char *s) {
  int i;
  char prev_char = 0;
  enum {
    char_fmt_null,
    char_fmt_percent,
    char_fmt_precision,
    char_fmt_length_mod,
    char_fmt_fmt_char
  } state;

  for (i = 0, state = char_fmt_null; s[i]; i++) {

    switch (s[i])
      {
      case '%':
	switch (state)
	  {
	  case char_fmt_null:
	    state = char_fmt_percent;
	    break;
	  case char_fmt_percent:
	    state = char_fmt_null;
	    break;
	    /* Any other state means we've reached a following '%' format. */
	  default:
	    return false;
	    break;
	  }
	break;
      case '.':
	if (state == char_fmt_precision)
	  continue;
	if (state == char_fmt_percent)
	  state = char_fmt_precision;
	break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	if (state == char_fmt_percent)
	  state = char_fmt_precision;
	break;
      case 'l':
	if ((state == char_fmt_percent) ||
	    (state == char_fmt_precision))
	  state = char_fmt_length_mod;
	break;
      case 'c':
	if (state == char_fmt_percent) {
	  if (prev_char == '%') {
	    return true;
	  }
	}
	break;
      case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
      case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
      case 'a': case 'A': case 's': case 'P': case 'n': case 'm':
	if (state == char_fmt_percent || state == char_fmt_precision)
	  return false;
	break;
      }
    if (!isspace((int)s[i]))
      prev_char = s[i];
  }
  return false;
}

static bool is_ptr_fmt (char *s) {
  int i;
  enum {
    ptr_fmt_null,
    ptr_fmt_percent,
    ptr_fmt_pound,
  } state;

  for (i = 0, state = ptr_fmt_null; s[i]; i++) {

    switch (s[i])
      {
      case '%':
	switch (state)
	  {
	  case ptr_fmt_null:
	    state = ptr_fmt_percent;
	    break;
	  case ptr_fmt_percent:
	    state = ptr_fmt_null;
	    break;
	    /* Any other state means we've reached a following '%' format. */
	  default:
	    return false;
	    break;
	  }
	break;
      case 'P':
      case 'p':
	if (state == ptr_fmt_percent) {
	  ptr_fmt_is_alt_int_fmt = false;
	  return true;
	}
	break;
      case '#':
	if (state == ptr_fmt_percent)
	  state = ptr_fmt_pound;
	break;
      case 'x':
      case 'X':
	if (state == ptr_fmt_pound) {
	  ptr_fmt_is_alt_int_fmt = true;
	  return true;
	}
      case 'd': case 'i': case 'o': case 'u': 
      case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
      case 'a': case 'A': case 's': case 'c': case 'n': case 'm':
	if (state == ptr_fmt_percent || state == ptr_fmt_pound)
	  return false;
	break;
      }
  }
  return false;
}

static bool is_double_fmt (char *s) {
  int i;
  enum {
    double_fmt_null,
    double_fmt_percent,
    double_fmt_precision,
    double_fmt_length_mod,
    double_fmt_fmt_char
  } state;

  for (i = 0, state = double_fmt_null; s[i]; i++) {

    switch (s[i])
      {
      case '%':
	switch (state)
	  {
	  case double_fmt_null:
	    state = double_fmt_percent;
	    break;
	  case double_fmt_percent:
	    state = double_fmt_null;
	    break;
	    /* Any other state means we've reached a following '%' format. */
	  default:
	    return false;
	    break;
	  }
	break;
      case '.':
	if (state == double_fmt_precision)
	  continue;
	if (state == double_fmt_percent)
	  state = double_fmt_precision;
	break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	if (state == double_fmt_precision)
	  continue;
	if (state == double_fmt_percent)
	    state = double_fmt_precision;
	break;
      case 'l':
	if ((state == double_fmt_percent) ||
	    (state == double_fmt_precision)) {
	  state = double_fmt_length_mod;
	} 
	break;
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
	if ((state == double_fmt_percent) ||
	    (state == double_fmt_precision) ||
	    (state == double_fmt_length_mod))
	  return true;
	break;
      }
  }
  return false;
}

static bool is_char_ptr_fmt (char *s) {
  int i;
  char prev_char = 0;
  enum {
    char_ptr_fmt_null,
    char_ptr_fmt_percent,
    char_ptr_fmt_precision,
    char_ptr_fmt_length_mod,
    char_ptr_fmt_fmt_char
  } state;

  for (i = 0, state = char_ptr_fmt_null; s[i]; i++) {

    switch (s[i])
      {
      case '%':
	switch (state)
	  {
	  case char_ptr_fmt_null:
	    state = char_ptr_fmt_percent;
	    break;
	  case char_ptr_fmt_percent:
	    state = char_ptr_fmt_null;
	    break;
	    /* Any other state means we've reached a following '%' format. */
	  default:
	    return false;
	  }
	 break;
      case '.':
	if (state == char_ptr_fmt_percent)
	  state = char_ptr_fmt_precision;
	break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	if (state == char_ptr_fmt_percent)
	  state = char_ptr_fmt_precision;
	break;
      case 'l':
	if ((state == char_ptr_fmt_percent) ||
	    (state == char_ptr_fmt_precision))
	  state = char_ptr_fmt_length_mod;
	break;
      case 's':
      case 'S':
	if (state == char_ptr_fmt_percent) {
	  if (prev_char == '%')
	    return true;
	}
	if ((state == char_ptr_fmt_precision) ||
	    (state == char_ptr_fmt_length_mod)) {
	  return true;
	} else {
	  if (s[i+1] == '\0') return false;
	}
	break;
      case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
      case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
      case 'a': case 'A': case 'c': case 'P': case 'n': case 'm':
	if ((state == char_ptr_fmt_percent) ||
	    (state == char_ptr_fmt_precision) ||
	    (state == char_ptr_fmt_length_mod))
	  return false;
	break;
      }
    if (!isspace ((int)s[i]))
      prev_char = s[i];
  }
  return false;
}

static bool is_int_fmt (char *s) {
   int i;
   enum {
     int_fmt_null,
     int_fmt_percent,
     int_fmt_precision,
     int_fmt_fmt_char
   } state;

   for (i = 0, state = int_fmt_null; s[i]; i++) {

     switch (s[i])
       {
       case '%':
	 switch (state)
	   {
	   case int_fmt_null:
	     state = int_fmt_percent;
	     break;
	   case int_fmt_percent:
	     state = int_fmt_null;
	     break;
	     /* Any other state means we've reached a following '%' format. */
	   default:
	     return false;
	     break;
	   }
	 break;
       case '.':
	 if (state == int_fmt_percent)
	   state = int_fmt_precision;
	 break;
       case '0':
       case '1':
       case '2':
       case '3':
       case '4':
       case '5':
       case '6':
       case '7':
       case '8':
       case '9':
	 if (state == int_fmt_percent)
	   state = int_fmt_precision;
	 break;
       case 'l':
       case 'L':
	 if ((state == int_fmt_percent) ||
	     (state == int_fmt_precision)) {
	   return false;
	 }
	break;
      case 'i':
      case 'd':
      case 'o':
      case 'u':
	if ((state == int_fmt_percent) ||
	    (state == int_fmt_precision))
	  return true;
	break;
      case 'x':
      case 'X':
	/* is_ptr_fmt* handles "%#x" formats. */
	if (s[i-1] == '#')
	  return false;
	else
	  return true;
	break;
      case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
      case 'a': case 'A': case 'c': case 's': case 'P': case 'n':
      case 'm':
 	if ((state == int_fmt_percent) ||
	    (state == int_fmt_precision))
	  return false;
	break;
     }
  }
  return false;
}

static inline int fmt_arg_prev_tok (MESSAGE_STACK messages, int tok) {
  while (tok <= P_MESSAGES) {
    if (!M_ISSPACE(messages[tok]))
      return tok;
    else
      ++tok;
  }
  return ERROR;
}

bool missing_fmt_arg_specifier;

FMT_ARG_CTYPE fmt_arg_type (MESSAGE_STACK messages, int expr_start_idx,
				 int stack_start_idx) {
   int i, n_th_arg;
   int paren_lvl, block_lvl, array_lvl, start_paren_lvl, start_array_lvl;
   MESSAGE *m;
   char *fmt_start;
   enum {
     fmt_arg_null,
     fmt_arg_expr,
     fmt_arg_separator,
     fmt_arg_literal
   } state;

   missing_fmt_arg_specifier = false;
   paren_lvl = block_lvl = array_lvl = n_th_arg = 0;
   start_paren_lvl = start_array_lvl = -1;
   for (i = expr_start_idx, state = fmt_arg_null; i <= stack_start_idx; i++) {

     m = messages[i];
     if (M_ISSPACE(m)) continue;

     switch (M_TOK(m))
       {
       case LABEL:
	 if ((state == fmt_arg_null) || (state == fmt_arg_separator))
	   state = fmt_arg_expr;
	 break; 
       case ARGSEPARATOR:
	 if (start_paren_lvl != -1) {
	   if (!block_lvl && !array_lvl) {
	     state = fmt_arg_separator;
	     --n_th_arg;
	   }
	 } else {
	   if (!paren_lvl && !block_lvl && !array_lvl) {
	     state = fmt_arg_separator;
	     --n_th_arg;
	   } else if (paren_lvl && state == fmt_arg_expr) {
	     state = fmt_arg_separator;
	     --n_th_arg;
	   }
	 }
	 break;
       case LITERAL:
	 if (state == fmt_arg_separator) {
	   if ((fmt_start = strchr (M_NAME(m), '%')) != NULL) {
	     int __n;
	     /*
	      *  n_th_arg is -1 for the first argument, -2 for the 
	      *  second, -3 for the third, ....
	      */
	     if (n_th_arg == -1) {
	       /* check for a "%%" sequence */
	       if (fmt_start[1] == '%') {
		 if ((fmt_start = index (&fmt_start[2], '%')) == NULL)
		   continue;
	       }
	       if (is_int_fmt (fmt_start)) {
		 return fmt_arg_int;
	       } else if (is_char_ptr_fmt (fmt_start)) {
		 return fmt_arg_char_ptr;
	       } else if (is_ptr_fmt (fmt_start)) {
		 return fmt_arg_ptr;
	       } else if (is_char_fmt (fmt_start)) {
		 return fmt_arg_char;
	       } else if (is_double_fmt (fmt_start)) {
		 return fmt_arg_double;
	       } else if (is_long_int_fmt (fmt_start)) {
		 return fmt_arg_long_int;
	       } else if (is_long_long_int_fmt (fmt_start)) {
		 return fmt_arg_long_long_int;
	       } else {
		 return fmt_arg_null;
	       }
	     } else {

	       for (__n = -2; ; __n--) {
		 if ((fmt_start = index (fmt_start + 1, '%')) == NULL) {
		   missing_fmt_arg_specifier = true;
		   return fmt_arg_null;
		 }

		 /* Check for literal "%%" */
		 if (fmt_start[1] == '%')
		   continue;

		 if (__n == n_th_arg)
		   break;
	       }

	       if (is_int_fmt (fmt_start)) {
		 return fmt_arg_int;
	       } else if (is_char_ptr_fmt(fmt_start)) {
		 return fmt_arg_char_ptr;
	       } else if (is_ptr_fmt(fmt_start)) {
		 return fmt_arg_ptr;
	       } else if (is_char_fmt (fmt_start)) {
		 return fmt_arg_char;
	       } else if (is_double_fmt (fmt_start)) {
		 return fmt_arg_double;
	       } else if (is_long_int_fmt (fmt_start)) {
		 return fmt_arg_long_int;
	       } else if (is_long_long_int_fmt (fmt_start)) {
		 return fmt_arg_long_long_int;
	       } else {
		 return fmt_arg_null;
	       }

	     }
	   }
	 } else {
	   state = fmt_arg_expr;
	 }
	 break;
       case OPENPAREN:
	 ++paren_lvl;
	 if (state == fmt_arg_null) {
	   start_paren_lvl = paren_lvl;
	   state = fmt_arg_expr;
	 } else {
	   int __prev_idx;
           if ((__prev_idx = fmt_arg_prev_tok (messages, i)) != ERROR) {
	     /*
	      *  Function call syntax.
	      */
	     if (M_TOK(messages[__prev_idx]) == LABEL)
	       return fmt_arg_null;
	   }
	 }
	 break;
       case CLOSEPAREN:
	 --paren_lvl;
	 if (state == fmt_arg_separator)
	   state = fmt_arg_expr;
	 break;
       case OPENBLOCK:
	 ++block_lvl;
	 break;
       case CLOSEBLOCK:
	 --block_lvl;
	 break;
       case ARRAYOPEN:
	 ++array_lvl;
	 if (state == fmt_arg_null)
	   start_array_lvl = array_lvl;
	 break;
       case ARRAYCLOSE:
	 --array_lvl;
	 break;
       case SEMICOLON:
	 return fmt_arg_null;
	 break;
       default:
	 break;
       } /* switch (M_TOK(m)) */
   }
   return fmt_arg_null;
}

FMT_ARG_CTYPE fmt_arg_type_ms (MSINFO *ms) {
   int i, n_th_arg;
   int paren_lvl, block_lvl, array_lvl, start_paren_lvl, start_array_lvl;
   MESSAGE *m;
   char *fmt_start;
   enum {
     fmt_arg_null,
     fmt_arg_expr,
     fmt_arg_separator,
     fmt_arg_literal
   } state;

   paren_lvl = block_lvl = array_lvl = n_th_arg = 0;
   start_paren_lvl = start_array_lvl = -1;
   for (i = ms -> tok, state = fmt_arg_null;
	i <= ms -> stack_start; i++) {

     m = ms -> messages[i];
     if (M_ISSPACE(m)) continue;

     switch (M_TOK(m))
       {
       case LABEL:
	 if ((state == fmt_arg_null) || (state == fmt_arg_separator))
	   state = fmt_arg_expr;
	 break; 
       case ARGSEPARATOR:
	 if (start_paren_lvl != -1) {
	   if (!block_lvl && !array_lvl) {
	     state = fmt_arg_separator;
	     --n_th_arg;
	   }
	 } else {
	   if (!paren_lvl && !block_lvl && !array_lvl) {
	     state = fmt_arg_separator;
	     --n_th_arg;
	   } else if (paren_lvl && state == fmt_arg_expr) {
	     state = fmt_arg_separator;
	     --n_th_arg;
	   }
	 }
	 break;
       case LITERAL:
	 if (state == fmt_arg_separator) {
	   if ((fmt_start = strchr (M_NAME(m), '%')) != NULL) {
	     int __n;
	     /*
	      *  n_th_arg is -1 for the first argument, -2 for the 
	      *  second, -3 for the third, ....
	      */
	     if (n_th_arg == -1) {
	       /* check for a "%%" sequence */
	       if (fmt_start[1] == '%') {
		 if ((fmt_start = index (&fmt_start[2], '%')) == NULL)
		   continue;
	       }
	       if (is_int_fmt (fmt_start)) {
		 return fmt_arg_int;
	       } else if (is_char_ptr_fmt (fmt_start)) {
		 return fmt_arg_char_ptr;
	       } else if (is_ptr_fmt (fmt_start)) {
		 return fmt_arg_ptr;
	       } else if (is_char_fmt (fmt_start)) {
		 return fmt_arg_char;
	       } else if (is_double_fmt (fmt_start)) {
		 return fmt_arg_double;
	       } else if (is_long_int_fmt (fmt_start)) {
		 return fmt_arg_long_int;
	       } else if (is_long_long_int_fmt (fmt_start)) {
		 return fmt_arg_long_long_int;
	       } else {
		 return fmt_arg_null;
	       }
	     } else {

	       for (__n = -2; ; __n--) {
		 if ((fmt_start = index (fmt_start + 1, '%')) == NULL)
		   return fmt_arg_null;

		 /* Check for literal "%%" */
		 if (fmt_start[1] == '%')
		   continue;

		 if (__n == n_th_arg)
		   break;
	       }

	       if (is_int_fmt (fmt_start)) {
		 return fmt_arg_int;
	       } else if (is_char_ptr_fmt(fmt_start)) {
		 return fmt_arg_char_ptr;
	       } else if (is_ptr_fmt(fmt_start)) {
		 return fmt_arg_ptr;
	       } else if (is_char_fmt (fmt_start)) {
		 return fmt_arg_char;
	       } else if (is_double_fmt (fmt_start)) {
		 return fmt_arg_double;
	       } else if (is_long_int_fmt (fmt_start)) {
		 return fmt_arg_long_int;
	       } else if (is_long_long_int_fmt (fmt_start)) {
		 return fmt_arg_long_long_int;
	       } else {
		 return fmt_arg_null;
	       }

	     }
	   }
	 } else {
	   state = fmt_arg_expr;
	 }
	 break;
       case OPENPAREN:
	 ++paren_lvl;
	 if (state == fmt_arg_null) {
	   start_paren_lvl = paren_lvl;
	   state = fmt_arg_expr;
	 } else {
	   int __prev_idx;
	   if ((__prev_idx = fmt_arg_prev_tok (ms -> messages, i)) != ERROR) {
	     /*
	      *  Function call syntax.
	      */
	     if (M_TOK(ms -> messages[__prev_idx]) == LABEL)
	       return fmt_arg_null;
	   }
	 }
	 break;
       case CLOSEPAREN:
	 --paren_lvl;
	 if (state == fmt_arg_separator)
	   state = fmt_arg_expr;
	 break;
       case OPENBLOCK:
	 ++block_lvl;
	 break;
       case CLOSEBLOCK:
	 --block_lvl;
	 break;
       case ARRAYOPEN:
	 ++array_lvl;
	 if (state == fmt_arg_null)
	   start_array_lvl = array_lvl;
	 break;
       case ARRAYCLOSE:
	 --array_lvl;
	 break;
       case SEMICOLON:
	 return fmt_arg_null;
	 break;
       default:
	 break;
       } /* switch (M_TOK(m)) */
   }
   return fmt_arg_null;
}
