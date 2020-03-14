/* $Id: xfthello.c,v 1.2 2020/02/29 12:42:55 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014, 2016, 2019 Robert Kiesling, rk3314042@gmail.com.
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
 *  xfthello.c  - FreeType2 version of xhello.c. 
 *
 *  If you build Ctalk with support for libxft (which is the default), 
 *  then you'll also need to configure the font libraries if you haven't
 *  already.  The X11FreeTypeFont section of the Ctalk Language Reference 
 *  contains information about how to enable FreeType font support.
 */

X11Pane instanceMethod putCenteredText (String text, Integer strWidth,
					Integer strHeight) {
  Integer new sizeX;
  Integer new sizeY;

  sizeX = self size x;
  sizeY = self size y;

  self clearWindow;

  self putStrXY (sizeX / 2) - (strWidth / 2), (sizeY / 2), text;
  return NULL;
}

int main (int argc, char **argv) {
  X11Pane new xPane;
  InputEvent new e;
  Integer new nEvents;
  Integer new strWidth;
  Integer new strHeight;
  Boolean new verbose;
  String new defaultFont;

  xPane backgroundColor = "white";
  xPane initialize 200, 100;
  xPane inputStream eventMask = WINDELETE|EXPOSE|MOVENOTIFY|RESIZENOTIFY;
  xPane map;
  xPane raiseWindow;

  xPane ftFontVar initFontLib;
  xPane ftFont "Nimbus Sans L", 0, 0, 0, 14.0;

  xPane openEventStream;

  /* When writing to an unbuffered pane; i.e., directly to a
     X11Pane instead of a X11Bitmap, using a method like 
     faceRegular binds the actual face to the pane's window. */
  xPane faceRegular;

  if (argc == 2) {
    if (!strcmp (argv[1], "-v")) {
      printf ("%s\n", xPane ftFontVar selectedFont);
      verbose = true;
    }
  }

  WriteFileStream classInit;

  strWidth = xPane ftFontVar textWidth "Hello, world!";
  strHeight = xPane ftFontVar textHeight "Hello, world!";
  xPane putCenteredText "Hello, world!", strWidth, strHeight;

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
	  xPane putCenteredText "Hello, world!", strWidth, strHeight;
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
	  xPane putCenteredText "Hello, world!", strWidth, strHeight;
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
	  xPane putCenteredText "Hello, world!", strWidth, strHeight;
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
