/* $Id: strcmp1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  char s1[255];
  int result;

  xstrcpy (s1, "s1");

  result = strcmp (s1, s1);

  if (!result) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
