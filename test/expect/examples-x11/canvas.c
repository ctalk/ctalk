int main (int argv, char **argc) {
  X11Pane new xPane;
  X11PaneDispatcher new xTopLevelPane;
  X11CanvasPane new xCanvasPane;
  InputEvent new e;
  Integer new nEvents;
  Integer new verbose;
  Exception new ex;
  String new text;
  Application new paneApp;

  paneApp enableExceptionTrace;
  paneApp installExitHandlerBasic;

  xPane initialize 0, 0, 200, 100;
  xPane inputStream eventMask =
    WINDELETE|BUTTONPRESS|BUTTONRELEASE|MOVENOTIFY|EXPOSE;
  xTopLevelPane attachTo xPane;
  xCanvasPane attachTo xTopLevelPane;
  xPane map;
  xPane raiseWindow;

  xPane openEventStream;

  xCanvasPane clear;
  xCanvasPane background "blue";
  xCanvasPane pen width = 5;
  xCanvasPane pen colorName = "white";

  xCanvasPane refresh;

  verbose = FALSE;
  if (argc == 2) {
    if (!strcmp (argv[1], "-v")) {
      verbose = TRUE;
    }
  }

  WriteFileStream classInit;

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
	  if (verbose) {
	    stdoutStream printOn "Expose\t\t%d\t%d\t%d\t%d\t%d\n",
	      e xEventData1, 
	      e xEventData2, 
	      e xEventData3, 
	      e xEventData4,
	      e xEventData5;
	  }
	  break;
	case BUTTONPRESS:
	  xCanvasPane drawPoint e xEventData1, e xEventData2;
	  if (verbose) {
	    stdoutStream printOn "ButtonPress\t\t%d\t%d\t%d\t%d\t%d\n",
	      e xEventData1, 
	      e xEventData2, 
	      e xEventData3, 
	      e xEventData4,
	      e xEventData5;
	  }
	  xCanvasPane refresh;
	  break;
	case BUTTONRELEASE:
	  if (verbose) {
	    stdoutStream printOn "ButtonRelease\t\t%d\t%d\t%d\t%d\t%d\n",
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
