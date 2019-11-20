
/* check that "self" within the argblk gets interpreted automagically
   as a Character object, because Strings are collections of
   Characters */

Integer instanceMethod checkArgBlk (void) {
  String new s;

  s = "Hello::world::again";

  s map {
    if (self == ':') {
      self = ' ';
    }
  }
  printf ("%s\n", s);
}

int main () {
  Integer new myInt;

  myInt checkArgBlk;
}
