/* $Id: cvarrcvr6.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Also like cvarrcvr2.c, except with subscripts.
 */

int main (int argc, char **argv) {
  int my_int;
  char c[2];
  c[0] = 'A';
  my_int = c[0] asInteger;
  printf ("%d\n", my_int);
  ++c[0];
  my_int = c[0] asInteger;
  printf ("%d\n", my_int);
  ++c[0];
  my_int = c[0] asInteger;
  printf ("%d\n", my_int);
}

