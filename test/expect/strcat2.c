/* $Id: strcat2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <string.h>

int main () {

  char *result, s1[255], s2[255];

  xstrcpy (s1, "s1");
  xstrcpy (s2, "s2");

  if ((result = xstrcat (s1, s2)) == s1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
