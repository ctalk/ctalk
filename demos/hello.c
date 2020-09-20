/* $Id: hello.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 *   hello.c - Print "Hello, world!" in Ctalk.
 *
 *   To build the program:
 *
 *   UNIX:
 *     $ ctcc hello.c -o hello
 *
 *   DJGPP;
 *     > ctalk hello.c -o hello.i && gcc hello.i -o hello -lctalk
 */

int main () {
  /*
   *  Initialize stdoutStream;
   */
  WriteFileStream classInit;

  stdoutStream printOn "Hello, world!\n";

  exit (0);
}

