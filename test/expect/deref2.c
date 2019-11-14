
int main () {

  Integer new i;
  Symbol new s;

  i = 1;

  /* Make sure this gets a correct warning if it doesn't return
     a Symbol... and it gets handled the same as s = &i; */
  s = i addressOf;

  printf ("%d\n", i);
  printf ("%d\n", s deref);

}
