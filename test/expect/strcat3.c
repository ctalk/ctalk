/* $Id: strcat3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  String new result;
  String new s1;
  String new s2;

  s1 = "s1";
  s2 = "s2";

  if ((result = xstrcat (s1, s2)) == s1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
