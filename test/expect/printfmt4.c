
/* this mainly checks the %ld printf format */

int main () {
  String new s, pat;
  Array new offsets;

  s = "Integer instanceMethod ++";

  pat = "Integer instanceMethod \\+\\+";

  s matchRegex pat, offsets;

  printf ("%ld\n", offsets at 0);
  printf ("%ld\n", offsets at 1);
}
