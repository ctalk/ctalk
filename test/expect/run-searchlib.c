/* This example uses both the uninstalled libdoc file in the 
   doc subdirectory and the installed version, so this test
   only works after Ctalk has been installed at least once. */
#include <stdlib.h>
int main () {
  system ("examples/searchlib -f ../../doc/libdoc __ctalkGLUT.*");
  printf ("\n");
  system ("examples/searchlib __ctalkToC.*");
  printf ("\n");
}
