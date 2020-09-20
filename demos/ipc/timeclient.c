/* $Id: timeclient.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
 *  timeclient.c - Signal handler that tells the application to 
 *  print the time and date on the terminal.  
 *
 *  Use with timesignal.c to signal this program's process ID.
 *  Launch this program either in the background or on another
 *  terminal.
 *
 *  $ ctcc timeclient.c
 *  $ ctcc timesignal.c
 *  $ ./timeclient &
 *  [1] <PID>
 *  $ ./timesignal <PID>
 */

SignalHandler instanceMethod handleSignal (__c_arg__ int signo) {
  time_t t;
  char buf[MAXLABEL];
  noMethodInit;
  t = time (NULL);
  __ctalkDecimalIntegerToASCII (t, buf);
  __ctalkNewSignalEventInternal (signo, getpid (), buf);
  return NULL;
}

SignalEvent instanceMethod printTime (void) {

  CTime new timeFromHandler;
  String new timeString;

  WriteFileStream classInit;

  timeFromHandler = self text asInteger;
  timeString = timeFromHandler cTime;
  stdoutStream writeStream timeString;

  return NULL;
}

int main () {

  SignalHandler new s;
  SignalEvent new eventSelf;
  SignalEvent new event;

  s setSigUsr2;
  s getPID;
  s installHandler handleSignal;

  while (pause()) {
    if (eventSelf pending) {
      event = eventSelf nextEvent;
      event printTime;
      /*
       *  The signal handler needs to be set 
       *  again after each usage.
       */
      s installHandler handleSignal;
    }
  }
}
