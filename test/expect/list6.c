/* Check self in a mixed C/Ctalk "if" clause in a C function. */


int main () {
  int x;
  Integer new i;
  List new myList;
  myList = 1, 2, 3;
  x = 0;

  myList map {
    printf ("%s ", self);
    if (x + self == 2) {
      printf ("\n");
      break;
    }
  }
  printf ("%d\n", 11); /* some unique value */
}
