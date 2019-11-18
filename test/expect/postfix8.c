
/* this tests the C variable writeback. */

int main () {
  Integer new myInt;
  int x1, x2;

  x1 = 1;
  x2 = 2;

  myInt = x1++ + x2++;
  printf ("%d\n", myInt);
  myInt = x2++ + x1++;
  printf ("%d\n", myInt);
  printf ("%d %d\n", x1, x2);

}
