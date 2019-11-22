
int main () {
  String new s, pat;
  Array new matches;
  Integer new n_matches, n_th_match;

  pat = "-(mo)|(ho)use";

  s = "-mouse-house-";

  n_matches = s matchRegex pat, matches;

  for (n_th_match = 0; n_th_match < n_matches; ++n_th_match) {
    printf ("Match %d. Matched %s at character index %ld.\n",
	    n_th_match, s matchAt n_th_match, s matchIndexAt n_th_match);
  }

  matches delete;

}
