int main () {
  Array new elements;
  String new s;
  ReadFileStream new r;

  r openOn "splitinput";

  s = r readAll;

  s split '=', elements;
  printf ("%s\n", elements at 1);

  r closeStream;
}
