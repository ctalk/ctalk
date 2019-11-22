int main () {
  String new string;
  String new pattern;
  Array new offsets;
  Integer new nMatches;

  pattern = "l*o";
  string = "Hello, world! Hello, world, Hello, world!";
  
  nMatches = string search pattern, offsets;

  printf ("nMatches: %d\n", nMatches);
  offsets map {
    printf ("%d\n", self);
  }
}
