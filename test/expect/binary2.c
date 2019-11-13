
#include <stdio.h>

int main () {
  Integer new i;
  String new s;
  Character new c;
  
  s = "Hello, world!";

  i = 0b111;

  c = s at i;

  printf ("%c\n", c);

  c = s at 0b111;

  printf ("%c\n", c);


  i = 111b;

  c = s at i;

  printf ("%c\n", c);

  c = s at 111b;

  printf ("%c\n", c);



}
