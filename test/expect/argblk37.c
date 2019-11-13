

int main () {
  short u_s;
  List new l;

  u_s = 3;
  l = "one", "two", "three";

  l map {
    printf ("%d\n", u_s++);
    u_s++;
  }
}
