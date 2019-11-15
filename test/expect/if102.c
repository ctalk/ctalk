

String instanceMethod ifAdd (void) {
  int x;
  Integer new x_2;

  x = 10;
  x_2 = 6;

  if (x + ++self length + x_2 < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self length = %d\n", self length);
  }

  if (x + --self length + x_2 < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self length = %d\n", self length);
  }

  if (x + ~self length + x_2 > 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self length = %d\n", ~self length);
  }

  if (x + !self length + x_2 < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self length = %d\n", !self length);
  }

  if (x + -self length + x_2 > 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass: self length = %d\n", -self length);
  }

}

int main () {

  String new s;

  s = "Hello, world!";

  s ifAdd;
}
