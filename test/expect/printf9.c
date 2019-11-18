
char *myFunc (void) {
  return "myFunc (non)error message";
}

int main () {
  printf ("%s\n", "Message: " + myFunc ());
}
