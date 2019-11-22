
int main () {

  Symbol new sym1;
  Symbol new sym2;
  Symbol new sym3;
  Integer new i;
  
  i = 2;

  sym3 = sym1;   /* Save the original value of sym1. */

  sym1 = i;
  printf ("%d\n", sym1);

  *sym2 = i;
  printf ("%d\n", *sym2);

  sym1 = sym3;  /* Restore sym1 to the original object. */

  i = 4;
  
  *sym1 = i;

  printf ("%d\n", *sym1);
}
