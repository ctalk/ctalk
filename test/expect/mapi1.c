/* Test a basic return expression starting with self that uses the C API. */

Integer instanceMethod myAdd (Integer i) {
  return INTVAL(__ctalk_self_internal_value () -> __o_value) +
    INTVAL(ARG(0) -> __o_value);
}

int main (void) {
  Integer new myInt, total;

  myInt = 5;
  
  total = myInt myAdd 2;
  printf ("%d\n", total);
}
