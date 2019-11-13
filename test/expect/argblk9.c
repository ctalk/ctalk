/* $Id: argblk9.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Argument block called from a function, 
 *  and super used as a receiver in a simple expression. 
 *  
 *  Prints the original list elements, and then four "... l3" strings.
 */

int main () {

  String new sPrefix;
  List new l;
  String new s;
  Integer new n;

  n = 0;

  sPrefix = "This element is ";

  l push "l1";
  l push "l2";

  l map {
    s = sPrefix + self;
    printf ("%s\n", s);

    /* This is the line we're testing. */
    super push "l3";

    /* break after printing four "... l3" strings. */
    n = n + 1;
    if (n >= 6)
      break;
  }

  exit(0);
}
