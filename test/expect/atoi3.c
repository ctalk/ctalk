/* $Id: atoi3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

int main () {

  Integer new myInt;
  String new myArg;

  myArg = "2";

  if ((myInt = atoi (myArg)) == 2) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
