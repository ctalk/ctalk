
int main () {
  Integer new i;
  Character new c;
  Integer new i_term_a;
  int i_term_b;
  String new s_term_a;
  char s_term_b[MAXLABEL];
  Float new f_term_a;
  double f_term_b;
  LongInteger new l_term_a;
  long long int l_term_b;
  s_term_a = "Hello, ";
  xstrcpy (s_term_b, "world!");
  i = (s_term_a + s_term_b) length;
  printf ("%d\n", i);
  printf ("%d\n", (s_term_a + s_term_b) length);
  if ((s_term_a + s_term_b) == "Hello, world!") {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if ("Hello, world!" == (s_term_a + s_term_b)) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  i_term_a = 10;
  i_term_b = 55;
  c = (i_term_a + i_term_b) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (i_term_a + i_term_b) asCharacter);
  f_term_a = 10.0;
  f_term_b = 56.0;
  c = (f_term_a + f_term_b) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (f_term_a + f_term_b) asCharacter);
  l_term_a = 10ll;
  l_term_b = 57ll;
  c = (l_term_a + l_term_b) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (l_term_a + l_term_b) asCharacter);
  l_term_a = 10L;
  l_term_b = 58L;
  c = (l_term_a + l_term_b) asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", (l_term_a + l_term_b) asCharacter);
}

