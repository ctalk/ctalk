/* $Id: radixof.c,v 1.2 2019/12/06 21:14:23 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015-2017, 2019  Robert Kiesling, rk3314042@gmail.com.
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

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "lex.h"

#define IS_SIGN(c,d) ((c == '-' || c == '+') && \
		      (isdigit (d) || d == '.'))

RADIX radix_of (char *buf) {

  RADIX radix = decimal;
  int i, firstdigit;
  
  if (buf == NULL)
    return radix;

  if (buf[0] && buf[1]) {

    firstdigit = (IS_SIGN (buf[0],(int)buf[1]) ? 1 : 0);

  } else {
    if (isdigit ((int)buf[0])) {
      firstdigit = 0;
    } else {
      return -1;
    }
  }

  for (i = firstdigit; i < strlen (buf); i++)
    if (!isxdigit ((int)buf[i]) && 
	(buf[i] != (char)'l') && (buf[i] != (char)'L') &&
	(buf[i] != (char)'u') && (buf[i] != (char)'U') &&
	(buf[i] != (char)'x') && (buf[i] != (char)'X') &&
	(buf[i] != (char)'b') && (buf[i] != (char)'B'))
      return -1;

#if defined(__GNUC__) && defined(__sparc__) && defined(__svr4__)
  if (buf[0] && buf[1] && !IS_SIGN (buf[0],(int)buf[1])) {
#else
  if (buf[0] && buf[1] && !IS_SIGN (buf[0],buf[1])) {
#endif
    if ((buf[0] == (char)'0') && (buf[1] != (char)'.')) {
      if (buf[1] == (char) 'x' || buf[1] == 'X') {
	radix = hexadecimal;
      } else if (buf[1] == (char) 'b' || buf[1] == (char) 'B') {
	radix = binary;
      } else if (buf[1] && (buf[1] >= '0' && buf[1] <= '7')) {
	radix = octal;
      }
    } else {
      if ((buf[strlen(buf)-1] == (char)'B') || 
	  (buf[strlen(buf)-1] == (char)'b'))
	radix = binary;
    }
  } else {
    if (buf[1] && buf[2]) {
      if ((buf[1] == (char)'0') && (buf[2] != (char)'.')) {
	if (buf[2] == (char) 'x' || buf[2] == 'X') {
	  radix = hexadecimal;
	} else {
	  if (buf[2] && (buf[2] >= '0' && buf[2] <= '7'))
	    radix = octal;
	}
      } else {
	if ((buf[strlen(buf)-1] == (char)'B') || 
	    (buf[strlen(buf)-1] == (char)'b'))
	  radix = binary;
      }
    }
  }
  return radix;
}

char *__ctalkLongLongRadixToDecimal (char *longlong) {
  static char buf[MAXMSG];
  unsigned long long int value;
  RADIX radix;

  radix = radix_of (longlong);
  switch (radix) 
    {
    case hexadecimal:
      value = strtoll (longlong, NULL, 16);
      sprintf (buf, "%lld", value);
      break;
    case octal:
      value = strtoll (longlong, NULL, 8);
      sprintf (buf, "%lld", value);
      break;
    case decimal:
    case binary:
      strcpy (buf, longlong);
      break;
    }

  if (!strstr (buf, "ll") && !strstr (buf, "L"))
    strcatx2 (buf, "ll", NULL);

  return buf;
}

char *__ctalkIntRadixToDecimalASCII (char *intbuf) {
  static char buf[MAXMSG];
  unsigned int value;
  int i, bit_exp, sum;
  RADIX radix;

  if ((radix = radix_of (intbuf)) == (RADIX)-1) {
    return intbuf;
  }

  switch (radix) 
    {
    case hexadecimal:
      value = strtol (intbuf, NULL, 16);
      __ctalkDecimalIntegerToASCII (value, buf);
      break;
    case octal:
      value = strtol (intbuf, NULL, 8);
      __ctalkDecimalIntegerToASCII (value, buf);
      break;
    case decimal:
      return intbuf;
      break;
    case binary:
      for (i = strlen (intbuf) - 2, bit_exp = 1, sum = 0; i >= 0; 
	   i--, bit_exp *= 2) {
	if ((intbuf[i] == (char)'b') || (intbuf[i] == (char)'B')) 
	  break;
	switch (intbuf[i])
	  {
	  case '1':
	    sum += bit_exp;
	    break;
	  case '0':
	    break;
	  default:
	    _warning ("Bad binary constant.\n");
	    break;
	  }
      }
      __ctalkDecimalIntegerToASCII (sum, buf);
      break;
    }

  return buf;
}

int __ctalkIntRadixToDecimal (char *intbuf) {
  static char buf[MAXMSG];
  unsigned int value;
  int i, bit_exp, sum;
  RADIX radix;

  if ((radix = radix_of (intbuf)) == (RADIX)-1) {
    return strtol (intbuf, NULL, 0);
  }

  switch (radix) 
    {
    case hexadecimal:
      value = strtol (intbuf, NULL, 16);
      break;
    case octal:
      value = strtol (intbuf, NULL, 8);
      break;
    case decimal:
      return strtol (intbuf, NULL, 10);
      break;
    case binary:
      for (i = strlen (intbuf) - 2, bit_exp = 1, sum = 0; i >= 0; 
	   i--, bit_exp *= 2) {
	if ((intbuf[i] == (char)'b') || (intbuf[i] == (char)'B')) 
	  break;
	switch (intbuf[i])
	  {
	  case '1':
	    sum += bit_exp;
	    break;
	  case '0':
	    break;
	  default:
	    _warning ("Bad binary constant.\n");
	    break;
	  }
      }
      value = sum;
      break;
    }

  return value;
}

/*
 *  If the formatted character is more than one digit, then
 *  convert from a decimal integer.
 */
char *__ctalkCharRadixToCharASCII (char *charbuf) {
  static char buf[MAXMSG];
  char tmpbuf[MAXMSG];
  unsigned int value;
  int char_is_quoted = 0;
  int i, bit_exp, sum;
  RADIX radix;

  if ((radix = radix_of (charbuf)) == (RADIX)-1) {
    return charbuf;
  }

  strcpy (tmpbuf, charbuf);
  while (*tmpbuf == '\'') {
    TRIM_CHAR(tmpbuf);
    ++char_is_quoted;
  }
  
  switch (radix) 
    {
    case hexadecimal:
      buf[0] = (char)strtol (tmpbuf, NULL, 16);
      break;
    case octal:
      buf[0] = (char) strtol (tmpbuf, NULL, 8);
      break;
    case decimal:
      if (isdigit ((int)*tmpbuf) && (strlen (tmpbuf) > 1)) {
	buf[0] = (char) strtol (tmpbuf, NULL, 10);
      } else {
	strcpy (buf, tmpbuf);
      }
      break;
    case binary:
      for (i = strlen (tmpbuf) - 2, bit_exp = 1, sum = 0; i >= 0; 
	   i--, bit_exp *= 2) {
	if ((tmpbuf[i] == (char)'b') || (tmpbuf[i] == (char)'B')) 
	  break;
	switch (tmpbuf[i])
	  {
	  case '1':
	    sum += bit_exp;
	    break;
	  case '0':
	    break;
	  default:
	    _warning ("Bad binary constant.\n");
	    break;
	  }
      }
      sprintf (buf, "%c", sum);
      break;
    default:
      strcpy (buf, tmpbuf);
      break;
    }

  while (char_is_quoted--) {
    sprintf (tmpbuf, "\'%s\'", buf);
    strcpy (buf, tmpbuf);
  }

  return buf;
}

char __ctalkCharRadixToChar (char *charbuf) {
  static char buf[MAXMSG];
  char tmpbuf[MAXMSG];
  unsigned int value;
  int char_is_quoted = 0;
  int i, bit_exp, sum;
  char result;
  RADIX radix;

  if (charbuf[0] != '\'') {
    if ((radix = radix_of (charbuf)) == (RADIX)-1) {
      return *charbuf;
    }
    strcpy (tmpbuf, charbuf);
  } else {
    if (charbuf[1] != '\0') {
      strcpy (tmpbuf, charbuf);
      while (*tmpbuf == '\'') {
	TRIM_CHAR(tmpbuf);
	++char_is_quoted;
      }
      if ((radix = radix_of (tmpbuf)) == (RADIX)-1) {
	return *tmpbuf;
      }
    }
  }
  
  switch (radix) 
    {
    case hexadecimal:
      result = (char)strtol (tmpbuf, NULL, 16);
      break;
    case octal:
      result = (char)strtol (tmpbuf, NULL, 8);
      break;
    case decimal:
      result = (char) strtol (tmpbuf, NULL, 10);
      break;
    case binary:
      for (i = strlen (tmpbuf) - 2, bit_exp = 1, sum = 0; i >= 0; 
	   i--, bit_exp *= 2) {
	if ((tmpbuf[i] == (char)'b') || (tmpbuf[i] == (char)'B')) 
	  break;
	switch (tmpbuf[i])
	  {
	  case '1':
	    result += bit_exp;
	    break;
	  case '0':
	    break;
	  default:
	    _warning ("Bad binary constant.\n");
	    break;
	  }
      }
      break;
    }
  return result;
}

