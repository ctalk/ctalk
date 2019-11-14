/* $Id: cvarrcvr7.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Also like cvarrcvr2.c, except with subscripts.
 */

int main (int argc, char **argv) {
  int my_int;
  int i[2];
  int n = 1;
  i[n] = 'A';
  my_int = i[n] asInteger;
  printf ("%d\n", my_int);
  ++i[n];
  my_int = i[n] asInteger;
  printf ("%d\n", my_int);
  ++i[n];
  my_int = i[n] asInteger;
  printf ("%d\n", my_int);
}

