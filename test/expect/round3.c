/* $Id: round3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>
#include <math.h>

int main () {

  Float new myFloat;
  double param;

  param = 1.5f;

  if ((myFloat = round (param)) == 2.0f) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
