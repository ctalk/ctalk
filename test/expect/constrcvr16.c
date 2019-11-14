
String instanceMethod catLengthConstant (void) {
  Integer new i;
  i = (self + "world!") length;
  printf ("%d\n", i);
  printf ("%d\n", (self + "world!") length);
  i = ("world!" + self) length;
  printf ("%d\n", i);
  printf ("%d\n", ("world!" + self) length);
}

String instanceMethod catLengthObject (void) {
  Integer new i;
  String new sArg;
  sArg = "world!";
  i = (self + sArg) length;
  printf ("%d\n", i);
  printf ("%d\n", (self + sArg) length);
  i = (sArg + self) length;
  printf ("%d\n", i);
  printf ("%d\n", (sArg + self) length);
}

String instanceMethod catLengthCVar (void) {
  Integer new i;
  char sArg[10];
  xstrcpy (sArg, "world!");
  i = (self + sArg) length;
  printf ("%d\n", i);
  printf ("%d\n", (self + sArg) length);
  i = (sArg + self) length;
  printf ("%d\n", i);
  printf ("%d\n", (sArg + self) length);
}

int main () {
  String new s;
  s = "Hello, ";
  s catLengthConstant;
  s catLengthObject;
  s catLengthCVar;
}
