
String instanceMethod mySearch (String __pattern, Array offsetsArray) {
  long long offsets[MAXARGS], o;
  int i;
  returnObjectClass Array;
  __ctalkSearchBuffer (__pattern, self, offsets);
  for (i = 0; ; i++) {
    o = offsets[i];
    offsetsArray atPut i, o;
    if (o == -1)
      break;
  }
  return offsetsArray;
}

int main () {
  String new s;
  String new pattern;
  Array new offsets;

  pattern = "He";
  s = "Hello, world! Hello, world, Hello, world!";
  
  s mySearch pattern, offsets;

  offsets map {
    printf ("%d\n", self);
  }
}
