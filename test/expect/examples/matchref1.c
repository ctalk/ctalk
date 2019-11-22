
int main (argc, argv) {
  String new text, pattern, fn_name;
  List new fn_list;

  fn_list = "strlen ()", "strcat(char *)", "strncpy (char *)",
    "stat (char *, struct stat *)";

  /* Match the first non-label character: either a space or a
     parenthesis. */
  pattern = "( *)\\(";

  fn_list map {
    if (self =~ pattern) {
      printf ("Matched text: \"%s\" at index: %d\n",
	      self matchAt 0, self matchIndexAt 0);
      fn_name = self subString 0, self matchIndexAt 0;
      printf ("Function name: %s\n", fn_name);
    }
  }

  return 0;
}
