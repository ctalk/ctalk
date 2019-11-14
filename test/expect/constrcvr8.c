
int main () {
  Integer new i;
  Character new c;
  int i_term_b;
  char s_term_b[MAXLABEL];
  double f_term_b;
  long long int l_term_b;
  xstrcpy (s_term_b, "world!");
  i = ("Hello, " + s_term_b) length;
  printf ("%d\n", i);
  printf ("%d\n", ("Hello, "+ s_term_b) length);
  if (("Hello, " + s_term_b) == "Hello, world!") {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if ("Hello, world!" == ("Hello, " + s_term_b)) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  i_term_b = 55;
  c = (10 + i_term_b) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (10 + i_term_b) asCharacter);
  f_term_b = 56.0;
  c = (10.0 + f_term_b) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (10.0 + f_term_b) asCharacter);
  l_term_b = 57ll;
  c = (10ll + l_term_b) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (10ll + l_term_b) asCharacter);
  l_term_b = 58L;
  c = (10L + l_term_b) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (10L + l_term_b) asCharacter);
}

