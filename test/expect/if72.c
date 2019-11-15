
String instanceMethod ifAdd (Integer x_2) {
  int x;

  x = 10;

  if (x + self length < x_2) {
    printf ("Fail\n");
  } else {
    printf ("Pass\n");
  }

}


int main () {

  String new s;
  Integer new z;

  s = "Hello, world!";
  z = 14;

  s ifAdd z;
}
