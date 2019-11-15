
/* this should generate a few warnings. */

Point instanceMethod ifAdd (void) {
  List new l;
  long double x_2;
  long double x[5];

  x_2 = 6.0L;
  l = "one";

  x[4] = 14.0L;

  l map {
    if (x[4] + x_2 + super y == 25.0L) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    if (x[4] + x_2 + super y < 14L) {
      printf ("Fail\n");
    } else {
      printf ("Pass\n");
    }

  }

}

int main () {

  Point new pt;

  pt y = 5;

  pt ifAdd;
}
