/* $Id: fabs4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  Float new myFloat;
  Float new floatParam;

  floatParam = -1.0;

  if ((myFloat = fabs (floatParam)) == 1.0) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
