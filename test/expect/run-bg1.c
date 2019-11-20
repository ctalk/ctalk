/* this doesn't work correctly unless libctalk is built with
   GNU Readline support. */
#include <stdlib.h>
int main () {
#ifdef __APPLE__
  system ("examples/background1");
#else
  system ("/bin/echo \"c\" | examples/background1");
#endif

  printf ("\n");
}
