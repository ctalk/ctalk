
/* this tests the ++ C ops using native C. */

int main () {
  Integer new myInt;
  long long int x1, x2;

  x1 = 1;
  x2 = 2;

  myInt = ++x1 + ++x2;
  printf ("%lld + %lld = %d\n", x1, x2, myInt);
  myInt = ++x1 + ++x2;
  printf ("%lld + %lld = %d\n", x1, x2, myInt);


}
