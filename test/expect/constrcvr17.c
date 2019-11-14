
String instanceMethod catLengthArg (String sArg) {
  Integer new i;
  i = (self + sArg) length;
  printf ("%d\n", i);
  printf ("%d\n", (self + sArg) length);
  i = (sArg + self) length;
  printf ("%d\n", i);
  printf ("%d\n", (sArg + self) length);
}

int main () {
  String new s;
  String new sArg;
  s = "Hello, ";
  sArg = "world!";
  s catLengthArg sArg;
}
