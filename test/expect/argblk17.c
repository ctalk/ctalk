/* $Id: argblk17.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Argument block called from a function, 
 *  and super used as a receiver in a simple expression. 
 *  
 *  Prints the original list elements, and then four "... l3" strings.
 *  Checks the use of super as a printf argument.
 *
 *  Also checks the use of super in an argument context:
 *
 *   i = super size;
 */

int main () {

  String new sPrefix;
  List new l;
  String new s;
  int i;

  sPrefix = "This element is ";

  l push "l1";
  l push "l2";

  l map {
    s = sPrefix + self;
    printf ("%s\n", s);

    super push "l3";

    printf ("%#x\n", super addressOf);

    i = super size;
    printf ("size: %d\n", i);

    /* break after pushing four (printing 3.5) "... l3" strings. */
    if (super size > 6) 
      break;

  }

  exit(0);
}
