
/* this tests the C variable writeback. */

int main () {
  List new myList;
  Integer new myInt;
  long int x;

  myList = "one";
  x = 1;

  myList map {
    myInt = x++;
    printf ("%d\n", myInt);
    myInt = x++;
    printf ("%d\n", myInt);
    printf ("%ld\n", x);
  }

}
