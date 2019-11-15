
int main () {
  Integer new i;
  Symbol new s;
  String new str;
  ObjectInspector new inspector;

  i = 100;

  *s = i;
  str = inspector formatObject s;
  printf ("%s\n", str);
}
