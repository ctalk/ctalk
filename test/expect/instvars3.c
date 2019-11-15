
String class MyClass;

MyClass instanceVariable myStringPart String "";

MyClass instanceMethod printWrapper (void) {
  String new str;
  str = "This is also " + self myStringPart + ".";
  printf ("%s\n", str);
}

int main () {
  String new str;
  MyClass new myMultiString;

  myMultiString myStringPart = "my string part";

  str = "This is " + myMultiString myStringPart + ".";
  printf ("%s\n", str);

  myMultiString printWrapper;
}
