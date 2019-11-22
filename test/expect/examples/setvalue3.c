
/*
 *  This case is not actually valid, because myInt's value is
 *  converted into an Integer.  But it works here.
 */

Integer class MyInteger;

MyInteger instanceMethod = set_value (int __intArg) {
  Integer new localInt;
  localInt copy __intArg asInteger;
  __ctalkAddInstanceVariable (self, "value", localInt value);
  return self;
}

int main () {
  MyInteger new myInt;

  myInt = 2;
  printf ("%d\n", myInt);
}
