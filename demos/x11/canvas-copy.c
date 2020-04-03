#include "coffee-cup.xpm"

/* Sometimes we still need to do this.... */
/* #define TEST_INSTANCEVAR_EXPRS */

/* 
   Set these to the width and height of your pixmap,
   and edit the pixmapFromData expression below to
   the xpm's declaration name. 
*/
#define XPM_WIDTH 127
#define XPM_HEIGHT 141

X11CanvasPane instanceMethod drawXPMs (X11Bitmap xpmBitmap) {
  Integer new i;
  for (i = 0; i < 5; i++) {
#ifdef TEST_INSTANCEVAR_EXPRS
    __ctalkX11CopyPixmapBasic (self displayPtr, self xWindowID, self xGC,
			       xpmBitmap xID, 0, 0, XPM_WIDTH, XPM_HEIGHT, 
			       (i *40), (i * 40));
#else

    self copy xpmBitmap, 0, 0, XPM_WIDTH, XPM_HEIGHT, (i* 40), (i * 40);

#endif


  }

  self refresh;

}

int main () {
  X11Pane new xPane;
  InputEvent new e;
  X11PaneDispatcher new xTopLevelPane;
  X11CanvasPane new xCanvasPane;
  Application new paneApp;
  X11Bitmap new srcBitmap;

  paneApp enableExceptionTrace;
  paneApp installExitHandlerBasic;

  xPane initialize 10, 10, 300, 300;
  xPane inputStream eventMask = WINDELETE;
  xTopLevelPane attachTo xPane;
  xCanvasPane attachTo xTopLevelPane;

  srcBitmap create xCanvasPane displayPtr, xCanvasPane xWindowID,
    XPM_WIDTH, XPM_HEIGHT, xCanvasPane depth;

  xPane map;
  xPane raiseWindow;
  xPane openEventStream;

  xCanvasPane background "white";

  srcBitmap pixmapFromData (0, 0, coffee_cup);

#ifdef TEST_INSTANCEVAR_EXPRS
  /* These are still here in case we want to test instancevar
     expressions as function arguments. */
  __ctalkX11CopyPixmapBasic (xCanvasPane displayPtr,
			     xCanvasPane xWindowID, xCanvasPane xGC,
			     srcBitmap xID, 0, 0, 
			     XPM_WIDTH, XPM_HEIGHT, 0, 0);
  __ctalkX11CopyPixmapBasic (xCanvasPane displayPtr,
			     xCanvasPane xWindowID, xCanvasPane xGC,
			     srcBitmap xID, 0, 0, 
			     XPM_WIDTH, XPM_HEIGHT, 25, 25);
#else

  xCanvasPane drawXPMs srcBitmap;

#endif

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
#ifdef TEST_INSTANCEVAR_EXPRS
	  /* ditto */
	  __ctalkX11CopyPixmapBasic (xCanvasPane displayPtr,
				     xCanvasPane xWindowID, xCanvasPane xGC,
				     srcBitmap xID, 0, 0, 
				     XPM_WIDTH, XPM_HEIGHT, 0, 0);
	  __ctalkX11CopyPixmapBasic (xCanvasPane displayPtr,
				     xCanasPane xWindowID, xCanvasPane xGC,
				     srcBitmap xID, 0, 0, X
				     PM_WIDTH, XPM_HEIGHT, 25, 25);
#else

	  xCanvasPane drawXPMs srcBitmap;

#endif
          break;
        default:
          break;
        }
    }
    usleep (100000);
  }
}

