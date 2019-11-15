/* Check the return mechanism for Lists in an argument block in a
   method. */

Integer instanceMethod parseList (void) {
  List new myList;
  myList = 'h', 'e', 'l', 'l', 'o';

  myList map {
    switch (self)
      {
      case 'l':
	printf ("\n");
	return 11;    /* some unique value. */
	break;
      default:
	printf ("%c ", self);
	break;
      }
  }
  printf ("\n");
}


int main () {
  Integer new i;
  Integer new result;
  
  result = i parseList;
  printf ("%d\n", result);
}
