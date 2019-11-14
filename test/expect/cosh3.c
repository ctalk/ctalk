/* $Id: cosh3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double d, d_arg;
  Float new myFloat;

  d_arg = 12.0;
  d = cosh (d_arg);

  if ((myFloat = cosh (d_arg)) == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
