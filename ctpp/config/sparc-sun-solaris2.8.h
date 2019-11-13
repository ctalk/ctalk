/* $Id: sparc-sun-solaris2.8.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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
 *  Built-in #defines for Sun Sparc (32 bit) with Solaris 2.8 and GCC.
 *  Should work with other versions of Solaris, although it hasn't been
 *  tested.
 */

#include <stdio.h>

static char *builtins[] = {
  /* symbol */        /* value */
  "__DBL_MIN_EXP__", "(-1021)",
  "__CHAR_BIT__", "8",
  "__WCHAR_MAX__", "2147483647",
  "__DBL_DENORM_MIN__", "4.9406564584124654e-324",
  "__FLT_EVAL_METHOD__", "0",
  "__DBL_MIN_10_EXP__", "(-307)",
  "__FINITE_MATH_ONLY__", "0",
  "sparc", "1",
  "__SHRT_MAX__", "32767",
  "__LDBL_MAX__", "1.18973149535723176508575932662800702e+4932L",
  "__unix", "1",
  "__LDBL_MAX_EXP__", "16384",
  "__SCHAR_MAX__", "127",
  "__USER_LABEL_PREFIX__", "",
  "__STDC_HOSTED__", "1",
  "__LDBL_HAS_INFINITY__", "1",
  "__DBL_DIG__", "15",
  "__FLT_EPSILON__", "1.19209290e-7F",
  "__LDBL_MIN__", "3.36210314311209350626267781732175260e-4932L",
  "__unix__", "1",
  "__DECIMAL_DIG__", "36",
  "__LDBL_HAS_QUIET_NAN__", "1",
  "__DBL_HAS_INFINITY__", "1",
  "__SVR4", "1",
  "__DBL_MAX_EXP__", "1024",
  "__LONG_LONG_MAX__", "9223372036854775807LL",
  "__sparc__", "1",
  "__GXX_ABI_VERSION", "1002",
  "__FLT_MIN_EXP__", "(-125)",
  "__DBL_HAS_QUIET_NAN__", "1",
  "__sun", "1",
  "__REGISTER_PREFIX__", "",
  "__NO_INLINE__", "1",
  "__FLT_MANT_DIG__", "24",
  "__VERSION__", "3.4.6",
  "__sparc", "1",
  "sun", "1",
  "unix", "1",
  "__SIZE_TYPE__", "unsigned int",
  "__ELF__", "1",
  "__FLT_RADIX__", "2",
  "__LDBL_EPSILON__", "1.92592994438723585305597794258492732e-34L",
  "__FLT_HAS_QUIET_NAN__", "1",
  "__FLT_MAX_10_EXP__", "38",
  "__LONG_MAX__", "2147483647L",
  "__FLT_HAS_INFINITY__", "1",
  "__PRAGMA_REDEFINE_EXTNAME", "1",
  "__LDBL_MANT_DIG__", "113",
  "__WCHAR_TYPE__", "long int",
  "__FLT_DIG__", "6",
  "__INT_MAX__", "2147483647",
  "__FLT_MAX_EXP__", "128",
  "__DBL_MANT_DIG__", "53",
  "__WINT_TYPE__", "long int",
  "__LDBL_MIN_EXP__", "(-16381)",
  "__LDBL_MAX_10_EXP__", "4932",
  "__DBL_EPSILON__", "2.2204460492503131e-16",
  "__sun__", "1",
  "__svr4__", "1",
  "__FLT_DENORM_MIN__", "1.40129846e-45F",
  "__FLT_MIN_10_EXP__", "(-37)",
  "__DBL_MAX_10_EXP__", "308",
  "__LDBL_DENORM_MIN__", "6.47517511943802511092443895822764655e-4966L",
  "__PTRDIFF_TYPE__", "int",
  "__LDBL_MIN_10_EXP__", "(-4931)",
  "__LDBL_DIG__", "33",
  "__STDC__", "1",
#if (__GNUC__ < 4) 
  "__DBL_MAX__", "1.7976931348623157e+308",
  "__DBL_MIN__", "2.2250738585072014e-308",
  "__FLT_MAX__", "3.40282347e+38F",
  "__FLT_MIN__", "1.17549435e-38F",
#else
  "__DBL_MAX__", "1.7976931348623157081452E+308",
  "__DBL_MIN__", "2.2250738585072013830903E-308",
  "__FLT_MAX__", "3.4028234663852885981170E+38F",
  "__FLT_MIN__", "1.1754943508222875079688E-38F",
#endif
  NULL, NULL
};
