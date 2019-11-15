
Integer instanceMethod ifAdd (Point pt) {
  int x;

  x = 10;

  /* all of these seem to be correct.... */

  if (x + pt y < ++self) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self = %d\n", self);
  }

  if (x + pt y < --self) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self = %d\n", self);
  }

  if (x + pt y < ~self) {
    printf ("Fail\n");
  } else {
    printf ("Pass: ~self = %d\n", ~self);
  }

  if (x + pt y < !self) {
    printf ("Fail\n");
  } else {
    printf ("Pass: !self = %d\n", !self);
  }

  if (x + pt y < -self) {
    printf ("Fail\n");
  } else {
    printf ("Pass: -self = %d\n", -self);
  }

}


int main () {

  Point new pt;
  Integer new z;

  pt y = 5;
  z = 14;

  z ifAdd pt;
}
