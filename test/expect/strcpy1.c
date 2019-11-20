/* $Id: strcpy1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  char *result, s1[255];

  result = xstrcpy (s1, "s1");
  if (!strcmp (result, s1)) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
