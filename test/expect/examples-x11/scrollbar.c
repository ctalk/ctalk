/* $Id: scrollbar.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2014 Robert Kiesling, rk3314042@gmail.com.
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

#include <stdlib.h>

int parse_args (int, char **);

String new fgColor;
String new bgColor;

int main (int argv, char **argc) {
  Integer new xWindowSize;
  Integer new yWindowSize;
  X11Pane new xPane;
  X11PaneDispatcher new xTopLevelPane;
  X11ScrollBarPane new xScrollBarPane;
  InputEvent new e;
  Integer new nEvents;
  Exception new ex;
  Application new scrollDemo;

  scrollDemo enableExceptionTrace;
  scrollDemo installExitHandlerBasic;
  scrollDemo installAbortHandlerBasic;

  xWindowSize = 400;
  yWindowSize = 400;

  fgColor = "white";
  bgColor = "white";

  parse_args (argc, argv);

  xPane initialize xWindowSize, yWindowSize;
  xPane inputStream eventMask = WINDELETE|EXPOSE|BUTTONPRESS|BUTTONRELEASE|MOTIONNOTIFY;
  xTopLevelPane attachTo xPane;
  xScrollBarPane attachTo xTopLevelPane;

  xPane map;
  xPane raiseWindow;

  /*
   *  When animating, the old position of the scroll thumb
   *  normally is set to the scrollbar's background color.
   */
  xScrollBarPane thumbErasePen colorName = bgColor;

  xPane openEventStream;

  xScrollBarPane background bgColor;
  xScrollBarPane foreground fgColor;
  xScrollBarPane clear;

  while (TRUE) {

    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;

      if (ex pending)
  	ex handle;

      xPane subPaneNotify e;

      switch (e eventClass value)
 	{
	case EXPOSE:
	  xScrollBarPane refresh;
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

void exit_help (char *cmd) {
  printf ("\nUsage: %s [-h] | [-f n] | [-m n]\n", cmd);
  printf ("-bg <color>  Background color.\n");
  printf ("-fg <color>  Foreground color.\n");
  printf ("-h           Print this message and exit.\n");
  printf ("Please report bugs to: rk3314042@gmail.com.\n");
  exit (0);
}

int parse_args (int c, char **a) {
  int i;
  for (i = 1; i < c; i++) {
    if (a[i][0] == '-') {
      switch (a[i][1])
	{
	case 'b':
	  /*** TODO! Braces are necessary to Fix framing error. */
	  {
	    bgColor = a[++i];
	  }
	  break;
	case 'h':
	  exit_help (a[0]);
	  break;
	case 'f':
	  if (a[i][2] == 'g') {
	    fgColor = a[++i];
	  }
	  break;
	}
    } else {
      exit_help (a[0]);
    }
  }
}
