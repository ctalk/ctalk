/* $Id: sqrt4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_squareroot;
  Float new squareRootObj;
  Float new argObj;

  argObj = 3.0;
  c_squareroot = sqrt(argObj);

  if ((squareRootObj = sqrt (argObj)) == c_squareroot) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
