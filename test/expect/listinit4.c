
int main () {
  List new l;

  l = "first", "second", "third", "fourth";

  l map {
    printf ("%s\n", self);
  }

  l = "fifth", "sixth", "seventh", "eighth";
  l map {
    printf ("%s\n", self);
  }

}
