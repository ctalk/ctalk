/* $Id: printf1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdio.h>

int main () {

  String new arg1;
  String new arg2;
  int result;

  arg1 = "s1";
  arg2 = "s2";

  result = printf ("%s %s\n", arg1, arg2);

  exit(0);
}
