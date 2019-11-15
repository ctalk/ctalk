/* $Id: getenv4.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  This should generate a warning similar to the following: 
 *
 *        From Object : addInstanceVariable
 *        From String : myGetEnv
 *        From <cfunc>
 *  /usr/local/include/ctalk/Object:0: Invalid operand: NULL argument in __ctalkAddInstanceVariable.
 */

#include <stdlib.h>

String instanceMethod myGetEnv (char *__envVar) {

  String new __envVarValue;

  __envVarValue = getenv (__envVar);
  if (!__envVarValue isNullValue) {
    self addInstanceVariable ("value", __envVarValue);
  } else {
    self addInstanceVariable ("value", NULLSTR);
  }

  return self;
}

int main () {
  String new pathString;

  /*
   *  Just about any receiver will do after the = sign, as long as
   *  it's a String object so we can recognize myGetEnv as a method.
   */

  if ((pathString =  pathString myGetEnv ("DUMMY")) == NULL) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
