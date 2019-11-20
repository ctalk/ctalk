

String instanceMethod mySuperclassName (void) {
  "Return a String with the name of the superclass of the
  class given by the receiver."
  return __ctalkGetClass (self -> __o_value) -> __o_superclassname;
}

int main () {
  String new myStr;
  myStr = "Key";
  printf ("%s\n", myStr mySuperclassName);
}
