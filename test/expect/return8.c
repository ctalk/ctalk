

/*  check that the compiler can handle some of the self return expressions
    generally. */

Object instanceMethod myClassName (void) {
  "Return a String with the class name of the receiver."
  returnObjectClass String;
  return self -> __o_classname;
}

int main () {
  Integer new myInt;
  printf ("%s\n", myInt myClassName);
}
