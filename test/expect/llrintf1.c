/* $Id: llrintf1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  LongInteger new myLongInt;

  myLongInt = llrintf(1.1f);

  if (myLongInt == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
