/* This is the example in the ctalkmethod man page. */

Float instanceMethod myAdd (Float f) {
  if (f is Float) {
    return self + f;
  } else {
    return self + f asFloat;
  }
}

int main (void) {
  Float new myFloat, total;

  myFloat = 3.0;
  total = myFloat myAdd 3.0;
  printf ("%f\n", total);
}
