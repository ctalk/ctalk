
/* this tests the C variable writeback. */

int main () {
  Integer new myInt;
  int x;

  x = 1;

  myInt = x++;
  printf ("%d\n", myInt);
  myInt = x++;
  printf ("%d\n", myInt);
  printf ("%d\n", x);

}
