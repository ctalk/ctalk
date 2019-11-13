/* $Id: atof2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdlib.h>

int main () {

  Float new myFloat;

  if ((myFloat = atof ("2.0")) == 2.0) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
