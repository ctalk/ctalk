
Integer instanceMethod myAdd (Point pt) {
  int x;
  int z;
  List new l;

  x = 10;
  z = 14;

  l init "one", "two", "three", "four", "five";

  l map {
    if (x + pt y + super < z) {
      printf ("Fail\n");
    } else {
      printf ("Pass: super = %d\n", super);
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
