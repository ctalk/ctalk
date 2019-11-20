#include <stdlib.h>
int main () {
  system ("echo -e Hello, world!\0032 | examples/ansiterminalstream1");
  printf ("\n");
}
