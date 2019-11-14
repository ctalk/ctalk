/* $Id: case4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* This checks the expression "c = self" in the argument block. */
/* It's okay if this prints a warning that '-' is not an integer
   on some systems (like Darwin). */

String instanceMethod mPrint1 (String text) {
  String new newText;
  
  text map {
    switch (self)
      {
      case 10:
	newText += "<br>";
	break;
      case 13:
	break;
      default:
	newText += self;
      }
  }
  text = newText;
  return text;
}

String instanceMethod mPrint2 (String text) {
  String new newText;
  char c;
  
  text map {
    c = self;
    switch (self)
      {
      case '-':
	break;
      case 13:
	break;
      case 10:
	newText += "<br>";
	break;
      default:
	newText += c;
      }
  }
  text = newText;
  return text;
}

int main (argc, argv) {
  String new text;

  text = "Hello,\nworld!\n";
  text mPrint1 text;
  printf ("%s\n", text);

  text = "Hello,-\nworld!\n";
  text mPrint2 text;
  printf ("%s\n", text);
}
