
Integer instanceMethod myStuff (Point pt) {
  int x, *x_2, **x_3;
  char s[3] = "1";
  char t[3] = "1";

  x = 6;
  x_2 = &x;
  x_3 = &x_2;

  if (s length + **x_3 + pt y == 12) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (**x_3 + s length + pt y < 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (**x_3 + pt y + s length == 12) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + **x_3 + s length + pt y == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + s length + **x_3 + pt y == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + pt y + s length + **x_3 == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + pt y + s length + **x_3 + t length == 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + pt y + t length + **x_3 + s length == 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
}


int main () {
  Point new pt;
  Integer new myInt;

  pt y = 5;
  myInt myStuff pt;
  
}
