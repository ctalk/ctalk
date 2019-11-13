/* $Id: textinput.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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
 *   textinput.c - Demonstration of basic console formatting 
 *                 functions (DJGPP only).  For extended key
 *                 handling, see the Win32TerminalStream section
 *                 of the Ctalk Reference Manual.
 *
 *   To build the program:
 *     > ctalk textinput.c -o textinput.i && \
 *       gcc textinput.i -o textinput -lctalk
 *
 */


int main () {

  Win32TerminalStream new console;
  String new input;

  console screenColor ("white", "blue");
  console clear;

  console gotoXY 21, 10;
  console cPutStr "Win32TerminalStream Class Demonstration";
  console gotoXY 27, 12;
  console cPutStr "Please enter some text.";
  console gotoXY 27, 14;
  console screenColor ("white", "black");
  console gotoXY 27, 14;
  console cPutStr "                       ";
  console gotoXY 29, 14;
  input = console cGetStr; 
  console gotoXY 24, 16;
  console screenColor ("white", "blue");
  console cPutStr "You typed: "
  console gotoXY 36, 16;
  console cPutStr input;

  console gotoXY 1, 24;
}
