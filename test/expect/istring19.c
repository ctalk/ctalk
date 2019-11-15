
String instanceMethod printIt (String t) {
  printf ("%s\n", t);
}

int main () {
  String new s, t;

  s = "Hello, world!";
  t = s;

  while (t++ != NULL) {
    s printIt t;
  }

}
