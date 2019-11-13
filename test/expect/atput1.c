

String instanceMethod  atPutAlNum (void) {

  self atPut (self length), 'a';
  self atPut (self length), 'b';
  self atPut (self length), 'c';
  self atPut (self length), 'd';
  self atPut (self length), 'e';
  self atPut (self length), 'f';
  self atPut (self length), 'g';
  self atPut (self length), 'h';
  self atPut (self length), 'i';
  self atPut (self length), 'j';
  self atPut (self length), 'k';
  self atPut (self length), 'l';
  self atPut (self length), 'm';
  self atPut (self length), 'n';
  self atPut (self length), 'o';
  self atPut (self length), 'p';
  self atPut (self length), 'r';
  self atPut (self length), 's';
  self atPut (self length), 't';
  self atPut (self length), 'u';
  self atPut (self length), 'v';
  self atPut (self length), 'w';
  self atPut (self length), 'x';
  self atPut (self length), 'y';
  self atPut (self length), 'z';
  self atPut (self length), '\n';
  return NULL;
}

int main () {
  String new s;
  s = "";
  s atPutAlNum;
  s atPutAlNum;
  s atPutAlNum;
  printf ("%s\n", s);
}
