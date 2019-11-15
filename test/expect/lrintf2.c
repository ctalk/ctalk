/* $Id: lrintf2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  Integer new myInt;

  if ((myInt = lrintf (1.1f)) == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
