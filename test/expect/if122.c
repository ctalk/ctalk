

Point instanceMethod ifAdd (void) {
  int x;
  Integer new x_2;
  List new l;

  x = 10;
  x_2 = 6;
  l = "one";

  l map {
    if (x + super y++ + x_2 == 21) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }

    if (x + super y-- + x_2 == 22) {
      printf ("Pass\n");
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
