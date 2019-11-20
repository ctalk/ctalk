/* Checks a lot of less obvious expressions. */

int main () {

  String new str;
  Array new offsets;
  Integer new matchStart;
  Integer new r;
  String new matchStr;

  str = "   \\Hello, world!\"\n";

  r = str matchRegex "^ *\\", offsets;
  if (r > 0) {
    offsets map {
      if (self == -1)
	break;
      matchStart = str matchLength;
      matchStr = str + matchStart;
      printf ("%s\n", matchStr);
    }
  }

  r = str matchRegex "^ *\\", offsets;
  if (r > 0) {
    offsets map {
      if (self == -1)
	break;
      matchStr = str + str matchLength;
      printf ("%s\n", matchStr);
    }
  }

  r = str matchRegex "^ *\\", offsets;
  if (r > 0) {
    offsets map {
      if (self == -1)
	break;
      matchStart = str matchLength;
      printf ("%s\n", str + matchStart);
    }
  }

  if (str matchRegex "^ *\\", offsets > 0)
    printf ("%s\n", eval str + str matchLength);
  
  if (str matchRegex "^ *\\", offsets > 0) {
    matchStart = str matchLength;
    printf ("%s\n", str + matchStart);
  }
  
}
