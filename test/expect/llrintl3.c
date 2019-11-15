/* $Id: llrintl3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  LongInteger new myLongInt;
  int param;

  param = 1.1f;

  if ((myLongInt = llrintl (param)) == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
