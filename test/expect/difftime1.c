/* $Id: difftime1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <time.h>

int main () {

  time_t t0, t1;
  long long int l;
  double time_diff;
  Float new timeDiffObj;

  t0 = time (NULL);
  for (l = 0; l <= 100000000; l++)
    ;
  t1 = time (NULL);

  time_diff = difftime (t1, t0);
  timeDiffObj = difftime (t1, t0);

  printf ("%f\n", time_diff);
  printf ("%f\n", timeDiffObj);

  if (timeDiffObj == time_diff) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
