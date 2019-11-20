/* $Id: strchr1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  char *result, s1[255];

  xstrcpy (s1, "s1");

  result = strchr (s1, '1');

  if (!strcmp (result, "1")) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
