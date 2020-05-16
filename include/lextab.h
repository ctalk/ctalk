/* $Id: lextab.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012  Robert Kiesling, rk3314042@gmail.com.
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

/*
 *  Tokenization classes.
 */
#define C_WHITESPACE 0
#define C_NEWLINE 1
#define NUMERIC_CONSTANT 2
#define C_LABEL 3
#define COMMENT_DIV_PAT 4
#define C_ASTERISK 5
#define C_LT 6
#define C_GT 7
#define C_BIT_XOR 8
#define PERIOD_ELLIPSIS 9
#define C_BOOLEAN_OR 10
#define C_AMPERSAND 11
#define C_EQ 12
#define C_BIT_COMP 13
#define C_EXCLAM 14
#define C_PLUS 15
#define C_MINUS 16
#define C_PREPROCESS 17
#define C_LITERAL 18
#define C_LITERAL_CHAR 19
#define C_MODULUS 20
#define LINESPLICE 21
#define MISC 22    /* uses ascii_chr_tok to find the token type */
#define C_EOS 23


/* With ASCII abbreviations. */
const int chr_tok_class[] = {
  C_EOS,  /* 0 - NUL. */
  MISC,  /* 1 - SOH */
  MISC,  /* 2 - STX */
  MISC,  /* 3 - ETX */
  MISC,  /* 4 - EOT */
  MISC,  /* 5 - ENQ */
  MISC,  /* 6 - ACK */
  MISC,  /* 7 - BEL */
  MISC,  /* 8 - BS  */
  C_WHITESPACE,    /* 9 - HT */
  C_NEWLINE,       /* 10 - LF */
  MISC,          /* 11 - VT */
  MISC,          /* 12 - FF */
  C_NEWLINE,      /* 13 - CR */
  MISC,          /* 14 - SO */
  MISC,          /* 15 - SI */
  MISC,          /* 16 - DLE */
  MISC,          /* 17 - DC1 */
  MISC,          /* 18 - DC2 */
  MISC,          /* 19 - DC3 */
  MISC,          /* 20 - DC4 */
  MISC,          /* 21 - NAK */
  MISC,          /* 22 - SYN */
  MISC,          /* 23 - ETB */
  MISC,          /* 24 - CAN */
  MISC,          /* 25 - EM  */
  MISC,          /* 26 - SUB */
  MISC,          /* 27 - ESC */
  MISC,          /* 28 - FS  */
  MISC,          /* 29 - GS  */
  MISC,          /* 30 - RS  */
  MISC,          /* 31 - US  */
  C_WHITESPACE,    /* 32 - space */
  C_EXCLAM,        /* 33 - ! */
  C_LITERAL,       /* 34 - " */
  C_PREPROCESS,    /* 35 - # */
  C_LABEL,         /* 36 - $ */
  C_MODULUS,       /* 37 - % */
  C_AMPERSAND,     /* 38 - & */
  C_LITERAL_CHAR,  /* 39 - ' */
  MISC,          /* 40 - ( */
  MISC,          /* 41 - ) */
  C_ASTERISK,      /* 42 - * */
  C_PLUS,          /* 43 - + */
  MISC,          /* 44 - , */
  C_MINUS,         /* 45 - - */
  PERIOD_ELLIPSIS, /* 46 - . */
  COMMENT_DIV_PAT,  /* 47 - / */
  NUMERIC_CONSTANT,  /* 48 - 0 */
  NUMERIC_CONSTANT,  /* 49 - 1 */
  NUMERIC_CONSTANT,  /* 50 - 2 */
  NUMERIC_CONSTANT,  /* 51 - 3 */
  NUMERIC_CONSTANT,  /* 52 - 4 */
  NUMERIC_CONSTANT,  /* 53 - 5 */
  NUMERIC_CONSTANT,  /* 54 - 6 */
  NUMERIC_CONSTANT,  /* 55 - 7 */
  NUMERIC_CONSTANT,  /* 56 - 8 */
  NUMERIC_CONSTANT,  /* 57 - 9 */
  MISC,              /* 58 - : */
  MISC,              /* 59 - ; */
  C_LT,                /* 60 - < */
  C_EQ,                /* 61 - = */
  C_GT,                /* 62 - > */
  MISC,              /* 63 - ? */
  MISC,              /* 64 - @ */
  C_LABEL,             /* 65 - A */
  C_LABEL,             /* 66 - B */
  C_LABEL,             /* 67 - C */
  C_LABEL,             /* 68 - D */
  C_LABEL,             /* 69 - E */
  C_LABEL,             /* 70 - F */
  C_LABEL,             /* 71 - G */
  C_LABEL,             /* 72 - H */
  C_LABEL,             /* 73 - I */
  C_LABEL,             /* 74 - J */
  C_LABEL,             /* 75 - K */
  C_LABEL,             /* 76 - L */
  C_LABEL,             /* 77 - M */
  C_LABEL,             /* 78 - N */
  C_LABEL,             /* 79 - O */
  C_LABEL,             /* 80 - P */
  C_LABEL,             /* 81 - Q */
  C_LABEL,             /* 82 - R */
  C_LABEL,             /* 83 - S */
  C_LABEL,             /* 84 - T */
  C_LABEL,             /* 85 - U */
  C_LABEL,             /* 86 - V */
  C_LABEL,             /* 87 - W */
  C_LABEL,             /* 88 - X */
  C_LABEL,             /* 89 - Y */
  C_LABEL,             /* 90 - Z */
  MISC,              /* 91 - [ */
  LINESPLICE,        /* 92 - \ */
  MISC,              /* 93 - ] */
  C_BIT_XOR,         /* 94 - ^ */
  C_LABEL,             /* 95 - _ */
  MISC,                /* 96 - ` */
  C_LABEL,             /* 97 - a */
  C_LABEL,             /* 98 - b */
  C_LABEL,             /* 99 - c */
  C_LABEL,             /* 100 - d */
  C_LABEL,             /* 101 - e */
  C_LABEL,             /* 102 - f */
  C_LABEL,             /* 103 - g */
  C_LABEL,             /* 104 - h */
  C_LABEL,             /* 105 - i */
  C_LABEL,             /* 106 - j */
  C_LABEL,             /* 107 - k */
  C_LABEL,             /* 108 - l */
  C_LABEL,             /* 109 - m */
  C_LABEL,             /* 110 - n */
  C_LABEL,             /* 111 - o */
  C_LABEL,             /* 112 - p */
  C_LABEL,             /* 113 - q */
  C_LABEL,             /* 114 - r */
  C_LABEL,             /* 115 - s */
  C_LABEL,             /* 116 - t */
  C_LABEL,             /* 117 - u */
  C_LABEL,             /* 118 - v */
  C_LABEL,             /* 119 - w */
  C_LABEL,             /* 120 - x */
  C_LABEL,             /* 121 - y */
  C_LABEL,             /* 122 - z */
  MISC,              /* 123 - { */
  C_BOOLEAN_OR,        /* 124 - | */
  MISC,              /* 125 - } */
  C_BIT_COMP,              /* 126 - ~ */
   0,  /* 127 */
};

const int ascii_chr_tok[] = {
  0,  /* 0. */
   0,  /* 1. */
   0,  /* 2 */
   0,  /* 3 */
   0,  /* 4 */
   0,  /* 5. */
   0,  /* 6 */
   0,  /* 7 */
   0,  /* 8 */
   WHITESPACE, /* 9 */
   0,          /* 10 */
   WHITESPACE, /* 11 */
   WHITESPACE, /* 12 */
   0,  /* 13 */
   0,  /* 14 */
   0,  /* 15 */
   0,  /* 16 */
   0,  /* 17 */
   0,  /* 18 */
   0,  /* 19 */
   0,  /* 20 */
   0,  /* 21 */
   0,  /* 22 */
   0,  /* 23 */
   0,  /* 24 */
   0,  /* 25 */
   0,  /* 26 */
   0,  /* 27 */
   0,  /* 28 */
   0,  /* 29 */
   0,  /* 30 */
   0,  /* 31 */
   WHITESPACE,  /* 32 */
   0,  /* 33 */
   0,  /* 34 */
   0,  /* 35 */
   0,  /* 36 */
   0,  /* 37 */
   0,  /* 38 */
   0,  /* 39 */
   OPENPAREN,  /* 40 */
   CLOSEPAREN,  /* 41 */
   0,  /* 42 */
   0,  /* 43 */
   ARGSEPARATOR,  /* 44 */
   0,  /* 45 */
   0,  /* 46 */
   0,  /* 47 */
   DIGIT,  /* 48 */
   DIGIT,  /* 49 */
   DIGIT,  /* 50 */
   DIGIT,  /* 51 */
   DIGIT,  /* 52 */
   DIGIT,  /* 53 */
   DIGIT,  /* 54 */
   DIGIT,  /* 55 */
   DIGIT,  /* 56 */
   DIGIT,  /* 57 */
   COLON,  /* 58 */
   SEMICOLON,  /* 59 */
   0,  /* 60 */
   0,  /* 61 */
   0,  /* 62 */
   CONDITIONAL,  /* 63 */
   0,       /* 64 */
   0,  /* 65 */
   0,  /* 66 */
   0,  /* 67 */
   0,  /* 68 */
   0,  /* 69 */
   0,  /* 70 */
   0,  /* 71 */
   0,  /* 72 */
   0,  /* 73 */
   0,  /* 74 */
   0,  /* 75 */
   0,  /* 76 */
   0,  /* 77 */
   0,  /* 78 */
   0,  /* 79 */
   0,  /* 80 */
   0,  /* 81 */
   0,  /* 82 */
   0,  /* 83 */
   0,  /* 84 */
   0,  /* 85 */
   0,  /* 86 */
   0,  /* 87 */
   0,  /* 88 */
   0,  /* 89 */
   0,  /* 90 */
   ARRAYOPEN,  /* 91 */
   0,  /* 92 */
   ARRAYCLOSE,  /* 93 */
   0,  /* 94 */
   0,  /* 95 */
   0,  /* 96 */
   0,  /* 97 */
   0,  /* 98 */
   0,  /* 99 */
   0,  /* 100 */
   0,  /* 101 */
   0,  /* 102 */
   0,  /* 103 */
   0,  /* 104 */
   0,  /* 105 */
   0,  /* 106 */
   0,  /* 107 */
   0,  /* 108 */
   0,  /* 109 */
   0,  /* 110 */
   0,  /* 111 */
   0,  /* 112 */
   0,  /* 113 */
   0,  /* 114 */
   0,  /* 115 */
   0,  /* 116 */
   0,  /* 117 */
   0,  /* 118 */
   0,  /* 119 */
   0,  /* 120 */
   0,  /* 121 */
   0,  /* 122 */
   OPENBLOCK,  /* 123 */
   0,  /* 124 */
   CLOSEBLOCK,  /* 125 */
   BIT_COMP,  /* 126 */
   0,  /* 127 */
};
