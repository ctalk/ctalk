
Integer instanceMethod myMethod (void) {
  Integer new i;
  Integer new i_term_a;
  Character new c;
  String new s_term_a;
  Float new f_term_a;
  LongInteger new l_term_a;
  s_term_a = "Hello, ";
  i = (s_term_a + "world!") length;
  printf ("%d\n", i);
  printf ("%d\n", (s_term_a + "world!") length);
  if ((s_term_a + "world!") == "Hello, world!") {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if ("Hello, world!" == (s_term_a + "world!")) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  i_term_a = 10;
  c = (i_term_a + 55) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (i_term_a + 55) asCharacter);
  f_term_a = 10.0;
  c = (f_term_a + 56.0) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (f_term_a + 56.0) asCharacter);
  l_term_a = 10ll;
  c = (l_term_a + 57ll) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (l_term_a + 57ll) asCharacter);
  l_term_a = 10L;
  c = (l_term_a + 58L) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (l_term_a + 58L) asCharacter);
}

int main () {
  Integer new i;
  i myMethod;
}