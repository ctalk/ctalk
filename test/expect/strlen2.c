/* $Id: strlen2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  Integer new length;

  if ((length = strlen ("Test string.")) == 12) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
