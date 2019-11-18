
/* this is here for reference with the C variable writeback - this
   example, tho' can increment the C variable via C */

int main () {
  Integer new myInt;
  int x;

  x = 1;

  myInt = ++x;
  printf ("%d\n", myInt);
  myInt = ++x;
  printf ("%d\n", myInt);
  printf ("%d\n", x);

}
