/* $Id: round1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>
#include <math.h>

int main () {

  Float new myFloat;

  myFloat = round(1.5);

  if (myFloat == 2.0) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
