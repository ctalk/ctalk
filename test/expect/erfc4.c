/* $Id: erfc4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

/*
 *  Test that erfc () returns the same value for both C and Ctalk.
 */

int main () {

  Float new myFloat;
  Float new myArg;
  double d;

  myArg = -1;
  d = erfc (myArg);

  if ((myFloat = erfc (myArg)) == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
