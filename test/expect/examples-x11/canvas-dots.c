int main () {
  X11Pane new xPane;
  InputEvent new e;
  X11PaneDispatcher new xTopLevelPane;
  X11CanvasPane new xCanvasPane;
  Application new paneApp;

  paneApp enableExceptionTrace;
  paneApp installExitHandlerBasic;

  xPane initialize 10, 10, 250, 250;
  xPane inputStream eventMask =        /* Tell the main window's event */
    WINDELETE|EXPOSE;                  /* object, a X11InputStream,    */
                                       /* which types of events we     */
                                       /* plan to use.                 */
  
  xTopLevelPane attachTo xPane;        /* The attachTo methods also */
  xCanvasPane attachTo xTopLevelPane;  /* set the dimensions of the */
                                       /* subpanes before they are  */
                                       /* mapped and raised along   */
                                       /* with the top-level pane.  */

  xPane map;
  xPane raiseWindow;
  xPane openEventStream;               /* Before we can do any      */
                                       /* drawing on the window, we */
                                       /* need to start sending and */
                                       /* receiving events from the */
                                       /* X server.  That is what   */
                                       /* openEventStream does.     */

  xPane background "yellow";           /* Setting the background of */
  xPane clearWindow;                   /* an X11Pane object sets the*/
                                       /* background of the actual  */
                                       /* window.                   */

  xCanvasPane background "yellow";     /* Setting the background of */
                                       /* a buffered pane like a    */
                                       /* X11CanvasPane sets the    */
                                       /* background color of its   */
                                       /* buffer.                   */

  xCanvasPane clearRectangle 0, 0, 250, 250; /* In both cases, we   */
                                             /* need to update the  */
                                             /* pane before the new */
                                             /* color is visible,   */
                                             /* with either,        */
                                             /* "clearWindow," or,  */
                                             /* "clearRectangle."   */
  xCanvasPane pen width = 100;
  xCanvasPane pen colorName = "red";
  xCanvasPane drawPoint 40, 40;
  xCanvasPane pen colorName = "green";
  xCanvasPane drawPoint 120, 40;
  xCanvasPane pen colorName = "blue";
  xCanvasPane drawPoint 80, 90;

  while (TRUE) {
    xPane inputStream queueInput;
    if (xPane inputStream eventPending) {
      e become xPane inputStream inputQueue unshift;
      xPane subPaneNotify e;          /* We need to notify subPanes */
                                      /* e.g., xCanvasPane of the   */
                                      /* input events from the GUI. */
      switch (e eventClass value)
        {
        case WINDELETE:
          xPane deleteAndClose;
          exit (0);
          break;
        case EXPOSE:
        case RESIZENOTIFY:
	  xCanvasPane pen width = 100;
	  xCanvasPane pen colorName = "red";
	  xCanvasPane drawPoint 40, 40;
	  xCanvasPane pen colorName = "green";
	  xCanvasPane drawPoint 120, 40;
	  xCanvasPane pen colorName = "blue";
	  xCanvasPane drawPoint 80, 90;
          break;
        default:
          break;
        }
    }
  }
}

