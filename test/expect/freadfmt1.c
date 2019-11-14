/* $Id: freadfmt1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  The arguments should still be treated as read-only.
 */

#include <stdio.h>

int main () {

  ReadFileStream new inputFile;
  String new arg1;
  String new arg2;

  inputFile openOn "testinput";

  inputFile readFormat "%s %s\n", arg1, arg2;

  printf ("%s ", arg1);
  printf ("%s\n",arg2);

  exit(0);
}
