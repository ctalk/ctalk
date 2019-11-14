/* $Id: condexprs1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test cases for functions that have templates.  When
 *  adding cases, also add to condexprs2.c.
 */

int main () {

  String new valStr;
  String new envVar;

  envVar = "PATH";

  if ((valStr = getenv (envVar)) == NULL) {
    printf ("Fail.\n");
  } else {
    printf ("Pass.\n");
  }
  if (!getenv (envVar)) {
    printf ("Fail.\n");
  } else {
    printf ("Pass.\n");
  }
  if (getenv (envVar) == NULL) {
    printf ("Fail.\n");
  } else {
    printf ("Pass.\n");
  }

  envVar = "FAKEVAR";

  if ((valStr = getenv (envVar)) == NULL) {
    printf ("Pass.\n");
  } else {
    printf ("Fail.\n");
  }
  if (!getenv (envVar)) {
    printf ("Pass.\n");
  } else {
    printf ("Fail.\n");
  }
  if (getenv (envVar) == NULL) {
    printf ("Pass.\n");
  } else {
    printf ("Fail.\n");
  }

  exit(0);
}
