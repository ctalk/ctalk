/* $Id: sqrt3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_squareroot, d_arg;
  Float new squareRootObj;

  d_arg = 3.0;
  c_squareroot = sqrt(d_arg);

  if ((squareRootObj = sqrt (d_arg)) == c_squareroot) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
