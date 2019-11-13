/* $Id: basiclist.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
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

List instanceMethod printListElement (void) {
  /*
   *  How to handle an Integer receiver when the method 
   *  doesn't know in advance what class "self" is.
   */
  Integer new element;
  element = self;
  printf ("%d ", element);
  return NULL;
}

int main () {

  List new l;
  Integer new i1;
  Integer new i2;
  Integer new i3;
  Integer new i4;
  Integer new i5;
  Integer new i6;

  i1 = 1;
  printf ("%d ", i1);
  i2 = 2;
  printf ("%d ", i2);
  i3 = 3;
  printf ("%d ", i3);
  printf ("\n");

  l push i1;
  l push i2;
  l push i3;

  i4 = l pop;
  printf ("%d ", i4);
  i5 = l pop;
  printf ("%d ", i5);
  i6 = l pop;
  printf ("%d ", i6);
  printf ("\n");

  l shift i1;
  printf ("%d ", i1);
  l shift i2;
  printf ("%d ", i2);
  l shift i3;
  printf ("%d ", i3);
  printf ("\n");

  i4 = l unshift;
  printf ("%d ", i4);
  i5 = l unshift;
  printf ("%d ", i5);
  i6 = l unshift;
  printf ("%d ", i6);
  printf ("\n");

  l shift i1;
  l shift i2;
  l shift i3;
  l shift i4;
  l shift i5;
  l shift i6;
  l map printListElement;
  printf ("\n");

  return 0;
}
