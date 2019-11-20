
/* This should produce an error. Remember that the error line is
 still relative to the start of the method. */

String instanceMethod mySuperclassName (void) {
  "Return a String with the name of the superclass of the
  class given by the receiver."
    return __ctalkGetClass (self superclassName) name;
}

int main () {
  String new myStr;
  myStr = "Key";
  printf ("%s\n", myStr mySuperclassName);
}
