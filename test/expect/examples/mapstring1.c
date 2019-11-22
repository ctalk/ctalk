String instanceMethod printSpaceChar (void) {
  printf (" %c", self);  /* Here, for each call to the printSpaceChar
                             method, "self" is each of myString's
                             successive characters. */
}

int main () {

  String new myString;

  myString = "Hello, world!";

  myString map printSpaceChar;

  printf ("\n");
}

