
Float instanceMethod myAdd2 (void) {
  Float new myFloatTwo;

  myFloatTwo = 2.0;

  return self + myFloatTwo;
}

int main (void) {
  Float new myFloat, total;

  myFloat = 5;
  
  total = myFloat myAdd2;
  printf ("%f\n", total);
}
