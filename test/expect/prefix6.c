String instanceMethod printChar (void) {
  char s1[MAXLABEL];
  xstrcpy (s1, "Hello, ");
  Character new c;

  c = (*s1);
  printf ("%c\n", c);
  printf ("%c\n", (*s1));
}

int main () {
  String new s;
  s printChar;
}
