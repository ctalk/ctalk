/* $Id: list1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Print a list, and then the result of List : size.
 */

int main () {

  List new l;

  l push "1";
  l push "2";
  l push "3";
  l push "4";
  l push "5";
  l push "6";

  l map {
    printf ("%s\n", self);
  }
  printf ("List size: %d\n", l size);

  exit(0);
}
