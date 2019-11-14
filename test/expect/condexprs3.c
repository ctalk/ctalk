
int main () {
  Integer new n;
  Boolean new b;

  b = False;

  for (n = 0; n < 10; n++) {

    if ((n == 1) && !b) {
      printf ("Pass n = %d\n", n);
    }
  }

  b = True;
  for (n = 0; n < 10; n++) {

    if ((n == 1) && !b) {
      /* this shouldn't print. */
      printf ("Fail n = %d\n", n);
    }
  }
  printf ("---------------\n");

  b = False;

  for (n = 0; n < 10; n++) {

    if (!b && (n == 1)) {
      printf ("Pass n = %d\n", n);
    }
  }

  b = True;
  for (n = 0; n < 10; n++) {

    if (!b && (n == 1)) {
      /* this shouldn't print. */
      printf ("Fail n = %d\n", n);
    }
  }
  printf ("---------------\n");

}
