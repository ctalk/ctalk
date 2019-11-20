
#include <stdio.h>

Symbol new somePtr;

void *my_void_ptr_function (void) {
  return (void *)"Hello, world!\n";
}

int main () {
  String new s;

  somePtr = my_void_ptr_function ();

  s = somePtr asString;

  printf ("%s\n", s);

  printf ("%s\n", somePtr asString);

  s = somePtr asAddrString;
  
  printf ("%s == %s\n", s, somePtr);

}
