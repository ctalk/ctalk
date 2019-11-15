
void myStuff (int intArg) {
  Integer new x_2;
  char s[3] = "1";
  char t[3] = "1";

  x_2 = 6;

  if (s length == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + x_2 == 7) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (x_2 + s length == 7) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + x_2 + intArg == 12) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (x_2 + s length + intArg < 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (x_2 + intArg + s length == 12) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + x_2 + s length + intArg == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + s length + x_2 + intArg == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + intArg + s length + x_2 == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + intArg + s length + x_2 + t length == 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s length + intArg + t length + x_2 + s length == 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
}

int main () {
  int myInt = 5;
  myStuff (myInt);
}
