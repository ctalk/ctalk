/* $Id: margexprs3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main (int argc, char **argv) {

  Array new cmdLineArgs;

  cmdLineArgs atPut 0, argv[0];

  printf ("%s\n", cmdLineArgs at 0);

  if (argc > 1) {
    cmdLineArgs atPut 0 + 1, argv[0 + 1];
    printf ("%s\n", cmdLineArgs at 1);
  }
  if (argc > 2) {
    cmdLineArgs atPut (0 + 2, argv[0 + 2]);
    printf ("%s\n", cmdLineArgs at 2);
  }
  if (argc > 3) {
    cmdLineArgs atPut (0 + 3), argv[0 + 3];
    printf ("%s\n", cmdLineArgs at 3);
  }
  if (argc > 4) {
    cmdLineArgs atPut (0 + 4), (argv[0 + 4]);
    printf ("%s\n", cmdLineArgs at 4);
  }

  /*
   *  Doesn't have any effect yet.
   */
/*   if (argc > 1) { */
/*     if (*argv[0] == '/')  */
/*       cmdLineArgs atPut 0 + 1, &argv[0 + 1][1]; */
/*     else */
/*       cmdLineArgs atPut 0 + 1, argv[0 + 1]; */
/*     printf ("%s\n", cmdLineArgs at 1); */
/*   } */

  exit(0);
}
