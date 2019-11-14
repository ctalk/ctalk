
#include <stdio.h>

Symbol new somePtr;

void *my_void_ptr_function (void) {
  return (void *)"Hello, world!\n";
}

int main () {
  char *t;

  somePtr = my_void_ptr_function ();

  /* This is the best we can do right now.  Check this in 
     another test program. */
  t = __ctalk_to_c_char_ptr (somePtr);

  
  printf ("%s\n", t);
}
