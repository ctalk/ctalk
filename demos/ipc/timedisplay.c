/* $Id: timedisplay.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

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
 *  timedisplay.c - send/receive signals between processes.
 *
 *  Sets the method SignalHandler : handleSignal as the
 *  handler for SIGUSR2 signals.  SignalHandler : handleSignal,
 *  when it is called by the system's signal, generates a
 *  SignalEvent object which is read by the SignalHandler :
 *  startTimeDisplay process, which has already set up
 *  sets up the signal handler and waits for SignalEvents
 *  created by the main process.
 */

SignalHandler instanceMethod handleSignal (__c_arg__ int signo) {
  char buf[2];
  noMethodInit;
  *buf = 0;
  __ctalkNewSignalEventInternal (signo, getpid (), buf);
  return NULL;
}

SignalEvent instanceMethod printTime (void) {

  CalendarTime new timeFromHandler;

  timeFromHandler utcTime;
  timeFromHandler localTime;
  printf ("%s\n", timeFromHandler cTimeString);

}

SignalHandler instanceMethod startTimeDisplay (void) {

  SignalHandler new s;
  SignalEvent new eventSelf;
  SignalEvent new event;

  s setSigUsr2;
  s installHandler handleSignal;

  while (pause()) {
    if (eventSelf pending) {
      /*
       *  The signal handler needs to be set 
       *  again after each usage.
       */
      s installHandler handleSignal;
      event = eventSelf nextEvent;
      event printTime;
      event delete;
    }
  }
}

int main (int argc, char **argv) {

  CTime new thisTime;
  CTime new prevTime;
  Method new timeDisplayInit;
  Integer new handlerProcessID;
  SignalHandler new s;
  Integer new child_retval, child_sig, child_errno, r;

  timeDisplayInit definedInstanceMethod "SignalHandler", 
    "startTimeDisplay";
  handlerProcessID =
    s backgroundMethodObjectMessage timeDisplayInit;

  s setSigUsr2;

  prevTime utcTime;

  while (1) {

    thisTime utcTime;

    if (thisTime != prevTime) {

      s signalProcessID handlerProcessID;
      prevTime = thisTime;

      /* Check for signals we don't handle. */
      r = s waitStatus handlerProcessID,
	child_retval, child_sig, child_errno;

      if (r == handlerProcessID) {
	if (child_sig) {
	  printf ("Child received signal %s - exiting.\n",
		  s sigName child_sig);
	  exit (1);
	}
      }

    }
    usleep (1000);
  }
  exit (0);
}

