


Point instanceMethod ifAdd (void) {
  Integer new x_2;
  List new l;
  int x;

  x = 10;
  x_2 = 6;
  l init "one";

  l map {
    if (x + x_2 - 10 == ++super y) {
      printf ("Pass: super y = %d\n", super y);
    } else {
      printf ("Fail\n");
    }
    if (x + x_2 - 11 == --super y) {
      printf ("Pass: super y = %d\n", super y);
    } else {
      printf ("Fail\n");
    }
    if (x + x_2 - 22 == ~super y) {
      printf ("Pass: ~super y = %d\n", ~super y);
    } else {
      printf ("Fail\n");
    }
    if (x + x_2 - 16 == !super y) {
      printf ("Pass: !super y = %d\n", !super y);
    } else {
      printf ("Fail\n");
    }
    if (x + x_2 - 21 == -super y) {
      printf ("Pass: -super y = %d\n", -super y);
    } else {
      printf ("Fail\n");
    }
  }
}

int main () {

  Point new pt;

  pt y = 5;

  pt ifAdd;
}
