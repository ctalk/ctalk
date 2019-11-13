/* $Id: argblk15.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Argument block called from a function, 
 *  and super used as a receiver in a simple expression. 
 *  
 *  Prints the original list elements, and then three "... l3" strings
 *  (the argblk pushes four).
 *  Checks the use of super as a C function argument.
 */

void my_fn (OBJECT *o) {
  printf ("%#x\n", (unsigned int)o);
}

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

    my_fn (super);

    /* break after pushing four (printing three) "... l3" strings. */
    if (super size > 6) 
      break;

  }

  exit(0);
}
