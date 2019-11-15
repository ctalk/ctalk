
Integer instanceMethod myStuff (Point pt) {
  List new l;
  int x_2;
  char s[3] = "1";
  char t[3] = "1";

  x_2 = 6;
  l = "one";

  l map {
    if (s length + x_2 + pt y == 12) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    if (x_2 + s length + pt y < 14) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    if (x_2 + pt y + s length == 12) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    if (s length + x_2 + s length + pt y == 13) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    if (s length + s length + x_2 + pt y == 13) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    if (s length + pt y + s length + x_2 == 13) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    if (s length + pt y + s length + x_2 + t length == 14) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
    if (s length + pt y + t length + x_2 + s length == 14) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  }
}


int main () {
  Point new pt;
  Integer new myInt;

  pt y = 5;
  myInt myStuff pt;
  
}
