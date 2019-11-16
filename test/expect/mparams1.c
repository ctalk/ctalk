Integer instanceMethod myAdd (Integer param1,
			      Integer param2,
			      Integer param3) {
  if (self == 1) {
    return self + param1;
  } else if (self == 2) {
    return self + param2;
  } else if (self == 3) {
    return self + param3;
  }
  return self;
}

int main (void) {
  Integer new myInt, myResult;
  
  myInt = 1;
  myResult = myInt myAdd 1, 2, 3;
  printf ("%d\n", myResult);
  myInt = 2;
  myResult = myInt myAdd 1, 2, 3;
  printf ("%d\n", myResult);
  myInt = 3;
  myResult = myInt myAdd 1, 2, 3;
  printf ("%d\n", myResult);

}
