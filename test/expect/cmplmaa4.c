/* pretend we're calculating  where to insert a line break. */
String instanceMethod lineBreak (void) {
  Integer new remaining;
  Integer new receiverLength;

  remaining = 10;
  receiverLength = self length;

  printf ("%d\n", self length);
  printf ("%d\n", receiverLength);
  printf ("%d\n", (self length) - remaining);
  printf ("%d\n", self length - remaining);
  printf ("%d\n", receiverLength - remaining);
}

int main (int argc, char *argv[]) {
  String new str;

  str = "This is text, and this is yet more text.";
  str lineBreak;
}
