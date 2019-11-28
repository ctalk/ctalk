/* $Id: fprintf1.c,v 1.3 2019/11/28 17:01:57 rkiesling Exp $ */

#include <math.h>

int main () {

  String new arg1;
  String new arg2;
  FILE *f;

  if ((f = xfopen ("testoutput.fprintf1", "w")) == NULL) {
    printf ("Fail.\n");
    return 1;
  }

  arg1 = "s1";
  arg2 = "s2";

  /* We want to check the direct function call here, and
     using an object for the return value causes Ctalk to
     use the xsprintf template ... we just want to check
     the basic function call here.  The template gets checked
     in fprintf2.c */
  xfprintf (f, "%s %s\n", arg1, arg2);

  fclose (f);

  exit(0);
}
