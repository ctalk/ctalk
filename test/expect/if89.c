
/* This and if86.c are similar, but we'll do it to keep all the
   sample sets (sort of) orthognal... */
Integer instanceMethod myAdd (Point pt) {
  List new l;
  int x;
  int z;

  x = 10;
  z = 14;
  l = "one", "two", "three", "four", "five";

  l map {
    if (x + pt y + super < z) {
      printf ("Fail\n");
    } else {
      printf ("Pass\n");
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
