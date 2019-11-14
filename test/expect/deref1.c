
int main () {

  Integer new i;
  Symbol new s;

  i = 1;

  /* This could use a better warning message - otherwise it gets a
     __ctalk_method () error at run time. */
  /* s = i; */
  *s = i;

  printf ("%d\n", i);
  printf ("%d\n", s deref);
}
