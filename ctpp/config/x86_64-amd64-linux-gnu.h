/* $Id: x86_64-amd64-linux-gnu.h,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright � 2005-2012 Robert Kiesling, rk3314042@gmail.com.
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
 *  Built-in #defines for linux-gnu with with amd64 processor.
 *  Tested on SGI Altix, SMP, SuSE Linux.
 */

#include <stdio.h>

static char *builtins[] = {
  /* symbol */        /* value */
  "__DBL_MIN_EXP__", "(-1021)",
  "__FLT_MIN__",     "1.17549435e-38F",
  "__CHAR_BIT__",    "8",
  "__WCHAR_MAX__",   "2147483647",
  "__DBL_DENORM_MIN__", "4.9406564584124654e-324",
  "__FLT_EVAL_METHOD__", "0",
  "__DBL_MIN_10_EXP__", "(-307)",
  "__FINITE_MATH_ONLY__", "0",
  "__SHRT_MAX__", "32767",
  "__LDBL_MAX__", "1.18973149535723176502e+4932L",
  "__UINTMAX_TYPE__", "long unsigned int",
  "__linux", "1",
  "__unix", "1",
  "__linux__", "1",
  "__SCHAR_MAX__", "127",
  "__USER_LABEL_PREFIX__", "",
  "__STDC_HOSTED__", "1",
  "__DBL_DIG__", "15",
  "__FLT_EPSILON__", "1.19209290e-7F",
  "__LDBL_MIN__", "3.36210314311209350626e-4932L",
  "__unix__", "1",
  "__DECIMAL_DIG__", "21",
  "__gnu_linux__", "1",
  "__LDBL_HAS_QUIET_NAN__", "1",
  "__MMX__", "1",
  "__DBL_MAX__", "1.7976931348623157e+308",
  "__DBL_HAS_INFINITY__", "1",
  "__DBL_MAX_EXP__", "1024",
  "__SSE2_MATH__", "1",
  "__amd64", "1",
  "__LONG_LONG_MAX__", "9223372036854775807LL",
  "__GXX_ABI_VERSION", "1002",
  "__FLT_MIN_EXP__", "(-125)",
  "__x86_64", "1",
  "__DBL_MIN__", "2.2250738585072014e-308",
  "__LP64__", "1",
  "__DBL_HAS_QUIET_NAN__", "1",
  "__REGISTER_PREFIX__", "",
  "__NO_INLINE__", "1",
  "__FLT_MANT_DIG__", "24",
  "__VERSION__", "4.1.2 20070115 (prerelease) (SUSE Linux)",
  "unix", "1",
  "__SIZE_TYPE__", "long unsigned int",
  "__ELF__", "1",
  "__FLT_RADIX__", "2",
  "__LDBL_EPSILON__", "1.08420217248550443401e-19L",
  "__SSE_MATH__", "1",
  "__k8", "1",
  "__LDBL_DIG__", "18",
  "__x86_64__", "1",
  "__FLT_HAS_QUIET_NAN__", "1",
  "__FLT_MAX_10_EXP__", "38",
  "__LONG_MAX__", "9223372036854775807L",
  "__FLT_HAS_INFINITY__", "1",
  "linux", "1",
  "__LDBL_MANT_DIG__", "64",
  "__k8__", "1",
  "__WCHAR_TYPE__", "int",
  "__FLT_DIG__", "6",
  "__INT_MAX__", "2147483647",
  "__FLT_MAX_EXP__", "128",
  "__DBL_MANT_DIG__", "53",
  "__WINT_TYPE__", "unsigned int",
  "__SSE__", "1",
  "__LDBL_MIN_EXP__", "(-16381)",
  "__amd64__", "1",
  "__LDBL_MAX_EXP__", "16384",
  "__LDBL_MAX_10_EXP__", "4932",
  "__DBL_EPSILON__", "2.2204460492503131e-16",
  "_LP64", "1",
  "__LDBL_HAS_INFINITY__", "1",
  "__INTMAX_MAX__", "9223372036854775807L",
  "__FLT_DENORM_MIN__", "1.40129846e-45F",
  "__FLT_MAX__", "3.40282347e+38F",
  "__SSE2__", "1",
  "__FLT_MIN_10_EXP__", "(-37)",
  "__INTMAX_TYPE__", "long int",
  "__DBL_MAX_10_EXP__", "308",
  "__LDBL_DENORM_MIN__", "3.64519953188247460253e-4951L",
  "__STDC__", "1",
  "__PTRDIFF_TYPE__", "long int",
  "__LDBL_MIN_10_EXP__", "(-4931)",
#if (__SIZEOF_POINTER__ == 8)
  "__SIZEOF_POINTER__", "8",
#endif
  NULL, NULL
};
