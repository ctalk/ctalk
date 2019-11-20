
Symbol instanceMethod printVal (void) {
  Integer new iLocal;
  iLocal = self getValue;
  printf ("%d\n", iLocal);
}

int main () {
  Symbol new s;
  Integer new i;
  
  i = 2;
  /* Try to generate a meaningful warning for a statement like
     s = i. */
  *s = i;
  s printVal;
}
