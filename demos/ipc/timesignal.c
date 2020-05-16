/* $Id: timesignal.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2014 Robert Kiesling, rk3314042@gmail.com.
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
 *  timesignal.c - Signal the timeclient program to print  
 *  the date and time on the terminal.
 *  
 *  Launch timeclient either in the background or on another
 *  terminal.
 *
 *  $ ctcc timeclient.c -o timeclient
 *  $ ctcc timesignal.c -o timesignal
 *  $ ./timeclient &
 *  [1] <PID>
 *  $ ./timesignal <PID>
 */

int pid;

int main (int argc, char **argv) {

  CTime new thisTime;
  CTime new prevTime;
  Integer new handlerProcessId;
  SignalHandler new s;

  if (argc != 2) {
    printf ("Usage: timesignal <pid>\n");
    exit (1);
  }

  handlerProcessId = atoi (argv[1]);

  s setSigUsr2;

  prevTime = 0;

  while (1) {
    thisTime utcTime;
    if (thisTime != prevTime) {
      s signalProcessID handlerProcessId;
      prevTime = thisTime;
    }
  }
}

