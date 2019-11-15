
int main () {

  int x;
  int z;
  int x_2;
  Point new pt;

  x = 10;
  pt y = 5;
  z = 14;
  x_2 = 6;

  if (++x + x_2 + pt y < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (--x + x_2 + pt y < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (~x + x_2 + pt y > z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (!x + x_2 + pt y > z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (-x + x_2 + pt y > z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + ++x_2 + pt y < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + --x_2 + pt y < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + ~x_2 + pt y > z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + !x_2 + pt y < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + -x_2 + pt y > z) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

}
