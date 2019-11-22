int main () {
  String new string;
  String new pattern;
  Array new offsets;
  Integer new nMatches;

  pattern = "He";
  string = "Hello, world! Hello, world, Hello, world!";
  
  nMatches = string quickSearch pattern, offsets;

  printf ("nMatches: %d\n", nMatches);
  offsets map {
    printf ("%d\n", self);
  }
}
