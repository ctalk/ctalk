/* $Id: acos1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double d;
  Float new myFloat;

  d = acos (0.5);
  myFloat = acos (0.5);

  if (myFloat == d) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
