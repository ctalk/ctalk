/* $Id: strchr2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  String new s;
  String new result;

  s = "s1";

  if ((result = strchr (s, '1')) == "1") {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
