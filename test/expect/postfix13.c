
/* this tests the C variable writeback. */

int main () {
  Integer new myInt;
  int x1;
  int *x;

  x1 = 1;
  x = &x1;

  myInt = (*x)++;
  printf ("%d\n", myInt);
  myInt = (*x)++;
  printf ("%d\n", myInt);
  printf ("%d\n", *x);

}
