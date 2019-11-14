/* $Id: ctime2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <time.h>

int main () {

  time_t t;
  char timebuf[255];
  String new timeStr;
  Integer new timeVal;

  t = time (NULL);
  timeVal = t;

  xstrcpy (timebuf, ctime (&t));

  if ((timeStr = ctime (timeVal)) == timebuf) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  printf ("%s\n", timebuf);
  printf ("%s\n", timeStr);

  exit(0);
}
