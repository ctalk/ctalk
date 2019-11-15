/* $Id: if9.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test an if expression with a while { }
 *  loop as the if block.
 */

int main (char **argv, int argc) {

  int i = 0;

  if (1)
    while (i < 10) {
      printf ("%d ", i++);
    }

  printf ("\n");
  i = 0;
  if (1)
    while (i < 10) {
      printf ("%d ", i);
      ++i;
    }
  printf ("\n");

  exit(0);
}
