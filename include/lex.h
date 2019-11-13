/* $Id: lex.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2018 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _LEX_H
#define _LEX_H

typedef enum {
  decimal,
  octal,
  hexadecimal,
  binary
} RADIX;

#define EOS                  -1
#define OPENPAREN             1
#define CLOSEPAREN            2
#define BAR                   3
#define CHAR                  4
#define LABEL                 5
#define LITERAL               6
#define LITERAL_CHAR          7
#define C_KEYWORD             8
#define PRIMITIVE             9
#define ARGSEPARATOR          10     /* Comma */
#define INTEGER               11
#define FLOAT                 12
#define DOUBLE FLOAT                 /* See the comment in lex.c. */
#define LONG                  13
#define LONGLONG              14
/* semantic use of label */
#define METHODMSGLABEL        15
#define SEMICOLON             16
#define OPENBLOCK             17
#define CLOSEBLOCK            18

#define RESULT                19    /* Result of subexression parse */

#define EXPRCLOSE             20
#define PREPROCESS            21
#define WHITESPACE            22

/* Semantic use of label. */
#define CTYPE                 23

#define OPENCCOMMENT          32
#define CLOSECCOMMENT         33
#define CPPCOMMENT            34
#define AMPERSAND             35
#define EXCLAM                36
#define CR                    37
#define LF                    38
#define NEWLINE               39

#define PREPROCESS_EVALED     40

#define EQ                    41
#define BOOLEAN_EQ            42

#define GT                    43
#define GE                    44
#define ASR                   45
#define ASR_ASSIGN            46

#define LT                    47
#define LE                    48
#define ASL                   49
#define ASL_ASSIGN            50

#define PLUS                  51
#define PLUS_ASSIGN           52
#define INCREMENT             53

#define MINUS                 54
#define MINUS_ASSIGN          55
#define DECREMENT             56
#define DEREF                 57

#define ASTERISK              58
#define MULT ASTERISK
#define MULT_ASSIGN           59

#define DIVIDE                60
#define DIV_ASSIGN            61
#define FWDSLASH DIVIDE

#define BIT_AND               AMPERSAND
#define BOOLEAN_AND           63
#define BIT_AND_ASSIGN        64

#define BIT_COMP              65

#define LOG_NEG EXCLAM
#define INEQUALITY            66

#define BIT_OR BAR
#define BOOLEAN_OR            67
#define BIT_OR_ASSIGN         68

#define BIT_XOR               69
#define BIT_XOR_ASSIGN        70

#define PERIOD                71
#define ELLIPSIS              72

#define LITERALIZE            73
#define POUND                 LITERALIZE
#define MACRO_CONCAT          74

#define SIZEOF                75

#define CONDITIONAL           76       /* question mark */
#define COLON                 77

#define ARRAYOPEN             78
#define ARRAYCLOSE            79

#define MODULUS               80
#define PERCENT               MODULUS

#define WLITERAL              81
#define WLITERAL_CHAR         82

#define BACKSLASH             83

#define DIGIT                 84
#define MODULUS_ASSIGN        85

#define DOLLAR                86

#define MATCH                 87
#define NOMATCH               88

#define PATTERN               89

#define SINGLEQUOTE           90

#define FMTPATTERN

#define ASCII_DIGIT(c) (ascii_chr_tok[(int)c] == DIGIT)
#define HEX_ASCII_DIGIT(c) ((ascii_chr_tok[(int)c] == DIGIT) || \
			    ((c) >= 'a' && (c) <= 'f') || \
			    ((c) >= 'A' && (c) <= 'F'))
#define INT_SUFFIX(c) ((c) == 'u' || (c) == 'U' || (c) == 'b' || (c) == 'B')
#define LONGLONG_INT_SUFFIX(c) ((c) == 'u' || (c) == 'U' || (c) == 'b' || \
				(c) == 'B' || (c) == 'l' || (c) == 'L')
#define FLOAT_SUFFIX(c) ((c) == 'f' || (c) == 'F')
#define LONG_DOUBLE_SUFFIX(c) ((c) == 'l' || (c) == 'L')

/* Operators that can't evaluate to a value; that is, not possible
   operands. */
#define IS_C_OP_TOKEN_NOEVAL(tok) \
  ((tok == EQ) || \
   (tok == BOOLEAN_EQ) || \
   (tok == GT) || \
   (tok == GE) || \
   (tok == ASR) || \
   (tok == ASR_ASSIGN) || \
   (tok == LT) || \
   (tok == LE) || \
   (tok == ASL) || \
   (tok == ASL_ASSIGN) || \
   (tok == PLUS) || \
   (tok == PLUS_ASSIGN) || \
   (tok == INCREMENT) || \
   (tok == MINUS) || \
   (tok == MINUS_ASSIGN) || \
   (tok == DECREMENT) || \
   (tok == DEREF) || \
   (tok == ASTERISK) || \
   (tok == MULT_ASSIGN) || \
   (tok == DIVIDE) || \
   (tok == DIV_ASSIGN) || \
   (tok == BIT_AND) || \
   (tok == BOOLEAN_AND) || \
   (tok == BIT_AND_ASSIGN) || \
   (tok == BIT_COMP) || \
   (tok == LOG_NEG) || \
   (tok == INEQUALITY) || \
   (tok == BIT_OR) || \
   (tok == BOOLEAN_OR) || \
   (tok == BIT_OR_ASSIGN) || \
   (tok == BIT_XOR) || \
   (tok == BIT_XOR_ASSIGN) || \
   (tok == PERIOD) || \
   (tok == ELLIPSIS) || \
   (tok == LITERALIZE) || \
   (tok == MACRO_CONCAT) ||\
   (tok == SEMICOLON) ||\
   (tok == ARGSEPARATOR) ||\
   (tok == OPENPAREN) ||\
   (tok == ARRAYOPEN) ||\
   (tok == OPENBLOCK) || \
   (tok == CONDITIONAL) || \
   (tok == COLON) || \
   (tok == MATCH) || \
   (tok == NOMATCH))

#define IS_C_UNARY_MATH_OP(tok) \
   ((tok == INCREMENT) || \
    (tok == DECREMENT) || \
    (tok == BIT_COMP) ||  \
    (tok == LOG_NEG) ||	  \
    (tok == AMPERSAND) || \
    (tok == ASTERISK) || \
    (tok == SIZEOF))

#define IS_C_BINARY_MATH_OP(tok) \
  ((tok == PLUS) || \
   (tok == MINUS) || \
   (tok == ASTERISK) || \
   (tok == DIVIDE) || \
   (tok == MODULUS) || \
   (tok == ASL) || \
   (tok == ASR) || \
   (tok == LT) || \
   (tok == LE) || \
   (tok == GT) || \
   (tok == GE) || \
   (tok == BOOLEAN_EQ) || \
   (tok == INEQUALITY) || \
   (tok == BIT_AND) || \
   (tok == BIT_OR) || \
   (tok == BIT_XOR) || \
   (tok == BOOLEAN_AND) || \
   (tok == BOOLEAN_OR) || \
   (tok == EQ) || \
   (tok == ASR_ASSIGN) || \
   (tok == ASL_ASSIGN) || \
   (tok == PLUS_ASSIGN) || \
   (tok == PLUS_ASSIGN) || \
   (tok == MINUS_ASSIGN) || \
   (tok == MULT_ASSIGN) || \
   (tok == DIV_ASSIGN) || \
   (tok == BIT_AND_ASSIGN) || \
   (tok == BIT_OR_ASSIGN) || \
   (tok == BIT_XOR_ASSIGN))

#define IS_C_OP(tok) \
  ((tok == OPENPAREN) || \
   (tok == CLOSEPAREN) || \
   (tok == ARRAYOPEN) || \
   (tok == ARRAYCLOSE) || \
   (tok == INCREMENT) || \
   (tok == DECREMENT) || \
   (tok == DEREF) || \
   (tok == SIZEOF) || \
   (tok == EXCLAM) || \
   (tok == PLUS) || \
   (tok == MINUS) || \
   (tok == ASTERISK) || \
   (tok == AMPERSAND) || \
   (tok == DIVIDE) || \
   (tok == MODULUS) || \
   (tok == ASL) || \
   (tok == ASR) || \
   (tok == LT) || \
   (tok == LE) || \
   (tok == GT) || \
   (tok == GE) || \
   (tok == BOOLEAN_EQ) || \
   (tok == INEQUALITY) || \
   (tok == BIT_AND) || \
   (tok == BIT_OR) || \
   (tok == BIT_XOR) || \
   (tok == BIT_COMP) || \
   (tok == BOOLEAN_AND) || \
   (tok == BOOLEAN_OR) || \
   (tok == CONDITIONAL) || \
   (tok == COLON) || \
   (tok == EQ) || \
   (tok == ASR_ASSIGN) || \
   (tok == ASL_ASSIGN) || \
   (tok == PLUS_ASSIGN) || \
   (tok == PLUS_ASSIGN) || \
   (tok == MINUS_ASSIGN) || \
   (tok == MULT_ASSIGN) || \
   (tok == DIV_ASSIGN) || \
   (tok == BIT_AND_ASSIGN) || \
   (tok == BIT_OR_ASSIGN) || \
   (tok == BIT_XOR_ASSIGN) || \
   (tok == LITERALIZE) || \
   (tok == MACRO_CONCAT) || \
   (tok == MATCH) ||\
   (tok == NOMATCH))

#define IS_C_OP_CHAR(c) \
  (((c) == '(') || \
   ((c) == ')') || \
   ((c) == '[') || \
   ((c) == ']') || \
   ((c) == '!') || \
   ((c) == '+') || \
   ((c) == '-') || \
   ((c) == '*') || \
   ((c) == '&') || \
   ((c) == '/') || \
   ((c) == '%') || \
   ((c) == '<') || \
   ((c) == '>') || \
   ((c) == '=') || \
   ((c) == '&') || \
   ((c) == '^') || \
   ((c) == '|') || \
   ((c) == '?') || \
   ((c) == ':') || \
   ((c) == '#') || \
   ((c) == '.') || \
   ((c) == ','))

#define IS_STDARG_FMT_CHAR(c) \
  (((c) == ' ') ||	      \
   ((c) == '%') ||	      \
   ((c) == ',') ||	      \
   ((c) == '-') ||	      \
   ((c) == '+') ||	      \
   ((c) == '#') ||	      \
   ((c) == '0') ||	      \
   ((c) == 'd') ||	      \
   ((c) == 'i') ||	      \
   ((c) == 'f') ||	      \
   ((c) == 'F') ||	      \
   ((c) == 'e') ||	      \
   ((c) == 'E') ||	      \
   ((c) == 'g') ||	      \
   ((c) == 'G') ||	      \
   ((c) == 'o') ||	      \
   ((c) == 'u') ||	      \
   ((c) == 'x') ||	      \
   ((c) == 'X') ||	      \
   ((c) == 'a') ||	      \
   ((c) == 'A') ||	      \
   ((c) == 'c') ||	      \
   ((c) == 'C') ||	      \
   ((c) == 'w') ||	      \
   ((c) == 's') ||	      \
   ((c) == 'S') ||	      \
   ((c) == 'p') ||	      \
   ((c) == 'n') ||	      \
   ((c) == 'h') ||	      \
   ((c) == 'l') ||	      \
   ((c) == 'j') ||	      \
   ((c) == 'z') ||	      \
   ((c) == 't') ||	      \
   ((c) == 'L') ||	      \
   ((c) == '1') ||	      \
   ((c) == '2') ||	      \
   ((c) == '3') ||	      \
   ((c) == '4') ||	      \
   ((c) == '5') ||	      \
   ((c) == '6') ||	      \
   ((c) == '7') ||	      \
   ((c) == '8') ||	      \
   ((c) == '9') ||	      \
   ((c) == '.'))

#define IS_NUMERIC_CONSTANT_TOK(tok) \
  (((tok) == LITERAL_CHAR) ||	     \
   ((tok) == INTEGER) ||	     \
   ((tok) == FLOAT) ||		     \
   ((tok) == DOUBLE) ||		     \
   ((tok) == LONG) ||		     \
   ((tok) == LONGLONG))

#define IS_CONSTANT_TOK(tok) \
  (((tok) == LITERAL_CHAR) ||	     \
   ((tok) == LITERAL) ||	     \
   ((tok) == INTEGER) ||	     \
   ((tok) == FLOAT) ||		     \
   ((tok) == DOUBLE) ||		     \
   ((tok) == LONG) ||		     \
   ((tok) == CHAR) ||		     \
   ((tok) == LONGLONG))

#define IS_C_ASSIGNMENT_OP(tok) \
  ((tok == EQ) || \
   (tok == ASR_ASSIGN) || \
   (tok == ASL_ASSIGN) || \
   (tok == PLUS_ASSIGN) || \
   (tok == MINUS_ASSIGN) || \
   (tok == MULT_ASSIGN) || \
   (tok == DIV_ASSIGN) || \
   (tok == BIT_AND_ASSIGN) || \
   (tok == BIT_OR_ASSIGN) || \
   (tok == BIT_XOR_ASSIGN))

#define IS_C_ASSIGNMENT_OP_NAME(s) \
  ((s[0] == '=') || \
   (s[0] == '>' && s[1] == '>' && s[2] == '=') || \
   (s[0] == '<' && s[1] == '<' && s[2] == '=') || \
   (s[0] == '+' && s[1] == '=') ||\
   (s[0] == '-' && s[1] == '=') || \
   (s[0] == '*' && s[1] == '=') || \
   (s[0] == '/' && s[1] == '=') || \
   (s[0] == '&' && s[1] == '=') || \
   (s[0] == '|' && s[1] == '=') || \
   (s[0] == '^' && s[1] == '='))

/* binary ops only - LOG_NEG gets handled as a prefix operator. */
#define IS_C_RELATIONAL_OP(tok) \
  ((tok == BOOLEAN_EQ) || 				\
   (tok == GT) ||					\
   (tok == GE) ||					\
   (tok == LT) ||					\
   (tok == LE) ||					\
   (tok == BOOLEAN_AND) ||				\
   (tok == BOOLEAN_OR) ||				\
   (tok == INEQUALITY))

#define IS_SIMPLE_SUBSCRIPT(messages,idx,start_idx,end_idx) \
  (is_simple_subscript((messages),(idx),(start_idx),(end_idx)))

#define IS_SEPARATOR(c) \
  ((c == ';') ||	\
   (c == ')') ||	\
   (c == ',') ||	\
   (c == ':') ||	\
   (c == '}'))

#define LABEL_CHAR(c) ((c >= 'a' && c <= 'z') || \
		       (c >= 'A' && c <= 'Z') ||			\
		       (c >= '0' && c <= '9') || (c == '_') || (c == '$'))

#endif /* _LEX_H */


