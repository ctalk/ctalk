int main () {
  X11Pane new xPane;
  InputEvent new e;
  X11PaneDispatcher new xTopLevelPane;
  X11CanvasPane new xCanvasPane;
  Application new paneApp;
  Circle new inner;
  Circle new outer;
  Circle new middle;
  Pen new innerPen;
  Pen new middlePen;
  Pen new outerPen;
  String new bgColor;

  paneApp enableExceptionTrace;
  paneApp installExitHandlerBasic;

  bgColor = "white";

  xPane initialize 10, 10, 300, 300;
  xPane inputStream eventMask = WINDELETE;
  xTopLevelPane attachTo xPane; 
  xCanvasPane attachTo xTopLevelPane;
  xPane map;
  xPane raiseWindow;

  xPane openEventStream;

  xPane clearWindow;
  xCanvasPane background bgColor;

  inner center x = 150;
  inner center y = 150;
  inner radius = 30;

  innerPen colorName = "navy";
  innerPen width = 10;

  middle center x = 150;
  middle center y = 150;
  middle radius = 40;

  middlePen colorName = "blue";
  middlePen width = 10;

  outer center x = 150;
  outer center y = 150;
  outer radius = 50;

  outerPen colorName = "sky blue";
  outerPen width = 10;

  xCanvasPane pen width = 1;
  xCanvasPane pen colorName = "black";

  xCanvasPane drawCircle outer, outerPen, FALSE, bgColor;
  xCanvasPane drawCircle middle, middlePen, FALSE, bgColor;
  xCanvasPane drawCircle inner, innerPen, FALSE, bgColor;
  xCanvasPane drawLine 50, 150, 250, 150;
  xCanvasPane drawLine 150, 50, 150, 250;
  xCanvasPane refresh;

  while (TRUE) {
    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;
      xPane subPaneNotify e;
      switch (e eventClass value)
        {
        case WINDELETE:
          xPane deleteAndClose;
          exit (0);
          break;
        case EXPOSE:
        case RESIZENOTIFY:
	  xCanvasPane drawCircle outer, outerPen, TRUE, bgColor;
	  xCanvasPane drawCircle middle, middlePen, TRUE, bgColor;
	  xCanvasPane drawCircle inner, innerPen, TRUE, bgColor;
	  xCanvasPane drawLine 50, 150, 250, 150;
	  xCanvasPane drawLine 150, 50, 150, 250;
	  xCanvasPane refresh;
          break;
        default:
          break;
        }
    }
  }
}

