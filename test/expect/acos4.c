/* $Id: acos4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double d, d_arg;
  Float new myFloat;
  Float new myArg;

  d_arg = 0.5;
  myArg = d_arg;
  d = acos (d_arg);

  if ((myFloat = acos (myArg)) == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
