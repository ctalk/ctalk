int main () {
  X11Pane new xPane;
  InputEvent new e;
  Pen new bluePen;
  Rectangle new rectangle;
  Rectangle new rectangle2;

  xPane initialize 10, 10, 100, 100;
  xPane inputStream eventMask = WINDELETE|EXPOSE;
  xPane map;
  xPane raiseWindow;
  xPane openEventStream;

  /*  
   *  The rectangle's sides are four Line objects:
   *  top, bottom, left, and right.  There is also
   *  a "dimensions" method that fills in all of 
   *  the sides' dimensions.
   */
  rectangle top start x = 10;
  rectangle top start y = 10;
  rectangle right end x = 80;
  rectangle right end y = 80;

  rectangle2 dimensions 40, 40, 40, 40;

  bluePen width = 3;
  bluePen colorName = "blue";

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
	  /*
	   *  To draw only the outline of the rectangle,
	   *  use the "draw" method instead.
	   */
	  rectangle fillWithPen xPane, bluePen;
	  rectangle2 clear xPane;
          break;
        default:
          break;
        }
    }
  }
}

