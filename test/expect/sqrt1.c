/* $Id: sqrt1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_squareroot, d_arg;
  Float new squareRootObj;

  d_arg = 3.0;
  squareRootObj = sqrt(3.0);
  c_squareroot = sqrt(3.0);

  if (squareRootObj == c_squareroot) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
