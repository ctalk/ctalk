
/* this tests C ++ prefix operators using native C.. */

int main () {
  Integer new myInt;
  int x1, x2;

  x1 = 1;
  x2 = 2;

  myInt = ++x1 + ++x2;
  printf ("%d + %d = %d\n", x1, x2, myInt);
  myInt = ++x2 + ++x1;
  printf ("%d + %d = %d\n", x1, x2, myInt);

}
