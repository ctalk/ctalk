/* $Id: fabs1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  Float new myFloat;

  myFloat = fabs(-1.0);

  if (myFloat == 1.0) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
