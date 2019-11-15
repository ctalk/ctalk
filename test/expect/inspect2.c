int main () {
  Point new i;
  Symbol new s;
  String new str;
  ObjectInspector new inspector;

  i x = 150;
  i y = 225;

  *s = i;
  str = inspector formatObject s;
  printf ("%s\n", str);
}
