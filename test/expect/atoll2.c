/* $Id: atoll2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

/*
 *  If testing LongIntegers, also test with the 'L' suffix.
 */

int main () {

  LongInteger new myLongInt;

#if defined(__APPLE__) || defined(__DJGPP__)
  if ((myLongInt = atol ("2l")) == 2l) {
#else
  if ((myLongInt = atoll ("2ll")) == 2ll) {
#endif
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
