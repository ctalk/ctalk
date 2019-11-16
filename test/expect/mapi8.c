/* Test a basic return expression starting with self that uses the C API. */
/* This is similar to mapi1.c, except with LongIntegers. */

LongInteger instanceMethod myAdd (Magnitude i) {
  return LLVAL(__ctalk_self_internal_value () -> __o_value) +
    LLVAL(ARG(0) -> __o_value);
}

int main (void) {
  LongInteger new myLongInt, total;

  myLongInt = 5L;
  
  total = myLongInt myAdd 2;
  printf ("%ld\n", total);
}
