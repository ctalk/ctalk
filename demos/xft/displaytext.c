/* $Id: displaytext.c,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $ */

/*
  This file is part of Ctalk.
  Copyright © 2016, 2017 Robert Kiesling, rk3314042@gmail.com.
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
 *  displaytext.c - Display large text.
 *
 *  The font sizes are set smaller than some systems are capable
 *  of displaying so the demo can run on as many different machines
 *  as possible.  In many cases, the program can display larger
 *  type sizes - to set the size of Xlib fonts, adjust this line:
 *
 *    xTextPane font "-*-helvetica-*-*-*-*-*-240-*-*-*-*-*-*"
 *
 *  and the program sets its FreeType font with this line.
 *
 *    xTextPane ftFontVar selectFont "URW Gothic L", 0, 0, 0, 36.0;
 */

Integer new useFtFont;

#define TEXT "Big Type"
#define SHADOW "black"
#define WINWIDTH 300
#define WINHEIGHT 200

X11TextPane instanceMethod displayXftText (void) {
  Integer new textWidth, textHeight, textX, textY;
  self faceBold;
  self ftFontVar namedX11Color SHADOW;
  textHeight = self ftFontVar textHeight TEXT;
  textWidth = self ftFontVar textWidth TEXT;
  textX = (WINWIDTH / 2) - (textWidth / 2);
  textY = (WINHEIGHT / 2) - (textHeight / 2);
  textY += textHeight * .4; /* Move the center of the text toward
			       the window's horizontal center. */
  (X11Bitmap *)self paneBuffer putStr textX + 2, textY + 2, TEXT;
  self ftFontVar namedX11Color "blue";
  (X11Bitmap *)self paneBuffer putStr textX, textY, TEXT;
}

X11TextPane instanceMethod displayXFontText (String fontDesc) {
  Integer new width, height, textX, textY;
  X11Font new font;
  font getFontInfo fontDesc;
  width = font textWidth TEXT;
  height = 24;   /* From the font descriptor, below. */
  textX = (WINWIDTH / 2) - (width / 2);
  textY = (WINHEIGHT / 2) - (height / 2);
  textY += height * .4;
  self foreground "blue";
  (X11Bitmap *)self paneBuffer putStr textX, textY, TEXT;
}

int main (int argv, char **argc) {
  X11Pane new xPane;
  X11PaneDispatcher new xTopLevelPane;
  X11TextPane new xTextPane;
  InputEvent new e;
  Integer new nEvents;
  Integer new verbose;
  Exception new ex;
  String new text;

  verbose = FALSE;
  useFtFont = FALSE;

  if (argc >= 2) {
    int i;
    for (i = 1; i < argc; i++) {
      if (!strcmp (argv[i], "-v")) {
	verbose = TRUE;
      } else {
	if (!strcmp (argv[i], "-f")) {
	  useFtFont = TRUE;
	} else {
	  printf ("usage: displaytext [-v] [-f]\n");
	  printf ("-f      Use FreeType fonts if available.\n");
	  printf ("-v      Display the font descriptor.\n");
	  exit (1);
	}
      }
    }
  }

  if (useFtFont) {
    xTextPane ftFontVar initFontLib;
    xTextPane ftFontVar selectFont "URW Gothic L", 0, 0, 0, 36.0;
    xTextPane ftFontVar namedX11Color SHADOW;
  }

  xPane initialize 25, 30, WINWIDTH, WINHEIGHT;
  xTopLevelPane attachTo xPane;
  xTextPane attachTo xTopLevelPane;
  xPane map;
  xPane raiseWindow;

  xPane openEventStream;

  xPane setWMTitle "Display Text Demo";

  if (!useFtFont) {
    xTextPane foreground SHADOW;
    xTextPane font "-*-helvetica-*-*-*-*-*-240-*-*-*-*-*-*";
  } else {
    xTextPane faceBold;
  }


  if (verbose) {
    if (useFtFont) {
      printf ("Font: %s\n", xTextPane ftFontVar selectedFont);
    } else {
      printf ("Font: %s\n", xTextPane fontDescStr);
    }
  }

  xTextPane background "white";
  xTextPane clear;

  if (useFtFont) {
    xTextPane displayXftText;
  } else {
    xTextPane displayXFontText "-*-helvetica-*-*-*-*-*-240-*-*-*-*-*-*";
  }

  xTextPane refresh;

  while (TRUE) {
    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;
      xPane subPaneNotify e;
      if (ex pending)
 	ex handle;
      switch (e eventClass value) 
	{
	case EXPOSE:
	  if (!useFtFont) {
	    xTextPane displayXFontText "-*-helvetica-*-*-*-*-*-240-*-*-*-*-*-*";
	  } else {
	    xTextPane displayXftText;
	  }
	  xTextPane refresh;
	  break;
	case WINDELETE:
 	  xPane deleteAndClose;
	  exit (0);
	  break;
	default:
	  break;
	}
    } else {
      if (xTextPane requestClose) {
	xPane deleteAndClose;
	exit (0);
      }
    }
  }
}
