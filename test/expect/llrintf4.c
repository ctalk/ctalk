/* $Id: llrintf4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

Float instanceMethod myMethod (Object floatParam) {

 LongInteger new myLongInt;

  if ((myLongInt = llrintf (floatParam)) == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

}

int main () {

  Float new myFloat;
  
  myFloat myMethod 1.1f;

  exit(0);
}