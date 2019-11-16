/* test that we can return a NUL terminating character usefully. */
int main (argc, argv) {
  String new text;
  List new charList;
  Character new c;
  Integer new i;

  text = "Hello,-\nworld!\n";
  text asList charList;
  
  i = 0;
  while (1) {
    c = text at i;
    switch (c)
      {
      case 10:
	printf ("\n");
	break;
      case 0:
	printf ("done! ");
	goto text_done;
	break;
      default:
	printf ("%c", c);
	break;
      }
    ++i;
  }
 text_done:
  printf ("\n");
}
