int main () {
  X11Pane new xPane;
  InputEvent new e;
  X11PaneDispatcher new xTopLevelPane;
  X11CanvasPane new xCanvasPane;
  Application new paneApp;

  paneApp enableExceptionTrace;
  paneApp installExitHandlerBasic;

  xPane initialize 10, 10, 100, 100;
  xPane inputStream eventMask = WINDELETE;
  xTopLevelPane attachTo xPane;
  xCanvasPane attachTo xTopLevelPane;
  xPane map;
  xPane raiseWindow;
  xPane openEventStream;

  xCanvasPane pen width = 10;
  xCanvasPane pen colorName = "blue";
  xCanvasPane drawRectangle 10, 10, 80, 80;

  xCanvasPane clearRectangle 50, 50, 50, 50;
  xCanvasPane refresh;

  while (TRUE) {
    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;
      switch (e eventClass value)
        {
        case WINDELETE:
          xPane deleteAndClose;
          exit (0);
          break;
        case EXPOSE:
        case RESIZENOTIFY:
	  xCanvasPane drawRectangle 10, 10, 80, 80;
	  xCanvasPane clearRectangle 50, 50, 50, 50;
	  xCanvasPane refresh;
          break;
        default:
          break;
        }
    }
  }
}

