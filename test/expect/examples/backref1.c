
int main () {
  String new s;
  String new pat;
  Integer new n_matches;
  Array new offsets;
  Integer new i;

  s = "1.mobile 2mobile mobile";
  pat = "(\\d\\p)?m";
  
  n_matches = s matchRegex pat, offsets;
  
  for (i = 0; i < n_matches; ++i) {
    printf ("%Ld\n", offsets at i);
  }

  for (i = 0; i < n_matches; ++i) {
    if ((s matchAt i) length == 0) {
      printf ("%d: %s\n", s matchIndexAt i, "(null)");
    } else {
      printf ("%d: %s\n", s matchIndexAt i, s matchAt i);
    }
  }
}
