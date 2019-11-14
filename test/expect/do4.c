/* $Id: do4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test a do loop with a negated object as the predicate.
 */

int main (char **argv, int argc) {

  Integer new i;
  Integer new j;

  i = 0;
  j = 0;
  do {
    printf ("%d ", j);
    j = j + 1;
    if (j == 10)
      i = 1;
  } while (!i);
  printf ("\n");

  exit(0);
}
