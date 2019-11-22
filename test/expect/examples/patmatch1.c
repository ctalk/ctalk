
int main () {

  String new s;
  Integer new n_offsets;
  Integer new i;
  
  s = "Hello?";

  if (s =~ /(o\?)/) {
    printf ("match\n");
    i = 0;
    n_offsets = s nMatches;
    while (i < n_offsets) {
      printf ("%d: %s\n", s matchIndexAt i, s matchAt i);
      ++i;
    }
  }
}
