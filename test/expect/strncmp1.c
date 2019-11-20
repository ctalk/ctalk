/* $Id: strncmp1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  char s1[255], s2[255];
  int result;

  xstrcpy (s1, "s1");
  xstrcpy (s2, "s2");

  result = strncmp (s1, s2, 1);

  if (!result) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
