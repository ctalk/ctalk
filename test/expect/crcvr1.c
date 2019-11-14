int main () {
  char s_char[MAXLABEL];
  char c;
  String new s;

  xstrcpy (s_char, "Hello, ");
  c = 'H';
  s = *s_char asString;
  printf ("%s\n", s);
  printf ("%s\n", c asString);
}
