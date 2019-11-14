/* $Id: cvarrcvr10.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like cvarrcvr4.c, with subscripts.
 */

int main (int argc, char **argv) {
  int my_int;
  char c[2];
  c[1] = 65;
  my_int = c[1]++ asInteger;
  printf ("%d\n", my_int);
  my_int = c[1]++ asInteger;
  printf ("%d\n", my_int);
  my_int = c[1]++ asInteger;
  printf ("%d\n", my_int);

  c[1] = 65;
  printf ("%d\n", c[1]++ asInteger);
  printf ("%d\n", c[1]++ asInteger);
  printf ("%d\n", c[1]++ asInteger);
  
}

