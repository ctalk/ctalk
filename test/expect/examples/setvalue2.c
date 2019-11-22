
/*
 *  This case is not actually valid, because myInt's value is
 *  converted into an Integer.  But it works here.
 */

Integer class MyInteger;

MyInteger instanceMethod = set_value (int __intArg) {
  self addInstanceVariable ("value", __intArg asInteger value);
  return self;
}

int main () {
  MyInteger new myInt;

  myInt = 2;
  printf ("%d\n", myInt);
}
