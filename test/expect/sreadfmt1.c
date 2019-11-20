int main () {

  String new s;
  String new arg1;
  String new arg2;

  s = "s1 s2";

  s readFormat "%s %s", arg1, arg2;

  printf ("%s ", arg1);
  printf ("%s\n", arg2);
}
