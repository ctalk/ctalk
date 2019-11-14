/* $Id: ctime1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <time.h>

int main () {

  time_t t;
  char timebuf[255];
  String new timeStr;

  t = time (NULL);

  xstrcpy (timebuf, ctime (&t));
  timeStr = ctime (&t);

  printf ("%s\n", timebuf);
  printf ("%s\n", timeStr);

  if (timeStr == timebuf) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
