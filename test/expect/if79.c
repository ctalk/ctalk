
String instanceMethod myAdd (Integer x_2) {
  int x;

  x = 10;

  if (x + self length + x_2 < 14) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }
}

int main () {

  Integer new x_2;
  String new s;

  s = "Hello, world!";
  x_2 = 6;

  s myAdd x_2;

}
