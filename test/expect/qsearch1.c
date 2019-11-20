
int main () {

  String new s;
  Integer new n_offsets;
  Array new offsets;
  String new pattern;
  Integer new idx;
  
  s = "Hello, woo";
  pattern = "woo";

  n_offsets = s quickSearch pattern, offsets;

  idx = offsets at 0;
  printf ("%d\n", idx);

  s = "Hello, woorld!";
  n_offsets = s quickSearch pattern, offsets;

  idx = offsets at 0;
  printf ("%d\n", idx);
}
