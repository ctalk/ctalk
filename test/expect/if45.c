
Integer instanceMethod ifAdd (Point pt) {
  int x;
  List new l;

  l init "one", "two", "three", "four", "five";

  x = 10;

  l map {
    if (x + pt y < super) {
      printf ("Fail\n");
    } else {
      printf ("Pass: pt y = %d; super = %d\n", pt y, super);
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
