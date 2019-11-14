
#include <stdio.h>

String new someStr;

char *my_char_ptr_function (void) {
  return "Hello, world!\n";
}

int main () {

  someStr = my_char_ptr_function ();

  printf ("%s\n", someStr);

}
