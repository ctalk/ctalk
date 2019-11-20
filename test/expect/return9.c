

/*  more checks that the compiler can handle some of the self return
    expressions (and legacy expressions) generally. */

Object instanceMethod myName (void) {
  "Return a String with the name of the receiver."
    returnObjectClass String;
    return self -> __o_name;
}

int main () {
  Integer new myInt;
  printf ("%s\n", myInt myName);
}
