/* $Id: for2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test a for loop with a C variable as the left term in the
 *  termination predicate, and an object as the right term.
 */

int main (char **argv, int argc) {

  Integer new idx;
  int i;

  i = 10;

  for (idx = 0; idx < i; idx = idx + 1)
    printf ("%d ", idx);
  printf ("\n");

  exit(0);
}
