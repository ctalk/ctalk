
Integer instanceMethod myAdd (Point pt) {
  int x;
  int z;

  x = 10;
  z = 14;

  if (x + pt y + ++self < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self = %d\n", self);
  }

  if (x + pt y + --self < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self = %d\n", self);
  }

  if (x + pt y + ~self > z) {
    printf ("Fail\n");
  } else {
    printf ("Pass: ~self = %d\n", ~self);
  }

  if (x + pt y + !self < z) {
    printf ("Fail\n");
  } else {
    printf ("Pass: !self = %d\n", !self);
  }

  if (x + pt y + -self > z) {
    printf ("Fail\n");
  } else {
    printf ("Pass: -self = %d\n", -self);
  }

}

int main () {
  Integer new x_2;
  Point new pt;

  pt y = 5;
  x_2 = 6;

  x_2 myAdd pt;
}
