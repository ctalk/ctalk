/* $Id: lex.c,v 1.2 2019/12/01 20:17:27 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2014, 2016, 2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <unistd.h>
#include "ctpp.h"
#include "typeof.h"
#include "prtinfo.h"

/* TO DO -
   Use the international abbreviations in iso646.h.
*/


extern int error_line;           /* Declared in errorloc.c. */
extern int error_column;

extern RT_INFO rtinfo;           /* Declared in rtinfo.c.   */
extern char source_file[FILENAME_MAX];
extern char output_file[FILENAME_MAX];
extern int keepcomments_opt;
extern int warnnestedcomments_opt;
extern int use_trigraphs_opt;
extern int warn_trigraphs_opt;
extern int string_concatenation_opt;
extern INCLUDE *includes[MAXARGS + 1];
extern int include_ptr;
#define INCLUDE_SOURCE_NAME ((include_ptr <= MAXARGS) ? \
			     includes[include_ptr] -> path : source_file)

extern int line_info_line;       /* Declared in lineinfo.c. */

#ifndef HAVE_OFF_T
extern int input_size;             /* Declared in lib/read.c.            */
#else
extern off_t input_size; 
#endif

int in_c_cmt, in_cplus_cmt;         /* Record if in a comment. */
int linesplice;                     /* Record line splices.    */

int rescanning = FALSE;     /* Set by preprocess.c when rescanning
				arguments. 
			     */

#define _MPUTCHAR(__m,__c) (__m)->name[0] = (__c); (__m)->name[1] = '\0';
#define _MPUT2CHAR(__m,__s) (__m)->name[0] = (__s[0]); \
				 (__m)->name[1] = (__s[1]); \
				 (__m)->name[2] = '\0';
#define _MPUT3CHAR(__m,__s) (__m)->name[0] = (__s[0]); \
				 (__m)->name[1] = (__s[1]); \
				 (__m)->name[2] = (__s[2]); \
				 (__m)->name[3] = '\0';

static int gnuc_attribute;  /* Set by __attribute__ label, cleared after )). */

static int preprocess_line = FALSE;  /* Used to determine statement endings -
                                        Set when we encounter a PREPROCESS
					token and cleared at the NEWLINE 
                                        token.  If set, statements end
                                        at the NEWLINE.  Otherwise, statements
                                        end at the SEMICOLON.
				     */
int comment = 0;                      /* Level of comment nesting. */

int str_term_err_loc;

static inline int __is_sign (char c, const char *buf, int idx,
			     bool *have_sign) {

  register int i;

  if (c == '-' || c == '+') {
    if (isdigit((int)buf[idx])) {
      // c is always at buf[idx-1].
      if ((i = (idx - 2)) >= 0) {
	while (isspace ((int)buf[i])) {
	  if (--i < 0) {
	    // leading spaces at beginning of buffer.
	    *have_sign = true;
	    return TRUE;
	  }
	}

	if (IS_C_OP_CHAR(buf[i])) {
	  *have_sign = true;
	  return TRUE;
	} else {
	  return FALSE;
	}

      } else {
	// c is at the beginning of the buffer.
	*have_sign = true;
	return TRUE;
      }
    }
  }

  return FALSE;
}

static bool is_decimal_exp (char *buf, int exp_ptr, bool *have_exp) {
  if (buf[exp_ptr] == 'e' || buf[exp_ptr] == 'E') {
    if (buf[exp_ptr+1] == '+' || buf[exp_ptr+1] == '-') {
      *have_exp = true;
    }
  }
  return false;
}

static bool is_hex_exp (char *buf, int exp_ptr, bool *have_exp) {
  if (buf[exp_ptr] == 'p' || buf[exp_ptr] == 'P') {
    if (buf[exp_ptr+1] == '+' || buf[exp_ptr+1] == '-') {
      *have_exp = true;
    }
  }
  return false;
}

bool escaped_quote (char *buf, char *quote_ptr) {
  if (quote_ptr - buf > 1) {
    if (*(quote_ptr - 1) == '\\' && *(quote_ptr - 2) != '\\') {
      return true;   /* the backslash is for the quote. */
    } else {
      return false;
    }
  } else if (quote_ptr - buf == 1) {
    if (*(quote_ptr - 1) == '\\') {
      return true;
    }
  }
  return false;
}

static bool is_first_binary_term (char *s) {
  /* Look for a noeval op followed by another term. The argument,
     's', should point to the space just after the first term. */
  int i;
  bool have_op = false;
  for (i = 0; s[i]; ++i) {
    if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\f') {
      continue;
    }
    if (IS_C_OP_CHAR(s[i])) {
      if (have_op == false) {
	/* Make sure a '%' isn't the start of a printf format. */
	if (s[i] == '%') {
	  if (IS_STDARG_FMT_CHAR(s[i+1])) {
	    return false;
	  }
	} else {
	  have_op = true;
	}
      }
      continue;
    }
    if (have_op) {
      if (IS_C_LABEL_CHAR(s[i]) || (s[i] == '"') || (s[i] == '\'') ||
	  (s[i] >= '0'  && s[i] <= '9')) {
	return true;
      } else {
	return false;
      }
    }
  }
  return false;
}

bool next_is_separator (char *s) {
  int i;
  for (i = 0; s[i]; ++i) {
    if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\f') {
      continue;
    }
    if (IS_SEPARATOR(s[i])) {
      /* Some single sepatators between quotes don't occur otherwise
	 than an opening quote, and we might need to defer that
	 until we can count arguments, so return false anyway. */
      ++i;
      while (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\f')
	i++;
      if (s[i] == '"') {
	return false;
      } else {
	return true;
      }
    } else {
      return false;
    }
  }
  return false;
}

static char *next_unescaped_quote (char *buf) {
  char *p, *q, *q2;
  p = buf;
 next_quote: if ((q = strchr (p, '"')) == NULL) {
    return NULL;
  } else {
    if (escaped_quote (buf, q)) {
      p = q + 1;
      goto next_quote;
    } else {
      if (!next_is_separator(q+1) && !is_first_binary_term (q+1)) {
	/* if we have a following alpha char -- check
	   the next following quote, to see if it has a separator
	   following it. That should mean that q is actually
	   the next opening quote. */
	if ((q2 = next_unescaped_quote (q + 1)) != NULL) {
	  if (next_is_separator (q2+1)) {
	    return NULL;
	  } else {
	    return q;
	  }
	} else {
	  return q;  /* if it's the last quote, we can't determine
			it here. */
	}
      } else {
	return q;
      }
    }
  }
}

/* Start directly after the supposed delimiter.  Check for the second
   unescaped delimiter and a separator character after that. */
static int char_is_delimiter (const char *buf, int open_delim_idx) {
  int i, n_parens = 0, lookback;
  char delim = buf[open_delim_idx];
  if (!ispunct((int)delim))
    return ERROR;
  /* we're going to be draconian and say that a pattern can ONLY
     appear after a =~ or !~ operator, or a =, in case we want
     to assign a pattern to a variable. */
  for (lookback = open_delim_idx - 1; lookback >= 0; --lookback) {
    if (isspace ((int)buf[lookback]))
      continue;
    if (buf[lookback] == '~') {
      if (lookback == 0) {
	return ERROR;
      } else if (buf[lookback - 1] == '!' || buf[lookback -1] == '=') {
	break;
      } else {
	return ERROR;
      }
    } else if (buf[lookback] == 'm') {  /* skip a m{ } for any delim char. */
      continue;
    } else if (buf[lookback] == '=') {  /* we also want to assign them */
      break;
    } else {
      return ERROR;
    }
  }

  /* if we encounter a parenthesis, go to the matching paren
     unless we find an unescaped delimiter. */
  for (i = open_delim_idx + 1; buf[i]; ++i) {
    if (buf[i] == '(') {
      ++n_parens;
      continue;
    } else if (buf[i] == ')') {
      --n_parens;
      continue;
    } else if (n_parens) {
      if (buf[i] == delim && buf[i-1] != '\\') {
	return ERROR;
      } else {
	continue;
      }
    }
      
    if ((buf[i] == delim) && (buf[i-1] != '\\')) {
      /* this helps us avoid an error when two delimiters end the
	 pattern. */
      if (buf[i+1] == 0) {
	return i;
      } else if (IS_SEPARATOR(buf[i+1]) && (buf[i+1] != delim)) {
	return i;
      }
    }
  }
  return ERROR;
}

static bool have_quote_scan;

#ifdef DJGPP
#define LINE_SPLICE_STR "\\\r\n"
#define LINE_SPLICE_STR_LENGTH 3
#else
#define LINE_SPLICE_STR "\\\n"
#define LINE_SPLICE_STR_LENGTH 2
#endif

int lexical (char *buf, long long *idx, MESSAGE *m) {

  int i, j, k;
  int numeric_type, is_unsigned, unicode_constant;
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
  int need_signed_hexadecimal_constant_warning = FALSE,
    need_signed_octal_constant_warning = FALSE;
#endif
  char tmpbuf[MAXMSG], *q, c, c_next;
  bool have_sign = false;
  bool have_exp = false;


  if (*idx == 0)
    have_quote_scan = false;

  c = buf[(*idx)++];

  /* WHITESPACE */
  /* Newline characters have their own token type, below. */
  if (c == ' ' || c == '\t' || c == '\f' || c == '\v') {
    _MPUTCHAR(m,c)
    for (i = 1;
	 ((buf[*idx] == ' ') || (buf[*idx] == '\t') || 
	  (buf[*idx] == '\f'));
	 i++, (*idx)++)
      m -> name[i] = buf[*idx];
    m -> name[i] = 0;
    m -> tokentype = WHITESPACE;
    return WHITESPACE;
  }

  /* CR - LF  to NEWLINE.
     A newline token can contain a run of newlines, *but it 
     should contain no other characters* between the newlines, 
     otherwise, line numbers will not be calculated correctly 
     in tokenize (), below. 

     The clause for '\r' is below, which translates CR's into
     system-specific generic '\n' characters.
  */ 
  if (c == '\n') {
    int buflength = 0;
    int resize_x = MAXLABEL;

    if (*idx >= 2) {
      if (buf[*idx - 2] == '\\') {
	/* strcpy (m -> name, " "); */
	_MPUTCHAR(m,' ')
	/* Increment the error line and reset error column also. */
	++error_line; error_column = 0;
	m -> tokentype = WHITESPACE;
	return WHITESPACE;
      }
    }

    /* strcpy (m -> name, "\n"); */
    _MPUTCHAR(m,'\n')

    while (((buf[*idx] == '\r') || (buf[*idx] == '\n')) &&
	   (buflength < (MAXLABEL / 2))) {
	if (++buflength >= resize_x) {
	  resize_x += resize_x;
	  resize_message (m, resize_x);
	}
	m -> name[buflength] = '\n';

      (*idx)++;
    }
    m -> name[buflength+1] = '\0';
    error_column = 0;
    if (preprocess_line)
      preprocess_line = FALSE;
    if (in_cplus_cmt)
      in_cplus_cmt = 0;
    m -> tokentype = NEWLINE;
    return NEWLINE;
  }

  if (c == '\r') {
    int buflength = 0;
    int resize_x = MAXLABEL;

    if (*idx >= 2) {
      if (buf[*idx - 2] == '\\') {
	_MPUTCHAR(m, ' ')
	/* Increment the error line and reset error column also. */
	++error_line; error_column = 0;
	m -> tokentype = WHITESPACE;
	return WHITESPACE;
      }
    }

    /* strcpy (m -> name, "\n"); */
    _MPUTCHAR(m,'\n')

    while (((buf[*idx] == '\r') || (buf[*idx] == '\n')) &&
	   (buflength < (MAXLABEL / 2))) {
      if (++buflength >= resize_x) {
	resize_x += resize_x;
	resize_message (m, resize_x);
      }
      m -> name[buflength] = '\n';
      /* if (buf[*idx] == c) { */
      /* 	strcat (m -> name, "\n"); */
	/* Don't increment the error line here - 
	   set_error_location does that for all tokens that 
	   contain newlines. */
      /* } */
      (*idx)++;;
    }
    m -> name[buflength+1] = '\0';
    error_column = 0;
    if (preprocess_line)
      preprocess_line = FALSE;
    if (in_cplus_cmt)
      in_cplus_cmt = 0;
    m -> tokentype = NEWLINE;
    return NEWLINE;
  }

  /* FLOAT, INTEGER, LONG, and LONGLONG                                  */
  if (isdigit ((int)c) || 
      (c == '.' && isdigit((int)buf[*idx])) ||
      __is_sign (c, buf, *idx, &have_sign)) {
    RADIX radix = decimal;

    unicode_constant = FALSE;
    numeric_type = INTEGER_T;
    is_unsigned = FALSE;

    /* Don't handle unicode constants (yet). */
    if (((*idx) > 3) && (buf[(*idx)-2] == '+') && 
	(buf[(*idx)-3] == 'U')) {
      unicode_constant = TRUE;
    }

    (*idx)--;

    j = *idx;
    k = 0;

    if (have_sign)
      m -> name[k++] = buf[j++];

    c = buf[j];
    c_next = buf[j+1];
    if (c == '0' && c_next != '.') {
      if (c_next == 'x' || c_next == 'X' || unicode_constant) {
	radix = hexadecimal;
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
	if (have_sign)
	  need_signed_hexadecimal_constant_warning = TRUE;
#endif
	m -> name[k++] = buf[j++];
	m -> name[k++] = buf[j++];
      } else if (c_next == 'b' || c_next == 'B') {
	radix = binary;
	m -> name[k++] = buf[j++];
	m -> name[k++] = buf[j++];
      } else if (c_next >= '0' && c_next <= '7') {
	radix = octal;
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
	if (have_sign)
	  need_signed_octal_constant_warning = TRUE;
#endif
	m -> name[k++] = buf[j++];
      }
    }

    while (buf[j]) {

      if (buf[j] == (char)'.' && isdigit ((int)buf[j+1]))
 	numeric_type = DOUBLE_T;

      if (numeric_type == INTEGER_T) {
	if (buf[j] == 'L') {
	  numeric_type = LONGLONG_T;
	} else if (buf[j] == 'l') {
	  if (buf[j+1] == 'l') {
	    numeric_type = LONGLONG_T;
	  } else {
	    numeric_type = LONG_T;
	  }
	}
      }

      if (numeric_type == DOUBLE_T) {
	if (buf[j] == 'f' || buf[j] == 'F') {
	  numeric_type = FLOAT_T;
	} else if (buf[j] == 'l' || buf[j] == 'L') {
	  numeric_type = LONGDOUBLE_T;
	}
      }

      /* 
       * Integers can also have 'u' and 'U' suffixes.
       */
      if (isdigit ((int)buf[j]) && 
	  (buf[j+1] == (char)'u' || buf[j+1] == (char)'U'))
	is_unsigned = TRUE;


      if (radix == decimal) {

	if (numeric_type == INTEGER_T) {
	  if (! (isdigit ((int)buf[j]) ||
		 (strspn (&buf[j], "uUbB") == 1))) {
	    break;
	  } else {
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == LONG_T || numeric_type == LONGLONG_T) {
	  if (! (isdigit ((int)buf[j]) || 
		 (strspn (&buf[j], "lLuUbB") >= 1))) {
	    break;
	  } else {
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == DOUBLE_T) {
	  if (!isdigit ((int)buf[j]) &&
	      (buf[j] != '.') &&
	      !is_decimal_exp (buf, j, &have_exp)) {
	    break;
	  } else {
	    if (have_exp) {
	      m -> name[k++] = buf[j++];
	    }
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == FLOAT_T) {
	  if (!isdigit ((int)buf[j]) &&
	     (buf[j] != '.') &&
	     (buf[j] != 'F') &&
	     (buf[j] != 'f') &&
	      !is_decimal_exp (buf, j, &have_exp)) {
	    break;
	  } else {
	    if (have_exp) {
	      m -> name[k++] = buf[j++];
	    }
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == LONGDOUBLE_T) {
	  if (!isdigit ((int)buf[j]) &&
	      (buf[j] != '.') &&
	      !(strspn (&buf[j], "lL") >= 1) &&
	      !is_decimal_exp (buf, j, &have_exp)) {
	    break;
	  } else {
	    if (have_exp) {
	      m -> name[k++] = buf[j++];
	    }
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	}

      } else if (radix == octal) {
      /* Octals need range checking. */
	if (isdigit ((int)buf[j])) {
	  if (buf[j] > '7') {
	    if (!comment)
	      _error ("Bad octal constant.\n");
	  } else {
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else {
	  break;
	}
      } else if (radix == binary) {
	if (buf[j] != '1' && buf[j] != '0') {
	  break;
	} else {
	  m -> name[k++] = buf[j++];
	  continue;
	}
      } else if (radix == hexadecimal) {
	if ((buf[j] == (char)'.') || (buf[j] == (char)'p') || 
	    (buf[j] == (char)'P') || (buf[j] == (char)'+') || 
	    (buf[j] == (char)'-')) 
	  numeric_type = DOUBLE_T;

	if (numeric_type == DOUBLE_T) {
	  if (buf[j] == 'f' || buf[j] == 'F') {
	    numeric_type = FLOAT_T;
	  } else if (buf[j] == 'l' || buf[j] == 'L') {
	    numeric_type = LONGDOUBLE_T;
	  }
	}

	if (numeric_type == INTEGER_T) {
	  if (!isxdigit ((int)buf[j])) {
	    break;
	  } else {
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == LONG_T) {
	  if (!isxdigit ((int)buf[j]) &&
	      (buf[j] != 'l')) {
	    break;
	  } else {
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == LONGLONG_T) {
	  if (!isxdigit ((int)buf[j]) &&
	      !(strspn (&buf[j], "lL"))) {
	    break;
	  } else {
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == DOUBLE_T) {
 	  if (!isxdigit ((int) buf[j]) && 
	      (buf[j] != '.') &&
	      !is_hex_exp (buf, j, &have_exp)) {
	    break;
	  } else {
	    if (have_exp) {
	      m -> name[k++] = buf[j++];
	    }
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == FLOAT_T) {
 	  if (!isxdigit ((int) buf[j]) && 
	      !(strspn (&buf[j], ".fF") == 1) &&
	      !is_hex_exp (buf, j, &have_exp)) {
	    break;
	  } else {
	    if (have_exp) {
	      m -> name[k++] = buf[j++];
	    }
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	} else if (numeric_type == LONGDOUBLE_T) {
 	  if (!isxdigit ((int) buf[j]) && 
	      !(strspn (&buf[j], ".lL") >= 1) &&
	      !is_hex_exp (buf, j, &have_exp)) {
	    break;
	  } else {
	    if (have_exp) {
	      m -> name[k++] = buf[j++];
	    }
	    m -> name[k++] = buf[j++];
	    continue;
	  }
	}
      }

    }
    m -> name[k] = '\0';
    *idx = j;
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
    if (!comment) {
      if (need_signed_hexadecimal_constant_warning) {
	_warning ("%s:%d: Signed hexadecimal constant %s.\n", 
		  INCLUDE_SOURCE_NAME, error_line, m -> name);
      }
      if (need_signed_octal_constant_warning) {
	_warning ("%s:%d: Signed octal constant %s.\n", 
		  INCLUDE_SOURCE_NAME, error_line, m -> name);
      }
    }
#endif
    switch (numeric_type)
      {
      case INTEGER_T:
	m -> tokentype = (is_unsigned) ? UINTEGER : INTEGER;
	return m -> tokentype;
	break;
      case DOUBLE_T:
      case FLOAT_T:
      case LONGDOUBLE_T:
 	m -> tokentype = DOUBLE;
 	return DOUBLE;
	break;
      case LONG_T:
	m -> tokentype = (is_unsigned) ? ULONG : LONG;
	return m -> tokentype;
	break;
      case LONGLONG_T:
	m -> tokentype = LONGLONG;
	return LONGLONG;
	break;
      }
  }

  /* LABEL */
  if (isalnum ((int)c) || c == '_' || c == '$') {
    i = 0;
    m -> name[i++] = c;

    while (buf[*idx]) {
      c = buf[*idx];
      if (isalnum ((int)c) || c == '_' || c == '$') {
	m -> name[i++] = c;
      } else {
	break;
      }
      if (i == MAXLABEL)
	break;
      (*idx)++;
    }
    m -> name[i] = '\0';
    m -> tokentype = LABEL;
    return LABEL;
  }

  /* OPENPAREN */
  if (c == '(') {

    _MPUTCHAR(m,c)
    m -> tokentype = OPENPAREN;
    return OPENPAREN; 
  } 

  /* CLOSEPAREN */
  if (c == ')') {
    _MPUTCHAR(m,c)
    m -> tokentype = CLOSEPAREN;
    return CLOSEPAREN; 
  } 

  /* OPENBLOCK */
  if (c == '{') {
    _MPUTCHAR(m,c)
    m -> tokentype = OPENBLOCK;
    return OPENBLOCK; 
  } 

  /* CLOSEBLOCK */
  if (c == '}') {
    _MPUTCHAR(m,c)
    m -> tokentype = CLOSEBLOCK;
    return CLOSEBLOCK; 
  } 

  /* OPENCCOMENT */
  if (c == '/' && buf[*idx] == '*') {
    int p_warn = 0;
    int name_i = 0;
    if (++in_c_cmt > 1) {
      if (!p_warn) {
	p_warn = 1;
	printf ("%s\n", &buf[*idx]);
      }
      _warning ("%s:%d: Comment begins inside another comment.\n",
		INCLUDE_SOURCE_NAME, error_line);
    }

    while (buf[*idx]) {
      if (buf[*idx] == '\n') {
	m -> name[name_i++] = '\n';
      } else if (buf[*idx] == '/' && buf[(*idx) + 1] == '*') {
	++in_c_cmt;
      } else if (buf[*idx] == '*' && buf[(*idx) + 1] == '/') {
	if (--in_c_cmt == 0) {
	  (*idx) += 2;
	  if (name_i == 0) {
	    m -> name[name_i++] = ' ';
	  }
	  m -> name[name_i] = 0;
	  m -> tokentype = WHITESPACE;
	  return WHITESPACE;
	}
      }
      (*idx)++;
    }

    _warning ("%s:%d: Couldn't find comment close.\n",
		INCLUDE_SOURCE_NAME, error_line);

  }

#if 0
  /* CLOSECCOMENT */
  /* don't delete in case it's needed later. */
  if (c == '*' && buf[*idx] == '/') {
    if (--in_c_cmt < 0) {
      _warning ("%s:%d: Unmatched comment close.\n",
		INCLUDE_SOURCE_NAME, error_line);
      in_c_cmt = 0;
    }
    m -> name[0] = c;
    m -> name[1] = buf[(*idx)++];
    m -> name[2] = 0;
    m -> tokentype = CLOSECCOMMENT;
    return CLOSECCOMMENT;
  }
#endif  

  /* CPPCOMMENT */
  if (c == '/' && buf[*idx] == '/') {
    if (!in_c_cmt) {
      ++in_cplus_cmt;
      q = index (&buf[*idx], '\n');
      strncpy (tmpbuf, &buf[(*idx) - 1], q - &buf[(*idx) - 1]);
      tmpbuf[q - &buf[(*idx) - 1]] = 0;
      strcpy (m -> name, tmpbuf);
      *idx += (q - &buf[*idx]);
      m -> tokentype = CPPCOMMENT;
      return CPPCOMMENT;
    }
  }

  /* SEMICOLON */
  if (c == ';') {
    _MPUTCHAR(m,c)
    m -> tokentype = SEMICOLON;
    return SEMICOLON;
  }

  /* ASTERISK, MULT_ASSIGN */
  if (c == '*') {
    switch (buf[*idx]) 
      {
      case '=':
	(*idx)++;
	_MPUT2CHAR(m, "*=");
	m -> tokentype = MULT_ASSIGN;
	return MULT_ASSIGN;
	break;  /* Not reached. */
      default:
	_MPUTCHAR(m,c)
        m -> tokentype = ASTERISK;
	return ASTERISK;
        break;  /* Not reached. */
      }
  }

  /* DIVIDE, DIV_ASSIGN, and /.../ delimited PATTERN */
  if (c == '/') {
    int end_idx, j;
    if (!comment && !preprocess_line) {
      if ((end_idx = char_is_delimiter (buf, (*idx) - 1)) != ERROR) {
	--(*idx);
	j = 0;
	if ((end_idx - *idx) > MAXLABEL) {
	  resize_message (m, (end_idx - *idx) + 1);
	}
	while (1) {
	  m -> name[j++] = buf[(*idx)++];
	  if (*idx > end_idx) {
	    m -> name[j] = 0;
	    m -> tokentype = PATTERN;
	    return PATTERN;
	  }
	}
      }
    }    

    switch (buf[*idx]) 
      {
      case '=':
	_MPUT2CHAR(m,"/=")
	++(*idx);
	m -> tokentype = DIV_ASSIGN;
	return DIV_ASSIGN;
	break; /* Not reached. */
      default:
    _MPUTCHAR(m,c)
	m -> tokentype = DIVIDE;
	return DIVIDE;
	break;  /* Not reached. */
      }

  }

  /* LT, ASL, ASL_ASSIGN */
  if (c == '<') {
    switch (buf[*idx])
      {
      case '<':
	(*idx)++;
	switch (buf[*idx])
	  {
	  case '=':
	    (*idx)++;
            _MPUT3CHAR(m,"<<=")
	    m -> tokentype = ASL_ASSIGN;
	    return ASL_ASSIGN;
	    break;  /* Not reached. */
	  default:
            _MPUT2CHAR(m,"<<")
	    m -> tokentype = ASL;
	    return ASL;
	    break;  /* Not reached. */
	  }
	break;
      case '=':
	(*idx)++;
        _MPUT2CHAR(m,"<=")
	m -> tokentype = LE;
	return LE;
	break;  /* Not reached. */
      default:
      _MPUTCHAR(m,c)
	m -> tokentype = LT;
	return LT; 
	break; /* Not reached. */
      }
  }

  /* GT, ASR, ASR_ASSIGN, GE */
  if (c == '>') {
    switch (buf[*idx])
      {
      case '>':
	(*idx)++;
	switch (buf[*idx])
	  {
	  case '=':
	    (*idx)++;
            _MPUT3CHAR(m, ">>=")
	    m -> tokentype = ASR_ASSIGN;
	    return ASR_ASSIGN;
	    break;  /* Not reached. */
	  default:
            _MPUT2CHAR(m, ">>")
	    m -> tokentype = ASR;
	    return ASR;
	    break;  /* Not reached. */
	  }
	break;
      case '=':
	(*idx)++;
        _MPUT2CHAR(m, ">=")
	m -> tokentype = GE;
	return GE;
	break; /* Not reached. */
      default:
    _MPUTCHAR(m,c)
    m -> tokentype = GT;
	return GT;
	break;  /* Not reached. */
      }
  }

  /* BIT_COMP */
  if (c == '~') {
    _MPUTCHAR(m,c)
    m -> tokentype = BIT_COMP;
    return BIT_COMP;
  }

  /* BIT_XOR, BIT_XOR_ASSIGN */
  if (c == '^') {
    switch (buf[*idx])
      {
      case '=':
	(*idx)++;
        _MPUT2CHAR(m,"^=")
	m -> tokentype = BIT_XOR_ASSIGN;
	return BIT_XOR_ASSIGN;
	break; /* Not reached. */
      default:
    _MPUTCHAR(m,c)
	m -> tokentype = BIT_XOR;
	return BIT_XOR;
	break;  /* Not reached. */
      }
  }

  /* PERIOD, ELLIPSIS */
  if (c == '.') {
    if (buf[*idx] == '.' && buf[*idx+1] == '.') {
      (*idx) += 2;
      _MPUT3CHAR(m,"...")
      m -> tokentype = ELLIPSIS;
      return ELLIPSIS;
    } else {
    _MPUTCHAR(m,c)
      m -> tokentype = PERIOD;
      return PERIOD;
    }
  }

  if (c == '?') {
    if (use_trigraphs_opt && buf[*idx] == '?' &&
	unescape_trigraph (&buf[*idx-1])) {
      /*
      *  buf[*idx] is the second '?' in the trigraph.
      */
      SNPRINTF (m -> name, MAXLABEL, "%c", unescape_trigraph (&buf[(*idx)-1]));
      m -> tokentype=trigraph_tokentype (unescape_trigraph (&buf[*idx-1]));
      if (warn_trigraphs_opt) {
      char s[5];
      substrcpy (s, &buf[*idx-1], 0, 3);
      _warning ("%s:%d: Warning: Trigraph sequence %s.\n", 
		INCLUDE_SOURCE_NAME, error_line, s);
      }
      *idx += 2;
      return m -> tokentype;
    }
    _MPUTCHAR(m,c)
    m -> tokentype = CONDITIONAL;
    return CONDITIONAL;
  }

  if (c == ':') {
    _MPUTCHAR(m,c)
    m -> tokentype = COLON;
    return COLON;
  }

  /* BIT_OR, BOOLEAN_OR, BIT_OR_ASSIGN */
  if (c == '|') {
    switch (buf[*idx])
      {
      case '|':
	(*idx)++;
        _MPUT2CHAR(m,"||");
	m -> tokentype = BOOLEAN_OR;
	return BOOLEAN_OR;
	break;
      case '=':
	(*idx)++;
        _MPUT2CHAR(m,"|=");
	m -> tokentype = BIT_OR_ASSIGN;
	return BIT_OR_ASSIGN;
	break;
      default:
    _MPUTCHAR(m,c)
	m -> tokentype = BAR;
	return BAR;
	break;
      }
  }

  /* AMPERSAND, BIT_AND_ASSIGN, BOOLEAN_AND */
  if (c == '&') {
    long long int c_idx;
    switch (buf[*idx])
      {
      case '&':
	(*idx)++;
        _MPUT2CHAR(m,"&&");
	m -> tokentype = BOOLEAN_AND;
	return BOOLEAN_AND;
	break;
      case '=':
	(*idx)++;
        _MPUT2CHAR(m,"&=");
	m -> tokentype = BIT_AND_ASSIGN;
	return BIT_AND_ASSIGN;
	break;
      default:
    _MPUTCHAR(m,c)
	/*
	 *  If the preceding non-whitespace character is an operator,
	 *  then the ampersand is an address-of operator, otherwise
	 *  it is a bitwise and operator.
	 */
	for (c_idx = (*idx) - 2; c_idx >= 0; c_idx--) {
	  if (isspace ((int)buf[c_idx])) 
	    continue;
	  if (IS_C_OP_CHAR (buf[c_idx])) {
	    m -> tokentype = AMPERSAND;
	    return AMPERSAND;
	  } else {
	    m -> tokentype = BIT_AND;
	    return BIT_AND;
	  }
	}
	break;
      }
  }

  /* EQ, BOOLEAN_EQ, MATCH */
  if (c == '=') {
    switch (buf[*idx])
      {
      case '=':
	(*idx)++;
        _MPUT2CHAR(m,"==");
	m -> tokentype = BOOLEAN_EQ;
	return BOOLEAN_EQ;
	break;
      case '~':
	(*idx)++;
        _MPUT2CHAR(m,"=~");
	m -> tokentype = MATCH;
	return MATCH;
	break;
      default:
    _MPUTCHAR(m,c)
	m -> tokentype = EQ;
	return EQ;
	break;
      }
  }

  /* BIT_COMP */
  if (c == '~') {
    _MPUTCHAR(m,c)
    m -> tokentype = BIT_COMP;
    return BIT_COMP;
  }

  /* EXCLAM, INEQUALITY, NOMATCH */
  if (c == '!') {
    switch (buf[*idx]) 
      {
      case '=':
	(*idx)++;
        _MPUT2CHAR(m,"!=");
	m -> tokentype = INEQUALITY;
	return INEQUALITY;
	break;
      case '~':
	(*idx)++;
        _MPUT2CHAR(m,"!~");
	m -> tokentype = NOMATCH;
	return NOMATCH;
	break;
      default:
    _MPUTCHAR(m,c)
	m -> tokentype = EXCLAM;
	return EXCLAM;
	break;
      }
  }

  /* PLUS, PLUS_ASSIGN, INCREMENT */
  /* Note that this occurs after the numeric constant tokens. */
  if (c == '+') {
    switch (buf[*idx])
      {
      case '=':
	(*idx)++;
        _MPUT2CHAR(m,"+=");
	m -> tokentype = PLUS_ASSIGN;
	return PLUS_ASSIGN;
	break;
      case '+':
	(*idx)++;
        _MPUT2CHAR(m,"++");
	m -> tokentype = INCREMENT;
	return INCREMENT;
	break;
      default:
    _MPUTCHAR(m,c)
	m -> tokentype = PLUS;
	return PLUS;
	break;
      }
  }

  /* MINUS, MINUS_ASSIGN, DECREMENT, DEREF */
  /* Note that this occurs after the checking for numeric constants. */
  if (c == '-') {
    switch (buf[*idx])
      {
      case '=':
	(*idx)++;
	/* strcpy (m -> name, "-="); */
        _MPUT2CHAR(m,"-=");
	m -> tokentype = MINUS_ASSIGN;
	return MINUS_ASSIGN;
	break;
      case '-':
	(*idx)++;
        _MPUT2CHAR(m,"--");
	m -> tokentype = DECREMENT;
	return DECREMENT;
	break;
      case '>':
	(*idx)++;
        _MPUT2CHAR(m,"->");
	m -> tokentype = DEREF;
	return DEREF;
	break;
      default:
    _MPUTCHAR(m,c)
	m -> tokentype = MINUS;
	return MINUS;
	break;
      }
  }

  /* PREPROCESS, LITERALIZE, MACRO_CONCAT */
  if (c == '#') {
    /*
     *  We need to ensure that the token is the first *non-whitespace*
     *  token after a newline, or is the first non-whitespace 
     *  token in the text.
     *
     *  The calling functions in preprocess.c should set,
     *  "rescanning," to True when rescanning individual arguments
     *  that have '#' at or near the beginning of the text, so the
     *  code below knows that the '#' characters do not represent the
     *  beginning of preprocessor directives.
     */
    int lookback = ((*idx - 2) > 0) ? *idx - 2 : 0;

    while (((buf[lookback] == ' ') || 
	   (buf[lookback] == '\t')) &&
	   (lookback > 0)) {
      lookback--;
    }

    if ((buf[lookback] == '\n' || lookback == 0) && ! rescanning) {

      /* 
       *  If this is a line marker, parse it and set the 
       *  error location.
       */

      if (isdigit ((int)buf[*idx + 1]) && !line_info_line) {
	char *n, linebuf[MAXMSG];
	if ((n = index (&buf[*idx + 1], '\n')) != NULL) {
	  substrcpy (linebuf, &buf[*idx-1], 0, n - &buf[*idx - 1]);
	} else {
	  strcpy (linebuf, &buf[*idx - 1]);
	}
	line_info_tok (linebuf);
      }
    _MPUTCHAR(m,c)
      m -> tokentype = PREPROCESS;
      preprocess_line = TRUE;
      return PREPROCESS;
    } else {
      if (buf[*idx] == '#') {
	(*idx)++;
        _MPUT2CHAR(m,"##");
	m -> tokentype = MACRO_CONCAT;
	return MACRO_CONCAT;
      } else {
    _MPUTCHAR(m,c)
	m -> tokentype = LITERALIZE;
	return LITERALIZE;
      }
    }
  }

  /* SIZEOF */
  if (c == 's' && !strncmp (&buf[*idx], "izeof", 5)) {
    (*idx) += 5;
    strcpy (m -> name, "sizeof");
    m -> tokentype = SIZEOF;
    return SIZEOF;
  }

  /* A m<pattern> */
  if (c == 'm') {
    int j, end_idx;
    if (!(in_c_cmt || in_cplus_cmt)) {
      if ((end_idx = char_is_delimiter (buf, *idx)) != ERROR) {
	j = 0;
	(*idx)--; /* we need to keep the 'm', too. */
	if ((end_idx - *idx) > MAXLABEL) {
	  resize_message (m, (end_idx - *idx) + 1);
	}
	while (1) {
	  m -> name[j++] = buf[(*idx)++];
	  if (*idx > end_idx) {
	    m -> name[j] = 0;
	    m -> tokentype = PATTERN;
	    return PATTERN;
	  }
	}
      }
    }
  }

  /* LITERAL */
  /* Check for quote characters ('"') and escaped quotes within
     the literal. */
  if (c == '"') {
    int j, k, esc_chr;
    bool in_test_str;
    int n_quotes;
    int input_length;
    int scan_c_cmt, scan_cplus_cmt;
    int end_this_str_idx;
    int tok_cat_idx, tok_cat_idx_2;
    int backoff = 1;   /* Avoid warnings. */
    int str_length_org = 0;
    char *str_end;
    char *splice_ptr;
    int str_length;

    if (!in_c_cmt && !in_cplus_cmt) {

      input_length = strlen (&buf[*idx]);
      if ((str_end = next_unescaped_quote (&buf[*idx])) == NULL) {
	char errbuf[MAXLABEL], *nl, *sl;
	int p_len, b_len;
	memset (errbuf, 0, MAXLABEL);
	if ((nl = strchr (&buf[*idx], '\n')) == NULL) {
	  p_len = strlen (&buf[*idx]);
	} else {
	  p_len = nl - &buf[*idx];
	}
	b_len = 80 - p_len;
	sl = &buf[(*idx) - b_len];
	if (sl < buf)
	  sl = buf;
	strncpy (errbuf, sl, b_len + p_len);
	printf ("%s:%d: Error: unmatched quote:\n\n\t%s\n\n",
		source_file, error_line, errbuf);
	if (file_exists (output_file)) {
	  unlink (output_file);
	}
	exit (EXIT_FAILURE);
      }
      str_length = str_end - (&buf[*idx] - 1);
      /*
       *  Easiest way to ensure that message can hold an
       *  entire string.  input_length + 2 is the maximum
       *  remaining input + opening quote + 1.
       */
      if (str_length >= MAXLABEL)
	resize_message (m, str_length + 2);

      /*
       *  Esc_chr needs a default value that can't be mistaken
       *  for a <string index> - 1, so use -2.
       */

      for (j = *idx - 1, in_test_str = false, k = 0, 
	     esc_chr = -2,n_quotes = 0, end_this_str_idx = -1,
	     scan_c_cmt = FALSE, scan_cplus_cmt = FALSE;
	   (j - *idx <= input_length) && buf[j]; 
	   j++) {
	switch (buf[j])
	  {
	  case '\"':
	    if (esc_chr == (j - 1)) {
	      esc_chr = -2;
	      if (end_this_str_idx == -1)
		m -> name[k++] = buf[j];
	    } else {
	      if (!scan_c_cmt && !scan_cplus_cmt)
		++n_quotes;
	      if (in_test_str) {
		in_test_str = false;
		if (end_this_str_idx == -1) {
		  m -> name[k++] = buf[j];
		  m -> name[k] = 0;
		}
		end_this_str_idx = j;
		str_length_org = k;
		if (have_quote_scan)
		  break;
	      } else {
		in_test_str = true;
		if (end_this_str_idx == -1)
		  m -> name[k++] = buf[j];
	      }
	    }
	    break;
	  case '\\':
	    if (esc_chr == (j - 1)) {
	      esc_chr = -2;
	      if (end_this_str_idx == -1)
		m -> name[k++] = buf[j];
	    } else {
	      esc_chr = j;
	      if (end_this_str_idx == -1)
		m -> name[k++] = buf[j];
	    }
	    break;
	  case '/':
	    /*
	     *  Note the start of a comment.
	     */
	    if (!in_test_str) {
	      switch (buf[j+1])
		{
		case '*':
		  scan_c_cmt = TRUE;
		  break;
		case '/':
		  scan_cplus_cmt = TRUE;
		  break;
		}
	    } else {
	      if (end_this_str_idx == -1)
		m -> name[k++] = buf[j];
	    }
	    break;
	  case '*':
	    /*
	     *  Note the end of a C comment.
	     */
	    if (!in_test_str) {
	      if ((buf[j+1] == '/') && scan_c_cmt)
		scan_c_cmt = FALSE;
	    } else {
	      if (end_this_str_idx == -1)
		m -> name[k++] = buf[j];
	    }
	    break;
	  case '\n':
	    /*
	     *  Note the end of a C++ comment, or stop at the 
	     *  end of a line that contains a preprocess 
	     *  directive.
	     */
	    if (!in_test_str)
	      if (scan_cplus_cmt) scan_cplus_cmt = FALSE;

	    if (preprocess_line) {
	      /*
	       *  Line splice - handle below.
	       */
	      if (esc_chr != (j - 1)) {
		str_length_org = k;
		m -> name[k] = 0;
		goto eoppl;
	      } else {
		if (end_this_str_idx == -1)
		  m -> name[k++] = buf[j];
	      }
	    } else {
	      if (end_this_str_idx == -1)
		m -> name[k++] = buf[j]; 
	    }
	    break;
	  case '\r':
	    /*
	     *  The same as above, but handle MS-DOG line endings
	     *  also.  NOTE - This is still largely untested.
	     */
	    if (!in_test_str)
	      if (scan_cplus_cmt) scan_cplus_cmt = FALSE;
	    if (preprocess_line) {
	      /*
	       *  Line splice - Handle below.
	       */
	      if (esc_chr != (j - 1)) {
		goto eoppl;
	      } else {
		if (end_this_str_idx == -1) { 
		  m -> name[k++] = buf[j++];
		  m -> name[k++] = buf[j];
		}
	      }
	    } else {
	      if (end_this_str_idx == -1) { 
		m -> name[k++] = buf[j++]; 
		m -> name[k++] = buf[j]; 
	      }
	    }
	    break;
	  default:
	    if (end_this_str_idx == -1)
	      m -> name[k++] = buf[j];
	    break;
	  }
      }

      eoppl:

      /*
       *  If the number of quotes to the end of the input is odd,
       *  then we have an unterminated string somehow.  
       */

      if (!have_quote_scan) {
	if (n_quotes % 2) {
	  /*
	   *  We have to find the location of the unterminated constant
	   *  immediately, due to the backoff down below.  First 
	   *  check for a missing quote, then for a mismatched
	   *  escaped quote.
	   */
	  if (!str_term_err_loc) {
	    str_term_err_loc = quote_mismatch_idx (buf, *idx-1);
	    if (!str_term_err_loc || !buf[str_term_err_loc])
	      str_term_err_loc = esc_quote_mismatch_idx (buf, *idx-1);
	  }
	  if (preprocess_line) {
	    if (index (m -> name, '\n')) {
	      backoff = index (m -> name, '\0') - index (m -> name, '\n') + 1;
	      m -> name[index (m -> name, '\n') - m -> name] = 0;
	    } else { /* if (index (m -> name, '\n')) */
	      backoff = 2;
	    }
	  }
	} else { /* if (n_quotes % 2) */
	  have_quote_scan = true;
	}
      } /* if (!have_quote_scan) */

      while ((splice_ptr = strstr (m -> name, LINE_SPLICE_STR)) != NULL) {
	strcpy (splice_ptr, splice_ptr + LINE_SPLICE_STR_LENGTH);
	++error_line;
      }

      *idx += str_length_org - backoff;
    tok_cat_start:
      for (tok_cat_idx = *idx; buf[tok_cat_idx]; ++tok_cat_idx) {
	if (isspace ((int)buf[tok_cat_idx])) {
	  continue;
	} else if (buf[tok_cat_idx] == '"') {
	  char *next_quote, *lookahead;
	  int message_size;
	  message_size = (strlen (m -> name) > MAXLABEL) ?
	    (strlen (m -> name) * 2) : MAXLABEL;
	  /* We have to do a syntax check to look for a document 
	     string - the second string is followed by a semicolon. 
	     (Ctalk can also adjust if it sees a similar construct
	     in a method argument while doing a semantics check.) */
	  if ((next_quote = strchr (&buf[tok_cat_idx + 1], '"'))
	      != NULL) {
	    for (lookahead = next_quote + 1; *lookahead; ++lookahead) {
	      if (isspace ((int)*lookahead))
		continue;
	      else if (*lookahead == ';')
		goto tok_cat_done;
	      else
		break;
	    }
	  }
	  --k;
	  *idx = tok_cat_idx + 1;
	  for (tok_cat_idx_2 = tok_cat_idx + 1; buf[tok_cat_idx_2];
	       ++tok_cat_idx_2) {
	    if (buf[tok_cat_idx_2] == '"') {
	      m -> name[k++] = buf[tok_cat_idx_2];
	      m -> name[k] = '\0';
	      *idx += 1;
	      /* look for yet another string following this one,
		 and repeat */
	      for (lookahead = &buf[*idx]; *lookahead; ++lookahead) {
		if (isspace ((int)*lookahead))
		  continue;
		else if (*lookahead == '"')
		  goto tok_cat_start;
		else
		  goto tok_cat_done;
	      }
	    } else {
	      m -> name[k++] = buf[tok_cat_idx_2];
	      *idx += 1;
	      if (k > (message_size - 3)) {
		message_size *= 2;
		resize_message (m, message_size);
	      }
	    }
	  }
	} else {
	  break;
	}
      }
    tok_cat_done:
      
      m -> tokentype = LITERAL;
      return LITERAL;
    } /*     if (!in_c_cmt && !in_cplus_cmt) */
  } /*   if (c == '"') */

  /* LITERAL_CHAR in single quotes.*/

  if (c == '\'') {
    if (!in_c_cmt && !in_cplus_cmt) {
      int i = 0;
      int realloc_x = MAXLABEL;
      m -> name[i++] = c;
      while (1) {
	m -> name[i++] = buf[(*idx)++];
	/*
	 *  If quoting an escaped single quote, break
	 *  on the final single quote only.
	 */
	if (i >= realloc_x) {
	  realloc_x *= 2;
	  resize_message (m, realloc_x);
	}
	if ((buf[(*idx) - 1] == '\'') && (buf[*idx] != '\''))
	  break;
      }
      m -> name[i] = '\0';
      if (!is_char_constant (m -> name)) {
	if (!strcmp (M_NAME(m), "''")) {
	  strcpy (m->name, "'\\0'");
	} else {
	  _warning ("%s:%d: Warning: Unknown character sequence %c.\n", 
		    INCLUDE_SOURCE_NAME, error_line, m -> name[0]);
	}
      }
      m -> tokentype = LITERAL_CHAR;
      return LITERAL_CHAR;
    }
  }

  /* ARGSEPARATOR */
  if (c == ',') {
    _MPUTCHAR(m,c)
    m -> tokentype = ARGSEPARATOR;
    return ARGSEPARATOR;
  }

  /* ARRAYOPEN */
  if (c == '[') {
    _MPUTCHAR(m,c)
    m -> tokentype = ARRAYOPEN;
    return ARRAYOPEN;
  }

  /* ARRAYCLOSE */
  if (c == ']') {
    _MPUTCHAR(m,c)
    m -> tokentype = ARRAYCLOSE;
    return ARRAYCLOSE;
  }

  /* MODULUS */
  if (c == '%') {
    _MPUTCHAR(m,c)
    m -> tokentype = MODULUS;
    return MODULUS;
  }

  if (c == 0) {
    m -> tokentype = EOS;
    _MPUTCHAR(m,'\n')
    return 0;
  }

  if (c == '\\') {
    /* 
     *  Line splice - replace escape and newline with whitespace.  
     */
    if (!strncmp (&buf[*idx], "\r\n", 2) ||
	!strncmp (&buf[*idx], "\n\r", 2)) {
      _MPUTCHAR(m, ' ')
      m -> tokentype = WHITESPACE;
      (*idx) += 2;
      ++linesplice;
      return WHITESPACE;
    } else {
      if ((buf[*idx] == '\n') || (buf[*idx] == '\r')) {
	_MPUTCHAR(m, ' ')
	m -> tokentype = WHITESPACE;
	++(*idx);
	++linesplice;
	return WHITESPACE;
      }
    }
  }


  _MPUTCHAR(m,c)
  m -> tokentype = CHAR;
  return c;
}

/* Tokenize a character string and place the messages on the 
   message stack used by the (push() () function.
   Return the last stack pointer.
*/

int tokenize (int (*push)(MESSAGE *), char *buf) {

  long long i = 0L, max;
  int tok;
  MESSAGE *m = NULL, 
    *m_cmt = NULL;
  int stack_ptr = ERROR;
  char tbuf[MAXLABEL],
    s[MAXMSG];

  max = (long long) strlen (buf);

  i = in_c_cmt = in_cplus_cmt = linesplice = str_term_err_loc = 0; 

  gnuc_attribute = FALSE;
  while (i < max) {

    if ((m = new_message()) == NULL)
      _error ("Tokenize: Invalid new_message.");

    tok = lexical (buf, &i, m);  /* get token */

    unterm_str_warning (i);

    if (keepcomments_opt) {

      /*
       *  Resize the message name to the size of the input, and
       *  collect the tokens.
       */
      if (tok == OPENCCOMMENT) {
	if ((++comment > 1) && warnnestedcomments_opt) {
	  SNPRINTF (s, MAXMSG, "%s: %d: Warning: \'\\*\' within a comment.\n",
		   source_file, error_line);
	  _warning (s);
	}
	if (comment == 1) {
	  m_cmt = m;
	  stack_ptr = (push) (m_cmt);
	  strcpy (tbuf, m_cmt -> name);
  	  free (m_cmt -> name);
 	  m_cmt -> name = calloc (input_size + 1, sizeof (char));
	  strcpy (m_cmt -> name, tbuf);
	} else {
	  strcat (m_cmt -> name, m -> name);
	  delete_message (m);
	}
	set_error_location (m_cmt, &error_line, &error_column);
      } else {
	if (tok == CLOSECCOMMENT) {
	  --comment;
	  strcat (m_cmt -> name, m -> name);
	  set_error_location (m, &error_line, &error_column);
	  delete_message (m);
	  if (!comment) {
	    m_cmt -> tokentype = PREPROCESS_EVALED;
	  }
	} else {
          if (comment) {
            strcat (m_cmt -> name, m -> name);
            set_error_location (m, &error_line, &error_column);
            delete_message (m);
          } else {
            if (tok == CPPCOMMENT) {
              m -> tokentype = PREPROCESS_EVALED;
              stack_ptr = (push) (m);
            } else {
	      stack_ptr = (push) (m);
	      set_error_location (m, &error_line, &error_column);
	    }
	  }
	}
      }

    } else { /* if (keepcomments_opt) */

      if (tok == OPENCCOMMENT){
	if ((++comment > 1) && warnnestedcomments_opt) {
	  char buf[MAXMSG];
	  SNPRINTF (buf, MAXMSG, 
		    "%s: %d: Warning: \'\\*\' within a comment.\n",
		   source_file, error_line);
	  _warning (buf);
	}
	_MPUTCHAR(m, ' ')
	m -> tokentype = WHITESPACE;
	stack_ptr = (push) (m);
	set_error_location (m, &error_line, &error_column);
	continue;
      }

      if (tok == CLOSECCOMMENT) {
	delete_message (m);
	--comment;
	continue;
      }

      if (tok == CPPCOMMENT) {
	_MPUTCHAR(m, ' ')
	m -> tokentype = WHITESPACE;
	stack_ptr = (push) (m);
	set_error_location (m, &error_line, &error_column);
	continue;
      }

      if (comment && m -> tokentype != NEWLINE) {
	delete_message (m);
	continue;
      }

      /*
       *  If lexical () encounters a line splice, 
       *  the function replaces it with white space.
       *  However, in order to keep the line numbers
       *  straight, we insert an equal number of spaces
       *  after the logical line - when lexical () 
       *  reaches the actual newline.
       *  
       *  We do this here so that we don't have to
       *  worry about inserting a line marker after a
       *  macro.
       */
      if (m -> tokentype == NEWLINE && linesplice) {
	while (--linesplice >= 0)
	  strcat (m->name, "\n");
	linesplice = 0;
      }

      stack_ptr = (push) (m);
      set_error_location (m, &error_line, &error_column);
    } /* !keepcomments_opt */
  }

  error_reset ();

  return stack_ptr;
}

/* Tokenize a character string and place the messages on the 
   message stack used by the (push() () function.
   Return the last stack pointer.
*/

int tokenize_reuse (int (*push)(MESSAGE *), char *buf) {

  long long i = 0L, max;
  int tok;
  MESSAGE *m = NULL, 
    *m_cmt = NULL;
  int stack_ptr = ERROR;
  char tbuf[MAXLABEL],
    s[MAXMSG];

  max = (long long) strlen (buf);

  i = in_c_cmt = in_cplus_cmt = linesplice = str_term_err_loc = 0; 

  gnuc_attribute = FALSE;
  while (i < max) {

    if ((m = get_reused_message()) == NULL)
      _error ("Tokenize_reuse: Invalid new_message.");

    tok = lexical (buf, &i, m);  /* get token */

    unterm_str_warning (i);

    if (keepcomments_opt) {

      /*
       *  Resize the message name to the size of the input, and
       *  collect the tokens.
       */
      if (tok == OPENCCOMMENT) {
	if ((++comment > 1) && warnnestedcomments_opt) {
	  SNPRINTF (s, MAXMSG, "%s: %d: Warning: \'\\*\' within a comment.\n",
		   source_file, error_line);
	  _warning (s);
	}
	if (comment == 1) {
	  m_cmt = m;
	  stack_ptr = (push) (m_cmt);
	  strcpy (tbuf, m_cmt -> name);
  	  free (m_cmt -> name);
 	  m_cmt -> name = calloc (input_size + 1, sizeof (char));
	  strcpy (m_cmt -> name, tbuf);
	} else {
	  strcat (m_cmt -> name, m -> name);
	  delete_message (m);
	}
	set_error_location (m_cmt, &error_line, &error_column);
      } else {
	if (tok == CLOSECCOMMENT) {
	  --comment;
	  strcat (m_cmt -> name, m -> name);
	  set_error_location (m, &error_line, &error_column);
	  delete_message (m);
	  if (!comment) {
	    m_cmt -> tokentype = PREPROCESS_EVALED;
	  }
	} else {
          if (comment) {
            strcat (m_cmt -> name, m -> name);
            set_error_location (m, &error_line, &error_column);
            delete_message (m);
          } else {
            if (tok == CPPCOMMENT) {
              m -> tokentype = PREPROCESS_EVALED;
              stack_ptr = (push) (m);
            } else {
	      stack_ptr = (push) (m);
	      set_error_location (m, &error_line, &error_column);
	    }
	  }
	}
      }

    } else { /* if (keepcomments_opt) */

      if (tok == OPENCCOMMENT){
	if ((++comment > 1) && warnnestedcomments_opt) {
	  char buf[MAXMSG];
	  SNPRINTF (buf, MAXMSG, 
		    "%s: %d: Warning: \'\\*\' within a comment.\n",
		   source_file, error_line);
	  _warning (buf);
	}
	_MPUTCHAR(m, ' ')
	m -> tokentype = WHITESPACE;
	stack_ptr = (push) (m);
	set_error_location (m, &error_line, &error_column);
	continue;
      }

      if (tok == CLOSECCOMMENT) {
	delete_message (m);
	--comment;
	continue;
      }

      if (tok == CPPCOMMENT) {
	_MPUTCHAR(m, ' ')
	m -> tokentype = WHITESPACE;
	stack_ptr = (push) (m);
	set_error_location (m, &error_line, &error_column);
	continue;
      }

      if (comment && m -> tokentype != NEWLINE) {
	delete_message (m);
	continue;
      }

      /*
       *  If lexical () encounters a line splice, 
       *  the function replaces it with white space.
       *  However, in order to keep the line numbers
       *  straight, we insert an equal number of spaces
       *  after the logical line - when lexical () 
       *  reaches the actual newline.
       *  
       *  We do this here so that we don't have to
       *  worry about inserting a line marker after a
       *  macro.
       */
      if (m -> tokentype == NEWLINE && linesplice) {
	while (--linesplice >= 0)
	  strcat (m->name, "\n");
	linesplice = 0;
      }

      stack_ptr = (push) (m);
      set_error_location (m, &error_line, &error_column);
    } /* !keepcomments_opt */
  }

  error_reset ();

  return stack_ptr;
}

/*
 *   Return true if the preceding message matches c.
 */
int prefix (MESSAGE_STACK messages, int msg_ptr, int c) {

  MESSAGE *m;

  m = messages[msg_ptr+1];

  if (!m || !IS_MESSAGE (m))
    return FALSE;

  if (m -> name[0] == (char) c)
    return TRUE;

  return FALSE;
}

int set_error_location (MESSAGE *m, int *line, int *col) {

  char *p, *q;

  m -> error_column = *col;
  m -> error_line = *line;
  if (m -> tokentype == NEWLINE) {
    *col = 1;
    *line += strlen (m -> name);
  } else {
    *col += strlen (m -> name);
  }
  if (m -> tokentype == LITERAL) {
    for (p = m -> name, q = index (m -> name, '\n'); ; 
	 q = index (p+1, '\n')) {
      if (!q)
	break;
      *col = 0;
      ++(*line);
      p = q;
    }      
    *col += strlen (p);
  }
  return SUCCESS;
}

/*
 * Collect messages into a buffer for retokenization. 
 */

char *collect_tokens (MESSAGE_STACK messages, int start, int end) {

  int i,
    buflength;
  char *buf;

  for (i = start, buflength = 0; i >= end; i--)
    buflength += strlen (messages[i] -> name);

  if ((buf = (char *)__xalloc (buflength + 1 * sizeof (char))) == NULL)
    _error ("collect_tokens: %s.", strerror (errno));

  for (i = start; i >= end; i--) 
    strcat (buf, messages[i] -> name);

  return buf;
}

char unescape_trigraph (char *s) {

  int i;
  struct _trigraph {char key, val; } trigraphs[] =
    { {'=',  '#'},
      {'(',  '['},
      {'/',  '\\'},
      {')',  ']'},
      {'\'', '^'},
      {'<',  '{'},
      {'!',  '|'},
      {'>',  '}'},
      {'-',  '~'},
      {0,    0}};

  for (i = 0; trigraphs[i].key; i++) {
    if (trigraphs[i].key == s[2])
      return trigraphs[i].val;
  }
  return 0;
}

int trigraph_tokentype (char c) {

  int i;
  struct _trigraph {char key; int val; } trigraphs[] =
    { {'#',  PREPROCESS },
      {'[',  ARRAYOPEN  },
      {'\\',  BACKSLASH  },
      {']',  ARRAYCLOSE },
      {'^', BIT_XOR    },
      {'{',  OPENBLOCK  },
      {'|',  BAR        },
      {'}',  CLOSEBLOCK },
      {'~',  BIT_COMP   },
      {0,    0}};

  for (i = 0; trigraphs[i].key; i++) {
    if (trigraphs[i].key == c)
      return trigraphs[i].val;
  }
  return 0;
}

/*
 *  Return True if a string contains a valid
 *  character constant or escape sequence.
 */
int is_char_constant (char *s) {

  RADIX r;
  char t[MAXMSG];

  strcpy (t, s);
  if (t[0] == '\'')
    TRIM_CHAR(t);

  switch (strlen (t))
    {
    case 0:
      return FALSE;
      break;
    case 1:
      if (t[0] == '\'' || t[0] == '\\' || t[0] == '\n')
	return FALSE;
      else
	return TRUE;
      break;
    case 2:
      /*
       *  Simple escape sequences.
       */
      if (t[0] == '\\') {
	if (t[1] == '\'' || t[1] == '\?' || 
	    t[1] == '\\' || t[1] == 'a' || t[1] == 'b' ||
	    t[1] == 'e' ||
	    t[1] == 'f' || t[1] == 'n' || t[1] == 'r' ||
	    t[1] == 't' || t[1] == 'v' || t[1] == '0' || t[1] == '\"') {
	  return TRUE;
	} else{
	  return FALSE;
	}
      } else {
	return FALSE;
      }
      break;
    default:
      /*
       *  Numeric escape sequences.
       */
      if (t[0] == '\\') {
	if ((r = radix_of (&t[1])) != (RADIX)ERROR) {
	  return TRUE;
	} else {
	  return FALSE;
	}
      }
      break;
    }

  return FALSE;
}

int quote_mismatch_idx (char *s, int start_idx) {

  int i, j, n_quotes, in_c_cmt, in_cplus_cmt;
  int q_err_idx = 0; /* Avoid a warning. */

  for (i = start_idx; s[i]; ++i) {
    if ((s[i] == '"') && (s[i-1] != '\\')) {
      for (j = i, n_quotes = 0, in_c_cmt = 0, in_cplus_cmt = 0; 
	   s[j]; ++j) {
	switch (s[j])
	  {
	  case '/':
	    if (s[j+1] == '*') {
	      ++j;
	      in_c_cmt = TRUE;
	    }
	    if (s[j+1] == '/') {
	      ++j;
	      in_c_cmt = TRUE;
	    }
	    break;
	  case '*':
	    if (s[j+1] == '/') {
	      ++j;
	      in_c_cmt = FALSE;
	    }
	    break;
	  case '\r':
	  case '\n':
	    if (in_cplus_cmt) in_cplus_cmt = FALSE;
	    break;
	  case '"':
	    if (s[j-1] != '\\')
	      if (!in_c_cmt && !in_cplus_cmt)
		++n_quotes;
	    break;
	  default:
	    break;
	  }
      }
      while (s[++i]) { 
	if ((s[i] == '"') && (s[i-1] != '\\'))  break; 
      }
      if (n_quotes % 2) {
	q_err_idx = i;
      } else {
	return q_err_idx;
      }
    }
  }
  return -1;
}

int esc_quote_mismatch_idx (char *s, int start_idx) {

  int i, j, idx, n_quotes;

  for (i = start_idx, idx = 0; s[i]; ++i) {
    if (s[i] == '\\' && s[i+1] == '"') {
      for (j = i + 2, n_quotes = 0; s[j]; j++) {
	if (s[j] == '\\' && s[j+1] == '"')
	  ++n_quotes;
      }
      if (n_quotes % 2) 
	idx = i;
      /*
       *  Try to find a change from an odd number of remaining quotes to
       *  and even number.
       */
      if (idx && !(n_quotes % 2))
	return i;
    }
  }
  return idx;
}

int unterm_str_warning (int idx) {
  if (str_term_err_loc && (idx > str_term_err_loc))  {
    _warning ("%s:%d: Unterminated string constant.\n", 
	      INCLUDE_SOURCE_NAME, error_line);
    str_term_err_loc = 0;
  }
  return SUCCESS;
}
