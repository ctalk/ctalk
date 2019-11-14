/* $Id: erfc2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

/*
 *  Test that erfc () returns the same value for both C and Ctalk.
 */

int main () {

  Float new myFloat;
  double d;

  d = erfc (1.0);

  if ((myFloat = erfc (1.0)) == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  printf ("%f\n", d);
  printf ("%f\n", myFloat);

  exit(0);
}
