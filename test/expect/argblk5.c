/* $Id: argblk5.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Argument block with local objects called from a function.
 */

int main () {

  List new l;
  String new sPrefix;
  String new s;

  sPrefix = "This element is ";

  l push "l1";
  l push "l2";

  l map {
    s = sPrefix + self;
    printf ("%s\n", s);
  }
  exit(0);
}
