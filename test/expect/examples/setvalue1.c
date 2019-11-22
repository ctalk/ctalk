
Integer class MyInteger;

MyInteger instanceMethod = set_value (int __intArg) {
  self addInstanceVariable ("value", __intArg value);
  return self;
}

int main () {
  MyInteger new myInt;

  myInt = 2;
  printf ("%d\n", myInt);
}
