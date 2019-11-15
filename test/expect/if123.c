
Point instanceMethod myAdd (void) {

  List new l;
  int x;
  int z;

  x = 10;
  z = 21;

  l = "one";
  
  l map {
    if (x + 6 + super y++ == z) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }

    z = 22;

    if (x + 6 + super y-- == z) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  }

}

int main () {

  Point new pt;

  pt y = 5;

  pt myAdd;
}
