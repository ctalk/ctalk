/* $Id: sqrt2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_squareroot;
  Float new squareRootObj;

  c_squareroot = sqrt(3.0);

  if ((squareRootObj = sqrt (3.0)) == c_squareroot) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
