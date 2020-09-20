/* $Id: ctypes-sunos.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */
/*
  This file is part of Ctalk.
  Copyright © 2005-2011 Robert Kiesling, rk3314042@gmail.com.
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

/* Make sure we can parse all the variables in the standard include files. */

#warning "assert.h."
#include <assert.h>
/* #warning "complex.h is not implemented." */
#warning "ctype.h."
#include <ctype.h>
#warning "errno.h."
#include <errno.h>
/* #warning "fenv.h is not implemented." */
#warning "float.h."
#include <float.h>
#warning "inttypes.h."
#include <inttypes.h>
#warning "iso646.h."
#include <iso646.h>
#warning "limits.h."
#include <limits.h>
#warning "locale.h."
#include <locale.h>
#warning "math.h."
#include <math.h>
#warning "setjmp.h."
#include <setjmp.h>
#warning "signal.h."
#include <signal.h>
#warning "stdarg.h."
#include <stdarg.h>
#warning "stdbool.h."
#warning "stddef.h."
#include <stddef.h> 
/* #warning "stdint.h is not implemented." */
#warning "stdio.h"
#include <stdio.h>
#warning "stdlib.h."
#include <stdlib.h> 
#warning "string.h."
#include <string.h> 
/* #warning "tgmath.h is not implemented." */
#warning "time.h."
#include <time.h>
#warning "wctype.h."
#include <wctype.h>

/* Test C data type declarations. */


/* Test extern storage class. */

extern void *_extrn_ptr_void;
/* - */
extern char _extrn_char;
/* - */
extern signed char _extrn_signed_char;
/* - */
extern unsigned char _extrn_unsigned_char;
/* - */
extern short _extrn_short;
extern signed short _extrn_signed_short;
extern signed short int _extrn_signed_short_int;
/* - */
extern unsigned short _extrn_unsigned_short;
extern unsigned short int _extrn_unsigned_short_int;
/* - */
extern int _extrn_int;
extern signed _extrn_signed;
/* - */
extern unsigned _extrn_unsigned;
extern unsigned int _extrn_unsigned_int;
/* - */
extern long _extrn_long;
extern signed long _extrn_signed_long;
extern signed long int _extrn_signed_long_int;
/* - */
extern unsigned long _extrn_unsigned_long;
extern unsigned long int _extrn_unsigned_long_int;
/* - */
extern long long extrn_long_long;
extern signed long long _extrn_signed_long_long;
extern long long int _extrn_long_long_int;
extern signed long long int _extrn_signed_long_long_int;
/* - */
extern unsigned long long _extrn_unsigned_long_long;
extern unsigned long long int _extrn_unsigned_long_long_int;
/* - */
extern float _extrn_float;
/* - */
extern double _extrn_double;
/* - */
extern long double _extrn_long_double;
/* - */
#if (__GNUC__ >= 3)
extern float _Complex _extrn_float_Complex;
/* - */
extern double _Complex _extrn_double_Complex;
/* - */
extern long double _Complex _extrn_long_double_Complex;
#endif

/* Test static storage class. */

static void *_stc_ptr_void;
/* - */
static char _stc_char;
/* - */
static signed char _stc_signed_char;
/* - */
static unsigned char _stc_unsigned_char;
/* - */
static short _stc_short;
static signed short _stc_signed_short;
static signed short int _stc_signed_short_int;
/* - */
static unsigned short _stc_unsigned_short;
static unsigned short int _stc_unsigned_short_int;
/* - */
static int _stc_int;
static signed _stc_signed;
/* - */
static unsigned _stc_unsigned;
static unsigned int _stc_unsigned_int;
/* - */
static long _stc_long;
static signed long _stc_signed_long;
static signed long int _stc_signed_long_int;
/* - */
static unsigned long _stc_unsigned_long;
static unsigned long int _stc_unsigned_long_int;
/* - */
static long long stc_long_long;
static signed long long _stc_signed_long_long;
static long long int _stc_long_long_int;
static signed long long int _stc_signed_long_long_int;
/* - */
static unsigned long long _stc_unsigned_long_long;
static unsigned long long int _stc_unsigned_long_long_int;
/* - */
static float _stc_float;
/* - */
static double _stc_double;
/* - */
static long double _stc_long_double;
/* - */
#if (__GNUC__ >= 3)
static float _Complex _stc_float_Complex;
/* - */
static double _Complex _stc_double_Complex;
/* - */
static long double _Complex _stc_long_double_Complex;
#endif

static struct s_st {
  int member_int_i;
  long member_long_int_l;
} s_st;

int main (int argc, char **argv) {

  int a = 1;
  int b = 2;
  int c = 3;
  int d = 4;
  int e = 5;

  double d1 = 1.0;
  double d2 = 2.0;
  double d3 = 3.0;
  double d4 = 4.0;
  double d5 = 5.0;

  long long int all = 1ll;
  long long int bll = 2ll;
  long long int cll = 3ll;
  long long int dll = 4ll;
  long long int ell = 5ll;

  Integer new intA;
  Integer new intB;
  Integer new intC;
  Integer new intD;
  Integer new intE;
  Integer new intResult;

  LongInteger new longIntA;
  LongInteger new longIntB;
  LongInteger new longIntC;
  LongInteger new longIntD;
  LongInteger new longIntE;
  LongInteger new longIntResult;

  Float new floatResult;

  String new cmdLine;

  intA = 1;
  intB = 2;
  intC = 3;
  intD = 4;
  intE = 5;

  intResult = 1 + 2 + 3;
  printf ("1 + 2 + 3 evaluates to %d.\n", intResult value);
  intResult = (1 + 2) + 3;
  printf ("(1 + 2) + 3 evaluates to %d.\n", intResult value);
  intResult = ((1 + 2) + 3) + 4;
  printf ("((1 + 2) + 3) + 4 evaluates to %d.\n", intResult value);
  intResult = (((1 + 2) + 3) + 4) + 5;
  printf ("(((1 + 2) + 3) + 4) + 5 evaluates to %d.\n", intResult value);
  intResult = ((1 + 1) + 2) + 3;
  printf ("((1 + 1) + 2) + 3 evaluates to %d.\n", intResult value);
  intResult = (1 + (2 + 2)) + 3;
  printf ("(1 + (2 + 2)) + 3 evaluates to %d.\n", intResult value);
  intResult = (1 + 2) + (3 + 3);
  printf ("(1 + 2) + (3 + 3) evaluates to %d.\n", intResult value);
  intResult = (1 + 2) + (3 + 3) + (4 + 4);
  printf ("(1 + 2) + (3 + 3) + (4 + 4) evaluates to %d.\n", intResult value);

  intResult = 1 - 2 - 3;
  printf ("1 - 2 - 3 evaluates to %d.\n", intResult value);
  intResult = (1 - 2) - 3;
  printf ("(1 - 2) - 3 evaluates to %d.\n", intResult value);
  intResult = ((1 - 2) - 3) - 4;
  printf ("((1 - 2) - 3) - 4 evaluates to %d.\n", intResult value);
  intResult = (((1 - 2) - 3) - 4) - 5;
  printf ("(((1 - 2) - 3) - 4) - 5 evaluates to %d.\n", intResult value);
  intResult = ((1 - 1) - 2) - 3;
  printf ("((1 - 1) - 2) - 3 evaluates to %d.\n", intResult value);
  intResult = (1 - (2 - 2)) - 3;
  printf ("(1 - (2 - 2)) - 3 evaluates to %d.\n", intResult value);
  intResult = (1 - 2) - (3 - 3);
  printf ("(1 - 2) - (3 - 3) evaluates to %d.\n", intResult value);
  intResult = (1 - 2) - (3 - 3) - (4 - 4);
  printf ("(1 - 2) - (3 - 3) - (4 - 4) evaluates to %d.\n", intResult value);

  intResult = 1 * 2 * 3;
  printf ("1 * 2 * 3 evaluates to %d.\n", intResult value);
  intResult = (1 * 2) * 3;
  printf ("(1 * 2) * 3 evaluates to %d.\n", intResult value);
  intResult = ((1 * 2) * 3) * 4;
  printf ("((1 * 2) * 3) * 4 evaluates to %d.\n", intResult value);
  intResult = (((1 * 2) * 3) * 4) * 5;
  printf ("(((1 * 2) * 3) * 4) * 5 evaluates to %d.\n", intResult value);
  intResult = ((1 * 1) * 2) * 3;
  printf ("((1 * 1) * 2) * 3 evaluates to %d.\n", intResult value);
  intResult = (1 * (2 * 2)) * 3;
  printf ("(1 * (2 * 2)) * 3 evaluates to %d.\n", intResult value);
  intResult = (1 * 2) * (3 * 3);
  printf ("(1 * 2) * (3 * 3) evaluates to %d.\n", intResult value);
  intResult = (1 * 2) * (3 * 3) * (4 * 4);
  printf ("(1 * 2) * (3 * 3) * (4 * 4) evaluates to %d.\n", intResult value);

  floatResult = 1.0 / 2.0 / 3.0;
  printf ("1.0 / 2.0 / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / 2.0) / 3.0;
  printf ("(1.0 / 2.0) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = ((1.0 / 2.0) / 3.0) / 4.0;
  printf ("((1.0 / 2.0) / 3.0) / 4.0 evaluates to %f.\n", floatResult value);
  floatResult = (((1.0 / 2.0) / 3.0) / 4.0) / 5.0;
  printf ("(((1.0 / 2.0) / 3.0) / 4.0) / 5.0 evaluates to %f.\n", 
	  floatResult value);
  floatResult = ((1.0 / 1.0) / 2.0) / 3.0;
  printf ("((1.0 / 1.0) / 2.0) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / (2.0 / 2.0)) / 3.0;
  printf ("(1.0 / (2.0 / 2.0)) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / 2.0) / (3.0 / 3.0);
  printf ("(1.0 / 2.0) / (3.0 / 3.0) evaluates to %f.\n", floatResult value);
  intResult = (1 / 2) / (3 / 3) / (4 / 4);
  printf ("(1 / 2) / (3 / 3) / (4 / 4) evaluates to %d.\n", intResult value);

  longIntResult = 1ll + 2ll + 3ll;
  printf ("1ll + 2ll + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll + 2ll) + 3ll;
  printf ("(1ll + 2ll) + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll + 2ll) + 3ll) + 4ll;
  printf ("((1ll + 2ll) + 3ll) + 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll + 2ll) + 3ll) + 4ll) + 5;
  printf ("(((1ll + 2ll) + 3ll) + 4ll) + 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll + 1ll) + 2ll) + 3ll;
  printf ("((1ll + 1ll) + 2ll) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + (2ll + 2ll)) + 3ll;
  printf ("(1ll + (2ll + 2ll)) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + 2ll) + (3ll + 3ll);
  printf ("(1ll + 2ll) + (3ll + 3ll) evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + 2ll) + (3ll + 3ll) + (4ll + 4ll);
  printf ("(1ll + 2ll) + (3ll + 3ll) + (4ll + 4ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll - 2ll - 3ll;
  printf ("1ll - 2ll - 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll - 2ll) - 3ll;
  printf ("(1ll - 2ll) - 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll - 2ll) - 3ll) - 4ll;
  printf ("((1ll - 2ll) - 3ll) - 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll - 2ll) - 3ll) - 4ll) - 5ll;
  printf ("(((1ll - 2ll) - 3ll) - 4ll) - 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll - 1ll) - 2ll) - 3ll;
  printf ("((1ll - 1ll) - 2ll) - 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll - (2ll - 2ll)) - 3ll;
  printf ("(1ll - (2ll - 2ll)) - 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll - 2ll) - (3ll - 3ll);
  printf ("(1ll - 2ll) - (3ll - 3ll) evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll - 2ll) - (3ll - 3ll) - (4ll - 4ll);
  printf ("(1ll - 2ll) - (3ll - 3ll) - (4ll - 4ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll * 2ll * 3ll;
  printf ("1ll * 2ll * 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll * 2ll) * 3ll;
  printf ("(1ll * 2ll) * 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll * 2ll) * 3ll) * 4ll;
  printf ("((1ll * 2ll) * 3ll) * 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll * 2ll) * 3ll) * 4ll) * 5ll;
  printf ("(((1ll * 2ll) * 3ll) * 4ll) * 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll * 1ll) * 2ll) * 3ll;
  printf ("((1ll * 1ll) * 2ll) * 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll * (2ll * 2ll)) * 3ll;
  printf ("(1ll * (2ll * 2ll)) * 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll * 2ll) * (3ll * 3ll);
  printf ("(1ll * 2ll) * (3ll * 3ll) evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll * 2ll) * (3ll * 3ll) * (4ll * 4ll);
  printf ("(1ll * 2ll) * (3ll * 3ll) * (4ll * 4ll) evaluates to %lld.\n", 
	  longIntResult value);

  printf ("int a = 1;\n");
  printf ("int b = 2;\n");
  printf ("int c = 3;\n");
  printf ("int d = 4;\n");
  printf ("int e = 5;\n");
  printf ("double d1 = 1.0;\n");
  printf ("double d2 = 2.0;\n");
  printf ("double d3 = 3.0;\n");
  printf ("double d4 = 4.0;\n");
  printf ("double d5 = 5.0;\n");
  printf ("long long int all = 1ll;\n");
  printf ("long long int bll = 2ll;\n");
  printf ("long long int cll = 3ll;\n");
  printf ("long long int dll = 4ll;\n");
  printf ("long long int ell = 5ll;\n");

  intResult = a + 2 + 3;
  printf ("a + 2 + 3 evaluates to %d.\n", intResult value);
  intResult = (a + 2) + 3;
  printf ("(a + 2) + 3 evaluates to %d.\n", intResult value);
  intResult = ((a + 2) + 3) + 4;
  printf ("((a + 2) + 3) + 4 evaluates to %d.\n", intResult value);
  intResult = (((a + 2) + 3) + 4) + 5;
  printf ("(((a + 2) + 3) + 4) + 5 evaluates to %d.\n", intResult value);
  intResult = ((a + a) + 2) + 3;
  printf ("((a + a) + 2) + 3 evaluates to %d.\n", intResult value);
  intResult = (a + (2 + 2)) + 3;
  printf ("(a + (2 + 2)) + 3 evaluates to %d.\n", intResult value);
  intResult = (a + 2) + (3 + 3);
  printf ("(a + 2) + (3 + 3) evaluates to %d.\n", intResult value);

  intResult = 1 + b + 3;
  printf ("1 + b + 3 evaluates to %d.\n", intResult value);
  intResult = (1 + b) + 3;
  printf ("(1 + b) + 3 evaluates to %d.\n", intResult value);
  intResult = ((1 + b) + 3) + 4;
  printf ("((1 + b) + 3) + 4 evaluates to %d.\n", intResult value);
  intResult = (((1 + b) + 3) + 4) + 5;
  printf ("(((1 + b) + 3) + 4) + 5 evaluates to %d.\n", intResult value);
  intResult = ((1 + 1) + b) + 3;
  printf ("((1 + 1) + b) + 3 evaluates to %d.\n", intResult value);
  intResult = (1 + (b + b)) + 3;
  printf ("(1 + (b + b)) + 3 evaluates to %d.\n", intResult value);
  intResult = (1 + b) + (3 + 3);
  printf ("(1 + b) + (3 + 3) evaluates to %d.\n", intResult value);

  intResult = 1 + 2 + c;
  printf ("1 + 2 + c evaluates to %d.\n", intResult value);
  intResult = (1 + 2) + c;
  printf ("(1 + 2) + c evaluates to %d.\n", intResult value);
  intResult = ((1 + 2) + c) + 4;
  printf ("((1 + 2) + c) + 4 evaluates to %d.\n", intResult value);
  intResult = (((1 + 2) + c) + 4) + 5;
  printf ("(((1 + 2) + c) + 4) + 5 evaluates to %d.\n", intResult value);
  intResult = ((1 + 1) + 2) + c;
  printf ("((1 + 1) + 2) + c evaluates to %d.\n", intResult value);
  intResult = (1 + (2 + 2)) + c;
  printf ("(1 + (2 + 2)) + c evaluates to %d.\n", intResult value);
  intResult = (1 + 2) + (c + c);
  printf ("(1 + 2) + (c + c) evaluates to %d.\n", intResult value);

  intResult = ((1 + 2) + 3) + d;
  printf ("((1 + 2) + 3) + d evaluates to %d.\n", intResult value);
  intResult = (((1 + 2) + 3) + d) + 5;
  printf ("(((1 + 2) + 3) + d) + 5 evaluates to %d.\n", intResult value);

  intResult = (((1 + 2) + 3) + 4) + e;
  printf ("(((1 + 2) + 3) + 4) + e evaluates to %d.\n", intResult value);

  intResult = a - 2 - 3;
  printf ("a - 2 - 3 evaluates to %d.\n", intResult value);
  intResult = (a - 2) - 3;
  printf ("(a - 2) - 3 evaluates to %d.\n", intResult value);
  intResult = ((a - 2) - 3) - 4;
  printf ("((a - 2) - 3) - 4 evaluates to %d.\n", intResult value);
  intResult = (((a - 2) - 3) - 4) - 5;
  printf ("(((a - 2) - 3) - 4) - 5 evaluates to %d.\n", intResult value);
  intResult = ((a - a) - 2) - 3;
  printf ("((a - a) - 2) - 3 evaluates to %d.\n", intResult value);
  intResult = (a - (2 - 2)) - 3;
  printf ("(a - (2 - 2)) - 3 evaluates to %d.\n", intResult value);
  intResult = (a - 2) - (3 - 3);
  printf ("(a - 2) - (3 - 3) evaluates to %d.\n", intResult value);

  intResult = 1 - b - 3;
  printf ("1 - b - 3 evaluates to %d.\n", intResult value);
  intResult = (1 - b) - 3;
  printf ("(1 - b) - 3 evaluates to %d.\n", intResult value);
  intResult = ((1 - b) - 3) - 4;
  printf ("((1 - b) - 3) - 4 evaluates to %d.\n", intResult value);
  intResult = (((1 - b) - 3) - 4) - 5;
  printf ("(((1 - b) - 3) - 4) - 5 evaluates to %d.\n", intResult value);
  intResult = ((1 - 1) - b) - 3;
  printf ("((1 - 1) - b) - 3 evaluates to %d.\n", intResult value);
  intResult = (1 - (b - b)) - 3;
  printf ("(1 - (b - b)) - 3 evaluates to %d.\n", intResult value);
  intResult = (1 - b) - (3 - 3);
  printf ("(1 - b) - (3 - 3) evaluates to %d.\n", intResult value);

  intResult = 1 - 2 - c;
  printf ("1 - 2 - c evaluates to %d.\n", intResult value);
  intResult = (1 - 2) - c;
  printf ("(1 - 2) - c evaluates to %d.\n", intResult value);
  intResult = ((1 - 2) - c) - 4;
  printf ("((1 - 2) - c) - 4 evaluates to %d.\n", intResult value);
  intResult = (((1 - 2) - c) - 4) - 5;
  printf ("(((1 - 2) - c) - 4) - 5 evaluates to %d.\n", intResult value);
  intResult = ((1 - 1) - 2) - c;
  printf ("((1 - 1) - 2) - c evaluates to %d.\n", intResult value);
  intResult = (1 - (2 - 2)) - c;
  printf ("(1 - (2 - 2)) - c evaluates to %d.\n", intResult value);
  intResult = (1 - 2) - (3 - c);
  printf ("(1 - 2) - (c - c) evaluates to %d.\n", intResult value);

  intResult = ((1 - 2) - 3) - d;
  printf ("((1 - 2) - 3) - d evaluates to %d.\n", intResult value);
  intResult = (((1 - 2) - 3) - d) - 5;
  printf ("(((1 - 2) - 3) - d) - 5 evaluates to %d.\n", intResult value);

  intResult = (((1 - 2) - 3) - 4) - e;
  printf ("(((1 - 2) - 3) - 4) - e evaluates to %d.\n", intResult value);

  intResult = a * 2 * 3;
  printf ("a * 2 * 3 evaluates to %d.\n", intResult value);
  intResult = (a * 2) * 3;
  printf ("(a * 2) * 3 evaluates to %d.\n", intResult value);
  intResult = ((a * 2) * 3) * 4;
  printf ("((a * 2) * 3) * 4 evaluates to %d.\n", intResult value);
  intResult = (((a * 2) * 3) * 4) * 5;
  printf ("(((a * 2) * 3) * 4) * 5 evaluates to %d.\n", intResult value);
  intResult = ((a * a) * 2) * 3;
  printf ("((a * a) * 2) * 3 evaluates to %d.\n", intResult value);
  intResult = (a * (2 * 2)) * 3;
  printf ("(a * (2 * 2)) * 3 evaluates to %d.\n", intResult value);
  intResult = (a * 2) * (3 * 3);
  printf ("(a * 2) * (3 * 3) evaluates to %d.\n", intResult value);

  intResult = 1 * b * 3;
  printf ("1 * b * 3 evaluates to %d.\n", intResult value);
  intResult = (1 * b) * 3;
  printf ("(1 * b) * 3 evaluates to %d.\n", intResult value);
  intResult = ((1 * b) * 3) * 4;
  printf ("((1 * b) * 3) * 4 evaluates to %d.\n", intResult value);
  intResult = (((1 * b) * 3) * 4) * 5;
  printf ("(((1 * b) * 3) * 4) * 5 evaluates to %d.\n", intResult value);
  intResult = ((1 * 1) * b) * 3;
  printf ("((1 * 1) * b) * 3 evaluates to %d.\n", intResult value);
  intResult = (1 * (b * b)) * 3;
  printf ("(1 * (b * b)) * 3 evaluates to %d.\n", intResult value);
  intResult = (1 * b) * (3 * 3);
  printf ("(1 * b) * (3 * 3) evaluates to %d.\n", intResult value);

  intResult = 1 * 2 * c;
  printf ("1 * 2 * c evaluates to %d.\n", intResult value);
  intResult = (1 * 2) * c;
  printf ("(1 * 2) * c evaluates to %d.\n", intResult value);
  intResult = ((1 * 2) * c) * 4;
  printf ("((1 * 2) * c) * 4 evaluates to %d.\n", intResult value);
  intResult = (((1 * 2) * c) * 4) * 5;
  printf ("(((1 * 2) * c) * 4) * 5 evaluates to %d.\n", intResult value);
  intResult = ((1 * 1) * 2) * c;
  printf ("((1 * 1) * 2) * c evaluates to %d.\n", intResult value);
  intResult = (1 * (2 * 2)) * c;
  printf ("(1 * (2 * 2)) * c evaluates to %d.\n", intResult value);
  intResult = (1 * 2) * (3 * c);
  printf ("(1 * 2) * (c * c) evaluates to %d.\n", intResult value);

  intResult = ((1 * 2) * 3) * d;
  printf ("((1 * 2) * 3) * d evaluates to %d.\n", intResult value);
  intResult = (((1 * 2) * 3) * d) * 5;
  printf ("(((1 * 2) * 3) * d) * 5 evaluates to %d.\n", intResult value);

  intResult = (((1 * 2) * 3) * 4) * e;
  printf ("(((1 * 2) * 3) * 4) * e evaluates to %d.\n", intResult value);

  longIntResult = all + 2ll + 3ll;
  printf ("all + 2ll + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (all + 2ll) + 3ll;
  printf ("(all + 2ll) + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((all + 2ll) + 3ll) + 4ll;
  printf ("((all + 2ll) + 3ll) + 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((all + 2ll) + 3ll) + 4ll) + 5ll;
  printf ("(((all + 2ll) + 3ll) + 4ll) + 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((all + all) + 2ll) + 3ll;
  printf ("((all + all) + 2ll) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (all + (2ll + 2ll)) + 3ll;
  printf ("(all + (2ll + 2ll)) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (all + 2ll) + (3ll + 3ll);
  printf ("(all + 2ll) + (3ll + 3ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll + bll + 3ll;
  printf ("1ll + bll + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll + bll) + 3ll;
  printf ("(1ll + bll) + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll + bll) + 3ll) + 4ll;
  printf ("((1ll + bll) + 3ll) + 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll + bll) + 3ll) + 4ll) + 5ll;
  printf ("(((1ll + bll) + 3ll) + 4ll) + 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll + 1ll) + bll) + 3ll;
  printf ("((1ll + 1ll) + bll) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + (bll + bll)) + 3ll;
  printf ("(1ll + (bll + bll)) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + bll) + (3ll + 3ll);
  printf ("(1ll + bll) + (3ll + 3ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll + 2ll + cll;
  printf ("1ll + 2ll + cll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll + 2ll) + cll;
  printf ("(1ll + 2ll) + cll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll + 2ll) + cll) + 4ll;
  printf ("((1ll + 2ll) + cll) + 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll + 2ll) + cll) + 4ll) + 5ll;
  printf ("(((1ll + 2ll) + cll) + 4ll) + 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll + 1ll) + 2ll) + cll;
  printf ("((1ll + 1ll) + 2ll) + cll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + (2ll + 2ll)) + cll;
  printf ("(1ll + (2ll + 2ll)) + cll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + 2ll) + (cll + cll);
  printf ("(1ll + 2ll) + (cll + cll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = ((1ll + 2ll) + 3ll) + dll;
  printf ("((1ll + 2ll) + 3ll) + dll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll + 2ll) + 3ll) + dll) + 5ll;
  printf ("(((1ll + 2ll) + 3ll) + dll) + 5ll evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = (((1ll + 2ll) + 3ll) + 4ll) + ell;
  printf ("(((1ll + 2ll) + 3ll) + 4ll) + ell evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = all - 2ll - 3ll;
  printf ("all - 2ll - 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (all - 2ll) - 3ll;
  printf ("(all - 2ll) - 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((all - 2ll) - 3ll) - 4ll;
  printf ("((all - 2ll) - 3ll) - 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((all - 2ll) - 3ll) - 4ll) - 5ll;
  printf ("(((all - 2ll) - 3ll) - 4ll) - 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((all - all) - 2ll) - 3ll;
  printf ("((all - all) - 2ll) - 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (all - (2ll - 2ll)) - 3ll;
  printf ("(all - (2ll - 2ll)) - 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (all - 2ll) - (3ll - 3ll);
  printf ("(all - 2ll) - (3ll - 3ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll - bll - 3ll;
  printf ("1ll - bll - 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll - bll) - 3ll;
  printf ("(1ll - bll) - 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll - bll) - 3ll) - 4ll;
  printf ("((1ll - bll) - 3ll) - 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll - bll) - 3ll) - 4ll) - 5ll;
  printf ("(((1ll - bll) - 3ll) - 4ll) - 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll - 1ll) - bll) - 3ll;
  printf ("((1ll - 1ll) - bll) - 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll - (bll - bll)) - 3ll;
  printf ("(1ll - (bll - bll)) - 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll - bll) - (3ll - 3ll);
  printf ("(1ll - bll) - (3ll - 3ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll - 2ll - cll;
  printf ("1ll - 2ll - cll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll - 2ll) - cll;
  printf ("(1ll - 2ll) - cll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll - 2ll) - cll) - 4ll;
  printf ("((1ll - 2ll) - cll) - 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll - 2ll) - cll) - 4ll) - 5ll;
  printf ("(((1ll - 2ll) - cll) - 4ll) - 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll - 1ll) - 2ll) - cll;
  printf ("((1ll - 1ll) - 2ll) - cll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll - (2ll - 2ll)) - cll;
  printf ("(1ll - (2ll - 2ll)) - cll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll - 2ll) - (3ll - cll);
  printf ("(1ll - 2ll) - (cll - cll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = ((1ll - 2ll) - 3ll) - dll;
  printf ("((1ll - 2ll) - 3ll) - dll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll - 2ll) - 3ll) - dll) - 5ll;
  printf ("(((1ll - 2ll) - 3ll) - dll) - 5ll evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = (((1ll - 2ll) - 3ll) - 4ll) - ell;
  printf ("(((1ll - 2ll) - 3ll) - 4ll) - ell evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = all * 2ll * 3ll;
  printf ("all * 2ll * 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (all * 2ll) * 3ll;
  printf ("(all * 2ll) * 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((all * 2ll) * 3ll) * 4ll;
  printf ("((all * 2ll) * 3ll) * 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((all * 2ll) * 3ll) * 4ll) * 5ll;
  printf ("(((all * 2ll) * 3ll) * 4ll) * 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((all * all) * 2ll) * 3ll;
  printf ("((all * all) * 2ll) * 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (all * (2ll * 2ll)) * 3ll;
  printf ("(all * (2ll * 2ll)) * 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (all * 2ll) * (3ll * 3ll);
  printf ("(all * 2ll) * (3ll * 3ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll * bll * 3ll;
  printf ("1ll * bll * 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll * bll) * 3ll;
  printf ("(1ll * bll) * 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll * bll) * 3ll) * 4ll;
  printf ("((1ll * bll) * 3ll) * 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll * bll) * 3ll) * 4ll) * 5ll;
  printf ("(((1ll * bll) * 3ll) * 4ll) * 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll * 1ll) * bll) * 3ll;
  printf ("((1ll * 1ll) * bll) * 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll * (bll * bll)) * 3ll;
  printf ("(1ll * (bll * bll)) * 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll * bll) * (3ll * 3ll);
  printf ("(1ll * bll) * (3ll * 3ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll * 2ll * c;
  printf ("1ll * 2ll * cll evaluates to %lld.\n", longIntResult value);
  longIntResult = (1ll * 2ll) * c;
  printf ("(1ll * 2ll) * cll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll * 2ll) * cll) * 4ll;
  printf ("((1ll * 2ll) * cll) * 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll * 2ll) * cll) * 4ll) * 5ll;
  printf ("(((1ll * 2ll) * cll) * 4ll) * 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll * 1ll) * 2ll) * c;
  printf ("((1ll * 1ll) * 2ll) * cll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll * (2ll * 2ll)) * c;
  printf ("(1ll * (2ll * 2ll)) * cll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll * 2ll) * (3ll * cll);
  printf ("(1ll * 2ll) * (cll * cll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = ((1ll * 2ll) * 3ll) * d;
  printf ("((1ll * 2ll) * 3ll) * d evaluates to %lld.\n", longIntResult value);
  longIntResult = (((1ll * 2ll) * 3ll) * d) * 5ll;
  printf ("(((1ll * 2ll) * 3ll) * d) * 5ll evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = (((1ll * 2ll) * 3ll) * 4ll) * ell;
  printf ("(((1ll * 2ll) * 3ll) * 4ll) * ell evaluates to %lld.\n", 
	  longIntResult value);

  floatResult = d1 / 2.0 / 3.0;
  printf ("d1 / 2.0 / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (d1 / 2.0) / 3.0;
  printf ("(d1 / 2.0) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = ((d1 / 2.0) / 3.0) / 4.0;
  printf ("((d1 / 2.0) / 3.0) / 4.0 evaluates to %f.\n", floatResult value);
  floatResult = (((d1 / 2.0) / 3.0) / 4.0) / 5.0;
  printf ("(((d1 / 2.0) / 3.0) / 4.0) / 5.0 evaluates to %f.\n", 
	  floatResult value);
  floatResult = ((d1 / 1.0) / 2.0) / 3.0;
  printf ("((d1 / 1.0) / 2.0) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (d1 / (2.0 / 2.0)) / 3.0;
  printf ("(d1 / (2.0 / 2.0)) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (d1 / 2.0) / (3.0 / 3.0);
  printf ("(d1 / 2.0) / (3.0 / 3.0) evaluates to %f.\n", floatResult value);

  floatResult = 1.0 / d2 / 3.0;
  printf ("1.0 / d2 / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / d2) / 3.0;
  printf ("(1.0 / d2) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = ((1.0 / d2) / 3.0) / 4.0;
  printf ("((1.0 / d2) / 3.0) / 4.0 evaluates to %f.\n", floatResult value);
  floatResult = (((1.0 / d2) / 3.0) / 4.0) / 5.0;
  printf ("(((1.0 / d2) / 3.0) / 4.0) / 5.0 evaluates to %f.\n", 
	  floatResult value);
  floatResult = ((1.0 / 1.0) / d2) / 3.0;
  printf ("((1.0 / 1.0) / d2) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / (d2 / d2)) / 3.0;
  printf ("(1.0 / (d2 / d2)) / 3.0 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / d2) / (3.0 / 3.0);
  printf ("(1.0 / d2) / (3.0 / 3.0) evaluates to %f.\n", floatResult value);

  floatResult = 1.0 / 2.0 / d3;
  printf ("1.0 / 2.0 / d3 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / 2.0) / d3;
  printf ("(1.0 / 2.0) / d3 evaluates to %f.\n", floatResult value);
  floatResult = ((1.0 / 2.0) / d3) / 4.0;
  printf ("((1.0 / 2.0) / d3) / 4.0 evaluates to %f.\n", floatResult value);
  floatResult = (((1.0 / 2.0) / d3) / 4.0) / 5.0;
  printf ("(((1.0 / 2.0) / d3) / 4.0) / 5.0 evaluates to %f.\n", 
	  floatResult value);
  floatResult = ((1.0 / 1.0) / 2.0) / d3;
  printf ("((1.0 / 1.0) / 2.0) / d3 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / (2.0 / 2.0)) / d3;
  printf ("(1.0 / (2.0 / 2.0)) / d3 evaluates to %f.\n", floatResult value);
  floatResult = (1.0 / 2.0) / (d3 / d3);
  printf ("(1.0 / 2.0) / (d3 / d3) evaluates to %f.\n", floatResult value);

  floatResult = ((1.0 / 2.0) / 3.0) / d4;
  printf ("((1.0 / 2.0) / 3.0) / d4 evaluates to %f.\n", floatResult value);
  floatResult = (((1.0 / 2.0) / 3.0) / d4) / 5.0;
  printf ("(((1.0 / 2.0) / 3.0) / d4) / 5.0 evaluates to %f.\n", 
	  floatResult value);

  floatResult = (((1.0 / 2.0) / 3.0) / 4.0) / d5;
  printf ("(((1.0 / 2.0) / 3.0) / 4.0) / d5 evaluates to %f.\n", 
	  floatResult value);

  printf ("intA = 1;\n");
  printf ("intB = 2;\n");
  printf ("intC = 3;\n");
  printf ("intD = 4;\n");
  printf ("intE = 5;\n");

  intResult = intA + 2 + 3;
  printf ("intA + 2 + 3 evaluates to %d.\n", intResult value);
  intResult = (intA + 2) + 3;
  printf ("(intA + 2) + 3 evaluates to %d.\n", intResult value);
  intResult = ((intA + 2) + 3) + 4;
  printf ("((intA + 2) + 3) + 4 evaluates to %d.\n", intResult value);
  intResult = (((intA + 2) + 3) + 4) + 5;
  printf ("(((intA + 2) + 3) + 4) + 5 evaluates to %d.\n", intResult value);
  intResult = ((intA + intA) + 2) + 3;
  printf ("((intA + intA) + 2) + 3 evaluates to %d.\n", intResult value);
  intResult = (intA + (2 + 2)) + 3;
  printf ("(intA + (2 + 2)) + 3 evaluates to %d.\n", intResult value);
  intResult = (intA + 2) + (3 + 3);
  printf ("(intA + 2) + (3 + 3) evaluates to %d.\n", intResult value);

  intResult = 1 + intB + 3;
  printf ("1 + intB + 3 evaluates to %d.\n", intResult value);
  intResult = ((1 + intB) + 3) + 4;
  printf ("((1 + intB) + 3) + 4 evaluates to %d.\n", intResult value);
  intResult = (((1 + intB) + 3) + 4) + 5;
  printf ("(((1 + intB) + 3) + 4) + 5 evaluates to %d.\n", intResult value);
  intResult = ((1 + 1) + intB) + 3;
  printf ("((1 + 1) + intB) + 3 evaluates to %d.\n", intResult value);
  intResult = (1 + (intB + intB)) + 3;
  printf ("(1 + (intB + intB)) + 3 evaluates to %d.\n", intResult value);
  intResult = (1 + intB) + (3 + 3);
  printf ("(1 + intB) + (3 + 3) evaluates to %d.\n", intResult value);

  intResult = 1 + 2 + intC;
  printf ("1 + 2 + intC evaluates to %d.\n", intResult value);
  intResult = ((1 + 2) + intC) + 4;
  printf ("((1 + 2) + intC) + 4 evaluates to %d.\n", intResult value);
  intResult = (((1 + 2) + intC) + 4) + 5;
  printf ("(((1 + 2) + intC) + 4) + 5 evaluates to %d.\n", intResult value);
  intResult = ((1 + 1) + 2) + intC;
  printf ("((1 + 1) + 2) + intC evaluates to %d.\n", intResult value);
  intResult = (1 + (2 + 2)) + intC;
  printf ("(1 + (2 + 2)) + intC evaluates to %d.\n", intResult value);
  intResult = (1 + 2) + (intC + intC);
  printf ("(1 + 2) + (intC + intC) evaluates to %d.\n", intResult value);

  intResult = ((1 + 2) + 3) + intD;
  printf ("((1 + 2) + 3) + intD evaluates to %d.\n", intResult value);
  intResult = (((1 + 2) + 3) + intD) + 5;
  printf ("(((1 + 2) + 3) + intD) + 5 evaluates to %d.\n", intResult value);

  intResult = (((1 + 2) + 3) + 4) + intE;
  printf ("(((1 + 2) + 3) + 4) + intE evaluates to %d.\n", intResult value);
  
  printf ("longIntA = 1ll;\n");
  printf ("longIntB = 2ll;\n");
  printf ("longIntC = 3ll;\n");
  printf ("longIntD = 4ll;\n");
  printf ("longIntE = 5ll;\n");

  longIntA = 1ll;
  longIntB = 2ll;
  longIntC = 3ll;
  longIntD = 4ll;
  longIntE = 5ll;

  longIntResult = longIntA + 2ll + 3ll;
  printf ("longIntA + 2ll + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = (longIntA + 2ll) + 3ll;
  printf ("(longIntA + 2ll) + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((longIntA + 2ll) + 3ll) + 4ll;
  printf ("((longIntA + 2ll) + 3ll) + 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((longIntA + 2ll) + 3ll) + 4ll) + 5ll;
  printf ("(((longIntA + 2ll) + 3ll) + 4ll) + 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((longIntA + longIntA) + 2ll) + 3ll;
  printf ("((longIntA + longIntA) + 2ll) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (longIntA + (2ll + 2ll)) + 3ll;
  printf ("(longIntA + (2ll + 2ll)) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (longIntA + 2ll) + (3ll + 3ll);
  printf ("(longIntA + 2ll) + (3ll + 3ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll + longIntB + 3ll;
  printf ("1ll + longIntB + 3ll evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll + longIntB) + 3ll) + 4ll;
  printf ("((1ll + longIntB) + 3ll) + 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll + longIntB) + 3ll) + 4ll) + 5ll;
  printf ("(((1ll + longIntB) + 3ll) + 4ll) + 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll + 1ll) + longIntB) + 3ll;
  printf ("((1ll + 1ll) + longIntB) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + (longIntB + longIntB)) + 3ll;
  printf ("(1ll + (longIntB + longIntB)) + 3ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + longIntB) + (3ll + 3ll);
  printf ("(1ll + longIntB) + (3ll + 3ll) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = 1ll + 2ll + longIntC;
  printf ("1ll + 2ll + longIntC evaluates to %lld.\n", longIntResult value);
  longIntResult = ((1ll + 2ll) + longIntC) + 4ll;
  printf ("((1ll + 2ll) + longIntC) + 4ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll + 2ll) + longIntC) + 4ll) + 5ll;
  printf ("(((1ll + 2ll) + longIntC) + 4ll) + 5ll evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = ((1ll + 1ll) + 2ll) + longIntC;
  printf ("((1ll + 1ll) + 2ll) + longIntC evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + (2ll + 2ll)) + longIntC;
  printf ("(1ll + (2ll + 2ll)) + longIntC evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (1ll + 2ll) + (longIntC + longIntC);
  printf ("(1ll + 2ll) + (longIntC + longIntC) evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = ((1ll + 2ll) + 3ll) + longIntD;
  printf ("((1ll + 2ll) + 3ll) + longIntD evaluates to %lld.\n", 
	  longIntResult value);
  longIntResult = (((1ll + 2ll) + 3ll) + longIntD) + 5ll;
  printf ("(((1ll + 2ll) + 3ll) + longIntD) + 5ll evaluates to %lld.\n", 
	  longIntResult value);

  longIntResult = (((1ll + 2ll) + 3ll) + 4ll) + longIntE;
  printf ("(((1ll + 2ll) + 3ll) + 4ll) + longIntE evaluates to %lld.\n", 
	  longIntResult value);
  
  printf ("\"\'c\' + longIntA + intA\" evaluates to: %lld.\n",
	  'c' + longIntA + intA);

  printf ("\"longIntA + \'d\'+ intA\" evaluates to: %lld.\n",
	  longIntA + 'd' + intA);

  printf ("\"longIntA + intA + \'e\'\" evaluates to: %lld.\n",
	  longIntA + intA + 'e');

  s_st.member_int_i = 1;

  intResult = s_st.member_int_i;

  printf ("struct member_int_i is: %d.\n", intResult);

  cmdLine = argv[0];

  printf ("argv[0] is: %s.\n", cmdLine);

  return 0;

}
