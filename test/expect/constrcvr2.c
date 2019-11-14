Integer instanceMethod myMethod (void) {
  Integer new i;
  Character new c;
  i = ("Hello, " + "world!") length;
  printf ("%d\n", i);
  printf ("%d\n", ("Hello, " + "world!") length);
  if (("Hello, " + "world!") == "Hello, world!") {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if ("Hello, world!" == ("Hello, " + "world!")) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  c = (10 + 55) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (10 + 55) asCharacter);
  c = (10.0 + 56.0) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (10.0 + 56.0) asCharacter);
  c = (10ll + 57ll) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (10ll + 57ll) asCharacter);
  c = (10L + 58L) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (10L + 58L) asCharacter);
}

int main () {
  Integer new i;
  i myMethod;
}
