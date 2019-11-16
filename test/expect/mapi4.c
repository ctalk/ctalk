/* Test a basic return expression beginning with self that uses the C API. */


Integer instanceMethod myAdd2 (void) {
  Integer new myIntTwo;

  myIntTwo = 2;

  return self + myIntTwo;
}

int main (void) {
  Integer new myInt, total;

  myInt = 5;
  
  total = myInt myAdd2;
  printf ("%d\n", total);
}
