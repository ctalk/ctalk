
int main () {

  Object new o;
  Symbol new s;

  *s = o addressOf;

  printf ("These two addresses should be identical.\n");
  printf ("%p\n", o addressOf);
  printf ("%p\n", s value);
}
