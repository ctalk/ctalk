/* $Id: atoll1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

/*
 *  If testing LongIntegers, also test with the 'L' suffix.
 */

int main () {

  LongInteger new myLongInt;

  myLongInt = atoll ("1L");

  if (myLongInt == 1ll) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
