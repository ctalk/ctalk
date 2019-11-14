/* $Id: do3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test a do loop with a C variable as the second term in the
 *  predicate.
 */

int main (char **argv, int argc) {

  Integer new i;
  int j;

  i = 0;
  j = 10;
  do {
    printf ("%d ", i);
    i = i + 1;
  } while (i < j);
  printf ("\n");

  exit(0);
}
