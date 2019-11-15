/* $Id: getenv5.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

String instanceMethod myGetEnv (char *__envVar) {

  String new __envVarValue;

  if ((__envVarValue = getenv (__envVar)) != NULL) {
    self addInstanceVariable ("value", __envVarValue);
  } else {
    self addInstanceVariable ("value", NULLSTR);
  }

  return self;
}

int main () {
  String new pathString;

  if (pathString envVarExists "PATH") {
    /*
     *  Just about any receiver will do after the = sign, as long as
     *  it's a String object so we can recognize myGetEnv as a method.
     */
    if ((pathString =  pathString myGetEnv ("PATH")) != NULL) {
      printf ("Pass\n");
    } else {
      printf ("Fail (1)\n");
    }
  } else {
    printf ("Fail (2)\n");
  }

  exit(0);
}
