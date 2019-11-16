/* Test a basic return expression beginning with self. */


Integer instanceMethod myAdd2 (void) {
  Float new myFloatTwo;
  int i;

  myFloatTwo = 2.0;
  i = 3;

  return self + myFloatTwo asInteger + i;
}

int main (void) {
  Integer new myInt, total;

  myInt = 5;
  
  total = myInt myAdd2;
  printf ("%d\n", total);
}
