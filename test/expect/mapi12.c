/* This is also like mapi4.c, except with a LongInteger receiver */


LongInteger instanceMethod myAdd2 (void) {
  Integer new myIntTwo;

  myIntTwo = 2;

  return self + myIntTwo asLongInteger;
}

int main (void) {
  LongInteger new myLongInt, total;

  myLongInt = 5;
  
  total = myLongInt myAdd2;
  printf ("%ld\n", total);
}
