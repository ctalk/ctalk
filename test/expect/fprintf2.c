/* $Id: fprintf2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdio.h>

int main () {

  Integer new result;
  char arg1[16];
  char arg2[16];
  FILE *f;

  if ((f = fopen ("testoutput.fprintf2", "w")) == NULL) {
    printf ("Fail.\n");
    return 1;
  }

  xstrcpy (arg1, "s1");
  xstrcpy (arg2, "s2");

  if ((result = fprintf (f, "%s %s\n", arg1, arg2)) == ERROR) {
     printf ("fprintf failed.\n");
     return 1;
  }

  fclose (f);

  exit(0);
}
