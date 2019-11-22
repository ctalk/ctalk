
Object instanceMethod bgMethod (void) {
#ifndef __APPLE__
  self inspect "bgMethod> ";
#endif
  printf ("bgMethod printed this.\n");
  fflush (stdout);  /* output past any logging latencies. */
}


pid_t wait(int *status);  /* Prototype prevents a warning with */
                          /* some compilers.                   */

int main () {
  Integer new i;
  Method new bgMethodObject;
  int status;

  bgMethodObject definedInstanceMethod "Object", "bgMethod";
  i backgroundMethodObjectMessage bgMethodObject;

  wait (&status);  /* Returns when the background */
                   /* process exits.              */

}
