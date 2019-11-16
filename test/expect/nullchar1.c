

/* Tests '\0' in a run-time expression.
   A method parameter is normally evaluated as a run-time expression. */

String instanceMethod testChar (String testStr) {

  testStr atPut (testStr length - 1), '\0';
}

int main () {

  String new s;

  s = "\"Hello, world!\"\"";

  s testChar s;

  printf ("%s\n", s);
}
