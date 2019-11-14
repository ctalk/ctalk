/* $Id: cbrt1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_cuberoot, d_arg;
  Float new cubeRootObj;

  d_arg = 3.0;
  cubeRootObj = cbrt(3.0);
  c_cuberoot = cbrt(3.0);

  if (cubeRootObj == c_cuberoot) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
