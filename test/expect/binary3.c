/* returns the recevier as a _decimal_ string. (binary formatted
   strings are not part of the C standard). */
#include <stdio.h>

int main () {
  String new s;

  s = 0b111 asString;

  printf ("%s\n", s);

  s = 0b111 asString;

  printf ("%s\n", s);
}
