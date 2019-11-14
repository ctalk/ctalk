/* $Id: cbrt3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_cuberoot, d_arg;
  Float new cubeRootObj;

  d_arg = 3.0;
  c_cuberoot = cbrt(d_arg);

  if ((cubeRootObj = cbrt (d_arg)) == c_cuberoot) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
