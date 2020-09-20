/* $Id: scancode.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
 *     scancode.c - Print the BIOS shift state and scan code
 *                  of a user keypress.  The shift state 
 *                  is in the Win32TerminalStream object's 
 *                  shiftState instance variable after 
 *                  receiving the biosKey message.  
 */        
int main () {
  Win32TerminalStream new term;
  Integer new scanCode;

  term clear;

  /* 
   *  Loop until finished.   The biosKey method
   *  waits for keystrokes.  The system's BIOS 
   *  generates an INTR signal for Ctrl-C so the 
   *  program doesn't have to monitor the keys to 
   *  exit the program. 
   */
  while (1) {
    scanCode = term biosKey;
    term printOn "%d : %d\r\n", term shiftState, scanCode;
  }
}
