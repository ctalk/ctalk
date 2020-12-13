/* $Id: break.h,v 1.1.1.1 2020/12/13 14:51:03 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2019 Robert Kiesling, rk3314042@gmail.com.
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

#ifndef _BREAK_H
#define _BREAK_H

/*
 *  Uncomment one of these depending on whether you
 *  want the compiler to break at the start of compiling
 *  a method, the main function, or any other C function.
 *  The ctalkmethods(1) manual page has more information.
 */

/*
 *  Uncomment this line and change METHOD_FN_HERE to the
 *  C name of your method to break on (in quotes).  Refer
 *  to the ctalkmethods(1) manual page for information
 *  about how to construct a function name from a method's
 *  class, selector, and parameter list.
 */
/* #define METHOD_BREAK METHOD_FN_HERE */

/*
 *  Uncomment to cause the compiler to break when it starts 
 *  compiling main.
 */
/* #define MAIN_BREAK */

/*
 *  Uncomment and set FN_NAME_HERE to the name of the C function
 *  that you want to break on (except for main).  Also, enclose the
 *  function name in quotes ("").
 */
/* #define FUNCTION_BREAK FN_NAME_HERE */

#define FN_BREAK_MSG "\n>>> Ctalk: break at start of function, \"%s.\"\n"

#define METHOD_BREAK_MSG "\n>>> Ctalk: break at start of method, \"%s.\"\n"

#endif   /* _BREAK_H */


