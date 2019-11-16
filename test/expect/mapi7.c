/* Test more return expressions that begin with self. */


Integer instanceMethod myAddTwo (Integer i) {
  int ii = 2;

  if (i is Integer) {
    return self + i;
  } else {
    return self + i asInteger + ii;
  }
}

int main (void) {
  Integer new myInt, total;
  Float new myFloat;

  myInt = 5;
  
  total = myInt myAddTwo 2;
  printf ("%d\n", total);

  myFloat = 3.0;
  total = myInt myAddTwo myFloat;
  printf ("%d\n", total);
}
