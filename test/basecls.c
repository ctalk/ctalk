/* $Id: basecls.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */
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

/* Test the ctalk base classes. */

extern void exit (int);

int main (int argc, char **argv) {
  Character new testChar;
  Integer new testInt;
  Integer new testIntResult;
  Float new testFloat;
  Float new testFloatResult;
  LongInteger new testLongInt;
  String new testString;

  testChar = 'a';
  testFloat = 1.5;
  testInt = 3;
  testLongInt = 1000000ll;
  testString = "This is the value of \"testString.\"";

  printf ("---------------------\n");
  printf ("Character Class\n");
  printf ("---------------------\n");
  printf ("The value of testChar is \'%c\'.\n", testChar);

  /*
   *  Test integer arithmetic with both a single object and an 
   *  expression in the argument list.
   */
  printf ("---------------------\n");
  printf ("Integer Class\n");
  printf ("---------------------\n");
  printf ("The value of testInt is %d.\n", testInt);
  testIntResult = testInt + testInt;
  printf ("The value of testInt + testInt is %d.\n", testIntResult);
  testIntResult = testInt - testInt;
  printf ("The value of testInt - testInt is %d.\n", testIntResult);
  printf ("The value of testInt * testInt is %d.\n", testInt * testInt);
  printf ("The value of testInt / testInt is %d.\n", testInt / testInt);
  printf ("The value of testInt << testInt is %d.\n", testInt << testInt);
  printf ("The value of testInt >> testInt is %d.\n", testInt >> testInt);
  printf ("The value of testInt & testInt is %d.\n", testInt & testInt);
  printf ("The value of testInt | testInt is %d.\n", testInt | testInt);
  printf ("The value of testInt ^ testInt is %d.\n", testInt ^ testInt);
  printf ("The value of testInt bitComp is %d.\n", testInt bitComp);

  printf ("---------------------\n");
  printf ("Float Class\n");
  printf ("---------------------\n");
  printf ("The value of testFloat is %f.\n", testFloat);
  testFloatResult = testFloat + testFloat;
  printf ("The value of testFloat + testFloat is %f.\n", testFloatResult);
  testFloatResult = testFloat - testFloat;
  printf ("The value of testFloat - testFloat is %f.\n", testFloatResult);
  printf ("The value of testFloat * testFloat is %f.\n", 
	  testFloat * testFloat);
  printf ("The value of testFloat / testFloat is %f.\n", 
	  testFloat / testFloat);

  printf ("---------------------\n");
  printf ("LongInteger Class\n");
  printf ("---------------------\n");
  printf ("The value of testLongInt is %lld.\n", testLongInt);
  printf ("The value of testLongInt + testLongInt is %lld.\n", 
	  testLongInt + testLongInt);
  printf ("The value of testLongInt - testLongInt is %lld.\n", 
	  testLongInt - testLongInt);
  printf ("The value of testLongInt * testLongInt is %lld.\n", 
	  testLongInt * testLongInt);
  printf ("The value of testLongInt / testLongInt is %lld.\n", 
	  testLongInt / testLongInt);

  printf ("---------------------\n");
  printf ("String Class\n");
  printf ("---------------------\n");
  printf ("The value of testString is: %s.\n", testString);
  printf ("The value of testString + testString is: %s.\n",
 	  testString + testString);

  testString = "\"Escaped double quotes.\"";
  printf ("%s\n", testString);
  testString = "\"\"Escaped double quotes.\"\"";
  printf ("%s\n", testString);
  testString = "\"\"\"Escaped double quotes.\"\"\"";
  printf ("%s\n", testString);
  testString = "\"\"\"\"Escaped double quotes.\"\"\"\"";
  printf ("%s\n", testString);
  testString = "\"\"\"\"\"Escaped double quotes.\"\"\"\"\"";
  printf ("%s\n", testString);

  testString = "This is the value of \"testString.\"";

  printf ("The value of testString subString 5, 2 is: %s.\n", 
	  testString subString 5, 2);
  printf ("The value of testString length is: %d.\n", 
	  testString length);
  exit (0);
}

