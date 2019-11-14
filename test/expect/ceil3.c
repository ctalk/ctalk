/* $Id: ceil3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_ceil, d_arg;
  Float new ceilObj;

  d_arg = 3.2;
  c_ceil = ceil (d_arg);

  if ((ceilObj = ceil(d_arg)) == c_ceil) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
