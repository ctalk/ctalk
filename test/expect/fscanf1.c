/* $Id: fscanf1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdio.h>

int main () {

  Integer new result;
  String new arg1;
  String new arg2;
  FILE *f;

  if ((f = fopen ("testinput", "r")) == NULL) {
    printf ("Fail.\n");
    return 1;
  }

  result = fscanf (f, "%s %s\n", arg1, arg2);

  if (result != 2) {
    printf ("Fail (result == %d)\n", result);
    return 1;
  }

  if (fclose (f)) {
    printf ("Fail (fclose (f))\n");
    return 1;
  }

  printf ("%s %s\n", arg1, arg2);
  fflush (stdout);  /* Make sure the results get printed if running
		       under a bunch of test scripts. */

  exit(0);
}
