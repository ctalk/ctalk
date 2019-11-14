/* $Id: erfc1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

/*
 *  Test that erfc () returns the same value for both C and Ctalk.
 */

int main () {

  Float new myFloat;
  double d;

  myFloat = erfc(-1);
  d = erfc (-1);

  if (myFloat == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
