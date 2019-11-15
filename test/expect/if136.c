


Point instanceMethod ifAdd (void) {
  List new l;
  int x_2;
  int x;

  x = 10;
  x_2 = 6;
  l = "one";

  l map {
    if (x++ + x_2 + super y < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass\n");
    }
    if (x-- + x_2 + super y < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass\n");
    }

    if (x + x_2++ + super y < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass\n");
    }
    if (x + x_2-- + super y < 14) {
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
