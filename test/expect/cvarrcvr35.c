/* $Id: cvarrcvr35.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* This checks the expression "c = self" in the argument block. */

String instanceMethod mPrint (String text) {
  char c;
  text map {
    c = self;
    switch (c)
      {
      default:
	printf ("%c", c);
      }
  }
  printf ("\n");
}

int main (argc, argv) {
  String new text;
  text = "Hello, world!\n";
  text mPrint text;
}
