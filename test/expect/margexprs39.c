
char *myarray[] = {
  "Hello,",
  "World!",
  "Today"
};

String instanceMethod myMethod (void) {
  String new myString;
  myString = myarray[0] subString 0, 2;
  printf ("%s\n", self);
}

int main () {
  String new myString;
  myString = myarray[0] subString 0, 2;
  printf ("%s\n", myString);
  myString myMethod;
}
