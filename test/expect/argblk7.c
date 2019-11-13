/* $Id: argblk7.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Argument block called from a method, with local vars from the method,
 *  and super used as a receiver in a simple instance variable expression.
 */

List instanceMethod mapPrint (void) {
  List new l;
  String new sPrefix;
  String new s;
  sPrefix = "This element is ";
  l become self;
  l map {
    s = sPrefix + self;
    printf ("%s\n", s);
    printf ("%s\n", super value);
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
