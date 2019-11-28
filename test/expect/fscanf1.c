/* $Id: fscanf1.c,v 1.2 2019/11/28 22:16:21 rkiesling Exp $ */

#include <stdio.h>

int main () {

  Integer new result;
  String new arg1;
  String new arg2;
  FILE *f;

  if ((f = xfopen ("testinput", "r")) == NULL) {
    printf ("Fail.\n");
    return 1;
  }

  result = xfscanf (f, "%s %s\n", arg1, arg2);

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
