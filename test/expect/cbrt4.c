/* $Id: cbrt4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_cuberoot;
  Float new cubeRootObj;
  Float new argObj;

  argObj = 3.0;
  c_cuberoot = cbrt(argObj);

  if ((cubeRootObj = cbrt (argObj)) == c_cuberoot) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
