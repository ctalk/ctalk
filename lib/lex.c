/* $Id: lex.c,v 1.2 2019/12/01 20:17:27 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2018  Robert Kiesling, rk3314042@gmail.com.
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
#include <stdbool.h>
#include "lex.h"
#include "lextab.h"
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "typeof.h"
#include "rtinfo.h"

/* TO DO -
   Use the international abbreviations in iso646.h.
*/


extern int error_line;      /* Declared in errorloc.c. */
extern int error_column;
extern RT_INFO rtinfo;      /* Declared in rtinfo.c.   */

extern int line_info_line;  /* Declared in lineinfo.c. */

extern I_PASS interpreter_pass;  /* Declared in main.c. */

bool have_quote_check = false;
extern bool natural_text;         /* Declared in textcx.c, changes
				    how to regard single quotes,
				    etc.*/

/*
 *  Used for parsing literals.
 */
#define DEFL_ESC_IDX -2
#define ADD_CHR_RESIZE if (end_str == -1) { m -> name[k++] = buf[j]; m -> name[k] = '\0';} else { continue ; } \
  if (k >= resize_x) {resize_x *= 2; resize_message (m, resize_x); } \
  if (buf[j] == '\n' || buf[j] == '\r') {++error_line; error_column = 1;} else {++error_column;}

#define _MPUTCHAR(__m,__c) (__m)->name[0] = (__c); (__m)->name[1] = '\0';
#define _MPUT2CHAR(__m,__s) (__m)->name[0] = (__s[0]); \
				 (__m)->name[1] = (__s[1]); \
				 (__m)->name[2] = '\0';
#define _MPUT3CHAR(__m,__s) (__m)->name[0] = (__s[0]); \
				 (__m)->name[1] = (__s[1]); \
				 (__m)->name[2] = (__s[2]); \
				 (__m)->name[3] = '\0';

static int gnuc_attribute;  /* Set by __attribute__ label, cleared after )). */

int esc_chr;


static bool __is_sign (char c, const char *buf, int idx,
		       bool *have_sign) {

  int i;

  if (ASCII_DIGIT(buf[idx])) {
    // c is always at buf[idx-1].
    if ((i = (idx - 2)) >= 0) {
      while (isspace ((int)buf[i])) {
	if (--i < 0) {
	  // leading spaces at beginning of buffer.
	  *have_sign = true;
	  return true;
	}
      }

      if (IS_C_OP_CHAR(buf[i])) {
	*have_sign = true;
	return true;
      } else {
	return false;
      }

    } else {
      // c is at the beginning of the buffer.
      *have_sign = true;
      return true;
    }
  }
  
  return false;
}

static bool is_decimal_exp (const char *r, bool *have_exp) {
  if (*r == 'e' || *r == 'E') {
    if (*(r+1) == '+' || *(r+1) == '-') {
      *have_exp = true;
      return true;
    }
  }
  return false;
}

static bool is_hex_exp (const char *r, bool *have_exp) {
  if (*r == 'p' || *r == 'P') {
    if (*(r+1) == '+' || *(r+1) == '-') {
      *have_exp = true;
      return true;
    }
  }
  return false;
}

/* comment_start_idx should point to the char immediately after
   the open comment string - "/*" or even the opening asterisk 
   char. */
static inline int find_c_comment_close (const char *buf, 
					int comment_start_idx) {
  int i, level;
  for (i = comment_start_idx, level = 1; buf[i]; i++) {
    if (buf[i] == '*') {
      if (buf[i+1] && (buf[i+1] == '/')) {
	if (level == 1) {
	  return i + 1;
	} else {
	  --level;
	}
      }
    }
    if (buf[i] == '/') {
      if (buf[i+1] && (buf[i+1] == '*')) {
	++level;
      }
    }
  }
  return -1;
}

static inline int find_cpp_comment_close (const char *buf, 
					  int comment_start_idx) {
  int i;
  for (i = comment_start_idx; buf[i]; i++) {
    if (buf[i] == '\n') {
      return i;
    }
  }
  return -1;
}

/* Start directly after the supposed delimiter.  Check for the second
   unescaped delimiter and a separator character after that. */
static int char_is_delimiter (const char *buf, int open_delim_idx) {
  int i, n_parens = 0, lookback;
  char delim = buf[open_delim_idx];
  if (!ispunct ((int)delim))
    return ERROR;
  /* also check that the starting char is not the first term of
     an expression. */
  /* patterns can (so far) only follow a =~ or !~ operator. */
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
    } else if (buf[lookback] == '=') {
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

/*
 *    Performs lexical scanning.  Returns the token in 
 *    m -> name.  The token types are defined in lex.h.
 */

void lexical (const char *buf, long long *idx, MESSAGE *m) {

  char c, c_next;
  int i, j, k;
  int numeric_type;
  int buflength;
  int resize_x;
  int in_str, in_quoted_chr, j_1, n_quotes, end_str;
  int end_idx;
  int lookback;
  int label_length;
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
  int need_signed_hexadecimal_constant_warning = FALSE,
    need_signed_octal_constant_warning = FALSE;
#endif
  char *q, *r, *s;
  bool have_sign = false;
  bool have_exp = false;
  RADIX radix = decimal;


  if (*idx == 0)
    have_quote_check = false;

  c = buf[(*idx)++];

  switch (chr_tok_class[c])
    {
    case C_WHITESPACE:
      --(*idx);
      q = m -> name;
      r = (char *)&buf[*idx];
      while (*r == c) {
	*q++ = *r++;
	if (q == s)
	  break;
      }
      *q = '\0';
      *idx += (q - m -> name);
      m -> tokentype = WHITESPACE;
      return;
      break;
    case C_NEWLINE:
      buflength = 0;
      resize_x = MAXLABEL - 1;
      if ((*idx >= 2) && buf[*idx - 2] == '\\') {
	_MPUTCHAR(m, ' ')
	  /* Increment the error line and reset error column also. */
	  ++error_line; error_column = 0;
	m -> tokentype = WHITESPACE;
	return;
      }

      _MPUTCHAR (m, '\n');
      if (natural_text) {
	m -> tokentype = NEWLINE;
	return;
      }
      
      while (buf[*idx] == c) {
	if (++buflength >= resize_x) {
	  resize_x += resize_x;
	  resize_message (m, resize_x);
	}
	m -> name[buflength] = '\n';
	(*idx)++;
      }
      m -> name[buflength+1] = '\0';
      m -> tokentype = NEWLINE;
      return;
      break;
    case C_ASTERISK:
      switch (buf[*idx]) 
	{
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, "*=")
	    m -> tokentype = MULT_ASSIGN;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	    m -> tokentype = ASTERISK;
	  return;
	  break;
	}
      break;
    case C_LT:
      switch (buf[*idx])
	{
	case '<':
	  (*idx)++;
	  switch (buf[*idx])
	    {
	    case '=':
	      (*idx)++;
	      _MPUT3CHAR(m, "<<=")
		m -> tokentype = ASL_ASSIGN;
	      return;
	      break;
	    default:
	      _MPUT2CHAR(m, "<<")
		m -> tokentype = ASL;
	      return;
	      break;
	    }
	  break;
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, "<=")
	    m -> tokentype = LE;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	    m -> tokentype = LT;
	  return;
	  break;
	}
      break;
    case C_GT:
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
	      return;
	      break;
	    default:
	      _MPUT2CHAR(m, ">>")
		m -> tokentype = ASR;
	      return;
	      break;
	    }
	  break;
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, ">=")
	    m -> tokentype = GE;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	    m -> tokentype = GT;
	  return;
	  break;
	}
      break;
    case C_BIT_XOR:
      switch (buf[*idx])
	{
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, "^=")
	    m -> tokentype = BIT_XOR_ASSIGN;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	    m -> tokentype = BIT_XOR;
	  return;
	  break;
	}
      break;
    case C_BOOLEAN_OR:
      switch (buf[*idx])
	{
	case '|':
	  (*idx)++;
	  _MPUT2CHAR(m, "||")
	    m -> tokentype = BOOLEAN_OR;
	  return;
	  break;
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, "|=")
	    m -> tokentype = BIT_OR_ASSIGN;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	    m -> tokentype = BAR;
	  return;
	  break;
	}
      break;
    case C_AMPERSAND:
      switch (buf[*idx])
	{
	case '&':
	  (*idx)++;
	  _MPUT2CHAR(m, "&&")
	    m -> tokentype = BOOLEAN_AND;
	  return;
	  break;
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, "&=")
	    m -> tokentype = BIT_AND_ASSIGN;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	/*
	 *  If the preceding non-whitespace character is an operator,
	 *  then the ampersand is an address-of operator, otherwise
	 *  it is a bitwise and operator.
	 * 
	 *  If the ampersand is the first character of the buffer,
	 *  then it is an address-of operator.
	 */
	    if (((*idx) - 1) == 0) {
	      m -> tokentype = AMPERSAND;
	      return;
	    } else {
	      int c_idx;
	      for (c_idx = (*idx) - 2; c_idx >= 0; c_idx--) {
		if (isspace ((int)buf[c_idx])) 
		  continue;
		if (IS_C_OP_CHAR (buf[c_idx])) {
		  if ((buf[c_idx] == ')') ||
		      (buf[c_idx] == ']')) {
		    m -> tokentype = BIT_AND;
		    return;
		  } else {
		    m -> tokentype = AMPERSAND;
		    return;
		  }
		} else {
		  m -> tokentype = BIT_AND;
		  return;
		}
	      }
	    }
	  break;
	}
      break;
    case C_EQ:
      switch (buf[*idx])
	{
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, "==")
	    m -> tokentype = BOOLEAN_EQ;
	  return;
	  break;
	case '~':
	  (*idx)++;
	  _MPUT2CHAR(m, "=~")
	    m -> tokentype = MATCH;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	    m -> tokentype = EQ;
	  return;
	  break;
	}
      break;
    case C_BIT_COMP:
      _MPUTCHAR(m,c)
	m -> tokentype = BIT_COMP;
      return;
      break;
    case C_EXCLAM:
      switch (buf[*idx]) 
	{
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, "!=")
	    m -> tokentype = INEQUALITY;
	  return;
	  break;
	case '~':
	  (*idx)++;
	  _MPUT2CHAR(m, "!~")
	    m -> tokentype = NOMATCH;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	    m -> tokentype = EXCLAM;
	  return;
	  break;
	}
      break;
    case C_MODULUS:
      switch (buf[*idx])
	{
	case '=':
	  (*idx)++;
	  _MPUT2CHAR(m, "%=")
	    m -> tokentype = MODULUS_ASSIGN;
	  return;
	  break;
	default:
	  _MPUTCHAR(m,c)
	    m -> tokentype = MODULUS;
	  return;
	  break;
	}
      break;
    case LINESPLICE:
      if ((buf[*idx] == '\r' && buf[*idx+1] == '\n') ||
	  (buf[*idx] == '\n' && buf[*idx+1] == '\r')) {
	_MPUTCHAR(m, ' ')
	  m -> tokentype = WHITESPACE;
	(*idx) += 2;
	return;
      } else {
	if ((buf[*idx] == '\n') || (buf[*idx] == '\r')) {
	  _MPUTCHAR(m, ' ')
	    m -> tokentype = WHITESPACE;
	  ++(*idx);
	  return;
	}
      }
      break;
    case C_LITERAL:
      for (j = *idx - 1, in_str = FALSE, in_quoted_chr = FALSE,
	     k = 0, resize_x = MAXLABEL - 1,
	     esc_chr = DEFL_ESC_IDX, n_quotes = 0, end_str = -1; 
	   buf[j];
	   j++) {
	switch (buf[j])
	  {
	  case '"':
	    if (esc_chr == (j - 1)) {
	      esc_chr = DEFL_ESC_IDX;
	      ADD_CHR_RESIZE
		} else {
	      ++n_quotes;
	      if (in_str == TRUE) {
		in_str = FALSE;
		ADD_CHR_RESIZE
		  end_str = j;
		if (have_quote_check)
		  break;
	      } else {
		in_str = TRUE;
		ADD_CHR_RESIZE
		  }
	    }
	    break;
	  case '\\':
	    esc_chr = (esc_chr == DEFL_ESC_IDX) ? j : DEFL_ESC_IDX;
	    ADD_CHR_RESIZE
	      break;
	  case '\'':
	    /*
	     *  Check for a '"' but not a "'c'"; i.e., only 
	     *  register the escape on the leading single quote.
	     */
	    if (!in_quoted_chr) {
	      /* TODO- Tidy this whole clause up a bit. */
	      if (esc_chr != DEFL_ESC_IDX) 
		esc_chr = DEFL_ESC_IDX;
	      in_quoted_chr = TRUE;
	    } else {
	      esc_chr = DEFL_ESC_IDX;
	      in_quoted_chr = FALSE;
	    }
	    ADD_CHR_RESIZE
	      break;

	    /* Scan past comments. */
	  case '/':
	    if (!in_str) {
	      if (buf[j+1]) {
		switch (buf[j+1])
		  {
		  case '*':
		    if ((j_1 = find_c_comment_close (buf, j + 1)) != ERROR)
		      j = j_1;
		    break;
		  case '/':
		    if ((j_1 = find_cpp_comment_close (buf, j + 1)) != ERROR)
		      j = j_1;
		    break;
		  default:
		    ADD_CHR_RESIZE
		      break;
		  }
	      } else {
		if (esc_chr != DEFL_ESC_IDX) esc_chr = DEFL_ESC_IDX;
		ADD_CHR_RESIZE
		  }
	    } else {
	      if (esc_chr != DEFL_ESC_IDX) esc_chr = DEFL_ESC_IDX;
	      ADD_CHR_RESIZE
		}
	    break;
	  default:
	    if (esc_chr != DEFL_ESC_IDX) esc_chr = DEFL_ESC_IDX;
	    ADD_CHR_RESIZE
	      break;
	  }
      }

      /*
       *  If the number of quotes to the end of the input is odd,
       *  then we have an unterminated string somehow.
       */

      if (!have_quote_check) {
	if (n_quotes % 2) {
	  switch (interpreter_pass)
	    {
	    case run_time_pass:
	      _warning ("%s:%d:warning: Unterminated string constant.\n",
			((__getClassLibRead () == True) ? __classLibFileName () : 
			 __argvFileName ()),
			error_line );
	      break;
	    case preprocessing_pass:
	    case var_pass:
	    case parsing_pass:
	    case library_pass:
	    case method_pass:
	    case expr_check:
	    case c_fn_pass:
	      _warning ("%s:%d:warning: Unterminated string constant.\n",
			__source_filename (), error_line);
	    default:
	      break;
	    }
	} else {
	  have_quote_check = true;
	}
      }

      m -> name[k] = 0;
      *idx += k - 1;
      m -> tokentype = LITERAL;
      return;
      break;
    case C_EOS:
      m -> tokentype = EOS;
      _MPUTCHAR(m,'\n')
	return;
      break;
    case C_LITERAL_CHAR:
      i = 0;
      resize_x = MAXLABEL - 1;
      m -> name[i++] = c;
      if (natural_text) {
	m -> name[i] = '\0';
	m -> tokentype = SINGLEQUOTE;
	return;
      }
      while (1) {
	if ((buf[*idx] == ';') && (buf[(*idx)-1] != '\''))
	  _error ("Unterminated character constant line %d, column %d.\n",
		  error_line, error_column);
	m -> name[i++] = buf[(*idx)++], m -> name[i] = 0;
	if (i >= resize_x) {
	  resize_x *= 2;
	  resize_message (m, resize_x);
	}
	/*
	 *  If quoting an escaped single quote, break
	 *  on the final single quote only.
	 */
	if ((buf[(*idx) - 1] == '\'') && (buf[*idx] != '\''))
	  break;
	/*
	 *  If we have double escaped NULL - '\\0', then
	 *  save it as '\0'.
	 */
	if (buf[*idx] == '0' && buf[*idx + 1] == '\'') {
	  m->name[0] = '\''; m->name[1] = '\\';m->name[2] = '0'; 
	  m -> name[3] = '\''; m -> name[4] = '\0';
	}
      }
      m -> tokentype = LITERAL_CHAR;
      return;
      break;
    case C_PREPROCESS:
      lookback = ((*idx - 2) > 0) ? *idx - 2 : 0;

      while (((buf[lookback] == ' ') || 
	      (buf[lookback] == '\t')) &&
	     (lookback > 0)) {
	lookback--;
      }

      if ((buf[lookback] == '\n' || lookback == 0)) {
	_MPUTCHAR(m,c)
	  m -> tokentype = PREPROCESS;
	return;
      } else {
	if (buf[*idx] == '#') {
	  (*idx)++;
	  _MPUT2CHAR(m, "##")
	    m -> tokentype = MACRO_CONCAT;
	  return;
	} else {
	  _MPUTCHAR(m,c)
	    m -> tokentype = LITERALIZE;
	  return;
	}
      }
    break;
    case COMMENT_DIV_PAT:
      if (buf[*idx] == '*') {
	int comment_close_idx;
	if ((comment_close_idx = find_c_comment_close (buf, *idx)) == -1) {
	  _warning ("Unterminated C comment.\n");
	}
	_MPUTCHAR(m, ' ')
	  (*idx) = comment_close_idx + 1;
	m -> tokentype = OPENCCOMMENT;
	return;
      } else if ((end_idx = char_is_delimiter (buf, (*idx) - 1)) != ERROR) {
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
	    return;
	  }
	}
      } else if (buf[*idx] == '/') {
	q = index (&buf[*idx], '\n');
	*idx += (q - &buf[*idx]);
	_MPUTCHAR(m, '\n')
	 m -> tokentype = CPPCOMMENT;
	return;
      } else {
	switch (buf[*idx]) 
	  {
	  case '=':
	    _MPUT2CHAR(m, "/=")
	      ++(*idx);
	    m -> tokentype = DIV_ASSIGN;
	    return;
	    break;
	  default:
	    _MPUTCHAR(m,c)
	      m -> tokentype = DIVIDE;
	    return;
	    break;
	  }
      }
      break;
    case C_LABEL:
      switch (c)
	{
	case 's':
	  if (!strncmp ((char *)&buf[*idx], (char *)"izeof", 5)) {
	    int _sizeof_char_after_idx = (*idx) + 5;
	    if (!isalnum ((int)buf[_sizeof_char_after_idx]) && 
		(buf[_sizeof_char_after_idx] != '_')) {
	      *idx = _sizeof_char_after_idx;
	      strcpy (m -> name, "sizeof");
	      m -> tokentype = SIZEOF;
	      return;
	    }
	  }
	  goto lex_all_labels;
	  break;
	case 'm':
	  if ((end_idx = char_is_delimiter (buf, *idx)) != ERROR) {
	    j = 0;
	    (*idx)--; /* the 'm' is still useful to the regexp parser. */
	    if ((end_idx - *idx) > MAXLABEL) {
	      resize_message (m, (end_idx - *idx) + 1);
	    }
	    while (1) {
	      m -> name[j++] = buf[(*idx)++];
	      if (*idx > end_idx) {
		m -> name[j] = 0;
		m -> tokentype = PATTERN;
		return;
	      }
	    }
	  }
	  goto lex_all_labels;
	  break;
	default:
	lex_all_labels:
	  q = m -> name;
	  *q++ = c;
	label_loop:
          if (LABEL_CHAR(buf[*idx])) {
	    *q++ = buf[*idx];
	  } else {
	    *q = '\0';
	    m -> tokentype = LABEL;
	    return;
	  }
	  if ((q - m -> name) == MAXLABEL) {
	    *q = '\0';
	    m -> tokentype = LABEL;
	    return;
	  }
	  (*idx)++;
	  goto label_loop;
	  break;
	}  /* switch (c) */
      break;
    case PERIOD_ELLIPSIS:
      if (ASCII_DIGIT(buf[*idx])) {
	goto lex_numeric_constant;
      } else {
	if (buf[*idx] == '.' && buf[*idx+1] == '.') {
	  (*idx) += 2;
	  _MPUT3CHAR(m, "...")
	    m -> tokentype = ELLIPSIS;
	  return;
	} else {
	  _MPUTCHAR(m,c)
	    m -> tokentype = PERIOD;
	  return;
	}
      }
      break;
    case C_PLUS:
      if (__is_sign (c, (char *)buf, *idx, &have_sign)) {
	goto lex_numeric_constant;
      } else {
	switch (buf[*idx])
	  {
	  case '=':
	    (*idx)++;
	    _MPUT2CHAR(m, "+=")
	      m -> tokentype = PLUS_ASSIGN;
	    return;
	    break;
	  case '+':
	    (*idx)++;
	    _MPUT2CHAR(m, "++")
	      m -> tokentype = INCREMENT;
	    return;
	    break;
	  default:
	    _MPUTCHAR(m,c)
	      m -> tokentype = PLUS;
	    return;
	    break;
	  }
      }
      break;
    case C_MINUS:
      if (__is_sign (c, (char *)buf, *idx, &have_sign)) {
	goto lex_numeric_constant;
      } else {
	switch (buf[*idx])
	  {
	  case '=':
	    (*idx)++;
	    _MPUT2CHAR(m, "-=")
	      m -> tokentype = MINUS_ASSIGN;
	    return;
	    break;
	  case '-':
	    (*idx)++;
	    _MPUT2CHAR(m, "--")
	      m -> tokentype = DECREMENT;
	    return;
	    break;
	  case '>':
	    (*idx)++;
	    _MPUT2CHAR(m, "->")
	      m -> tokentype = DEREF;
	    return;
	    break;
	  default:
	    _MPUTCHAR(m,c)
	      m -> tokentype = MINUS;
	    return;
	    break;
	  }
      }
      break;
    case NUMERIC_CONSTANT:
    lex_numeric_constant:
      (*idx)--;
      r = (char *)&buf[*idx];
      q = m -> name;
      if (have_sign)
	*q++ = *r++;
      numeric_type = INTEGER_T;
    
      c_next = *(r+1);
      if ((*r == '0') && (c_next != '.')) {
	if (c_next == 'x' || c_next == 'X') {
	  radix = hexadecimal;
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
	  if (have_sign)
	    need_signed_hexadecimal_constant_warning = TRUE;
#endif
	  *q++ = *r++;
	  *q++ = *r++;
	} else if (c_next >= '0' && c_next <= '7') {
	  radix = octal;
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
	  if (have_sign)
	    need_signed_octal_constant_warning = TRUE;
#endif
	  *q++ = *r++;
	}
      }

    numeric_scan:

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
	if (*r == (char)'.' && ASCII_DIGIT ((int)*(r+1)))
#else
	  if (*r == (char)'.' && ASCII_DIGIT (*(r+1)))
#endif
	    numeric_type = DOUBLE_T;

	switch (*r) 
	  {
	  case 'l':
	    if (*(r+1) == (char)'l') {
	      if (numeric_type == INTEGER_T) {
		numeric_type = LONGLONG_T;
	      } else {
		if (numeric_type == DOUBLE_T) {
		  numeric_type = LONGDOUBLE_T;
		}
	      }
	      *q++ = *r++;
	    } else {
	      if (numeric_type == INTEGER_T) {
		numeric_type = LONG_T;
	      } else {
		if (numeric_type == DOUBLE_T) {
		  numeric_type = LONGDOUBLE_T;
		}
	      }
	    }
	    break;
	  case 'L':
	    if (numeric_type == INTEGER_T) {
	      numeric_type = LONGLONG_T;
	    } else {
	      if (numeric_type == DOUBLE_T) {
		numeric_type = LONGDOUBLE_T;
	      }
	    }
	    break;
	  case 'f':
	  case 'F':
	    if (numeric_type == DOUBLE_T) 
	      numeric_type = FLOAT_T;
	    break;
	  }

	switch (radix)
	  {
	  case decimal:
	    switch (numeric_type)
	      {
	      case INTEGER_T:
		if (!ASCII_DIGIT ((int)*r) && !INT_SUFFIX(*r)) {
		  goto numeric_scan_done;
		} else {
		  *q++ = *r++;
		  goto numeric_scan;
		}
		break;
	      case LONG_T:
	      case LONGLONG_T:
		if (!ASCII_DIGIT ((int)*r) &&
		    !LONGLONG_INT_SUFFIX(*r)) {
		  goto numeric_scan_done;
		} else {
		  *q++ = *r++;
		  goto numeric_scan;
		}
		break;
	      case DOUBLE_T:
		if (!ASCII_DIGIT ((int)*r) && (*r != '.') &&
		    !is_decimal_exp (r, &have_exp)) {
		  goto numeric_scan_done;
		} else {
		  if (have_exp) {
		    *q++ = *r++;
		  }
		  *q++ = *r++;
		  goto numeric_scan;
		}
		break;
	      case FLOAT_T:
		if (!ASCII_DIGIT ((int)*r) && (*r != '.') &&
		    !FLOAT_SUFFIX(*r) &&
		    !is_decimal_exp (r, &have_exp)) {
		  goto numeric_scan_done;
		} else {
		  if (have_exp) {
		    *q++ = *r++;
		  }
		  *q++ = *r++;
		  goto numeric_scan;
		}
		break;
	      case LONGDOUBLE_T:
		if (!ASCII_DIGIT ((int)*r) && (*r != '.') &&
		    !LONG_DOUBLE_SUFFIX(*r) &&
		    !is_decimal_exp (r, &have_exp)) {
		  goto numeric_scan_done;
		} else {
		  if (have_exp) {
		    *q++ = *r++;
		  }
		  *q++ = *r++;
		  goto numeric_scan;
		}
		break;
	      } /* switch (numeric_type) */
	    break; /* case decimal: */

	  case octal:
	    /* Octals need range checking. */

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
	    if (ASCII_DIGIT((int)*r) && 
#else
	    if (ASCII_DIGIT(*r) && 
#endif
		((*r < (char)'0') || (*r > (char)'7')))
		_error ("Bad octal constant.");

	    if (*r < '0' || *r > '7') {
	      goto numeric_scan_done;
	    } else {
	      *q++ = *r++;
	      goto numeric_scan;
	    }
	    break; /* case octal: */

        case hexadecimal:
	  if ((*r == '.') ||
	      is_hex_exp (r, &have_exp)) {
	    numeric_type = DOUBLE_T;
	    have_exp = false; /* reset in case this is used below */
	  }

	  switch (numeric_type)
	    {
	    case INTEGER_T:
	      if (!HEX_ASCII_DIGIT (*r) && !INT_SUFFIX(*r)) {
		goto numeric_scan_done;
	      } else {
		*q++ = *r++;
		goto numeric_scan;
	      }
	      break;
	    case LONG_T:
	    case LONGLONG_T:
	      if (!HEX_ASCII_DIGIT (*r) &&
		  !LONGLONG_INT_SUFFIX(*r)) {
		goto numeric_scan_done;
	      } else {
		*q++ = *r++;
		goto numeric_scan;
	      }
	      break;
	    case DOUBLE_T:
	      if (!HEX_ASCII_DIGIT (*r) && (*r != '.') &&
		  !is_hex_exp (r, &have_exp)) {
		goto numeric_scan_done;
	      } else {
		if (have_exp) {
		  *q++ = *r++;
		}
		*q++ = *r++;
		goto numeric_scan;
	      }
	      break;
	    case LONGDOUBLE_T:
	      if (!HEX_ASCII_DIGIT (*r) && (*r != '.') &&
		  !LONG_DOUBLE_SUFFIX(c) &&
		  !is_hex_exp (r, &have_exp)) {
		goto numeric_scan_done;
	      } else {
		if (have_exp) {
		  *q++ = *r++;
		}
		*q++ = *r++;
		goto numeric_scan;
	      }
	      break;
	    case FLOAT_T:
	      if (!HEX_ASCII_DIGIT (*r) && (*r != '.') &&
		  !FLOAT_SUFFIX(*r) &&
		  !is_hex_exp (r, &have_exp)) {
		goto numeric_scan_done;
	      } else {
		if (have_exp) {
		  *q++ = *r++;
		}
		*q++ = *r++;
		goto numeric_scan;
	      }
	      break;
	    } /* switch (numeric_type) */
	  break; /* case hexadecimal: */
        case binary:
	  break;
       } /* switch (radix) */
	    
  numeric_scan_done:
	*q = '\0';

	*idx += (r - &buf[*idx]);
	
#ifndef WITHOUT_SIGNED_HEX_OCTAL_CONSTANT_WARNINGS
    if (need_signed_hexadecimal_constant_warning) {
      _warning ("Signed hexadecimal constant %s.\n", m -> name);
    }
    if (need_signed_octal_constant_warning) {
      _warning ("Signed octal constant %s.\n", m -> name);
    }
#endif
    switch (numeric_type)
      {
      case INTEGER_T:
	m -> tokentype = INTEGER;
	break;
      case DOUBLE_T:
      case FLOAT_T:
      case LONGDOUBLE_T:
 	m -> tokentype = DOUBLE;
	break;
      case LONG_T:
	m -> tokentype = LONG;
	break;
      case LONGLONG_T:
	m -> tokentype = LONGLONG;
	break;
      }
    return;
    break;
    case MISC:
      _MPUTCHAR(m,c)
	m -> tokentype = ascii_chr_tok[c];
      return;
      break;
    } /* switch (chr_tok_class[c]) */

  _MPUTCHAR(m,c)
  m -> tokentype = CHAR;

}

static inline void tokenize_error_location (MESSAGE *m, int *line, int *col) {

  register char *p;

   if (!strpbrk (m -> name, "\n")) {
     m -> error_column = *col;
     m -> error_line = *line;
     p = m -> name;
     while (*p++)
       ++(*col);
     return;
   }
  if (m -> tokentype == NEWLINE) {
    *col = 1;
    p = m -> name;
    while (*p++)
      ++(*line);
  } else if (m -> tokentype != LITERAL) {
    /* LITERAL tokens update the error line and column
       in lexical. */
    p = m -> name;
    while (*p++)
      ++(*col);
  }
}

/* Tokenize a character string and place the messages on the 
   message stack used by the (push() () function.
   Return the last stack pointer.
*/

int tokenize (int (*push)(MESSAGE *), char *buf) {

  long long i = 0L;
  MESSAGE *m;
  int stack_ptr = ERROR;             /* Avoid warnings. */
  int comment_open_line = FALSE;

  gnuc_attribute = FALSE;
  while (buf[i]) {

    if ((m = new_message()) == NULL)
      _error ("Tokenize: Invalid new_message.");

  reuse_message:
    lexical (buf, &i, m);  /* get token */

    if (m -> tokentype == OPENCCOMMENT){
      /* Eats the whole comment now. */
      _MPUTCHAR(m, ' ')
	m -> tokentype = WHITESPACE;
      stack_ptr = (push) (m);
      continue;
    }

    stack_ptr = (push) (m);
    tokenize_error_location (m, &error_line, &error_column);
  }

  error_line = error_column = 1;

  return stack_ptr;
}

int tokenize_reuse (int (*push)(MESSAGE *), char *buf) {

  long long i = 0L;
  MESSAGE *m;
  int stack_ptr = ERROR;             /* Avoid warnings. */
  int comment_open_line = FALSE;

  gnuc_attribute = FALSE;
  while (buf[i]) {

    if ((m = get_reused_message()) == NULL)
      _error ("Tokenize: Invalid new_message.");

  reuse_message:
    lexical (buf, &i, m);  /* get token */

    if (m -> tokentype == OPENCCOMMENT){
      /* Eats the whole comment now. */
      _MPUTCHAR(m, ' ')
	m -> tokentype = WHITESPACE;
      stack_ptr = (push) (m);
      continue;
    }

    stack_ptr = (push) (m);
  }

  error_line = error_column = 1;

  return stack_ptr;
}

int tokenize_no_error (int (*push)(MESSAGE *), char *buf) {

  long long i = 0L;
  MESSAGE *m;
  int stack_ptr = ERROR;             /* Avoid warnings. */
  int comment_open_line = FALSE;

  gnuc_attribute = FALSE;
  while (buf[i]) {

    if ((m = get_reused_message ()) == NULL)
      _error ("Tokenize: Invalid new_message.");

  reuse_message:
    lexical (buf, &i, m);  /* get token */

    if (m -> tokentype == OPENCCOMMENT){
      /* Eats the whole comment now. */
      _MPUTCHAR(m, ' ')
	m -> tokentype = WHITESPACE;
      stack_ptr = (push) (m);
      continue;
    }

    stack_ptr = (push) (m);
  }

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

/*
 * Collect messages into a buffer for retokenization. 
 */

char *collect_tokens (MESSAGE_STACK messages, int start, int end) {

  int i, size, cnt;
  char *buf, *l, *m;

  size = MAXMSG;

  if ((buf = __xalloc (size)) == NULL)
    _error ("collect_tokens: %s.", strerror (errno));

  l = buf;
  cnt = 0;
  for (i = start; i >= end; i--) { 

    if (!IS_MESSAGE(messages[i]))
      return buf;

    m = messages[i] -> name;
    
    
    while (*m) {
      *l++ = *m++;
      ++cnt;
      if (cnt >= size) {
	_warning ("collect_tokens: buffer overflow.\n");
	return buf;
      }
    }
  }

  return buf;
}

char *collect_tokens_buf (MESSAGE_STACK messages, int start, int end,
			   char *buf_out) {

  int i;
  char *l, *m;

  l = buf_out;
  for (i = start; i >= end; i--) { 

    if (!IS_MESSAGE(messages[i]))
      return buf_out;

    m = messages[i] -> name;
    
    while (*m) {
      *l++ = *m++;
    }
  }
  *l = 0;

  return buf_out;
}

int find_expression_limit (MSINFO *ms) {

  int idx, n_parens, n_blocks, n_braces, have_parens;
  int has_fn_form;
  int lookahead_idx;
  int lookahead_idx_2;
  int fn_idx;
  int prev_tok = -1;
  MESSAGE *m_tok;
  MESSAGE_STACK messages = ms -> messages;

  n_parens = n_blocks = n_braces = have_parens = 0;
  has_fn_form = -1;

  for (idx = ms -> tok; idx > ms -> stack_ptr; idx--) {
    if (M_ISSPACE(messages[idx]))
      continue;
    m_tok = messages[idx];
    if ((lookahead_idx = __ctalkNextLangMsg (messages, idx, ms -> stack_ptr))
 	!= ERROR) {
      switch (M_TOK(messages[lookahead_idx]))
	{
	case ARGSEPARATOR:
 	  if ((!n_parens) ||
	      /* Catches the final paren before a comma. */
	      ((M_TOK(m_tok) == CLOSEPAREN) && (n_parens == 1))) {
	    if (messages[ms -> tok] -> attrs &
		RT_TOK_IS_PRINTF_ARG) {
	      /*
	       *  Handles the case where the expression is part of a 
	       *  *printf () argument that is a function call; e.g., 
	       *
	       *   printf ("fmt", <fn> (<arg1>, <arg2>, <argn>), <printf_arg2>)
	       *
	       *  So the arguments <arg1>, <arg2>, <argn>, etc., get 
	       *  grouped with <fn>, not the printf format.
	       */
	      if (obj_expr_is_arg (messages, ms -> tok,
				   ms -> stack_start,
				   &fn_idx) != ERROR) {
		return idx;
	      } else {
		if (is_fmt_arg (messages, ms -> tok, ms -> stack_start,
				ms -> stack_ptr)) 
		  return idx;
	      }
	    } else {
	      if (obj_expr_is_arg (messages, ms -> tok,
				   ms -> stack_start,
				   &fn_idx) != ERROR) {
		return idx;
	      }
	    }
 	  }
 	  break;
	case CLOSEPAREN:
	  if (!have_parens) {
	    /* If we have a C fn as the closing argument of a list
	       without parentheses. */
	    if (M_TOK(m_tok) == OPENPAREN) {
	      if ((lookahead_idx_2 = 
		   __ctalkNextLangMsg (messages, lookahead_idx, ms -> stack_ptr))
		  != -1) {
		if (M_TOK(messages[lookahead_idx_2]) == SEMICOLON) {
		  return lookahead_idx;
		}
	      }
	    }
	  }
	  /* If it's the second closing paren in (*fn)(), keep going. */
	  if (prev_tok != OPENPAREN) {
	    if (!n_parens) {
	      return idx;
	    }
	  }
	  break;
 	}
    } else {
      if (idx == (ms -> stack_ptr + 1)) {
	return idx;
      }
    }
    switch (M_TOK(m_tok))
      {
      case LABEL:
	/*
	 *  Check for C function syntax at the first label.
	 */
	if (has_fn_form == -1) {
	  if ((lookahead_idx = __ctalkNextLangMsg (messages, idx, ms -> stack_ptr))
	      != -1) {
	    if (M_TOK(messages[lookahead_idx]) == OPENPAREN) {
	      has_fn_form = 1;
	    } else {
	      has_fn_form = 0;
	    }
	  } else {
	    return idx;
	  }
	} else {
	  if (lookahead_idx == ERROR) {
	    return idx;
	  }
	}
	break;
      case OPENPAREN:
	++n_parens;
	have_parens = TRUE;
	break;
      case CLOSEPAREN:
	--n_parens;
 	if (n_parens < 0) {
 	  ++idx;
	  return idx;
	} else {
	  if (n_parens == 0) {

	    /* This can occur during an expr_check. */
	    if (idx == (ms -> stack_ptr + 1))
	      return idx;

	    /*
	     *  Only return if the token after the closing paren
	     *  is an argument list terminator.  If the next token
	     *  is a label or operator, keep going.
	     */
	    if (M_TOK(messages[ms -> tok]) == OPENPAREN) {
	      if ((lookahead_idx_2 = 
		   __ctalkNextLangMsg (messages, idx, ms -> stack_ptr))
		  != ERROR) {
		if (METHOD_ARG_TERM_MSG_TYPE(messages[lookahead_idx_2])) {
		  return idx;
		} else {
		  /* An expression like: (*fn)()
		                             ^--- you are here.
		  */
		  if (M_TOK(messages[lookahead_idx_2]) == OPENPAREN) {
		    prev_tok = M_TOK(messages[lookahead_idx_2]);
		    continue;
		  } else if (M_TOK(messages[lookahead_idx_2]) == LABEL) {
		    /* (<label-or-prefix-op> <label-series>) <label> ... 
                                  lookahead_idx_2 ---------^
		    */
		    prev_tok = M_TOK(messages[lookahead_idx_2]);
		    continue;
		  }
		}
	      } else {
		return idx;
	      }
	    } else {
	      /*
	       *  Or... If we find another closing paren, check for
	       *  a matching open paren immediately before the
	       *  expression's first token.
	       */
	      if ((lookahead_idx_2 = 
		   __ctalkNextLangMsg (messages, idx, ms -> stack_ptr))
		  != ERROR) {
		if (M_TOK(messages[lookahead_idx_2]) == CLOSEPAREN) {
		  int open_paren_ptr, expr_start_prev_tok_ptr;
		  if ((open_paren_ptr = __ctalkMatchParenRev 
		       (messages, lookahead_idx_2, ms -> stack_start))
		      != ERROR) {
		    if ((expr_start_prev_tok_ptr = __ctalkPrevLangMsg
			 (messages, ms -> tok, ms -> stack_start))
			!= ERROR) {
		      if (expr_start_prev_tok_ptr == open_paren_ptr) {
			return idx;
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
	break;
      case ARRAYOPEN:
	++n_blocks;
	break;
      case ARRAYCLOSE:
	if (--n_blocks < 0) {
	  /* Don't include in expression. */
	  ++idx;
	  return idx;
	}
	break;
      case OPENBLOCK:
	++n_braces;
	break;
      case CLOSEBLOCK:
	if (--n_braces < 0)
	  return idx;
	break;
      case SEMICOLON:
	/* Don't include in expression. */
	++idx;
	return idx;
	break;
      case ARGSEPARATOR:
 	if (!n_parens && have_parens && has_fn_form)
 	  return idx;
	break;
      default:
	break;
      }
  }

  return idx;
}

/*
 *  Collect an expression into a buffer.
 */
char *collect_expression_buf (MSINFO *ms, 
			      int *expr_end_idx, char *buf_out) { 

  *expr_end_idx = find_expression_limit (ms);

  collect_tokens_buf (ms -> messages,
		      ms -> tok, *expr_end_idx, buf_out);
  return buf_out;
}

char *collect_expression (MSINFO *ms, 
			      int *expr_end_idx) { 
  char *exprbuf;
  *expr_end_idx = find_expression_limit (ms);

  exprbuf = collect_tokens (ms -> messages,
			    ms -> tok, *expr_end_idx);
  return exprbuf;
}

int expr_outer_parens (MESSAGE_STACK messages, int expr_start_idx,
		       int expr_end_idx, int stack_start_idx, 
		       int stack_end_idx, int *start_paren_idx,
		       int *end_paren_idx) {

  int lookback_idx,
    paren_level,
    i,
    expr_limit_tok;
  MESSAGE *m;

  paren_level = 0;
  *start_paren_idx = *end_paren_idx = -1;
  for (lookback_idx = expr_start_idx; lookback_idx <= stack_start_idx; 
	 lookback_idx++) {
    
    m = messages[lookback_idx];

    if (M_ISSPACE(m)) continue;

    expr_limit_tok = M_TOK(m);

    if ((expr_limit_tok == SEMICOLON) ||
	(expr_limit_tok == OPENBLOCK) ||
	(expr_limit_tok == ARRAYOPEN) ||
	(expr_limit_tok == CONDITIONAL) ||
	(expr_limit_tok == COLON)) {
      goto expr_start_found;
    }
  }

 expr_start_found:

  for (i = lookback_idx; i >= expr_start_idx; i--) {
    if (M_TOK(messages[i]) == OPENPAREN) {
      ++paren_level;
      *start_paren_idx = i;
      if ((*end_paren_idx = __ctalkMatchParen (messages, i, stack_end_idx))
	  == ERROR) {
	_warning ("Mismatched parentheses.\n");
      }
    }

    if (M_TOK(messages[i]) == CLOSEPAREN)
    --paren_level;
  }

  return paren_level;
}
