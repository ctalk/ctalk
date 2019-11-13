/* $Id: argblk3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

List instanceMethod mapPrint (void) {
  List new l;
  l become self;
  l map {
    printf ("%s\n", self);
  }
  return NULL;
}

int main () {

  List new l;

  l push "l1";
  l push "l2";

  l mapPrint;

  exit(0);
}
