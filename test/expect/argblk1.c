/* $Id: argblk1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main () {

  List new l;
  String new s;

  l push "l1";
  l push "l2";

  l map { 
    printf ("%s\n", self);
  }

  exit(0);
}
