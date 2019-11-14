
int main () {

  Point new p;
  Symbol new s;

  p x = 1;
  s = p x addressOf addressOf;
  printf ("%d\n", p x);
  printf ("%d\n", s deref deref);

}
