int main () {
  String new s;
  Array new a;
  Array new floatArray;
  Character new c;
  Integer new i;
  Float new f;
  Key new k;
  Key new j;

  s = "Hello, ";
  c = *s;
  printf ("%c\n", c);
  printf ("%c\n", *s);

  printf ("-----------\n");

  a atPut 0, 65;
  k = *a;
  i = *k;
  printf ("%c\n", (char)i);
  printf ("%c\n", (char)*k);
  printf ("%c\n", (char)**a);

  i = **a;
  printf ("%c\n", (char)i);

  printf ("-----------\n");

  floatArray atPut 0, 66.0;
  k = *floatArray;
  f = *k;

  printf ("%2.2lf\n", f);
  printf ("%2.2lf\n", *k);
  printf ("%2.2lf\n", **floatArray);

  f = **floatArray;
  printf ("%2.2lf\n", f);

}
