/* $Id: getenv2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

int main () {

  char c_path[FILENAME_MAX];
  String new pathString;

  xstrcpy (c_path, getenv ("PATH"));

  if ((pathString = getenv ("PATH")) == c_path) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
