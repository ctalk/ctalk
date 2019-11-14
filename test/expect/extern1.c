/* this checks the function (not prototype) frobbing in 
   extern_declaration. */

extern void printInt  (int i, int j) {
  printf ("%d %d\n", i, j);
}

int main () {
  Float new f;
  Integer new i;

  f = 5.1;
  i = 6;
  printInt (i, (int)f);
}
