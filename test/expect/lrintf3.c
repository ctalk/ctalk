/* $Id: lrintf3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  Integer new myInt;
  int param;

  param = 1.1f;

  if ((myInt = lrintf (param)) == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
