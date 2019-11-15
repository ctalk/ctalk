

Integer instanceMethod ifAdd (Point pt) {
  int x;
  int x_2;

  x = 10;
  x_2 = 6;

  if (++x + x_2 + pt y < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (--x + x_2 + pt y < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (~x + x_2 + pt y > 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (!x + x_2 + pt y > 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (-x + x_2 + pt y > 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + ++x_2 + pt y < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + --x_2 + pt y < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + ~x_2 + pt y > 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + !x_2 + pt y < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

  if (x + -x_2 + pt y > 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

}

int main () {

  Integer new myInt;
  Point new pt;

  pt y = 5;

  myInt ifAdd pt;
}
