/* $Id: strcmp2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  String new s;
  Integer new result;
  Integer new cmpEq;

  s = "s";
  cmpEq = 0;

  if ((result = strcmp (s, s)) == cmpEq) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
