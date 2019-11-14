/* $Id: erf2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

/*
 *  Test that erf () returns the same value for both C and Ctalk.
 */

int main () {

  Float new myFloat;
  double d;

  d = erf (-1);

  if ((myFloat = erf (-1)) == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
