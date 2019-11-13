
/* Should build without a warning. */

int main () {

  Array new myArray;
  int i;

  myArray atPut 0, 1;
  myArray atPut 1, 10;
  myArray atPut 2, 20;
  myArray atPut 3, 30;

  for (i = 0; i < 4; i++) {
    if ((myArray at i) > 10)
      printf ("%d\n", myArray at i);
  }

}
