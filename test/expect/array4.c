
int main () {
  String new s;
  Array new a;

  a atPut 0, 'h';
  a atPut 1, 'e';
  a atPut 2, 'l';
  a atPut 3, 'l';
  a atPut 4, 'o';

  s = a asString;

  printf ("%s\n", s);
}
