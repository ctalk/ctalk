String instanceMethod printChar (void) {
  String new s;
  Array new floatArray;
  Character new c;
  Float new f;
  Key new k;

  s = "Hello, ";
  c = *s;
  printf ("%c\n", c);
  printf ("%c\n", *s);

  floatArray atPut 0, 66.0;
  k = *floatArray;
  f = *k;
  printf ("%2.2lf\n", f);
  printf ("%2.2lf\n", *k);
  printf ("%2.2lf\n", **floatArray);

  f = **floatArray;
  printf ("%2.2lf\n", f);

}

int main () {
  String new s;
  s printChar;
}
