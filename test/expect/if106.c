


Point instanceMethod ifAdd (void) {
  Integer new x_2;
  List new l;
  int x;

  x = 10;
  x_2 = 6;
  l init "one";

  l map {
    if (x + ++super y + x_2 < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass: super y = %d\n", super y);
    }
    if (x + --super y + x_2 < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass: super y = %d\n", super y);
    }
    if (x + ~super y + x_2 > 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass: ~super y = %d\n", ~super y);
    }
    if (x + !super y + x_2 < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass: !super y = %d\n", !super y);
    }
    if (x + -super y + x_2 > 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass: -super y = %d\n", -super y);
    }
  }

}

int main () {

  Point new pt;

  pt y = 5;

  pt ifAdd;
}
