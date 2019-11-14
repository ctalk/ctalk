/* checks the handling of "required" break statements within an
   argument block. */

int main () {
  String new str;

  str = "Hello, world!";

  str map {
    if (self == 'o') {
      break;
    }
    printf ("%c", self);
  }
  printf ("\n");

  str map {
    switch (self)
      {
      case 'a':
      case 'e':
      case 'i':
      case 'o':
      case 'u':
	if (self == 'o') {
	  break;
	}
	self = self toUpper;
	break;
      }
    printf ("%c", self);
  }
  printf ("\n");
}
