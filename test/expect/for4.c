/* $Id: for4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test for loop initializers.
 */

int main (char **argv, int argc) {

  Integer new idx;
  Integer new idx2;
  Integer new i;
  int i_int;

  i = 10;

  for (i_int = 0; i_int < i; ++i_int)
    printf ("%d ", i_int);
  printf ("\n");

  for (idx = 0, i_int = 0; idx < i; idx = idx + 1)
    printf ("%d ", idx);
  printf ("\n");

  for (i_int = 0, idx = 0; idx < i; idx = idx + 1)
    printf ("%d ", idx);
  printf ("\n");

  for (i_int = 0, idx = 0, idx2 = 0; idx2 < i; ++idx2)
    printf ("%d ", idx2);
  printf ("\n");

  exit(0);
}
