/* $Id: cosh2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double d;
  Float new myFloat;

  d = cosh (12.0);

  if ((myFloat = cosh (12.0)) == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
