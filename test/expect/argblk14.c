/* $Id: argblk14.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Argument block called from a function, 
 *  and super used as a receiver in a simple expression. 
 *  
 *  Prints the original list elements, and then four "... l3" strings.
 *  Checks the use of super in a control block predicate.
 */

int main () {

  String new sPrefix;
  List new l;
  String new s;

  sPrefix = "This element is ";

  l push "l1";
  l push "l2";

  l map {
    s = sPrefix + self;
    printf ("%s\n", s);

    super push "l3";

    /* break after pushing four  "... l3" strings (printing three of them). */
    if (super size > 6) 
      break;

  }

  exit(0);
}
