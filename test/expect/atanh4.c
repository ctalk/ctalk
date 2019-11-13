/* $Id: atanh4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double d, d_arg;
  Float new myFloat;
  Float new myArg;

  d_arg = 0.75;
  myArg = d_arg;
  d = atanh (d_arg);

  if ((myFloat = atanh (myArg)) == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
