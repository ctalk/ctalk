/* $Id: fprintf2.c,v 1.3 2019/11/28 17:01:57 rkiesling Exp $ */

#include <stdio.h>

int main () {

  Integer new result;
  char arg1[16];
  char arg2[16];
  FILE *f;

  if ((f = xfopen ("testoutput.fprintf2", "w")) == NULL) {
    printf ("Fail.\n");
    return 1;
  }

  xstrcpy (arg1, "s1");
  xstrcpy (arg2, "s2");

#if 0
  /***/ /* make sure that user templates with stdargs works 
      in ufntmp.c ... */
  if ((result = xfprintf_mine (f, "%s %s\n", arg1, arg2)) == ERROR) {
     printf ("fprintf failed.\n");
     return 1;
  }
#endif  
  if ((result = xfprintf (f, "%s %s\n", arg1, arg2)) == ERROR) {
     printf ("fprintf failed.\n");
     return 1;
  }

  fclose (f);

  exit(0);
}
