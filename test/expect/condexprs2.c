/* $Id: condexprs2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test cases for functions that do not have templates.  When
 *  adding cases, also add to condexprs1.c.
 */

char *myGetEnv (char *s) {
  return getenv(s);
}

int main () {

  String new valStr;
  String new envVar;
  char e[1024];

  envVar = "PATH";
  xstrcpy (e, "PATH");

  if (!myGetEnv (envVar)) {
    printf ("Fail.\n");
  } else {
    printf ("Pass.\n");
  }
  if (myGetEnv (envVar) == NULL) {
    printf ("Fail.\n");
  } else {
    printf ("Pass.\n");
  }
  if ((valStr = myGetEnv (e)) == NULL) {
    printf ("Fail.\n");
  } else {
    printf ("Pass.\n");
  }

  envVar = "FAKEVAR";

  if (!myGetEnv (envVar)) {
    printf ("Pass.\n");
  } else {
    printf ("Fail.\n");
  }
  if (myGetEnv (envVar) == NULL) {
    printf ("Pass.\n");
  } else {
    printf ("Fail.\n");
  }
  if ((valStr = myGetEnv (envVar)) == NULL) {
    printf ("Pass.\n");
  } else {
    printf ("Fail.\n");
  }

  exit(0);
}
