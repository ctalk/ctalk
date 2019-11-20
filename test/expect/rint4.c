/* $Id: rint4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>
#include <math.h>

Float instanceMethod myMethod (Float floatParam) {

  Float new myFloat;

  if ((myFloat = rint (floatParam)) == 2.0f) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

}

int main () {

  Float new myFloat;
  
  myFloat myMethod 1.5f;

  exit(0);
}
