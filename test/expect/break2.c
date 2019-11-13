
int main () {
  List new l;
  l push "0";
  l push "1";
  l push "2";
  l push "3";

  l map {
    if (self value == "2")
      break;
    printf ("%s\n", self value);
  }
}
