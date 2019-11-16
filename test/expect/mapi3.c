/* This is the example in the ctalkmethod man page. */
/* The compiler should print a warning about the receiver/
   operand class mismatch. */


Integer instanceMethod myAdd (Magnitude i) {
  if (i is Integer) {
    return self + i;
  } else {
    return self + i asInteger;
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
