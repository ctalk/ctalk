
/* this tests the C variable writeback. */

int main () {
  List new myList;
  Integer new myInt;
  int *x;
  int x_t;

  x = &x_t;
  x_t = 1;
  myList = "one";

  myList map {
    myInt = ++(*x);
    printf ("%d\n", myInt);
    myInt = ++(*x);
    printf ("%d\n", myInt);
    printf ("%d\n", *x);
  }

}
