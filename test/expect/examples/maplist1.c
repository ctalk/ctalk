
List instanceMethod printElement (void) {
  printf ("%s\n", self);  /* Here, for each call to the printElement
                             method, "self" is each of myList's
                             successive members, which are String
                             objects. */
}

int main () {

  List new myList;

  myList = "item1", "item2", "item3";  /* Initialize the List with
                                          three String objects. */
  myList map printElement;

}
