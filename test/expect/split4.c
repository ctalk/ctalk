
int main () {
  Array new splitText;
  char text[] = "one two three four five six";

  text split ' ', splitText;

  splitText map {
    printf ("%s\n", self);
  }
}
