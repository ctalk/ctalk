/* Check the return mechanism for Lists in an argument block in a
   method. */

Integer instanceMethod parseList (void) {
  List new myList;
  myList = "h", "e", "l", "l", "o";

  myList map {
    printf ("%s ", self);
    if (self == "l") {
      printf ("\n");
      return 11;    /* some unique value. */
    }
  }
}

int main () {
  Integer new i;
  printf ("%d\n", i parseList);
}
