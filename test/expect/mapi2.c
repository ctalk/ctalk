/* Test more return expressions using that begin with self using the C API. */
/* This is the example in the ctalkmethod man page. */


Integer instanceMethod myAdd (Magnitude i) {
  int a, result;
  if (i is Integer) {
    return INTVAL(__ctalk_self_internal_value () -> __o_value)
      + INTVAL(ARG(0) -> __o_value);
  } else {
    a = i asInteger;
    result = INTVAL(__ctalk_self_internal_value () -> __o_value) + a;
    return result;
  }
}

int main (void) {
  Integer new myInt, total;
  Float new myFloat;

  myInt = 5;
  
  total = myInt myAdd 2;
  printf ("%d\n", total);

  myFloat = 3.0;
  total = myInt myAdd myFloat;
  printf ("%d\n", total);
}
