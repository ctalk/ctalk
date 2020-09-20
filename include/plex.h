/* $Id: plex.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of ctalk.
  Copyright © 2005-2007, 2016 Robert Kiesling, rkiesling@users.sourceforge.net
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

#ifndef _PLEX_H
#define _PLEX_H

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

#define BIT_AND               62
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

#define UINTEGER              84
#define ULONG                 85

#define MATCH                 86
#define NOMATCH               87

#define PATTERN               88

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
   (tok == COLON))

#define IS_C_UNARY_OP(tok) \
   ((tok == INCREMENT) || \
   (tok == DECREMENT) || \
   (tok == BIT_COMP) || \
   (tok == LOG_NEG) || \
   (tok == SIZEOF))

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
   (tok == MACRO_CONCAT))

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

/* Characters that cannot end an operand. Used to determine if a 
   following + or - is a unary or binary op.  Note that ()[]{} are 
   not included in this class. */
#define NOEVAL_CHAR_SUFFIX "=><+-*/&~!^|.#;,?:"

#define IS_SEPARATOR(c) \
  ((c == ';') ||	\
   (c == ')') ||	\
   (c == ',') ||	\
   (c == ':') ||	\
   (c == '}'))

#ifndef IS_C_LABEL
/* Also defined in ctalk.h. */
#define IS_C_LABEL(s) (((*s) >= 'a' && (*s) <= 'z') || \
	((*s) >= 'A' && (*s) <= 'Z') || ((*s) == '_'))
#endif

#define IS_C_LABEL_CHAR(c) (((c) >= 'a' && (c) <= 'z') || \
	((c) >= 'A' && (c) <= 'Z') || ((c) == '_'))

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

#endif  /* __PLEX_H */

