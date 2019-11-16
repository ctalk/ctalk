
ANSIScrollPane class ANSITestScrollPane;

#define SCROLL_DEFAULT_ORIGIN_X 1
#define SCROLL_DEFAULT_ORIGIN_Y 1
#define SCROLL_DEFAULT_SIZE_X 3
#define SCROLL_DEFAULT_SIZE_Y 20

ANSITestScrollPane instanceMethod new (String __paneName) {
  "ANSIScrollPane constructor.  The argument, a String,
  contains the name of the new object.  Sets the
  default size of the new pane, and sets the
  viewHeight, the height of the scrollbar's interior,
  to leave space around the edges for borders and
  shadows."
    /* OBJECT *paneBuffer_value_alias; *//***/
  ANSIWidgetPane super new __paneName;
  __paneName paneStream openInputQueue;

  __paneName shadow = TRUE;
  __paneName border = TRUE;

  __paneName origin x = SCROLL_DEFAULT_ORIGIN_X;
  __paneName origin y = SCROLL_DEFAULT_ORIGIN_Y;
  __paneName size x = SCROLL_DEFAULT_SIZE_X;
  __paneName size y = SCROLL_DEFAULT_SIZE_Y;
  __paneName viewWidth = SCROLL_DEFAULT_SIZE_X;
  __paneName viewHeight = SCROLL_DEFAULT_SIZE_Y - 2;

  __paneName addBuffer __paneName size x, __paneName size y, 1;

  return __paneName;
}

int main () {
  ANSITestScrollPane new pane;
  printf ("ok!\n");
  pane cleanup;
}
