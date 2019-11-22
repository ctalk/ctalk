
int main () {
  String new string, pattern;
  Array new offsets;
  Integer new nMatches, i;

  pattern = "(l*o)";
  string = "Hello, world! Hello, world, Hello, world!";
  
  nMatches = string matchRegex pattern, offsets;

  printf ("nMatches: %d\n", nMatches);
  offsets map {
    printf ("%d\n", self);
  }
  for (i = 0; i < nMatches; ++i) {
    printf ("%s\n", string matchAt i);
  }
}
