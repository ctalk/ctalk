

int main () {
  String new str;

  str = "Hello, world!";

  str map {
    switch (self)
      {
      case 'a':
      case 'e':
      case 'i':
      case 'o':
      case 'u':
	(Character *)self = self toUpper;
	break;
      }
    printf ("%c", self);
  }
  printf ("\n");
}
