

#include <stdio.h>

int main () {
  String new argstring;
  String new resultstring;
  Integer new i;

  i enableExceptionTrace;

  ReadFileStream classInit;

  argstring = "";
  for (i = 0; i < 65535; i = i + 1) {
    argstring = argstring + (stdinStream readChar asString);
    printf ("%d ", i);
    fflush (stdout);
  }
}
