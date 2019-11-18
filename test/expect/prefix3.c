String instanceMethod printChar (void) {
  String new s;
  Array new a;
  Character new c;
  Integer new i;
  Key new k;

  s = "Hello, ";
  c = *s;
  printf ("%c\n", c);
  printf ("%c\n", *s);

  a atPut 0, 65;
  k = *a;
  i = *k;
  printf ("%c\n", (char)i);
  printf ("%c\n", (char)*k);
  printf ("%c\n", (char)**a);

  i = **a;
  printf ("%c\n", (char)i);
}

int main () {
  String new s;
  s printChar;
}
