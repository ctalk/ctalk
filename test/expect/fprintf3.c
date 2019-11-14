/* $Id: fprintf3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdio.h>

int main () {

  Integer new result;
  String new arg1;
  String new arg2;
  WriteFileStream new f;

  f openOn "testoutput.fprintf3";

  arg1 = "s1";
  arg2 = "s2";

  if ((result = fprintf (f, "%s %s\n", arg1, arg2)) == ERROR) {
     printf ("fprintf failed.\n");
     return 1;
  }

  f closeStream;

  exit(0);
}
