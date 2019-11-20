
/* check that "self" within the argblk gets interpreted automagically
   as a Character object, because Strings are collections of
   Characters */

Integer instanceMethod checkArgBlk (String s) {

  s map {
    if (self == ':') {
      self = ' ';
    }
  }
  printf ("%s\n", s);
}

int main () {
  Integer new myInt;
  String new s;

  s = "Hello::world::again";


  myInt checkArgBlk s;
}
