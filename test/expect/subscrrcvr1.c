/* $Id: subscrrcvr1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* Similar to margexprs6.c, but with C variables. */
/*
 *  A subscripted variable on the right side of an assignment is
 *  tested elsewhere.  Check for overloaded CVARS as printf 
 *  format arguments.
 */

int main (int argc, char **argv) {
  int i_array[2], i;
  i_array[0] = i = 65;
  Character new c;
  c = i_array[0] asCharacter;
  printf ("%c\n", c);
  printf ("%c\n", i asCharacter);
  printf ("%c\n", i_array[0] asCharacter);
}
