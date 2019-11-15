
/* */

int main () {
  Integer new i;
  Symbol new sym;

  i = 1024;

  /* Here also, a warning in the set value method would help, instead
     of waiting for an error from __ctalk_method ().
     Also try both in sequence and try to do something intelligent.
  */
  /* sym = i; */
  *sym = i;

  printf ("%d\n", i);
  printf ("%d\n", sym getValue);
  printf ("%d\n", *sym);

  return 0;
}
