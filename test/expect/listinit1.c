
int main () {
  List new l;

  l init "first", "second", "third", "fourth";

  l map {
    printf ("%s\n", self);
  }
}
