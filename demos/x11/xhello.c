/* $Id: xhello.c,v 1.2 2020/02/29 12:22:21 rkiesling Exp $ */

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
 *  xhello.c
 *
 *  Demonstration of how to get X11Pane objects to respond
 *  to events from a X server; in this case, ConfigureNotify
 *  and Expose events, which are generated when the window is 
 *  moved, resized, or uncovered.
 *
 *  X11Pane objects define only a default fixed-width font
 *  in this Ctalk version, so this program can calculate
 *  text dimensions without consulting the X server.
 *  Instead, the macros below should be correct for most
 *  X Window systems.
 */

#define FIXED_CHAR_WIDTH 8
#define FIXED_CHAR_HEIGHT 12

X11Pane instanceMethod putCenteredText (String text) {
  Integer new sizeX;
  Integer new sizeY;
  Integer new textCharWidth;
  Integer new textXSize;
  Integer new textYSize;

  sizeX = self size x;
  sizeY = self size y;
  textCharWidth = text length;

  textXSize = textCharWidth * FIXED_CHAR_WIDTH;
  textYSize = FIXED_CHAR_HEIGHT;

  self clearWindow;

  self putStrXY (sizeX / 2) - (textXSize / 2), 
    (sizeY / 2) - (textYSize / 2), 
    text;
  return NULL;
}

int main (int argv, char **argc) {
  X11Pane new xPane;
  InputEvent new e;
  Integer new nEvents;
  Integer new verbose;

  xPane backgroundColor = "white";
  
  xPane initialize 200, 100;
  xPane map;
  xPane raiseWindow;

  xPane openEventStream;

  xPane font "fixed";
  
  verbose = FALSE;
  if (argc == 2) {
    if (!strcmp (argv[1], "-v")) {
      verbose = TRUE;
    }
  }

  if (verbose)
    printf ("Actual font: %s\n", xPane fontVar fontDesc);

  WriteFileStream classInit;

  xPane putCenteredText "Hello, world!";

  while (TRUE) {
    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;
      switch (e eventClass value) 
        {
          /*
           *  Handle both types of events in case the window
           *  manager doesn't distinguish between them.
           */
        case MOVENOTIFY:
          xPane putCenteredText "Hello, world!";
          if (verbose) {
            stdoutStream printOn "MOVENOTIFY\t%d\t%d\t%d\t%d\n",
              e xEventData1, 
              e xEventData2, 
              e xEventData3, 
              e xEventData4;
            stdoutStream printOn "Window\t\t%d\t%d\t%d\t%d\n",
              xPane origin x, 
              xPane origin y, 
              xPane size x,
              xPane size y;
          }
          break;
        case RESIZENOTIFY:
          xPane putCenteredText "Hello, world!";
          if (verbose) {
            stdoutStream printOn "RESIZENOTIFY\t%d\t%d\t%d\t%d\n",
              e xEventData1, 
              e xEventData2, 
              e xEventData3, 
              e xEventData4;
            stdoutStream printOn "Window\t\t%d\t%d\t%d\t%d\n",
              xPane origin x, 
              xPane origin y, 
              xPane size x,
              xPane size y;
          }
          break;
        case EXPOSE:
          xPane putCenteredText "Hello, world!";
          if (verbose) {
            stdoutStream printOn "Expose\t\t%d\t%d\t%d\t%d\t%d\n",
              e xEventData1, 
              e xEventData2, 
              e xEventData3, 
              e xEventData4,
              e xEventData5;
          }
          break;
        case WINDELETE:
          xPane deleteAndClose;
          exit (0);
          break;
        default:
          break;
        }
    }
  }
}
