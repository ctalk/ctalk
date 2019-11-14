/* $Id: difftime3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <time.h>

int main () {

  long long int t0, t1;
  long long int l;
  double time_diff;
  Float new timeDiffObj;
  Integer new t0Obj;
  Integer new t1Obj;

  t0 = time (NULL);
  for (l = 0; l <= 100000000; l++)
    ;
  t1 = time (NULL);

  time_diff = difftime (t1, t0);

  t0Obj = t0;
  t1Obj = t1;

  if ((timeDiffObj = difftime (t1Obj, t0Obj)) == time_diff) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  printf ("%lf\n", time_diff);
  printf ("%lf\n", timeDiffObj);

  exit(0);
}
