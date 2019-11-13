/* $Id: ia64-unknown-linux-gnu.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

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
 *  Built-in #defines for linux-gnu with with Intel Itanium processor.
 *  Tested on SGI Altix, SMP, SuSE Linux.
 */

#include <stdio.h>

static char *builtins[] = {
  /* symbol */        /* value */
  "__DBL_MIN_EXP__", "(-1021)",
  "__ia64__", "1",
  "__FLT_MIN__", "1.17549435e-38F",
  "__CHAR_BIT__", "8",
  "__WCHAR_MAX__", "2147483647",
  "__DBL_DENORM_MIN__", "4.9406564584124654e-324",
  "__FLT_EVAL_METHOD__", "0",
  "__unix__", "1",
  "unix", "1",
  "__SIZE_TYPE__", "long unsigned int",
  "__ia64", "1",
  "__ELF__", "1",
  "__DBL_MIN_10_EXP__", "(-307)",
  "__FINITE_MATH_ONLY__", "0",
  "__FLT_RADIX__", "2",
  "__LDBL_EPSILON__", "1.08420217248550443401e-19L",
  "__SHRT_MAX__", "32767",
  "__LDBL_MAX__", "1.18973149535723176502e+4932L",
  "__linux", "1",
  "__unix", "1",
  "__LDBL_MAX_EXP__", "16384",
  "__LONG_MAX__", "9223372036854775807L",
  "__linux__", "1",
  "__SCHAR_MAX__", "127",
  "__DBL_DIG__", "15",
  "__USER_LABEL_PREFIX__", "",
  "linux", "1",
  "__STDC_HOSTED__", "1",
  "__LDBL_MANT_DIG__", "64",
  "__itanium__", "1",
  "__FLT_EPSILON__", "1.19209290e-7F",
  "_LONGLONG", "1",
  "__LDBL_MIN__", "3.36210314311209350626e-4932L",
  "__WCHAR_TYPE__", "int",
  "__FLT_DIG__", "6",
  "__FLT_MAX_10_EXP__", "38",
  "__INT_MAX__", "2147483647",
  "__gnu_linux__", "1",
  "__FLT_MAX_EXP__", "128",
  "__DECIMAL_DIG__", "21",
  "__DBL_MANT_DIG__", "53",
  "__WINT_TYPE__", "unsigned int",
  "__LDBL_MIN_EXP__", "(-16381)",
  "__LDBL_MAX_10_EXP__", "4932",
  "__DBL_EPSILON__", "2.2204460492503131e-16",
  "_LP64", "1",
  "__DBL_MAX__", "1.7976931348623157e+308",
  "__DBL_MAX_EXP__", "1024",
  "__FLT_DENORM_MIN__", "1.40129846e-45F",
  "__LONG_LONG_MAX__", "9223372036854775807LL",
  "__FLT_MAX__", "3.40282347e+38F",
  "__GXX_ABI_VERSION", "102",
  "__FLT_MIN_10_EXP__", "(-37)",
  "__FLT_MIN_EXP__", "(-125)",
  "__DBL_MAX_10_EXP__", "308",
  "__LDBL_DENORM_MIN__", "3.64519953188247460253e-4951L",
  "__DBL_MIN__", "2.2250738585072014e-308",
  "__PTRDIFF_TYPE__", "long int",
  "__LP64__", "1",
  "__LDBL_MIN_10_EXP__", "(-4931)",
  "__REGISTER_PREFIX__", "",
  "__LDBL_DIG__", "18",
  "__NO_INLINE__", "1",
  "__FLT_MANT_DIG__", "24",
  "__VERSION__", "3.3.3 (SuSE Linux)",
  "__STDC__", "1",
  "STDC", "1",
  NULL, NULL
};
