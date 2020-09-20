/* $Id: pcvars.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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

/*
 *  Abbreviated version of cvars.c in the main ctalk code.  Only
 *  pmatch_type is used for pmatch_type () is used.
 *  etc.
 */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include "ctpp.h"
#include "typeof.h"

int pmatch_type (VAL *val1, VAL *val2) {

  int retval = ERROR;

  if (val1 -> __type == val2 -> __type) {
    retval = SUCCESS;
  } else {

    switch (val1 -> __type) 
      {
      case INTEGER_T:
	switch (val2 -> __type)
	  {
	  case LONG_T:
	    val1 -> __type = LONG_T;
	    retval = SUCCESS;
	    break;
	  case LONGLONG_T:
	    val1 -> __type = LONGLONG_T;
	    val1 -> __value.__ll = (long long) val1 -> __value.__i;
	    retval = SUCCESS;
	    break;
#ifndef __APPLE__
	  case DOUBLE_T:
	    val1 -> __type = DOUBLE_T;
	    val1 -> __value.__d = (double) val1 -> __value.__i;
	    retval = SUCCESS;
	    break;
	  case LONGDOUBLE_T:
	    val1 -> __type = LONGDOUBLE_T;
	    val1 -> __value.__ld = (long double) val1 -> __value.__i;
	    retval = SUCCESS;
	    break;
#else
	  case DOUBLE_T:
	  case LONGDOUBLE_T:
	    val1 -> __type = DOUBLE_T;
	    val1 -> __value.__d = (double) val1 -> __value.__i;
	    retval = SUCCESS;
	    break;
#endif
	  }
	break;
      case LONG_T:
	switch (val2 -> __type)
	  {
	  case INTEGER_T:
	    val2 -> __type = LONG_T;
	    retval = SUCCESS;
	    break;
	  case LONGLONG_T:
	    val1 -> __type = LONGLONG_T;
	    val1 -> __value.__ll = (long long) val1 -> __value.__l;
	    retval = SUCCESS;
	    break;
#ifndef __APPLE__
	  case DOUBLE_T:
	    val1 -> __type = DOUBLE_T;
	    val1 -> __value.__d = (double) val1 -> __value.__l;
	    retval = SUCCESS;
	    break;
	  case LONGDOUBLE_T:
	    val1 -> __type = LONGDOUBLE_T;
	    val1 -> __value.__ld = (long double) val1 -> __value.__l;
	    retval = SUCCESS;
	    break;
#else
	  case DOUBLE_T:
	  case LONGDOUBLE_T:
	    val1 -> __type = DOUBLE_T;
	    val1 -> __value.__d = (double) val1 -> __value.__l;
	    retval = SUCCESS;
	    break;
#endif
	  }
	break;
      case LONGLONG_T:
	switch (val2 -> __type)
	  {
	  case INTEGER_T:
	    val2 -> __type = LONGLONG_T;
	    val2 -> __value.__ll = (long long) val2 -> __value.__i;
	    retval = SUCCESS;
	    break;
	  case LONG_T:
	    val2 -> __type = LONGLONG_T;
	    val2 -> __value.__ll = (long long) val2 -> __value.__l;
	    retval = SUCCESS;
	    break;
#ifndef __APPLE__
	  case DOUBLE_T:
	    val1 -> __type = DOUBLE_T;
	    val1 -> __value.__d = (double) val1 -> __value.__ll;
	    retval = SUCCESS;
	    break;
	  case LONGDOUBLE_T:
	    val1 -> __type = LONGDOUBLE_T;
	    val1 -> __value.__ld = (long double) val1 -> __value.__ll;
	    retval = SUCCESS;
	    break;
#else
	  case DOUBLE_T:
	  case LONGDOUBLE_T:
	    val1 -> __type = DOUBLE_T;
	    val1 -> __value.__d = (double) val1 -> __value.__ll;
	    retval = SUCCESS;
	    break;
#endif
	  }
	break;
      default:
	_warning ("Unimplemented type %d in match_type.", val1 -> __type);
	break;
      }
  }
  

  return retval;
}

