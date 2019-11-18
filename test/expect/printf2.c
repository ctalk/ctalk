/* $Id: printf2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <stdio.h>

int main () {

  Integer new result;
  char s1[16];
  char s2[16];

  xstrcpy (s1, "s1");
  xstrcpy (s2, "s2");

  if ((result = printf ("%s %s\n", s1, s2)) == ERROR) {
     printf ("printf failed.\n");
     return 1;
  }

  exit(0);
}
