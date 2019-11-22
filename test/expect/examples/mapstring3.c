/* this checks the argument block return mechanism. */

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
	  printf ("\n");
	  return 11;     /* some unique value that we can check. */
	}
	break;
      }
    (Character *)self -= 32;
    printf ("%c", self);
  }
  printf ("\n");
}
