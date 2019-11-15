
/* this should generate a few warnings. */

int main () {

  Point new pt;
  List new l;
  double x_2;
  double x[5] = {10.0, 11.0, 12.0, 13.0, 14.0};

  x_2 = 6.0;
  l = "one";
  pt y = 5;

  x[3] = 1.0;

  l map {
    if (x[4] + x_2 + pt y == 25.0) {
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
