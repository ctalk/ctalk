
Object instanceMethod myMethod (Integer myIntArg) {
  ++myIntArg;
  printf ("Pass\n");
}

int main (int argc, char **argv) {
  Object new myObj;
  Integer new myInt;
  myInt = 0;

  myObj myMethod myInt;
}
