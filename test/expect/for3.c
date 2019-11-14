/* $Id: for3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test a for loop with objects as both terms in the
 *  termination predicate.
 */

int main (char **argv, int argc) {

  Integer new idx;
  Integer new i;

  i = 10;

  for (idx = 0; idx < i; idx = idx + 1)
    printf ("%d ", idx);
  printf ("\n");

  exit(0);
}
