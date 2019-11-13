/* $Id: argblk2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

List instanceMethod mapPrint (void) {
  self map {                /* The first self refers to the receiver */
    printf ("%s\n", self);  /* of mapPrint.  The second self refers  */
  }                         /* to the list element from map.         */
  return NULL;
}

int main () {

  List new l;

  l push "l1";
  l push "l2";

  l mapPrint;

  exit(0);
}
