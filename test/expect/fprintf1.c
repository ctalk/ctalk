/* $Id: fprintf1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  Integer new result;
  String new arg1;
  String new arg2;
  FILE *f;

  if ((f = fopen ("testoutput.fprintf1", "w")) == NULL) {
    printf ("Fail.\n");
    return 1;
  }

  arg1 = "s1";
  arg2 = "s2";

/*   result = fprintf (f, "%s %s\n", arg1, arg2); */
  /*
   *  Work around some undefined interaction with fprintf.
   */
  fprintf (f, "%s ", arg1);
  fprintf (f, "%s\n", arg2);

  fclose (f);

  exit(0);
}
