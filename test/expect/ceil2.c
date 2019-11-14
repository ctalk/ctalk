/* $Id: ceil2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_ceil;
  Float new ceilObj;

  c_ceil = ceil (3.2);

  if ((ceilObj = ceil(3.2)) == c_ceil) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
