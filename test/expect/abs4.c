/* $Id: abs4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

Integer instanceMethod myMethod (Integer intParam) {

  Integer new myInt;

  if ((myInt = abs (intParam)) == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

}

int main () {

  Integer new myInt;
  
  myInt myMethod -1;

  exit(0);
}
