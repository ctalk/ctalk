
Integer instanceMethod ifAdd (Point pt) {
  List new l;
  int x;

  x = 9;
  l = "one";

  l map {
    if (x + pt y == super++) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }

    x = 10;

    if (x + pt y == super--) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  }

}


int main () {

  Point new pt;
  Integer new z;

  pt y = 5;
  z = 14;

  z ifAdd pt;
}
