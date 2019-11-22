
List instanceMethod printElement (String leftMargin) {
  printf ("%s%s\n", leftMargin, self);

}

int main () {

  List new myList;
  String new leftMargin;

  myList = "item1", "item2", "item3";  /* Initialize the List with
                                          three String objects. */
  leftMargin = "- ";

  myList map printElement, leftMargin;

}
