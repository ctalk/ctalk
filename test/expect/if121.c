
Integer instanceMethod myAdd (Point pt) {
  List new l;
  int x;
  int z;

  x = 3;
  z = 14;
  l = "one";

  l map {
    if (x + pt y + super++ == z) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }

    z = 15;

    if (x + pt y + super-- == z) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  }

}

int main () {
  Integer new x_2;
  Point new pt;

  pt y = 5;
  x_2 = 6;

  x_2 myAdd pt;
}
