/* Test a basic return expression beginning with self. */
/* like mapi5.c, except with LongInteger receivers. */


LongInteger instanceMethod myAdd2 (void) {
  Float new myFloatTwo;

  myFloatTwo = 2.0;

  return self + myFloatTwo asLongInteger;
}

int main (void) {
  LongInteger new myLongInt, total;

  myLongInt = 5;
  
  total = myLongInt myAdd2;
  printf ("%ld\n", total);
}
