
Integer new xWindowOrg, yWindowOrg, xWindowSize, yWindowSize;

int main () {
  Application new circleApp;
  X11Pane new xPane;
  X11PaneDispatcher new dispatcher;
  X11CanvasPane new canvas;
  InputEvent new e;
  Exception new ex;
  Circle new aCircle;

  xWindowOrg = 100;
  yWindowOrg = 100;
  xWindowSize = 200;
  yWindowSize = 200;

  xPane inputStream eventMask = WINDELETE|KEYPRESS|EXPOSE|RESIZENOTIFY;
  xPane initialize xWindowOrg, yWindowOrg, xWindowSize, yWindowSize;
  dispatcher attachTo xPane;
  canvas attachTo dispatcher;

  xPane map;
  xPane raiseWindow;

  xPane openEventStream;

  while (TRUE) {

    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;
      xPane subPaneNotify e;
      if (ex pending)
	ex handle;
      switch (e eventClass value)
	{
	case RESIZENOTIFY:
	  aCircle center x = canvas size x / 2;
	  aCircle center y = canvas size y / 2;
	  if (canvas size x > canvas size y) {
	    aCircle radius = canvas size y / 2;
	  } else {
	    aCircle radius = canvas size x / 2;
	  }
	  canvas paneBuffer background "blue";
	  canvas clear;
	  canvas drawCircle aCircle, false, "blue";
	  canvas refresh;
	  break;
	case WINDELETE:
	  xPane deleteAndClose;
	  exit (0);
	  break;
	}
    } else {
      circleApp uSleep 1000;
    }
  }
}
