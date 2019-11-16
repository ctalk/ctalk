/* This is the example in the ctalkmethod man page. */
/* This is similar to mapi3.c, but with LongIntegers. */


LongInteger instanceMethod myAdd (LongInteger l) {
  if (l is LongInteger) {
    return self + l;
  } else {
    return self + l asLongInteger;
  }
}

int main (void) {
  LongInteger new myLongInt, total;
  Float new myFloat;

  myLongInt = 5;
  
  total = myLongInt myAdd 2;
  printf ("%d\n", total);

  myFloat = 3.0;
  total = myLongInt myAdd myFloat;
  printf ("%d\n", total);
}
