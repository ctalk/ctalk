/* $Id: margexprs9.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* This is similar to margexprs3.c, except with C variables as terms. */

int main (int argc, char **argv) {

  int i_term, i_term_2;

  Array new cmdLineArgs;
  i_term = 0;
  i_term_2 = 0;

  cmdLineArgs atPut i_term, argv[i_term];

  printf ("%s\n", cmdLineArgs at i_term);

  cmdLineArgs atPut i_term + 0, argv[i_term + 0];
  printf ("%s\n", cmdLineArgs at i_term);

  cmdLineArgs atPut 0 + i_term_2, argv[0 + i_term_2];
  printf ("%s\n", cmdLineArgs at i_term);

  cmdLineArgs atPut i_term + i_term_2, argv[i_term + i_term_2];
  printf ("%s\n", cmdLineArgs at i_term);

  exit(0);
}
