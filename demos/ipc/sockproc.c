/* $Id: sockproc.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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
 *     sockproc.c
 *
 *     Starts a server process in the background which 
 *     uses a UNIXNetworkStreamReader object to read
 *     the system's clock time which is sent to it by
 *     a UNIXNetworkStreamWriter object created by
 *     the foreground process.
 *
 */

#define SOCK_BASENAME "testsocket.$$"

String new gSocketPath; /* The socket path is common to both
			   processes. */

Object instanceMethod startTimeServer (void) {
  UNIXNetworkStreamReader new reader;
  String new utcTimeString;
  CalendarTime new timeNow;
  SystemErrnoException new ex;

  reader openOn gSocketPath;
  if (ex pending) {
    ex handle;
    return ERROR;
  }

  while (1) {
    utcTimeString = reader sockRead;
    if (ex pending) {
      ex handle;
      return ERROR;
    }
    if (utcTimeString length > 0) {
      timeNow = utcTimeString asInteger;
      timeNow localTime;
      printf ("%s\n", timeNow cTimeString);
    }
  }
}

int main (int argc, char **argv) {

  CTime new thisTime;
  CTime new prevTime;
  UNIXNetworkStream new strObject;
  UNIXNetworkStreamWriter new client;
  Method new timeServerInit;
  Integer new childPID;
  SystemErrnoException new ex;
  Object new procRcvr;

  client enableExceptionTrace;

  gSocketPath = strObject makeSocketPath SOCK_BASENAME;

  printf ("gSocketPath: %s\n", gSocketPath);

  timeServerInit definedInstanceMethod "Object", 
    "startTimeServer";
  childPID = 
    procRcvr backgroundMethodObjectMessage timeServerInit;

  client openOn gSocketPath;
  if (ex pending) {
    ex handle;
    return ERROR;
  }

  prevTime utcTime;

  while (1) {

    thisTime utcTime;

    if (thisTime != prevTime) {

      client sockWrite (thisTime asString);

      if (ex pending) {
	ex handle;
	return ERROR;
      }
      
      prevTime = thisTime;

    }
    usleep (1000);
  }
  exit (0);
}

