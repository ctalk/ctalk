int main () {
  char s1[MAXLABEL];
  int i1[10];
  Character new c;
  Integer new i;
  long long int l1[10];
  LongInteger new l;

  xstrcpy (s1, "Hello, ");
  i1[0] = 'A';
  l1[0] = 'B';

  c = *s1;
  printf ("%c\n", c);
  printf ("%c\n", *s1);

  i = *i1;
  printf ("%c\n", (char)i);
  printf ("%c\n", (char)*i1);

  l = *l1;
  printf ("%c\n", (char)l);
  printf ("%c\n", (char)*l1);
}
