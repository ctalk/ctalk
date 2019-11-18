
char *myFunc (void) {
  return "myFunc (non)error message";
}

int main () {
  String new myString;
  char s[15];

  myString = "Message: ";
  xstrcpy (s, "hello!");
  printf ("%s\n", myString + myFunc () + ".");

}
