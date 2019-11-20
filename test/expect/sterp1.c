
/* check that "self" within the argblk gets interpreted automagically
   as a Character object, because Strings are collections of
   Characters */

int main () {
  String new s;

  s = "Hello::world::again";

  s map {
    if (self == ':') {
      self = ' ';
    }
  }
  printf ("%s\n", s);
}
