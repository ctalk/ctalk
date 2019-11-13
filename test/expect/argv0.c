/* $Id: argv0.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdio.h>

int main (char **argv, int argc) {

  String new cmdName;

  cmdName = argv[0];

  fprintf (stdout, "%s\n", cmdName);

  exit(0);
}
