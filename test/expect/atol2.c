/* $Id: atol2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

int main () {

  Integer new myInt;

  if ((myInt = atoi ("2l")) == 2l) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
