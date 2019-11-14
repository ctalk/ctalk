/* $Id: cvarrcvr9.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like cvarrcvr3.c, with subscripts, and done in a method.
 */

Character instanceMethod myMethod (void) {
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

int main (int argc, char **argv) {
  char c[2];
  c[2] myMethod;
}

