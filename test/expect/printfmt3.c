
/* Like printfmt2.c, but this should be more likely to segfault.
   Great if we get to the, "Pass," printf (). */

#include <stdio.h>

int main () {

  /* The formats are supposed to be incorrect here - don't fix! */
  printf ("- %%%d%% - abcdefghijklmnopqrstuvwxyz %% %s %%\n", 
	  "Hello, world!", 1);

  /* If we don't segfault, we pass. */
  printf ("Pass\n");

}
