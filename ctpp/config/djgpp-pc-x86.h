/* $Id: djgpp-pc-x86.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 *  Built-in #defines for DJGPP, Version 2.  See builtins.c
 *  for information.
 */

#include <stdio.h>

static char *builtins[] = {
  /* symbol */              /* value */
  "__GO32", "1",
  "__DBL_MIN_EXP__", "(-1021)",
  "__FLT_MIN__",  "1.17549435e-38F",
  "__CHAR_BIT__", "8",
  "__WCHAR_MAX__", "65535U",
  "__DBL_DENORM_MIN__", "4.9406564584124654e-324",
  "__FLT_EVAL_METHOD__", "2",
  "__DBL_MIN_10_EXP__", "(-307)",
  "__FINITE_MATH_ONLY__", "0",
  "__SHRT_MAX__", "32767",
  "__LDBL_MAX__", "1.18973149535723176502e+4932L",
  "__UINTMAX_TYPE__", "long long unsigned int",
  "__unix", "1",
  "__LDBL_MAX_EXP__", "16384",
  "__DJGPP", "2",
  "__SCHAR_MAX__", "127",
  "__USER_LABEL_PREFIX__", "_",
  "__STDC_HOSTED__", "1",
  "__LDBL_HAS_INFINITY__", "1",
  "__MSDOS__", "1",
  "__DBL_DIG__", "15",
  "__FLT_EPSILON__", "1.19209290e-7F",
  "__LDBL_MIN__", "3.36210314311209350626e-4932L",
  "__DJGPP__", "2",
  "__unix__", "1",
  "__DECIMAL_DIG__", "21",
  "__LDBL_HAS_QUIET_NAN__", "1",
  "__tune_i586__", "1",
  "__DBL_MAX__", "1.7976931348623157e+308",
  "__DBL_HAS_INFINITY__", "1",
  "__DJGPP_MINOR__", "3",
  "__DBL_MAX_EXP__", "1024",
  "__LONG_LONG_MAX__", "9223372036854775807LL",
  "__GXX_ABI_VERSION", "1002",
  "__FLT_MIN_EXP__", "(-125)",
  "__GO32__", "1",
  "GO32", "1",
  "DJGPP_MINOR", "3",
  "__DBL_MIN__", "2.2250738585072014e-308",
  "MSDOS", "1",
  "__DBL_HAS_QUIET_NAN__", "1",
  "__REGISTER_PREFIX__",   "",
  "__NO_INLINE__", "1",
  "__i386", "1",
  "__FLT_MANT_DIG__", "24",
  "__VERSION__", "\"4.0.1\"",
  "i386", "1",
  "unix", "1",
  "__i386__", "1",
  "__SIZE_TYPE__", "long unsigned int",
  "__FLT_RADIX__", "2",
  "__LDBL_EPSILON__", "1.08420217248550443401e-19L",
  "__FLT_HAS_QUIET_NAN__", "1",
  "__FLT_MAX_10_EXP__", "38",
  "__LONG_MAX__", "2147483647L",
  "__FLT_HAS_INFINITY__", "1",
  "__tune_pentium__", "1",
  "DJGPP", "2",
  "__LDBL_MANT_DIG__", "64",
  "__WCHAR_TYPE__", "short unsigned int",
  "__FLT_DIG__", "6",
  "__INT_MAX__", "2147483647",
  "__FLT_MAX_EXP__", "128",
  "__DBL_MANT_DIG__", "53",
  "__WINT_TYPE__", "int",
  "__LDBL_MIN_EXP__", "(-16381)",
  "__LDBL_MAX_10_EXP__", "4932",
  "__DBL_EPSILON__", "2.2204460492503131e-16",
  "__INTMAX_MAX__", "9223372036854775807LL",
  "__FLT_DENORM_MIN__", "1.40129846e-45F",
  "__MSDOS", "1",
  "__FLT_MAX__", "3.40282347e+38F",
  "__FLT_MIN_10_EXP__", "(-37)",
  "__INTMAX_TYPE__", "long long int",
  "__DBL_MAX_10_EXP__", "308",
  "__LDBL_DENORM_MIN__", "3.64519953188247460253e-4951L",
  "__PTRDIFF_TYPE__", "int",
  "__LDBL_MIN_10_EXP__", "(-4931)",
  "__DJGPP_MINOR", "3",
  "__LDBL_DIG__", "18",
  /*
   *  These cause unexplained errors or warnings with dos.h, depending on 
   *  whether cpp or ctpp is used. (Requires defs from another .h file?)
   *  Not certain where uint comes from at all....  Arrgh.
   */
  "__attribute__((packed))", " ",
  "uint", "int",
  /* 
   * GNU cpp defines these, gcc doesn't. 
   */
  "__STDC__",               "1",
  "STDC",                   "1",
  NULL, NULL
};
