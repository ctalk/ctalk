/* this checks for typecast expressions that are passed through
   by parser_pass without any processing. */

void printInt  (int i, int j) {
  printf ("%d %d\n", i, j);
}

int main () {
  Float new f;
  Integer new i;

  f = 5.1;
  i = 6;
  printInt (i, (int)f);
}
