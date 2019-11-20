/* $Id: sscanf2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/* 
 *   Note - sscanf shows the same issues with pointers as printf etc. 
 */

#include <stdio.h>

int main () {

  Integer new result;
  String new s1;
  String new s2;

  if ((result = sscanf ("s1 s2", "%s %s\n", s1, s2)) == ERROR) {
     printf ("sscanf failed result = %d.\n", result);
     return 1;
  }

/*   printf ("%s %s\n", s1, s2); */
  printf("%s ", s1);
  printf("%s\n", s2);

  exit(0);
}
