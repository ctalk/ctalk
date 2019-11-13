/* $Id: atoi1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

int main () {

  Integer new myInt;

  myInt = atoi ("1");

  if (myInt == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
