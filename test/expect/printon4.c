
char *myFunc (void) {
  return "myFunc error message";
}

int main () {
  String new s;

  s printOn "%s", "Error message: " + myFunc () + ".";

  printf ("%s\n", s);
}
