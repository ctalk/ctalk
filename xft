Integer new useFtFont;

void exit_help () {
  printf ("usage: x11textpane [-h] | [-v] [-f] [-r <filename> ]\n");
  printf ("-f             Use FreeType fonts if available.\n");
  printf ("-h             Print this message and exit.\n");
  printf ("-r <filename>  Read <filename> and display its contents.\n");
  printf ("-v             Display the font descriptor.\n");
  exit (1);
}

int main (int argc, char **argv) {
  X11Pane new xPane;
  X11PaneDispatcher new xTopLevelPane;
  X11TextPane new xTextPane;
  InputEvent new e;
  Integer new nEvents;
  Integer new verbose;
  Exception new ex;
  String new text;
  String new fileName;
  X11Cursor new watchCursor;
  Application new textPaneApp;
  ReadFileStream new readFile;
  Integer new i;

  verbose = FALSE;
  useFtFont = FALSE;
  fileName = "";

  textPaneApp parseArgs argc, argv;

  for (i = 0; i < textPaneApp cmdLineArgc; ++i) {
    if (textPaneApp cmdLineArgs at i == "-f") {
      useFtFont = TRUE;
    } else if (textPaneApp cmdLineArgs at i == "-v") {
      verbose = TRUE;
    } else if (textPaneApp cmdLineArgs at i == "-h") {
      exit_help ();
    } else if (textPaneApp cmdLineArgs at i == "-r") {
      ++i;
      fileName = textPaneApp cmdLineArgs at i;
    }
  }

  if (useFtFont)
    xTextPane ftFontVar initFontLib;

  xPane initialize 25, 30, 500, 340;
  xTopLevelPane attachTo xPane;
  xTextPane attachTo xTopLevelPane;
  xPane map;
  xPane raiseWindow;
  watchCursor watch;

  xPane openEventStream;

  if (fileName length > 0) {
    xPane setWMTitle fileName;
  } else {
    xPane setWMTitle "X11TextPane Demo";
  }

  if (!useFtFont) {
    xTextPane foreground "darkslategray";
    xTextPane font "-*-helvetica-*-*-*-*-14-140-*-*-*-*-*-*";
  } else {
    xTextPane ftFontVar namedX11Color "black";
    xTextPane ftFontVar selectFontFromXLFD
    "-*-DejaVu Sans-medium-r-*-*-12-72-72-*-*-*-*-*";
  }

  if (verbose) {
    if (useFtFont) {
      printf ("Font: %s\n", xTextPane ftFontVar selectedFont);
    } else {
      printf ("Font: %s\n", xTextPane fontDescStr);
    }
  }

  WriteFileStream classInit;

  xTextPane background "white";
  xTextPane clear;

  if (fileName length > 0) {
    readFile openOn fileName;
    text = readFile readAll;
    readFile closeStream;
  } else {
    text =  "NAME\n";
    text += "<center>X11TextPane Demo</center>\n";
    text += "\n";
    text += "SYNOPSIS\n";
    text += "x11textpane [-f] [-v] [-r <i>filename</i>]\n\n";
    text += "DESCRIPTION\n";
    text += "Objects of <b>X11TextPane</b> class display multiple lines ";
    text += "of text in a X window.\n";
    text += "\n";
    text += "The <b>X11TextPane</b> class provides the following commands ";
    text += "to move around the text.\n";
    text += "\n";
    text += "<i>j</i> | <i>Control-N</i> | <i>Up</i> arrow\n";
    text += "          Scroll the text down by one line.\n\n";
    text += "<i>k</i> | <i>Control-P</i> | <i>Down</i> arrow.\n";
    text += "          Scroll the text up by one line.\n\n";
    text += "<i>Control-V</i>\n";
    text += "          Scroll down one screenful.\n\n";
    text += "<i>Control-T</i>\n";
    text += "          Scroll up one screenful.\n\n";
    text += "<i>Control-Q</i>\n";
    text += "          Go to the start of the text\n\n";
    text += "<i>Control-Z</i>\n";
    text += "          Go to the end of the text\n\n";
    text += "To close the window, press <i>Escape,</i> or, if the desktop ";
    text += "supports it, select the, <i>\"Close Window,\"</i> option on ";
    text += "the window's title bar.";
    text += "\n\n";
    text += "<b><center>Displaying Text</center></b>\n\n";
    text += "The <b>X11TextPane</b> class defines several methods to add text:\n";
    text += "\n";
    text += "<b>addText</b> - Formats normal text to fit within the window's ";
    text += "viewing area.  The text can be any length and contain any ";
    text += "number of lines.\n\n";
    text += "<b>putStr</b> - Takes a <i>string</i> and <i>x, y</i> coordinates ";
    text += "as its ";
    text += "arguments, and draws the text at that position in the window. ";
    text += "The program must then call <b>refresh</b> in order ";
    text += "for the text to be visible.";
    text += "\n\n";
    text += "<b>printOn</b> - Prints a formatted string at the location ";
    text += "given by the arguments.  The method formats the text using ";
    text += "a <i>printf</i> style format string and arguments.  When ";
    text += "displaying text with this function, the program must call ";
    text += "the <b>refresh</b> method for the text to be visible.\n\n";
    text += "<b><center>Formatting Text</center></b>\n\n";
    text += "A X11TextPane object recognizes some basic formatting commands.\n\n";
    text += "\\<b>, \\</b>\n";
    text += "          Begin and end <b>bold</b> type.\n";
    text += "\\<i>, \\</i>\n";
    text += "          Begin and end <i>italic</i> type.\n";
    text += "\\<center>, \\</center>\n";
    text += "          Begin and end centered text.\n\n";
  }

  xTextPane faceRegular;
  xPane useCursor watchCursor;
  xTextPane addText text;
  xTextPane displayText;
  xPane defaultCursor;

  while (TRUE) {
    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;
      xPane subPaneNotify e;
      if (ex pending)
 	ex handle;
      switch (e eventClass value) 
	{
	  /*
	   *  Handle both types of events in case the window
	   *  manager doesn't distinguish between them.
	   */
	case MOVENOTIFY:
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
	  /* if (verbose) {
	    stdoutStream printOn "Expose\t\t%d\t%d\t%d\t%d\t%d\n",
	      e xEventData1, 
	      e xEventData2, 
	      e xEventData3, 
	      e xEventData4,
	      e xEventData5;
	      } */
#if 0
	  xTextPane gotoXY 100, 50;
	  xTextPane putStr "Your text here.";
	  xTextPane refresh;
#endif
	  break;
	case KEYPRESS:
	  break;
	case KEYRELEASE:
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
