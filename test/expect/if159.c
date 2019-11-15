

String instanceMethod ifAdd (void) {
  int x;
  Integer new x_2;

  x = 10;
  x_2 = 6;

  if (self length + x + x_2 < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

}

int main () {

  String new s;

  s = "Hello, world!";

  s ifAdd;
}
