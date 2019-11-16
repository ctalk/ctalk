/* Test more return expressions using that begin with self using the C API. */
/* This is the example in the ctalkmethod man page. */
/* this is the same as mapi2.c, except with LongIntegers. */


LongInteger instanceMethod myAdd (Magnitude i) {
  long long int a, result;
  if (i is LongInteger) {
    return LLVAL(__ctalk_self_internal_value () -> __o_value)
      + LLVAL(ARG(0) -> __o_value);
  } else {
    a = i asLongInteger;
    result = LLVAL(__ctalk_self_internal_value () -> __o_value) + a;
    return result;
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
