
/* check that "self" within the argblk gets interpreted automagically
   as a Character object, because Strings are collections of
   Characters */

String instanceMethod checkArgBlk (void) {

  self map {
    if (self == ':') {
      self = ' ';
    }
  }
  printf ("%s\n", self);
}

int main () {
  String new s;

  s = "Hello::world::again";

  s checkArgBlk;
}
