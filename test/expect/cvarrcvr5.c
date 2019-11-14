/* $Id: cvarrcvr5.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like cvarrcvr2.c, except with subscripts.
 */

int main (int argc, char **argv) {
  int my_int[2];
  char c;
  c = 'A';
  my_int[0] = c asInteger;
  printf ("%d\n", my_int[0]);
  ++c;
  my_int[0] = c asInteger;
  printf ("%d\n", my_int[0]);
  ++c;
  my_int[0] = c asInteger;
  printf ("%d\n", my_int[0]);
}

