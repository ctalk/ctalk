/* $Id: getenv7.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  This should generate an exception or two similar to the following: 
 *
 * /usr/local/include/ctalk/List:0: Ambiguous operand: The expression,
 *        "myGetEnv ("DUMMY")," does not seem to have a receiver.
 * 
 *__ctalkGetInstanceVariable: result (Class Boolean) variable, "value," undefined.
  */

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

  if ((pathString =  myGetEnv ("DUMMY")) != NULL) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
