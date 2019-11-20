/* $Id: strncmp2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  String new s1;
  String new s2;
  Integer new result;
  Integer new cmpEq;

  s1 = "s1";
  s2 = "s2";
  cmpEq = 0;

  if ((result = strncmp (s1, s2, 1)) == cmpEq) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
