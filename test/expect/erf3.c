/* $Id: erf3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

/*
 *  Test that erf () returns the same value for both C and Ctalk.
 */

int main () {

  Float new myFloat;
  double d, d_arg;

  d_arg = -1;
  d = erf (d_arg);

  if ((myFloat = erf (d_arg)) == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
