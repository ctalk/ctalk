/* $Id: sprintf2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdio.h>

int main () {
  Integer new result;
  String new arg1;
  String new arg2;
  String new str;

  arg1 = "s1";
  arg2 = "s2";

  if ((result = xsprintf (str, "%s %s\n", arg1, arg2)) == ERROR) {
    printf ("sprintf failed result = %d.\n", result);
    return 1;
  }

  printf ("%s\n", str);

  exit(0);
}
