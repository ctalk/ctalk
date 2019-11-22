
Object instanceMethod myPrint (String fmt, ...) {
  String new s;
  s vPrintOn fmt;
  return s;
}


int main () {
  Object new obj;
  Integer new i;
  String new str;

  i = 5;

  str = obj myPrint "Hello, world no. %d", i;

  printf ("%s\n", str);
}
