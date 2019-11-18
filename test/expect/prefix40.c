
/* this tests the prefix ++ operators using native C. */

int main () {
  Integer new myInt;
  long int x1, x2;

  x1 = 1;
  x2 = 2;

  myInt = ++x1 + ++x2;
  printf ("%ld + %ld = %d\n", x1, x2, myInt);
  myInt = ++x1 + ++x2;
  printf ("%ld + %ld = %d\n", x1, x2, myInt);

}
