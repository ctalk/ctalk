/* $Id: do1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test a do loop with a C variable as the first term in the
 *  predicate.
 */

int main (char **argv, int argc) {

  Integer new i;
  int j;

  i = 10;
  j = 0;

  do {
    printf ("%d ", j);
    j = j + 1;
  } while (j < i);
  printf ("\n");

  exit(0);
}
