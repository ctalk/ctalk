
String instanceMethod mySearch (String __pattern, Array offsetsArray) {
  long long offsets[MAXARGS];
  int i;
  returnObjectClass Array;
  __ctalkMatchText (__pattern, self, offsets);
  for (i = 0; ; i++) {
    offsetsArray atPut i, offsets[i];
    if (offsets[i] == -1)
      break;
  }
  return offsetsArray;
}

int main () {
  String new s;
  String new pattern;
  Array new offsets;

  pattern = "!$";

  s = "Hello, world! Hello, world, Hello, world!";
  
  s mySearch pattern, offsets;

  offsets map {
    printf ("%d\n", self);
  }
}
