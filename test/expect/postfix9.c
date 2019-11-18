
/* this tests the C variable writeback. */

int main () {
  Integer new myInt;
  long int x1, x2;

  x1 = 1;
  x2 = 2;

  myInt = x1++ + x2++;
  printf ("%d\n", myInt);
  myInt = x1++ + x2++;
  printf ("%d\n", myInt);
  printf ("%ld %ld\n", x1, x2);

}
