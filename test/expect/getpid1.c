/* $Id: getpid1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <sys/types.h>
#include <unistd.h>

int main () {

  Integer new pidInt;
  int pid;

  pid = getpid ();

  if ((pidInt = getpid ()) == pid) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
