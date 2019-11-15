
int main () {
  List new l;

  l init "first", "second", "third", "fourth";
  l append "fifth", "sixth", "seventh", "eigth";

  l map {
    printf ("%s\n", self);
  }
}
