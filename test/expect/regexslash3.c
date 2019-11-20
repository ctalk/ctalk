/* Checks for a ^ match at the beginning of a line. */

int main () {

  String new str;
  Array new offsets;
  Integer new matchStart;
  Integer new r;
  String new matchStr;

  str = "Hello, world!\n   \\Hello, world!\"\n";

  r = str matchRegex "^ *\\", offsets;
  if (r > 0) {
    offsets map {
      if (self == -1)
	break;
      matchStr = str + self;
      printf ("%s -- %d\n", matchStr, self);
    }
  }

  str = "   \\Hello, world!\n   \\Hello, world!\"\n";

  r = str matchRegex "^ *\\", offsets;
  if (r > 0) {
    offsets map {
      if (self == -1)
	break;
      matchStr = str + self;
      printf ("%s -- %d\n", matchStr, self);
    }
  }

}
