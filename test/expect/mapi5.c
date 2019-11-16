/* Test a basic return expression beginning with self. */


Integer instanceMethod myAdd2 (void) {
  Float new myFloatTwo;

  myFloatTwo = 2.0;

  return self + myFloatTwo asInteger;
}

int main (void) {
  Integer new myInt, total;

  myInt = 5;
  
  total = myInt myAdd2;
  printf ("%d\n", total);
}
