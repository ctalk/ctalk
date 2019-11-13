/* $Id: atoll3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

/*
 *  If testing LongIntegers, also test with the 'L' suffix.
 */

int main () {

  LongInteger new myLongInt;
  String new myArg;

  myArg = "2ll";

#if defined(__APPLE__) || defined(__DJGPP__)
  if ((myLongInt = atol (myArg)) == 2l) {
#else
  if ((myLongInt = atoll (myArg)) == 2ll) {
#endif
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
