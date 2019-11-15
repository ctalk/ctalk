
String instanceMethod printList (void) {
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

int main () {
  String new myString;
  myString printList;
  printf ("------------\n");
  myString printList;
}
