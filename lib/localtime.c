/* $Id: localtime.c,v 1.1.1.1 2019/10/26 23:40:50 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014  Robert Kiesling, rk3314042@gmail.com.
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
 *  C wrapper for localtime () and localtime_r () library functions.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"


#ifdef __DJGPP__
void __ctalkLocalTime (time_t utctime, int *seconds_return, 
		       int *minutes_return, int *hours_return,
		       int *dom_return, int *mon_return,
		       int *years_return, int *dow_return,
		       int *doy_return, int *have_dst_return) {
#else
void __ctalkLocalTime (long int utctime, int *seconds_return,
		      int *minutes_return, int *hours_return,
		      int *dom_return, int *mon_return,
		      int *years_return, int *dow_return,
		      int *doy_return, int *have_dst_return) {
#endif

#ifdef HAVE_LOCALTIME_R

  static struct tm __tm;

  (void) localtime_r (&utctime, &__tm);

  *seconds_return = __tm.tm_sec;
  *minutes_return = __tm.tm_min;
  *hours_return = __tm.tm_hour;
  *dom_return = __tm.tm_mday;
  *mon_return = __tm.tm_mon;
  *years_return = __tm.tm_year;
  *dow_return = __tm.tm_wday;
  *doy_return = __tm.tm_yday;
  *have_dst_return = __tm.tm_isdst;

#else /* #ifdef HAVE_LOCALTIME_R */

  struct tm *__tm_ptr = localtime (&utctime);

  *seconds_return = __tm_ptr -> tm_sec;
  *minutes_return = __tm_ptr -> tm_min;
  *hours_return = __tm_ptr -> tm_hour;
  *dom_return = __tm_ptr -> tm_mday;
  *mon_return = __tm_ptr -> tm_mon;
  *years_return = __tm_ptr -> tm_year;
  *dow_return = __tm_ptr -> tm_wday;
  *doy_return = __tm_ptr -> tm_yday;
  *have_dst_return = __tm_ptr -> tm_isdst;
  
#endif /* #ifdef HAVE_LOCALTIME_R */
}
