#include <stdlib.h>
int main () {
  printf ("Output should appear on ttyS1 (Linux) or ttya (SPARC) if "
	  "connected.\n");
  system ("echo -e Hello, world!\0033\0133\0101 | examples/ansiterminalstream3");
  printf ("\n");
}
