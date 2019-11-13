/* $Id: ctrlstrc.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
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

/* Test the ctalk control structures. */

#include <stdio.h>

int main (int argc, char **argv) {

  Integer new testInt;
  Integer new testInt2;
  Integer new intA;
  Integer new intB;

  testInt = 1;

  if (testInt) {
    printf ("if (testInt) {} evaluates to True.\n");
  }

  if (!testInt) {
    printf ("if (testInt) {} evaluates to False.\n");
  } else {
    printf ("if (testInt) {} does not evaluate to False.\n");
  }

  testInt2 = 2;

  if (!testInt) {
    printf ("if (testInt) {} evaluates to False.\n");
  } else if (testInt2) {
    printf ("if (testInt2) {} evaluates to True.\n");
  }

  printf ("While loop....\n");
  while (testInt <= 100) {
    printf (".");
    testInt = testInt + 1;
  }
  printf ("\n");

  printf ("Do loop....\n");
  testInt = 1;
  do {
    printf (".");
    testInt = testInt + 1;
  } while (testInt <= 100);

  printf ("\n");

  printf ("For loops....\n");
  for (testInt = 0; testInt <= 100; testInt = testInt + 2) {
    printf ("%d ", testInt);
  }

  printf ("\n\n");

  for (intA = 0; intA <= 10; intA = intA + 1) {
    for (intB = 0; intB <= 10; intB = intB + 1) {
      printf ("%3d,%2d ", intA, intB);
    }
    printf ("\n");
  }

  return 0;
}

