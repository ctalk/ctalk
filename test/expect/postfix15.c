
/* this tests the C variable writeback. */

int main () {
  Integer new myInt;
  long int x1;
  long int *x;

  x1 = 1;
  x = &x1;

  myInt = (*x)++;
  printf ("%d\n", myInt);
  myInt = (*x)++;
  printf ("%d\n", myInt);
  printf ("%ld\n", *x);

}
