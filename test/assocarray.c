/* $Id: assocarray.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
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


AssociativeArray instanceMethod printValue (void) {

  printf ("%s\t", self name);
  printf ("%s\n", self value);
  return NULL;
}

AssociativeArray instanceMethod printKeyValue (void) {

  String new valueObject;
  printf ("%s =>\t", self name);
  valueObject =self getValue;
  printf ("%s\n", valueObject);
  return NULL;
}

int main () {
  AssociativeArray new assocArray;
  String new s1;
  String new s2;
  String new s3;

  WriteFileStream classInit;
  
  s1 = "string1 value";
  s2 = "string2 value";
  s3 = "string3 value";

  assocArray atPut "string1", s1;
  assocArray atPut "string2", s2;
  assocArray atPut "string3", s3;

  stdoutStream printOn ("%s\n%s\n%s\n\n", (assocArray at "string1"),
			(assocArray at "string2"),
			(assocArray at "string3"));
  stdoutStream printOn "%s\n%s\n%s\n\n", (assocArray at "string1"),
    (assocArray at "string2"),
    (assocArray at "string3");
  stdoutStream printOn "%s\n%s\n%s\n\n", assocArray at "string1",
    assocArray at "string2",
    assocArray at "string3";

  assocArray map printValue;

  assocArray mapKeys printKeyValue;

  return 0;
}
