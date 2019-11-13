/* $Id: atof1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

int main () {

  Float new myFloat;

  myFloat = atof ("1.0");

  if (myFloat == 1.0) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
