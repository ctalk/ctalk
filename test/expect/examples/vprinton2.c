
Object instanceMethod myPrint (Integer dummy1,
			       Integer dummy2,
			       String fmt, ...) {
  String new s;
  s vPrintOn fmt;
  return s;
}


int main () {
  Object new obj;
  Integer new i, d1, d2;
  String new str;

  i = 5;
  d1 = 10;
  d2 = 10;

  str = obj myPrint d1, d2, "Hello, world no. %d", i;

  printf ("%s\n", str);
}
