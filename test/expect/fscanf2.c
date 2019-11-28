/* $Id: fscanf2.c,v 1.2 2019/11/28 22:16:22 rkiesling Exp $ */

#include <stdio.h>

int main () {

  Integer new result;
  String new s1;
  String new s2;
  ReadFileStream new f;

  f openOn "testinput";

  if ((result = xfscanf (f, "%s %s\n", s1, s2)) == ERROR) {
     printf ("fscanf failed result = %d.\n", result);
     return 1;
  }

  f closeStream;

  printf ("%s ", s1);
  printf ("%s\n", s2);
  fflush (stdout);  /* Make sure the output gets printed when the
		       machine is loaded. */

  exit(0);
}
