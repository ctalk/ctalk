
                              /* Here again, "leftMargin" is passed as */
                              /* the second argument to map, below. */
List instanceMethod printItem (String leftMargin) {
  Integer new element;
  element = self;
  printf ("%s%d ", leftMargin, element);
  return NULL;
}

int main () {

  List new l;
  String new leftMargin;

  leftMargin = "  ";

  l = 1, 2, 3;

  l map printItem, leftMargin;  /* "leftMargin" is the first argument when */
                               /* printItem is called.                    */
  printf ("\n");
}
