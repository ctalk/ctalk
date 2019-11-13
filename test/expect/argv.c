/* $Id: argv.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdio.h>

int main (char **argv, int argc) {

  Array new argArray;
  int i;

  for (i = 0; i < argc; i++) 
    argArray atPut i, argv[i];

  for (i = 0; i < argc; i++)
    printf ("%s ", argArray at i);

  printf ("\n");

  exit (0);
}
