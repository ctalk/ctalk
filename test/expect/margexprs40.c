

char *my_c_string = "Hello, world!";

String instanceMethod myMethod (void) {
  String new myString;
  myString = my_c_string subString 0, 2;
  printf ("%s\n", myString);
}

int main () {
  String new myString;
  myString = my_c_string subString 0, 2;
  printf ("%s\n", myString);
  myString myMethod;
}
