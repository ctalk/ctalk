/* $Id: list2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Print the size of an empty list, which should be zero.
 */

int main () {

  List new l;

  l map {
    printf ("%s\n", self);
  }
  printf ("List size: %d\n", l size);

  exit(0);
}
