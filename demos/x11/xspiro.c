/* $Id: xspiro.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

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
 *  xspiro.c - Create spirograph-like drawings.  Compatible with 
 *  Linux and OS X platforms.  The X11 libs need some compatibility
 *  updates for Solaris platforms.
 *
 *   Build the program with a command like the following.
 *
 *    $ ctcc -m xspiro.c -o xspiro
 *
 *   The -m option links the program with the system's math libraries, 
 *   which implement the sin and cos functions.
 *
 *   "xspiro -h" prints the command line options.
 *
 *   The drawings are two epicycloids, with points
 *   defined by the following equations.
 *
 *    x = (R+r)*cos(t) - (r + s) * cos(((R+r)/r)*t)
 *    y = (R+r)*sin(t) - (r + s) * sin(((R+r)/r)*t)
 *
 *    R = Radius of fixed circle.
 *    r = Radius of moving circle.
 *    s = Offset of pen in moving circle.
 *
 */

#include <math.h>
#include <stdlib.h>

/* From x11keysymdefs.h */
#define XK_Escape  65307

Float new rFixed;
Float new rMoving;

String new fgColor;
String new bgColor;

int parse_args (int, char **);

Float instanceMethod plotX (Float rTotal, Float r, Integer s) {
  int s_int, x;
  double r_total, r_double, t_double;

  returnObjectClass Integer;

  r_total = rTotal value;
  r_double = r value;
  s_int = s value;
  t_double = self value;

  x = (int)r_total*cos(t_double) - 
    (r_double + s_int) * cos((double)(r_total/r_double)*t_double);

  methodReturnInteger(x);
}

Float instanceMethod plotY (Float rTotal, Float r, Integer s) {
  int s_int, y;
  double r_total, r_double, t_double;

  returnObjectClass Integer;

  r_total = rTotal value;
  r_double = r value;
  s_int = s value;
  t_double = self value;

  y = (int)r_total*sin(t_double) - 
    (r_double + s_int) * sin((double)(r_total/r_double)*t_double);
  methodReturnInteger(y);
}

int main (int argv, char **argc) {
  Float new t;
  Integer new s;
  Integer new x;
  Integer new y;
  Float new rTotal;
  Integer new xCenterOffset;
  Integer new yCenterOffset;
  Integer new xWindowSize;
  Integer new yWindowSize;
  X11Pane new xPane;
  X11PaneDispatcher new xTopLevelPane;
  X11CanvasPane new xCanvasPane;
  InputEvent new e;
  Integer new nEvents;
  Exception new ex;
  Application new xspiro;

  xspiro enableExceptionTrace;
  xspiro installExitHandlerBasic;
  xspiro installAbortHandlerBasic;

  xWindowSize = 400;
  yWindowSize = 400;
  xCenterOffset = xWindowSize / 2;
  yCenterOffset = yWindowSize / 2;

  rFixed = 90.0;
  rMoving = 53.5;

  fgColor = "white";
  bgColor = "blue";

  parse_args (argc, argv);

  xPane inputStream eventMask = WINDELETE|KEYPRESS;
  xPane initialize xWindowSize, yWindowSize;
  xTopLevelPane attachTo xPane;
  xCanvasPane attachTo xTopLevelPane;

  xPane map;
  xPane raiseWindow;

  xCanvasPane pen width = 1;
  xCanvasPane pen colorName = fgColor;
  xCanvasPane pen alpha = 0x8888;

  xPane openEventStream;

  xCanvasPane background bgColor;
  xCanvasPane clear;

  rTotal = rFixed + rMoving;

  t = 0.0;
  s = 0;

  while (TRUE) {

    x = t plotX rTotal, rMoving, s;
    y = t plotY rTotal, rMoving, s;
    xCanvasPane drawPoint x + xCenterOffset, y + yCenterOffset;
    xCanvasPane refresh;
    t = t + 0.005;

    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;
      if (ex pending)
  	ex handle;
      switch (e eventClass value)
 	{
	case KEYPRESS:
	  if (e xEventData5 value == XK_Escape) {
	    xPane deleteAndClose;
	    exit (0);
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

void exit_help (char *cmd) {
  printf ("\nUsage: %s [-h] | [-f n] | [-m n]\n", cmd);
  printf ("-bg <color>  Background color.\n");
  printf ("-fg <color>  Foreground color.\n");
  printf ("-f n         Size of fixed circle (default is 90.0).\n");
  printf ("-h           Print this message and exit.\n");
  printf ("-m n         Size of moving circle (default is 53.5).\n\n");
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
	  bgColor = a[++i];
	  break;
	case 'h':
	  exit_help (a[0]);
	  break;
	case 'f':
	  if (a[i][2] == 'g') {
	    fgColor = a[++i];
	  } else {
	    rFixed = strtod (a[++i], (char **)NULL);
	  }
	  break;
	case 'm':
	  rMoving = strtod (a[++i], (char **)NULL);
	  break;
	}
    } else {
      exit_help (a[0]);
    }
  }
}
