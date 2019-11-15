

void ifAdd (void) {
  List new l;
  Point new pt;
  struct {
    int mbr;
  } s;
  int x_2;

  pt y = 5;
  s.mbr = 4;
  x_2 = 6;
  l = "one";

  l map {
    if (s.mbr + x_2 + pt y < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass\n");
    }

    if (x_2 + s.mbr + pt y < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass\n");
    }
  }

}

int main () {

  ifAdd ();
}
