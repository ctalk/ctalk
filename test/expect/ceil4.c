/* $Id: ceil4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_ceil;
  Float new ceilObj;
  Float new argObj;

  argObj = 3.2;
  c_ceil = ceil (argObj);

  if ((ceilObj = ceil(argObj)) == c_ceil) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}