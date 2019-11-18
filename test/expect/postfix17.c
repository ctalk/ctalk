
/* this tests the C variable writeback. */

int main () {
  Integer new myInt;
  long long int x1;
  long long int *x;

  x1 = 1;
  x = &x1;

  myInt = (*x)++;
  printf ("%d\n", myInt);
  myInt = (*x)++;
  printf ("%d\n", myInt);
  printf ("%lld\n", *x);

}
