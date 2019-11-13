/* $Id: argblk8.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Argument block called from a method, with local vars from the method,
 *  and super used as a receiver in a simple expression. 
 *  
 *  Prints the original list elements, and then four "... l3" strings.
 */

List instanceMethod mapPrint (void) {
  String new sPrefix;
  String new s;
  Integer new n;
  sPrefix = "This element is ";
  n = 0;

  self map {
    s = sPrefix + self;
    printf ("%s\n", s);

    /* This is the line we're testing. */
    super push "l3";

    /* break after printing four "... l3" strings. */
    n = n + 1;
    if (n >= 6)
      break;
  }
  return NULL;
}

int main () {

  List new l;
  String new s;

  l push "l1";
  l push "l2";

  l mapPrint;

  exit(0);
}
