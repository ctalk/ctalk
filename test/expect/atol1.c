/* $Id: atol1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

int main () {

  Integer new myInt;

  myInt = atol ("1l");

  if (myInt == 1l) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
