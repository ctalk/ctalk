
int ifAdd (void) {
  List new l;
  Point new pt;
  int x_2;

  pt y = 5;
  x_2 = 6;
  l = "one", "two", "three";

  l map {
    if (self length + x_2 + pt y < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass: self length = %d\n", self length);
    }

    if (self length + pt y + x_2 < 14) {
      printf ("Fail\n");
    } else {
      printf ("Pass: self length = %d\n", self length);
    }
  }

}

int main () {

  ifAdd ();
}
