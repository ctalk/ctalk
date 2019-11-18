
char *myFunc (int index, char *msg) {
  static char buf[0xff];
  sprintf (buf, "%d: %s", index, msg);
  return buf;
}

int main () {
  String new s;
  String new msg;
  Integer new intNo;

  intNo = 2;
  msg = "Test error message";

  s printOn "%s", "Error message: " + myFunc (intNo, msg) + ".";

  printf ("%s\n", s);
}
