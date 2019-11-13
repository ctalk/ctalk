/* $Id: atol3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

/*
 *  Note that long ints map to Integers, so if changing
 *  this, check with, "l," and without suffix. 
 */

int main () {

  Integer new myInt;
  String new myArg;

  myArg = "21";

  if ((myInt = atol (myArg)) == 21) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
