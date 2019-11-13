/* $Id: argblk10.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  C ints used within an argument block within a method.
 */

List instanceMethod myMethod (void) {

  String new sPrefix;
  List new l;
  String new s;
  int n = 0;

  sPrefix = "This element is ";

  l push "l1";
  l push "l2";

  l map {
    s = sPrefix + self;
    printf ("%s\n", s);

    l push "l3";

    /* break after printing four "... l3" strings. */
    n = n + 1;
    if (n >= 6)
      break;
  }

  return NULL;
}

int main (void) {
  List new l_rcvr;

  l_rcvr myMethod;
}
