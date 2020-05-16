/* $Id: testmethod.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2008-2011 Robert Kiesling, rk3314042@gmail.com.
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

AssociativeArray instanceMethod printEnvVar (void) {
  String new keyName;
  String new keyValue;
  /*
   *  Each member of an AssociativeArray is a Key object,
   *  so the actual receiver of a method called inline
   *  by "map" (class AssociativeArray) is a Key.
   *
   *  The method, "name" (class Object) creates an object
   *  as a return value. The method needs to store it
   *  in keyName so the program can delete the object when
   *  it is no longer needed.  Ctalk still does not, at this
   *  time, use "self" in some C expressions, so the method stores 
   *  the result of the "getValue" method (class Key) in keyValue.
   */
  keyName = self name;
  printf ("<tt>%s=", keyName);
  keyValue = self getValue;
  printf ("\"%s\"</tt><br>\n", keyValue);
  return NULL;
}

