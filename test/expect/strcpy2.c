/* $Id: strcpy2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  String new s;
  String new result;

  if ((result = xstrcpy (s, "s1")) == s) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
