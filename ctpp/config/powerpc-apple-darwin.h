/* $Id: powerpc-apple-darwin.h,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
 *  Built-in #defines for Mac OS X PowerPC.  Note:  If you need more 
 *  recent #defines than these, create a new .h file in the following 
 *  manner. 
 * 
 *  1. Print GCC's builtin #defines, and output to a file.  
 *    $ echo ' ' | cpp -dM -  > cpp.out
 *
 *  2. Edit a copy of this file with the definitions contained in 
 *     cpp.out, and name the file after the processor-arch-os 
 *     trigraph (with a .h suffix of course) printed by 
 *     ctpp/config.guess.
 *
 *  3. Edit ctpp/ctpp/builtins.c to include the new file.
 *
 *  4. Build and install ctpp and test the definitions.  
 *     $ cd ctpp/test
 *     $ make
 *     # Note that for pre-10.3 systems, you may need to use
 *     # the following command.
 *     $ make -f Makefile.darwin
 *
 *  5. When the tests conclude without errors or warnings, send 
 *     patches to rk3314042@gmail.com, so the update can be included 
 *     in further releases of ctpp and Ctalk.
 *  
 *  Here we define __STRICT_ANSI__ for long long compatibility
 *  with earlier OS X versions.  Ctalk implements its own atoll with
 *  Darwin, and its possible to implement libc compatibility 
 *  functions if the class libraries need them. 
 */

#include <stdio.h>

static char *builtins[] = {
  /* symbol */        /* value */
  "__DBL_MIN_EXP__", "(-1021)",
  "__BIG_ENDIAN__", "1",
  "__FLT_MIN__", "1.17549435e-38F",
  "__CHAR_BIT__", "8",
  "__WCHAR_MAX__", "2147483647",
  "__DBL_DENORM_MIN__", "4.9406564584124654e-324",
  "__FLT_EVAL_METHOD__", "0",
  "__SIZE_TYPE__", "long unsigned int",
  "__DBL_MIN_10_EXP__", "(-307)",
  "__FINITE_MATH_ONLY__", "0",
  "__FLT_RADIX__", "2",
  "__LDBL_EPSILON__", "2.2204460492503131e-16L",
  "__SHRT_MAX__", "32767",
  "__LDBL_MAX__", "1.7976931348623157e+308L",
  "__NATURAL_ALIGNMENT__", "1",
  "_ARCH_PPC", "1",
  "__LDBL_MAX_EXP__", "1024",
  "__LONG_MAX__", "2147483647L",
  "__SCHAR_MAX__", "127",
  "__DBL_DIG__", "15",
  "_BIG_ENDIAN", "1",
  "__USER_LABEL_PREFIX__", "_",
  "__STDC_HOSTED__", "1",
  "__LDBL_MANT_DIG__", "53",
  "__FLT_EPSILON__", "1.19209290e-7F",
  "__LDBL_MIN__", "2.2250738585072014e-308L",
  "__DYNAMIC__", "1",
  "__WCHAR_TYPE__", "int",
  "__ppc__", "1",
  "__FLT_DIG__", "6",
  "__FLT_MAX_10_EXP__", "38",
  "__APPLE__", "1",
  "__INT_MAX__", "2147483647",
  "__MACH__", "1",
  "__FLT_MAX_EXP__", "128",
  "__DECIMAL_DIG__", "17",
  "__DBL_MANT_DIG__", "53",
  "__WINT_TYPE__", "unsigned int",
  "__LDBL_MIN_EXP__", "(-1021)",
  "__LDBL_MAX_10_EXP__", "308",
  "__DBL_EPSILON__", "2.2204460492503131e-16",
  "__DBL_MAX__", "1.7976931348623157e+308",
  "__DBL_MAX_EXP__", "1024",
  "__FLT_DENORM_MIN__", "1.40129846e-45F",
  "__LONG_LONG_MAX__", "9223372036854775807LL",
  "__FLT_MAX__", "3.40282347e+38F",
  "__GXX_ABI_VERSION", "102",
  "__FLT_MIN_10_EXP__", "(-37)",
  "__FLT_MIN_EXP__", "(-125)",
  "__DBL_MAX_10_EXP__", "308",
  "__LDBL_DENORM_MIN__", "4.9406564584124654e-324L",
  "__DBL_MIN__", "2.2250738585072014e-308",
  "__PTRDIFF_TYPE__", "int",
  "__LDBL_MIN_10_EXP__", "(-307)",
  "__REGISTER_PREFIX__", "", 
  "__LDBL_DIG__", "15",
  "__POWERPC__", "1",
  "__NO_INLINE__", "1",
  "__FLT_MANT_DIG__", "24",
  "__STDC__", "1",
  "__STRICT_ANSI__", "1",
  "__VERSION__", "Ctpp (powerpc-apple-darwin)",
  NULL, NULL
};
